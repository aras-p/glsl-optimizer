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

 /*
  * Authors:
  *   Zack Rusin <zack@tungstengraphics.com>
  */

#include "st_cache.h"

#include "st_context.h"

#include "pipe/p_state.h"

#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"

/* This function will either find the state of the given template
 * in the cache or it will create a new state state from the given
 * template, will insert it in the cache and return it.
 */
struct pipe_blend_state * st_cached_blend_state(
   struct st_context *st,
   const struct pipe_blend_state *blend)
{
   unsigned hash_key = cso_construct_key((void*)blend, sizeof(struct pipe_blend_state));
   struct cso_hash_iter iter = cso_find_state_template(st->cache,
                                                       hash_key, CSO_BLEND,
                                                       (void*)blend);
   if (cso_hash_iter_is_null(iter)) {
      const struct pipe_blend_state *created_state = st->pipe->create_blend_state(
         st->pipe, blend);
      iter = cso_insert_state(st->cache, hash_key, CSO_BLEND,
                              (void*)created_state);
   }
   return (struct pipe_blend_state*)(cso_hash_iter_data(iter));
}

struct pipe_sampler_state * st_cached_sampler_state(
   struct st_context *st,
   const struct pipe_sampler_state *sampler)
{
   unsigned hash_key = cso_construct_key((void*)sampler, sizeof(struct pipe_sampler_state));
   struct cso_hash_iter iter = cso_find_state_template(st->cache,
                                                       hash_key, CSO_SAMPLER,
                                                       (void*)sampler);
   if (cso_hash_iter_is_null(iter)) {
      const struct pipe_sampler_state *created_state = st->pipe->create_sampler_state(
         st->pipe, sampler);
      iter = cso_insert_state(st->cache, hash_key, CSO_SAMPLER,
                              (void*)created_state);
   }
   return (struct pipe_sampler_state*)(cso_hash_iter_data(iter));
}

struct pipe_depth_stencil_state * st_cached_depth_stencil_state(
   struct st_context *st,
   const struct pipe_depth_stencil_state *depth_stencil)
{
   unsigned hash_key = cso_construct_key((void*)depth_stencil, sizeof(struct pipe_depth_stencil_state));
   struct cso_hash_iter iter = cso_find_state_template(st->cache,
                                                       hash_key, CSO_DEPTH_STENCIL,
                                                       (void*)depth_stencil);
   if (cso_hash_iter_is_null(iter)) {
      const struct pipe_depth_stencil_state *created_state = st->pipe->create_depth_stencil_state(
         st->pipe, depth_stencil);
      iter = cso_insert_state(st->cache, hash_key, CSO_DEPTH_STENCIL,
                              (void*)created_state);
   }
   return (struct pipe_depth_stencil_state*)(cso_hash_iter_data(iter));
}
