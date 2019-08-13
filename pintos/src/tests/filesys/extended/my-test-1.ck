# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(my-test-1) begin
(my-test-1) create "file1"
(my-test-1) open "file1"
(my-test-1) writing 100KB (200 blocks) to "file1"
(my-test-1) block_read is called 0 times in 200 times of writes
(my-test-1) close "file1"
(my-test-1) end

EOF
pass;
