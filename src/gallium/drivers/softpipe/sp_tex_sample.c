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

#include "sp_context.h"
#include "sp_quad.h"
#include "sp_surface.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"
#include "sp_tile_cache.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
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
compute_lambda(const struct pipe_texture *tex,
               const struct pipe_sampler_state *sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias)
{
   float rho, lambda;

   assert(sampler->normalized_coords);

   assert(s);
   {
      float dsdx = s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT];
      float dsdy = s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT];
      dsdx = fabsf(dsdx);
      dsdy = fabsf(dsdy);
      rho = MAX2(dsdx, dsdy) * tex->width[0];
   }
   if (t) {
      float dtdx = t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT];
      float dtdy = t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT];
      float max;
      dtdx = fabsf(dtdx);
      dtdy = fabsf(dtdy);
      max = MAX2(dtdx, dtdy) * tex->height[0];
      rho = MAX2(rho, max);
   }
   if (p) {
      float dpdx = p[QUAD_BOTTOM_RIGHT] - p[QUAD_BOTTOM_LEFT];
      float dpdy = p[QUAD_TOP_LEFT]     - p[QUAD_BOTTOM_LEFT];
      float max;
      dpdx = fabsf(dpdx);
      dpdy = fabsf(dpdy);
      max = MAX2(dpdx, dpdy) * tex->depth[0];
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
choose_mipmap_levels(const struct pipe_texture *texture,
                     const struct pipe_sampler_state *sampler,
                     const float s[QUAD_SIZE],
                     const float t[QUAD_SIZE],
                     const float p[QUAD_SIZE],
                     boolean computeLambda,
                     float lodbias,
                     unsigned *level0, unsigned *level1, float *levelBlend,
                     unsigned *imgFilter)
{
   if (sampler->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
      /* no mipmap selection needed */
      *level0 = *level1 = CLAMP((int) sampler->min_lod,
                                0, (int) texture->last_level);

      if (sampler->min_img_filter != sampler->mag_img_filter) {
         /* non-mipmapped texture, but still need to determine if doing
          * minification or magnification.
          */
         float lambda = compute_lambda(texture, sampler, s, t, p, lodbias);
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
      float lambda;

      if (computeLambda)
         /* fragment shader */
         lambda = compute_lambda(texture, sampler, s, t, p, lodbias);
      else
         /* vertex shader */
         lambda = lodbias; /* not really a bias, but absolute LOD */

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
 * XXX maybe move this into sp_tile_cache.c and merge with the
 * sp_get_cached_tile_tex() function.  Also, get 4 texels instead of 1...
 */
static void
get_texel(const struct tgsi_sampler *tgsi_sampler,
          unsigned face, unsigned level, int x, int y, int z,
          float rgba[NUM_CHANNELS][QUAD_SIZE], unsigned j)
{
   const struct sp_shader_sampler *samp = sp_shader_sampler(tgsi_sampler);
   struct softpipe_context *sp = samp->sp;
   const uint unit = samp->unit;
   const struct pipe_texture *texture = sp->texture[unit];
   const struct pipe_sampler_state *sampler = sp->sampler[unit];

   if (x < 0 || x >= (int) texture->width[level] ||
       y < 0 || y >= (int) texture->height[level] ||
       z < 0 || z >= (int) texture->depth[level]) {
      rgba[0][j] = sampler->border_color[0];
      rgba[1][j] = sampler->border_color[1];
      rgba[2][j] = sampler->border_color[2];
      rgba[3][j] = sampler->border_color[3];
   }
   else {
      const int tx = x % TILE_SIZE;
      const int ty = y % TILE_SIZE;
      const struct softpipe_cached_tile *tile
         = sp_get_cached_tile_tex(sp, samp->cache,
                                  x, y, z, face, level);
      rgba[0][j] = tile->data.color[ty][tx][0];
      rgba[1][j] = tile->data.color[ty][tx][1];
      rgba[2][j] = tile->data.color[ty][tx][2];
      rgba[3][j] = tile->data.color[ty][tx][3];
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
 */
static INLINE void
shadow_compare(uint compare_func,
               float rgba[NUM_CHANNELS][QUAD_SIZE],
               const float p[QUAD_SIZE],
               uint j)
{
   int k;
   switch (compare_func) {
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
 * Common code for sampling 1D/2D/cube textures.
 * Could probably extend for 3D...
 */
static void
sp_get_samples_2d_common(const struct tgsi_sampler *tgsi_sampler,
                         const float s[QUAD_SIZE],
                         const float t[QUAD_SIZE],
                         const float p[QUAD_SIZE],
                         boolean computeLambda,
                         float lodbias,
                         float rgba[NUM_CHANNELS][QUAD_SIZE],
                         const unsigned faces[4])
{
   const struct sp_shader_sampler *samp = sp_shader_sampler(tgsi_sampler);
   const struct softpipe_context *sp = samp->sp;
   const uint unit = samp->unit;
   const struct pipe_texture *texture = sp->texture[unit];
   const struct pipe_sampler_state *sampler = sp->sampler[unit];
   const uint compare_func = sampler->compare_func;
   unsigned level0, level1, j, imgFilter;
   int width, height;
   float levelBlend;

   choose_mipmap_levels(texture, sampler, s, t, p, computeLambda, lodbias,
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
               shadow_compare(compare_func, rgba, p, j);
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
                  shadow_compare(compare_func, rgba2, p, j);
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
               shadow_compare(compare_func, tx, p, 0);
               shadow_compare(compare_func, tx, p, 1);
               shadow_compare(compare_func, tx, p, 2);
               shadow_compare(compare_func, tx, p, 3);
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
               x0[j] /= 2;
               y0[j] /= 2;
               x1[j] /= 2;
               y1[j] /= 2;
               get_texel(tgsi_sampler, faces[j], level1, x0[j], y0[j], 0, tx, 0);
               get_texel(tgsi_sampler, faces[j], level1, x1[j], y0[j], 0, tx, 1);
               get_texel(tgsi_sampler, faces[j], level1, x0[j], y1[j], 0, tx, 2);
               get_texel(tgsi_sampler, faces[j], level1, x1[j], y1[j], 0, tx, 3);
               if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE){
                  shadow_compare(compare_func, tx, p, 0);
                  shadow_compare(compare_func, tx, p, 1);
                  shadow_compare(compare_func, tx, p, 2);
                  shadow_compare(compare_func, tx, p, 3);
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
sp_get_samples_1d(const struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  boolean computeLambda,
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   static const unsigned faces[4] = {0, 0, 0, 0};
   static const float tzero[4] = {0, 0, 0, 0};
   sp_get_samples_2d_common(sampler, s, tzero, NULL,
                            computeLambda, lodbias, rgba, faces);
}


static INLINE void
sp_get_samples_2d(const struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  boolean computeLambda,
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   static const unsigned faces[4] = {0, 0, 0, 0};
   sp_get_samples_2d_common(sampler, s, t, p,
                            computeLambda, lodbias, rgba, faces);
}


static INLINE void
sp_get_samples_3d(const struct tgsi_sampler *tgsi_sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  boolean computeLambda,
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_shader_sampler *samp = sp_shader_sampler(tgsi_sampler);
   const struct softpipe_context *sp = samp->sp;
   const uint unit = samp->unit;
   const struct pipe_texture *texture = sp->texture[unit];
   const struct pipe_sampler_state *sampler = sp->sampler[unit];
   /* get/map pipe_surfaces corresponding to 3D tex slices */
   unsigned level0, level1, j, imgFilter;
   int width, height, depth;
   float levelBlend;
   const uint face = 0;

   choose_mipmap_levels(texture, sampler, s, t, p, computeLambda, lodbias,
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
sp_get_samples_cube(const struct tgsi_sampler *sampler,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE],
                    boolean computeLambda,
                    float lodbias,
                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   unsigned faces[QUAD_SIZE], j;
   float ssss[4], tttt[4];
   for (j = 0; j < QUAD_SIZE; j++) {
      faces[j] = choose_cube_face(s[j], t[j], p[j], ssss + j, tttt + j);
   }
   sp_get_samples_2d_common(sampler, ssss, tttt, NULL,
                            computeLambda, lodbias, rgba, faces);
}


static void
sp_get_samples_rect(const struct tgsi_sampler *tgsi_sampler,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE],
                    boolean computeLambda,
                    float lodbias,
                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_shader_sampler *samp = sp_shader_sampler(tgsi_sampler);
   const struct softpipe_context *sp = samp->sp;
   const uint unit = samp->unit;
   const struct pipe_texture *texture = sp->texture[unit];
   const struct pipe_sampler_state *sampler = sp->sampler[unit];
   const uint face = 0;
   const uint compare_func = sampler->compare_func;
   unsigned level0, level1, j, imgFilter;
   int width, height;
   float levelBlend;

   choose_mipmap_levels(texture, sampler, s, t, p, computeLambda, lodbias,
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
               shadow_compare(compare_func, rgba, p, j);
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
               shadow_compare(compare_func, tx, p, 0);
               shadow_compare(compare_func, tx, p, 1);
               shadow_compare(compare_func, tx, p, 2);
               shadow_compare(compare_func, tx, p, 3);
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
 * Common code for vertex/fragment program texture sampling.
 */
static INLINE void
sp_get_samples(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               boolean computeLambda,
               float lodbias,
               float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_shader_sampler *samp = sp_shader_sampler(tgsi_sampler);
   const struct softpipe_context *sp = samp->sp;
   const uint unit = samp->unit;
   const struct pipe_texture *texture = sp->texture[unit];
   const struct pipe_sampler_state *sampler = sp->sampler[unit];

   if (!texture)
      return;

   switch (texture->target) {
   case PIPE_TEXTURE_1D:
      assert(sampler->normalized_coords);
      sp_get_samples_1d(tgsi_sampler, s, t, p, computeLambda, lodbias, rgba);
      break;
   case PIPE_TEXTURE_2D:
      if (sampler->normalized_coords)
         sp_get_samples_2d(tgsi_sampler, s, t, p, computeLambda, lodbias, rgba);
      else
         sp_get_samples_rect(tgsi_sampler, s, t, p, computeLambda, lodbias, rgba);
      break;
   case PIPE_TEXTURE_3D:
      assert(sampler->normalized_coords);
      sp_get_samples_3d(tgsi_sampler, s, t, p, computeLambda, lodbias, rgba);
      break;
   case PIPE_TEXTURE_CUBE:
      assert(sampler->normalized_coords);
      sp_get_samples_cube(tgsi_sampler, s, t, p, computeLambda, lodbias, rgba);
      break;
   default:
      assert(0);
   }

#if 0 /* DEBUG */
   {
      int i;
      printf("Sampled at %f, %f, %f:\n", s[0], t[0], p[0]);
      for (i = 0; i < 4; i++) {
         printf("Frag %d: %f %f %f %f\n", i,
                rgba[0][i],
                rgba[1][i],
                rgba[2][i],
                rgba[3][i]);
      }
   }
#endif
}


/**
 * Called via tgsi_sampler::get_samples() when running a fragment shader.
 * Get four filtered RGBA values from the sampler's texture.
 */
void
sp_get_samples_fragment(struct tgsi_sampler *tgsi_sampler,
                        const float s[QUAD_SIZE],
                        const float t[QUAD_SIZE],
                        const float p[QUAD_SIZE],
                        float lodbias,
                        float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   sp_get_samples(tgsi_sampler, s, t, p, TRUE, lodbias, rgba);
}


/**
 * Called via tgsi_sampler::get_samples() when running a vertex shader.
 * Get four filtered RGBA values from the sampler's texture.
 */
void
sp_get_samples_vertex(struct tgsi_sampler *tgsi_sampler,
                      const float s[QUAD_SIZE],
                      const float t[QUAD_SIZE],
                      const float p[QUAD_SIZE],
                      float lodbias,
                      float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   sp_get_samples(tgsi_sampler, s, t, p, FALSE, lodbias, rgba);
}
