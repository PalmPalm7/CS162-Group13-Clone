#include "tests/lib.h"
#include "tests/main.h"
void 
test_main(void)
{
    CHECK(create("test.data",20),"create test.data");
    int f = open("test.data");
    msg("open test.data");
    seek(f,21);
    int pos = tell (f);
    msg ("current position is %d bytes",pos);
}