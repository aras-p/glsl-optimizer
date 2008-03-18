/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Functions to produce packed colors/Z from floats.
 */


#include "pipe/p_compiler.h"


/**
 * Note rgba outside [0,1] will be clamped for int pixel formats.
 */
static INLINE void
util_pack_color(const float rgba[4], enum pipe_format format, void *dest)
{
   ubyte r, g, b, a;

   if (pf_size_x(format) <= 8) {
      /* format uses 8-bit components or less */
      UNCLAMPED_FLOAT_TO_UBYTE(r, rgba[0]);
      UNCLAMPED_FLOAT_TO_UBYTE(g, rgba[1]);
      UNCLAMPED_FLOAT_TO_UBYTE(b, rgba[2]);
      UNCLAMPED_FLOAT_TO_UBYTE(a, rgba[3]);
   }

   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (r << 24) | (g << 16) | (b << 8) | a;
      }
      return;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (b << 24) | (g << 16) | (r << 8) | a;
      }
      return;
   case PIPE_FORMAT_R5G6B5_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
      }
      return;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      {
         float *d = (float *) dest;
         d[0] = rgba[0];
         d[1] = rgba[1];
         d[2] = rgba[2];
         d[3] = rgba[3];
      }
      return;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      {
         float *d = (float *) dest;
         d[0] = rgba[0];
         d[1] = rgba[1];
         d[2] = rgba[2];
      }
      return;
   /* XXX lots more cases to add */
   default:
      debug_printf("gallium: unhandled format in util_pack_color()");
   }
}
 

/**
 * Note: it's assumed that z is in [0,1]
 */
static INLINE uint
util_pack_z(enum pipe_format format, double z)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      return (uint) (z * 0xffff);
   case PIPE_FORMAT_Z32_UNORM:
      /* special-case to avoid overflow */
      if (z == 1.0)
         return 0xffffffff;
      else
         return (uint) (z * 0xffffffff);
   case PIPE_FORMAT_S8Z24_UNORM:
      return (uint) (z * 0xffffff);
   case PIPE_FORMAT_Z24S8_UNORM:
      return ((uint) (z * 0xffffff)) << 8;
   default:
      debug_printf("gallium: unhandled fomrat in util_pack_z()");
      return 0;
   }
}
