/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:
 *  Brian Paul
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "draw/draw_context.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"
#include "sp_tex_tile_cache.h"


/**
 * Bind a range [start, start+num-1] of samplers for a shader stage.
 */
static void
softpipe_bind_sampler_states(struct pipe_context *pipe,
                             unsigned shader,
                             unsigned start,
                             unsigned num,
                             void **samplers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(shader < PIPE_SHADER_TYPES);
   assert(start + num <= Elements(softpipe->samplers[shader]));

   draw_flush(softpipe->draw);

   /* set the new samplers */
   for (i = 0; i < num; i++) {
      softpipe->samplers[shader][start + i] = samplers[i];
   }

   /* find highest non-null samplers[] entry */
   {
      unsigned j = MAX2(softpipe->num_samplers[shader], start + num);
      while (j > 0 && softpipe->samplers[shader][j - 1] == NULL)
         j--;
      softpipe->num_samplers[shader] = j;
   }

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_samplers(softpipe->draw,
                        shader,
                        softpipe->samplers[shader],
                        softpipe->num_samplers[shader]);
   }

   softpipe->dirty |= SP_NEW_SAMPLER;
}


static void
softpipe_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}


void
softpipe_set_sampler_views(struct pipe_context *pipe,
                           unsigned shader,
                           unsigned start,
                           unsigned num,
                           struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(shader < PIPE_SHADER_TYPES);
   assert(start + num <= Elements(softpipe->sampler_views[shader]));

   draw_flush(softpipe->draw);

   /* set the new sampler views */
   for (i = 0; i < num; i++) {
      struct sp_sampler_view *sp_sviewsrc;
      struct sp_sampler_view *sp_sviewdst =
         &softpipe->tgsi.sampler[shader]->sp_sview[start + i];
      struct pipe_sampler_view **pview = &softpipe->sampler_views[shader][start + i];
      pipe_sampler_view_reference(pview, views[i]);
      sp_tex_tile_cache_set_sampler_view(softpipe->tex_cache[shader][start + i],
                                         views[i]);
      /*
       * We don't really have variants, however some bits are different per shader,
       * so just copy?
       */
      sp_sviewsrc = (struct sp_sampler_view *)*pview;
      if (sp_sviewsrc) {
         memcpy(sp_sviewdst, sp_sviewsrc, sizeof(*sp_sviewsrc));
         sp_sviewdst->compute_lambda = softpipe_get_lambda_func(&sp_sviewdst->base, shader);
         sp_sviewdst->cache = softpipe->tex_cache[shader][start + i];
      }
      else {
         memset(sp_sviewdst, 0,  sizeof(*sp_sviewsrc));
      }
   }


   /* find highest non-null sampler_views[] entry */
   {
      unsigned j = MAX2(softpipe->num_sampler_views[shader], start + num);
      while (j > 0 && softpipe->sampler_views[shader][j - 1] == NULL)
         j--;
      softpipe->num_sampler_views[shader] = j;
   }

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_sampler_views(softpipe->draw,
                             shader,
                             softpipe->sampler_views[shader],
                             softpipe->num_sampler_views[shader]);
   }

   softpipe->dirty |= SP_NEW_TEXTURE;
}


static void
softpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}


void
softpipe_init_sampler_funcs(struct pipe_context *pipe)
{
   pipe->create_sampler_state = softpipe_create_sampler_state;
   pipe->bind_sampler_states = softpipe_bind_sampler_states;
   pipe->delete_sampler_state = softpipe_delete_sampler_state;

   pipe->create_sampler_view = softpipe_create_sampler_view;
   pipe->set_sampler_views = softpipe_set_sampler_views;
   pipe->sampler_view_destroy = softpipe_sampler_view_destroy;
}

