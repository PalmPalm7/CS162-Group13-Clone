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

We use LFU algorithm as our replacement policy. We have a member variable `ref_count` to store the how many time it is used. And every time we do the swapping ,we traversal all the entry in the cache and find the the entry with minimum ref_count to swap. 




### Synchronization
#### Synchronize the cache
Everytime we read or write the cache we use the function `cache_read` and `cache_write` which we call the `intr_disable` at the beginning of the function and set the intr level to its old to make sure our reading and writing cache is atomically.
The these questions could be solved.

1. Because cache operation is atomically so if one process is actively reading or writing the cache other process have no chance to write any thing into cache triggering swapping.
2.  Because cache operation is atomically when one block is swapping out of cache the other process could access the cache.
3. How  other processes are prevented from accessing the block before it is fully loaded? Same like aforemetioned reasons.

#### Coalesce multiple writes
The cache we implement is write-back cache so we only write the cache back when it is swapped out which means we could do many writes on this cache with only one disk operation.What's more because the cache operation is atomic , the synchroniztion issue of multiple writes occuring simutaneously is solved.

#### Rationale 
We choose the LFU because it is easy to implement and the number of cache entry is not large and traversal the cache is affordable.





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
The new members of the inode_disk struct will keep track of the different types of inode pointers, with the direct and singly indirect pointers used first.  `void place_file_in_inode()` will take in the amount of bytes the file needs access to and an existing inode array with indirect pointers potentially pointing to NULL if the file was not taking up a significant amount of space previously.  It will set up any new pointers the new file size requires and fill out the `inode_array` accordingly.  In order to accomodate any attempts to write past the already allocated memory, this function will be called if a thread attempts to write at a location past the previous file size.  Any read past the previously determined file size will automatically return NULL.  This design will not support sparse files.


### Synchronization
There are two methods we thought up about. One is to put lock directly on everysingle blocks of memory, which feels redundant. Therefore, we decided to put a lock on the freemap and use `free_map_allocate_nc ()` to ensure that whether all blocks are allocated.


### Rationale
Not all of the direct and indirect pointers are necessary to satisfy the minimum project requirements.  One doubly indirect pointer accounts for a file size of 8MB exactly on its own.  However, in order to speed up the time it takes to access files much smaller than 8MB, 12 direct and 1 indirect pointers were included. Supporting sparse files was considered, but it was eventually determined to be an unnecessary coding complication.


## Addtional Question
### write  behind
We use interrupt to add the featuresof writing behind.We set up a timer to a proper number and mode and register a interrupt handler to handle this interrupt. Everytime the timer generate a interrupt signal ,the handler will execute the context switching , disable interrupt and call the function `sync` to write all the dirty block to the disk.There is not any problems of synchronization because what we do is just write something to the some disk sector which is not accessed by any threads. And we do not need to acquire any locks too.
### read ahead
The cache should transparent to the user. So we add this features inside the file system and don't change the syscall  `read`. Now we need to modify the 

```
void cache_write(const block_sector_t sector,char data[]);
bool cache_read(const block_sector_t sector, char data[]);
```
a little bit, we add the argument 

```
void cache_write(const block_sector_t sector,char data[],struct inode inode);
bool cache_read(const block_sector_t sector, char data[],struct inode inode);
```
`inode` to help us finding the next  block by  traversing the all the block that inode point to and cache the next few block in cache.
