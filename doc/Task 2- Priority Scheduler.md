## Task 2: Priority Scheduler
###Data structures and functions
####1.Adding data structure
Add data structure  `queue` and `queue_element`.

```
struct queue_elem{
    struct queue_elem* left_child;
    struct queue_elem* right_child;
    struct queue_elem* parent;
    int priority;
    int external_donation;//This member is added to avoid priority inversion
    int internal_donation;//This member is added to avoid starvation. When a thread is blocked or waiting for some resources for too long,it increases.
};

```
```
struct queue
{
    struct queue_elem root;//The head of queue
    int count; //count on number of threads in the queue
};
```
Using this priority queue(maxheap indeed) replace the list in `struct thread`.

####2.Adding functions
```
struct queue_elem *queue_root (struct queue* queue);//return first element of a priority queue
struct queue_elem *queue_pop (struct queue* queue);//pop out the first element of a priority queue
struct queue_elem *queue_insert(struct queue* queue);//insert the element into the queue and reorder this priority queue to maintain the property of a heap
int queue_size(struct queue* queue);//return the size a priority queue
```


###Algorithms
####1.Choosing the next thread to run
Currently the strategy pintos used  to chose a next thread to run is by calling a function `list_pop_front` which is just pop a element from a ready list.

However we change the **ready list** to aforementioned **priority queue** to store the ready threads. And we change the function`list_pop_front` to `queue_pop` which will pop out an element with higgest priority if priority queue is not empty or return *NULL* if it is empty.

####2.Acquiring and releasing a Lock
In fact, the implementations of `lock_acquire` ,`lock_try_acquire` and `lock_release` is just calling the functions `sema_down`,`sema_try_down`and`sema_up`.So we just to re-implement these three functions.

`sema_up` is pretty simple, basically it adds one to the value `semaphore.value` and if the value greater than zero it  calls a function `queue_pop` modified by ours to pop out a thread with highest priority and unblock it 

So we foucs on `sema_down` and `sema_try_down`  in two different circumstances.

- For a interrupt handler: if program want to acquire a lock,it should call the `sema_try_down` which  if `semaphore.value` is less equal than 0 it returns false else it substract one from `semaphore.value` then returns ture.If the functions returns ture ,the interrupt handlers would konw it have already acquire a lock and execute next code or if it returns false, the handlers would abort because the interrupt handlers couldn't sleep and they couldn't be added into a waiting list.

- For a normal thread:if a normal thread wants to acquire a lock,it should call the `sema_down` which first subtract one from  `semaphore.value` and if it less than 0 the current thead will be added to waiting priority queue and call `thread_block` to unblock itself and find next thread to run.

####4.Computing the effective priority
The effctive priority consist of mainly three part.

- original priority which is set when thread created
- external donation:When a low original priority thread manage to acquire a lock,this value could increase to avoid  priority inversion.The increasing strategy will be demostarted in the part 7(Default value:0)
- internal donation: When a low original priority thread wait  lock/semaphore for too long, it increase to avoid starvation. The increasing strategy we exploited will be demostrated in the part 5.(Default value:0)

effetive priority = original priority + external donation + internal donation

####5.Priority scheduling for semaphores and locks
Just like I mentioned, the priority scheduling for locks depends on semaphores.So we just focus on the strategy of scheduling for semaphores and we just mentions how `sema_down`,`sema_try_down`and`sema_up` three funtions works in two different situations ,so we put our attention on the  increasing strategy of `internal donation`.

When `sema_up` is called and if the waiting priority queue has some members, the thread with highest priority would be pop out.However threads with low priority have little chances to acquire locks/semaphores and starvation happens.To avoid this, every time after the `queue_pop` is called, the value of `internal donation` in **queue_element** of waiting priority queue plus one.

####6.Priority scheduling for condition variables
####7.Changing thread's priority
this part is for handi, to illustarte priority donation.

####8.priority queue




###Synchronization

###Rationale