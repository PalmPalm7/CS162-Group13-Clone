# Final Report for Project 1: Threads
===================================

There was a significant misunderstanding when it came to what needed to be implemented for the efficient alarm clock prior to the design review.  The original belief was simply that *timer_sleep()* needed to be modified to change a thread's status to blocked if it wasn't already and to unblock it after a certain amount of time had passed without returning from the function until after the thread has been unblocked.  After the design review, the plan for the efficient alarm clock added the use of a list of sleeping threads that would be added to in *timer_sleep()* and taken away from in *timer_interrupt()* according to the value of a new member of the thread struct, *wake_time*.  When added to the list of sleeping threads, the current thread would avoid being placed on the *ready_list* and would only be on the *all_list* and *sleep_list* while it is sleeping.  Meanwhile, *timer_interrupt()* would be repeatedly called and move any threads that had a *wake_time* in the past off of the *sleep_list* and back onto the *ready_list* unblocked.

Originally, the plan was to have the default scheduler use a list of all of the threads attempting to acquire a lock and to use the *sema_up* and *sema_down* threads to manipulate the priorities of threads as they acquired locks.  After the design review, the decision was made to instead create additional members of the thread struct, including an entirely new priority donation struct, to keep track of the various donations.  Additionally, the default implementations of *sema_up* and *sema_down* were mostly kept intact and new functions were added to keep track of the synchronization between different threads as they interact with locks.

Prior to the design review, we were not as familiar with the thread system in pintos as we should have been. In order to implement the MLFQS, it was necessary to add a lot of callback function for updating each thread at a typical timing, and it is useless to change the other thread manipulation.  

The following functions were all modified or added to fully implement the MLFQS:

```
struct thread //Add recent_cpu and nice_value members
static struct thread *next_thread_to_run(void); //Choose the thread with the highest calculated priority to run
void schedule(); //Enable MLFQS functionality
void thread_ticks(); //Update the load average, recent cpu, and priority at the appropriate intervals
int thread_get_nice(void)  //Get the thead`s nice value
void thread_set_nice(int new_nice) //Set the thread`s nice value
int thread_get_recent_cpu(void) //Get recent cpu time
int thread_get_load_avg(void) //Get load average


fixed_point_t load_avg //Store the load average 
void update_all_recent_cpu(struct thread* t,void *aux UNUSED) //Update all threads' recent cpu usage
void thread_calculate_priority(struct thread* t,void *aux UNUSED) //Update all threads' prioirty
```

Among them the four calculation, which is `thread_get_nice`, `thread_set_nice`,`thread_get_recent_cpu`and`thread_get_load_avg` are implemented as the design. But since the design phase do not have much time, and code review consumes a lot of effort, we considered little about the possible unintended situations.For example, when implementing load average calculation, the first design was to store  `load_average` as an integer, which is 100 times real value, rounded to the nearest integer. That means every time we do the calculation, we need to cast it into fixed point real, then unscale by 100, the scale by 100 and round to integer after the calculation is done. We did not foreseen the expensive calculation would crash the timer and leads to a series of failure in the designing phrase, so deugging this case have consumed a heavy waste on time and effort.




Josh handled the efficient alarm clock, integration of the code between the three different tasks, and polishing the design doc and final report. 
Zuxin worked on implementing the MLFQS scheduler, including calculate load average, recent cpu and thread priority， and filling the documents with part related with MLFQS.  
Handi and Gary focused on creating the default scheduler, lock acquisition/release, and priority donation when multiple threads attempt to acquire the same lock, designing the scheduler and priority donation part, then filled them into the documents.

The group members have all put a great effort in finishing this projecet. For example, implementing their part, Josh and Gary often stays up at night to 3A.M., while other members sacrifice their weekend for the project. We did put a lot of work on the project, this may be something that went well for the project.
We think one of the major part we need to improve is the skill for code review. At first, Zuxin have a lot of missed understand about how the schedule work. He have no idea about how threads are removed from the `ready_list`,therefore he did a poor design on the scheduling, which causes a lot of problems when he implemented his part.
Besides, since some of our member are not so familiar with git, when we working in our part simutaneously, the code was pushed one after one, causing a lot of confilict. It was not before the TA noticed us that we started to use branch to finish our part seperately, then merge them afterwards.

##priority donation 
There is a lot of changes between the design doc and the real code. 


1. Initially we create a  tree data structure to represent the priority queue which is very troublesome but very intuitive. And we use this  data structure to maintian a heap which will always pop out the threads with max priority and minimize the overhead of `next _thread_to_run` in the long run.   However in the real code we abandon the new tree data structure and we use existing linked list to organize the threads with different priority, specificlly the `all_list` and `ready_list`.
2. we abandon the priority queue which is unecessary for pintos with few thread running simultaneously. we just change the strategy of the function `next _thread_to_run` . Basically what we do is traversing the `ready_list` and pop out the `list_elem` with biggest priority.
3. Adding the `thread_yield` calling at the end of function `create_thread` and `thread_set_priority` which is neglected when we work on design doc.weadd these two function in case of two situations. (1).A low priority thread create a high priority thread and low thread need to yield the cpu immediately.(2). A high priority thread set it's own priority lower than the second biggest priority in `ready_list` and in this situation current thread is required to yield the cpu immediately.
4. Adding some member variables in `struct thread` to track priority donation. My orginal design is to use local variable to record every priority donation. However whenwereally implement the functions,it turns out that we have to change a priority and restore it in two different functions.I could use global variable to record the donation but it could make code very complicated  damaging the readability of code. we choose to record the priority dontion within thread because the priority donation only affect the thread which receive the donation.
5. Changing the stratrety of finding the next thread in the waiting list of condition variables, lock and semaphore to satisfy the requirement of thread with highest priority.However we didn't notice is when we work on our design doc and we only change the strategy of `next_thread_to_run`.
6. We use `thread_foreach` to check all the thread in the `all_list`  to find the owner a of lock and if we find the owner of a lock we do priority donation  if we can't do nothing which is different comparing we thought in design doc.We thought we could create a new static list `lock_list` to record the thread which own a lock and every time we check the owner of lock we just need to go through the `lock_list`.However we couldn't find out the reason why the `lock_list` couldn't be initialize correctly and every time i loop the `lock_list` it always can't jump out of the loop. So we have to use the existing threads.
7. The member variable `own_lock ` and `orginal_priority` was designed to restore the the original priority.Now we reuse them but with different purposes.`lock_own` is to denote the number of priority donation record stored in `priority_donation[MAX_DONATION_NUM]` and `orginal_priority` is to store the priority when `thread_set_priority` is called but the current thread owns locks.
8. We add the pointer `waiting_lock` to denote the lock that a thread is waiting for which  is not included in doc.We add  the waiting lock to handle the situation like the "donation chain".
9. In the priority donation function `void thread_priority_donation(struct thread *thread, void *lock)` we did not use our method in the design doc -- setting its own priority level by using the newly defined structure `struct priority_donation` inside of our pre-defined thread structure. Therefore, we added data structures like `struct priority_donation priority_donation[MAX_DONATION_NUM]; ` to innerly keep track of the priority level. Furthermore, we implemented the priority donation chain function for recursive donations. <br /> For exmaple, for lock 1, 2 and 3 along with thread A B C and D which have lowest to highest priority respectively. We have thread A with lock 1, thread B with lock 2, thread C with lock 3, thread D without lock. <br />
![](./lock_donate_chain.png)
 In such case where a chained donation is involved, we will use the `void thread_priority_chain_donation(struct lock* lock,int priority_donation)` function to recursively donate the level. Another improvemennt is the implementation of `struct list_elem* list_max_thread(struct list *list)`, in which we loop through all the list element to find the thread with the highest priority. 





Reflection and improvment

The scheduler works fine though its time complexity is O(n) but for the limited thread in pintos ,it will not cost much to check all the thread in `ready_list`.

We don't have to go through all the threads recorded in `all_list`  to find the owner of lock, actually we could create a new list `lock_list` to record the owner of the list which may save a lot time, especially if there is a lot of thread in system but few of them owns a lock.







