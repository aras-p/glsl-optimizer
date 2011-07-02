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
 */

#ifndef EGL_DRI2_INCLUDED
#define EGL_DRI2_INCLUDED

#ifdef HAVE_X11_PLATFORM
#include <xcb/xcb.h>
#include <xcb/dri2.h>
#include <xcb/xfixes.h>
#include <X11/Xlib-xcb.h>
#endif

#ifdef HAVE_WAYLAND_PLATFORM
#include <wayland-client.h>
#include "wayland-drm.h"
#include "wayland-egl-priv.h"
#endif

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#ifdef HAVE_DRM_PLATFORM
#include <gbm_driint.h>
#endif

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"
#include "eglimage.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct dri2_egl_driver
{
   _EGLDriver base;

   void *handle;
   _EGLProc (*get_proc_address)(const char *procname);
   void (*glFlush)(void);
};

struct dri2_egl_display
{
   int                       dri2_major;
   int                       dri2_minor;
   __DRIscreen              *dri_screen;
   int                       own_dri_screen;
   const __DRIconfig       **driver_configs;
   void                     *driver;
   __DRIcoreExtension       *core;
   __DRIdri2Extension       *dri2;
   __DRIswrastExtension     *swrast;
   __DRI2flushExtension     *flush;
   __DRItexBufferExtension  *tex_buffer;
   __DRIimageExtension      *image;
   int                       fd;

#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_device    *gbm_dri;
#endif

   char                     *device_name;
   char                     *driver_name;

   __DRIdri2LoaderExtension    dri2_loader_extension;
   __DRIswrastLoaderExtension  swrast_loader_extension;
   const __DRIextension     *extensions[4];

#ifdef HAVE_X11_PLATFORM
   xcb_connection_t         *conn;
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_display        *wl_dpy;
   struct wl_drm            *wl_server_drm;
   struct wl_drm            *wl_drm;
   int			     authenticated;
#endif

   int (*authenticate) (_EGLDisplay *disp, uint32_t id);
};

struct dri2_egl_context
{
   _EGLContext   base;
   __DRIcontext *dri_context;
};

#ifdef HAVE_WAYLAND_PLATFORM
enum wayland_buffer_type {
   WL_BUFFER_FRONT,
   WL_BUFFER_BACK,
   WL_BUFFER_THIRD,
   WL_BUFFER_COUNT
};

#define __DRI_BUFFER_COUNT 10
#endif

enum dri2_surface_type {
   DRI2_WINDOW_SURFACE,
   DRI2_PIXMAP_SURFACE,
   DRI2_PBUFFER_SURFACE
};

struct dri2_egl_surface
{
   _EGLSurface          base;
   __DRIdrawable       *dri_drawable;
   __DRIbuffer          buffers[5];
   int                  buffer_count;
   int                  have_fake_front;
   int                  swap_interval;

#ifdef HAVE_X11_PLATFORM
   xcb_drawable_t       drawable;
   xcb_xfixes_region_t  region;
   int                  depth;
   int                  bytes_per_pixel;
   xcb_gcontext_t       gc;
   xcb_gcontext_t       swapgc;
#endif

   enum dri2_surface_type type;
#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_egl_window  *wl_win;
   struct wl_egl_pixmap  *wl_pix;
   struct wl_buffer      *wl_drm_buffer[WL_BUFFER_COUNT];
   int                    wl_buffer_lock[WL_BUFFER_COUNT];
   int                    dx;
   int                    dy;
   __DRIbuffer           *dri_buffers[__DRI_BUFFER_COUNT];
   __DRIbuffer           *third_buffer;
   __DRIbuffer           *pending_buffer;
   EGLBoolean             block_swap_buffers;
#endif
};

struct dri2_egl_buffer {
   __DRIbuffer *dri_buffer;
   struct dri2_egl_display *dri2_dpy;
};


struct dri2_egl_config
{
   _EGLConfig         base;
   const __DRIconfig *dri_single_config;
   const __DRIconfig *dri_double_config;
};

struct dri2_egl_image
{
   _EGLImage   base;
   __DRIimage *dri_image;
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(dri2_egl)
_EGL_DRIVER_TYPECAST(dri2_egl_image, _EGLImage, obj)

extern const __DRIimageLookupExtension image_lookup_extension;
extern const __DRIuseInvalidateExtension use_invalidate;

EGLBoolean
dri2_load_driver(_EGLDisplay *disp);

/* Helper for platforms not using dri2_create_screen */
void
dri2_setup_screen(_EGLDisplay *disp);

EGLBoolean
dri2_load_driver_swrast(_EGLDisplay *disp);

EGLBoolean
dri2_create_screen(_EGLDisplay *disp);

__DRIimage *
dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data);

struct dri2_egl_config *
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
		int depth, EGLint surface_type, const EGLint *attr_list);

_EGLImage *
dri2_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		      _EGLContext *ctx, EGLenum target,
		      EGLClientBuffer buffer, const EGLint *attr_list);

EGLBoolean
dri2_initialize_x11(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_wayland(_EGLDriver *drv, _EGLDisplay *disp);

char *
dri2_get_driver_for_fd(int fd);
char *
dri2_get_device_name_for_fd(int fd);

#endif /* EGL_DRI2_INCLUDED */
