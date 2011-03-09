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
#include "state_tracker/drm_driver.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri2_buffer.h"

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
   struct dri_context *ctx = drawable->context;

   dri2InvalidateDrawable(dPriv);
   drawable->dPriv->lastStamp = *drawable->dPriv->pStamp;

   if (ctx)
      ctx->st->notify_invalid_framebuffer(ctx->st, &drawable->base);
}

static const __DRI2flushExtension dri2FlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    dri2_flush_drawable,
    dri2_invalidate_drawable,
};

/**
 * Retrieve __DRIbuffer from the DRI loader.
 */
static __DRIbuffer *
dri2_drawable_get_buffers(struct dri_drawable *drawable,
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
      unsigned bind;
      int att, bpp;

      dri_drawable_get_format(drawable, statts[i], &format, &bind);
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
 * Process __DRIbuffer and convert them into pipe_resources.
 */
static void
dri2_drawable_process_buffers(struct dri_drawable *drawable,
                              __DRIbuffer *buffers, unsigned count)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct pipe_resource templ;
   struct winsys_handle whandle;
   boolean have_depth = FALSE;
   unsigned i, bind;

   if (drawable->old_num == count &&
       drawable->old_w == dri_drawable->w &&
       drawable->old_h == dri_drawable->h &&
       memcmp(drawable->old, buffers, sizeof(__DRIbuffer) * count) == 0)
      return;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_resource_reference(&drawable->textures[i], NULL);

   memset(&templ, 0, sizeof(templ));
   templ.target = screen->target;
   templ.last_level = 0;
   templ.width0 = dri_drawable->w;
   templ.height0 = dri_drawable->h;
   templ.depth0 = 1;
   templ.array_size = 1;

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

      dri_drawable_get_format(drawable, statt, &format, &bind);
      if (statt == ST_ATTACHMENT_INVALID || format == PIPE_FORMAT_NONE)
         continue;

      templ.format = format;
      templ.bind = bind;
      whandle.handle = buf->name;
      whandle.stride = buf->pitch;

      drawable->textures[statt] =
         screen->base.screen->resource_from_handle(screen->base.screen,
               &templ, &whandle);
   }

   drawable->old_num = count;
   drawable->old_w = dri_drawable->w;
   drawable->old_h = dri_drawable->h;
   memcpy(drawable->old, buffers, sizeof(__DRIbuffer) * count);
}

static __DRIbuffer *
dri2_allocate_buffer(__DRIscreen *sPriv,
                     unsigned attachment, unsigned format,
                     int width, int height)
{
   struct dri_screen *screen = dri_screen(sPriv);
   struct dri2_buffer *buffer;
   struct pipe_resource templ;
   enum st_attachment_type statt;
   enum pipe_format pf;
   unsigned bind = 0;
   struct winsys_handle whandle;

   switch (attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
         statt = ST_ATTACHMENT_FRONT_LEFT;
         bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
         break;
      case __DRI_BUFFER_BACK_LEFT:
         statt = ST_ATTACHMENT_BACK_LEFT;
         bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
         break;
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_STENCIL:
            statt = ST_ATTACHMENT_DEPTH_STENCIL;
            bind = PIPE_BIND_DEPTH_STENCIL; /* XXX sampler? */
         break;
      default:
         statt = ST_ATTACHMENT_INVALID;
         break;
   }

   switch (format) {
      case 32:
         pf = PIPE_FORMAT_B8G8R8X8_UNORM;
         break;
      case 16:
         pf = PIPE_FORMAT_Z16_UNORM;
         break;
      default:
         return NULL;
   }

   buffer = CALLOC_STRUCT(dri2_buffer);
   if (!buffer)
      return NULL;

   memset(&templ, 0, sizeof(templ));
   templ.bind = bind;
   templ.format = pf;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;

   buffer->resource =
      screen->base.screen->resource_create(screen->base.screen, &templ);
   if (!buffer->resource)
      return NULL;

   memset(&whandle, 0, sizeof(whandle));
   whandle.type = DRM_API_HANDLE_TYPE_SHARED;
   screen->base.screen->resource_get_handle(screen->base.screen,
         buffer->resource, &whandle);

   buffer->base.attachment = attachment;
   buffer->base.name = whandle.handle;
   buffer->base.cpp = util_format_get_blocksize(pf);
   buffer->base.pitch = whandle.stride;

   return &buffer->base;
}

static void
dri2_release_buffer(__DRIscreen *sPriv, __DRIbuffer *bPriv)
{
   struct dri2_buffer *buffer = dri2_buffer(bPriv);

   pipe_resource_reference(&buffer->resource, NULL);
   FREE(buffer);
}

/*
 * Backend functions for st_framebuffer interface.
 */

static void
dri2_allocate_textures(struct dri_drawable *drawable,
                       const enum st_attachment_type *statts,
                       unsigned count)
{
   __DRIbuffer *buffers;
   unsigned num_buffers = count;

   buffers = dri2_drawable_get_buffers(drawable, statts, &num_buffers);
   if (buffers)
      dri2_drawable_process_buffers(drawable, buffers, num_buffers);
}

static void
dri2_flush_frontbuffer(struct dri_drawable *drawable,
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

static __DRIimage *
dri2_lookup_egl_image(struct dri_screen *screen, void *handle)
{
   __DRIimageLookupExtension *loader = screen->sPriv->dri2.image;
   __DRIimage *img;

   if (!loader->lookupEGLImage)
      return NULL;

   img = loader->lookupEGLImage(screen->sPriv,
				handle, screen->sPriv->loaderPrivate);

   return img;
}

static __DRIimage *
dri2_create_image_from_name(__DRIscreen *_screen,
                            int width, int height, int format,
                            int name, int pitch, void *loaderPrivate)
{
   struct dri_screen *screen = dri_screen(_screen);
   __DRIimage *img;
   struct pipe_resource templ;
   struct winsys_handle whandle;
   unsigned tex_usage;
   enum pipe_format pf;

   tex_usage = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;

   switch (format) {
   case __DRI_IMAGE_FORMAT_RGB565:
      pf = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   case __DRI_IMAGE_FORMAT_XRGB8888:
      pf = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   case __DRI_IMAGE_FORMAT_ARGB8888:
      pf = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      pf = PIPE_FORMAT_NONE;
      break;
   }
   if (pf == PIPE_FORMAT_NONE)
      return NULL;

   img = CALLOC_STRUCT(__DRIimageRec);
   if (!img)
      return NULL;

   memset(&templ, 0, sizeof(templ));
   templ.bind = tex_usage;
   templ.format = pf;
   templ.target = screen->target;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;

   memset(&whandle, 0, sizeof(whandle));
   whandle.handle = name;
   whandle.stride = pitch * util_format_get_blocksize(pf);

   img->texture = screen->base.screen->resource_from_handle(screen->base.screen,
         &templ, &whandle);
   if (!img->texture) {
      FREE(img);
      return NULL;
   }

   img->level = 0;
   img->layer = 0;
   img->loader_private = loaderPrivate;

   return img;
}

static __DRIimage *
dri2_create_image_from_renderbuffer(__DRIcontext *context,
				    int renderbuffer, void *loaderPrivate)
{
   struct dri_context *ctx = dri_context(context);

   if (!ctx->st->get_resource_for_egl_image)
      return NULL;

   /* TODO */
   return NULL;
}

static __DRIimage *
dri2_create_image(__DRIscreen *_screen,
                   int width, int height, int format,
                   unsigned int use, void *loaderPrivate)
{
   struct dri_screen *screen = dri_screen(_screen);
   __DRIimage *img;
   struct pipe_resource templ;
   unsigned tex_usage;
   enum pipe_format pf;

   tex_usage = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   if (use & __DRI_IMAGE_USE_SCANOUT)
      tex_usage |= PIPE_BIND_SCANOUT;
   if (use & __DRI_IMAGE_USE_SHARE)
      tex_usage |= PIPE_BIND_SHARED;
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (width != 64 || height != 64)
         return NULL;
      tex_usage |= PIPE_BIND_CURSOR;
   }

   switch (format) {
   case __DRI_IMAGE_FORMAT_RGB565:
      pf = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   case __DRI_IMAGE_FORMAT_XRGB8888:
      pf = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   case __DRI_IMAGE_FORMAT_ARGB8888:
      pf = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      pf = PIPE_FORMAT_NONE;
      break;
   }
   if (pf == PIPE_FORMAT_NONE)
      return NULL;

   img = CALLOC_STRUCT(__DRIimageRec);
   if (!img)
      return NULL;

   memset(&templ, 0, sizeof(templ));
   templ.bind = tex_usage;
   templ.format = pf;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;

   img->texture = screen->base.screen->resource_create(screen->base.screen, &templ);
   if (!img->texture) {
      FREE(img);
      return NULL;
   }

   img->level = 0;
   img->layer = 0;

   img->loader_private = loaderPrivate;
   return img;
}

static GLboolean
dri2_query_image(__DRIimage *image, int attrib, int *value)
{
   struct winsys_handle whandle;
   memset(&whandle, 0, sizeof(whandle));

   switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      image->texture->screen->resource_get_handle(image->texture->screen,
            image->texture, &whandle);
      *value = whandle.stride;
      return GL_TRUE;
   case __DRI_IMAGE_ATTRIB_HANDLE:
      whandle.type = DRM_API_HANDLE_TYPE_KMS;
      image->texture->screen->resource_get_handle(image->texture->screen,
         image->texture, &whandle);
      *value = whandle.handle;
      return GL_TRUE;
   case __DRI_IMAGE_ATTRIB_NAME:
      whandle.type = DRM_API_HANDLE_TYPE_SHARED;
      image->texture->screen->resource_get_handle(image->texture->screen,
         image->texture, &whandle);
      *value = whandle.handle;
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}

static __DRIimage *
dri2_dup_image(__DRIimage *image, void *loaderPrivate)
{
   __DRIimage *img;

   img = CALLOC_STRUCT(__DRIimageRec);
   if (!img)
      return NULL;

   img->texture = NULL;
   pipe_resource_reference(&img->texture, image->texture);
   img->level = image->level;
   img->layer = image->layer;
   img->loader_private = loaderPrivate;

   return img;
}

static void
dri2_destroy_image(__DRIimage *img)
{
   pipe_resource_reference(&img->texture, NULL);
   FREE(img);
}

static struct __DRIimageExtensionRec dri2ImageExtension = {
    { __DRI_IMAGE, __DRI_IMAGE_VERSION },
    dri2_create_image_from_name,
    dri2_create_image_from_renderbuffer,
    dri2_destroy_image,
    dri2_create_image,
    dri2_query_image,
    dri2_dup_image,
};

/*
 * Backend function init_screen.
 */

static const __DRIextension *dri_screen_extensions[] = {
   &driReadDrawableExtension,
   &driCopySubBufferExtension.base,
   &driSwapControlExtension.base,
   &driMediaStreamCounterExtension.base,
   &driTexBufferExtension.base,
   &dri2FlushExtension.base,
   &dri2ImageExtension.base,
   &dri2ConfigQueryExtension.base,
   NULL
};

/**
 * This is the driver specific part of the createNewScreen entry point.
 *
 * Returns the struct gl_config supported by this driver.
 */
static const __DRIconfig **
dri2_init_screen(__DRIscreen * sPriv)
{
   const __DRIconfig **configs;
   struct dri_screen *screen;
   struct pipe_screen *pscreen;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;

   sPriv->private = (void *)screen;
   sPriv->extensions = dri_screen_extensions;

   pscreen = driver_descriptor.create_screen(screen->fd);
   /* dri_init_screen_helper checks pscreen for us */

   configs = dri_init_screen_helper(screen, pscreen, 32);
   if (!configs)
      goto fail;

   sPriv->api_mask = 0;
   if (screen->st_api->profile_mask & ST_PROFILE_DEFAULT_MASK)
      sPriv->api_mask |= 1 << __DRI_API_OPENGL;
   if (screen->st_api->profile_mask & ST_PROFILE_OPENGL_ES1_MASK)
      sPriv->api_mask |= 1 << __DRI_API_GLES;
   if (screen->st_api->profile_mask & ST_PROFILE_OPENGL_ES2_MASK)
      sPriv->api_mask |= 1 << __DRI_API_GLES2;

   screen->auto_fake_front = dri_with_format(sPriv);
   screen->broken_invalidate = !sPriv->dri2.useInvalidate;
   screen->lookup_egl_image = dri2_lookup_egl_image;

   return configs;
fail:
   dri_destroy_screen_helper(screen);
   FREE(screen);
   return NULL;
}

static boolean
dri2_create_context(gl_api api, const struct gl_config * visual,
                    __DRIcontext * cPriv, void *sharedContextPrivate)
{
   struct dri_context *ctx = NULL;

   if (!dri_create_context(api, visual, cPriv, sharedContextPrivate))
      return FALSE;

   ctx = cPriv->driverPrivate;

   return TRUE;
}

static boolean
dri2_create_buffer(__DRIscreen * sPriv,
                   __DRIdrawable * dPriv,
                   const struct gl_config * visual, boolean isPixmap)
{
   struct dri_drawable *drawable = NULL;

   if (!dri_create_buffer(sPriv, dPriv, visual, isPixmap))
      return FALSE;

   drawable = dPriv->driverPrivate;

   drawable->allocate_textures = dri2_allocate_textures;
   drawable->flush_frontbuffer = dri2_flush_frontbuffer;

   return TRUE;
}

/**
 * DRI driver virtual function table.
 *
 * DRI versions differ in their implementation of init_screen and swap_buffers.
 */
const struct __DriverAPIRec driDriverAPI = {
   .InitScreen = NULL,
   .InitScreen2 = dri2_init_screen,
   .DestroyScreen = dri_destroy_screen,
   .CreateContext = dri2_create_context,
   .DestroyContext = dri_destroy_context,
   .CreateBuffer = dri2_create_buffer,
   .DestroyBuffer = dri_destroy_buffer,
   .MakeCurrent = dri_make_current,
   .UnbindContext = dri_unbind_context,

   .GetSwapInfo = NULL,
   .GetDrawableMSC = NULL,
   .WaitForMSC = NULL,

   .SwapBuffers = NULL,
   .CopySubBuffer = NULL,

   .AllocateBuffer = dri2_allocate_buffer,
   .ReleaseBuffer  = dri2_release_buffer,
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driLegacyExtension.base,
    &driDRI2Extension.base,
    NULL
};

/* vim: set sw=3 ts=8 sts=3 expandtab: */
