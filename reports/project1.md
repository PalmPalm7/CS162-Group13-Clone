# Final Report for Project 1: Threads
===================================


At the design phrases, we are not so familiar with the thread system in pintos, thus we think it is necessary to modify lots of functions that related with thread in mlfqs task, and we need manually pull the thread with highest priority. After the pintos revision, we have deeper understanding towards the pintos system, thus there are not so many function modified or added.

here is the actual added or modified function:
```
struct thread //add recent_cpu and nice_value
static struct thread *next_thread_to_run(void); //enable mlfqs
void schedule(); //enable mlfqs 
void thread_ticks();
int thread_get_nice(void)  // get the thead`s nice value
void thread_set_nice(int new_nice)// set the thread`s nice value
int thread_get_recent_cpu(void) //get recent cpu time in moving average
int thread_get_load_avg(void) //get loaded average


fixed_point_t load_avg //hold the load average for calculation
void update_all_recent_cpu(struct thread* t,void *aux UNUSED) //used to update all recent cpu
void thread_calculate_priority(struct thread* t,void *aux UNUSED) //used to update all prioirty

```

