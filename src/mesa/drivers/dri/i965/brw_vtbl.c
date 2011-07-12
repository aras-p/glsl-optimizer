/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
**********************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"
#include "main/framebuffer.h"

#include "intel_batchbuffer.h" 
#include "intel_regions.h" 
#include "intel_fbo.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_wm.h"

#include "../glsl/ralloc.h"

static void
dri_bo_release(drm_intel_bo **bo)
{
   drm_intel_bo_unreference(*bo);
   *bo = NULL;
}


/**
 * called from intelDestroyContext()
 */
static void brw_destroy_context( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw_destroy_state(brw);
   brw_draw_destroy( brw );
   brw_clear_validated_bos(brw);
   ralloc_free(brw->wm.compile_data);

   dri_bo_release(&brw->curbe.curbe_bo);
   dri_bo_release(&brw->vs.const_bo);
   dri_bo_release(&brw->wm.const_bo);

   free(brw->curbe.last_buf);
   free(brw->curbe.next_buf);
}

/**
 * Update the hardware state for drawing into a window or framebuffer object.
 *
 * Called by glDrawBuffer, glBindFramebufferEXT, MakeCurrent, and other
 * places within the driver.
 *
 * Basically, this needs to be called any time the current framebuffer
 * changes, the renderbuffers change, or we need to draw into different
 * color buffers.
 */
static void
brw_update_draw_buffer(struct intel_context *intel)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_region *colorRegions[MAX_DRAW_BUFFERS], *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;
   bool fb_has_hiz = intel_framebuffer_has_hiz(fb);

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /*
    * If intel_context is using separate stencil, but the depth attachment
    * (gl_framebuffer.Attachment[BUFFER_DEPTH]) has a packed depth/stencil
    * format, then we must install the real depth buffer at fb->_DepthBuffer
    * and set fb->_DepthBuffer->Wrapped before calling _mesa_update_framebuffer.
    * Otherwise, _mesa_update_framebuffer will create and install a swras
    * depth wrapper instead.
    *
    * Ditto for stencil.
    */
   irbDepth = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   if (irbDepth && irbDepth->Base.Format == MESA_FORMAT_X8_Z24) {
      _mesa_reference_renderbuffer(&fb->_DepthBuffer, &irbDepth->Base);
      irbDepth->Base.Wrapped = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   }

   irbStencil = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   if (irbStencil && irbStencil->Base.Format == MESA_FORMAT_S8) {
      _mesa_reference_renderbuffer(&fb->_StencilBuffer, &irbStencil->Base);
      irbStencil->Base.Wrapped = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   }

   /* Do this here, not core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & _NEW_BUFFERS) {
      /* this updates the DrawBuffer->_NumColorDrawBuffers fields, etc */
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* this may occur when we're called by glBindFrameBuffer() during
       * the process of someone setting up renderbuffers, etc.
       */
      /*_mesa_debug(ctx, "DrawBuffer: incomplete user FBO\n");*/
      return;
   }

   /* How many color buffers are we drawing into?
    *
    * If there are zero buffers or the buffer is too big, don't configure any
    * regions for hardware drawing.  We'll fallback to software below.  Not
    * having regions set makes some of the software fallback paths faster.
    */
   if ((fb->Width > ctx->Const.MaxRenderbufferSize)
       || (fb->Height > ctx->Const.MaxRenderbufferSize)
       || (fb->_NumColorDrawBuffers == 0)) {
      /* writing to 0  */
      colorRegions[0] = NULL;
   }
   else if (fb->_NumColorDrawBuffers > 1) {
       int i;
       struct intel_renderbuffer *irb;

       for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
           irb = intel_renderbuffer(fb->_ColorDrawBuffers[i]);
           colorRegions[i] = irb ? irb->region : NULL;
       }
   }
   else {
      /* Get the intel_renderbuffer for the single colorbuffer we're drawing
       * into.
       */
      if (fb->Name == 0) {
	 /* drawing to window system buffer */
	 if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT)
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_FRONT_LEFT);
	 else
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      }
      else {
	 /* drawing to user-created FBO */
	 struct intel_renderbuffer *irb;
	 irb = intel_renderbuffer(fb->_ColorDrawBuffers[0]);
	 colorRegions[0] = (irb && irb->region) ? irb->region : NULL;
      }
   }

   if (!colorRegions[0]) {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
   }
   else {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
   }

   /* Check for depth fallback. */
   if (irbDepth && irbDepth->region) {
      assert(!fb_has_hiz || irbDepth->Base.Format != MESA_FORMAT_S8_Z24);
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
      depthRegion = irbDepth->region;
   } else if (irbDepth && !irbDepth->region) {
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_TRUE);
      depthRegion = NULL;
   } else { /* !irbDepth */
      /* No fallback is needed because there is no depth buffer. */
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
      depthRegion = NULL;
   }

   /* Check for stencil fallback. */
   if (irbStencil && irbStencil->region) {
      if (!intel->has_separate_stencil)
	 assert(irbStencil->Base.Format == MESA_FORMAT_S8_Z24);
      if (fb_has_hiz || intel->must_use_separate_stencil)
	 assert(irbStencil->Base.Format == MESA_FORMAT_S8);
      if (irbStencil->Base.Format == MESA_FORMAT_S8)
	 assert(intel->has_separate_stencil);
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
   } else if (irbStencil && !irbStencil->region) {
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_TRUE);
   } else { /* !irbStencil */
      /* No fallback is needed because there is no stencil buffer. */
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
   }

   /* If we have a (packed) stencil buffer attached but no depth buffer,
    * we still need to set up the shared depth/stencil state so we can use it.
    */
   if (depthRegion == NULL && irbStencil && irbStencil->region
       && irbStencil->Base.Format == MESA_FORMAT_S8_Z24) {
      depthRegion = irbStencil->region;
   }

   /*
    * Update depth and stencil test state
    */
   if (ctx->Driver.Enable) {
      ctx->Driver.Enable(ctx, GL_DEPTH_TEST,
                         (ctx->Depth.Test && fb->Visual.depthBits > 0));
      ctx->Driver.Enable(ctx, GL_STENCIL_TEST,
                         (ctx->Stencil.Enabled && fb->Visual.stencilBits > 0));
   }
   else {
      /* Mesa's Stencil._Enabled field is updated when
       * _NEW_BUFFERS | _NEW_STENCIL, but i965 code assumes that the value
       * only changes with _NEW_STENCIL (which seems sensible).  So flag it
       * here since this is the _NEW_BUFFERS path.
       */
      intel->NewGLState |= (_NEW_DEPTH | _NEW_STENCIL);
   }

   intel->vtbl.set_draw_region(intel, colorRegions, depthRegion,
                               fb->_NumColorDrawBuffers);
   intel->NewGLState |= _NEW_BUFFERS;

   /* update viewport since it depends on window size */
#ifdef I915
   intelCalcViewport(ctx);
#else
   intel->NewGLState |= _NEW_VIEWPORT;
#endif
   /* Set state we know depends on drawable parameters:
    */
   if (ctx->Driver.Scissor)
      ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			  ctx->Scissor.Width, ctx->Scissor.Height);
   intel->NewGLState |= _NEW_SCISSOR;

   if (ctx->Driver.DepthRange)
      ctx->Driver.DepthRange(ctx,
			     ctx->Viewport.Near,
			     ctx->Viewport.Far);

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
   else
      intel->NewGLState |= _NEW_POLYGON;
}

/**
 * called from intelDrawBuffer()
 */
static void brw_set_draw_region( struct intel_context *intel, 
                                 struct intel_region *color_regions[],
                                 struct intel_region *depth_region,
                                 GLuint num_color_regions)
{
}


/**
 * called from intel_batchbuffer_flush and children before sending a
 * batchbuffer off.
 */
static void brw_finish_batch(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   brw_emit_query_end(brw);

   if (brw->curbe.curbe_bo) {
      drm_intel_gem_bo_unmap_gtt(brw->curbe.curbe_bo);
      drm_intel_bo_unreference(brw->curbe.curbe_bo);
      brw->curbe.curbe_bo = NULL;
   }
}


/**
 * called from intelFlushBatchLocked
 */
static void brw_new_batch( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   /* Mark all context state as needing to be re-emitted.
    * This is probably not as severe as on 915, since almost all of our state
    * is just in referenced buffers.
    */
   brw->state.dirty.brw |= BRW_NEW_CONTEXT | BRW_NEW_BATCH;

   /* Assume that the last command before the start of our batch was a
    * primitive, for safety.
    */
   intel->batch.need_workaround_flush = true;

   brw->state_batch_count = 0;

   brw->vb.nr_current_buffers = 0;

   /* Mark that the current program cache BO has been used by the GPU.
    * It will be reallocated if we need to put new programs in for the
    * next batch.
    */
   brw->cache.bo_used_by_gpu = true;
}

static void brw_invalidate_state( struct intel_context *intel, GLuint new_state )
{
   /* nothing */
}

/**
 * \see intel_context.vtbl.is_hiz_depth_format
 */
static bool brw_is_hiz_depth_format(struct intel_context *intel,
                                    gl_format format)
{
   /* In the future, this will support Z_FLOAT32. */
   return intel->has_hiz && (format == MESA_FORMAT_X8_Z24);
}


void brwInitVtbl( struct brw_context *brw )
{
   brw->intel.vtbl.check_vertex_size = 0;
   brw->intel.vtbl.emit_state = 0;
   brw->intel.vtbl.reduced_primitive_state = 0;
   brw->intel.vtbl.render_start = 0;
   brw->intel.vtbl.update_texture_state = 0;

   brw->intel.vtbl.invalidate_state = brw_invalidate_state;
   brw->intel.vtbl.new_batch = brw_new_batch;
   brw->intel.vtbl.finish_batch = brw_finish_batch;
   brw->intel.vtbl.destroy = brw_destroy_context;
   brw->intel.vtbl.update_draw_buffer = brw_update_draw_buffer;
   brw->intel.vtbl.set_draw_region = brw_set_draw_region;
   brw->intel.vtbl.debug_batch = brw_debug_batch;
   brw->intel.vtbl.render_target_supported = brw_render_target_supported;
   brw->intel.vtbl.is_hiz_depth_format = brw_is_hiz_depth_format;
}
