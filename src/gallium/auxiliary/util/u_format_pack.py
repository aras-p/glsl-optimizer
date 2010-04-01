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
import math

from u_format_parse import *


def generate_f16_to_f32():
    '''Naive implementation, need something faster that operates on bits'''

    print '''
static float
f16_to_f32(uint16_t h)
{
    unsigned mantissa = h & 0x3ff;
    unsigned exponent = (h >> 10) & 0x1f;
    float sign = (h & 0x8000) ? -1.0f : 1.0f;

    if (exponent == 0) {
        if (mantissa == 0) {
            return sign * 0.0f;
        }
        return sign * powf(2.0f, -14.0f) * (float)mantissa / 1024.0f;
    }
    if (exponent == 31) {
        if (mantissa == 0) {
            /* XXX: infinity */
            return sign * 100000.0f;
        }
        /* XXX: NaN */
        return 1000.0f;
    }
    return sign * powf(2.0f, (float)exponent - 15.0f) * (1.0f + (float)mantissa / 1024.0f);
}
'''

def generate_f32_to_f16():
    print '''
static uint16_t
f32_to_f16(float f)
{
    /* TODO */
    return 0;
}
'''

def generate_format_type(format):
    '''Generate a structure that describes the format.'''

    print 'union util_format_%s {' % format.short_name()
    if format.is_bitmask() or format.short_name() == "r11g11b10_float":
        print '   uint%u_t value;' % (format.block_size(),)
    print '   struct {'
    for channel in format.channels:
        if (format.is_bitmask() or format.is_mixed()) and not format.is_array() or format.short_name() == "r11g11b10_float":
            if channel.type == VOID:
                if channel.size:
                    print '      unsigned %s:%u;' % (channel.name, channel.size)
            elif channel.type == UNSIGNED:
                print '      unsigned %s:%u;' % (channel.name, channel.size)
            elif channel.type == SIGNED:
                print '      int %s:%u;' % (channel.name, channel.size)
            elif channel.type == FLOAT:
                if channel.size == 32:
                    print '      float %s;' % (channel.name)
                else:
                    print '      unsigned %s:%u;' % (channel.name, channel.size)
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


def generate_srgb_tables():
    print 'static ubyte srgb_to_linear[256] = {'
    for i in range(256):
        print '   %s,' % (int(math.pow((i / 255.0 + 0.055) / 1.055, 2.4) * 255))
    print '};'
    print
    print 'static ubyte linear_to_srgb[256] = {'
    print '   0,'
    for i in range(1, 256):
        print '   %s,' % (int((1.055 * math.pow(i / 255.0, 0.41666) - 0.055) * 255))
    print '};'
    print


def bswap_format(format):
    '''Generate a structure that describes the format.'''

    if format.is_bitmask() and not format.is_array() and format.block_size() > 8:
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
            channel = format.channels[0]
            if channel.type in (UNSIGNED, VOID):
                return 'uint%u_t' % channel.size
            elif channel.type == SIGNED:
                return 'int%u_t' % channel.size
            elif channel.type == FLOAT:
                if channel.size == 16:
                    return 'uint16_t'
                elif channel.size == 32:
                    return 'float'
                elif channel.size == 64:
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


def clamp_expr(src_channel, dst_channel, dst_native_type, value):
    '''Generate the expression to clamp the value in the source type to the
    destination type range.'''

    if src_channel == dst_channel:
        return value

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
        if src_channel.size == dst_channel.size:
            return value
        if src_channel.size == 64:
            value = '(float)%s' % (value)
        elif src_channel.size == 16:
            value = 'f16_to_f32(%s)' % (value)
        if dst_channel.size == 16:
            value = 'f32_to_f16(%s)' % (value)
        elif dst_channel.size == 64:
            value = '(double)%s' % (value)
        return value
    
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

    if src_channel.type in (SIGNED, UNSIGNED) and dst_channel.type in (SIGNED, UNSIGNED):
        if not src_channel.norm and not dst_channel.norm:
            # neither is normalized -- just cast
            return '(%s)%s' % (dst_native_type, value)

        src_one = get_one(src_channel)
        dst_one = get_one(dst_channel)

        if src_one > dst_one and src_channel.norm and dst_channel.norm:
            # We can just bitshift
            src_shift = get_one_shift(src_channel)
            dst_shift = get_one_shift(dst_channel)
            value = '(%s >> %s)' % (value, src_shift - dst_shift)
        else:
            # We need to rescale using an intermediate type big enough to hold the multiplication of both
            tmp_native_type = intermediate_native_type(src_channel.size + dst_channel.size, src_channel.sign and dst_channel.sign)
            value = '((%s)%s)' % (tmp_native_type, value)
            value = '(%s * 0x%x / 0x%x)' % (value, dst_one, src_one)
        value = '(%s)%s' % (dst_native_type, value)
        return value

    assert False


def generate_unpack_kernel(format, dst_channel, dst_native_type):

    if not is_format_supported(format):
        return
    
    assert format.layout == PLAIN

    src_native_type = native_type(format)

    if format.is_bitmask():
        depth = format.block_size()
        print '         uint%u_t value = *(uint%u_t *)src;' % (depth, depth) 

        # Declare the intermediate variables
        for i in range(format.nr_channels()):
            src_channel = format.channels[i]
            if src_channel.type == UNSIGNED:
                print '         uint%u_t %s;' % (depth, src_channel.name)
            elif src_channel.type == SIGNED:
                print '         int%u_t %s;' % (depth, src_channel.name)

        if depth > 8:
            print '#ifdef PIPE_ARCH_BIG_ENDIAN'
            print '         value = util_bswap%u(value);' % depth
            print '#endif'

        # Compute the intermediate unshifted values 
        shift = 0
        for i in range(format.nr_channels()):
            src_channel = format.channels[i]
            value = 'value'
            if src_channel.type == UNSIGNED:
                if shift:
                    value = '%s >> %u' % (value, shift)
                if shift + src_channel.size < depth:
                    value = '(%s) & 0x%x' % (value, (1 << src_channel.size) - 1)
            elif src_channel.type == SIGNED:
                if shift + src_channel.size < depth:
                    # Align the sign bit
                    lshift = depth - (shift + src_channel.size)
                    value = '%s << %u' % (value, lshift)
                # Cast to signed
                value = '(int%u_t)(%s) ' % (depth, value)
                if src_channel.size < depth:
                    # Align the LSB bit
                    rshift = depth - src_channel.size
                    value = '(%s) >> %u' % (value, rshift)
            else:
                value = None
                
            if value is not None:
                print '         %s = %s;' % (src_channel.name, value)
                
            shift += src_channel.size

        # Convert, swizzle, and store final values
        for i in range(4):
            swizzle = format.swizzles[i]
            if swizzle < 4:
                src_channel = format.channels[swizzle]
                value = src_channel.name 
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
            print '         dst[%u] = %s; /* %s */' % (i, value, 'rgba'[i])
        
    else:
        print '         union util_format_%s pixel;' % format.short_name()
        print '         memcpy(&pixel, src, sizeof pixel);'
        bswap_format(format)
    
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
            print '         dst[%u] = %s; /* %s */' % (i, value, 'rgba'[i])
    

def generate_pack_kernel(format, src_channel, src_native_type):

    if not is_format_supported(format):
        return
    
    dst_native_type = native_type(format)

    assert format.layout == PLAIN

    inv_swizzle = format.inv_swizzles()

    if format.is_bitmask():
        depth = format.block_size()
        print '         uint%u_t value = 0;' % depth 

        shift = 0
        for i in range(4):
            dst_channel = format.channels[i]
            if inv_swizzle[i] is not None:
                value ='src[%u]' % inv_swizzle[i]
                value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
                if format.colorspace == ZS:
                    if i == 3:
                        value = get_one(dst_channel)
                    elif i >= 1:
                        value = '0'
                if dst_channel.type in (UNSIGNED, SIGNED):
                    if shift + dst_channel.size < depth:
                        value = '(%s) & 0x%x' % (value, (1 << dst_channel.size) - 1)
                    if shift:
                        value = '(%s) << %u' % (value, shift)
                    if dst_channel.type == SIGNED:
                        # Cast to unsigned
                        value = '(uint%u_t)(%s) ' % (depth, value)
                else:
                    value = None
                if value is not None:
                    print '         value |= %s;' % (value)
                
            shift += dst_channel.size

        if depth > 8:
            print '#ifdef PIPE_ARCH_BIG_ENDIAN'
            print '         value = util_bswap%u(value);' % depth
            print '#endif'
        
        print '         *(uint%u_t *)dst = value;' % depth 

    else:
        print '         union util_format_%s pixel;' % format.short_name()
    
        for i in range(4):
            dst_channel = format.channels[i]
            width = dst_channel.size
            if inv_swizzle[i] is None:
                continue
            value ='src[%u]' % inv_swizzle[i]
            value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
            if format.colorspace == ZS:
                if i == 3:
                    value = get_one(dst_channel)
                elif i >= 1:
                    value = '0'
            print '         pixel.chan.%s = %s;' % (dst_channel.name, value)
    
        bswap_format(format)
        print '         memcpy(dst, &pixel, sizeof pixel);'
    

def generate_format_unpack(format, dst_channel, dst_native_type, dst_suffix):
    '''Generate the function to unpack pixels from a particular format'''

    name = format.short_name()

    print 'static INLINE void'
    print 'util_format_%s_unpack_%s(%s *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)' % (name, dst_suffix, dst_native_type)
    print '{'

    if is_format_supported(format):
        print '   unsigned x, y;'
        print '   for(y = 0; y < height; y += %u) {' % (format.block_height,)
        print '      %s *dst = dst_row;' % (dst_native_type)
        print '      const uint8_t *src = src_row;'
        print '      for(x = 0; x < width; x += %u) {' % (format.block_width,)
        
        generate_unpack_kernel(format, dst_channel, dst_native_type)
    
        print '         src += %u;' % (format.block_size() / 8,)
        print '         dst += 4;'
        print '      }'
        print '      src_row += src_stride;'
        print '      dst_row += dst_stride/sizeof(*dst_row);'
        print '   }'

    print '}'
    print
    

def generate_format_pack(format, src_channel, src_native_type, src_suffix):
    '''Generate the function to pack pixels to a particular format'''

    name = format.short_name()

    print 'static INLINE void'
    print 'util_format_%s_pack_%s(uint8_t *dst_row, unsigned dst_stride, const %s *src_row, unsigned src_stride, unsigned width, unsigned height)' % (name, src_suffix, src_native_type)
    print '{'
    
    if is_format_supported(format):
        print '   unsigned x, y;'
        print '   for(y = 0; y < height; y += %u) {' % (format.block_height,)
        print '      const %s *src = src_row;' % (src_native_type)
        print '      uint8_t *dst = dst_row;'
        print '      for(x = 0; x < width; x += %u) {' % (format.block_width,)
    
        generate_pack_kernel(format, src_channel, src_native_type)
            
        print '         src += 4;'
        print '         dst += %u;' % (format.block_size() / 8,)
        print '      }'
        print '      dst_row += dst_stride;'
        print '      src_row += src_stride/sizeof(*src_row);'
        print '   }'
        
    print '}'
    print
    

def generate_format_fetch(format, dst_channel, dst_native_type, dst_suffix):
    '''Generate the function to unpack pixels from a particular format'''

    name = format.short_name()

    print 'static INLINE void'
    print 'util_format_%s_fetch_%s(%s *dst, const uint8_t *src, unsigned i, unsigned j)' % (name, dst_suffix, dst_native_type)
    print '{'

    if is_format_supported(format):
        generate_unpack_kernel(format, dst_channel, dst_native_type)

    print '}'
    print


def generate(formats):
    print
    print '#include "pipe/p_compiler.h"'
    print '#include "u_math.h"'
    print '#include "u_format.h"'
    print

    generate_f16_to_f32()
    generate_f32_to_f16()

    for format in formats:
        if is_format_supported(format):
            generate_format_type(format)

    channel = Channel(FLOAT, False, 32)
    native_type = 'float'
    suffix = 'float'

    for format in formats:
        if format.layout != 's3tc':
            generate_format_unpack(format, channel, native_type, suffix)
            generate_format_pack(format, channel, native_type, suffix)
            generate_format_fetch(format, channel, native_type, suffix)

    channel = Channel(UNSIGNED, True, 8)
    native_type = 'uint8_t'
    suffix = '8unorm'

    for format in formats:
        if format.layout != 's3tc':
            generate_format_unpack(format, channel, native_type, suffix)
            generate_format_pack(format, channel, native_type, suffix)

