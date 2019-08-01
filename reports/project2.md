Final Report for Project 2: User Programs
=========================================

## Argument Passing
If any user level thread is created, it will have to call the function `load` to load the elf32 format executable file which also calls the function `setup_stack` to set the stack pointer which currently does not pass any arguments. The stack pointer is adjusted before passing the argumet.  Specifically the thread-safe function `strtok_r` is used to retrieve the argument in the variable `file_name`. Every time the argument is passed, the length of string is subtracted and the string is copied to the stack. The pointer of the string is recorded and, the string is inserted, the pointer is inserted in the stack with the order from right to left. Finally, the number of the arugments and the indirect pointer are passed onto the stack.

## Process Control Syscalls

While trying to implement the ideas outlined in the design document, it was determined that many of the simpler functions, such as `practice` and `halt`, could be implemented inline. Additionally, `exec` and `exit` already exist as functions in the skeleton code, so it was necessary to name the syscall functionality for those commands `handle_exec` and `handle_exit` respectively within syscall.c.

To ensure wait twice would not cause threads to block, it was important to free the wait status as soon as the `sema_down` was completed. `handle_exit` was used before a `thread_exit` to ensure the wait status was correctly released and the kernel thread can exit normally.

To implement the `exec` syscall, one semaphore `end_l` and one lock `exec_lock` were integrated to synchrnoize loading the status of a child process. Previously, the plan for dealing with the load status was not fully developed. In addition, to protect an executing file from allowing write permissions, the `load` function in process.c was modified, and `struct thread` held the executable file before it is closed. `file_deny_write` was also invoked to protect the executable.

## File Operation Syscalls

It was not mentioned in the design document, but the file descriptor and list of open file descriptors needed to be exclusive to each thread.  In order to accomodate this, they were created as additional members of the existing thread struct in thread.h.  It was also necessary to add multiple checks to make sure the syscall arguments themselves were valid. After the design review, the idea of complicated synchronization attempts such as waiting for a file to be created before attempting to perform any operations on it or individual locks for each file was thrown out in favor of a global lock.  Read and write syscall implementations were substantially modified in order to allow access to `stdin` and `stdout`.  A new function called `files_helper` was added to look through the list of open file descriptors and return the `file_info` struct for the correct file. 

## Student Testing Report
Two test cases were created to test the tell and seek syscalls.
First is `seek-and-tell`, which tests the basic function of the seek and tell syscalls.
In this test, `seek` is called to move the file pointer and use `tell` to test if the file pointer is moved or not.

seek-and-tell.output
```
Copying tests/userprog/seek-and-tell to scratch partition...
qemu -hda /tmp/d70x2WKV1R.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading..........
Kernel command line: -q -f extract run seek-and-tell
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  366,592,000 loops/s.
hda: 5,040 sectors (2 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 167 sectors (83 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 102 sectors (51 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'seek-and-tell' into the file system...
Erasing ustar archive...
Executing 'seek-and-tell':
(seek-and-tell) begin
(seek-and-tell) create test.data
(seek-and-tell) open test.data
(seek-and-tell) current position is 5 bytes
(seek-and-tell) end
seek-and-tell: exit(0)
Execution of 'seek-and-tell' complete.
Timer: 58 ticks
Thread: 0 idle ticks, 57 kernel ticks, 1 user ticks
hda2 (filesys): 87 reads, 213 writes
hda3 (scratch): 101 reads, 2 writes
Console: 1021 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
`
seek-and-tell.result
`
PASS
```


The second test is `seek-big`, which moves the file pointer out of the size of the file and tests if `tell` would return the right file pointer postion.
seek-big.output
```
Copying tests/userprog/seek-big to scratch partition...
qemu -hda /tmp/bmmRVIB_lt.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading..........
Kernel command line: -q -f extract run seek-big
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  419,020,800 loops/s.
hda: 5,040 sectors (2 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 167 sectors (83 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 102 sectors (51 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'seek-big' into the file system...
Erasing ustar archive...
Executing 'seek-big':
(seek-big) begin
(seek-big) create test.data
(seek-big) open test.data
(seek-big) current position is 21 bytes
(seek-big) end
seek-big: exit(0)
Execution of 'seek-big' complete.
Timer: 56 ticks
Thread: 0 idle ticks, 55 kernel ticks, 1 user ticks
hda2 (filesys): 87 reads, 213 writes
hda3 (scratch): 101 reads, 2 writes
Console: 972 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
`
seek-big.result
`
PASS
```



## Reflection and improvment

Josh handled the open, close, create, and remove syscalls as well as the invalid argument checking for all file operation syscalls. Josh also worked on the File Operation Syscalls section of the documents and polishing the design document and final report. 
Zuxin worked on implementing the process control syscalls as well as planning and writing the Process Control Syscalls portion of the documents, and protection from writing and openning executable.
Handi handled the write, read, seek, filesize, and tell syscalls.  He also helped with planning the file operation syscall implementation.
Gary focused on creating the argument handler and the student testing code and report.  He also filled in with various other tasks as needed and wrote the Argument Handler section of the documents.
Everybody put significant effort into integrating the code between the various tasks.

The group certainly had a tendency to get the majority of the work done close to the deadline for a given part, much more so than in project 1. This is largely due to a midterm during the first week and the group was not fully prepared to work on the project, and the necessary part comes out very late, which pushes the test time back to the very end of the deadline. It will be better to try to spread out the work across the two weeks, while things are not getting so well.  Meeting in person to work on implementing the code seemed to be a particularly effective strategy despite mostly doing so out of necessity.  Furthermore, the group had encountered a few issues with git and should try to make sure that no issues arise due to improper use of git in the future.
