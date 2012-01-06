#===-- AMDGPUGenShaderPatterns.pl - TODO: Add brief description -------===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===----------------------------------------------------------------------===#
#
# TODO: Add full description
#
#===----------------------------------------------------------------------===#

use strict;
use warnings;

use AMDGPUConstants;

my $reg_prefix = $ARGV[0];

for (my $i = 0; $i < CONST_REG_COUNT * 4; $i++) {
  my $index = get_hw_index($i);
  my $chan = get_chan_str($i);
print <<STRING;
def : Pat <
  (int_AMDGPU_load_const $i),
  (f32 (MOV (f32 $reg_prefix$index\_$chan)))
>;
STRING
}
