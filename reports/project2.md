Final Report for Project 2: User Programs
=========================================

## Argument Passing

## Process Control Syscalls

While trying to implement the ideas outlined in the design document, it was determined that many of the simpler functions, such as `practice` and `halt`, could be implemented inline. Additionally, `exec` and `exit` already exist as functions in the skeleton code, so it was necessary to name the syscall functionality for those commands `handle_exec` and `handle_exit` respectively within syscall.c.

## File Operation Syscalls

It was not mentioned in the design document, but the file descriptor and list of open file descriptors needed to be exclusive to each thread.  In order to accomodate this, they were created as additional members of the existing thread struct in thread.h.  It was also necessary to add multiple checks to make sure the syscall arguments themselves were valid. After the design review, the idea of complicated synchronization attempts such as waiting for a file to be created before attempting to perform any operations on it or individual locks for each file was thrown out in favor of a global lock.  Read and write syscall implementations were substantially modified in order to allow access to `stdin` and `stdout`.  A new function called `files_helper` was added to look through the list of open file descriptors and return the `file_info` struct for the correct file. 

## Student Testing Report



## Reflection and improvment

Josh handled the open, close, create, and remove syscalls as well as the invalid argument checking for all file operation syscalls. Josh also worked on the File Operation Syscalls section of the documents and polishing the design document and final report. 
Zuxin worked on implementing the process control syscalls as well as planning and writing the Process Control Syscalls portion of the documents.
Handi handled the write, read, seek, filesize, and tell syscalls.  He also helped with planning the file operation syscall implementation.
Gary focused on creating the argument handler and the student testing code and report.  He also filled in with various other tasks as needed and wrote the Argument Handler section of the documents.
Everybody put significant effort into integrating the code between the various tasks.

The group certainly had a tendency to get the majority of the work done close to the deadline for a given part, much more so than in project 1.  In the future, it will be better to try to spread out the work across the two weeks.  Meeting in person to work on implementing the code seemed to be a particularly effective strategy despite mostly doing so out of necessity.  Furthermore, the group had a few issues with git and should try to make sure that no issues arise due to improper use of git in the future.
