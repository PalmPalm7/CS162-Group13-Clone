#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"
#include "filesys/off_t.h"
#include "threads/thread.h"
/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 14


/*macros to specifies directory and files*/
#define IS_DIR 1
#define IS_REG 0

struct inode;

/* A directory. */
struct dir
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
    bool deny_write;
  };

/* A single directory entry. */
struct dir_entry
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    struct dir_entry* parent_dir;        /*parent dirctory entry, only used in memory*/
    int type;                           /* type of the directory entry*/
    bool in_use;                        /* In use or free? */
  };

/* Opening and closing directories. */
bool dir_create (block_sector_t sector, size_t entry_cnt);
struct dir *dir_open (struct inode *);
struct dir *dir_open_root (void);
struct dir *dir_reopen (struct dir *);
void dir_close (struct dir *);
struct inode *dir_get_inode (struct dir *);

/* Reading and writing. */
bool dir_lookup (const struct dir *, const char *name, struct inode **);
bool dir_add (struct dir *, const char *name, block_sector_t, int type);
bool dir_remove (struct dir *, const char *name);
bool dir_readdir (struct dir *, char name[NAME_MAX + 1]);

/*helper function to iteratively find long path*/
bool find_path(const struct dir *dir, const char *name,
               char *tail_name, struct dir *file_dir);

struct dir_entry* dir_getdirent(const struct dir *dir, const char *name);
struct dir* dir_open_current();
    
#endif /* filesys/directory.h */
