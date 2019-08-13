#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Testing Git 2*/
/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define CACHE_SIZE 32
/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t direct[124]; /* 12 direct pointers */
    block_sector_t indirect; /* a singly indirect pointer */
    block_sector_t doubly_indirect; /* a doubly indirect pointer */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    // uint32_t unused[125];               /* Not used. */
  };


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    // struct inode_disk data;             /* Inode content. */
  };

struct cache_entry
  {
    block_sector_t sector;
    struct lock lock;
    int ref_count;
    bool write;
    char data[BLOCK_SECTOR_SIZE];
  };

struct block_cache
  {
    struct lock entry_num_lock;
    int entry_num;
    struct cache_entry cache_entrys[CACHE_SIZE];
  };

struct block_cache cache;

block_sector_t read_sector (block_sector_t sector, off_t offset);

void cache_write(struct block *block, const block_sector_t sector, void *data);


void cache_init()
{
  cache.entry_num = 0;
  lock_init (&(cache.entry_num_lock));
  int i = 0;
  for( i = 0; i < CACHE_SIZE; i++)
    {
      cache.cache_entrys[i].sector = -1;
      cache.cache_entrys[i].ref_count = 0;
      lock_init(&(cache.cache_entrys[i].lock));
      cache.cache_entrys[i].write = false;
    }
}


bool cache_read(struct block *block, const block_sector_t sector, void *data)
{
  (char*)data;
  int i = 0;
  bool found = false;
  lock_acquire(&cache.entry_num_lock);
  for ( i = 0; i < cache.entry_num; i++)
    {
      lock_acquire(&(cache.cache_entrys[i].lock));
      if (cache.cache_entrys[i].sector == sector )
      {
        cache.cache_entrys[i].ref_count = 0;
        memcpy(data, cache.cache_entrys[i].data, BLOCK_SECTOR_SIZE);
        found = true;
      }
      else
      {
        cache.cache_entrys[i].ref_count+=1;
      }
      lock_release(&(cache.cache_entrys[i].lock));
    }
  lock_release(&cache.entry_num_lock);
  if (found)
  {
    return found;
  }

  lock_acquire(&cache.entry_num_lock);
  for (i = 0; i < cache.entry_num; i++)
    {
      lock_acquire(&(cache.cache_entrys[i].lock));
      cache.cache_entrys[i].ref_count -= 1;
      lock_release(&(cache.cache_entrys[i].lock));
    }
  lock_release(&cache.entry_num_lock);


  block_read (fs_device,sector,(void*)data);

  cache_write (fs_device,sector,data);
  return found;
}

void cache_read_bytes(struct block *block, const block_sector_t sector_idx, void *buff, int offset, int size)
{
  uint8_t *bounce = NULL;
  bounce = malloc(BLOCK_SECTOR_SIZE);
  if(bounce == NULL)
  {
    bounce = malloc(BLOCK_SECTOR_SIZE);
    if (bounce == NULL)
      return;
  }
  cache_read (block, sector_idx, bounce);
  memcpy(buff, bounce + offset, size);
  free(bounce);
}

void cache_write(struct block *block, const block_sector_t sector, void *data)
{
  
  (char*)data;
  int i = 0; 
  int replace = 0;
  int max_index = 0, max = 0;
  int found = 0;
  
  lock_acquire(&cache.entry_num_lock);
  for ( i = 0; i < cache.entry_num; i++)
  {
    lock_acquire (&(cache.cache_entrys[i].lock));
    if (cache.cache_entrys[i].sector == sector )
    {
      cache.cache_entrys[i].ref_count = 0;
      found = 1;
      replace = i;
    }
    else
    {
      cache.cache_entrys[i].ref_count+=1;
      if (cache.cache_entrys[i].ref_count > max)
      {
        max = cache.cache_entrys[i].ref_count;
        max_index = i;
      }
    }
    lock_release (&(cache.cache_entrys[i].lock));
  }
  lock_release(&cache.entry_num_lock);
  if (found)
  {
    lock_acquire(&(cache.cache_entrys[replace].lock));
    cache.cache_entrys[replace].sector = sector;
    cache.cache_entrys[replace].ref_count = 0;
    cache.cache_entrys[replace].write = true;
    memcpy(cache.cache_entrys[replace].data, data, BLOCK_SECTOR_SIZE);
    lock_release(&(cache.cache_entrys[replace].lock));
    return;
  }
  lock_acquire(&(cache.entry_num_lock));
  if (cache.entry_num < CACHE_SIZE)
  {
    lock_acquire(&(cache.cache_entrys[cache.entry_num].lock));
    cache.cache_entrys[cache.entry_num].sector = sector;
    cache.cache_entrys[cache.entry_num].ref_count = 0;
    cache.cache_entrys[cache.entry_num].write = true;
    memcpy(cache.cache_entrys[cache.entry_num].data, data, BLOCK_SECTOR_SIZE);
    lock_release(&(cache.cache_entrys[cache.entry_num].lock));
    cache.entry_num+=1;
  }
  lock_release(&(cache.entry_num_lock));
  if (cache.entry_num < CACHE_SIZE)
  return;

  lock_acquire(&(cache.entry_num_lock));
  if (cache.entry_num == CACHE_SIZE)
  {
    lock_acquire(&(cache.cache_entrys[max_index].lock));
    if (cache.cache_entrys[max_index].write)
    {
      block_write(fs_device,cache.cache_entrys[max_index].sector, \
      cache.cache_entrys[max_index].data);
    }
    cache.cache_entrys[max_index].sector = sector;
    cache.cache_entrys[max_index].ref_count = 0;
    cache.cache_entrys[max_index].write = true;
    memcpy(cache.cache_entrys[max_index].data, data, BLOCK_SECTOR_SIZE);
    lock_release(&(cache.cache_entrys[max_index].lock));
  }
  lock_release(&(cache.entry_num_lock));
}

bool cache_write_bytes(struct block *block, const block_sector_t sector_idx, void *buffer, int sector_ofs, int chunk_size,int sector_left)
{
  uint8_t *bounce = NULL;
  bounce = malloc(BLOCK_SECTOR_SIZE);
  if(bounce == NULL);
  {
    bounce = malloc(BLOCK_SECTOR_SIZE);
    if (bounce == NULL)
    return false;
  }
    
  if(sector_ofs > 0 || chunk_size < sector_left)
    cache_read(fs_device,sector_idx,bounce);
  else
    memset (bounce, 0, BLOCK_SECTOR_SIZE);
  memcpy (bounce + sector_ofs, buffer, chunk_size);
  cache_write (fs_device,sector_idx,bounce);
  free(bounce);
  return true;
}





void cache_sync()
{
  int i = 0;
  lock_acquire(&cache.entry_num_lock);
  for (i = 0; i < cache.entry_num ; i++)
  {
    lock_acquire(&(cache.cache_entrys[i].lock));
    block_write(fs_device,cache.cache_entrys[i].sector,cache.cache_entrys[i].data);
    lock_release(&(cache.cache_entrys[i].lock));
  }
  lock_release(&cache.entry_num_lock);
};


/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
//-> doesn't work, need to actually read the contents of that sector
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  struct inode_disk disk_inode;
  cache_read(fs_device, inode->sector, &disk_inode);
  if (pos < disk_inode.length)
  {
    if (pos < 124 * BLOCK_SECTOR_SIZE)
    {  
      int direct_num = pos / BLOCK_SECTOR_SIZE;
      block_sector_t rv = disk_inode.direct[direct_num];
      return rv;
    }
    else if (pos < (124 + 128) * BLOCK_SECTOR_SIZE)
    {
      int indirect_num = pos / BLOCK_SECTOR_SIZE - 124;
      block_sector_t rv = read_sector(disk_inode.indirect, indirect_num); 
      return rv;
    }
    else 
    {
      int doubly_indirect_num = (pos / BLOCK_SECTOR_SIZE - 124 - 128) / 128;
      int inner_indirect_num = pos / BLOCK_SECTOR_SIZE - doubly_indirect_num;
      block_sector_t inner_indirect_sector = read_sector(disk_inode.doubly_indirect, doubly_indirect_num);
      return read_sector(inner_indirect_sector, inner_indirect_num);
    }
  }
  else
    return -1;
}

/* Read the sector located at an offset.  For indirect and doubly indirect pointers only */ 

block_sector_t
read_sector (block_sector_t sector, off_t offset)
{
  block_sector_t buffer[1];
  off_t bytes_read = 0;
  block_sector_t *bounce = NULL;

  /* Disk sector to read, starting byte offset within sector. */
  block_sector_t sector_idx = sector;
  int sector_ofs = offset;


  /* Number of bytes to actually copy out of this sector. */
  int chunk_size = 4;

  /* Read sector into bounce buffer, then partially copy
      into caller's buffer. */
  if (bounce == NULL)
    {
      bounce = malloc (BLOCK_SECTOR_SIZE);
    }
  cache_read (fs_device, sector_idx, bounce);
  memcpy (buffer, bounce + sector_ofs, 4);

  free (bounce);
  block_sector_t new_sector = buffer[0];

  return new_sector;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
  cache_init();
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      int sector_count = (int) length / BLOCK_SECTOR_SIZE;
      if (sector_count * BLOCK_SECTOR_SIZE < length) 
        ++sector_count;
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;

      // Allocate direct inodes
      static char zeros[BLOCK_SECTOR_SIZE];
      memset(zeros, 0, BLOCK_SECTOR_SIZE);

      int j;
      for (j = 0; j < sector_count && j < 124; j++) 
        {
        if (free_map_allocate (1, &disk_inode->direct[j]))
          {
            cache_write (fs_device, disk_inode->direct[j], zeros);
            success = true;
          }
        }

      // Allocate indirect inode
      if (sector_count > 124)
        {
          if (free_map_allocate (1, &disk_inode->indirect))
          {

            block_sector_t indirect_blocks[128];

            // Fill indirect inode
            for (j = 0; j < 128 && j < sector_count - 124; j++) 
              {
              if (free_map_allocate (1, &indirect_blocks[j])) 
                {
                  cache_write (fs_device, indirect_blocks[j], zeros);
                }
              }

            // Cache indirect inode
            cache_write (fs_device, disk_inode->indirect, indirect_blocks);
            success = true;
          }
        }

      // Allocate doubly indirect inode
      if (sector_count > 124 + 128)
        {
          if (free_map_allocate (1, &disk_inode->doubly_indirect))
          {

            block_sector_t doubly_indirect_blocks[128];

            int sectors_left = sector_count - 124 - 128;

            // Fill doubly indirect inode
            for (j = 0; j < 128 && sectors_left > 0; j++) 
              {
              if (free_map_allocate (1, &doubly_indirect_blocks[j])) 
                {
                  block_sector_t inner_doubly_indirect_blocks[128];

                  // Fill inner doubly indirect inode
                  int k;
                  for (k = 0; k < 128 && sectors_left > 0; k++) 
                    {
                    if (free_map_allocate (1, &inner_doubly_indirect_blocks[k])) 
                      {
                        cache_write (fs_device, inner_doubly_indirect_blocks[k], zeros);
                      }
                    sectors_left -= 1;
                    }

                  // Cache inner doubly indirect inode
                  cache_write (fs_device, doubly_indirect_blocks[k], inner_doubly_indirect_blocks);
                }
              }

            // Cache doubly indirect inode
              cache_write (fs_device, disk_inode->doubly_indirect, doubly_indirect_blocks);
            success = true;
          }
        }
      if (sector_count > 0) 
      {
        cache_write (fs_device, sector, disk_inode);   
      }
      else
      {
        cache_write (fs_device, sector, disk_inode);   
        success = true;
      }
      free (disk_inode);
    }
  return success;
}


/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */

struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          // free_map_release (inode->data.direct[0],
                            // bytes_to_sectors (inode->data.length));
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  struct inode_disk disk_inode;

  cache_read(fs_device, inode->sector, &disk_inode);

  if (offset >= disk_inode.length)
  {
    static char zeros[BLOCK_SECTOR_SIZE];
    memset(zeros, 0, BLOCK_SECTOR_SIZE);
    memcpy (buffer + bytes_read, zeros, size);
  }


  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          cache_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          cache_read_bytes(fs_device, sector_idx, \
          buffer + bytes_read, sector_ofs,chunk_size);





        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Adds necessary pointers to inode to make it have the correct new file size*/

void
add_inode (struct inode_disk *disk_inode, off_t new_length) 
{
  int old_length = disk_inode->length;
  int total_sectors = new_length / BLOCK_SECTOR_SIZE;
  if (total_sectors * BLOCK_SECTOR_SIZE < new_length) 
    ++total_sectors;

  int current_sectors = old_length / BLOCK_SECTOR_SIZE;
  if (current_sectors * BLOCK_SECTOR_SIZE < old_length) 
    ++current_sectors;  

  int new_sectors = total_sectors - current_sectors;


  static char zeros[BLOCK_SECTOR_SIZE];
  memset(zeros, 0, BLOCK_SECTOR_SIZE);

  if (new_sectors <= 0) 
  {
    if (new_length > old_length)
      old_length = new_length;
    disk_inode->length = old_length;
    return;
  }

  if (current_sectors < 124)
    {
    if (total_sectors > 124)
      new_sectors -= 124 - current_sectors;
    else 
      new_sectors = 0;
      
    // Allocate direct inodes
    int j;
    for (j = current_sectors; j < total_sectors && j < 124; j++) 
      {
      if (free_map_allocate (1, &disk_inode->direct[j]))
        {
          cache_write (fs_device, disk_inode->direct[j], zeros);
        }
      current_sectors = current_sectors + 1;
      }
    }

  if (new_sectors > 0 && current_sectors < 124 + 128)
    {
      if (total_sectors > 124 + 128)
        new_sectors -= 128 - current_sectors;
      else 
        new_sectors = 0;
      block_sector_t indirect_blocks[128];

      if (current_sectors == 124)
      {
        free_map_allocate (1, &disk_inode->indirect);
      }
      else
      {
        cache_read(fs_device, disk_inode->indirect, indirect_blocks);
      }

      // Fill indirect inode
      int j = 0;
      for (j = current_sectors; j < 128 + 124 && j < total_sectors; j++) 
        {
        if (free_map_allocate (1, &indirect_blocks[j - 124])) 
          {
            cache_write (fs_device, indirect_blocks[j - 124], zeros);
          }
        current_sectors += 1;
        }


      // Cache indirect inode
      cache_write (fs_device, disk_inode->indirect, indirect_blocks);
    }

  if (new_sectors > 0)
    {

            block_sector_t doubly_indirect_blocks[128];

            if (current_sectors == 124 + 128)
            {
              free_map_allocate (1, &disk_inode->doubly_indirect);
              cache_read(fs_device, zeros, doubly_indirect_blocks);
            }
            else
            {
              cache_read(fs_device, disk_inode->doubly_indirect, doubly_indirect_blocks);
            }

            int outer_start = (current_sectors - 124 - 128) / 128;
            int inner_start = (current_sectors - 124 - 128) % 128;

            // Fill doubly indirect inode
            int j = 0;
            for (j = outer_start; j < 128 && new_sectors > 0; j++) 
              {

                block_sector_t inner_doubly_indirect_blocks[128];

                if (current_sectors == 124 + 128 + 128 * j)
                {
                  free_map_allocate (1, &doubly_indirect_blocks[j]);                
                }
                else
                {
                  cache_read(fs_device, doubly_indirect_blocks[j], inner_doubly_indirect_blocks);
                }

                  // Fill inner doubly indirect inode
                  int k;
                  for (k = inner_start; k < 128 && new_sectors > 0; k++) 
                    {
                    inner_start = 0;
                    if (free_map_allocate (1, &inner_doubly_indirect_blocks[k])) 
                      {
                        cache_write (fs_device, inner_doubly_indirect_blocks[k], zeros);
                      }
                    new_sectors -= 1;
                    }

                  // Cache inner doubly indirect inode
                  cache_write (fs_device, doubly_indirect_blocks[j], inner_doubly_indirect_blocks);
              }

            // Cache doubly indirect inode
              cache_write (fs_device, disk_inode->doubly_indirect, doubly_indirect_blocks);
          
    }
  disk_inode->length = new_length;

  return new_length;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  if (inode->deny_write_cnt)
    return 0;

  struct inode_disk disk_inode;

  cache_read(fs_device, inode->sector, &disk_inode);

  add_inode(&disk_inode, size + offset);
  cache_write (fs_device, inode->sector, &disk_inode);

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          cache_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else
        {
          /* We need a bounce buffer. */
          // if (bounce == NULL)
          //   {
          //     bounce = malloc (BLOCK_SECTOR_SIZE);
          //     if (bounce == NULL)
          //       break;
          //   }
          ASSERT(cache_write_bytes (fs_device,sector_idx,buffer+bytes_written,sector_ofs,chunk_size,sector_left));
          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          // if (sector_ofs > 0 || chunk_size < sector_left)
          //   cache_read (fs_device, sector_idx, bounce);
          // else
          //   memset (bounce, 0, BLOCK_SECTOR_SIZE);
          // memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          // cache_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  cache_write(fs_device, inode->sector, &disk_inode);
  free(bounce);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  struct inode_disk disk_inode;
  cache_read(fs_device, inode->sector, &disk_inode);
  return disk_inode.length;
}

bool inode_removed(struct inode* inode)
  {
    return inode -> removed;
  }
