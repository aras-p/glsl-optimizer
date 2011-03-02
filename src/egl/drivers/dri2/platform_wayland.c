/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>

#include "egl_dri2.h"

#include <wayland-client.h>
#include "wayland-drm-client-protocol.h"

static void
sync_callback(void *data)
{
   int *done = data;

   *done = 1;
}

static void
force_roundtrip(struct wl_display *display)
{
   int done = 0;

   wl_display_sync_callback(display, sync_callback, &done);
   wl_display_iterate(display, WL_DISPLAY_WRITABLE);
   while (!done)
      wl_display_iterate(display, WL_DISPLAY_READABLE);
}


/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
dri2_create_surface(_EGLDriver *drv, _EGLDisplay *disp, EGLint type,
		    _EGLConfig *conf, EGLNativeWindowType window,
		    const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   struct dri2_egl_buffer *dri2_buf;
   int i;

   (void) drv;

   dri2_surf = malloc(sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }
   
   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surf;

   for (i = 0; i < WL_BUFFER_COUNT; ++i)
      dri2_surf->wl_drm_buffer[i] = NULL;

   for (i = 0; i < __DRI_BUFFER_COUNT; ++i)
      dri2_surf->dri_buffers[i] = NULL;

   dri2_surf->pending_buffer = NULL;
   dri2_surf->block_swap_buffers = EGL_FALSE;

   switch (type) {
   case EGL_WINDOW_BIT:
      dri2_surf->wl_win = (struct wl_egl_window *) window;
      dri2_surf->type = DRI2_WINDOW_SURFACE;

      dri2_surf->base.Width =  -1;
      dri2_surf->base.Height = -1;
      break;
   case EGL_PIXMAP_BIT:
      dri2_surf->wl_pix = (struct wl_egl_pixmap *) window;
      dri2_surf->type = DRI2_PIXMAP_SURFACE;

      dri2_surf->base.Width  = dri2_surf->wl_pix->width;
      dri2_surf->base.Height = dri2_surf->wl_pix->height;

      if (dri2_surf->wl_pix->name > 0) {
         dri2_buf = dri2_surf->wl_pix->driver_private;
         dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT] = dri2_buf->dri_buffer;
      }
      break;
   default: 
      goto cleanup_surf;
   }

   dri2_surf->dri_drawable = 
      (*dri2_dpy->dri2->createNewDrawable) (dri2_dpy->dri_screen,
					    type == EGL_WINDOW_BIT ?
					    dri2_conf->dri_double_config : 
					    dri2_conf->dri_single_config,
					    dri2_surf);
   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_dri_drawable;
   }

   return &dri2_surf->base;

 cleanup_dri_drawable:
   dri2_dpy->core->destroyDrawable(dri2_surf->dri_drawable);
 cleanup_surf:
   free(dri2_surf);

   return NULL;
}

/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
dri2_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativeWindowType window,
			   const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_WINDOW_BIT, conf,
			      window, attrib_list);
}

static _EGLSurface *
dri2_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativePixmapType pixmap,
			   const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_PIXMAP_BIT, conf,
			      pixmap, attrib_list);
}

/**
 * Called via eglDestroySurface(), drv->API.DestroySurface().
 */
static EGLBoolean
dri2_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   int i;

   (void) drv;

   if (!_eglPutSurface(surf))
      return EGL_TRUE;

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);

   for (i = 0; i < WL_BUFFER_COUNT; ++i)
      if (dri2_surf->wl_drm_buffer[i])
         wl_buffer_destroy(dri2_surf->wl_drm_buffer[i]);

   for (i = 0; i < __DRI_BUFFER_COUNT; ++i)
      if (dri2_surf->dri_buffers[i] && !(i == __DRI_BUFFER_FRONT_LEFT &&
          dri2_surf->type == DRI2_PIXMAP_SURFACE))
         dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                       dri2_surf->dri_buffers[i]);

   free(surf);

   return EGL_TRUE;
}

static void
dri2_wl_egl_pixmap_destroy(struct wl_egl_pixmap *egl_pixmap)
{
   struct dri2_egl_buffer *dri2_buf = egl_pixmap->driver_private;

   assert(dri2_buf);

   dri2_buf->dri2_dpy->dri2->releaseBuffer(dri2_buf->dri2_dpy->dri_screen,
                                           dri2_buf->dri_buffer);

   free(dri2_buf);
   
   egl_pixmap->driver_private = NULL;
   egl_pixmap->destroy = NULL;
   egl_pixmap->name = 0;
}

static void
dri2_process_back_buffer(struct dri2_egl_surface *dri2_surf, unsigned format)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   (void) format;

   switch (dri2_surf->type) {
   case DRI2_WINDOW_SURFACE:
      /* allocate a front buffer for our double-buffered window*/
      dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT] = 
         dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
               __DRI_BUFFER_FRONT_LEFT, format,
               dri2_surf->base.Width, dri2_surf->base.Height);
      break;
   default:
      break;
   }
}

static void
dri2_process_front_buffer(struct dri2_egl_surface *dri2_surf, unsigned format)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct dri2_egl_buffer *dri2_buf;

   switch (dri2_surf->type) {
   case DRI2_PIXMAP_SURFACE:
      dri2_buf = malloc(sizeof *dri2_buf);
      if (!dri2_buf)
         return;

      dri2_buf->dri_buffer = dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT];
      dri2_buf->dri2_dpy   = dri2_dpy;

      dri2_surf->wl_pix->name           = dri2_buf->dri_buffer->name;
      dri2_surf->wl_pix->stride         = dri2_buf->dri_buffer->pitch;
      dri2_surf->wl_pix->driver_private = dri2_buf;
      dri2_surf->wl_pix->destroy        = dri2_wl_egl_pixmap_destroy;
      break;
   default:
      break;
   }
}

static void
dri2_release_pending_buffer(void *data)
{
   struct dri2_egl_surface *dri2_surf = data;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   /* FIXME: print internal error */
   if (!dri2_surf->pending_buffer)
      return;
   
   dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                 dri2_surf->pending_buffer);
   dri2_surf->pending_buffer = NULL;
}

static void
dri2_release_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int i;

   for (i = 0; i < __DRI_BUFFER_COUNT; ++i) {
      if (dri2_surf->dri_buffers[i]) {
         switch (i) {
         case __DRI_BUFFER_FRONT_LEFT:
            if (dri2_surf->pending_buffer)
               force_roundtrip(dri2_dpy->wl_dpy->display);
            dri2_surf->pending_buffer = dri2_surf->dri_buffers[i];
            wl_display_sync_callback(dri2_dpy->wl_dpy->display,
                                     dri2_release_pending_buffer, dri2_surf);
            break;
         default:
            dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                          dri2_surf->dri_buffers[i]);
            break;
         }
         dri2_surf->dri_buffers[i] = NULL;
      }
   }
}

static __DRIbuffer *
dri2_get_buffers_with_format(__DRIdrawable * driDrawable,
			     int *width, int *height,
			     unsigned int *attachments, int count,
			     int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int i;

   if (dri2_surf->type == DRI2_WINDOW_SURFACE &&
       (dri2_surf->base.Width != dri2_surf->wl_win->width || 
        dri2_surf->base.Height != dri2_surf->wl_win->height)) {

      dri2_release_buffers(dri2_surf);

      dri2_surf->base.Width  = dri2_surf->wl_win->width;
      dri2_surf->base.Height = dri2_surf->wl_win->height;
      dri2_surf->dx = dri2_surf->wl_win->dx;
      dri2_surf->dy = dri2_surf->wl_win->dy;

      for (i = 0; i < WL_BUFFER_COUNT; ++i) {
         if (dri2_surf->wl_drm_buffer[i])
            wl_buffer_destroy(dri2_surf->wl_drm_buffer[i]);
         dri2_surf->wl_drm_buffer[i]  = NULL;
      }
   }

   dri2_surf->buffer_count = 0;
   for (i = 0; i < 2*count; i+=2) {
      assert(attachments[i] < __DRI_BUFFER_COUNT);
      assert(dri2_surf->buffer_count < 5);

      if (dri2_surf->dri_buffers[attachments[i]] == NULL) {

         dri2_surf->dri_buffers[attachments[i]] =
            dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
                  attachments[i], attachments[i+1],
                  dri2_surf->base.Width, dri2_surf->base.Height);

         if (!dri2_surf->dri_buffers[attachments[i]]) 
            continue;

         if (attachments[i] == __DRI_BUFFER_FRONT_LEFT)
            dri2_process_front_buffer(dri2_surf, attachments[i+1]);
         else if (attachments[i] == __DRI_BUFFER_BACK_LEFT)
            dri2_process_back_buffer(dri2_surf, attachments[i+1]);
      }

      memcpy(&dri2_surf->buffers[dri2_surf->buffer_count],
             dri2_surf->dri_buffers[attachments[i]],
             sizeof(__DRIbuffer));

      dri2_surf->buffer_count++;
   }

   assert(dri2_surf->type == DRI2_PIXMAP_SURFACE ||
          dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT]);

   *out_count = dri2_surf->buffer_count;
   if (dri2_surf->buffer_count == 0)
	   return NULL;

   *width = dri2_surf->base.Width;
   *height = dri2_surf->base.Height;

   return dri2_surf->buffers;
}

static __DRIbuffer *
dri2_get_buffers(__DRIdrawable * driDrawable,
		 int *width, int *height,
		 unsigned int *attachments, int count,
		 int *out_count, void *loaderPrivate)
{
   unsigned int *attachments_with_format;
   __DRIbuffer *buffer;
   const unsigned int format = 32;
   int i;

   attachments_with_format = calloc(count * 2, sizeof(unsigned int));
   if (!attachments_with_format) {
      *out_count = 0;
      return NULL;
   }

   for (i = 0; i < count; ++i) {
      attachments_with_format[2*i] = attachments[i];
      attachments_with_format[2*i + 1] = format;
   }

   buffer =
      dri2_get_buffers_with_format(driDrawable,
				   width, height,
				   attachments_with_format, count,
				   out_count, loaderPrivate);

   free(attachments_with_format);

   return buffer;
}


static void
dri2_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
   (void) driDrawable;

   /* FIXME: Does EGL support front buffer rendering at all? */

#if 0
   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   dri2WaitGL(dri2_surf);
#else
   (void) loaderPrivate;
#endif
}

static struct wl_buffer *
wayland_create_buffer(struct dri2_egl_surface *dri2_surf, __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   return wl_drm_create_buffer(dri2_dpy->wl_dpy->drm, buffer->name,
                               dri2_surf->base.Width, dri2_surf->base.Height,
                               buffer->pitch, dri2_surf->wl_win->visual);
}

static void
wayland_frame_callback(void *data, uint32_t time)
{
   struct dri2_egl_surface *dri2_surf = data;

   dri2_surf->block_swap_buffers = EGL_FALSE;
}

static inline void
pointer_swap(const void **p1, const void **p2)
{
   const void *tmp = *p1;
   *p1 = *p2;
   *p2 = tmp;
}

/**
 * Called via eglSwapBuffers(), drv->API.SwapBuffers().
 */
static EGLBoolean
dri2_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);

   while (dri2_surf->block_swap_buffers)
      wl_display_iterate(dri2_dpy->wl_dpy->display, WL_DISPLAY_READABLE);

   dri2_surf->block_swap_buffers = EGL_TRUE;
   wl_display_frame_callback(dri2_dpy->wl_dpy->display,
	 wayland_frame_callback, dri2_surf);

   if (dri2_surf->type == DRI2_WINDOW_SURFACE) {
      pointer_swap(
	    (const void **) &dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT],
	    (const void **) &dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT]);

      dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT]->attachment = 
	 __DRI_BUFFER_FRONT_LEFT;
      dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT]->attachment = 
	 __DRI_BUFFER_BACK_LEFT;

      pointer_swap((const void **) &dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT],
		   (const void **) &dri2_surf->wl_drm_buffer[WL_BUFFER_BACK]);

      if (!dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT])
	 dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT] =
	    wayland_create_buffer(dri2_surf,
		  dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT]);

      wl_surface_attach(dri2_surf->wl_win->surface,
	    dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT],
	    dri2_surf->dx, dri2_surf->dy);

      dri2_surf->wl_win->attached_width  = dri2_surf->base.Width;
      dri2_surf->wl_win->attached_height = dri2_surf->base.Height;
      /* reset resize growing parameters */
      dri2_surf->dx = 0;
      dri2_surf->dy = 0;

      wl_surface_damage(dri2_surf->wl_win->surface, 0, 0,
	    dri2_surf->base.Width, dri2_surf->base.Height);
   }

   _EGLContext *ctx;
   if (dri2_drv->glFlush) {
      ctx = _eglGetCurrentContext();
      if (ctx && ctx->DrawSurface == &dri2_surf->base)
         dri2_drv->glFlush();
   }

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);
   (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

/**
 * Called via eglCreateImageKHR(), drv->API.CreateImageKHR().
 */
static _EGLImage *
dri2_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
			     EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct wl_egl_pixmap *wl_egl_pixmap = (struct wl_egl_pixmap *) buffer;
   struct dri2_egl_buffer *dri2_buf;
   EGLint wl_attr_list[] = {
		EGL_WIDTH,		0,
		EGL_HEIGHT,		0,
		EGL_DRM_BUFFER_STRIDE_MESA,	0,
		EGL_DRM_BUFFER_FORMAT_MESA,	EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_NONE
   };

   dri2_buf = malloc(sizeof *dri2_buf);
   if (!dri2_buf)
           return NULL;

   dri2_buf->dri2_dpy = dri2_dpy;
   dri2_buf->dri_buffer =
      dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
				     __DRI_BUFFER_FRONT_LEFT, 32,
				     wl_egl_pixmap->width,
				     wl_egl_pixmap->height);

   wl_egl_pixmap->name    = dri2_buf->dri_buffer->name;
   wl_egl_pixmap->stride  = dri2_buf->dri_buffer->pitch;
   wl_egl_pixmap->destroy = dri2_wl_egl_pixmap_destroy;
   wl_egl_pixmap->driver_private = dri2_buf;

   wl_attr_list[1] = wl_egl_pixmap->width;
   wl_attr_list[3] = wl_egl_pixmap->height;
   wl_attr_list[5] = wl_egl_pixmap->stride / 4;


   return dri2_create_image_khr(disp->Driver, disp, ctx, EGL_DRM_BUFFER_MESA,
	       (EGLClientBuffer)(intptr_t) wl_egl_pixmap->name, wl_attr_list);
}

static _EGLImage *
dri2_wayland_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
			      _EGLContext *ctx, EGLenum target,
			      EGLClientBuffer buffer, const EGLint *attr_list)
{
   (void) drv;

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      return dri2_create_image_khr_pixmap(disp, ctx, buffer, attr_list);
   default:
      return dri2_create_image_khr(drv, disp, ctx, target, buffer, attr_list);
   }
}

static int
dri2_wayland_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   int ret = 0;

   dri2_dpy->wl_dpy->authenticated = false;

   wl_drm_authenticate(dri2_dpy->wl_dpy->drm, id);
   force_roundtrip(dri2_dpy->wl_dpy->display);

   if (!dri2_dpy->wl_dpy->authenticated)
      ret = -1;

   /* reset authenticated */
   dri2_dpy->wl_dpy->authenticated = true;

   return ret;
}

/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
dri2_terminate(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   _eglReleaseDisplayResources(drv, disp);
   _eglCleanupDisplay(disp);

   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
   close(dri2_dpy->fd);
   dlclose(dri2_dpy->driver);
   free(dri2_dpy->driver_name);
   free(dri2_dpy);
   disp->DriverData = NULL;

   return EGL_TRUE;
}

EGLBoolean
dri2_initialize_wayland(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   int i;

   drv->API.CreateWindowSurface = dri2_create_window_surface;
   drv->API.CreatePixmapSurface = dri2_create_pixmap_surface;
   drv->API.DestroySurface = dri2_destroy_surface;
   drv->API.SwapBuffers = dri2_swap_buffers;
   drv->API.CreateImageKHR = dri2_wayland_create_image_khr;
   drv->API.Terminate = dri2_terminate;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;
   dri2_dpy->wl_dpy = disp->PlatformDisplay;

   if (dri2_dpy->wl_dpy->fd == -1)
      force_roundtrip(dri2_dpy->wl_dpy->display);
   if (dri2_dpy->wl_dpy->fd == -1)
      goto cleanup_dpy;

   dri2_dpy->fd = dup(dri2_dpy->wl_dpy->fd);
   if (dri2_dpy->fd < 0) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to dup fd");
      goto cleanup_dpy;
   }

   if (!dri2_dpy->wl_dpy->authenticated)
      force_roundtrip(dri2_dpy->wl_dpy->display);
   if (!dri2_dpy->wl_dpy->authenticated)
      goto cleanup_dpy;

   dri2_dpy->driver_name = dri2_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");
      goto cleanup_fd;
   }

   dri2_dpy->device_name = strdup(dri2_dpy->wl_dpy->device_name);
   if (dri2_dpy->device_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get device name");
      goto cleanup_driver_name;
   }

   if (!dri2_load_driver(disp))
      goto cleanup_device_name;

   dri2_dpy->dri2_loader_extension.base.name = __DRI_DRI2_LOADER;
   dri2_dpy->dri2_loader_extension.base.version = 3;
   dri2_dpy->dri2_loader_extension.getBuffers = dri2_get_buffers;
   dri2_dpy->dri2_loader_extension.flushFrontBuffer = dri2_flush_front_buffer;
   dri2_dpy->dri2_loader_extension.getBuffersWithFormat =
      dri2_get_buffers_with_format;
      
   dri2_dpy->extensions[0] = &dri2_dpy->dri2_loader_extension.base;
   dri2_dpy->extensions[1] = &image_lookup_extension.base;
   dri2_dpy->extensions[2] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_driver;

   for (i = 0; dri2_dpy->driver_configs[i]; i++)
      dri2_add_config(disp, dri2_dpy->driver_configs[i], i + 1, 0,
		      EGL_WINDOW_BIT | EGL_PIXMAP_BIT, NULL);


   disp->Extensions.MESA_drm_image = EGL_TRUE;
   disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_image_pixmap = EGL_TRUE;
   disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   disp->Extensions.KHR_gl_texture_2D_image = EGL_TRUE;

   disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
   dri2_dpy->authenticate = dri2_wayland_authenticate;

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_device_name:
   free(dri2_dpy->device_name);
 cleanup_driver_name:
   free(dri2_dpy->driver_name);
 cleanup_fd:
   close(dri2_dpy->fd);
 cleanup_dpy:
   free(dri2_dpy);
   
   return EGL_FALSE;
}
