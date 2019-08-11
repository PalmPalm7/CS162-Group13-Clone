Final Report for Project 3: File System
=======================================

###Task 3###
We modified a lot compared with the formal design doc.
in directory.c

```
/*helper function to iteratively find long path*/
bool find_path(const struct dir *dir, const char *name,
               char *tail_name, struct dir *file_dir);

struct dir_entry* dir_getdirent(const struct dir *dir, const char *name);
struct dir* dir_open_current();
```

was added. 

`find_path` find the directory for a relative or absolute path, then return the last entey of the path, and the last opened directory.

`dir_getdirent` is used to get the directory entry, which is helpful to check whether it is a directory, or implement the syscall.

`dir_open_current` support work directory for the threads.
