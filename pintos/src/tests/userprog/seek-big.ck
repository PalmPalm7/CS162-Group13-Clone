# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected([<<'EOF']);
(seek-big) begin
(seek-big) create test.data
(seek-big) open test.data
(seek-big) current position is 21 bytes
(seek-big) end
seek-big: exit(0)
EOF
pass;

