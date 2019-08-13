/* Test writing full blocks to disk without reading them first. */

#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BUF_SIZE 512
#define NUM_BLOCKS 64
static const char file_name[] = "file1";
char buf[BUF_SIZE];

void
test_main (void)
{
  int fd;
  int read_cnt1;
  int read_cnt2;
  int write_cnt1;
  int write_cnt2;
  size_t ret_val;
  CHECK (create (file_name, 0), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_cnt1 = buffer_readcnt();
  read_cnt2 = buffer_writecnt();
  random_bytes (buf, sizeof buf);
  msg ("writing 100KB (200 blocks) to \"%s\"", file_name);
  int i;
  for(i = 0; i < 200; i++)
  {
    ret_val = write (fd, buf, BUF_SIZE);
  }
  read_cnt2 = buffer_readcnt();
  write_cnt2 = buffer_writecnt();
  msg ("block_read is called %d times in 200 times of writes", read_cnt2 - read_cnt1);
  msg ("block_write is called %d times in 200 times of writes", write_cnt2 - write_cnt1);
  close (fd);
  msg ("close \"%s\"", file_name);
}