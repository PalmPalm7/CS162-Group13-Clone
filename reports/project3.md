Final Report for Project 3: File System
=======================================

### Task 3
We modified a lot compared with the formal design doc.
in directory.c

```
/*helper function to iteratively find long path*/
bool find_path(const struct dir *dir, const char *name,
               char *tail_name, struct dir *file_dir);

struct dir_entry* dir_getdirent(const struct dir *dir, const char *name);
struct dir* dir_open_current();

void set_work_dir(tid_t ptid, tid_t ctid);
struct thread* get_thread(tid_t tid); 
```

was added. 

`find_path` find the directory for a relative or absolute path, then return the last entey of the path, and the last opened directory.

`dir_getdirent` is used to get the directory entry, which is helpful to check whether it is a directory, or implement the syscall.

`dir_open_current` support work directory for the threads. Everytime it is called, it seek current thread's work directory and open it. Because some kernel thread's work directory will not be set properly (they do not invoke `process_execute` and it is hard to manage its initial state)

`get_thread` are used to get the given thread with tid, `set_work_dir` will open child process's working directory the same as the parent process.

Besides, the sohpiscated syscall related with subdirectory is very hard to implemented and far beyond our imagination on design phase.

here are the modified functions
```
bool dir_readdir(struct dir* dir, const char* name) /* masked . and .. access from readdir*/

bool filesys_create (const char *name, off_t initial_size, int type) /* add a type member to specify the file's type*/

static void do_format (void) /* add . and .. for root directory*/
```
And the syscall are not implemented as planned as well

for `CHDIR`, we first use `find_path` to get the real directory that contains the target directory, then open up the real directory, and forward current work directory into the real directory, then if there exist target directory, we open it, then we forward current work directory into the directory and return true.

for `MKDIR`, we use `find_path` to get the real directory(current directory was stored in a `temp` pointer), then see if it contains such directory. If not, we create a new file with directory type, then we open up the new directory, and add . and .. directory entry, then close the new directory, and switch working directory back to `temp`, then close the real directory that stores the new directory.


