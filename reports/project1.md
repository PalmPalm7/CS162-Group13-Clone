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
