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
#include "pipe/p_inlines.h"

#include "draw/draw_context.h"

#include "sp_context.h"
#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tile_cache.h"
#include "draw/draw_context.h"



void *
softpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   return mem_dup(sampler, sizeof(*sampler));
}


void
softpipe_bind_sampler_states(struct pipe_context *pipe,
                             unsigned num, void **sampler)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_samplers &&
       !memcmp(softpipe->sampler, sampler, num * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num; ++i)
      softpipe->sampler[i] = sampler[i];
   for (i = num; i < PIPE_MAX_SAMPLERS; ++i)
      softpipe->sampler[i] = NULL;

   softpipe->num_samplers = num;

   softpipe->dirty |= SP_NEW_SAMPLER;
}


void
softpipe_set_sampler_textures(struct pipe_context *pipe,
                              unsigned num, struct pipe_texture **texture)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_textures &&
       !memcmp(softpipe->texture, texture, num * sizeof(struct pipe_texture *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_texture *tex = i < num ? texture[i] : NULL;

      pipe_texture_reference(&softpipe->texture[i], tex);
      sp_tile_cache_set_texture(pipe, softpipe->tex_cache[i], tex);
   }

   softpipe->num_textures = num;

   softpipe->dirty |= SP_NEW_TEXTURE;
}


void
softpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}



