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
 * Pixel format accessor functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */
'''


import math
import sys

from u_format_pack import *


def is_format_supported(format):
    '''Determines whether we actually have the plumbing necessary to generate the 
    to read/write to/from this format.'''

    # FIXME: Ideally we would support any format combination here.

    # XXX: It should be straightforward to support srgb
    if format.colorspace not in ('rgb', 'zs'):
        return False

    if format.layout != PLAIN:
        return False

    for i in range(4):
        channel = format.channels[i]
        if channel.type not in (VOID, UNSIGNED, FLOAT):
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
            if channel.type == UNSIGNED:
                return 'uint%u_t' % channel.size
            elif channel.type == SIGNED:
                return 'int%u_t' % channel.size
            elif channel.type == FLOAT:
                if channel.size == 32:
                    return 'float'
                elif channel.size == 64:
                    return 'double'
                else:
                    assert False
            else:
                assert False
    else:
        assert False


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


def generate_format_read(format, dst_channel, dst_native_type, dst_suffix):
    '''Generate the function to read pixels from a particular format'''

    name = format.short_name()

    src_native_type = native_type(format)

    print 'static void'
    print 'util_format_%s_read_%s(%s *dst, unsigned dst_stride, const uint8_t *src, unsigned src_stride, unsigned x0, unsigned y0, unsigned w, unsigned h)' % (name, dst_suffix, dst_native_type)
    print '{'
    print '   unsigned x, y;'
    print '   const uint8_t *src_row = src + y0*src_stride;'
    print '   %s *dst_row = dst;' % dst_native_type
    print '   for (y = 0; y < h; ++y) {'
    print '      const %s *src_pixel = (const %s *)(src_row + x0*%u);' % (src_native_type, src_native_type, format.stride())
    print '      %s *dst_pixel = dst_row;' %dst_native_type
    print '      for (x = 0; x < w; ++x) {'

    names = ['']*4
    if format.colorspace == 'rgb':
        for i in range(4):
            swizzle = format.swizzles[i]
            if swizzle < 4:
                names[swizzle] += 'rgba'[i]
    elif format.colorspace == 'zs':
        swizzle = format.swizzles[0]
        if swizzle < 4:
            names[swizzle] = 'z'
        else:
            assert False
    else:
        assert False

    if format.layout == PLAIN:
        if not format.is_array():
            print '         %s pixel = *src_pixel++;' % src_native_type
            shift = 0;
            for i in range(4):
                src_channel = format.channels[i]
                width = src_channel.size
                if names[i]:
                    value = 'pixel'
                    mask = (1 << width) - 1
                    if shift:
                        value = '(%s >> %u)' % (value, shift)
                    if shift + width < format.block_size():
                        value = '(%s & 0x%x)' % (value, mask)
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
                    print '         %s %s = %s;' % (dst_native_type, names[i], value)
                shift += width
        else:
            for i in range(4):
                src_channel = format.channels[i]
                if names[i]:
                    value = 'src_pixel[%u]' % i
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
                    print '         %s %s = %s;' % (dst_native_type, names[i], value)
            print '         src_pixel += %u;' % (format.nr_channels())
    else:
        assert False

    for i in range(4):
        if format.colorspace == 'rgb':
            swizzle = format.swizzles[i]
            if swizzle < 4:
                value = names[swizzle]
            elif swizzle == SWIZZLE_0:
                value = '0'
            elif swizzle == SWIZZLE_1:
                value = get_one(dst_channel)
            else:
                assert False
        elif format.colorspace == 'zs':
            if i < 3:
                value = 'z'
            else:
                value = get_one(dst_channel)
        else:
            assert False
        print '         *dst_pixel++ = %s; /* %s */' % (value, 'rgba'[i])

    print '      }'
    print '      src_row += src_stride;'
    print '      dst_row += dst_stride/sizeof(*dst_row);'
    print '   }'
    print '}'
    print
    

def generate_format_write(format, src_channel, src_native_type, src_suffix):
    '''Generate the function to write pixels to a particular format'''

    name = format.short_name()

    dst_native_type = native_type(format)

    print 'static void'
    print 'util_format_%s_write_%s(const %s *src, unsigned src_stride, uint8_t *dst, unsigned dst_stride, unsigned x0, unsigned y0, unsigned w, unsigned h)' % (name, src_suffix, src_native_type)
    print '{'
    print '   unsigned x, y;'
    print '   uint8_t *dst_row = dst + y0*dst_stride;'
    print '   const %s *src_row = src;' % src_native_type
    print '   for (y = 0; y < h; ++y) {'
    print '      %s *dst_pixel = (%s *)(dst_row + x0*%u);' % (dst_native_type, dst_native_type, format.stride())
    print '      const %s *src_pixel = src_row;' %src_native_type
    print '      for (x = 0; x < w; ++x) {'

    inv_swizzle = format.inv_swizzles()

    if format.layout == PLAIN:
        if not format.is_array():
            print '         %s pixel = 0;' % dst_native_type
            shift = 0;
            for i in range(4):
                dst_channel = format.channels[i]
                width = dst_channel.size
                if inv_swizzle[i] is not None:
                    value = 'src_pixel[%u]' % inv_swizzle[i]
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
                    if shift:
                        value = '(%s << %u)' % (value, shift)
                    print '         pixel |= %s;' % value
                shift += width
            print '         *dst_pixel++ = pixel;'
        else:
            for i in range(4):
                dst_channel = format.channels[i]
                if inv_swizzle[i] is not None:
                    value = 'src_pixel[%u]' % inv_swizzle[i]
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value)
                    print '         *dst_pixel++ = %s;' % value
    else:
        assert False
    print '         src_pixel += 4;'

    print '      }'
    print '      dst_row += dst_stride;'
    print '      src_row += src_stride/sizeof(*src_row);'
    print '   }'
    print '}'
    print
    

def generate_read(formats, dst_channel, dst_native_type, dst_suffix):
    '''Generate the dispatch function to read pixels from any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_read(format, dst_channel, dst_native_type, dst_suffix)

    print 'void'
    print 'util_format_read_%s(enum pipe_format format, %s *dst, unsigned dst_stride, const void *src, unsigned src_stride, unsigned x, unsigned y, unsigned w, unsigned h)' % (dst_suffix, dst_native_type)
    print '{'
    print '   void (*func)(%s *dst, unsigned dst_stride, const uint8_t *src, unsigned src_stride, unsigned x0, unsigned y0, unsigned w, unsigned h);' % dst_native_type
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            print '      func = &util_format_%s_read_%s;' % (format.short_name(), dst_suffix)
            print '      break;'
    print '   default:'
    print '      debug_printf("unsupported format\\n");'
    print '      return;'
    print '   }'
    print '   func(dst, dst_stride, (const uint8_t *)src, src_stride, x, y, w, h);'
    print '}'
    print


def generate_write(formats, src_channel, src_native_type, src_suffix):
    '''Generate the dispatch function to write pixels to any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_write(format, src_channel, src_native_type, src_suffix)

    print 'void'
    print 'util_format_write_%s(enum pipe_format format, const %s *src, unsigned src_stride, void *dst, unsigned dst_stride, unsigned x, unsigned y, unsigned w, unsigned h)' % (src_suffix, src_native_type)
    
    print '{'
    print '   void (*func)(const %s *src, unsigned src_stride, uint8_t *dst, unsigned dst_stride, unsigned x0, unsigned y0, unsigned w, unsigned h);' % src_native_type
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            print '      func = &util_format_%s_write_%s;' % (format.short_name(), src_suffix)
            print '      break;'
    print '   default:'
    print '      debug_printf("unsupported format\\n");'
    print '      return;'
    print '   }'
    print '   func(src, src_stride, (uint8_t *)dst, dst_stride, x, y, w, h);'
    print '}'
    print


def main():
    formats = []
    for arg in sys.argv[1:]:
        formats.extend(parse(arg))

    print '/* This file is autogenerated by u_format_access.py from u_format.csv. Do not edit directly. */'
    print
    # This will print the copyright message on the top of this file
    print __doc__.strip()
    print
    print '#include "pipe/p_compiler.h"'
    print '#include "u_math.h"'
    print '#include "u_format_pack.h"'
    print

    generate_srgb_tables()

    type = Channel(FLOAT, False, 32)
    native_type = 'float'
    suffix = '4f'

    generate_read(formats, type, native_type, suffix)
    generate_write(formats, type, native_type, suffix)

    type = Channel(UNSIGNED, True, 8)
    native_type = 'uint8_t'
    suffix = '4ub'

    generate_read(formats, type, native_type, suffix)
    generate_write(formats, type, native_type, suffix)


if __name__ == '__main__':
    main()
