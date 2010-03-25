/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@vmware.com>
 *    Jakob Bornecrantz <wallbraker@gmail.com>
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_debug.h"
#include "state_tracker/drm_api.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"
#include "dri2.h"

/**
 * DRI2 flush extension.
 */
static void
dri2_flush_drawable(__DRIdrawable *draw)
{
}

static void
dri2_invalidate_drawable(__DRIdrawable *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_context *ctx = dri_context(dPriv->driContextPriv);

   dri2InvalidateDrawable(dPriv);
   drawable->dPriv->lastStamp = *drawable->dPriv->pStamp;

   if (ctx)
      ctx->st->notify_invalid_framebuffer(ctx->st, drawable->stfb);
}

static const __DRI2flushExtension dri2FlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    dri2_flush_drawable,
    dri2_invalidate_drawable,
};

/**
 * These are used for GLX_EXT_texture_from_pixmap
 */
static void
dri2_set_tex_buffer2(__DRIcontext *pDRICtx, GLint target,
                     GLint format, __DRIdrawable *dPriv)
{
   struct dri_context *ctx = dri_context(pDRICtx);
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_texture *pt;

   dri_st_framebuffer_validate_att(drawable->stfb, ST_ATTACHMENT_FRONT_LEFT);

   pt = drawable->textures[ST_ATTACHMENT_FRONT_LEFT];

   if (pt) {
      ctx->st->teximage(ctx->st,
            (target == GL_TEXTURE_2D) ? ST_TEXTURE_2D : ST_TEXTURE_RECT,
            0, drawable->stvis.color_format, pt, FALSE);
   }
}

static void
dri2_set_tex_buffer(__DRIcontext *pDRICtx, GLint target,
                    __DRIdrawable *dPriv)
{
   dri2_set_tex_buffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

static const __DRItexBufferExtension dri2TexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   dri2_set_tex_buffer,
   dri2_set_tex_buffer2,
};

/**
 * Get the format of an attachment.
 */
static INLINE enum pipe_format
dri_drawable_get_format(struct dri_drawable *drawable,
                        enum st_attachment_type statt)
{
   enum pipe_format format;

   switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
   case ST_ATTACHMENT_BACK_LEFT:
   case ST_ATTACHMENT_FRONT_RIGHT:
   case ST_ATTACHMENT_BACK_RIGHT:
      format = drawable->stvis.color_format;
      break;
   case ST_ATTACHMENT_DEPTH_STENCIL:
      format = drawable->stvis.depth_stencil_format;
      break;
   default:
      format = PIPE_FORMAT_NONE;
      break;
   }

   return format;
}

/**
 * Retrieve __DRIbuffer from the DRI loader.
 */
static __DRIbuffer *
dri_drawable_get_buffers(struct dri_drawable *drawable,
                         const enum st_attachment_type *statts,
                         unsigned *count)
{
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct __DRIdri2LoaderExtensionRec *loader = drawable->sPriv->dri2.loader;
   boolean with_format;
   __DRIbuffer *buffers;
   int num_buffers;
   unsigned attachments[10];
   unsigned num_attachments, i;

   assert(loader);
   with_format = dri_with_format(drawable->sPriv);

   num_attachments = 0;

   /* for Xserver 1.6.0 (DRI2 version 1) we always need to ask for the front */
   if (!with_format)
      attachments[num_attachments++] = __DRI_BUFFER_FRONT_LEFT;

   for (i = 0; i < *count; i++) {
      enum pipe_format format;
      int att, bpp;

      format = dri_drawable_get_format(drawable, statts[i]);
      if (format == PIPE_FORMAT_NONE)
         continue;

      switch (statts[i]) {
      case ST_ATTACHMENT_FRONT_LEFT:
         /* already added */
         if (!with_format)
            continue;
         att = __DRI_BUFFER_FRONT_LEFT;
         break;
      case ST_ATTACHMENT_BACK_LEFT:
         att = __DRI_BUFFER_BACK_LEFT;
         break;
      case ST_ATTACHMENT_FRONT_RIGHT:
         att = __DRI_BUFFER_FRONT_RIGHT;
         break;
      case ST_ATTACHMENT_BACK_RIGHT:
         att = __DRI_BUFFER_BACK_RIGHT;
         break;
      case ST_ATTACHMENT_DEPTH_STENCIL:
         att = __DRI_BUFFER_DEPTH_STENCIL;
         break;
      default:
         att = -1;
         break;
      }

      bpp = util_format_get_blocksizebits(format);

      if (att >= 0) {
         attachments[num_attachments++] = att;
         if (with_format) {
            attachments[num_attachments++] = bpp;
         }
      }
   }

   if (with_format) {
      num_attachments /= 2;
      buffers = loader->getBuffersWithFormat(dri_drawable,
            &dri_drawable->w, &dri_drawable->h,
            attachments, num_attachments,
            &num_buffers, dri_drawable->loaderPrivate);
   }
   else {
      buffers = loader->getBuffers(dri_drawable,
            &dri_drawable->w, &dri_drawable->h,
            attachments, num_attachments,
            &num_buffers, dri_drawable->loaderPrivate);
   }

   if (buffers) {
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

      *count = num_buffers;
   }

   return buffers;
}

/**
 * Process __DRIbuffer and convert them into pipe_textures.
 */
static void
dri_drawable_process_buffers(struct dri_drawable *drawable,
                             __DRIbuffer *buffers, unsigned count)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct pipe_texture templ;
   struct winsys_handle whandle;
   boolean have_depth = FALSE;
   unsigned i;

   if (drawable->old_num == count &&
       drawable->old_w == dri_drawable->w &&
       drawable->old_h == dri_drawable->h &&
       memcmp(drawable->old, buffers, sizeof(__DRIbuffer) * count) == 0)
      return;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_texture_reference(&drawable->textures[i], NULL);

   memset(&templ, 0, sizeof(templ));
   templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = dri_drawable->w;
   templ.height0 = dri_drawable->h;
   templ.depth0 = 1;

   memset(&whandle, 0, sizeof(whandle));

   for (i = 0; i < count; i++) {
      __DRIbuffer *buf = &buffers[i];
      enum st_attachment_type statt;
      enum pipe_format format;

      switch (buf->attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
         if (!screen->auto_fake_front) {
            statt = ST_ATTACHMENT_INVALID;
            break;
         }
         /* fallthrough */
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
         statt = ST_ATTACHMENT_FRONT_LEFT;
         break;
      case __DRI_BUFFER_BACK_LEFT:
         statt = ST_ATTACHMENT_BACK_LEFT;
         break;
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_STENCIL:
         /* use only the first depth/stencil buffer */
         if (!have_depth) {
            have_depth = TRUE;
            statt = ST_ATTACHMENT_DEPTH_STENCIL;
         }
         else {
            statt = ST_ATTACHMENT_INVALID;
         }
         break;
      default:
         statt = ST_ATTACHMENT_INVALID;
         break;
      }

      format = dri_drawable_get_format(drawable, statt);
      if (statt == ST_ATTACHMENT_INVALID || format == PIPE_FORMAT_NONE)
         continue;

      templ.format = format;
      whandle.handle = buf->name;
      whandle.stride = buf->pitch;

      drawable->textures[statt] =
         screen->pipe_screen->texture_from_handle(screen->pipe_screen,
               &templ, &whandle);
   }

   drawable->old_num = count;
   drawable->old_w = dri_drawable->w;
   drawable->old_h = dri_drawable->h;
   memcpy(drawable->old, buffers, sizeof(__DRIbuffer) * count);
}

/*
 * Backend functions for st_framebuffer interface.
 */

void
dri_allocate_textures(struct dri_drawable *drawable,
                      const enum st_attachment_type *statts,
                      unsigned count)
{
   __DRIbuffer *buffers;
   unsigned num_buffers = count;

   buffers = dri_drawable_get_buffers(drawable, statts, &num_buffers);
   dri_drawable_process_buffers(drawable, buffers, num_buffers);
}

void
dri_flush_frontbuffer(struct dri_drawable *drawable,
                      enum st_attachment_type statt)
{
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct __DRIdri2LoaderExtensionRec *loader = drawable->sPriv->dri2.loader;

   if (loader->flushFrontBuffer == NULL)
      return;

   if (statt == ST_ATTACHMENT_FRONT_LEFT) {
      loader->flushFrontBuffer(dri_drawable, dri_drawable->loaderPrivate);
   }
}

/*
 * Backend function init_screen.
 */

static const __DRIextension *dri_screen_extensions[] = {
   &driReadDrawableExtension,
   &driCopySubBufferExtension.base,
   &driSwapControlExtension.base,
   &driFrameTrackingExtension.base,
   &driMediaStreamCounterExtension.base,
   &dri2TexBufferExtension.base,
   &dri2FlushExtension.base,
   NULL
};

/**
 * This is the driver specific part of the createNewScreen entry point.
 *
 * Returns the __GLcontextModes supported by this driver.
 */
const __DRIconfig **
dri_init_screen2(__DRIscreen * sPriv)
{
   struct dri_screen *screen;
   struct drm_create_screen_arg arg;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->api = drm_api_create();
   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   sPriv->private = (void *)screen;
   sPriv->extensions = dri_screen_extensions;
   arg.mode = DRM_CREATE_NORMAL;

   screen->pipe_screen = screen->api->create_screen(screen->api, screen->fd, &arg);
   if (!screen->pipe_screen) {
      debug_printf("%s: failed to create pipe_screen\n", __FUNCTION__);
      goto fail;
   }

   screen->smapi = dri_create_st_manager(screen);
   if (!screen->smapi)
      goto fail;

   driParseOptionInfo(&screen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   screen->auto_fake_front = dri_with_format(sPriv);

   return dri_fill_in_modes(screen, 32);
fail:
   dri_destroy_screen(sPriv);
   return NULL;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
