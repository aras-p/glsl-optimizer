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

/* XXX DRI1 is untested after the switch to st_api.h */

#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
#include "pipe/p_context.h"
#include "state_tracker/dri1_api.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"
#include "dri1.h"

static INLINE void
dri1_lock(struct dri_context *ctx)
{
   drm_context_t hw_context = ctx->cPriv->hHWContext;
   char ret = 0;

   DRM_CAS(ctx->lock, hw_context, DRM_LOCK_HELD | hw_context, ret);
   if (ret) {
      drmGetLock(ctx->sPriv->fd, hw_context, 0);
      ctx->stLostLock = TRUE;
      ctx->wsLostLock = TRUE;
   }
   ctx->isLocked = TRUE;
}

static INLINE void
dri1_unlock(struct dri_context *ctx)
{
   ctx->isLocked = FALSE;
   DRM_UNLOCK(ctx->sPriv->fd, ctx->lock, ctx->cPriv->hHWContext);
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
dri1_swap_fences_clear(struct dri_drawable *drawable)
{
   struct pipe_screen *screen = dri_screen(drawable->sPriv)->pipe_screen;
   struct pipe_fence_handle *fence;

   while (drawable->cur_fences) {
      fence = dri_swap_fences_pop_front(drawable);
      screen->fence_reference(screen, &fence, NULL);
   }
}

static void
dri1_update_drawables_locked(struct dri_context *ctx,
			     __DRIdrawable * driDrawPriv,
			     __DRIdrawable * driReadPriv)
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
 */
static void
dri1_propagate_drawable_change(struct dri_context *ctx)
{
   __DRIdrawable *dPriv = ctx->dPriv;
   __DRIdrawable *rPriv = ctx->rPriv;
   struct dri_drawable *draw = dri_drawable(dPriv);
   struct dri_drawable *read = dri_drawable(rPriv);
   boolean flushed = FALSE;

   if (dPriv && draw->texture_stamp != dPriv->lastStamp) {
      ctx->st->flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      flushed = TRUE;
      ctx->st->notify_invalid_framebuffer(ctx->st, draw->stfb);
   }

   if (rPriv && dPriv != rPriv && read->texture_stamp != rPriv->lastStamp) {
      if (!flushed)
	 ctx->st->flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      ctx->st->notify_invalid_framebuffer(ctx->st, read->stfb);
   }
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
dri1_swap_copy(struct pipe_context *pipe,
	       struct pipe_surface *dst,
	       struct pipe_surface *src,
	       __DRIdrawable * dPriv, const struct drm_clip_rect *bbox)
{
   struct drm_clip_rect clip;
   struct drm_clip_rect *cur;
   int i;

   cur = dPriv->pClipRects;

   for (i = 0; i < dPriv->numClipRects; ++i) {
      if (dri1_intersect_src_bbox(&clip, dPriv->x, dPriv->y, cur++, bbox)) {
         if (pipe->surface_copy) {
            pipe->surface_copy(pipe, dst, clip.x1, clip.y1,
                               src,
                               (int)clip.x1 - dPriv->x,
                               (int)clip.y1 - dPriv->y,
                               clip.x2 - clip.x1, clip.y2 - clip.y1);
         } else {
            util_surface_copy(pipe, FALSE, dst, clip.x1, clip.y1,
                              src,
                              (int)clip.x1 - dPriv->x,
                              (int)clip.y1 - dPriv->y,
                              clip.x2 - clip.x1, clip.y2 - clip.y1);
         }
      }
   }
}

static struct pipe_surface *
dri1_get_pipe_surface(struct dri_drawable *drawable, struct pipe_texture *ptex)
{
   struct pipe_screen *pipe_screen = dri_screen(drawable->sPriv)->pipe_screen;
   struct pipe_surface *psurf = drawable->dri1_surface;

   if (!psurf || psurf->texture != ptex) {
      pipe_surface_reference(&drawable->dri1_surface, NULL);

      drawable->dri1_surface = pipe_screen->get_tex_surface(pipe_screen,
            ptex, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ);

      psurf = drawable->dri1_surface;
   }

   return psurf;
}

static struct pipe_context *
dri1_get_pipe_context(struct dri_drawable *drawable)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   struct pipe_context *pipe = screen->dri1_pipe;

   if (!pipe) {
      screen->dri1_pipe =
         screen->pipe_screen->context_create(screen->pipe_screen, NULL);
      pipe = screen->dri1_pipe;
   }

   return pipe;
}

static void
dri1_present_texture_locked(__DRIdrawable * dPriv,
                            struct pipe_texture *ptex,
                            const struct drm_clip_rect *sub_box,
                            struct pipe_fence_handle **fence)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_context *pipe;
   struct pipe_surface *psurf;
   struct drm_clip_rect bbox;
   boolean visible = TRUE;

   *fence = NULL;

   bbox.x1 = 0;
   bbox.x2 = ptex->width0;
   bbox.y1 = 0;
   bbox.y2 = ptex->height0;

   if (sub_box)
      visible = dri1_intersect_src_bbox(&bbox, 0, 0, &bbox, sub_box);
   if (!visible)
      return;

   pipe = dri1_get_pipe_context(drawable);
   psurf = dri1_get_pipe_surface(drawable, ptex);
   if (!pipe || !psurf)
      return;

   if (__dri1_api_hooks->present_locked) {
      __dri1_api_hooks->present_locked(pipe, psurf,
                                       dPriv->pClipRects, dPriv->numClipRects,
                                       dPriv->x, dPriv->y, &bbox, fence);
   } else if (__dri1_api_hooks->front_srf_locked) {
      struct pipe_surface *front = __dri1_api_hooks->front_srf_locked(pipe);

      if (front)
         dri1_swap_copy(pipe, front, psurf, dPriv, &bbox);

      pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, fence);
   }
}

static void
dri1_copy_to_front(struct dri_context *ctx,
		   struct pipe_texture *ptex,
		   __DRIdrawable * dPriv,
		   const struct drm_clip_rect *sub_box,
		   struct pipe_fence_handle **fence)
{
   boolean save_lost_lock;

   dri1_lock(ctx);
   save_lost_lock = ctx->stLostLock;
   dri1_update_drawables_locked(ctx, dPriv, dPriv);

   dri1_present_texture_locked(dPriv, ptex, sub_box, fence);

   ctx->stLostLock = save_lost_lock;

   /**
    * FIXME: Revisit this: Update drawables on copy_sub_buffer ?
    */

   if (!sub_box)
      dri1_update_drawables_locked(ctx, ctx->dPriv, ctx->rPriv);

   dri1_unlock(ctx);
   dri1_propagate_drawable_change(ctx);
}

void
dri1_flush_frontbuffer(struct dri_drawable *drawable,
                       struct pipe_texture *ptex)
{
   struct st_api *stapi = dri_get_st_api();
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   struct pipe_screen *pipe_screen = screen->pipe_screen;
   struct dri_context *ctx;
   struct pipe_fence_handle *dummy_fence;
   struct st_context_iface *st = stapi->get_current(stapi);

   if (!st)
      return;

   ctx = (struct dri_context *) st->st_manager_private;

   dri1_copy_to_front(ctx, ptex, ctx->dPriv, NULL, &dummy_fence);
   pipe_screen->fence_reference(pipe_screen, &dummy_fence, NULL);

   /**
    * FIXME: Do we need swap throttling here?
    */
}

void
dri1_swap_buffers(__DRIdrawable * dPriv)
{
   struct dri_context *ctx = dri_get_current();
   struct dri_drawable *draw = dri_drawable(dPriv);
   struct dri_screen *screen = dri_screen(draw->sPriv);
   struct pipe_screen *pipe_screen = screen->pipe_screen;
   struct pipe_fence_handle *fence;
   struct pipe_texture *ptex;

   assert(__dri1_api_hooks != NULL);

   if (!ctx)
      return;			       /* For now */

   ptex = draw->textures[ST_ATTACHMENT_BACK_LEFT];
   if (ptex) {
      ctx->st->flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      fence = dri_swap_fences_pop_front(draw);
      if (fence) {
	 (void)pipe_screen->fence_finish(pipe_screen, fence, 0);
	 pipe_screen->fence_reference(pipe_screen, &fence, NULL);
      }
      dri1_copy_to_front(ctx, ptex, dPriv, NULL, &fence);
      dri_swap_fences_push_back(draw, fence);
      pipe_screen->fence_reference(pipe_screen, &fence, NULL);
   }
}

void
dri1_copy_sub_buffer(__DRIdrawable * dPriv, int x, int y, int w, int h)
{
   struct dri_context *ctx = dri_get_current();
   struct dri_screen *screen = dri_screen(dPriv->driScreenPriv);
   struct pipe_screen *pipe_screen = screen->pipe_screen;
   struct drm_clip_rect sub_bbox;
   struct dri_drawable *draw = dri_drawable(dPriv);
   struct pipe_fence_handle *dummy_fence;
   struct pipe_texture *ptex;

   assert(__dri1_api_hooks != NULL);

   if (!ctx)
      return;

   sub_bbox.x1 = x;
   sub_bbox.x2 = x + w;
   sub_bbox.y1 = y;
   sub_bbox.y2 = y + h;

   ptex = draw->textures[ST_ATTACHMENT_BACK_LEFT];
   if (ptex) {
      ctx->st->flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      dri1_copy_to_front(ctx, ptex, dPriv, &sub_bbox, &dummy_fence);
      pipe_screen->fence_reference(pipe_screen, &dummy_fence, NULL);
   }
}

void
dri1_allocate_textures(struct dri_drawable *drawable,
                       unsigned width, unsigned height,
                       unsigned mask)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   struct pipe_texture templ;
   int i;

   /* remove outdated textures */
   if (drawable->old_w != width || drawable->old_h != height) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
         pipe_texture_reference(&drawable->textures[i], NULL);
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.last_level = 0;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      enum pipe_format format;
      unsigned tex_usage;

      /* the texture already exists or not requested */
      if (drawable->textures[i] || !(mask & (1 << i))) {
         /* remember the texture */
         if (drawable->textures[i])
            mask |= (1 << i);
         continue;
      }

      switch (i) {
      case ST_ATTACHMENT_FRONT_LEFT:
      case ST_ATTACHMENT_BACK_LEFT:
      case ST_ATTACHMENT_FRONT_RIGHT:
      case ST_ATTACHMENT_BACK_RIGHT:
         format = drawable->stvis.color_format;
         tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                     PIPE_TEXTURE_USAGE_RENDER_TARGET;
         break;
      case ST_ATTACHMENT_DEPTH_STENCIL:
         format = drawable->stvis.depth_stencil_format;
         tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
         break;
      default:
         format = PIPE_FORMAT_NONE;
         break;
      }

      if (templ.format != PIPE_FORMAT_NONE) {
         templ.format = format;
         templ.tex_usage = tex_usage;

         drawable->textures[i] =
            screen->pipe_screen->texture_create(screen->pipe_screen, &templ);
      }
   }

   drawable->old_w = width;
   drawable->old_h = height;
   drawable->texture_mask = mask;
}

static void
st_dri_lock(struct pipe_context *pipe)
{
   dri1_lock((struct dri_context *)pipe->priv);
}

static void
st_dri_unlock(struct pipe_context *pipe)
{
   dri1_unlock((struct dri_context *)pipe->priv);
}

static boolean
st_dri_is_locked(struct pipe_context *pipe)
{
   return ((struct dri_context *)pipe->priv)->isLocked;
}

static boolean
st_dri_lost_lock(struct pipe_context *pipe)
{
   return ((struct dri_context *)pipe->priv)->wsLostLock;
}

static void
st_dri_clear_lost_lock(struct pipe_context *pipe)
{
   ((struct dri_context *)pipe->priv)->wsLostLock = FALSE;
}

static struct dri1_api_lock_funcs dri1_lf = {
   .lock = st_dri_lock,
   .unlock = st_dri_unlock,
   .is_locked = st_dri_is_locked,
   .is_lock_lost = st_dri_lost_lock,
   .clear_lost_lock = st_dri_clear_lost_lock
};

static const __DRIextension *dri1_screen_extensions[] = {
   &driReadDrawableExtension,
   &driCopySubBufferExtension.base,
   &driSwapControlExtension.base,
   &driFrameTrackingExtension.base,
   &driMediaStreamCounterExtension.base,
   NULL
};

struct dri1_api *__dri1_api_hooks = NULL;

static INLINE void
dri1_copy_version(struct dri1_api_version *dst,
		  const struct __DRIversionRec *src)
{
   dst->major = src->major;
   dst->minor = src->minor;
   dst->patch_level = src->patch;
}

const __DRIconfig **
dri1_init_screen(__DRIscreen * sPriv)
{
   struct dri_screen *screen;
   const __DRIconfig **configs;
   struct dri1_create_screen_arg arg;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->api = drm_api_create();
   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   screen->drmLock = (drmLock *) & sPriv->pSAREA->lock;

   sPriv->private = (void *)screen;
   sPriv->extensions = dri1_screen_extensions;

   arg.base.mode = DRM_CREATE_DRI1;
   arg.lf = &dri1_lf;
   arg.ddx_info = sPriv->pDevPriv;
   arg.ddx_info_size = sPriv->devPrivSize;
   arg.sarea = sPriv->pSAREA;
   dri1_copy_version(&arg.ddx_version, &sPriv->ddx_version);
   dri1_copy_version(&arg.dri_version, &sPriv->dri_version);
   dri1_copy_version(&arg.drm_version, &sPriv->drm_version);
   arg.api = NULL;

   screen->pipe_screen = screen->api->create_screen(screen->api, screen->fd, &arg.base);

   if (!screen->pipe_screen || !arg.api) {
      debug_printf("%s: failed to create dri1 screen\n", __FUNCTION__);
      goto out_no_screen;
   }

   __dri1_api_hooks = arg.api;

   driParseOptionInfo(&screen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   /**
    * FIXME: If the driver supports format conversion swapbuffer blits, we might
    * want to support other color bit depths than the server is currently
    * using.
    */

   configs = dri_fill_in_modes(screen, sPriv->fbBPP);
   if (!configs)
      goto out_no_configs;

   return configs;
 out_no_configs:
   screen->pipe_screen->destroy(screen->pipe_screen);
 out_no_screen:
   FREE(screen);
   return NULL;
}
