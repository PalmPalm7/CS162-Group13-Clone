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

struct file_info
  {
    int reader_count;
    int file_descriptor;
    const char *file_name;
    struct file *file;
    struct list_elem elem;
  };

static void syscall_handler (struct intr_frame *f);
struct file_info* files_helper (int fd);
struct file_info* create_files_struct(struct file *open_file);


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
	uint32_t* args = ((uint32_t*) f->esp);
  switch (args[0]) {
    case SYS_READ:
    case SYS_WRITE:
    case SYS_CREATE:
    case SYS_SEEK:
    case SYS_EXIT:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    case SYS_TELL:
    case SYS_CLOSE:
      break;
  }

  switch (args[0]) {
    case SYS_CREATE:
    case SYS_REMOVE:
    case SYS_OPEN:
      break;
    case SYS_WRITE:
    case SYS_READ:
      break;
  } if (args[0] == SYS_EXIT) {
    f->eax = args[1];
    printf("%s: exit(%d)\n", &thread_current ()->name, args[1]);
    thread_exit();
  } else if (args[0] == SYS_READ && args[1] == 0) {
    uint8_t *buffer = (uint8_t*) args[2];
    int i = 0;
    while (i < args[3]) {
      buffer[i] = input_getc ();
      if (buffer[i++] == '\n')
        break;
    }
    f->eax = i;
  }
  else if (args[0] == SYS_WRITE && args[1] == 1) {
    f->eax = args[3];
  } else if (args[0] == SYS_CREATE) {
  		filesys_create(args[1], args[2]);
  } else if (args[0] == SYS_REMOVE) {
  		filesys_remove(args[1]);
  } 
  // printf("System call number: %d\n", args[0]);
  else if (args[0] == SYS_OPEN) {
  	struct file *open_file = filesys_open(args[1]);
  	struct file_info *f1 = create_files_struct(open_file);
  	f1->file_name = args[1];
  	list_push_back(&open_list, &f1->elem);
  	f->eax = f1->file_descriptor;
  } else {
    // TODO: Find the current file
    struct file_info *curr_file = files_helper (args[1]);
    if (curr_file == NULL)
      f->eax = -1;

    else if (args[0] == SYS_FILESIZE)
      f->eax = file_length (curr_file->file);

    else if (args[0] == SYS_READ)
      file_read (curr_file->file, (void *) args[2], args[3]);

    else if (args[0] == SYS_WRITE)
      file_write (curr_file->file, (void *) args[2], args[3]);

    else if (args[0] == SYS_SEEK)
      file_seek (curr_file->file, args[2]);

    else if (args[0] == SYS_TELL)
      f->eax = file_tell (curr_file->file);

    else if (args[0] == SYS_CLOSE) {
  	file_close(curr_file->file);
  	list_remove(&curr_file->elem);
    }
  }
}

struct file_info*
create_files_struct(struct file *open_file) {
	struct file_info *f1;
	f1->reader_count = 0;
	f1->file_descriptor = find_fd();
	f1->file = open_file;
	return f1;
}

struct file_info* 
files_helper (int fd) {
  // TODO: Loop over the current file list
  struct list_elem *e;
  struct file_info *f;
  for(e = list_begin (&open_list); e != list_end (&open_list);
      e = list_next (e))
    {
      f = list_entry (e, struct file_info, elem);
      if (f->file_descriptor == fd)
      {
        return f;
      }
    }
  return NULL;
}