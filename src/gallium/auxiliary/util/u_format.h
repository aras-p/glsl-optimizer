/**************************************************************************
 *
 * Copyright 2009 Vmware, Inc.
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


#ifndef U_FORMAT_H
#define U_FORMAT_H


#include "pipe/p_format.h"


/**
 * Describe how to best pack/unpack pixels into/from the prescribed format.
 *
 * These are used for automatic code generation of pixel packing and unpacking
 * routines (in compile time, e.g., u_format_access.py, or in runtime, like
 * llvmpipe does).
 *
 * Thumb rule is: if you're not code generating pixel packing/unpacking then
 * these are irrelevant for you.
 *
 * Note that this can be deduced from other values in util_format_description
 * structure. This is by design, to make code generation of pixel
 * packing/unpacking/sampling routines simple and efficient.
 *
 * XXX: This should be renamed to something like util_format_pack.
 */
enum util_format_layout {
   /**
    * Single scalar component.
    */
   UTIL_FORMAT_LAYOUT_SCALAR = 0,

   /**
    * One or more components of mixed integer formats, arithmetically encoded
    * in a word up to 32bits.
    */
   UTIL_FORMAT_LAYOUT_ARITH = 1,

   /**
    * One or more components, no mixed formats, each with equal power of two
    * number of bytes.
    */
   UTIL_FORMAT_LAYOUT_ARRAY = 2,

   /**
    * XXX: Not used yet. These might go away and be replaced by a single entry,
    * for formats where multiple pixels have to be
    * read in order to determine a single pixel value (i.e., block.width > 1
    * || block.height > 1)
    */
   UTIL_FORMAT_LAYOUT_YUV = 3,
   UTIL_FORMAT_LAYOUT_DXT = 4
};


struct util_format_block
{
   /** Block width in pixels */
   unsigned width;
   
   /** Block height in pixels */
   unsigned height;

   /** Block size in bits */
   unsigned bits;
};


enum util_format_type {
   UTIL_FORMAT_TYPE_VOID = 0,
   UTIL_FORMAT_TYPE_UNSIGNED = 1,
   UTIL_FORMAT_TYPE_SIGNED = 2,
   UTIL_FORMAT_TYPE_FIXED = 3,
   UTIL_FORMAT_TYPE_FLOAT = 4
};


enum util_format_swizzle {
   UTIL_FORMAT_SWIZZLE_X = 0,
   UTIL_FORMAT_SWIZZLE_Y = 1,
   UTIL_FORMAT_SWIZZLE_Z = 2,
   UTIL_FORMAT_SWIZZLE_W = 3,
   UTIL_FORMAT_SWIZZLE_0 = 4,
   UTIL_FORMAT_SWIZZLE_1 = 5,
   UTIL_FORMAT_SWIZZLE_NONE = 6
};


enum util_format_colorspace {
   UTIL_FORMAT_COLORSPACE_RGB = 0,
   UTIL_FORMAT_COLORSPACE_SRGB = 1,
   UTIL_FORMAT_COLORSPACE_YUV = 2,
   UTIL_FORMAT_COLORSPACE_ZS = 3,
};


struct util_format_channel_description
{
   unsigned type:6;
   unsigned normalized:1;
   unsigned size:9;
};


struct util_format_description
{
   enum pipe_format format;
   const char *name;
   struct util_format_block block;
   enum util_format_layout layout;
   struct util_format_channel_description channel[4];
   unsigned char swizzle[4];
   enum util_format_colorspace colorspace;
};


extern const struct util_format_description 
util_format_description_table[];


const struct util_format_description *
util_format_description(enum pipe_format format);


void
util_format_read_4f(enum pipe_format format,
                    float *dst, unsigned dst_stride, 
                    const void *src, unsigned src_stride, 
                    unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_write_4f(enum pipe_format format,
                     const float *src, unsigned src_stride, 
                     void *dst, unsigned dst_stride, 
                     unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_read_4ub(enum pipe_format format,
                     uint8_t *dst, unsigned dst_stride, 
                     const void *src, unsigned src_stride, 
                     unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_write_4ub(enum pipe_format format,
                      const uint8_t *src, unsigned src_stride, 
                      void *dst, unsigned dst_stride, 
                      unsigned x, unsigned y, unsigned w, unsigned h);

#endif /* ! U_FORMAT_H */
