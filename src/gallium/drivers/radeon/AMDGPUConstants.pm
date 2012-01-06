#===-- AMDGPUConstants.pm - TODO: Add brief description -------===#
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

package AMDGPUConstants;

use base 'Exporter';

use constant CONST_REG_COUNT => 256;
use constant TEMP_REG_COUNT => 128;

our @EXPORT = ('TEMP_REG_COUNT', 'CONST_REG_COUNT', 'get_hw_index', 'get_chan_str');

sub get_hw_index {
  my ($index) = @_;
  return int($index / 4);
}

sub get_chan_str {
  my ($index) = @_;
  my $chan = $index % 4;
  if ($chan == 0 )  {
    return 'X';
  } elsif ($chan == 1) {
    return 'Y';
  } elsif ($chan == 2) {
    return 'Z';
  } elsif ($chan == 3) {
    return 'W';
  } else {
    die("Unknown chan value: $chan");
  }
}

1;
