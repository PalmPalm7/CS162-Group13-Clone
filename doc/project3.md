Design Document for Project 3: File System
==========================================

## Group Members

* Josh Alexander <josh.alexander1315@berkeley.edu>
* Zuxin Li <lizx2019@berkeley.edu>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>

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

### Algorithms
The new members of the inode_disk struct will keep track of the different types of inode pointers, with the direct and singly indirect pointers used first.  `void place_file_in_inode()` will take in the amount of bytes the file needs access to and an existing inode array with indirect pointers potentially pointing to NULL if the file was not taking up a significant amount of space previously.  It will set up any new pointers the new file size requires and fill out the `inode_array` accordingly.  In order to accomodate any attempts to write past the already allocated memory, this function will be called if a thread attempts to write at a location past the previous file size.  Any read past the previously determined file size will automatically return NULL.  This design will not support sparse files.


### Synchronization


### Rationale
Not all of the direct and indirect pointers are necessary to satisfy the minimum project requirements.  One doubly indirect pointer accounts for a file size of 8MB exactly on its own.  However, in order to speed up the time it takes to access files much smaller than 8MB, 12 direct and 1 indirect pointers were included. Supporting sparse files was considered, but it was eventually determined to be an unnecessary coding complication.

