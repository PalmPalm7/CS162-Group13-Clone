# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected([<<'EOF']);
(seek-and-tell) begin
(seek-and-tell) create test.data
(seek-and-tell) open test.data
(seek-and-tell) current position is 5 bytes
(seek-and-tell) end
seek-and-tell: exit(0)
EOF
pass;

