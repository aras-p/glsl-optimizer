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
 * XXX look into getting texels for all four pixels in a quad at once.
 */
static uint
get_texel(uint unit, vec_uint4 coordinate)
{
   /*
    * XXX we could do the "/ TILE_SIZE" and "% TILE_SIZE" operations as
    * SIMD since X and Y are already in a SIMD register.
    */
   const unsigned texture_ea = (uintptr_t) spu.texture[unit].start;
   ushort x = spu_extract(coordinate, 0);
   ushort y = spu_extract(coordinate, 1);
   unsigned tile_offset = sizeof(tile_t)
      * ((y / TILE_SIZE * spu.texture[unit].tiles_per_row) + (x / TILE_SIZE));
   ushort texel_offset = (ushort) 4
      * (ushort) (((ushort) (y % TILE_SIZE) * (ushort) TILE_SIZE) + (x % TILE_SIZE));
   vec_uint4 tmp;

   spu_dcache_fetch_unaligned((qword *) & tmp,
                              texture_ea + tile_offset + texel_offset,
                              4);
   return spu_extract(tmp, 0);
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
 * Get texture sample at texcoord.
 */
vector float
sample_texture_nearest(uint unit, vector float texcoord)
{
   vector float tc = spu_mul(texcoord, spu.texture[unit].tex_size);
   vector unsigned int itc = spu_convtu(tc, 0);  /* convert to int */
   itc = spu_and(itc, spu.texture[unit].tex_size_mask); /* mask (GL_REPEAT) */
   uint texel = get_texel(unit, itc);
   return spu_unpack_A8R8G8B8(texel);
}


/**
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

   /* GL_REPEAT wrap mode: */
   is = spu_and(is, spu.texture[unit].tex_size_x_mask);
   it = spu_and(it, spu.texture[unit].tex_size_y_mask);

   get_four_texels(unit, is, it, texels);

   /* convert four packed ARGBA pixels to float RRRR,GGGG,BBBB,AAAA */
   spu_unpack_A8R8G8B8_transpose4(texels, colors);
}


vector float
sample_texture_bilinear(uint unit, vector float texcoord)
{
   static const vec_uint4 offset_x = {0, 0, 1, 1};
   static const vec_uint4 offset_y = {0, 1, 0, 1};

   vector float tc = spu_mul(texcoord, spu.texture[unit].tex_size);
   tc = spu_add(tc, spu_splats(-0.5f));  /* half texel bias */

   /* integer texcoords S,T: */
   vec_uint4 itc = spu_convtu(tc, 0);  /* convert to int */

   vec_uint4 texels[4];
   
   /* setup texcoords for quad:
    *  +-----+-----+
    *  |x0,y0|x1,y1|
    *  +-----+-----+
    *  |x2,y2|x3,y3|
    *  +-----+-----+
    */
   vec_uint4 x = spu_splats(spu_extract(itc, 0));
   vec_uint4 y = spu_splats(spu_extract(itc, 1));
   x = spu_add(x, offset_x);
   y = spu_add(y, offset_y);

   /* GL_REPEAT wrap mode: */
   x = spu_and(x, spu.texture[unit].tex_size_x_mask);
   y = spu_and(y, spu.texture[unit].tex_size_y_mask);

   get_four_texels(unit, x, y, texels);

   /* integer A8R8G8B8 to float texel conversion */
   vector float texel00 = spu_unpack_A8R8G8B8(spu_extract(texels[0], 0));
   vector float texel01 = spu_unpack_A8R8G8B8(spu_extract(texels[1], 0));
   vector float texel10 = spu_unpack_A8R8G8B8(spu_extract(texels[2], 0));
   vector float texel11 = spu_unpack_A8R8G8B8(spu_extract(texels[3], 0));


   /* Compute weighting factors in [0,1]
    * Multiply texcoord by 1024, AND with 1023, convert back to float.
    */
   vector float tc1024 = spu_mul(tc, spu_splats(1024.0f));
   vector signed int itc1024 = spu_convts(tc1024, 0);
   itc1024 = spu_and(itc1024, spu_splats((1 << 10) - 1));
   vector float weight = spu_convtf(itc1024, 10);

   /* smeared frac and 1-frac */
   vector float sfrac = spu_splats(spu_extract(weight, 0));
   vector float tfrac = spu_splats(spu_extract(weight, 1));
   vector float sfrac1 = spu_sub(spu_splats(1.0f), sfrac);
   vector float tfrac1 = spu_sub(spu_splats(1.0f), tfrac);

   /* multiply the samples (colors) by the S/T weights */
   texel00 = spu_mul(spu_mul(texel00, sfrac1), tfrac1);
   texel10 = spu_mul(spu_mul(texel10, sfrac ), tfrac1);
   texel01 = spu_mul(spu_mul(texel01, sfrac1), tfrac );
   texel11 = spu_mul(spu_mul(texel11, sfrac ), tfrac );

   /* compute sum of weighted samples */
   vector float texel_sum = spu_add(texel00, texel01);
   texel_sum = spu_add(texel_sum, texel10);
   texel_sum = spu_add(texel_sum, texel11);

   return texel_sum;
}


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
