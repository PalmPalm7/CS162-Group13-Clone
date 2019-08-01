#Buffer cache

##Data Structures and functions

we use global variable `cache` to cache the data in disk, the data structures of the variable are followings:


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
`cache_entry` is the entry of cache which has member variable `sector` to store the index of sector  `ref_count` to record how many times it is used before swapping out and data.

`block_cache` has `entry_num` to record the number cache entry  and cache_entrys array to record the enry.

And  a set of functions is:

```
void cache_init();// init the cache setting the entry_num and ref_count to 0
void cache_write(const block_sector_t sector,char data[]);
bool cache_read(const block_sector_t sector, char data[]);
void cache_sync();//flush every cache_entry.data to disk

```

`cache_write() ` takes the argument: sector and data. If we find the sector in cache we only change the data in cache and set `write` to true.  Every time  if a process access disk it calls the function `block_read`  or  `block_write` which means it fails to find valid entry in cache ,We need to cache it and `block_read ` return the data which read from the disk so we put the data and disk_index into the argument and `cache_write()` swap the cache entry according to our  LFU replacement policy or just add one valid entry if the cache is not full.When we do the swapping we first check the value of `write` if it is true write the data of cache entry by calling `blocl_write` or if it is false we do nothing.

`cache_read` will be called every time if a process attempt to access disk. So process will always check the cache before access the disk.When it is called it will plus one on the corresponed ref_count of entry.

`void cache_sync()` traversal every entry in the cache and call the function `block_write` to flush the data of entry into the disk.

So we add the `cache_read` to check cache before any disk operation if found then just read or modify the cache  and add `cache_write` to swap the entry of cache if not found.

We need to modify the following function because of these functions all involve disk operation

```
inode_create (block_sector_t sector, off_t length);
off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size,off_t offset);
struct inode *inode_open (block_sector_t sector);
off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset);
void fsutil_cat (char **argv);
void fsutil_append (char **argv);
void fsutil_extract (char **argv UNUSED);
```
##Algorithm

We use LFU algorithm as our replacement policy. We have a member variable `ref_count` to store the how many time it is used. And every time we do the swapping ,we traversal all the entry in the cache and find the the entry with minimum ref_count to swap. 




##Synchronization
###Synchronize the cache
Everytime we read or write the cache we use the function `cache_read` and `cache_write` which we call the `intr_disable` at the beginning of the function and set the intr level to its old to make sure our reading and writing cache is atomically.
The these questions could be solved.

1. Because cache operation is atomically so if one process is actively reading or writing the cache other process have no chance to write any thing into cache triggering swapping.
2.  Because cache operation is atomically when one block is swapping out of cache the other process could access the cache.
3. How  other processes are prevented from accessing the block before it is fully loaded? Same like aforemetioned reasons.

###coalesce multiple writes
The cache we implement is write-back cache so we only write the cache back when it is swapped out which means we could do many writes on this cache with only one disk operation.What's more because the cache operation is atomic , the synchroniztion issue of multiple writes occuring simutaneously is solved.

###Rationale 
We choose the LFU because it is easy to implement and the number of cache entry is not large and traversal the cache is affroaable.


#Addtional Question
##write  behind
We use interrupt to add the featuresof writing behind.We set up a timer to a proper number and mode and register a interrupt handler to handle this interrupt. Everytime the timer generate a interrupt signal ,the handler will execute the context switching , disable interrupt and call the function `sync` to write all the dirty block to the disk.There is not any problems of synchronization because what we do is just write something to the some disk sector which is not accessed by any threads. And we do not need to acquire any locks too.
##read ahead
The cache should transparent to the user.So we add this features inside the file system and don't change the syscall  `read`.Now we need to modify the 

```
void cache_write(const block_sector_t sector,char data[]);
bool cache_read(const block_sector_t sector, char data[]);
```
a little bit ,we add the argument 

```
void cache_write(const block_sector_t sector,char data[],struct inode inode);
bool cache_read(const block_sector_t sector, char data[],struct inode inode);
```
`inode` to help us finding the next  block by  traversing the all the block that inode point to and cache the next few block in cache.
