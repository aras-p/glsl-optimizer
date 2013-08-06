/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/

/**
 * @file
 * SRGB translation.
 *
 * @author Brian Paul <brianp@vmware.com>
 * @author Michal Krol <michal@vmware.com>
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef U_FORMAT_SRGB_H_
#define U_FORMAT_SRGB_H_


#include "pipe/p_compiler.h"
#include "u_pack_color.h"
#include "u_math.h"


extern const float
util_format_srgb_8unorm_to_linear_float_table[256];

extern const uint8_t
util_format_srgb_to_linear_8unorm_table[256];

extern const uint8_t
util_format_linear_to_srgb_8unorm_table[256];

extern const unsigned
util_format_linear_to_srgb_helper_table[104];


/**
 * Convert a unclamped linear float to srgb value in the [0,255].
 */
static INLINE uint8_t
util_format_linear_float_to_srgb_8unorm(float x)
{
   /* this would be exact but (probably much) slower */
   if (0) {
      if (x >= 1.0f)
         return 255;
      else if (x >= 0.0031308f)
         return float_to_ubyte(1.055f * powf(x, 0.41666666f) - 0.055f);
      else if (x > 0.0f)
         return float_to_ubyte(12.92f * x);
      else
         return 0;
   }
   else {
      /*
       * This is taken from https://gist.github.com/rygorous/2203834
       * Use LUT and do linear interpolation.
       */
      union fi almostone, minval, f;
      unsigned tab, bias, scale, t;

      almostone.ui = 0x3f7fffff;
      minval.ui = (127-13) << 23;

      /*
       * Clamp to [2^(-13), 1-eps]; these two values map to 0 and 1, respectively.
       * The tests are carefully written so that NaNs map to 0, same as in the
       * reference implementation.
       */
      if (!(x > minval.f))
         x = minval.f;
      if (x > almostone.f)
         x = almostone.f;

      /* Do the table lookup and unpack bias, scale */
      f.f = x;
      tab = util_format_linear_to_srgb_helper_table[(f.ui - minval.ui) >> 20];
      bias = (tab >> 16) << 9;
      scale = tab & 0xffff;

      /* Grab next-highest mantissa bits and perform linear interpolation */
      t = (f.ui >> 12) & 0xff;
      return (uint8_t) ((bias + scale*t) >> 16);
   }
}


/**
 * Convert an 8-bit sRGB value from non-linear space to a
 * linear RGB value in [0, 1].
 * Implemented with a 256-entry lookup table.
 */
static INLINE float
util_format_srgb_8unorm_to_linear_float(uint8_t x)
{
   return util_format_srgb_8unorm_to_linear_float_table[x];
}


/*
 * XXX These 2 functions probably don't make a lot of sense (but lots
 * of potential callers which most likely all don't make sense neither)
 */

/**
 * Convert a 8bit normalized value from linear to srgb.
 */
static INLINE uint8_t
util_format_linear_to_srgb_8unorm(uint8_t x)
{
   return util_format_linear_to_srgb_8unorm_table[x];
}


/**
 * Convert a 8bit normalized value from srgb to linear.
 */
static INLINE uint8_t
util_format_srgb_to_linear_8unorm(uint8_t x)
{
   return util_format_srgb_to_linear_8unorm_table[x];
}


#endif /* U_FORMAT_SRGB_H_ */
