Design Document for Project 3: File System
==========================================

## Group Members

* Josh Alexander <josh.alexander1315@berkeley.edu>
* Zuxin Li <lizx2019@berkeley.edu>
* Handi Xie <hxie14@berkeley.edu>
* Wenzheng Guo <guowz19@berkeley.edu>

## Task 1: Buffer cache

### Data Structures and Functions

The global variable `cache` caches the data in disk. The data structures involving the variable are:


```
struct cache_entry
  {
    block_sector_t sector;
    int ref_count;
    bool write;//if the entry is modify it should be true else false
    char data[BLOCK_SECTOR_SIZE];
  };

struct block_cache
  {
    int entry_num;
    struct cache_entry cache_entrys[CACHE_SIZE];
  };

struct block_cache cache;
```
`cache_entry` is the entry of the cache that has the member variable `sector` to store the index of the sector, `ref_count` to record how many times it has been used before swapping out, and `data` for the cache data.

`block_cache` has `entry_num` to record the number of the cache entry and the `cache_entrys` array to record the enry.

More cache related functions are:

```
void cache_init();// init the cache setting the entry_num and ref_count to 0
void cache_write(const block_sector_t sector,char data[]);
bool cache_read(const block_sector_t sector, char data[]);
void cache_sync();//flush every cache_entry.data to disk

/* a cached version of block read and write*/
void cached_block_read (struct block *, block_sector_t, void *);
void cached_block_write (struct block *, block_sector_t, void *);
```

`cache_write() ` takes in two arguments, sector and data. Ifthe sector is found in the cache, only the data in the cache will be changed and `write` will be set to true.  If a process accesses disk, it calls the function `block_read`  or  `block_write` which meant it failed to find a valid entry in the cache. `block_read ` will return the data read from the disk to be put, alongside the disk_index, into the argument. `cache_write()` will swap the cache entry according to the LFU replacement policy or add one valid entry if the cache is not full.  While swapping, the value of `write` will be checked. If it is true, the data of the cache entry will be written by calling `blocl_write`.

`cache_read` will be called every time a process attempts to access disk. The process will always check the cache before accessing disk. When it is called, it will increment the corresponeding `ref_count` of the entry.

`void cache_sync()` traverses every entry in the cache and calls the function `block_write` to flush the data of entry into the disk.

`cache_read` checks the cache before any disk operation. If the data is found on the cache, read or modify the cache.  If not, add `cache_write` to swap the entry of the cache.

The following functions will be modified since they all involve disk operations:

```
inode_create (block_sector_t sector, off_t length);
off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size,off_t offset);
struct inode *inode_open (block_sector_t sector);
off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset);
void fsutil_cat (char **argv);
void fsutil_append (char **argv);
void fsutil_extract (char **argv UNUSED);
```
### Algorithms

The LFU algorithm is used as the replacement policy. A member variable `ref_count` stores the how many time it has been used. Every time the swapping is done, all of the entries in the cache are traversed to find the the entry with the smallest `ref_count` to swap. 




### Synchronization
#### Synchronize the cache
Every read or write to the cache the functions `cache_read` and `cache_write` will be used.  They call `intr_disable` at the beginning of the function and set the interrupt level to its old state to make sure reading and writing to the cache is atomic. Cache operation is atomic so if one process is actively reading or writing to the cache other process have no chance to write anything into the cache triggering swapping.  The atomic nature of cache operations also prevent processes from accessing the cache while a block is being swapped out.  This also prevents other processes from accessing the block before it is fully loaded.

#### Coalesce multiple writes
The cache implemented is a write-back cache so the cache only writes back when a block containing the new information is swapped out. This means many writes can be completed on this cache with only one disk operation. Furthermore, since the cache operations are atomic, the synchroniztion issue of multiple writes occuring simutaneously will never arise.

#### Rationale 
The LFU replacement policy was chosen because it is easy to implement and the number of cache entries is not large while cache traversal is affordable.





## Task 2: Extensible Files
### Data Structures
The following will be added to the `struct inode_disk`:

```
struct inode_disk
{
  block_sector_t direct[12]; /* 12 direct pointers */
  block_sector_t indirect; /* a singly indirect pointer */
  block_sector_t doubly_indirect; /* a doubly indirect pointer */
  uint32_t unused[113]; /* Not used. */
};
```

Some new functions will be added:
```
void place_file_in_inode(int file_size, block_sector_t* inode_array); /* fill inode_array according to file size*/ 

void parse_indirect(block_sector_t indirect, block_sector_t* indirects_in_block) /*parse indirect in direct pointer`s array*/
```

And several functions will be modified:

```
/* split the data in buffer into different blocks*/ 

off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset);

/*read from blocks and indirect blocks, then put them together in the buffer*/

off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)

```

###### free-map.c

```
static struct lock *map_lock;

/* Allocates cnt sectors and stores them in *sectors */
bool map_allocate (size_t cnt, block_sector_t *sectors);
```

### Algorithms
The new members of the inode_disk struct will keep track of the different types of inode pointers, with the direct and singly indirect pointers used first.  `void place_file_in_inode()` will take in the amount of bytes the file needs access to and an existing inode array with indirect pointers potentially pointing to NULL if the file was not taking up a significant amount of space previously.  It will set up any new pointers the new file size requires and fill out the `inode_array` accordingly.  In order to accomodate any attempts to write past the already allocated memory, this function will be called if a thread attempts to write at a location past the previous file size.  Any read past the previously determined file size will automatically return NULL.  This function will store which disk blocks are newly added in case the filesystem is unable to allocate new ones in order to be able to roll back to the previous state.  This design will not support sparse files.


### Synchronization
There are two methods we thought up about. One is to put lock directly on everysingle blocks of memory, which feels redundant. Therefore, we decided to put a lock on the freemap and use `free_map_allocate_nc ()` to ensure that whether all blocks are allocated.


### Rationale
Not all of the direct and indirect pointers are necessary to satisfy the minimum project requirements.  One doubly indirect pointer accounts for a file size of 8MB exactly on its own.  However, in order to speed up the time it takes to access files much smaller than 8MB, 12 direct and 1 indirect pointers were included. Supporting sparse files was considered, but it was eventually determined to be an unnecessary coding complication.


## Addtional Question
### write  behind
Interrupts are used to add the features of write behind caches. A timer is set to an appropriate number and mode which will call an interrupt handler to handle the interrupt. Every time the timer generates an interrupt signal, the handler will execute the context switching, disable interrupts, and call the function `sync` to write all of the dirty blocks to the disk. There are not any problems with respect to synchronization since it simply writes something to the same disk sector which is not accessed by any other threads. No locks need to be acquired.

### read ahead
The cache should be transparent to the user. This feature is added inside the file system and doesn't change the syscall `read`. These functions do need to be adjusted.

```
void cache_write(const block_sector_t sector,char data[]);
bool cache_read(const block_sector_t sector, char data[]);

void cache_write(const block_sector_t sector,char data[],struct inode inode);
bool cache_read(const block_sector_t sector, char data[],struct inode inode);
```
The `inode` argument is added to help find the next block by traversing all of the blocks that inode points to and caching the next few blocks in the cache.
