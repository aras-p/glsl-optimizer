#!/usr/bin/env python

'''
/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Pixel format packing and unpacking functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */
'''


import sys

from u_format_parse import *


def generate_format_type(format):
    '''Generate a structure that describes the format.'''

    print 'union util_format_%s {' % format.short_name()
    if format.is_bitmask():
        print '   uint%u_t value;' % (format.block_size(),)
    print '   struct {'
    for channel in format.channels:
        if format.is_bitmask() and not format.is_array():
            if channel.type == VOID:
                if channel.size:
                    print '      unsigned %s:%u;' % (channel.name, channel.size)
            elif channel.type == UNSIGNED:
                print '      unsigned %s:%u;' % (channel.name, channel.size)
            elif channel.type == SIGNED:
                print '      int %s:%u;' % (channel.name, channel.size)
            else:
                assert 0
        else:
            assert channel.size % 8 == 0 and is_pot(channel.size)
            if channel.type == VOID:
                if channel.size:
                    print '      uint%u_t %s;' % (channel.size, channel.name)
            elif channel.type == UNSIGNED:
                print '      uint%u_t %s;' % (channel.size, channel.name)
            elif channel.type in (SIGNED, FIXED):
                print '      int%u_t %s;' % (channel.size, channel.name)
            elif channel.type == FLOAT:
                if channel.size == 64:
                    print '      double %s;' % (channel.name)
                elif channel.size == 32:
                    print '      float %s;' % (channel.name)
                elif channel.size == 16:
                    print '      uint16_t %s;' % (channel.name)
                else:
                    assert 0
            else:
                assert 0
    print '   } chan;'
    print '};'
    print


def bswap_format(format):
    '''Generate a structure that describes the format.'''

    if format.is_bitmask() and not format.is_array():
        print '#ifdef PIPE_ARCH_BIG_ENDIAN'
        print '   pixel.value = util_bswap%u(pixel.value);' % format.block_size()
        print '#endif'


def is_format_supported(format):
    '''Determines whether we actually have the plumbing necessary to generate the 
    to read/write to/from this format.'''

    # FIXME: Ideally we would support any format combination here.

    if format.layout != PLAIN:
        return False

    for i in range(4):
        channel = format.channels[i]
        if channel.type not in (VOID, UNSIGNED, SIGNED, FLOAT):
            return False

    # We can only read a color from a depth/stencil format if the depth channel is present
    if format.colorspace == 'zs' and format.swizzles[0] == SWIZZLE_NONE:
        return False

    return True


def native_type(format):
    '''Get the native appropriate for a format.'''

    if format.layout == PLAIN:
        if not format.is_array():
            # For arithmetic pixel formats return the integer type that matches the whole pixel
            return 'uint%u_t' % format.block_size()
        else:
            # For array pixel formats return the integer type that matches the color channel
            type = format.channels[0]
            if type.type == UNSIGNED:
                return 'uint%u_t' % type.size
            elif type.type == SIGNED:
                return 'int%u_t' % type.size
            elif type.type == FLOAT:
                if type.size == 32:
                    return 'float'
                elif type.size == 64:
                    return 'double'
                else:
                    assert False
            else:
                assert False
    else:
        assert False


def intermediate_native_type(bits, sign):
    '''Find a native type adequate to hold intermediate results of the request bit size.'''

    bytes = 4 # don't use anything smaller than 32bits
    while bytes * 8 < bits:
        bytes *= 2
    bits = bytes*8

    if sign:
        return 'int%u_t' % bits
    else:
        return 'uint%u_t' % bits


def get_one_shift(type):
    '''Get the number of the bit that matches unity for this type.'''
    if type.type == 'FLOAT':
        assert False
    if not type.norm:
        return 0
    if type.type == UNSIGNED:
        return type.size
    if type.type == SIGNED:
        return type.size - 1
    if type.type == FIXED:
        return type.size / 2
    assert False


def get_one(type):
    '''Get the value of unity for this type.'''
    if type.type == 'FLOAT' or not type.norm:
        return 1
    else:
        return (1 << get_one_shift(type)) - 1


def generate_clamp():
    '''Code generate the clamping functions for each type.

    We don't use a macro so that arguments with side effects, 
    like *src_pixel++ are correctly handled.
    '''

    for suffix, native_type in [
        ('', 'double'),
        ('f', 'float'),
        ('ui', 'unsigned int'),
        ('si', 'int'),
    ]:
        print 'static INLINE %s' % native_type
        print 'clamp%s(%s value, %s lbound, %s ubound)' % (suffix, native_type, native_type, native_type)
        print '{'
        print '   if(value < lbound)'
        print '      return lbound;'
        print '   if(value > ubound)'
        print '      return ubound;'
        print '   return value;'
        print '}'
        print


def clamp_expr(src_channel, dst_channel, dst_native_type, value):
    '''Generate the expression to clamp the value in the source type to the
    destination type range.'''

    if src_channel == dst_channel:
        return value

    # Pick the approriate clamp function
    if src_channel.type == FLOAT:
        if src_channel.size == 32:
            func = 'clampf'
        elif src_channel.size == 64:
            func = 'clamp'
        else:
            assert False
    elif src_channel.type == UNSIGNED:
        func = 'clampui'
    elif src_channel.type == SIGNED:
        func = 'clampsi'
    else:
        assert False

    src_min = src_channel.min()
    src_max = src_channel.max()
    dst_min = dst_channel.min()
    dst_max = dst_channel.max()

    if src_min < dst_min and src_max > dst_max:
        return 'CLAMP(%s, %s, %s)' % (value, dst_min, dst_max)

    if src_max > dst_max:
        return 'MIN2(%s, %s)' % (value, dst_max)
        
    if src_min < dst_min:
        return 'MAX2(%s, %s)' % (value, dst_min)

    return value


def conversion_expr(src_channel, dst_channel, dst_native_type, value, clamp=True):
    '''Generate the expression to convert a value between two types.'''

    if src_channel == dst_channel:
        return value

    if src_channel.type == FLOAT and dst_channel.type == FLOAT:
        return '(%s)%s' % (dst_native_type, value)
    
    if not src_channel.norm and not dst_channel.norm:
        return '(%s)%s' % (dst_native_type, value)

    if clamp:
        value = clamp_expr(src_channel, dst_channel, dst_native_type, value)

    if dst_channel.type == FLOAT:
        if src_channel.norm:
            one = get_one(src_channel)
            if src_channel.size <= 23:
                scale = '(1.0f/0x%x)' % one
            else:
                # bigger than single precision mantissa, use double
                scale = '(1.0/0x%x)' % one
            value = '(%s * %s)' % (value, scale)
        return '(%s)%s' % (dst_native_type, value)

    if src_channel.type == FLOAT:
        if dst_channel.norm:
            dst_one = get_one(dst_channel)
            if dst_channel.size <= 23:
                scale = '0x%x' % dst_one
            else:
                # bigger than single precision mantissa, use double
                scale = '(double)0x%x' % dst_one
            value = '(%s * %s)' % (value, scale)
        return '(%s)%s' % (dst_native_type, value)

    if not src_channel.norm and not dst_channel.norm:
        # neither is normalized -- just cast
        return '(%s)%s' % (dst_native_type, value)

    if src_channel.type in (SIGNED, UNSIGNED) and dst_channel.type in (SIGNED, UNSIGNED):
        src_one = get_one(src_channel)
        dst_one = get_one(dst_channel)

        if src_one > dst_one and src_channel.norm:
            # We can just bitshift
            src_shift = get_one_shift(src_channel)
            dst_shift = get_one_shift(dst_channel)
            value = '(%s >> %s)' % (value, src_shift - dst_shift)
        else:
            # We need to rescale using an intermediate type big enough to hold the multiplication of both
            tmp_native_type = intermediate_native_type(src_channel.size + dst_channel.size, src_channel.sign and dst_channel.sign)
            value = '(%s)%s' % (tmp_native_type, value)
            value = '(%s * 0x%x / 0x%x)' % (value, dst_one, src_one)
        value = '(%s)%s' % (dst_native_type, value)
        return value

    assert False


def generate_format_unpack(format, dst_channel, dst_native_type, dst_suffix):
    '''Generate the function to unpack pixels from a particular format'''

    name = format.short_name()

    src_native_type = native_type(format)

    print 'static INLINE void'
    print 'util_format_%s_unpack_%s(%s *dst, const void *src)' % (name, dst_suffix, dst_native_type)
    print '{'
    print '   union util_format_%s pixel;' % format.short_name()
    print '   memcpy(&pixel, src, sizeof pixel);'
    bswap_format(format)

    assert format.layout == PLAIN

    for i in range(4):
        swizzle = format.swizzles[i]
        if swizzle < 4:
            src_channel = format.channels[swizzle]
            value = 'pixel.chan.%s' % src_channel.name 
            value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
        elif swizzle == SWIZZLE_0:
            value = '0'
        elif swizzle == SWIZZLE_1:
            value = get_one(dst_channel)
        elif swizzle == SWIZZLE_NONE:
            value = '0'
        else:
            assert False
        if format.colorspace == ZS:
            if i == 3:
                value = get_one(dst_channel)
            elif i >= 1:
                value = 'dst[0]'
        print '   dst[%u] = %s; /* %s */' % (i, value, 'rgba'[i])

    print '}'
    print
    

def generate_format_pack(format, src_channel, src_native_type, src_suffix):
    '''Generate the function to pack pixels to a particular format'''

    name = format.short_name()

    dst_native_type = native_type(format)

    print 'static INLINE void'
    print 'util_format_%s_pack_%s(void *dst, %s r, %s g, %s b, %s a)' % (name, src_suffix, src_native_type, src_native_type, src_native_type, src_native_type)
    print '{'
    print '   union util_format_%s pixel;' % format.short_name()

    assert format.layout == PLAIN

    inv_swizzle = format.inv_swizzles()

    for i in range(4):
        dst_channel = format.channels[i]
        width = dst_channel.size
        if inv_swizzle[i] is None:
            continue
        value = 'rgba'[inv_swizzle[i]]
        value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
        if format.colorspace == ZS:
            if i == 3:
                value = get_one(dst_channel)
            elif i >= 1:
                value = '0'
        print '   pixel.chan.%s = %s;' % (dst_channel.name, value)

    bswap_format(format)
    print '   memcpy(dst, &pixel, sizeof pixel);'
    print '}'
    print
    

def generate_unpack(formats, dst_channel, dst_native_type, dst_suffix):
    '''Generate the dispatch function to unpack pixels from any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_unpack(format, dst_channel, dst_native_type, dst_suffix)

    print 'static INLINE void'
    print 'util_format_unpack_%s(enum pipe_format format, %s *dst, const void *src)' % (dst_suffix, dst_native_type)
    print '{'
    print '   void (*func)(%s *dst, const void *src);' % dst_native_type
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            print '      func = &util_format_%s_unpack_%s;' % (format.short_name(), dst_suffix)
            print '      break;'
    print '   default:'
    print '      debug_printf("unsupported format\\n");'
    print '      return;'
    print '   }'
    print '   func(dst, src);'
    print '}'
    print


def generate_pack(formats, src_channel, src_native_type, src_suffix):
    '''Generate the dispatch function to pack pixels to any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_pack(format, src_channel, src_native_type, src_suffix)

    print 'static INLINE void'
    print 'util_format_pack_%s(enum pipe_format format, void *dst, %s r, %s g, %s b, %s a)' % (src_suffix, src_native_type, src_native_type, src_native_type, src_native_type)
    print '{'
    print '   void (*func)(void *dst, %s r, %s g, %s b, %s a);' % (src_native_type, src_native_type, src_native_type, src_native_type)
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            print '      func = &util_format_%s_pack_%s;' % (format.short_name(), src_suffix)
            print '      break;'
    print '   default:'
    print '      debug_printf("%s: unsupported format\\n", __FUNCTION__);'
    print '      return;'
    print '   }'
    print '   func(dst, r, g, b, a);'
    print '}'
    print


def main():
    formats = []
    for arg in sys.argv[1:]:
        formats.extend(parse(arg))

    print '/* This file is autogenerated by u_format_pack.py from u_format.csv. Do not edit directly. */'
    print
    # This will print the copyright message on the top of this file
    print __doc__.strip()

    print
    print '#ifndef U_FORMAT_PACK_H'
    print '#define U_FORMAT_PACK_H'
    print
    print '#include "pipe/p_compiler.h"'
    print '#include "u_math.h"'
    print '#include "u_format.h"'
    print

    generate_clamp()

    for format in formats:
        if format.layout == PLAIN:
            generate_format_type(format)

    channel = Channel(FLOAT, False, 32)
    native_type = 'float'
    suffix = '4f'

    generate_unpack(formats, channel, native_type, suffix)
    generate_pack(formats, channel, native_type, suffix)

    channel = Channel(UNSIGNED, True, 8)
    native_type = 'uint8_t'
    suffix = '4ub'

    generate_unpack(formats, channel, native_type, suffix)
    generate_pack(formats, channel, native_type, suffix)

    print
    print '#ifdef __cplusplus'
    print '}'
    print '#endif'
    print
    print '#endif /* ! U_FORMAT_PACK_H */'


if __name__ == '__main__':
    main()
