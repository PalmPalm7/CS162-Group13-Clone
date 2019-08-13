Final Report for Project 3: File System
=======================================
## Task 1
There is a lot of modifications comparing with the initial design of cache.
we change the following things:

1.  we use the lock `entry_num_lock` and `lock` in each cache entry  instead of disabling interrupts to accomplish exclusively accessing the the `entry_num` and entries of cache. Because  disabling interrupts cost a lot and the frequency of query cache is very high and if we do use disabling interrrupts as sychronization methods it may bring a lot of  unecessary overhead.
2. we still keep `ref_count `  in the `cache_entry` however the funtion of this member variable changes. We use it to realize the **Clock Algrithm** rather than **LFU Algrithm** which only takes the hitting times into consideration while ignore the influence of order of each accessing.
3. We directly use `cache_read` and `cache_write` to replace the `block_read` and `block_write` instead of using wrapper function `cache_block_ read` and `cache_block_write` to do so.
4. we add two functions `cache_read_bytes ` and `cache_write_bytes ` to do the sectors to bytes translation so we don't have to use any `bounce` in `inode` functions.

For the synchroniztion of  accessing cache what we do is very simple but effective. No matter what operation we want to do for a cache entry we should get the lock of each entry firstly, in this way, we treat read and write equally and they also  exclusively access same entry.

There is a redundant member variable `write` in `cache_entry` which is used for denoting this entry should be write back or not. However we have to write back entry in cache to disk bacause  before we swap it out we don't do anything to the corresponded disk sector.

## Task 2
At the suggestion of the TA, the number of direct inodes were increased to 124.  The function `inode_write_at` was designed to expand the file to the necessary length prior to attempting to write to the file.  The new design incorporates a modified `byte_to_sector` function that properly returns the inode sector desired.  Furthermore, `inode_close` now iterates through every allocated sector for a given `inode_disk` and frees their mapping for future use.  Multiple helper functions: `add_inode`, `read_sector`, and `deallocate_inode` were added to assist with `inode_write_at`, `byte_to_sector`, and `inode_close` respectively. There was no additional mapping function used as that idea from the design document was scrapped.

## Task 3
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

## Student Testing Report

###my-test-1
This test checks the buffer cache's ability to write to full blocks and not reading them first. The test case involves two new syscalls: `buffer_readcnt` and `buffer_writecnt` to get the number of block_read and block_write.

- 200 times will certainly cause indirect blocks to be used and cause some since the kernel does not support extensible files.
- In more make check, it actually does not pass the persistence test. We believe it is due to unnecesarry reads called in the test cases.

######output
```
Copying tests/filesys/extended/my-test-1 to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu -hda /tmp/BneSQ7nZuL.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading...........
Kernel command line: -q -f extract run my-test-1
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  223,232,000 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 185 sectors (92 kB), Pintos OS kernel (20)
hda2: 236 sectors (118 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'my-test-1' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'my-test-1':
(my-test-1) begin
(my-test-1) create "file1"
(my-test-1) open "file1"
(my-test-1) writing 100KB (200 blocks) to "file1"
(my-test-1) block_read is called 0 times in 200 times of writes
(my-test-1) block_write is called 201 times in 200 times of writes
(my-test-1) close "file1"
(my-test-1) end
my-test-1: exit(0)
Execution of 'my-test-1' complete.
Timer: 88 ticks
Thread: 0 idle ticks, 77 kernel ticks, 11 user ticks
hdb1 (filesys): 36 reads, 774 writes
hda2 (scratch): 235 reads, 2 writes
Console: 1253 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

######result
```
PASS
```

###my-test-2
This test checks the buffer cache's ability to write and read byte-to-byte while exceeding the maximum buffer size allowed. We call the buffer this way so we could expect a different result since twice the buffer size could cause write to re-write on what was already stored in buffer and slow down, slower than our implementation expected.

######output
```
Copying tests/filesys/extended/my-test-2 to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu -hda /tmp/8XKYEb7ALG.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading...........
Kernel command line: -q -f extract run my-test-2
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  226,508,800 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 185 sectors (92 kB), Pintos OS kernel (20)
hda2: 236 sectors (118 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'my-test-2' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'my-test-2':
(my-test-2) begin
(my-test-2) create "file2"
(my-test-2) open "file2"
(my-test-2) writing 64KB to "file2"
(my-test-2) block_read is called 36 times in 64 times of writes
(my-test-2) block_write is called 545 times in 64 times of writes
(my-test-2) close "file2"
(my-test-2) end
my-test-2: exit(0)
Execution of 'my-test-2' complete.
Timer: 79 ticks
Thread: 0 idle ticks, 76 kernel ticks, 3 user ticks
hdb1 (filesys): 36 reads, 577 writes
hda2 (scratch): 235 reads, 2 writes
Console: 1237 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

######result
```
PASS
```

## Reflection

Josh primarily handled the second task, extendable files in both documents and implemented it in Pintos.  Zuxin worked on the third task dealing with directories for the final report as well as coding it.  Gary completed and explained the buffer cache in Pintos and both documents. Handi wrote the student testing report and was in charge of overall synchronization.  
Timeliness was once again a fairly large issue as the group had to eventually resort to a 6 hour worksession on Monday to focus on the persistence tests, the `dir-vine` test, and the student testing document.
