# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(my-test-2) begin
(my-test-2) create "file2"
(my-test-2) open "file2"
(my-test-2) writing 64KB to "file2"
(my-test-2) block_read is called 36 times in 64 times of writes
(my-test-2) block_write is called 545 times in 64 times of writes
(my-test-2) close "file2"
(my-test-2) end

EOF
pass;
