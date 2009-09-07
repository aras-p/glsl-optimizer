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

/* Authors:
 *  Brian Paul
 */

#include "util/u_memory.h"

#include "draw/draw_context.h"

#include "lp_context.h"
#include "lp_context.h"
#include "lp_state.h"
#include "lp_texture.h"
#include "lp_tex_cache.h"
#include "draw/draw_context.h"



void *
llvmpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   return mem_dup(sampler, sizeof(*sampler));
}


void
llvmpipe_bind_sampler_states(struct pipe_context *pipe,
                             unsigned num, void **sampler)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == llvmpipe->num_samplers &&
       !memcmp(llvmpipe->sampler, sampler, num * sizeof(void *)))
      return;

   draw_flush(llvmpipe->draw);

   for (i = 0; i < num; ++i)
      llvmpipe->sampler[i] = sampler[i];
   for (i = num; i < PIPE_MAX_SAMPLERS; ++i)
      llvmpipe->sampler[i] = NULL;

   llvmpipe->num_samplers = num;

   llvmpipe->dirty |= LP_NEW_SAMPLER;
}


void
llvmpipe_set_sampler_textures(struct pipe_context *pipe,
                              unsigned num, struct pipe_texture **texture)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == llvmpipe->num_textures &&
       !memcmp(llvmpipe->texture, texture, num * sizeof(struct pipe_texture *)))
      return;

   draw_flush(llvmpipe->draw);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_texture *tex = i < num ? texture[i] : NULL;

      pipe_texture_reference(&llvmpipe->texture[i], tex);
      lp_tex_tile_cache_set_texture(llvmpipe->tex_cache[i], tex);

      if(tex) {
         struct llvmpipe_texture *lp_tex = llvmpipe_texture(tex);
         struct lp_jit_texture *jit_tex = &llvmpipe->jit_context.textures[i];
         jit_tex->width = tex->width[0];
         jit_tex->height = tex->height[0];
         jit_tex->stride = lp_tex->stride[0];
         if(!lp_tex->dt)
            jit_tex->data = lp_tex->data;
      }
   }

   llvmpipe->num_textures = num;

   llvmpipe->dirty |= LP_NEW_TEXTURE;
}


void
llvmpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}



