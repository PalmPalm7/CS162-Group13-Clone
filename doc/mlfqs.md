## part 3 Multi-level Feedback Queue Schedule

### Data structures and functions
```
struct thread // add load_avg and recent_cpu for priority calculation
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

Then we fetch nice value of a thread, and use it to calculate recent CPU time.First we set recent CPU as 0, then we use formula 

>	recent_cpu = (2*load_avg)/(2*load_avg+1)*recent_cpu+nice.

and then we get the recent_cpu value, finally we called

>	priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)

and we got the priority
Here, we are using the fixed point value in the middle of the calculation ,then get the integer as the output in each step.`

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
