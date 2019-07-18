#Process control syscalls#

##Data structures##

int syscall_handler(struct intr_frame *f UNUSED)
int practice(int i)
void halt(void)
void exit(int status)
pid_t exec(const char *cmd_line)
int wait(pid_t pid)

struct wait_status {
  int return_val;
  tid_t process_id;
  int ref_cnt;
  struct list_elem elem;
}
struct list all_wait_status;


##Algorithm##
in `syscall_handler`, we put every syscalls in an `switch case` statement,which then calls seperate function to handle.
since we already have all the syacall code defined in `syscall-nr.h`, we can just use them in `syacall_handler`

in `practice` function, we will just use `return i++;` then the lib function will take care of the rest of things 

in `halt` function, we can just call `shutdown_power_off()`

in `exit` function, first we check the resources occupied by process, then release all of the resources(releasing locks, free memory, closing file for the process). Second, check if there is any wait process. if it is, then wake them up and send the return status to them. At last, we print the required information about the process.

in `wait` function, first we check if there is any tid related with current process. If not, then return -1 instantly. If there is, then call `sema_down`
