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


static uint
get_texel(uint unit, vec_uint4 coordinate)
{
   vec_uint4 tmp;
   unsigned x = spu_extract(coordinate, 0);
   unsigned y = spu_extract(coordinate, 1);
   const unsigned tiles_per_row = spu.texture[unit].width / TILE_SIZE;
   unsigned tile_offset = sizeof(tile_t) * ((y / TILE_SIZE * tiles_per_row) 
                                            + (x / TILE_SIZE));
   unsigned texel_offset = 4 * (((y % TILE_SIZE) * TILE_SIZE)
                                + (x % TILE_SIZE));

   spu_dcache_fetch_unaligned((qword *) & tmp,
                              spu.texture[unit].start + tile_offset + texel_offset,
                              4);
   return spu_extract(tmp, 0);
}


static void
get_four_texels(vec_uint4 x, vec_uint4 y, vec_uint4 *texels)
{
   const uint unit = 0;
   const unsigned texture_ea = (uintptr_t) spu.texture[unit].start;
   vec_uint4 tile_x = spu_rlmask(x, -5);
   vec_uint4 tile_y = spu_rlmask(y, -5);
   const qword offset_x = si_andi((qword) x, 0x1f);
   const qword offset_y = si_andi((qword) y, 0x1f);

   const qword tiles_per_row = (qword) spu_splats(spu.texture[unit].width / TILE_SIZE);
   const qword tile_size = (qword) spu_splats(sizeof(tile_t));

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
 * XXX this is extremely primitive for now.
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
   
   vec_uint4 x = spu_splats(spu_extract(itc, 0));
   vec_uint4 y = spu_splats(spu_extract(itc, 1));

   x = spu_add(x, offset_x);
   y = spu_add(y, offset_y);

   x = spu_and(x, spu.texture[unit].tex_size_x_mask);
   y = spu_and(y, spu.texture[unit].tex_size_y_mask);

   get_four_texels(x, y, texels);

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
