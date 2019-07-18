# Process control syscalls #

## Data structures ##

```
/*in syscall.c*/
int syscall_handler(struct intr_frame *f UNUSED)
int practice(int i)
void halt(void)
void exit(int status)
pid_t exec(const char *cmd_line)
int wait(pid_t pid)

/*in thread.c and thread.h*/
struct wait_status {
  int return_val;
  tid_t child_pid;
  tid_t parent_pid;
  struct semaphore end;
  int ref_cnt;
  lock ref_cnt lock;
  struct list_elem elem;
}

struct list wait_list; /* store all the wait_status*/


/*in process.c*/
tid_t process_execute(const char *file_name)

```

## Algorithm ##
in `syscall_handler`, we put every syscalls in an `switch case` statement,which then calls seperate function to handle.
since we already have all the syacall code defined in `syscall-nr.h`, we can just use them in `syacall_handler`

in `practice` function, we will just use `return i++;` then the lib function will take care of the rest of things 

in `halt` function, we can just call `shutdown_power_off()`

in `exit` function, first we check the resources occupied by process, then release all of the resources(releasing locks, free memory, closing file for the process). second, check if there is any tid on the wait_list that is identical to child_pid or parent_pid, if there is, set the return value, decrement the reference count with lock to synchronize.If ref_cnt becomes 0, then remove the wait_status ,otherwise call `sema_down` and remove wait_status. 


in `wait` function, first we check the waitlist with parent_pid and child_pid. If there is no eligible wait_status(i.e. no wait_status have the same pair of parent_pid and child_pid), then return -1 instantly. If there is, then call `sema_down` to the semaphore and get wait_status`s `return_val`, then use this as a return value.

in `exec` function, we just call `process_execute` function to start the new process.
in `process_execute` function ,we will create a new wait_status for the child process and parent process, so as to enable `wait`


## Synchronization ##
In `practice` and `halt` function, there is no syncrhonization issues, so we do not need to think about synchronization.

In `exit`,`wait` and `exit`, 


## Rationale ##

### Alternative design 1 ###
modify `struct thread`, add `child_pid` list and `parent_pid` for each process, and two semaphore to identify if a process is terminated, or there is a child terminated. whenever there is a `exec`, we add a `child_pid` to the parent`s list. Whenever there is a `wait`, we check the `child_pid` list to do the sanity check, and `sema_down` for specific child( or any child). 
