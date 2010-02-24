/**************************************************************************
 *
 * Copyright 2009-2010 Vmware, Inc.
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
 * Describe how to pack/unpack pixels into/from the prescribed format.
 *
 * XXX: This could be renamed to something like util_format_pack, or broke down
 * in flags inside util_format_block that said exactly what we want.
 */
enum util_format_layout {
   /**
    * Formats with util_format_block::width == util_format_block::height == 1
    * that can be described as an ordinary data structure.
    */
   UTIL_FORMAT_LAYOUT_PLAIN = 0,

   /**
    * Formats with sub-sampled channels.
    *
    * This is for formats like YV12 where there is less than one sample per
    * pixel.
    *
    * XXX: This could actually b
    */
   UTIL_FORMAT_LAYOUT_SUBSAMPLED = 3,

   /**
    * An unspecified compression algorithm.
    */
   UTIL_FORMAT_LAYOUT_COMPRESSED = 4
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

   /**
    * Pixel block dimensions.
    */
   struct util_format_block block;

   enum util_format_layout layout;

   /**
    * The number of channels.
    */
   unsigned nr_channels:3;

   /**
    * Whether all channels have the same number of (whole) bytes.
    */
   unsigned is_array:1;

   /**
    * Whether channels have mixed types (ignoring UTIL_FORMAT_TYPE_VOID).
    */
   unsigned is_mixed:1;

   /**
    * Input channel description.
    *
    * Only valid for UTIL_FORMAT_LAYOUT_PLAIN formats.
    */
   struct util_format_channel_description channel[4];

   /**
    * Output channel swizzle.
    *
    * The order is either:
    * - RGBA
    * - YUV(A)
    * - ZS
    * depending on the colorspace.
    */
   unsigned char swizzle[4];

   /**
    * Colorspace transformation.
    */
   enum util_format_colorspace colorspace;
};


extern const struct util_format_description 
util_format_description_table[];


const struct util_format_description *
util_format_description(enum pipe_format format);


/*
 * Format query functions.
 */

static INLINE const char *
util_format_name(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return "???";
   }

   return desc->name;
}

static INLINE boolean 
util_format_is_compressed(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return FALSE;
   }

   return desc->layout == UTIL_FORMAT_LAYOUT_COMPRESSED ? TRUE : FALSE;
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

   return desc->block.width;
}

static INLINE uint
util_format_get_blockheight(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return 1;
   }

   return desc->block.height;
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

   switch (desc->colorspace) {
   case UTIL_FORMAT_COLORSPACE_RGB:
   case UTIL_FORMAT_COLORSPACE_SRGB:
      return desc->swizzle[3] != UTIL_FORMAT_SWIZZLE_1;
   case UTIL_FORMAT_COLORSPACE_YUV:
      return FALSE;
   case UTIL_FORMAT_COLORSPACE_ZS:
      return FALSE;
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
