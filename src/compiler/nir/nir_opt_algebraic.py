# -*- coding: utf-8 -*-
#
# Copyright (C) 2014 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import argparse
from collections import OrderedDict
import nir_algebraic
from nir_algebraic import TestStatus
from nir_opcodes import type_sizes
import itertools
import struct
from math import pi
import math

# Convenience variables
a = 'a'
b = 'b'
c = 'c'
d = 'd'
e = 'e'
NAN = math.nan

has_fmulz = '(options->has_fmulz || \
              (options->has_fmulz_no_denorms && \
               !nir_is_denorm_preserve(info->float_controls_execution_mode, 32)))'

has_ffmaz = '(options->has_ffmaz_no_denorms && \
              !nir_is_denorm_preserve(info->float_controls_execution_mode, 32))'

denorm_ftz_16 = 'nir_is_denorm_flush_to_zero(info->float_controls_execution_mode, 16)'
denorm_ftz_32 = 'nir_is_denorm_flush_to_zero(info->float_controls_execution_mode, 32)'
denorm_ftz_64 = 'nir_is_denorm_flush_to_zero(info->float_controls_execution_mode, 64)'

# Written in the form (<search>, <replace>) where <search> is an expression
# and <replace> is either an expression or a value.  An expression is
# defined as a tuple of the form ([~]<op>, <src0>, <src1>, <src2>, <src3>)
# where each source is either an expression or a value.  A value can be
# either a numeric constant or a string representing a variable name.
#
# If the opcode in a search expression is prefixed by a '~' character, this
# indicates that the operation is inexact.  Such operations will only get
# applied to SSA values that do not have the exact bit set.  This should be
# used by by any optimizations that are not bit-for-bit exact.  It should not,
# however, be used for backend-requested lowering operations as those need to
# happen regardless of precision.
#
# Variable names are specified as "[#]name[@type][(cond)][.swiz]" where:
# "#" indicates that the given variable will only match constants,
# type indicates that the given variable will only match values from ALU
#    instructions with the given output type,
# (cond) specifies an additional condition function (see nir_search_helpers.h),
# swiz is a swizzle applied to the variable (only in the <replace> expression)
#
# For constants, you have to be careful to make sure that it is the right
# type because python is unaware of the source and destination types of the
# opcodes.
#
# All expression types can have a bit-size specified.  For opcodes, this
# looks like "op@32", for variables it is "a@32" to specify a size.
# In the search half of the expression this indicates that it
# should only match that particular bit-size.  In the replace half of the
# expression this indicates that the constructed value should have that
# bit-size.
#
# If the opcode in a replacement expression is prefixed by a '!' character,
# this indicated that the new expression will be marked exact.
#
# A special condition "many-comm-expr" can be used with expressions to note
# that the expression and its subexpressions have more commutative expressions
# than nir_replace_instr can handle.  If this special condition is needed with
# another condition, the two can be separated by a comma (e.g.,
# "(many-comm-expr,is_used_once)").
#
# Another set of special "conditions" are
# "nsz": sign of zero is not preserved
# "ninf": infinities are not preserved
# "nnan": nan is not preserved
# These relate to the float controls/fpfastmath and more descriptions of the
# expression than conditions. That is, an expression with the "nsz" condition
# means that the replacement expression won't preserve the sign of zero of the
# result, and so it will be skipped if the matching instruction has the
# 'signed_zero_preserve' flag set.

# based on https://web.archive.org/web/20180105155939/http://forum.devmaster.net/t/fast-and-accurate-sine-cosine/9648
def lowered_sincos(c):
    x = ('fsub', ('fmul', 2.0, ('ffract', ('fadd', ('fmul', 0.5 / pi, a), c))), 1.0)
    x = ('fmul', ('fsub', x, ('fmul', x, ('fabs', x))), 4.0)
    return ('ffma_weak', ('ffma_weak', x, ('fabs', x), ('fneg', x)), 0.225, x)

def intBitsToFloat(i):
    return struct.unpack('!f', struct.pack('!I', i))[0]

# Takes a pattern as input and returns a list of patterns where each
# pattern has a different permutation of fneg/fabs(value) as the replacement
# for the key operands in replacements.
def add_fabs_fneg(pattern, replacements, commutative = True):
    def to_list(pattern):
        return [to_list(i) if isinstance(i, tuple) else i for i in pattern]

    def to_tuple(pattern):
        return tuple(to_tuple(i) if isinstance(i, list) else i for i in pattern)

    def replace_varible(pattern, search, replace):
        for i in range(len(pattern)):
            if pattern[i] == search:
                pattern[i] = replace
            elif isinstance(pattern[i], list):
                replace_varible(pattern[i], search, replace)

    if commutative:
        perms = itertools.combinations_with_replacement(range(4), len(replacements))
    else:
        perms = itertools.product(range(4), repeat=len(replacements))

    result = []

    for perm in perms:
        curr = to_list(pattern)

        for i, (search, base) in enumerate(replacements.items()):
            if perm[i] == 0:
                replace = ['fneg', ['fabs', base]]
            elif perm[i] == 1:
                replace = ['fabs', base]
            elif perm[i] == 2:
                replace = ['fneg', base]
            elif perm[i] == 3:
                replace = base

            replace_varible(curr, search, replace)

        result.append(to_tuple(curr))
    return result

optimize_fcanonicalize = [
   # Eliminate all fcanonicalize if we are not required to flush denormals.
   # Technically this is inexact for the case where we don't know the denorms
   # are preserved - but so is any pattern where one float opcode is replaced by
   # another, because we don't know if they flush the same way.
   # Constant folding would also not flush, so there is already a lot of handwaving
   # involved, and this mode is supposed to be fast, not 100% reproducible.
   (('fcanonicalize', 'a@16'), a, '!'+denorm_ftz_16),
   (('fcanonicalize', 'a@32'), a, '!'+denorm_ftz_32),
   (('fcanonicalize', 'a@64'), a, '!'+denorm_ftz_64),

   # If denormals are required to be flushed we can still
   # eliminate it if any denormals are already flushed or will be flushed.
   (('fcanonicalize(is_only_used_as_float)', a), a),
   (('fcanonicalize', 'a(is_created_as_float)'), a, 'true', TestStatus.UNSUPPORTED),

   # Integral numbers are not denormal.
   (('fcanonicalize', 'a(is_integral)'), a),
]

optimizations = [
   # These will be recreated by late_algebraic if supported.
   # Lowering here means we don't have to duplicate all other optimization patterns.
   (('fgeu', a, b), ('inot', ('flt', a, b))),
   (('fltu', a, b), ('inot', ('fge', a, b))),
   (('fneo', 0.0, a), ('flt', 0.0, ('fabs', a))),
   (('fequ', 0.0, a), ('inot', ('flt', 0.0, ('fabs', a)))),


   (('imul', a, '#b(is_pos_power_of_two)'), ('ishl', a, ('find_lsb', b)), '!options->lower_bitops'),
   (('imul24_relaxed', a, '#b(is_pos_power_of_two)'), ('ishl', a, ('find_lsb', b)), '!options->lower_bitops'),
   (('umul24_relaxed', a, '#b(is_pos_power_of_two)'), ('ishl', a, ('find_lsb', b)), '!options->lower_bitops'),
   (('imul', 'a@8', 0x80), ('ishl', a, 7), '!options->lower_bitops'),
   (('imul', 'a@16', 0x8000), ('ishl', a, 15), '!options->lower_bitops'),
   (('imul', 'a@32', 0x80000000), ('ishl', a, 31), '!options->lower_bitops'),
   (('imul', 'a@64', 0x8000000000000000), ('ishl', a, 63), '!options->lower_bitops'),
   (('imul', a, '#b(is_neg_power_of_two)'), ('ineg', ('ishl', a, ('find_lsb', ('iabs', b)))), '!options->lower_bitops'),
   (('ishl', a, '#b'), ('imul', a, ('ishl', 1, b)), 'options->lower_bitops'),

   (('imul@64', a, '#b(is_bitcount2)'), ('iadd', ('ishl', a, ('ufind_msb', b)), ('ishl', a, ('find_lsb', b))),
    '!options->lower_bitops && (options->lower_int64_options & (nir_lower_imul64 | nir_lower_shift64)) == nir_lower_imul64'),

   (('unpack_64_2x32_split_x', ('imul_2x32_64(is_used_once)', a, b)), ('imul', a, b)),
   (('unpack_64_2x32_split_x', ('umul_2x32_64(is_used_once)', a, b)), ('imul', a, b)),
   (('imul_2x32_64', a, b), ('pack_64_2x32_split', ('imul', a, b), ('imul_high', a, b)), 'options->lower_mul_2x32_64'),
   (('umul_2x32_64', a, b), ('pack_64_2x32_split', ('imul', a, b), ('umul_high', a, b)), 'options->lower_mul_2x32_64'),
   (('udiv', a, 1), a),
   (('idiv', a, 1), a),
   (('umod', a, 1), 0),
   (('imod', a, 1), 0),
   (('imod', a, -1), 0),
   (('irem', a, 1), 0),
   (('irem', a, -1), 0),
   (('udiv', a, '#b(is_pos_power_of_two)'), ('ushr', a, ('find_lsb', b)), '!options->lower_bitops'),
   (('idiv', a, '#b(is_pos_power_of_two)'), ('imul', ('isign', a), ('ushr', ('iabs', a), ('find_lsb', b))), '!options->lower_bitops'),
   (('idiv', a, '#b(is_neg_power_of_two)'), ('ineg', ('imul', ('isign', a), ('ushr', ('iabs', a), ('find_lsb', ('iabs', b))))), '!options->lower_bitops'),
   (('umod', a, '#b(is_pos_power_of_two)'), ('iand', a, ('isub', b, 1)), '!options->lower_bitops'),
   (('imod', a, '#b(is_pos_power_of_two)'), ('iand', a, ('isub', b, 1)), '!options->lower_bitops'),
   (('imod', a, '#b(is_neg_power_of_two)'), ('bcsel', ('ieq', ('ior', a, b), b), 0, ('ior', a, b)), '!options->lower_bitops'),
   # 'irem(a, b)' -> 'a - ((a < 0 ? (a + b - 1) : a) & -b)'
   (('irem', a, '#b(is_pos_power_of_two)'),
    ('isub', a, ('iand', ('bcsel', ('ilt', a, 0), ('iadd', a, ('isub', b, 1)), a), ('ineg', b))),
    '!options->lower_bitops'),
   (('irem', a, '#b(is_neg_power_of_two)'), ('irem', a, ('iabs', b)), '!options->lower_bitops'),
]

# An instruction with src = bcsel selecting between 2 constants makes that src
# non-constant, but we can make it constant by folding the operation into bcsel.
for op in ['idiv', 'udiv', 'umod', 'umod', 'irem']:
    optimizations += [((op, a, ('bcsel', b, '#c', '#d')), ('bcsel', b, (op, a, c), (op, a, d)))]

optimizations += [
   (('fmul', ('fsign', a), ('ffloor', ('fadd', ('fabs', a), 0.5))), ('ftrunc', ('fadd', a, ('fmul', ('fsign', a), 0.5))), '!options->lower_ftrunc || options->lower_ffloor'),

   (('fmul_rtz@32', a, b), ('fmul', a, b), 'nir_is_rounding_mode_rtz(info->float_controls_execution_mode, 32)'),

   (('fneg', ('fneg', a)), ('fcanonicalize', a)),
   (('ineg', ('ineg', a)), a),
   (('fabs', ('fneg', a)), ('fabs', a)),
   (('fabs', ('u2f', a)), ('u2f', a)),
   (('iabs', ('iabs', a)), ('iabs', a)),
   (('iabs', ('ineg', a)), ('iabs', a)),
   (('fadd(nsz)', a, 0.0), ('fcanonicalize', a)),
   (('fadd', a, -0.0), ('fcanonicalize', a)),
   (('fadd', ('b2f', a), 0.0), ('b2f', a)),
   (('iadd', a, 0), a),
   (('iadd_sat', a, 0), a),
   (('isub_sat', a, 0), a),
   (('uadd_sat', a, 0), a),
   (('usub_sat', a, 0), a),
   (('usadd_4x8_vc4', a, 0), a),
   (('usadd_4x8_vc4', a, ~0), ~0),
   (('~fadd', ('fmul', a, b), ('fmul', a, c)), ('fmul', a, ('fadd', b, c))),
   (('~fadd', ('fmulz', a, b), ('fmulz', a, c)), ('fmulz', a, ('fadd', b, c))),
   (('iadd', ('imul', a, b), ('imul', a, c)), ('imul', a, ('iadd', b, c))),
   (('iadd', ('ishl', b, a), ('ishl', c, a)), ('ishl', ('iadd', b, c), a)),
   (('iand', ('iand', a, b), ('iand(is_used_once)', a, c)), ('iand', ('iand', a, b), c)),
   (('ior', ('ior', a, b), ('ior(is_used_once)', a, c)), ('ior', ('ior', a, b), c)),
   (('iand', ('ior(is_used_once)', a, b), ('ior(is_used_once)', a, c)), ('ior', a, ('iand', b, c))),
   (('ior', ('iand(is_used_once)', a, b), ('iand(is_used_once)', a, c)), ('iand', a, ('ior', b, c))),
   # (a & b) | (a | c)  =>  ((a & b) | a) | c  =>  a | c
   (('ior', ('iand', a, b), ('ior', a, c)), ('ior', a, c)),
   # (a & b) & (a | c)  =>  b & (a & (a | c))  =>  b & a
   (('iand', ('iand', a, b), ('ior', a, c)), ('iand', a, b)),
   (('ieq', ('iand', a, '#b(is_pos_power_of_two)'), b), ('ine', ('iand', a, b), 0)),
   (('ine', ('iand', a, '#b(is_pos_power_of_two)'), b), ('ieq', ('iand', a, b), 0)),
   (('uge', ('iand', a, '#b(is_pos_power_of_two)'), b), ('ine', ('iand', a, b), 0)),
   (('ult', ('iand', a, '#b(is_pos_power_of_two)'), b), ('ieq', ('iand', a, b), 0)),
   (('ige', ('iand', a, b), '#b(is_pos_power_of_two)'), ('ine', ('iand', a, b), 0)),
   (('ilt', ('iand', a, b), '#b(is_pos_power_of_two)'), ('ieq', ('iand', a, b), 0)),
   (('ieq', ('ushr(is_used_once)', a, '#b'), 0), ('ult', a, ('ishl', 1, b))),
   (('ine', ('ushr(is_used_once)', a, '#b'), 0), ('uge', a, ('ishl', 1, b))),
   (('~fadd', ('fneg', a), a), 0.0),
   (('fadd(nnan)', ('fneg', a), a), 0.0),
   (('iadd', ('ineg', a), a), 0),
   (('iadd', ('ineg', a), ('iadd', a, b)), b),
   (('iadd', a, ('iadd', ('ineg', a), b)), b),
   (('~fadd', ('fneg', a), ('fadd', a, b)), ('fcanonicalize', b)),
   (('~fadd', a, ('fadd', ('fneg', a), b)), ('fcanonicalize', b)),
   (('fadd', ('fsat', a), ('fsat', ('fneg', a))), ('fsat', ('fabs', a))),
   (('fadd', a, a), ('fmul', a, 2.0)),
   (('fadd(contract)', a, ('fadd(is_used_once)', a, b)), ('fadd', b, ('fmul', a, 2.0))),
   # The only effect a*0.0 should have is when 'a' is infinity, -0.0 or NaN
   (('fmul(nsz,nnan)', 'a', 0.0), 0.0),
   (('fmul(nsz,nnan)', 'a', -0.0), 0.0),
   (('fmulz', a, 0.0), 0.0),
   (('fmulz', a, -0.0), 0.0),
   (('fmulz(nsz)', a, 'b(is_finite_not_zero)'), ('fmul', a, b)),
   (('fmulz(nsz)', 'a(is_finite)', 'b(is_finite)'), ('fmul', a, b)),
   (('fmulz', a, a), ('fmul', a, a)),
   (('ffmaz(nsz)', a, 'b(is_finite_not_zero)', c), ('ffma', a, b, c)),
   (('ffmaz', 'a(is_finite)', 'b(is_finite)', c), ('ffma', a, b, c)),
   (('ffmaz', a, a, b), ('ffma', a, a, b)),
   (('imul', a, 0), 0),
   (('imul24_relaxed', a, 0), 0),
   (('umul24_relaxed', a, 0), 0),
   (('umul_unorm_4x8_vc4', a, 0), 0),
   (('umul_unorm_4x8_vc4', a, ~0), a),
   (('fmul', a, 1.0), ('fcanonicalize', a)),
   (('imul', a, 1), a),
   (('imul24_relaxed', a, 1), a),
   (('umul24_relaxed', a, 1), a),
   (('fmul', a, -1.0), ('fneg', a)),
   (('imul', a, -1), ('ineg', a)),
   (('imul24_relaxed', a, -1), ('ineg', a)),
   # If a < 0: fsign(a)*a*a => -1*a*a => -a*a => abs(a)*a
   # If a > 0: fsign(a)*a*a => 1*a*a => a*a => abs(a)*a
   # If a == 0: fsign(a)*a*a => 0*0*0 => abs(0)*0
   # If a != a: fsign(a)*a*a => 0*NaN*NaN => abs(NaN)*NaN
   (('fmul', ('fsign', a), ('fmul', a, a)), ('fmul', ('fabs', a), a)),
   (('fmul', ('fmul', ('fsign', a), a), a), ('fmul', ('fabs', a), a)),
   (('ffma(nsz,nnan)', 0.0, a, b), ('fcanonicalize', b)),
   (('ffma(nsz,nnan)', -0.0, a, b), ('fcanonicalize', b)),
   (('ffmaz', 0.0, a, b), ('fadd', 0.0, b)),
   (('ffmaz', -0.0, a, b), ('fadd', 0.0, b)),
   (('ffma(nsz)', a, b, 0.0), ('fmul', a, b)),
   (('ffmaz(nsz)', a, b, 0.0), ('fmulz', a, b)),
   (('ffma', a, b, -0.0), ('fmul', a, b)),
   (('ffmaz', a, b, -0.0), ('fmulz', a, b)),
   (('ffma', 1.0, a, b), ('fadd', a, b)),
   (('ffmaz(nsz)', 1.0, a, b), ('fadd', a, b)),
   (('ffma', -1.0, a, b), ('fadd', ('fneg', a), b)),
   (('ffmaz(nsz)', -1.0, a, b), ('fadd', ('fneg', a), b)),
   (('flrp(nnan,nsz)', a, b, 0.0), ('fcanonicalize', a)),
   (('flrp(nnan,nsz)', a, b, -0.0), ('fcanonicalize', a)),
   (('flrp(nnan,nsz)', a, b, 1.0), ('fcanonicalize', b)),
   (('~flrp', a, a, b), ('fcanonicalize', a)),
   (('flrp(nnan,nsz)', 0.0, a, b), ('fmul', a, b)),
   (('flrp(nnan,nsz)', -0.0, a, b), ('fmul', a, b)),

   # flrp(a, a + b, c) => a + flrp(0, b, c) => a + (b * c)
   (('~flrp', a, ('fadd(is_used_once)', a, b), c), ('fadd', ('fmul', b, c), a)),

   (('sdot_4x8_iadd', a, 0, b), b),
   (('udot_4x8_uadd', a, 0, b), b),
   (('sdot_4x8_iadd_sat', a, 0, b), b),
   (('udot_4x8_uadd_sat', a, 0, b), b),
   (('sdot_2x16_iadd', a, 0, b), b),
   (('udot_2x16_uadd', a, 0, b), b),
   (('sdot_2x16_iadd_sat', a, 0, b), b),
   (('udot_2x16_uadd_sat', a, 0, b), b),

   # sudot_4x8_iadd is not commutative at all, so the patterns must be
   # duplicated with zeros on each of the first positions.
   (('sudot_4x8_iadd', a, 0, b), b),
   (('sudot_4x8_iadd', 0, a, b), b),
   (('sudot_4x8_iadd_sat', a, 0, b), b),
   (('sudot_4x8_iadd_sat', 0, a, b), b),

   (('iadd', ('sdot_4x8_iadd(is_used_once)', a, b, '#c'), '#d'), ('sdot_4x8_iadd', a, b, ('iadd', c, d))),
   (('iadd', ('udot_4x8_uadd(is_used_once)', a, b, '#c'), '#d'), ('udot_4x8_uadd', a, b, ('iadd', c, d))),
   (('iadd', ('sudot_4x8_iadd(is_used_once)', a, b, '#c'), '#d'), ('sudot_4x8_iadd', a, b, ('iadd', c, d))),
   (('iadd', ('sdot_2x16_iadd(is_used_once)', a, b, '#c'), '#d'), ('sdot_2x16_iadd', a, b, ('iadd', c, d))),
   (('iadd', ('udot_2x16_uadd(is_used_once)', a, b, '#c'), '#d'), ('udot_2x16_uadd', a, b, ('iadd', c, d))),

   # Try to let constant folding eliminate the dot-product part.  These are
   # safe because the dot product cannot overflow 32 bits.
   (('iadd', ('sdot_4x8_iadd', a, b, 0), c), ('sdot_4x8_iadd', a, b, c)),
   (('iadd', ('udot_4x8_uadd', a, b, 0), c), ('udot_4x8_uadd', a, b, c)),
   (('iadd', ('sudot_4x8_iadd', a, b, 0), c), ('sudot_4x8_iadd', a, b, c)),
   (('iadd', ('sdot_2x16_iadd', a, b, 0), c), ('sdot_2x16_iadd', a, b, c)),
   (('iadd', ('udot_2x16_uadd', a, b, 0), c), ('udot_2x16_uadd', a, b, c)),
   (('sdot_4x8_iadd', '#a', '#b', c), ('iadd', ('sdot_4x8_iadd', a, b, 0), c)),
   (('udot_4x8_uadd', '#a', '#b', c), ('iadd', ('udot_4x8_uadd', a, b, 0), c)),
   (('sudot_4x8_iadd', '#a', '#b', c), ('iadd', ('sudot_4x8_iadd', a, b, 0), c)),
   (('sdot_2x16_iadd', '#a', '#b', c), ('iadd', ('sdot_2x16_iadd', a, b, 0), c)),
   (('udot_2x16_uadd', '#a', '#b', c), ('iadd', ('udot_2x16_uadd', a, b, 0), c)),
   (('sdot_4x8_iadd_sat', '#a', '#b', c), ('iadd_sat', ('sdot_4x8_iadd', a, b, 0), c), '!options->lower_iadd_sat'),
   (('udot_4x8_uadd_sat', '#a', '#b', c), ('uadd_sat', ('udot_4x8_uadd', a, b, 0), c), '!options->lower_uadd_sat'),
   (('sudot_4x8_iadd_sat', '#a', '#b', c), ('iadd_sat', ('sudot_4x8_iadd', a, b, 0), c), '!options->lower_iadd_sat'),
   (('sdot_2x16_iadd_sat', '#a', '#b', c), ('iadd_sat', ('sdot_2x16_iadd', a, b, 0), c), '!options->lower_iadd_sat'),
   (('udot_2x16_uadd_sat', '#a', '#b', c), ('uadd_sat', ('udot_2x16_uadd', a, b, 0), c), '!options->lower_uadd_sat'),

   # Optimize open-coded fmulz.
   # (b==0.0 ? 0.0 : a) * (a==0.0 ? 0.0 : b) -> fmulz(a, b)
   *add_fabs_fneg((('fmul@32(nsz)', ('bcsel', ('feq', b, 0.0), 0.0, 'ma'), ('bcsel', ('feq', a, 0.0), 0.0, 'mb')),
    ('fmulz', 'ma', 'mb'), has_fmulz), {'ma' : a, 'mb' : b}),
   # (b!=0.0 ? a : 0.0) * (a==0.0 ? 0.0 : b) -> fmulz(a, b)
   *add_fabs_fneg((('fmul@32(nsz)', ('bcsel', ('fneu', b, 0.0), 'ma', 0.0), ('bcsel', ('feq', a, 0.0), 0.0, 'mb')),
    ('fmulz', 'ma', 'mb'), has_fmulz), {'ma' : a, 'mb' : b}),
   # (b!=0.0 ? a : 0.0) * (a!=0.0 ? b : 0.0) -> fmulz(a, b)
   *add_fabs_fneg((('fmul@32(nsz)', ('bcsel', ('fneu', b, 0.0), 'ma', 0.0), ('bcsel', ('fneu', a, 0.0), 'mb', 0.0)),
    ('fmulz', 'ma', 'mb'), has_fmulz), {'ma' : a, 'mb' : b}),
   # b2f(a != 0.0 && b) * (b ? a : 0.0)
   *add_fabs_fneg((('fmul@32(nsz)', ('b2f', ('iand', ('fneu', a, 0.0), b)), ('bcsel', b, 'ma', 0.0)),
    ('fmulz', 'ma', ('b2f', b)), has_fmulz), {'ma' : a}),
   # b2f(!(a == 0.0 || b)) * (b ? 0.0 : a)
   *add_fabs_fneg((('fmul@32(nsz)', ('b2f', ('inot', ('ior', ('feq', a, 0.0), b))), ('bcsel', b, 0.0, 'ma')),
    ('fmulz', 'ma', ('b2f', ('inot', b))), has_fmulz), {'ma' : a}),


   # (min(abs(a), abs(b)) == 0.0 ? 0.0 : a * b) -> fmulz(a,b)
   *add_fabs_fneg((('bcsel', ('feq', ('fmin', ('fabs', a), ('fabs', b)), 0.0), 0.0, ('fmul@32', 'ma', 'mb')),
    ('fmulz', 'ma', 'mb'), has_fmulz), {'ma': a, 'mb': b}),

   # a * (a == 0.0 ? 0.0 : b(is_non_const_zero))
   *add_fabs_fneg((('fmul@32(nsz)', 'ma', ('bcsel', ('feq', a, 0.0), 0.0, '#b(is_not_const_zero)')),
    ('fmulz', 'ma', b), has_fmulz), {'ma' : a}),

   # ffma(b==0.0 ? 0.0 : a, a==0.0 ? 0.0 : b, c) -> ffmaz(a, b, c)
   *add_fabs_fneg((('ffma@32(nsz)', ('bcsel', ('feq', b, 0.0), 0.0, 'ma'), ('bcsel', ('feq', a, 0.0), 0.0, 'mb'), c),
    ('ffmaz', 'ma', 'mb', c), has_ffmaz), {'ma' : a, 'mb' : b}),
   *add_fabs_fneg((('ffma@32(nsz)', 'ma', ('bcsel', ('feq', a, 0.0), 0.0, '#b(is_not_const_zero)'), c),
    ('ffmaz', 'ma', b, c), has_ffmaz), {'ma' : a}),
   *add_fabs_fneg((('ffma@32(nsz)', ('b2f', ('iand', ('fneu', a, 0.0), b)), ('bcsel', b, 'ma', 0.0), c),
    ('ffmaz', 'ma', ('b2f', b), c), has_ffmaz), {'ma' : a}),
   *add_fabs_fneg((('ffma@32(nsz)', ('b2f', ('inot', ('ior', ('feq', a, 0.0), b))), ('bcsel', b, 0.0, 'ma'), c),
    ('ffmaz', 'ma', ('b2f', ('inot', b)), c), has_ffmaz), {'ma' : a}),

   # b == 0.0 ? 1.0 : fexp2(fmul(a, b)) -> fexp2(fmulz(a, b))
   *add_fabs_fneg((('bcsel(nsz,nnan,ninf)', ('feq', b, 0.0), 1.0, ('fexp2', ('fmul@32', a, 'mb'))),
    ('fexp2', ('fmulz', a, 'mb')),
    has_fmulz), {'mb': b}),
   *add_fabs_fneg((('bcsel', ('feq', b, 0.0), 1.0, ('fexp2', ('fmulz', a, 'mb'))),
    ('fexp2', ('fmulz', a, 'mb'))), {'mb': b}),
]

# float multadd related rules
for sz in (16, 32, 64):
    has_ffma = f'(options->float_mul_add{sz} & nir_float_muladd_support_has_ffma)'
    has_fmad = f'(options->float_mul_add{sz} & nir_float_muladd_support_has_fmad)'
    has_none = f'(!options->float_mul_add{sz})'
    prefers_split = f'(options->float_mul_add{sz} & nir_float_muladd_support_prefers_split)'
    keep_ffma_weak = f'(options->float_mul_add{sz} & nir_float_muladd_support_keep_weak_ffma)'
    optimizations += [
        # Split contract ffma_weak for better optimizations. It will be refused later.
        ((f'ffma_weak@{sz}(contract)', a, b, c), ('fadd', ('fmul', a, b), c), f'!{keep_ffma_weak}'),

        # Convert ffma_weak to backend supported ops
        ((f'ffma_weak@{sz}', a, b, c), ('ffma', a, b, c), f'!{keep_ffma_weak} && !{prefers_split} && {has_ffma}'),
        ((f'ffma_weak@{sz}', a, b, c), ('fadd', ('fmul', a, b), c), f'!{keep_ffma_weak} && ({prefers_split} || {has_fmad} || {has_none})'),
    ]

optimizations += [
    # Split fmad better for optimizations. It might get merged to fmad later.
    (('fmad', a, b, c), ('fadd', ('fmul', a, b), c)),
    (('fmadz', a, b, c), ('fadd', ('fmulz', a, b), c)),
]

# Bitwise operations affecting the sign may be replaced by equivalent
# floating point operations, except possibly for denormal
# behaviour hence the is_only_used_as_float.
for sz in (16, 32, 64):
    sign_bit = 1 << (sz - 1)

    optimizations.extend([
        (('iand(is_only_used_as_float)', f'a@{sz}', sign_bit - 1), ('fabs', a)),
        (('ixor(is_only_used_as_float)', f'a@{sz}', sign_bit), ('fneg', a)),
        (('ior(is_only_used_as_float)', f'a@{sz}', sign_bit), ('fneg', ('fabs', a))),
    ])

# Shorthand for the expansion of just the dot product part of the [iu]dp4a
# instructions.
sdot_4x8_a_b = ('iadd', ('iadd', ('imul24_relaxed', ('extract_i8', a, 0), ('extract_i8', b, 0)),
                                 ('imul24_relaxed', ('extract_i8', a, 1), ('extract_i8', b, 1))),
                        ('iadd', ('imul24_relaxed', ('extract_i8', a, 2), ('extract_i8', b, 2)),
                                 ('imul24_relaxed', ('extract_i8', a, 3), ('extract_i8', b, 3))))
udot_4x8_a_b = ('iadd', ('iadd', ('umul24_relaxed', ('extract_u8', a, 0), ('extract_u8', b, 0)),
                                 ('umul24_relaxed', ('extract_u8', a, 1), ('extract_u8', b, 1))),
                        ('iadd', ('umul24_relaxed', ('extract_u8', a, 2), ('extract_u8', b, 2)),
                                 ('umul24_relaxed', ('extract_u8', a, 3), ('extract_u8', b, 3))))
sudot_4x8_a_b = ('iadd', ('iadd', ('imul24_relaxed', ('extract_i8', a, 0), ('extract_u8', b, 0)),
                                  ('imul24_relaxed', ('extract_i8', a, 1), ('extract_u8', b, 1))),
                         ('iadd', ('imul24_relaxed', ('extract_i8', a, 2), ('extract_u8', b, 2)),
                                  ('imul24_relaxed', ('extract_i8', a, 3), ('extract_u8', b, 3))))
sdot_2x16_a_b = ('iadd', ('imul24_relaxed', ('extract_i16', a, 0), ('extract_i16', b, 0)),
                         ('imul24_relaxed', ('extract_i16', a, 1), ('extract_i16', b, 1)))
udot_2x16_a_b = ('iadd', ('umul24_relaxed', ('extract_u16', a, 0), ('extract_u16', b, 0)),
                         ('umul24_relaxed', ('extract_u16', a, 1), ('extract_u16', b, 1)))

optimizations.extend([
   (('sdot_4x8_iadd', a, b, c), ('iadd', sdot_4x8_a_b, c), '!options->has_sdot_4x8'),
   (('udot_4x8_uadd', a, b, c), ('iadd', udot_4x8_a_b, c), '!options->has_udot_4x8'),
   (('sudot_4x8_iadd', a, b, c), ('iadd', sudot_4x8_a_b, c), '!options->has_sudot_4x8'),
   (('sdot_2x16_iadd', a, b, c), ('iadd', sdot_2x16_a_b, c), '!options->has_dot_2x16'),
   (('udot_2x16_uadd', a, b, c), ('iadd', udot_2x16_a_b, c), '!options->has_dot_2x16'),

   # For the unsigned dot-product, the largest possible value 4*(255*255) =
   # 0x3f804, so we don't have to worry about that intermediate result
   # overflowing.  0x100000000 - 0x3f804 = 0xfffc07fc.  If c is a constant
   # that is less than 0xfffc07fc, then the result cannot overflow ever.
   (('udot_4x8_uadd_sat', a, b, '#c(is_ult_0xfffc07fc)'), ('udot_4x8_uadd', a, b, c)),
   (('udot_4x8_uadd_sat', a, b, c), ('uadd_sat', ('udot_4x8_uadd', a, b, 0), c), '!options->has_udot_4x8_sat'),

   # For the signed dot-product, the largest positive value is 4*(-128*-128) =
   # 0x10000, and the largest negative value is 4*(-128*127) = -0xfe00.  We
   # don't have to worry about that intermediate result overflowing or
   # underflowing.
   (('sdot_4x8_iadd_sat', a, b, c), ('iadd_sat', ('sdot_4x8_iadd', a, b, 0), c), '!options->has_sdot_4x8_sat'),

   (('sudot_4x8_iadd_sat', a, b, c), ('iadd_sat', ('sudot_4x8_iadd', a, b, 0), c), '!options->has_sudot_4x8_sat'),

   (('udot_2x16_uadd_sat', a, b, c), ('uadd_sat', udot_2x16_a_b, c), '!options->has_dot_2x16'),
   (('sdot_2x16_iadd_sat', a, b, c), ('iadd_sat', sdot_2x16_a_b, c), '!options->has_dot_2x16'),
])

# Float sizes
for s in [16, 32, 64]:
    optimizations.extend([
       (('~flrp@{}'.format(s), a, b, ('b2f', 'c@1')), ('bcsel', c, ('fcanonicalize', b), ('fcanonicalize', a)), 'options->lower_flrp{}'.format(s)),

       (('~flrp@{}'.format(s), a, ('fadd', a, b), c), ('fadd', ('fmul', b, c), a), 'options->lower_flrp{}'.format(s)),
       (('~flrp@{}'.format(s), ('fadd(is_used_once)', a, b), ('fadd(is_used_once)', a, c), d), ('fadd', ('flrp', b, c, d), a), 'options->lower_flrp{}'.format(s)),
       (('~flrp@{}'.format(s), a, ('fmul(is_used_once)', a, b), c), ('fmul', ('flrp', 1.0, b, c), a), 'options->lower_flrp{}'.format(s)),

       (('~fadd@{}'.format(s), ('fmul', a, ('fadd', 1.0, ('fneg', c))), ('fmul', b, c)), ('flrp', a, b, c), '!options->lower_flrp{}'.format(s)),
       # These are the same as the previous three rules, but it depends on
       # 1-fsat(x) <=> fsat(1-x).  See below.
       (('~fadd@{}'.format(s), ('fmul', a, ('fsat', ('fadd', 1.0, ('fneg', c)))), ('fmul', b, ('fsat', c))), ('flrp', a, b, ('fsat', c)), '!options->lower_flrp{}'.format(s)),
       (('~fadd@{}'.format(s), a, ('fmul', c, ('fadd', b, ('fneg', a)))), ('flrp', a, b, c), '!options->lower_flrp{}'.format(s)),

       # 1 - ((1 - a) * (1 - b))
       # 1 - (1 - a - b + a*b)
       # 1 - 1 + a + b - a*b
       # a + b - a*b
       # a + b*(1 - a)
       # b*(1 - a) + 1*a
       # flrp(b, 1, a)
       (('~fadd@{}'.format(s), 1.0, ('fneg', ('fmul', ('fadd', 1.0, ('fneg', a)), ('fadd', 1.0, ('fneg', b))))), ('flrp', b, 1.0, a), '!options->lower_flrp{}'.format(s)),
    ])

optimizations.extend([
   (('~fadd', ('fmul', a, ('b2f', ('inot', 'c@1'))), ('fmul', b, ('b2f',  c))), ('bcsel', c, ('fcanonicalize', b), ('fcanonicalize', a))),
   (('~fadd', a, ('fmul', ('b2f', 'c@1'), ('fadd', b, ('fneg', a)))), ('bcsel', c, ('fcanonicalize', b), ('fcanonicalize', a))),

   (('~flrp', ('fmul(is_used_once)', a, b), ('fmul(is_used_once)', a, c), d), ('fmul', ('flrp', b, c, d), a)),

   (('~flrp', a, 0.0, c), ('fadd', ('fmul', ('fneg', a), c), a)),

   # D3D9 vertex shader trunc
   (('fadd', ('ffloor', a), ('b2f', ('iand', ('flt', a, 0), ('flt', ('fneg', ('ffract', a)), ('ffract', a))))), ('ftrunc', ('fadd', a, 0))),
   # D3D9 pixel shader trunc
   (('fadd', ('ffloor', a), ('b2f', ('inot', ('fge', 0, ('fmin', ('fneg', a), ('ffract', a)))))), ('ftrunc', ('fadd', a, 0))),
   (('fadd', ('ffloor', a), ('b2f', ('flt', 0, ('fmin', ('fneg', a), ('ffract', a))))), ('ftrunc', ('fadd', a, 0))),

   (('fadd(nnan,nsz)', a, ('ffract', ('fneg', a))), ('fceil', a), '!options->lower_fceil'),

   (('ftrunc@16', a), ('bcsel', ('flt', a, 0.0), ('fneg', ('ffloor', ('fabs', a))), ('ffloor', ('fabs', a))), 'options->lower_ftrunc'),
   (('ftrunc@32', a), ('bcsel', ('flt', a, 0.0), ('fneg', ('ffloor', ('fabs', a))), ('ffloor', ('fabs', a))), 'options->lower_ftrunc'),
   (('ftrunc@64', a), ('bcsel', ('flt', a, 0.0), ('fneg', ('ffloor', ('fabs', a))), ('ffloor', ('fabs', a))),
    '(options->lower_ftrunc || (options->lower_doubles_options & nir_lower_dtrunc)) && (!(options->lower_doubles_options & nir_lower_dfloor) || !(options->lower_doubles_options & nir_lower_dfract))'),

   (('ffloor@16', a), ('fsub', a, ('!ffract', a)), 'options->lower_ffloor'),
   (('ffloor@32', a), ('fsub', a, ('!ffract', a)), 'options->lower_ffloor'),
   (('ffloor@64', a), ('fsub', a, ('!ffract', a)), '(options->lower_ffloor || (options->lower_doubles_options & nir_lower_dfloor)) && !(options->lower_doubles_options & nir_lower_dfract)'),
   (('fadd@16', a, ('fadd@16', b, ('fneg', ('ffract', a)))), ('fadd@16', b, ('ffloor', a)), '!options->lower_ffloor'),
   (('fadd@32', a, ('fadd@32', b, ('fneg', ('ffract', a)))), ('fadd@32', b, ('ffloor', a)), '!options->lower_ffloor'),
   (('fadd@64', a, ('fadd@64', b, ('fneg', ('ffract', a)))), ('fadd@64', b, ('ffloor', a)), '!options->lower_ffloor && !(options->lower_doubles_options & nir_lower_dfloor)'),
   (('fadd@16(nnan)', a, ('fneg', ('ffract', a))), ('ffloor', a), '!options->lower_ffloor'),
   (('fadd@32(nnan)', a, ('fneg', ('ffract', a))), ('ffloor', a), '!options->lower_ffloor'),
   (('fadd@64(nnan)', a, ('fneg', ('ffract', a))), ('ffloor', a), '!options->lower_ffloor && !(options->lower_doubles_options & nir_lower_dfloor)'),
   (('ffract@16', a), ('fsub', a, ('ffloor', a)), 'options->lower_ffract'),
   (('ffract@32', a), ('fsub', a, ('ffloor', a)), 'options->lower_ffract'),
   (('ffract@64', a), ('fsub', a, ('ffloor', a)),
    '(options->lower_ffract || (options->lower_doubles_options & nir_lower_dfract)) && !(options->lower_doubles_options & nir_lower_dfloor)'),
   (('fadd@16', a, ('fneg(is_used_once)', ('ffloor(is_used_once)', a))), ('ffract', a), '!options->lower_ffract'),
   (('fadd@32', a, ('fneg(is_used_once)', ('ffloor(is_used_once)', a))), ('ffract', a), '!options->lower_ffract'),
   (('fadd@64', a, ('fneg(is_used_once)', ('ffloor(is_used_once)', a))), ('ffract', a), '!options->lower_ffract && !(options->lower_doubles_options & nir_lower_dfract)'),
   (('fceil', a), ('fneg', ('ffloor', ('fneg', a))), 'options->lower_fceil'),

   (('fmul', ('fadd', ('bcsel', a, ('fmul', b, c), 0), '#d'), '#e'),
    ('bcsel', a, ('fmul', ('fadd', ('fmul', b, c), d), e), ('fmul', ('fadd', d, 0.0), e))),

   (('fdph', a, b), ('fdot4', ('vec4', 'a.x', 'a.y', 'a.z', 1.0), b), 'options->lower_fdph'),

   (('fdot4', a, 0.0), 0.0, 'true', TestStatus.XFAIL), # XFAIL is that fdot(NaN, 0.0...) produces 0.0 instead of NaN.
   (('fdot3', a, 0.0), 0.0, 'true', TestStatus.XFAIL),
   (('fdot2', a, 0.0), 0.0, 'true', TestStatus.XFAIL),

   (('fdot4', ('vec4', a, b,   c,   1.0), d), ('fdph',  ('vec3', a, b, c), d), '!options->lower_fdph'),
   (('fdot4', ('vec4', a, 0.0, 0.0, 0.0), b), ('fmul', a, 'b.x'), 'true', TestStatus.XFAIL), # XFAIL is that fdot(vec(-1.0, 0.0...), 0.0) produces -0.0 instead of 0.0.
   (('fdot4', ('vec4', a, b,   0.0, 0.0), c), ('fdot2', ('vec2', a, b), c), 'true', TestStatus.XFAIL),
   (('fdot4', ('vec4', a, b,   c,   0.0), d), ('fdot3', ('vec3', a, b, c), d), 'true', TestStatus.XFAIL),
   (('fdot4', 'a(w_is_zero)', b), ('fdot3', 'a.xyz', 'b.xyz'), 'true', TestStatus.XFAIL), # XFAIL is that fdot((1,-1,0,0), (0,.12345,1,inf) produces -.12345 instead of -nan)
   (('fdot4', 'a(z_is_zero)', b), ('fdot3', 'a.xyw', 'b.xyw'), 'true', TestStatus.XFAIL),
   (('fdot4', 'a(y_is_zero)', b), ('fdot3', 'a.xzw', 'b.xzw'), 'true', TestStatus.XFAIL),
   (('fdot4', 'a(x_is_zero)', b), ('fdot3', 'a.yzw', 'b.yzw'), 'true', TestStatus.XFAIL),

   (('fdot3', ('vec3', a, 0.0, 0.0), b), ('fmul', a, 'b.x'), 'true', TestStatus.XFAIL),
   (('fdot3', ('vec3', a, b,   0.0), c), ('fdot2', ('vec2', a, b), c), 'true', TestStatus.XFAIL),
   (('fdot3', 'a(x_is_zero)', b), ('fdot2', 'a.yz', 'b.yz'), 'true', TestStatus.XFAIL),
   (('fdot3', 'a(y_is_zero)', b), ('fdot2', 'a.xz', 'b.xz'), 'true', TestStatus.XFAIL),
   (('fdot3', 'a(z_is_zero)', b), ('fdot2', 'a.xy', 'b.xy'), 'true', TestStatus.XFAIL),

   (('fdot2', ('vec2', a, 0.0), b), ('fmul', a, 'b.x'), 'true', TestStatus.XFAIL),
   (('fdot2', 'a(x_is_zero)', b), ('fmul', 'a.y', 'b.y'), 'true', TestStatus.XFAIL),
   (('fdot2', 'a(y_is_zero)', b), ('fmul', 'a.x', 'b.x'), 'true', TestStatus.XFAIL),
   (('fdot2', a, 1.0), ('fadd', 'a.x', 'a.y')),

   # If x >= 0 and x <= 1: fsat(1 - x) == 1 - fsat(x) trivially
   # If x < 0: 1 - fsat(x) => 1 - 0 => 1 and fsat(1 - x) => fsat(> 1) => 1
   # If x > 1: 1 - fsat(x) => 1 - 1 => 0 and fsat(1 - x) => fsat(< 0) => 0
   (('fadd', ('fneg(is_used_once)', ('fsat(is_used_once,nnan)', 'a(is_not_fmul)')), 1.0), ('fsat', ('fadd', 1.0, ('fneg', a)))),

   # (a * #b + #c) << #d
   # ((a * #b) << #d) + (#c << #d)
   # (a * (#b << #d)) + (#c << #d)
   (('ishl', ('iadd', ('imul', a, '#b'), '#c'), '#d'),
    ('iadd', ('imul', a, ('ishl', b, d)), ('ishl', c, d))),

   # (a * #b) << #c  =>  a * (#b << #c)
   (('ishl', ('imul', a, '#b'), '#c'), ('imul', a, ('ishl', b, c))),
   # (a << #b) * #c  =>  a * (#c << #b)
   (('imul', ('ishl', a, '#b'), '#c'), ('imul', a, ('ishl', c, b))),
])

optimizations.extend(optimize_fcanonicalize)

# Care must be taken here.  Shifts in NIR uses only the lower log2(bitsize)
# bits of the second source.  These replacements must correctly handle the
# case where (b % bitsize) + (c % bitsize) >= bitsize.
for s in [8, 16, 32, 64]:
   mask = s - 1

   ishl = "ishl@{}".format(s)
   ishl_once = "ishl@{}(is_used_once)".format(s)
   ishr = "ishr@{}".format(s)
   ushr = "ushr@{}".format(s)

   in_bounds = lambda x, y: ('ult', ('iadd', ('iand', x, mask), ('iand', y, mask)), s)

   optimizations.extend([
       ((ishl, (ishl, a, '#b'), '#c'), ('bcsel', in_bounds(b, c), (ishl, a, ('iadd', b, c)), 0)),
       ((ushr, (ushr, a, '#b'), '#c'), ('bcsel', in_bounds(b, c), (ushr, a, ('iadd', b, c)), 0)),

       # To get get -1 for large shifts of negative values, ishr must instead
       # clamp the shift count to the maximum value.
       ((ishr, (ishr, a, '#b'), '#c'),
        (ishr, a, ('imin', ('iadd', ('iand', b, mask), ('iand', c, mask)), s - 1))),

       ((ishl, ('iadd(is_used_once)', (ishl_once, a, '#b'), (ishl_once, c, '#d')), '#e'),
        ('iadd', ('bcsel', in_bounds(b, e), ('ishl', a, ('iand', ('iadd', b, e), mask)), 0),
                 ('bcsel', in_bounds(d, e), ('ishl', c, ('iand', ('iadd', d, e), mask)), 0))),
   ])

# Optimize a pattern of address calculation created by DXVK where the offset is
# divided by 4 and then multipled by 4. This can be turned into an iand and the
# additions before can be reassociated to CSE the iand instruction.

for size, mask in ((8, 0xff), (16, 0xffff), (32, 0xffffffff), (64, 0xffffffffffffffff)):
    a_sz = 'a@{}'.format(size)

    optimizations.extend([
       # 'a >> #b << #b' -> 'a & ~((1 << #b) - 1)'
       (('ishl', ('ushr', a_sz, '#b'), b), ('iand', a, ('ishl', mask, b))),
       (('ishl', ('ishr', a_sz, '#b'), b), ('iand', a, ('ishl', mask, b))),

       # This does not trivially work with ishr.
       (('ushr', ('ishl', a_sz, '#b'), b), ('iand', a, ('ushr', mask, b))),
    ])

# Collapses ubfe(ubfe(a, b, c), d, e) when b, c, d, e are constants.
def ubfe_ubfe(a, b, c, d, e):
    inner_offset = ('iand', b, 0x1f)
    inner_bits = ('umin', ('iand', c, 0x1f), ('isub', 32, inner_offset))
    outer_offset = ('iand', d, 0x1f)
    outer_bits = ('iand', e, 0x1f)

    offset = ('iadd', inner_offset, outer_offset)
    bits = ('umin', outer_bits, ('imax', ('isub', inner_bits, outer_offset), 0))
    collapsed = ('ubfe', a, offset, bits)
    offset_out_of_range = ('ilt', 31, offset)

    # This will be constant-folded to either 0 or the collapsed ubfe,
    # whose offset and bits operands will also be constant folded.
    return ('bcsel', offset_out_of_range, 0, collapsed)

optimizations.extend([
    # Create bitfield extract from right-shift + and pattern.
    (('iand@32', ('ushr@32(is_used_once)', a, b), '#c(is_const_bitmask)'),
     ('ubfe', a, b, ('bit_count', c)),
     'options->has_bfe && !options->avoid_ternary_with_two_constants'),

    (('iand@32', ('ushr@32', a, b), ('bfm', c, 0)),
     ('ubfe', a, b, c), 'options->has_bfe'),

    (('ushr', ('iand', a, ('bfm', c, b)), b),
     ('ubfe', a, b, c), 'options->has_bfe'),

    (('ushr@32', ('iand(is_used_once)', a, '#b(is_const_bfm)'), '#c'),
     ('bcsel', ('ilt', ('find_lsb', b), ('iand', c, 0x1f)),
      ('ushr', ('ubfe', a, ('find_lsb', b), ('bit_count', b)), ('isub', c, ('find_lsb', b))),
      ('ishl', ('ubfe', a, ('find_lsb', b), ('bit_count', b)), ('isub', ('find_lsb', b), c))),
     'options->has_bfe && !options->avoid_ternary_with_two_constants'),

    # Collapse two bitfield extracts with constant operands into a single one.
    (('ubfe', ('ubfe', a, '#b', '#c'), '#d', '#e'),
     ubfe_ubfe(a, b, c, d, e)),

    # Collapse non-zero right-shift into bitfield extract.
    (('ushr@32', ('ubfe', a, '#b', '#c'), '#d(is_5lsb_not_zero)'),
     ubfe_ubfe(a, b, c, d, 31)),
])

for log2 in range(1, 7): # powers of two from 2 to 64
   v = 1 << log2
   mask = -v
   a_is_multiple = 'a(is_unsigned_multiple_of_{})'.format(v)

   optimizations.extend([
       (('iand', a_is_multiple, mask), a),
       # Reassociate for improved CSE
       (('iand', ('iadd', a_is_multiple, b), mask), ('iadd', ('iand', b, mask), a)),
   ])

# To save space in the state tables, reduce to the set that is known to help.
# Previously, this was range(1, 32).  In addition, a couple rules inside the
# loop are commented out.  Revisit someday, probably after mesa/#2635 has some
# resolution.
for i in [1, 2, 16, 24]:
    lo_mask = 0xffffffff >> i
    hi_mask = (0xffffffff << i) & 0xffffffff

    optimizations.extend([
        # This pattern seems to only help in the soft-fp64 code.
        (('ishl@32', ('iand', 'a@32', lo_mask), i), ('ishl', a, i)),
#        (('ushr@32', ('iand', 'a@32', hi_mask), i), ('ushr', a, i)),
#        (('ishr@32', ('iand', 'a@32', hi_mask), i), ('ishr', a, i)),

        (('iand', ('ishl', 'a@32', i), hi_mask), ('ishl', a, i)),
        (('iand', ('ushr', 'a@32', i), lo_mask), ('ushr', a, i)),
#        (('iand', ('ishr', 'a@32', i), lo_mask), ('ushr', a, i)), # Yes, ushr is correct
    ])

optimizations.extend([
   # This is common for address calculations.  Reassociating may enable the
   # 'a<<c' to be CSE'd.  It also helps architectures that have an ISHLADD
   # instruction or a constant offset field for in load / store instructions.
   (('ishl', ('iadd', a, '#b'), '#c'), ('iadd', ('ishl', a, c), ('ishl', b, c))),
   (('ishl', ('iadd(is_used_once)', ('iadd', a, '#b'), c), '#d'),
    ('iadd', ('ishl', ('iadd', a, c), d), ('ishl', b, d))),
   (('ishl', ('iadd(is_used_once)', ('iadd(is_used_once)', ('iadd', a, '#b'), c), d), '#e'),
    ('iadd', ('ishl', ('iadd', ('iadd', a, c), d), e), ('ishl', b, e))),

   # (a + #b) * #c => (a * #c) + (#b * #c)
   (('imul', ('iadd(is_used_once)', a, '#b'), '#c'), ('iadd', ('imul', a, c), ('imul', b, c))),

   # ((a + #b) + c) * #d => ((a + c) * #d) + (#b * #d)
   (('imul', ('iadd(is_used_once)', ('iadd(is_used_once)', a, '#b'), c), '#d'),
    ('iadd', ('imul', ('iadd', a, c), d), ('imul', b, d))),
   (('ishl', ('iadd(is_used_once)', ('iadd(is_used_once)', a, '#b'), c), '#d'),
    ('iadd', ('ishl', ('iadd', a, c), d), ('ishl', b, d))),

   # Comparison simplifications
   (('inot', ('flt(is_used_once)', 'a(is_a_number)', 'b(is_a_number)')), ('fge', a, b)),
   (('inot', ('fge(is_used_once)', 'a(is_a_number)', 'b(is_a_number)')), ('flt', a, b)),
   (('inot', ('feq(is_used_once)', a, b)), ('fneu', a, b)),
   (('inot', ('fneu(is_used_once)', a, b)), ('feq', a, b)),
   (('inot', ('ilt(is_used_once)', a, b)), ('ige', a, b)),
   (('inot', ('ult(is_used_once)', a, b)), ('uge', a, b)),
   (('inot', ('ige(is_used_once)', a, b)), ('ilt', a, b)),
   (('inot', ('uge(is_used_once)', a, b)), ('ult', a, b)),
   (('inot', ('ieq(is_used_once)', a, b)), ('ine', a, b)),
   (('inot', ('ine(is_used_once)', a, b)), ('ieq', a, b)),

   (('iand', ('feq', a, b), ('fneu', a, b)), False),
   (('iand', ('flt', a, b), ('flt', b, a)), False),
   (('iand', ('ieq', a, b), ('ine', a, b)), False),
   (('iand', ('ilt', a, b), ('ilt', b, a)), False),
   (('iand', ('ult', a, b), ('ult', b, a)), False),

   (('ieq', 'a', ('inot', 'a')), False),
   (('ine', 'a', ('inot', 'a')), True),

   # This helps some shaders because, after some optimizations, they end up
   # with patterns like (-a < -b) || (b < a).  In an ideal world, this sort of
   # matching would be handled by CSE.
   (('flt', ('fneg', a), ('fneg', b)), ('flt', b, a)),
   (('fge', ('fneg', a), ('fneg', b)), ('fge', b, a)),
   (('feq', ('fneg', a), ('fneg', b)), ('feq', b, a)),
   (('fneu', ('fneg', a), ('fneg', b)), ('fneu', b, a)),
   (('flt', ('fneg', a), '#b'), ('flt', ('fneg', b), a)),
   (('flt', '#b', ('fneg', a)), ('flt', a, ('fneg', b))),
   (('fge', ('fneg', a), '#b'), ('fge', ('fneg', b), a)),
   (('fge', '#b', ('fneg', a)), ('fge', a, ('fneg', b))),
   (('fneu', ('fneg', a), '#b'), ('fneu', ('fneg', b), a)),
   (('feq', '#b', ('fneg', a)), ('feq', a, ('fneg', b))),
   (('flt', a, -0.0), ('flt', a, 0.0)),
   (('flt', -0.0, a), ('flt', 0.0, a)),
   (('fge', a, -0.0), ('fge', a, 0.0)),
   (('fge', -0.0, a), ('fge', 0.0, a)),
   (('fneu', a, -0.0), ('fneu', 0.0, a)),
   (('feq', -0.0, a), ('feq', a, 0.0)),

   (('ieq', ('ineg', a), '#b'),  ('ieq', a, ('ineg', b))),
   (('ine', ('ineg', a), '#b'),  ('ine', a, ('ineg', b))),
   (('ieq', ('inot', a), '#b'),  ('ieq', a, ('inot', b))),
   (('ine', ('inot', a), '#b'),  ('ine', a, ('inot', b))),
   (('ieq', ('iabs', a), 0),  ('ieq', a, 0)),
   (('ine', ('iabs', a), 0),  ('ine', a, 0)),

   (('ieq', ('bit_count(is_used_once)', a), 0),  ('ieq', a, 0)),
   (('ine', ('bit_count(is_used_once)', a), 0),  ('ine', a, 0)),

   (('ieq', ('ineg', a), ('ineg', b)), ('ieq', a, b)),
   (('ine', ('ineg', a), ('ineg', b)), ('ine', a, b)),
   (('ieq', ('inot', a), ('inot', b)), ('ieq', a, b)),
   (('ine', ('inot', a), ('inot', b)), ('ine', a, b)),

   (('fneu', ('fabs', a), 0.0), ('fneu', a, 0.0)),
   (('feq', ('fabs', a), 0.0), ('feq', a, 0.0)),
   (('fneu', ('fabs', a), ('fabs', a)), ('fneu', a, a)),
   (('feq', ('fabs', a), ('fabs', a)), ('feq', a, a)),

   # b < fsat(NaN) -> b < 0 -> false, and b < Nan -> false.
   (('flt', '#b(is_a_number_gt_0_and_lt_1)', ('fsat(is_used_once)', a)), ('flt', b, a)),

   # fsat(NaN) >= b -> 0 >= b -> false, and NaN >= b -> false.
   (('fge', ('fsat(is_used_once)', a), '#b(is_a_number_gt_0_and_lt_1)'), ('fge', a, b)),

   # b == fsat(NaN) -> b == 0 -> false, and b == NaN -> false.
   (('feq', ('fsat(is_used_once)', a), '#b(is_a_number_gt_0_and_lt_1)'), ('feq', a, b)),

   # b != fsat(NaN) -> b != 0 -> true, and b != NaN -> true.
   (('fneu', ('fsat(is_used_once)', a), '#b(is_a_number_gt_0_and_lt_1)'), ('fneu', a, b)),

   # fsat(NaN) >= 1 -> 0 >= 1 -> false, and NaN >= 1 -> false.
   (('fge', ('fsat(is_used_once)', a), 1.0), ('fge', a, 1.0)),

   # 0 < fsat(NaN) -> 0 < 0 -> false, and 0 < NaN -> false.
   (('flt', 0.0, ('fsat(is_used_once)', a)), ('flt', 0.0, a)),

   (('bcsel', ('feq', a, 'b(is_a_number_not_zero)'), b, a), a),
   (('bcsel', ('fneu', a, 'b(is_a_number_not_zero)'), a, b), a),
   (('bcsel(is_only_used_as_float_nsz)', ('feq', a, 0.0), 0.0, a),  a),
   (('bcsel(is_only_used_as_float_nsz)', ('fneu', a, 0.0), a, 0.0), a),
   (('bcsel(is_only_used_as_float_nsz)', ('feq', a, 0.0), -0.0, a),  a),
   (('bcsel(is_only_used_as_float_nsz)', ('fneu', a, 0.0), a, -0.0), a),
   (('bcsel(is_only_used_as_float_nsz)', ('ior',  ('feq', a, 0.0),  b), 0.0, a), ('bcsel', b, 0, a)),
   (('bcsel(is_only_used_as_float_nsz)', ('iand', ('fneu', a, 0.0), b), a, 0.0), ('bcsel', b, a, 0)),
   (('bcsel(is_only_used_as_float_nsz)', ('ior',  ('feq', a, 0.0),  b), -0.0, a), ('bcsel', b, 0, a)),
   (('bcsel(is_only_used_as_float_nsz)', ('iand', ('fneu', a, 0.0), b), a, -0.0), ('bcsel', b, a, 0)),
   (('bcsel', ('feq', a, 0), 0, ('fsat', ('fmul', a, 'b(is_a_number)'))), ('fsat(preserve_sz)', ('fmul', a, b))),
   (('bcsel', ('fneu', a, 0), ('fsat', ('fmul', a, 'b(is_a_number)')), 0), ('fsat(preserve_sz)', ('fmul', a, b))),
   (('bcsel', ('feq', a, 0), 0, ('fsat', ('fmul_rtz', a, 'b(is_a_number)'))), ('fsat(preserve_sz)', ('fmul_rtz', a, b))),
   (('bcsel', ('fneu', a, 0), ('fsat', ('fmul_rtz', a, 'b(is_a_number)')), 0), ('fsat(preserve_sz)', ('fmul_rtz', a, b))),
   (('bcsel', ('feq', a, 0), b, ('fadd', a, 'b(is_not_zero)')), ('fadd', a, b)),
   (('bcsel', ('fneu', a, 0), ('fadd', a, 'b(is_not_zero)'), b), ('fadd', a, b)),

   (('fneu', ('fadd', ('b2f', 'a@1'), ('b2f', 'b@1')), 0.0), ('ior', a, b)),
   (('fneu', ('b2f', 'a@1'), ('fneg', ('b2f', 'b@1'))),      ('ior', a, b)),
   (('fneu', ('fadd', ('b2f', 'a@1'), ('fneg', ('b2f', 'b@1'))), 0.0), ('ixor', a, b)),
   (('fneu',          ('b2f', 'a@1') ,          ('b2f', 'b@1') ),      ('ixor', a, b)),
   (('feq', ('fadd', ('b2f', 'a@1'), ('b2f', 'b@1')), 0.0), ('inot', ('ior', a, b))),
   (('feq', ('b2f', 'a@1'), ('fneg', ('b2f', 'b@1'))),      ('inot', ('ior', a, b))),
   (('feq', ('fadd', ('b2f', 'a@1'), ('fneg', ('b2f', 'b@1'))), 0.0), ('ieq', a, b)),
   (('feq',          ('b2f', 'a@1') ,          ('b2f', 'b@1') ),      ('ieq', a, b)),

   # 0 < b2f(a) + b2f(b)
   # 0 != b2f(a) + b2f(b)       b2f must be 0 or 1, so the sum is non-negative
   # a || b
   (('flt', 0.0, ('fadd', ('b2f', 'a@1'), ('b2f', 'b@1'))), ('ior', a, b)),

   # 0 >= b2f(a) + b2f(b)
   # 0 == b2f(a) + b2f(b)       b2f must be 0 or 1, so the sum is non-negative
   # !(a || b)
   (('fge', 0.0, ('fadd', ('b2f', 'a@1'), ('b2f', 'b@1'))), ('inot', ('ior', a, b))),

   (('flt', a, ('fneg', a)), ('flt', a, 0.0)),
   (('fge', a, ('fneg', a)), ('fge', a, 0.0)),

   # Some optimizations (below) convert things like (a < b || c < b) into
   # (min(a, c) < b).  However, this interfers with the previous optimizations
   # that try to remove comparisons with negated sums of b2f.  This just
   # breaks that apart.
   (('flt', ('fmin', c, ('fneg', ('fadd', ('b2f', 'a@1'), ('b2f', 'b@1')))), 0.0),
    ('ior', ('flt', c, 0.0), ('ior', a, b))),

   (('flt(nnan,ninf)', ('fadd', a, b), a), ('flt', b, 0.0)),
   (('fge(nnan,ninf)', ('fadd', a, b), a), ('fge', b, 0.0)),
   (('feq(nnan,ninf)', ('fadd', a, b), a), ('feq', b, 0.0)),
   (('fneu(nnan,ninf)', ('fadd', a, b), a), ('fneu', b, 0.0)),
   (('flt',  ('~fadd(is_used_once)', a, '#b(is_finite)'), '#c(is_finite)'), ('flt', a,  ('fadd', c, ('fneg', b)))),
   (('fge',  ('~fadd(is_used_once)', a, '#b(is_finite)'), '#c(is_finite)'), ('fge', a,  ('fadd', c, ('fneg', b)))),
   (('feq',  ('~fadd(is_used_once)', a, '#b(is_finite)'), '#c(is_finite)'), ('feq', a,  ('fadd', c, ('fneg', b)))),
   (('fneu', ('~fadd(is_used_once)', a, '#b(is_finite)'), '#c(is_finite)'), ('fneu', a, ('fadd', c, ('fneg', b)))),
   (('flt',  '#c(is_finite)', ('~fadd(is_used_once)', a, '#b(is_finite)')), ('flt', ('fadd', c, ('fneg', b)), a)),
   (('fge',  '#c(is_finite)', ('~fadd(is_used_once)', a, '#b(is_finite)')), ('fge', ('fadd', c, ('fneg', b)), a)),


   # Cannot remove the addition from ilt or ige due to overflow.
   (('ieq', ('iadd', a, b), a), ('ieq', b, 0)),
   (('ine', ('iadd', a, b), a), ('ine', b, 0)),

   (('ieq', 'a@1', False), ('inot', a)),
   (('ieq', 'a@1', True), a),
   (('ieq', ('b2i', 'a@1'), ('b2i', 'b@1')), ('ieq', a, b)),
   (('ine', 'a@1', False), a),
   (('ine', 'a@1', True), ('inot', a)),
   (('ine', ('b2i', 'a@1'), ('b2i', 'b@1')), ('ixor', a, b)),

   (('fneu', ('u2f', a), 0.0), ('ine', a, 0)),
   (('feq', ('u2f', a), 0.0), ('ieq', a, 0)),
   (('fge', ('u2f', a), 0.0), True),
   (('fge', 0.0, ('u2f', a)), ('ieq', 0, a)),
   (('flt', ('u2f', a), 0.0), False),
   (('flt', 0.0, ('u2f', a)), ('ine', 0, a)),
   (('fneu', ('i2f', a), 0.0), ('ine', a, 0)),
   (('feq', ('i2f', a), 0.0), ('ieq', a, 0)),
   (('fge', ('i2f', a), 0.0), ('ige', a, 0)),
   (('fge', 0.0, ('i2f', a)), ('ige', 0, a)),
   (('flt', ('i2f', a), 0.0), ('ilt', a, 0)),
   (('flt', 0.0, ('i2f', a)), ('ilt', 0, a)),

   # 0.0 < fabs(a)
   # fabs(a) > 0.0
   # fabs(a) != 0.0 because fabs(a) must be >= 0
   # a != 0.0
   (('flt(nnan)', 0.0, ('fabs', a)), ('fneu', a, 0.0)),

   # 0.0 >= fabs(a)
   # 0.0 == fabs(a)   because fabs(a) must be >= 0
   # 0.0 == a
   (('fge', 0.0, ('fabs', a)), ('feq', a, 0.0)),

   # (a >= 0.0) && (a <= 1.0) -> fsat(a) == a
   #
   # This should be NaN safe.
   #
   # NaN >= 0 && 1 >= NaN -> false && false -> false
   #
   # vs.
   #
   # NaN == fsat(NaN) -> NaN == 0 -> false
   (('iand', ('fge', a, 0.0), ('fge', 1.0, a)), ('feq', a, ('fsat', a)), '!options->lower_fsat'),

   # Note: fmin(-a, -b) == -fmax(a, b)
   (('fmax',                        ('b2f(is_used_once)', 'a@1'),           ('b2f', 'b@1')),           ('b2f', ('ior', a, b))),
   (('fmax', ('fneg(is_used_once)', ('b2f(is_used_once)', 'a@1')), ('fneg', ('b2f', 'b@1'))), ('fneg', ('b2f', ('iand', a, b)))),
   (('fmin',                        ('b2f(is_used_once)', 'a@1'),           ('b2f', 'b@1')),           ('b2f', ('iand', a, b))),
   (('fmin', ('fneg(is_used_once)', ('b2f(is_used_once)', 'a@1')), ('fneg', ('b2f', 'b@1'))), ('fneg', ('b2f', ('ior', a, b)))),

   (('bcsel', a, ('b2f', 'b@1'), 0), ('b2f', ('bcsel', a, b, False))),
   (('bcsel', a, ('b2f', 'b@1'), 1.0), ('b2f', ('bcsel', a, b, True))),
   (('bcsel', a, 0, ('b2f', 'b@1')), ('b2f', ('bcsel', a, False, b))),
   (('bcsel', a, 1.0, ('b2f', 'b@1')), ('b2f', ('bcsel', a, True, b))),

   # These patterns can occur especially when continue statements are removed
   # from loops like 'for (i = 0; i < x; i++)'.
   (('bcsel', a, ('iadd', b, -1), b), ('iadd', b, ('ineg', ('b2i', a)))),
   (('bcsel', a, ('iadd', b,  1), b), ('iadd', b,          ('b2i', a) )),
   (('bcsel(is_only_used_as_float)',     a, ('fadd', b, -1.0), b), ('fadd', b, ('fneg', ('b2f', a)))),
   (('bcsel(is_only_used_as_float_nsz)', a, ('fadd', b,  1.0), b), ('fadd', b,          ('b2f', a) )),

   (('bcsel', a, b, ('iadd', b, -1)), ('iadd', b, ('ineg', ('b2i', ('inot', a))))),
   (('bcsel', a, b, ('iadd', b,  1)), ('iadd', b,          ('b2i', ('inot', a)) )),
   (('bcsel(is_only_used_as_float)',     a, b, ('fadd', b, -1.0)), ('fadd', b, ('fneg', ('b2f', ('inot', a))))),
   (('bcsel(is_only_used_as_float_nsz)', a, b, ('fadd', b,  1.0)), ('fadd', b,          ('b2f', ('inot', a)) )),

   # Since b2i(inot(a)) == 1 - b2i(a), 1 - b2i(inot(a)) == b2i(a).
   (('iadd', '#a', ('ineg', ('b2i', ('inot', b)))), ('iadd', ('iadd', a, -1),          ('b2i', b) )),
   (('iadd', '#a',          ('b2i', ('inot', b)) ), ('iadd', ('iadd', a,  1), ('ineg', ('b2i', b)))),
   (('~fadd', '#a', ('fneg', ('b2f', ('inot', b)))), ('fadd', ('fadd', a, -1.0),          ('b2f', b) )),
   (('~fadd', '#a',          ('b2f', ('inot', b)) ), ('fadd', ('fadd', a,  1.0), ('fneg', ('b2f', b)))),

   # The preceeding patterns can create some messes that other patterns can't
   # clean up.
   (('fadd', ('fneg', ('b2f', b)), ('fneg', ('b2f', ('inot', b)))), -1.0),
   (('fadd', ('b2f', b), ('b2f', ('inot', b))), 1.0),
   (('fadd', ('fadd', a, ('fneg', ('b2f', b))), ('fneg', ('b2f', ('inot', b)))), ('fadd', a, -1.0)),
   (('fadd', ('fadd', a, ('b2f', b)), ('b2f', ('inot', b))), ('fadd', a, 1.0)),

   # fmin(b2f(a), b)
   # bcsel(a, fmin(b2f(a), b), fmin(b2f(a), b))
   # bcsel(a, fmin(b2f(True), b), fmin(b2f(False), b))
   # bcsel(a, fmin(1.0, b), fmin(0.0, b))
   #
   # Since b is a constant, constant folding will eliminate the fmin and the
   # fmax.  If b is > 1.0, the bcsel will be replaced with a b2f.
   (('fmin', ('b2f', 'a@1'), '#b'), ('bcsel', a, ('fmin', b, 1.0), ('fmin', b, 0.0))),

   (('flt', ('fadd(is_used_once)', a, ('fneg', b)), 0.0), ('flt', a, b)),

   (('bcsel(is_only_used_as_float_nsz)', ('flt', b, 'a(is_a_number)'), b, a), ('fmin(preserve_nan_inf)', a, b)),
   (('bcsel(is_only_used_as_float_nsz)', ('flt', 'a(is_a_number)', b), b, a), ('fmax(preserve_nan_inf)', a, b)),
   (('bcsel(is_only_used_as_float_nsz)', ('fge', 'a(is_a_number)', b), b, a), ('fmin(preserve_nan_inf)', a, b)),
   (('bcsel(is_only_used_as_float_nsz)', ('fge', b, 'a(is_a_number)'), b, a), ('fmax(preserve_nan_inf)', a, b)),
   (('bcsel', ('inot', a), b, c), ('bcsel', a, c, b)),
   (('bcsel', a, ('bcsel', a, b, c), d), ('bcsel', a, b, d)),
   (('bcsel', a, b, ('bcsel', a, c, d)), ('bcsel', a, b, d)),
   (('bcsel', a, ('bcsel', b, c, d), ('bcsel(is_used_once)', b, c, 'e')), ('bcsel', b, c, ('bcsel', a, d, 'e'))),
   (('bcsel', a, ('bcsel(is_used_once)', b, c, d), ('bcsel', b, c, 'e')), ('bcsel', b, c, ('bcsel', a, d, 'e'))),
   (('bcsel', a, ('bcsel', b, c, d), ('bcsel(is_used_once)', b, 'e', d)), ('bcsel', b, ('bcsel', a, c, 'e'), d)),
   (('bcsel', a, ('bcsel(is_used_once)', b, c, d), ('bcsel', b, 'e', d)), ('bcsel', b, ('bcsel', a, c, 'e'), d)),
   (('bcsel', a, True, b), ('ior', a, b)),
   (('bcsel', a, a, b), ('ior', a, b)),
   (('bcsel', a, b, False), ('iand', a, b)),
   (('bcsel', a, b, a), ('iand', a, b)),
   (('bcsel', a, b, True), ('ior', ('inot', a), b)),
   (('bcsel', a, False, b), ('iand', ('inot', a), b)),
   (('fmin', a, a), ('fcanonicalize', a)),
   (('fmax', a, a), ('fcanonicalize', a)),
   (('imin', a, a), a),
   (('imax', a, a), a),
   (('umin', a, a), a),
   (('umin', a, 0), 0),
   (('umin', a, -1), a),
   (('umax', a, a), a),
   (('umax', a, 0), a),
   (('umax', a, -1), -1),
   (('fmax', ('fmax', a, b), b), ('fmax', a, b)),
   (('umax', ('umax', a, b), b), ('umax', a, b)),
   (('imax', ('imax', a, b), b), ('imax', a, b)),
   (('fmin', ('fmin', a, b), b), ('fmin', a, b)),
   (('umin', ('umin', a, b), b), ('umin', a, b)),
   (('imin', ('imin', a, b), b), ('imin', a, b)),
   (('fmax', ('fmax', ('fmax', a, b), c), a), ('fmax', ('fmax', a, b), c)),
   (('umax', ('umax', ('umax', a, b), c), a), ('umax', ('umax', a, b), c)),
   (('imax', ('imax', ('imax', a, b), c), a), ('imax', ('imax', a, b), c)),
   (('fmin', ('fmin', ('fmin', a, b), c), a), ('fmin', ('fmin', a, b), c)),
   (('umin', ('umin', ('umin', a, b), c), a), ('umin', ('umin', a, b), c)),
   (('imin', ('imin', ('imin', a, b), c), a), ('imin', ('imin', a, b), c)),
   (('fmin', ('fmax', 'a(is_finite)', b), a), ('fmul', 1.0, a)),
   (('fmax', ('fmin', 'a(is_finite)', b), a), ('fmul', 1.0, a)),
   (('umin', ('umax', a, b), a), a),
   (('umax', ('umin', a, b), a), a),
   (('imin', ('imax', a, b), a), a),
   (('imax', ('imin', a, b), a), a),
   (('fmax(nsz)', 'a(is_a_number_not_negative)', 'b(is_not_positive)'), ('fcanonicalize', a)),
   (('fmin(nsz)', 'a(is_a_number_not_positive)', 'b(is_not_negative)'), ('fcanonicalize', a)),
   (('fmax', 'a(is_a_number_not_negative)', 'b(is_lt_zero)'), ('fcanonicalize', a)),
   (('fmin', 'a(is_a_number_not_positive)', 'b(is_gt_zero)'), ('fcanonicalize', a)),
   (('fneg', ('fmax(is_used_once)', ('fneg', a), ('fneg', b))), ('fmin', a, b)),
   (('fneg', ('fmin(is_used_once)', ('fneg', a), ('fneg', b))), ('fmax', a, b)),
   (('fneg', ('fmax(is_used_once)', ('fneg', a), '#b')), ('fmin', a, ('fneg', b))),
   (('fneg', ('fmin(is_used_once)', ('fneg', a), '#b')), ('fmax', a, ('fneg', b))),
   (('fmin(nsz)', a, -0.0), ('fmin', a, 0.0)),
   (('fmax(nsz)', a, -0.0), ('fmax', a, 0.0)),
])

for op in ['ine', 'ieq', 'ilt', 'ige', 'ult', 'uge', 'bitz', 'bitnz',
           'fneu', 'feq', 'flt', 'fge', 'fneo', 'fequ']:

    commutative = op in ['feq', 'fneu', 'fequ', 'fneo', 'ine', 'ieq']

    optimizations.extend([
        ((op, ('bcsel(is_used_once)', a, b, '#c'), '#d'), ('bcsel', a, (op, b, d), (op, c, d))),
        ((op, ('bcsel(is_used_once)', a, '#b', c), '#d'), ('bcsel', a, (op, b, d), (op, c, d))),
        ((op, ('bcsel', a, '#b', '#c'), '#d'), ('bcsel', a, (op, b, d), (op, c, d))),
    ])

    if not commutative:
        optimizations.extend([
            ((op, '#d', ('bcsel(is_used_once)', a, b, '#c')), ('bcsel', a, (op, d, b), (op, d, c))),
            ((op, '#d', ('bcsel(is_used_once)', a, '#b', c)), ('bcsel', a, (op, d, b), (op, d, c))),
            ((op, '#d', ('bcsel', a, '#b', '#c')), ('bcsel', a, (op, d, b), (op, d, c))),
        ])

    if not op in ['bitz', 'bitnz']:
        optimizations.extend([
            ((op, ('b2f', 'a@1'), '#b'), ('bcsel', a, (op, 1.0, b), (op, 0.0, b))),
            ((op, ('b2i', 'a@1'), '#b'), ('bcsel', a, (op, 1, b), (op, 0, b))),
        ])

    if not op in ['bitz', 'bitnz'] and not commutative:
        optimizations.extend([
            ((op, '#b', ('b2f', 'a@1')), ('bcsel', a, (op, b, 1.0), (op, b, 0.0))),
            ((op, '#b', ('b2i', 'a@1')), ('bcsel', a, (op, b, 1), (op, b, 0))),
        ])

# Integer sizes
for s in [8, 16, 32, 64]:
    optimizations.extend([
       (('iand@{}'.format(s), a, ('inot', ('ishr', a, s - 1))), ('imax', a, 0)),

       # Simplify logic to detect sign of an integer.
       (('ieq', ('iand', 'a@{}'.format(s), 1 << (s - 1)), 0),            ('ige', a, 0)),
       (('ine', ('iand', 'a@{}'.format(s), 1 << (s - 1)), 1 << (s - 1)), ('ige', a, 0)),
       (('ine', ('iand', 'a@{}'.format(s), 1 << (s - 1)), 0),            ('ilt', a, 0)),
       (('ieq', ('iand', 'a@{}'.format(s), 1 << (s - 1)), 1 << (s - 1)), ('ilt', a, 0)),
       (('ine', ('ushr', 'a@{}'.format(s), s - 1), 0), ('ilt', a, 0)),
       (('ieq', ('ushr', 'a@{}'.format(s), s - 1), 0), ('ige', a, 0)),
       (('ieq', ('ushr', 'a@{}'.format(s), s - 1), 1), ('ilt', a, 0)),
       (('ine', ('ushr', 'a@{}'.format(s), s - 1), 1), ('ige', a, 0)),
       (('ine', ('ishr', 'a@{}'.format(s), s - 1), 0), ('ilt', a, 0)),
       (('ieq', ('ishr', 'a@{}'.format(s), s - 1), 0), ('ige', a, 0)),
       (('ieq', ('ishr', 'a@{}'.format(s), s - 1), -1), ('ilt', a, 0)),
       (('ine', ('ishr', 'a@{}'.format(s), s - 1), -1), ('ige', a, 0)),
    ])

optimizations.extend([
   (('fmin', a, ('fneg', a)), ('fneg', ('fabs', a))),
   (('imin', a, ('ineg', a)), ('ineg', ('iabs', a))),
   (('fmin', a, ('fneg', ('fabs', a))), ('fneg', ('fabs', a))),
   (('imin', a, ('ineg', ('iabs', a))), ('ineg', ('iabs', a))),
   (('fmin', a, ('fabs', a)), ('fcanonicalize', a)),
   (('imin', a, ('iabs', a)), a),
   (('fmax', a, ('fneg', ('fabs', a))), ('fcanonicalize', a)),
   (('imax', a, ('ineg', ('iabs', a))), a),
   (('fmax', a, ('fabs', a)), ('fabs', a)),
   (('imax', a, ('iabs', a)), ('iabs', a)),
   (('fmax', a, ('fneg', a)), ('fabs', a)),
   (('imax', a, ('ineg', a)), ('iabs', a), '!options->lower_iabs'),
   (('fmax(nnan)', ('fabs', a), 0.0), ('fabs', a)),
   (('fmin', ('fmax', a, 0.0), 1.0), ('fsat', a), '!options->lower_fsat'),
   # fmax(fmin(a, 1.0), 0.0) is not NaN correct because it returns 1.0 on NaN, while
   # fsat(a) returns 0.0.
   (('fmax', ('fmin(nnan)', a, 1.0), 0.0), ('fsat', a), '!options->lower_fsat'),
   # fmin(fmax(a, -1.0), 0.0) is not NaN/signed zero correct because it returns -1.0 on NaN, while
   # fneg(fsat(fneg(a))) returns -0.0 on NaN.
   (('fmin(nsz)', ('fmax(nnan)', a, -1.0),  0.0), ('fneg', ('fsat', ('fneg', a))), '!options->lower_fsat'),
   # fmax(fmin(a, 0.0), -1.0) is inexact because it returns 0.0 on NaN, while
   # fneg(fsat(fneg(a))) returns -0.0 on NaN.
   (('fmax', ('fmin(nsz)', a,  0.0), -1.0), ('fneg', ('fsat', ('fneg', a))), '!options->lower_fsat'),
   # fsat(fsign(NaN)) = fsat(0) = 0, and b2f(0 < NaN) = b2f(False) = 0. Mark
   # the new comparison precise to prevent it being changed to 'a != 0'.
   (('fsat', ('fsign', a)), ('b2f', ('flt(preserve_nan_inf)', 0.0, a))),
   (('fsat', ('b2f', a)), ('b2f', a)),
   (('fsat', a), ('fmin', ('fmax', a, 0.0), 1.0), 'options->lower_fsat'),
   (('fsat', ('fsat', a)), ('fsat', a)),
   (('fsat', ('fneg(is_used_once)', ('fadd(is_used_once)', a, b))), ('fsat', ('fadd', ('fneg', a), ('fneg', b))), '!options->lower_fsat'),
   (('fsat', ('fneg(is_used_once)', ('fmul(is_used_once)', a, b))), ('fsat', ('fmul', ('fneg', a), b)), '!options->lower_fsat'),
   (('fsat', ('fneg(is_used_once)', ('fmul_rtz(is_used_once)', a, b))), ('fsat', ('fmul_rtz', ('fneg', a), b)), '!options->lower_fsat'),
   (('fsat(nsz)', ('fneg(is_used_once)', ('fmulz(is_used_once)', a, b))), ('fsat', ('fmulz', ('fneg', a), b)), '!options->lower_fsat'),
   (('fsat', ('fabs(is_used_once)', ('fmul(is_used_once)', a, b))), ('fsat', ('fmul', ('fabs', a), ('fabs', b))), '!options->lower_fsat'),
   (('fsat', ('fabs(is_used_once)', ('fmul_rtz(is_used_once)', a, b))), ('fsat', ('fmul_rtz', ('fabs', a), ('fabs', b))), '!options->lower_fsat'),
   (('fmin', ('fmax', ('fmin', ('fmax', a, b), c), b), c), ('fmin', ('fmax', a, b), c)),
   (('imin', ('imax', ('imin', ('imax', a, b), c), b), c), ('imin', ('imax', a, b), c)),
   (('umin', ('umax', ('umin', ('umax', a, b), c), b), c), ('umin', ('umax', a, b), c)),
   # Both the left and right patterns are "b" when isnan(a), so this is exact.
   (('fmax', ('fsat', a), '#b(is_zero_to_one)'), ('fsat', ('fmax', a, b))),
   (('fmax', ('fsat(is_used_once)', a), ('fsat(is_used_once)', b)), ('fsat', ('fmax', a, b))),
   # The left pattern is 0.0 when isnan(a) (because fmin(fsat(NaN), b) ->
   # fmin(0.0, b)) while the right one is "b", so this optimization is not NaN correct.
   (('fmin(nsz)', ('fsat(nnan)', a), '#b(is_zero_to_one)'), ('fsat', ('fmin', a, b))),

   (('fsat', 'a(is_a_number_ge_pos_one)'), 1.0),
   (('fsat', 'a(is_not_positive)'), 0.0),

   (('fsat(nsz)', 'a(is_a_number_zero_to_one)'), ('fcanonicalize', a)),

   # Let constant folding do its job. This can have emergent behaviour.
   (('fneg', ('bcsel(is_used_once)', a, '#b', '#c')), ('bcsel', a, ('fneg', b), ('fneg', c))),

   # max(-min(b, a), b) -> max(abs(b), -a)
   # min(-max(b, a), b) -> min(-abs(b), -a)
   (('fmax', ('fneg', ('fmin', b, a)), b), ('fmax', ('fabs', b), ('fneg', a))),
   (('fmin', ('fneg', ('fmax', b, a)), b), ('fmin', ('fneg', ('fabs', b)), ('fneg', a))),

   # This should be NaN safe since max(NaN, 0) = fsat(NaN) = 0.
   (('fmax', 'a(is_le_pos_one)', 0.0), ('fsat', a), '!options->lower_fsat'),

   (('fmin(nsz)', 'a(is_a_number_not_negative)', 1.0), ('fsat', a), '!options->lower_fsat'),

   (('fsat', ('fmax', a, 'b(is_not_positive)')), ('fsat', a)),
   (('fsat', ('fmin', 'a(is_a_number)', 'b(is_ge_pos_one)')), ('fsat', a)),

   (('fsat', ('bcsel(is_used_once)', a, b, '#c')), ('bcsel', a, ('fsat', b), ('fsat', c))),
   (('fsat', ('bcsel(is_used_once)', a, '#b', c)), ('bcsel', a, ('fsat', b), ('fsat', c))),

   (('extract_u8', ('imin', ('imax', a, 0), 0xff), 0), ('imin', ('imax', a, 0), 0xff)),

   # The ior versions are exact because fmin and fmax will always pick a
   # non-NaN value, if one exists.  Therefore (a < NaN) || (a < c) == a <
   # fmax(NaN, c) == a < c. If the source comparisons were NaN preserving,
   # so should be the replacement, which prevents further optimizations
   (('ior', ('flt(is_used_once)', a, b), ('flt', a, c)), ('flt', a, ('fmax', b, c))),
   (('ior', ('flt(is_used_once)', a, c), ('flt', b, c)), ('flt', ('fmin', a, b), c)),
   (('ior', ('fge(is_used_once)', a, b), ('fge', a, c)), ('fge', a, ('fmin', b, c))),
   (('ior', ('fge(is_used_once)', a, c), ('fge', b, c)), ('fge', ('fmax', a, b), c)),
   (('ior', ('flt', a, '#b'), ('flt', a, '#c')), ('flt', a, ('fmax', b, c))),
   (('ior', ('flt', '#a', c), ('flt', '#b', c)), ('flt', ('fmin', a, b), c)),
   (('ior', ('fge', a, '#b'), ('fge', a, '#c')), ('fge', a, ('fmin', b, c))),
   (('ior', ('fge', '#a', c), ('fge', '#b', c)), ('fge', ('fmax', a, b), c)),
   (('iand', ('flt(is_used_once,nnan)', a, b), ('flt(nnan)', a, c)), ('flt', a, ('fmin', b, c))),
   (('iand', ('flt(is_used_once,nnan)', a, c), ('flt(nnan)', b, c)), ('flt', ('fmax', a, b), c)),
   (('iand', ('fge(is_used_once,nnan)', a, b), ('fge(nnan)', a, c)), ('fge', a, ('fmax', b, c))),
   (('iand', ('fge(is_used_once,nnan)', a, c), ('fge(nnan)', b, c)), ('fge', ('fmin', a, b), c)),
   (('iand', ('flt', a, '#b(is_a_number)'), ('flt', a, '#c(is_a_number)')), ('flt', a, ('fmin', b, c))),
   (('iand', ('flt', '#a(is_a_number)', c), ('flt', '#b(is_a_number)', c)), ('flt', ('fmax', a, b), c)),
   (('iand', ('fge', a, '#b(is_a_number)'), ('fge', a, '#c(is_a_number)')), ('fge', a, ('fmax', b, c))),
   (('iand', ('fge', '#a(is_a_number)', c), ('fge', '#b(is_a_number)', c)), ('fge', ('fmin', a, b), c)),

   # Law of trichotomy. This pattern is load-bearing on AGX for optimizing
   # emulated transform feedback.
   (('iand', ('uge', a, b), ('ult', a, b)), False),

   # A number of shaders contain a pattern like a.x < 0.0 || a.x > 1.0 || a.y
   # < 0.0, || a.y > 1.0 || ...  These patterns rearrange and replace in a
   # single step.  Doing just the replacement can lead to an infinite loop as
   # the pattern is repeatedly applied to the result of the previous
   # application of the pattern.
   (('ior', ('ior(is_used_once)', ('flt(is_used_once)', a, c), d), ('flt', b, c)), ('ior', ('flt', ('fmin', a, b), c), d)),
   (('ior', ('ior(is_used_once)', ('flt', a, c), d), ('flt(is_used_once)', b, c)), ('ior', ('flt', ('fmin', a, b), c), d)),
   (('ior', ('ior(is_used_once)', ('flt(is_used_once)', a, b), d), ('flt', a, c)), ('ior', ('flt', a, ('fmax', b, c)), d)),
   (('ior', ('ior(is_used_once)', ('flt', a, b), d), ('flt(is_used_once)', a, c)), ('ior', ('flt', a, ('fmax', b, c)), d)),

   # This is how SpvOpFOrdNotEqual might be implemented.  If both values are
   # numbers, then it can be replaced with fneu.
   (('ior', ('flt', 'a(is_a_number)', 'b(is_a_number)'), ('flt', b, a)), ('fneu', a, b)),

   # Other patterns may optimize the resulting iand tree further.
   (('umin', ('iand', a, '#b(is_pos_power_of_two)'), ('iand', c, b)),
    ('iand', ('iand', a, b), ('iand', c, b))),
])

# Float sizes
for s in [16, 32, 64]:
    if s == 64:
        match_fsign_cond = "!options->lower_fsign & !(options->lower_doubles_options & nir_lower_dsign)"
    else:
        match_fsign_cond = "!options->lower_fsign"
    optimizations.extend([
       # These derive from the previous patterns with the application of b < 0 <=>
       # 0 < -b.  The transformation should be applied if either comparison is
       # used once as this ensures that the number of comparisons will not
       # increase.  The sources to the ior and iand are not symmetric, so the
       # rules have to be duplicated to get this behavior.
       (('ior', ('flt(is_used_once)', 0.0, 'a@{}'.format(s)), ('flt', 'b@{}'.format(s), 0.0)), ('flt', 0.0, ('fmax', a, ('fneg', b)))),
       (('ior', ('flt', 0.0, 'a@{}'.format(s)), ('flt(is_used_once)', 'b@{}'.format(s), 0.0)), ('flt', 0.0, ('fmax', a, ('fneg', b)))),
       (('ior', ('fge(is_used_once)', 0.0, 'a@{}'.format(s)), ('fge', 'b@{}'.format(s), 0.0)), ('fge', 0.0, ('fmin', a, ('fneg', b)))),
       (('ior', ('fge', 0.0, 'a@{}'.format(s)), ('fge(is_used_once)', 'b@{}'.format(s), 0.0)), ('fge', 0.0, ('fmin', a, ('fneg', b)))),
       (('iand', ('flt(is_used_once,nnan)', 0.0, 'a@{}'.format(s)), ('flt(nnan)', 'b@{}'.format(s), 0.0)), ('flt', 0.0, ('fmin', a, ('fneg', b)))),
       (('iand', ('flt(nnan)', 0.0, 'a@{}'.format(s)), ('flt(is_used_once,nnan)', 'b@{}'.format(s), 0.0)), ('flt', 0.0, ('fmin', a, ('fneg', b)))),
       (('iand', ('fge(is_used_once,nnan)', 0.0, 'a@{}'.format(s)), ('fge(nnan)', 'b@{}'.format(s), 0.0)), ('fge', 0.0, ('fmax', a, ('fneg', b)))),
       (('iand', ('fge(nnan)', 0.0, 'a@{}'.format(s)), ('fge(is_used_once,nnan)', 'b@{}'.format(s), 0.0)), ('fge', 0.0, ('fmax', a, ('fneg', b)))),

       (('ior', ('feq(is_used_once)', 'a@{}'.format(s), 0.0), ('feq', 'b@{}'.format(s), 0.0)), ('feq', ('fmin', ('fabs', a), ('fabs', b)), 0.0)),
       (('ior', ('fneu(is_used_once)', 'a@{}'.format(s), 0.0), ('fneu', 'b@{}'.format(s), 0.0)), ('fneu', ('fadd', ('fabs', a), ('fabs', b)), 0.0)),
       (('iand', ('feq(is_used_once)', 'a@{}'.format(s), 0.0), ('feq', 'b@{}'.format(s), 0.0)), ('feq', ('fadd', ('fabs', a), ('fabs', b)), 0.0)),
       (('iand', ('fneu(is_used_once)', 'a@{}'.format(s), 0.0), ('fneu', 'b@{}'.format(s), 0.0)), ('fneu', ('fmin', ('fabs', a), ('fabs', b)), 0.0)),

       # The (i2f32, ...) part is an open-coded fsign.  When that is combined
       # with the bcsel, it's basically copysign(1.0, a).  There are some
       # behavior differences between this pattern and copysign w.r.t. ±0 and
       # NaN.  copysign(x, y) blindly takes the sign bit from y and applies it
       # to x, regardless of whether either or both values are NaN.
       #
       # If a != a: bcsel(False, 1.0, i2f(b2i(False) - b2i(False))) = 0,
       #            int(NaN >= 0.0) - int(NaN < 0.0) = 0 - 0 = 0
       # If a == ±0: bcsel(True, 1.0, ...) = 1.0,
       #            int(±0.0 >= 0.0) - int(±0.0 < 0.0) = 1 - 0 = 1
       #
       # For all other values of 'a', the original and replacement behave as
       # copysign.
       #
       # Note: Use b2i32 in the replacement because some platforms that
       # support fp16 don't support int16.
       (('bcsel@{}'.format(s), ('feq', a, 0.0), 1.0, ('i2f{}'.format(s), ('iadd', ('b2i{}'.format(s), ('flt', 0.0, 'a@{}'.format(s))), ('ineg', ('b2i{}'.format(s), ('flt', 'a@{}'.format(s), 0.0)))))),
        ('i2f{}'.format(s), ('iadd', ('b2i32', ('fge', a, 0.0)), ('ineg', ('b2i32', ('flt', a, 0.0)))))),

       # Signed pow() used in Control. It's not enough to match just the
       # copysign piece because we would require extra instructions to handle
       # the various floating point special cases. Specializing to the pow
       # sequence lets us collapse all the sign fixup code.
       (('fmul', ('fexp2', ('fmul', ('flog2', ('fabs', a)), b)),
         ('i2f', ('iadd',           ('b2i', ('flt', 0.0, a)),
                          ('ineg', ('b2i', ('flt', a, 0.0)))))),

        ('bcsel', ('flt', a, 0.0),
         ('fneg', ('fexp2', ('fmul', ('flog2', ('fabs', a)), b))),
                  ('fexp2', ('fmul', ('flog2', ('fabs', a)), b))), 'true', TestStatus.XFAIL), # XFAIL is that a=0.0, b=-1.0 ends up producing inf instead of NaN (thanks to eliding an fmul(inf, 0.0))

       # The C spec says, "If the value of the integral part cannot be represented
       # by the integer type, the behavior is undefined."  "Undefined" can mean
       # "the conversion doesn't happen at all."
       (('~i2f{}'.format(s), ('f2i', 'a@{}'.format(s))), ('ftrunc', a)),

       # Ironically, mark these as imprecise because removing the conversions may
       # preserve more precision than doing the conversions (e.g.,
       # uint(float(0x81818181u)) == 0x81818200).
       (('~f2i{}'.format(s), ('i2f', 'a@{}'.format(s))), a),
       (('~f2i{}'.format(s), ('u2f', 'a@{}'.format(s))), a),
       (('~f2u{}'.format(s), ('i2f', 'a@{}'.format(s))), a),
       (('~f2u{}'.format(s), ('u2f', 'a@{}'.format(s))), a),

       (('fadd', ('b2f{}'.format(s), ('flt', 0.0, 'a@{}'.format(s))), ('fneg', ('b2f{}'.format(s), ('flt', 'a@{}'.format(s), 0.0)))), ('fsign', a), match_fsign_cond),
       (('iadd', ('b2i{}'.format(s), ('flt', 0, 'a@{}'.format(s))), ('ineg', ('b2i{}'.format(s), ('flt', 'a@{}'.format(s), 0)))), ('f2i{}'.format(s), ('fsign', a)), match_fsign_cond),

       # float? -> float? -> floatS ==> float? -> floatS
       (('~f2f{}'.format(s), ('f2f', a)), ('f2f{}'.format(s), a)),

       # int? -> float? -> floatS ==> int? -> floatS
       (('~f2f{}'.format(s), ('u2f', a)), ('u2f{}'.format(s), a)),
       (('~f2f{}'.format(s), ('i2f', a)), ('i2f{}'.format(s), a)),

       # float? -> float? -> intS ==> float? -> intS
       (('~f2u{}'.format(s), ('f2f', a)), ('f2u{}'.format(s), a)),
       (('~f2i{}'.format(s), ('f2f', a)), ('f2i{}'.format(s), a)),

       # HLSL's sign function returns an integer
       (('i2f{}'.format(s), ('f2i', ('fsign', 'a@{}'.format(s)))), ('fsign', a)),
    ])

    if s == 32:
        optimizations.extend([(('bcsel', a, ('b2f(is_used_once)', 'b@{}'.format(s)), ('b2f', 'c@{}'.format(s))), ('b2f', ('bcsel', a, b, c)))])

    for B in [32, 64]:
        if s < B:
            optimizations.extend([
               # S = smaller, B = bigger
               # floatS -> floatB -> floatS ==> identity
               (('f2f{}'.format(s), ('f2f{}'.format(B), 'a@{}'.format(s))), ('fcanonicalize', a)),

               # floatS -> floatB -> intB ==> floatS -> intB
               (('f2u{}'.format(B), ('f2f{}'.format(B), 'a@{}'.format(s))), ('f2u{}'.format(B), a)),
               (('f2i{}'.format(B), ('f2f{}'.format(B), 'a@{}'.format(s))), ('f2i{}'.format(B), a)),

               # int? -> floatB -> floatS ==> int? -> floatS
               (('f2f{}'.format(s), ('u2f{}'.format(B), a)), ('u2f{}'.format(s), a)),
               (('f2f{}'.format(s), ('i2f{}'.format(B), a)), ('i2f{}'.format(s), a)),
            ])

for S in [1, 8, 16, 32]:
    for B in [8, 16, 32, 64]:
        if B <= S:
            continue
        optimizations.extend([
            # intS -> intB -> intS ==> identity
            (('i2i{}'.format(S), ('i2i{}'.format(B), 'a@{}'.format(S))), a),
            (('u2u{}'.format(S), ('u2u{}'.format(B), 'a@{}'.format(S))), a),
        ])

for S in [1, 8, 16, 32]:
    for B in [16, 32, 64]:
        for C in [8, 16, 32, 64]:
            if C <= S:
                continue
            optimizations.extend([
                # intS -> intC -> floatB ==> intS -> floatB
                (('u2f{}'.format(B), ('u2u{}'.format(C), 'a@{}'.format(S))), ('u2f{}'.format(B), a)),
                (('i2f{}'.format(B), ('i2i{}'.format(C), 'a@{}'.format(S))), ('i2f{}'.format(B), a)),
            ])

# mediump variants of the above
optimizations.extend([
    # int32 -> float32 -> float16 ==> int32 -> float16
    (('f2fmp', ('u2f32', 'a@32')), ('u2fmp', a), 'true', TestStatus.UNSUPPORTED),
    (('f2fmp', ('i2f32', 'a@32')), ('i2fmp', a), 'true', TestStatus.UNSUPPORTED),

    # float32 -> float16 -> int16 ==> float32 -> int16
    (('f2u16', ('f2fmp', 'a@32')), ('f2u16', a), 'true', TestStatus.UNSUPPORTED),
    (('f2i16', ('f2fmp', 'a@32')), ('f2i16', a), 'true', TestStatus.UNSUPPORTED),

    # float32 -> int32 -> int16 ==> float32 -> int16
    (('i2imp', ('f2u32', 'a@32')), ('f2ump', a), 'true', TestStatus.UNSUPPORTED),
    (('i2imp', ('f2i32', 'a@32')), ('f2imp', a), 'true', TestStatus.UNSUPPORTED),

    # int32 -> int16 -> float16 ==> int32 -> float16
    (('u2f16', ('i2imp', 'a@32')), ('u2f16', a), 'true', TestStatus.UNSUPPORTED),
    (('i2f16', ('i2imp', 'a@32')), ('i2f16', a), 'true', TestStatus.UNSUPPORTED),
])

# Clean up junk left from 8-bit integer to 16-bit integer lowering.
optimizations.extend([
    # The u2u16(u2u8(X)) just masks off the upper 8-bits of X.  This can be
    # accomplished by mask the upper 8-bit of the immediate operand to the
    # iand instruction.  Often times, both patterns will end up being applied
    # to the same original expression tree.
    (('iand', ('u2u16', ('u2u8', 'a@16')), '#b'),               ('iand', a, ('iand', b, 0xff))),
    (('u2u16', ('u2u8(is_used_once)', ('iand', 'a@16', '#b'))), ('iand', a, ('iand', b, 0xff))),
])

for op in ['iand', 'ior', 'ixor']:
    optimizations.extend([
        (('u2u8', (op, ('u2u16', ('u2u8', 'a@16')), ('u2u16', ('u2u8', 'b@16')))), ('u2u8', (op, a, b))),
        (('u2u8', (op, ('u2u16', ('u2u8', 'a@32')), ('u2u16', ('u2u8', 'b@32')))), ('u2u8', (op, a, b))),

        # Undistribute extract from a logic op
        ((op, ('extract_i8', a, '#b'), ('extract_i8', c, b)), ('extract_i8', (op, a, c), b)),
        ((op, ('extract_u8', a, '#b'), ('extract_u8', c, b)), ('extract_u8', (op, a, c), b)),
        ((op, ('extract_i16', a, '#b'), ('extract_i16', c, b)), ('extract_i16', (op, a, c), b)),
        ((op, ('extract_u16', a, '#b'), ('extract_u16', c, b)), ('extract_u16', (op, a, c), b)),

        # Undistribute shifts from a logic op
        ((op, ('ushr(is_used_once)', a, '#b'), ('ushr', c, b)), ('ushr', (op, a, c), b)),
        ((op, ('ishr(is_used_once)', a, '#b'), ('ishr', c, b)), ('ishr', (op, a, c), b)),
        ((op, ('ishl(is_used_once)', a, '#b'), ('ishl', c, b)), ('ishl', (op, a, c), b)),
    ])

# Integer sizes
for s in [8, 16, 32, 64]:
    amount_bits = int(math.log2(s))

    lower_umin = 'options->lower_umin'
    lower_umax = 'options->lower_umax'
    lower_imin = 'false'
    lower_imax = 'false'
    lower_ior = 'options->lower_bitops'
    if s == 64:
       lower_umin = '(options->lower_umin || (options->lower_int64_options & nir_lower_minmax64) != 0)'
       lower_umax = '(options->lower_umax || (options->lower_int64_options & nir_lower_minmax64) != 0)'
       lower_imin = '((options->lower_int64_options & nir_lower_minmax64) != 0)'
       lower_imax = '((options->lower_int64_options & nir_lower_minmax64) != 0)'
       lower_ior = '(options->lower_bitops || (options->lower_int64_options & nir_lower_logic64) != 0)'

    optimizations.extend([
       (('iand', ('ieq', 'a@{}'.format(s), 0), ('ieq', 'b@{}'.format(s), 0)), ('ieq', ('ior', a, b), 0), lower_umax + ' && !' + lower_ior),
       (('ior',  ('ine', 'a@{}'.format(s), 0), ('ine', 'b@{}'.format(s), 0)), ('ine', ('ior', a, b), 0), lower_umin + ' && !' + lower_ior),
       (('iand', ('ieq', 'a@{}'.format(s), 0), ('ieq', 'b@{}'.format(s), 0)), ('ieq', ('umax', a, b), 0), '!'+lower_umax),
       (('ior',  ('ieq', 'a@{}'.format(s), 0), ('ieq', 'b@{}'.format(s), 0)), ('ieq', ('umin', a, b), 0), '!'+lower_umin),
       (('iand', ('ine', 'a@{}'.format(s), 0), ('ine', 'b@{}'.format(s), 0)), ('ine', ('umin', a, b), 0), '!'+lower_umin),
       (('ior',  ('ine', 'a@{}'.format(s), 0), ('ine', 'b@{}'.format(s), 0)), ('ine', ('umax', a, b), 0), '!'+lower_umax),

       (('bcsel', ('ult', 'b@{}'.format(s), a), b, a), ('umin', a, b), '!'+lower_umin),
       (('bcsel', ('ult', 'a@{}'.format(s), b), b, a), ('umax', a, b), '!'+lower_umax),
       (('bcsel', ('uge', 'a@{}'.format(s), b), b, a), ('umin', a, b), '!'+lower_umin),
       (('bcsel', ('uge', 'b@{}'.format(s), a), b, a), ('umax', a, b), '!'+lower_umax),
       (('bcsel', ('ilt', 'b@{}'.format(s), a), b, a), ('imin', a, b), '!'+lower_imin),
       (('bcsel', ('ilt', 'a@{}'.format(s), b), b, a), ('imax', a, b), '!'+lower_imax),
       (('bcsel', ('ige', 'a@{}'.format(s), b), b, a), ('imin', a, b), '!'+lower_imin),
       (('bcsel', ('ige', 'b@{}'.format(s), a), b, a), ('imax', a, b), '!'+lower_imax),

       # SM5 32-bit shifts are defined to use the 5 least significant bits (or 4 bits for 16 bits)
       (('ishl', 'a@{}'.format(s), ('iand', s - 1, b)), ('ishl', a, b)),
       (('ishr', 'a@{}'.format(s), ('iand', s - 1, b)), ('ishr', a, b)),
       (('ushr', 'a@{}'.format(s), ('iand', s - 1, b)), ('ushr', a, b)),
       (('ushr', 'a@{}'.format(s), ('ishl(is_used_once)', ('iand', b, 1), amount_bits - 1)), ('ushr', a, ('ishl', b, amount_bits - 1))),
       (('ushr', 'a@{}'.format(s), ('ishl(is_used_once)', ('iand', b, 3), amount_bits - 2)), ('ushr', a, ('ishl', b, amount_bits - 2))),

       (('ior', ('ilt(is_used_once)', f'a@{s}', b), ('ilt', a, c)), ('ilt', a, ('imax', b, c)), '!'+lower_imax),
       (('ior', ('ilt(is_used_once)', f'a@{s}', c), ('ilt', b, c)), ('ilt', ('imin', a, b), c), '!'+lower_imin),
       (('ior', ('ige(is_used_once)', f'a@{s}', b), ('ige', a, c)), ('ige', a, ('imin', b, c)), '!'+lower_imin),
       (('ior', ('ige(is_used_once)', f'a@{s}', c), ('ige', b, c)), ('ige', ('imax', a, b), c), '!'+lower_imax),
       (('ior', ('ult(is_used_once)', f'a@{s}', b), ('ult', a, c)), ('ult', a, ('umax', b, c)), '!'+lower_umax),
       (('ior', ('ult(is_used_once)', f'a@{s}', c), ('ult', b, c)), ('ult', ('umin', a, b), c), '!'+lower_umin),
       (('ior', ('uge(is_used_once)', f'a@{s}', b), ('uge', a, c)), ('uge', a, ('umin', b, c)), '!'+lower_umin),
       (('ior', ('uge(is_used_once)', f'a@{s}', c), ('uge', b, c)), ('uge', ('umax', a, b), c), '!'+lower_umax),
       (('iand', ('ilt(is_used_once)', f'a@{s}', b), ('ilt', a, c)), ('ilt', a, ('imin', b, c)), '!'+lower_imin),
       (('iand', ('ilt(is_used_once)', f'a@{s}', c), ('ilt', b, c)), ('ilt', ('imax', a, b), c), '!'+lower_imax),
       (('iand', ('ige(is_used_once)', f'a@{s}', b), ('ige', a, c)), ('ige', a, ('imax', b, c)), '!'+lower_imax),
       (('iand', ('ige(is_used_once)', f'a@{s}', c), ('ige', b, c)), ('ige', ('imin', a, b), c), '!'+lower_imin),
       (('iand', ('ult(is_used_once)', f'a@{s}', b), ('ult', a, c)), ('ult', a, ('umin', b, c)), '!'+lower_umin),
       (('iand', ('ult(is_used_once)', f'a@{s}', c), ('ult', b, c)), ('ult', ('umax', a, b), c), '!'+lower_umax),
       (('iand', ('uge(is_used_once)', f'a@{s}', b), ('uge', a, c)), ('uge', a, ('umax', b, c)), '!'+lower_umax),
       (('iand', ('uge(is_used_once)', f'a@{s}', c), ('uge', b, c)), ('uge', ('umin', a, b), c), '!'+lower_umin),
    ])

    # There are no 64bit booleans in NIR
    if s == 32:
        # True/False are ~0 and 0 in NIR.  b2i of True is 1, and -1 is ~0 (True).
        optimizations.extend([(('ineg', ('b2i{}'.format(s), 'a@{}'.format(s))), a)])

optimizations.extend([
   # Common pattern like 'if (i == 0 || i == 1 || ...)'
   (('ior', ('ieq', a, 0), ('ieq', a, 1)), ('uge', 1, a)),
   (('ior', ('uge', 1, a), ('ieq', a, 2)), ('uge', 2, a)),
   (('ior', ('uge', 2, a), ('ieq', a, 3)), ('uge', 3, a)),

   # Similar to the previous patterns, but the association of the comparisons
   # is differnet.
   (('ior', ('ieq', a, 0), ('ior', ('ieq', a, 1), b)), ('ior', ('uge', 1, a), b)),
   (('ior', ('uge', 1, a), ('ior', ('ieq', a, 2), b)), ('ior', ('uge', 2, a), b)),
   (('ior', ('uge', 2, a), ('ior', ('ieq', a, 3), b)), ('ior', ('uge', 3, a), b)),

   (('ior', a, ('ieq', a, False)), True),

   (('uge', a, 1), ('ine', a, 0)),
   (('ult', a, 1), ('ieq', a, 0)),
   (('uge', 0, a), ('ieq', a, 0)),
   (('ult', 0, a), ('ine', a, 0)),

   (('b2i', ('ine', 'a@1', 'b@1')), ('b2i', ('ixor', a, b))),

   (('ishl', ('b2i32', ('ine', ('iand', 'a@32', '#b(is_pos_power_of_two)'), 0)), '#c'),
    ('bcsel', ('ige', ('iand', c, 31), ('find_lsb', b)),
              ('ishl', ('iand', a, b), ('iadd', ('iand', c, 31), ('ineg', ('find_lsb', b)))),
              ('ushr', ('iand', a, b), ('iadd', ('ineg', ('iand', c, 31)), ('find_lsb', b)))
    )
   ),

   (('b2i32', ('ine', ('iand', 'a@32', '#b(is_pos_power_of_two)'), 0)),
    ('ushr', ('iand', a, b), ('find_lsb', b)), '!options->lower_bitops'),

   (('ior',  ('b2i', a), ('iand', b, 1)), ('iand', ('ior', ('b2i', a), b), 1)),
   (('iand', ('b2i', a), ('iand', b, 1)), ('iand', ('b2i', a), b)),

   # This pattern occurs coutresy of __flt64_nonnan in the soft-fp64 code.
   # The first part of the iand comes from the !__feq64_nonnan.
   #
   # The second pattern is a reformulation of the first based on the relation
   # (a == 0 || y == 0) <=> umin(a, y) == 0, where b in the first equation
   # happens to be y == 0.
   (('iand', ('inot', ('iand', ('ior', ('ieq', a, 0),  b), c)), ('ilt', a, 0)),
    ('iand', ('inot', ('iand',                         b , c)), ('ilt', a, 0))),
   (('iand', ('inot', ('iand', ('ieq', ('umin', a, b), 0), c)), ('ilt', a, 0)),
    ('iand', ('inot', ('iand', ('ieq',             b , 0), c)), ('ilt', a, 0))),

   # These patterns can result when (a < b || a < c) => (a < min(b, c))
   # transformations occur before constant propagation and loop-unrolling.
   #
   # The flt versions are exact.  If isnan(a), the original pattern is
   # trivially false, and the replacements are false too.  If isnan(b):
   #
   #    a < fmax(NaN, a) => a < a => false vs a < NaN => false
   (('flt', a, ('fmax', b, a)), ('flt', a, b)),
   (('flt', ('fmin', a, b), a), ('flt', b, a)),
   (('fge(nnan)', a, ('fmin', b, a)), True),
   (('fge(nnan)', ('fmax', a, b), a), True),
   (('flt', a, ('fmin', b, a)), False),
   (('flt', ('fmax', a, b), a), False),
   (('fge(nnan)', a, ('fmax', b, a)), ('fge', a, b)),
   (('fge(nnan)', ('fmin', a, b), a), ('fge', b, a)),

   (('ilt', a, ('imax', b, a)), ('ilt', a, b)),
   (('ilt', ('imin', a, b), a), ('ilt', b, a)),
   (('ige', a, ('imin', b, a)), True),
   (('ige', ('imax', a, b), a), True),
   (('ult', a, ('umax', b, a)), ('ult', a, b)),
   (('ult', ('umin', a, b), a), ('ult', b, a)),
   (('uge', a, ('umin', b, a)), True),
   (('uge', ('umax', a, b), a), True),
   (('ilt', a, ('imin', b, a)), False),
   (('ilt', ('imax', a, b), a), False),
   (('ige', a, ('imax', b, a)), ('ige', a, b)),
   (('ige', ('imin', a, b), a), ('ige', b, a)),
   (('ult', a, ('umin', b, a)), False),
   (('ult', ('umax', a, b), a), False),
   (('uge', a, ('umax', b, a)), ('uge', a, b)),
   (('uge', ('umin', a, b), a), ('uge', b, a)),
   (('ult', a, ('iand', b, a)), False),
   (('ult', ('ior', a, b), a), False),
   (('uge', a, ('iand', b, a)), True),
   (('uge', ('ior', a, b), a), True),

   (('ilt', '#a', ('imax', '#b', c)), ('ior', ('ilt', a, b), ('ilt', a, c))),
   (('ilt', ('imin', '#a', b), '#c'), ('ior', ('ilt', a, c), ('ilt', b, c))),
   (('ige', '#a', ('imin', '#b', c)), ('ior', ('ige', a, b), ('ige', a, c))),
   (('ige', ('imax', '#a', b), '#c'), ('ior', ('ige', a, c), ('ige', b, c))),
   (('ult', '#a', ('umax', '#b', c)), ('ior', ('ult', a, b), ('ult', a, c))),
   (('ult', ('umin', '#a', b), '#c'), ('ior', ('ult', a, c), ('ult', b, c))),
   (('uge', '#a', ('umin', '#b', c)), ('ior', ('uge', a, b), ('uge', a, c))),
   (('uge', ('umax', '#a', b), '#c'), ('ior', ('uge', a, c), ('uge', b, c))),
   (('ilt', '#a', ('imin', '#b', c)), ('iand', ('ilt', a, b), ('ilt', a, c))),
   (('ilt', ('imax', '#a', b), '#c'), ('iand', ('ilt', a, c), ('ilt', b, c))),
   (('ige', '#a', ('imax', '#b', c)), ('iand', ('ige', a, b), ('ige', a, c))),
   (('ige', ('imin', '#a', b), '#c'), ('iand', ('ige', a, c), ('ige', b, c))),
   (('ult', '#a', ('umin', '#b', c)), ('iand', ('ult', a, b), ('ult', a, c))),
   (('ult', ('umax', '#a', b), '#c'), ('iand', ('ult', a, c), ('ult', b, c))),
   (('uge', '#a', ('umax', '#b', c)), ('iand', ('uge', a, b), ('uge', a, c))),
   (('uge', ('umin', '#a', b), '#c'), ('iand', ('uge', a, c), ('uge', b, c))),

   # These follow from the preceeding uge and ult patterns if various patterns
   # like uge(0, a) or ult(a, 1) are replaced with ieq or ine comparisons with
   # zero.
   (('ieq', ('umin', '#a', b), 0), ('ior', ('ieq', a, 0), ('ieq', b, 0))),
   (('ine', ('umax', '#a', b), 0), ('ior', ('ine', a, 0), ('ine', b, 0))),
   (('ieq', ('umax', '#a', b), 0), ('iand', ('ieq', a, 0), ('ieq', b, 0))),
   (('ine', ('umin', '#a', b), 0), ('iand', ('ine', a, 0), ('ine', b, 0))),

   # Thanks to sign extension, the ishr(a, b) is negative if and only if a is
   # negative.
   (('bcsel', ('ilt', a, 0), ('ineg', ('ishr', a, b)), ('ishr', a, b)),
    ('iabs', ('ishr', a, b))),
   (('iabs', ('ishr', ('iabs', a), b)), ('ushr', ('iabs', a), b)),
   (('iabs', ('ushr', ('iabs', a), b)), ('ushr', ('iabs', a), b)),

   (('fabs', ('slt', a, b)), ('slt', a, b)),
   (('fabs', ('sge', a, b)), ('sge', a, b)),
   (('fabs', ('seq', a, b)), ('seq', a, b)),
   (('fabs', ('sne', a, b)), ('sne', a, b)),
   (('slt', a, b), ('b2f', ('flt', a, b)), 'options->lower_scmp'),
   (('sge', a, b), ('b2f', ('fge', a, b)), 'options->lower_scmp'),
   (('seq', a, b), ('b2f', ('feq', a, b)), 'options->lower_scmp'),
   (('sne', a, b), ('b2f', ('fneu', a, b)), 'options->lower_scmp'),
   (('slt', a, -0.0), ('slt', a, 0.0)),
   (('slt', -0.0, a), ('slt', 0.0, a)),
   (('sge', a, -0.0), ('sge', a, 0.0)),
   (('sge', -0.0, a), ('sge', 0.0, a)),
   (('seq', a, -0.0), ('seq', a, 0.0)),
   (('sne', a, -0.0), ('sne', a, 0.0)),
   (('seq', ('seq', a, b), 1.0), ('seq', a, b)),
   (('seq', ('sne', a, b), 1.0), ('sne', a, b)),
   (('seq', ('slt', a, b), 1.0), ('slt', a, b)),
   (('seq', ('sge', a, b), 1.0), ('sge', a, b)),
   (('sne', ('seq', a, b), 0.0), ('seq', a, b)),
   (('sne', ('sne', a, b), 0.0), ('sne', a, b)),
   (('sne', ('slt', a, b), 0.0), ('slt', a, b)),
   (('sne', ('sge', a, b), 0.0), ('sge', a, b)),
   (('seq', ('seq', a, b), 0.0), ('sne', a, b)),
   (('seq', ('sne', a, b), 0.0), ('seq', a, b)),
   (('seq', ('slt', a, b), 0.0), ('sge', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer
   (('seq', ('sge', a, b), 0.0), ('slt', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer
   (('sne', ('seq', a, b), 1.0), ('sne', a, b)),
   (('sne', ('sne', a, b), 1.0), ('seq', a, b)),
   (('sne', ('slt', a, b), 1.0), ('sge', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer
   (('sne', ('sge', a, b), 1.0), ('slt', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer

   # Vulkan allows us to use any rounding mode, so choose rtz because it's simple.
   # Avoid some NaNs being converted to Inf if the lsb are cut off.
   (('f2bf', a), ('bcsel', ('fneu(preserve_nan_inf)', a, a), -1, ('unpack_32_2x16_split_y', a)), 'options->lower_bfloat16_conversions'),
   (('bf2f', a), ('pack_32_2x16', ('vec2', 0, a)), 'options->lower_bfloat16_conversions'),
])

# D3D Boolean emulation
for s in [8, 16, 32, 64]:
   cond = 'true'
   if s == 64:
       cond = '!(options->lower_int64_options & nir_lower_conv64)'

   optimizations.extend([
      (('bcsel@{}'.format(s), a, -1, 0), ('ineg', ('b2i', 'a@1')), cond),
      (('bcsel@{}'.format(s), a, 0, -1), ('ineg', ('b2i', ('inot', a))), cond),
      (('bcsel@{}'.format(s), a, 1, 0), ('b2i', 'a@1'), cond),
      (('bcsel@{}'.format(s), a, 0, 1), ('b2i', ('inot', a)), cond),
   ])

optimizations.extend([
   (('iand', ('ineg', ('b2i', 'a@1')), b),
    ('bcsel', a, b, 0)),
   (('ior', ('ineg', ('b2i','a@1')), ('ineg', ('b2i', 'b@1'))),
    ('ineg', ('b2i', ('ior', a, b)))),
   (('ige', ('ineg', ('b2i', 'a@1')), 0), ('inot', a)),
   (('ilt', ('ineg', ('b2i', 'a@1')), 0), a),
   (('bcsel', a, ('b2i', 'b@1'), ('b2i', 'c@1')), ('b2i', ('bcsel', a, b, c))),
   (('bcsel', a, ('b2i', 'b@1'), 0), ('b2i', ('bcsel', a, b, False))),
   (('bcsel', a, ('b2i', 'b@1'), 1), ('b2i', ('bcsel', a, b, True))),
   (('bcsel', a, 0, ('b2i', 'b@1')), ('b2i', ('bcsel', a, False, b))),
   (('bcsel', a, 1, ('b2i', 'b@1')), ('b2i', ('bcsel', a, True, b))),

   (('bcsel', a, ('ineg', ('b2i', 'b@1')), ('ineg', ('b2i', 'c@1'))), ('ineg', ('b2i', ('bcsel', a, b, c)))),
   (('bcsel', a, ('ineg', ('b2i', 'b@1')), 0), ('ineg', ('b2i', ('bcsel', a, b, False)))),
   (('bcsel', a, ('ineg', ('b2i', 'b@1')), -1), ('ineg', ('b2i', ('bcsel', a, b, True)))),
   (('bcsel', a, 0, ('ineg', ('b2i', 'b@1'))), ('ineg', ('b2i', ('bcsel', a, False, b)))),
   (('bcsel', a, -1, ('ineg', ('b2i', 'b@1'))), ('ineg', ('b2i', ('bcsel', a, True, b)))),

   (('inot', ('ineg', ('b2i', a))), ('ineg', ('b2i', ('inot', a)))),

   (('ishl', ('ineg', ('b2i', a)), '#b'),
    ('iand', ('ishl', -1, b), ('ineg', ('b2i', a)))),
   (('ushr', ('ineg', ('b2i', a)), '#b'),
    ('iand', ('ushr', -1, b), ('ineg', ('b2i', a)))),

   (('ine', ('b2i', 'a@1'), ('ineg', ('b2i', 'b@1'))), ('ior', a, b)),
   (('ieq', ('b2i', 'a@1'), ('ineg', ('b2i', 'b@1'))), ('inot', ('ior', a, b))),

   (('imul', ('b2i', 'a@1'), b), ('bcsel', a, b, 0)),
   (('imul', ('ineg', ('b2i', 'a@1')), b), ('ineg', ('bcsel', a, b, 0))),

   (('ishl', ('b2i', 'a@1'), '#b'), ('bcsel', a, ('ishl', 1, b), 0)),
   (('ishl', ('ineg', ('b2i', 'a@1')), '#b'), ('bcsel', a, ('ishl', -1, b), 0)),
])

for op in ('ior', 'iand', 'ixor'):
    optimizations.extend([
        ((op,          ('b2i', 'a@1') ,          ('b2i', 'b@1') ),          ('b2i', (op, a, b)) ),
        ((op, ('ineg', ('b2i', 'a@1')), ('ineg', ('b2i', 'b@1'))), ('ineg', ('b2i', (op, a, b)))),

        # This pattern can result when D3D-style Booleans are combined with some
        # algebraic optimizations of masking.
        (('iand', (op, ('b2i', 'a@1'), ('ineg', ('b2i', 'b@1'))), 1),       ('b2i', (op, a, b)) ),
    ])


for compare in [('fneu', a, 0.0), ('inot', ('feq', a, 0.0))]:
    mod = [a, ('fneg', a), ('fabs', a), ('fneg', ('fabs', a))]

    for i in range(len(mod)):
        for neg_b2f in [False, True]:

            search_mod = mod[i]
            replace_mod = mod[i ^ int(neg_b2f)];
            search_b2f = ('fneg', ('b2f', compare)) if neg_b2f else ('b2f', compare)

            replace_mod_mul = ('fcanonicalize', a) if replace_mod == a else replace_mod

            optimizations.extend([
                (('fmul', search_b2f, search_mod), replace_mod_mul),
                (('ffma', search_b2f, search_mod, b), ('fadd', replace_mod, b)),
            ])

optimizations.extend([
   (('feq', ('seq', a, b), 1.0), ('feq', a, b)),
   (('feq', ('sne', a, b), 1.0), ('fneu', a, b)),
   (('feq', ('slt', a, b), 1.0), ('flt', a, b)),
   (('feq', ('sge', a, b), 1.0), ('fge', a, b)),
   (('fneu', ('seq', a, b), 0.0), ('feq', a, b)),
   (('fneu', ('sne', a, b), 0.0), ('fneu', a, b)),
   (('fneu', ('slt', a, b), 0.0), ('flt', a, b)),
   (('fneu', ('sge', a, b), 0.0), ('fge', a, b)),
   (('feq', ('seq', a, b), 0.0), ('fneu', a, b)),
   (('feq', ('sne', a, b), 0.0), ('feq', a, b)),
   (('feq', ('slt', a, b), 0.0), ('fge', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer
   (('feq', ('sge', a, b), 0.0), ('flt', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer
   (('fneu', ('seq', a, b), 1.0), ('fneu', a, b)),
   (('fneu', ('sne', a, b), 1.0), ('feq', a, b)),
   (('fneu', ('slt', a, b), 1.0), ('fge', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer
   (('fneu', ('sge', a, b), 1.0), ('flt', a, b), 'true', TestStatus.XFAIL), # XFAIL is that a=NaN, b=0.0 produces the wrong answer

   (('fneu', ('fneg', a), a), ('fneu', a, 0.0)),
   (('feq', ('fneg', a), a), ('feq', a, 0.0)),
   # Emulating booleans
   (('imul', ('b2i', 'a@1'), ('b2i', 'b@1')), ('b2i', ('iand', a, b))),
   (('iand', ('b2i', 'a@1'), ('b2i', 'b@1')), ('b2i', ('iand', a, b))),
   (('ior', ('b2i', 'a@1'), ('b2i', 'b@1')), ('b2i', ('ior', a, b))),
   (('fmul', ('b2f', 'a@1'), ('b2f', 'b@1')), ('b2f', ('iand', a, b))),
   (('ffma', ('b2f', 'a@1'), ('b2f', 'b@1'), c), ('fadd', ('b2f', ('iand', a, b)), c)),
   (('fadd', 1.0, ('fneg', ('b2f', a))), ('b2f', ('inot', a))),
   (('fadd(nsz)', -1.0, ('b2f', a)), ('fneg', ('b2f', ('inot', a)))),
   (('fsat', ('fadd', ('b2f', 'a@1'), ('b2f', 'b@1'))), ('b2f', ('ior', a, b))),
   (('fsat', ('fadd', ('b2f', 'a@1'), ('fneg', ('b2f', 'b@1')))), ('b2f', ('iand', a, ('inot', b)))),
   (('fmax', ('fadd', ('b2f', 'a@1'), ('fneg', ('b2f', 'b@1'))), 0.0), ('b2f', ('iand', a, ('inot', b)))),

   # For this optimization, there are a few things to consider:
   # The replacement must flush denorms, fcanonicalize/fneg takes care of that.
   # For fmul, if b is not finite, b2f(False) * b would need to be NaN.
   # If b is negative, b2f(False) * b would be -0.0, not +0.0, hence the nsz.
   # For fmulz, if b is -0.0, b2f(True) * b would need to be +0.0, not b.
   # So even there nsz is needed.
   # When the multiplication is only used by fadd, it's not a clear win
   # because of potential fma fusion.
   (('fmul(nsz,is_not_only_used_by_fadd)',  ('b2f', 'a@1'), 'b(is_finite)'),           ('bcsel', a, ('fcanonicalize', b), 0.0)),
   (('fmul(nsz,is_not_only_used_by_fadd)',  ('fneg', ('b2f', 'a@1')), 'b(is_finite)'), ('bcsel', a, ('fneg', b), 0.0)),
   (('fmulz(nsz,is_not_only_used_by_fadd)', ('b2f', 'a@1'), b),           ('bcsel', a, ('fcanonicalize', b), 0.0)),
   (('fmulz(nsz,is_not_only_used_by_fadd)', ('fneg', ('b2f', 'a@1')), b), ('bcsel', a, ('fneg', b), 0.0)),

   # Comparison with the same args.  Note that these are only done for the
   # float versions when the source must be a number.  Generally, NaN cmp NaN
   # produces the opposite result of X cmp X.  flt is the outlier.  NaN < NaN
   # is false, and, for any number X, X < X is also false.
   (('ilt', a, a), False),
   (('ige', a, a), True),
   (('ieq', a, a), True),
   (('ine', a, a), False),
   (('ult', a, a), False),
   (('uge', a, a), True),
   (('flt', a, a), False),
   (('fge', 'a(is_a_number)', a), True),
   (('feq', 'a(is_a_number)', a), True),
   (('fneu', 'a(is_a_number)', a), False),
   # Logical and bit operations
   (('iand', a, a), a),
   (('iand', a, 0), 0),
   (('iand', a, -1), a),
   (('iand', a, ('inot', a)), 0),
   (('ior', a, a), a),
   (('ior', a, 0), a),
   (('ior', a, -1), -1),
   (('ior', a, ('inot', a)), -1),
   (('ixor', a, a), 0),
   (('ixor', a, 0), a),
   (('ixor', a, ('ixor', a, b)), b),
   (('ixor', a, -1), ('inot', a)),
   (('inot', ('inot', a)), a),
   (('ior', ('iand', a, b), b), b),
   (('ior', ('ior', a, b), b), ('ior', a, b)),
   (('iand', ('ior', a, b), b), b),
   (('iand', ('iand', a, b), b), ('iand', a, b)),

   # It is common for sequences of (x & 1) to occur in large trees.  Replacing
   # an expression like ((a & 1) & (b & 1)) with ((a & b) & 1) allows the "&
   # 1" to eventually bubble up to the top of the tree.
   (('iand', ('iand(is_used_once)', a, b), ('iand(is_used_once)', a, c)),
    ('iand', a, ('iand', b, c))),

   (('iand@64', a, '#b(is_lower_half_zero)'),
    ('pack_64_2x32_split', 0,
                           ('iand', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b))),
     '!options->lower_pack_64_2x32_split'),
   (('iand@64', a, '#b(is_upper_half_zero)'),
    ('pack_64_2x32_split', ('iand', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_x', b)),
                           0),
     '!options->lower_pack_64_2x32_split'),
   (('iand@64', a, '#b(is_lower_half_negative_one)'),
    ('pack_64_2x32_split', ('unpack_64_2x32_split_x', a),
                           ('iand', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b))),
     '!options->lower_pack_64_2x32_split'),
   (('iand@64', a, '#b(is_upper_half_negative_one)'),
    ('pack_64_2x32_split', ('iand', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_x', b)),
                           ('unpack_64_2x32_split_y', a)),
     '!options->lower_pack_64_2x32_split'),

   (('ior@64', a, '#b(is_lower_half_zero)'),
    ('pack_64_2x32_split', ('unpack_64_2x32_split_x', a),
                           ('ior', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b))),
     '!options->lower_pack_64_2x32_split'),
   (('ior@64', a, '#b(is_upper_half_zero)'),
    ('pack_64_2x32_split', ('ior', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_x', b)),
                           ('unpack_64_2x32_split_y', a)),
     '!options->lower_pack_64_2x32_split'),
   (('ior@64', a, '#b(is_lower_half_negative_one)'),
    ('pack_64_2x32_split', -1,
                           ('ior', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b))),
     '!options->lower_pack_64_2x32_split'),
   (('ior@64', a, '#b(is_upper_half_negative_one)'),
    ('pack_64_2x32_split', ('ior', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_x', b)),
                           -1),
     '!options->lower_pack_64_2x32_split'),

   (('ixor@64', a, '#b(is_lower_half_zero)'),
    ('pack_64_2x32_split', ('unpack_64_2x32_split_x', a),
                           ('ixor', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b))),
     '!options->lower_pack_64_2x32_split'),
   (('ixor@64', a, '#b(is_upper_half_zero)'),
    ('pack_64_2x32_split', ('ixor', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_x', b)),
                           ('unpack_64_2x32_split_y', a)),
     '!options->lower_pack_64_2x32_split'),

   # DeMorgan's Laws
   (('iand', ('inot', a), ('inot', b)), ('inot', ('ior',  a, b))),
   (('ior',  ('inot', a), ('inot', b)), ('inot', ('iand', a, b))),
   # Shift optimizations
   (('ishl', 0, a), 0),
   (('ishl', a, 0), a),
   (('ishr', 0, a), 0),
   (('ishr', -1, a), -1),
   (('ishr', a, 0), a),
   (('ushr', 0, a), 0),
   (('ushr', a, 0), a),
   (('bcsel', ('ieq', b, 0), a, ('ushr', a, b)), ('ushr', a, b)),
   (('bcsel', ('ieq', b, 0), a, ('ishr', a, b)), ('ishr', a, b)),
   (('bcsel', ('ieq', b, 0), a, ('ishl', a, b)), ('ishl', a, b)),
   (('bcsel', ('ine', b, 0), ('ushr', a, b), a), ('ushr', a, b)),
   (('bcsel', ('ine', b, 0), ('ishr', a, b), a), ('ishr', a, b)),
   (('bcsel', ('ine', b, 0), ('ishl', a, b), a), ('ishl', a, b)),
   (('ior', ('ishl@16', a, b), ('ushr@16', a, ('iadd', 16, ('ineg', b)))), ('urol', a, b), 'options->has_rotate16'),
   (('ior', ('ishl@16', a, b), ('ushr@16', a, ('isub', 16, b))), ('urol', a, b), 'options->has_rotate16'),
   (('ior', ('ishl@32', a, b), ('ushr@32', a, ('iadd', 32, ('ineg', b)))), ('urol', a, b), 'options->has_rotate32'),
   (('ior', ('ishl@32', a, b), ('ushr@32', a, ('isub', 32, b))), ('urol', a, b), 'options->has_rotate32'),
   (('ior', ('ushr@16', a, b), ('ishl@16', a, ('iadd', 16, ('ineg', b)))), ('uror', a, b), 'options->has_rotate16'),
   (('ior', ('ushr@16', a, b), ('ishl@16', a, ('isub', 16, b))), ('uror', a, b), 'options->has_rotate16'),
   (('ior', ('ushr@32', a, b), ('ishl@32', a, ('iadd', 32, ('ineg', b)))), ('uror', a, b), 'options->has_rotate32'),
   (('ior', ('ushr@32', a, b), ('ishl@32', a, ('isub', 32, b))), ('uror', a, b), 'options->has_rotate32'),
   (('urol@8',  a, b), ('ior', ('ishl', a, b), ('ushr', a, ('isub',  8, b))), '!options->has_rotate8'),
   (('urol@16', a, b), ('ior', ('ishl', a, b), ('ushr', a, ('isub', 16, b))), '!options->has_rotate16'),
   (('urol@32', a, b), ('ior', ('ishl', a, b), ('ushr', a, ('isub', 32, b))), '!options->has_rotate32'),
   (('urol@64', a, b), ('ior', ('ishl', a, b), ('ushr', a, ('isub', 64, b)))),
   (('uror@8',  a, b), ('ior', ('ushr', a, b), ('ishl', a, ('isub',  8, b))), '!options->has_rotate8'),
   (('uror@16', a, b), ('ior', ('ushr', a, b), ('ishl', a, ('isub', 16, b))), '!options->has_rotate16'),
   (('uror@32', a, b), ('ior', ('ushr', a, b), ('ishl', a, ('isub', 32, b))), '!options->has_rotate32'),
   (('uror@64', a, b), ('ior', ('ushr', a, b), ('ishl', a, ('isub', 64, b)))),

   (('bitfield_select', 0xff000000, ('ishl', 'b@32', 24), ('ushr', a, 8)), ('shfr', b, a, 8), 'options->has_shfr32'),
   (('bitfield_select', 0xffff0000, ('ishl', 'b@32', 16), ('extract_u16', a, 1)), ('shfr', b, a, 16), 'options->has_shfr32'),
   (('bitfield_select', 0xffffff00, ('ishl', 'b@32', 8), ('extract_u8', a, 3)), ('shfr', b, a, 24), 'options->has_shfr32'),
   (('ior', ('ishl', 'b@32', 24), ('ushr', a, 8)), ('shfr', b, a, 8), 'options->has_shfr32'),
   (('ior', ('ishl', 'b@32', 16), ('extract_u16', a, 1)), ('shfr', b, a, 16), 'options->has_shfr32'),
   (('ior', ('ishl', 'b@32', 8), ('extract_u8', a, 3)), ('shfr', b, a, 24), 'options->has_shfr32'),
   (('bcsel', ('ieq', c, 0), a, ('ior', ('ishl', 'b@32', ('iadd', 32, ('ineg', c))), ('ushr@32', a, c))), ('shfr', b, a, c), 'options->has_shfr32', TestStatus.XFAIL),
   (('bcsel', ('ine', c, 0), ('ior', ('ishl', 'b@32', ('iadd', 32, ('ineg', c))), ('ushr@32', a, c)), a), ('shfr', b, a, c), 'options->has_shfr32', TestStatus.XFAIL),
   (('ior', ('ishl', 'a@32', ('iadd', 32, ('ineg', b))), ('ushr@32', a, b)), ('shfr', a, a, b), 'options->has_shfr32 && !options->has_rotate32'),

   # bfi(X, a, b) = (b & ~X) | (a & X)
   # If X = ~0: (b & 0) | (a & 0xffffffff) = a
   # If X = 0:  (b & 0xffffffff) | (a & 0) = b
   (('bfi', 0xffffffff, a, b), a),
   (('bfi', 0x00000000, a, b), b),

   # The result of -int(some_bool) is 0 or 0xffffffff, so the result of the
   # bfi is either b or c.
   (('bfi', ('ineg', ('b2i', 'a@1')), b, c), ('bcsel', a, b, c)),

   # bfi(a, 0, b) = (0 << find_lsb(a)) & a) | (b & ~a)
   #              = b & ~a
   (('bfi', a, 0, b), ('iand', ('inot', a), b)),

   # bfi(a, b, b) = ((b << find_lsb(a)) & a) | (b & ~a)
   #              = (a & b) | (b & ~a)    If a is odd, find_lsb(a) == 0
   #              = b
   (('bfi', '#a(is_odd)', b, b), b),

   # bfi(a, a, b) = ((a << find_lsb(a)) & a) | (b & ~a)
   #              = (a & a) | (b & ~a)    If a is odd, find_lsb(a) == 0
   #              = a | (b & ~a)
   #              = a | b
   (('bfi', '#a(is_odd)', a, b), ('ior', a, b)),

   # bfi(a, b, 0) = ((b << find_lsb(a)) & a) | (0 & ~a)
   #              = ((b << find_lsb(a)) & a)
   #              = (b & a)               If a is odd, find_lsb(a) == 0
   (('bfi', '#a(is_odd)', b, 0), ('iand', a, b)),

   (('bfi', '#a(is_odd)', b, c), ('bitfield_select', a, b, c), 'options->has_bitfield_select'),

   # Because 'a' is a positive power of two, the result of the bfi is either 0
   # or 'a' depending on whether or not 'b' is odd.  Use 'b&1' for the zero
   # value to help platforms that can't have two constants in a bcsel.
   (('u2f32', ('bfi', '#a(is_pos_power_of_two)', b, 0)),
    ('bcsel', ('ieq', ('iand', b, 1), 0), ('iand', b, 1), ('u2f', a))),
   (('u2f', ('bfi', '#a(is_pos_power_of_two)', b, 0)),
    ('bcsel', ('ieq', ('iand', b, 1), 0), 0, ('u2f', a))),

   # Exponential/logarithmic identities
   (('fexp2(contract)', ('flog2', a)), ('fcanonicalize', a)), # 2^lg2(a) = a
   (('flog2(contract)', ('fexp2', a)), ('fcanonicalize', a)), # lg2(2^a) = a
   # 32-bit fpow should use fmulz to fix https://gitlab.freedesktop.org/mesa/mesa/-/issues/11464 (includes apitrace)
   (('fpow@32', a, b), ('fexp2', ('fmulz(preserve_nan_inf)', ('flog2', a), b)), 'options->lower_fpow && ' + has_fmulz), # a^b = 2^(lg2(a)*b)
   (('fpow', a, b), ('fexp2', ('fmul', ('flog2', a), b)), 'options->lower_fpow'), # a^b = 2^(lg2(a)*b)
   (('fexp2(contract)', ('fmul', ('flog2', a), b)), ('fpow', a, b), '!options->lower_fpow'), # 2^(lg2(a)*b) = a^b
   (('~fexp2', ('fadd', ('fmul', ('flog2', a), b), ('fmul', ('flog2', c), d))),
    ('~fmul', ('fpow', a, b), ('fpow', c, d)), '!options->lower_fpow'), # 2^(lg2(a) * b + lg2(c) + d) = a^b * c^d
   (('fexp2(contract)', ('fmul', ('flog2', a), 0.5)), ('fsqrt', a)),
   (('fexp2(contract)', ('fmul', ('flog2', a), 2.0)), ('fmul', a, a)),
   (('fexp2(contract)', ('fmul', ('flog2', a), 3.0)), ('fmul', ('fmul', a, a), a)),
   (('fexp2(contract)', ('fmul', ('flog2', a), 4.0)), ('fmul', ('fmul', a, a), ('fmul', a, a))),
   (('fexp2(contract)', ('fmul', ('flog2', a), 5.0)), ('fmul', ('fmul', ('fmul', a, a), ('fmul', a, a)), a)),
   (('fexp2(contract)', ('fmul', ('flog2', a), 6.0)), ('fmul', ('fmul', ('fmul', a, a), ('fmul', a, a)), ('fmul', a, a))),
   (('fexp2(contract)', ('fmul', ('flog2', a), 8.0)), ('fmul', ('fmul', ('fmul', a, a), ('fmul', a, a)), ('fmul', ('fmul', a, a), ('fmul', a, a)))),
   (('fpow(contract)', a, 1.0), ('fcanonicalize', a)),
   (('fpow(contract)', a, 2.0), ('fmul', a, a)),
   (('~fpow', a, 3.0), ('fmul', ('fmul', a, a), a)),
   (('~fpow', a, 4.0), ('fmul', ('fmul', a, a), ('fmul', a, a))),
   (('fpow(contract)', 2.0, a), ('fexp2', a)),
   (('~fpow', ('fpow', a, 2.2), 0.454545), ('fcanonicalize', a)),
   (('~fpow', ('fabs', ('fpow', a, 2.2)), 0.454545), ('fabs', a)),
   (('fsqrt(contract)', ('fexp2', a)), ('fexp2', ('fmul', 0.5, a))),
   (('frcp(contract)', ('fexp2', a)), ('fexp2', ('fneg', a))),
   (('frsq(contract)', ('fexp2', a)), ('fexp2', ('fmul', -0.5, a))),
   (('flog2(contract)', ('fsqrt', a)), ('fmul', 0.5, ('flog2', a))),
   (('flog2(contract)', ('frcp', a)), ('fneg', ('flog2', a))),
   (('flog2(contract)', ('frsq', a)), ('fmul', -0.5, ('flog2', a))),
   (('flog2(contract)', ('fpow', a, b)), ('fmul', b, ('flog2', a))),
   (('~fmul', ('fexp2(is_used_once)', a), ('fexp2(is_used_once)', b)), ('fexp2', ('fadd', a, b))),
   (('bcsel', ('flt', a, 0.0), 0.0, ('fsqrt(nnan,nsz)', a)), ('fsqrt', ('fmax', a, 0.0))),
   (('bcsel', ('fge', 0.0, a), 0.0, ('fsqrt(nnan)', a)), ('fsqrt', ('fmax', a, 0.0))),
   (('bcsel', ('flt', 0.0, a), ('fsqrt', a), 0.0), ('fsqrt', ('fmax', a, 0.0))),
   (('bcsel', ('fge', a, 0.0), ('fsqrt(nsz)', a), 0.0), ('fsqrt', ('fmax', a, 0.0))),
   (('fmul(contract)', ('fsqrt', a), ('fsqrt', a)), ('fabs',a)),
   (('fmulz(contract)', ('fsqrt', a), ('fsqrt', a)), ('fabs', a)),
   # Division and reciprocal
   (('fdiv(contract)', 1.0, a), ('frcp', a)),
   (('fdiv', a, b), ('fmul', a, ('frcp', b)), 'options->lower_fdiv'),
   (('frcp(contract)', ('frcp', a)), ('fcanonicalize', a)),
   (('frcp(contract)', ('fsqrt', a)), ('frsq', a)),
   (('fsqrt', a), ('frcp', ('frsq', a)), 'options->lower_fsqrt'),
   (('frcp(contract)', ('frsq', a)), ('fsqrt', a), '!options->lower_fsqrt'),
   # Trig
   (('fsin', a), lowered_sincos(0.5), 'options->lower_sincos'),
   (('fcos', a), lowered_sincos(0.75), 'options->lower_sincos'),
   # Boolean simplifications
   (('ieq', a, True), a),
   (('ine(is_not_used_by_if)', a, True), ('inot', a)),
   (('ine', a, False), a),
   (('ieq(is_not_used_by_if)', a, False), ('inot', 'a')),
   (('bcsel', a, True, False), a),
   (('bcsel', a, False, True), ('inot', a)),
   (('bcsel', True, b, c), b),
   (('bcsel', False, b, c), c),

   (('bcsel', a, b, b), b),
   (('fcsel', a, b, b), ('fcanonicalize', b)),

   # With D3D booleans, imax is AND and umax is OR
   (('imax', ('ineg', ('b2i', 'a@1')), ('ineg', ('b2i', 'b@1'))),
    ('ineg', ('b2i', ('iand', a, b)))),
   (('imin', ('ineg', ('b2i', 'a@1')), ('ineg', ('b2i', 'b@1'))),
    ('ineg', ('b2i', ('ior', a, b)))),
   (('umax', ('ineg', ('b2i', 'a@1')), ('ineg', ('b2i', 'b@1'))),
    ('ineg', ('b2i', ('ior', a, b)))),
   (('umin', ('ineg', ('b2i', 'a@1')), ('ineg', ('b2i', 'b@1'))),
    ('ineg', ('b2i', ('iand', a, b)))),
   (('umax', ('b2i', 'a@1'), ('b2i', 'b@1')), ('b2i', ('ior',  a, b))),
   (('umin', ('b2i', 'a@1'), ('b2i', 'b@1')), ('b2i', ('iand', a, b))),

   # Clean up LLVM booleans. b2i output is 0/1 so iand is a no-op.
   (('iand', ('b2i', a), 1), ('b2i', a)),

   (('ine', ('umin', ('ineg', ('b2i', 'a@1')), b), 0), ('iand', a, ('ine', b, 0))),
   (('ine', ('umax', ('ineg', ('b2i', 'a@1')), b), 0), ('ior' , a, ('ine', b, 0))),
   (('ine', ('umin', ('b2i', 'a@1'), b), 0), ('iand', a, ('ine', b, 0))),
   (('ine', ('umax', ('b2i', 'a@1'), b), 0), ('ior' , a, ('ine', b, 0))),
   (('ieq', ('umin', ('ineg', ('b2i', 'a@1')), b), 0), ('ior', ('inot', a), ('ieq', b, 0))),
   (('ieq', ('umax', ('ineg', ('b2i', 'a@1')), b), 0), ('iand' , ('inot', a), ('ieq', b, 0))),
   (('ieq', ('umin', ('b2i', 'a@1'), b), 0), ('ior', ('inot', a), ('ieq', b, 0))),
   (('ieq', ('umax', ('b2i', 'a@1'), b), 0), ('iand' , ('inot', a), ('ieq', b, 0))),

   # Conversions
   (('f2i', ('ftrunc', a)), ('f2i', a)),
   (('f2u', ('ftrunc', 'a(is_not_negative)')), ('f2u', a)),
   (('f2i', ('ffloor', 'a(is_not_negative)')), ('f2i', a)),
   (('f2u', ('ffloor', a)), ('f2u', a)),
   (('f2u(contract)', ('fadd', 'a(is_integral_not_negative)', 'b(is_a_number_gt_0_and_lt_1)')), ('f2u', a)),
   (('f2i(contract)', ('fadd', 'a(is_integral_not_negative)', 'b(is_a_number_gt_0_and_lt_1)')), ('f2i', a)),
   (('f2u32', ('u2f32', 'a@16')), ('u2u32', a)),
   (('f2i32', ('u2f32', 'a@16')), ('u2u32', a)),

   # Section 3.3.11 (Conversion Instructions) of the SPIR-V 1.6 spec says:
   #
   #    "Behavior is undefined if Result Type is not wide enough to hold the
   #    converted value."
   #
   # Unsigned integers cannot hold negative values, so squash them to
   # zero. This is what the conversion instruction on many GPUs would do
   # anyway.
   (('f2u', 'a(is_not_positive)'), 0),

   # Conversions from 16 bits to 32 bits and back can always be removed
   (('f2fmp', ('f2f32', 'a@16')), a, 'true', TestStatus.UNSUPPORTED),
   (('i2imp', ('i2i32', 'a@16')), a, 'true', TestStatus.UNSUPPORTED),
   (('i2imp', ('u2u32', 'a@16')), a, 'true', TestStatus.UNSUPPORTED),

   (('f2imp', ('f2f32', 'a@16')), ('f2i16', a), 'true', TestStatus.UNSUPPORTED),
   (('f2ump', ('f2f32', 'a@16')), ('f2u16', a), 'true', TestStatus.UNSUPPORTED),
   (('i2fmp', ('i2i32', 'a@16')), ('i2f16', a), 'true', TestStatus.UNSUPPORTED),
   (('u2fmp', ('u2u32', 'a@16')), ('u2f16', a), 'true', TestStatus.UNSUPPORTED),

   (('f2fmp', ('b2f32', 'a@1')), ('b2f16', a), 'true', TestStatus.UNSUPPORTED),
   (('i2imp', ('b2i32', 'a@1')), ('b2i16', a), 'true', TestStatus.UNSUPPORTED),
   (('i2imp', ('b2i32', 'a@1')), ('b2i16', a), 'true', TestStatus.UNSUPPORTED),

   (('f2imp', ('b2f32', 'a@1')), ('b2i16', a), 'true', TestStatus.UNSUPPORTED),
   (('f2ump', ('b2f32', 'a@1')), ('b2i16', a), 'true', TestStatus.UNSUPPORTED),
   (('i2fmp', ('b2i32', 'a@1')), ('b2f16', a), 'true', TestStatus.UNSUPPORTED),
   (('u2fmp', ('b2i32', 'a@1')), ('b2f16', a), 'true', TestStatus.UNSUPPORTED),

   # Conversions to 16 bits would be lossy so they should only be removed if
   # the instruction was generated by the precision lowering pass.
   (('f2f32', ('f2fmp', 'a@32')), a, 'true', TestStatus.UNSUPPORTED),
   (('i2i32', ('i2imp', 'a@32')), a, 'true', TestStatus.UNSUPPORTED),
   (('u2u32', ('i2imp', 'a@32')), a, 'true', TestStatus.UNSUPPORTED),

   # typeA@32 -> typeB@16 -> typeB@32 ==> typeA@32 -> typeB@32
   (('i2i32', ('f2imp', 'a@32')), ('f2i32', a), 'true', TestStatus.UNSUPPORTED),
   (('u2u32', ('f2ump', 'a@32')), ('f2u32', a), 'true', TestStatus.UNSUPPORTED),
   (('f2f32', ('i2fmp', 'a@32')), ('i2f32', a), 'true', TestStatus.UNSUPPORTED),
   (('f2f32', ('u2fmp', 'a@32')), ('u2f32', a), 'true', TestStatus.UNSUPPORTED),

   # typeA@32 -> typeA@16 -> typeB@32 ==> typeA@32 -> typeB@32
   (('f2i32', ('f2fmp', 'a@32')), ('f2i32', a), 'true', TestStatus.UNSUPPORTED),
   (('f2u32', ('f2fmp', 'a@32')), ('f2u32', a), 'true', TestStatus.UNSUPPORTED),
   (('i2f32', ('i2imp', 'a@32')), ('i2f32', a), 'true', TestStatus.UNSUPPORTED),

   (('ffloor', 'a(is_integral)'), a),
   (('fceil', 'a(is_integral)'), a),
   (('ftrunc', 'a(is_integral)'), a),
   (('fround_even', 'a(is_integral)'), a),

   (('ffract(nnan,contract)', ('fadd', a, 'b(is_integral)')), ('ffract', a)),
   (('ffloor(nsz,contract)', ('fadd', 'a(is_a_number_gt_0_and_lt_1)', 'b(is_integral)')), b),
   (('ftrunc(nsz,contract)', ('fadd', 'a(is_a_number_gt_0_and_lt_1)', 'b(is_integral_not_negative)')), b),

   # fract(x) = x - floor(x), so fract(NaN/Inf) = NaN
   (('ffract(nnan)', 'a(is_integral)'), 0.0),
   (('ffract', 'a(is_a_number_gt_0_and_lt_1)'), a),
   (('ffract', ('ffract', a)), ('ffract', a)),
   (('fabs(nsz)', 'a(is_not_negative)'), ('fcanonicalize', a)),
   (('fabs', 'a(is_not_negative_or_negative_zero)'), ('fcanonicalize', a)),
   (('fabs(nsz)', 'a(is_not_positive)'), ('fneg', a)),

   (('fneu', 'a(is_not_zero)', 0.0), True),
   (('feq', 'a(is_not_zero)', 0.0), False),

   # In this chart, + means value > 0 and - means value < 0.
   #
   # + >= + -> unknown  0 >= + -> false    - >= + -> false
   # + >= 0 -> true     0 >= 0 -> true     - >= 0 -> false
   # + >= - -> true     0 >= - -> true     - >= - -> unknown
   #
   # Using grouping conceptually similar to a Karnaugh map...
   #
   # (+ >= 0, + >= -, 0 >= 0, 0 >= -) == (is_not_negative >= is_not_positive) -> true
   # (0 >= +, - >= +) == (is_not_positive >= gt_zero) -> false
   # (- >= +, - >= 0) == (lt_zero >= is_not_negative) -> false
   #
   # The flt cases just invert the expected result.
   #
   # The results expecting true, must be marked imprecise.  The results
   # expecting false are fine because NaN compared >= or < anything is false.

   (('fge', 'a(is_a_number_not_negative)', 'b(is_a_number_not_positive)'), True),
   (('fge', 'a(is_not_positive)',          'b(is_gt_zero)'),               False),
   (('fge', 'a(is_lt_zero)',               'b(is_not_negative)'),          False),

   (('flt', 'a(is_not_negative)',          'b(is_not_positive)'),          False),
   (('flt', 'a(is_a_number_not_positive)', 'b(is_a_number_gt_zero)'),      True),
   (('flt', 'a(is_a_number_lt_zero)',      'b(is_a_number_not_negative)'), True),

   (('ult', a, 0), False),
])

# Doing a 32-bit fadd/fmul on 16-bit operands is equivalent to a 16-bit
# fadd/fmul, contract is required for the double rounding.
for op in ['fadd', 'fmul']:
    optimizations += [
        (('f2fmp', (f'{op}(is_used_once,contract)', ('f2f32', 'a@16'), '#b(is_representable_as_f16)')), (op, a, ('f2fmp', b)), 'true', TestStatus.UNSUPPORTED),
        (('f2fmp', (f'{op}(is_used_once,contract)', ('f2f32', 'a@16'), ('f2f32', 'b@16'))), (op, a, b), 'true', TestStatus.UNSUPPORTED),
    ]

for bits in [16, 32, 64]:
    cond = '!(options->lower_doubles_options & nir_lower_fp64_full_software)' if bits == 64 else 'true'
    bcsel = 'bcsel@{}'.format(bits)
    bcsel_nsz = bcsel + '(is_only_used_as_float_nsz)'
    optimizations += [
        ((bcsel, a, 1.0, 0.0), ('b2f(preserve_sz)', a), cond),
        ((bcsel, a, 0.0, 1.0), ('b2f(preserve_sz)', ('inot', a)), cond),
        ((bcsel, a, -1.0, -0.0), ('fneg(preserve_sz)', ('b2f(preserve_sz)', a)), cond),
        ((bcsel, a, -0.0, -1.0), ('fneg(preserve_sz)', ('b2f(preserve_sz)', ('inot', a))), cond),
        ((bcsel_nsz, a, 1.0, -0.0), ('b2f', a), cond),
        ((bcsel_nsz, a, -0.0, 1.0), ('b2f', ('inot', a)), cond),
        ((bcsel_nsz, a, -1.0, 0.0), ('fneg', ('b2f', a)), cond),
        ((bcsel_nsz, a, 0.0, -1.0), ('fneg', ('b2f', ('inot', a))), cond),
    ]

# Packing and then unpacking does nothing
for pack, bits, compbits in [('pack_64_2x32', 64, 32), ('pack_32_2x16', 32, 16)]:
    unpack = 'un' + pack
    optimizations += [
        ((unpack + '_split_x', (pack + '_split', a, b)), a),
        ((unpack + '_split_y', (pack + '_split', a, b)), b),
        ((unpack + '_split_x', (pack, a)), 'a.x'),
        ((unpack + '_split_y', (pack, a)), 'a.y'),
        ((unpack + '_split_x', ('u2u' + str(bits), 'a@' + str(compbits))), a),
        ((unpack + '_split_x', ('i2i' + str(bits), 'a@' + str(compbits))), a),
        ((unpack + '_split_y', ('i2i' + str(bits) + '(is_used_once)', 'a@' + str(compbits))), ('ishr', a, compbits - 1)),
        ((unpack, (pack + '_split', a, b)), ('vec2', a, b)),
        ((unpack, (pack, a)), a),
        ((pack + '_split', (unpack + '_split_x', a), (unpack + '_split_y', a)), a),
        ((pack + '_split', (unpack, a), (unpack + '.y', a)), a),
        ((pack, ('vec2', (unpack + '_split_x', a), (unpack + '_split_y', a))), a),
        ((pack, (unpack, a)), a),
    ]

optimizations.extend([
   (('unpack_64_2x32_split_y', ('u2u64', 'a@1')), 0),
   (('unpack_64_2x32_split_y', ('u2u64', 'a@8')), 0),
   (('unpack_64_2x32_split_y', ('u2u64', 'a@16')), 0),
   (('unpack_64_2x32_split_y', ('u2u64', 'a@32')), 0), # Don't do that for u64 -> u64
   (('unpack_double_2x32_dxil', ('pack_double_2x32_dxil', a)), a),
   (('pack_double_2x32_dxil', ('unpack_double_2x32_dxil', a)), a),

   (('unpack_64_4x16', ('pack_64_4x16', a)), a),
   (('pack_64_4x16', ('unpack_64_4x16', a)), a),
   (('unpack_32_4x8', ('pack_32_4x8', a)), a),
   (('pack_32_4x8', ('unpack_32_4x8', a)), a),

   (('unpack_64_4x16', ('pack_64_2x32', ('vec2', ('pack_32_2x16_split', a, b), ('pack_32_2x16_split', c, d)))), ('vec4', a, b, c, d)),
   (('unpack_64_4x16', ('pack_64_2x32_split', ('pack_32_2x16_split', a, b), ('pack_32_2x16_split', c, d))), ('vec4', a, b, c, d)),

   (('pack_64_2x32_split', ('pack_32_2x16_split', a, b), ('pack_32_2x16_split', c, d)),
    ('pack_64_4x16', ('vec4', a, b, c, d)), '!options->lower_pack_64_4x16'),
   (('pack_64_2x32', ('vec2', ('pack_32_2x16_split', a, b), ('pack_32_2x16_split', c, d))),
    ('pack_64_4x16', ('vec4', a, b, c, d)), '!options->lower_pack_64_4x16'),
   (('pack_64_2x32', ('vec2', ('pack_32_2x16', ('vec2', a, b)), ('pack_32_2x16', ('vec2', c, d)))),
    ('pack_64_4x16', ('vec4', a, b, c, d)), '!options->lower_pack_64_4x16'),

   # Comparing two halves of an unpack separately.  While this optimization
   # should be correct for non-constant values, it's less obvious that it's
   # useful in that case.  For constant values, the pack will fold and we're
   # guaranteed to reduce the whole tree to one instruction.
   (('iand', ('ieq', ('unpack_32_2x16_split_x', a), '#b'),
             ('ieq', ('unpack_32_2x16_split_y', a), '#c')),
    ('ieq', a, ('pack_32_2x16_split', b, c))),

   # Byte extraction
   (('ushr', 'a@16',  8), ('extract_u8', a, 1), '!options->lower_extract_byte'),
   (('ushr', 'a@32', 24), ('extract_u8', a, 3), '!options->lower_extract_byte'),
   (('ushr', 'a@64', 56), ('extract_u8', a, 7), '!options->lower_extract_byte'),
   (('ishr', 'a@16',  8), ('extract_i8', a, 1), '!options->lower_extract_byte'),
   (('ishr', 'a@32', 24), ('extract_i8', a, 3), '!options->lower_extract_byte'),
   (('ishr', 'a@64', 56), ('extract_i8', a, 7), '!options->lower_extract_byte'),
   (('iand', 0xff, a), ('extract_u8', a, 0), '!options->lower_extract_byte'),
   (('ishr', ('iand', 'a@32', 0x0000ff00),  8), ('extract_u8', a, 1), '!options->lower_extract_byte'),
   (('ishr', ('iand', 'a@64', 0x0000ff00),  8), ('extract_u8', a, 1), '!options->lower_extract_byte'),
   (('ishr', ('iand',  a,     0x00ff0000), 16), ('extract_u8', a, 2), '!options->lower_extract_byte'),

   # Common pattern in many Vulkan CTS tests that read 8-bit integers from a
   # storage buffer.
   (('u2u8', ('extract_u16', a, 1)), ('u2u8', ('extract_u8', a, 2)), '!options->lower_extract_byte'),
   (('u2u8', ('ushr', a, 8)), ('u2u8', ('extract_u8', a, 1)), '!options->lower_extract_byte'),

   # Common pattern after lowering 8-bit integers to 16-bit.
   (('i2i16', ('u2u8', ('extract_u8', a, b))), ('i2i16', ('extract_i8', a, b))),
   (('u2u16', ('u2u8', ('extract_u8', a, b))), ('u2u16', ('extract_u8', a, b))),

   (('ubfe', a,  0, 8), ('extract_u8', a, 0), '!options->lower_extract_byte'),
   (('ubfe', a,  8, 8), ('extract_u8', a, 1), '!options->lower_extract_byte'),
   (('ubfe', a, 16, 8), ('extract_u8', a, 2), '!options->lower_extract_byte'),
   (('ubfe', a, 24, 8), ('extract_u8', a, 3), '!options->lower_extract_byte'),
   (('ibfe', a,  0, 8), ('extract_i8', a, 0), '!options->lower_extract_byte'),
   (('ibfe', a,  8, 8), ('extract_i8', a, 1), '!options->lower_extract_byte'),
   (('ibfe', a, 16, 8), ('extract_i8', a, 2), '!options->lower_extract_byte'),
   (('ibfe', a, 24, 8), ('extract_i8', a, 3), '!options->lower_extract_byte'),

   (('extract_u8', ('extract_i8', a, b), 0), ('extract_u8', a, b)),
   (('extract_u8', ('extract_u8', a, b), 0), ('extract_u8', a, b)),

   # The extract_X8(a & 0xff) patterns aren't included because the iand will
   # already be converted to extract_u8.
   (('extract_i8', ('iand', a, 0x0000ff00), 1), ('extract_i8', a, 1)),
   (('extract_i8', ('iand', a, 0x00ff0000), 2), ('extract_i8', a, 2)),
   (('extract_i8', ('iand', a, 0xff000000), 3), ('extract_i8', a, 3)),

   (('extract_u8', ('iand', a, 0x0000ff00), 1), ('extract_u8', a, 1)),
   (('extract_u8', ('iand', a, 0x00ff0000), 2), ('extract_u8', a, 2)),
   (('extract_u8', ('iand', a, 0xff000000), 3), ('extract_u8', a, 3)),

   (('ior', ('bcsel', ('ieq', ('iand', a, 0x00000080), 0), 0, ~0xff), ('extract_u8', a, 0)), ('extract_i8', a, 0)),
   (('ior', ('bcsel', ('ieq', ('iand', a, 0x00008000), 0), 0, ~0xff), ('extract_u8', a, 1)), ('extract_i8', a, 1)),
   (('ior', ('bcsel', ('ieq', ('iand', a, 0x00800000), 0), 0, ~0xff), ('extract_u8', a, 2)), ('extract_i8', a, 2)),
   (('ior', ('bcsel', ('ige',          'a@32',         0), 0, ~0xff), ('extract_u8', a, 3)), ('extract_i8', a, 3)),
   (('ior', ('bcsel', ('ine', ('iand', a, 0x00000080), 0), ~0xff, 0), ('extract_u8', a, 0)), ('extract_i8', a, 0)),
   (('ior', ('bcsel', ('ine', ('iand', a, 0x00008000), 0), ~0xff, 0), ('extract_u8', a, 1)), ('extract_i8', a, 1)),
   (('ior', ('bcsel', ('ine', ('iand', a, 0x00800000), 0), ~0xff, 0), ('extract_u8', a, 2)), ('extract_i8', a, 2)),
   (('ior', ('bcsel', ('ilt',          'a@32',         0), ~0xff, 0), ('extract_u8', a, 3)), ('extract_i8', a, 3)),

   (('extract_i8', ('extract_i16', a, 1), 0), ('extract_i8', a, 2)),
   (('extract_i8', ('extract_i16', a, 1), 1), ('extract_i8', a, 3)),
   (('extract_i8', ('extract_u16', a, 1), 0), ('extract_i8', a, 2)),
   (('extract_i8', ('extract_u16', a, 1), 1), ('extract_i8', a, 3)),
   (('extract_u8', ('extract_i16', a, 1), 0), ('extract_u8', a, 2)),
   (('extract_u8', ('extract_i16', a, 1), 1), ('extract_u8', a, 3)),
   (('extract_u8', ('extract_u16', a, 1), 0), ('extract_u8', a, 2)),
   (('extract_u8', ('extract_u16', a, 1), 1), ('extract_u8', a, 3)),

   (('iand', ('extract_u8',  a, 0), '#b'), ('iand', a, ('iand', b, 0x00ff))),
   (('iand', ('extract_u16', a, 0), '#b'), ('iand', a, ('iand', b, 0xffff))),

   (('ieq', ('iand', ('extract_u8',  a, '#b'), '#c'), 0), ('ieq', ('iand', a, ('ishl', ('iand', c, 0x00ff), ('imul', ('i2i32', b),  8))), 0)),
   (('ine', ('iand', ('extract_u8',  a, '#b'), '#c'), 0), ('ine', ('iand', a, ('ishl', ('iand', c, 0x00ff), ('imul', ('i2i32', b),  8))), 0)),
   (('ieq', ('iand', ('extract_u16(is_used_once)', a, '#b'), '#c'), 0), ('ieq', ('iand', a, ('ishl', ('iand', c, 0xffff), ('imul', ('i2i32', b), 16))), 0)),
   (('ine', ('iand', ('extract_u16(is_used_once)', a, '#b'), '#c'), 0), ('ine', ('iand', a, ('ishl', ('iand', c, 0xffff), ('imul', ('i2i32', b), 16))), 0)),

    # Word extraction
   (('ushr', ('ishl', 'a@32', 16), 16), ('extract_u16', a, 0), '!options->lower_extract_word'),
   (('ushr', 'a@32', 16), ('extract_u16', a, 1), '!options->lower_extract_word'),
   (('ishr', ('ishl', 'a@32', 16), 16), ('extract_i16', a, 0), '!options->lower_extract_word'),
   (('ishr', 'a@32', 16), ('extract_i16', a, 1), '!options->lower_extract_word'),
   (('iand', 0xffff, a), ('extract_u16', a, 0), '!options->lower_extract_word'),

   (('ubfe', a,  0, 16), ('extract_u16', a, 0), '!options->lower_extract_word'),
   (('ubfe', a, 16, 16), ('extract_u16', a, 1), '!options->lower_extract_word'),
   (('ibfe', a,  0, 16), ('extract_i16', a, 0), '!options->lower_extract_word'),
   (('ibfe', a, 16, 16), ('extract_i16', a, 1), '!options->lower_extract_word'),

   (('u2u32', ('unpack_32_2x16_split_x', a)), ('extract_u16', a, 0), '!options->lower_extract_word'),
   (('u2u32', ('unpack_32_2x16_split_y', a)), ('extract_u16', a, 1), '!options->lower_extract_word'),

   # Packing a u8vec4 to write to an SSBO.
   (('ior', ('ishl', ('u2u32', 'a@8'), 24), ('ior', ('ishl', ('u2u32', 'b@8'), 16), ('ior', ('ishl', ('u2u32', 'c@8'), 8), ('u2u32', 'd@8')))),
    ('pack_32_4x8', ('vec4', d, c, b, a)), 'options->has_pack_32_4x8'),

   (('ior', ('ishl', a, 16), ('u2u32', 'b@16')), ('pack_32_2x16_split', b, ('u2u16', a)), '!options->lower_pack_32_2x16_split && !options->lower_pack_split'),

   # Mixed 16-bit/8-bit loads vectorized to 8-bit vector load and then lowered to 32-bit
   (('ior', ('u2u16', ('unpack_32_4x8', a)), ('ishl', ('u2u16', ('unpack_32_4x8.y', a)), 8)),
    ('unpack_32_2x16_split_x', a), '!options->lower_unpack_32_2x16_split'),
   (('ior', ('u2u16', ('unpack_32_4x8.z', a)), ('ishl', ('u2u16', ('unpack_32_4x8.w', a)), 8)),
    ('unpack_32_2x16_split_y', a), '!options->lower_unpack_32_2x16_split'),

   # Prefer 16bit unpack/extract because it's easier to vectorize
   (('i2i16', ('unpack_32_4x8(xz_components_unused).y', a)), ('extract_i8', ('unpack_32_2x16.x', a), 1), '!options->lower_extract_byte'),
   (('i2i16', ('unpack_32_4x8(xz_components_unused).w', a)), ('extract_i8', ('unpack_32_2x16.y', a), 1), '!options->lower_extract_byte'),

   (('extract_u16', ('extract_i16', a, b), 0), ('extract_u16', a, b)),
   (('extract_u16', ('extract_u16', a, b), 0), ('extract_u16', a, b)),

   # Downcast followed by upcast
   (('u2u32', ('u2u8', 'a@32')), ('extract_u8', a, 0), '!options->lower_extract_byte'),
   (('u2u32', ('i2i8', 'a@32')), ('extract_u8', a, 0), '!options->lower_extract_byte'),
   (('i2i32', ('i2i8', 'a@32')), ('extract_i8', a, 0), '!options->lower_extract_byte'),
   (('i2i32', ('u2u8', 'a@32')), ('extract_i8', a, 0), '!options->lower_extract_byte'),
   (('u2u32', ('u2u16', 'a@32')), ('extract_u16', a, 0), '!options->lower_extract_word'),
   (('u2u32', ('i2i16', 'a@32')), ('extract_u16', a, 0), '!options->lower_extract_word'),
   (('i2i32', ('i2i16', 'a@32')), ('extract_i16', a, 0), '!options->lower_extract_word'),
   (('i2i32', ('u2u16', 'a@32')), ('extract_i16', a, 0), '!options->lower_extract_word'),

   # The extract_X16(a & 0xff) patterns aren't included because the iand will
   # already be converted to extract_u8.
   (('extract_i16', ('iand', a, 0x00ff0000), 1), ('extract_u8', a, 2), '!options->lower_extract_byte'), # extract_u8 is correct
   (('extract_u16', ('iand', a, 0x00ff0000), 1), ('extract_u8', a, 2), '!options->lower_extract_byte'),

   # Lower pack/unpack
   (('pack_64_2x32_split', a, b), ('ior', ('u2u64', a), ('ishl', ('u2u64', b), 32)), 'options->lower_pack_64_2x32_split'),
   (('pack_32_2x16_split', a, b), ('ior', ('u2u32', a), ('ishl', ('u2u32', b), 16)), 'options->lower_pack_32_2x16_split || options->lower_pack_split'),
   (('pack_half_2x16_split', a, b), ('pack_half_2x16_rtz_split', a, b), 'options->has_pack_half_2x16_rtz'),
   (('unpack_64_2x32_split_x', a), ('u2u32', a), 'options->lower_unpack_64_2x32_split'),
   (('unpack_64_2x32_split_y', a), ('u2u32', ('ushr', a, 32)), 'options->lower_unpack_64_2x32_split'),
   (('unpack_32_2x16_split_x', a), ('u2u16', a), 'options->lower_unpack_32_2x16_split || options->lower_pack_split'),
   (('unpack_32_2x16_split_y', a), ('u2u16', ('ushr', a, 16)), 'options->lower_unpack_32_2x16_split || options->lower_pack_split'),

   (('unpack_64_2x32_split_x', ('ushr', a, 32)), ('unpack_64_2x32_split_y', a), '!options->lower_unpack_64_2x32_split'),
   (('u2u32', ('ushr', 'a@64', 32)), ('unpack_64_2x32_split_y', a), '!options->lower_unpack_64_2x32_split'),

   # Useless masking before unpacking
   (('unpack_32_2x16_split_x', ('iand', a, 0xffff)), ('unpack_32_2x16_split_x', a)),
   (('unpack_64_2x32_split_x', ('iand', a, 0xffffffff)), ('unpack_64_2x32_split_x', a)),
   (('unpack_32_2x16_split_y', ('iand', a, 0xffff0000)), ('unpack_32_2x16_split_y', a)),
   (('unpack_64_2x32_split_y', ('iand', a, 0xffffffff00000000)), ('unpack_64_2x32_split_y', a)),

   (('unpack_32_2x16_split_x', ('extract_u16', a, 0)), ('unpack_32_2x16_split_x', a)),
   (('unpack_32_2x16_split_x', ('extract_u16', a, 1)), ('unpack_32_2x16_split_y', a)),

   # Optimize half packing
   (('ishl', ('pack_half_2x16', ('vec2', a, 0)), 16), ('pack_half_2x16', ('vec2', 0, a))),
   (('ushr', ('pack_half_2x16', ('vec2', 0, a)), 16), ('pack_half_2x16', ('vec2', a, 0))),

   (('iadd', ('pack_half_2x16', ('vec2', a, 0)), ('pack_half_2x16', ('vec2', 0, b))),
    ('pack_half_2x16', ('vec2', a, b))),
   (('ior', ('pack_half_2x16', ('vec2', a, 0)), ('pack_half_2x16', ('vec2', 0, b))),
    ('pack_half_2x16', ('vec2', a, b))),

   (('ishl', ('pack_half_2x16_split', a, 0), 16), ('pack_half_2x16_split', 0, a)),
   (('ushr', ('pack_half_2x16_split', 0, a), 16), ('pack_half_2x16_split', a, 0)),
   (('extract_u16', ('pack_half_2x16_split', 0, a), 1), ('pack_half_2x16_split', a, 0)),

   (('ishl', ('pack_half_2x16_rtz_split', a, 0), 16), ('pack_half_2x16_rtz_split', 0, a)),
   (('ushr', ('pack_half_2x16_rtz_split', 0, a), 16), ('pack_half_2x16_rtz_split', a, 0)),
   (('extract_u16', ('pack_half_2x16_rtz_split', 0, a), 1), ('pack_half_2x16_rtz_split', a, 0)),

   (('iadd', ('pack_half_2x16_split', a, 0), ('pack_half_2x16_split', 0, b)), ('pack_half_2x16_split', a, b)),
   (('ior',  ('pack_half_2x16_split', a, 0), ('pack_half_2x16_split', 0, b)), ('pack_half_2x16_split', a, b)),

   (('iadd', ('pack_half_2x16_rtz_split', a, 0), ('pack_half_2x16_rtz_split', 0, b)), ('pack_half_2x16_rtz_split', a, b)),
   (('ior',  ('pack_half_2x16_rtz_split', a, 0), ('pack_half_2x16_rtz_split', 0, b)), ('pack_half_2x16_rtz_split', a, b)),

   (('pack_uint_2x16', ('vec2', ('pack_half_2x16_rtz_split', a, 0), ('pack_half_2x16_rtz_split', b, 0))), ('pack_half_2x16_rtz_split', a, b)),

   (('bfi', 0xffff0000, ('pack_half_2x16_split', a, b), ('pack_half_2x16_split', c, d)),
    ('pack_half_2x16_split', c, a)),

   # The important part here is that ~0xf & 0xfffffffc = ~0xf.
   (('iand', ('bfi', 0x0000000f, '#a', b), 0xfffffffc),
    ('bfi', 0x0000000f, ('iand', a, 0xfffffffc), b)),
   (('iand', ('bfi', 0x00000007, '#a', b), 0xfffffffc),
    ('bfi', 0x00000007, ('iand', a, 0xfffffffc), b)),

   # 0x0f << 3 == 0x78, so that's already the maximum possible value.
   (('umin', ('ishl', ('iand', a, 0xf), 3), 0x78), ('ishl', ('iand', a, 0xf), 3)),

   (('extract_i8', ('pack_32_4x8_split', a, b, c, d), 0), ('i2i', a)),
   (('extract_i8', ('pack_32_4x8_split', a, b, c, d), 1), ('i2i', b)),
   (('extract_i8', ('pack_32_4x8_split', a, b, c, d), 2), ('i2i', c)),
   (('extract_i8', ('pack_32_4x8_split', a, b, c, d), 3), ('i2i', d)),
   (('extract_u8', ('pack_32_4x8_split', a, b, c, d), 0), ('u2u', a)),
   (('extract_u8', ('pack_32_4x8_split', a, b, c, d), 1), ('u2u', b)),
   (('extract_u8', ('pack_32_4x8_split', a, b, c, d), 2), ('u2u', c)),
   (('extract_u8', ('pack_32_4x8_split', a, b, c, d), 3), ('u2u', d)),

   (('unpack_32_2x16', ('extract_u16', a, 0)), ('vec2', ('unpack_32_2x16.x', a), 0)),
   (('unpack_32_2x16', ('extract_u16', a, 1)), ('vec2', ('unpack_32_2x16.y', a), 0)),

   # Reduce intermediate precision with int64.
   (('u2u32', ('iadd(is_used_once)', 'a@64', b)),
    ('iadd', ('u2u32', a), ('u2u32', b))),
   (('unpack_64_2x32_split_x', ('iadd(is_used_once)', 'a@64', b)),
    ('iadd', ('u2u32', a), ('u2u32', b))),

   (('u2u32', ('imul(is_used_once)', 'a@64', b)),
    ('imul', ('u2u32', a), ('u2u32', b))),
   (('unpack_64_2x32_split_x', ('imul(is_used_once)', 'a@64', b)),
    ('imul', ('u2u32', a), ('u2u32', b))),

   (('u2f32', ('u2u64', 'a@32')), ('u2f32', a)),

   # UINT32_MAX < a just checks the high half of a 64-bit value. This occurs
   # when lowering convert_uint_sat(ulong). Although the replacement is more
   # instructions, it replaces a 64-bit instruction with a 32-bit instruction
   # and a move that will likely be coalesced.
   (('ult', 0xffffffff, 'a@64'), ('ine', ('unpack_64_2x32_split_y', a), 0)),

   # Redundant trip through 8-bit
   (('i2i16', ('u2u8', ('iand', 'a@16', 1))), ('iand', 'a@16', 1)),
   (('u2u16', ('u2u8', ('iand', 'a@16', 1))), ('iand', 'a@16', 1)),

   # Lowered pack followed by lowered unpack, for the high bits
   (('u2u32', ('ushr', ('ior', ('ishl', a, 32), ('u2u64', 'b@8')), 32)), ('u2u32', a)),
   (('u2u32', ('ushr', ('ior', ('ishl', a, 32), ('u2u64', 'b@16')), 32)), ('u2u32', a)),
   (('u2u32', ('ushr', ('ior', ('ishl', a, 32), ('u2u64', 'b@32')), 32)), ('u2u32', a)),
   (('u2u16', ('ushr', ('ior', ('ishl', a, 16), ('u2u32', 'b@8')), 16)), ('u2u16', a)),
   (('u2u16', ('ushr', ('ior', ('ishl', a, 16), ('u2u32', 'b@16')), 16)), ('u2u16', a)),
])

# After the ('extract_u8', a, 0) pattern, above, triggers, there will be
# patterns like those below.
for extract_op in ('extract_u8', 'extract_i8'):
   for op in ('ushr', 'ishr'):
      optimizations.extend([((extract_op, (op, a, 8), i), (extract_op, a, i + 1)) for i in range (0, 3)])
      optimizations.extend([((extract_op, (op, 'a@32',  8 * i), 0), (extract_op, a, i)) for i in range(2, 4)])
      optimizations.extend([((extract_op, (op, 'a@64',  8 * i), 0), (extract_op, a, i)) for i in range(2, 8)])

# After the ('extract_[iu]8', a, 3) patterns, above, trigger, there will be
# patterns like those below.
for op in ('extract_u8', 'extract_i8'):
   optimizations.extend([((op, ('ishl', 'a@16',      8),     1), (op, a, 0))])
   optimizations.extend([((op, ('ishl', 'a@32', 24 - 8 * i), 3), (op, a, i)) for i in range(2, -1, -1)])
   optimizations.extend([((op, ('ishl', 'a@64', 56 - 8 * i), 7), (op, a, i)) for i in range(6, -1, -1)])

for op, repl in [('ieq', 'ieq'), ('ine', 'ine'),
                 ('ult', 'ult'), ('ilt', 'ult'),
                 ('uge', 'uge'), ('ige', 'uge')]:
   optimizations.extend([
      ((op, ('pack_64_2x32_split', a, 0), ('pack_64_2x32_split', b, 0)), (repl, a, b)),
      ((op, ('pack_64_2x32_split', a, 0), '#b(is_upper_half_zero)'), (repl, a, ('unpack_64_2x32_split_x', b))),
      ((op, '#a(is_upper_half_zero)', ('pack_64_2x32_split', b, 0)), (repl, ('unpack_64_2x32_split_x', a), b)),

      ((op, ('pack_64_2x32_split', 0, a), ('pack_64_2x32_split', 0, b)), (op, a, b)),
      ((op, ('pack_64_2x32_split', 0, a), '#b(is_lower_half_zero)'), (op, a, ('unpack_64_2x32_split_y', b))),
      ((op, '#a(is_lower_half_zero)', ('pack_64_2x32_split', 0, b)), (op, ('unpack_64_2x32_split_y', a), b)),
   ])

optimizations.extend([
   # Subtracts
   (('ussub_4x8_vc4', a, 0), a),
   (('ussub_4x8_vc4', a, ~0), 0),
   # Lower all Subtractions first - they can get recombined later
   (('fsub', a, b), ('fadd', a, ('fneg', b))),
   (('isub', a, b), ('iadd', a, ('ineg', b))),
   (('uabs_usub', a, b), ('bcsel', ('ult', a, b), ('ineg', ('isub', a, b)), ('isub', a, b))),
   # This is correct.  We don't need isub_sat because the result type is unsigned, so it cannot overflow.
   (('uabs_isub', a, b), ('bcsel', ('ilt', a, b), ('ineg', ('isub', a, b)), ('isub', a, b))),
   (('bitz', a, b), ('inot', ('bitnz', a, b))),

   # Propagate negation up multiplication chains
   (('fmul(is_used_by_non_fsat)', ('fneg', a), b), ('fneg', ('fmul', a, b))),
   (('fmulz(is_used_by_non_fsat,nsz)', ('fneg', a), b), ('fneg', ('fmulz', a, b))),
   (('ffma', ('fneg', a), ('fneg', b), c), ('ffma', a, b, c)),
   (('ffmaz', ('fneg', a), ('fneg', b), c), ('ffmaz', a, b, c)),
   (('imul', ('ineg', a), b), ('ineg', ('imul', a, b))),

   # Propagate constants up multiplication chains
   (('~fmul', ('fmul(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c'), ('fmul', ('fmul', a, c), b)),
   (('~fmulz', ('fmulz(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c'), ('fmulz', ('fmulz', a, c), b)),
   (('~fmul', ('fmulz(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c(is_finite_not_zero)'), ('fmulz', ('fmul', a, c), b)),
   (('imul', ('imul(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c'), ('imul', ('imul', a, c), b)),
   # Prefer moving out a multiplication for more MAD/FMA-friendly code
   (('~fadd', ('fadd(is_used_once)', 'a(is_not_const)', 'b(is_fmul)'), '#c'), ('fadd', ('fadd', a, c), b)),
   (('~fadd', ('fadd(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c'), ('fadd', ('fadd', a, c), b)),
   (('iadd', ('iadd(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c'), ('iadd', ('iadd', a, c), b)),

   # Reassociate constants in add/mul chains so they can be folded together.
   # For now, we mostly only handle cases where the constants are separated by
   # a single non-constant.  We could do better eventually.
   (('~fmul', '#a', ('fmul', b, '#c')), ('fmul', ('fmul', a, c), b)),
   (('~fmulz', '#a', ('fmulz', b, '#c')), ('fmulz', ('fmulz', a, c), b)),
   (('~fmul', '#a(is_finite_not_zero)', ('fmulz', b, '#c')), ('fmulz', ('fmul', a, c), b)),
   (('imul', '#a', ('imul', b, '#c')), ('imul', ('imul', a, c), b)),
   (('~fadd', '#a',          ('fadd', b, '#c')),  ('fadd', ('fadd', a,          c),           b)),
   (('~fadd', '#a', ('fneg', ('fadd', b, '#c'))), ('fadd', ('fadd', a, ('fneg', c)), ('fneg', b))),
   (('iadd', '#a', ('iadd', b, '#c')), ('iadd', ('iadd', a, c), b)),
   (('iand', '#a', ('iand', b, '#c')), ('iand', ('iand', a, c), b)),
   (('ior',  '#a', ('ior',  b, '#c')), ('ior',  ('ior',  a, c), b)),
   (('ixor', '#a', ('ixor', b, '#c')), ('ixor', ('ixor', a, c), b)),
   (('ior', ('iand', a, '#c'), ('ior', b, ('iand', a, '#d'))), ('ior', b, ('iand', a, ('ior', c, d)))),

   # Reassociate add chains for more MAD/FMA-friendly code
   (('~fadd', ('fadd(is_used_once)', 'a(is_fmul)', 'b(is_fmul)'), 'c(is_not_fmul)'), ('fadd', ('fadd', a, c), b)),

   # Drop mul-div by the same value when there's no wrapping.
   (('idiv', ('imul(no_signed_wrap)', a, b), b), a, 'true', TestStatus.UNSUPPORTED), # No support for testing nsw

   # By definition...
   (('bcsel', ('ige', ('find_lsb', a), 0), ('find_lsb', a), -1), ('find_lsb', a)),
   (('bcsel', ('ige', ('ifind_msb', a), 0), ('ifind_msb', a), -1), ('ifind_msb', a)),
   (('bcsel', ('ige', ('ufind_msb', a), 0), ('ufind_msb', a), -1), ('ufind_msb', a)),
   (('bcsel', ('ige', ('ifind_msb_rev', a), 0), ('ifind_msb_rev', a), -1), ('ifind_msb_rev', a)),
   (('bcsel', ('ige', ('ufind_msb_rev', a), 0), ('ufind_msb_rev', a), -1), ('ufind_msb_rev', a)),

   (('bcsel', ('ine', a, 0), ('find_lsb', a), -1), ('find_lsb', a)),
   (('bcsel', ('ine', a, 0), ('ifind_msb', a), -1), ('ifind_msb', a)),
   (('bcsel', ('ine', a, 0), ('ufind_msb', a), -1), ('ufind_msb', a)),
   (('bcsel', ('ine', a, 0), ('ifind_msb_rev', a), -1), ('ifind_msb_rev', a)),
   (('bcsel', ('ine', a, 0), ('ufind_msb_rev', a), -1), ('ufind_msb_rev', a)),

   (('bcsel', ('ine', a, -1), ('ifind_msb', a), -1), ('ifind_msb', a)),
   (('bcsel', ('ine', a, -1), ('ifind_msb_rev', a), -1), ('ifind_msb_rev', a)),

   (('bcsel', ('ine', ('ifind_msb', 'a@32'), -1), ('iadd', 31, ('ineg', ('ifind_msb', a))), -1), ('ifind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ine', ('ufind_msb', 'a@32'), -1), ('iadd', 31, ('ineg', ('ufind_msb', a))), -1), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ieq', ('ifind_msb', 'a@32'), -1), -1, ('iadd', 31, ('ineg', ('ifind_msb', a)))), ('ifind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ieq', ('ufind_msb', 'a@32'), -1), -1, ('iadd', 31, ('ineg', ('ufind_msb', a)))), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ine', ('ifind_msb', 'a@32'), -1), ('iadd', 31, ('ineg', ('ifind_msb', a))), ('ifind_msb', a)), ('ifind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ine', ('ufind_msb', 'a@32'), -1), ('iadd', 31, ('ineg', ('ufind_msb', a))), ('ufind_msb', a)), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ieq', ('ifind_msb', 'a@32'), -1), ('ifind_msb', a), ('iadd', 31, ('ineg', ('ifind_msb', a)))), ('ifind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ieq', ('ufind_msb', 'a@32'), -1), ('ufind_msb', a), ('iadd', 31, ('ineg', ('ufind_msb', a)))), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ine', 'a@32', 0), ('iadd', 31, ('ineg', ('ufind_msb', a))), -1), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ieq', 'a@32', 0), -1, ('iadd', 31, ('ineg', ('ufind_msb', a)))), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ine', 'a@32', 0), ('iadd', 31, ('ineg', ('ufind_msb', a))), ('ufind_msb', a)), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('bcsel', ('ieq', 'a@32', 0), ('ufind_msb', a), ('iadd', 31, ('ineg', ('ufind_msb', a)))), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),

   (('bcsel', ('ine', ('ifind_msb_rev', 'a@32'), -1), ('iadd', 31, ('ineg', ('ifind_msb_rev', a))), -1), ('ifind_msb', a), '!options->lower_ifind_msb'),
   (('bcsel', ('ine', ('ufind_msb_rev', 'a@32'), -1), ('iadd', 31, ('ineg', ('ufind_msb_rev', a))), -1), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ieq', ('ifind_msb_rev', 'a@32'), -1), -1, ('iadd', 31, ('ineg', ('ifind_msb_rev', a)))), ('ifind_msb', a), '!options->lower_ifind_msb'),
   (('bcsel', ('ieq', ('ufind_msb_rev', 'a@32'), -1), -1, ('iadd', 31, ('ineg', ('ufind_msb_rev', a)))), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ine', ('ifind_msb_rev', 'a@32'), -1), ('iadd', 31, ('ineg', ('ifind_msb_rev', a))), ('ifind_msb_rev', a)), ('ifind_msb', a), '!options->lower_ifind_msb'),
   (('bcsel', ('ine', ('ufind_msb_rev', 'a@32'), -1), ('iadd', 31, ('ineg', ('ufind_msb_rev', a))), ('ufind_msb_rev', a)), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ieq', ('ifind_msb_rev', 'a@32'), -1), ('ifind_msb_rev', a), ('iadd', 31, ('ineg', ('ifind_msb_rev', a)))), ('ifind_msb', a), '!options->lower_ifind_msb'),
   (('bcsel', ('ieq', ('ufind_msb_rev', 'a@32'), -1), ('ufind_msb_rev', a), ('iadd', 31, ('ineg', ('ufind_msb_rev', a)))), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ine', 'a@32', 0), ('iadd', 31, ('ineg', ('ufind_msb_rev', a))), -1), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ieq', 'a@32', 0), -1, ('iadd', 31, ('ineg', ('ufind_msb_rev', a)))), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ine', 'a@32', 0), ('iadd', 31, ('ineg', ('ufind_msb_rev', a))), ('ufind_msb_rev', a)), ('ufind_msb', a), '!options->lower_ufind_msb'),
   (('bcsel', ('ieq', 'a@32', 0), ('ufind_msb_rev', a), ('iadd', 31, ('ineg', ('ufind_msb_rev', a)))), ('ufind_msb', a), '!options->lower_ufind_msb'),

   # Clear the LSB
   (('iand', a, ('inot', ('ishl', 1, ('find_lsb', a)))), ('iand', a, ('inot', ('ineg', a)))),

   # ufind_msb_rev can only have a 32-bit source, so we gate this on 32-bit.
   (('find_lsb', ('bitfield_reverse', 'a@32')), ('ufind_msb_rev', a), 'options->has_find_msb_rev'),
   (('ufind_msb_rev', ('bitfield_reverse', 'a@32')), ('find_lsb', a), '!options->lower_find_lsb'),

   (('ifind_msb', ('f2i32(is_used_once)', a)), ('ufind_msb', ('f2i32', ('fabs', a))), 'true', TestStatus.XFAIL), # ifind_msb(f2i32(-1.0)) goes from ~0 to 0.
   (('ifind_msb', ('extract_u8', a, b)),       ('ufind_msb', ('extract_u8', a, b))),
   (('ifind_msb', ('extract_u16', a, b)),      ('ufind_msb', ('extract_u16', a, b))),
   (('ifind_msb', ('imax', a, 1)),             ('ufind_msb', ('imax', a, 1))),

   (('fmul', ('bcsel(is_used_once)', c, -1.0, 1.0), b), ('bcsel', c, ('fneg', b), ('fcanonicalize', b))),
   (('fmul', ('bcsel(is_used_once)', c, 1.0, -1.0), b), ('bcsel', c, ('fcanonicalize', b), ('fneg', b))),
   (('fmulz(nsz)', ('bcsel(is_used_once)', c, -1.0, 1.0), b), ('bcsel', c, ('fneg', b), ('fcanonicalize', b))),
   (('fmulz(nsz)', ('bcsel(is_used_once)', c, 1.0, -1.0), b), ('bcsel', c, ('fcanonicalize', b), ('fneg', b))),
   (('fabs', ('bcsel(is_used_once)', b, ('fneg', a), a)), ('fabs', a)),
   (('fabs', ('bcsel(is_used_once)', b, a, ('fneg', a))), ('fabs', a)),
   (('bcsel(is_only_used_as_float_nsz)', ('flt', a, 0.0), ('fneg', a), a), ('fabs', a)),

   (('bcsel', a, ('bcsel(is_used_once)', b, c, d), d), ('bcsel', ('iand', a, b), c, d)),
   (('bcsel', a, ('bcsel(is_used_once)', b, d, c), d), ('bcsel', ('iand', a, ('inot', b)), c, d)),
   (('bcsel', a, b, ('bcsel(is_used_once)', c, b, d)), ('bcsel', ('ior', a, c), b, d)),
   (('bcsel', a, b, ('bcsel(is_used_once)', c, d, b)), ('bcsel', ('iand', c, ('inot', a)), d, b)),

   # Misc. lowering
   (('fmod', a, b), ('fsub', a, ('fmul', b, ('ffloor', ('fdiv', a, b)))), 'options->lower_fmod'),
   (('frem', a, b), ('fsub', a, ('fmul', b, ('ftrunc', ('fdiv', a, b)))), 'options->lower_fmod'),
   (('uadd_carry', a, b), ('b2i', ('ult', ('iadd', a, b), a)), 'options->lower_uadd_carry'),
   (('usub_borrow', a, b), ('b2i', ('ult', a, b)), 'options->lower_usub_borrow'),

   # hand-rolled iadd64
   (('vec2', ('iadd@32', a, c), ('iadd', ('iadd', b, d), ('uadd_carry', a, c))),
     ('unpack_64_2x32', ('iadd', ('pack_64_2x32_split', a, b), ('pack_64_2x32_split', c, d))), '!(options->lower_int64_options & nir_lower_iadd64)'),

   (('ihadd', a, b), ('iadd', ('iand', a, b), ('ishr', ('ixor', a, b), 1)), 'options->lower_hadd'),
   (('uhadd', a, b), ('iadd', ('iand', a, b), ('ushr', ('ixor', a, b), 1)), 'options->lower_hadd'),
   (('irhadd', a, b), ('isub', ('ior', a, b), ('ishr', ('ixor', a, b), 1)), 'options->lower_hadd'),
   (('urhadd', a, b), ('isub', ('ior', a, b), ('ushr', ('ixor', a, b), 1)), 'options->lower_hadd'),
   (('ihadd@64', a, b), ('iadd', ('iand', a, b), ('ishr', ('ixor', a, b), 1)), 'options->lower_hadd64 || (options->lower_int64_options & nir_lower_iadd64) != 0'),
   (('uhadd@64', a, b), ('iadd', ('iand', a, b), ('ushr', ('ixor', a, b), 1)), 'options->lower_hadd64 || (options->lower_int64_options & nir_lower_iadd64) != 0'),
   (('irhadd@64', a, b), ('isub', ('ior', a, b), ('ishr', ('ixor', a, b), 1)), 'options->lower_hadd64 || (options->lower_int64_options & nir_lower_iadd64) != 0'),
   (('urhadd@64', a, b), ('isub', ('ior', a, b), ('ushr', ('ixor', a, b), 1)), 'options->lower_hadd64 || (options->lower_int64_options & nir_lower_iadd64) != 0'),

   (('imul_32x16', a, b), ('imul', a, ('extract_i16', b, 0)), 'options->lower_mul_32x16'),
   (('umul_32x16', a, b), ('imul', a, ('extract_u16', b, 0)), 'options->lower_mul_32x16'),

   (('uadd_sat@64', a, b), ('bcsel', ('ult', ('iadd', a, b), a), -1, ('iadd', a, b)),
    'options->lower_uadd_sat || (options->lower_int64_options & (nir_lower_iadd64 | nir_lower_uadd_sat64)) != 0'),
   (('uadd_sat', a, b), ('bcsel', ('ult', ('iadd', a, b), a), -1, ('iadd', a, b)), 'options->lower_uadd_sat'),
   (('usub_sat', a, b), ('bcsel', ('ult', a, b), 0, ('isub', a, b)), 'options->lower_usub_sat'),
   (('usub_sat@64', a, b), ('bcsel', ('ult', a, b), 0, ('isub', a, b)), '(options->lower_int64_options & nir_lower_usub_sat64) != 0'),

   # int64_t sum = a + b;
   #
   # if (a < 0 && b < 0 && a < sum)
   #    sum = INT64_MIN;
   # } else if (a >= 0 && b >= 0 && sum < a)
   #    sum = INT64_MAX;
   # }
   #
   # A couple optimizations are applied.
   #
   # 1. a < sum => sum >= 0.  This replacement works because it is known that
   #    a < 0 and b < 0, so sum should also be < 0 unless there was
   #    underflow.
   #
   # 2. sum < a => sum < 0.  This replacement works because it is known that
   #    a >= 0 and b >= 0, so sum should also be >= 0 unless there was
   #    overflow.
   #
   # 3. Invert the second if-condition and swap the order of parameters for
   #    the bcsel. !(a >= 0 && b >= 0 && sum < 0) becomes !(a >= 0) || !(b >=
   #    0) || !(sum < 0), and that becomes (a < 0) || (b < 0) || (sum >= 0)
   #
   # On Intel Gen11, this saves ~11 instructions.
   (('iadd_sat@64', a, b), ('bcsel',
                            ('iand', ('iand', ('ilt', a, 0), ('ilt', b, 0)), ('ige', ('iadd', a, b), 0)),
                            0x8000000000000000,
                            ('bcsel',
                             ('ior', ('ior', ('ilt', a, 0), ('ilt', b, 0)), ('ige', ('iadd', a, b), 0)),
                             ('iadd', a, b),
                             0x7fffffffffffffff)),
    '(options->lower_int64_options & nir_lower_iadd_sat64) != 0'),

   # int64_t sum = a - b;
   #
   # if (a < 0 && b >= 0 && a < sum)
   #    sum = INT64_MIN;
   # } else if (a >= 0 && b < 0 && a >= sum)
   #    sum = INT64_MAX;
   # }
   #
   # Optimizations similar to the iadd_sat case are applied here.
   (('isub_sat@64', a, b), ('bcsel',
                            ('iand', ('iand', ('ilt', a, 0), ('ige', b, 0)), ('ige', ('isub', a, b), 0)),
                            0x8000000000000000,
                            ('bcsel',
                             ('ior', ('ior', ('ilt', a, 0), ('ige', b, 0)), ('ige', ('isub', a, b), 0)),
                             ('isub', a, b),
                             0x7fffffffffffffff)),
    '(options->lower_int64_options & nir_lower_iadd_sat64) != 0'),

   # These are done here instead of in the backend because the int64 lowering
   # pass will make a mess of the patterns.  The first patterns are
   # conditioned on nir_lower_minmax64 because it was not clear that it was
   # always an improvement on platforms that have real int64 support.  No
   # shaders in shader-db hit this, so it was hard to say one way or the
   # other.
   (('ilt', ('imax(is_used_once)', 'a@64', 'b@64'), 0), ('ilt', ('imax', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b)), 0), '(options->lower_int64_options & nir_lower_minmax64) != 0'),
   (('ilt', ('imin(is_used_once)', 'a@64', 'b@64'), 0), ('ilt', ('imin', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b)), 0), '(options->lower_int64_options & nir_lower_minmax64) != 0'),
   (('ige', ('imax(is_used_once)', 'a@64', 'b@64'), 0), ('ige', ('imax', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b)), 0), '(options->lower_int64_options & nir_lower_minmax64) != 0'),
   (('ige', ('imin(is_used_once)', 'a@64', 'b@64'), 0), ('ige', ('imin', ('unpack_64_2x32_split_y', a), ('unpack_64_2x32_split_y', b)), 0), '(options->lower_int64_options & nir_lower_minmax64) != 0'),
   (('ilt', 'a@64', 0), ('ilt', ('unpack_64_2x32_split_y', a), 0), '(options->lower_int64_options & nir_lower_icmp64) != 0'),
   (('ige', 'a@64', 0), ('ige', ('unpack_64_2x32_split_y', a), 0), '(options->lower_int64_options & nir_lower_icmp64) != 0'),

   (('ine', 'a@64', 0), ('ine', ('ior', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_y', a)), 0), '(options->lower_int64_options & nir_lower_icmp64) != 0'),
   (('ieq', 'a@64', 0), ('ieq', ('ior', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_y', a)), 0), '(options->lower_int64_options & nir_lower_icmp64) != 0'),
   # 0u < uint(a) <=> uint(a) != 0u
   (('ult', 0, 'a@64'), ('ine', ('ior', ('unpack_64_2x32_split_x', a), ('unpack_64_2x32_split_y', a)), 0), '(options->lower_int64_options & nir_lower_icmp64) != 0'),

   (('ibitfield_extract', 'value@32', 'offset', 'bits'),
    ('bcsel', ('ult', 31, 'bits'), 'value',
              ('ibfe', 'value', 'offset', 'bits')),
    'options->lower_bitfield_extract && options->has_bfe'),

   (('ubitfield_extract', 'value@32', 'offset', 'bits'),
    ('bcsel', ('ult', 31, 'bits'), 'value',
              ('ubfe', 'value', 'offset', 'bits')),
    'options->lower_bitfield_extract && options->has_bfe'),

   # (src0 & src1) | (~src0 & src2). Constant fold if a src is 0/-1.
   (('bitfield_select', a, b, 0), ('iand', a, b)),
   (('bitfield_select', a, 0, b), ('iand', ('inot', a), b)),
   (('bitfield_select', 0, a, b), b),
   (('bitfield_select', a, b, -1), ('ior', ('inot', a), b)),
   (('bitfield_select', a, -1, b), ('ior', a, b)),
   (('bitfield_select', -1, a, b), a),
   (('bitfield_select', a, b, b), b),
   (('bitfield_select', a, ('inot', b), b), ('ixor', a, b)),
   (('bitfield_select', a, b, ('inot', b)), ('inot', ('ixor', a, b))),
   (('bitfield_select', a, ('iand', a, b), c), ('bitfield_select', a, b, c)),
   (('bitfield_select', a, b, ('iand', ('inot', a), c)), ('bitfield_select', a, b, c)),
   (('bitfield_select', ('inot', a), b, c), ('bitfield_select', a, c, b)),
   (('bitfield_select', ('ineg', ('b2i', 'a@1')), b, c), ('bcsel', a, b, c)),

   (('ior', ('iand', a, b), ('iand', ('inot', a), c)), ('bitfield_select', a, b, c), 'options->has_bitfield_select'),
   (('iadd', ('iand', a, b), ('iand', ('inot', a), c)), ('bitfield_select', a, b, c), 'options->has_bitfield_select'),
   (('ixor', ('iand', a, b), ('iand', ('inot', a), c)), ('bitfield_select', a, b, c), 'options->has_bitfield_select'),
   (('ixor', ('iand', a, ('ixor', b, c)), c), ('bitfield_select', a, b, c), 'options->has_bitfield_select'),

   # The patterns above could create 1bit bitfield_select, but that's really just bcsel.
   (('bitfield_select@1', a, b, c), ('bcsel', a, b, c)),

   # Note that these opcodes are defined to only use the five least significant bits of 'offset' and 'bits'
   (('ubfe', 'value', 'offset', ('iand', 31, 'bits')), ('ubfe', 'value', 'offset', 'bits')),
   (('ubfe', 'value', ('iand', 31, 'offset'), 'bits'), ('ubfe', 'value', 'offset', 'bits')),
   (('ibfe', 'value', 'offset', ('iand', 31, 'bits')), ('ibfe', 'value', 'offset', 'bits')),
   (('ibfe', 'value', ('iand', 31, 'offset'), 'bits'), ('ibfe', 'value', 'offset', 'bits')),
   (('bfm', 'bits', ('iand', 31, 'offset')), ('bfm', 'bits', 'offset')),
   (('bfm', ('iand', 31, 'bits'), 'offset'), ('bfm', 'bits', 'offset')),

   # Optimizations for ubitfield_extract(value, offset, umin(bits, 32-(offset&0x1f))) and such
   (('ult', a, ('umin', ('iand', a, b), c)), False),
   (('ult', 31, ('umin', '#bits(is_ult_32)', a)), False),
   (('ubfe', 'value', 'offset', ('umin', 'width', ('iadd', 32, ('ineg', ('iand', 31, 'offset'))))),
    ('ubfe', 'value', 'offset', 'width'), 'true', TestStatus.XFAIL),
   (('ibfe', 'value', 'offset', ('umin', 'width', ('iadd', 32, ('ineg', ('iand', 31, 'offset'))))),
    ('ibfe', 'value', 'offset', 'width'), 'true', TestStatus.XFAIL),
   (('bfm', ('umin', 'width', ('iadd', 32, ('ineg', ('iand', 31, 'offset')))), 'offset'),
    ('bfm', 'width', 'offset'), 'true', TestStatus.XFAIL),

   # open-coded BFM
   (('iadd@32', ('ishl', 1, a), -1), ('bfm', a, 0), 'options->has_bfm'),
   (('ishl', ('bfm', a, 0), b), ('bfm', a, b)),

   # Section 8.8 (Integer Functions) of the GLSL 4.60 spec says:
   #
   #    If bits is zero, the result will be zero.
   #
   # These patterns prevent other patterns from generating invalid results
   # when count is zero.
   (('ubfe', a, b, 0), 0),
   (('ibfe', a, b, 0), 0),

   (('ubfe', a, 0, '#b'), ('iand', a, ('ushr', 0xffffffff, ('ineg', b))), 'true', TestStatus.XFAIL),

   (('b2i32', ('ine', ('ubfe', a, b, 1), 0)), ('ubfe', a, b, 1)),
   (('b2i32', ('ine', ('ibfe', a, b, 1), 0)), ('ubfe', a, b, 1)), # ubfe in the replacement is correct
   (('ine', ('ibfe(is_used_once)', a, '#b', '#c'), 0), ('ine', ('iand', a, ('ishl', ('ushr', 0xffffffff, ('ineg', c)), b)), 0), 'true', TestStatus.XFAIL),
   (('ieq', ('ibfe(is_used_once)', a, '#b', '#c'), 0), ('ieq', ('iand', a, ('ishl', ('ushr', 0xffffffff, ('ineg', c)), b)), 0), 'true', TestStatus.XFAIL),
   (('ine', ('ubfe(is_used_once)', a, '#b', '#c'), 0), ('ine', ('iand', a, ('ishl', ('ushr', 0xffffffff, ('ineg', c)), b)), 0), 'true', TestStatus.XFAIL),
   (('ieq', ('ubfe(is_used_once)', a, '#b', '#c'), 0), ('ieq', ('iand', a, ('ishl', ('ushr', 0xffffffff, ('ineg', c)), b)), 0), 'true', TestStatus.XFAIL),

   (('ine', ('iand(is_used_once)', ('ushr', a, '#b'), '#c'), 0), ('ine', ('iand', a, ('ishl', c, b)), 0)),
   (('ine', ('iand(is_used_once)', ('ishl', a, '#b'), '#c'), 0), ('ine', ('iand', a, ('ushr', c, b)), 0)),
   (('ieq', ('iand(is_used_once)', ('ushr', a, '#b'), '#c'), 0), ('ieq', ('iand', a, ('ishl', c, b)), 0)),
   (('ieq', ('iand(is_used_once)', ('ishl', a, '#b'), '#c'), 0), ('ieq', ('iand', a, ('ushr', c, b)), 0)),

   (('ifind_msb', 'value'),
    ('ufind_msb', ('bcsel', ('ilt', 'value', 0), ('inot', 'value'), 'value')),
    'options->lower_ifind_msb && !options->has_find_msb_rev && !options->has_uclz'),

   (('ifind_msb', 'value'),
    ('bcsel', ('ige', ('ifind_msb_rev', 'value'), 0),
     ('isub', 31, ('ifind_msb_rev', 'value')),
     ('ifind_msb_rev', 'value')),
    'options->lower_ifind_msb && options->has_find_msb_rev'),

   # uclz of an absolute value source almost always does the right thing.
   # There are a couple problem values:
   #
   # * 0x80000000.  Since abs(0x80000000) == 0x80000000, uclz returns 0.
   #   However, findMSB(int(0x80000000)) == 30.
   #
   # * 0xffffffff.  Since abs(0xffffffff) == 1, uclz returns 31.  Section 8.8
   #   (Integer Functions) of the GLSL 4.50 spec says:
   #
   #    For a value of zero or negative one, -1 will be returned.
   #
   # * Negative powers of two.  uclz(abs(-(1<<x))) returns x, but
   #   findMSB(-(1<<x)) should return x-1.
   #
   # For all negative number cases, including 0x80000000 and 0xffffffff, the
   # correct value is obtained from uclz if instead of negating the (already
   # negative) value the logical-not is used.  A conditional logical-not can
   # be achieved by (x ^ (x >> 31)).
   (('ifind_msb', 'value'),
    ('isub', 31, ('uclz', ('ixor', 'value', ('ishr', 'value', 31)))),
    'options->lower_ifind_msb && options->has_uclz'),

   (('ufind_msb', 'value@32'),
    ('bcsel', ('ige', ('ufind_msb_rev', 'value'), 0),
     ('isub', 31, ('ufind_msb_rev', 'value')),
     ('ufind_msb_rev', 'value')),
    'options->lower_ufind_msb && options->has_find_msb_rev'),

   (('ufind_msb', 'value@32'),
    ('isub', 31, ('uclz', 'value')),
    'options->lower_ufind_msb && options->has_uclz'),

   (('uclz', a), ('umin', 32, ('ufind_msb_rev', a)), '!options->has_uclz && options->has_find_msb_rev'),

   (('find_lsb', 'value@64'),
    ('ufind_msb', ('iand', 'value', ('ineg', 'value'))),
    'options->lower_find_lsb'),

   (('find_lsb', 'value'),
    ('ufind_msb', ('u2u32', ('iand', 'value', ('ineg', 'value')))),
    'options->lower_find_lsb'),

   (('extract_i8', a, 'b@32'),
    ('ishr', ('ishl', a, ('imul', ('isub', 3, b), 8)), 24),
    'options->lower_extract_byte'),

   (('extract_u8', a, 'b@32'),
    ('iand', ('ushr', a, ('imul', b, 8)), 0xff),
    'options->lower_extract_byte'),

   (('extract_i16', a, 'b@32'),
    ('ishr', ('ishl', a, ('imul', ('isub', 1, b), 16)), 16),
    'options->lower_extract_word'),

   (('extract_u16', a, 'b@32'),
    ('iand', ('ushr', a, ('imul', b, 16)), 0xffff),
    'options->lower_extract_word'),

    (('pack_unorm_2x16', 'v'),
     ('pack_uvec2_to_uint',
        ('f2u32', ('fround_even', ('fmul', ('fsat', 'v'), 65535.0)))),
     'options->lower_pack_unorm_2x16'),

    (('pack_unorm_4x8', 'v'),
     ('pack_uvec4_to_uint',
        ('f2u32', ('fround_even', ('fmul', ('fsat', 'v'), 255.0)))),
     'options->lower_pack_unorm_4x8 && !options->has_pack_32_4x8'),

    (('pack_unorm_4x8', 'v'),
     ('pack_32_4x8',
        ('f2u8', ('fround_even', ('fmul', ('fsat', 'v'), 255.0)))),
     'options->lower_pack_unorm_4x8 && options->has_pack_32_4x8'),

    (('pack_snorm_2x16', 'v'),
     ('pack_uvec2_to_uint',
        ('f2i32', ('fround_even', ('fmul', ('fmin', 1.0, ('fmax', -1.0, 'v')), 32767.0)))),
     'options->lower_pack_snorm_2x16'),

    (('pack_snorm_4x8', 'v'),
     ('pack_uvec4_to_uint',
        ('f2i32', ('fround_even', ('fmul', ('fmin', 1.0, ('fmax', -1.0, 'v')), 127.0)))),
     'options->lower_pack_snorm_4x8 && !options->has_pack_32_4x8'),

    (('pack_snorm_4x8', 'v'),
     ('pack_32_4x8',
        ('f2i8', ('fround_even', ('fmul', ('fmin', 1.0, ('fmax', -1.0, 'v')), 127.0)))),
     'options->lower_pack_snorm_4x8 && options->has_pack_32_4x8'),

    (('unpack_unorm_2x16', 'v'),
     ('fdiv', ('u2f32', ('vec2', ('extract_u16', 'v', 0),
                                  ('extract_u16', 'v', 1))),
              65535.0),
     'options->lower_unpack_unorm_2x16'),

    (('unpack_unorm_4x8', 'v'),
     ('fdiv', ('u2f32', ('vec4', ('extract_u8', 'v', 0),
                                  ('extract_u8', 'v', 1),
                                  ('extract_u8', 'v', 2),
                                  ('extract_u8', 'v', 3))),
              255.0),
     'options->lower_unpack_unorm_4x8'),

    (('unpack_snorm_2x16', 'v'),
     ('fmin', 1.0, ('fmax', -1.0, ('fdiv', ('i2f', ('vec2', ('extract_i16', 'v', 0),
                                                            ('extract_i16', 'v', 1))),
                                           32767.0))),
     'options->lower_unpack_snorm_2x16'),

    (('unpack_snorm_4x8', 'v'),
     ('fmin', 1.0, ('fmax', -1.0, ('fdiv', ('i2f', ('vec4', ('extract_i8', 'v', 0),
                                                            ('extract_i8', 'v', 1),
                                                            ('extract_i8', 'v', 2),
                                                            ('extract_i8', 'v', 3))),
                                           127.0))),
     'options->lower_unpack_snorm_4x8'),

    # f2u32(fmul(unpack_*norm_*)). These are exact replacements.
    # 0x37800080 = 1.0 / 65535.0
    # 0x3b808081 = 1.0 / 255.0
    # 0x38000100 = 1.0 / 32767.0
    # 0x3c010204 = 1.0 / 127.0
    (('f2u32', ('fmul', ('fmul', ('u2f32', ('extract_u16', 'a@32', b)), 0x37800080), 65535.0)),
     ('extract_u8', a, b),
     '!nir_is_rounding_mode_rtz(info->float_controls_execution_mode, 32)'),
    (('f2u32', ('fmul', ('fmul', ('u2f32', ('extract_u8', 'a@32', b)), 0x3b808081), 255.0)),
     ('extract_u8', a, b)),
    (('f2u32', ('fmul', ('fmin', 1.0, ('fmax', -1.0, ('fmul', ('i2f32', ('extract_i16', 'a@32', b)), 0x38000100))), 32767.0)),
     ('imax', ('extract_i16', a, b), -32767),
     '!nir_is_rounding_mode_rtz(info->float_controls_execution_mode, 32)'),
    (('f2u32', ('fmul', ('fmin', 1.0, ('fmax', -1.0, ('fmul', ('i2f32', ('extract_i8', 'a@32', b)), 0x3c010204))), 127.0)),
     ('imax', ('extract_i8', a, b), -127),
     '!nir_is_rounding_mode_rtz(info->float_controls_execution_mode, 32)'),

   (('pack_half_2x16_split', 'a@32', 'b@32'),
    ('ior', ('ishl', ('u2u32', ('f2f16', b)), 16), ('u2u32', ('f2f16', a))),
    'options->lower_pack_split'),

   (('isign', a), ('imin', ('imax', a, -1), 1), 'options->lower_isign'),
   (('imin', ('imax', a, -1), 1), ('isign', a), '!options->lower_isign'),
   (('imax', ('imin', a, 1), -1), ('isign', a), '!options->lower_isign'),
   # float(0 < NaN) - float(NaN < 0) = float(False) - float(False) = 0 - 0 = 0
   # Mark the new comparisons precise to prevent them being changed to 'a !=
   # 0' or 'a == 0'.
   (('fsign', a), ('fsub', ('b2f', ('flt(preserve_nan_inf)', 0.0, a)), ('b2f', ('flt(preserve_nan_inf)', a, 0.0))), 'options->lower_fsign'),
   (('fsign', 'a@64'), ('fsub', ('b2f', ('flt(preserve_nan_inf)', 0.0, a)), ('b2f', ('flt(preserve_nan_inf)', a, 0.0))), 'options->lower_doubles_options & nir_lower_dsign'),

   # Address/offset calculations:
   # Drivers supporting imul24 should use a pass like nir_lower_amul(), this
   # rule converts everyone else to imul:
   (('amul', a, b), ('imul', a, b), '!options->has_imul24 && !options->has_amul'),

   # udiv_aligned_4 assumes the source is a multiple of 4 specifically to enable
   # this identity. Usually this transform would require masking.
   (('amul', ('udiv_aligned_4', a), 4), a),
   (('imul', ('udiv_aligned_4', a), 4), a),

   (('umul24', a, b),
    ('imul', ('iand', a, 0xffffff), ('iand', b, 0xffffff)),
    '!options->has_umul24'),
   (('umad24', a, b, c),
    ('iadd', ('imul', ('iand', a, 0xffffff), ('iand', b, 0xffffff)), c),
    '!options->has_umad24'),

   # Relaxed 24bit ops
   (('imul24_relaxed', a, b), ('imul24', a, b), '!options->has_mul24_relaxed && options->has_imul24'),
   (('imul24_relaxed', a, b), ('imul', a, b), '!options->has_mul24_relaxed && !options->has_imul24'),
   (('umad24_relaxed', a, b, c), ('umad24', a, b, c),  'options->has_umad24'),
   (('umad24_relaxed', a, b, c), ('iadd', ('umul24_relaxed', a, b), c), '!options->has_umad24'),
   (('umul24_relaxed', a, b), ('umul24', a, b), '!options->has_mul24_relaxed && options->has_umul24'),
   (('umul24_relaxed', a, b), ('imul', a, b), '!options->has_mul24_relaxed && !options->has_umul24'),

   (('imad24_ir3', a, b, 0), ('imul24', a, b)),
   (('imad24_ir3', a, 0, c), (c)),
   (('imad24_ir3', a, 1, c), ('iadd', a, c)), # this is not correct -- a's sign extension gets dropped.

   # if first two srcs are const, crack apart the imad so constant folding
   # can clean up the imul:
   # TODO ffma should probably get a similar rule:
   (('imad24_ir3', '#a', '#b', c), ('iadd', ('imul24', a, b), c)),

   # These will turn 24b address/offset calc back into 32b shifts, but
   # it should be safe to get back some of the bits of precision that we
   # already decided were no necessary:
   (('imul24', a, '#b@32(is_pos_power_of_two)'), ('ishl', a, ('find_lsb', b)), '!options->lower_bitops'),
   (('imul24', a, '#b@32(is_neg_power_of_two)'), ('ineg', ('ishl', a, ('find_lsb', ('iabs', b)))), '!options->lower_bitops'),
   (('imul24', a, 0), (0)),

   (('imul_high@16', a, b), ('i2i16', ('ishr', ('imul24_relaxed', ('i2i32', a), ('i2i32', b)), 16)), 'options->lower_mul_high16'),
   (('umul_high@16', a, b), ('u2u16', ('ushr', ('umul24_relaxed', ('u2u32', a), ('u2u32', b)), 16)), 'options->lower_mul_high16'),

   # Optimize vec2 unsigned comparison predicates to usub_sat with clamp.
   (('b2i16', ('vec2', ('ult', 'a@16', b), ('ult', 'c@16', d))),
    ('umin', 1, ('usub_sat', ('vec2', b, d), ('vec2', a, c))),
    'options->vectorize_vec2_16bit && !options->lower_usub_sat'),
   (('b2i16', ('vec2', ('uge', 'a@16', '#b(is_not_uint_zero)'), ('uge', 'c@16', '#d(is_not_uint_zero)'))),
    ('umin', 1, ('usub_sat', ('vec2', a, c), ('iadd', ('vec2', b, d), -1))),
    'options->vectorize_vec2_16bit && !options->lower_usub_sat'),
   (('b2i16', ('vec2', ('uge', '#a(is_not_uint_max)', 'b@16'), ('uge', '#c(is_not_uint_max)', 'd@16'))),
    ('umin', 1, ('usub_sat', ('iadd', ('vec2', a, c), 1), ('vec2', b, d))),
    'options->vectorize_vec2_16bit && !options->lower_usub_sat'),
])

for bit_size in [8, 16, 32, 64]:
   cond = '!options->lower_uadd_sat'
   if bit_size == 64:
      cond += ' && !(options->lower_int64_options & (nir_lower_iadd64 | nir_lower_uadd_sat64))'
   add = 'iadd@' + str(bit_size)

   optimizations += [
      (('bcsel', ('ult', ('iadd', a, b), a), -1, (add, a, b)), ('uadd_sat', a, b), cond),
      (('bcsel', ('uge', ('iadd', a, b), a), (add, a, b), -1), ('uadd_sat', a, b), cond),
      (('bcsel', ('ieq', ('uadd_carry', a, b), 0), (add, a, b), -1), ('uadd_sat', a, b), cond),
      (('bcsel', ('ine', ('uadd_carry', a, b), 0), -1, (add, a, b)), ('uadd_sat', a, b), cond),
   ]

optimizations += [
   # Alternative 32-bit lowerings that use bitfield_select/bfi.
   (('bitfield_insert', 'base@32', 'insert', 'offset', 'bits'),
    ('bcsel', ('ult', 31, 'bits'), 'insert',
              ('bitfield_select', ('bfm', 'bits', 'offset'), ('ishl', 'insert', 'offset'), 'base')),
    'options->lower_bitfield_insert && options->has_bfm && options->has_bitfield_select && !options->has_bfi'),
   (('bitfield_insert', 'base@32', 'insert', 'offset', 'bits'),
    ('bcsel', ('ult', 31, 'bits'), 'insert',
              ('bfi', ('bfm', 'bits', 'offset'), 'insert', 'base')),
    'options->lower_bitfield_insert && options->has_bfm && options->has_bfi'),
]

# 64bit lowering is handled by nir_lower_int64
for bit_size in [8, 16, 32]:
   bit_size_str = f'{bit_size}' if bit_size < 32 else ''
   extract_opt = f'options->lower_bitfield_extract{bit_size_str}'
   if bit_size == 32:
       extract_opt += ' && !options->has_bfe'

   optimizations += [
       (('ibitfield_extract', f'value@{bit_size}', 'offset', 'bits'),
        ('bcsel', ('ieq', 0, 'bits'),
         0,
         ('ishr',
          ('ishl', 'value', ('isub', ('isub', bit_size, 'bits'), 'offset')),
          ('isub', bit_size, 'bits'))),
        extract_opt),

       (('ubitfield_extract', f'value@{bit_size}', 'offset', 'bits'),
        ('iand',
         ('ushr', 'value', 'offset'),
         ('bcsel', ('ieq', 'bits', bit_size),
          -1,
          ('isub', ('ishl', 1, 'bits'), 1))),
        extract_opt),
   ]

for sz in [8, 16, 32, 64]:
   base = f'base@{sz}'

   if sz == 8 or sz == 16:
      optimizations += [
         # Alternative 8-bit/16-bit lowerings that use bitfield_select/bfi.
         (('bitfield_insert', base, 'insert', 'offset', 'bits'),
          ('bitfield_select', (f'u2u{sz}', ('bfm', 'bits', 'offset')), ('ishl', 'insert', 'offset'), 'base'),
          'options->lower_bitfield_insert && options->has_bfm && options->has_bitfield_select && !options->has_bfi'),
         (('bitfield_insert', base, 'insert', 'offset', 'bits'),
          (f'u2u{sz}', ('bfi', ('bfm', 'bits', 'offset'), ('u2u32', 'insert'), ('u2u32', 'base'))),
          'options->lower_bitfield_insert && options->has_bfm && options->has_bfi'),
      ]

   optimizations += [
       # Alternative lowering that doesn't rely on bfi or bfm.
       (('bitfield_insert', base, 'insert', 'offset', 'bits'),
        ('bcsel', ('ult', sz - 1, 'bits'), 'insert',
        (('ior',
         ('iand', 'base', ('inot', ('ishl', ('isub', ('ishl', 1, 'bits'), 1), 'offset'))),
         ('iand', ('ishl', 'insert', 'offset'), ('ishl', ('isub', ('ishl', 1, 'bits'), 1), 'offset'))))),
        'true' if sz == 64 else 'options->lower_bitfield_insert && (!options->has_bfm || (!options->has_bfi && !options->has_bitfield_select))', TestStatus.XFAIL if sz == 64 else TestStatus.PASS),
    ]

for bit_size in [8, 16, 32, 64]:
   cond = '!options->lower_usub_sat'
   if bit_size == 64:
      cond += ' && !(options->lower_int64_options & nir_lower_usub_sat64)'
   add = 'iadd@' + str(bit_size)

   optimizations += [
      (('bcsel', ('ult', a, b), 0, (add, a, ('ineg', b))), ('usub_sat', a, b), cond),
      (('bcsel', ('uge', a, b), (add, a, ('ineg', b)), 0), ('usub_sat', a, b), cond),
      (('bcsel', ('ieq', ('usub_borrow', a, b), 0), (add, a, ('ineg', b)), 0), ('usub_sat', a, b), cond),
      (('bcsel', ('ine', ('usub_borrow', a, b), 0), 0, (add, a, ('ineg', b))), ('usub_sat', a, b), cond),
   ]

# bit_size dependent lowerings
for bit_size in [8, 16, 32, 64]:
   # convenience constants
   intmax = (1 << (bit_size - 1)) - 1
   intmin = 1 << (bit_size - 1)

   optimizations += [
      (('iadd_sat@' + str(bit_size), a, b),
       ('bcsel', ('ige', b, 1), ('bcsel', ('ilt', ('iadd', a, b), a), intmax, ('iadd', a, b)),
                                ('bcsel', ('ilt', a, ('iadd', a, b)), intmin, ('iadd', a, b))), 'options->lower_iadd_sat'),
      (('isub_sat@' + str(bit_size), a, b),
       ('bcsel', ('ilt', b, 0), ('bcsel', ('ilt', ('isub', a, b), a), intmax, ('isub', a, b)),
                                ('bcsel', ('ilt', a, ('isub', a, b)), intmin, ('isub', a, b))), 'options->lower_iadd_sat'),
   ]

invert = OrderedDict([('feq', 'fneu'), ('fneu', 'feq')])

for left, right in itertools.combinations_with_replacement(invert.keys(), 2):
   optimizations.append((('inot', ('ior(is_used_once)', (left + '(is_used_once)', a, b),
                                                        (right + '(is_used_once)', c, d))),
                         ('iand', (invert[left], a, b), (invert[right], c, d))))
   optimizations.append((('inot', ('iand(is_used_once)', (left + '(is_used_once)', a, b),
                                                         (right + '(is_used_once)', c, d))),
                         ('ior', (invert[left], a, b), (invert[right], c, d))))

# Optimize x2yN(b2x(x)) -> b2y
for x, y in itertools.product(['f', 'u', 'i'], ['f', 'u', 'i']):
   if x != 'f' and y != 'f' and x != y:
      continue

   b2x = 'b2f' if x == 'f' else 'b2i'
   b2y = 'b2f' if y == 'f' else 'b2i'
   x2yN = '{}2{}'.format(x, y)
   optimizations.append(((x2yN, (b2x, a)), (b2y, a)))

optimizations += [
   (('f2f16_rtz', ('b2f', a)), ('b2f16', a)),
   (('f2f16_rtne', ('b2f', a)), ('b2f16', a)),
]

# Optimize away x2xN(a@N)
for t in ['int', 'uint', 'float', 'bool']:
   for N in type_sizes(t):
      x2xN = '{0}2{0}{1}'.format(t[0], N)
      aN = 'a@{0}'.format(N)
      optimizations.append(((x2xN, aN), a))

# Optimize x2xN(y2yM(a@P)) -> y2yN(a) for integers
# In particular, we can optimize away everything except upcast of downcast and
# upcasts where the type differs from the other cast
for N, M in itertools.product(type_sizes('uint'), type_sizes('uint')):
   if N < M:
      # The outer cast is a down-cast.  It doesn't matter what the size of the
      # argument of the inner cast is because we'll never been in the upcast
      # of downcast case.  Regardless of types, we'll always end up with y2yN
      # in the end.
      for x, y in itertools.product(['i', 'u'], ['i', 'u']):
         x2xN = '{0}2{0}{1}'.format(x, N)
         y2yM = '{0}2{0}{1}'.format(y, M)
         y2yN = '{0}2{0}{1}'.format(y, N)
         optimizations.append(((x2xN, (y2yM, a)), (y2yN, a)))
   elif N > M:
      # If the outer cast is an up-cast, we have to be more careful about the
      # size of the argument of the inner cast and with types.  In this case,
      # the type is always the type of type up-cast which is given by the
      # outer cast.
      for P in type_sizes('uint'):
         # We can't optimize away up-cast of down-cast.
         if M < P:
            continue

         # Because we're doing down-cast of down-cast, the types always have
         # to match between the two casts
         for x in ['i', 'u']:
            x2xN = '{0}2{0}{1}'.format(x, N)
            x2xM = '{0}2{0}{1}'.format(x, M)
            aP = 'a@{0}'.format(P)
            optimizations.append(((x2xN, (x2xM, aP)), (x2xN, a)))
   else:
      # The N == M case is handled by other optimizations
      pass

# Downcast operations should be able to see through pack
for t in ['i', 'u']:
    for N in [8, 16, 32]:
        x2xN = '{0}2{0}{1}'.format(t, N)
        optimizations += [
            ((x2xN, ('pack_64_2x32_split', a, b)), (x2xN, a)),
            ((x2xN, ('pack_64_2x32_split', a, b)), (x2xN, a)),
        ]

# Optimize comparisons with up-casts
for t in ['int', 'uint', 'float']:
    for N, M in itertools.product(type_sizes(t), repeat=2):
        if N == 1 or N >= M:
            continue

        cond = 'true'
        if N == 8:
            cond = 'options->support_8bit_alu'
        elif N == 16:
            cond = 'options->support_16bit_alu'
        x2xM = '{0}2{0}{1}'.format(t[0], M)
        x2xN = '{0}2{0}{1}'.format(t[0], N)
        aN = 'a@' + str(N)
        bN = 'b@' + str(N)
        xeq = 'feq' if t == 'float' else 'ieq'
        xne = 'fneu' if t == 'float' else 'ine'
        xge = '{0}ge'.format(t[0])
        xlt = '{0}lt'.format(t[0])

        # Up-casts are lossless so for correctly signed comparisons of
        # up-casted values we can do the comparison at the largest of the two
        # original sizes and drop one or both of the casts.  (We have
        # optimizations to drop the no-op casts which this may generate.)
        for P in type_sizes(t):
            if P == 1 or P > N:
                continue

            bP = 'b@' + str(P)
            optimizations += [
                ((xeq, (x2xM, aN), (x2xM, bP)), (xeq, a, (x2xN, b)), cond),
                ((xne, (x2xM, aN), (x2xM, bP)), (xne, a, (x2xN, b)), cond),
                ((xge, (x2xM, aN), (x2xM, bP)), (xge, a, (x2xN, b)), cond),
                ((xlt, (x2xM, aN), (x2xM, bP)), (xlt, a, (x2xN, b)), cond),
                ((xge, (x2xM, bP), (x2xM, aN)), (xge, (x2xN, b), a), cond),
                ((xlt, (x2xM, bP), (x2xM, aN)), (xlt, (x2xN, b), a), cond),
            ]

        # The next bit doesn't work on floats because the range checks would
        # get way too complicated.
        if t in ['int', 'uint']:
            if t == 'int':
                xN_min = -(1 << (N - 1))
                xN_max = (1 << (N - 1)) - 1
            elif t == 'uint':
                xN_min = 0
                xN_max = (1 << N) - 1
            else:
                assert False

            # If we're up-casting and comparing to a constant, we can unfold
            # the comparison into a comparison with the shrunk down constant
            # and a check that the constant fits in the smaller bit size.
            optimizations += [
                ((xeq, (x2xM, aN), '#b'),
                 ('iand', (xeq, a, (x2xN, b)), (xeq, (x2xM, (x2xN, b)), b)), cond),
                ((xne, (x2xM, aN), '#b'),
                 ('ior', (xne, a, (x2xN, b)), (xne, (x2xM, (x2xN, b)), b)), cond),
                ((xlt, (x2xM, aN), '#b'),
                 ('iand', (xlt, xN_min, b),
                          ('ior', (xlt, xN_max, b), (xlt, a, (x2xN, b)))), cond),
                ((xlt, '#a', (x2xM, bN)),
                 ('iand', (xlt, a, xN_max),
                          ('ior', (xlt, a, xN_min), (xlt, (x2xN, a), b))), cond),
                ((xge, (x2xM, aN), '#b'),
                 ('iand', (xge, xN_max, b),
                          ('ior', (xge, xN_min, b), (xge, a, (x2xN, b)))), cond),
                ((xge, '#a', (x2xM, bN)),
                 ('iand', (xge, a, xN_min),
                          ('ior', (xge, a, xN_max), (xge, (x2xN, a), b))), cond),
            ]

# Convert masking followed by signed downcast to just unsigned downcast
optimizations += [
    (('i2i32', ('iand', 'a@64', 0xffffffff)), ('u2u32', a)),
    (('i2i16', ('iand', 'a@32', 0xffff)), ('u2u16', a)),
    (('i2i16', ('iand', 'a@64', 0xffff)), ('u2u16', a)),
    (('i2i8', ('iand', 'a@16', 0xff)), ('u2u8', a)),
    (('i2i8', ('iand', 'a@32', 0xff)), ('u2u8', a)),
    (('i2i8', ('iand', 'a@64', 0xff)), ('u2u8', a)),
]

# Optimize fmin/fmax with constant followed by conversion to 16bit
# Assume 16bit fmin/fmax is faster.
for f2f16 in ['f2f16', 'f2f16_rtz', 'f2f16_rtne']:
   optimizations += [
      ((f2f16, ('fmax(is_used_once)', a, '#b')), ('fmax', (f2f16, a), (f2f16, b)), 'options->support_16bit_alu'),
      ((f2f16, ('fmin(is_used_once)', a, '#b')), ('fmin', (f2f16, a), (f2f16, b)), 'options->support_16bit_alu'),
      ((f2f16, ('vec2(is_used_once)', ('fmax(is_used_once)', a, '#b'), ('fmax(is_used_once)', c, '#d'))),
      ('fmax', (f2f16, ('vec2', a, c)), (f2f16, ('vec2', b, d))), 'options->support_16bit_alu'),
      ((f2f16, ('vec2(is_used_once)', ('fmin(is_used_once)', a, '#b'), ('fmin(is_used_once)', c, '#d'))),
      ('fmin', (f2f16, ('vec2', a, c)), (f2f16, ('vec2', b, d))), 'options->support_16bit_alu'),

      ((f2f16, ('f2f32', 'a@16')), ('fcanonicalize', a)),
      ((f2f16, ('bcsel', a, '#b', c)), ('bcsel', a, (f2f16, b), (f2f16, c)), 'options->support_16bit_alu'),
      ((f2f16, ('bcsel', a, b, '#c')), ('bcsel', a, (f2f16, b), (f2f16, c)), 'options->support_16bit_alu'),
   ]

optimizations += [
   (('pack_half_2x16_rtz_split', ('fmax(is_used_once)', a, '#b'), ('fmax(is_used_once)', c, '#d')),
    ('pack_32_2x16', ('fmax', ('unpack_32_2x16', ('pack_half_2x16_rtz_split', a, c)), ('unpack_32_2x16', ('pack_half_2x16_rtz_split', b, d)))),
    'options->vectorize_vec2_16bit'),
   (('pack_half_2x16_rtz_split', ('fmin(is_used_once)', a, '#b'), ('fmin(is_used_once)', c, '#d')),
    ('pack_32_2x16', ('fmin', ('unpack_32_2x16', ('pack_half_2x16_rtz_split', a, c)), ('unpack_32_2x16', ('pack_half_2x16_rtz_split', b, d)))),
    'options->vectorize_vec2_16bit'),
   (('pack_half_2x16_rtz_split', ('fneg(is_used_once)', ('fmax(is_used_once)', a, '#b')), ('fneg(is_used_once)', ('fmax(is_used_once)', c, '#d'))),
    ('pack_32_2x16', ('fmin', ('fneg', ('unpack_32_2x16', ('pack_half_2x16_rtz_split', a, c))), ('fneg', ('unpack_32_2x16', ('pack_half_2x16_rtz_split', b, d))))),
    'options->vectorize_vec2_16bit'),
   (('pack_half_2x16_rtz_split', ('fneg(is_used_once)', ('fmin(is_used_once)', a, '#b')), ('fneg(is_used_once)', ('fmin(is_used_once)', c, '#d'))),
    ('pack_32_2x16', ('fmax', ('fneg', ('unpack_32_2x16', ('pack_half_2x16_rtz_split', a, c))), ('fneg', ('unpack_32_2x16', ('pack_half_2x16_rtz_split', b, d))))),
    'options->vectorize_vec2_16bit'),

   (('pack_half_2x16_rtz_split', ('bcsel(is_used_once)', a, b, '#c'), ('bcsel(is_used_once)', a, d, '#e')),
    ('bcsel', a, ('pack_half_2x16_rtz_split', b, d), ('pack_half_2x16_rtz_split', c, e))),
   (('pack_half_2x16_rtz_split', ('bcsel(is_used_once)', a, '#b', c), ('bcsel(is_used_once)', a, '#d', e)),
    ('bcsel', a, ('pack_half_2x16_rtz_split', b, d), ('pack_half_2x16_rtz_split', c, e))),

   (('pack_half_2x16_rtz_split', ('b2f', 'a@1'), ('b2f', a)), ('bcsel', a, 0x3c003c00, 0)),

   (('pack_half_2x16_rtz_split', ('f2f32', 'a@16'), b), ('pack_32_2x16', ('vec2', ('fcanonicalize', a), ('f2f16_rtz', b)))),
   (('pack_half_2x16_rtz_split', a, ('f2f32', 'b@16')), ('pack_32_2x16', ('vec2', ('f2f16_rtz', a), ('fcanonicalize', b)))),

   (('pack_32_2x16_split', 'a(is_undef)', ('bcsel', b, '#c', d)),
    ('bcsel', b, ('pack_32_2x16_split', 0, c), ('pack_32_2x16_split', a, d)),
    'true', TestStatus.UNSUPPORTED),
   (('pack_32_2x16_split', 'a(is_undef)', ('bcsel', b, c, '#d')),
    ('bcsel', b, ('pack_32_2x16_split', a, c), ('pack_32_2x16_split', 0, d)),
    'true', TestStatus.UNSUPPORTED),
]

# Some operations such as iadd have the property that the bottom N bits of the
# output only depends on the bottom N bits of each of the inputs so we can
# remove casts
for N in [16, 32]:
    for M in [8, 16]:
        if M >= N:
            continue

        aN = 'a@' + str(N)
        u2uM = 'u2u{0}'.format(M)
        i2iM = 'i2i{0}'.format(M)

        for x in ['u', 'i']:
            x2xN = '{0}2{0}{1}'.format(x, N)
            extract_xM = 'extract_{0}{1}'.format(x, M)

            x2xN_M_bits = '{0}(only_lower_{1}_bits_used)'.format(x2xN, M)
            extract_xM_M_bits = \
                '{0}(only_lower_{1}_bits_used)'.format(extract_xM, M)
            optimizations += [
                ((x2xN_M_bits, (u2uM, aN)), a),
                ((extract_xM_M_bits, aN, 0), a),
            ]

            bcsel_M_bits = 'bcsel(only_lower_{0}_bits_used)'.format(M)
            optimizations += [
                ((bcsel_M_bits, c, (x2xN, (u2uM, aN)), b), ('bcsel', c, a, b)),
                ((bcsel_M_bits, c, (x2xN, (i2iM, aN)), b), ('bcsel', c, a, b)),
                ((bcsel_M_bits, c, (extract_xM, aN, 0), b), ('bcsel', c, a, b)),
            ]

            for op in ['iadd', 'imul', 'iand', 'ior', 'ixor']:
                op_M_bits = '{0}(only_lower_{1}_bits_used)'.format(op, M)
                optimizations += [
                    ((op_M_bits, (x2xN, (u2uM, aN)), b), (op, a, b)),
                    ((op_M_bits, (x2xN, (i2iM, aN)), b), (op, a, b)),
                    ((op_M_bits, (extract_xM, aN, 0), b), (op, a, b)),
                ]

def fexp2i(exp, bits):
   # Generate an expression which constructs value 2.0^exp or 0.0.
   #
   # We assume that exp is already in a valid range:
   #
   #   * [-15, 15] for 16-bit float
   #   * [-127, 127] for 32-bit float
   #   * [-1023, 1023] for 16-bit float
   #
   # If exp is the lowest value in the valid range, a value of 0.0 is
   # constructed.  Otherwise, the value 2.0^exp is constructed.
   if bits == 16:
      return ('i2i16', ('ishl', ('iadd', exp, 15), 10))
   elif bits == 32:
      return ('ishl', ('iadd', exp, 127), 23)
   elif bits == 64:
      return ('pack_64_2x32_split', 0, ('ishl', ('iadd', exp, 1023), 20))
   else:
      assert False

def ldexp(f, exp, bits):
   # The maximum possible range for a normal exponent is [-126, 127] and,
   # throwing in denormals, you get a maximum range of [-149, 127].  This
   # means that we can potentially have a swing of +-276.  If you start with
   # FLT_MAX, you actually have to do ldexp(FLT_MAX, -278) to get it to flush
   # all the way to zero.  The GLSL spec only requires that we handle a subset
   # of this range.  From version 4.60 of the spec:
   #
   #    "If exp is greater than +128 (single-precision) or +1024
   #    (double-precision), the value returned is undefined. If exp is less
   #    than -126 (single-precision) or -1022 (double-precision), the value
   #    returned may be flushed to zero. Additionally, splitting the value
   #    into a significand and exponent using frexp() and then reconstructing
   #    a floating-point value using ldexp() should yield the original input
   #    for zero and all finite non-denormalized values."
   #
   # The SPIR-V spec has similar language.
   #
   # In order to handle the maximum value +128 using the fexp2i() helper
   # above, we have to split the exponent in half and do two multiply
   # operations.
   #
   # First, we clamp exp to a reasonable range.  Specifically, we clamp to
   # twice the full range that is valid for the fexp2i() function above.  If
   # exp/2 is the bottom value of that range, the fexp2i() expression will
   # yield 0.0f which, when multiplied by f, will flush it to zero which is
   # allowed by the GLSL and SPIR-V specs for low exponent values.  If the
   # value is clamped from above, then it must have been above the supported
   # range of the GLSL built-in and therefore any return value is acceptable.
   if bits == 16:
      exp = ('imin', ('imax', exp, -30), 30)
   elif bits == 32:
      exp = ('imin', ('imax', exp, -254), 254)
   elif bits == 64:
      exp = ('imin', ('imax', exp, -2046), 2046)
   else:
      assert False

   # Now we compute two powers of 2, one for exp/2 and one for exp-exp/2.
   # (We use ishr which isn't the same for -1, but the -1 case still works
   # since we use exp-exp/2 as the second exponent.)  While the spec
   # technically defines ldexp as f * 2.0^exp, simply multiplying once doesn't
   # work with denormals and doesn't allow for the full swing in exponents
   # that you can get with normalized values.  Instead, we create two powers
   # of two and multiply by them each in turn.  That way the effective range
   # of our exponent is doubled.
   pow2_1 = fexp2i(('ishr', exp, 1), bits)
   pow2_2 = fexp2i(('isub', exp, ('ishr', exp, 1)), bits)
   return ('!fmul', ('!fmul', f, pow2_1), pow2_2)

optimizations += [
   (('ldexp@16', 'x', 'exp'), ldexp('x', 'exp', 16), '!options->has_ldexp'),
   (('ldexp@32', 'x', 'exp'), ldexp('x', 'exp', 32), '!options->has_ldexp'),
   (('ldexp@64', 'x', 'exp'), ldexp('x', 'exp', 64), '!options->has_ldexp'),
   (('fexp2(contract)', ('i2f', 'a@8')), ('ldexp', 1.0, ('i2i32', a)), 'options->has_ldexp'),
   (('fexp2(contract)', ('i2f', 'a@16')), ('ldexp', 1.0, ('i2i32', a)), 'options->has_ldexp'),
   (('fexp2(contract)', ('i2f', 'a@32')), ('ldexp', 1.0, a), 'options->has_ldexp'),
   (('fexp2(contract,ninf)', ('u2f', a)), ('ldexp', 1.0, ('u2u32', a)), 'options->has_ldexp'),
]

# XCOM 2 (OpenGL) open-codes bitfieldReverse()
def bitfield_reverse_xcom2(u):
    step1 = ('iadd', ('ishl', u, 16), ('ushr', u, 16))
    step2 = ('iadd', ('iand', ('ishl', step1, 1), 0xaaaaaaaa), ('iand', ('ushr', step1, 1), 0x55555555))
    step3 = ('iadd', ('iand', ('ishl', step2, 2), 0xcccccccc), ('iand', ('ushr', step2, 2), 0x33333333))
    step4 = ('iadd', ('iand', ('ishl', step3, 4), 0xf0f0f0f0), ('iand', ('ushr', step3, 4), 0x0f0f0f0f))
    step5 = ('iadd(many-comm-expr)', ('iand', ('ishl', step4, 8), 0xff00ff00), ('iand', ('ushr', step4, 8), 0x00ff00ff))

    return step5

# Unreal Engine 4 demo applications open-codes bitfieldReverse()
def bitfield_reverse_ue4(u):
    step1 = ('ior', ('ishl', u, 16), ('ushr', u, 16))
    step2 = ('ior', ('ishl', ('iand', step1, 0x00ff00ff), 8), ('ushr', ('iand', step1, 0xff00ff00), 8))
    step3 = ('ior', ('ishl', ('iand', step2, 0x0f0f0f0f), 4), ('ushr', ('iand', step2, 0xf0f0f0f0), 4))
    step4 = ('ior', ('ishl', ('iand', step3, 0x33333333), 2), ('ushr', ('iand', step3, 0xcccccccc), 2))
    step5 = ('ior(many-comm-expr)', ('ishl', ('iand', step4, 0x55555555), 1), ('ushr', ('iand', step4, 0xaaaaaaaa), 1))

    return step5

# Cyberpunk 2077 open-codes bitfieldReverse()
def bitfield_reverse_cp2077(u):
    step1 = ('ior', ('ishl', u, 16), ('ushr', u, 16))
    step2 = ('ior', ('iand', ('ishl', step1, 1), 0xaaaaaaaa), ('iand', ('ushr', step1, 1), 0x55555555))
    step3 = ('ior', ('iand', ('ishl', step2, 2), 0xcccccccc), ('iand', ('ushr', step2, 2), 0x33333333))
    step4 = ('ior', ('iand', ('ishl', step3, 4), 0xf0f0f0f0), ('iand', ('ushr', step3, 4), 0x0f0f0f0f))
    step5 = ('ior(many-comm-expr)', ('iand', ('ishl', step4, 8), 0xff00ff00), ('iand', ('ushr', step4, 8), 0x00ff00ff))

    return step5

optimizations += [(bitfield_reverse_xcom2('x@32'), ('bitfield_reverse', 'x'), '!options->lower_bitfield_reverse')]
optimizations += [(bitfield_reverse_ue4('x@32'), ('bitfield_reverse', 'x'), '!options->lower_bitfield_reverse')]
optimizations += [(bitfield_reverse_cp2077('x@32'), ('bitfield_reverse', 'x'), '!options->lower_bitfield_reverse')]

# VKD3D-Proton DXBC f32 to f16 conversion implements a float conversion using PackHalf2x16.
# Because the spec does not specify a rounding mode or behaviour regarding infinity,
# it emits a sequence to ensure D3D-like behaviour for infinity.
# When we know the current backend already behaves like we need, we can eliminate the extra sequence.
#
# Input is f32, output is u32 that has the f16 packed into its low bits.
def vkd3d_proton_packed_f2f16_rtz_lo(a, abs_a):
    packed_half = ('pack_half_2x16_rtz_split', a, 0)
    f32_was_not_inf = ('fneu', abs_a, 0x7f800000)
    f16_is_now_inf = ('ieq', ('iand', packed_half, 0x7fff), 0x7c00)
    return ('iadd', packed_half, ('ineg', ('b2i', ('iand', f32_was_not_inf, f16_is_now_inf))))

optimizations += [
   (vkd3d_proton_packed_f2f16_rtz_lo('x', ('fabs', 'x')), ('pack_half_2x16_rtz_split', 'x', 0)),
   (vkd3d_proton_packed_f2f16_rtz_lo('x(is_not_negative)', 'x'), ('pack_half_2x16_rtz_split', 'x', 0)),
   (vkd3d_proton_packed_f2f16_rtz_lo(('fneg', 'x'), ('fabs', 'x')), ('pack_half_2x16_rtz_split', ('fneg', 'x'), 0)),
]

def vkd3d_proton_msad():
   pattern = None
   for i in range(4):
      ref = ('extract_u8', 'a@32', i)
      src = ('extract_u8', 'b@32', i)
      sad = ('iabs', ('iadd', ref, ('ineg', src)))
      msad = ('bcsel', ('ieq', ref, 0), 0, sad)
      if pattern == None:
         pattern = msad
      else:
         pattern = ('iadd', pattern, msad)
   pattern = (pattern[0] + '(many-comm-expr)', *pattern[1:])
   return pattern

optimizations += [
   (vkd3d_proton_msad(), ('msad_4x8', a, b, 0), 'options->has_msad'),
   (('iadd', ('msad_4x8', a, b, 0), c), ('msad_4x8', a, b, c)),
]

# VKD3D-Proton patterns for FP16_OVFL=1 conversion to e4m3fn
def vkd3d_proton_f2e4m3_ovfl(variant, x, nan):
   if variant == 0:
      cond = ('feq', ('fabs', x), float('inf'))
   elif variant == 1:
      cond = ('feq', f'{x}(is_not_negative)', float('inf'))
   elif variant == 2:
      cond = ('feq', f'{x}(is_not_positive)', -float('inf'))

   return ('bcsel', cond, f'#{nan}(is_nan)', x)


for var in range(3):
   optimizations += [
      (('f2e4m3fn_sat', vkd3d_proton_f2e4m3_ovfl(var, a, b)),
       ('f2e4m3fn_satfn', a), 'options->has_f2e4m3fn_satfn', TestStatus.UNSUPPORTED), # All inputs skipped
   ]

for var0, var1 in itertools.product(range(3), repeat=2):
   optimizations += [
      (('f2e4m3fn_sat', ('vec2', vkd3d_proton_f2e4m3_ovfl(var0, a, b),
                                 vkd3d_proton_f2e4m3_ovfl(var1, c, d))),
       ('f2e4m3fn_satfn', ('vec2', a, c)), 'options->has_f2e4m3fn_satfn', TestStatus.UNSUPPORTED), # All inputs skipped
   ]

# "all_equal(eq(a, b), vec(~0))" is the same as "all_equal(a, b)"
# "any_nequal(neq(a, b), vec(0))" is the same as "any_nequal(a, b)"
for ncomp in [2, 3, 4, 8, 16]:
   # The test suite rejects the tests for the >=4 components, because they're just too slow to run
   status = TestStatus.UNSUPPORTED if ncomp >= 4 else TestStatus.PASS
   optimizations += [
      (('ball_iequal' + str(ncomp), ('ieq', a, b), ~0), ('ball_iequal' + str(ncomp), a, b), 'true', status),
      (('ball_iequal' + str(ncomp), ('feq', a, b), ~0), ('ball_fequal' + str(ncomp), a, b), 'true', status),
      (('bany_inequal' + str(ncomp), ('ine', a, b), 0), ('bany_inequal' + str(ncomp), a, b), 'true', status),
      (('bany_inequal' + str(ncomp), ('fneu', a, b), 0), ('bany_fnequal' + str(ncomp), a, b), 'true', status),
   ]

# For any float comparison operation, "cmp", if you have "a == a && a cmp b"
# then the "a == a" is redundant because it's equivalent to "a is not NaN"
# and, if a is a NaN then the second comparison will fail anyway.
for op in ['flt', 'fge', 'feq']:
   optimizations += [
      (('iand', ('feq', a, a), (op, a, b)), ('!' + op, a, b)),
      (('iand', ('feq', a, a), (op, b, a)), ('!' + op, b, a)),
   ]

# For example, this converts things like
#
#    1 + mix(0, a - 1, condition)
#
# into
#
#    mix(1, (a-1)+1, condition)
#
# Other optimizations will rearrange the constants.
for op in ['fadd', 'fmul', 'fmulz', 'iadd', 'imul']:
   optimizations += [
      ((op, ('bcsel(is_used_once)', a, '#b', c), '#d'), ('bcsel', a, (op, b, d), (op, c, d)))
   ]

# Some optimizations for ir3-specific instructions.
optimizations += [
   # 'al * bl': If either 'al' or 'bl' is zero, return zero.
   (('umul_16x16', '#a(is_lower_half_zero)', 'b'), (0)),
   # '(al * bh) << 16 + c': If either 'al' or 'bh' is zero, return 'c'.
   (('imadsh_mix16', '#a@32(is_lower_half_zero)', 'b@32', 'c@32'), ('c')),
   (('imadsh_mix16', 'a@32', '#b@32(is_upper_half_zero)', 'c@32'), ('c')),
]

# These kinds of sequences can occur after nir_opt_peephole_select.
#
# NOTE: fadd is not handled here because that gets in the way of ffma
# generation in the i965 driver.  Instead, fadd and ffma are handled in
# late_optimizations.

for op in ['flrp']:
    optimizations += [
        (('bcsel', a, (op + '(is_used_once)', b, c, d), (op, b, c, e)), (op, b, c, ('bcsel', a, d, e))),
        (('bcsel', a, (op, b, c, d), (op + '(is_used_once)', b, c, e)), (op, b, c, ('bcsel', a, d, e))),
        (('bcsel', a, (op + '(is_used_once)', b, c, d), (op, b, e, d)), (op, b, ('bcsel', a, c, e), d)),
        (('bcsel', a, (op, b, c, d), (op + '(is_used_once)', b, e, d)), (op, b, ('bcsel', a, c, e), d)),
        (('bcsel', a, (op + '(is_used_once)', b, c, d), (op, e, c, d)), (op, ('bcsel', a, b, e), c, d)),
        (('bcsel', a, (op, b, c, d), (op + '(is_used_once)', e, c, d)), (op, ('bcsel', a, b, e), c, d)),
    ]

for op in ['fmulz', 'fmul', 'iadd', 'imul', 'iand', 'ior', 'ixor', 'fmin', 'fmax', 'imin', 'imax', 'umin', 'umax']:
    optimizations += [
        (('bcsel', a, (op + '(is_used_once)', b, c), (op, b, 'd(is_not_const)')), (op, b, ('bcsel', a, c, d))),
        (('bcsel', a, (op + '(is_used_once)', b, 'c(is_not_const)'), (op, b, d)), (op, b, ('bcsel', a, c, d))),
        (('bcsel', a, (op, b, 'c(is_not_const)'), (op + '(is_used_once)', b, d)), (op, b, ('bcsel', a, c, d))),
        (('bcsel', a, (op, b, c), (op + '(is_used_once)', b, 'd(is_not_const)')), (op, b, ('bcsel', a, c, d))),
    ]

for op in ['fpow']:
    optimizations += [
        (('bcsel', a, (op + '(is_used_once)', b, c), (op, b, d)), (op, b, ('bcsel', a, c, d))),
        (('bcsel', a, (op, b, c), (op + '(is_used_once)', b, d)), (op, b, ('bcsel', a, c, d))),
        (('bcsel', a, (op + '(is_used_once)', b, c), (op, d, c)), (op, ('bcsel', a, b, d), c)),
        (('bcsel', a, (op, b, c), (op + '(is_used_once)', d, c)), (op, ('bcsel', a, b, d), c)),
    ]

for op in ['frcp', 'frsq', 'fsqrt', 'fexp2', 'flog2', 'fsign', 'fsin', 'fcos', 'fsin_normalized_2_pi', 'fcos_normalized_2_pi', 'fsin_mdg', 'fcos_mdg', 'fsin_agx', 'fneg', 'fabs', 'fsign', 'fcanonicalize']:
    optimizations += [
        (('bcsel', c, (op + '(is_used_once)', a), (op + '(is_used_once)', b)), (op, ('bcsel', c, a, b))),
    ]

for op in ['ineg', 'iabs', 'inot', 'isign', 'fcanonicalize']:
    optimizations += [
        ((op, ('bcsel', c, '#a', '#b')), ('bcsel', c, (op, a), (op, b))),
    ]

optimizations.extend([
    (('fisnormal', 'a@16'), ('ult', 0xfff, ('iadd', ('ishl', a, 1), 0x800)), 'options->lower_fisnormal'),
    (('fisnormal', 'a@32'), ('ult', 0x1ffffff, ('iadd', ('ishl', a, 1), 0x1000000)), 'options->lower_fisnormal'),
    (('fisnormal', 'a@64'), ('ult', 0x3fffffffffffff, ('iadd', ('ishl', a, 1), 0x20000000000000)), 'options->lower_fisnormal')
    ])


"""
  if (fabs(val) < SMALLEST_NORMALIZED_FLOAT16)
     return (val & SIGN_BIT) /* +0.0 or -0.0 as appropriate */;
  else
     return f2f32(f2f16(val));
"""
optimizations.extend([
    (('fquantize2f16', 'a@32'),
     ('bcsel', ('flt(preserve_nan_inf)', ('fabs(preserve_nan_inf)', a), math.ldexp(1.0, -14)),
               ('iand', a, 1 << 31),
               ('!f2f32', ('!f2f16_rtne', a))),
     'options->lower_fquantize2f16'),
    ])

for s in range(0, 31):
    mask = 0xffffffff << s

    # bfi is ((mask & ...) | (~mask & ...)). Since the two sources of the ior
    # will never both have the same bits set, replacing the ior with an iadd
    # is safe (i.e., a carry out of a bit can never be generated). The iadd is
    # more likely to participate in other optimization patterns (e.g., iadd of
    # constant reassociation)
    optimizations.extend([
        (('bfi', mask, a, '#b'), ('iadd', ('ishl', a, s), ('iand', b, ~mask)),
         'options->avoid_ternary_with_two_constants'),
    ])

# NaN propagation: Binary opcodes. If any operand is NaN, replace it with NaN.
# (unary opcodes with NaN are evaluated by nir_opt_constant_folding, not here)
for op in ['fadd', 'fdiv', 'fmod', 'fmul', 'fpow', 'frem', 'fsub']:
    optimizations += [((op, '#a(is_nan)', b), NAN, 'true', TestStatus.XFAIL if op == 'fpow' else TestStatus.PASS)] # XFAIL is fpow(NaN, 0.0) producing NaN instead of 1.0.
    optimizations += [((op, a, '#b(is_nan)'), NAN, 'true', TestStatus.XFAIL if op == 'fpow' else TestStatus.PASS)] # some opcodes are not commutative.  XFAIL is fpow(1.0, NaN) producing NaN instead of 1.0.

# NaN propagation: Trinary opcodes. If any operand is NaN, replace it with NaN.
for op in ['ffma', 'flrp']:
    optimizations += [((op, '#a(is_nan)', b, c), NAN)]
    optimizations += [((op, a, '#b(is_nan)', c), NAN)] # some opcodes are not commutative
    optimizations += [((op, a, b, '#c(is_nan)'), NAN)]

# NaN propagation: FP min/max. Pick the non-NaN operand.
for op in ['fmin', 'fmax']:
    optimizations += [((op, '#a(is_nan)', b), b)] # commutative

# NaN propagation: ldexp is NaN if the first operand is NaN.
optimizations += [(('ldexp', '#a(is_nan)', b), NAN, 'true', TestStatus.XFAIL)] # XFAIL is ldexp(NaN, 0.0) producing NaN instead of 0.0.

# NaN propagation: Dot opcodes. If any component is NaN, replace it with NaN.
for op in ['fdot2', 'fdot3', 'fdot4', 'fdot5', 'fdot8', 'fdot16']:
    optimizations += [((op, '#a(is_any_comp_nan)', b), NAN)] # commutative

# NaN propagation: FP comparison opcodes except !=. Replace it with false.
for op in ['feq', 'fge', 'flt']:
    optimizations += [((op, '#a(is_nan)', b), False)]
    optimizations += [((op, a, '#b(is_nan)'), False)] # some opcodes are not commutative

# NaN propagation: FP comparison opcodes using !=. Replace it with true.
# Operator != is the only opcode where a comparison with NaN returns true.
for op in ['fneu']:
    optimizations += [((op, '#a(is_nan)', b), True)] # commutative

# NaN propagation: FP comparison opcodes except != returning FP 0 or 1.
for op in ['seq', 'sge', 'slt']:
    optimizations += [((op, '#a(is_nan)', b), 0.0)]
    optimizations += [((op, a, '#b(is_nan)'), 0.0)] # some opcodes are not commutative

# NaN propagation: FP comparison opcodes using != returning FP 0 or 1.
# Operator != is the only opcode where a comparison with NaN returns true.
optimizations += [(('sne', '#a(is_nan)', b), 1.0)] # commutative

# This section contains optimizations to propagate downsizing conversions of
# constructed vectors into vectors of downsized components. Whether this is
# useful depends on the SIMD semantics of the backend. On a true SIMD machine,
# this reduces the register pressure of the vector itself and often enables the
# conversions to be eliminated via other algebraic rules or constant folding.
# In the worst case on a SIMD architecture, the propagated conversions may be
# revectorized via nir_opt_vectorize so instruction count is minimally
# impacted.
#
# On a machine with SIMD-within-a-register only, this actually
# counterintuitively hurts instruction count. These machines are the same that
# require vectorize_vec2_16bit, so we predicate the optimizations on that flag
# not being set.
#
# Finally for scalar architectures, there should be no difference in generated
# code since it all ends up scalarized at the end, but it might minimally help
# compile-times.

for i in range(2, 4 + 1):
   for T in ('f', 'u', 'i'):
      vec_inst = ('vec' + str(i),)

      indices = ['a', 'b', 'c', 'd']
      suffix_in = tuple((indices[j] + '@32') for j in range(i))

      to_16 = '{}2{}16'.format(T, T)
      to_mp = '{}2{}mp'.format(T, T)

      out_16 = tuple((to_16, indices[j]) for j in range(i))
      out_mp = tuple((to_mp, indices[j]) for j in range(i))

      optimizations  += [
         ((to_16, vec_inst + suffix_in), vec_inst + out_16, '!options->vectorize_vec2_16bit'),
      ]
      # u2ump doesn't exist, because it's equal to i2imp
      if T in ['f', 'i']:
          optimizations  += [
              ((to_mp, vec_inst + suffix_in), vec_inst + out_mp,
               '!options->vectorize_vec2_16bit', TestStatus.UNSUPPORTED)
          ]

# This section contains "late" optimizations that should be run before
# creating ffma and calling regular optimizations for the final time.
# Optimizations should go here if they help code generation and conflict
# with the regular optimizations.
before_ffma_optimizations = [
   # Propagate constants down multiplication chains
   (('~fmul', ('fmul(is_used_once)', 'a(is_not_const)', '#b'), 'c(is_not_const)'), ('fmul', ('fmul', a, c), b)),
   (('imul', ('imul(is_used_once)', 'a(is_not_const)', '#b'), 'c(is_not_const)'), ('imul', ('imul', a, c), b)),
   (('~fadd', ('fadd(is_used_once)', 'a(is_not_const)', '#b'), 'c(is_not_const)'), ('fadd', ('fadd', a, c), b)),
   (('iadd', ('iadd(is_used_once)', 'a(is_not_const)', '#b'), 'c(is_not_const)'), ('iadd', ('iadd', a, c), b)),

   (('~fadd', ('fmul', a, b), ('fmul', a, c)), ('fmul', a, ('fadd', b, c))),
   (('iadd', ('imul', a, b), ('imul', a, c)), ('imul', a, ('iadd', b, c))),
   (('~fadd', ('fneg', a), a), 0.0),
   (('iadd', ('ineg', a), a), 0),
   (('iadd', ('ineg', a), ('iadd', a, b)), b),
   (('iadd', a, ('iadd', ('ineg', a), b)), b),
   (('~fadd', ('fneg', a), ('fadd', a, b)), ('fcanonicalize', b)),
   (('~fadd', a, ('fadd', ('fneg', a), b)), ('fcanonicalize', b)),

   (('~flrp', ('fadd(is_used_once)', a, -1.0), ('fadd(is_used_once)', a,  1.0), d), ('fadd', ('flrp', -1.0,  1.0, d), a)),
   (('~flrp', ('fadd(is_used_once)', a,  1.0), ('fadd(is_used_once)', a, -1.0), d), ('fadd', ('flrp',  1.0, -1.0, d), a)),
   (('~flrp', ('fadd(is_used_once)', a, '#b'), ('fadd(is_used_once)', a, '#c'), d), ('fadd', ('fmul', d, ('fadd', c, ('fneg', b))), ('fadd', a, b))),
]

# This section contains "late" optimizations that should be run after the
# regular optimizations have finished.  Optimizations should go here if
# they help code generation but do not necessarily produce code that is
# more easily optimizable.
late_optimizations = [
   # The rearrangements are fine w.r.t. NaN.  However, they produce incorrect
   # results if one operand is +Inf and the other is -Inf.
   #
   # 1. Inf + -Inf = NaN
   # 2. ∀x: x + NaN = NaN and x - NaN = NaN
   # 3. ∀x: x != NaN = true
   # 4. ∀x, ∀ cmp ∈ {<, >, ≤, ≥, =}: x cmp NaN = false
   #
   #               a=Inf, b=-Inf   a=-Inf, b=Inf    a=NaN    b=NaN
   #  (a+b) < 0        false            false       false    false
   #      a < -b       false            false       false    false
   #  (a+b) >= 0       false            false       false    false
   #      a >= -b      true             true        false    false
   #  (a+b) == 0       false            false       false    false
   #      a == -b      true             true        false    false
   #  (a+b) != 0       true             true        true     true
   #      a != -b      false            false       true     true
   #
   #  For ordered/unordered counterparts:
   #
   #               a=Inf, b=-Inf   a=-Inf, b=Inf    a=NaN    b=NaN
   #  (a+b) <u 0       true             true        true     true
   #      a <u -b      false            false       true     true
   #  (a+b) >=u 0      true             true        true     true
   #      a >=u -b     true             true        true     true
   #  (a+b) ==u 0      true             true        true     true
   #      a ==u -b     true             true        true     true
   #  (a+b) !=o 0      false            false       false    false
   #      a !=o -b     false            false       false    false
   (('flt', ('fadd(is_used_once)', a, b),  0.0), ('flt',          a, ('fneg', b))),
   (('flt', 0.0, ('fadd(is_used_once)', a, b) ), ('flt', ('fneg', a),         b)),
   (('fltu', ('fadd(is_used_once,ninf)', a, b),  0.0), ('fltu',          a, ('fneg', b))),
   (('fltu', 0.0, ('fadd(is_used_once,ninf)', a, b) ), ('fltu', ('fneg', a),         b)),
   (('fge', ('fadd(is_used_once,ninf)', a, b),  0.0), ('fge',          a, ('fneg', b))),
   (('fge', 0.0, ('fadd(is_used_once,ninf)', a, b) ), ('fge', ('fneg', a),         b)),
   (('fgeu', ('fadd(is_used_once)', a, b),  0.0), ('fgeu',          a, ('fneg', b))),
   (('fgeu', 0.0, ('fadd(is_used_once)', a, b) ), ('fgeu', ('fneg', a),         b)),
   (('feq', ('fadd(is_used_once,ninf)', a, b), 0.0), ('feq', a, ('fneg', b))),
   (('fequ', ('fadd(is_used_once)', a, b), 0.0), ('fequ', a, ('fneg', b))),
   (('fneo', ('fadd(is_used_once)', a, b), 0.0), ('fneo', a, ('fneg', b))),
   (('fneu', ('fadd(is_used_once,ninf)', a, b), 0.0), ('fneu', a, ('fneg', b))),

   # If either source must be finite, then the original (a+b) cannot produce
   # NaN due to Inf-Inf.  The patterns and the replacements produce the same
   # result if b is NaN. Therefore, the replacements are exact.
   (('fge', ('fadd(is_used_once)', 'a(is_finite)', b),  0.0), ('fge',          a, ('fneg', b))),
   (('fge', 0.0, ('fadd(is_used_once)', 'a(is_finite)', b) ), ('fge', ('fneg', a),         b)),
   (('fltu', ('fadd(is_used_once)', 'a(is_finite)', b),  0.0), ('fltu',          a, ('fneg', b))),
   (('fltu', 0.0, ('fadd(is_used_once)', 'a(is_finite)', b) ), ('fltu', ('fneg', a),         b)),
   (('feq',  ('fadd(is_used_once)', 'a(is_finite)', b), 0.0), ('feq',  a, ('fneg', b))),
   (('fneu', ('fadd(is_used_once)', 'a(is_finite)', b), 0.0), ('fneu', a, ('fneg', b))),

   # This is how SpvOpFOrdNotEqual might be implemented.  Replace it with
   # SpvOpLessOrGreater.
   *add_fabs_fneg((('iand', ('fneu', 'ma', 'mb'), ('iand', ('feq', a, a), ('feq', b, b))), ('ior', ('flt', 'ma', 'mb'), ('flt', 'mb', 'ma'))), {'ma' : a, 'mb' : b}),
   (('iand', ('fneu', a, 0.0), ('feq', a, a)), ('flt', 0.0, ('fabs', a))),

   # This is how SpvOpFUnordEqual might be implemented.  Replace it with
   # !SpvOpLessOrGreater.
   *add_fabs_fneg((('ior', ('feq', 'ma', 'mb'), ('ior', ('fneu', a, a), ('fneu', b, b))), ('inot', ('ior', ('flt', 'ma', 'mb'), ('flt', 'mb', 'ma')))), {'ma' : a, 'mb' : b}),
   (('ior', ('feq', a, 0.0), ('fneu', a, a)), ('inot', ('!flt', 0.0, ('fabs', a)))),

   *add_fabs_fneg((('ior', ('flt', 'ma', 'mb'), ('ior', ('fneu', a, a), ('fneu', b, b))), ('inot', ('fge', 'ma', 'mb'))), {'ma' : a, 'mb' : b}, False),
   *add_fabs_fneg((('ior', ('fge', 'ma', 'mb'), ('ior', ('fneu', a, a), ('fneu', b, b))), ('inot', ('flt', 'ma', 'mb'))), {'ma' : a, 'mb' : b}, False),
   *add_fabs_fneg((('ior', ('flt', 'ma', 'b(is_a_number)'), ('fneu', a, a)), ('inot', ('fge', 'ma', b))), {'ma' : a}),
   *add_fabs_fneg((('ior', ('fge', 'ma', 'b(is_a_number)'), ('fneu', a, a)), ('inot', ('flt', 'ma', b))), {'ma' : a}),
   *add_fabs_fneg((('ior', ('flt', 'a(is_a_number)', 'mb'), ('fneu', b, b)), ('inot', ('fge', a, 'mb'))), {'mb' : b}),
   *add_fabs_fneg((('ior', ('fge', 'a(is_a_number)', 'mb'), ('fneu', b, b)), ('inot', ('flt', a, 'mb'))), {'mb' : b}),
   *add_fabs_fneg((('iand', ('fneu', 'ma', 'b(is_a_number)'), ('feq', a, a)), ('fneo', 'ma', b), 'options->has_fneo_fcmpu'), {'ma' : a}),
   *add_fabs_fneg((('ior', ('feq', 'ma', 'b(is_a_number)'), ('fneu', a, a)), ('fequ', 'ma', b), 'options->has_fneo_fcmpu'), {'ma' : a}),

   (('ior', ('flt', a, b), ('flt', b, a)), ('fneo', a, b), 'options->has_fneo_fcmpu'),
   (('flt', 0.0, ('fabs', a)), ('fneo', 0.0, a), 'options->has_fneo_fcmpu'),


   # These don't interfere with the previous optimizations which include this
   # in the search expression, because nir_algebraic_impl visits instructions
   # in reverse order.
   (('ior', ('fneu', 'a@16', a), ('fneu', 'b@16', b)), ('funord', a, b), 'options->has_ford_funord'),
   (('iand', ('feq', 'a@16', a), ('feq', 'b@16', b)), ('ford', a, b), 'options->has_ford_funord'),
   (('ior', ('fneu', 'a@32', a), ('fneu', 'b@32', b)), ('funord', a, b), 'options->has_ford_funord'),
   (('iand', ('feq', 'a@32', a), ('feq', 'b@32', b)), ('ford', a, b), 'options->has_ford_funord'),
   (('ior', ('fneu', 'a@64', a), ('fneu', 'b@64', b)), ('funord', a, b), 'options->has_ford_funord'),
   (('iand', ('feq', 'a@64', a), ('feq', 'b@64', b)), ('ford', a, b), 'options->has_ford_funord'),

   (('inot', ('ford(is_used_once)', a, b)), ('funord', a, b)),
   (('inot', ('funord(is_used_once)', a, b)), ('ford', a, b)),
   (('inot', ('feq(is_used_once)', a, b)), ('fneu', a, b)),
   (('inot', ('fneu(is_used_once)', a, b)), ('feq', a, b)),
   (('inot', ('fequ(is_used_once)', a, b)), ('fneo', a, b)),
   (('inot', ('fneo(is_used_once)', a, b)), ('fequ', a, b)),
   (('inot', ('flt(is_used_once)', a, b)), ('fgeu', a, b), 'options->has_fneo_fcmpu'),
   (('inot', ('fgeu(is_used_once)', a, b)), ('flt', a, b)),
   (('inot', ('fge(is_used_once)', a, b)), ('fltu', a, b), 'options->has_fneo_fcmpu'),
   (('inot', ('fltu(is_used_once)', a, b)), ('fge', a, b)),

   (('fneg(is_only_used_as_float)', ('fneg', a)), a),

   # combine imul and iadd to imad
   (('iadd@32', ('imul(is_only_used_by_iadd)', a, b), c), ('imad', a, b, c), 'options->has_imad32'),

   # Drivers do not actually implement udiv_aligned_4, it is just used to
   # optimize scratch lowering.
   (('udiv_aligned_4', a), ('ushr', a, 2)),

   # Since b2i(inot(a)) == 1 - b2i(a), 1 - b2i(inot(a)) == b2i(a).
   (('iadd3', a,          ('b2i', ('inot', b)),  -1), ('iadd', a, ('ineg', ('b2i', b)))),
   (('iadd3', a, ('ineg', ('b2i', ('inot', b))),  1), ('iadd', a,          ('b2i', b) )),
   (('iadd@16', a, ('ineg', ('b2i', ('inot', b)))), ('iadd3', a,          ('b2i', b) , -1), 'options->has_iadd3'),
   (('iadd@16', a,          ('b2i', ('inot', b)) ), ('iadd3', a, ('ineg', ('b2i', b)),  1), 'options->has_iadd3'),
   (('iadd@32', a, ('ineg', ('b2i', ('inot', b)))), ('iadd3', a,          ('b2i', b) , -1), 'options->has_iadd3'),
   (('iadd@32', a,          ('b2i', ('inot', b)) ), ('iadd3', a, ('ineg', ('b2i', b)),  1), 'options->has_iadd3'),
   (('iadd@64', a, ('ineg', ('b2i', ('inot', b)))), ('iadd3', a,          ('b2i', b) , -1), 'options->has_iadd3 && !(options->lower_int64_options & nir_lower_iadd3_64)'),
   (('iadd@64', a,          ('b2i', ('inot', b)) ), ('iadd3', a, ('ineg', ('b2i', b)),  1), 'options->has_iadd3 && !(options->lower_int64_options & nir_lower_iadd3_64)'),
]

for int_sz in (8, 16, 32):
    # Note: Python's float is 64-bit, so it should be able to exactly
    # represent these values for upto 32 bits.
    uintmax = float((1 << int_sz) - 1)
    intmax = float((1 << (int_sz - 1)) - 1)
    intmin = float(1 << (int_sz - 1))

    # Don't generate patterns that try to emit saturating conversion from
    # 64-bit float to 8-bit integer. These are generally not supported by any
    # drivers.
    all_float_sizes = (16, 32, 64) if int_sz > 8 else (16, 32)

    for float_sz in all_float_sizes:
        # The floating point type can only precisely represent the signed
        # integer minimum or maximum if it has enough mantissa and exponent
        # bits.
        if float_sz > int_sz:
            late_optimizations.extend([
                # This requires is_a_number because f2i_sat(NaN) is zero, but
                # fmax(intmin, NaN) is intmin.
                ((f'f2i{int_sz}', ('fmax', f'a@{float_sz}(is_a_number)', intmin)), (f'f2i{int_sz}_sat', a), 'options->has_f2i_sat', TestStatus.UNSUPPORTED), # all inputs skipped

                ((f'f2i{int_sz}', ('fmin', f'a@{float_sz}(is_a_number)', intmax)), (f'f2i{int_sz}_sat', a), 'options->has_f2i_sat'),
                ((f'f2u{int_sz}', ('fmin', f'a@{float_sz}(is_a_number)', uintmax)), (f'f2u{int_sz}_sat', a), 'options->has_f2u_sat'),
            ])

        late_optimizations.extend([
            # This does not require is_a_number because both f2u_sat(NaN) and
            # fmax(NaN, 0) are zero.
            ((f'f2u{int_sz}', ('fmax', f'a@{float_sz}', 0.0)), (f'f2u{int_sz}_sat', a), 'options->has_f2u_sat'),

            ((f'f2u{int_sz}', ('ftrunc', f'a@{float_sz}')), (f'f2u{int_sz}_sat', a), 'options->has_f2u_sat'),

            # f2i(NaN) and f2u(NaN) are zero.
            ((f'f2i{int_sz}', ('bcsel', ('feq', f'a@{float_sz}', a), a, 0.0)), (f'f2i{int_sz}_sat', a), 'options->has_f2i_sat'),
            ((f'f2u{int_sz}', ('bcsel', ('feq', f'a@{float_sz}', a), a, 0.0)), (f'f2u{int_sz}_sat', a), 'options->has_f2u_sat'),

            ((f'f2i{int_sz}', ('bcsel', ('fneu', f'a@{float_sz}', a), 0.0, a)), (f'f2i{int_sz}_sat', a), 'options->has_f2i_sat'),
            ((f'f2u{int_sz}', ('bcsel', ('fneu', f'a@{float_sz}', a), 0.0, a)), (f'f2u{int_sz}_sat', a), 'options->has_f2u_sat'),
        ])

# re-combine inexact mul+add to ffma. Do this before fsub so that a * b - c
# gets combined to fma(a, b, -c).
for sz, mulz in itertools.product([16, 32, 64], [False, True]):
    # fmulz/ffmaz only for fp32
    if mulz and sz != 32:
        continue

    # Fuse the correct fmul. Only consider fmuls where the only users are fadd
    # (or fneg/fabs which are assumed to be propagated away), as a heuristic to
    # avoid fusing in cases where it's harmful.
    fmul = ('fmulz' if mulz else 'fmul') + '(is_only_used_by_fadd)'

    option = f'options->float_mul_add{sz}'
    option_avoid_abs = 'options->avoid_ternary_with_fabs'
    option_has_fmad = f'({option} & nir_float_muladd_support_has_fmad)'
    option_has_ffma = f'({option} & nir_float_muladd_support_has_ffma)'
    option_prefer_split = f'({option} & nir_float_muladd_support_prefers_split)'
    option_fuse = f'({option} & nir_float_muladd_support_fuse)'

    option_fmad = f'{option_fuse} && (!{option_has_ffma} ||  {option_prefer_split}) && {option_has_fmad}'
    option_ffma = f'{option_fuse} && (!{option_has_fmad} || !{option_prefer_split}) && {option_has_ffma}'

    if mulz:
        option_ffma += f' && {has_ffmaz}'

    for fmad in ['ffma', 'fmad']:
        option = option_fmad if fmad == 'fmad' else option_ffma
        # contract is only needed for ffma
        fadd = f'fadd@{sz}' if fmad == 'fmad' else f'fadd@{sz}(contract)'
        fmad = fmad + 'z' if mulz else fmad

        late_optimizations.extend([
            ((fadd, (fmul, a, b), c), (fmad, a, b, c), option),

            ((fadd, ('fneg(is_only_used_by_fadd)', (fmul, a, b)), c),
             (fmad, ('fneg', a), b, c), option),

            ((fadd, ('fabs(is_only_used_by_fadd)', (fmul, a, b)), c),
             (fmad, ('fabs', a), ('fabs', b), c), f'{option} && !{option_avoid_abs}'),

            ((fadd, ('fneg(is_only_used_by_fadd)', ('fabs', (fmul, a, b))), c),
             (fmad, ('fneg', ('fabs', a)), ('fabs', b), c), f'{option} && !{option_avoid_abs}'),
        ])

late_optimizations.extend([
   # Subtractions get lowered during optimization, so we need to recombine them
   (('fadd@16', a, ('fneg', 'b')), ('fsub', 'a', 'b'), 'options->has_fsub'),
   (('fadd@32', a, ('fneg', 'b')), ('fsub', 'a', 'b'), 'options->has_fsub'),
   (('fadd@64', a, ('fneg', 'b')), ('fsub', 'a', 'b'), 'options->has_fsub && !(options->lower_doubles_options & nir_lower_dsub)'),

   (('fneg', a), ('fmul', a, -1.0), 'options->lower_fneg'),
   (('iadd', a, ('ineg', 'b')), ('isub', 'a', 'b'), 'options->has_isub || options->lower_ineg'),
   (('ineg', a), ('isub', 0, a), 'options->lower_ineg'),
   (('iabs', a), ('imax', a, ('ineg', a)), 'options->lower_iabs'),
])

for s in [8, 16, 32, 64]:
   cond = 'options->has_iadd3'
   if s == 64:
      cond += ' && !(options->lower_int64_options & nir_lower_iadd3_64)'

   iadd = "iadd@{}".format(s)

   # On Intel GPUs, the constant field for an ADD3 instruction must be either
   # int16_t or uint16_t.
   late_optimizations.extend([
      ((iadd, ('iadd(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), 'c(is_not_const)'), ('iadd3', a, b, c), cond),
      ((iadd, ('iadd(is_used_once)', '#a(is_16_bits)',  'b(is_not_const)'), 'c(is_not_const)'), ('iadd3', a, b, c), cond),
      ((iadd, ('iadd(is_used_once)', 'a(is_not_const)', 'b(is_not_const)'), '#c(is_16_bits)'),   ('iadd3', a, b, c), cond),
      ((iadd, ('ineg', ('iadd(is_used_once)', 'a(is_not_const)', 'b(is_not_const)')), 'c(is_not_const)'), ('iadd3', ('ineg', a), ('ineg', b), c), cond),
      ((iadd, ('ineg', ('iadd(is_used_once)', '#a(is_16_bits)',  'b(is_not_const)')), 'c(is_not_const)'), ('iadd3', ('ineg', a), ('ineg', b), c), cond),
      ((iadd, ('ineg', ('iadd(is_used_once)', 'a(is_not_const)', 'b(is_not_const)')), '#c(is_16_bits)'),  ('iadd3', ('ineg', a), ('ineg', b), c), cond),

      ((iadd, ('ishl', a, 1), 'b(is_not_const)'), ('iadd3', a, a, b), cond),
      ((iadd, ('ishl', a, 1), '#b(is_16_bits)' ), ('iadd3', a, a, b), cond),
      ((iadd, ('ineg', ('ishl', a, 1)), 'b(is_not_const)'), ('iadd3', ('ineg', a), ('ineg', a), b), cond),
      ((iadd, ('ineg', ('ishl', a, 1)), '#b(is_16_bits)' ), ('iadd3', ('ineg', a), ('ineg', a), b), cond),

      # Use special checks to ensure (b+b) or -(b+b) fit in 16 bits.
      (('ishl@{}'.format(s), ('iadd', a, '#b(is_2x_16_bits)'), 1), ('iadd3', a, a, ('iadd', b, b)), cond),
      (('ishl@{}'.format(s), ('ineg', ('iadd', a, '#b(is_neg2x_16_bits)')), 1), ('iadd3', ('ineg', a), ('ineg', a), ('ineg', ('iadd', b, b))), cond),
   ])

late_optimizations.extend([
    # fneg_lo / fneg_hi
   (('vec2(is_only_used_as_float)', ('fneg@16', a), b), ('fmul', ('vec2', a, b), ('vec2', -1.0, 1.0)), 'options->vectorize_vec2_16bit'),
   (('vec2(is_only_used_as_float)', a, ('fneg@16', b)), ('fmul', ('vec2', a, b), ('vec2', 1.0, -1.0)), 'options->vectorize_vec2_16bit'),

   # These are duplicated from the main optimizations table.  The late
   # patterns that rearrange expressions like x - .5 < 0 to x < .5 can create
   # new patterns like these.  The patterns that compare with zero are removed
   # because they are unlikely to be created in by anything in
   # late_optimizations.
   (('flt', '#b(is_a_number_gt_0_and_lt_1)', ('fsat(is_used_once)', a)), ('flt', b, a)),
   (('fge', ('fsat(is_used_once)', a), '#b(is_a_number_gt_0_and_lt_1)'), ('fge', a, b)),
   (('feq', ('fsat(is_used_once)', a), '#b(is_a_number_gt_0_and_lt_1)'), ('feq', a, b)),
   (('fneu', ('fsat(is_used_once)', a), '#b(is_a_number_gt_0_and_lt_1)'), ('fneu', a, b)),

   (('fge', ('fsat(is_used_once)', a), 1.0), ('fge', a, 1.0)),

   (('fge', ('fmin(is_used_once,nnan)', ('fadd(is_used_once)', a, b), ('fadd', c, d)), 0.0), ('iand', ('fge', a, ('fneg', b)), ('fge', c, ('fneg', d)))),
])

for cmp in ['flt', 'fltu', 'fge', 'fgeu', 'feq', 'fequ', 'fneo', 'fneu']:
   late_optimizations.extend([
      ((cmp, ('fneg', a), ('fneg', b)), (cmp, b, a)),
      ((cmp, ('fneg', a), '#b'), (cmp, ('fneg', b), a)),
   ])

for cmp in ['flt', 'fltu', 'fge', 'fgeu']:
   late_optimizations.extend([
      ((cmp, '#b', ('fneg', a)), (cmp, a, ('fneg', b))),
   ])

late_optimizations.extend([
   (('ior', a, a), a),
   (('iand', a, a), a),

   (('fadd', ('fneg(is_used_once)', ('fsat(is_used_once,nnan)', 'a(is_not_fmul)')), 1.0), ('fsat', ('fadd', 1.0, ('fneg', a)))),

   (('fsqrt', ('fsat(is_used_once)', 'a(cannot_add_output_modifier)')), ('fsat', ('fsqrt', a))),

   (('fdot2', a, b), ('fdot2_replicated', a, b), 'options->fdot_replicates'),
   (('fdot3', a, b), ('fdot3_replicated', a, b), 'options->fdot_replicates'),
   (('fdot4', a, b), ('fdot4_replicated', a, b), 'options->fdot_replicates'),
   (('fdph', a, b), ('fdph_replicated', a, b), 'options->fdot_replicates'),

   (('~flrp', ('fadd(is_used_once)', a, b), ('fadd(is_used_once)', a, c), d), ('fadd', ('flrp', b, c, d), a)),

   # Lower roundEven for hardware without a native round instruction.
   # Round to the nearest integer using ffloor and ffract, and on a half break
   # the tie towards the even neighbour. Kept in late_optimizations so that
   # is_integral() can remove the roundEven first.
   (('fround_even', a), ('fadd', ('ffloor', a),
                         ('bcsel', ('flt', 0.5, ('ffract', a)),
                          1.0,
                          ('bcsel', ('feq', ('ffract', a), 0.5),
                           ('fmul', ('ffract', ('fmul', ('ffloor', a), 0.5)), 2.0),
                           0.0))), 'options->lower_fround_even'),

   # A similar operation could apply to any ffma(#a, b, #(-a/2)), but this
   # particular operation is common for expanding values stored in a texture
   # from [0,1] to [-1,1].
   (('~ffma@32', a,  2.0, -1.0), ('flrp', -1.0,  1.0,          a ), '!options->lower_flrp32'),
   (('~ffma@32', a, -2.0, -1.0), ('flrp', -1.0,  1.0, ('fneg', a)), '!options->lower_flrp32'),
   (('~ffma@32', a, -2.0,  1.0), ('flrp',  1.0, -1.0,          a ), '!options->lower_flrp32'),
   (('~ffma@32', a,  2.0,  1.0), ('flrp',  1.0, -1.0, ('fneg', a)), '!options->lower_flrp32'),
   (('~fmad@32', a,  2.0, -1.0), ('flrp', -1.0,  1.0,          a ), '!options->lower_flrp32'),
   (('~fmad@32', a, -2.0, -1.0), ('flrp', -1.0,  1.0, ('fneg', a)), '!options->lower_flrp32'),
   (('~fmad@32', a, -2.0,  1.0), ('flrp',  1.0, -1.0,          a ), '!options->lower_flrp32'),
   (('~fmad@32', a,  2.0,  1.0), ('flrp',  1.0, -1.0, ('fneg', a)), '!options->lower_flrp32'),
   (('~fadd@32', ('fmul(is_used_once)',  2.0, a), -1.0), ('flrp', -1.0,  1.0,          a ), '!options->lower_flrp32'),
   (('~fadd@32', ('fmul(is_used_once)', -2.0, a), -1.0), ('flrp', -1.0,  1.0, ('fneg', a)), '!options->lower_flrp32'),
   (('~fadd@32', ('fmul(is_used_once)', -2.0, a),  1.0), ('flrp',  1.0, -1.0,          a ), '!options->lower_flrp32'),
   (('~fadd@32', ('fmul(is_used_once)',  2.0, a),  1.0), ('flrp',  1.0, -1.0, ('fneg', a)), '!options->lower_flrp32'),

    # flrp(a, b, a)
    # a*(1-a) + b*a
    # a + -a*a + a*b    (1)
    # a + a*(b - a)
    # Option 1: ffma(a, (b-a), a)
    #
    # Alternately, after (1):
    # a*(1+b) + -a*a
    # a*((1+b) + -a)
    #
    # Let b=1
    #
    # Option 2: ffma(a, 2, -(a*a))
    # Option 3: ffma(a, 2, (-a)*a)
    # Option 4: ffma(a, -a, (2*a)
    # Option 5: a * (2 - a)
    #
    # There are a lot of other possible combinations.
   (('~ffma@32', ('fadd', b, ('fneg', a)), a, a), ('flrp', a, b, a), '!options->lower_flrp32'),
   (('~ffma@32', a, 2.0, ('fneg', ('fmul', a, a))), ('flrp', a, 1.0, a), '!options->lower_flrp32'),
   (('~ffma@32', a, 2.0, ('fmul', ('fneg', a), a)), ('flrp', a, 1.0, a), '!options->lower_flrp32'),
   (('~ffma@32', a, ('fneg', a), ('fmul', 2.0, a)), ('flrp', a, 1.0, a), '!options->lower_flrp32'),
   (('~fmad@32', ('fadd', b, ('fneg', a)), a, a), ('flrp', a, b, a), '!options->lower_flrp32'),
   (('~fmad@32', a, 2.0, ('fneg', ('fmul', a, a))), ('flrp', a, 1.0, a), '!options->lower_flrp32'),
   (('~fmad@32', a, 2.0, ('fmul', ('fneg', a), a)), ('flrp', a, 1.0, a), '!options->lower_flrp32'),
   (('~fmad@32', a, ('fneg', a), ('fmul', 2.0, a)), ('flrp', a, 1.0, a), '!options->lower_flrp32'),
   (('~fmul@32', a, ('fadd', 2.0, ('fneg', a))),    ('flrp', a, 1.0, a), '!options->lower_flrp32'),

   # we do these late so that we don't get in the way of creating ffmas
   (('fmin', ('fadd(is_used_once)', '#c', a), ('fadd(is_used_once)', '#c', b)), ('fadd', c, ('fmin', a, b)), 'true', TestStatus.XFAIL), # XFAIL is that fmin(fadd(inf, -inf), fadd(inf, 0)) produces -NaN instead of inf.
   (('fmax', ('fadd(is_used_once)', '#c', a), ('fadd(is_used_once)', '#c', b)), ('fadd', c, ('fmax', a, b)), 'true', TestStatus.XFAIL), # XFAIL is that fmax(fadd(-inf, inf), fadd(-inf, 0)) produces -NaN instead of -inf.

   # Putting this in 'optimizations' interferes with the bcsel(a, op(b, c),
   # op(b, d)) => op(b, bcsel(a, c, d)) transformations.  I do not know why.
   (('bcsel@32', ('feq', ('fsqrt', 'a(is_a_number_not_negative)'), 0.0), intBitsToFloat(0x7f7fffff), ('frsq', a)),
    ('fmin', ('frsq', a), intBitsToFloat(0x7f7fffff))),
])

# Things that look like DPH in the source shader may get expanded to
# something that looks like dot(v1.xyz, v2.xyz) + v1.w by the time it gets
# to NIR.  After FFMA is generated, this can look like:
#
#    fadd(ffma(v1.z, v2.z, ffma(v1.y, v2.y, fmul(v1.x, v2.x))), v1.w)
#
# Reassociate the last addition into the first multiplication.
#
# Some shaders do not use 'invariant' in vertex and (possibly) geometry
# shader stages on some outputs that are intended to be invariant.  For
# various reasons, this optimization may not be fully applied in all
# shaders used for different rendering passes of the same geometry.  This
# can result in Z-fighting artifacts (at best).  For now, disable this
# optimization in these stages.  See bugzilla #111490.  In tessellation
# stages applications seem to use 'precise' when necessary, so allow the
# optimization in those stages.
for fmad in ['ffma', 'fmad']:
   late_optimizations.extend([
      (('~fadd', (f'{fmad}(is_used_once)', a, b, (f'{fmad}(is_used_once)', c, d, (fmad, e, 'f', ('fmul(is_used_once)', 'g(is_not_const_and_not_fsign)', 'h(is_not_const_and_not_fsign)')))), 'i(is_not_const)'),
       (fmad, a, b, (fmad, c, d, (fmad, e, 'f', (fmad, 'g', 'h', 'i')))), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
      (('~fadd', (f'{fmad}(is_used_once)', a, b, (fmad, c, d, ('fmul(is_used_once)', 'e(is_not_const_and_not_fsign)', 'f(is_not_const_and_not_fsign)'))), 'g(is_not_const)'),
       (fmad, a, b, (fmad, c, d, (fmad, e, 'f', 'g'))), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
      (('~fadd', (f'{fmad}(is_used_once)', a, b, ('fmul(is_used_once)', 'c(is_not_const_and_not_fsign)', 'd(is_not_const_and_not_fsign)') ), 'e(is_not_const)'),
       (fmad, a, b, (fmad, c, d, e)), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
      (('~fadd', ('fneg', (f'{fmad}(is_used_once)', a, b, (fmad, c, d, ('fmul(is_used_once)', 'e(is_not_const_and_not_fsign)', 'f(is_not_const_and_not_fsign)')))), 'g(is_not_const)'),
       (fmad, ('fneg', a), b, (fmad, ('fneg', c), d, (fmad, ('fneg', e), 'f', 'g'))), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
   ])

for fmadz in ['ffmaz', 'fmadz']:
   late_optimizations.extend([
      (('~fadd', (f'{fmadz}(is_used_once)', a, b, (fmadz, c, d, ('fmulz(is_used_once)', 'e(is_not_const_and_not_fsign)', 'f(is_not_const_and_not_fsign)'))), 'g(is_not_const)'),
       (fmadz, a, b, (fmadz, c, d, (fmadz, e, 'f', 'g'))), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
      (('~fadd', (f'{fmadz}(is_used_once)', a, b, ('fmulz(is_used_once)', 'c(is_not_const_and_not_fsign)', 'd(is_not_const_and_not_fsign)') ), 'e(is_not_const)'),
       (fmadz, a, b, (fmadz, c, d, e)), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
      (('~fadd', ('fneg', (f'{fmadz}(is_used_once)', a, b, (fmadz, c, d, ('fmulz(is_used_once)', 'e(is_not_const_and_not_fsign)', 'f(is_not_const_and_not_fsign)')))), 'g(is_not_const)'),
       (fmadz, ('fneg', a), b, (fmadz, ('fneg', c), d, (fmadz, ('fneg', e), 'f', 'g'))), '(info->stage != MESA_SHADER_VERTEX && info->stage != MESA_SHADER_GEOMETRY) && !options->intel_vec4'),
   ])


late_optimizations.extend([
   (('fmul(contract)', a, ('ldexp(is_used_once)', 1.0, b)), ('ldexp', a, b), 'options->has_ldexp'),
   (('frcp(contract,ninf)', ('ldexp', 1.0, b)), ('ldexp', 1.0, ('ineg', b)), 'options->has_ldexp'),

   # Section 8.8 (Integer Functions) of the GLSL 4.60 spec says:
   #
   #    If bits is zero, the result will be zero.
   #
   # These prevent the next two lowerings generating incorrect results when
   # count is zero.
   (('ubfe', a, b, 0), 0),
   (('ibfe', a, b, 0), 0),

   # On Intel GPUs, BFE is a 3-source instruction.  Like all 3-source
   # instructions on Intel GPUs, it cannot have an immediate values as
   # sources.  There are also limitations on source register strides.  As a
   # result, it is very easy for 3-source instruction combined with either
   # loads of immediate values or copies from weird register strides to be
   # more expensive than the primitive instructions it represents.
   (('ubfe', a, '#b', '#c'), ('iand', ('ushr', 0xffffffff, ('ineg', c)), ('ushr', a, b)), 'options->avoid_ternary_with_two_constants', TestStatus.XFAIL), # XFAIL is that ubfe(1,0,0) produces 1 instead of 0

   # b is the lowest order bit to be extracted and c is the number of bits to
   # extract.  The inner shift removes the bits above b + c by shifting left
   # 32 - (b + c).  ishl only sees the low 5 bits of the shift count, which is
   # -(b + c).  The outer shift moves the bit that was at b to bit zero.
   # After the first shift, that bit is now at b + (32 - (b + c)) or 32 - c.
   # This means that it must be shifted right by 32 - c or -c bits.
   (('ibfe', a, '#b', '#c'), ('ishr', ('ishl', a, ('ineg', ('iadd', b, c))), ('ineg', c)), 'options->avoid_ternary_with_two_constants', TestStatus.XFAIL), # XFAIL is that ibfe(1,0,0) produces 1 instead of 0

   # Clean up no-op shifts that may result from the bfe lowerings.
   (('ishl', a, 0), a),
   (('ishl', a, -32), a),
   (('ishr', a, 0), a),
   (('ishr', a, -32), a),
   (('ushr', a, 0), a),

   (('extract_i8', ('extract_i8', a, b), 0), ('extract_i8', a, b)),
   (('extract_i8', ('extract_u8', a, b), 0), ('extract_i8', a, b)),
   (('extract_u8', ('extract_i8', a, b), 0), ('extract_u8', a, b)),
   (('extract_u8', ('extract_u8', a, b), 0), ('extract_u8', a, b)),

   # open coded bit test
   (('ine', ('iand', a, '#b(is_pos_power_of_two)'), 0), ('bitnz', a, ('find_lsb', b)), 'options->has_bit_test'),
   (('ieq', ('iand', a, '#b(is_pos_power_of_two)'), 0), ('bitz', a, ('find_lsb', b)), 'options->has_bit_test'),
   (('ine', ('iand', a, '#b(is_pos_power_of_two)'), b), ('bitz', a, ('find_lsb', b)), 'options->has_bit_test'),
   (('ieq', ('iand', a, '#b(is_pos_power_of_two)'), b), ('bitnz', a, ('find_lsb', b)), 'options->has_bit_test'),
   (('ine', ('iand', a, ('ishl', 1, b)), 0), ('bitnz', a, b), 'options->has_bit_test'),
   (('ieq', ('iand', a, ('ishl', 1, b)), 0), ('bitz', a, b), 'options->has_bit_test'),
   (('ine', ('iand', a, ('ishl', 1, b)), ('ishl', 1, b)), ('bitz', a, b), 'options->has_bit_test'),
   (('ieq', ('iand', a, ('ishl', 1, b)), ('ishl', 1, b)), ('bitnz', a, b), 'options->has_bit_test'),
   (('bitz', ('ushr', a, b), 0), ('bitz', a, b)),
   (('bitz', ('ishr', a, b), 0), ('bitz', a, b)),
   (('bitnz', ('ushr', a, b), 0), ('bitnz', a, b)),
   (('bitnz', ('ishr', a, b), 0), ('bitnz', a, b)),
   (('bitz', ('unpack_64_2x32_split_x', ('ushr', a, b)), 0), ('bitz', a, b)),
   (('bitz', ('unpack_64_2x32_split_x', ('ishr', a, b)), 0), ('bitz', a, b)),
   (('bitnz', ('unpack_64_2x32_split_x', ('ushr', a, b)), 0), ('bitnz', a, b)),
   (('bitnz', ('unpack_64_2x32_split_x', ('ishr', a, b)), 0), ('bitnz', a, b)),
   (('ine', ('ubfe', a, b, 1), 0), ('bitnz', a, b), 'options->has_bit_test'),
   (('ieq', ('ubfe', a, b, 1), 0), ('bitz', a, b), 'options->has_bit_test'),
   (('ine', ('ubfe', a, b, 1), 1), ('bitz', a, b), 'options->has_bit_test'),
   (('ieq', ('ubfe', a, b, 1), 1), ('bitnz', a, b), 'options->has_bit_test'),
   (('ine', ('ibfe', a, b, 1), 0), ('bitnz', a, b), 'options->has_bit_test'),
   (('ieq', ('ibfe', a, b, 1), 0), ('bitz', a, b), 'options->has_bit_test'),
   (('ine', ('ibfe', a, b, 1), -1), ('bitz', a, b), 'options->has_bit_test'),
   (('ieq', ('ibfe', a, b, 1), -1), ('bitnz', a, b), 'options->has_bit_test'),
   (('inot', ('bitnz', a, b)), ('bitz', a, b)),
   (('inot', ('bitz', a, b)), ('bitnz', a, b)),
   (('bitnz', ('inot', a), b), ('bitz', a, b)),
   (('bitz', ('inot', a), b), ('bitnz', a, b)),
])

late_optimizations += [
   # If we can't eliminate it, lower it so that backends don't have to deal with
   # it.
   (('fcanonicalize', a), ('fmul', a, 1.0), '!options->has_fcanonicalize'),
]

# A few more extract cases we'd rather leave late
for N in [16, 32]:
    aN = 'a@{0}'.format(N)
    u2uM = 'u2u{0}'.format(M)
    i2iM = 'i2i{0}'.format(M)

    for x in ['u', 'i']:
        x2xN = '{0}2{0}{1}'.format(x, N)
        extract_x8 = 'extract_{0}8'.format(x)
        extract_x16 = 'extract_{0}16'.format(x)

        late_optimizations.extend([
            ((x2xN, ('u2u8', aN)), (extract_x8, a, 0), '!options->lower_extract_byte'),
            ((x2xN, ('i2i8', aN)), (extract_x8, a, 0), '!options->lower_extract_byte'),
        ])

        if N > 16:
            late_optimizations.extend([
                ((x2xN, ('u2u16', aN)), (extract_x16, a, 0), '!options->lower_extract_word'),
                ((x2xN, ('i2i16', aN)), (extract_x16, a, 0), '!options->lower_extract_word'),
            ])

# Byte insertion
late_optimizations.extend([(('ishl', ('extract_u8', 'a@32', 0), 8 * i), ('insert_u8', a, i), '!options->lower_insert_byte') for i in range(1, 4)])
late_optimizations.extend([(('iand', ('ishl', 'a@32', 8 * i), 0xff << (8 * i)), ('insert_u8', a, i), '!options->lower_insert_byte') for i in range(1, 4)])
late_optimizations.append((('ishl', 'a@32', 24), ('insert_u8', a, 3), '!options->lower_insert_byte'))

late_optimizations += [
   # Word insertion
   (('ishl', 'a@32', 16), ('insert_u16', a, 1), '!options->lower_insert_word'),

   # Extract and then insert
   (('insert_u8', ('extract_u8', 'a', 0), b), ('insert_u8', a, b)),
   (('insert_u16', ('extract_u16', 'a', 0), b), ('insert_u16', a, b)),
]

# Float sizes
for s in [16, 32, 64]:
    late_optimizations.extend([
       (('~fadd@{}'.format(s), 1.0, ('fmul(is_used_once)', c , ('fadd', b, -1.0 ))), ('fadd', ('fadd', 1.0, ('fneg', c)), ('fmul', b, c)), 'options->lower_flrp{}'.format(s)),
    ])

for op in ['fadd']:
    late_optimizations += [
        (('bcsel', a, (op + '(is_used_once)', b, c), (op, b, d)), (op, b, ('bcsel', a, c, d))),
        (('bcsel', a, (op, b, c), (op + '(is_used_once)', b, d)), (op, b, ('bcsel', a, c, d))),
    ]

for op in ['ffma', 'ffmaz']:
    late_optimizations += [
        (('bcsel', a, (op + '(is_used_once)', b, c, d), (op, b, c, e)), (op, b, c, ('bcsel', a, d, e))),
        (('bcsel', a, (op, b, c, d), (op + '(is_used_once)', b, c, e)), (op, b, c, ('bcsel', a, d, e))),

        (('bcsel', a, (op + '(is_used_once)', b, c, d), (op, b, e, d)), (op, b, ('bcsel', a, c, e), d)),
        (('bcsel', a, (op, b, c, d), (op + '(is_used_once)', b, e, d)), (op, b, ('bcsel', a, c, e), d)),
    ]

# Lower fmulz using min(abs(a), abs(b))==0.0
late_optimizations += [
   (('fmulz@32', a, b),
    ('bcsel', ('feq', ('fmin', ('fabs', a), ('fabs', b)), 0.0), 0.0, ('fmul', a, b)), 'options->lower_fmulz_with_abs_min'),
   (('ffmaz@32', a, b, c),
    ('bcsel', ('feq', ('fmin', ('fabs', a), ('fabs', b)), 0.0), c, ('ffma@32', a, b, c)), 'options->lower_fmulz_with_abs_min'),
   (('fmadz@32', a, b, c),
    ('bcsel', ('feq', ('fmin', ('fabs', a), ('fabs', b)), 0.0), c, ('fmad@32', a, b, c)), 'options->lower_fmulz_with_abs_min'),
]

# mediump: If an opcode is surrounded by conversions, remove the conversions.
# The rationale is that type conversions + the low precision opcode are more
# expensive that the same arithmetic opcode at higher precision.
#
# This must be done in late optimizations, because we need normal optimizations to
# first eliminate temporary up-conversions such as in op1(f2fmp(f2f32(op2()))).
#
# Unary opcodes
for op in ['fabs', 'fceil', 'fcos', 'fexp2', 'ffloor', 'ffract', 'flog2', 'fneg',
           'frcp', 'fround_even', 'frsq', 'fsat', 'fsign', 'fsin', 'fsqrt']:
    late_optimizations += [(('~f2f32', (op, ('f2fmp', a))), (op, a), 'true', TestStatus.UNSUPPORTED)]

# Binary opcodes
for op in ['fadd', 'fdiv', 'fmax', 'fmin', 'fmod', 'fmul', 'fpow', 'frem']:
    late_optimizations += [(('~f2f32', (op, ('f2fmp', a), ('f2fmp', b))), (op, a, b), 'true', TestStatus.UNSUPPORTED)]

# Ternary opcodes
for op in ['ffma', 'flrp']:
    late_optimizations += [(('~f2f32', (op, ('f2fmp', a), ('f2fmp', b), ('f2fmp', c))), (op, a, b, c), 'true', TestStatus.UNSUPPORTED)]

# Comparison opcodes
for op in ['feq', 'fge', 'flt', 'fneu']:
    late_optimizations += [(('~' + op, ('f2fmp', a), ('f2fmp', b)), (op, a, b), 'true', TestStatus.UNSUPPORTED)]

# Do this last, so that the f2fmp patterns above have effect.
late_optimizations += [
  # Convert *2*mp instructions to concrete *2*16 instructions. At this point
  # any conversions that could have been removed will have been removed in
  # nir_opt_algebraic so any remaining ones are required.
  (('f2fmp', a), ('f2f16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),
  (('f2imp', a), ('f2i16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),
  (('f2ump', a), ('f2u16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),
  (('i2imp', a), ('i2i16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),
  (('i2fmp', a), ('i2f16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),
  (('i2imp', a), ('u2u16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),
  (('u2fmp', a), ('u2f16', a), "!options->preserve_mediump", TestStatus.UNSUPPORTED),

  # Without preserving inf, comparing against inf is undefined
  (('fisfinite(ninf)', a), ('feq', a, a)),
  (('fisfinite', a), ('flt', ('fabs', a), float("inf"))),

  (('f2f16', a), ('f2f16_rtz', a), "options->force_f2f16_rtz && !nir_is_rounding_mode_rtne(info->float_controls_execution_mode, 16)"),

  (('fcsel', ('slt', 0, a), b, c), ('fcsel_gt', a, b, c), "options->has_fused_comp_and_csel"),
  (('fcsel', ('slt', a, 0), b, c), ('fcsel_gt', ('fneg', a), b, c), "options->has_fused_comp_and_csel"),
  (('fcsel', ('sge', a, 0), b, c), ('fcsel_ge', a, b, c), "options->has_fused_comp_and_csel"),
  (('fcsel', ('sge', 0, a), b, c), ('fcsel_ge', ('fneg', a), b, c), "options->has_fused_comp_and_csel"),

  (('bcsel', ('ilt', 0, 'a@32'), 'b@32', 'c@32'), ('i32csel_gt', a, b, c), "options->has_fused_comp_and_csel && !options->no_integers"),
  (('bcsel', ('ilt', 'a@32', 0), 'b@32', 'c@32'), ('i32csel_ge', a, c, b), "options->has_fused_comp_and_csel && !options->no_integers"),
  (('bcsel', ('ige', 'a@32', 0), 'b@32', 'c@32'), ('i32csel_ge', a, b, c), "options->has_fused_comp_and_csel && !options->no_integers"),
  (('bcsel', ('ige', 0, 'a@32'), 'b@32', 'c@32'), ('i32csel_gt', a, c, b), "options->has_fused_comp_and_csel && !options->no_integers"),

  (('bcsel', ('flt', 0, 'a@32'), 'b@32', 'c@32'), ('fcsel_gt', a, b, c), "options->has_fused_comp_and_csel"),
  (('bcsel', ('flt', 'a@32', 0), 'b@32', 'c@32'), ('fcsel_gt', ('fneg', a), b, c), "options->has_fused_comp_and_csel"),
  (('bcsel', ('fge', 'a@32', 0), 'b@32', 'c@32'), ('fcsel_ge', a, b, c), "options->has_fused_comp_and_csel"),
  (('bcsel', ('fge', 0, 'a@32'), 'b@32', 'c@32'), ('fcsel_ge', ('fneg', a), b, c), "options->has_fused_comp_and_csel"),
]

for s in [16, 32, 64]:
    late_optimizations.extend([
        (('bcsel@{}'.format(s), ('ieq', 0, 'a@{}'.format(s)), 'b@{}'.format(s), 'c@{}'.format(s)), ('icsel_eqz', a, b, c), "options->has_icsel_eqz{} && !options->no_integers".format(s)),
        (('bcsel@{}'.format(s), ('ine', 0, 'a@{}'.format(s)), 'b@{}'.format(s), 'c@{}'.format(s)), ('icsel_eqz', a, c, b), "options->has_icsel_eqz{} && !options->no_integers".format(s)),
    ])

late_optimizations += [
    (('f2i32', ('fround_even(is_used_once)', 'a@32')), ('f2i32_rtne', a), 'options->has_f2i32_rtne'),
]

distribute_src_mods = [
   # Try to remove some spurious negations rather than pushing them down.
   (('fmul', ('fneg', a), ('fneg', b)), ('fmul', a, b)),
   (('ffma', ('fneg', a), ('fneg', b), c), ('ffma', a, b, c)),
   (('fdot2_replicated', ('fneg', a), ('fneg', b)), ('fdot2_replicated', a, b)),
   (('fdot3_replicated', ('fneg', a), ('fneg', b)), ('fdot3_replicated', a, b)),
   (('fdot4_replicated', ('fneg', a), ('fneg', b)), ('fdot4_replicated', a, b)),
   (('fneg(is_only_used_as_float)', ('fneg', a)), a),

   (('fneg', ('fmul(is_used_once)', a, b)), ('fmul', ('fneg', a), b)),
   (('fneg', ('fmul_rtz(is_used_once)', a, b)), ('fmul_rtz', ('fneg', a), b)),
   (('fabs', ('fmul(is_used_once)', a, b)), ('fmul', ('fabs', a), ('fabs', b))),
   (('fabs', ('fmul_rtz(is_used_once)', a, b)), ('fmul_rtz', ('fabs', a), ('fabs', b))),

   (('fneg', ('ffma(is_used_once,nsz)', a, b, c)), ('ffma', ('fneg', a), b, ('fneg', c))),
   (('fneg', ('flrp(is_used_once)', a, b, c)), ('flrp', ('fneg', a), ('fneg', b), c), 'true', TestStatus.XFAIL), # XFAIL is -flrp(0, -1, 0) is 0.0 instead of -0.0
   (('fneg', ('fadd(is_used_once,nsz)', a, b)), ('fadd', ('fneg', a), ('fneg', b))),

   # Note that fmin <-> fmax.  I don't think there is a way to distribute
   # fabs() into fmin or fmax.
   (('fneg', ('fmin(is_used_once)', a, b)), ('fmax', ('fneg', a), ('fneg', b))),
   (('fneg', ('fmax(is_used_once)', a, b)), ('fmin', ('fneg', a), ('fneg', b))),

   (('fneg', ('fdot2_replicated(is_used_once)', a, b)), ('fdot2_replicated', ('fneg', a), b), 'true', TestStatus.XFAIL), # -fdot2(-1, 0) replacement produces 0 instead of -0.
   (('fneg', ('fdot3_replicated(is_used_once)', a, b)), ('fdot3_replicated', ('fneg', a), b), 'true', TestStatus.XFAIL),
   (('fneg', ('fdot4_replicated(is_used_once)', a, b)), ('fdot4_replicated', ('fneg', a), b), 'true', TestStatus.XFAIL),

   # fdph works mostly like fdot, but to get the correct result, the negation
   # must be applied to the second source.
   (('fneg', ('fdph_replicated(is_used_once)', a, b)), ('fdph_replicated', a, ('fneg', b)), 'true', TestStatus.XFAIL),

   (('fneg', ('fsign(is_used_once)', a)), ('fsign', ('fneg', a))),
   (('fabs', ('fsign(is_used_once)', a)), ('fsign', ('fabs', a))),
]

# Reassociate multiplication of mat*mat*vec. The typical case of
# mat4*mat4*vec4 is 80 scalar multiplications, but mat4*(mat4*vec4) is only 32.
mat_mul_optimizations = []
for t in ['f', 'i']:
   add_first = '~{}add(many-comm-expr)'.format(t)
   add_used_once = '~{}add(is_used_once)'.format(t)
   add = '~{}add'.format(t)
   mul = '~{}mul'.format(t)

   # Variable names used below were selected based on these layouts:
   #     mat4             mat4         vec4
   # [a, b, c, d]   [q,   r,  s,  t]   [gg]
   # [e, f, g, h]   [u,   v,  w,  x]   [hh]
   # [i, j, k, l] x [y,   z, aa, bb] x [ii]
   # [m, n, o, p]   [cc, dd, ee, ff]   [jj]

   step1 = (add_used_once, (add, (add, (mul, 'a', 'q'), (mul, 'b', 'u')), (mul, 'c', 'y')), (mul, 'd', 'cc'))
   step2 = (add_used_once, (add, (add, (mul, 'a', 'r'), (mul, 'b', 'v')), (mul, 'c', 'z')), (mul, 'd', 'dd'))
   step3 = (add_used_once, (add, (add, (mul, 'a', 's'), (mul, 'b', 'w')), (mul, 'c', 'aa')), (mul, 'd', 'ee'))
   step4 = (add_used_once, (add, (add, (mul, 'a', 't'), (mul, 'b', 'x')), (mul, 'c', 'bb')), (mul, 'd', 'ff'))

   step5 = (add, (add, (add, (mul, 'q', 'gg'), (mul, 'r', 'hh')), (mul, 's', 'ii')), (mul, 't', 'jj'))
   step6 = (add, (add, (add, (mul, 'u', 'gg'), (mul, 'v', 'hh')), (mul, 'w', 'ii')), (mul, 'x', 'jj'))
   step7 = (add, (add, (add, (mul, 'y', 'gg'), (mul, 'z', 'hh')), (mul, 'aa', 'ii')), (mul, 'bb', 'jj'))
   step8 = (add, (add, (add, (mul, 'cc', 'gg'), (mul, 'dd', 'hh')), (mul, 'ee', 'ii')), (mul, 'ff', 'jj'))

   # This finds and replaces common (mat4*mat4)*vec4 with something that will get optimised down to mat4*(mat4*vec4)
   mat_mul_optimizations += [((add_first, (add, (add, (mul, step1, 'gg'), (mul, step2, 'hh')), (mul, step3, 'ii')), (mul, step4, 'jj')), (add, (add, (add, (mul, step5, 'a'), (mul, step6, 'b')), (mul, step7, 'c')), (mul, step8, 'd')))]

   # This helps propagate the above improvement further up the mul chain e.g. mat4*mat4*mat4*vec4 to (mat4*vec4)*mat4*mat4
   mat_mul_optimizations += [((add_first, (add, (add, (mul, 'gg', step1), (mul,'hh', step2)), (mul, 'ii', step3)), (mul, 'jj', step4)), (add, (add, (add, (mul, step5, 'a'), (mul, step6, 'b')), (mul, step7, 'c')), (mul, step8, 'd')))]

   # Below handles a real world shader that looks like this mat4*mat4*vec4(xyz, 1.0) where the the multiplication of the 1.0 constant has been optimised away
   step5_no_w_mul = (add, (add, (add, (mul, 'q', 'gg'), (mul, 'r', 'hh')), (mul, 's', 'ii')), 't')
   step6_no_w_mul = (add, (add, (add, (mul, 'u', 'gg'), (mul, 'v', 'hh')), (mul, 'w', 'ii')), 'x')
   step7_no_w_mul = (add, (add, (add, (mul, 'y', 'gg'), (mul, 'z', 'hh')), (mul, 'aa', 'ii')), 'bb')
   step8_no_w_mul = (add, (add, (add, (mul, 'cc', 'gg'), (mul, 'dd', 'hh')), (mul, 'ee', 'ii')), 'ff')

   mat_mul_optimizations += [((add_first, (add, (add, (mul, step1, 'gg'), (mul, step2, 'hh')), (mul, step3, 'ii')), step4), (add, (add, (add, (mul, step5_no_w_mul, 'a'), (mul, step6_no_w_mul, 'b')), (mul, step7_no_w_mul, 'c')), (mul, step8_no_w_mul, 'd')))]

   # Below handles a real world shader that looks like this mat4*mat4*vec4(xy, 0.0, 1.0) where the the multiplication of the 0.0 and 1.0 constants have been optimised away
   step5_zero_z_no_w_mul = (add, (add, (mul, 'q', 'gg'), (mul, 'r', 'hh')), 't')
   step6_zero_z_no_w_mul = (add, (add, (mul, 'u', 'gg'), (mul, 'v', 'hh')), 'x')
   step7_zero_z_no_w_mul = (add, (add, (mul, 'y', 'gg'), (mul, 'z', 'hh')), 'bb')
   step8_zero_z_no_w_mul = (add, (add, (mul, 'cc', 'gg'), (mul, 'dd', 'hh')), 'ff')

   mat_mul_optimizations += [((add_first, (add, (mul, step1, 'gg'), step4), (mul, step2, 'hh')), (add, (add, (add, (mul, step5_zero_z_no_w_mul, 'a'), (mul, step6_zero_z_no_w_mul, 'b')), (mul, step7_zero_z_no_w_mul, 'c')), (mul, step8_zero_z_no_w_mul, 'd')))]

before_lower_int64_optimizations = [
    # The i2i64(a) implies that 'a' has at most 32-bits of data.
    (('ishl', ('i2i64', a), b),
     # Effective shift count of zero, just return 'a'.
     ('bcsel', ('ieq', ('iand', b, 63), 0), ('i2i64', a),
      ('bcsel', ('ilt', ('iand', b, 63), 32),
       # Shifting less than 32 bits, so both 32-bit halves will have
       # some data. These (and the else case) shift counts are of 32-bit
       # values, so the shift counts are implicitly moduolo 32.
       ('pack_64_2x32_split', ('ishl', ('i2i32', a), b), ('ishr', ('i2i32', a),          ('iadd', ('ineg', b), 32) )),
       # Shifting 32 bits or more, so lower 32 bits must be zero.
       ('pack_64_2x32_split', 0                        , ('ishl', ('i2i32', a), ('iabs', ('iadd', ('ineg', b), 32)))))),
     '(options->lower_int64_options & nir_lower_shift64) != 0', TestStatus.XFAIL),

    (('ishl', ('u2u64', a), b),
     ('bcsel', ('ieq', ('iand', b, 63), 0), ('u2u64', a),
      ('bcsel', ('ilt', ('iand', b, 63), 32),
       ('pack_64_2x32_split', ('ishl', ('u2u32', a), b), ('ushr', ('u2u32', a),          ('iadd', ('ineg', b), 32) )),
       ('pack_64_2x32_split', 0                        , ('ishl', ('u2u32', a), ('iabs', ('iadd', ('ineg', b), 32)))))),
     '(options->lower_int64_options & nir_lower_shift64) != 0', TestStatus.XFAIL),

    # If ineg64 is lowered, then the negation is not free. Try to eliminate
    # some of the negations.
    (('iadd@64', ('ineg', a), ('ineg(is_used_once)', b)), ('isub', ('ineg', a), b), '(options->lower_int64_options & nir_lower_ineg64) != 0'),
    (('iadd@64', a, ('ineg', b)), ('isub', a, b), '(options->lower_int64_options & nir_lower_ineg64) != 0'),
    (('isub@64', a, ('ineg', b)), ('iadd', a, b), '(options->lower_int64_options & nir_lower_ineg64) != 0'),
    (('isub@64', ('ineg', a), ('ineg', b)), ('isub', b, a), '(options->lower_int64_options & nir_lower_ineg64) != 0'),

    (('imul@64', ('ineg', a), ('ineg', b)), ('imul', a, b)),
    (('idiv@64', ('ineg', a), ('ineg', b)), ('idiv', a, b), 'true', TestStatus.XFAIL),

    # If the hardware can do int64, the shift is the same cost as the add. It
    # should be fine to do this transformation unconditionally.
    (('iadd', ('i2i64', a), ('i2i64', a)), ('ishl', ('i2i64', a), 1)),
    (('iadd', ('u2u64', a), ('u2u64', a)), ('ishl', ('u2u64', a), 1)),
]

reassoc_fma_optimizations = [
    # Try to reassociate fadd to make more adds have a fmul source
    (('~fadd', ('fadd(is_used_once)', 'a(is_fmul)', ('fadd(is_used_once)', 'b(is_fmul)', ('fadd(is_used_once)', 'c(is_fmul)', 'd(is_fmul)'))), 'e(is_not_fmul)'),
     ('fadd', a, ('fadd', b, ('fadd', c, ('fadd', d, e))))),
    (('~fadd', ('fadd(is_used_once)', 'a(is_fmul)', ('fadd(is_used_once)', 'b(is_fmul)', 'c(is_fmul)')), 'd(is_not_fmul)'),
     ('fadd', a, ('fadd', b, ('fadd', c, d)))),
    (('~fadd', ('fadd(is_used_once)', 'a(is_fmul)', 'b(is_fmul)'), 'c(is_not_fmul)'),
     ('fadd', a, ('fadd', b, c))),

    (('~fadd', ('fneg(is_used_once)', ('fadd(is_used_once)', 'a(is_fmul)', ('fadd(is_used_once)', 'b(is_fmul)', ('fadd(is_used_once)', 'c(is_fmul)', 'd(is_fmul)')))), 'e(is_not_fmul)'),
     ('fadd', ('fneg', a), ('fneg', ('fadd', b, ('fadd', c, ('fadd', d, ('fneg', e))))))),
    (('~fadd', ('fneg(is_used_once)', ('fadd(is_used_once)', 'a(is_fmul)', ('fadd(is_used_once)', 'b(is_fmul)', 'c(is_fmul)'))), 'd(is_not_fmul)'),
     ('fadd', ('fneg', a), ('fneg', ('fadd', b, ('fadd', c, ('fneg', d)))))),
    (('~fadd', ('fneg(is_used_once)', ('fadd(is_used_once)', 'a(is_fmul)', 'b(is_fmul)')), 'c(is_not_fmul)'),
     ('fadd', ('fneg', a), ('fadd', ('fneg', b), c))),
]

# Those optimizations try to reverse integer promotion found in e.g. OpenCL C. Those should be ran
# before any bit_size lowering is done.
integer_promotion_optimizations = []
for s in [8, 16]:
    u2u = 'u2u{}'.format(s)
    aN = 'a@{}'.format(s)
    bN = 'b@{}'.format(s)

    for op in ['ineg', 'inot']:
        integer_promotion_optimizations.extend([
            ((u2u, (op, 'a@32')), (op, (u2u, a))),
        ])

    for op in ['iadd', 'imul', 'iand', 'ior', 'ixor']:
        integer_promotion_optimizations.extend([
            ((u2u, (op, 'a@32', 'b@32')), (op, (u2u, a), (u2u, b))),
        ])

    # idiv and irem are more restrictive because we can't simply trim the inputs arbitrarily.
    for op in ['idiv', 'irem']:
        integer_promotion_optimizations.extend([
            ((u2u, (op, ('i2i32', aN), ('i2i32', bN))), (op, a, b)),
        ])

parser = argparse.ArgumentParser()
parser.add_argument('--out', required=True)
parser.add_argument('--out-tests')
# glsl-optimizer change: emit only the listed passes (comma separated)
parser.add_argument('--passes')
args = parser.parse_args()

build_tests = args.out_tests is not None

passes = []

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_algebraic",
    optimizations,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_algebraic_before_ffma",
    before_ffma_optimizations,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_algebraic_before_lower_int64",
    before_lower_int64_optimizations,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_algebraic_late",
    late_optimizations,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_algebraic_distribute_src_mods",
    distribute_src_mods,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_algebraic_integer_promotion",
    integer_promotion_optimizations,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_reassociate_matrix_mul",
    mat_mul_optimizations,
    build_tests=build_tests
))

passes.append(nir_algebraic.AlgebraicPass(
    "nir_opt_reassociate_for_fma",
    reassoc_fma_optimizations,
    build_tests=build_tests
))

# glsl-optimizer change: --passes filter
if args.passes:
    wanted = args.passes.split(',')
    passes = [p for p in passes if p.pass_name in wanted]
    assert len(passes) == len(wanted), "unknown pass in --passes"

if build_tests:
    with open(args.out_tests, "w", encoding='utf-8') as f:
        for p in passes:
            f.write(p.render_tests())

with open(args.out, "w", encoding='utf-8') as f:
    for p in passes:
        f.write(p.render())
