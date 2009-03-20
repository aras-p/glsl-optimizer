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
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"

#include "util/u_memory.h"


static void
dri_copy_to_front(__DRIdrawablePrivate *dPriv,
                  struct pipe_surface *from,
                  int x, int y, unsigned w, unsigned h)
{
   /* TODO send a message to the Xserver to copy to the real front buffer */
}


static struct pipe_surface *
dri_surface_from_handle(struct pipe_screen *screen,
                        unsigned handle,
                        enum pipe_format format,
                        unsigned width,
                        unsigned height,
                        unsigned pitch)
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

   texture = screen->texture_blanket(screen,
                                     &templat,
                                     &pitch,
                                     buf);

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
dri_get_buffers(__DRIdrawablePrivate *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_surface *surface = NULL;
   struct pipe_screen *screen = dri_screen(drawable->sPriv)->pipe_screen;
   __DRIbuffer *buffers = NULL;
   __DRIscreen *dri_screen = drawable->sPriv;
   __DRIdrawable *dri_drawable = drawable->dPriv;
   boolean have_depth = FALSE;
   int i, count;

   buffers = (*dri_screen->dri2.loader->getBuffers)(dri_drawable,
                                                    &dri_drawable->w,
                                                    &dri_drawable->h,
                                                    drawable->attachments,
                                                    drawable->num_attachments,
                                                    &count,
                                                    dri_drawable->loaderPrivate);

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
                                        dri_drawable->h,
                                        buffers[i].pitch);

      st_set_framebuffer_surface(drawable->stfb, index, surface);
      pipe_surface_reference(&surface, NULL);
   }
   /* this needed, or else the state tracker fails to pick the new buffers */
   st_resize_framebuffer(drawable->stfb, dri_drawable->w, dri_drawable->h);
}


void
dri_flush_frontbuffer(struct pipe_screen *screen,
                      struct pipe_surface *surf,
                      void *context_private)
{
   struct dri_context *ctx = (struct dri_context *)context_private;
   dri_copy_to_front(ctx->dPriv, surf, 0, 0, surf->width, surf->height);
}


void
dri_swap_buffers(__DRIdrawablePrivate * dPriv)
{
   /* not needed for dri2 */
   assert(0);
}


void
dri_copy_sub_buffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h)
{
   /* not needed for dri2 */
   assert(0);
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
boolean
dri_create_buffer(__DRIscreenPrivate *sPriv,
                  __DRIdrawablePrivate *dPriv,
                  const __GLcontextModes *visual,
                  boolean isPixmap)
{
   enum pipe_format colorFormat, depthFormat, stencilFormat;
   struct dri_screen *screen = sPriv->private;
   struct dri_drawable *drawable = NULL;
   struct pipe_screen *pscreen = screen->pipe_screen;
   int i;

   if (isPixmap)
      goto fail; /* not implemented */

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
                                       PIPE_TEXTURE_USAGE_RENDER_TARGET |
                                       PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0))
         depthFormat = PIPE_FORMAT_Z24S8_UNORM;
      else
         depthFormat = PIPE_FORMAT_S8Z24_UNORM;
   } else
      depthFormat = PIPE_FORMAT_NONE;

   if (visual->stencilBits) {
      if (pscreen->is_format_supported(pscreen, PIPE_FORMAT_Z24S8_UNORM,
                                       PIPE_TEXTURE_2D,
                                       PIPE_TEXTURE_USAGE_RENDER_TARGET |
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
                                          dPriv->h,
                                          (void*) drawable);
   if (drawable->stfb == NULL)
      goto fail;

   drawable->sPriv = sPriv;
   drawable->dPriv = dPriv;
   dPriv->driverPrivate = (void *) drawable;

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

   return GL_TRUE;
fail:
   FREE(drawable);
   return GL_FALSE;
}


void
dri_destroy_buffer(__DRIdrawablePrivate *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);

   st_unreference_framebuffer(drawable->stfb);

   FREE(drawable);
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
