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


/**
 * Zbuffer/depth test code.
 */


#ifndef SPU_ZTEST_H
#define SPU_ZTEST_H


#ifdef __SPU__
#include <spu_intrinsics.h>
#endif



/**
 * Perform Z testing for a 16-bit/value Z buffer.
 *
 * \param zvals  vector of four fragment zvalues as floats
 * \param zbuf   ptr to vector of ushort[8] zbuffer values.  Note that this
 *               contains the Z values for 2 quads, 8 pixels.
 * \param x      x coordinate of quad (only lsbit is significant)
 * \param inMask indicates which fragments in the quad are alive
 * \return new mask indicating which fragments are alive after ztest
 */
static INLINE vector unsigned int
spu_z16_test_less(vector float zvals, vector unsigned short *zbuf,
                  uint x, vector unsigned int inMask)
{
#define ZERO 0x80
   vector unsigned int zvals_ui4, zbuf_ui4, mask;

   /* convert floats to uints in [0, 65535] */
   zvals_ui4 = spu_convtu(zvals, 32); /* convert to [0, 2^32] */
   zvals_ui4 = spu_rlmask(zvals_ui4, -16);  /* right shift 16 */

   /* XXX this conditional could be removed with a bit of work */
   if (x & 1) {
      /* convert zbuffer values from ushorts to uints */
      /* gather lower four ushorts */
      zbuf_ui4 = spu_shuffle((vector unsigned int) *zbuf,
                             (vector unsigned int) *zbuf,
                             ((vector unsigned char) {
                                ZERO, ZERO,  8,  9, ZERO, ZERO, 10, 11,
                                ZERO, ZERO, 12, 13, ZERO, ZERO, 14, 15}));
      /* mask = (zbuf_ui4 < zvals_ui4) ? ~0 : 0 */
      mask = spu_cmpgt(zbuf_ui4, zvals_ui4);
      /* mask &= inMask */
      mask = spu_and(mask, inMask);
      /* zbuf = mask ? zval : zbuf */
      zbuf_ui4 = spu_sel(zbuf_ui4, zvals_ui4, mask);
      /* convert zbuffer values from uints back to ushorts, preserve lower 4 */
      *zbuf = (vector unsigned short)
         spu_shuffle(zbuf_ui4, (vector unsigned int) *zbuf,
                     ((vector unsigned char) {
                        16, 17, 18, 19, 20, 21, 22, 23,
                        2, 3, 6, 7, 10, 11, 14, 15}));
   }
   else {
      /* convert zbuffer values from ushorts to uints */
      /* gather upper four ushorts */
      zbuf_ui4 = spu_shuffle((vector unsigned int) *zbuf,
                             (vector unsigned int) *zbuf,
                             ((vector unsigned char) {
                                ZERO, ZERO, 0, 1, ZERO, ZERO, 2, 3,
                                ZERO, ZERO, 4, 5, ZERO, ZERO, 6, 7}));
      /* mask = (zbuf_ui4 < zvals_ui4) ? ~0 : 0 */
      mask = spu_cmpgt(zbuf_ui4, zvals_ui4);
      /* mask &= inMask */
      mask = spu_and(mask, inMask);
      /* zbuf = mask ? zval : zbuf */
      zbuf_ui4 = spu_sel(zbuf_ui4, zvals_ui4, mask);
      /* convert zbuffer values from uints back to ushorts, preserve upper 4 */
      *zbuf = (vector unsigned short)
         spu_shuffle(zbuf_ui4, (vector unsigned int) *zbuf,
                     ((vector unsigned char) {
                        2, 3, 6, 7, 10, 11, 14, 15,
                        24, 25, 26, 27, 28, 29, 30, 31}));
   }
   return mask;
#undef ZERO
}


/**
 * As above, but Zbuffer values as 32-bit uints
 */
static INLINE vector unsigned int
spu_z32_test_less(vector float zvals, vector unsigned int *zbuf_ptr,
                  vector unsigned int inMask)
{
   vector unsigned int zvals_ui4, mask, zbuf = *zbuf_ptr;

   /* convert floats to uints in [0, 0xffffffff] */
   zvals_ui4 = spu_convtu(zvals, 32);
   /* mask = (zbuf < zvals_ui4) ? ~0 : 0 */
   mask = spu_cmpgt(zbuf, zvals_ui4);
   /* mask &= inMask */
   mask = spu_and(mask, inMask);
   /* zbuf = mask ? zval : zbuf */
   *zbuf_ptr = spu_sel(zbuf, zvals_ui4, mask);

   return mask;
}


#endif /* SPU_ZTEST_H */
