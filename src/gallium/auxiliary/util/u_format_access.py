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

from u_format_parse import *


def is_format_supported(format):
    '''Determines whether we actually have the plumbing necessary to generate the 
    to read/write to/from this format.'''

    # FIXME: Ideally we would support any format combination here.

    # XXX: It should be straightforward to support srgb
    if format.colorspace not in ('rgb', 'zs'):
        return False

    if format.layout not in (ARITH, ARRAY):
        return False

    for i in range(4):
        type = format.in_types[i]
        if type.kind not in (VOID, UNSIGNED, FLOAT):
            return False

    # We can only read a color from a depth/stencil format if the depth channel is present
    if format.colorspace == 'zs' and format.out_swizzle[0] == SWIZZLE_NONE:
        return False

    return True


def native_type(format):
    '''Get the native appropriate for a format.'''

    if format.layout in (ARITH, ARRAY):
        if not format.is_array():
            # For arithmetic pixel formats return the integer type that matches the whole pixel
            return 'uint%u_t' % format.block_size()
        else:
            # For array pixel formats return the integer type that matches the color channel
            type = format.in_types[0]
            if type.kind == UNSIGNED:
                return 'uint%u_t' % type.size
            elif type.kind == SIGNED:
                return 'int%u_t' % type.size
            elif type.kind == FLOAT:
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
    if type.kind == 'FLOAT':
        assert False
    if not type.norm:
        return 0
    if type.kind == UNSIGNED:
        return type.size
    if type.kind == SIGNED:
        return type.size - 1
    if type.kind == FIXED:
        return type.size / 2
    assert False


def get_one(type):
    '''Get the value of unity for this type.'''
    if type.kind == 'FLOAT' or not type.norm:
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


def clamp_expr(src_type, dst_type, dst_native_type, value):
    '''Generate the expression to clamp the value in the source type to the
    destination type range.'''

    if src_type == dst_type:
        return value

    # Pick the approriate clamp function
    if src_type.kind == FLOAT:
        if src_type.size == 32:
            func = 'clampf'
        elif src_type.size == 64:
            func = 'clamp'
        else:
            assert False
    elif src_type.kind == UNSIGNED:
        func = 'clampui'
    elif src_type.kind == SIGNED:
        func = 'clampsi'
    else:
        assert False

    # Clamp floats to [-1, 1] or [0, 1] range
    if src_type.kind == FLOAT and dst_type.norm:
        max = 1
        if src_type.sign and dst_type.sign:
            min = -1
        else:
            min = 0
        return '%s(%s, %s, %s)' % (func, value, min, max)
                
    # FIXME: Also clamp scaled values

    return value


def conversion_expr(src_type, dst_type, dst_native_type, value):
    '''Generate the expression to convert a value between two types.'''

    if src_type == dst_type:
        return value

    if src_type.kind == FLOAT and dst_type.kind == FLOAT:
        return '(%s)%s' % (dst_native_type, value)
    
    if not src_type.norm and not dst_type.norm:
        return '(%s)%s' % (dst_native_type, value)

    value = clamp_expr(src_type, dst_type, dst_native_type, value)

    if dst_type.kind == FLOAT:
        if src_type.norm:
            one = get_one(src_type)
            if src_type.size <= 23:
                scale = '(1.0f/0x%x)' % one
            else:
                # bigger than single precision mantissa, use double
                scale = '(1.0/0x%x)' % one
            value = '(%s * %s)' % (value, scale)
        return '(%s)%s' % (dst_native_type, value)

    if src_type.kind == FLOAT:
        if dst_type.norm:
            dst_one = get_one(dst_type)
            if dst_type.size <= 23:
                scale = '0x%x' % dst_one
            else:
                # bigger than single precision mantissa, use double
                scale = '(double)0x%x' % dst_one
            value = '(%s * %s)' % (value, scale)
        return '(%s)%s' % (dst_native_type, value)

    if src_type.kind == dst_type.kind:
        src_one = get_one(src_type)
        dst_one = get_one(dst_type)

        if src_one > dst_one and src_type.norm and dst_type.norm:
            # We can just bitshift
            src_shift = get_one_shift(src_type)
            dst_shift = get_one_shift(dst_type)
            value = '(%s >> %s)' % (value, src_shift - dst_shift)
        else:
            # We need to rescale using an intermediate type big enough to hold the multiplication of both
            tmp_native_type = intermediate_native_type(src_type.size + dst_type.size, src_type.sign and dst_type.sign)
            value = '(%s)%s' % (tmp_native_type, value)
            value = '%s * 0x%x / 0x%x' % (value, dst_one, src_one)
        value = '(%s)%s' % (dst_native_type, value)
        return value

    assert False


def compute_inverse_swizzle(format):
    '''Return an array[4] of inverse swizzle terms'''
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

    return inv_swizzle


def generate_format_read(format, dst_type, dst_native_type, dst_suffix):
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

    if format.layout in (ARITH, ARRAY):
        if not format.is_array():
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
        else:
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
                value = get_one(dst_type)
            else:
                assert False
        elif format.colorspace == 'zs':
            if i < 3:
                value = 'z'
            else:
                value = get_one(dst_type)
        else:
            assert False
        print '         *dst_pixel++ = %s; /* %s */' % (value, 'rgba'[i])

    print '      }'
    print '      src_row += src_stride;'
    print '      dst_row += dst_stride/sizeof(*dst_row);'
    print '   }'
    print '}'
    print
    

def generate_format_write(format, src_type, src_native_type, src_suffix):
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

    inv_swizzle = compute_inverse_swizzle(format)

    if format.layout in (ARITH, ARRAY):
        if not format.is_array():
            print '         %s pixel = 0;' % dst_native_type
            shift = 0;
            for i in range(4):
                dst_type = format.in_types[i]
                width = dst_type.size
                if inv_swizzle[i] is not None:
                    value = 'src_pixel[%u]' % inv_swizzle[i]
                    value = conversion_expr(src_type, dst_type, dst_native_type, value)
                    if shift:
                        value = '(%s << %u)' % (value, shift)
                    print '         pixel |= %s;' % value
                shift += width
            print '         *dst_pixel++ = pixel;'
        else:
            for i in range(4):
                dst_type = format.in_types[i]
                if inv_swizzle[i] is not None:
                    value = 'src_pixel[%u]' % inv_swizzle[i]
                    value = conversion_expr(src_type, dst_type, dst_native_type, value)
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
    

def generate_read(formats, dst_type, dst_native_type, dst_suffix):
    '''Generate the dispatch function to read pixels from any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_read(format, dst_type, dst_native_type, dst_suffix)

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


def generate_write(formats, src_type, src_native_type, src_suffix):
    '''Generate the dispatch function to write pixels to any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_write(format, src_type, src_native_type, src_suffix)

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
    print '#include "u_format.h"'
    print '#include "u_math.h"'
    print

    generate_clamp()

    type = Type(FLOAT, False, 32)
    native_type = 'float'
    suffix = '4f'

    generate_read(formats, type, native_type, suffix)
    generate_write(formats, type, native_type, suffix)

    type = Type(UNSIGNED, True, 8)
    native_type = 'uint8_t'
    suffix = '4ub'

    generate_read(formats, type, native_type, suffix)
    generate_write(formats, type, native_type, suffix)


if __name__ == '__main__':
    main()
