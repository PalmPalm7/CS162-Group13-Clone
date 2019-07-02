# Final Report for Project 1: Threads
===================================

## Alarm Clock
#### Data Structures/Functions
No additional data structure is necessary for the new `timer_sleep()` to be implemented.  The necessary functions to be added will be thread_current 
```
()->status = THREAD_BLOCKED 
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
#### Algorithms
The thread will be changed to the blocked state when `timer_sleep()` is called with a positive argument.  Once the correct amount of real-time has passed, it will be changed from blocked back to ready.  If `timer_sleep()` is called with a negative or zero argument, nothing will happen.

#### Synchronization
None of the current synchronization will be modified by the change in `timer_sleep()`.  Only the current thread will be put to sleep and will not go back to the ready state until after the timer is up.  Since no synchronization items will be modified, it is safe to assume that the existing synchronization tactics are sufficient.

#### Rationale
This design is an improvement upon the existing design since there is no busy waiting involved.  The thread simply goes to the existing blocked state throughout the duration of the given timer, then switches back to the read state afterwards.

