#include "tests/lib.h"
#include "tests/main.h"
void 
test_main(void)
{
    CHECK(create("test.data",10),"create test.data");
    int fd = open("test.data");
    msg("open test.data");
    seek(fd,5);
    int pos = tell (fd);
    msg ("current position is %d bytes",pos);
}