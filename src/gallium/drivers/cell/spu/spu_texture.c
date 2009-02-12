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


#include <math.h>

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
   uint lvl;
   for (lvl = 0; lvl < CELL_MAX_TEXTURE_LEVELS; lvl++) {
      uint unit = 0;
      uint bytes = 4 * spu.texture[unit].level[lvl].width
         * spu.texture[unit].level[lvl].height;

      if (spu.texture[unit].target == PIPE_TEXTURE_CUBE)
         bytes *= 6;
      else if (spu.texture[unit].target == PIPE_TEXTURE_3D)
         bytes *= spu.texture[unit].level[lvl].depth;

      spu_dcache_mark_dirty((unsigned) spu.texture[unit].level[lvl].start, bytes);
   }
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
get_four_texels(const struct spu_texture_level *tlevel, uint face,
                vec_int4 x, vec_int4 y,
                vec_uint4 *texels)
{
   unsigned texture_ea = (uintptr_t) tlevel->start;
   const vec_int4 tile_x = spu_rlmask(x, -5);  /* tile_x = x / 32 */
   const vec_int4 tile_y = spu_rlmask(y, -5);  /* tile_y = y / 32 */
   const qword offset_x = si_andi((qword) x, 0x1f); /* offset_x = x & 0x1f */
   const qword offset_y = si_andi((qword) y, 0x1f); /* offset_y = y & 0x1f */

   const qword tiles_per_row = (qword) spu_splats(tlevel->tiles_per_row);
   const qword tile_size = (qword) spu_splats((unsigned) sizeof(tile_t));

   qword tile_offset = si_mpya((qword) tile_y, tiles_per_row, (qword) tile_x);
   tile_offset = si_mpy((qword) tile_offset, tile_size);

   qword texel_offset = si_a(si_mpyui(offset_y, 32), offset_x);
   texel_offset = si_mpyui(texel_offset, 4);
   
   vec_uint4 offset = (vec_uint4) si_a(tile_offset, texel_offset);
   
   texture_ea = texture_ea + face * tlevel->bytes_per_image;

   spu_dcache_fetch_unaligned((qword *) & texels[0],
                              texture_ea + spu_extract(offset, 0), 4);
   spu_dcache_fetch_unaligned((qword *) & texels[1],
                              texture_ea + spu_extract(offset, 1), 4);
   spu_dcache_fetch_unaligned((qword *) & texels[2],
                              texture_ea + spu_extract(offset, 2), 4);
   spu_dcache_fetch_unaligned((qword *) & texels[3],
                              texture_ea + spu_extract(offset, 3), 4);
}


/** clamp vec to [0, max] */
static INLINE vector signed int
spu_clamp(vector signed int vec, vector signed int max)
{
   static const vector signed int zero = {0,0,0,0};
   vector unsigned int c;
   c = spu_cmpgt(vec, zero);    /* c = vec > zero ? ~0 : 0 */
   vec = spu_sel(zero, vec, c);
   c = spu_cmpgt(vec, max);    /* c = vec > max ? ~0 : 0 */
   vec = spu_sel(vec, max, c);
   return vec;
}



/**
 * Do nearest texture sampling for four pixels.
 * \param colors  returned colors in SOA format (rrrr, gggg, bbbb, aaaa).
 */
void
sample_texture_2d_nearest(vector float s, vector float t,
                          uint unit, uint level, uint face,
                          vector float colors[4])
{
   const struct spu_texture_level *tlevel = &spu.texture[unit].level[level];
   vector float ss = spu_mul(s, tlevel->scale_s);
   vector float tt = spu_mul(t, tlevel->scale_t);
   vector signed int is = spu_convts(ss, 0);
   vector signed int it = spu_convts(tt, 0);
   vec_uint4 texels[4];

   /* PIPE_TEX_WRAP_REPEAT */
   is = spu_and(is, tlevel->mask_s);
   it = spu_and(it, tlevel->mask_t);

   /* PIPE_TEX_WRAP_CLAMP */
   is = spu_clamp(is, tlevel->max_s);
   it = spu_clamp(it, tlevel->max_t);

   get_four_texels(tlevel, face, is, it, texels);

   /* convert four packed ARGBA pixels to float RRRR,GGGG,BBBB,AAAA */
   spu_unpack_A8R8G8B8_transpose4(texels, colors);
}


/**
 * Do bilinear texture sampling for four pixels.
 * \param colors  returned colors in SOA format (rrrr, gggg, bbbb, aaaa).
 */
void
sample_texture_2d_bilinear(vector float s, vector float t,
                           uint unit, uint level, uint face,
                           vector float colors[4])
{
   const struct spu_texture_level *tlevel = &spu.texture[unit].level[level];
   static const vector float half = {-0.5f, -0.5f, -0.5f, -0.5f};

   vector float ss = spu_madd(s, tlevel->scale_s, half);
   vector float tt = spu_madd(t, tlevel->scale_t, half);

   vector signed int is0 = spu_convts(ss, 0);
   vector signed int it0 = spu_convts(tt, 0);

   /* is + 1, it + 1 */
   vector signed int is1 = spu_add(is0, 1);
   vector signed int it1 = spu_add(it0, 1);

   /* PIPE_TEX_WRAP_REPEAT */
   is0 = spu_and(is0, tlevel->mask_s);
   it0 = spu_and(it0, tlevel->mask_t);
   is1 = spu_and(is1, tlevel->mask_s);
   it1 = spu_and(it1, tlevel->mask_t);

   /* PIPE_TEX_WRAP_CLAMP */
   is0 = spu_clamp(is0, tlevel->max_s);
   it0 = spu_clamp(it0, tlevel->max_t);
   is1 = spu_clamp(is1, tlevel->max_s);
   it1 = spu_clamp(it1, tlevel->max_t);

   /* get packed int texels */
   vector unsigned int texels[16];
   get_four_texels(tlevel, face, is0, it0, texels + 0);  /* upper-left */
   get_four_texels(tlevel, face, is1, it0, texels + 4);  /* upper-right */
   get_four_texels(tlevel, face, is0, it1, texels + 8);  /* lower-left */
   get_four_texels(tlevel, face, is1, it1, texels + 12); /* lower-right */

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
 * Bilinear filtering, using int instead of float arithmetic for computing
 * sample weights.
 */
void
sample_texture_2d_bilinear_int(vector float s, vector float t,
                               uint unit, uint level, uint face,
                               vector float colors[4])
{
   const struct spu_texture_level *tlevel = &spu.texture[unit].level[level];
   static const vector float half = {-0.5f, -0.5f, -0.5f, -0.5f};

   /* Scale texcoords by size of texture, and add half pixel bias */
   vector float ss = spu_madd(s, tlevel->scale_s, half);
   vector float tt = spu_madd(t, tlevel->scale_t, half);

   /* convert float coords to fixed-pt coords with 7 fraction bits */
   vector signed int is = spu_convts(ss, 7);  /* XXX really need floor() here */
   vector signed int it = spu_convts(tt, 7);  /* XXX really need floor() here */

   /* compute integer texel weights in [0, 127] */
   vector signed int sWeights0 = spu_and(is, 127);
   vector signed int tWeights0 = spu_and(it, 127);
   vector signed int sWeights1 = spu_sub(127, sWeights0);
   vector signed int tWeights1 = spu_sub(127, tWeights0);

   /* texel coords: is0 = is / 128, it0 = is / 128 */
   vector signed int is0 = spu_rlmask(is, -7);
   vector signed int it0 = spu_rlmask(it, -7);

   /* texel coords: i1 = is0 + 1, it1 = it0 + 1 */
   vector signed int is1 = spu_add(is0, 1);
   vector signed int it1 = spu_add(it0, 1);

   /* PIPE_TEX_WRAP_REPEAT */
   is0 = spu_and(is0, tlevel->mask_s);
   it0 = spu_and(it0, tlevel->mask_t);
   is1 = spu_and(is1, tlevel->mask_s);
   it1 = spu_and(it1, tlevel->mask_t);

   /* PIPE_TEX_WRAP_CLAMP */
   is0 = spu_clamp(is0, tlevel->max_s);
   it0 = spu_clamp(it0, tlevel->max_t);
   is1 = spu_clamp(is1, tlevel->max_s);
   it1 = spu_clamp(it1, tlevel->max_t);

   /* get packed int texels */
   vector unsigned int texels[16];
   get_four_texels(tlevel, face, is0, it0, texels + 0);  /* upper-left */
   get_four_texels(tlevel, face, is1, it0, texels + 4);  /* upper-right */
   get_four_texels(tlevel, face, is0, it1, texels + 8);  /* lower-left */
   get_four_texels(tlevel, face, is1, it1, texels + 12); /* lower-right */

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
   c0 = (vector unsigned int) si_mpy((qword) texel0, si_mpy((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpy((qword) texel4, si_mpy((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpy((qword) texel8, si_mpy((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpy((qword) texel12, si_mpy((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[0] = spu_convtf(cSum, 22);

   /* green */
   c0 = (vector unsigned int) si_mpy((qword) texel1, si_mpy((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpy((qword) texel5, si_mpy((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpy((qword) texel9, si_mpy((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpy((qword) texel13, si_mpy((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[1] = spu_convtf(cSum, 22);

   /* blue */
   c0 = (vector unsigned int) si_mpy((qword) texel2, si_mpy((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpy((qword) texel6, si_mpy((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpy((qword) texel10, si_mpy((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpy((qword) texel14, si_mpy((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[2] = spu_convtf(cSum, 22);

   /* alpha */
   c0 = (vector unsigned int) si_mpy((qword) texel3, si_mpy((qword) sWeights1, (qword) tWeights1)); /*ul*/
   c1 = (vector unsigned int) si_mpy((qword) texel7, si_mpy((qword) sWeights0, (qword) tWeights1)); /*ur*/
   c2 = (vector unsigned int) si_mpy((qword) texel11, si_mpy((qword) sWeights1, (qword) tWeights0)); /*ll*/
   c3 = (vector unsigned int) si_mpy((qword) texel15, si_mpy((qword) sWeights0, (qword) tWeights0)); /*lr*/
   cSum = spu_add(spu_add(c0, c1), spu_add(c2, c3));
   colors[3] = spu_convtf(cSum, 22);
}



/**
 * Compute level of detail factor from texcoords.
 */
static INLINE float
compute_lambda_2d(uint unit, vector float s, vector float t)
{
   uint baseLevel = 0;
   float width = spu.texture[unit].level[baseLevel].width;
   float height = spu.texture[unit].level[baseLevel].width;
   float dsdx = width * (spu_extract(s, 1) - spu_extract(s, 0));
   float dsdy = width * (spu_extract(s, 2) - spu_extract(s, 0));
   float dtdx = height * (spu_extract(t, 1) - spu_extract(t, 0));
   float dtdy = height * (spu_extract(t, 2) - spu_extract(t, 0));
#if 0
   /* ideal value */
   float x = dsdx * dsdx + dtdx * dtdx;
   float y = dsdy * dsdy + dtdy * dtdy;
   float rho = x > y ? x : y;
   rho = sqrtf(rho);
#else
   /* approximation */
   dsdx = fabsf(dsdx);
   dsdy = fabsf(dsdy);
   dtdx = fabsf(dtdx);
   dtdy = fabsf(dtdy);
   float rho = (dsdx + dsdy + dtdx + dtdy) * 0.5;
#endif
   float lambda = logf(rho) * 1.442695f; /* compute logbase2(rho) */
   return lambda;
}


/**
 * Blend two sets of colors according to weight.
 */
static void
blend_colors(vector float c0[4], const vector float c1[4], float weight)
{
   vector float t = spu_splats(weight);
   vector float dc0 = spu_sub(c1[0], c0[0]);
   vector float dc1 = spu_sub(c1[1], c0[1]);
   vector float dc2 = spu_sub(c1[2], c0[2]);
   vector float dc3 = spu_sub(c1[3], c0[3]);
   c0[0] = spu_madd(dc0, t, c0[0]);
   c0[1] = spu_madd(dc1, t, c0[1]);
   c0[2] = spu_madd(dc2, t, c0[2]);
   c0[3] = spu_madd(dc3, t, c0[3]);
}


/**
 * Texture sampling with level of detail selection and possibly mipmap
 * interpolation.
 */
void
sample_texture_2d_lod(vector float s, vector float t,
                      uint unit, uint level_ignored, uint face,
                      vector float colors[4])
{
   /*
    * Note that we're computing a lambda/lod here that's used for all
    * four pixels in the quad.
    */
   float lambda = compute_lambda_2d(unit, s, t);

   (void) face;
   (void) level_ignored;

   /* apply lod bias */
   lambda += spu.sampler[unit].lod_bias;

   /* clamp */
   if (lambda < spu.sampler[unit].min_lod)
      lambda = spu.sampler[unit].min_lod;
   else if (lambda > spu.sampler[unit].max_lod)
      lambda = spu.sampler[unit].max_lod;

   if (lambda <= 0.0f) {
      /* magnify */
      spu.mag_sample_texture_2d[unit](s, t, unit, 0, face, colors);
   }
   else {
      /* minify */
      if (spu.sampler[unit].min_img_filter == PIPE_TEX_FILTER_LINEAR) {
         /* sample two mipmap levels and interpolate */
         int level = (int) lambda;
         if (level > (int) spu.texture[unit].max_level)
            level = spu.texture[unit].max_level;
         spu.min_sample_texture_2d[unit](s, t, unit, level, face, colors);
         if (spu.sampler[unit].min_img_filter == PIPE_TEX_FILTER_LINEAR) {
            /* sample second mipmap level */
            float weight = lambda - (float) level;
            level++;
            if (level <= (int) spu.texture[unit].max_level) {
               vector float colors2[4];
               spu.min_sample_texture_2d[unit](s, t, unit, level, face, colors2);
               blend_colors(colors, colors2, weight);
            }
         }
      }
      else {
         /* sample one mipmap level */
         int level = (int) (lambda + 0.5f);
         if (level > (int) spu.texture[unit].max_level)
            level = spu.texture[unit].max_level;
         spu.min_sample_texture_2d[unit](s, t, unit, level, face, colors);
      }
   }
}


/** XXX need a SIMD version of this */
static unsigned
choose_cube_face(float rx, float ry, float rz, float *newS, float *newT)
{
   /*
      major axis
      direction     target                             sc     tc    ma
      ----------    -------------------------------    ---    ---   ---
       +rx          TEXTURE_CUBE_MAP_POSITIVE_X_EXT    -rz    -ry   rx
       -rx          TEXTURE_CUBE_MAP_NEGATIVE_X_EXT    +rz    -ry   rx
       +ry          TEXTURE_CUBE_MAP_POSITIVE_Y_EXT    +rx    +rz   ry
       -ry          TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT    +rx    -rz   ry
       +rz          TEXTURE_CUBE_MAP_POSITIVE_Z_EXT    +rx    -ry   rz
       -rz          TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT    -rx    -ry   rz
   */
   const float arx = fabsf(rx);
   const float ary = fabsf(ry);
   const float arz = fabsf(rz);
   unsigned face;
   float sc, tc, ma;

   if (arx > ary && arx > arz) {
      if (rx >= 0.0F) {
         face = PIPE_TEX_FACE_POS_X;
         sc = -rz;
         tc = -ry;
         ma = arx;
      }
      else {
         face = PIPE_TEX_FACE_NEG_X;
         sc = rz;
         tc = -ry;
         ma = arx;
      }
   }
   else if (ary > arx && ary > arz) {
      if (ry >= 0.0F) {
         face = PIPE_TEX_FACE_POS_Y;
         sc = rx;
         tc = rz;
         ma = ary;
      }
      else {
         face = PIPE_TEX_FACE_NEG_Y;
         sc = rx;
         tc = -rz;
         ma = ary;
      }
   }
   else {
      if (rz > 0.0F) {
         face = PIPE_TEX_FACE_POS_Z;
         sc = rx;
         tc = -ry;
         ma = arz;
      }
      else {
         face = PIPE_TEX_FACE_NEG_Z;
         sc = -rx;
         tc = -ry;
         ma = arz;
      }
   }

   *newS = (sc / ma + 1.0F) * 0.5F;
   *newT = (tc / ma + 1.0F) * 0.5F;

   return face;
}



void
sample_texture_cube(vector float s, vector float t, vector float r,
                    uint unit, vector float colors[4])
{
   uint p, faces[4], level = 0;
   float newS[4], newT[4];

   /* Compute cube faces referenced by the four sets of texcoords.
    * XXX we should SIMD-ize this.
    */
   for (p = 0; p < 4; p++) {      
      float rx = spu_extract(s, p);
      float ry = spu_extract(t, p);
      float rz = spu_extract(r, p);
      faces[p] = choose_cube_face(rx, ry, rz, &newS[p], &newT[p]);
   }

   if (faces[0] == faces[1] &&
       faces[0] == faces[2] &&
       faces[0] == faces[3]) {
      /* GOOD!  All four texcoords refer to the same cube face */
      s = (vector float) {newS[0], newS[1], newS[2], newS[3]};
      t = (vector float) {newT[0], newT[1], newT[2], newT[3]};
      spu.sample_texture_2d[unit](s, t, unit, level, faces[0], colors);
   }
   else {
      /* BAD!  The four texcoords refer to different faces */
      for (p = 0; p < 4; p++) {      
         vector float c[4];

         spu.sample_texture_2d[unit](spu_splats(newS[p]), spu_splats(newT[p]),
                                     unit, level, faces[p], c);

         float red = spu_extract(c[0], p);
         float green = spu_extract(c[1], p);
         float blue = spu_extract(c[2], p);
         float alpha = spu_extract(c[3], p);

         colors[0] = spu_insert(red,   colors[0], p);
         colors[1] = spu_insert(green, colors[1], p);
         colors[2] = spu_insert(blue,  colors[2], p);
         colors[3] = spu_insert(alpha, colors[3], p);
      }
   }
}
