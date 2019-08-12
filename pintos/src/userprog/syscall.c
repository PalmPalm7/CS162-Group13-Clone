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
#include "filesys/inode.h"

static void syscall_handler (struct intr_frame *f);
struct file_info* files_helper (int fd);
struct file_info* create_files_struct(struct file *open_file);
int write (int fd, const void *buffer, unsigned length);
int read (int fd, const void *buffer, unsigned length);
int seek (int fd, unsigned length);
tid_t handle_exec(const char *cmd_line);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);

static void clear_all_file();

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
  uint32_t* pagedir = thread_current()->pagedir;
  /*sanity check 1, the syscall attribute should not exceeding memory space*/
  if((int*)f->esp <= 0x08048000 ||(args+1 >= PHYS_BASE))
    {
      handle_exit(-1);
      thread_exit();
    }
  /* sanity check 2, the syscall attributes should not reach out of the process`s pages*/
  if (pagedir_get_page ( pagedir, args) == NULL)
    {
      handle_exit(-1);
      thread_exit();
    } 
  /*shared by subdirectory syscalls*/  
  char tail_name[NAME_MAX+1] ;
  struct thread *t = thread_current ();
  struct dir *next_dir = (struct dir*) malloc (sizeof (struct dir));

  switch (args[0]) 
  {
    case SYS_EXIT:
      {
         f->eax = args[1];
         handle_exit(args[1]);
         thread_exit();
         break;
      }
    case SYS_EXEC: 
      { 
        if(pagedir_get_page (pagedir, args[1]) == NULL)
         {
           handle_exit(-1);
           thread_exit();
         }
         f->eax = handle_exec (args[1]); 
         break;
      } 
    case SYS_WAIT:
      {
        f->eax = process_wait(args[1]);
        break;
      } 
    case SYS_PRACTICE:
      {
        f->eax =  args[1] + 1;
        break;
      }
    case SYS_HALT: 
      {
        cache_sync();
        shutdown_power_off();
      }
    case SYS_READ:
      {
        if( args[1] == 0) 
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
          }
        else
          f->eax = read (args[1], (void *) args[2], args[3]);
         break;
     } 

    case SYS_CREATE: 
     {
       if (args[1] == 0) 
         {
           f->eax = -1;
           handle_exit(-1);
           thread_exit();
         } 
       else 
         {
           void* valid_adress = pagedir_get_page(pagedir, args[1]);
           if (valid_adress == NULL) 
             {
               f->eax = -1;
               handle_exit(-1);
               thread_exit();
             }

           if (args[1] == NULL || !strcmp(args[1], "")) 
             {
               f->eax = 0;
               break;
             } 
           if( t -> work_dir == NULL)
             t -> work_dir = dir_open_root ();
           if(find_path( t-> work_dir, args[1], tail_name, next_dir))
             { 
               /* currently move to given directory, then move back*/
               struct dir *temp = t -> work_dir;
               t -> work_dir = next_dir;
  	       f -> eax = filesys_create(tail_name, args[2],IS_REG);
               t -> work_dir = temp;
             }
           else
             f -> eax = 0;
        }
        break;

     }
   case SYS_REMOVE: 
    {
      
      find_path (t-> work_dir, args[1], tail_name, next_dir);

      struct dir *temp = t -> work_dir;
      t -> work_dir = next_dir;
      f->eax = filesys_remove(tail_name);
      t -> work_dir = temp;
      break;
    } 

  case SYS_OPEN: 
   {
     if(!is_user_vaddr(args[1]))
     { 
        handle_exit(-1);
        thread_exit();
     }
     void* valid_adress = pagedir_get_page(pagedir, args[1]);
     if (valid_adress == NULL ) 
       {
         f->eax = -1;
         handle_exit(-1);
         thread_exit();
       } 
     else if (args[1] == NULL || !strcmp(args[1], "")) 
       {
         f->eax = -1;
       } 
     else 
       {
         struct dir_entry* dirent = dir_getdirent (dir_open_current (), args[1]);
         if(dirent == NULL)
           {
             f -> eax = -1;
             break;
           }

         struct file *open_file = file_open (inode_open (dirent -> inode_sector));
    	  if (open_file != NULL)
            {
    	      struct file_info *f1 = create_files_struct (open_file);
              f1 -> dirent = dirent;
  	      list_push_back (&thread_current()->open_list, &f1->elem);
  	      f->eax = f1->file_descriptor;
  	    } 
          else 
            {
              f->eax = -1;
            }
      
        }
      break;
    }

  case SYS_WRITE:
    {
       f->eax = write (args[1], (void *) args[2], args[3]);
       break;
    }
  case SYS_CHDIR:
    {
      if (args[1] == NULL || !strcmp(args[1], "")) 
        {
          f->eax = false;
          break;
        } 
      if (t -> work_dir == NULL)
        t -> work_dir = dir_open_root ();
      if (find_path (t -> work_dir ,args[1], tail_name, next_dir))
        {
          struct inode *in;
          if(t -> work_dir -> inode != next_dir -> inode)
            dir_close (t -> work_dir);
          dir_lookup (next_dir, tail_name, &in);
          dir_close (next_dir);
          if (in != NULL)
            {
              t -> work_dir = dir_open (in);
              f -> eax = true;
              break;
            }
        }
      /* file not found or error path*/
      f -> eax = false;
      break;      
    }
  case SYS_MKDIR:
    {
      if (args[1] == NULL || !strcmp(args[1], "")) 
        {
          f->eax = false;
          break;
        }
       if(find_path(dir_open_current(), args[1], tail_name, next_dir))
         {
           struct inode *in;
           /* there is given directory in the path*/
           if (dir_lookup(next_dir, tail_name, &in))
             {
               f->eax = false;
               dir_close(next_dir);
               break;
             }
           else
             {
               /* since filesys_create will always lookup working directory
                  we need to change current working directroy into next_dir*/
               struct dir* temp = t-> work_dir;
               t -> work_dir = next_dir;
               f -> eax = filesys_create(tail_name, 512, IS_DIR);
               /* dive into the newly create directory, then add . and .. into it*/
               struct inode* new_dir_inode;
               dir_lookup(next_dir, tail_name, &new_dir_inode);
               struct dir* new_dir = dir_open (new_dir_inode);
               dir_add (next_dir, "..", inode_get_inumber(next_dir -> inode), IS_DIR);
               dir_add (new_dir, ".", inode_get_inumber(new_dir -> inode), IS_DIR);
               dir_close(new_dir);

               t -> work_dir = temp;
               dir_close(next_dir);
             }
         }
       else
         f -> eax = false;
       break;
    }
   case SYS_READDIR:
     {
       struct file_info *dir_fd = files_helper (args[1]);
       struct dir *dir_r = dir_open (inode_open(dir_fd -> dirent -> inode_sector));
       f -> eax = dir_readdir(dir_r, args[2]);
       break;
     }
   case SYS_ISDIR:
     {
       struct file_info *dir_fd = files_helper (args[1]);
       f -> eax = dir_fd -> dirent -> type == IS_DIR ? true : false;
       break;
     }
   case SYS_INUMBER:
     {
       struct file_info *dir_fd = files_helper (args[1]);
       f -> eax = dir_fd -> dirent -> inode_sector;
       break;
     }
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
        {
          int test = file_tell (curr_file->file);
          f->eax = test;

        }
       
  
      else if (args[0] == SYS_CLOSE) 
             {
               if (args[1] == 1 || args[1] == 2) 
                 {
                   f->eax = -1;
                 }
               else 
                {
                  if (curr_file->file_descriptor == thread_current()->fd_count) 
                    {
                      thread_current()->fd_count = thread_current()->fd_count - 1;
                    }
                  list_remove(&curr_file->elem);
    	          file_close(curr_file->file);
                  free(curr_file -> dirent);
    	          free(curr_file);
                 }
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
  if (!is_user_vaddr(buffer) || buffer == NULL) 
    {
      handle_exit(-1);
      thread_exit();
    } 
  else 
    {
      struct file_info *curr_file = files_helper (fd);
      if (curr_file == NULL || curr_file ->dirent ->type == IS_DIR)
        return -1;
      int ret = file_read(curr_file->file, buffer, length);
      return ret;
    }
}

int write (int fd, const void *buffer, unsigned length)
{
  uint32_t* pagedir = thread_current()->pagedir;

  if (fd == 1) /* if fd == STDOUT_FILENO */
  {
    putbuf(buffer,length);
    return length;
  }
  void* valid_adress = pagedir_get_page(pagedir, buffer);
  if (valid_adress == NULL) {
    handle_exit(-1);
    thread_exit();
  } 
  else 
    {
      struct file_info *curr_file = files_helper (fd);
      if (curr_file == NULL || curr_file -> dirent -> type == IS_DIR)
        return -1;
      int ret = file_write(curr_file->file, buffer, length);
      return ret;
    } 
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
create_files_struct(struct file *open_file) 
{
  struct file_info *f1 = malloc(sizeof(struct file_info));
  f1->reader_count = 0;
  f1->file_descriptor = find_fd();
  f1->file = open_file;
  f1->removed = false;
  return f1;
}

/*
  find file info for a given file discriptor*/
struct file_info* 
files_helper (int fd) 
{
  // Loop over the current file list
  struct list_elem *e;
  struct file_info *f;
  struct list open_list = thread_current()->open_list;
  int maxfd = thread_current()->fd_count;

  if (list_empty(&open_list)) 
    return NULL;
  


  for(e = list_begin (&open_list); e != list_end (&open_list);
      e = list_next (e))
    {
      f = list_entry (e, struct file_info, elem);
      if (f->file_descriptor == fd)
      {
        return f;
      }
      if (f->file_descriptor >= maxfd) {
        return NULL;
      }
    }
  return NULL;
}

/* handling exit of a process
   the function should set properly wait status, and sema_up for the process_wait
   and do the cleaning of all files, then print out the exit code at last*/
void 
handle_exit(int ret_val)
{
  tid_t now_tid = thread_current () -> tid;
  struct list_elem* e;
  for (e = list_begin (&wait_list); e!=list_end (&wait_list);
      e = list_next (e))
    {
      struct wait_status *status = list_entry(e,struct wait_status, elem);
      if (status -> child_pid == now_tid)
        {
          status -> return_val = ret_val;
          lock_acquire (&status -> ref_cnt_lock);
          status -> ref_cnt --;
          lock_release (&status -> ref_cnt_lock);
          sema_up(&status -> end_p);
        }
      else if (status -> parent_pid == now_tid)/* parent process exit*/
             {
               lock_acquire (&status -> ref_cnt_lock);
               status -> ref_cnt --;
               lock_release (&status -> ref_cnt_lock);
             }
     if(status -> ref_cnt == 0)
       {
         e = list_remove(e);
         free(status);
       }
     if(e == list_end(&wait_list))
       break;
     }
   clear_all_file();
   printf ("%s: exit(%d)\n", &thread_current ()->name, ret_val);
}

/*
  handle for execute syscalls
  how to dealing with tid is implemented in process_execute
  this handler only do some sanity check and get the tid*/

tid_t 
handle_exec(const char *cmd_line)
{
  if (cmd_line > PHYS_BASE) 
    return -1;
  tid_t child_tid;
  child_tid = process_execute (cmd_line); 
  return child_tid; 
}

/*
  close all the file open by a process
  this function is called on handle_exit */
void clear_all_file()
{
  struct thread *t = thread_current();
  struct list_elem* e;
  while (list_size (&(t -> open_list)))
  {
    e = list_pop_front (& (t -> open_list));
    struct file_info* fi = list_entry(e, struct file_info, elem);
    file_close (fi-> file);
    free(fi);
  }
}
