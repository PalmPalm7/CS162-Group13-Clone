/* Test writing full blocks to disk without reading them first. */

#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BUF_SIZE 32
#define NUM_BLOCKS 64
static const char file_name[] = "file2";
char buf[BUF_SIZE];

void
test_main (void)
{
  int fd;
  int read_cnt;
  int write_cnt;
  CHECK (create (file_name, 0), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_cnt = buffer_readcnt();
  write_cnt = buffer_writecnt();
  random_bytes (buf, sizeof buf);
  msg ("writing 64KB to \"%s\"", file_name);
  int i;
  for(i = 0; i < 64; i++)
  {
    write (fd, buf, BUF_SIZE);
  }
  read_cnt = buffer_readcnt();
  write_cnt = buffer_writecnt();
  msg ("block_read is called %d times in 64 times of writes", read_cnt);
  msg ("block_write is called %d times in 64 times of writes", write_cnt);
  msg ("close \"%s\"", file_name);
  close (fd);
  remove (file_name);
}