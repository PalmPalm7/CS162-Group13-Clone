# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
my ($a) = random_bytes (2048);
my ($b) = random_bytes (2048);
check_archive ({});
pass;
