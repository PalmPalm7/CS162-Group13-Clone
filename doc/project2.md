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
