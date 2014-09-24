/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_upload_mgr.h"

#include "ilo_blit.h"
#include "ilo_blitter.h"
#include "ilo_cp.h"
#include "ilo_draw.h"
#include "ilo_gpgpu.h"
#include "ilo_query.h"
#include "ilo_render.h"
#include "ilo_resource.h"
#include "ilo_screen.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_transfer.h"
#include "ilo_video.h"
#include "ilo_context.h"

static void
ilo_context_cp_submitted(struct ilo_cp *cp, void *data)
{
   struct ilo_context *ilo = ilo_context(data);

   /* builder buffers are reallocated */
   ilo_render_invalidate_builder(ilo->render);
}

static void
ilo_flush(struct pipe_context *pipe,
          struct pipe_fence_handle **f,
          unsigned flags)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_cp_submit(ilo->cp,
         (flags & PIPE_FLUSH_END_OF_FRAME) ? "frame end" : "user request");

   if (f) {
      *f = (struct pipe_fence_handle *)
         ilo_fence_create(pipe->screen, ilo->cp->last_submitted_bo);
   }
}

static void
ilo_render_condition(struct pipe_context *pipe,
                     struct pipe_query *query,
                     boolean condition,
                     uint mode)
{
   struct ilo_context *ilo = ilo_context(pipe);

   /* reference count? */
   ilo->render_condition.query = query;
   ilo->render_condition.condition = condition;
   ilo->render_condition.mode = mode;
}

bool
ilo_skip_rendering(struct ilo_context *ilo)
{
   uint64_t result;
   bool wait;

   if (!ilo->render_condition.query)
      return false;

   switch (ilo->render_condition.mode) {
   case PIPE_RENDER_COND_WAIT:
   case PIPE_RENDER_COND_BY_REGION_WAIT:
      wait = true;
      break;
   case PIPE_RENDER_COND_NO_WAIT:
   case PIPE_RENDER_COND_BY_REGION_NO_WAIT:
   default:
      wait = false;
      break;
   }

   if (ilo->base.get_query_result(&ilo->base, ilo->render_condition.query,
            wait, (union pipe_query_result *) &result))
      return ((bool) result == ilo->render_condition.condition);
   else
      return false;
}

static void
ilo_context_destroy(struct pipe_context *pipe)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_state_vector_cleanup(&ilo->state_vector);

   if (ilo->uploader)
      u_upload_destroy(ilo->uploader);

   if (ilo->blitter)
      ilo_blitter_destroy(ilo->blitter);
   if (ilo->render)
      ilo_render_destroy(ilo->render);
   if (ilo->shader_cache)
      ilo_shader_cache_destroy(ilo->shader_cache);
   if (ilo->cp)
      ilo_cp_destroy(ilo->cp);

   util_slab_destroy(&ilo->transfer_mempool);

   FREE(ilo);
}

static struct pipe_context *
ilo_context_create(struct pipe_screen *screen, void *priv)
{
   struct ilo_screen *is = ilo_screen(screen);
   struct ilo_context *ilo;

   ilo = CALLOC_STRUCT(ilo_context);
   if (!ilo)
      return NULL;

   ilo->winsys = is->winsys;
   ilo->dev = &is->dev;

   /*
    * initialize first, otherwise it may not be safe to call
    * ilo_context_destroy() on errors
    */
   util_slab_create(&ilo->transfer_mempool,
         sizeof(struct ilo_transfer), 64, UTIL_SLAB_SINGLETHREADED);

   ilo->shader_cache = ilo_shader_cache_create();
   ilo->cp = ilo_cp_create(ilo->dev, ilo->winsys, ilo->shader_cache);
   if (ilo->cp)
      ilo->render = ilo_render_create(&ilo->cp->builder);

   if (!ilo->cp || !ilo->shader_cache || !ilo->render) {
      ilo_context_destroy(&ilo->base);
      return NULL;
   }

   ilo_cp_set_submit_callback(ilo->cp,
         ilo_context_cp_submitted, (void *) ilo);

   ilo->base.screen = screen;
   ilo->base.priv = priv;

   ilo->base.destroy = ilo_context_destroy;
   ilo->base.flush = ilo_flush;
   ilo->base.render_condition = ilo_render_condition;

   ilo_init_draw_functions(ilo);
   ilo_init_query_functions(ilo);
   ilo_init_state_functions(ilo);
   ilo_init_blit_functions(ilo);
   ilo_init_transfer_functions(ilo);
   ilo_init_video_functions(ilo);
   ilo_init_gpgpu_functions(ilo);

   ilo_init_draw(ilo);
   ilo_state_vector_init(ilo->dev, &ilo->state_vector);

   /*
    * These must be called last as u_upload/u_blitter are clients of the pipe
    * context.
    */
   ilo->uploader = u_upload_create(&ilo->base, 1024 * 1024, 16,
         PIPE_BIND_CONSTANT_BUFFER | PIPE_BIND_INDEX_BUFFER);
   if (!ilo->uploader) {
      ilo_context_destroy(&ilo->base);
      return NULL;
   }

   ilo->blitter = ilo_blitter_create(ilo);
   if (!ilo->blitter) {
      ilo_context_destroy(&ilo->base);
      return NULL;
   }

   return &ilo->base;
}

/**
 * Initialize context-related functions.
 */
void
ilo_init_context_functions(struct ilo_screen *is)
{
   is->base.context_create = ilo_context_create;
}
