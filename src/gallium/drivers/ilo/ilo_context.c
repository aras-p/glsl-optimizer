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

#include "ilo_3d.h"
#include "ilo_blit.h"
#include "ilo_blitter.h"
#include "ilo_cp.h"
#include "ilo_gpgpu.h"
#include "ilo_query.h"
#include "ilo_resource.h"
#include "ilo_screen.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_transfer.h"
#include "ilo_video.h"
#include "ilo_context.h"

static void
ilo_context_cp_flushed(struct ilo_cp *cp, void *data)
{
   struct ilo_context *ilo = ilo_context(data);

   if (ilo->last_cp_bo)
      intel_bo_unreference(ilo->last_cp_bo);

   /* remember the just flushed bo, on which fences could wait */
   ilo->last_cp_bo = cp->bo;
   intel_bo_reference(ilo->last_cp_bo);

   ilo_3d_cp_flushed(ilo->hw3d);
}

static void
ilo_flush(struct pipe_context *pipe,
          struct pipe_fence_handle **f,
          unsigned flags)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (f) {
      struct ilo_fence *fence;

      fence = CALLOC_STRUCT(ilo_fence);
      if (fence) {
         pipe_reference_init(&fence->reference, 1);

         /* reference the batch bo that we want to wait on */
         if (ilo_cp_empty(ilo->cp))
            fence->bo = ilo->last_cp_bo;
         else
            fence->bo = ilo->cp->bo;

         if (fence->bo)
            intel_bo_reference(fence->bo);
      }

      *f = (struct pipe_fence_handle *) fence;
   }

   ilo_cp_flush(ilo->cp,
         (flags & PIPE_FLUSH_END_OF_FRAME) ? "frame end" : "user request");
}

static void
ilo_context_destroy(struct pipe_context *pipe)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_cleanup_states(ilo);

   if (ilo->last_cp_bo)
      intel_bo_unreference(ilo->last_cp_bo);

   if (ilo->uploader)
      u_upload_destroy(ilo->uploader);

   if (ilo->blitter)
      ilo_blitter_destroy(ilo->blitter);
   if (ilo->hw3d)
      ilo_3d_destroy(ilo->hw3d);
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
   int cp_size;

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

   /* 8192 DWords */
   cp_size = 8192;
   if (cp_size * 4 > is->dev.max_batch_size)
      cp_size = is->dev.max_batch_size / 4;

   ilo->cp = ilo_cp_create(ilo->winsys, cp_size, is->dev.has_llc);
   ilo->shader_cache = ilo_shader_cache_create();
   if (ilo->cp)
      ilo->hw3d = ilo_3d_create(ilo->cp, ilo->dev);

   if (!ilo->cp || !ilo->shader_cache || !ilo->hw3d) {
      ilo_context_destroy(&ilo->base);
      return NULL;
   }

   ilo_cp_set_flush_callback(ilo->cp,
         ilo_context_cp_flushed, (void *) ilo);

   ilo->base.screen = screen;
   ilo->base.priv = priv;

   ilo->base.destroy = ilo_context_destroy;
   ilo->base.flush = ilo_flush;

   ilo_init_3d_functions(ilo);
   ilo_init_query_functions(ilo);
   ilo_init_state_functions(ilo);
   ilo_init_blit_functions(ilo);
   ilo_init_transfer_functions(ilo);
   ilo_init_video_functions(ilo);
   ilo_init_gpgpu_functions(ilo);

   ilo_init_states(ilo);

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
