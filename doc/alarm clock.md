
## Alarm Clock
#### Data Structures/Functions
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
#### Algorithms
The thread will be changed to the blocked state when `timer_sleep()` is called with a positive argument.  Once the correct amount of real-time has passed, it will be changed from blocked back to ready and added to the ready list.  If `timer_sleep()` is called with a negative or zero argument, nothing will happen.

#### Synchronization
None of the current synchronization will be modified by the change in `timer_sleep()`.  Only the current thread will be put to sleep and will not go back to the ready state until after the timer is up.  Since no synchronization items will be modified, it is safe to assume that the existing synchronization tactics are sufficient.

#### Rationale
This design is an improvement upon the existing design since there is no busy waiting involved.  The thread simply goes to the existing blocked state throughout the duration of the given timer, then switches back to the read state afterwards. This solution was chosen since it has minimal synchronization modifications and only added existing code from thread.c to the `timer_sleep()` function.

## Additional Questions
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
