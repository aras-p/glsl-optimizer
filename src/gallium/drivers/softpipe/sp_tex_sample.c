/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * All Rights Reserved.
 * Copyright 2008-2010 VMware, Inc.  All rights reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
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
 *   Keith Whitwell
 */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "sp_quad.h"   /* only for #define QUAD_* tokens */
#include "sp_tex_sample.h"
#include "sp_texture.h"
#include "sp_tex_tile_cache.h"


/** Set to one to help debug texture sampling */
#define DEBUG_TEX 0


/*
 * Return fractional part of 'f'.  Used for computing interpolation weights.
 * Need to be careful with negative values.
 * Note, if this function isn't perfect you'll sometimes see 1-pixel bands
 * of improperly weighted linear-filtered textures.
 * The tests/texwrap.c demo is a good test.
 */
static INLINE float
frac(float f)
{
   return f - floorf(f);
}



/**
 * Linear interpolation macro
 */
static INLINE float
lerp(float a, float v0, float v1)
{
   return v0 + a * (v1 - v0);
}


/**
 * Do 2D/bilinear interpolation of float values.
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
 * Compute coord % size for repeat wrap modes.
 * Note that if coord is negative, coord % size doesn't give the right
 * value.  To avoid that problem we add a large multiple of the size
 * (rather than using a conditional).
 */
static INLINE int
repeat(int coord, unsigned size)
{
   return (coord + size * 1024) % size;
}


/**
 * Apply texture coord wrapping mode and return integer texture indexes
 * for a vector of four texcoords (S or T or P).
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the incoming texcoords
 * \param size  the texture image size
 * \param icoord  returns the integer texcoords
 */
static void
wrap_nearest_repeat(float s, unsigned size, int *icoord)
{
   /* s limited to [0,1) */
   /* i limited to [0,size-1] */
   int i = util_ifloor(s * size);
   *icoord = repeat(i, size);
}


static void
wrap_nearest_clamp(float s, unsigned size, int *icoord)
{
   /* s limited to [0,1] */
   /* i limited to [0,size-1] */
   if (s <= 0.0F)
      *icoord = 0;
   else if (s >= 1.0F)
      *icoord = size - 1;
   else
      *icoord = util_ifloor(s * size);
}


static void
wrap_nearest_clamp_to_edge(float s, unsigned size, int *icoord)
{
   /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   if (s < min)
      *icoord = 0;
   else if (s > max)
      *icoord = size - 1;
   else
      *icoord = util_ifloor(s * size);
}


static void
wrap_nearest_clamp_to_border(float s, unsigned size, int *icoord)
{
   /* s limited to [min,max] */
   /* i limited to [-1, size] */
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   if (s <= min)
      *icoord = -1;
   else if (s >= max)
      *icoord = size;
   else
      *icoord = util_ifloor(s * size);
}


static void
wrap_nearest_mirror_repeat(float s, unsigned size, int *icoord)
{
   const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   const int flr = util_ifloor(s);
   float u = frac(s);
   if (flr & 1)
      u = 1.0F - u;
   if (u < min)
      *icoord = 0;
   else if (u > max)
      *icoord = size - 1;
   else
      *icoord = util_ifloor(u * size);
}


static void
wrap_nearest_mirror_clamp(float s, unsigned size, int *icoord)
{
   /* s limited to [0,1] */
   /* i limited to [0,size-1] */
   const float u = fabsf(s);
   if (u <= 0.0F)
      *icoord = 0;
   else if (u >= 1.0F)
      *icoord = size - 1;
   else
      *icoord = util_ifloor(u * size);
}


static void
wrap_nearest_mirror_clamp_to_edge(float s, unsigned size, int *icoord)
{
   /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   const float u = fabsf(s);
   if (u < min)
      *icoord = 0;
   else if (u > max)
      *icoord = size - 1;
   else
      *icoord = util_ifloor(u * size);
}


static void
wrap_nearest_mirror_clamp_to_border(float s, unsigned size, int *icoord)
{
   /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   const float u = fabsf(s);
   if (u < min)
      *icoord = -1;
   else if (u > max)
      *icoord = size;
   else
      *icoord = util_ifloor(u * size);
}


/**
 * Used to compute texel locations for linear sampling
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the texcoord
 * \param size  the texture image size
 * \param icoord0  returns first texture index
 * \param icoord1  returns second texture index (usually icoord0 + 1)
 * \param w  returns blend factor/weight between texture indices
 * \param icoord  returns the computed integer texture coord
 */
static void
wrap_linear_repeat(float s, unsigned size,
                   int *icoord0, int *icoord1, float *w)
{
   float u = s * size - 0.5F;
   *icoord0 = repeat(util_ifloor(u), size);
   *icoord1 = repeat(*icoord0 + 1, size);
   *w = frac(u);
}


static void
wrap_linear_clamp(float s, unsigned size,
                  int *icoord0, int *icoord1, float *w)
{
   float u = CLAMP(s, 0.0F, 1.0F);
   u = u * size - 0.5f;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   *w = frac(u);
}


static void
wrap_linear_clamp_to_edge(float s, unsigned size,
                          int *icoord0, int *icoord1, float *w)
{
   float u = CLAMP(s, 0.0F, 1.0F);
   u = u * size - 0.5f;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord0 < 0)
      *icoord0 = 0;
   if (*icoord1 >= (int) size)
      *icoord1 = size - 1;
   *w = frac(u);
}


static void
wrap_linear_clamp_to_border(float s, unsigned size,
                            int *icoord0, int *icoord1, float *w)
{
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   float u = CLAMP(s, min, max);
   u = u * size - 0.5f;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   *w = frac(u);
}


static void
wrap_linear_mirror_repeat(float s, unsigned size,
                          int *icoord0, int *icoord1, float *w)
{
   const int flr = util_ifloor(s);
   float u = frac(s);
   if (flr & 1)
      u = 1.0F - u;
   u = u * size - 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord0 < 0)
      *icoord0 = 0;
   if (*icoord1 >= (int) size)
      *icoord1 = size - 1;
   *w = frac(u);
}


static void
wrap_linear_mirror_clamp(float s, unsigned size,
                         int *icoord0, int *icoord1, float *w)
{
   float u = fabsf(s);
   if (u >= 1.0F)
      u = (float) size;
   else
      u *= size;
   u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   *w = frac(u);
}


static void
wrap_linear_mirror_clamp_to_edge(float s, unsigned size,
                                 int *icoord0, int *icoord1, float *w)
{
   float u = fabsf(s);
   if (u >= 1.0F)
      u = (float) size;
   else
      u *= size;
   u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord0 < 0)
      *icoord0 = 0;
   if (*icoord1 >= (int) size)
      *icoord1 = size - 1;
   *w = frac(u);
}


static void
wrap_linear_mirror_clamp_to_border(float s, unsigned size,
                                   int *icoord0, int *icoord1, float *w)
{
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   float u = fabsf(s);
   if (u <= min)
      u = min * size;
   else if (u >= max)
      u = max * size;
   else
      u *= size;
   u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   *w = frac(u);
}


/**
 * PIPE_TEX_WRAP_CLAMP for nearest sampling, unnormalized coords.
 */
static void
wrap_nearest_unorm_clamp(float s, unsigned size, int *icoord)
{
   int i = util_ifloor(s);
   *icoord = CLAMP(i, 0, (int) size-1);
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_BORDER for nearest sampling, unnormalized coords.
 */
static void
wrap_nearest_unorm_clamp_to_border(float s, unsigned size, int *icoord)
{
   *icoord = util_ifloor( CLAMP(s, -0.5F, (float) size + 0.5F) );
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_EDGE for nearest sampling, unnormalized coords.
 */
static void
wrap_nearest_unorm_clamp_to_edge(float s, unsigned size, int *icoord)
{
   *icoord = util_ifloor( CLAMP(s, 0.5F, (float) size - 0.5F) );
}


/**
 * PIPE_TEX_WRAP_CLAMP for linear sampling, unnormalized coords.
 */
static void
wrap_linear_unorm_clamp(float s, unsigned size,
                        int *icoord0, int *icoord1, float *w)
{
   /* Not exactly what the spec says, but it matches NVIDIA output */
   float u = CLAMP(s - 0.5F, 0.0f, (float) size - 1.0f);
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   *w = frac(u);
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_BORDER for linear sampling, unnormalized coords.
 */
static void
wrap_linear_unorm_clamp_to_border(float s, unsigned size,
                                  int *icoord0, int *icoord1, float *w)
{
   float u = CLAMP(s, -0.5F, (float) size + 0.5F);
   u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord1 > (int) size - 1)
      *icoord1 = size - 1;
   *w = frac(u);
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_EDGE for linear sampling, unnormalized coords.
 */
static void
wrap_linear_unorm_clamp_to_edge(float s, unsigned size,
                                int *icoord0, int *icoord1, float *w)
{
   float u = CLAMP(s, +0.5F, (float) size - 0.5F);
   u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord1 > (int) size - 1)
      *icoord1 = size - 1;
   *w = frac(u);
}


/**
 * Do coordinate to array index conversion.  For array textures.
 */
static INLINE void
wrap_array_layer(float coord, unsigned size, int *layer)
{
   int c = util_ifloor(coord + 0.5F);
   *layer = CLAMP(c, 0, (int) size - 1);
}


/**
 * Examine the quad's texture coordinates to compute the partial
 * derivatives w.r.t X and Y, then compute lambda (level of detail).
 */
static float
compute_lambda_1d(const struct sp_sampler_view *sview,
                  const float s[TGSI_QUAD_SIZE],
                  const float t[TGSI_QUAD_SIZE],
                  const float p[TGSI_QUAD_SIZE])
{
   const struct pipe_resource *texture = sview->base.texture;
   float dsdx = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   float dsdy = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   float rho = MAX2(dsdx, dsdy) * u_minify(texture->width0, sview->base.u.tex.first_level);

   return util_fast_log2(rho);
}


static float
compute_lambda_2d(const struct sp_sampler_view *sview,
                  const float s[TGSI_QUAD_SIZE],
                  const float t[TGSI_QUAD_SIZE],
                  const float p[TGSI_QUAD_SIZE])
{
   const struct pipe_resource *texture = sview->base.texture;
   float dsdx = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   float dsdy = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   float dtdx = fabsf(t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]);
   float dtdy = fabsf(t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]);
   float maxx = MAX2(dsdx, dsdy) * u_minify(texture->width0, sview->base.u.tex.first_level);
   float maxy = MAX2(dtdx, dtdy) * u_minify(texture->height0, sview->base.u.tex.first_level);
   float rho  = MAX2(maxx, maxy);

   return util_fast_log2(rho);
}


static float
compute_lambda_3d(const struct sp_sampler_view *sview,
                  const float s[TGSI_QUAD_SIZE],
                  const float t[TGSI_QUAD_SIZE],
                  const float p[TGSI_QUAD_SIZE])
{
   const struct pipe_resource *texture = sview->base.texture;
   float dsdx = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   float dsdy = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   float dtdx = fabsf(t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]);
   float dtdy = fabsf(t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]);
   float dpdx = fabsf(p[QUAD_BOTTOM_RIGHT] - p[QUAD_BOTTOM_LEFT]);
   float dpdy = fabsf(p[QUAD_TOP_LEFT]     - p[QUAD_BOTTOM_LEFT]);
   float maxx = MAX2(dsdx, dsdy) * u_minify(texture->width0, sview->base.u.tex.first_level);
   float maxy = MAX2(dtdx, dtdy) * u_minify(texture->height0, sview->base.u.tex.first_level);
   float maxz = MAX2(dpdx, dpdy) * u_minify(texture->depth0, sview->base.u.tex.first_level);
   float rho;

   rho = MAX2(maxx, maxy);
   rho = MAX2(rho, maxz);

   return util_fast_log2(rho);
}


/**
 * Compute lambda for a vertex texture sampler.
 * Since there aren't derivatives to use, just return 0.
 */
static float
compute_lambda_vert(const struct sp_sampler_view *sview,
                    const float s[TGSI_QUAD_SIZE],
                    const float t[TGSI_QUAD_SIZE],
                    const float p[TGSI_QUAD_SIZE])
{
   return 0.0f;
}



/**
 * Get a texel from a texture, using the texture tile cache.
 *
 * \param addr  the template tex address containing cube, z, face info.
 * \param x  the x coord of texel within 2D image
 * \param y  the y coord of texel within 2D image
 * \param rgba  the quad to put the texel/color into
 *
 * XXX maybe move this into sp_tex_tile_cache.c and merge with the
 * sp_get_cached_tile_tex() function.
 */




static INLINE const float *
get_texel_2d_no_border(const struct sp_sampler_view *sp_sview,
                       union tex_tile_address addr, int x, int y)
{
   const struct softpipe_tex_cached_tile *tile;
   addr.bits.x = x / TEX_TILE_SIZE;
   addr.bits.y = y / TEX_TILE_SIZE;
   y %= TEX_TILE_SIZE;
   x %= TEX_TILE_SIZE;

   tile = sp_get_cached_tile_tex(sp_sview->cache, addr);

   return &tile->data.color[y][x][0];
}


static INLINE const float *
get_texel_2d(const struct sp_sampler_view *sp_sview,
             const struct sp_sampler *sp_samp,
             union tex_tile_address addr, int x, int y)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   unsigned level = addr.bits.level;

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level)) {
      return sp_samp->base.border_color.f;
   }
   else {
      return get_texel_2d_no_border( sp_sview, addr, x, y );
   }
}


/*
 * Here's the complete logic (HOLY CRAP) for finding next face and doing the
 * corresponding coord wrapping, implemented by get_next_face,
 * get_next_xcoord, get_next_ycoord.
 * Read like that (first line):
 * If face is +x and s coord is below zero, then
 * new face is +z, new s is max , new t is old t
 * (max is always cube size - 1).
 *
 * +x s- -> +z: s = max,   t = t
 * +x s+ -> -z: s = 0,     t = t
 * +x t- -> +y: s = max,   t = max-s
 * +x t+ -> -y: s = max,   t = s
 *
 * -x s- -> -z: s = max,   t = t
 * -x s+ -> +z: s = 0,     t = t
 * -x t- -> +y: s = 0,     t = s
 * -x t+ -> -y: s = 0,     t = max-s
 *
 * +y s- -> -x: s = t,     t = 0
 * +y s+ -> +x: s = max-t, t = 0
 * +y t- -> -z: s = max-s, t = 0
 * +y t+ -> +z: s = s,     t = 0
 *
 * -y s- -> -x: s = max-t, t = max
 * -y s+ -> +x: s = t,     t = max
 * -y t- -> +z: s = s,     t = max
 * -y t+ -> -z: s = max-s, t = max

 * +z s- -> -x: s = max,   t = t
 * +z s+ -> +x: s = 0,     t = t
 * +z t- -> +y: s = s,     t = max
 * +z t+ -> -y: s = s,     t = 0

 * -z s- -> +x: s = max,   t = t
 * -z s+ -> -x: s = 0,     t = t
 * -z t- -> +y: s = max-s, t = 0
 * -z t+ -> -y: s = max-s, t = max
 */


/*
 * seamless cubemap neighbour array.
 * this array is used to find the adjacent face in each of 4 directions,
 * left, right, up, down. (or -x, +x, -y, +y).
 */
static const unsigned face_array[PIPE_TEX_FACE_MAX][4] = {
   /* pos X first then neg X is Z different, Y the same */
   /* PIPE_TEX_FACE_POS_X,*/
   { PIPE_TEX_FACE_POS_Z, PIPE_TEX_FACE_NEG_Z,
     PIPE_TEX_FACE_POS_Y, PIPE_TEX_FACE_NEG_Y },
   /* PIPE_TEX_FACE_NEG_X */
   { PIPE_TEX_FACE_NEG_Z, PIPE_TEX_FACE_POS_Z,
     PIPE_TEX_FACE_POS_Y, PIPE_TEX_FACE_NEG_Y },

   /* pos Y first then neg Y is X different, X the same */
   /* PIPE_TEX_FACE_POS_Y */
   { PIPE_TEX_FACE_NEG_X, PIPE_TEX_FACE_POS_X,
     PIPE_TEX_FACE_NEG_Z, PIPE_TEX_FACE_POS_Z },

   /* PIPE_TEX_FACE_NEG_Y */
   { PIPE_TEX_FACE_NEG_X, PIPE_TEX_FACE_POS_X,
     PIPE_TEX_FACE_POS_Z, PIPE_TEX_FACE_NEG_Z },

   /* pos Z first then neg Y is X different, X the same */
   /* PIPE_TEX_FACE_POS_Z */
   { PIPE_TEX_FACE_NEG_X, PIPE_TEX_FACE_POS_X,
     PIPE_TEX_FACE_POS_Y, PIPE_TEX_FACE_NEG_Y },

   /* PIPE_TEX_FACE_NEG_Z */
   { PIPE_TEX_FACE_POS_X, PIPE_TEX_FACE_NEG_X,
     PIPE_TEX_FACE_POS_Y, PIPE_TEX_FACE_NEG_Y }
};

static INLINE unsigned
get_next_face(unsigned face, int idx)
{
   return face_array[face][idx];
}

/*
 * return a new xcoord based on old face, old coords, cube size
 * and fall_off_index (0 for x-, 1 for x+, 2 for y-, 3 for y+)
 */
static INLINE int
get_next_xcoord(unsigned face, unsigned fall_off_index, int max, int xc, int yc)
{
   if ((face == 0 && fall_off_index != 1) ||
       (face == 1 && fall_off_index == 0) ||
       (face == 4 && fall_off_index == 0) ||
       (face == 5 && fall_off_index == 0)) {
      return max;
   }
   if ((face == 1 && fall_off_index != 0) ||
       (face == 0 && fall_off_index == 1) ||
       (face == 4 && fall_off_index == 1) ||
       (face == 5 && fall_off_index == 1)) {
      return 0;
   }
   if ((face == 4 && fall_off_index >= 2) ||
       (face == 2 && fall_off_index == 3) ||
       (face == 3 && fall_off_index == 2)) {
      return xc;
   }
   if ((face == 5 && fall_off_index >= 2) ||
       (face == 2 && fall_off_index == 2) ||
       (face == 3 && fall_off_index == 3)) {
      return max - xc;
   }
   if ((face == 2 && fall_off_index == 0) ||
       (face == 3 && fall_off_index == 1)) {
      return yc;
   }
   /* (face == 2 && fall_off_index == 1) ||
      (face == 3 && fall_off_index == 0)) */
   return max - yc;
}

/*
 * return a new ycoord based on old face, old coords, cube size
 * and fall_off_index (0 for x-, 1 for x+, 2 for y-, 3 for y+)
 */
static INLINE int
get_next_ycoord(unsigned face, unsigned fall_off_index, int max, int xc, int yc)
{
   if ((fall_off_index <= 1) && (face <= 1 || face >= 4)) {
      return yc;
   }
   if (face == 2 ||
       (face == 4 && fall_off_index == 3) ||
       (face == 5 && fall_off_index == 2)) {
      return 0;
   }
   if (face == 3 ||
       (face == 4 && fall_off_index == 2) ||
       (face == 5 && fall_off_index == 3)) {
      return max;
   }
   if ((face == 0 && fall_off_index == 3) ||
       (face == 1 && fall_off_index == 2)) {
      return xc;
   }
   /* (face == 0 && fall_off_index == 2) ||
      (face == 1 && fall_off_index == 3) */
   return max - xc;
}


static INLINE const float *
get_texel_cube_seamless(const struct sp_sampler_view *sp_sview,
                        union tex_tile_address addr, int x, int y,
                        float *corner)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   unsigned level = addr.bits.level;
   unsigned face = addr.bits.face;
   int new_x, new_y, max_x;

   max_x = (int) u_minify(texture->width0, level);

   assert(texture->width0 == texture->height0);
   new_x = x;
   new_y = y;

   /* change the face */
   if (x < 0) {
      /*
       * Cheat with corners. They are difficult and I believe because we don't get
       * per-pixel faces we can actually have multiple corner texels per pixel,
       * which screws things up majorly in any case (as the per spec behavior is
       * to average the 3 remaining texels, which we might not have).
       * Hence just make sure that the 2nd coord is clamped, will simply pick the
       * sample which would have fallen off the x coord, but not y coord.
       * So the filter weight of the samples will be wrong, but at least this
       * ensures that only valid texels near the corner are used.
       */
      if (y < 0 || y >= max_x) {
         y = CLAMP(y, 0, max_x - 1);
      }
      new_x = get_next_xcoord(face, 0, max_x -1, x, y);
      new_y = get_next_ycoord(face, 0, max_x -1, x, y);
      face = get_next_face(face, 0);
   } else if (x >= max_x) {
      if (y < 0 || y >= max_x) {
         y = CLAMP(y, 0, max_x - 1);
      }
      new_x = get_next_xcoord(face, 1, max_x -1, x, y);
      new_y = get_next_ycoord(face, 1, max_x -1, x, y);
      face = get_next_face(face, 1);
   } else if (y < 0) {
      new_x = get_next_xcoord(face, 2, max_x -1, x, y);
      new_y = get_next_ycoord(face, 2, max_x -1, x, y);
      face = get_next_face(face, 2);
   } else if (y >= max_x) {
      new_x = get_next_xcoord(face, 3, max_x -1, x, y);
      new_y = get_next_ycoord(face, 3, max_x -1, x, y);
      face = get_next_face(face, 3);
   }

   addr.bits.face = face;
   return get_texel_2d_no_border( sp_sview, addr, new_x, new_y );
}

/* Gather a quad of adjacent texels within a tile:
 */
static INLINE void
get_texel_quad_2d_no_border_single_tile(const struct sp_sampler_view *sp_sview,
                                        union tex_tile_address addr,
                                        unsigned x, unsigned y,
                                        const float *out[4])
{
    const struct softpipe_tex_cached_tile *tile;

   addr.bits.x = x / TEX_TILE_SIZE;
   addr.bits.y = y / TEX_TILE_SIZE;
   y %= TEX_TILE_SIZE;
   x %= TEX_TILE_SIZE;

   tile = sp_get_cached_tile_tex(sp_sview->cache, addr);
      
   out[0] = &tile->data.color[y  ][x  ][0];
   out[1] = &tile->data.color[y  ][x+1][0];
   out[2] = &tile->data.color[y+1][x  ][0];
   out[3] = &tile->data.color[y+1][x+1][0];
}


/* Gather a quad of potentially non-adjacent texels:
 */
static INLINE void
get_texel_quad_2d_no_border(const struct sp_sampler_view *sp_sview,
                            union tex_tile_address addr,
                            int x0, int y0,
                            int x1, int y1,
                            const float *out[4])
{
   out[0] = get_texel_2d_no_border( sp_sview, addr, x0, y0 );
   out[1] = get_texel_2d_no_border( sp_sview, addr, x1, y0 );
   out[2] = get_texel_2d_no_border( sp_sview, addr, x0, y1 );
   out[3] = get_texel_2d_no_border( sp_sview, addr, x1, y1 );
}

/* Can involve a lot of unnecessary checks for border color:
 */
static INLINE void
get_texel_quad_2d(const struct sp_sampler_view *sp_sview,
                  const struct sp_sampler *sp_samp,
                  union tex_tile_address addr,
                  int x0, int y0,
                  int x1, int y1,
                  const float *out[4])
{
   out[0] = get_texel_2d( sp_sview, sp_samp, addr, x0, y0 );
   out[1] = get_texel_2d( sp_sview, sp_samp, addr, x1, y0 );
   out[3] = get_texel_2d( sp_sview, sp_samp, addr, x1, y1 );
   out[2] = get_texel_2d( sp_sview, sp_samp, addr, x0, y1 );
}



/* 3d variants:
 */
static INLINE const float *
get_texel_3d_no_border(const struct sp_sampler_view *sp_sview,
                       union tex_tile_address addr, int x, int y, int z)
{
   const struct softpipe_tex_cached_tile *tile;

   addr.bits.x = x / TEX_TILE_SIZE;
   addr.bits.y = y / TEX_TILE_SIZE;
   addr.bits.z = z;
   y %= TEX_TILE_SIZE;
   x %= TEX_TILE_SIZE;

   tile = sp_get_cached_tile_tex(sp_sview->cache, addr);

   return &tile->data.color[y][x][0];
}


static INLINE const float *
get_texel_3d(const struct sp_sampler_view *sp_sview,
             const struct sp_sampler *sp_samp,
             union tex_tile_address addr, int x, int y, int z)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   unsigned level = addr.bits.level;

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level) ||
       z < 0 || z >= (int) u_minify(texture->depth0, level)) {
      return sp_samp->base.border_color.f;
   }
   else {
      return get_texel_3d_no_border( sp_sview, addr, x, y, z );
   }
}


/* Get texel pointer for 1D array texture */
static INLINE const float *
get_texel_1d_array(const struct sp_sampler_view *sp_sview,
                   const struct sp_sampler *sp_samp,
                   union tex_tile_address addr, int x, int y)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   unsigned level = addr.bits.level;

   if (x < 0 || x >= (int) u_minify(texture->width0, level)) {
      return sp_samp->base.border_color.f;
   }
   else {
      return get_texel_2d_no_border(sp_sview, addr, x, y);
   }
}


/* Get texel pointer for 2D array texture */
static INLINE const float *
get_texel_2d_array(const struct sp_sampler_view *sp_sview,
                   const struct sp_sampler *sp_samp,
                   union tex_tile_address addr, int x, int y, int layer)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   unsigned level = addr.bits.level;

   assert(layer < (int) texture->array_size);
   assert(layer >= 0);

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level)) {
      return sp_samp->base.border_color.f;
   }
   else {
      return get_texel_3d_no_border(sp_sview, addr, x, y, layer);
   }
}


/* Get texel pointer for cube array texture */
static INLINE const float *
get_texel_cube_array(const struct sp_sampler_view *sp_sview,
                     const struct sp_sampler *sp_samp,
                     union tex_tile_address addr, int x, int y, int layer)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   unsigned level = addr.bits.level;

   assert(layer < (int) texture->array_size);
   assert(layer >= 0);

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level)) {
      return sp_samp->base.border_color.f;
   }
   else {
      return get_texel_3d_no_border(sp_sview, addr, x, y, layer);
   }
}
/**
 * Given the logbase2 of a mipmap's base level size and a mipmap level,
 * return the size (in texels) of that mipmap level.
 * For example, if level[0].width = 256 then base_pot will be 8.
 * If level = 2, then we'll return 64 (the width at level=2).
 * Return 1 if level > base_pot.
 */
static INLINE unsigned
pot_level_size(unsigned base_pot, unsigned level)
{
   return (base_pot >= level) ? (1 << (base_pot - level)) : 1;
}


static void
print_sample(const char *function, const float *rgba)
{
   debug_printf("%s %g %g %g %g\n",
                function,
                rgba[0], rgba[TGSI_NUM_CHANNELS], rgba[2*TGSI_NUM_CHANNELS], rgba[3*TGSI_NUM_CHANNELS]);
}


static void
print_sample_4(const char *function, float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   debug_printf("%s %g %g %g %g, %g %g %g %g, %g %g %g %g, %g %g %g %g\n",
                function,
                rgba[0][0], rgba[1][0], rgba[2][0], rgba[3][0],
                rgba[0][1], rgba[1][1], rgba[2][1], rgba[3][1],
                rgba[0][2], rgba[1][2], rgba[2][2], rgba[3][2],
                rgba[0][3], rgba[1][3], rgba[2][3], rgba[3][3]);
}


/* Some image-filter fastpaths:
 */
static INLINE void
img_filter_2d_linear_repeat_POT(struct sp_sampler_view *sp_sview,
                                struct sp_sampler *sp_samp,
                                float s,
                                float t,
                                float p,
                                unsigned level,
                                unsigned face_id,
                                float *rgba)
{
   unsigned xpot = pot_level_size(sp_sview->xpot, level);
   unsigned ypot = pot_level_size(sp_sview->ypot, level);
   int xmax = (xpot - 1) & (TEX_TILE_SIZE - 1); /* MIN2(TEX_TILE_SIZE, xpot) - 1; */
   int ymax = (ypot - 1) & (TEX_TILE_SIZE - 1); /* MIN2(TEX_TILE_SIZE, ypot) - 1; */
   union tex_tile_address addr;
   int c;

   float u = s * xpot - 0.5F;
   float v = t * ypot - 0.5F;

   int uflr = util_ifloor(u);
   int vflr = util_ifloor(v);

   float xw = u - (float)uflr;
   float yw = v - (float)vflr;

   int x0 = uflr & (xpot - 1);
   int y0 = vflr & (ypot - 1);

   const float *tx[4];
      
   addr.value = 0;
   addr.bits.level = level;

   /* Can we fetch all four at once:
    */
   if (x0 < xmax && y0 < ymax) {
      get_texel_quad_2d_no_border_single_tile(sp_sview, addr, x0, y0, tx);
   }
   else {
      unsigned x1 = (x0 + 1) & (xpot - 1);
      unsigned y1 = (y0 + 1) & (ypot - 1);
      get_texel_quad_2d_no_border(sp_sview, addr, x0, y0, x1, y1, tx);
   }

   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++) {
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw, 
                                       tx[0][c], tx[1][c], 
                                       tx[2][c], tx[3][c]);
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static INLINE void
img_filter_2d_nearest_repeat_POT(struct sp_sampler_view *sp_sview,
                                 struct sp_sampler *sp_samp,
                                 float s,
                                 float t,
                                 float p,
                                 unsigned level,
                                 unsigned face_id,
                                 float rgba[TGSI_QUAD_SIZE])
{
   unsigned xpot = pot_level_size(sp_sview->xpot, level);
   unsigned ypot = pot_level_size(sp_sview->ypot, level);
   const float *out;
   union tex_tile_address addr;
   int c;

   float u = s * xpot;
   float v = t * ypot;

   int uflr = util_ifloor(u);
   int vflr = util_ifloor(v);

   int x0 = uflr & (xpot - 1);
   int y0 = vflr & (ypot - 1);

   addr.value = 0;
   addr.bits.level = level;

   out = get_texel_2d_no_border(sp_sview, addr, x0, y0);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static INLINE void
img_filter_2d_nearest_clamp_POT(struct sp_sampler_view *sp_sview,
                                struct sp_sampler *sp_samp,
                                float s,
                                float t,
                                float p,
                                unsigned level,
                                unsigned face_id,
                                float rgba[TGSI_QUAD_SIZE])
{
   unsigned xpot = pot_level_size(sp_sview->xpot, level);
   unsigned ypot = pot_level_size(sp_sview->ypot, level);
   union tex_tile_address addr;
   int c;

   float u = s * xpot;
   float v = t * ypot;

   int x0, y0;
   const float *out;

   addr.value = 0;
   addr.bits.level = level;

   x0 = util_ifloor(u);
   if (x0 < 0) 
      x0 = 0;
   else if (x0 > (int) xpot - 1)
      x0 = xpot - 1;

   y0 = util_ifloor(v);
   if (y0 < 0) 
      y0 = 0;
   else if (y0 > (int) ypot - 1)
      y0 = ypot - 1;
   
   out = get_texel_2d_no_border(sp_sview, addr, x0, y0);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_1d_nearest(struct sp_sampler_view *sp_sview,
                      struct sp_sampler *sp_samp,
                      float s,
                      float t,
                      float p,
                      unsigned level,
                      unsigned face_id,
                      float rgba[TGSI_QUAD_SIZE])
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width;
   int x;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->nearest_texcoord_s(s, width, &x);

   out = get_texel_2d(sp_sview, sp_samp, addr, x, 0);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_1d_array_nearest(struct sp_sampler_view *sp_sview,
                            struct sp_sampler *sp_samp,
                            float s,
                            float t,
                            float p,
                            unsigned level,
                            unsigned face_id,
                            float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width;
   int x, layer;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->nearest_texcoord_s(s, width, &x);
   wrap_array_layer(t, texture->array_size, &layer);

   out = get_texel_1d_array(sp_sview, sp_samp, addr, x, layer);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_2d_nearest(struct sp_sampler_view *sp_sview,
                      struct sp_sampler *sp_samp,
                      float s,
                      float t,
                      float p,
                      unsigned level,
                      unsigned face_id,
                      float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x, y;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = level;

   sp_samp->nearest_texcoord_s(s, width, &x);
   sp_samp->nearest_texcoord_t(t, height, &y);

   out = get_texel_2d(sp_sview, sp_samp, addr, x, y);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_2d_array_nearest(struct sp_sampler_view *sp_sview,
                            struct sp_sampler *sp_samp,
                            float s,
                            float t,
                            float p,
                            unsigned level,
                            unsigned face_id,
                            float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x, y, layer;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = level;

   sp_samp->nearest_texcoord_s(s, width, &x);
   sp_samp->nearest_texcoord_t(t, height, &y);
   wrap_array_layer(p, texture->array_size, &layer);

   out = get_texel_2d_array(sp_sview, sp_samp, addr, x, y, layer);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static INLINE union tex_tile_address
face(union tex_tile_address addr, unsigned face )
{
   addr.bits.face = face;
   return addr;
}


static void
img_filter_cube_nearest(struct sp_sampler_view *sp_sview,
                        struct sp_sampler *sp_samp,
                        float s,
                        float t,
                        float p,
                        unsigned level,
                        unsigned face_id,
                        float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x, y;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = level;

   /*
    * If NEAREST filtering is done within a miplevel, always apply wrap
    * mode CLAMP_TO_EDGE.
    */
   if (sp_samp->base.seamless_cube_map) {
      wrap_nearest_clamp_to_edge(s, width, &x);
      wrap_nearest_clamp_to_edge(t, height, &y);
   } else {
      /* Would probably make sense to ignore mode and just do edge clamp */
      sp_samp->nearest_texcoord_s(s, width, &x);
      sp_samp->nearest_texcoord_t(t, height, &y);
   }

   out = get_texel_2d(sp_sview, sp_samp, face(addr, face_id), x, y);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}

static void
img_filter_cube_array_nearest(struct sp_sampler_view *sp_sview,
                              struct sp_sampler *sp_samp,
                              float s,
                              float t,
                              float p,
                              unsigned level,
                              unsigned face_id,
                              float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x, y, layer;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = level;

   sp_samp->nearest_texcoord_s(s, width, &x);
   sp_samp->nearest_texcoord_t(t, height, &y);
   wrap_array_layer(p, texture->array_size, &layer);

   out = get_texel_cube_array(sp_sview, sp_samp, addr, x, y, layer * 6 + face_id);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}

static void
img_filter_3d_nearest(struct sp_sampler_view *sp_sview,
                      struct sp_sampler *sp_samp,
                      float s,
                      float t,
                      float p,
                      unsigned level,
                      unsigned face_id,
                      float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height, depth;
   int x, y, z;
   union tex_tile_address addr;
   const float *out;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);
   depth = u_minify(texture->depth0, level);

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   sp_samp->nearest_texcoord_s(s, width,  &x);
   sp_samp->nearest_texcoord_t(t, height, &y);
   sp_samp->nearest_texcoord_p(p, depth,  &z);

   addr.value = 0;
   addr.bits.level = level;

   out = get_texel_3d(sp_sview, sp_samp, addr, x, y, z);
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = out[c];
}


static void
img_filter_1d_linear(struct sp_sampler_view *sp_sview,
                     struct sp_sampler *sp_samp,
                     float s,
                     float t,
                     float p,
                     unsigned level,
                     unsigned face_id,
                     float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width;
   int x0, x1;
   float xw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1;
   int c;

   width = u_minify(texture->width0, level);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->linear_texcoord_s(s, width, &x0, &x1, &xw);

   tx0 = get_texel_2d(sp_sview, sp_samp, addr, x0, 0);
   tx1 = get_texel_2d(sp_sview, sp_samp, addr, x1, 0);

   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp(xw, tx0[c], tx1[c]);
}


static void
img_filter_1d_array_linear(struct sp_sampler_view *sp_sview,
                           struct sp_sampler *sp_samp,
                           float s,
                           float t,
                           float p,
                           unsigned level,
                           unsigned face_id,
                           float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width;
   int x0, x1, layer;
   float xw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1;
   int c;

   width = u_minify(texture->width0, level);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->linear_texcoord_s(s, width, &x0, &x1, &xw);
   wrap_array_layer(t, texture->array_size, &layer);

   tx0 = get_texel_1d_array(sp_sview, sp_samp, addr, x0, layer);
   tx1 = get_texel_1d_array(sp_sview, sp_samp, addr, x1, layer);

   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp(xw, tx0[c], tx1[c]);
}


static void
img_filter_2d_linear(struct sp_sampler_view *sp_sview,
                     struct sp_sampler *sp_samp,
                     float s,
                     float t,
                     float p,
                     unsigned level,
                     unsigned face_id,
                     float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x0, y0, x1, y1;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1, *tx2, *tx3;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->linear_texcoord_s(s, width,  &x0, &x1, &xw);
   sp_samp->linear_texcoord_t(t, height, &y0, &y1, &yw);

   tx0 = get_texel_2d(sp_sview, sp_samp, addr, x0, y0);
   tx1 = get_texel_2d(sp_sview, sp_samp, addr, x1, y0);
   tx2 = get_texel_2d(sp_sview, sp_samp, addr, x0, y1);
   tx3 = get_texel_2d(sp_sview, sp_samp, addr, x1, y1);

   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
                                          tx0[c], tx1[c],
                                          tx2[c], tx3[c]);
}


static void
img_filter_2d_array_linear(struct sp_sampler_view *sp_sview,
                           struct sp_sampler *sp_samp,
                           float s,
                           float t,
                           float p,
                           unsigned level,
                           unsigned face_id,
                           float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x0, y0, x1, y1, layer;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1, *tx2, *tx3;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->linear_texcoord_s(s, width,  &x0, &x1, &xw);
   sp_samp->linear_texcoord_t(t, height, &y0, &y1, &yw);
   wrap_array_layer(p, texture->array_size, &layer);

   tx0 = get_texel_2d_array(sp_sview, sp_samp, addr, x0, y0, layer);
   tx1 = get_texel_2d_array(sp_sview, sp_samp, addr, x1, y0, layer);
   tx2 = get_texel_2d_array(sp_sview, sp_samp, addr, x0, y1, layer);
   tx3 = get_texel_2d_array(sp_sview, sp_samp, addr, x1, y1, layer);

   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
                                          tx0[c], tx1[c],
                                          tx2[c], tx3[c]);
}


static void
img_filter_cube_linear(struct sp_sampler_view *sp_sview,
                       struct sp_sampler *sp_samp,
                       float s,
                       float t,
                       float p,
                       unsigned level,
                       unsigned face_id,
                       float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x0, y0, x1, y1;
   float xw, yw; /* weights */
   union tex_tile_address addr, addrj;
   const float *tx0, *tx1, *tx2, *tx3;
   float corner0[TGSI_QUAD_SIZE], corner1[TGSI_QUAD_SIZE],
         corner2[TGSI_QUAD_SIZE], corner3[TGSI_QUAD_SIZE];
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = level;

   /*
    * For seamless if LINEAR filtering is done within a miplevel,
    * always apply wrap mode CLAMP_TO_BORDER.
    */
   if (sp_samp->base.seamless_cube_map) {
      /* Note this is a bit overkill, actual clamping is not required */
      wrap_linear_clamp_to_border(s, width, &x0, &x1, &xw);
      wrap_linear_clamp_to_border(t, height, &y0, &y1, &yw);
   } else {
      /* Would probably make sense to ignore mode and just do edge clamp */
      sp_samp->linear_texcoord_s(s, width,  &x0, &x1, &xw);
      sp_samp->linear_texcoord_t(t, height, &y0, &y1, &yw);
   }

   addrj = face(addr, face_id);

   if (sp_samp->base.seamless_cube_map) {
      tx0 = get_texel_cube_seamless(sp_sview, addrj, x0, y0, corner0);
      tx1 = get_texel_cube_seamless(sp_sview, addrj, x1, y0, corner1);
      tx2 = get_texel_cube_seamless(sp_sview, addrj, x0, y1, corner2);
      tx3 = get_texel_cube_seamless(sp_sview, addrj, x1, y1, corner3);
   } else {
      tx0 = get_texel_2d(sp_sview, sp_samp, addrj, x0, y0);
      tx1 = get_texel_2d(sp_sview, sp_samp, addrj, x1, y0);
      tx2 = get_texel_2d(sp_sview, sp_samp, addrj, x0, y1);
      tx3 = get_texel_2d(sp_sview, sp_samp, addrj, x1, y1);
   }
   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
                                          tx0[c], tx1[c],
                                          tx2[c], tx3[c]);
}


static void
img_filter_cube_array_linear(struct sp_sampler_view *sp_sview,
                             struct sp_sampler *sp_samp,
                             float s,
                             float t,
                             float p,
                             unsigned level,
                             unsigned face_id,
                             float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height;
   int x0, y0, x1, y1, layer;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1, *tx2, *tx3;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = level;

   sp_samp->linear_texcoord_s(s, width,  &x0, &x1, &xw);
   sp_samp->linear_texcoord_t(t, height, &y0, &y1, &yw);
   wrap_array_layer(p, texture->array_size, &layer);

   tx0 = get_texel_cube_array(sp_sview, sp_samp, addr, x0, y0, layer * 6 + face_id);
   tx1 = get_texel_cube_array(sp_sview, sp_samp, addr, x1, y0, layer * 6 + face_id);
   tx2 = get_texel_cube_array(sp_sview, sp_samp, addr, x0, y1, layer * 6 + face_id);
   tx3 = get_texel_cube_array(sp_sview, sp_samp, addr, x1, y1, layer * 6 + face_id);

   /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
                                          tx0[c], tx1[c],
                                          tx2[c], tx3[c]);
}

static void
img_filter_3d_linear(struct sp_sampler_view *sp_sview,
                     struct sp_sampler *sp_samp,
                     float s,
                     float t,
                     float p,
                     unsigned level,
                     unsigned face_id,
                     float *rgba)
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   int width, height, depth;
   int x0, x1, y0, y1, z0, z1;
   float xw, yw, zw; /* interpolation weights */
   union tex_tile_address addr;
   const float *tx00, *tx01, *tx02, *tx03, *tx10, *tx11, *tx12, *tx13;
   int c;

   width = u_minify(texture->width0, level);
   height = u_minify(texture->height0, level);
   depth = u_minify(texture->depth0, level);

   addr.value = 0;
   addr.bits.level = level;

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   sp_samp->linear_texcoord_s(s, width,  &x0, &x1, &xw);
   sp_samp->linear_texcoord_t(t, height, &y0, &y1, &yw);
   sp_samp->linear_texcoord_p(p, depth,  &z0, &z1, &zw);


   tx00 = get_texel_3d(sp_sview, sp_samp, addr, x0, y0, z0);
   tx01 = get_texel_3d(sp_sview, sp_samp, addr, x1, y0, z0);
   tx02 = get_texel_3d(sp_sview, sp_samp, addr, x0, y1, z0);
   tx03 = get_texel_3d(sp_sview, sp_samp, addr, x1, y1, z0);
      
   tx10 = get_texel_3d(sp_sview, sp_samp, addr, x0, y0, z1);
   tx11 = get_texel_3d(sp_sview, sp_samp, addr, x1, y0, z1);
   tx12 = get_texel_3d(sp_sview, sp_samp, addr, x0, y1, z1);
   tx13 = get_texel_3d(sp_sview, sp_samp, addr, x1, y1, z1);
      
      /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_QUAD_SIZE; c++)
      rgba[TGSI_NUM_CHANNELS*c] =  lerp_3d(xw, yw, zw,
                                           tx00[c], tx01[c],
                                           tx02[c], tx03[c],
                                           tx10[c], tx11[c],
                                           tx12[c], tx13[c]);
}


/* Calculate level of detail for every fragment,
 * with lambda already computed.
 * Note that lambda has already been biased by global LOD bias.
 * \param biased_lambda per-quad lambda.
 * \param lod_in per-fragment lod_bias or explicit_lod.
 * \param lod returns the per-fragment lod.
 */
static INLINE void
compute_lod(const struct pipe_sampler_state *sampler,
            enum tgsi_sampler_control control,
            const float biased_lambda,
            const float lod_in[TGSI_QUAD_SIZE],
            float lod[TGSI_QUAD_SIZE])
{
   float min_lod = sampler->min_lod;
   float max_lod = sampler->max_lod;
   uint i;

   switch (control) {
   case tgsi_sampler_lod_none:
   case tgsi_sampler_lod_zero:
   /* XXX FIXME */
   case tgsi_sampler_derivs_explicit:
      lod[0] = lod[1] = lod[2] = lod[3] = CLAMP(biased_lambda, min_lod, max_lod);
      break;
   case tgsi_sampler_lod_bias:
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         lod[i] = biased_lambda + lod_in[i];
         lod[i] = CLAMP(lod[i], min_lod, max_lod);
      }
      break;
   case tgsi_sampler_lod_explicit:
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         lod[i] = CLAMP(lod_in[i], min_lod, max_lod);
      }
      break;
   default:
      assert(0);
      lod[0] = lod[1] = lod[2] = lod[3] = 0.0f;
   }
}


/* Calculate level of detail for every fragment.
 * \param lod_in per-fragment lod_bias or explicit_lod.
 * \param lod results per-fragment lod.
 */
static INLINE void
compute_lambda_lod(struct sp_sampler_view *sp_sview,
                   struct sp_sampler *sp_samp,
                   const float s[TGSI_QUAD_SIZE],
                   const float t[TGSI_QUAD_SIZE],
                   const float p[TGSI_QUAD_SIZE],
                   const float lod_in[TGSI_QUAD_SIZE],
                   enum tgsi_sampler_control control,
                   float lod[TGSI_QUAD_SIZE])
{
   const struct pipe_sampler_state *sampler = &sp_samp->base;
   float lod_bias = sampler->lod_bias;
   float min_lod = sampler->min_lod;
   float max_lod = sampler->max_lod;
   float lambda;
   uint i;

   switch (control) {
   case tgsi_sampler_lod_none:
      /* XXX FIXME */
   case tgsi_sampler_derivs_explicit:
      lambda = sp_sview->compute_lambda(sp_sview, s, t, p) + lod_bias;
      lod[0] = lod[1] = lod[2] = lod[3] = CLAMP(lambda, min_lod, max_lod);
      break;
   case tgsi_sampler_lod_bias:
      lambda = sp_sview->compute_lambda(sp_sview, s, t, p) + lod_bias;
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         lod[i] = lambda + lod_in[i];
         lod[i] = CLAMP(lod[i], min_lod, max_lod);
      }
      break;
   case tgsi_sampler_lod_explicit:
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         lod[i] = CLAMP(lod_in[i], min_lod, max_lod);
      }
      break;
   case tgsi_sampler_lod_zero:
      /* this is all static state in the sampler really need clamp here? */
      lod[0] = lod[1] = lod[2] = lod[3] = CLAMP(lod_bias, min_lod, max_lod);
      break;
   default:
      assert(0);
      lod[0] = lod[1] = lod[2] = lod[3] = 0.0f;
   }
}


static void
mip_filter_linear(struct sp_sampler_view *sp_sview,
                  struct sp_sampler *sp_samp,
                  img_filter_func min_filter,
                  img_filter_func mag_filter,
                  const float s[TGSI_QUAD_SIZE],
                  const float t[TGSI_QUAD_SIZE],
                  const float p[TGSI_QUAD_SIZE],
                  const float c0[TGSI_QUAD_SIZE],
                  const float lod_in[TGSI_QUAD_SIZE],
                  enum tgsi_sampler_control control,
                  float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   const struct pipe_sampler_view *psview = &sp_sview->base;
   int j;
   float lod[TGSI_QUAD_SIZE];

   compute_lambda_lod(sp_sview, sp_samp, s, t, p, lod_in, control, lod);

   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      int level0 = psview->u.tex.first_level + (int)lod[j];

      if (lod[j] < 0.0)
         mag_filter(sp_sview, sp_samp, s[j], t[j], p[j],
                    psview->u.tex.first_level,
                    sp_sview->faces[j], &rgba[0][j]);

      else if (level0 >= (int) psview->u.tex.last_level)
         min_filter(sp_sview, sp_samp, s[j], t[j], p[j], psview->u.tex.last_level,
                    sp_sview->faces[j], &rgba[0][j]);

      else {
         float levelBlend = frac(lod[j]);
         float rgbax[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
         int c;

         min_filter(sp_sview, sp_samp, s[j], t[j], p[j], level0,
                    sp_sview->faces[j], &rgbax[0][0]);
         min_filter(sp_sview, sp_samp, s[j], t[j], p[j], level0+1,
                    sp_sview->faces[j], &rgbax[0][1]);

         for (c = 0; c < 4; c++) {
            rgba[c][j] = lerp(levelBlend, rgbax[c][0], rgbax[c][1]);
         }
      }
   }

   if (DEBUG_TEX) {
      print_sample_4(__FUNCTION__, rgba);
   }
}


/**
 * Compute nearest mipmap level from texcoords.
 * Then sample the texture level for four elements of a quad.
 * \param c0  the LOD bias factors, or absolute LODs (depending on control)
 */
static void
mip_filter_nearest(struct sp_sampler_view *sp_sview,
                   struct sp_sampler *sp_samp,
                   img_filter_func min_filter,
                   img_filter_func mag_filter,
                   const float s[TGSI_QUAD_SIZE],
                   const float t[TGSI_QUAD_SIZE],
                   const float p[TGSI_QUAD_SIZE],
                   const float c0[TGSI_QUAD_SIZE],
                   const float lod_in[TGSI_QUAD_SIZE],
                   enum tgsi_sampler_control control,
                   float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   const struct pipe_sampler_view *psview = &sp_sview->base;
   float lod[TGSI_QUAD_SIZE];
   int j;

   compute_lambda_lod(sp_sview, sp_samp, s, t, p, lod_in, control, lod);

   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      if (lod[j] < 0.0)
         mag_filter(sp_sview, sp_samp, s[j], t[j], p[j],
                    psview->u.tex.first_level,
                    sp_sview->faces[j], &rgba[0][j]);
      else {
         int level = psview->u.tex.first_level + (int)(lod[j] + 0.5F);
         level = MIN2(level, (int)psview->u.tex.last_level);
         min_filter(sp_sview, sp_samp, s[j], t[j], p[j],
                    level, sp_sview->faces[j], &rgba[0][j]);
      }
   }

   if (DEBUG_TEX) {
      print_sample_4(__FUNCTION__, rgba);
   }
}


static void
mip_filter_none(struct sp_sampler_view *sp_sview,
                struct sp_sampler *sp_samp,
                img_filter_func min_filter,
                img_filter_func mag_filter,
                const float s[TGSI_QUAD_SIZE],
                const float t[TGSI_QUAD_SIZE],
                const float p[TGSI_QUAD_SIZE],
                const float c0[TGSI_QUAD_SIZE],
                const float lod_in[TGSI_QUAD_SIZE],
                enum tgsi_sampler_control control,
                float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   float lod[TGSI_QUAD_SIZE];
   int j;

   compute_lambda_lod(sp_sview, sp_samp, s, t, p, lod_in, control, lod);

   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      if (lod[j] < 0.0) { 
         mag_filter(sp_sview, sp_samp, s[j], t[j], p[j],
                    sp_sview->base.u.tex.first_level,
                    sp_sview->faces[j], &rgba[0][j]);
      }
      else {
         min_filter(sp_sview, sp_samp, s[j], t[j], p[j],
                    sp_sview->base.u.tex.first_level,
                    sp_sview->faces[j], &rgba[0][j]);
      }
   }
}


static void
mip_filter_none_no_filter_select(struct sp_sampler_view *sp_sview,
                                 struct sp_sampler *sp_samp,
                                 img_filter_func min_filter,
                                 img_filter_func mag_filter,
                                 const float s[TGSI_QUAD_SIZE],
                                 const float t[TGSI_QUAD_SIZE],
                                 const float p[TGSI_QUAD_SIZE],
                                 const float c0[TGSI_QUAD_SIZE],
                                 const float lod_in[TGSI_QUAD_SIZE],
                                 enum tgsi_sampler_control control,
                                 float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   int j;

   for (j = 0; j < TGSI_QUAD_SIZE; j++)
      mag_filter(sp_sview, sp_samp, s[j], t[j], p[j],
                 sp_sview->base.u.tex.first_level,
                 sp_sview->faces[j], &rgba[0][j]);
}


/* For anisotropic filtering */
#define WEIGHT_LUT_SIZE 1024

static float *weightLut = NULL;

/**
 * Creates the look-up table used to speed-up EWA sampling
 */
static void
create_filter_table(void)
{
   unsigned i;
   if (!weightLut) {
      weightLut = (float *) MALLOC(WEIGHT_LUT_SIZE * sizeof(float));

      for (i = 0; i < WEIGHT_LUT_SIZE; ++i) {
         float alpha = 2;
         float r2 = (float) i / (float) (WEIGHT_LUT_SIZE - 1);
         float weight = (float) exp(-alpha * r2);
         weightLut[i] = weight;
      }
   }
}


/**
 * Elliptical weighted average (EWA) filter for producing high quality
 * anisotropic filtered results.
 * Based on the Higher Quality Elliptical Weighted Average Filter
 * published by Paul S. Heckbert in his Master's Thesis
 * "Fundamentals of Texture Mapping and Image Warping" (1989)
 */
static void
img_filter_2d_ewa(struct sp_sampler_view *sp_sview,
                  struct sp_sampler *sp_samp,
                  img_filter_func min_filter,
                  img_filter_func mag_filter,
                  const float s[TGSI_QUAD_SIZE],
                  const float t[TGSI_QUAD_SIZE],
                  const float p[TGSI_QUAD_SIZE],
                  unsigned level,
                  const float dudx, const float dvdx,
                  const float dudy, const float dvdy,
                  float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   const struct pipe_resource *texture = sp_sview->base.texture;

   // ??? Won't the image filters blow up if level is negative?
   unsigned level0 = level > 0 ? level : 0;
   float scaling = 1.0f / (1 << level0);
   int width = u_minify(texture->width0, level0);
   int height = u_minify(texture->height0, level0);

   float ux = dudx * scaling;
   float vx = dvdx * scaling;
   float uy = dudy * scaling;
   float vy = dvdy * scaling;

   /* compute ellipse coefficients to bound the region: 
    * A*x*x + B*x*y + C*y*y = F.
    */
   float A = vx*vx+vy*vy+1;
   float B = -2*(ux*vx+uy*vy);
   float C = ux*ux+uy*uy+1;
   float F = A*C-B*B/4.0f;

   /* check if it is an ellipse */
   /* ASSERT(F > 0.0); */

   /* Compute the ellipse's (u,v) bounding box in texture space */
   float d = -B*B+4.0f*C*A;
   float box_u = 2.0f / d * sqrtf(d*C*F); /* box_u -> half of bbox with   */
   float box_v = 2.0f / d * sqrtf(A*d*F); /* box_v -> half of bbox height */

   float rgba_temp[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
   float s_buffer[TGSI_QUAD_SIZE];
   float t_buffer[TGSI_QUAD_SIZE];
   float weight_buffer[TGSI_QUAD_SIZE];
   unsigned buffer_next;
   int j;
   float den; /* = 0.0F; */
   float ddq;
   float U; /* = u0 - tex_u; */
   int v;

   /* Scale ellipse formula to directly index the Filter Lookup Table.
    * i.e. scale so that F = WEIGHT_LUT_SIZE-1
    */
   double formScale = (double) (WEIGHT_LUT_SIZE - 1) / F;
   A *= formScale;
   B *= formScale;
   C *= formScale;
   /* F *= formScale; */ /* no need to scale F as we don't use it below here */

   /* For each quad, the du and dx values are the same and so the ellipse is
    * also the same. Note that texel/image access can only be performed using
    * a quad, i.e. it is not possible to get the pixel value for a single
    * tex coord. In order to have a better performance, the access is buffered
    * using the s_buffer/t_buffer and weight_buffer. Only when the buffer is
    * full, then the pixel values are read from the image.
    */
   ddq = 2 * A;
   
   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      /* Heckbert MS thesis, p. 59; scan over the bounding box of the ellipse
       * and incrementally update the value of Ax^2+Bxy*Cy^2; when this
       * value, q, is less than F, we're inside the ellipse
       */
      float tex_u = -0.5F + s[j] * texture->width0 * scaling;
      float tex_v = -0.5F + t[j] * texture->height0 * scaling;

      int u0 = (int) floorf(tex_u - box_u);
      int u1 = (int) ceilf(tex_u + box_u);
      int v0 = (int) floorf(tex_v - box_v);
      int v1 = (int) ceilf(tex_v + box_v);

      float num[4] = {0.0F, 0.0F, 0.0F, 0.0F};
      buffer_next = 0;
      den = 0;
      U = u0 - tex_u;
      for (v = v0; v <= v1; ++v) {
         float V = v - tex_v;
         float dq = A * (2 * U + 1) + B * V;
         float q = (C * V + B * U) * V + A * U * U;

         int u;
         for (u = u0; u <= u1; ++u) {
            /* Note that the ellipse has been pre-scaled so F =
             * WEIGHT_LUT_SIZE - 1
             */
            if (q < WEIGHT_LUT_SIZE) {
               /* as a LUT is used, q must never be negative;
                * should not happen, though
                */
               const int qClamped = q >= 0.0F ? q : 0;
               float weight = weightLut[qClamped];

               weight_buffer[buffer_next] = weight;
               s_buffer[buffer_next] = u / ((float) width);
               t_buffer[buffer_next] = v / ((float) height);
            
               buffer_next++;
               if (buffer_next == TGSI_QUAD_SIZE) {
                  /* 4 texel coords are in the buffer -> read it now */
                  unsigned jj;
                  /* it is assumed that samp->min_img_filter is set to
                   * img_filter_2d_nearest or one of the
                   * accelerated img_filter_2d_nearest_XXX functions.
                   */
                  for (jj = 0; jj < buffer_next; jj++) {
                     min_filter(sp_sview, sp_samp, s_buffer[jj], t_buffer[jj], p[jj],
                                level, sp_sview->faces[j], &rgba_temp[0][jj]);
                     num[0] += weight_buffer[jj] * rgba_temp[0][jj];
                     num[1] += weight_buffer[jj] * rgba_temp[1][jj];
                     num[2] += weight_buffer[jj] * rgba_temp[2][jj];
                     num[3] += weight_buffer[jj] * rgba_temp[3][jj];
                  }

                  buffer_next = 0;
               }

               den += weight;
            }
            q += dq;
            dq += ddq;
         }
      }

      /* if the tex coord buffer contains unread values, we will read
       * them now.
       */
      if (buffer_next > 0) {
         unsigned jj;
         /* it is assumed that samp->min_img_filter is set to
          * img_filter_2d_nearest or one of the
          * accelerated img_filter_2d_nearest_XXX functions.
          */
         for (jj = 0; jj < buffer_next; jj++) {
            min_filter(sp_sview, sp_samp, s_buffer[jj], t_buffer[jj], p[jj],
                       level, sp_sview->faces[j], &rgba_temp[0][jj]);
            num[0] += weight_buffer[jj] * rgba_temp[0][jj];
            num[1] += weight_buffer[jj] * rgba_temp[1][jj];
            num[2] += weight_buffer[jj] * rgba_temp[2][jj];
            num[3] += weight_buffer[jj] * rgba_temp[3][jj];
         }
      }

      if (den <= 0.0F) {
         /* Reaching this place would mean that no pixels intersected
          * the ellipse.  This should never happen because the filter
          * we use always intersects at least one pixel.
          */

         /*rgba[0]=0;
         rgba[1]=0;
         rgba[2]=0;
         rgba[3]=0;*/
         /* not enough pixels in resampling, resort to direct interpolation */
         min_filter(sp_sview, sp_samp, s[j], t[j], p[j], level,
                    sp_sview->faces[j], &rgba_temp[0][j]);
         den = 1;
         num[0] = rgba_temp[0][j];
         num[1] = rgba_temp[1][j];
         num[2] = rgba_temp[2][j];
         num[3] = rgba_temp[3][j];
      }

      rgba[0][j] = num[0] / den;
      rgba[1][j] = num[1] / den;
      rgba[2][j] = num[2] / den;
      rgba[3][j] = num[3] / den;
   }
}


/**
 * Sample 2D texture using an anisotropic filter.
 */
static void
mip_filter_linear_aniso(struct sp_sampler_view *sp_sview,
                        struct sp_sampler *sp_samp,
                        img_filter_func min_filter,
                        img_filter_func mag_filter,
                        const float s[TGSI_QUAD_SIZE],
                        const float t[TGSI_QUAD_SIZE],
                        const float p[TGSI_QUAD_SIZE],
                        const float c0[TGSI_QUAD_SIZE],
                        const float lod_in[TGSI_QUAD_SIZE],
                        enum tgsi_sampler_control control,
                        float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   const struct pipe_resource *texture = sp_sview->base.texture;
   const struct pipe_sampler_view *psview = &sp_sview->base;
   int level0;
   float lambda;
   float lod[TGSI_QUAD_SIZE];

   float s_to_u = u_minify(texture->width0, psview->u.tex.first_level);
   float t_to_v = u_minify(texture->height0, psview->u.tex.first_level);
   float dudx = (s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]) * s_to_u;
   float dudy = (s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]) * s_to_u;
   float dvdx = (t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]) * t_to_v;
   float dvdy = (t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]) * t_to_v;
   
   if (control == tgsi_sampler_lod_bias ||
       control == tgsi_sampler_lod_none ||
       /* XXX FIXME */
       control == tgsi_sampler_derivs_explicit) {
      /* note: instead of working with Px and Py, we will use the 
       * squared length instead, to avoid sqrt.
       */
      float Px2 = dudx * dudx + dvdx * dvdx;
      float Py2 = dudy * dudy + dvdy * dvdy;

      float Pmax2;
      float Pmin2;
      float e;
      const float maxEccentricity = sp_samp->base.max_anisotropy * sp_samp->base.max_anisotropy;
      
      if (Px2 < Py2) {
         Pmax2 = Py2;
         Pmin2 = Px2;
      }
      else {
         Pmax2 = Px2;
         Pmin2 = Py2;
      }
      
      /* if the eccentricity of the ellipse is too big, scale up the shorter
       * of the two vectors to limit the maximum amount of work per pixel
       */
      e = Pmax2 / Pmin2;
      if (e > maxEccentricity) {
         /* float s=e / maxEccentricity;
            minor[0] *= s;
            minor[1] *= s;
            Pmin2 *= s; */
         Pmin2 = Pmax2 / maxEccentricity;
      }
      
      /* note: we need to have Pmin=sqrt(Pmin2) here, but we can avoid
       * this since 0.5*log(x) = log(sqrt(x))
       */
      lambda = 0.5F * util_fast_log2(Pmin2) + sp_samp->base.lod_bias;
      compute_lod(&sp_samp->base, control, lambda, lod_in, lod);
   }
   else {
      assert(control == tgsi_sampler_lod_explicit ||
             control == tgsi_sampler_lod_zero);
      compute_lod(&sp_samp->base, control, sp_samp->base.lod_bias, lod_in, lod);
   }
   
   /* XXX: Take into account all lod values.
    */
   lambda = lod[0];
   level0 = psview->u.tex.first_level + (int)lambda;

   /* If the ellipse covers the whole image, we can
    * simply return the average of the whole image.
    */
   if (level0 >= (int) psview->u.tex.last_level) {
      int j;
      for (j = 0; j < TGSI_QUAD_SIZE; j++)
         min_filter(sp_sview, sp_samp, s[j], t[j], p[j], psview->u.tex.last_level,
                    sp_sview->faces[j], &rgba[0][j]);
   }
   else {
      /* don't bother interpolating between multiple LODs; it doesn't
       * seem to be worth the extra running time.
       */
      img_filter_2d_ewa(sp_sview, sp_samp, min_filter, mag_filter,
                        s, t, p, level0,
                        dudx, dvdx, dudy, dvdy, rgba);
   }

   if (DEBUG_TEX) {
      print_sample_4(__FUNCTION__, rgba);
   }
}


/**
 * Specialized version of mip_filter_linear with hard-wired calls to
 * 2d lambda calculation and 2d_linear_repeat_POT img filters.
 */
static void
mip_filter_linear_2d_linear_repeat_POT(
   struct sp_sampler_view *sp_sview,
   struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   const float c0[TGSI_QUAD_SIZE],
   const float lod_in[TGSI_QUAD_SIZE],
   enum tgsi_sampler_control control,
   float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   const struct pipe_sampler_view *psview = &sp_sview->base;
   int j;
   float lod[TGSI_QUAD_SIZE];

   compute_lambda_lod(sp_sview, sp_samp, s, t, p, lod_in, control, lod);

   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      int level0 = psview->u.tex.first_level + (int)lod[j];

      /* Catches both negative and large values of level0:
       */
      if ((unsigned)level0 >= psview->u.tex.last_level) {
         if (level0 < 0)
            img_filter_2d_linear_repeat_POT(sp_sview, sp_samp, s[j], t[j], p[j],
                                            psview->u.tex.first_level,
                                            sp_sview->faces[j], &rgba[0][j]);
         else
            img_filter_2d_linear_repeat_POT(sp_sview, sp_samp, s[j], t[j], p[j],
                                            psview->u.tex.last_level,
                                            sp_sview->faces[j], &rgba[0][j]);

      }
      else {
         float levelBlend = frac(lod[j]);
         float rgbax[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
         int c;

         img_filter_2d_linear_repeat_POT(sp_sview, sp_samp, s[j], t[j], p[j], level0,
                                         sp_sview->faces[j], &rgbax[0][0]);
         img_filter_2d_linear_repeat_POT(sp_sview, sp_samp, s[j], t[j], p[j], level0+1,
                                         sp_sview->faces[j], &rgbax[0][1]);

         for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            rgba[c][j] = lerp(levelBlend, rgbax[c][0], rgbax[c][1]);
      }
   }

   if (DEBUG_TEX) {
      print_sample_4(__FUNCTION__, rgba);
   }
}


/**
 * Do shadow/depth comparisons.
 */
static void
sample_compare(struct sp_sampler_view *sp_sview,
               struct sp_sampler *sp_samp,
               const float s[TGSI_QUAD_SIZE],
               const float t[TGSI_QUAD_SIZE],
               const float p[TGSI_QUAD_SIZE],
               const float c0[TGSI_QUAD_SIZE],
               const float c1[TGSI_QUAD_SIZE],
               enum tgsi_sampler_control control,
               float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   const struct pipe_sampler_state *sampler = &sp_samp->base;
   int j;
   int k[4];
   float pc[4];
   const struct util_format_description *format_desc;
   unsigned chan_type;

   /**
    * Compare texcoord 'p' (aka R) against texture value 'rgba[0]'
    * for 2D Array texture we need to use the 'c0' (aka Q).
    * When we sampled the depth texture, the depth value was put into all
    * RGBA channels.  We look at the red channel here.
    */

   if (sp_sview->base.texture->target == PIPE_TEXTURE_2D_ARRAY ||
       sp_sview->base.texture->target == PIPE_TEXTURE_CUBE) {
      pc[0] = c0[0];
      pc[1] = c0[1];
      pc[2] = c0[2];
      pc[3] = c0[3];
   } else if (sp_sview->base.texture->target == PIPE_TEXTURE_CUBE_ARRAY) {
      pc[0] = c1[0];
      pc[1] = c1[1];
      pc[2] = c1[2];
      pc[3] = c1[3];
   } else {
      pc[0] = p[0];
      pc[1] = p[1];
      pc[2] = p[2];
      pc[3] = p[3];
   }

   format_desc = util_format_description(sp_sview->base.format);
   /* not entirely sure we couldn't end up with non-valid swizzle here */
   chan_type = format_desc->swizzle[0] <= UTIL_FORMAT_SWIZZLE_W ?
                  format_desc->channel[format_desc->swizzle[0]].type :
                  UTIL_FORMAT_TYPE_FLOAT;
   if (chan_type != UTIL_FORMAT_TYPE_FLOAT) {
      /*
       * clamping is a result of conversion to texture format, hence
       * doesn't happen with floats. Technically also should do comparison
       * in texture format (quantization!).
       */
      pc[0] = CLAMP(pc[0], 0.0F, 1.0F);
      pc[1] = CLAMP(pc[1], 0.0F, 1.0F);
      pc[2] = CLAMP(pc[2], 0.0F, 1.0F);
      pc[3] = CLAMP(pc[3], 0.0F, 1.0F);
   }

   /* compare four texcoords vs. four texture samples */
   switch (sampler->compare_func) {
   case PIPE_FUNC_LESS:
      k[0] = pc[0] < rgba[0][0];
      k[1] = pc[1] < rgba[0][1];
      k[2] = pc[2] < rgba[0][2];
      k[3] = pc[3] < rgba[0][3];
      break;
   case PIPE_FUNC_LEQUAL:
      k[0] = pc[0] <= rgba[0][0];
      k[1] = pc[1] <= rgba[0][1];
      k[2] = pc[2] <= rgba[0][2];
      k[3] = pc[3] <= rgba[0][3];
      break;
   case PIPE_FUNC_GREATER:
      k[0] = pc[0] > rgba[0][0];
      k[1] = pc[1] > rgba[0][1];
      k[2] = pc[2] > rgba[0][2];
      k[3] = pc[3] > rgba[0][3];
      break;
   case PIPE_FUNC_GEQUAL:
      k[0] = pc[0] >= rgba[0][0];
      k[1] = pc[1] >= rgba[0][1];
      k[2] = pc[2] >= rgba[0][2];
      k[3] = pc[3] >= rgba[0][3];
      break;
   case PIPE_FUNC_EQUAL:
      k[0] = pc[0] == rgba[0][0];
      k[1] = pc[1] == rgba[0][1];
      k[2] = pc[2] == rgba[0][2];
      k[3] = pc[3] == rgba[0][3];
      break;
   case PIPE_FUNC_NOTEQUAL:
      k[0] = pc[0] != rgba[0][0];
      k[1] = pc[1] != rgba[0][1];
      k[2] = pc[2] != rgba[0][2];
      k[3] = pc[3] != rgba[0][3];
      break;
   case PIPE_FUNC_ALWAYS:
      k[0] = k[1] = k[2] = k[3] = 1;
      break;
   case PIPE_FUNC_NEVER:
      k[0] = k[1] = k[2] = k[3] = 0;
      break;
   default:
      k[0] = k[1] = k[2] = k[3] = 0;
      assert(0);
      break;
   }

   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      rgba[0][j] = k[j];
      rgba[1][j] = k[j];
      rgba[2][j] = k[j];
      rgba[3][j] = 1.0F;
   }
}


static void
do_swizzling(const struct pipe_sampler_view *sview,
             float in[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE],
             float out[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   int j;
   const unsigned swizzle_r = sview->swizzle_r;
   const unsigned swizzle_g = sview->swizzle_g;
   const unsigned swizzle_b = sview->swizzle_b;
   const unsigned swizzle_a = sview->swizzle_a;

   switch (swizzle_r) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[0][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[0][j] = 1.0f;
      break;
   default:
      assert(swizzle_r < 4);
      for (j = 0; j < 4; j++)
         out[0][j] = in[swizzle_r][j];
   }

   switch (swizzle_g) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[1][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[1][j] = 1.0f;
      break;
   default:
      assert(swizzle_g < 4);
      for (j = 0; j < 4; j++)
         out[1][j] = in[swizzle_g][j];
   }

   switch (swizzle_b) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[2][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[2][j] = 1.0f;
      break;
   default:
      assert(swizzle_b < 4);
      for (j = 0; j < 4; j++)
         out[2][j] = in[swizzle_b][j];
   }

   switch (swizzle_a) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[3][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[3][j] = 1.0f;
      break;
   default:
      assert(swizzle_a < 4);
      for (j = 0; j < 4; j++)
         out[3][j] = in[swizzle_a][j];
   }
}


static wrap_nearest_func
get_nearest_unorm_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_nearest_unorm_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_nearest_unorm_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_nearest_unorm_clamp_to_border;
   default:
      debug_printf("illegal wrap mode %d with non-normalized coords\n", mode);
      return wrap_nearest_unorm_clamp;
   }
}


static wrap_nearest_func
get_nearest_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
      return wrap_nearest_repeat;
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_nearest_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_nearest_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_nearest_clamp_to_border;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      return wrap_nearest_mirror_repeat;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      return wrap_nearest_mirror_clamp;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      return wrap_nearest_mirror_clamp_to_edge;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      return wrap_nearest_mirror_clamp_to_border;
   default:
      assert(0);
      return wrap_nearest_repeat;
   }
}


static wrap_linear_func
get_linear_unorm_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_linear_unorm_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_linear_unorm_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_linear_unorm_clamp_to_border;
   default:
      debug_printf("illegal wrap mode %d with non-normalized coords\n", mode);
      return wrap_linear_unorm_clamp;
   }
}


static wrap_linear_func
get_linear_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
      return wrap_linear_repeat;
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_linear_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_linear_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_linear_clamp_to_border;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      return wrap_linear_mirror_repeat;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      return wrap_linear_mirror_clamp;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      return wrap_linear_mirror_clamp_to_edge;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      return wrap_linear_mirror_clamp_to_border;
   default:
      assert(0);
      return wrap_linear_repeat;
   }
}


/**
 * Is swizzling needed for the given state key?
 */
static INLINE bool
any_swizzle(const struct pipe_sampler_view *view)
{
   return (view->swizzle_r != PIPE_SWIZZLE_RED ||
           view->swizzle_g != PIPE_SWIZZLE_GREEN ||
           view->swizzle_b != PIPE_SWIZZLE_BLUE ||
           view->swizzle_a != PIPE_SWIZZLE_ALPHA);
}


static img_filter_func
get_img_filter(const struct sp_sampler_view *sp_sview,
               const struct pipe_sampler_state *sampler,
               unsigned filter)
{
   switch (sp_sview->base.texture->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_1d_nearest;
      else
         return img_filter_1d_linear;
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_1d_array_nearest;
      else
         return img_filter_1d_array_linear;
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      /* Try for fast path:
       */
      if (sp_sview->pot2d &&
          sampler->wrap_s == sampler->wrap_t &&
          sampler->normalized_coords) 
      {
         switch (sampler->wrap_s) {
         case PIPE_TEX_WRAP_REPEAT:
            switch (filter) {
            case PIPE_TEX_FILTER_NEAREST:
               return img_filter_2d_nearest_repeat_POT;
            case PIPE_TEX_FILTER_LINEAR:
               return img_filter_2d_linear_repeat_POT;
            default:
               break;
            }
            break;
         case PIPE_TEX_WRAP_CLAMP:
            switch (filter) {
            case PIPE_TEX_FILTER_NEAREST:
               return img_filter_2d_nearest_clamp_POT;
            default:
               break;
            }
         }
      }
      /* Otherwise use default versions:
       */
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_2d_nearest;
      else
         return img_filter_2d_linear;
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_2d_array_nearest;
      else
         return img_filter_2d_array_linear;
      break;
   case PIPE_TEXTURE_CUBE:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_cube_nearest;
      else
         return img_filter_cube_linear;
      break;
   case PIPE_TEXTURE_CUBE_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_cube_array_nearest;
      else
         return img_filter_cube_array_linear;
      break;
   case PIPE_TEXTURE_3D:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_3d_nearest;
      else
         return img_filter_3d_linear;
      break;
   default:
      assert(0);
      return img_filter_1d_nearest;
   }
}


static void
sample_mip(struct sp_sampler_view *sp_sview,
           struct sp_sampler *sp_samp,
           const float s[TGSI_QUAD_SIZE],
           const float t[TGSI_QUAD_SIZE],
           const float p[TGSI_QUAD_SIZE],
           const float c0[TGSI_QUAD_SIZE],
           const float lod[TGSI_QUAD_SIZE],
           enum tgsi_sampler_control control,
           float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   mip_filter_func mip_filter;
   img_filter_func min_img_filter = NULL;
   img_filter_func mag_img_filter = NULL;

   if (sp_sview->pot2d & sp_samp->min_mag_equal_repeat_linear) {
      mip_filter = mip_filter_linear_2d_linear_repeat_POT;
   }
   else {
      mip_filter = sp_samp->mip_filter;
      min_img_filter = get_img_filter(sp_sview, &sp_samp->base, sp_samp->min_img_filter);
      if (sp_samp->min_mag_equal) {
         mag_img_filter = min_img_filter;
      }
      else {
         mag_img_filter = get_img_filter(sp_sview, &sp_samp->base, sp_samp->base.mag_img_filter);
      }
   }

   mip_filter(sp_sview, sp_samp, min_img_filter, mag_img_filter,
              s, t, p, c0, lod, control, rgba);

   if (sp_samp->base.compare_mode != PIPE_TEX_COMPARE_NONE) {
      sample_compare(sp_sview, sp_samp, s, t, p, c0, lod, control, rgba);
   }

   if (sp_sview->need_swizzle) {
      float rgba_temp[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
      memcpy(rgba_temp, rgba, sizeof(rgba_temp));
      do_swizzling(&sp_sview->base, rgba_temp, rgba);
   }

}


/**
 * Use 3D texcoords to choose a cube face, then sample the 2D cube faces.
 * Put face info into the sampler faces[] array.
 */
static void
sample_cube(struct sp_sampler_view *sp_sview,
            struct sp_sampler *sp_samp,
            const float s[TGSI_QUAD_SIZE],
            const float t[TGSI_QUAD_SIZE],
            const float p[TGSI_QUAD_SIZE],
            const float c0[TGSI_QUAD_SIZE],
            const float c1[TGSI_QUAD_SIZE],
            enum tgsi_sampler_control control,
            float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   unsigned j;
   float ssss[4], tttt[4];

   /* Not actually used, but the intermediate steps that do the
    * dereferencing don't know it.
    */
   static float pppp[4] = { 0, 0, 0, 0 };

   pppp[0] = c0[0];
   pppp[1] = c0[1];
   pppp[2] = c0[2];
   pppp[3] = c0[3];
   /*
     major axis
     direction    target                             sc     tc    ma
     ----------   -------------------------------    ---    ---   ---
     +rx          TEXTURE_CUBE_MAP_POSITIVE_X_EXT    -rz    -ry   rx
     -rx          TEXTURE_CUBE_MAP_NEGATIVE_X_EXT    +rz    -ry   rx
     +ry          TEXTURE_CUBE_MAP_POSITIVE_Y_EXT    +rx    +rz   ry
     -ry          TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT    +rx    -rz   ry
     +rz          TEXTURE_CUBE_MAP_POSITIVE_Z_EXT    +rx    -ry   rz
     -rz          TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT    -rx    -ry   rz
   */

   /* Choose the cube face and compute new s/t coords for the 2D face.
    *
    * Use the same cube face for all four pixels in the quad.
    *
    * This isn't ideal, but if we want to use a different cube face
    * per pixel in the quad, we'd have to also compute the per-face
    * LOD here too.  That's because the four post-face-selection
    * texcoords are no longer related to each other (they're
    * per-face!)  so we can't use subtraction to compute the partial
    * deriviates to compute the LOD.  Doing so (near cube edges
    * anyway) gives us pretty much random values.
    */
   {
      /* use the average of the four pixel's texcoords to choose the face */
      const float rx = 0.25F * (s[0] + s[1] + s[2] + s[3]);
      const float ry = 0.25F * (t[0] + t[1] + t[2] + t[3]);
      const float rz = 0.25F * (p[0] + p[1] + p[2] + p[3]);
      const float arx = fabsf(rx), ary = fabsf(ry), arz = fabsf(rz);

      if (arx >= ary && arx >= arz) {
         float sign = (rx >= 0.0F) ? 1.0F : -1.0F;
         uint face = (rx >= 0.0F) ? PIPE_TEX_FACE_POS_X : PIPE_TEX_FACE_NEG_X;
         for (j = 0; j < TGSI_QUAD_SIZE; j++) {
            const float ima = -0.5F / fabsf(s[j]);
            ssss[j] = sign *  p[j] * ima + 0.5F;
            tttt[j] =         t[j] * ima + 0.5F;
            sp_sview->faces[j] = face;
         }
      }
      else if (ary >= arx && ary >= arz) {
         float sign = (ry >= 0.0F) ? 1.0F : -1.0F;
         uint face = (ry >= 0.0F) ? PIPE_TEX_FACE_POS_Y : PIPE_TEX_FACE_NEG_Y;
         for (j = 0; j < TGSI_QUAD_SIZE; j++) {
            const float ima = -0.5F / fabsf(t[j]);
            ssss[j] =        -s[j] * ima + 0.5F;
            tttt[j] = sign * -p[j] * ima + 0.5F;
            sp_sview->faces[j] = face;
         }
      }
      else {
         float sign = (rz >= 0.0F) ? 1.0F : -1.0F;
         uint face = (rz >= 0.0F) ? PIPE_TEX_FACE_POS_Z : PIPE_TEX_FACE_NEG_Z;
         for (j = 0; j < TGSI_QUAD_SIZE; j++) {
            const float ima = -0.5F / fabsf(p[j]);
            ssss[j] = sign * -s[j] * ima + 0.5F;
            tttt[j] =         t[j] * ima + 0.5F;
            sp_sview->faces[j] = face;
         }
      }
   }

   sample_mip(sp_sview, sp_samp, ssss, tttt, pppp, c0, c1, control, rgba);
}


static void
sp_get_dims(struct sp_sampler_view *sp_sview, int level,
            int dims[4])
{
   const struct pipe_sampler_view *view = &sp_sview->base;
   const struct pipe_resource *texture = view->texture;

   if (texture->target == PIPE_BUFFER) {
      dims[0] = (view->u.buf.last_element - view->u.buf.first_element) + 1;
      /* the other values are undefined, but let's avoid potential valgrind
       * warnings.
       */
      dims[1] = dims[2] = dims[3] = 0;
      return;
   }

   /* undefined according to EXT_gpu_program */
   level += view->u.tex.first_level;
   if (level > view->u.tex.last_level)
      return;

   dims[3] = view->u.tex.last_level - view->u.tex.first_level + 1;
   dims[0] = u_minify(texture->width0, level);

   switch(texture->target) {
   case PIPE_TEXTURE_1D_ARRAY:
      dims[1] = view->u.tex.last_layer - view->u.tex.first_layer + 1;
      /* fallthrough */
   case PIPE_TEXTURE_1D:
      return;
   case PIPE_TEXTURE_2D_ARRAY:
      dims[2] = view->u.tex.last_layer - view->u.tex.first_layer + 1;
      /* fallthrough */
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
      dims[1] = u_minify(texture->height0, level);
      return;
   case PIPE_TEXTURE_3D:
      dims[1] = u_minify(texture->height0, level);
      dims[2] = u_minify(texture->depth0, level);
      return;
   case PIPE_TEXTURE_CUBE_ARRAY:
      dims[1] = u_minify(texture->height0, level);
      dims[2] = (view->u.tex.last_layer - view->u.tex.first_layer + 1) / 6;
      break;
   default:
      assert(!"unexpected texture target in sp_get_dims()");
      return;
   }
}

/**
 * This function is only used for getting unfiltered texels via the
 * TXF opcode.  The GL spec says that out-of-bounds texel fetches
 * produce undefined results.  Instead of crashing, lets just clamp
 * coords to the texture image size.
 */
static void
sp_get_texels(struct sp_sampler_view *sp_sview,
              const int v_i[TGSI_QUAD_SIZE],
              const int v_j[TGSI_QUAD_SIZE],
              const int v_k[TGSI_QUAD_SIZE],
              const int lod[TGSI_QUAD_SIZE],
              const int8_t offset[3],
              float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   union tex_tile_address addr;
   const struct pipe_resource *texture = sp_sview->base.texture;
   int j, c;
   const float *tx;
   int width, height, depth;

   addr.value = 0;
   /* TODO write a better test for LOD */
   addr.bits.level = lod[0];

   width = u_minify(texture->width0, addr.bits.level);
   height = u_minify(texture->height0, addr.bits.level);
   depth = u_minify(texture->depth0, addr.bits.level);

   switch(texture->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
         int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
         tx = get_texel_2d_no_border(sp_sview, addr, x, 0);
         for (c = 0; c < 4; c++) {
            rgba[c][j] = tx[c];
         }
      }
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
         int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
         int y = CLAMP(v_j[j], sp_sview->base.u.tex.first_layer, sp_sview->base.u.tex.last_layer);
         tx = get_texel_2d_no_border(sp_sview, addr, x, y);
         for (c = 0; c < 4; c++) {
            rgba[c][j] = tx[c];
         }
      }
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
         int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
         int y = CLAMP(v_j[j] + offset[1], 0, height - 1);
         tx = get_texel_2d_no_border(sp_sview, addr, x, y);
         for (c = 0; c < 4; c++) {
            rgba[c][j] = tx[c];
         }
      }
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
         int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
         int y = CLAMP(v_j[j] + offset[1], 0, height - 1);
         int layer = CLAMP(v_k[j], sp_sview->base.u.tex.first_layer, sp_sview->base.u.tex.last_layer);
         tx = get_texel_3d_no_border(sp_sview, addr, x, y, layer);
         for (c = 0; c < 4; c++) {
            rgba[c][j] = tx[c];
         }
      }
      break;
   case PIPE_TEXTURE_3D:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
         int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
         int y = CLAMP(v_j[j] + offset[1], 0, height - 1);
         int z = CLAMP(v_k[j] + offset[2], 0, depth - 1);
         tx = get_texel_3d_no_border(sp_sview, addr, x, y, z);
         for (c = 0; c < 4; c++) {
            rgba[c][j] = tx[c];
         }
      }
      break;
   case PIPE_TEXTURE_CUBE: /* TXF can't work on CUBE according to spec */
   default:
      assert(!"Unknown or CUBE texture type in TXF processing\n");
      break;
   }

   if (sp_sview->need_swizzle) {
      float rgba_temp[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
      memcpy(rgba_temp, rgba, sizeof(rgba_temp));
      do_swizzling(&sp_sview->base, rgba_temp, rgba);
   }
}


void *
softpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct sp_sampler *samp = CALLOC_STRUCT(sp_sampler);

   samp->base = *sampler;

   /* Note that (for instance) linear_texcoord_s and
    * nearest_texcoord_s may be active at the same time, if the
    * sampler min_img_filter differs from its mag_img_filter.
    */
   if (sampler->normalized_coords) {
      samp->linear_texcoord_s = get_linear_wrap( sampler->wrap_s );
      samp->linear_texcoord_t = get_linear_wrap( sampler->wrap_t );
      samp->linear_texcoord_p = get_linear_wrap( sampler->wrap_r );

      samp->nearest_texcoord_s = get_nearest_wrap( sampler->wrap_s );
      samp->nearest_texcoord_t = get_nearest_wrap( sampler->wrap_t );
      samp->nearest_texcoord_p = get_nearest_wrap( sampler->wrap_r );
   }
   else {
      samp->linear_texcoord_s = get_linear_unorm_wrap( sampler->wrap_s );
      samp->linear_texcoord_t = get_linear_unorm_wrap( sampler->wrap_t );
      samp->linear_texcoord_p = get_linear_unorm_wrap( sampler->wrap_r );

      samp->nearest_texcoord_s = get_nearest_unorm_wrap( sampler->wrap_s );
      samp->nearest_texcoord_t = get_nearest_unorm_wrap( sampler->wrap_t );
      samp->nearest_texcoord_p = get_nearest_unorm_wrap( sampler->wrap_r );
   }

   samp->min_img_filter = sampler->min_img_filter;

   switch (sampler->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NONE:
      if (sampler->min_img_filter == sampler->mag_img_filter)
         samp->mip_filter = mip_filter_none_no_filter_select;
      else
         samp->mip_filter = mip_filter_none;
      break;

   case PIPE_TEX_MIPFILTER_NEAREST:
      samp->mip_filter = mip_filter_nearest;
      break;

   case PIPE_TEX_MIPFILTER_LINEAR:
      if (sampler->min_img_filter == sampler->mag_img_filter &&
          sampler->normalized_coords &&
          sampler->wrap_s == PIPE_TEX_WRAP_REPEAT &&
          sampler->wrap_t == PIPE_TEX_WRAP_REPEAT &&
          sampler->min_img_filter == PIPE_TEX_FILTER_LINEAR &&
          sampler->max_anisotropy <= 1) {
         samp->min_mag_equal_repeat_linear = TRUE;
      }
      samp->mip_filter = mip_filter_linear;

      /* Anisotropic filtering extension. */
      if (sampler->max_anisotropy > 1) {
         samp->mip_filter = mip_filter_linear_aniso;

         /* Override min_img_filter:
          * min_img_filter needs to be set to NEAREST since we need to access
          * each texture pixel as it is and weight it later; using linear
          * filters will have incorrect results.
          * By setting the filter to NEAREST here, we can avoid calling the
          * generic img_filter_2d_nearest in the anisotropic filter function,
          * making it possible to use one of the accelerated implementations
          */
         samp->min_img_filter = PIPE_TEX_FILTER_NEAREST;

         /* on first access create the lookup table containing the filter weights. */
        if (!weightLut) {
           create_filter_table();
        }
      }
      break;
   }
   if (samp->min_img_filter == sampler->mag_img_filter) {
      samp->min_mag_equal = TRUE;
   }

   return (void *)samp;
}


compute_lambda_func
softpipe_get_lambda_func(const struct pipe_sampler_view *view, unsigned shader)
{
   if (shader != PIPE_SHADER_FRAGMENT)
      return compute_lambda_vert;

   switch (view->texture->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return compute_lambda_1d;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      return compute_lambda_2d;
   case PIPE_TEXTURE_3D:
      return compute_lambda_3d;
   default:
      assert(0);
      return compute_lambda_1d;
   }
}


struct pipe_sampler_view *
softpipe_create_sampler_view(struct pipe_context *pipe,
                             struct pipe_resource *resource,
                             const struct pipe_sampler_view *templ)
{
   struct sp_sampler_view *sview = CALLOC_STRUCT(sp_sampler_view);
   struct softpipe_resource *spr = (struct softpipe_resource *)resource;

   if (sview) {
      struct pipe_sampler_view *view = &sview->base;
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_resource_reference(&view->texture, resource);
      view->context = pipe;

      if (any_swizzle(view)) {
         sview->need_swizzle = TRUE;
      }

      if (resource->target == PIPE_TEXTURE_CUBE ||
          resource->target == PIPE_TEXTURE_CUBE_ARRAY)
         sview->get_samples = sample_cube;
      else {
         sview->get_samples = sample_mip;
      }
      sview->pot2d = spr->pot &&
                     (resource->target == PIPE_TEXTURE_2D ||
                      resource->target == PIPE_TEXTURE_RECT);

      sview->xpot = util_logbase2( resource->width0 );
      sview->ypot = util_logbase2( resource->height0 );
   }

   return (struct pipe_sampler_view *) sview;
}


static void
sp_tgsi_get_dims(struct tgsi_sampler *tgsi_sampler,
                 const unsigned sview_index,
                 int level, int dims[4])
{
   struct sp_tgsi_sampler *sp_samp = (struct sp_tgsi_sampler *)tgsi_sampler;

   assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   /* always have a view here but texture is NULL if no sampler view was set. */
   if (!sp_samp->sp_sview[sview_index].base.texture) {
      dims[0] = dims[1] = dims[2] = dims[3] = 0;
      return;
   }
   sp_get_dims(&sp_samp->sp_sview[sview_index], level, dims);
}


static void
sp_tgsi_get_samples(struct tgsi_sampler *tgsi_sampler,
                    const unsigned sview_index,
                    const unsigned sampler_index,
                    const float s[TGSI_QUAD_SIZE],
                    const float t[TGSI_QUAD_SIZE],
                    const float p[TGSI_QUAD_SIZE],
                    const float c0[TGSI_QUAD_SIZE],
                    const float lod[TGSI_QUAD_SIZE],
                    float derivs[3][2][TGSI_QUAD_SIZE],
                    const int8_t offset[3],
                    enum tgsi_sampler_control control,
                    float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   struct sp_tgsi_sampler *sp_samp = (struct sp_tgsi_sampler *)tgsi_sampler;

   assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   assert(sampler_index < PIPE_MAX_SAMPLERS);
   assert(sp_samp->sp_sampler[sampler_index]);
   /* always have a view here but texture is NULL if no sampler view was set. */
   if (!sp_samp->sp_sview[sview_index].base.texture) {
      int i, j;
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
         for (i = 0; i < TGSI_QUAD_SIZE; i++) {
            rgba[j][i] = 0.0f;
         }
      }
      return;
   }
   sp_samp->sp_sview[sview_index].get_samples(&sp_samp->sp_sview[sview_index],
                                              sp_samp->sp_sampler[sampler_index],
                                              s, t, p, c0, lod, control, rgba);
}


static void
sp_tgsi_get_texel(struct tgsi_sampler *tgsi_sampler,
                  const unsigned sview_index,
                  const int i[TGSI_QUAD_SIZE],
                  const int j[TGSI_QUAD_SIZE], const int k[TGSI_QUAD_SIZE],
                  const int lod[TGSI_QUAD_SIZE], const int8_t offset[3],
                  float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
{
   struct sp_tgsi_sampler *sp_samp = (struct sp_tgsi_sampler *)tgsi_sampler;

   assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   /* always have a view here but texture is NULL if no sampler view was set. */
   if (!sp_samp->sp_sview[sview_index].base.texture) {
      int i, j;
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
         for (i = 0; i < TGSI_QUAD_SIZE; i++) {
            rgba[j][i] = 0.0f;
         }
      }
      return;
   }
   sp_get_texels(&sp_samp->sp_sview[sview_index], i, j, k, lod, offset, rgba);
}


struct sp_tgsi_sampler *
sp_create_tgsi_sampler(void)
{
   struct sp_tgsi_sampler *samp = CALLOC_STRUCT(sp_tgsi_sampler);
   if (!samp)
      return NULL;

   samp->base.get_dims = sp_tgsi_get_dims;
   samp->base.get_samples = sp_tgsi_get_samples;
   samp->base.get_texel = sp_tgsi_get_texel;

   return samp;
}

