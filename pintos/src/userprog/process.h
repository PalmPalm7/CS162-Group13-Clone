#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"


tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
int find_fd(void);

/*keep the status of the two processes*/
struct wait_status {
  int return_val;
  tid_t child_pid;
  tid_t parent_pid;
  struct semaphore end_p;
  int ref_cnt;
  struct lock ref_cnt_lock;
  struct list_elem elem;
 
};


/* status of all wait */
struct list wait_list;

struct thread* get_thread (tid_t tid);

void set_work_dir (tid_t ptid, tid_t ctid);

#endif /* userprog/process.h */
