#===-- AMDGPUGenInstrEnums.pl - TODO: Add brief description -------===#
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

use warnings;
use strict;

my @F32_MULTICLASSES = qw {
  UnaryIntrinsicFloat
  UnaryIntrinsicFloatScalar
  BinaryIntrinsicFloat
  TernaryIntrinsicFloat
  BinaryOpMCFloat
};

my @I32_MULTICLASSES = qw {
  BinaryOpMCInt
  BinaryOpMCi32
  BinaryOpMCi32Const
};

my @GENERATION_ENUM = qw {
  R600_CAYMAN
  R600
  EG
  EG_CAYMAN
  CAYMAN
  SI
};

my $FILE_TYPE = $ARGV[0];

open AMDIL, '<', 'AMDILInstructions.td';

my @INST_ENUMS = ('NONE', 'FEQ', 'FGE', 'FLT', 'FNE', 'MOVE_f32', 'MOVE_i32', 'FTOI', 'ITOF', 'CMOVLOG_f32', 'UGT', 'IGE', 'INE', 'UGE', 'IEQ', 'BINARY_OR_i32', 'BINARY_NOT_i32');

while (<AMDIL>) {
  if ($_ =~ /defm\s+([A-Z_]+)\s+:\s+([A-Za-z0-9]+)</) {
    if (grep {$_ eq $2} @F32_MULTICLASSES) {
      push @INST_ENUMS, "$1\_f32";

    } elsif (grep {$_ eq $2} @I32_MULTICLASSES) {
      push @INST_ENUMS, "$1\_i32";
    }
  } elsif ($_ =~ /def\s+([A-Z_]+)(_[fi]32)/) {
    push @INST_ENUMS, "$1$2";
  }
}

if ($FILE_TYPE eq 'td') {

  print_td_enum('AMDILInst', 'AMDILInstEnums', 'field bits<16>', @INST_ENUMS);

  print_td_enum('AMDGPUGen', 'AMDGPUGenEnums', 'field bits<3>', @GENERATION_ENUM);

  my %constants = (
    'PI' =>      '0x40490fdb',
    'TWO_PI' =>     '0x40c90fdb',
    'TWO_PI_INV' => '0x3e22f983'
  );

  print "class Constants {\n";
  foreach (keys(%constants)) {
    print "int $_ = $constants{$_};\n";
  }
  print "}\n";
  print "def CONST : Constants;\n";

} elsif ($FILE_TYPE eq 'h') {

  print "unsigned GetRealAMDILOpcode(unsigned internalOpcode) const;\n";

  print_h_enum('AMDILTblgenOpcode', @INST_ENUMS);

  print_h_enum('AMDGPUGen', @GENERATION_ENUM);

} elsif ($FILE_TYPE eq 'inc') {
  print "unsigned AMDGPUInstrInfo::GetRealAMDILOpcode(unsigned internalOpcode) const\n{\n";
  print "  switch(internalOpcode) {\n";
  #Start at 1 so we skip NONE
  for (my $i = 1; $i < scalar(@INST_ENUMS); $i++) {
    my $inst = $INST_ENUMS[$i];
    print "  case AMDGPUInstrInfo::$inst: return AMDIL::$inst;\n";
  }
  print "  default: abort();\n";
  print "  }\n}\n";
}


sub print_td_enum {
  my ($instance, $class, $field, @values) = @_;

  print "class $class {\n";

  for (my $i = 0; $i < scalar(@values); $i++) {
    print "  $field $values[$i] = $i;\n";
  }
  print "}\n";

  print "def $instance : $class;\n";
}

sub print_h_enum {

  my ($enum, @list) = @_;
  print "enum $enum {\n";

  for (my $i = 0; $i < scalar(@list); $i++) {
    print "  $list[$i] = $i";
    if ($i != $#list) {
      print ',';
    }
    print "\n";
  }
  print "};\n";
}

