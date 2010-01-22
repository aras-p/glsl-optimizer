/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights reserved.
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
 * Texture sampling
 *
 * Authors:
 *   Brian Paul
 */

#include "lp_context.h"
#include "lp_quad.h"
#include "lp_texture.h"
#include "lp_tex_sample.h"
#include "lp_tex_cache.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"



/*
 * Note, the FRAC macro has to work perfectly.  Otherwise you'll sometimes
 * see 1-pixel bands of improperly weighted linear-filtered textures.
 * The tests/texwrap.c demo is a good test.
 * Also note, FRAC(x) doesn't truly return the fractional part of x for x < 0.
 * Instead, if x < 0 then FRAC(x) = 1 - true_frac(x).
 */
#define FRAC(f)  ((f) - util_ifloor(f))


/**
 * Linear interpolation macro
 */
static INLINE float
lerp(float a, float v0, float v1)
{
   return v0 + a * (v1 - v0);
}


/**
 * Do 2D/biliner interpolation of float values.
 * v00, v10, v01 and v11 are typically four texture samples in a square/box.
 * a and b are the horizontal and vertical interpolants.
 * It's important that this function is inlined when compiled with
 * optimization!  If we find that's not true on some systems, convert
 * to a macro.
 */
static INLINE float
lerp_2d(float a, float b,
        float v00, float v10, float v01, float v11)
{
   const float temp0 = lerp(a, v00, v10);
   const float temp1 = lerp(a, v01, v11);
   return lerp(b, temp0, temp1);
}


/**
 * As above, but 3D interpolation of 8 values.
 */
static INLINE float
lerp_3d(float a, float b, float c,
        float v000, float v100, float v010, float v110,
        float v001, float v101, float v011, float v111)
{
   const float temp0 = lerp_2d(a, b, v000, v100, v010, v110);
   const float temp1 = lerp_2d(a, b, v001, v101, v011, v111);
   return lerp(c, temp0, temp1);
}



/**
 * If A is a signed integer, A % B doesn't give the right value for A < 0
 * (in terms of texture repeat).  Just casting to unsigned fixes that.
 */
#define REMAINDER(A, B) ((unsigned) (A) % (unsigned) (B))


/**
 * Apply texture coord wrapping mode and return integer texture indexes
 * for a vector of four texcoords (S or T or P).
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the incoming texcoords
 * \param size  the texture image size
 * \param icoord  returns the integer texcoords
 * \return  integer texture index
 */
static INLINE void
nearest_texcoord_4(unsigned wrapMode, const float s[4], unsigned size,
                   int icoord[4])
{
   uint ch;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      /* s limited to [0,1) */
      /* i limited to [0,size-1] */
      for (ch = 0; ch < 4; ch++) {
         int i = util_ifloor(s[ch] * size);
         icoord[ch] = REMAINDER(i, size);
      }
      return;
   case PIPE_TEX_WRAP_CLAMP:
      /* s limited to [0,1] */
      /* i limited to [0,size-1] */
      for (ch = 0; ch < 4; ch++) {
         if (s[ch] <= 0.0F)
            icoord[ch] = 0;
         else if (s[ch] >= 1.0F)
            icoord[ch] = size - 1;
         else
            icoord[ch] = util_ifloor(s[ch] * size);
      }
      return;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const float min = 1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            if (s[ch] < min)
               icoord[ch] = 0;
            else if (s[ch] > max)
               icoord[ch] = size - 1;
            else
               icoord[ch] = util_ifloor(s[ch] * size);
         }
      }
      return;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [-1, size] */
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            if (s[ch] <= min)
               icoord[ch] = -1;
            else if (s[ch] >= max)
               icoord[ch] = size;
            else
               icoord[ch] = util_ifloor(s[ch] * size);
         }
      }
      return;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      {
         const float min = 1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            const int flr = util_ifloor(s[ch]);
            float u;
            if (flr & 1)
               u = 1.0F - (s[ch] - (float) flr);
            else
               u = s[ch] - (float) flr;
            if (u < min)
               icoord[ch] = 0;
            else if (u > max)
               icoord[ch] = size - 1;
            else
               icoord[ch] = util_ifloor(u * size);
         }
      }
      return;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      for (ch = 0; ch < 4; ch++) {
         /* s limited to [0,1] */
         /* i limited to [0,size-1] */
         const float u = fabsf(s[ch]);
         if (u <= 0.0F)
            icoord[ch] = 0;
         else if (u >= 1.0F)
            icoord[ch] = size - 1;
         else
            icoord[ch] = util_ifloor(u * size);
      }
      return;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const float min = 1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            const float u = fabsf(s[ch]);
            if (u < min)
               icoord[ch] = 0;
            else if (u > max)
               icoord[ch] = size - 1;
            else
               icoord[ch] = util_ifloor(u * size);
         }
      }
      return;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            const float u = fabsf(s[ch]);
            if (u < min)
               icoord[ch] = -1;
            else if (u > max)
               icoord[ch] = size;
            else
               icoord[ch] = util_ifloor(u * size);
         }
      }
      return;
   default:
      assert(0);
   }
}


/**
 * Used to compute texel locations for linear sampling for four texcoords.
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the texcoords
 * \param size  the texture image size
 * \param icoord0  returns first texture indexes
 * \param icoord1  returns second texture indexes (usually icoord0 + 1)
 * \param w  returns blend factor/weight between texture indexes
 * \param icoord  returns the computed integer texture coords
 */
static INLINE void
linear_texcoord_4(unsigned wrapMode, const float s[4], unsigned size,
                  int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;

   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      for (ch = 0; ch < 4; ch++) {
         float u = s[ch] * size - 0.5F;
         icoord0[ch] = REMAINDER(util_ifloor(u), size);
         icoord1[ch] = REMAINDER(icoord0[ch] + 1, size);
         w[ch] = FRAC(u);
      }
      break;;
   case PIPE_TEX_WRAP_CLAMP:
      for (ch = 0; ch < 4; ch++) {
         float u = CLAMP(s[ch], 0.0F, 1.0F);
         u = u * size - 0.5f;
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         w[ch] = FRAC(u);
      }
      break;;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      for (ch = 0; ch < 4; ch++) {
         float u = CLAMP(s[ch], 0.0F, 1.0F);
         u = u * size - 0.5f;
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         if (icoord0[ch] < 0)
            icoord0[ch] = 0;
         if (icoord1[ch] >= (int) size)
            icoord1[ch] = size - 1;
         w[ch] = FRAC(u);
      }
      break;;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            float u = CLAMP(s[ch], min, max);
            u = u * size - 0.5f;
            icoord0[ch] = util_ifloor(u);
            icoord1[ch] = icoord0[ch] + 1;
            w[ch] = FRAC(u);
         }
      }
      break;;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      for (ch = 0; ch < 4; ch++) {
         const int flr = util_ifloor(s[ch]);
         float u;
         if (flr & 1)
            u = 1.0F - (s[ch] - (float) flr);
         else
            u = s[ch] - (float) flr;
         u = u * size - 0.5F;
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         if (icoord0[ch] < 0)
            icoord0[ch] = 0;
         if (icoord1[ch] >= (int) size)
            icoord1[ch] = size - 1;
         w[ch] = FRAC(u);
      }
      break;;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      for (ch = 0; ch < 4; ch++) {
         float u = fabsf(s[ch]);
         if (u >= 1.0F)
            u = (float) size;
         else
            u *= size;
         u -= 0.5F;
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         w[ch] = FRAC(u);
      }
      break;;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      for (ch = 0; ch < 4; ch++) {
         float u = fabsf(s[ch]);
         if (u >= 1.0F)
            u = (float) size;
         else
            u *= size;
         u -= 0.5F;
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         if (icoord0[ch] < 0)
            icoord0[ch] = 0;
         if (icoord1[ch] >= (int) size)
            icoord1[ch] = size - 1;
         w[ch] = FRAC(u);
      }
      break;;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         for (ch = 0; ch < 4; ch++) {
            float u = fabsf(s[ch]);
            if (u <= min)
               u = min * size;
            else if (u >= max)
               u = max * size;
            else
               u *= size;
            u -= 0.5F;
            icoord0[ch] = util_ifloor(u);
            icoord1[ch] = icoord0[ch] + 1;
            w[ch] = FRAC(u);
         }
      }
      break;;
   default:
      assert(0);
   }
}


/**
 * For RECT textures / unnormalized texcoords
 * Only a subset of wrap modes supported.
 */
static INLINE void
nearest_texcoord_unnorm_4(unsigned wrapMode, const float s[4], unsigned size,
                          int icoord[4])
{
   uint ch;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_CLAMP:
      for (ch = 0; ch < 4; ch++) {
         int i = util_ifloor(s[ch]);
         icoord[ch]= CLAMP(i, 0, (int) size-1);
      }
      return;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      /* fall-through */
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      for (ch = 0; ch < 4; ch++) {
         icoord[ch]= util_ifloor( CLAMP(s[ch], 0.5F, (float) size - 0.5F) );
      }
      return;
   default:
      assert(0);
   }
}


/**
 * For RECT textures / unnormalized texcoords.
 * Only a subset of wrap modes supported.
 */
static INLINE void
linear_texcoord_unnorm_4(unsigned wrapMode, const float s[4], unsigned size,
                         int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_CLAMP:
      for (ch = 0; ch < 4; ch++) {
         /* Not exactly what the spec says, but it matches NVIDIA output */
         float u = CLAMP(s[ch] - 0.5F, 0.0f, (float) size - 1.0f);
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         w[ch] = FRAC(u);
      }
      return;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      /* fall-through */
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      for (ch = 0; ch < 4; ch++) {
         float u = CLAMP(s[ch], 0.5F, (float) size - 0.5F);
         u -= 0.5F;
         icoord0[ch] = util_ifloor(u);
         icoord1[ch] = icoord0[ch] + 1;
         if (icoord1[ch] > (int) size - 1)
            icoord1[ch] = size - 1;
         w[ch] = FRAC(u);
      }
      break;
   default:
      assert(0);
   }
}


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
   const float arx = fabsf(rx), ary = fabsf(ry), arz = fabsf(rz);
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

   *newS = ( sc / ma + 1.0F ) * 0.5F;
   *newT = ( tc / ma + 1.0F ) * 0.5F;

   return face;
}


/**
 * Examine the quad's texture coordinates to compute the partial
 * derivatives w.r.t X and Y, then compute lambda (level of detail).
 *
 * This is only done for fragment shaders, not vertex shaders.
 */
static float
compute_lambda(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias)
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;
   float rho, lambda;

   if (samp->processor == TGSI_PROCESSOR_VERTEX)
      return lodbias;

   assert(sampler->normalized_coords);

   assert(s);
   {
      float dsdx = s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT];
      float dsdy = s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT];
      dsdx = fabsf(dsdx);
      dsdy = fabsf(dsdy);
      rho = MAX2(dsdx, dsdy) * texture->width[0];
   }
   if (t) {
      float dtdx = t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT];
      float dtdy = t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT];
      float max;
      dtdx = fabsf(dtdx);
      dtdy = fabsf(dtdy);
      max = MAX2(dtdx, dtdy) * texture->height[0];
      rho = MAX2(rho, max);
   }
   if (p) {
      float dpdx = p[QUAD_BOTTOM_RIGHT] - p[QUAD_BOTTOM_LEFT];
      float dpdy = p[QUAD_TOP_LEFT]     - p[QUAD_BOTTOM_LEFT];
      float max;
      dpdx = fabsf(dpdx);
      dpdy = fabsf(dpdy);
      max = MAX2(dpdx, dpdy) * texture->depth[0];
      rho = MAX2(rho, max);
   }

   lambda = util_fast_log2(rho);
   lambda += lodbias + sampler->lod_bias;
   lambda = CLAMP(lambda, sampler->min_lod, sampler->max_lod);

   return lambda;
}


/**
 * Do several things here:
 * 1. Compute lambda from the texcoords, if needed
 * 2. Determine if we're minifying or magnifying
 * 3. If minifying, choose mipmap levels
 * 4. Return image filter to use within mipmap images
 * \param level0  Returns first mipmap level to sample from
 * \param level1  Returns second mipmap level to sample from
 * \param levelBlend  Returns blend factor between levels, in [0,1]
 * \param imgFilter  Returns either the min or mag filter, depending on lambda
 */
static void
choose_mipmap_levels(struct tgsi_sampler *tgsi_sampler,
                     const float s[QUAD_SIZE],
                     const float t[QUAD_SIZE],
                     const float p[QUAD_SIZE],
                     float lodbias,
                     unsigned *level0, unsigned *level1, float *levelBlend,
                     unsigned *imgFilter)
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;

   if (sampler->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
      /* no mipmap selection needed */
      *level0 = *level1 = CLAMP((int) sampler->min_lod,
                                0, (int) texture->last_level);

      if (sampler->min_img_filter != sampler->mag_img_filter) {
         /* non-mipmapped texture, but still need to determine if doing
          * minification or magnification.
          */
         float lambda = compute_lambda(tgsi_sampler, s, t, p, lodbias);
         if (lambda <= 0.0) {
            *imgFilter = sampler->mag_img_filter;
         }
         else {
            *imgFilter = sampler->min_img_filter;
         }
      }
      else {
         *imgFilter = sampler->mag_img_filter;
      }
   }
   else {
      float lambda = compute_lambda(tgsi_sampler, s, t, p, lodbias);

      if (lambda <= 0.0) { /* XXX threshold depends on the filter */
         /* magnifying */
         *imgFilter = sampler->mag_img_filter;
         *level0 = *level1 = 0;
      }
      else {
         /* minifying */
         *imgFilter = sampler->min_img_filter;

         /* choose mipmap level(s) and compute the blend factor between them */
         if (sampler->min_mip_filter == PIPE_TEX_MIPFILTER_NEAREST) {
            /* Nearest mipmap level */
            const int lvl = (int) (lambda + 0.5);
            *level0 =
            *level1 = CLAMP(lvl, 0, (int) texture->last_level);
         }
         else {
            /* Linear interpolation between mipmap levels */
            const int lvl = (int) lambda;
            *level0 = CLAMP(lvl,     0, (int) texture->last_level);
            *level1 = CLAMP(lvl + 1, 0, (int) texture->last_level);
            *levelBlend = FRAC(lambda);  /* blending weight between levels */
         }
      }
   }
}


/**
 * Get a texel from a texture, using the texture tile cache.
 *
 * \param face  the cube face in 0..5
 * \param level  the mipmap level
 * \param x  the x coord of texel within 2D image
 * \param y  the y coord of texel within 2D image
 * \param z  which slice of a 3D texture
 * \param rgba  the quad to put the texel/color into
 * \param j  which element of the rgba quad to write to
 *
 * XXX maybe move this into lp_tile_cache.c and merge with the
 * lp_get_cached_tile_tex() function.  Also, get 4 texels instead of 1...
 */
static void
get_texel_quad_2d(const struct tgsi_sampler *tgsi_sampler,
                  unsigned face, unsigned level, int x, int y, 
                  const uint8_t *out[4])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);

   const struct llvmpipe_cached_tex_tile *tile
      = lp_get_cached_tex_tile(samp->cache,
                               tex_tile_address(x, y, 0, face, level));

   y %= TEX_TILE_SIZE;
   x %= TEX_TILE_SIZE;
      
   out[0] = &tile->color[y  ][x  ][0];
   out[1] = &tile->color[y  ][x+1][0];
   out[2] = &tile->color[y+1][x  ][0];
   out[3] = &tile->color[y+1][x+1][0];
}

static INLINE const uint8_t *
get_texel_2d_ptr(const struct tgsi_sampler *tgsi_sampler,
                 unsigned face, unsigned level, int x, int y)
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);

   const struct llvmpipe_cached_tex_tile *tile
      = lp_get_cached_tex_tile(samp->cache,
                               tex_tile_address(x, y, 0, face, level));

   y %= TEX_TILE_SIZE;
   x %= TEX_TILE_SIZE;

   return &tile->color[y][x][0];
}


static void
get_texel_quad_2d_mt(const struct tgsi_sampler *tgsi_sampler,
                     unsigned face, unsigned level, 
                     int x0, int y0, 
                     int x1, int y1,
                     const uint8_t *out[4])
{
   unsigned i;

   for (i = 0; i < 4; i++) {
      unsigned tx = (i & 1) ? x1 : x0;
      unsigned ty = (i >> 1) ? y1 : y0;

      out[i] = get_texel_2d_ptr( tgsi_sampler, face, level, tx, ty );
   }
}

static void
get_texel(const struct tgsi_sampler *tgsi_sampler,
                 unsigned face, unsigned level, int x, int y, int z,
                 float rgba[NUM_CHANNELS][QUAD_SIZE], unsigned j)
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;

   if (x < 0 || x >= (int) texture->width[level] ||
       y < 0 || y >= (int) texture->height[level] ||
       z < 0 || z >= (int) texture->depth[level]) {
      rgba[0][j] = sampler->border_color[0];
      rgba[1][j] = sampler->border_color[1];
      rgba[2][j] = sampler->border_color[2];
      rgba[3][j] = sampler->border_color[3];
   }
   else {
      const unsigned tx = x % TEX_TILE_SIZE;
      const unsigned ty = y % TEX_TILE_SIZE;
      const struct llvmpipe_cached_tex_tile *tile;

      tile = lp_get_cached_tex_tile(samp->cache,
                                    tex_tile_address(x, y, z, face, level));

      rgba[0][j] = ubyte_to_float(tile->color[ty][tx][0]);
      rgba[1][j] = ubyte_to_float(tile->color[ty][tx][1]);
      rgba[2][j] = ubyte_to_float(tile->color[ty][tx][2]);
      rgba[3][j] = ubyte_to_float(tile->color[ty][tx][3]);
      if (0)
      {
         debug_printf("Get texel %f %f %f %f from %s\n",
                      rgba[0][j], rgba[1][j], rgba[2][j], rgba[3][j],
                      pf_name(texture->format));
      }
   }
}


/**
 * Compare texcoord 'p' (aka R) against texture value 'rgba[0]'
 * When we sampled the depth texture, the depth value was put into all
 * RGBA channels.  We look at the red channel here.
 * \param rgba  quad of (depth) texel values
 * \param p  texture 'P' components for four pixels in quad
 * \param j  which pixel in the quad to test [0..3]
 */
static INLINE void
shadow_compare(const struct pipe_sampler_state *sampler,
               float rgba[NUM_CHANNELS][QUAD_SIZE],
               const float p[QUAD_SIZE],
               uint j)
{
   int k;
   switch (sampler->compare_func) {
   case PIPE_FUNC_LESS:
      k = p[j] < rgba[0][j];
      break;
   case PIPE_FUNC_LEQUAL:
      k = p[j] <= rgba[0][j];
      break;
   case PIPE_FUNC_GREATER:
      k = p[j] > rgba[0][j];
      break;
   case PIPE_FUNC_GEQUAL:
      k = p[j] >= rgba[0][j];
      break;
   case PIPE_FUNC_EQUAL:
      k = p[j] == rgba[0][j];
      break;
   case PIPE_FUNC_NOTEQUAL:
      k = p[j] != rgba[0][j];
      break;
   case PIPE_FUNC_ALWAYS:
      k = 1;
      break;
   case PIPE_FUNC_NEVER:
      k = 0;
      break;
   default:
      k = 0;
      assert(0);
      break;
   }

   /* XXX returning result for default GL_DEPTH_TEXTURE_MODE = GL_LUMINANCE */
   rgba[0][j] = rgba[1][j] = rgba[2][j] = (float) k;
   rgba[3][j] = 1.0F;
}


/**
 * As above, but do four z/texture comparisons.
 */
static INLINE void
shadow_compare4(const struct pipe_sampler_state *sampler,
                float rgba[NUM_CHANNELS][QUAD_SIZE],
                const float p[QUAD_SIZE])
{
   int j, k0, k1, k2, k3;
   float val;

   /* compare four texcoords vs. four texture samples */
   switch (sampler->compare_func) {
   case PIPE_FUNC_LESS:
      k0 = p[0] < rgba[0][0];
      k1 = p[1] < rgba[0][1];
      k2 = p[2] < rgba[0][2];
      k3 = p[3] < rgba[0][3];
      break;
   case PIPE_FUNC_LEQUAL:
      k0 = p[0] <= rgba[0][0];
      k1 = p[1] <= rgba[0][1];
      k2 = p[2] <= rgba[0][2];
      k3 = p[3] <= rgba[0][3];
      break;
   case PIPE_FUNC_GREATER:
      k0 = p[0] > rgba[0][0];
      k1 = p[1] > rgba[0][1];
      k2 = p[2] > rgba[0][2];
      k3 = p[3] > rgba[0][3];
      break;
   case PIPE_FUNC_GEQUAL:
      k0 = p[0] >= rgba[0][0];
      k1 = p[1] >= rgba[0][1];
      k2 = p[2] >= rgba[0][2];
      k3 = p[3] >= rgba[0][3];
      break;
   case PIPE_FUNC_EQUAL:
      k0 = p[0] == rgba[0][0];
      k1 = p[1] == rgba[0][1];
      k2 = p[2] == rgba[0][2];
      k3 = p[3] == rgba[0][3];
      break;
   case PIPE_FUNC_NOTEQUAL:
      k0 = p[0] != rgba[0][0];
      k1 = p[1] != rgba[0][1];
      k2 = p[2] != rgba[0][2];
      k3 = p[3] != rgba[0][3];
      break;
   case PIPE_FUNC_ALWAYS:
      k0 = k1 = k2 = k3 = 1;
      break;
   case PIPE_FUNC_NEVER:
      k0 = k1 = k2 = k3 = 0;
      break;
   default:
      k0 = k1 = k2 = k3 = 0;
      assert(0);
      break;
   }

   /* convert four pass/fail values to an intensity in [0,1] */
   val = 0.25F * (k0 + k1 + k2 + k3);

   /* XXX returning result for default GL_DEPTH_TEXTURE_MODE = GL_LUMINANCE */
   for (j = 0; j < 4; j++) {
      rgba[0][j] = rgba[1][j] = rgba[2][j] = val;
      rgba[3][j] = 1.0F;
   }
}



static void
lp_get_samples_2d_linear_repeat_POT(struct tgsi_sampler *tgsi_sampler,
                                    const float s[QUAD_SIZE],
                                    const float t[QUAD_SIZE],
                                    const float p[QUAD_SIZE],
                                    float lodbias,
                                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   unsigned  j;
   unsigned level = samp->level;
   unsigned xpot = 1 << (samp->xpot - level);
   unsigned ypot = 1 << (samp->ypot - level);
   unsigned xmax = (xpot - 1) & (TEX_TILE_SIZE - 1); /* MIN2(TEX_TILE_SIZE, xpot) - 1; */
   unsigned ymax = (ypot - 1) & (TEX_TILE_SIZE - 1); /* MIN2(TEX_TILE_SIZE, ypot) - 1; */
      
   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      float u = s[j] * xpot - 0.5F;
      float v = t[j] * ypot - 0.5F;

      int uflr = util_ifloor(u);
      int vflr = util_ifloor(v);

      float xw = u - (float)uflr;
      float yw = v - (float)vflr;

      int x0 = uflr & (xpot - 1);
      int y0 = vflr & (ypot - 1);

      const uint8_t *tx[4];
      

      /* Can we fetch all four at once:
       */
      if (x0 < xmax && y0 < ymax)
      {
         get_texel_quad_2d(tgsi_sampler, 0, level, x0, y0, tx);
      }
      else 
      {
         unsigned x1 = (x0 + 1) & (xpot - 1);
         unsigned y1 = (y0 + 1) & (ypot - 1);
         get_texel_quad_2d_mt(tgsi_sampler, 0, level, 
                              x0, y0, x1, y1, tx);
      }


      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp_2d(xw, yw, 
                              ubyte_to_float(tx[0][c]), ubyte_to_float(tx[1][c]),
                              ubyte_to_float(tx[2][c]), ubyte_to_float(tx[3][c]));
      }
   }
}


static void
lp_get_samples_2d_nearest_repeat_POT(struct tgsi_sampler *tgsi_sampler,
                                     const float s[QUAD_SIZE],
                                     const float t[QUAD_SIZE],
                                     const float p[QUAD_SIZE],
                                     float lodbias,
                                     float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   unsigned  j;
   unsigned level = samp->level;
   unsigned xpot = 1 << (samp->xpot - level);
   unsigned ypot = 1 << (samp->ypot - level);

   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      float u = s[j] * xpot;
      float v = t[j] * ypot;

      int uflr = util_ifloor(u);
      int vflr = util_ifloor(v);

      int x0 = uflr & (xpot - 1);
      int y0 = vflr & (ypot - 1);

      const uint8_t *out = get_texel_2d_ptr(tgsi_sampler, 0, level, x0, y0);

      for (c = 0; c < 4; c++) {
         rgba[c][j] = ubyte_to_float(out[c]);
      }
   }
}


static void
lp_get_samples_2d_nearest_clamp_POT(struct tgsi_sampler *tgsi_sampler,
                                     const float s[QUAD_SIZE],
                                     const float t[QUAD_SIZE],
                                     const float p[QUAD_SIZE],
                                     float lodbias,
                                     float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   unsigned  j;
   unsigned level = samp->level;
   unsigned xpot = 1 << (samp->xpot - level);
   unsigned ypot = 1 << (samp->ypot - level);

   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      float u = s[j] * xpot;
      float v = t[j] * ypot;

      int x0, y0;
      const uint8_t *out;

      x0 = util_ifloor(u);
      if (x0 < 0) 
         x0 = 0;
      else if (x0 > xpot - 1)
         x0 = xpot - 1;

      y0 = util_ifloor(v);
      if (y0 < 0) 
         y0 = 0;
      else if (y0 > ypot - 1)
         y0 = ypot - 1;
      
      out = get_texel_2d_ptr(tgsi_sampler, 0, level, x0, y0);

      for (c = 0; c < 4; c++) {
         rgba[c][j] = ubyte_to_float(out[c]);
      }
   }
}


static void
lp_get_samples_2d_linear_mip_linear_repeat_POT(struct tgsi_sampler *tgsi_sampler,
                                               const float s[QUAD_SIZE],
                                               const float t[QUAD_SIZE],
                                               const float p[QUAD_SIZE],
                                               float lodbias,
                                               float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   int level0;
   float lambda;

   lambda = compute_lambda(tgsi_sampler, s, t, p, lodbias);
   level0 = (int)lambda;

   if (lambda < 0.0) { 
      samp->level = 0;
      lp_get_samples_2d_linear_repeat_POT( tgsi_sampler,
                                           s, t, p, 0, rgba );
   }
   else if (level0 >= texture->last_level) {
      samp->level = texture->last_level;
      lp_get_samples_2d_linear_repeat_POT( tgsi_sampler,
                                           s, t, p, 0, rgba );
   }
   else {
      float levelBlend = lambda - level0;
      float rgba0[4][4];
      float rgba1[4][4];
      int c,j;

      samp->level = level0;
      lp_get_samples_2d_linear_repeat_POT( tgsi_sampler,
                                           s, t, p, 0, rgba0 );

      samp->level = level0+1;
      lp_get_samples_2d_linear_repeat_POT( tgsi_sampler,
                                           s, t, p, 0, rgba1 );

      for (j = 0; j < QUAD_SIZE; j++) {
         for (c = 0; c < 4; c++) {
            rgba[c][j] = lerp(levelBlend, rgba0[c][j], rgba1[c][j]);
         }
      }
   }
}

/**
 * Common code for sampling 1D/2D/cube textures.
 * Could probably extend for 3D...
 */
static void
lp_get_samples_2d_common(struct tgsi_sampler *tgsi_sampler,
                         const float s[QUAD_SIZE],
                         const float t[QUAD_SIZE],
                         const float p[QUAD_SIZE],
                         float lodbias,
                         float rgba[NUM_CHANNELS][QUAD_SIZE],
                         const unsigned faces[4])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;
   unsigned level0, level1, j, imgFilter;
   int width, height;
   float levelBlend = 0.0F;

   choose_mipmap_levels(tgsi_sampler, s, t, p, 
                        lodbias,
                        &level0, &level1, &levelBlend, &imgFilter);

   assert(sampler->normalized_coords);

   width = texture->width[level0];
   height = texture->height[level0];

   assert(width > 0);

   switch (imgFilter) {
   case PIPE_TEX_FILTER_NEAREST:
      {
         int x[4], y[4];
         nearest_texcoord_4(sampler->wrap_s, s, width, x);
         nearest_texcoord_4(sampler->wrap_t, t, height, y);

         for (j = 0; j < QUAD_SIZE; j++) {
            get_texel(tgsi_sampler, faces[j], level0, x[j], y[j], 0, rgba, j);
            if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
               shadow_compare(sampler, rgba, p, j);
            }

            if (level0 != level1) {
               /* get texels from second mipmap level and blend */
               float rgba2[4][4];
               unsigned c;
               x[j] /= 2;
               y[j] /= 2;
               get_texel(tgsi_sampler, faces[j], level1, x[j], y[j], 0,
                         rgba2, j);
               if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE){
                  shadow_compare(sampler, rgba2, p, j);
               }

               for (c = 0; c < NUM_CHANNELS; c++) {
                  rgba[c][j] = lerp(levelBlend, rgba[c][j], rgba2[c][j]);
               }
            }
         }
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      {
         int x0[4], y0[4], x1[4], y1[4];
         float xw[4], yw[4]; /* weights */

         linear_texcoord_4(sampler->wrap_s, s, width, x0, x1, xw);
         linear_texcoord_4(sampler->wrap_t, t, height, y0, y1, yw);

         for (j = 0; j < QUAD_SIZE; j++) {
            float tx[4][4]; /* texels */
            int c;
            get_texel(tgsi_sampler, faces[j], level0, x0[j], y0[j], 0, tx, 0);
            get_texel(tgsi_sampler, faces[j], level0, x1[j], y0[j], 0, tx, 1);
            get_texel(tgsi_sampler, faces[j], level0, x0[j], y1[j], 0, tx, 2);
            get_texel(tgsi_sampler, faces[j], level0, x1[j], y1[j], 0, tx, 3);
            if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
               shadow_compare4(sampler, tx, p);
            }

            /* interpolate R, G, B, A */
            for (c = 0; c < 4; c++) {
               rgba[c][j] = lerp_2d(xw[j], yw[j],
                                    tx[c][0], tx[c][1],
                                    tx[c][2], tx[c][3]);
            }

            if (level0 != level1) {
               /* get texels from second mipmap level and blend */
               float rgba2[4][4];

               /* XXX: This is incorrect -- will often end up with (x0
                *  == x1 && y0 == y1), meaning that we fetch the same
                *  texel four times and linearly interpolate between
                *  identical values.  The correct approach would be to
                *  call linear_texcoord again for the second level.
                */
               x0[j] /= 2;
               y0[j] /= 2;
               x1[j] /= 2;
               y1[j] /= 2;
               get_texel(tgsi_sampler, faces[j], level1, x0[j], y0[j], 0, tx, 0);
               get_texel(tgsi_sampler, faces[j], level1, x1[j], y0[j], 0, tx, 1);
               get_texel(tgsi_sampler, faces[j], level1, x0[j], y1[j], 0, tx, 2);
               get_texel(tgsi_sampler, faces[j], level1, x1[j], y1[j], 0, tx, 3);
               if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE){
                  shadow_compare4(sampler, tx, p);
               }

               /* interpolate R, G, B, A */
               for (c = 0; c < 4; c++) {
                  rgba2[c][j] = lerp_2d(xw[j], yw[j],
                                        tx[c][0], tx[c][1], tx[c][2], tx[c][3]);
               }

               for (c = 0; c < NUM_CHANNELS; c++) {
                  rgba[c][j] = lerp(levelBlend, rgba[c][j], rgba2[c][j]);
               }
            }
         }
      }
      break;
   default:
      assert(0);
   }
}


static INLINE void
lp_get_samples_1d(struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   static const unsigned faces[4] = {0, 0, 0, 0};
   static const float tzero[4] = {0, 0, 0, 0};
   lp_get_samples_2d_common(sampler, s, tzero, NULL,
                            lodbias, rgba, faces);
}


static INLINE void
lp_get_samples_2d(struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   static const unsigned faces[4] = {0, 0, 0, 0};
   lp_get_samples_2d_common(sampler, s, t, p,
                            lodbias, rgba, faces);
}


static INLINE void
lp_get_samples_3d(struct tgsi_sampler *tgsi_sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;
   /* get/map pipe_surfaces corresponding to 3D tex slices */
   unsigned level0, level1, j, imgFilter;
   int width, height, depth;
   float levelBlend;
   const uint face = 0;

   choose_mipmap_levels(tgsi_sampler, s, t, p, 
                        lodbias,
                        &level0, &level1, &levelBlend, &imgFilter);

   assert(sampler->normalized_coords);

   width = texture->width[level0];
   height = texture->height[level0];
   depth = texture->depth[level0];

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   switch (imgFilter) {
   case PIPE_TEX_FILTER_NEAREST:
      {
         int x[4], y[4], z[4];
         nearest_texcoord_4(sampler->wrap_s, s, width, x);
         nearest_texcoord_4(sampler->wrap_t, t, height, y);
         nearest_texcoord_4(sampler->wrap_r, p, depth, z);
         for (j = 0; j < QUAD_SIZE; j++) {
            get_texel(tgsi_sampler, face, level0, x[j], y[j], z[j], rgba, j);
            if (level0 != level1) {
               /* get texels from second mipmap level and blend */
               float rgba2[4][4];
               unsigned c;
               x[j] /= 2;
               y[j] /= 2;
               z[j] /= 2;
               get_texel(tgsi_sampler, face, level1, x[j], y[j], z[j], rgba2, j);
               for (c = 0; c < NUM_CHANNELS; c++) {
                  rgba[c][j] = lerp(levelBlend, rgba2[c][j], rgba[c][j]);
               }
            }
         }
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      {
         int x0[4], x1[4], y0[4], y1[4], z0[4], z1[4];
         float xw[4], yw[4], zw[4]; /* interpolation weights */
         linear_texcoord_4(sampler->wrap_s, s, width,  x0, x1, xw);
         linear_texcoord_4(sampler->wrap_t, t, height, y0, y1, yw);
         linear_texcoord_4(sampler->wrap_r, p, depth,  z0, z1, zw);

         for (j = 0; j < QUAD_SIZE; j++) {
            int c;
            float tx0[4][4], tx1[4][4];
            get_texel(tgsi_sampler, face, level0, x0[j], y0[j], z0[j], tx0, 0);
            get_texel(tgsi_sampler, face, level0, x1[j], y0[j], z0[j], tx0, 1);
            get_texel(tgsi_sampler, face, level0, x0[j], y1[j], z0[j], tx0, 2);
            get_texel(tgsi_sampler, face, level0, x1[j], y1[j], z0[j], tx0, 3);
            get_texel(tgsi_sampler, face, level0, x0[j], y0[j], z1[j], tx1, 0);
            get_texel(tgsi_sampler, face, level0, x1[j], y0[j], z1[j], tx1, 1);
            get_texel(tgsi_sampler, face, level0, x0[j], y1[j], z1[j], tx1, 2);
            get_texel(tgsi_sampler, face, level0, x1[j], y1[j], z1[j], tx1, 3);

            /* interpolate R, G, B, A */
            for (c = 0; c < 4; c++) {
               rgba[c][j] = lerp_3d(xw[j], yw[j], zw[j],
                                    tx0[c][0], tx0[c][1],
                                    tx0[c][2], tx0[c][3],
                                    tx1[c][0], tx1[c][1],
                                    tx1[c][2], tx1[c][3]);
            }

            if (level0 != level1) {
               /* get texels from second mipmap level and blend */
               float rgba2[4][4];
               x0[j] /= 2;
               y0[j] /= 2;
               z0[j] /= 2;
               x1[j] /= 2;
               y1[j] /= 2;
               z1[j] /= 2;
               get_texel(tgsi_sampler, face, level1, x0[j], y0[j], z0[j], tx0, 0);
               get_texel(tgsi_sampler, face, level1, x1[j], y0[j], z0[j], tx0, 1);
               get_texel(tgsi_sampler, face, level1, x0[j], y1[j], z0[j], tx0, 2);
               get_texel(tgsi_sampler, face, level1, x1[j], y1[j], z0[j], tx0, 3);
               get_texel(tgsi_sampler, face, level1, x0[j], y0[j], z1[j], tx1, 0);
               get_texel(tgsi_sampler, face, level1, x1[j], y0[j], z1[j], tx1, 1);
               get_texel(tgsi_sampler, face, level1, x0[j], y1[j], z1[j], tx1, 2);
               get_texel(tgsi_sampler, face, level1, x1[j], y1[j], z1[j], tx1, 3);

               /* interpolate R, G, B, A */
               for (c = 0; c < 4; c++) {
                  rgba2[c][j] = lerp_3d(xw[j], yw[j], zw[j],
                                        tx0[c][0], tx0[c][1],
                                        tx0[c][2], tx0[c][3],
                                        tx1[c][0], tx1[c][1],
                                        tx1[c][2], tx1[c][3]);
               }

               /* blend mipmap levels */
               for (c = 0; c < NUM_CHANNELS; c++) {
                  rgba[c][j] = lerp(levelBlend, rgba[c][j], rgba2[c][j]);
               }
            }
         }
      }
      break;
   default:
      assert(0);
   }
}


static void
lp_get_samples_cube(struct tgsi_sampler *sampler,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE],
                    float lodbias,
                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   unsigned faces[QUAD_SIZE], j;
   float ssss[4], tttt[4];
   for (j = 0; j < QUAD_SIZE; j++) {
      faces[j] = choose_cube_face(s[j], t[j], p[j], ssss + j, tttt + j);
   }
   lp_get_samples_2d_common(sampler, ssss, tttt, NULL,
                            lodbias, rgba, faces);
}


static void
lp_get_samples_rect(struct tgsi_sampler *tgsi_sampler,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE],
                    float lodbias,
                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;
   const uint face = 0;
   unsigned level0, level1, j, imgFilter;
   int width, height;
   float levelBlend;

   choose_mipmap_levels(tgsi_sampler, s, t, p, 
                        lodbias,
                        &level0, &level1, &levelBlend, &imgFilter);

   /* texture RECTS cannot be mipmapped */
   assert(level0 == level1);

   width = texture->width[level0];
   height = texture->height[level0];

   assert(width > 0);

   switch (imgFilter) {
   case PIPE_TEX_FILTER_NEAREST:
      {
         int x[4], y[4];
         nearest_texcoord_unnorm_4(sampler->wrap_s, s, width, x);
         nearest_texcoord_unnorm_4(sampler->wrap_t, t, height, y);
         for (j = 0; j < QUAD_SIZE; j++) {
            get_texel(tgsi_sampler, face, level0, x[j], y[j], 0, rgba, j);
            if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
               shadow_compare(sampler, rgba, p, j);
            }
         }
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      {
         int x0[4], y0[4], x1[4], y1[4];
         float xw[4], yw[4]; /* weights */
         linear_texcoord_unnorm_4(sampler->wrap_s, s, width,  x0, x1, xw);
         linear_texcoord_unnorm_4(sampler->wrap_t, t, height, y0, y1, yw);
         for (j = 0; j < QUAD_SIZE; j++) {
            float tx[4][4]; /* texels */
            int c;
            get_texel(tgsi_sampler, face, level0, x0[j], y0[j], 0, tx, 0);
            get_texel(tgsi_sampler, face, level0, x1[j], y0[j], 0, tx, 1);
            get_texel(tgsi_sampler, face, level0, x0[j], y1[j], 0, tx, 2);
            get_texel(tgsi_sampler, face, level0, x1[j], y1[j], 0, tx, 3);
            if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
               shadow_compare4(sampler, tx, p);
            }
            for (c = 0; c < 4; c++) {
               rgba[c][j] = lerp_2d(xw[j], yw[j],
                                    tx[c][0], tx[c][1], tx[c][2], tx[c][3]);
            }
         }
      }
      break;
   default:
      assert(0);
   }
}


/**
 * Error condition handler
 */
static INLINE void
lp_get_samples_null(struct tgsi_sampler *tgsi_sampler,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE],
                    float lodbias,
                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   int i,j;

   for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
         rgba[i][j] = 1.0;
}

/**
 * Called via tgsi_sampler::get_samples() when using a sampler for the
 * first time.  Determine the actual sampler function, link it in and
 * call it.
 */
void
lp_get_samples(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias,
               float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct lp_shader_sampler *samp = lp_shader_sampler(tgsi_sampler);
   const struct pipe_texture *texture = samp->texture;
   const struct pipe_sampler_state *sampler = samp->sampler;

   /* Default to the 'undefined' case:
    */
   tgsi_sampler->get_samples = lp_get_samples_null;

   if (!texture) {
      assert(0);                /* is this legal?? */
      goto out;
   }

   if (!sampler->normalized_coords) {
      assert (texture->target == PIPE_TEXTURE_2D);
      tgsi_sampler->get_samples = lp_get_samples_rect;
      goto out;
   }

   switch (texture->target) {
   case PIPE_TEXTURE_1D:
      tgsi_sampler->get_samples = lp_get_samples_1d;
      break;
   case PIPE_TEXTURE_2D:
      tgsi_sampler->get_samples = lp_get_samples_2d;
      break;
   case PIPE_TEXTURE_3D:
      tgsi_sampler->get_samples = lp_get_samples_3d;
      break;
   case PIPE_TEXTURE_CUBE:
      tgsi_sampler->get_samples = lp_get_samples_cube;
      break;
   default:
      assert(0);
      break;
   }

   /* Do this elsewhere: 
    */
   samp->xpot = util_unsigned_logbase2( samp->texture->width[0] );
   samp->ypot = util_unsigned_logbase2( samp->texture->height[0] );

   /* Try to hook in a faster sampler.  Ultimately we'll have to
    * code-generate these.  Luckily most of this looks like it is
    * orthogonal state within the sampler.
    */
   if (texture->target == PIPE_TEXTURE_2D &&
       sampler->min_img_filter == sampler->mag_img_filter &&
       sampler->wrap_s == sampler->wrap_t &&
       sampler->compare_mode == FALSE &&
       sampler->normalized_coords) 
   {
      if (sampler->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
         samp->level = CLAMP((int) sampler->min_lod,
                             0, (int) texture->last_level);

         if (sampler->wrap_s == PIPE_TEX_WRAP_REPEAT) {
            switch (sampler->min_img_filter) {
            case PIPE_TEX_FILTER_NEAREST:
               tgsi_sampler->get_samples = lp_get_samples_2d_nearest_repeat_POT;
               break;
            case PIPE_TEX_FILTER_LINEAR:
               tgsi_sampler->get_samples = lp_get_samples_2d_linear_repeat_POT;
               break;
            default:
               break;
            }
         } 
         else if (sampler->wrap_s == PIPE_TEX_WRAP_CLAMP) {
            switch (sampler->min_img_filter) {
            case PIPE_TEX_FILTER_NEAREST:
               tgsi_sampler->get_samples = lp_get_samples_2d_nearest_clamp_POT;
               break;
            default:
               break;
            }
         }
      }
      else if (sampler->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
         if (sampler->wrap_s == PIPE_TEX_WRAP_REPEAT) {
            switch (sampler->min_img_filter) {
            case PIPE_TEX_FILTER_LINEAR:
               tgsi_sampler->get_samples = lp_get_samples_2d_linear_mip_linear_repeat_POT;
               break;
            default:
               break;
            }
         } 
      }
   }
   else if (0) {
      _debug_printf("target %d/%d min_mip %d/%d min_img %d/%d wrap %d/%d compare %d/%d norm %d/%d\n",
                    texture->target, PIPE_TEXTURE_2D,
                    sampler->min_mip_filter, PIPE_TEX_MIPFILTER_NONE,
                    sampler->min_img_filter, sampler->mag_img_filter,
                    sampler->wrap_s, sampler->wrap_t,
                    sampler->compare_mode, FALSE,
                    sampler->normalized_coords, TRUE);
   }

out:
   tgsi_sampler->get_samples( tgsi_sampler, s, t, p, lodbias, rgba );
}


void PIPE_CDECL
lp_fetch_texel_soa( struct tgsi_sampler **samplers,
                    uint32_t unit,
                    float *store )
{
   struct tgsi_sampler *sampler = samplers[unit];

#if 0
   uint j;

   debug_printf("%s sampler: %p (%p) store: %p\n",
                __FUNCTION__,
                sampler, *sampler,
                store );

   debug_printf("lodbias %f\n", store[12]);

   for (j = 0; j < 4; j++)
      debug_printf("sample %d texcoord %f %f\n",
                   j,
                   store[0+j],
                   store[4+j]);
#endif

   {
      float rgba[NUM_CHANNELS][QUAD_SIZE];
      sampler->get_samples(sampler,
                           &store[0],
                           &store[4],
                           &store[8],
                           0.0f, /*store[12],  lodbias */
                           rgba);
      memcpy(store, rgba, sizeof rgba);
   }

#if 0
   for (j = 0; j < 4; j++)
      debug_printf("sample %d result %f %f %f %f\n",
                   j,
                   store[0+j],
                   store[4+j],
                   store[8+j],
                   store[12+j]);
#endif
}


#include "lp_bld_type.h"
#include "lp_bld_intr.h"
#include "lp_bld_tgsi.h"


struct lp_c_sampler_soa
{
   struct lp_build_sampler_soa base;

   LLVMValueRef context_ptr;

   LLVMValueRef samplers_ptr;

   /** Coords/texels store */
   LLVMValueRef store_ptr;
};


static void
lp_c_sampler_soa_destroy(struct lp_build_sampler_soa *sampler)
{
   FREE(sampler);
}


static void
lp_c_sampler_soa_emit_fetch_texel(struct lp_build_sampler_soa *_sampler,
                                  LLVMBuilderRef builder,
                                  struct lp_type type,
                                  unsigned unit,
                                  unsigned num_coords,
                                  const LLVMValueRef *coords,
                                  LLVMValueRef lodbias,
                                  LLVMValueRef *texel)
{
   struct lp_c_sampler_soa *sampler = (struct lp_c_sampler_soa *)_sampler;
   LLVMTypeRef vec_type = LLVMTypeOf(coords[0]);
   LLVMValueRef args[3];
   unsigned i;

   if(!sampler->samplers_ptr)
      sampler->samplers_ptr = lp_jit_context_samplers(builder, sampler->context_ptr);

   if(!sampler->store_ptr)
      sampler->store_ptr = LLVMBuildArrayAlloca(builder,
                                            vec_type,
                                            LLVMConstInt(LLVMInt32Type(), 4, 0),
                                            "texel_store");

   for (i = 0; i < num_coords; i++) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef coord_ptr = LLVMBuildGEP(builder, sampler->store_ptr, &index, 1, "");
      LLVMBuildStore(builder, coords[i], coord_ptr);
   }

   args[0] = sampler->samplers_ptr;
   args[1] = LLVMConstInt(LLVMInt32Type(), unit, 0);
   args[2] = sampler->store_ptr;

   lp_build_intrinsic(builder, "fetch_texel", LLVMVoidType(), args, 3);

   for (i = 0; i < NUM_CHANNELS; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef texel_ptr = LLVMBuildGEP(builder, sampler->store_ptr, &index, 1, "");
      texel[i] = LLVMBuildLoad(builder, texel_ptr, "");
   }
}


struct lp_build_sampler_soa *
lp_c_sampler_soa_create(LLVMValueRef context_ptr)
{
   struct lp_c_sampler_soa *sampler;

   sampler = CALLOC_STRUCT(lp_c_sampler_soa);
   if(!sampler)
      return NULL;

   sampler->base.destroy = lp_c_sampler_soa_destroy;
   sampler->base.emit_fetch_texel = lp_c_sampler_soa_emit_fetch_texel;
   sampler->context_ptr = context_ptr;

   return &sampler->base;
}

