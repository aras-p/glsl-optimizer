/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_inlines.h"
#include "state_tracker/drm_api.h"
#include "state_tracker/dri1_api.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"

#include "util/u_memory.h"

static void
dri_copy_to_front(__DRIdrawablePrivate * dPriv,
		  struct pipe_surface *from,
		  int x, int y, unsigned w, unsigned h)
{
   /* TODO send a message to the Xserver to copy to the real front buffer */
}

static struct pipe_surface *
dri_surface_from_handle(struct pipe_screen *screen,
			unsigned handle,
			enum pipe_format format,
			unsigned width, unsigned height, unsigned pitch)
{
   struct pipe_surface *surface = NULL;
   struct pipe_texture *texture = NULL;
   struct pipe_texture templat;
   struct pipe_buffer *buf = NULL;

   buf = drm_api_hooks.buffer_from_handle(screen, "dri2 buffer", handle);
   if (!buf)
      return NULL;

   memset(&templat, 0, sizeof(templat));
   templat.tex_usage |= PIPE_TEXTURE_USAGE_RENDER_TARGET;
   templat.target = PIPE_TEXTURE_2D;
   templat.last_level = 0;
   templat.depth[0] = 1;
   templat.format = format;
   templat.width[0] = width;
   templat.height[0] = height;
   pf_get_block(templat.format, &templat.block);

   texture = screen->texture_blanket(screen, &templat, &pitch, buf);

   /* we don't need the buffer from this point on */
   pipe_buffer_reference(&buf, NULL);

   if (!texture)
      return NULL;

   surface = screen->get_tex_surface(screen, texture, 0, 0, 0,
				     PIPE_BUFFER_USAGE_GPU_READ |
				     PIPE_BUFFER_USAGE_GPU_WRITE);

   /* we don't need the texture from this point on */
   pipe_texture_reference(&texture, NULL);
   return surface;
}

/**
 * This will be called a drawable is known to have been resized.
 */
void
dri_get_buffers(__DRIdrawablePrivate * dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_surface *surface = NULL;
   struct pipe_screen *screen = dri_screen(drawable->sPriv)->pipe_screen;
   __DRIbuffer *buffers = NULL;
   __DRIscreen *dri_screen = drawable->sPriv;
   __DRIdrawable *dri_drawable = drawable->dPriv;
   boolean have_depth = FALSE;
   int i, count;

   buffers = (*dri_screen->dri2.loader->getBuffers) (dri_drawable,
						     &dri_drawable->w,
						     &dri_drawable->h,
						     drawable->attachments,
						     drawable->
						     num_attachments, &count,
						     dri_drawable->
						     loaderPrivate);

   if (buffers == NULL) {
      return;
   }

   /* set one cliprect to cover the whole dri_drawable */
   dri_drawable->x = 0;
   dri_drawable->y = 0;
   dri_drawable->backX = 0;
   dri_drawable->backY = 0;
   dri_drawable->numClipRects = 1;
   dri_drawable->pClipRects[0].x1 = 0;
   dri_drawable->pClipRects[0].y1 = 0;
   dri_drawable->pClipRects[0].x2 = dri_drawable->w;
   dri_drawable->pClipRects[0].y2 = dri_drawable->h;
   dri_drawable->numBackClipRects = 1;
   dri_drawable->pBackClipRects[0].x1 = 0;
   dri_drawable->pBackClipRects[0].y1 = 0;
   dri_drawable->pBackClipRects[0].x2 = dri_drawable->w;
   dri_drawable->pBackClipRects[0].y2 = dri_drawable->h;

   for (i = 0; i < count; i++) {
      enum pipe_format format = 0;
      int index = 0;

      switch (buffers[i].attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
	 index = ST_SURFACE_FRONT_LEFT;
	 format = PIPE_FORMAT_A8R8G8B8_UNORM;
	 break;
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
	 index = ST_SURFACE_FRONT_LEFT;
	 format = PIPE_FORMAT_A8R8G8B8_UNORM;
	 break;
      case __DRI_BUFFER_BACK_LEFT:
	 index = ST_SURFACE_BACK_LEFT;
	 format = PIPE_FORMAT_A8R8G8B8_UNORM;
	 break;
      case __DRI_BUFFER_DEPTH:
	 index = ST_SURFACE_DEPTH;
	 format = PIPE_FORMAT_Z24S8_UNORM;
	 break;
      case __DRI_BUFFER_STENCIL:
	 index = ST_SURFACE_DEPTH;
	 format = PIPE_FORMAT_Z24S8_UNORM;
	 break;
      case __DRI_BUFFER_ACCUM:
      default:
	 assert(0);
      }
      assert(buffers[i].cpp == 4);

      if (index == ST_SURFACE_DEPTH) {
	 if (have_depth)
	    continue;
	 else
	    have_depth = TRUE;
      }

      surface = dri_surface_from_handle(screen,
					buffers[i].name,
					format,
					dri_drawable->w,
					dri_drawable->h, buffers[i].pitch);

      st_set_framebuffer_surface(drawable->stfb, index, surface);
      pipe_surface_reference(&surface, NULL);
   }
   /* this needed, or else the state tracker fails to pick the new buffers */
   st_resize_framebuffer(drawable->stfb, dri_drawable->w, dri_drawable->h);
}

void
dri_flush_frontbuffer(struct pipe_screen *screen,
		      struct pipe_surface *surf, void *context_private)
{
   struct dri_context *ctx = (struct dri_context *)context_private;

   dri_copy_to_front(ctx->dPriv, surf, 0, 0, surf->width, surf->height);
}

/**
 * This is called when we need to set up GL rendering to a new X window.
 */
boolean
dri_create_buffer(__DRIscreenPrivate * sPriv,
		  __DRIdrawablePrivate * dPriv,
		  const __GLcontextModes * visual, boolean isPixmap)
{
   enum pipe_format colorFormat, depthFormat, stencilFormat;
   struct dri_screen *screen = sPriv->private;
   struct dri_drawable *drawable = NULL;
   struct pipe_screen *pscreen = screen->pipe_screen;
   int i;

   if (isPixmap)
      goto fail;		       /* not implemented */

   drawable = CALLOC_STRUCT(dri_drawable);
   if (drawable == NULL)
      goto fail;

   /* XXX: todo: use the pipe_screen queries to figure out which
    * render targets are supportable.
    */
   assert(visual->redBits == 8);
   assert(visual->depthBits == 24 || visual->depthBits == 0);
   assert(visual->stencilBits == 8 || visual->stencilBits == 0);

   colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

   if (visual->depthBits) {
      if (pscreen->is_format_supported(pscreen, PIPE_FORMAT_Z24S8_UNORM,
				       PIPE_TEXTURE_2D,
				       PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0))
	 depthFormat = PIPE_FORMAT_Z24S8_UNORM;
      else
	 depthFormat = PIPE_FORMAT_S8Z24_UNORM;
   } else
      depthFormat = PIPE_FORMAT_NONE;

   if (visual->stencilBits) {
      if (pscreen->is_format_supported(pscreen, PIPE_FORMAT_Z24S8_UNORM,
				       PIPE_TEXTURE_2D,
				       PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0))
	 stencilFormat = PIPE_FORMAT_Z24S8_UNORM;
      else
	 stencilFormat = PIPE_FORMAT_S8Z24_UNORM;
   } else
      stencilFormat = PIPE_FORMAT_NONE;

   drawable->stfb = st_create_framebuffer(visual,
					  colorFormat,
					  depthFormat,
					  stencilFormat,
					  dPriv->w,
					  dPriv->h, (void *)drawable);
   if (drawable->stfb == NULL)
      goto fail;

   drawable->sPriv = sPriv;
   drawable->dPriv = dPriv;
   dPriv->driverPrivate = (void *)drawable;

   /* setup dri2 buffers information */
   i = 0;
   drawable->attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
#if 0
   /* TODO incase of double buffer visual, delay fake creation */
   drawable->attachments[i++] = __DRI_BUFFER_FAKE_FRONT_LEFT;
#endif
   if (visual->doubleBufferMode)
      drawable->attachments[i++] = __DRI_BUFFER_BACK_LEFT;
   if (visual->depthBits)
      drawable->attachments[i++] = __DRI_BUFFER_DEPTH;
   if (visual->stencilBits)
      drawable->attachments[i++] = __DRI_BUFFER_STENCIL;
   drawable->num_attachments = i;

   drawable->desired_fences = 2;

   return GL_TRUE;
 fail:
   FREE(drawable);
   return GL_FALSE;
}

static struct pipe_fence_handle *
dri_swap_fences_pop_front(struct dri_drawable *draw)
{
   struct pipe_screen *screen = dri_screen(draw->sPriv)->pipe_screen;
   struct pipe_fence_handle *fence = NULL;

   if (draw->cur_fences >= draw->desired_fences) {
      screen->fence_reference(screen, &fence, draw->swap_fences[draw->tail]);
      screen->fence_reference(screen, &draw->swap_fences[draw->tail++], NULL);
      --draw->cur_fences;
      draw->tail &= DRI_SWAP_FENCES_MASK;
   }
   return fence;
}

static void
dri_swap_fences_push_back(struct dri_drawable *draw,
			  struct pipe_fence_handle *fence)
{
   struct pipe_screen *screen = dri_screen(draw->sPriv)->pipe_screen;

   if (!fence)
      return;

   if (draw->cur_fences < DRI_SWAP_FENCES_MAX) {
      draw->cur_fences++;
      screen->fence_reference(screen, &draw->swap_fences[draw->head++],
			      fence);
      draw->head &= DRI_SWAP_FENCES_MASK;
   }
}

void
dri_destroy_buffer(__DRIdrawablePrivate * dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_fence_handle *fence;
   struct pipe_screen *screen = dri_screen(drawable->sPriv)->pipe_screen;

   st_unreference_framebuffer(drawable->stfb);
   drawable->desired_fences = 0;
   while (drawable->cur_fences) {
      fence = dri_swap_fences_pop_front(drawable);
      screen->fence_reference(screen, &fence, NULL);
   }

   FREE(drawable);
}

static void
dri1_update_drawables_locked(struct dri_context *ctx,
			     __DRIdrawablePrivate * driDrawPriv,
			     __DRIdrawablePrivate * driReadPriv)
{
   if (ctx->stLostLock) {
      ctx->stLostLock = FALSE;
      if (driDrawPriv == driReadPriv)
	 DRI_VALIDATE_DRAWABLE_INFO(ctx->sPriv, driDrawPriv);
      else
	 DRI_VALIDATE_TWO_DRAWABLES_INFO(ctx->sPriv, driDrawPriv,
					 driReadPriv);
   }
}

/**
 * This ensures all contexts which bind to a drawable pick up the
 * drawable change and signal new buffer state.
 * Calling st_resize_framebuffer for each context may seem like overkill,
 * but no new buffers will actually be allocated if the dimensions don't
 * change.
 */

static void
dri1_propagate_drawable_change(struct dri_context *ctx)
{
   __DRIdrawablePrivate *dPriv = ctx->dPriv;
   __DRIdrawablePrivate *rPriv = ctx->rPriv;
   boolean flushed = FALSE;

   if (dPriv && ctx->d_stamp != dPriv->lastStamp) {

      st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      flushed = TRUE;
      ctx->d_stamp = dPriv->lastStamp;
      st_resize_framebuffer(dri_drawable(dPriv)->stfb, dPriv->w, dPriv->h);

   }

   if (rPriv && dPriv != rPriv && ctx->r_stamp != rPriv->lastStamp) {

      if (!flushed)
	 st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      ctx->r_stamp = rPriv->lastStamp;
      st_resize_framebuffer(dri_drawable(rPriv)->stfb, rPriv->w, rPriv->h);

   } else if (rPriv && dPriv == rPriv) {

      ctx->r_stamp = ctx->d_stamp;

   }
}

void
dri1_update_drawables(struct dri_context *ctx,
		      struct dri_drawable *draw, struct dri_drawable *read)
{
   dri_lock(ctx);
   dri1_update_drawables_locked(ctx, draw->dPriv, read->dPriv);
   dri_unlock(ctx);

   dri1_propagate_drawable_change(ctx);
}

static INLINE boolean
dri1_intersect_src_bbox(struct drm_clip_rect *dst,
			int dst_x,
			int dst_y,
			const struct drm_clip_rect *src,
			const struct drm_clip_rect *bbox)
{
   int xy1;
   int xy2;

   xy1 = ((int)src->x1 > (int)bbox->x1 + dst_x) ? src->x1 :
      (int)bbox->x1 + dst_x;
   xy2 = ((int)src->x2 < (int)bbox->x2 + dst_x) ? src->x2 :
      (int)bbox->x2 + dst_x;
   if (xy1 >= xy2 || xy1 < 0)
      return FALSE;

   dst->x1 = xy1;
   dst->x2 = xy2;

   xy1 = ((int)src->y1 > (int)bbox->y1 + dst_y) ? src->y1 :
      (int)bbox->y1 + dst_y;
   xy2 = ((int)src->y2 < (int)bbox->y2 + dst_y) ? src->y2 :
      (int)bbox->y2 + dst_y;
   if (xy1 >= xy2 || xy1 < 0)
      return FALSE;

   dst->y1 = xy1;
   dst->y2 = xy2;
   return TRUE;
}

static void
dri1_swap_copy(struct dri_context *ctx,
	       struct pipe_surface *dst,
	       struct pipe_surface *src,
	       __DRIdrawablePrivate * dPriv, const struct drm_clip_rect *bbox)
{
   struct pipe_context *pipe = ctx->pipe;
   struct drm_clip_rect clip;
   struct drm_clip_rect *cur;
   int i;

   cur = dPriv->pClipRects;

   for (i = 0; i < dPriv->numClipRects; ++i) {
      if (dri1_intersect_src_bbox(&clip, dPriv->x, dPriv->y, cur++, bbox))
	 pipe->surface_copy(pipe, dst, clip.x1, clip.y1,
			    src,
			    (int)clip.x1 - dPriv->x,
			    (int)clip.y1 - dPriv->y,
			    clip.x2 - clip.x1, clip.y2 - clip.y1);
   }
}

static void
dri1_copy_to_front(struct dri_context *ctx,
		   struct pipe_surface *surf,
		   __DRIdrawablePrivate * dPriv,
		   const struct drm_clip_rect *sub_box,
		   struct pipe_fence_handle **fence)
{
   struct pipe_context *pipe = ctx->pipe;
   boolean save_lost_lock;
   uint cur_w;
   uint cur_h;
   struct drm_clip_rect bbox;
   boolean visible = TRUE;

   *fence = NULL;

   dri_lock(ctx);
   save_lost_lock = ctx->stLostLock;
   dri1_update_drawables_locked(ctx, dPriv, dPriv);
   st_get_framebuffer_dimensions(dri_drawable(dPriv)->stfb, &cur_w, &cur_h);

   bbox.x1 = 0;
   bbox.x2 = cur_w;
   bbox.y1 = 0;
   bbox.y2 = cur_h;

   if (sub_box)
      visible = dri1_intersect_src_bbox(&bbox, 0, 0, &bbox, sub_box);

   if (visible && __dri1_api_hooks->present_locked) {

      __dri1_api_hooks->present_locked(pipe,
				       surf,
				       dPriv->pClipRects,
				       dPriv->numClipRects,
				       dPriv->x, dPriv->y, &bbox, fence);

   } else if (visible && __dri1_api_hooks->front_srf_locked) {

      struct pipe_surface *front = __dri1_api_hooks->front_srf_locked(pipe);

      if (front)
	 dri1_swap_copy(ctx, front, surf, dPriv, &bbox);

      st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, fence);
   }

   ctx->stLostLock = save_lost_lock;

   /**
    * FIXME: Revisit this: Update drawables on copy_sub_buffer ?
    */

   if (!sub_box)
      dri1_update_drawables_locked(ctx, ctx->dPriv, ctx->rPriv);

   dri_unlock(ctx);
   dri1_propagate_drawable_change(ctx);
}

void
dri1_flush_frontbuffer(struct pipe_screen *screen,
		       struct pipe_surface *surf, void *context_private)
{
   struct dri_context *ctx = (struct dri_context *)context_private;
   struct pipe_fence_handle *dummy_fence;

   dri1_copy_to_front(ctx, surf, ctx->dPriv, NULL, &dummy_fence);
   screen->fence_reference(screen, &dummy_fence, NULL);

   /**
    * FIXME: Do we need swap throttling here?
    */
}

void
dri_swap_buffers(__DRIdrawablePrivate * dPriv)
{
   struct dri_context *ctx;
   struct pipe_surface *back_surf;
   struct dri_drawable *draw = dri_drawable(dPriv);
   struct pipe_screen *screen = dri_screen(draw->sPriv)->pipe_screen;
   struct pipe_fence_handle *fence;
   struct st_context *st = st_get_current();

   assert(__dri1_api_hooks != NULL);

   if (!st)
      return;			       /* For now */

   ctx = (struct dri_context *)st->pipe->priv;

   st_get_framebuffer_surface(draw->stfb, ST_SURFACE_BACK_LEFT, &back_surf);
   if (back_surf) {
      st_notify_swapbuffers(draw->stfb);
      st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      fence = dri_swap_fences_pop_front(draw);
      if (fence) {
	 (void)screen->fence_finish(screen, fence, 0);
	 screen->fence_reference(screen, &fence, NULL);
      }
      dri1_copy_to_front(ctx, back_surf, dPriv, NULL, &fence);
      dri_swap_fences_push_back(draw, fence);
      screen->fence_reference(screen, &fence, NULL);
   }
}

void
dri_copy_sub_buffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h)
{
   struct pipe_screen *screen = dri_screen(dPriv->driScreenPriv)->pipe_screen;
   struct drm_clip_rect sub_bbox;
   struct dri_context *ctx;
   struct pipe_surface *back_surf;
   struct dri_drawable *draw = dri_drawable(dPriv);
   struct pipe_fence_handle *dummy_fence;
   struct st_context *st = st_get_current();

   assert(__dri1_api_hooks != NULL);

   if (!st)
      return;

   ctx = (struct dri_context *)st->pipe->priv;

   sub_bbox.x1 = x;
   sub_bbox.x2 = x + w;
   sub_bbox.y1 = y;
   sub_bbox.y2 = y + h;

   st_get_framebuffer_surface(draw->stfb, ST_SURFACE_BACK_LEFT, &back_surf);
   if (back_surf) {
      st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      dri1_copy_to_front(ctx, back_surf, dPriv, &sub_bbox, &dummy_fence);
      screen->fence_reference(screen, &dummy_fence, NULL);
   }
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
