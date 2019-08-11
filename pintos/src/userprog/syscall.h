#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <string.h>
#include "threads/interrupt.h"
#include "userprog/process.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/directory.h"

struct file_info
  {
    int reader_count;
    int file_descriptor;
    struct file *file;
    struct dir_entry* dirent; 
    struct list_elem elem;
    bool removed;
  };

void syscall_init (void);

#endif /* userprog/syscall.h */
