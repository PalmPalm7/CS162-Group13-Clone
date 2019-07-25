Design Document for Project 2: User Programs
============================================

## Group Members

* Josh Alexander <josh.alexander1315@berkeley.edu>
* Handi Xie <hxie14@berkeley.edu>
* Wenzheng Guo <guowz19@berkeley.edu>
* Zuxin Li <lizx2019@berkeley.edu>
```
tid_t process_execute(const char *file_name) //enable argument parsing
void character_parsing(const char *file_name, char* attributes)//arguments parsing function

int practice(int i)
void halt(void)
void exit(int status)
pid_t exec(const char *cmd_line)
int wait(pid_t pid)

struct thread //originally, a thread will not hold its child process, so we may need to modify that

bool create(const char *file,unsigned initial_size)
bool remove(const char *file)
int open(const char *file)
int filesize(int fd)
int read(int fd, void *buffer, unsigned size)
int write(int fd, const void *buffer, unsigned size)
void seek(int fd, unsigned position)
unsigned tell(int fd)
void close(int fd)
```

## Task 1: Argument Passing
### 1.Data structures and functions 
Two macros will be added to restrict the maximum number of comand-line arguments could be passed into a program and the maximum length of an argument.

```
#define MAX_ARGUMENT 40 //max number of argumets
#define ARGUMENT_MAX_LENGH 50//max length of an argument
```
A local variable `argument[MAX_ARGUMENT][ARGUMENT_MAX_LENGH]` will store the parsed argument and the file name.

### 2.Algorithms 
Parsing the argument does not rely on I/O redirection or background processes. The argument is split into `*file_name` in the function `load` (in process.c) along single or continuous spaces. It will push the arguments on the stack when the function `setup_stack` returns and move the stack pointer to the top of the stack just above the fake return address. It will be implemented by calling the function ` push_argument` right after calling `setup_stack`.

```
  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;
  /* Push the argument and set the esp pointing to the top of stack  */
  push_argument(argument[MAX_ARGUMENT][ARGUMENT_MAX_LENGH],esp);

```

### 3.Synchronization
Sometimes the user could execute the same program multiple times causing multiple processes with the same excutable file to exist. When processes are created, the system will malloc different pages isolating the addresses of the processes.

### 4.Rationale
Another design that was considered involved changing the functionality of `process_excute` and passing the command-line arguments to this function directly.  However, if this function was to be modified, it would necessitate many other changes across the program due to how heavily other functions depend on it.

Our design is  that we don't change the function and we just add some comments to modify the stack pointer and push some arguments before the user proscess runs. This strategy is very neat and have  little side effect to the function.



## Task 2: Process control syscalls

### Data structures

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

### Algorithm
in `syscall_handler`, we put every syscalls in an `switch case` statement,which then calls seperate function to handle.
since we already have all the syacall code defined in `syscall-nr.h`, we can just use them in `syacall_handler`

in `practice` function, we will just use `return i++;` then the lib function will take care of the rest of things 

in `halt` function, we can just call `shutdown_power_off()`

in `exit` function, first we check the resources occupied by process, then release all of the resources(releasing locks, free memory, closing file for the process). second, check if there is any tid on the wait_list that is identical to child_pid or parent_pid, if there is, set the return value, decrement the reference count with lock to synchronize.If ref_cnt becomes 0, then remove the wait_status ,otherwise call `sema_down` and remove wait_status. 


in `wait` function, first we check the waitlist with parent_pid and child_pid. If there is no eligible wait_status(i.e. no wait_status have the same pair of parent_pid and child_pid), then return -1 instantly. If there is, then call `sema_down` to the semaphore and get wait_status's `return_val`, then use this as a return value.

in `exec` function, we just call `process_execute` function to start the new process.
in `process_execute` function ,we will create a new wait_status for the child process and parent process, so as to enable `wait`


### Synchronization 
In `practice` and `halt` function, there is no syncrhonization issues, so we do not need to think about synchronization.

In `exit`,`wait` and `exit`, 


### Rationale

#### Alternative design 1
modify `struct thread`, add `child_pid` list and `parent_pid` for each process, and two semaphore to identify if a process is terminated, or there is a child terminated. whenever there is a `exec`, we add a `child_pid` to the parent's list. Whenever there is a `wait`, we check the `child_pid` list to do the sanity check, and `sema_down` for specific child( or any child). 





## Task 3: File Operation Syscalls
### Data Structures
struct files that contains
The file descriptor, file name, lock, flag, reader count, list elem

```
Struct files {
	Int reader_count;
	Int file_descriptor;
	Char* file_name;
	Struct lock* file_lock;
	Struct list_elem elem;
}
```

### Algorithms

#### create
First, the file name will be checked to make sure it has no more than 14 characters. Next, `filesys_create (const char *name, off_t initial_size)` from filesys.c will be invoked to create the file. If the call succeeds, the filename will be added to the file list. While a file is being created, a flag will be upped to prevent any issues with trying to perform operations on a file before it finishes being created. If no file is currently being created, this flag will be set to 0.

#### remove
This function will simple invoke `filesys_remove (const char *name)` from filesys.c to remove a file. If the file has been removed, add the filename will be removed from the filelist.

#### open
This function will call `filesys_open (const char *name)` on a given file.  This function will fail if no file named NAME exists, or if an internal memory allocation fails. If this occurs, it will return the value -1.

#### filesize
If the file exists, this function will call `file_length (struct file *file)` from file.c which returns the size of the file in bytes.

#### read
If the file exists and the amount to be read is less than the filesize, the function `file_read (struct file *file, void *buffer, off_t size)` from file.c to read from a file into a buffer. Right before calling `file_read`, this function will call `file_deny_write` from file.c to ensure it is not edited during the read. After finishing the read, this function will check if there are other readers and call `file_allow_write` from file.c if there are none.

#### write
Write will behave very similar to read, except it will always allow other writers to edit the file once it has finished writing.  First, this function will invoke `file_write (struct file *file, void *buffer, off_t size)`  from file.c to write from a buffer into a file. This function will assert the amount to be written is less than the file size and the filename is on the file list. It will also call `file_deny_write` from file.c before writing and file_allow_write from file.c after it finishes.

#### tell
This will invoke `file_tell (struct file *file)` from file.c and return the current position in the given file to the caller.

#### close
Close will first assert that the given filename is on the file list.  It will then remove the file from the file list and call `file_close` from file.c to close the file.

### Synchronization
The file lock will make sure that multiple operations do not try to modify the same file at the same time, with the exception of multiple files attempting to read the same file.  When first attempting to read or write to a file, `file_deny_write` will be immediately called to ensure that it can not have any additional writers until the action completes at which point `file_allow_write` will be called if the `reader_count` for the file is 0.  The `reader_count` for each file will help make sure that all readers have finished with a file before anything can be written to it.  Finally, a flag to signify if a file is currently being created will be implemented to ensure that any attempts to operate on a file that does not exist can wait until a file finishes being created before returning an error that a file doesn’t exist.  This can come in handy if a file is in the middle of being created, but a context switch causes to trying to write to the file to fail.



### Rationale
Why using a list to keep track of the list of file currently used by any process over an array
There is no fixed size on the number of files currently called by process neither do we know what is the maximum number of files pintos could handle
Linked list structure is already implemented in lib/kernel/list.c


## Additional Questions
### Question 1

In file `sc-bad-sp.c` , a constant `.-(64*1024*1024)` in assembly code is invalid because user level code could only access virtual addresses ranging from 0GB - 3GB - 1byte. Any accesses outside of this area will cause a page fault. So, the following function `fail` should never be called.

### Question 2
In file `sc-boundary-2.c`, a variable `p[1]` at line 17 wiil cause a problem. First, three bytes of this `p[1]` variable are located at the end of a page and the last byte of it is located in a completely new page. If a system call encounters this situation, the process should be terminated immediately.  So, the following function `fail` should never be called.

### Question 3
The existing tests do not cover the sitation where the value of `PHYS_BASE` changes.  A new test in the suite could change it to 2GB. In this situation, creating a stack pointer that points to a valid address between 2GB-3GB will cause a page fault.

### Question 4
 
###### GDB Question1
```
{tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000ee0c "\210", <incomplete sequence \357>, priority = 31, allelem = {prev = 0
xc0034b50 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0034b58 <all
_list+8>}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

There is only one currently running thread.

```
* 1    Thread <main>     process_execute (file_name=file_name@entry=0xc0007d50 "args-none") at ../../userprog/process.c:32
```

There is another waiting thread.

###### GDB Question 2  
```
#0  process_execute (file_name=file_name@entry=0xc0007d50 "args-none") at ../../userprog/process.c:36

 sema_init (&temporary, 0);


#1  0xc002025e in run_task (argv=0xc0034a0c <argv+12>) at ../../threads/init.c:288

 process_wait (process_execute (task));

#2  0xc00208e4 in run_actions (argv=0xc0034a0c <argv+12>) at ../../threads/init.c:340

     a->function (argv);

#3  main () at ../../threads/init.c:133

 run_actions (argv);
```

###### GDB Question 3 
```
1    Thread <main>     start_process (file_name_=0xc0109000) at ../../userprog/process.c:55
```
There is only one running thread and two waiting threads.

```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eebc "\001", priority = 31, allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0036554 <temporary+4>, next = 0xc003655c <temporary+12>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority =0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "args-none\000\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0034b58 <all_list+8>}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

###### GDB Question 4 
At line 45 in process.c 
```
tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
```

###### GDB Question 5 
0x0804870c

###### GDB Question 6 
``` 
_start (argc=<error reading variable: can't compute CFA for this frame>, argv=<error reading variable: can't compute CFA for this frame>) at ../../lib/user/entry.c:9
```
###### GDB Question 7 
Argc and argv aren’t implemented yet, so the program segfaults when it tries to use them.




