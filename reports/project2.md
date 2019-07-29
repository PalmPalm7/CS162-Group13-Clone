Final Report for Project 2: User Programs
=========================================

## Changes made in syscall part

In real implementation, we found some of the function maybe unnecessarily, so they are implemented inline.
- practice
- halt

And for specification, the exec and exit function are using the other name. Since they will be called out of the syscall.c
so it is important to give them a different name.

- handle_exit
- handle_exec
