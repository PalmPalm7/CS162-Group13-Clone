#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/timer.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of processes in a sleeping state from timer_sleep(). */
static struct list sleep_list;


/* List of process that currently own a lock */
// static struct list lock_list;


/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;


static struct list_elem* one_elem;
/* Stack frame for kernel_thread(). */
struct kernel_thread_frame
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

static int load_avg; /* load avg for mlfqs*/
static int64_t mlfqs_ticks;

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */

   /* Modified to initialize lock_list*/
void
thread_init (void)
{
  ASSERT (intr_get_level () == INTR_OFF);

//   list_init (&lock_list);
  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);
  list_init (&sleep_list);
  

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
 
  /*initialize load average*/
  if(thread_mlfqs){
    load_avg = 0;
    mlfqs_ticks = timer_ticks();
  }

}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void)
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);
  
  load_avg = 0;
  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);

}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void)
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;
  if(thread_mlfqs){
    //timer_calibrate();
 /* for mlfqs*/ 
    t->recent_cpu += 100;
    /*calculate each second*/
    if(timer_ticks() % (int64_t)100 == 0){
      fixed_point_t former_load_avg = fix_int(load_avg); /*get former load average*/
      former_load_avg = fix_unscale(former_load_avg, 100); /* divide by 100 */
      fixed_point_t new_load_avg = fix_add(fix_mul(fix_frac(59 , 60) , former_load_avg),
                                           fix_scale(fix_frac(1 , 60) , list_size(&ready_list))); /*calculate by formular*/
      new_load_avg = fix_scale(new_load_avg, 100); /* multiple by 100*/ 
      //load_avg = fix_round(new_load_avg); /*truncate to integer and store in global variables*/
      load_avg = kernel_ticks;
    }
  }

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();

}

/* Prints thread statistics. */
void
thread_print_stats (void)
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux)
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;
  

  /* Initiate the member variables for priority donation */
//   t->lock_own = 0;
//   t->orginal_priority = t->priority;
//   t->donation.count = 0;
//   int i;
//   for(i = 0; i < MAX_DONATION_NUM; i++)
//   {
//     t->donation.priority_donation_slots[i].priority_donation = -1;
//     t->donation.priority_donation_slots[i].sema = NULL;
//   }
  
  /* Add to run queue. */
  thread_unblock (t);

  /* If the thread create a thread with bigger priority yield the CPU */
  /* Newly added*/
  if(thread_current()->priority < t->priority)
    thread_yield();
  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void)
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t)
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  list_push_back (&ready_list, &t->elem);
  t->status = THREAD_READY;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void)
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void)
{
  struct thread *t = running_thread ();

  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void)
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void)
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void)
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;

  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread)
    list_push_back (&ready_list, &cur->elem);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

// void thread_sema_foreach(thread_action_func *func,void *aux)
// {
//   struct list_elem *e;
//   ASSERT (intr_get_level () == INTR_OFF);
//   intr_disable();
//   printf("ADDED:%p\n",one_elem);
//   printf("HEAD ADDRESS:%p\n",list_head (&lock_list));
//   printf("FIRST ELEMENT:%p\n",list_begin (&lock_list));
//   printf("FIRST ELEMENT's NEXT:%p\n",list_begin (&lock_list)->next);
//   list_begin (&lock_list)->next = list_end (&lock_list);
//   printf("FIRST ELEMENT's NEXT:%p\n",list_begin (&lock_list)->next);
//   printf("FIRST ELEMENT's NEXT:%p\n",list_begin (&lock_list)->next);
//   printf("FIRST ELEMENT's PREV:%p\n",list_begin (&lock_list)->prev);
//   printf("TAIL ADDRESS:%p\n",list_end (&lock_list));
//   printf("TAIL ADDRESS's previous:%p\n",list_end (&lock_list)->prev);
//   printf("%p\n",list_end (&lock_list)->next);
//   //e = list_begin (&lock_list);
//   printf("FINAL CHECK:%p\n",list_begin (&lock_list)->next);

//   for (e = list_begin (&lock_list); e != list_end (&lock_list);
//        e = list_next (e))
//     {
//       printf("%p\n",e);
//       printf("loop\n");
//       struct thread *t = list_entry (e, struct thread, allelem);
//       func (t, aux);
//     }
// }


// void thread_lock_list_add(struct list_elem *elem)
// {
//   one_elem = elem;
//   list_push_back(&lock_list,elem);
// }




/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority)
{
  thread_current ()->priority = new_priority;
  thread_yield();
}



/* In this function we need to implement 3 checks
    1.if new_priority bigger than t->priority set t->priority as new_priority
    2.when function returns check if thread_priority_donation_tem bigger than t->priority 
      if so t->priority = thread_priority_donation_tem else do noting
    3.check if t->own_lock == 0 if so restore the priority to orginal_priority
  */




/* check thread t's priority donation slots find if any 
    slot's sema got the semaphore if it find one set it to current priority and
    return non -1  */
// int 
// priority_donation_check_and_set (struct thread *t, struct semaphore *sema,int current_priority)
// {
//   int i=0;
//   for (i = 0; i < t->donation.count; i++)
//   {
//     if (t->donation.priority_donation_slots[i].sema == sema)
//     {
//       if (t->donation.priority_donation_slots[i].priority_donation < current_priority)
//       {
//         t->donation.priority_donation_slots[i].priority_donation = current_priority;
//         priority_donation_selfcheck (t);
//       }
//       return t->donation.priority_donation_slots[i].priority_donation;
//     }
//     return -1;
//   }
// }
/* maintain the property of priority donation slot which is if own_lock is not zero
  the priority always equal the biggest one in priority_donation_slot .  */

// void 
// priority_donation_selfcheck (struct thread *t)
// {
//   int max = 0;
//   int max_index = 0;
//   int j,i;
//   for(j = t->donation.count; j < MAX_DONATION_NUM; j++)
//   {
//     t->donation.priority_donation_slots[j].priority_donation = -1;
//     t->donation.priority_donation_slots[j].sema = NULL;
//   }


//   for(i = 0; i < t->donation.count; i++)
//   {
//     if (t->donation.priority_donation_slots[i].priority_donation > max)
//     {
//       max = t->donation.priority_donation_slots[i].priority_donation;
//       max_index = i;
//     }
//   }
//   t->priority = max;
// }

// /* */
// void priority_donation_release(struct thread *t,struct semaphore *sema)
// {
//   int i,j;
//   for (i = 0; i < t->donation.count; i++)
//   {
//       if(t->donation.priority_donation_slots[i].sema == sema){
//         for(j = i; j < t->donation.count - 1; j++)
//         {
//           t->donation.priority_donation_slots[j].priority_donation = t->donation.priority_donation_slots[j+1].priority_donation;
//           t->donation.priority_donation_slots[j].sema = t->donation.priority_donation_slots[j+1].sema;
//         }
//         t->donation.count--;
        
//       }
//   }
//   priority_donation_selfcheck(t);
// }


// int
// thread_lock_list_empty(void)
// {
//   return list_empty(&lock_list);
// }



/* Returns the current thread's priority. */
int
thread_get_priority (void)
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice)
{
  running_thread()->nice_value = nice; 
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void)
{
  return running_thread()->nice_value;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void)
{
//  if(timer_ticks() % (int64_t)100 == 0){
//    fixed_point_t former_load_avg = fix_int(load_avg); /*get former load average*/
//    former_load_avg = fix_unscale(former_load_avg, 100); /* divide by 100 */
//    fixed_point_t new_load_avg = fix_add(fix_mul(fix_frac(59 , 60) , former_load_avg),
//                                         fix_scale(fix_frac(1 , 60) , list_size(&ready_list))); /*calculate by formular*/
//    new_load_avg = fix_scale(new_load_avg, 100); /* multiple by 100*/ 
//    load_avg = fix_round(new_load_avg); /*truncate to integer and store in global variables*/
//   // load_avg = kernel_ticks;
//  }
//
  return load_avg;
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void)
{
  return running_thread()->recent_cpu;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED)
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;)
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux)
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void)
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->magic = THREAD_MAGIC;
 
  /*set values for scheduling*/
  t->nice_value = 0;
  t->recent_cpu = 0;

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size)
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
// static struct thread *
// next_thread_to_run (void)
// {
//   if (list_empty (&ready_list))
//     return idle_thread;
//   else
//     return list_entry (list_pop_front (&ready_list), struct thread, elem);
// }







static struct thread *
next_thread_to_run (void)
{
  struct list_elem *e;
  struct list_elem *r;
  struct thread *max;
  struct thread *t;
  enum intr_level old_level;

  if (list_empty (&ready_list))
    return idle_thread;
  else
  {
    old_level = intr_disable ();
    max = list_entry (list_begin (&ready_list), struct thread, elem);
    r = list_begin (&ready_list);
    for(e = list_begin (&ready_list);e != list_end (&ready_list);
        e = list_next (e))
      {
        t = list_entry (e, struct thread, elem);
        if (t->priority > max->priority)
        {
          max = t;
          r = e;
        }
      }
    list_remove(r);
    intr_set_level (old_level);
  }
  return max;
}


struct list_elem *
pop_out_max_priority_thread(struct list *thread_list)
{
  ASSERT(!list_empty(thread_list));
  struct list_elem *e;
  struct list_elem *r;
  struct thread *t;
  struct thread *max;
  enum intr_level old_level;
  old_level = intr_disable ();
  max = list_entry (list_begin (thread_list), struct thread, elem);
  r = list_begin (thread_list);
  for(e = list_begin (thread_list);e != list_end (thread_list);
      e = list_next (e))
    {
      t = list_entry (e, struct thread, elem);
      if (t->priority > max->priority)
      {
        max = t;
        r = e;
      }
    }
  list_remove(r);
  intr_set_level (old_level);
  return r;
}



/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();

  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread)
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void)
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void)
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Returns True if thread_1 has an earlier wake time than thread_2 and false otherwise. */
static bool
wake_time_comp (const struct list_elem *a, const struct list_elem *b, void *aux) 
{
   struct thread *thread_1 = list_entry (a, struct thread, elem);
   struct thread *thread_2 = list_entry (b, struct thread, elem);
   return (thread_2->wake_time >= thread_1->wake_time);
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
