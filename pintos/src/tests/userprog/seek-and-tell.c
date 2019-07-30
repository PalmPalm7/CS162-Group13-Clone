#include "tests/lib.h"
#include "tests/main.h"
void 
test_main(void)
{
    CHECK(create("test.data",10),"create test.data");
    int fd = open("test.data");
    msg("open test.data");
    // int size = filesize(fd);
    // msg("file is %d bytes",size);
    seek(fd,5);
    int pos = tell (fd);
    msg ("current postion is %d bytes",pos);
    seek(fd,11);
    pos = tell (fd);
    msg("current postion is %d bytes",pos);
}