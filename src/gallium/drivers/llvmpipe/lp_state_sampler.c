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

#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "draw/draw_context.h"

#include "lp_context.h"
#include "lp_context.h"
#include "lp_state.h"
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
llvmpipe_bind_vertex_sampler_states(struct pipe_context *pipe,
                                    unsigned num_samplers,
                                    void **samplers)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned i;

   assert(num_samplers <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num_samplers == llvmpipe->num_vertex_samplers &&
       !memcmp(llvmpipe->vertex_samplers, samplers, num_samplers * sizeof(void *)))
      return;

   draw_flush(llvmpipe->draw);

   for (i = 0; i < num_samplers; ++i)
      llvmpipe->vertex_samplers[i] = samplers[i];
   for (i = num_samplers; i < PIPE_MAX_VERTEX_SAMPLERS; ++i)
      llvmpipe->vertex_samplers[i] = NULL;

   llvmpipe->num_vertex_samplers = num_samplers;

   llvmpipe->dirty |= LP_NEW_SAMPLER;
}


void
llvmpipe_set_fragment_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == llvmpipe->num_fragment_sampler_views &&
       !memcmp(llvmpipe->fragment_sampler_views, views, num * sizeof(struct pipe_sampler_view *)))
      return;

   draw_flush(llvmpipe->draw);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&llvmpipe->fragment_sampler_views[i], view);
   }

   llvmpipe->num_fragment_sampler_views = num;

   llvmpipe->dirty |= LP_NEW_SAMPLER_VIEW;
}


void
llvmpipe_set_vertex_sampler_views(struct pipe_context *pipe,
                                  unsigned num,
                                  struct pipe_sampler_view **views)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num == llvmpipe->num_vertex_sampler_views &&
       !memcmp(llvmpipe->vertex_sampler_views, views, num * sizeof(struct pipe_sampler_view *))) {
      return;
   }

   draw_flush(llvmpipe->draw);

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&llvmpipe->vertex_sampler_views[i], view);
   }

   llvmpipe->num_vertex_sampler_views = num;

   llvmpipe->dirty |= LP_NEW_SAMPLER_VIEW;
}


struct pipe_sampler_view *
llvmpipe_create_sampler_view(struct pipe_context *pipe,
                            struct pipe_texture *texture,
                            const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

   if (view) {
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_texture_reference(&view->texture, texture);
      view->context = pipe;
   }

   return view;
}


void
llvmpipe_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   pipe_texture_reference(&view->texture, NULL);
   FREE(view);
}


void
llvmpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}



