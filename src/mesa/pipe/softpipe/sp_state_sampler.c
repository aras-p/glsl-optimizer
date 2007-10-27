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

#include "sp_context.h"
#include "sp_state.h"
#include "sp_tile_cache.h"


void *
softpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct pipe_sampler_state *state = malloc(sizeof(struct pipe_sampler_state));
   memcpy(state, sampler, sizeof(struct pipe_sampler_state));
   return state;
}

void
softpipe_bind_sampler_state(struct pipe_context *pipe,
                            unsigned unit, void *sampler)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   softpipe->sampler[unit] = (struct pipe_sampler_state *)sampler;

   softpipe->dirty |= SP_NEW_SAMPLER;
}


void
softpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   free(sampler);
}


void
softpipe_set_texture_state(struct pipe_context *pipe,
                           unsigned unit,
                           struct pipe_mipmap_tree *texture)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   softpipe->texture[unit] = texture;  /* ptr, not struct */

   sp_tile_cache_set_texture(softpipe->tex_cache[unit], texture);

   softpipe->dirty |= SP_NEW_TEXTURE;
}


void
softpipe_set_sampler_units(struct pipe_context *pipe,
                           uint num_samplers, const uint *units )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;
   for (i = 0; i < num_samplers; i++)
      softpipe->sampler_units[i] = units[i];
   softpipe->dirty |= SP_NEW_SAMPLER;
}


