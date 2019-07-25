#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"


tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
int find_fd(void);

struct file_info
  {
    int reader_count;
    int file_descriptor;
    const char *file_name;
    struct file *file;
    struct list_elem elem;
  };

#endif /* userprog/process.h */
