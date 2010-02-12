/*
 * Copyright © 2010 Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <xf86drm.h>
#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include <xcb/xcb.h>
#include <xcb/dri2.h>
#include <xcb/xfixes.h>
#include <X11/Xlib-xcb.h>

#include <glapi/glapi.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"
#include "eglimage.h"

struct dri2_egl_driver
{
   _EGLDriver base;
};

struct dri2_egl_display
{
   xcb_connection_t         *conn;
   int                       dri2_major;
   int                       dri2_minor;
   __DRIscreen              *dri_screen;
   void                     *driver;
   __DRIcoreExtension       *core;
   __DRIdri2Extension       *dri2;
   __DRI2flushExtension     *flush;
   __DRItexBufferExtension  *tex_buffer;
   __DRIimageExtension      *image;
   int                       fd;

   __DRIdri2LoaderExtension  loader_extension;
   __DRIimageLookupExtension image_lookup_extension;
   const __DRIextension     *extensions[3];
};

struct dri2_egl_context
{
   _EGLContext   base;
   __DRIcontext *dri_context;
};

struct dri2_egl_surface
{
   _EGLSurface          base;
   __DRIdrawable       *dri_drawable;
   xcb_drawable_t       drawable;
   __DRIbuffer          buffers[5];
   int                  buffer_count;
   xcb_xfixes_region_t  region;
   int                  have_fake_front;
   int                  swap_interval;
};

struct dri2_egl_config
{
   _EGLConfig         base;
   const __DRIconfig *dri_config;
};

struct dri2_egl_image
{
   _EGLImage   base;
   __DRIimage *dri_image;
   int         width;
   int         height;
   int         name;
   int         pitch;
   int         cpp;
   int         format;
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(dri2_egl)
_EGL_DRIVER_TYPECAST(dri2_egl_image, _EGLImage, obj)

EGLint dri2_to_egl_attribute_map[] = {
   0,
   EGL_BUFFER_SIZE,		/* __DRI_ATTRIB_BUFFER_SIZE */
   EGL_LEVEL,			/* __DRI_ATTRIB_LEVEL */
   EGL_RED_SIZE,		/* __DRI_ATTRIB_RED_SIZE */
   EGL_GREEN_SIZE,		/* __DRI_ATTRIB_GREEN_SIZE */
   EGL_BLUE_SIZE,		/* __DRI_ATTRIB_BLUE_SIZE */
   EGL_LUMINANCE_SIZE,		/* __DRI_ATTRIB_LUMINANCE_SIZE */
   EGL_ALPHA_SIZE,		/* __DRI_ATTRIB_ALPHA_SIZE */
   0,				/* __DRI_ATTRIB_ALPHA_MASK_SIZE */
   EGL_DEPTH_SIZE,		/* __DRI_ATTRIB_DEPTH_SIZE */
   EGL_STENCIL_SIZE,		/* __DRI_ATTRIB_STENCIL_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_RED_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_GREEN_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_BLUE_SIZE */
   0,				/* __DRI_ATTRIB_ACCUM_ALPHA_SIZE */
   EGL_SAMPLE_BUFFERS,		/* __DRI_ATTRIB_SAMPLE_BUFFERS */
   EGL_SAMPLES,			/* __DRI_ATTRIB_SAMPLES */
   0,				/* __DRI_ATTRIB_RENDER_TYPE, */
   0,				/* __DRI_ATTRIB_CONFIG_CAVEAT */
   0,				/* __DRI_ATTRIB_CONFORMANT */
   0,				/* __DRI_ATTRIB_DOUBLE_BUFFER */
   0,				/* __DRI_ATTRIB_STEREO */
   0,				/* __DRI_ATTRIB_AUX_BUFFERS */
   0,				/* __DRI_ATTRIB_TRANSPARENT_TYPE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_INDEX_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_RED_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_GREEN_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_BLUE_VALUE */
   0,				/* __DRI_ATTRIB_TRANSPARENT_ALPHA_VALUE */
   0,				/* __DRI_ATTRIB_FLOAT_MODE */
   0,				/* __DRI_ATTRIB_RED_MASK */
   0,				/* __DRI_ATTRIB_GREEN_MASK */
   0,				/* __DRI_ATTRIB_BLUE_MASK */
   0,				/* __DRI_ATTRIB_ALPHA_MASK */
   EGL_MAX_PBUFFER_WIDTH,	/* __DRI_ATTRIB_MAX_PBUFFER_WIDTH */
   EGL_MAX_PBUFFER_HEIGHT,	/* __DRI_ATTRIB_MAX_PBUFFER_HEIGHT */
   EGL_MAX_PBUFFER_PIXELS,	/* __DRI_ATTRIB_MAX_PBUFFER_PIXELS */
   0,				/* __DRI_ATTRIB_OPTIMAL_PBUFFER_WIDTH */
   0,				/* __DRI_ATTRIB_OPTIMAL_PBUFFER_HEIGHT */
   0,				/* __DRI_ATTRIB_VISUAL_SELECT_GROUP */
   0,				/* __DRI_ATTRIB_SWAP_METHOD */
   EGL_MAX_SWAP_INTERVAL,	/* __DRI_ATTRIB_MAX_SWAP_INTERVAL */
   EGL_MIN_SWAP_INTERVAL,	/* __DRI_ATTRIB_MIN_SWAP_INTERVAL */
   0,				/* __DRI_ATTRIB_BIND_TO_TEXTURE_RGB */
   0,				/* __DRI_ATTRIB_BIND_TO_TEXTURE_RGBA */
   0,				/* __DRI_ATTRIB_BIND_TO_MIPMAP_TEXTURE */
   0,				/* __DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS */
   0,				/* __DRI_ATTRIB_YINVERTED */
};

static void
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
		int depth, xcb_visualtype_t *visual)
{
   struct dri2_egl_config *conf;
   struct dri2_egl_display *dri2_dpy;
   _EGLConfig base;
   unsigned int attrib, value, double_buffer;
   EGLint key, bind_to_texture_rgb, bind_to_texture_rgba;
   int i;

   dri2_dpy = disp->DriverData;
   _eglInitConfig(&base, disp, id);
   
   i = 0;
   while (dri2_dpy->core->indexConfigAttrib(dri_config, i++, &attrib, &value)) {
      switch (attrib) {
      case __DRI_ATTRIB_RENDER_TYPE:
	 if (value & __DRI_ATTRIB_RGBA_BIT)
	    value = EGL_RGB_BUFFER;
	 else if (value & __DRI_ATTRIB_LUMINANCE_BIT)
	    value = EGL_LUMINANCE_BUFFER;
	 else
	    /* not valid */;
	 _eglSetConfigKey(&base, EGL_COLOR_BUFFER_TYPE, value);
	 break;	 

      case __DRI_ATTRIB_CONFIG_CAVEAT:
         if (value & __DRI_ATTRIB_NON_CONFORMANT_CONFIG)
            value = EGL_NON_CONFORMANT_CONFIG;
         else if (value & __DRI_ATTRIB_SLOW_BIT)
            value = EGL_SLOW_CONFIG;
	 else
	    value = EGL_NONE;
	 _eglSetConfigKey(&base, EGL_CONFIG_CAVEAT, value);
         break;

      case __DRI_ATTRIB_BIND_TO_TEXTURE_RGB:
	 bind_to_texture_rgb = value;
	 break;

      case __DRI_ATTRIB_BIND_TO_TEXTURE_RGBA:
	 bind_to_texture_rgba = value;
	 break;

      case __DRI_ATTRIB_DOUBLE_BUFFER:
	 double_buffer = value;
	 break;

      default:
	 key = dri2_to_egl_attribute_map[attrib];
	 if (key != 0)
	    _eglSetConfigKey(&base, key, value);
	 break;
      }
   }

   /* In EGL, double buffer or not isn't a config attribute.  Pixmaps
    * surfaces are always single buffered, pbuffer surfaces are always
    * back buffers and windows can be either, selected by passing an
    * attribute at window surface construction time.  To support this
    * we ignore all double buffer configs and manipulate the buffer we
    * return in the getBuffer callback to get the behaviour we want. */

   if (double_buffer)
      return;

   if (visual != NULL) {
      if (depth != _eglGetConfigKey(&base, EGL_BUFFER_SIZE))
	 return;

      _eglSetConfigKey(&base, EGL_SURFACE_TYPE,
		       EGL_WINDOW_BIT | EGL_PIXMAP_BIT | EGL_PBUFFER_BIT |
		       EGL_SWAP_BEHAVIOR_PRESERVED_BIT);

      _eglSetConfigKey(&base, EGL_NATIVE_VISUAL_ID, visual->visual_id);
      _eglSetConfigKey(&base, EGL_NATIVE_VISUAL_TYPE, visual->_class);
   } else {
      _eglSetConfigKey(&base, EGL_SURFACE_TYPE,
		       EGL_PIXMAP_BIT | EGL_PBUFFER_BIT);
   }

   _eglSetConfigKey(&base, EGL_NATIVE_RENDERABLE, EGL_TRUE);
   _eglSetConfigKey(&base, EGL_BIND_TO_TEXTURE_RGB, bind_to_texture_rgb);
   if (_eglGetConfigKey(&base, EGL_ALPHA_SIZE) > 0)
      _eglSetConfigKey(&base,
		       EGL_BIND_TO_TEXTURE_RGBA, bind_to_texture_rgba);

   /* EGL_OPENGL_ES_BIT, EGL_OPENVG_BIT, EGL_OPENGL_ES2_BIT */
   _eglSetConfigKey(&base, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT);
   _eglSetConfigKey(&base, EGL_CONFORMANT, EGL_OPENGL_BIT);

   if (!_eglValidateConfig(&base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "DRI2: failed to validate config %d", id);
      return;
   }

   conf = malloc(sizeof *conf);
   if (conf != NULL) {
      memcpy(&conf->base, &base, sizeof base);
      conf->dri_config = dri_config;
      _eglAddConfig(disp, &conf->base);
   }
}

/**
 * Process list of buffer received from the server
 *
 * Processes the list of buffers received in a reply from the server to either
 * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
 */
static void
dri2_process_buffers(struct dri2_egl_surface *dri2_surf,
		     xcb_dri2_dri2_buffer_t *buffers, unsigned count)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   xcb_rectangle_t rectangle;
   int i;

   dri2_surf->buffer_count = count;
   dri2_surf->have_fake_front = 0;

   /* This assumes the DRI2 buffer attachment tokens matches the
    * __DRIbuffer tokens. */
   for (i = 0; i < count; i++) {
      dri2_surf->buffers[i].attachment = buffers[i].attachment;
      dri2_surf->buffers[i].name = buffers[i].name;
      dri2_surf->buffers[i].pitch = buffers[i].pitch;
      dri2_surf->buffers[i].cpp = buffers[i].cpp;
      dri2_surf->buffers[i].flags = buffers[i].flags;

      /* We only use the DRI drivers single buffer configs.  This
       * means that if we try to render to a window, DRI2 will give us
       * the fake front buffer, which we'll use as a back buffer.
       * Note that EGL doesn't require that several clients rendering
       * to the same window must see the same aux buffers. */
      if (dri2_surf->buffers[i].attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)
         dri2_surf->have_fake_front = 1;
   }

   if (dri2_surf->region != XCB_NONE)
      xcb_xfixes_destroy_region(dri2_dpy->conn, dri2_surf->region);

   rectangle.x = 0;
   rectangle.y = 0;
   rectangle.width = dri2_surf->base.Width;
   rectangle.height = dri2_surf->base.Height;
   dri2_surf->region = xcb_generate_id(dri2_dpy->conn);
   xcb_xfixes_create_region(dri2_dpy->conn, dri2_surf->region, 1, &rectangle);
}

static __DRIbuffer *
dri2_get_buffers(__DRIdrawable * driDrawable,
		int *width, int *height,
		unsigned int *attachments, int count,
		int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_dri2_get_buffers_reply_t *reply;
   xcb_dri2_get_buffers_cookie_t cookie;

   cookie = xcb_dri2_get_buffers_unchecked (dri2_dpy->conn,
					    dri2_surf->drawable,
					    count, count, attachments);
   reply = xcb_dri2_get_buffers_reply (dri2_dpy->conn, cookie, NULL);
   buffers = xcb_dri2_get_buffers_buffers (reply);
   if (buffers == NULL)
      return NULL;

   *out_count = reply->count;
   dri2_surf->base.Width = *width = reply->width;
   dri2_surf->base.Height = *height = reply->height;
   dri2_process_buffers(dri2_surf, buffers, *out_count);		       

   free(reply);

   return dri2_surf->buffers;
}

static void
dri2_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
   /* FIXME: Does EGL support front buffer rendering at all? */

#if 0
   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   dri2WaitGL(dri2_surf);
#endif
}

static __DRIimage *
dri2_lookup_egl_image(__DRIcontext *context, void *image, void *data)
{
   struct dri2_egl_context *dri2_ctx = data;
   _EGLDisplay *disp = dri2_ctx->base.Resource.Display;
   struct dri2_egl_image *dri2_img;
   _EGLImage *img;

   img = _eglLookupImage(image, disp);
   if (img == NULL) {
      _eglError(EGL_BAD_PARAMETER, "dri2_lookup_egl_image");
      return NULL;
   }

   dri2_img = dri2_egl_image(image);

   return dri2_img->dri_image;
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
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_dri2_get_buffers_with_format_reply_t *reply;
   xcb_dri2_get_buffers_with_format_cookie_t cookie;
   xcb_dri2_attach_format_t *format_attachments;

   format_attachments = (xcb_dri2_attach_format_t *) attachments;
   cookie = xcb_dri2_get_buffers_with_format_unchecked (dri2_dpy->conn,
							dri2_surf->drawable,
							count, count,
							format_attachments);

   reply = xcb_dri2_get_buffers_with_format_reply (dri2_dpy->conn,
						   cookie, NULL);
   if (reply == NULL)
      return NULL;

   buffers = xcb_dri2_get_buffers_with_format_buffers (reply);
   dri2_surf->base.Width = *width = reply->width;
   dri2_surf->base.Height = *height = reply->height;
   *out_count = reply->count;
   dri2_process_buffers(dri2_surf, buffers, *out_count);		       

   free(reply);

   return dri2_surf->buffers;
}

#ifdef GLX_USE_TLS
static const char dri_driver_format[] = "%.*s/tls/%.*s_dri.so";
#else
static const char dri_driver_format[] = "%.*s/%.*s_dri.so";
#endif

static const char dri_driver_path[] = DEFAULT_DRIVER_DIR;

struct dri2_extension_match {
   const char *name;
   int version;
   int offset;
};

static struct dri2_extension_match dri2_driver_extensions[] = {
   { __DRI_CORE, 1, offsetof(struct dri2_egl_display, core) },
   { __DRI_DRI2, 1, offsetof(struct dri2_egl_display, dri2) },
   { NULL }
};

static struct dri2_extension_match dri2_core_extensions[] = {
   { __DRI2_FLUSH, 1, offsetof(struct dri2_egl_display, flush) },
   { __DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer) },
   { __DRI_IMAGE, 1, offsetof(struct dri2_egl_display, image) },
   { NULL }
};

static EGLBoolean
dri2_bind_extensions(struct dri2_egl_display *dri2_dpy,
		     struct dri2_extension_match *matches,
		     const __DRIextension **extensions)
{
   int i, j, ret = EGL_TRUE;
   void *field;

   for (i = 0; extensions[i]; i++) {
      _eglLog(_EGL_DEBUG, "DRI2: found extension `%s'", extensions[i]->name);
      for (j = 0; matches[j].name; j++) {
	 if (strcmp(extensions[i]->name, matches[j].name) == 0 &&
	     extensions[i]->version >= matches[j].version) {
	    field = ((char *) dri2_dpy + matches[j].offset);
	    *(const __DRIextension **) field = extensions[i];
	    _eglLog(_EGL_INFO, "DRI2: found extension %s version %d",
		    extensions[i]->name, extensions[i]->version);
	 }
      }
   }
   
   for (j = 0; matches[j].name; j++) {
      field = ((char *) dri2_dpy + matches[j].offset);
      if (*(const __DRIextension **) field == NULL) {
	 _eglLog(_EGL_FATAL, "DRI2: did not find extension %s version %d",
		 matches[j].name, matches[j].version);
	 ret = EGL_FALSE;
      }
   }

   return ret;
}

/**
 * Called via eglInitialize(), GLX_drv->API.Initialize().
 */
static EGLBoolean
dri2_initialize(_EGLDriver *drv, _EGLDisplay *disp,
		EGLint *major, EGLint *minor)
{
   const __DRIextension **extensions;
   const __DRIconfig **driver_configs;
   struct dri2_egl_display *dri2_dpy;
   char path[PATH_MAX], *search_paths, *p, *next, *end;
   xcb_xfixes_query_version_reply_t *xfixes_query;
   xcb_xfixes_query_version_cookie_t xfixes_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_connect_reply_t *connect;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_dri2_authenticate_reply_t *authenticate;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_generic_error_t *error;
   drm_magic_t magic;
   xcb_screen_iterator_t s;
   xcb_depth_iterator_t d;
   xcb_visualtype_t *visuals;
   int i, j, id;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   disp->DriverData = (void *) dri2_dpy;
   if (disp->NativeDisplay != NULL)
      dri2_dpy->conn = XGetXCBConnection(disp->NativeDisplay);
   else
      dri2_dpy->conn = xcb_connect(0, 0);
   if (!dri2_dpy->conn) {
      _eglLog(_EGL_WARNING, "DRI2: xcb_connect failed");
      goto cleanup_dpy;
   }

   xcb_prefetch_extension_data (dri2_dpy->conn, &xcb_xfixes_id);
   xcb_prefetch_extension_data (dri2_dpy->conn, &xcb_dri2_id);

   xfixes_query_cookie = xcb_xfixes_query_version(dri2_dpy->conn,
						  XCB_XFIXES_MAJOR_VERSION,
						  XCB_XFIXES_MINOR_VERSION);
   
   dri2_query_cookie = xcb_dri2_query_version (dri2_dpy->conn,
					       XCB_DRI2_MAJOR_VERSION,
					       XCB_DRI2_MINOR_VERSION);

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   connect_cookie = xcb_dri2_connect_unchecked (dri2_dpy->conn,
						s.data->root,
						XCB_DRI2_DRIVER_TYPE_DRI);
   
   xfixes_query =
      xcb_xfixes_query_version_reply (dri2_dpy->conn,
				      xfixes_query_cookie, &error);
   if (xfixes_query == NULL ||
       error != NULL || xfixes_query->major_version < 2) {
      _eglLog(_EGL_FATAL, "DRI2: failed to query xfixes version");
      free(error);
      goto cleanup_conn;
   }
   free(xfixes_query);

   dri2_query =
      xcb_dri2_query_version_reply (dri2_dpy->conn, dri2_query_cookie, &error);
   if (dri2_query == NULL || error != NULL) {
      _eglLog(_EGL_FATAL, "DRI2: failed to query version");
      free(error);
      goto cleanup_conn;
   }
   dri2_dpy->dri2_major = dri2_query->major_version;
   dri2_dpy->dri2_minor = dri2_query->minor_version;
   free(dri2_query);

   connect = xcb_dri2_connect_reply (dri2_dpy->conn, connect_cookie, NULL);
   if (connect == NULL ||
       connect->driver_name_length + connect->device_name_length == 0) {
      _eglLog(_EGL_FATAL, "DRI2: failed to authenticate");
      goto cleanup_connect;
   }

   search_paths = NULL;
   if (geteuid() == getuid()) {
      /* don't allow setuid apps to use LIBGL_DRIVERS_PATH */
      search_paths = getenv("LIBGL_DRIVERS_PATH");
   }
   if (search_paths == NULL)
      search_paths = DEFAULT_DRIVER_DIR;

   dri2_dpy->driver = NULL;
   end = search_paths + strlen(search_paths);
   for (p = search_paths; p < end && dri2_dpy->driver == NULL; p = next + 1) {
      next = strchr(p, ':');
      if (next == NULL)
         next = end;

      snprintf(path, sizeof path,
	       dri_driver_format,
	       (int) (next - p), p,
	       xcb_dri2_connect_driver_name_length (connect),
	       xcb_dri2_connect_driver_name (connect));

      dri2_dpy->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
   }

   if (dri2_dpy->driver == NULL) {
      _eglLog(_EGL_FATAL,
	      "DRI2: failed to open any driver (search paths %s)",
	      search_paths);
      goto cleanup_connect;
   }

   _eglLog(_EGL_DEBUG, "DRI2: dlopen(%s)", path);
   extensions = dlsym(dri2_dpy->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      _eglLog(_EGL_FATAL,
	      "DRI2: driver exports no extensions (%s)", dlerror());
      goto cleanup_driver;
   }

   if (!dri2_bind_extensions(dri2_dpy, dri2_driver_extensions, extensions))
      goto cleanup_driver;

   snprintf(path, sizeof path, "%.*s",
	    xcb_dri2_connect_device_name_length (connect),
	    xcb_dri2_connect_device_name (connect));
   dri2_dpy->fd = open (path, O_RDWR);
   if (dri2_dpy->fd == -1) {
      _eglLog(_EGL_FATAL,
	      "DRI2: could not open %s (%s)", path, strerror(errno));
      goto cleanup_driver;
   }

   if (drmGetMagic(dri2_dpy->fd, &magic)) {
      _eglLog(_EGL_FATAL, "DRI2: failed to get drm magic");
      goto cleanup_fd;
   }

   authenticate_cookie = xcb_dri2_authenticate_unchecked (dri2_dpy->conn,
							  s.data->root, magic);
   authenticate = xcb_dri2_authenticate_reply (dri2_dpy->conn,
					       authenticate_cookie, NULL);
   if (authenticate == NULL || !authenticate->authenticated) {
      _eglLog(_EGL_FATAL, "DRI2: failed to authenticate");
      free(authenticate);
      goto cleanup_fd;
   }

   free(authenticate);
   if (dri2_dpy->dri2_minor >= 1) {
      dri2_dpy->loader_extension.base.name = __DRI_DRI2_LOADER;
      dri2_dpy->loader_extension.base.version = 3;
      dri2_dpy->loader_extension.getBuffers = dri2_get_buffers;
      dri2_dpy->loader_extension.flushFrontBuffer = dri2_flush_front_buffer;
      dri2_dpy->loader_extension.getBuffersWithFormat =
	 dri2_get_buffers_with_format;
   } else {
      dri2_dpy->loader_extension.base.name = __DRI_DRI2_LOADER;
      dri2_dpy->loader_extension.base.version = 2;
      dri2_dpy->loader_extension.getBuffers = dri2_get_buffers;
      dri2_dpy->loader_extension.flushFrontBuffer = dri2_flush_front_buffer;
      dri2_dpy->loader_extension.getBuffersWithFormat = NULL;
   }
      
   dri2_dpy->image_lookup_extension.base.name = __DRI_IMAGE_LOOKUP;
   dri2_dpy->image_lookup_extension.base.version = 1;
   dri2_dpy->image_lookup_extension.lookupEGLImage = dri2_lookup_egl_image;

   dri2_dpy->extensions[0] = &dri2_dpy->loader_extension.base;
   dri2_dpy->extensions[1] = &dri2_dpy->image_lookup_extension.base;
   dri2_dpy->extensions[2] = NULL;

   dri2_dpy->dri_screen =
      dri2_dpy->dri2->createNewScreen(0, dri2_dpy->fd, dri2_dpy->extensions,
				      &driver_configs, dri2_dpy);

   if (dri2_dpy->dri_screen == NULL) {
      _eglLog(_EGL_FATAL, "DRI2: failed to create dri screen");
      goto cleanup_fd;
   }

   extensions = dri2_dpy->core->getExtensions(dri2_dpy->dri_screen);
   if (!dri2_bind_extensions(dri2_dpy, dri2_core_extensions, extensions))
      goto cleanup_dri_screen;

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   d = xcb_screen_allowed_depths_iterator(s.data);
   id = 1;
   while (d.rem > 0) {
      EGLBoolean class_added[6] = { 0, };

      visuals = xcb_depth_visuals(d.data);
      for (i = 0; i < xcb_depth_visuals_length(d.data); i++) {
	 if (class_added[visuals[i]._class])
	    continue;

	 class_added[visuals[i]._class] = EGL_TRUE;
	 for (j = 0; driver_configs[j]; j++)
	    dri2_add_config(disp, driver_configs[j],
			    id++, d.data->depth, &visuals[i]);

      }

      xcb_depth_next(&d);      
   }

   if (!disp->NumConfigs) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create any config");
      goto cleanup_configs;
   }

   disp->ClientAPIsMask = EGL_OPENGL_BIT;
   disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_image_pixmap = EGL_TRUE;

   /* we're supporting EGL 1.4 */
   *major = 1;
   *minor = 4;
   free (connect);
   return EGL_TRUE;

 cleanup_configs:
   _eglCleanupDisplay(disp);
 cleanup_dri_screen:
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
 cleanup_fd:
   close(dri2_dpy->fd);
 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_connect:
   free(connect);
 cleanup_conn:
   if (disp->NativeDisplay == NULL)
      xcb_disconnect(dri2_dpy->conn);
 cleanup_dpy:
   free(dri2_dpy);

   return EGL_FALSE;
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
   if (disp->NativeDisplay == NULL)
      xcb_disconnect(dri2_dpy->conn);
   free(dri2_dpy);
   disp->DriverData = NULL;

   return EGL_TRUE;
}


/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static _EGLContext *
dri2_create_context(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
		    _EGLContext *share_list, const EGLint *attrib_list)
{
   struct dri2_egl_context *dri2_ctx;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx_shared = dri2_egl_context(share_list);
   struct dri2_egl_config *dri2_config = dri2_egl_config(conf);

   dri2_ctx = malloc(sizeof *dri2_ctx);
   if (!dri2_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&dri2_ctx->base, disp, conf, attrib_list))
      goto cleanup;

   dri2_ctx->dri_context =
      dri2_dpy->dri2->createNewContext(dri2_dpy->dri_screen,
				       dri2_config->dri_config,
				       dri2_ctx_shared ? 
				       dri2_ctx_shared->dri_context : NULL,
				       dri2_ctx);

   if (!dri2_ctx->dri_context)
      goto cleanup;

   return &dri2_ctx->base;

 cleanup:
   free(dri2_ctx);
   return NULL;
}

static EGLBoolean
dri2_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);

   if (_eglIsSurfaceBound(surf))
      return EGL_TRUE;

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);
   
   xcb_dri2_destroy_drawable (dri2_dpy->conn, dri2_surf->drawable);

   if (surf->Type == EGL_PBUFFER_BIT)
      xcb_free_pixmap (dri2_dpy->conn, dri2_surf->drawable);

   free(surf);

   return EGL_TRUE;
}

/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
dri2_make_current(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *dsurf,
		  _EGLSurface *rsurf, _EGLContext *ctx)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_dsurf = dri2_egl_surface(dsurf);
   struct dri2_egl_surface *dri2_rsurf = dri2_egl_surface(rsurf);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   __DRIdrawable *ddraw, *rdraw;
   __DRIcontext *cctx;

   /* bind the new context and return the "orphaned" one */
   if (!_eglBindContext(&ctx, &dsurf, &rsurf))
      return EGL_FALSE;

   ddraw = (dri2_dsurf) ? dri2_dsurf->dri_drawable : NULL;
   rdraw = (dri2_rsurf) ? dri2_rsurf->dri_drawable : NULL;
   cctx = (dri2_ctx) ? dri2_ctx->dri_context : NULL;

   if ((cctx == NULL && ddraw == NULL && rdraw == NULL) ||
       dri2_dpy->core->bindContext(cctx, ddraw, rdraw)) {
      if (dsurf && !_eglIsSurfaceLinked(dsurf))
	 dri2_destroy_surface(drv, disp, dsurf);
      if (rsurf && rsurf != dsurf && !_eglIsSurfaceLinked(dsurf))
	 dri2_destroy_surface(drv, disp, rsurf);
      if (ctx != NULL && !_eglIsContextLinked(ctx))
	 dri2_dpy->core->unbindContext(dri2_egl_context(ctx)->dri_context);

      return EGL_TRUE;
   } else {
      _eglBindContext(&ctx, &dsurf, &rsurf);

      return EGL_FALSE;
   }
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
   xcb_get_geometry_cookie_t cookie;
   xcb_get_geometry_reply_t *reply;
   xcb_screen_iterator_t s;
   xcb_generic_error_t *error;

   dri2_surf = malloc(sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }
   
   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surf;

   dri2_surf->region = XCB_NONE;
   if (type == EGL_PBUFFER_BIT) {
      dri2_surf->drawable = xcb_generate_id(dri2_dpy->conn);
      s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
      xcb_create_pixmap(dri2_dpy->conn,
			_eglGetConfigKey(conf, EGL_BUFFER_SIZE),
			dri2_surf->drawable, s.data->root,
			dri2_surf->base.Width, dri2_surf->base.Height);
   } else {
      dri2_surf->drawable = window;
   }

   dri2_surf->dri_drawable = 
      (*dri2_dpy->dri2->createNewDrawable) (dri2_dpy->dri_screen,
					    dri2_conf->dri_config, dri2_surf);
   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_pixmap;
   }

   xcb_dri2_create_drawable (dri2_dpy->conn, dri2_surf->drawable);

   if (type != EGL_PBUFFER_BIT) {
      cookie = xcb_get_geometry (dri2_dpy->conn, dri2_surf->drawable);
      reply = xcb_get_geometry_reply (dri2_dpy->conn, cookie, &error);
      if (reply == NULL || error != NULL) {
	 _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
	 free(error);
	 goto cleanup_dri_drawable;
      }

      dri2_surf->base.Width = reply->width;
      dri2_surf->base.Height = reply->height;
      free(reply);
   }

   return &dri2_surf->base;

 cleanup_dri_drawable:
   dri2_dpy->core->destroyDrawable(dri2_surf->dri_drawable);
 cleanup_pixmap:
   if (type == EGL_PBUFFER_BIT)
      xcb_free_pixmap(dri2_dpy->conn, dri2_surf->drawable);
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

static _EGLSurface *
dri2_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *disp,
			    _EGLConfig *conf, const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_PBUFFER_BIT, conf,
			      XCB_WINDOW_NONE, attrib_list);
}

static EGLBoolean
dri2_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   xcb_dri2_copy_region_cookie_t cookie;

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

#if 0
   /* FIXME: Add support for dri swapbuffers, that'll give us swap
    * interval and page flipping (at least for fullscreen windows) as
    * well as the page flip event.  Unless surface->SwapBehavior is
    * EGL_BUFFER_PRESERVED. */
#if __DRI2_FLUSH_VERSION >= 2
   if (pdraw->psc->f)
      (*pdraw->psc->f->flushInvalidate)(pdraw->driDrawable);
#endif
#endif

   if (!dri2_surf->have_fake_front)
      return EGL_TRUE;

   cookie = xcb_dri2_copy_region_unchecked(dri2_dpy->conn,
					   dri2_surf->drawable,
					   dri2_surf->region,
					   XCB_DRI2_ATTACHMENT_BUFFER_FRONT_LEFT,
					   XCB_DRI2_ATTACHMENT_BUFFER_FAKE_FRONT_LEFT);
   free(xcb_dri2_copy_region_reply(dri2_dpy->conn, cookie, NULL));

   return EGL_TRUE;
}

/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
dri2_get_proc_address(_EGLDriver *drv, const char *procname)
{
   /* FIXME: Do we need to support lookup of EGL symbols too? */

   return (_EGLProc) _glapi_get_proc_address(procname);
}

static EGLBoolean
dri2_wait_client(_EGLDriver *drv, _EGLDisplay *disp, _EGLContext *ctx)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(ctx->DrawSurface);

   /* FIXME: If EGL allows frontbuffer rendering for window surfaces,
    * we need to copy fake to real here.*/

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static EGLBoolean
dri2_wait_native(_EGLDriver *drv, _EGLDisplay *disp, EGLint engine)
{
   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");
   /* glXWaitX(); */

   return EGL_TRUE;
}

static void
dri2_unload(_EGLDriver *drv)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
   free(dri2_drv);
}

static EGLBoolean
dri2_copy_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf,
		  EGLNativePixmapType target)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   xcb_gcontext_t gc;

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

   gc = xcb_generate_id(dri2_dpy->conn);
   xcb_create_gc(dri2_dpy->conn, gc, target, 0, NULL);
   xcb_copy_area(dri2_dpy->conn,
		  dri2_surf->drawable,
		  target,
		  gc,
		  0, 0,
		  0, 0,
		  dri2_surf->base.Width,
		  dri2_surf->base.Height);
   xcb_free_gc(dri2_dpy->conn, gc);

   return EGL_TRUE;
}

static EGLBoolean
dri2_bind_tex_image(_EGLDriver *drv,
		    _EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   struct dri2_egl_context *dri2_ctx;
   _EGLContext *ctx;
   GLint format, target;

   ctx = _eglGetCurrentContext();
   dri2_ctx = dri2_egl_context(ctx);

   if (buffer != EGL_BACK_BUFFER) {
      _eglError(EGL_BAD_PARAMETER, "eglBindTexImage");
      return EGL_FALSE;
   }

   /* We allow binding pixmaps too... Not conformat, but we can do it
    * for free and it's useful for X compositors.  Supposedly there's
    * a EGL_NOKIA_texture_from_pixmap extension that allows that, but
    * I couldn't find it at this time. */
   if ((dri2_surf->base.Type & (EGL_PBUFFER_BIT | EGL_PIXMAP_BIT)) == 0) {
      _eglError(EGL_BAD_SURFACE, "eglBindTexImage");
      return EGL_FALSE;
   }

   switch (dri2_surf->base.TextureFormat) {
   case EGL_TEXTURE_RGB:
      format = __DRI_TEXTURE_FORMAT_RGB;
      break;
   case EGL_TEXTURE_RGBA:
      format = __DRI_TEXTURE_FORMAT_RGBA;
      break;
   default:
      _eglError(EGL_BAD_MATCH, "eglBindTexImage");
      return EGL_FALSE;
   }

   switch (dri2_surf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglBindTexImage");
      return EGL_FALSE;
   }

   (*dri2_dpy->tex_buffer->setTexBuffer2)(dri2_ctx->dri_context,
					  target, format,
					  dri2_surf->dri_drawable);

   return dri2_surf->base.BoundToTexture = EGL_TRUE;
}

static EGLBoolean
dri2_release_tex_image(_EGLDriver *drv,
		       _EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
{
   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		      _EGLContext *ctx, EGLenum target,
		      EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   struct dri2_egl_image *dri2_img;
   unsigned int attachments[1];
   xcb_drawable_t drawable;
   xcb_dri2_get_buffers_cookie_t buffers_cookie;
   xcb_dri2_get_buffers_reply_t *buffers_reply;
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_get_geometry_cookie_t geometry_cookie;
   xcb_get_geometry_reply_t *geometry_reply;
   xcb_generic_error_t *error;

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   if (!_eglInitImage(&dri2_img->base, disp, attr_list))
      return EGL_NO_IMAGE_KHR;

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      drawable = (xcb_drawable_t) buffer;
      xcb_dri2_create_drawable (dri2_dpy->conn, drawable);
      attachments[0] = XCB_DRI2_ATTACHMENT_BUFFER_FRONT_LEFT;
      buffers_cookie =
	 xcb_dri2_get_buffers_unchecked (dri2_dpy->conn,
					 drawable, 1, 1, attachments);
      geometry_cookie = xcb_get_geometry (dri2_dpy->conn, drawable);
      buffers_reply = xcb_dri2_get_buffers_reply (dri2_dpy->conn,
						  buffers_cookie, NULL);
      buffers = xcb_dri2_get_buffers_buffers (buffers_reply);
      if (buffers == NULL) {
	 free(dri2_img);
	 return NULL;
      }

      geometry_reply = xcb_get_geometry_reply (dri2_dpy->conn,
					       geometry_cookie, &error);
      if (geometry_reply == NULL || error != NULL) {
	 _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
	 free(error);
	 free(dri2_img);
	 free(buffers_reply);
      }

      dri2_img->width = buffers_reply->width;
      dri2_img->height = buffers_reply->height;
      dri2_img->name = buffers[0].name;
      dri2_img->pitch = buffers[0].pitch / buffers[0].cpp;
      dri2_img->cpp = buffers[0].cpp;
      switch (geometry_reply->depth) {
      case 16:
	 dri2_img->format = __DRI_IMAGE_FORMAT_RGB565;
	 break;
      case 24:
	 dri2_img->format = __DRI_IMAGE_FORMAT_XRGB8888;
	 break;
      case 32:
	 dri2_img->format = __DRI_IMAGE_FORMAT_ARGB8888;
	 break;
      }
      free(buffers_reply);
      free(geometry_reply);
      break;

   default:
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
      free(dri2_img);
      return EGL_NO_IMAGE_KHR;
   }

   dri2_img->dri_image =
      dri2_dpy->image->createImageFromName(dri2_ctx->dri_context,
					   dri2_img->width,
					   dri2_img->height,
					   dri2_img->format,
					   dri2_img->name,
					   dri2_img->pitch,
					   dri2_img);

   return &dri2_img->base;
}

static EGLBoolean
dri2_destroy_image_khr(_EGLDriver *drv, _EGLDisplay *disp, _EGLImage *image)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(image);

   dri2_dpy->image->destroyImage(dri2_img->dri_image);
   free(dri2_img);

   return EGL_TRUE;
}

/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_eglMain(const char *args)
{
   struct dri2_egl_driver *dri2_drv;

   dri2_drv = malloc(sizeof *dri2_drv);
   if (!dri2_drv)
      return NULL;

   _eglInitDriverFallbacks(&dri2_drv->base);
   dri2_drv->base.API.Initialize = dri2_initialize;
   dri2_drv->base.API.Terminate = dri2_terminate;
   dri2_drv->base.API.CreateContext = dri2_create_context;
   dri2_drv->base.API.MakeCurrent = dri2_make_current;
   dri2_drv->base.API.CreateWindowSurface = dri2_create_window_surface;
   dri2_drv->base.API.CreatePixmapSurface = dri2_create_pixmap_surface;
   dri2_drv->base.API.CreatePbufferSurface = dri2_create_pbuffer_surface;
   dri2_drv->base.API.DestroySurface = dri2_destroy_surface;
   dri2_drv->base.API.SwapBuffers = dri2_swap_buffers;
   dri2_drv->base.API.GetProcAddress = dri2_get_proc_address;
   dri2_drv->base.API.WaitClient = dri2_wait_client;
   dri2_drv->base.API.WaitNative = dri2_wait_native;
   dri2_drv->base.API.CopyBuffers = dri2_copy_buffers;
   dri2_drv->base.API.BindTexImage = dri2_bind_tex_image;
   dri2_drv->base.API.ReleaseTexImage = dri2_release_tex_image;
   dri2_drv->base.API.CreateImageKHR = dri2_create_image_khr;
   dri2_drv->base.API.DestroyImageKHR = dri2_destroy_image_khr;

   dri2_drv->base.Name = "DRI2";
   dri2_drv->base.Unload = dri2_unload;

   return &dri2_drv->base;
}
