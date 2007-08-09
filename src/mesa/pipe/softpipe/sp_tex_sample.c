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


#include "main/macros.h"
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_tex_sample.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/tgsi/core/tgsi_exec.h"


/*
 * Note, the FRAC macro has to work perfectly.  Otherwise you'll sometimes
 * see 1-pixel bands of improperly weighted linear-filtered textures.
 * The tests/texwrap.c demo is a good test.
 * Also note, FRAC(x) doesn't truly return the fractional part of x for x < 0.
 * Instead, if x < 0 then FRAC(x) = 1 - true_frac(x).
 */
#define FRAC(f)  ((f) - IFLOOR(f))


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
static INLINE GLfloat
lerp_2d(GLfloat a, GLfloat b,
        GLfloat v00, GLfloat v10, GLfloat v01, GLfloat v11)
{
   const GLfloat temp0 = LERP(a, v00, v10);
   const GLfloat temp1 = LERP(a, v01, v11);
   return LERP(b, temp0, temp1);
}


/**
 * Compute the remainder of a divided by b, but be careful with
 * negative values so that REPEAT mode works right.
 */
static INLINE GLint
repeat_remainder(GLint a, GLint b)
{
   if (a >= 0)
      return a % b;
   else
      return (a + 1) % b + b - 1;
}


/**
 * Apply texture coord wrapping mode and return integer texture index.
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the texcoord
 * \param size  the texture image size
 * \return  integer texture index
 */
static INLINE GLint
nearest_texcoord(GLuint wrapMode, GLfloat s, GLuint size)
{
   GLint i;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      /* s limited to [0,1) */
      /* i limited to [0,size-1] */
      i = IFLOOR(s * size);
      i = repeat_remainder(i, size);
      return i;
   case PIPE_TEX_WRAP_CLAMP:
      /* s limited to [0,1] */
      /* i limited to [0,size-1] */
      if (s <= 0.0F)
         i = 0;
      else if (s >= 1.0F)
         i = size - 1;
      else
         i = IFLOOR(s * size);
      return i;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const GLfloat min = 1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         if (s < min)
            i = 0;
         else if (s > max)
            i = size - 1;
         else
            i = IFLOOR(s * size);
      }
      return i;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [-1, size] */
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         if (s <= min)
            i = -1;
         else if (s >= max)
            i = size;
         else
            i = IFLOOR(s * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      {
         const GLfloat min = 1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         const GLint flr = IFLOOR(s);
         GLfloat u;
         if (flr & 1)
            u = 1.0F - (s - (GLfloat) flr);
         else
            u = s - (GLfloat) flr;
         if (u < min)
            i = 0;
         else if (u > max)
            i = size - 1;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      {
         /* s limited to [0,1] */
         /* i limited to [0,size-1] */
         const GLfloat u = FABSF(s);
         if (u <= 0.0F)
            i = 0;
         else if (u >= 1.0F)
            i = size - 1;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const GLfloat min = 1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         const GLfloat u = FABSF(s);
         if (u < min)
            i = 0;
         else if (u > max)
            i = size - 1;
         else
            i = IFLOOR(u * size);
      }
      return i;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         /* s limited to [min,max] */
         /* i limited to [0, size-1] */
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         const GLfloat u = FABSF(s);
         if (u < min)
            i = -1;
         else if (u > max)
            i = size;
         else
            i = IFLOOR(u * size);
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
linear_texcoord(GLuint wrapMode, GLfloat s, GLuint size,
                GLint *i0, GLint *i1, GLfloat *a)
{
   GLfloat u;
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      u = s * size - 0.5F;
      *i0 = repeat_remainder(IFLOOR(u), size);
      *i1 = repeat_remainder(*i0 + 1, size);
      break;
   case PIPE_TEX_WRAP_CLAMP:
      if (s <= 0.0F)
         u = 0.0F;
      else if (s >= 1.0F)
         u = (GLfloat) size;
      else
         u = s * size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (s <= 0.0F)
         u = 0.0F;
      else if (s >= 1.0F)
         u = (GLfloat) size;
      else
         u = s * size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      if (*i0 < 0)
         *i0 = 0;
      if (*i1 >= (GLint) size)
         *i1 = size - 1;
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         if (s <= min)
            u = min * size;
         else if (s >= max)
            u = max * size;
         else
            u = s * size;
         u -= 0.5F;
         *i0 = IFLOOR(u);
         *i1 = *i0 + 1;
      }
      break;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      {
         const GLint flr = IFLOOR(s);
         if (flr & 1)
            u = 1.0F - (s - (GLfloat) flr);
         else
            u = s - (GLfloat) flr;
         u = (u * size) - 0.5F;
         *i0 = IFLOOR(u);
         *i1 = *i0 + 1;
         if (*i0 < 0)
            *i0 = 0;
         if (*i1 >= (GLint) size)
            *i1 = size - 1;
      }
      break;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      u = FABSF(s);
      if (u >= 1.0F)
         u = (GLfloat) size;
      else
         u *= size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      break;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      u = FABSF(s);
      if (u >= 1.0F)
         u = (GLfloat) size;
      else
         u *= size;
      u -= 0.5F;
      *i0 = IFLOOR(u);
      *i1 = *i0 + 1;
      if (*i0 < 0)
         *i0 = 0;
      if (*i1 >= (GLint) size)
         *i1 = size - 1;
      break;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         const GLfloat min = -1.0F / (2.0F * size);
         const GLfloat max = 1.0F - min;
         u = FABSF(s);
         if (u <= min)
            u = min * size;
         else if (u >= max)
            u = max * size;
         else
            u *= size;
         u -= 0.5F;
         *i0 = IFLOOR(u);
         *i1 = *i0 + 1;
      }
      break;
   default:
      assert(0);
   }
   *a = FRAC(u);
}


static GLuint
choose_cube_face(const GLfloat texcoord[4], GLfloat newCoord[4])
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
   const GLfloat rx = texcoord[0];
   const GLfloat ry = texcoord[1];
   const GLfloat rz = texcoord[2];
   const GLfloat arx = FABSF(rx), ary = FABSF(ry), arz = FABSF(rz);
   GLuint face;
   GLfloat sc, tc, ma;

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

   newCoord[0] = ( sc / ma + 1.0F ) * 0.5F;
   newCoord[1] = ( tc / ma + 1.0F ) * 0.5F;

   return face;
}


static void
sp_get_sample_1d(struct tgsi_sampler *sampler,
                 const GLfloat strq[4], GLfloat lambda, GLfloat rgba[4])
{
   struct pipe_context *pipe = (struct pipe_context *) sampler->pipe;
   struct pipe_surface *ps
      = pipe->get_tex_surface(pipe, sampler->texture, 0, 0, 0);

   switch (sampler->state->min_img_filter) {
   case PIPE_TEX_FILTER_NEAREST:
      {
         GLint x;
         x = nearest_texcoord(sampler->state->wrap_s, strq[0],
                              sampler->texture->width0);
         ps->get_tile(ps, x, 0, 1, 1, rgba);
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
      {
         GLfloat t0[4], t1[4];
         GLint x0, x1;
         GLfloat a;
         linear_texcoord(sampler->state->wrap_s, strq[0],
                         sampler->texture->width0, &x0, &x1, &a);
         ps->get_tile(ps, x0, 0, 1, 1, t0);
         ps->get_tile(ps, x1, 0, 1, 1, t1);

         rgba[0] = LERP(a, t0[0], t1[0]);
         rgba[1] = LERP(a, t0[1], t1[1]);
         rgba[2] = LERP(a, t0[2], t1[2]);
         rgba[3] = LERP(a, t0[3], t1[3]);
      }
      break;
   default:
      assert(0);
   }
}

static GLuint
choose_mipmap_level(struct tgsi_sampler *sampler, GLfloat lambda)
{
   if (sampler->state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
      return 0;
   }
   else {
      GLint level = (int) lambda;
      level = CLAMP(level, sampler->texture->first_level,
		    sampler->texture->last_level);
      return level;
   }
}


/**
 * Called via tgsi_sampler::get_sample()
 * Use the sampler's state setting to get a filtered RGBA value
 * from the sampler's texture (mipmap tree).
 *
 * XXX we can implement many versions of this function, each
 * tightly coded for a specific combination of sampler state
 * (nearest + repeat), (bilinear mipmap + clamp), etc.
 *
 * The update_samplers() function in st_atom_sampler.c could create
 * a new tgsi_sampler object for each state combo it finds....
 */
static void
sp_get_sample_2d(struct tgsi_sampler *sampler,
                 const GLfloat strq[4], GLfloat lambda, GLfloat rgba[4])
{
   struct pipe_context *pipe = (struct pipe_context *) sampler->pipe;
   GLuint filter;
   GLint level0;

   if (lambda < 0.0)
      filter = sampler->state->mag_img_filter;
   else
      filter = sampler->state->min_img_filter;

   level0 = choose_mipmap_level(sampler, lambda);

   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
      {
         GLint x = nearest_texcoord(sampler->state->wrap_s, strq[0],
                                    sampler->texture->level[level0].width);
         GLint y = nearest_texcoord(sampler->state->wrap_t, strq[1],
                                    sampler->texture->level[level0].height);
         GLint cx = x / SAMPLER_CACHE_SIZE;
         GLint cy = y / SAMPLER_CACHE_SIZE;
         if (cx != sampler->cache_x || cy != sampler->cache_y ||
             level0 != sampler->cache_level) {
            /* cache miss, replace cache with new tile */
            struct pipe_surface *ps
               = pipe->get_tex_surface(pipe, sampler->texture, 0, level0, 0);
            assert(ps->width == sampler->texture->level[level0].width);
            assert(ps->height == sampler->texture->level[level0].height);
            sampler->cache_level = level0;
            sampler->cache_x = cx;
            sampler->cache_y = cy;
            ps->get_tile(ps,
                         cx * SAMPLER_CACHE_SIZE,
                         cy * SAMPLER_CACHE_SIZE,
                         SAMPLER_CACHE_SIZE, SAMPLER_CACHE_SIZE,
                         (GLfloat *) sampler->cache);
            /*printf("cache miss (%d, %d)\n", x, y);*/
         }
         else {
            /*printf("cache hit (%d, %d)\n", x, y);*/
         }
         /* get texel from cache */
         cx = x % SAMPLER_CACHE_SIZE;
         cy = y % SAMPLER_CACHE_SIZE;
         COPY_4V(rgba, sampler->cache[cy][cx]);
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
      {
         GLfloat t00[4], t01[4], t10[4], t11[4];
         GLint x0, y0, x1, y1;
         GLfloat a, b;
         struct pipe_surface *ps
            = pipe->get_tex_surface(pipe, sampler->texture, 0, level0, 0);

         linear_texcoord(sampler->state->wrap_s, strq[0],
                         sampler->texture->width0, &x0, &x1, &a);
         linear_texcoord(sampler->state->wrap_t, strq[1],
                         sampler->texture->height0, &y0, &y1, &b);
         ps->get_tile(ps, x0, y0, 1, 1, t00);
         ps->get_tile(ps, x1, y0, 1, 1, t10);
         ps->get_tile(ps, x0, y1, 1, 1, t01);
         ps->get_tile(ps, x1, y1, 1, 1, t11);

         rgba[0] = lerp_2d(a, b, t00[0], t10[0], t01[0], t11[0]);
         rgba[1] = lerp_2d(a, b, t00[1], t10[1], t01[1], t11[1]);
         rgba[2] = lerp_2d(a, b, t00[2], t10[2], t01[2], t11[2]);
         rgba[3] = lerp_2d(a, b, t00[3], t10[3], t01[3], t11[3]);
      }
      break;
      /*
      {
	 GLuint level0, level1;
	 level0 = choose_mipmap_level(sampler, lambda);
      }
      break;
      */
   default:
      assert(0);
   }
}


static void
sp_get_sample_3d(struct tgsi_sampler *sampler,
                 const GLfloat strq[4], GLfloat lamba, GLfloat rgba[4])
{
   /* get/map pipe_surfaces corresponding to 3D tex slices */
}


static void
sp_get_sample_cube(struct tgsi_sampler *sampler,
                   const GLfloat strq[4], GLfloat lambda, GLfloat rgba[4])
{
   GLfloat st[4];
   GLuint face = choose_cube_face(strq, st);

   /* get/map surface corresponding to the face */
}


void
sp_get_sample(struct tgsi_sampler *sampler,
              const GLfloat strq[4], GLfloat lambda, GLfloat rgba[4])
{
   switch (sampler->texture->target) {
   case GL_TEXTURE_1D:
      sp_get_sample_1d(sampler, strq, lambda, rgba);
      break;
   case GL_TEXTURE_2D:
      sp_get_sample_2d(sampler, strq, lambda, rgba);
      break;
   case GL_TEXTURE_3D:
      sp_get_sample_3d(sampler, strq, lambda, rgba);
      break;
   case GL_TEXTURE_CUBE_MAP:
      sp_get_sample_cube(sampler, strq, lambda, rgba);
      break;
   default:
      assert(0);
   }
}

