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


import sys
import os.path

sys.path.insert(0, os.path.join(os.path.dirname(sys.argv[0]), '../../auxiliary/util'))

from u_format_access import *


def generate_format_read(format, dst_type, dst_native_type, dst_suffix):
    '''Generate the function to read pixels from a particular format'''

    name = short_name(format)

    src_native_type = native_type(format)

    print 'static void'
    print 'lp_tile_%s_read_%s(%s *dst, const uint8_t *src, unsigned src_stride, unsigned x0, unsigned y0, unsigned w, unsigned h)' % (name, dst_suffix, dst_native_type)
    print '{'
    print '   unsigned x, y;'
    print '   const uint8_t *src_row = src + y0*src_stride;'
    print '   for (y = 0; y < h; ++y) {'
    print '      const %s *src_pixel = (const %s *)(src_row + x0*%u);' % (src_native_type, src_native_type, format.stride())
    print '      for (x = 0; x < w; ++x) {'

    names = ['']*4
    if format.colorspace == 'rgb':
        for i in range(4):
            swizzle = format.out_swizzle[i]
            if swizzle < 4:
                names[swizzle] += 'rgba'[i]
    elif format.colorspace == 'zs':
        swizzle = format.out_swizzle[0]
        if swizzle < 4:
            names[swizzle] = 'z'
        else:
            assert False
    else:
        assert False

    if format.layout == ARITH:
        print '         %s pixel = *src_pixel++;' % src_native_type
        shift = 0;
        for i in range(4):
            src_type = format.in_types[i]
            width = src_type.size
            if names[i]:
                value = 'pixel'
                mask = (1 << width) - 1
                if shift:
                    value = '(%s >> %u)' % (value, shift)
                if shift + width < format.block_size():
                    value = '(%s & 0x%x)' % (value, mask)
                value = conversion_expr(src_type, dst_type, dst_native_type, value)
                print '         %s %s = %s;' % (dst_native_type, names[i], value)
            shift += width
    elif format.layout == ARRAY:
        for i in range(4):
            src_type = format.in_types[i]
            if names[i]:
                value = '(*src_pixel++)'
                value = conversion_expr(src_type, dst_type, dst_native_type, value)
                print '         %s %s = %s;' % (dst_native_type, names[i], value)
    else:
        assert False

    for i in range(4):
        if format.colorspace == 'rgb':
            swizzle = format.out_swizzle[i]
            if swizzle < 4:
                value = names[swizzle]
            elif swizzle == SWIZZLE_0:
                value = '0'
            elif swizzle == SWIZZLE_1:
                value = '1'
            else:
                assert False
        elif format.colorspace == 'zs':
            if i < 3:
                value = 'z'
            else:
                value = '1'
        else:
            assert False
        print '         TILE_PIXEL(dst, x, y, %u) = %s; /* %s */' % (i, value, 'rgba'[i])

    print '      }'
    print '      src_row += src_stride;'
    print '   }'
    print '}'
    print
    

def generate_format_write(format, src_type, src_native_type, src_suffix):
    '''Generate the function to write pixels to a particular format'''

    name = short_name(format)

    dst_native_type = native_type(format)

    print 'static void'
    print 'lp_tile_%s_write_%s(const %s *src, uint8_t *dst, unsigned dst_stride, unsigned x0, unsigned y0, unsigned w, unsigned h)' % (name, src_suffix, src_native_type)
    print '{'
    print '   unsigned x, y;'
    print '   uint8_t *dst_row = dst + y0*dst_stride;'
    print '   for (y = 0; y < h; ++y) {'
    print '      %s *dst_pixel = (%s *)(dst_row + x0*%u);' % (dst_native_type, dst_native_type, format.stride())
    print '      for (x = 0; x < w; ++x) {'

    inv_swizzle = [None]*4
    if format.colorspace == 'rgb':
        for i in range(4):
            swizzle = format.out_swizzle[i]
            if swizzle < 4:
                inv_swizzle[swizzle] = i
    elif format.colorspace == 'zs':
        swizzle = format.out_swizzle[0]
        if swizzle < 4:
            inv_swizzle[swizzle] = 0
    else:
        assert False

    if format.layout == ARITH:
        print '         %s pixel = 0;' % dst_native_type
        shift = 0;
        for i in range(4):
            dst_type = format.in_types[i]
            width = dst_type.size
            if inv_swizzle[i] is not None:
                value = 'TILE_PIXEL(src, x, y, %u)' % inv_swizzle[i]
                value = conversion_expr(src_type, dst_type, dst_native_type, value)
                if shift:
                    value = '(%s << %u)' % (value, shift)
                print '         pixel |= %s;' % value
            shift += width
        print '         *dst_pixel++ = pixel;'
    elif format.layout == ARRAY:
        for i in range(4):
            dst_type = format.in_types[i]
            if inv_swizzle[i] is not None:
                value = 'TILE_PIXEL(src, x, y, %u)' % inv_swizzle[i]
                value = conversion_expr(src_type, dst_type, dst_native_type, value)
                print '         *dst_pixel++ = %s;' % value
    else:
        assert False

    print '      }'
    print '      dst_row += dst_stride;'
    print '   }'
    print '}'
    print
    

def generate_read(formats, dst_type, dst_native_type, dst_suffix):
    '''Generate the dispatch function to read pixels from any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_read(format, dst_type, dst_native_type, dst_suffix)

    print 'void'
    print 'lp_tile_read_%s(enum pipe_format format, %s *dst, const void *src, unsigned src_stride, unsigned x, unsigned y, unsigned w, unsigned h)' % (dst_suffix, dst_native_type)
    print '{'
    print '   void (*func)(%s *dst, const uint8_t *src, unsigned src_stride, unsigned x0, unsigned y0, unsigned w, unsigned h);' % dst_native_type
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            print '      func = &lp_tile_%s_read_%s;' % (short_name(format), dst_suffix)
            print '      break;'
    print '   default:'
    print '      debug_printf("unsupported format\\n");'
    print '      return;'
    print '   }'
    print '   func(dst, (const uint8_t *)src, src_stride, x, y, w, h);'
    print '}'
    print


def generate_write(formats, src_type, src_native_type, src_suffix):
    '''Generate the dispatch function to write pixels to any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_write(format, src_type, src_native_type, src_suffix)

    print 'void'
    print 'lp_tile_write_%s(enum pipe_format format, const %s *src, void *dst, unsigned dst_stride, unsigned x, unsigned y, unsigned w, unsigned h)' % (src_suffix, src_native_type)
    
    print '{'
    print '   void (*func)(const %s *src, uint8_t *dst, unsigned dst_stride, unsigned x0, unsigned y0, unsigned w, unsigned h);' % src_native_type
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            print '      func = &lp_tile_%s_write_%s;' % (short_name(format), src_suffix)
            print '      break;'
    print '   default:'
    print '      debug_printf("unsupported format\\n");'
    print '      return;'
    print '   }'
    print '   func(src, (uint8_t *)dst, dst_stride, x, y, w, h);'
    print '}'
    print


def main():
    formats = []
    for arg in sys.argv[1:]:
        formats.extend(parse(arg))

    print '/* This file is autogenerated by lp_tile_soa.py from u_format.csv. Do not edit directly. */'
    print
    # This will print the copyright message on the top of this file
    print __doc__.strip()
    print
    print '#include "pipe/p_compiler.h"'
    print '#include "util/u_format.h"'
    print '#include "util/u_math.h"'
    print '#include "lp_tile_soa.h"'
    print
    print 'const unsigned char'
    print 'tile_offset[TILE_VECTOR_HEIGHT][TILE_VECTOR_WIDTH] = {'
    print '   {  0,  1,  4,  5,  8,  9, 12, 13},'
    print '   {  2,  3,  6,  7, 10, 11, 14, 15}'
    print '};'
    print

    generate_clamp()

    type = Type(UNSIGNED, True, 8)
    native_type = 'uint8_t'
    suffix = '4ub'

    generate_read(formats, type, native_type, suffix)
    generate_write(formats, type, native_type, suffix)


if __name__ == '__main__':
    main()
