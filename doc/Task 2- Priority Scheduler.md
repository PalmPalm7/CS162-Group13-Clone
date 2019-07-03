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
    int internal_donation;//This member is added to avoid starvation. When a thread is blocked or waiting for some resources for too long,it increases.
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
The effctive priority consist of mainly three part.

- original priority which is set when thread created
- internal donation: When a low original priority thread wait  lock/semaphore for too long, it increase to avoid starvation. The increasing strategy we exploited will be demostrated in the part 5.(Default value:0)

effetive priority = original priority + internal donation

#### 5.Priority scheduling for semaphores and locks
Just like I mentioned, the priority scheduling for locks depends on semaphores.So we just focus on the strategy of scheduling for semaphores and we just mentions how `sema_down`,`sema_try_down`and`sema_up` three funtions works in two different situations ,so we put our attention on the  increasing strategy of `internal donation`.

When `sema_up` is called and if the waiting priority queue has some members, the thread with highest priority would be pop out.However threads with low priority have little chances to acquire locks/semaphores and starvation happens.To avoid this, every time after the `queue_pop` is called, the value of `internal donation` in **queue_element** of waiting priority queue plus one.

#### 6.Priority scheduling for condition variables
#### 7.Changing thread's priority
this part is for handi, to illustarte priority donation.

#### 8.priority queue




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






###Rationale
