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
#include "util/u_debug.h"

#ifdef __cplusplus
extern "C" {
#endif


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
   UTIL_FORMAT_COLORSPACE_ZS = 3
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


/*
 * Format query functions.
 */

static INLINE boolean 
util_format_is_compressed(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return FALSE;
   }

   return desc->layout == UTIL_FORMAT_LAYOUT_DXT ? TRUE : FALSE;
}

static INLINE boolean 
util_format_is_depth_or_stencil(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return FALSE;
   }

   return desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS ? TRUE : FALSE;
}

static INLINE boolean 
util_format_is_depth_and_stencil(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return FALSE;
   }

   if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS) {
      return FALSE;
   }

   return (desc->swizzle[0] != UTIL_FORMAT_SWIZZLE_NONE &&
           desc->swizzle[1] != UTIL_FORMAT_SWIZZLE_NONE) ? TRUE : FALSE;
}


/**
 * Return total bits needed for the pixel format per block.
 */
static INLINE uint
util_format_get_blocksizebits(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return 0;
   }

   return desc->block.bits;
}

/**
 * Return bytes per block (not pixel) for the given format.
 */
static INLINE uint
util_format_get_blocksize(enum pipe_format format)
{
   uint bits = util_format_get_blocksizebits(format);

   assert(bits % 8 == 0);

   return bits / 8;
}

static INLINE uint
util_format_get_blockwidth(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return 1;
   }

   switch (desc->layout) {
   case UTIL_FORMAT_LAYOUT_YUV:
      return 2;
   case UTIL_FORMAT_LAYOUT_DXT:
      return 4;
   default:
      return 1;
   }
}

static INLINE uint
util_format_get_blockheight(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return 1;
   }

   switch (desc->layout) {
   case UTIL_FORMAT_LAYOUT_DXT:
      return 4;
   default:
      return 1;
   }
}

static INLINE unsigned
util_format_get_nblocksx(enum pipe_format format,
                         unsigned x)
{
   unsigned blockwidth = util_format_get_blockwidth(format);
   return (x + blockwidth - 1) / blockwidth;
}

static INLINE unsigned
util_format_get_nblocksy(enum pipe_format format,
                         unsigned y)
{
   unsigned blockheight = util_format_get_blockheight(format);
   return (y + blockheight - 1) / blockheight;
}

static INLINE unsigned
util_format_get_nblocks(enum pipe_format format,
                        unsigned width,
                        unsigned height)
{
   return util_format_get_nblocksx(format, width) * util_format_get_nblocksy(format, height);
}

static INLINE size_t
util_format_get_stride(enum pipe_format format,
                       unsigned width)
{
   return util_format_get_nblocksx(format, width) * util_format_get_blocksize(format);
}

static INLINE size_t
util_format_get_2d_size(enum pipe_format format,
                        size_t stride,
                        unsigned height)
{
   return util_format_get_nblocksy(format, height) * stride;
}

static INLINE uint
util_format_get_component_bits(enum pipe_format format,
                               enum util_format_colorspace colorspace,
                               uint component)
{
   const struct util_format_description *desc = util_format_description(format);
   enum util_format_colorspace desc_colorspace;

   assert(format);
   if (!format) {
      return 0;
   }

   assert(component < 4);

   /* Treat RGB and SRGB as equivalent. */
   if (colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      colorspace = UTIL_FORMAT_COLORSPACE_RGB;
   }
   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      desc_colorspace = UTIL_FORMAT_COLORSPACE_RGB;
   } else {
      desc_colorspace = desc->colorspace;
   }

   if (desc_colorspace != colorspace) {
      return 0;
   }

   switch (desc->swizzle[component]) {
   case UTIL_FORMAT_SWIZZLE_X:
      return desc->channel[0].size;
   case UTIL_FORMAT_SWIZZLE_Y:
      return desc->channel[1].size;
   case UTIL_FORMAT_SWIZZLE_Z:
      return desc->channel[2].size;
   case UTIL_FORMAT_SWIZZLE_W:
      return desc->channel[3].size;
   default:
      return 0;
   }
}

static INLINE boolean
util_format_has_alpha(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return FALSE;
   }

   switch (desc->layout) {
   case UTIL_FORMAT_LAYOUT_SCALAR:
   case UTIL_FORMAT_LAYOUT_ARITH:
   case UTIL_FORMAT_LAYOUT_ARRAY:
      /* FIXME: pf_get_component_bits( PIPE_FORMAT_A8L8_UNORM, PIPE_FORMAT_COMP_A ) should not return 0 right? */
      if (format == PIPE_FORMAT_A8_UNORM ||
          format == PIPE_FORMAT_A8L8_UNORM ||
          format == PIPE_FORMAT_A8L8_SRGB) {
         return TRUE;
      }
      return util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 3) != 0;
   case UTIL_FORMAT_LAYOUT_YUV:
      return FALSE;
   case UTIL_FORMAT_LAYOUT_DXT:
      switch (format) {
      case PIPE_FORMAT_DXT1_RGBA:
      case PIPE_FORMAT_DXT3_RGBA:
      case PIPE_FORMAT_DXT5_RGBA:
      case PIPE_FORMAT_DXT1_SRGBA:
      case PIPE_FORMAT_DXT3_SRGBA:
      case PIPE_FORMAT_DXT5_SRGBA:
         return TRUE;
      default:
         return FALSE;
      }
   default:
      assert(0);
      return FALSE;
   }
}


/*
 * Format access functions.
 */

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

#ifdef __cplusplus
} // extern "C" {
#endif

#endif /* ! U_FORMAT_H */
