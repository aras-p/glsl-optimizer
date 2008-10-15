/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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



#ifndef SPU_COLORPACK_H
#define SPU_COLORPACK_H


#include <transpose_matrix4x4.h>
#include <spu_intrinsics.h>


static INLINE unsigned int
spu_pack_R8G8B8A8(vector float rgba)
{
  vector unsigned int out = spu_convtu(rgba, 32);

  out = spu_shuffle(out, out, ((vector unsigned char) {
                                  0, 4, 8, 12, 0, 0, 0, 0, 
                                  0, 0, 0, 0, 0, 0, 0, 0 }) );

  return spu_extract(out, 0);
}


static INLINE unsigned int
spu_pack_A8R8G8B8(vector float rgba)
{
  vector unsigned int out = spu_convtu(rgba, 32);
  out = spu_shuffle(out, out, ((vector unsigned char) {
                                  12, 0, 4, 8, 0, 0, 0, 0, 
                                  0, 0, 0, 0, 0, 0, 0, 0}) );
  return spu_extract(out, 0);
}


static INLINE unsigned int
spu_pack_B8G8R8A8(vector float rgba)
{
  vector unsigned int out = spu_convtu(rgba, 32);
  out = spu_shuffle(out, out, ((vector unsigned char) {
                                  8, 4, 0, 12, 0, 0, 0, 0, 
                                  0, 0, 0, 0, 0, 0, 0, 0}) );
  return spu_extract(out, 0);
}


static INLINE unsigned int
spu_pack_color_shuffle(vector float rgba, vector unsigned char shuffle)
{
  vector unsigned int out = spu_convtu(rgba, 32);
  out = spu_shuffle(out, out, shuffle);
  return spu_extract(out, 0);
}


static INLINE vector float
spu_unpack_B8G8R8A8(uint color)
{
   vector unsigned int color_u4 = spu_splats(color);
   color_u4 = spu_shuffle(color_u4, color_u4,
                          ((vector unsigned char) {
                             2, 2, 2, 2,
                             1, 1, 1, 1,
                             0, 0, 0, 0,
                             3, 3, 3, 3}) );
   return spu_convtf(color_u4, 32);
}


static INLINE vector float
spu_unpack_A8R8G8B8(uint color)
{
   vector unsigned int color_u4 = spu_splats(color);
   color_u4 = spu_shuffle(color_u4, color_u4,
                          ((vector unsigned char) {
                             1, 1, 1, 1,
                             2, 2, 2, 2,
                             3, 3, 3, 3,
                             0, 0, 0, 0}) );
   return spu_convtf(color_u4, 32);
}


/**
 * \param color_in - array of 32-bit packed ARGB colors
 * \param color_out - returns float colors in RRRR, GGGG, BBBB, AAAA order
 */
static INLINE void
spu_unpack_A8R8G8B8_transpose4(const vector unsigned int color_in[4],
                               vector float color_out[4])
{
   vector unsigned int c0;

   c0 = spu_shuffle(color_in[0], color_in[0],
                    ((vector unsigned char) {
                       1, 1, 1, 1,  2, 2, 2, 2,  3, 3, 3, 3,  0, 0, 0, 0}) );
   color_out[0] = spu_convtf(c0, 32);

   c0 = spu_shuffle(color_in[1], color_in[1],
                    ((vector unsigned char) {
                       1, 1, 1, 1,  2, 2, 2, 2,  3, 3, 3, 3,  0, 0, 0, 0}) );
   color_out[1] = spu_convtf(c0, 32);

   c0 = spu_shuffle(color_in[2], color_in[2],
                    ((vector unsigned char) {
                       1, 1, 1, 1,  2, 2, 2, 2,  3, 3, 3, 3,  0, 0, 0, 0}) );
   color_out[2] = spu_convtf(c0, 32);

   c0 = spu_shuffle(color_in[3], color_in[3],
                    ((vector unsigned char) {
                       1, 1, 1, 1,  2, 2, 2, 2,  3, 3, 3, 3,  0, 0, 0, 0}) );
   color_out[3] = spu_convtf(c0, 32);

   _transpose_matrix4x4(color_out, color_out);
}



#endif /* SPU_COLORPACK_H */
