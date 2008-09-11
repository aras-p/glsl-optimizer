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


#ifndef U_PACK_COLOR_H
#define U_PACK_COLOR_H


#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "util/u_math.h"


/**
 * Pack ubyte R,G,B,A into dest pixel.
 */
static INLINE void
util_pack_color_ub(ubyte r, ubyte g, ubyte b, ubyte a,
                   enum pipe_format format, void *dest)
{
   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (r << 24) | (g << 16) | (b << 8) | a;
      }
      return;
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (r << 24) | (g << 16) | (b << 8) | 0xff;
      }
      return;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (0xff << 24) | (r << 16) | (g << 8) | b;
      }
      return;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (b << 24) | (g << 16) | (r << 8) | a;
      }
      return;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (b << 24) | (g << 16) | (r << 8) | 0xff;
      }
      return;
   case PIPE_FORMAT_R5G6B5_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
      }
      return;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((a & 0x80) << 8) | ((r & 0xf8) << 7) | ((g & 0xf8) << 2) | (b >> 3);
      }
      return;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((a & 0xf0) << 8) | ((r & 0xf0) << 4) | ((g & 0xf0) << 0) | (b >> 4);
      }
      return;
   case PIPE_FORMAT_A8_UNORM:
      {
         ubyte *d = (ubyte *) dest;
         *d = a;
      }
      return;
   case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
      {
         ubyte *d = (ubyte *) dest;
         *d = r;
      }
      return;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      {
         float *d = (float *) dest;
         d[0] = (float)r / 255.0f;
         d[1] = (float)g / 255.0f;
         d[2] = (float)b / 255.0f;
         d[3] = (float)a / 255.0f;
      }
      return;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      {
         float *d = (float *) dest;
         d[0] = (float)r / 255.0f;
         d[1] = (float)g / 255.0f;
         d[2] = (float)b / 255.0f;
      }
      return;

   /* XXX lots more cases to add */
   default:
      debug_print_format("gallium: unhandled format in util_pack_color_ub()", format);
      assert(0);
   }
}
 

/**
 * Unpack RGBA from a packed pixel, returning values as ubytes in [0,255].
 */
static INLINE void
util_unpack_color_ub(enum pipe_format format, const void *src,
                     ubyte *r, ubyte *g, ubyte *b, ubyte *a)
{
   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      {
         uint p = ((const uint *) src)[0];
         *r = (ubyte) ((p >> 24) & 0xff);
         *g = (ubyte) ((p >> 16) & 0xff);
         *b = (ubyte) ((p >>  8) & 0xff);
         *a = (ubyte) ((p >>  0) & 0xff);
      }
      return;
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      {
         uint p = ((const uint *) src)[0];
         *r = (ubyte) ((p >> 24) & 0xff);
         *g = (ubyte) ((p >> 16) & 0xff);
         *b = (ubyte) ((p >>  8) & 0xff);
         *a = (ubyte) 0xff;
      }
      return;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      {
         uint p = ((const uint *) src)[0];
         *r = (ubyte) ((p >> 16) & 0xff);
         *g = (ubyte) ((p >>  8) & 0xff);
         *b = (ubyte) ((p >>  0) & 0xff);
         *a = (ubyte) ((p >> 24) & 0xff);
      }
      return;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      {
         uint p = ((const uint *) src)[0];
         *r = (ubyte) ((p >> 16) & 0xff);
         *g = (ubyte) ((p >>  8) & 0xff);
         *b = (ubyte) ((p >>  0) & 0xff);
         *a = (ubyte) 0xff;
      }
      return;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      {
         uint p = ((const uint *) src)[0];
         *r = (ubyte) ((p >>  8) & 0xff);
         *g = (ubyte) ((p >> 16) & 0xff);
         *b = (ubyte) ((p >> 24) & 0xff);
         *a = (ubyte) ((p >>  0) & 0xff);
      }
      return;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      {
         uint p = ((const uint *) src)[0];
         *r = (ubyte) ((p >>  8) & 0xff);
         *g = (ubyte) ((p >> 16) & 0xff);
         *b = (ubyte) ((p >> 24) & 0xff);
         *a = (ubyte) 0xff;
      }
      return;
   case PIPE_FORMAT_R5G6B5_UNORM:
      {
         ushort p = ((const ushort *) src)[0];
         *r = (ubyte) (((p >> 8) & 0xf8) | ((p >> 13) & 0x7));
         *g = (ubyte) (((p >> 3) & 0xfc) | ((p >>  9) & 0x3));
         *b = (ubyte) (((p << 3) & 0xf8) | ((p >>  2) & 0x7));
         *a = (ubyte) 0xff;
      }
      return;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      {
         ushort p = ((const ushort *) src)[0];
         *r = (ubyte) (((p >>  7) & 0xf8) | ((p >> 12) & 0x7));
         *g = (ubyte) (((p >>  2) & 0xf8) | ((p >>  7) & 0x7));
         *b = (ubyte) (((p <<  3) & 0xf8) | ((p >>  2) & 0x7));
         *a = (ubyte) (0xff * (p >> 15));
      }
      return;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      {
         ushort p = ((const ushort *) src)[0];
         *r = (ubyte) (((p >> 4) & 0xf0) | ((p >>  8) & 0xf));
         *g = (ubyte) (((p >> 0) & 0xf0) | ((p >>  4) & 0xf));
         *b = (ubyte) (((p << 4) & 0xf0) | ((p >>  0) & 0xf));
         *a = (ubyte) (((p >> 8) & 0xf0) | ((p >> 12) & 0xf));
      }
      return;
   case PIPE_FORMAT_A8_UNORM:
      {
         ubyte p = ((const ubyte *) src)[0];
         *r = *g = *b = (ubyte) 0xff;
         *a = p;
      }
      return;
   case PIPE_FORMAT_L8_UNORM:
      {
         ubyte p = ((const ubyte *) src)[0];
         *r = *g = *b = p;
         *a = (ubyte) 0xff;
      }
      return;
   case PIPE_FORMAT_I8_UNORM:
      {
         ubyte p = ((const ubyte *) src)[0];
         *r = *g = *b = *a = p;
      }
      return;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      {
         const float *p = (const float *) src;
         *r = float_to_ubyte(p[0]);
         *g = float_to_ubyte(p[1]);
         *b = float_to_ubyte(p[2]);
         *a = float_to_ubyte(p[3]);
      }
      return;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      {
         const float *p = (const float *) src;
         *r = float_to_ubyte(p[0]);
         *g = float_to_ubyte(p[1]);
         *b = float_to_ubyte(p[2]);
         *a = (ubyte) 0xff;
      }
      return;

   case PIPE_FORMAT_R32G32_FLOAT:
      {
         const float *p = (const float *) src;
         *r = float_to_ubyte(p[0]);
         *g = float_to_ubyte(p[1]);
         *b = *a = (ubyte) 0xff;
      }
      return;

   case PIPE_FORMAT_R32_FLOAT:
      {
         const float *p = (const float *) src;
         *r = float_to_ubyte(p[0]);
         *g = *b = *a = (ubyte) 0xff;
      }
      return;

   /* XXX lots more cases to add */
   default:
      debug_print_format("gallium: unhandled format in util_unpack_color_ub()",
                         format);
      assert(0);
   }
}
 


/**
 * Note rgba outside [0,1] will be clamped for int pixel formats.
 */
static INLINE void
util_pack_color(const float rgba[4], enum pipe_format format, void *dest)
{
   ubyte r, g, b, a;

   if (pf_size_x(format) <= 8) {
      /* format uses 8-bit components or less */
      r = float_to_ubyte(rgba[0]);
      g = float_to_ubyte(rgba[1]);
      b = float_to_ubyte(rgba[2]);
      a = float_to_ubyte(rgba[3]);
   }

   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (r << 24) | (g << 16) | (b << 8) | a;
      }
      return;
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (r << 24) | (g << 16) | (b << 8) | 0xff;
      }
      return;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (0xff << 24) | (r << 16) | (g << 8) | b;
      }
      return;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (b << 24) | (g << 16) | (r << 8) | a;
      }
      return;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      {
         uint *d = (uint *) dest;
         *d = (b << 24) | (g << 16) | (r << 8) | 0xff;
      }
      return;
   case PIPE_FORMAT_R5G6B5_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
      }
      return;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((a & 0x80) << 8) | ((r & 0xf8) << 7) | ((g & 0xf8) << 2) | (b >> 3);
      }
      return;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      {
         ushort *d = (ushort *) dest;
         *d = ((a & 0xf0) << 8) | ((r & 0xf0) << 4) | ((g & 0xf0) << 0) | (b >> 4);
      }
      return;
   case PIPE_FORMAT_A8_UNORM:
      {
         ubyte *d = (ubyte *) dest;
         *d = a;
      }
      return;
   case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
      {
         ubyte *d = (ubyte *) dest;
         *d = r;
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
      debug_print_format("gallium: unhandled format in util_pack_color()", format);
      assert(0);
   }
}
 

/**
 * Note: it's assumed that z is in [0,1]
 */
static INLINE uint
util_pack_z(enum pipe_format format, double z)
{
   if (z == 0.0)
      return 0;

   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (z == 1.0)
         return 0xffff;
      return (uint) (z * 0xffff);
   case PIPE_FORMAT_Z32_UNORM:
      /* special-case to avoid overflow */
      if (z == 1.0)
         return 0xffffffff;
      return (uint) (z * 0xffffffff);
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      if (z == 1.0)
         return 0xffffff;
      return (uint) (z * 0xffffff);
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      if (z == 1.0)
         return 0xffffff00;
      return ((uint) (z * 0xffffff)) << 8;
   default:
      debug_print_format("gallium: unhandled format in util_pack_z()", format);
      assert(0);
      return 0;
   }
}


/**
 * Pack 4 ubytes into a 4-byte word
 */
static INLINE unsigned
pack_ub4(ubyte b0, ubyte b1, ubyte b2, ubyte b3)
{
   return ((((unsigned int)b0) << 0) |
	   (((unsigned int)b1) << 8) |
	   (((unsigned int)b2) << 16) |
	   (((unsigned int)b3) << 24));
}


/**
 * Pack/convert 4 floats into one 4-byte word.
 */
static INLINE unsigned
pack_ui32_float4(float a, float b, float c, float d)
{
   return pack_ub4( float_to_ubyte(a),
		    float_to_ubyte(b),
		    float_to_ubyte(c),
		    float_to_ubyte(d) );
}



#endif /* U_PACK_COLOR_H */
