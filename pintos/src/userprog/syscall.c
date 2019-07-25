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
int write (int fd, const void *buffer, unsigned length);
int read (int fd, const void *buffer, unsigned length);
int seek (int fd, unsigned length);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
	struct list open_list = thread_current()->open_list;
	uint32_t* args = ((uint32_t*) f->esp);
  if((int*)f->esp <= 0x08048000 ){
    printf("%s: exit(%d)\n", &thread_current ()->name, -1);
    thread_exit();
  }
  

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
    default:
    thread_exit();
    return;
  }

  switch (args[0]) {
    case SYS_CREATE:
    case SYS_REMOVE:
    case SYS_OPEN:
      break;
    case SYS_WRITE:
    case SYS_READ:
      break;
  }
  switch (args[0]) 
  {
   case SYS_EXIT:
     {
        if((args+1) >= 0xbffffffc)
        {
          printf("%s: exit(%d)\n", &thread_current ()->name, -1);
          thread_exit ();
        }
        f->eax = args[1];
        printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
        thread_exit();
        break;
     }
   case SYS_PRACTICE:
     {
       f -> eax =  args[1] + 1;
       break; 
     }
   case SYS_HALT:
     {
       shutdown_power_off();
     }   
   case SYS_READ: 
     if(args[1] == 0) 
       {
         uint8_t *buffer = (uint8_t*) args[2];
         int i = 0;
         while (i < args[3]) 
         {
           buffer[i] = input_getc ();
           if (buffer[i++] == '\n')
            break;
          }
          f->eax = i;
          break;
        }
     else
       {
          f->eax = read (args[1], (void *) args[2], args[3]);break;
       }
    case SYS_CREATE:
    {
      f->eax = filesys_create(args[1], args[2]);
      break;
    } 
    case SYS_REMOVE: 
    {
      f->eax = filesys_remove(args[1]);
      break;
    } 
  // printf("System call number: %d\n", args[0]);
    case SYS_OPEN:
    {
	if (args[1] == NULL || !strcmp(args[1], "")) {
		f->eax = -1;
	} else {
  		struct file *open_file = filesys_open(args[1]);
  		if (open_file != NULL){
	  		struct file_info *f1 = create_files_struct(open_file);
	  		f1->file_name = args[1];
	  		list_push_back(&thread_current()->open_list, &f1->elem);
	  		f->eax = f1->file_descriptor;
		} else {
			f->eax = -1;
		}
	  }
       break;
    }

    case SYS_WRITE: 
     f->eax = write (args[1], (void *) args[2], args[3]);break;


    case SYS_SEEK:
      f->eax = seek (args[1], args[2]); break;
	  
    default:
    {
      // TODO: Find the current file
      struct file_info *curr_file = files_helper (args[1]);
      if (curr_file == NULL)
        f->eax = -1;

      else if (args[0] == SYS_FILESIZE)
        f->eax = file_length (curr_file->file);
    

      else if (args[0] == SYS_SEEK)
        file_seek (curr_file->file, args[2]);

      else if (args[0] == SYS_TELL)
        f->eax = file_tell (curr_file->file);

      else if (args[0] == SYS_CLOSE) {
    	file_close(curr_file->file);
    	list_remove(&curr_file->elem);
    	free(curr_file);
      }
    }
  }
}

int read (int fd, const void *buffer, unsigned length)
{
  if (fd == 0) /* if fd == STDIN_FILENO */
  {
    int i = 0;
    for (i; i < length; i++)
    {
      char *temp;
      temp = (char*) (buffer + i); /* Write the next char */
      *temp = input_getc();
      return 0;
    }
  }

  struct file_info *curr_file = files_helper (fd);
  if (curr_file == NULL)
    return -1;
  int ret = file_read(curr_file, buffer, length);
  return ret;
}

int write (int fd, const void *buffer, unsigned length)
{
  if (fd == 1) /* if fd == STDOUT_FILENO */
  {
    putbuf(buffer,length);
    return length;
  }
  struct file_info *curr_file = files_helper (fd);
  if(curr_file == NULL)
    return -1;
  int ret = file_write(curr_file->file, buffer, length);
  return ret;
}

int seek (int fd, unsigned length)
{
  struct file_info *curr_file = files_helper (fd);
  if(curr_file == NULL)
    return -1;
  file_seek(curr_file, length);
}

unsigned tell (int fd)
{
  struct file_info *curr_file = files_helper (fd);
  if (curr_file == NULL)
    return -1;
  unsigned ret = file_tell (curr_file);
  return ret;
}

struct file_info*
create_files_struct(struct file *open_file) {
	struct file_info *f1 = malloc(sizeof(struct file_info));
	f1->reader_count = 0;
	f1->file_descriptor = find_fd();
	f1->file = open_file;
	return f1;
}

struct file_info* 
files_helper (int fd) {
  // Loop over the current file list
  struct list_elem *e;
  struct file_info *f;
  struct list open_list = thread_current()->open_list;

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
