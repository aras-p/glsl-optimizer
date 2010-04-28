/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <vl_winsys.h>
#include <driclient.h>
#include <state_tracker/dri1_api.h>
#include <pipe/p_video_context.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>

struct vl_dri_screen
{
   struct vl_screen base;
   Visual *visual;
   struct drm_api *api;
   dri_screen_t *dri_screen;
   dri_framebuffer_t dri_framebuf;
   struct dri1_api *api_hooks;
   boolean dri2;
};

struct vl_dri_context
{
   struct vl_context base;
   boolean is_locked;
   boolean lost_lock;
   drmLock *lock;
   dri_context_t *dri_context;
   int fd;
   struct pipe_video_context *vpipe;
   dri_drawable_t *drawable;
   struct pipe_surface *dri2_front;
};

static void
vl_dri_lock(void *priv)
{
   struct vl_dri_context *vl_dri_ctx = priv;
   drm_context_t hw_context;
   char ret = 0;

   assert(priv);

   hw_context = vl_dri_ctx->dri_context->drm_context;

   DRM_CAS(vl_dri_ctx->lock, hw_context, DRM_LOCK_HELD | hw_context, ret);
   if (ret) {
      drmGetLock(vl_dri_ctx->fd, hw_context, 0);
      vl_dri_ctx->lost_lock = TRUE;
   }
   vl_dri_ctx->is_locked = TRUE;
}

static void
vl_dri_unlock(void *priv)
{
   struct vl_dri_context *vl_dri_ctx = priv;
   drm_context_t hw_context;

   assert(priv);

   hw_context = vl_dri_ctx->dri_context->drm_context;

   vl_dri_ctx->is_locked = FALSE;
   DRM_UNLOCK(vl_dri_ctx->fd, vl_dri_ctx->lock, hw_context);
}

static boolean
vl_dri_is_locked(void *priv)
{
   struct vl_dri_context *vl_dri_ctx = priv;

   assert(priv);

   return vl_dri_ctx->is_locked;
}

static boolean
vl_dri_lost_lock(void *priv)
{
   struct vl_dri_context *vl_dri_ctx = priv;

   assert(priv);

   return vl_dri_ctx->lost_lock;
}

static void
vl_dri_clear_lost_lock(void *priv)
{
   struct vl_dri_context *vl_dri_ctx = priv;

   assert(priv);

   vl_dri_ctx->lost_lock = FALSE;
}

struct dri1_api_lock_funcs dri1_lf =
{
   .lock = vl_dri_lock,
   .unlock = vl_dri_unlock,
   .is_locked = vl_dri_is_locked,
   .is_lock_lost = vl_dri_lost_lock,
   .clear_lost_lock = vl_dri_clear_lost_lock
};

static void
vl_dri_copy_version(struct dri1_api_version *dst, dri_version_t *src)
{
   assert(src);
   assert(dst);
   dst->major = src->major;
   dst->minor = src->minor;
   dst->patch_level = src->patch;
}

static boolean
vl_dri_intersect_src_bbox(struct drm_clip_rect *dst, int dst_x, int dst_y,
                          const struct drm_clip_rect *src, const struct drm_clip_rect *bbox)
{
   int xy1;
   int xy2;

   assert(dst);
   assert(src);
   assert(bbox);

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
vl_clip_copy(struct vl_dri_context *vl_dri_ctx,
             struct pipe_surface *dst,
             struct pipe_surface *src,
             const struct drm_clip_rect *src_bbox)
{
   struct pipe_video_context *vpipe;
   struct drm_clip_rect clip;
   struct drm_clip_rect *cur;
   int i;

   assert(vl_dri_ctx);
   assert(dst);
   assert(src);
   assert(src_bbox);

   vpipe = vl_dri_ctx->base.vpipe;

   assert(vl_dri_ctx->drawable->cliprects);
   assert(vl_dri_ctx->drawable->num_cliprects > 0);

   cur = vl_dri_ctx->drawable->cliprects;

   for (i = 0; i < vl_dri_ctx->drawable->num_cliprects; ++i) {
      if (vl_dri_intersect_src_bbox(&clip, vl_dri_ctx->drawable->x, vl_dri_ctx->drawable->y, cur++, src_bbox))
         vpipe->surface_copy
         (
            vpipe, dst, clip.x1, clip.y1, src,
            (int)clip.x1 - vl_dri_ctx->drawable->x,
            (int)clip.y1 - vl_dri_ctx->drawable->y,
            clip.x2 - clip.x1, clip.y2 - clip.y1
         );
   }
}

static void
vl_dri_update_drawables_locked(struct vl_dri_context *vl_dri_ctx)
{
   struct vl_dri_screen *vl_dri_scrn;

   assert(vl_dri_ctx);

   vl_dri_scrn = (struct vl_dri_screen*)vl_dri_ctx->base.vscreen;

   if (vl_dri_ctx->lost_lock) {
      vl_dri_ctx->lost_lock = FALSE;
      DRI_VALIDATE_DRAWABLE_INFO(vl_dri_scrn->dri_screen, vl_dri_ctx->drawable);
   }
}

static void
vl_dri_flush_frontbuffer(struct pipe_screen *screen,
                         struct pipe_surface *surf, void *context_private)
{
   struct vl_dri_context *vl_dri_ctx = (struct vl_dri_context*)context_private;
   struct vl_dri_screen *vl_dri_scrn;
   struct drm_clip_rect src_bbox;
   boolean save_lost_lock = FALSE;

   assert(screen);
   assert(surf);
   assert(context_private);

   vl_dri_scrn = (struct vl_dri_screen*)vl_dri_ctx->base.vscreen;

   vl_dri_lock(vl_dri_ctx);

   save_lost_lock = vl_dri_ctx->lost_lock;

   vl_dri_update_drawables_locked(vl_dri_ctx);

   if (vl_dri_ctx->drawable->cliprects) {
      src_bbox.x1 = 0;
      src_bbox.x2 = vl_dri_ctx->drawable->w;
      src_bbox.y1 = 0;
      src_bbox.y2 = vl_dri_ctx->drawable->h;

#if 0
      if (vl_dri_scrn->_api_hooks->present_locked)
         vl_dri_scrn->api_hooks->present_locked(pipe, surf,
                                                vl_dri_ctx->drawable->cliprects,
                                                vl_dri_ctx->drawable->num_cliprects,
                                                vl_dri_ctx->drawable->x, vl_dri_drawable->y,
                                                &bbox, NULL /*fence*/);
      else
#endif
      if (vl_dri_scrn->api_hooks->front_srf_locked) {
         struct pipe_surface *front = vl_dri_scrn->api_hooks->front_srf_locked(screen);

         if (front)
            vl_clip_copy(vl_dri_ctx, front, surf, &src_bbox);

         //st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, fence);
      }
   }

   vl_dri_ctx->lost_lock = save_lost_lock;

   vl_dri_unlock(vl_dri_ctx);
}

static struct pipe_surface*
vl_dri2_get_front(struct vl_dri_screen *vl_dri_scrn, Drawable drawable)
{
   int w, h;
   unsigned int attachments[1] = {DRI_BUFFER_FRONT_LEFT};
   int count;
   DRI2Buffer *dri2_front;
   struct pipe_texture template, *front_tex;
   struct pipe_surface *front_surf = NULL;

   assert(vl_dri_scrn);

   dri2_front = DRI2GetBuffers(vl_dri_scrn->dri_screen->display,
                               drawable, &w, &h, attachments, 1, &count);
   if (dri2_front) {
      front_tex = vl_dri_scrn->api->texture_from_shared_handle(vl_dri_scrn->api, vl_dri_scrn->base.pscreen,
                                                               &template, "", dri2_front->pitch, dri2_front->name);
      if (front_tex)
         front_surf = vl_dri_scrn->base.pscreen->get_tex_surface(vl_dri_scrn->base.pscreen,
                                                                 front_tex, 0, 0, 0,
                                                                 PIPE_BUFFER_USAGE_GPU_READ_WRITE);
      pipe_texture_reference(&front_tex, NULL);
   }

   return front_surf;
}

static void
vl_dri2_flush_frontbuffer(struct pipe_screen *screen,
                          struct pipe_surface *surf, void *context_private)
{
   struct vl_dri_context *vl_dri_ctx = (struct vl_dri_context*)context_private;
   struct vl_dri_screen *vl_dri_scrn;
   struct pipe_video_context *vpipe;

   assert(screen);
   assert(surf);
   assert(context_private);
   assert(vl_dri_ctx->dri2_front);

   vl_dri_scrn = (struct vl_dri_screen*)vl_dri_ctx->base.vscreen;
   vpipe = vl_dri_ctx->base.vpipe;

   /* XXX: Why not just render to fake front? */
   vpipe->surface_copy(vpipe, vl_dri_ctx->dri2_front, 0, 0, surf, 0, 0, surf->width, surf->height);

   //st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, fence);
}


Drawable
vl_video_bind_drawable(struct vl_context *vctx, Drawable drawable)
{
   struct vl_dri_context *vl_dri_ctx = (struct vl_dri_context*)vctx;
   struct vl_dri_screen *vl_dri_scrn;
   dri_drawable_t *dri_drawable;
   Drawable old_drawable = None;

   assert(vctx);

   if (vl_dri_ctx->drawable)
      old_drawable = vl_dri_ctx->drawable->x_drawable;

   if (drawable != old_drawable) {
      vl_dri_scrn = (struct vl_dri_screen*)vl_dri_ctx->base.vscreen;
      if (vl_dri_scrn->dri2) {
         /* XXX: Need dri2CreateDrawable()? */
         vl_dri_ctx->dri2_front = vl_dri2_get_front(vl_dri_scrn, drawable);
      }
      else {
         driCreateDrawable(vl_dri_scrn->dri_screen, drawable, &dri_drawable);
         vl_dri_ctx->drawable = dri_drawable;
      }
   }

   return old_drawable;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_dri_screen *vl_dri_scrn;
   struct dri1_create_screen_arg arg;

   assert(display);

   vl_dri_scrn = CALLOC_STRUCT(vl_dri_screen);
   if (!vl_dri_scrn)
      return NULL;

   /* Try DRI2 first */
   if (dri2CreateScreen(display, screen, &vl_dri_scrn->dri_screen)) {
      /* If not, try DRI */
      if (driCreateScreen(display, screen, &vl_dri_scrn->dri_screen, &vl_dri_scrn->dri_framebuf)) {
         /* Now what? */
         FREE(vl_dri_scrn);
         return NULL;
      }
      else {
         /* Got DRI */
         arg.base.mode = DRM_CREATE_DRI1;
         arg.lf = &dri1_lf;
         arg.ddx_info = vl_dri_scrn->dri_framebuf.private;
         arg.ddx_info_size = vl_dri_scrn->dri_framebuf.private_size;
         arg.sarea = vl_dri_scrn->dri_screen->sarea;
         vl_dri_copy_version(&arg.ddx_version, &vl_dri_scrn->dri_screen->ddx);
         vl_dri_copy_version(&arg.dri_version, &vl_dri_scrn->dri_screen->dri);
         vl_dri_copy_version(&arg.drm_version, &vl_dri_scrn->dri_screen->drm);
         arg.api = NULL;
         vl_dri_scrn->dri2 = FALSE;
      }
   }
   else {
      /* Got DRI2 */
      arg.base.mode = DRM_CREATE_NORMAL;
      vl_dri_scrn->dri2 = TRUE;
   }

   vl_dri_scrn->api = drm_api_create();
   if (!vl_dri_scrn->api) {
      FREE(vl_dri_scrn);
      return NULL;
   }

   vl_dri_scrn->base.pscreen = vl_dri_scrn->api->create_screen(vl_dri_scrn->api,
                                                               vl_dri_scrn->dri_screen->fd,
                                                               &arg.base);

   if (!vl_dri_scrn->base.pscreen) {
      FREE(vl_dri_scrn);
      return NULL;
   }

   if (!vl_dri_scrn->dri2) {
      vl_dri_scrn->visual = XDefaultVisual(display, screen);
      vl_dri_scrn->api_hooks = arg.api;
      vl_dri_scrn->base.format = vl_dri_scrn->api_hooks->front_srf_locked(vl_dri_scrn->base.pscreen)->format;
      vl_dri_scrn->base.pscreen->flush_frontbuffer = vl_dri_flush_frontbuffer;
   }
   else
      vl_dri_scrn->base.pscreen->flush_frontbuffer = vl_dri2_flush_frontbuffer;

   return &vl_dri_scrn->base;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vscreen;

   assert(vscreen);

   vl_dri_scrn->base.pscreen->destroy(vl_dri_scrn->base.pscreen);
   if (vl_dri_scrn->dri2)
      dri2DestroyScreen(vl_dri_scrn->dri_screen);
   else
      driDestroyScreen(vl_dri_scrn->dri_screen);
   FREE(vl_dri_scrn);
}

struct vl_context*
vl_video_create(struct vl_screen *vscreen,
                enum pipe_video_profile profile,
                enum pipe_video_chroma_format chroma_format,
                unsigned width, unsigned height)
{
   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vscreen;
   struct vl_dri_context *vl_dri_ctx;

   vl_dri_ctx = CALLOC_STRUCT(vl_dri_context);
   if (!vl_dri_ctx)
      return NULL;

   /* XXX: Is default visual correct/sufficient here? */
   if (!vl_dri_scrn->dri2)
      driCreateContext(vl_dri_scrn->dri_screen, vl_dri_scrn->visual, &vl_dri_ctx->dri_context);

   if (!vscreen->pscreen->video_context_create) {
      debug_printf("[G3DVL] No video support found on %s/%s.\n",
                   vscreen->pscreen->get_vendor(vscreen->pscreen),
                   vscreen->pscreen->get_name(vscreen->pscreen));
      FREE(vl_dri_ctx);
      return NULL;
   }

   vl_dri_ctx->base.vpipe = vscreen->pscreen->video_context_create(vscreen->pscreen,
                                                                   profile, chroma_format,
                                                                   width, height,
                                                                   vl_dri_ctx->dri_context);

   if (!vl_dri_ctx->base.vpipe) {
      FREE(vl_dri_ctx);
      return NULL;
   }

   vl_dri_ctx->base.vpipe->priv = vl_dri_ctx;
   vl_dri_ctx->base.vscreen = vscreen;
   vl_dri_ctx->fd = vl_dri_scrn->dri_screen->fd;
   if (!vl_dri_scrn->dri2)
      vl_dri_ctx->lock = (drmLock*)&vl_dri_scrn->dri_screen->sarea->lock;

   return &vl_dri_ctx->base;
}

void vl_video_destroy(struct vl_context *vctx)
{
   struct vl_dri_context *vl_dri_ctx = (struct vl_dri_context*)vctx;

   assert(vctx);

   vl_dri_ctx->base.vpipe->destroy(vl_dri_ctx->base.vpipe);
   if (!((struct vl_dri_screen *)vctx->vscreen)->dri2)
      driDestroyContext(vl_dri_ctx->dri_context);
   FREE(vl_dri_ctx);
}
