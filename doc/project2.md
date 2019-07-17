Design Document for Project 2: User Programs
============================================

## Group Members

* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>
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

##Task 1: Argument Passing
### 1.Data structures and functions 
Add the two macros to restrict the maximum number of comand-line arguments could be passed into a program and  the maximum length of an argument.

```
#define MAX_ARGUMENT 40 //max number of argumets
#define ARGUMENT_MAX_LENGH 50//max length of an argument
```
add the local variable `argument[MAX_ARGUMENT][ARGUMENT_MAX_LENGH]` to store parsed argument and file name

### 2.Algorithms 
Parsing the argumet is pretty simple, since we do not need to worry about the I/O redirection or background process. What we do is basically spliting the argument `*file_name` in function `load` (in process.c) by single or continuous spaces, pushing the arguments  on stack when function `setup_stack` returns  and adjusting the `*sp` making it pointed at the top of stack which is just above the fake return address.We implement by call the function ` push_argument` just after calling `setup_stack`.

```
  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;
  /* Push the argument and set the esp pointing to the top of stack  */
  push_argument(argument[MAX_ARGUMENT][ARGUMENT_MAX_LENGH],esp);

```

###3.Synchronization
Sometimes user could execute same execute file for many times which cause multiple processes with same excuteble file exsit, however when  processes are creating system will malloc different page isolating the address of processes.So there may not be any issues of problems.

###4.Rationale
Another design is that we can change the function of `process_excute` and pass the command-line arguments to this function directly,however if we modify this top-level function we have change many other function it invokes which is very troublesome and may encounter many problems that you can't predict.

Our design is  that we don't change the function and we just add some comments to modify the stack pointer and push some arguments before the user proscess runs. This strategy is very neat and have  little side effect to the function.

##Additional Question
###Question 1

In file `sc-bad-sp.c` ,a constant `.-(64*1024*1024)`  in assemble code  is invalid, because user level code could only access virtual address ranging from 0GB - 3GB-1byte.Any access out of this area will cause a page fault.So the following function `fail` should never be called.

###Question 2
In file `sc-boundary-2.c`, a variable `p[1]`  at 17th lines of the file wiil cause a problems.
First three bytes of this `p[1]` variable is located in the end of a page and the last bytes of it is located in a new pages.If a system call encounter this situation, it should be terminated immediately.So the following function `fail` should never be called.

###Question 3
The exsiting test could cover the sitation that if the value of `PHYS_BASE`  changes like it changes to 2GB, a stack pointer that points to a valid address between 2GB-3GB will cause a page fault.




