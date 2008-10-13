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


#include <transpose_matrix4x4.h>

#include "pipe/p_compiler.h"
#include "spu_main.h"
#include "spu_texture.h"
#include "spu_tile.h"
#include "spu_colorpack.h"
#include "spu_dcache.h"


/**
 * Mark all tex cache entries as invalid.
 */
void
invalidate_tex_cache(void)
{
   uint unit = 0;
   uint bytes = 4 * spu.texture[unit].width
      * spu.texture[unit].height;

   spu_dcache_mark_dirty((unsigned) spu.texture[unit].start, bytes);
}


/**
 * Get four texels from locations (x[0], y[0]), (x[1], y[1]) ...
 *
 * NOTE: in the typical case of bilinear filtering, the four texels
 * are in a 2x2 group so we could get by with just two dcache fetches
 * (two side-by-side texels per fetch).  But when bilinear filtering
 * wraps around a texture edge, we'll probably need code like we have
 * now.
 * FURTHERMORE: since we're rasterizing a quad of 2x2 pixels at a time,
 * it's quite likely that the four pixels in a quad will need some of the
 * same texels.  So look into doing texture fetches for four pixels at
 * a time.
 */
static void
get_four_texels(uint unit, vec_uint4 x, vec_uint4 y, vec_uint4 *texels)
{
   const unsigned texture_ea = (uintptr_t) spu.texture[unit].start;
   vec_uint4 tile_x = spu_rlmask(x, -5);  /* tile_x = x / 32 */
   vec_uint4 tile_y = spu_rlmask(y, -5);  /* tile_y = y / 32 */
   const qword offset_x = si_andi((qword) x, 0x1f); /* offset_x = x & 0x1f */
   const qword offset_y = si_andi((qword) y, 0x1f); /* offset_y = y & 0x1f */

   const qword tiles_per_row = (qword) spu_splats(spu.texture[unit].tiles_per_row);
   const qword tile_size = (qword) spu_splats((unsigned) sizeof(tile_t));

   qword tile_offset = si_mpya((qword) tile_y, tiles_per_row, (qword) tile_x);
   tile_offset = si_mpy((qword) tile_offset, tile_size);

   qword texel_offset = si_a(si_mpyui(offset_y, 32), offset_x);
   texel_offset = si_mpyui(texel_offset, 4);
   
   vec_uint4 offset = (vec_uint4) si_a(tile_offset, texel_offset);
   
   spu_dcache_fetch_unaligned((qword *) & texels[0],
                              texture_ea + spu_extract(offset, 0), 4);
   spu_dcache_fetch_unaligned((qword *) & texels[1],
                              texture_ea + spu_extract(offset, 1), 4);
   spu_dcache_fetch_unaligned((qword *) & texels[2],
                              texture_ea + spu_extract(offset, 2), 4);
   spu_dcache_fetch_unaligned((qword *) & texels[3],
                              texture_ea + spu_extract(offset, 3), 4);
}



/**
 * Do nearest texture sampling for four pixels.
 * \param colors  returned colors in SOA format (rrrr, gggg, bbbb, aaaa).
 */
void
sample_texture4_nearest(vector float s, vector float t,
                        vector float r, vector float q,
                        uint unit, vector float colors[4])
{
   vector float ss = spu_mul(s, spu.texture[unit].width4);
   vector float tt = spu_mul(t, spu.texture[unit].height4);
   vector unsigned int is = spu_convtu(ss, 0);
   vector unsigned int it = spu_convtu(tt, 0);
   vec_uint4 texels[4];

   /* PIPE_TEX_WRAP_REPEAT */
   is = spu_and(is, spu.texture[unit].tex_size_x_mask);
   it = spu_and(it, spu.texture[unit].tex_size_y_mask);

   get_four_texels(unit, is, it, texels);

   /* convert four packed ARGBA pixels to float RRRR,GGGG,BBBB,AAAA */
   spu_unpack_A8R8G8B8_transpose4(texels, colors);
}


/**
 * Do bilinear texture sampling for four pixels.
 * \param colors  returned colors in SOA format (rrrr, gggg, bbbb, aaaa).
 */
void
sample_texture4_bilinear(vector float s, vector float t,
                         vector float r, vector float q,
                         uint unit, vector float colors[4])
{
   vector float ss = spu_madd(s, spu.texture[unit].width4,  spu_splats(-0.5f));
   vector float tt = spu_madd(t, spu.texture[unit].height4, spu_splats(-0.5f));

   vector unsigned int is0 = spu_convtu(ss, 0);
   vector unsigned int it0 = spu_convtu(tt, 0);

   /* is + 1, it + 1 */
   vector unsigned int is1 = spu_add(is0, 1);
   vector unsigned int it1 = spu_add(it0, 1);

   /* PIPE_TEX_WRAP_REPEAT */
   is0 = spu_and(is0, spu.texture[unit].tex_size_x_mask);
   it0 = spu_and(it0, spu.texture[unit].tex_size_y_mask);
   is1 = spu_and(is1, spu.texture[unit].tex_size_x_mask);
   it1 = spu_and(it1, spu.texture[unit].tex_size_y_mask);

   /* get packed int texels */
   vector unsigned int texels[16];
   get_four_texels(unit, is0, it0, texels + 0);  /* upper-left */
   get_four_texels(unit, is1, it0, texels + 4);  /* upper-right */
   get_four_texels(unit, is0, it1, texels + 8);  /* lower-left */
   get_four_texels(unit, is1, it1, texels + 12); /* lower-right */

   /* XXX possibly rework following code to compute the weighted sample
    * colors with integer arithmetic for fewer int->float conversions.
    */

   /* convert packed int texels to float colors */
   vector float ftexels[16];
   spu_unpack_A8R8G8B8_transpose4(texels + 0, ftexels + 0);
   spu_unpack_A8R8G8B8_transpose4(texels + 4, ftexels + 4);
   spu_unpack_A8R8G8B8_transpose4(texels + 8, ftexels + 8);
   spu_unpack_A8R8G8B8_transpose4(texels + 12, ftexels + 12);

   /* Compute weighting factors in [0,1]
    * Multiply texcoord by 1024, AND with 1023, convert back to float.
    */
   vector float ss1024 = spu_mul(ss, spu_splats(1024.0f));
   vector signed int iss1024 = spu_convts(ss1024, 0);
   iss1024 = spu_and(iss1024, 1023);
   vector float sWeights0 = spu_convtf(iss1024, 10);

   vector float tt1024 = spu_mul(tt, spu_splats(1024.0f));
   vector signed int itt1024 = spu_convts(tt1024, 0);
   itt1024 = spu_and(itt1024, 1023);
   vector float tWeights0 = spu_convtf(itt1024, 10);

   /* 1 - sWeight and 1 - tWeight */
   vector float sWeights1 = spu_sub(spu_splats(1.0f), sWeights0);
   vector float tWeights1 = spu_sub(spu_splats(1.0f), tWeights0);

   /* reds, for four pixels */
   ftexels[ 0] = spu_mul(ftexels[ 0], spu_mul(sWeights1, tWeights1)); /*ul*/
   ftexels[ 4] = spu_mul(ftexels[ 4], spu_mul(sWeights0, tWeights1)); /*ur*/
   ftexels[ 8] = spu_mul(ftexels[ 8], spu_mul(sWeights1, tWeights0)); /*ll*/
   ftexels[12] = spu_mul(ftexels[12], spu_mul(sWeights0, tWeights0)); /*lr*/
   colors[0] = spu_add(spu_add(ftexels[0], ftexels[4]),
                       spu_add(ftexels[8], ftexels[12]));

   /* greens, for four pixels */
   ftexels[ 1] = spu_mul(ftexels[ 1], spu_mul(sWeights1, tWeights1)); /*ul*/
   ftexels[ 5] = spu_mul(ftexels[ 5], spu_mul(sWeights0, tWeights1)); /*ur*/
   ftexels[ 9] = spu_mul(ftexels[ 9], spu_mul(sWeights1, tWeights0)); /*ll*/
   ftexels[13] = spu_mul(ftexels[13], spu_mul(sWeights0, tWeights0)); /*lr*/
   colors[1] = spu_add(spu_add(ftexels[1], ftexels[5]),
                       spu_add(ftexels[9], ftexels[13]));

   /* blues, for four pixels */
   ftexels[ 2] = spu_mul(ftexels[ 2], spu_mul(sWeights1, tWeights1)); /*ul*/
   ftexels[ 6] = spu_mul(ftexels[ 6], spu_mul(sWeights0, tWeights1)); /*ur*/
   ftexels[10] = spu_mul(ftexels[10], spu_mul(sWeights1, tWeights0)); /*ll*/
   ftexels[14] = spu_mul(ftexels[14], spu_mul(sWeights0, tWeights0)); /*lr*/
   colors[2] = spu_add(spu_add(ftexels[2], ftexels[6]),
                       spu_add(ftexels[10], ftexels[14]));

   /* alphas, for four pixels */
   ftexels[ 3] = spu_mul(ftexels[ 3], spu_mul(sWeights1, tWeights1)); /*ul*/
   ftexels[ 7] = spu_mul(ftexels[ 7], spu_mul(sWeights0, tWeights1)); /*ur*/
   ftexels[11] = spu_mul(ftexels[11], spu_mul(sWeights1, tWeights0)); /*ll*/
   ftexels[15] = spu_mul(ftexels[15], spu_mul(sWeights0, tWeights0)); /*lr*/
   colors[3] = spu_add(spu_add(ftexels[3], ftexels[7]),
                       spu_add(ftexels[11], ftexels[15]));
}



/**
 * Adapted from /opt/cell/sdk/usr/spu/include/transpose_matrix4x4.h
 */
static INLINE void
transpose(vector unsigned int *mOut0,
          vector unsigned int *mOut1,
          vector unsigned int *mOut2,
          vector unsigned int *mOut3,
          vector unsigned int *mIn)
{
  vector unsigned int abcd, efgh, ijkl, mnop;	/* input vectors */
  vector unsigned int aeim, bfjn, cgko, dhlp;	/* output vectors */
  vector unsigned int aibj, ckdl, emfn, gohp;	/* intermediate vectors */

  vector unsigned char shufflehi = ((vector unsigned char) {
					       0x00, 0x01, 0x02, 0x03,
					       0x10, 0x11, 0x12, 0x13,
					       0x04, 0x05, 0x06, 0x07,
					       0x14, 0x15, 0x16, 0x17});
  vector unsigned char shufflelo = ((vector unsigned char) {
					       0x08, 0x09, 0x0A, 0x0B,
					       0x18, 0x19, 0x1A, 0x1B,
					       0x0C, 0x0D, 0x0E, 0x0F,
					       0x1C, 0x1D, 0x1E, 0x1F});
  abcd = *(mIn+0);
  efgh = *(mIn+1);
  ijkl = *(mIn+2);
  mnop = *(mIn+3);

  aibj = spu_shuffle(abcd, ijkl, shufflehi);
  ckdl = spu_shuffle(abcd, ijkl, shufflelo);
  emfn = spu_shuffle(efgh, mnop, shufflehi);
  gohp = spu_shuffle(efgh, mnop, shufflelo);

  aeim = spu_shuffle(aibj, emfn, shufflehi);
  bfjn = spu_shuffle(aibj, emfn, shufflelo);
  cgko = spu_shuffle(ckdl, gohp, shufflehi);
  dhlp = spu_shuffle(ckdl, gohp, shufflelo);

  *mOut0 = aeim;
  *mOut1 = bfjn;
  *mOut2 = cgko;
  *mOut3 = dhlp;
}


/**
 * Bilinear filtering, using int intead of float arithmetic
 */
void
sample_texture4_bilinear_2(vector float s, vector float t,
                         vector float r, vector float q,
                         uint unit, vector float colors[4])
{
   static const vector float half = {-0.5f, -0.5f, -0.5f, -0.5f};
   /* Scale texcoords by size of texture, and add half pixel bias */
   vector float ss = spu_madd(s, spu.texture[unit].width4, half);
   vector float tt = spu_madd(t, spu.texture[unit].height4, half);

   /* convert float coords to fixed-pt coords with 8 fraction bits */
   vector unsigned int is = spu_convtu(ss, 8);
   vector unsigned int it = spu_convtu(tt, 8);

   /* compute integer texel weights in [0, 255] */
   vector signed int sWeights0 = spu_and((vector signed int) is, 255);
   vector signed int tWeights0 = spu_and((vector signed int) it, 255);
   vector signed int sWeights1 = spu_sub(255, sWeights0);
   vector signed int tWeights1 = spu_sub(255, tWeights0);

   /* texel coords: is0 = is / 256, it0 = is / 256 */
   vector unsigned int is0 = spu_rlmask(is, -8);
   vector unsigned int it0 = spu_rlmask(it, -8);

   /* texel coords: i1 = is0 + 1, it1 = it0 + 1 */
   vector unsigned int is1 = spu_add(is0, 1);
   vector unsigned int it1 = spu_add(it0, 1);

   /* PIPE_TEX_WRAP_REPEAT */
   is0 = spu_and(is0, spu.texture[unit].tex_size_x_mask);
   it0 = spu_and(it0, spu.texture[unit].tex_size_y_mask);
   is1 = spu_and(is1, spu.texture[unit].tex_size_x_mask);
   it1 = spu_and(it1, spu.texture[unit].tex_size_y_mask);

   /* get packed int texels */
   vector unsigned int texels[16];
   get_four_texels(unit, is0, it0, texels + 0);  /* upper-left */
   get_four_texels(unit, is1, it0, texels + 4);  /* upper-right */
   get_four_texels(unit, is0, it1, texels + 8);  /* lower-left */
   get_four_texels(unit, is1, it1, texels + 12); /* lower-right */

   /* twiddle packed 32-bit BGRA pixels into RGBA as four unsigned ints */
   {
      static const unsigned char ZERO = 0x80;
      int i;
      for (i = 0; i < 16; i++) {
         texels[i] = spu_shuffle(texels[i], texels[i],
                                 ((vector unsigned char) {
                                    ZERO, ZERO, ZERO, 1,
                                    ZERO, ZERO, ZERO, 2,
                                    ZERO, ZERO, ZERO, 3,
                                    ZERO, ZERO, ZERO, 0}));
      }
   }

   /* convert RGBA,RGBA,RGBA,RGBA to RRRR,GGGG,BBBB,AAAA */
   vector unsigned int texel0, texel1, texel2, texel3, texel4, texel5, texel6, texel7,
      texel8, texel9, texel10, texel11, texel12, texel13, texel14, texel15;
   transpose(&texel0, &texel1, &texel2, &texel3, texels + 0);
   transpose(&texel4, &texel5, &texel6, &texel7, texels + 4);
   transpose(&texel8, &texel9, &texel10, &texel11, texels + 8);
   transpose(&texel12, &texel13, &texel14, &texel15, texels + 12);

   /* computed weighted colors */
   vector unsigned int c0, c1, c2, c3, cSum;

   /* red */
   c0 = (vector unsigned int) si_mpyu((qword) texel0, si_mpyu((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpyu((qword) texel4, si_mpyu((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpyu((qword) texel8, si_mpyu((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpyu((qword) texel12, si_mpyu((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[0] = spu_convtf(cSum, 24);

   /* green */
   c0 = (vector unsigned int) si_mpyu((qword) texel1, si_mpyu((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpyu((qword) texel5, si_mpyu((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpyu((qword) texel9, si_mpyu((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpyu((qword) texel13, si_mpyu((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[1] = spu_convtf(cSum, 24);

   /* blue */
   c0 = (vector unsigned int) si_mpyu((qword) texel2, si_mpyu((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpyu((qword) texel6, si_mpyu((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpyu((qword) texel10, si_mpyu((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpyu((qword) texel14, si_mpyu((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[2] = spu_convtf(cSum, 24);

   /* alpha */
   c0 = (vector unsigned int) si_mpyu((qword) texel3, si_mpyu((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpyu((qword) texel7, si_mpyu((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpyu((qword) texel11, si_mpyu((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpyu((qword) texel15, si_mpyu((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[3] = spu_convtf(cSum, 24);
}
