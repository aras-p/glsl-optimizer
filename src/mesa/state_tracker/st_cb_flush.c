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
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/context.h"
#include "st_context.h"
#include "st_cb_bitmap.h"
#include "st_cb_flush.h"
#include "st_cb_fbo.h"
#include "st_public.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"


static INLINE GLboolean
is_front_buffer_dirty(struct st_context *st)
{
   return st->frontbuffer_status == FRONT_STATUS_DIRTY;
}


/**
 * Tell the winsys to display the front color buffer on-screen.
 */
static void
display_front_buffer(struct st_context *st)
{
   GLframebuffer *fb = st->ctx->DrawBuffer;
   struct st_renderbuffer *strb
      = st_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
   struct pipe_surface *front_surf = strb->surface;

   /* Hook for copying "fake" frontbuffer if necessary:
    */
   st->pipe->winsys->flush_frontbuffer( st->pipe->winsys, front_surf,
                                        st->pipe->priv );

   /*
   st->frontbuffer_status = FRONT_STATUS_UNDEFINED;
   */
}


void st_flush( struct st_context *st, uint pipeFlushFlags,
               struct pipe_fence_handle **fence )
{
   FLUSH_VERTICES(st->ctx, 0);

   st_flush_bitmap_cache(st);

   st->pipe->flush( st->pipe, pipeFlushFlags, fence );
}


/**
 * Flush, and wait for completion.
 */
void st_finish( struct st_context *st )
{
   struct pipe_fence_handle *fence = NULL;

   st_flush(st, PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME, &fence);

   st->pipe->winsys->fence_finish(st->pipe->winsys, fence, 0);
   st->pipe->winsys->fence_reference(st->pipe->winsys, &fence, NULL);
}



/**
 * Called via ctx->Driver.Flush()
 */
static void st_glFlush(GLcontext *ctx)
{
   struct st_context *st = ctx->st;

   if (is_front_buffer_dirty(st)) {
      st_finish(st);
      display_front_buffer(st);
   }
   else {
      st_flush(st, PIPE_FLUSH_RENDER_CACHE, NULL);
   }
}


/**
 * Called via ctx->Driver.Finish()
 */
static void st_glFinish(GLcontext *ctx)
{
   struct st_context *st = ctx->st;

   st_finish(st);

   if (is_front_buffer_dirty(st)) {
      display_front_buffer(st);
   }
}


void st_init_flush_functions(struct dd_function_table *functions)
{
   functions->Flush = st_glFlush;
   functions->Finish = st_glFinish;
}
