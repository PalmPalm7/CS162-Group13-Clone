## part 3 Multi-lvel Feedback Queue Schedule

### Data structures and functions
```
#define LIST_NUM =64
struct multi_queue
{
  struct list thread_lists[LIST_NUM];
  struct list blocked_list;
}
static struct thread * next_thread_to_run(void);
static struct thread * running_thread();
// should we modify thread_yield, then?

 
```
### Algorithms

### Synchronization

### Rationale
