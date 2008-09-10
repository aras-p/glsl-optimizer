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
 * Texture sampling
 *
 * Authors:
 *   Brian Paul
 */

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_tex_sample.h"
#include "sp_tile_cache.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "tgsi/tgsi_exec.h"
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
#define LERP(T, A, B)  ( (A) + (T) * ((B) - (A)) )


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
   const float temp0 = LERP(a, v00, v10);
   const float temp1 = LERP(a, v01, v11);
   return LERP(b, temp0, temp1);
}


/**
 * If A is a signed integer, A % B doesn't give the right value for A < 0
 * (in terms of texture repeat).  Just casting to unsigned fixes that.
 */
#define REMAINDER(A, B) ((unsigned) (A) % (unsigned) (B))


/**
 * Apply texture coord wrapping mode and return integer texture index.
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the texcoord
 * \param size  the texture image size
 * \return  integer texture index
 */
static INLINE int
nearest_texcoord(unsigned wrapMode, float s, unsigned size)
{
   int i;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      /* s limited to [0,1) */
      /* i limited to [0,size-1] */
      i = util_ifloor(s * size);
      i = REMAINDER(i, size);
      return i;
   case PIPE_TEX_WRAP_CLAMP:
      /* s limited to [0,1] */
      /* i limited to [0,size-1] */
      if (s <= 0.0F)
         i = 0;
      else if (s >= 1.0F)
         i = size - 1;
      else
         i = util_ifloor(s * size);
      return i;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const float min = 1.0F / (2.0F * size);
         const float max = 1.0F - min;
         if (s < min)
            i = 0;
         else if (s > max)
            i = size - 1;
         else
            i = util_ifloor(s * size);
      }
      return i;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [-1, size] */
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         if (s <= min)
            i = -1;
         else if (s >= max)
            i = size;
         else
            i = util_ifloor(s * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      {
         const float min = 1.0F / (2.0F * size);
         const float max = 1.0F - min;
         const int flr = util_ifloor(s);
         float u;
         if (flr & 1)
            u = 1.0F - (s - (float) flr);
         else
            u = s - (float) flr;
         if (u < min)
            i = 0;
         else if (u > max)
            i = size - 1;
         else
            i = util_ifloor(u * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      {
         /* s limited to [0,1] */
         /* i limited to [0,size-1] */
         const float u = fabsf(s);
         if (u <= 0.0F)
            i = 0;
         else if (u >= 1.0F)
            i = size - 1;
         else
            i = util_ifloor(u * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const float min = 1.0F / (2.0F * size);
         const float max = 1.0F - min;
         const float u = fabsf(s);
         if (u < min)
            i = 0;
         else if (u > max)
            i = size - 1;
         else
            i = util_ifloor(u * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         const float u = fabsf(s);
         if (u < min)
            i = -1;
         else if (u > max)
            i = size;
         else
            i = util_ifloor(u * size);
      }
      return i;
   default:
      assert(0);
      return 0;
   }
}


/**
 * Used to compute texel locations for linear sampling.
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the texcoord
 * \param size  the texture image size
 * \param i0  returns first texture index
 * \param i1  returns second texture index (usually *i0 + 1)
 * \param a  returns blend factor/weight between texture indexes
 */
static INLINE void
linear_texcoord(unsigned wrapMode, float s, unsigned size,
                int *i0, int *i1, float *a)
{
   float u;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      u = s * size - 0.5F;
      *i0 = REMAINDER(util_ifloor(u), size);
      *i1 = REMAINDER(*i0 + 1, size);
      break;
   case PIPE_TEX_WRAP_CLAMP:
      if (s <= 0.0F)
         u = 0.0F;
      else if (s >= 1.0F)
         u = (float) size;
      else
         u = s * size;
      u -= 0.5F;
      *i0 = util_ifloor(u);
      *i1 = *i0 + 1;
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (s <= 0.0F)
         u = 0.0F;
      else if (s >= 1.0F)
         u = (float) size;
      else
         u = s * size;
      u -= 0.5F;
      *i0 = util_ifloor(u);
      *i1 = *i0 + 1;
      if (*i0 < 0)
         *i0 = 0;
      if (*i1 >= (int) size)
         *i1 = size - 1;
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         if (s <= min)
            u = min * size;
         else if (s >= max)
            u = max * size;
         else
            u = s * size;
         u -= 0.5F;
         *i0 = util_ifloor(u);
         *i1 = *i0 + 1;
      }
      break;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      {
         const int flr = util_ifloor(s);
         if (flr & 1)
            u = 1.0F - (s - (float) flr);
         else
            u = s - (float) flr;
         u = (u * size) - 0.5F;
         *i0 = util_ifloor(u);
         *i1 = *i0 + 1;
         if (*i0 < 0)
            *i0 = 0;
         if (*i1 >= (int) size)
            *i1 = size - 1;
      }
      break;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      u = fabsf(s);
      if (u >= 1.0F)
         u = (float) size;
      else
         u *= size;
      u -= 0.5F;
      *i0 = util_ifloor(u);
      *i1 = *i0 + 1;
      break;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      u = fabsf(s);
      if (u >= 1.0F)
         u = (float) size;
      else
         u *= size;
      u -= 0.5F;
      *i0 = util_ifloor(u);
      *i1 = *i0 + 1;
      if (*i0 < 0)
         *i0 = 0;
      if (*i1 >= (int) size)
         *i1 = size - 1;
      break;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         const float min = -1.0F / (2.0F * size);
         const float max = 1.0F - min;
         u = fabsf(s);
         if (u <= min)
            u = min * size;
         else if (u >= max)
            u = max * size;
         else
            u *= size;
         u -= 0.5F;
         *i0 = util_ifloor(u);
         *i1 = *i0 + 1;
      }
      break;
   default:
      assert(0);
   }
   *a = FRAC(u);
}


/**
 * For RECT textures / unnormalized texcoords
 * Only a subset of wrap modes supported.
 */
static INLINE int
nearest_texcoord_unnorm(unsigned wrapMode, float s, unsigned size)
{
   int i;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_CLAMP:
      i = util_ifloor(s);
      return CLAMP(i, 0, (int) size-1);
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      /* fall-through */
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return util_ifloor( CLAMP(s, 0.5F, (float) size - 0.5F) );
   default:
      assert(0);
      return 0;
   }
}


/**
 * For RECT textures / unnormalized texcoords.
 * Only a subset of wrap modes supported.
 */
static INLINE void
linear_texcoord_unnorm(unsigned wrapMode, float s, unsigned size,
                       int *i0, int *i1, float *a)
{
   switch (wrapMode) {
   case PIPE_TEX_WRAP_CLAMP:
      /* Not exactly what the spec says, but it matches NVIDIA output */
      s = CLAMP(s - 0.5F, 0.0f, (float) size - 1.0f);
      *i0 = util_ifloor(s);
      *i1 = *i0 + 1;
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      /* fall-through */
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      s = CLAMP(s, 0.5F, (float) size - 0.5F);
      s -= 0.5F;
      *i0 = util_ifloor(s);
      *i1 = *i0 + 1;
      if (*i1 > (int) size - 1)
         *i1 = size - 1;
      break;
   default:
      assert(0);
   }
   *a = FRAC(s);
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
compute_lambda(struct tgsi_sampler *sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias)
{
   float rho, lambda;

   assert(sampler->state->normalized_coords);

   assert(s);
   {
      float dsdx = s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT];
      float dsdy = s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT];
      dsdx = fabsf(dsdx);
      dsdy = fabsf(dsdy);
      rho = MAX2(dsdx, dsdy) * sampler->texture->width[0];
   }
   if (t) {
      float dtdx = t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT];
      float dtdy = t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT];
      float max;
      dtdx = fabsf(dtdx);
      dtdy = fabsf(dtdy);
      max = MAX2(dtdx, dtdy) * sampler->texture->height[0];
      rho = MAX2(rho, max);
   }
   if (p) {
      float dpdx = p[QUAD_BOTTOM_RIGHT] - p[QUAD_BOTTOM_LEFT];
      float dpdy = p[QUAD_TOP_LEFT]     - p[QUAD_BOTTOM_LEFT];
      float max;
      dpdx = fabsf(dpdx);
      dpdy = fabsf(dpdy);
      max = MAX2(dpdx, dpdy) * sampler->texture->depth[0];
      rho = MAX2(rho, max);
   }

   lambda = util_fast_log2(rho);
   lambda += lodbias + sampler->state->lod_bias;
   lambda = CLAMP(lambda, sampler->state->min_lod, sampler->state->max_lod);

   return lambda;
}


/**
 * Do several things here:
 * 1. Compute lambda from the texcoords, if needed
 * 2. Determine if we're minifying or magnifying
 * 3. If minifying, choose mipmap levels
 * 4. Return image filter to use within mipmap images
 */
static void
choose_mipmap_levels(struct tgsi_sampler *sampler,
                     const float s[QUAD_SIZE],
                     const float t[QUAD_SIZE],
                     const float p[QUAD_SIZE],
                     float lodbias,
                     unsigned *level0, unsigned *level1, float *levelBlend,
                     unsigned *imgFilter)
{
   if (sampler->state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
      /* no mipmap selection needed */
      *level0 = *level1 = CLAMP((int) sampler->state->min_lod,
                                0, (int) sampler->texture->last_level);

      if (sampler->state->min_img_filter != sampler->state->mag_img_filter) {
         /* non-mipmapped texture, but still need to determine if doing
          * minification or magnification.
          */
         float lambda = compute_lambda(sampler, s, t, p, lodbias);
         if (lambda <= 0.0) {
            *imgFilter = sampler->state->mag_img_filter;
         }
         else {
            *imgFilter = sampler->state->min_img_filter;
         }
      }
      else {
         *imgFilter = sampler->state->mag_img_filter;
      }
   }
   else {
      float lambda;

      if (1)
         /* fragment shader */
         lambda = compute_lambda(sampler, s, t, p, lodbias);
      else
         /* vertex shader */
         lambda = lodbias; /* not really a bias, but absolute LOD */

      if (lambda <= 0.0) { /* XXX threshold depends on the filter */
         /* magnifying */
         *imgFilter = sampler->state->mag_img_filter;
         *level0 = *level1 = 0;
      }
      else {
         /* minifying */
         *imgFilter = sampler->state->min_img_filter;

         /* choose mipmap level(s) and compute the blend factor between them */
         if (sampler->state->min_mip_filter == PIPE_TEX_MIPFILTER_NEAREST) {
            /* Nearest mipmap level */
            const int lvl = (int) (lambda + 0.5);
            *level0 =
            *level1 = CLAMP(lvl, 0, (int) sampler->texture->last_level);
         }
         else {
            /* Linear interpolation between mipmap levels */
            const int lvl = (int) lambda;
            *level0 = CLAMP(lvl,     0, (int) sampler->texture->last_level);
            *level1 = CLAMP(lvl + 1, 0, (int) sampler->texture->last_level);
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
get_texel(struct tgsi_sampler *sampler,
          unsigned face, unsigned level, int x, int y, int z,
          float rgba[NUM_CHANNELS][QUAD_SIZE], unsigned j)
{
   if (x < 0 || x >= (int) sampler->texture->width[level] ||
       y < 0 || y >= (int) sampler->texture->height[level] ||
       z < 0 || z >= (int) sampler->texture->depth[level]) {
      rgba[0][j] = sampler->state->border_color[0];
      rgba[1][j] = sampler->state->border_color[1];
      rgba[2][j] = sampler->state->border_color[2];
      rgba[3][j] = sampler->state->border_color[3];
   }
   else {
      const int tx = x % TILE_SIZE;
      const int ty = y % TILE_SIZE;
      const struct softpipe_cached_tile *tile
         = sp_get_cached_tile_tex(sampler->pipe, sampler->cache,
                                  x, y, z, face, level);
      rgba[0][j] = tile->data.color[ty][tx][0];
      rgba[1][j] = tile->data.color[ty][tx][1];
      rgba[2][j] = tile->data.color[ty][tx][2];
      rgba[3][j] = tile->data.color[ty][tx][3];
      if (0)
      {
         debug_printf("Get texel %f %f %f %f from %s\n",
                      rgba[0][j], rgba[1][j], rgba[2][j], rgba[3][j],
                      pf_name(sampler->texture->format));
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

   rgba[0][j] = rgba[1][j] = rgba[2][j] = (float) k;
}


/**
 * Common code for sampling 1D/2D/cube textures.
 * Could probably extend for 3D...
 */
static void
sp_get_samples_2d_common(struct tgsi_sampler *sampler,
                         const float s[QUAD_SIZE],
                         const float t[QUAD_SIZE],
                         const float p[QUAD_SIZE],
                         float lodbias,
                         float rgba[NUM_CHANNELS][QUAD_SIZE],
                         const unsigned faces[4])
{
   const uint compare_func = sampler->state->compare_func;
   unsigned level0, level1, j, imgFilter;
   int width, height;
   float levelBlend;

   choose_mipmap_levels(sampler, s, t, p, lodbias,
                        &level0, &level1, &levelBlend, &imgFilter);

   assert(sampler->state->normalized_coords);

   width = sampler->texture->width[level0];
   height = sampler->texture->height[level0];

   assert(width > 0);

   switch (imgFilter) {
   case PIPE_TEX_FILTER_NEAREST:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = nearest_texcoord(sampler->state->wrap_s, s[j], width);
         int y = nearest_texcoord(sampler->state->wrap_t, t[j], height);
         get_texel(sampler, faces[j], level0, x, y, 0, rgba, j);
         if (sampler->state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
            shadow_compare(compare_func, rgba, p, j);
         }

         if (level0 != level1) {
            /* get texels from second mipmap level and blend */
            float rgba2[4][4];
            unsigned c;
            x = x / 2;
            y = y / 2;
            get_texel(sampler, faces[j], level1, x, y, 0, rgba2, j);
            if (sampler->state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE){
               shadow_compare(compare_func, rgba2, p, j);
            }

            for (c = 0; c < NUM_CHANNELS; c++) {
               rgba[c][j] = LERP(levelBlend, rgba[c][j], rgba2[c][j]);
            }
         }
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      for (j = 0; j < QUAD_SIZE; j++) {
         float tx[4][4], a, b;
         int x0, y0, x1, y1, c;
         linear_texcoord(sampler->state->wrap_s, s[j], width,  &x0, &x1, &a);
         linear_texcoord(sampler->state->wrap_t, t[j], height, &y0, &y1, &b);
         get_texel(sampler, faces[j], level0, x0, y0, 0, tx, 0);
         get_texel(sampler, faces[j], level0, x1, y0, 0, tx, 1);
         get_texel(sampler, faces[j], level0, x0, y1, 0, tx, 2);
         get_texel(sampler, faces[j], level0, x1, y1, 0, tx, 3);
         if (sampler->state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
            shadow_compare(compare_func, tx, p, 0);
            shadow_compare(compare_func, tx, p, 1);
            shadow_compare(compare_func, tx, p, 2);
            shadow_compare(compare_func, tx, p, 3);
         }

         for (c = 0; c < 4; c++) {
            rgba[c][j] = lerp_2d(a, b, tx[c][0], tx[c][1], tx[c][2], tx[c][3]);
         }

         if (level0 != level1) {
            /* get texels from second mipmap level and blend */
            float rgba2[4][4];
            x0 = x0 / 2;
            y0 = y0 / 2;
            x1 = x1 / 2;
            y1 = y1 / 2;
            get_texel(sampler, faces[j], level1, x0, y0, 0, tx, 0);
            get_texel(sampler, faces[j], level1, x1, y0, 0, tx, 1);
            get_texel(sampler, faces[j], level1, x0, y1, 0, tx, 2);
            get_texel(sampler, faces[j], level1, x1, y1, 0, tx, 3);
            if (sampler->state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE){
               shadow_compare(compare_func, tx, p, 0);
               shadow_compare(compare_func, tx, p, 1);
               shadow_compare(compare_func, tx, p, 2);
               shadow_compare(compare_func, tx, p, 3);
            }

            for (c = 0; c < 4; c++) {
               rgba2[c][j] = lerp_2d(a, b,
                                     tx[c][0], tx[c][1], tx[c][2], tx[c][3]);
            }

            for (c = 0; c < NUM_CHANNELS; c++) {
               rgba[c][j] = LERP(levelBlend, rgba[c][j], rgba2[c][j]);
            }
         }
      }
      break;
   default:
      assert(0);
   }
}


static void
sp_get_samples_1d(struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   static const unsigned faces[4] = {0, 0, 0, 0};
   static const float tzero[4] = {0, 0, 0, 0};
   sp_get_samples_2d_common(sampler, s, tzero, NULL, lodbias, rgba, faces);
}


static void
sp_get_samples_2d(struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   static const unsigned faces[4] = {0, 0, 0, 0};
   sp_get_samples_2d_common(sampler, s, t, p, lodbias, rgba, faces);
}


static void
sp_get_samples_3d(struct tgsi_sampler *sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  float lodbias,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   /* get/map pipe_surfaces corresponding to 3D tex slices */
   unsigned level0, level1, j, imgFilter;
   int width, height, depth;
   float levelBlend;
   const uint face = 0;

   choose_mipmap_levels(sampler, s, t, p, lodbias,
                        &level0, &level1, &levelBlend, &imgFilter);

   assert(sampler->state->normalized_coords);

   width = sampler->texture->width[level0];
   height = sampler->texture->height[level0];
   depth = sampler->texture->depth[level0];

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   switch (imgFilter) {
   case PIPE_TEX_FILTER_NEAREST:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = nearest_texcoord(sampler->state->wrap_s, s[j], width);
         int y = nearest_texcoord(sampler->state->wrap_t, t[j], height);
         int z = nearest_texcoord(sampler->state->wrap_r, p[j], depth);
         get_texel(sampler, face, level0, x, y, z, rgba, j);

         if (level0 != level1) {
            /* get texels from second mipmap level and blend */
            float rgba2[4][4];
            unsigned c;
            x /= 2;
            y /= 2;
            z /= 2;
            get_texel(sampler, face, level1, x, y, z, rgba2, j);
            for (c = 0; c < NUM_CHANNELS; c++) {
               rgba[c][j] = LERP(levelBlend, rgba2[c][j], rgba[c][j]);
            }
         }
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      for (j = 0; j < QUAD_SIZE; j++) {
         float texel0[4][4], texel1[4][4];
         float xw, yw, zw; /* interpolation weights */
         int x0, x1, y0, y1, z0, z1, c;
         linear_texcoord(sampler->state->wrap_s, s[j], width,  &x0, &x1, &xw);
         linear_texcoord(sampler->state->wrap_t, t[j], height, &y0, &y1, &yw);
         linear_texcoord(sampler->state->wrap_r, p[j], depth,  &z0, &z1, &zw);
         get_texel(sampler, face, level0, x0, y0, z0, texel0, 0);
         get_texel(sampler, face, level0, x1, y0, z0, texel0, 1);
         get_texel(sampler, face, level0, x0, y1, z0, texel0, 2);
         get_texel(sampler, face, level0, x1, y1, z0, texel0, 3);
         get_texel(sampler, face, level0, x0, y0, z1, texel1, 0);
         get_texel(sampler, face, level0, x1, y0, z1, texel1, 1);
         get_texel(sampler, face, level0, x0, y1, z1, texel1, 2);
         get_texel(sampler, face, level0, x1, y1, z1, texel1, 3);

         /* 3D lerp */
         for (c = 0; c < 4; c++) {
            float ctemp0[4][4], ctemp1[4][4];
            ctemp0[c][j] = lerp_2d(xw, yw,
                                   texel0[c][0], texel0[c][1],
                                   texel0[c][2], texel0[c][3]);
            ctemp1[c][j] = lerp_2d(xw, yw,
                                   texel1[c][0], texel1[c][1],
                                   texel1[c][2], texel1[c][3]);
            rgba[c][j] = LERP(zw, ctemp0[c][j], ctemp1[c][j]);
         }

         if (level0 != level1) {
            /* get texels from second mipmap level and blend */
            float rgba2[4][4];
            x0 /= 2;
            y0 /= 2;
            z0 /= 2;
            x1 /= 2;
            y1 /= 2;
            z1 /= 2;
            get_texel(sampler, face, level1, x0, y0, z0, texel0, 0);
            get_texel(sampler, face, level1, x1, y0, z0, texel0, 1);
            get_texel(sampler, face, level1, x0, y1, z0, texel0, 2);
            get_texel(sampler, face, level1, x1, y1, z0, texel0, 3);
            get_texel(sampler, face, level1, x0, y0, z1, texel1, 0);
            get_texel(sampler, face, level1, x1, y0, z1, texel1, 1);
            get_texel(sampler, face, level1, x0, y1, z1, texel1, 2);
            get_texel(sampler, face, level1, x1, y1, z1, texel1, 3);

            /* 3D lerp */
            for (c = 0; c < 4; c++) {
               float ctemp0[4][4], ctemp1[4][4];
               ctemp0[c][j] = lerp_2d(xw, yw,
                                      texel0[c][0], texel0[c][1],
                                      texel0[c][2], texel0[c][3]);
               ctemp1[c][j] = lerp_2d(xw, yw,
                                      texel1[c][0], texel1[c][1],
                                      texel1[c][2], texel1[c][3]);
               rgba2[c][j] = LERP(zw, ctemp0[c][j], ctemp1[c][j]);
            }

            /* blend mipmap levels */
            for (c = 0; c < NUM_CHANNELS; c++) {
               rgba[c][j] = LERP(levelBlend, rgba[c][j], rgba2[c][j]);
            }
         }
      }
      break;
   default:
      assert(0);
   }
}


static void
sp_get_samples_cube(struct tgsi_sampler *sampler,
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
   sp_get_samples_2d_common(sampler, ssss, tttt, NULL, lodbias, rgba, faces);
}


static void
sp_get_samples_rect(struct tgsi_sampler *sampler,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE],
                    float lodbias,
                    float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   //sp_get_samples_2d_common(sampler, s, t, p, lodbias, rgba, faces);
   static const uint face = 0;
   const uint compare_func = sampler->state->compare_func;
   unsigned level0, level1, j, imgFilter;
   int width, height;
   float levelBlend;

   choose_mipmap_levels(sampler, s, t, p, lodbias,
                        &level0, &level1, &levelBlend, &imgFilter);

   /* texture RECTS cannot be mipmapped */
   assert(level0 == level1);

   width = sampler->texture->width[level0];
   height = sampler->texture->height[level0];

   assert(width > 0);

   switch (imgFilter) {
   case PIPE_TEX_FILTER_NEAREST:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = nearest_texcoord_unnorm(sampler->state->wrap_s, s[j], width);
         int y = nearest_texcoord_unnorm(sampler->state->wrap_t, t[j], height);
         get_texel(sampler, face, level0, x, y, 0, rgba, j);
         if (sampler->state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
            shadow_compare(compare_func, rgba, p, j);
         }
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      for (j = 0; j < QUAD_SIZE; j++) {
         float tx[4][4], a, b;
         int x0, y0, x1, y1, c;
         linear_texcoord_unnorm(sampler->state->wrap_s, s[j], width,  &x0, &x1, &a);
         linear_texcoord_unnorm(sampler->state->wrap_t, t[j], height, &y0, &y1, &b);
         get_texel(sampler, face, level0, x0, y0, 0, tx, 0);
         get_texel(sampler, face, level0, x1, y0, 0, tx, 1);
         get_texel(sampler, face, level0, x0, y1, 0, tx, 2);
         get_texel(sampler, face, level0, x1, y1, 0, tx, 3);
         if (sampler->state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
            shadow_compare(compare_func, tx, p, 0);
            shadow_compare(compare_func, tx, p, 1);
            shadow_compare(compare_func, tx, p, 2);
            shadow_compare(compare_func, tx, p, 3);
         }

         for (c = 0; c < 4; c++) {
            rgba[c][j] = lerp_2d(a, b, tx[c][0], tx[c][1], tx[c][2], tx[c][3]);
         }
      }
      break;
   default:
      assert(0);
   }
}




/**
 * Called via tgsi_sampler::get_samples()
 * Use the sampler's state setting to get a filtered RGBA value
 * from the sampler's texture.
 *
 * XXX we can implement many versions of this function, each
 * tightly coded for a specific combination of sampler state
 * (nearest + repeat), (bilinear mipmap + clamp), etc.
 *
 * The update_samplers() function in st_atom_sampler.c could create
 * a new tgsi_sampler object for each state combo it finds....
 */
void
sp_get_samples(struct tgsi_sampler *sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias,
               float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   if (!sampler->texture)
      return;

   switch (sampler->texture->target) {
   case PIPE_TEXTURE_1D:
      assert(sampler->state->normalized_coords);
      sp_get_samples_1d(sampler, s, t, p, lodbias, rgba);
      break;
   case PIPE_TEXTURE_2D:
      if (sampler->state->normalized_coords)
         sp_get_samples_2d(sampler, s, t, p, lodbias, rgba);
      else
         sp_get_samples_rect(sampler, s, t, p, lodbias, rgba);
      break;
   case PIPE_TEXTURE_3D:
      assert(sampler->state->normalized_coords);
      sp_get_samples_3d(sampler, s, t, p, lodbias, rgba);
      break;
   case PIPE_TEXTURE_CUBE:
      assert(sampler->state->normalized_coords);
      sp_get_samples_cube(sampler, s, t, p, lodbias, rgba);
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

