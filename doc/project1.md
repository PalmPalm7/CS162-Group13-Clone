Design Document for Project 1: Threads
======================================

## Group Members

* Zuxin Li <lizx2019@berkeley.edu>
* Joshua Alexander <josh.alexander1315@berkeley.edu>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>

## Task 1: Efficient Alarm Clock
### Data Structures/Functions
No additional data structure is necessary for the new `timer_sleep()` to be implemented.  The necessary functions to be added will be 
```
thread_current()->status = THREAD_BLOCKED 
```
when the thread is put to sleep and 
```
ASSERT (t->status == THREAD_BLOCKED);
t->status = THREAD_READY;
list_push_back (&ready_list, &t->elem);
```
For the thread t to wake it up after 
```
ticks * TIMER_FREQ --realtime >= timer_elapsed(start);
```
### Algorithms
The thread will be changed to the blocked state when `timer_sleep()` is called with a positive argument.  Once the correct amount of real-time has passed, it will be changed from blocked back to ready and added to the ready list.  If `timer_sleep()` is called with a negative or zero argument, nothing will happen.

### Synchronization
None of the current synchronization will be modified by the change in `timer_sleep()`.  Only the current thread will be put to sleep and will not go back to the ready state until after the timer is up.  Since no synchronization items will be modified, it is safe to assume that the existing synchronization tactics are sufficient.

### Rationale
This design is an improvement upon the existing design since there is no busy waiting involved.  The thread simply goes to the existing blocked state throughout the duration of the given timer, then switches back to the read state afterwards. This solution was chosen since it has minimal synchronization modifications and only added existing code from thread.c to the `timer_sleep()` function.

## Task 2: Priority Scheduler
### Data structures and functions
#### 1.Adding data structure
Add data structure  `queue` and `queue_element`.

```
struct queue_elem{
    struct queue_elem* left_child;
    struct queue_elem* right_child;
    struct queue_elem* parent;
    int priority;
    int original_priority;//it is same with priority before any priority donation
    int internal_donation;//This member is added to avoid starvation. 
    When a thread is blocked or waiting for some resources for too long,it increases.
    int own_lock;//record current thread own how many locks if it's 0 priority should set to original priority
};
g
```
```
struct queue
{
    struct queue_elem root;//The head of queue
    int count; //count on number of threads in the queue
};
```
Using this priority queue(maxheap indeed) replace the list in `struct thread`.

#### 2.Adding functions
```
#define queue_entry(QUEUE_ELEM, STRUCT, MEMBER)//it is a MACRO to mimic the list_entry,which is returning thread pointer from a queue_elem
struct queue_elem *queue_root (struct queue* queue);//return first element of a priority queue
struct queue_elem *queue_pop (struct queue* queue);//pop out the first element of a priority queue
struct queue_elem *queue_insert(struct queue* queue);//insert the element into the queue and reorder this priority queue to maintain the property of a heap
int queue_size(struct queue* queue);//return the size a priority queue
```


### Algorithms
#### 1.Choosing the next thread to run
Currently the strategy pintos used  to chose a next thread to run is by calling a function `list_pop_front` which is just pop a element from a ready list.

However we change the **ready list** to aforementioned **priority queue** to store the ready threads. And we change the function`list_pop_front` to `queue_pop` which will pop out an element with higgest priority if priority queue is not empty or return *NULL* if it is empty.

#### 2.Acquiring and releasing a Lock
In fact, the implementations of `lock_acquire` ,`lock_try_acquire` and `lock_release` is just calling the functions `sema_down`,`sema_try_down`and`sema_up`.So we just to re-implement these three functions.

`sema_up` is pretty simple, basically it adds one to the value `semaphore.value` and if the value greater than zero it  calls a function `queue_pop` modified by ours to pop out a thread with highest priority and unblock it 

So we foucs on `sema_down` and `sema_try_down`  in two different circumstances.

- For a interrupt handler: if program want to acquire a lock,it should call the `sema_try_down` which  if `semaphore.value` equals 0 it returns false else it substract one from `semaphore.value` then returns ture.If the functions returns ture ,the interrupt handlers would konw it have already acquire a lock and execute next code or if it returns false, the handlers would abort because the interrupt handlers couldn't sleep and they couldn't be added into a waiting list.

- For a normal thread:if a normal thread wants to acquire a lock,it should call the `sema_down` which first subtract one from  `semaphore.value` and if it equals 0 the current thead will be added to waiting priority queue and call `thread_block` to unblock itself and find next thread to run.

#### 4.Computing the effective priority
The effctive priority consist of mainly two parts.

- original priority which is set when thread created
- internal donation: When a low original priority thread wait  lock/semaphore for too long, it increase to avoid starvation. The increasing strategy we exploited will be demostrated in the part 5.(Default value:0)

effetive priority = original priority + internal donation

#### 5.Priority scheduling for semaphores and locks
Just like I mentioned, the priority scheduling for locks depends on semaphores.So we just focus on the strategy of scheduling for semaphores and we just mentions how `sema_down`,`sema_try_down`and`sema_up` three funtions works in two different situations ,so we put our attention on the  increasing strategy of `internal donation`.

When `sema_up` is called and if the waiting priority queue has some members, the thread with highest priority would be pop out.However threads with low priority have little chances to acquire locks/semaphores and starvation happens.To avoid this, every time after the `queue_pop` is called, the value of `internal donation` in **queue_element** of waiting priority queue *plus one*.

#### 6.Priority scheduling for condition variables
In pintos, condition variables combine semaphores and locks ,it maps one lock to multiple condition variables and it uses semaphores which are initialized to 0 to materialize a waiting list.
Specificly,if one thread wants to wait for a condition variables it calls function `cond_wait` to block itself  put itself to waiting list and find next thread to run.
And if a condtion variable is ready,current thread could calls functions `cond_signal` to unblock the thead in waiting list.

#### 7.Changing thread's priority
In changing the thread's priority, we modify `int priority` in the thread structure. In the case of a priority donation, whereas a lock is acquired by a low priority thread while a high priority thread is also on the `ready_list`. We should perform a priority donation.
Get the both the high priority thread and low priority thread's priority level by calling `int thread_get_priority (void)` and save on `int temp_priority_high` and `int temp_priority_low` respectively then set the low priority to `temp_priority_high` by calling `void thread_set_priority (int temp_priority_high)` after the lock is released, change back its priority level using `void thread_set_priority (int temp_priority_low)`

#### 8.priority queue
Modify the list structure that originially calls struct list from lib/kernel/list.c to a priority queue that could be created from the doubly linked list structure that is implemented in lib/kernel/list.c.
Time complexity of doubly-linked list is Θ(n) for access Θ(n) for search while we are using priority queue to pop the highest priority in each iteration, so it will have time complexity of Θ(1) for access if no speciic thread is called.




### Synchronization
#### 1.Multiple Threads Access same lock and  semaphore
When a thread  is acquiring a lock/semaphore,it will disable interrupt  utill this  thread successfully get a lock or put itself to the waiting list,which means acquiring a lock/semaphore is atomic and there is no thread switching  when a thread is trying to acquire a lock/semaphore.So we could say acquiring lock and semaphore is thread-safe.

#### 2.Accessing shared variable
There is two possible circumstances when different Threads access shared variable.
 
 - A  normal thread  **A** is accessing a shared variable and we could use a lock or a semaphore to synchronize this shared variable if necessary.And when another thread **B** preempts, it encounter a lock and it will put itself in waiting list and block itself,choosing next thread to run.
 - A  normal thread  **A** is accessing a shared variable and we could use a lock or a semaphore to synchronize this shared variable if necessary.However if an interrupt happens some interrupt handlers run and when it encouters a lock  they will try acquire a lock if they successful acquire a lock handlers continue ,if they fails the handlers won't put themselves to waiting list and they maybe continue to run without accessing the variable or abort.


#### 3.List and other data structure
List and other data structure may not be thread sate in pintos, so sometimes we could use lock to restrict mutiple threads modifying same pointers simultaneous.
 
#### 4.calling functions
When two threas call a same function if they do not access shared variable they don't have any problems of synchronization. Or if they deed access shared variable ,use lock or semaphore just mentioned in part 2.

#### 5.Memory deallocation
A page of thread will be deallocated only when function `thread_schedule_tail` is being called.And in this function the thread which tagged *THREAD_DYING* could be released.And only the function exit, a thread could be tagged with *THREAD_DYING*.So if we do not modify the code of `thread_schedule_tail` and do not change  when a thread should be tagged  with *THREAD_DYING*, the memory of running thread coudn't be deallocated.

### Rationale
Data structure modification, reasons are described in `8.priority queue`
As for coding, we would not need much coding from swapping from doubly linked list to a priority queue because it is one of the basic tasks we have learned in data structure.
As for time complexity, it is explained in `8.priority queue`.
As for space complexity, doubly linked list and queue both have worst case of O(n).
We did not modify much since we were basically using the same structure but in a different way.
We also solved starvation in algorithm secion 4 and 5 (which would be implemented actually in the next task).

## Task 3: Multi-level Feedback Queue Schedule

### Data structures and functions
```
struct thread // add recent_cpu for priority calculation
static struct thread * next_thread_to_run(void); //involk fetch_thread() and enable mlfqs
static struct thread * running_thread(); //change ready list and enable mlfqs
void thread_unblock(struct thread *t) //add it into thread_lists
void thread_block(struct thread *t) // add it into blocked_list
static struct thread * fetch_thread()// the next_thread_to_run should call that, scan through the thread list , then fetch the correct one
static void init_thread(struct thread*t, chonst char *name, int priority); set the priority 
void thread_schedule_tail(struct thread *prev);// reset the priority then sent back to the thread_lists
int thread_get_nice(void)  // get the thead`s nice value
void thread_set_nice(int new_nice)// set the thread`s nice value
int thread_get_recent_cpu(void) //get recent cpu time in moving average
int thread_get_load_avg(void) //get loaded average
int load_avg() //hold the load average for calculation

```
### Algorithms

- Creating a new thread

- Choosing the next thread to run

  We do a scan through the list, and marked the highest thread, then we picked up the thread with highest priority and send to the scheduler.

- Changing thread`s priority

  This happened in scheduling. When schedule() is called, and the thread has not finished, we re-calculate the priority for the thread, then put it back to the list.

- Calculate the priority

  First we need to calculate the load average(initial value 0). since the list_size is already implemented, we just call that to calculate the thread numbers in the ready_list,
by calling
 
>	load_avg = (59/60) * load_avg +(1/60) * list_size(&ready_list)

 we got load average.

Then we fetch nice value of a thread, and use it to calculate recent CPU time.First we set recent_cpu = recent_cpu + 1, 
and then we get the recent_cpu value, finally we called

>	priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)

and we got the priority
Here, we are using the fixed point value in the middle of the calculation ,then get the integer as the output in each step.`

after each second, we update recent_cpu  in thread_tick by

>	recent_cpu = (2*load_avg)/(2*load_avg+1)*recent_cpu+nice.

### Synchronization

 when the kernel called the scheduler, we block any interrupt for the scheduler, so there is no need for
considering synchronization issues.

### Rationale

- Alternative design 1: give 64 lists ,each response for 1 priority, when scheduling happened, we just moved the thread in the lists that fits its priority.
This design is useful for the situation that we need to handle many threads( typically more than 64), while we would not counter so many threads in pintos, so many of the lists may be empty in most of the time. plus, if we choose this kind, we should modify all the functions related with ready_list, thus this is not the choice. 
- Alternative design 2: give more than one list( for example,2), and we destributed threads into those lists by their priority (for example,threads with priority higher than 31 should go to list 1, otherwise list 0). Similarly, it takes less time to find a thread to run, but it takes up more memories, and we should modify a lot of code to implement that, causing the program exponentially buggy and hard to debug.


  In our current design, it may be less efficient if there is too much thread in the kernel, which is unlikely happended in pintos. Since we have ready-to-work data structure, there would not be too much code to be written, just to calculate the priority and choose the next thread part will need some code. 
  Assume we have n threads in the kernel, the time complexity and space complexity will all be O(n), since we have only one list, and we survey through the ready list to fine the next thread.
  We have considered about extensibility, that is why we choose not to modify next_thread_to_run() directly, because we may want to change another policy for that, for this we can just modify 'fetch_thread()', and leave the other things unchanged.

## Additional Questions

### 1. Effective Priority Test Case
Create 3 threads: `thread1` with priority 1, `thread2` with priority 2, and `thread3` with priority 3. Have `thread1` and `thread3` both recquire a lock to access a section that prints the name of the current thread. Additionally, `thread2` should print out its name right before it finishes executing.  Ready `thread1` first and have it acquire the lock.  Then, as soon as it acquires a lock, ready `thread2` and `thread3` and continue until all 3 until they have finished.  The correct output should be 
```
thread1
thread3
thread2
```
This is because `thread1` should accept a priority donation once `thread3` asks to acquire the lock that `thread1` currently has.  Therefore, `thread1` will have its priority raised to 3 and finish executing the portion of its code that recquires the lock and print `thread1` first.  However, the actual output will be 
```
thread2
thread1
thread3
```
This is because `thread2` will execute and print after `thread3` reaches the point where it is no longer ready and waits to acquire the lock from `thread1` since it will have the highest base priority remaining. 

### 2. MLFQS Scheduler table


Assume 20ms for a tick      

| timer ticks | R(A) | R(B) | R(C) | P(A) | P(B) | P(C) | thread to run |
|---|---|---|---|---|---|---|---|
|0          |  0   |  0   |  0   |  63  |  61  |  59  | A |
|4          |  4   |  0   |  0   |  62  |  61  |  59  | A |
|8          |  8   |  0   |  0   |  61  |  61  |  59  | A |
|12          |  12  |  0   |  0   |  60  |  61  |  59  | B |
|16          |  12  |  4   |  0   |  60  |  60  |  59  | A |
|20          |  16  |  4   |  0   |  59  |  60  |  59  | B |
|24          |  16  |  8   |  0   |  59  |  59  |  59  | A |
|28          |  20  |  8   |  0   |  58  |  59  |  59  | B |
|32          |  20  |  12  |  0   |  58  |  58  |  59  | C |
|36          |  20  |  12  |  4   |  58  |  58  |  58  | A |
