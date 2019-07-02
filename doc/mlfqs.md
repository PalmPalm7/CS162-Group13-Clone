## part 3 Multi-lvel Feedback Queue Schedule

### Data structures and functions
```
static struct thread * next_thread_to_run(void); //change ready list and enable mlfqs
static struct thread * running_thread(); //change ready list and enable mlfqs
void thread_init(void) //change ready list
void thread_unblock(struct thread *t) //add it into previous thread_lists
void thread_block(struct thread *t) // add it into blocked_list
void thread_yield(void)// add to the end of thread_lists
static struct thread * fetch_thread()// the next_thread_to_run should call that, scan through the thread list 
static void init_thread(struct thread*t, chonst char *name, int priority);
void thread_schedule_tail(struct thread *prev);// reset the priority
int thread_get_nice(void)
void thread_set_nice(int new_nice)
int thread_get_recent_cpu(void)
int thread_get_load_avg(void)

```
### Algorithms
- Block any interrupt
  First we block any interrupt for the scheduler to help it schedule successfully

- Choosing the next thread to run
  We do a scan through all the queue, from the highest priority to the lowest one,
  as long as we found the one with some thread in it, we do round robin in that queue.

- Changing thread`s priority
  if time tick has done, and the thread has not yet completed , we push the thread
  into a lower level


### Synchronization

- when the kernel called the scheduler, we block any interrupt for the scheduler.
- when a threads asked for a lock which cannot be satisfied, we blocked it and put it 
into the waiters lists.

### Rationale
