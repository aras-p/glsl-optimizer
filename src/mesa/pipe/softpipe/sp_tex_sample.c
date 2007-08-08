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
#include "sp_surface.h"
#include "sp_surface.h"
#include "sp_tex_sample.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/tgsi/core/tgsi_exec.h"


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
 * Apply texture coord wrapping mode and return integer texture
 * row/column/img index.
 */
static INLINE void
nearest_texcoord(GLuint wrapMode, GLfloat s, GLuint width, GLint *k)
{
   /* XXX this arithmetic isn't exactly right, add more wrap modes */
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      *k = (int) (s * width) % width;
      break;
   case PIPE_TEX_WRAP_CLAMP:
      *k = (int) CLAMP(s, 0.0, 1.0) * width;
      break;
   default:
      assert(0);
      *k = 0;
   }
}

static INLINE void
linear_texcoord(GLuint wrapMode, GLfloat s, GLuint width, GLint *k0, GLint *k1,
                GLfloat *a)
{
   /* XXX this arithmetic isn't exactly right, add more wrap modes */
   switch (wrapMode) {
   case PIPE_TEX_WRAP_REPEAT:
      *k0 = (int) (s * width) % width;
      *k1 = (*k0 + 1) % width;
      break;
   case PIPE_TEX_WRAP_CLAMP:
      *k0 = (int) CLAMP(s, 0.0, 1.0) * width;
      *k1 = (*k0 + 1);
      if (*k1 >= width)
         *k1 = width - 1;
      break;
   default:
      assert(0);
      *k0 = *k1 = 0;
   }
   /* kludge the lerp weight: */
   *a = (s * width) - (int) (s * width);
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
void
sp_get_sample(struct tgsi_sampler *sampler,
              const GLfloat strq[4], GLfloat rgba[4])
{
   struct pipe_context *pipe = (struct pipe_context *) sampler->pipe;
   struct pipe_surface *ps
      = pipe->get_tex_surface(pipe, sampler->texture, 0, 0, 0);

   switch (sampler->state->min_filter) {
   case PIPE_TEX_FILTER_NEAREST:
      {
         GLint x, y;
         nearest_texcoord(sampler->state->wrap_s, strq[0],
                          sampler->texture->width0, &x);
         nearest_texcoord(sampler->state->wrap_t, strq[1],
                          sampler->texture->height0, &y);
         ps->get_tile(ps, x, y, 1, 1, rgba);
      }
      break;
   case PIPE_TEX_FILTER_LINEAR:
      {
         GLfloat t00[4], t01[4], t10[4], t11[4];
         GLint x0, y0, x1, y1;
         GLfloat a, b;
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
   default:
      assert(0);
   }
}
