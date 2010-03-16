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
#include "dri1.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "main/mtypes.h"
#include "main/renderbuffer.h"
#include "state_tracker/drm_api.h"
#include "state_tracker/dri1_api.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_cb_fbo.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
 
static struct pipe_surface *
dri_surface_from_handle(struct drm_api *api,
			struct pipe_screen *screen,
			unsigned handle,
			enum pipe_format format,
			unsigned width, unsigned height, unsigned pitch)
{
   struct pipe_surface *surface = NULL;
   struct pipe_texture *texture = NULL;
   struct pipe_texture templat;
   struct winsys_handle whandle;

   memset(&templat, 0, sizeof(templat));
   templat.tex_usage |= PIPE_TEXTURE_USAGE_RENDER_TARGET;
   templat.target = PIPE_TEXTURE_2D;
   templat.last_level = 0;
   templat.depth0 = 1;
   templat.format = format;
   templat.width0 = width;
   templat.height0 = height;

   memset(&whandle, 0, sizeof(whandle));
   whandle.handle = handle;
   whandle.stride = pitch;

   texture = screen->texture_from_handle(screen, &templat, &whandle);

   if (!texture) {
      debug_printf("%s: Failed to blanket the buffer with a texture\n", __func__);
      return NULL;
   }

   surface = screen->get_tex_surface(screen, texture, 0, 0, 0,
				     PIPE_BUFFER_USAGE_GPU_READ |
				     PIPE_BUFFER_USAGE_GPU_WRITE);

   /* we don't need the texture from this point on */
   pipe_texture_reference(&texture, NULL);
   return surface;
}

/**
 * Pixmaps have will have the same name of fake front and front.
 */
static boolean
dri2_check_if_pixmap(__DRIbuffer *buffers, int count)
{
   boolean found = FALSE;
   boolean is_pixmap = FALSE;
   unsigned name;
   int i;

   for (i = 0; i < count; i++) {
      switch (buffers[i].attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
         if (found) {
            is_pixmap = buffers[i].name == name;
         } else {
            name = buffers[i].name;
            found = TRUE;
         }
      default:
	 continue;
      }
   }

   return is_pixmap;
}

/**
 * This will be called a drawable is known to have been resized.
 */
void
dri_get_buffers(__DRIdrawable * dPriv)
{

   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_surface *surface = NULL;
   struct dri_screen *st_screen = dri_screen(drawable->sPriv);
   struct pipe_screen *screen = st_screen->pipe_screen;
   __DRIbuffer *buffers = NULL;
   __DRIscreen *dri_screen = drawable->sPriv;
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct drm_api *api = st_screen->api;
   boolean have_depth = FALSE;
   int i, count;

   if ((dri_screen->dri2.loader
        && (dri_screen->dri2.loader->base.version > 2)
        && (dri_screen->dri2.loader->getBuffersWithFormat != NULL))) {
      buffers = (*dri_screen->dri2.loader->getBuffersWithFormat)
                (dri_drawable, &dri_drawable->w, &dri_drawable->h,
                 drawable->attachments, drawable->num_attachments,
                 &count, dri_drawable->loaderPrivate);
   } else {
      assert(dri_screen->dri2.loader);
      buffers = (*dri_screen->dri2.loader->getBuffers) (dri_drawable,
                                                        &dri_drawable->w,
                                                        &dri_drawable->h,
                                                        drawable->attachments,
                                                        drawable->
                                                        num_attachments, &count,
                                                        dri_drawable->
                                                        loaderPrivate);
   }

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

   if (drawable->old_num == count &&
       drawable->old_w == dri_drawable->w &&
       drawable->old_h == dri_drawable->h &&
       memcmp(drawable->old, buffers, sizeof(__DRIbuffer) * count) == 0) {
       return;
   } else {
      drawable->old_num = count;
      drawable->old_w = dri_drawable->w;
      drawable->old_h = dri_drawable->h;
      memcpy(drawable->old, buffers, sizeof(__DRIbuffer) * count);
   }

   drawable->is_pixmap = dri2_check_if_pixmap(buffers, count);

   for (i = 0; i < count; i++) {
      enum pipe_format format = 0;
      int index = 0;

      switch (buffers[i].attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
	 if (!st_screen->auto_fake_front)
	    continue;
	 /* fallthrough */
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
	 index = ST_SURFACE_FRONT_LEFT;
	 format = drawable->color_format;
	 break;
      case __DRI_BUFFER_BACK_LEFT:
	 index = ST_SURFACE_BACK_LEFT;
	 format = drawable->color_format;
	 break;
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_STENCIL:
	 index = ST_SURFACE_DEPTH;
	 format = drawable->depth_stencil_format;
	 break;
      case __DRI_BUFFER_ACCUM:
      default:
	 assert(0);
      }

      if (index == ST_SURFACE_DEPTH) {
	 if (have_depth)
	    continue;
	 else
	    have_depth = TRUE;
      }

      surface = dri_surface_from_handle(api,
					screen,
					buffers[i].name,
					format,
					dri_drawable->w,
					dri_drawable->h, buffers[i].pitch);

      switch (buffers[i].attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
      case __DRI_BUFFER_BACK_LEFT:
	 drawable->color_format = surface->format;
	 break;
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_STENCIL:
	 drawable->depth_stencil_format = surface->format;
	 break;
      case __DRI_BUFFER_ACCUM:
      default:
	 assert(0);
      }

      st_set_framebuffer_surface(drawable->stfb, index, surface);
      pipe_surface_reference(&surface, NULL);
   }
   /* this needed, or else the state tracker fails to pick the new buffers */
   st_resize_framebuffer(drawable->stfb, dri_drawable->w, dri_drawable->h);
}

/**
 * These are used for GLX_EXT_texture_from_pixmap
 */
void dri2_set_tex_buffer2(__DRIcontext *pDRICtx, GLint target,
                          GLint format, __DRIdrawable *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_surface *ps;

   if (!drawable->stfb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer) {
      struct gl_renderbuffer *rb =
         st_new_renderbuffer_fb(drawable->color_format, 0 /*XXX*/, FALSE);
      _mesa_add_renderbuffer(&drawable->stfb->Base, BUFFER_FRONT_LEFT, rb);
   }

   dri_get_buffers(drawable->dPriv);
   st_get_framebuffer_surface(drawable->stfb, ST_SURFACE_FRONT_LEFT, &ps);

   if (!ps)
      return;

   st_bind_texture_surface(ps, target == GL_TEXTURE_2D ? ST_TEXTURE_2D :
                           ST_TEXTURE_RECT, 0, drawable->color_format);
}

void dri2_set_tex_buffer(__DRIcontext *pDRICtx, GLint target,
                         __DRIdrawable *dPriv)
{
   dri2_set_tex_buffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

void
dri_update_buffer(struct pipe_screen *screen, void *context_private)
{
   struct dri_context *ctx = (struct dri_context *)context_private;

   if (ctx->d_stamp == *ctx->dPriv->pStamp &&
       ctx->r_stamp == *ctx->rPriv->pStamp)
      return;

   ctx->d_stamp = *ctx->dPriv->pStamp;
   ctx->r_stamp = *ctx->rPriv->pStamp;

   /* Ask the X server for new renderbuffers. */
   dri_get_buffers(ctx->dPriv);
   if (ctx->dPriv != ctx->rPriv)
      dri_get_buffers(ctx->rPriv);

}

void
dri_flush_frontbuffer(struct pipe_screen *screen,
		      struct pipe_surface *surf, void *context_private)
{
   struct dri_context *ctx = (struct dri_context *)context_private;
   struct dri_drawable *drawable = dri_drawable(ctx->dPriv);
   __DRIdrawable *dri_drawable = ctx->dPriv;
   __DRIscreen *dri_screen = ctx->sPriv;

   /* XXX Does this function get called with DRI1? */

   if (ctx->dPriv == NULL) {
      debug_printf("%s: no drawable bound to context\n", __func__);
      return;
   }

#if 0
   /* TODO if rendering to pixmaps is slow enable this code. */
   if (drawable->is_pixmap)
      return;
#else
   (void)drawable;
#endif

   (*dri_screen->dri2.loader->flushFrontBuffer)(dri_drawable,
						dri_drawable->loaderPrivate);
}

/**
 * This is called when we need to set up GL rendering to a new X window.
 */
boolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const __GLcontextModes * visual, boolean isPixmap)
{
   struct dri_screen *screen = sPriv->private;
   struct dri_drawable *drawable = NULL;
   int i;

   if (isPixmap)
      goto fail;		       /* not implemented */

   drawable = CALLOC_STRUCT(dri_drawable);
   if (drawable == NULL)
      goto fail;

   if (visual->redBits == 8) {
      if (visual->alphaBits == 8)
         drawable->color_format = PIPE_FORMAT_B8G8R8A8_UNORM;
      else
         drawable->color_format = PIPE_FORMAT_B8G8R8X8_UNORM;
   } else {
      drawable->color_format = PIPE_FORMAT_B5G6R5_UNORM;
   }

   switch(visual->depthBits) {
   default:
   case 0:
      drawable->depth_stencil_format = PIPE_FORMAT_NONE;
      break;
   case 16:
      drawable->depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
      break;
   case 24:
      if (visual->stencilBits == 0) {
	 drawable->depth_stencil_format = (screen->d_depth_bits_last) ?
                                          PIPE_FORMAT_Z24X8_UNORM:
                                          PIPE_FORMAT_X8Z24_UNORM;
      } else {
	 drawable->depth_stencil_format = (screen->sd_depth_bits_last) ?
                                          PIPE_FORMAT_Z24S8_UNORM:
                                          PIPE_FORMAT_S8Z24_UNORM;
      }
      break;
   case 32:
      drawable->depth_stencil_format = PIPE_FORMAT_Z32_UNORM;
      break;
   }

   drawable->stfb = st_create_framebuffer(visual,
					  drawable->color_format,
					  drawable->depth_stencil_format,
					  drawable->depth_stencil_format,
					  dPriv->w,
					  dPriv->h, (void *)drawable);
   if (drawable->stfb == NULL)
      goto fail;

   drawable->sPriv = sPriv;
   drawable->dPriv = dPriv;
   dPriv->driverPrivate = (void *)drawable;

   /* setup dri2 buffers information */
   /* TODO incase of double buffer visual, delay fake creation */
   i = 0;
   if (sPriv->dri2.loader
       && (sPriv->dri2.loader->base.version > 2)
       && (sPriv->dri2.loader->getBuffersWithFormat != NULL)) {
      drawable->attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      drawable->attachments[i++] = visual->rgbBits;
      if (!screen->auto_fake_front)  {
         drawable->attachments[i++] = __DRI_BUFFER_FAKE_FRONT_LEFT;
         drawable->attachments[i++] = visual->rgbBits;
      }
      if (visual->doubleBufferMode) {
         drawable->attachments[i++] = __DRI_BUFFER_BACK_LEFT;
         drawable->attachments[i++] = visual->rgbBits;
      }
      if (visual->depthBits && visual->stencilBits) {
         drawable->attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
         drawable->attachments[i++] = visual->depthBits + visual->stencilBits;
      } else if (visual->depthBits) {
         drawable->attachments[i++] = __DRI_BUFFER_DEPTH;
         drawable->attachments[i++] = visual->depthBits;
      } else if (visual->stencilBits) {
         drawable->attachments[i++] = __DRI_BUFFER_STENCIL;
         drawable->attachments[i++] = visual->stencilBits;
      }
      drawable->num_attachments = i / 2;
   } else {
      drawable->attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      if (!screen->auto_fake_front)
         drawable->attachments[i++] = __DRI_BUFFER_FAKE_FRONT_LEFT;
      if (visual->doubleBufferMode)
         drawable->attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      if (visual->depthBits && visual->stencilBits)
         drawable->attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
      else if (visual->depthBits)
         drawable->attachments[i++] = __DRI_BUFFER_DEPTH;
      else if (visual->stencilBits)
         drawable->attachments[i++] = __DRI_BUFFER_STENCIL;
      drawable->num_attachments = i;
   }

   drawable->desired_fences = 2;

   return GL_TRUE;
fail:
   FREE(drawable);
   return GL_FALSE;
}

void
dri_destroy_buffer(__DRIdrawable * dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);

   st_unreference_framebuffer(drawable->stfb);
   drawable->desired_fences = 0;

   dri1_swap_fences_clear(drawable);

   FREE(drawable);
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
