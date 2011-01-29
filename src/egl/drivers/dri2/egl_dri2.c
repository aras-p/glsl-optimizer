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
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIBUDEV
#include <libudev.h>
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
   xcb_connection_t         *conn;
   int                       dri2_major;
   int                       dri2_minor;
   __DRIscreen              *dri_screen;
   const __DRIconfig       **driver_configs;
   void                     *driver;
   __DRIcoreExtension       *core;
   __DRIdri2Extension       *dri2;
   __DRI2flushExtension     *flush;
   __DRItexBufferExtension  *tex_buffer;
   __DRIimageExtension      *image;
   int                       fd;

   char                     *device_name;
   char                     *driver_name;

   __DRIdri2LoaderExtension  loader_extension;
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
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(dri2_egl)
_EGL_DRIVER_TYPECAST(dri2_egl_image, _EGLImage, obj)

static const __DRIuseInvalidateExtension use_invalidate = {
   { __DRI_USE_INVALIDATE, 1 }
};

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
   EGL_Y_INVERTED_NOK,		/* __DRI_ATTRIB_YINVERTED */
};

static struct dri2_egl_config *
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
		int depth, EGLint surface_type)
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
   double_buffer = 0;
   bind_to_texture_rgb = 0;
   bind_to_texture_rgba = 0;

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
      return NULL;

   if (depth > 0 && depth != base.BufferSize)
      return NULL;

   base.NativeRenderable = EGL_TRUE;

   base.SurfaceType = surface_type;
   if (surface_type & (EGL_PIXMAP_BIT | EGL_PBUFFER_BIT)) {
      base.BindToTextureRGB = bind_to_texture_rgb;
      if (base.AlphaSize > 0)
         base.BindToTextureRGBA = bind_to_texture_rgba;
   }

   base.RenderableType = disp->ClientAPIs;
   base.Conformant = disp->ClientAPIs;

   if (!_eglValidateConfig(&base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "DRI2: failed to validate config %d", id);
      return NULL;
   }

   conf = malloc(sizeof *conf);
   if (conf != NULL) {
      memcpy(&conf->base, &base, sizeof base);
      conf->dri_config = dri_config;
      _eglLinkConfig(&conf->base);
   }

   return conf;
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
   unsigned i;

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

   (void) driDrawable;

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
   (void) driDrawable;

   /* FIXME: Does EGL support front buffer rendering at all? */

#if 0
   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   dri2WaitGL(dri2_surf);
#else
   (void) loaderPrivate;
#endif
}

static __DRIimage *
dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data)
{
   _EGLDisplay *disp = data;
   struct dri2_egl_image *dri2_img;
   _EGLImage *img;

   (void) screen;

   img = _eglLookupImage(image, disp);
   if (img == NULL) {
      _eglError(EGL_BAD_PARAMETER, "dri2_lookup_egl_image");
      return NULL;
   }

   dri2_img = dri2_egl_image(image);

   return dri2_img->dri_image;
}

static const __DRIimageLookupExtension image_lookup_extension = {
   { __DRI_IMAGE_LOOKUP, 1 },
   dri2_lookup_egl_image
};

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

   (void) driDrawable;

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

static const char dri_driver_path[] = DEFAULT_DRIVER_DIR;

struct dri2_extension_match {
   const char *name;
   int version;
   int offset;
};

static struct dri2_extension_match dri2_driver_extensions[] = {
   { __DRI_CORE, 1, offsetof(struct dri2_egl_display, core) },
   { __DRI_DRI2, 1, offsetof(struct dri2_egl_display, dri2) },
   { NULL, 0, 0 }
};

static struct dri2_extension_match dri2_core_extensions[] = {
   { __DRI2_FLUSH, 1, offsetof(struct dri2_egl_display, flush) },
   { __DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer) },
   { __DRI_IMAGE, 1, offsetof(struct dri2_egl_display, image) },
   { NULL, 0, 0 }
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

static char *
dri2_strndup(const char *s, int length)
{
   char *d;

   d = malloc(length + 1);
   if (d == NULL)
      return NULL;

   memcpy(d, s, length);
   d[length] = '\0';

   return d;
}

static EGLBoolean
dri2_connect(struct dri2_egl_display *dri2_dpy)
{
   xcb_xfixes_query_version_reply_t *xfixes_query;
   xcb_xfixes_query_version_cookie_t xfixes_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_connect_reply_t *connect;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_generic_error_t *error;
   xcb_screen_iterator_t s;

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
      return EGL_FALSE;
   }
   free(xfixes_query);

   dri2_query =
      xcb_dri2_query_version_reply (dri2_dpy->conn, dri2_query_cookie, &error);
   if (dri2_query == NULL || error != NULL) {
      _eglLog(_EGL_FATAL, "DRI2: failed to query version");
      free(error);
      return EGL_FALSE;
   }
   dri2_dpy->dri2_major = dri2_query->major_version;
   dri2_dpy->dri2_minor = dri2_query->minor_version;
   free(dri2_query);

   connect = xcb_dri2_connect_reply (dri2_dpy->conn, connect_cookie, NULL);
   if (connect == NULL ||
       connect->driver_name_length + connect->device_name_length == 0) {
      _eglLog(_EGL_FATAL, "DRI2: failed to authenticate");
      return EGL_FALSE;
   }

   dri2_dpy->device_name =
      dri2_strndup(xcb_dri2_connect_device_name (connect),
		   xcb_dri2_connect_device_name_length (connect));
		   
   dri2_dpy->driver_name =
      dri2_strndup(xcb_dri2_connect_driver_name (connect),
		   xcb_dri2_connect_driver_name_length (connect));

   if (dri2_dpy->device_name == NULL || dri2_dpy->driver_name == NULL) {
      free(dri2_dpy->device_name);
      free(dri2_dpy->driver_name);
      free(connect);
      return EGL_FALSE;
   }
   free(connect);

   return EGL_TRUE;
}

static EGLBoolean
dri2_authenticate(struct dri2_egl_display *dri2_dpy)
{
   xcb_dri2_authenticate_reply_t *authenticate;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_screen_iterator_t s;
   drm_magic_t magic;

   if (drmGetMagic(dri2_dpy->fd, &magic)) {
      _eglLog(_EGL_FATAL, "DRI2: failed to get drm magic");
      return EGL_FALSE;
   }

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   authenticate_cookie =
      xcb_dri2_authenticate_unchecked(dri2_dpy->conn, s.data->root, magic);
   authenticate =
      xcb_dri2_authenticate_reply(dri2_dpy->conn, authenticate_cookie, NULL);
   if (authenticate == NULL || !authenticate->authenticated) {
      _eglLog(_EGL_FATAL, "DRI2: failed to authenticate");
      free(authenticate);
      return EGL_FALSE;
   }

   free(authenticate);

   return EGL_TRUE;
}

static EGLBoolean
dri2_add_configs_for_visuals(struct dri2_egl_display *dri2_dpy,
			     _EGLDisplay *disp)
{
   xcb_screen_iterator_t s;
   xcb_depth_iterator_t d;
   xcb_visualtype_t *visuals;
   int i, j, id;
   struct dri2_egl_config *conf;
   EGLint surface_type;

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   d = xcb_screen_allowed_depths_iterator(s.data);
   id = 1;

   surface_type =
      EGL_WINDOW_BIT |
      EGL_PIXMAP_BIT |
      EGL_PBUFFER_BIT |
      EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

   while (d.rem > 0) {
      EGLBoolean class_added[6] = { 0, };

      visuals = xcb_depth_visuals(d.data);
      for (i = 0; i < xcb_depth_visuals_length(d.data); i++) {
	 if (class_added[visuals[i]._class])
	    continue;

	 class_added[visuals[i]._class] = EGL_TRUE;
	 for (j = 0; dri2_dpy->driver_configs[j]; j++) {
	    conf = dri2_add_config(disp, dri2_dpy->driver_configs[j],
				   id++, d.data->depth, surface_type);
	    if (conf == NULL)
	       continue;
	    _eglSetConfigKey(&conf->base,
			     EGL_NATIVE_VISUAL_ID, visuals[i].visual_id);
	    _eglSetConfigKey(&conf->base,
			     EGL_NATIVE_VISUAL_TYPE, visuals[i]._class);
	 }
      }

      xcb_depth_next(&d);      
   }

   if (!_eglGetArraySize(disp->Configs)) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create any config");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

static EGLBoolean
dri2_load_driver(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = disp->DriverData;
   const __DRIextension **extensions;
   char path[PATH_MAX], *search_paths, *p, *next, *end;

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
      int len;
      next = strchr(p, ':');
      if (next == NULL)
         next = end;

      len = next - p;
#if GLX_USE_TLS
      snprintf(path, sizeof path,
	       "%.*s/tls/%s_dri.so", len, p, dri2_dpy->driver_name);
      dri2_dpy->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
      if (dri2_dpy->driver == NULL) {
	 snprintf(path, sizeof path,
		  "%.*s/%s_dri.so", len, p, dri2_dpy->driver_name);
	 dri2_dpy->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	 if (dri2_dpy->driver == NULL)
	    _eglLog(_EGL_DEBUG, "failed to open %s: %s\n", path, dlerror());
      }
   }

   if (dri2_dpy->driver == NULL) {
      _eglLog(_EGL_WARNING,
	      "DRI2: failed to open any driver (search paths %s)",
	      search_paths);
      return EGL_FALSE;
   }

   _eglLog(_EGL_DEBUG, "DRI2: dlopen(%s)", path);
   extensions = dlsym(dri2_dpy->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      _eglLog(_EGL_WARNING,
	      "DRI2: driver exports no extensions (%s)", dlerror());
      dlclose(dri2_dpy->driver);
      return EGL_FALSE;
   }

   if (!dri2_bind_extensions(dri2_dpy, dri2_driver_extensions, extensions)) {
      dlclose(dri2_dpy->driver);
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

static EGLBoolean
dri2_create_screen(_EGLDisplay *disp)
{
   const __DRIextension **extensions;
   struct dri2_egl_display *dri2_dpy;
   unsigned int api_mask;

   dri2_dpy = disp->DriverData;
   dri2_dpy->dri_screen =
      dri2_dpy->dri2->createNewScreen(0, dri2_dpy->fd, dri2_dpy->extensions,
				      &dri2_dpy->driver_configs, disp);

   if (dri2_dpy->dri_screen == NULL) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create dri screen");
      return EGL_FALSE;
   }

   extensions = dri2_dpy->core->getExtensions(dri2_dpy->dri_screen);
   if (!dri2_bind_extensions(dri2_dpy, dri2_core_extensions, extensions))
      goto cleanup_dri_screen;

   if (dri2_dpy->dri2->base.version >= 2)
      api_mask = dri2_dpy->dri2->getAPIMask(dri2_dpy->dri_screen);
   else
      api_mask = 1 << __DRI_API_OPENGL;

   disp->ClientAPIs = 0;
   if (api_mask & (1 <<__DRI_API_OPENGL))
      disp->ClientAPIs |= EGL_OPENGL_BIT;
   if (api_mask & (1 <<__DRI_API_GLES))
      disp->ClientAPIs |= EGL_OPENGL_ES_BIT;
   if (api_mask & (1 << __DRI_API_GLES2))
      disp->ClientAPIs |= EGL_OPENGL_ES2_BIT;

   if (dri2_dpy->dri2->base.version >= 2) {
      disp->Extensions.KHR_surfaceless_gles1 = EGL_TRUE;
      disp->Extensions.KHR_surfaceless_gles2 = EGL_TRUE;
      disp->Extensions.KHR_surfaceless_opengl = EGL_TRUE;
   }

   return EGL_TRUE;

 cleanup_dri_screen:
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);

   return EGL_FALSE;
}

static EGLBoolean
dri2_initialize_x11(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;

   (void) drv;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   disp->DriverData = (void *) dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->conn = xcb_connect(0, 0);
   } else {
      dri2_dpy->conn = XGetXCBConnection((Display *) disp->PlatformDisplay);
   }

   if (xcb_connection_has_error(dri2_dpy->conn)) {
      _eglLog(_EGL_WARNING, "DRI2: xcb_connect failed");
      goto cleanup_dpy;
   }

   if (dri2_dpy->conn) {
      if (!dri2_connect(dri2_dpy))
	 goto cleanup_conn;
   }

   if (!dri2_load_driver(disp))
      goto cleanup_conn;

   dri2_dpy->fd = open(dri2_dpy->device_name, O_RDWR);
   if (dri2_dpy->fd == -1) {
      _eglLog(_EGL_WARNING,
	      "DRI2: could not open %s (%s)", dri2_dpy->device_name,
              strerror(errno));
      goto cleanup_driver;
   }

   if (dri2_dpy->conn) {
      if (!dri2_authenticate(dri2_dpy))
	 goto cleanup_fd;
   }

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
      
   dri2_dpy->extensions[0] = &dri2_dpy->loader_extension.base;
   dri2_dpy->extensions[1] = &image_lookup_extension.base;
   dri2_dpy->extensions[2] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_fd;

   if (dri2_dpy->conn) {
      if (!dri2_add_configs_for_visuals(dri2_dpy, disp))
	 goto cleanup_configs;
   }

   disp->Extensions.MESA_drm_image = EGL_TRUE;
   disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_image_pixmap = EGL_TRUE;
   disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   disp->Extensions.KHR_gl_texture_2D_image = EGL_TRUE;
   disp->Extensions.NOK_swap_region = EGL_TRUE;
   disp->Extensions.NOK_texture_from_pixmap = EGL_TRUE;

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_configs:
   _eglCleanupDisplay(disp);
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
 cleanup_fd:
   close(dri2_dpy->fd);
 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_conn:
   if (disp->PlatformDisplay == NULL)
      xcb_disconnect(dri2_dpy->conn);
 cleanup_dpy:
   free(dri2_dpy);

   return EGL_FALSE;
}

#ifdef HAVE_LIBUDEV

struct dri2_driver_map {
   int vendor_id;
   const char *driver;
   const int *chip_ids;
   int num_chips_ids;
};

const int i915_chip_ids[] = {
   0x3577, /* PCI_CHIP_I830_M */
   0x2562, /* PCI_CHIP_845_G */
   0x3582, /* PCI_CHIP_I855_GM */
   0x2572, /* PCI_CHIP_I865_G */
   0x2582, /* PCI_CHIP_I915_G */
   0x258a, /* PCI_CHIP_E7221_G */
   0x2592, /* PCI_CHIP_I915_GM */
   0x2772, /* PCI_CHIP_I945_G */
   0x27a2, /* PCI_CHIP_I945_GM */
   0x27ae, /* PCI_CHIP_I945_GME */
   0x29b2, /* PCI_CHIP_Q35_G */
   0x29c2, /* PCI_CHIP_G33_G */
   0x29d2, /* PCI_CHIP_Q33_G */
   0xa001, /* PCI_CHIP_IGD_G */
   0xa011, /* Pineview */
};

const int i965_chip_ids[] = {
   0x0042, /* PCI_CHIP_ILD_G */
   0x0046, /* PCI_CHIP_ILM_G */
   0x0102, /* PCI_CHIP_SANDYBRIDGE_GT1 */
   0x0106, /* PCI_CHIP_SANDYBRIDGE_M_GT1 */
   0x010a, /* PCI_CHIP_SANDYBRIDGE_S */
   0x0112, /* PCI_CHIP_SANDYBRIDGE_GT2 */
   0x0116, /* PCI_CHIP_SANDYBRIDGE_M_GT2 */
   0x0122, /* PCI_CHIP_SANDYBRIDGE_GT2_PLUS */
   0x0126, /* PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS */
   0x29a2, /* PCI_CHIP_I965_G */
   0x2992, /* PCI_CHIP_I965_Q */
   0x2982, /* PCI_CHIP_I965_G_1 */
   0x2972, /* PCI_CHIP_I946_GZ */
   0x2a02, /* PCI_CHIP_I965_GM */
   0x2a12, /* PCI_CHIP_I965_GME */
   0x2a42, /* PCI_CHIP_GM45_GM */
   0x2e02, /* PCI_CHIP_IGD_E_G */
   0x2e12, /* PCI_CHIP_Q45_G */
   0x2e22, /* PCI_CHIP_G45_G */
   0x2e32, /* PCI_CHIP_G41_G */
   0x2e42, /* PCI_CHIP_B43_G */
   0x2e92, /* PCI_CHIP_B43_G1 */
};

const int r100_chip_ids[] = {
   0x4C57, /* PCI_CHIP_RADEON_LW */
   0x4C58, /* PCI_CHIP_RADEON_LX */
   0x4C59, /* PCI_CHIP_RADEON_LY */
   0x4C5A, /* PCI_CHIP_RADEON_LZ */
   0x5144, /* PCI_CHIP_RADEON_QD */
   0x5145, /* PCI_CHIP_RADEON_QE */
   0x5146, /* PCI_CHIP_RADEON_QF */
   0x5147, /* PCI_CHIP_RADEON_QG */
   0x5159, /* PCI_CHIP_RADEON_QY */
   0x515A, /* PCI_CHIP_RADEON_QZ */
   0x5157, /* PCI_CHIP_RV200_QW */
   0x5158, /* PCI_CHIP_RV200_QX */
   0x515E, /* PCI_CHIP_RN50_515E */
   0x5969, /* PCI_CHIP_RN50_5969 */
   0x4136, /* PCI_CHIP_RS100_4136 */
   0x4336, /* PCI_CHIP_RS100_4336 */
   0x4137, /* PCI_CHIP_RS200_4137 */
   0x4337, /* PCI_CHIP_RS200_4337 */
   0x4237, /* PCI_CHIP_RS250_4237 */
   0x4437, /* PCI_CHIP_RS250_4437 */
};

const int r200_chip_ids[] = {
   0x5148, /* PCI_CHIP_R200_QH */
   0x514C, /* PCI_CHIP_R200_QL */
   0x514D, /* PCI_CHIP_R200_QM */
   0x4242, /* PCI_CHIP_R200_BB */
   0x4243, /* PCI_CHIP_R200_BC */
   0x4966, /* PCI_CHIP_RV250_If */
   0x4967, /* PCI_CHIP_RV250_Ig */
   0x4C64, /* PCI_CHIP_RV250_Ld */
   0x4C66, /* PCI_CHIP_RV250_Lf */
   0x4C67, /* PCI_CHIP_RV250_Lg */
   0x5960, /* PCI_CHIP_RV280_5960 */
   0x5961, /* PCI_CHIP_RV280_5961 */
   0x5962, /* PCI_CHIP_RV280_5962 */
   0x5964, /* PCI_CHIP_RV280_5964 */
   0x5965, /* PCI_CHIP_RV280_5965 */
   0x5C61, /* PCI_CHIP_RV280_5C61 */
   0x5C63, /* PCI_CHIP_RV280_5C63 */
   0x5834, /* PCI_CHIP_RS300_5834 */
   0x5835, /* PCI_CHIP_RS300_5835 */
   0x7834, /* PCI_CHIP_RS350_7834 */
   0x7835, /* PCI_CHIP_RS350_7835 */
};

const int r300_chip_ids[] = {
   0x4144, /* PCI_CHIP_R300_AD */
   0x4145, /* PCI_CHIP_R300_AE */
   0x4146, /* PCI_CHIP_R300_AF */
   0x4147, /* PCI_CHIP_R300_AG */
   0x4E44, /* PCI_CHIP_R300_ND */
   0x4E45, /* PCI_CHIP_R300_NE */
   0x4E46, /* PCI_CHIP_R300_NF */
   0x4E47, /* PCI_CHIP_R300_NG */
   0x4E48, /* PCI_CHIP_R350_NH */
   0x4E49, /* PCI_CHIP_R350_NI */
   0x4E4B, /* PCI_CHIP_R350_NK */
   0x4148, /* PCI_CHIP_R350_AH */
   0x4149, /* PCI_CHIP_R350_AI */
   0x414A, /* PCI_CHIP_R350_AJ */
   0x414B, /* PCI_CHIP_R350_AK */
   0x4E4A, /* PCI_CHIP_R360_NJ */
   0x4150, /* PCI_CHIP_RV350_AP */
   0x4151, /* PCI_CHIP_RV350_AQ */
   0x4152, /* PCI_CHIP_RV350_AR */
   0x4153, /* PCI_CHIP_RV350_AS */
   0x4154, /* PCI_CHIP_RV350_AT */
   0x4155, /* PCI_CHIP_RV350_AU */
   0x4156, /* PCI_CHIP_RV350_AV */
   0x4E50, /* PCI_CHIP_RV350_NP */
   0x4E51, /* PCI_CHIP_RV350_NQ */
   0x4E52, /* PCI_CHIP_RV350_NR */
   0x4E53, /* PCI_CHIP_RV350_NS */
   0x4E54, /* PCI_CHIP_RV350_NT */
   0x4E56, /* PCI_CHIP_RV350_NV */
   0x5460, /* PCI_CHIP_RV370_5460 */
   0x5462, /* PCI_CHIP_RV370_5462 */
   0x5464, /* PCI_CHIP_RV370_5464 */
   0x5B60, /* PCI_CHIP_RV370_5B60 */
   0x5B62, /* PCI_CHIP_RV370_5B62 */
   0x5B63, /* PCI_CHIP_RV370_5B63 */
   0x5B64, /* PCI_CHIP_RV370_5B64 */
   0x5B65, /* PCI_CHIP_RV370_5B65 */
   0x3150, /* PCI_CHIP_RV380_3150 */
   0x3152, /* PCI_CHIP_RV380_3152 */
   0x3154, /* PCI_CHIP_RV380_3154 */
   0x3155, /* PCI_CHIP_RV380_3155 */
   0x3E50, /* PCI_CHIP_RV380_3E50 */
   0x3E54, /* PCI_CHIP_RV380_3E54 */
   0x4A48, /* PCI_CHIP_R420_JH */
   0x4A49, /* PCI_CHIP_R420_JI */
   0x4A4A, /* PCI_CHIP_R420_JJ */
   0x4A4B, /* PCI_CHIP_R420_JK */
   0x4A4C, /* PCI_CHIP_R420_JL */
   0x4A4D, /* PCI_CHIP_R420_JM */
   0x4A4E, /* PCI_CHIP_R420_JN */
   0x4A4F, /* PCI_CHIP_R420_JO */
   0x4A50, /* PCI_CHIP_R420_JP */
   0x4A54, /* PCI_CHIP_R420_JT */
   0x5548, /* PCI_CHIP_R423_UH */
   0x5549, /* PCI_CHIP_R423_UI */
   0x554A, /* PCI_CHIP_R423_UJ */
   0x554B, /* PCI_CHIP_R423_UK */
   0x5550, /* PCI_CHIP_R423_5550 */
   0x5551, /* PCI_CHIP_R423_UQ */
   0x5552, /* PCI_CHIP_R423_UR */
   0x5554, /* PCI_CHIP_R423_UT */
   0x5D57, /* PCI_CHIP_R423_5D57 */
   0x554C, /* PCI_CHIP_R430_554C */
   0x554D, /* PCI_CHIP_R430_554D */
   0x554E, /* PCI_CHIP_R430_554E */
   0x554F, /* PCI_CHIP_R430_554F */
   0x5D48, /* PCI_CHIP_R430_5D48 */
   0x5D49, /* PCI_CHIP_R430_5D49 */
   0x5D4A, /* PCI_CHIP_R430_5D4A */
   0x5D4C, /* PCI_CHIP_R480_5D4C */
   0x5D4D, /* PCI_CHIP_R480_5D4D */
   0x5D4E, /* PCI_CHIP_R480_5D4E */
   0x5D4F, /* PCI_CHIP_R480_5D4F */
   0x5D50, /* PCI_CHIP_R480_5D50 */
   0x5D52, /* PCI_CHIP_R480_5D52 */
   0x4B49, /* PCI_CHIP_R481_4B49 */
   0x4B4A, /* PCI_CHIP_R481_4B4A */
   0x4B4B, /* PCI_CHIP_R481_4B4B */
   0x4B4C, /* PCI_CHIP_R481_4B4C */
   0x564A, /* PCI_CHIP_RV410_564A */
   0x564B, /* PCI_CHIP_RV410_564B */
   0x564F, /* PCI_CHIP_RV410_564F */
   0x5652, /* PCI_CHIP_RV410_5652 */
   0x5653, /* PCI_CHIP_RV410_5653 */
   0x5657, /* PCI_CHIP_RV410_5657 */
   0x5E48, /* PCI_CHIP_RV410_5E48 */
   0x5E4A, /* PCI_CHIP_RV410_5E4A */
   0x5E4B, /* PCI_CHIP_RV410_5E4B */
   0x5E4C, /* PCI_CHIP_RV410_5E4C */
   0x5E4D, /* PCI_CHIP_RV410_5E4D */
   0x5E4F, /* PCI_CHIP_RV410_5E4F */
   0x5A41, /* PCI_CHIP_RS400_5A41 */
   0x5A42, /* PCI_CHIP_RS400_5A42 */
   0x5A61, /* PCI_CHIP_RC410_5A61 */
   0x5A62, /* PCI_CHIP_RC410_5A62 */
   0x5954, /* PCI_CHIP_RS480_5954 */
   0x5955, /* PCI_CHIP_RS480_5955 */
   0x5974, /* PCI_CHIP_RS482_5974 */
   0x5975, /* PCI_CHIP_RS482_5975 */
   0x7100, /* PCI_CHIP_R520_7100 */
   0x7101, /* PCI_CHIP_R520_7101 */
   0x7102, /* PCI_CHIP_R520_7102 */
   0x7103, /* PCI_CHIP_R520_7103 */
   0x7104, /* PCI_CHIP_R520_7104 */
   0x7105, /* PCI_CHIP_R520_7105 */
   0x7106, /* PCI_CHIP_R520_7106 */
   0x7108, /* PCI_CHIP_R520_7108 */
   0x7109, /* PCI_CHIP_R520_7109 */
   0x710A, /* PCI_CHIP_R520_710A */
   0x710B, /* PCI_CHIP_R520_710B */
   0x710C, /* PCI_CHIP_R520_710C */
   0x710E, /* PCI_CHIP_R520_710E */
   0x710F, /* PCI_CHIP_R520_710F */
   0x7140, /* PCI_CHIP_RV515_7140 */
   0x7141, /* PCI_CHIP_RV515_7141 */
   0x7142, /* PCI_CHIP_RV515_7142 */
   0x7143, /* PCI_CHIP_RV515_7143 */
   0x7144, /* PCI_CHIP_RV515_7144 */
   0x7145, /* PCI_CHIP_RV515_7145 */
   0x7146, /* PCI_CHIP_RV515_7146 */
   0x7147, /* PCI_CHIP_RV515_7147 */
   0x7149, /* PCI_CHIP_RV515_7149 */
   0x714A, /* PCI_CHIP_RV515_714A */
   0x714B, /* PCI_CHIP_RV515_714B */
   0x714C, /* PCI_CHIP_RV515_714C */
   0x714D, /* PCI_CHIP_RV515_714D */
   0x714E, /* PCI_CHIP_RV515_714E */
   0x714F, /* PCI_CHIP_RV515_714F */
   0x7151, /* PCI_CHIP_RV515_7151 */
   0x7152, /* PCI_CHIP_RV515_7152 */
   0x7153, /* PCI_CHIP_RV515_7153 */
   0x715E, /* PCI_CHIP_RV515_715E */
   0x715F, /* PCI_CHIP_RV515_715F */
   0x7180, /* PCI_CHIP_RV515_7180 */
   0x7181, /* PCI_CHIP_RV515_7181 */
   0x7183, /* PCI_CHIP_RV515_7183 */
   0x7186, /* PCI_CHIP_RV515_7186 */
   0x7187, /* PCI_CHIP_RV515_7187 */
   0x7188, /* PCI_CHIP_RV515_7188 */
   0x718A, /* PCI_CHIP_RV515_718A */
   0x718B, /* PCI_CHIP_RV515_718B */
   0x718C, /* PCI_CHIP_RV515_718C */
   0x718D, /* PCI_CHIP_RV515_718D */
   0x718F, /* PCI_CHIP_RV515_718F */
   0x7193, /* PCI_CHIP_RV515_7193 */
   0x7196, /* PCI_CHIP_RV515_7196 */
   0x719B, /* PCI_CHIP_RV515_719B */
   0x719F, /* PCI_CHIP_RV515_719F */
   0x7200, /* PCI_CHIP_RV515_7200 */
   0x7210, /* PCI_CHIP_RV515_7210 */
   0x7211, /* PCI_CHIP_RV515_7211 */
   0x71C0, /* PCI_CHIP_RV530_71C0 */
   0x71C1, /* PCI_CHIP_RV530_71C1 */
   0x71C2, /* PCI_CHIP_RV530_71C2 */
   0x71C3, /* PCI_CHIP_RV530_71C3 */
   0x71C4, /* PCI_CHIP_RV530_71C4 */
   0x71C5, /* PCI_CHIP_RV530_71C5 */
   0x71C6, /* PCI_CHIP_RV530_71C6 */
   0x71C7, /* PCI_CHIP_RV530_71C7 */
   0x71CD, /* PCI_CHIP_RV530_71CD */
   0x71CE, /* PCI_CHIP_RV530_71CE */
   0x71D2, /* PCI_CHIP_RV530_71D2 */
   0x71D4, /* PCI_CHIP_RV530_71D4 */
   0x71D5, /* PCI_CHIP_RV530_71D5 */
   0x71D6, /* PCI_CHIP_RV530_71D6 */
   0x71DA, /* PCI_CHIP_RV530_71DA */
   0x71DE, /* PCI_CHIP_RV530_71DE */
   0x7281, /* PCI_CHIP_RV560_7281 */
   0x7283, /* PCI_CHIP_RV560_7283 */
   0x7287, /* PCI_CHIP_RV560_7287 */
   0x7290, /* PCI_CHIP_RV560_7290 */
   0x7291, /* PCI_CHIP_RV560_7291 */
   0x7293, /* PCI_CHIP_RV560_7293 */
   0x7297, /* PCI_CHIP_RV560_7297 */
   0x7280, /* PCI_CHIP_RV570_7280 */
   0x7288, /* PCI_CHIP_RV570_7288 */
   0x7289, /* PCI_CHIP_RV570_7289 */
   0x728B, /* PCI_CHIP_RV570_728B */
   0x728C, /* PCI_CHIP_RV570_728C */
   0x7240, /* PCI_CHIP_R580_7240 */
   0x7243, /* PCI_CHIP_R580_7243 */
   0x7244, /* PCI_CHIP_R580_7244 */
   0x7245, /* PCI_CHIP_R580_7245 */
   0x7246, /* PCI_CHIP_R580_7246 */
   0x7247, /* PCI_CHIP_R580_7247 */
   0x7248, /* PCI_CHIP_R580_7248 */
   0x7249, /* PCI_CHIP_R580_7249 */
   0x724A, /* PCI_CHIP_R580_724A */
   0x724B, /* PCI_CHIP_R580_724B */
   0x724C, /* PCI_CHIP_R580_724C */
   0x724D, /* PCI_CHIP_R580_724D */
   0x724E, /* PCI_CHIP_R580_724E */
   0x724F, /* PCI_CHIP_R580_724F */
   0x7284, /* PCI_CHIP_R580_7284 */
   0x793F, /* PCI_CHIP_RS600_793F */
   0x7941, /* PCI_CHIP_RS600_7941 */
   0x7942, /* PCI_CHIP_RS600_7942 */
   0x791E, /* PCI_CHIP_RS690_791E */
   0x791F, /* PCI_CHIP_RS690_791F */
   0x796C, /* PCI_CHIP_RS740_796C */
   0x796D, /* PCI_CHIP_RS740_796D */
   0x796E, /* PCI_CHIP_RS740_796E */
   0x796F, /* PCI_CHIP_RS740_796F */
};

const int r600_chip_ids[] = {
   0x9400, /* PCI_CHIP_R600_9400 */
   0x9401, /* PCI_CHIP_R600_9401 */
   0x9402, /* PCI_CHIP_R600_9402 */
   0x9403, /* PCI_CHIP_R600_9403 */
   0x9405, /* PCI_CHIP_R600_9405 */
   0x940A, /* PCI_CHIP_R600_940A */
   0x940B, /* PCI_CHIP_R600_940B */
   0x940F, /* PCI_CHIP_R600_940F */
   0x94C0, /* PCI_CHIP_RV610_94C0 */
   0x94C1, /* PCI_CHIP_RV610_94C1 */
   0x94C3, /* PCI_CHIP_RV610_94C3 */
   0x94C4, /* PCI_CHIP_RV610_94C4 */
   0x94C5, /* PCI_CHIP_RV610_94C5 */
   0x94C6, /* PCI_CHIP_RV610_94C6 */
   0x94C7, /* PCI_CHIP_RV610_94C7 */
   0x94C8, /* PCI_CHIP_RV610_94C8 */
   0x94C9, /* PCI_CHIP_RV610_94C9 */
   0x94CB, /* PCI_CHIP_RV610_94CB */
   0x94CC, /* PCI_CHIP_RV610_94CC */
   0x94CD, /* PCI_CHIP_RV610_94CD */
   0x9580, /* PCI_CHIP_RV630_9580 */
   0x9581, /* PCI_CHIP_RV630_9581 */
   0x9583, /* PCI_CHIP_RV630_9583 */
   0x9586, /* PCI_CHIP_RV630_9586 */
   0x9587, /* PCI_CHIP_RV630_9587 */
   0x9588, /* PCI_CHIP_RV630_9588 */
   0x9589, /* PCI_CHIP_RV630_9589 */
   0x958A, /* PCI_CHIP_RV630_958A */
   0x958B, /* PCI_CHIP_RV630_958B */
   0x958C, /* PCI_CHIP_RV630_958C */
   0x958D, /* PCI_CHIP_RV630_958D */
   0x958E, /* PCI_CHIP_RV630_958E */
   0x958F, /* PCI_CHIP_RV630_958F */
   0x9500, /* PCI_CHIP_RV670_9500 */
   0x9501, /* PCI_CHIP_RV670_9501 */
   0x9504, /* PCI_CHIP_RV670_9504 */
   0x9505, /* PCI_CHIP_RV670_9505 */
   0x9506, /* PCI_CHIP_RV670_9506 */
   0x9507, /* PCI_CHIP_RV670_9507 */
   0x9508, /* PCI_CHIP_RV670_9508 */
   0x9509, /* PCI_CHIP_RV670_9509 */
   0x950F, /* PCI_CHIP_RV670_950F */
   0x9511, /* PCI_CHIP_RV670_9511 */
   0x9515, /* PCI_CHIP_RV670_9515 */
   0x9517, /* PCI_CHIP_RV670_9517 */
   0x9519, /* PCI_CHIP_RV670_9519 */
   0x95C0, /* PCI_CHIP_RV620_95C0 */
   0x95C2, /* PCI_CHIP_RV620_95C2 */
   0x95C4, /* PCI_CHIP_RV620_95C4 */
   0x95C5, /* PCI_CHIP_RV620_95C5 */
   0x95C6, /* PCI_CHIP_RV620_95C6 */
   0x95C7, /* PCI_CHIP_RV620_95C7 */
   0x95C9, /* PCI_CHIP_RV620_95C9 */
   0x95CC, /* PCI_CHIP_RV620_95CC */
   0x95CD, /* PCI_CHIP_RV620_95CD */
   0x95CE, /* PCI_CHIP_RV620_95CE */
   0x95CF, /* PCI_CHIP_RV620_95CF */
   0x9590, /* PCI_CHIP_RV635_9590 */
   0x9591, /* PCI_CHIP_RV635_9591 */
   0x9593, /* PCI_CHIP_RV635_9593 */
   0x9595, /* PCI_CHIP_RV635_9595 */
   0x9596, /* PCI_CHIP_RV635_9596 */
   0x9597, /* PCI_CHIP_RV635_9597 */
   0x9598, /* PCI_CHIP_RV635_9598 */
   0x9599, /* PCI_CHIP_RV635_9599 */
   0x959B, /* PCI_CHIP_RV635_959B */
   0x9610, /* PCI_CHIP_RS780_9610 */
   0x9611, /* PCI_CHIP_RS780_9611 */
   0x9612, /* PCI_CHIP_RS780_9612 */
   0x9613, /* PCI_CHIP_RS780_9613 */
   0x9614, /* PCI_CHIP_RS780_9614 */
   0x9615, /* PCI_CHIP_RS780_9615 */
   0x9616, /* PCI_CHIP_RS780_9616 */
   0x9710, /* PCI_CHIP_RS880_9710 */
   0x9711, /* PCI_CHIP_RS880_9711 */
   0x9712, /* PCI_CHIP_RS880_9712 */
   0x9713, /* PCI_CHIP_RS880_9713 */
   0x9714, /* PCI_CHIP_RS880_9714 */
   0x9715, /* PCI_CHIP_RS880_9715 */
   0x9440, /* PCI_CHIP_RV770_9440 */
   0x9441, /* PCI_CHIP_RV770_9441 */
   0x9442, /* PCI_CHIP_RV770_9442 */
   0x9443, /* PCI_CHIP_RV770_9443 */
   0x9444, /* PCI_CHIP_RV770_9444 */
   0x9446, /* PCI_CHIP_RV770_9446 */
   0x944A, /* PCI_CHIP_RV770_944A */
   0x944B, /* PCI_CHIP_RV770_944B */
   0x944C, /* PCI_CHIP_RV770_944C */
   0x944E, /* PCI_CHIP_RV770_944E */
   0x9450, /* PCI_CHIP_RV770_9450 */
   0x9452, /* PCI_CHIP_RV770_9452 */
   0x9456, /* PCI_CHIP_RV770_9456 */
   0x945A, /* PCI_CHIP_RV770_945A */
   0x945B, /* PCI_CHIP_RV770_945B */
   0x945E, /* PCI_CHIP_RV770_945E */
   0x9460, /* PCI_CHIP_RV790_9460 */
   0x9462, /* PCI_CHIP_RV790_9462 */
   0x946A, /* PCI_CHIP_RV770_946A */
   0x946B, /* PCI_CHIP_RV770_946B */
   0x947A, /* PCI_CHIP_RV770_947A */
   0x947B, /* PCI_CHIP_RV770_947B */
   0x9480, /* PCI_CHIP_RV730_9480 */
   0x9487, /* PCI_CHIP_RV730_9487 */
   0x9488, /* PCI_CHIP_RV730_9488 */
   0x9489, /* PCI_CHIP_RV730_9489 */
   0x948A, /* PCI_CHIP_RV730_948A */
   0x948F, /* PCI_CHIP_RV730_948F */
   0x9490, /* PCI_CHIP_RV730_9490 */
   0x9491, /* PCI_CHIP_RV730_9491 */
   0x9495, /* PCI_CHIP_RV730_9495 */
   0x9498, /* PCI_CHIP_RV730_9498 */
   0x949C, /* PCI_CHIP_RV730_949C */
   0x949E, /* PCI_CHIP_RV730_949E */
   0x949F, /* PCI_CHIP_RV730_949F */
   0x9540, /* PCI_CHIP_RV710_9540 */
   0x9541, /* PCI_CHIP_RV710_9541 */
   0x9542, /* PCI_CHIP_RV710_9542 */
   0x954E, /* PCI_CHIP_RV710_954E */
   0x954F, /* PCI_CHIP_RV710_954F */
   0x9552, /* PCI_CHIP_RV710_9552 */
   0x9553, /* PCI_CHIP_RV710_9553 */
   0x9555, /* PCI_CHIP_RV710_9555 */
   0x9557, /* PCI_CHIP_RV710_9557 */
   0x955F, /* PCI_CHIP_RV710_955F */
   0x94A0, /* PCI_CHIP_RV740_94A0 */
   0x94A1, /* PCI_CHIP_RV740_94A1 */
   0x94A3, /* PCI_CHIP_RV740_94A3 */
   0x94B1, /* PCI_CHIP_RV740_94B1 */
   0x94B3, /* PCI_CHIP_RV740_94B3 */
   0x94B4, /* PCI_CHIP_RV740_94B4 */
   0x94B5, /* PCI_CHIP_RV740_94B5 */
   0x94B9, /* PCI_CHIP_RV740_94B9 */
   0x68E0, /* PCI_CHIP_CEDAR_68E0 */
   0x68E1, /* PCI_CHIP_CEDAR_68E1 */
   0x68E4, /* PCI_CHIP_CEDAR_68E4 */
   0x68E5, /* PCI_CHIP_CEDAR_68E5 */
   0x68E8, /* PCI_CHIP_CEDAR_68E8 */
   0x68E9, /* PCI_CHIP_CEDAR_68E9 */
   0x68F1, /* PCI_CHIP_CEDAR_68F1 */
   0x68F8, /* PCI_CHIP_CEDAR_68F8 */
   0x68F9, /* PCI_CHIP_CEDAR_68F9 */
   0x68FE, /* PCI_CHIP_CEDAR_68FE */
   0x68C0, /* PCI_CHIP_REDWOOD_68C0 */
   0x68C1, /* PCI_CHIP_REDWOOD_68C1 */
   0x68C8, /* PCI_CHIP_REDWOOD_68C8 */
   0x68C9, /* PCI_CHIP_REDWOOD_68C9 */
   0x68D8, /* PCI_CHIP_REDWOOD_68D8 */
   0x68D9, /* PCI_CHIP_REDWOOD_68D9 */
   0x68DA, /* PCI_CHIP_REDWOOD_68DA */
   0x68DE, /* PCI_CHIP_REDWOOD_68DE */
   0x68A0, /* PCI_CHIP_JUNIPER_68A0 */
   0x68A1, /* PCI_CHIP_JUNIPER_68A1 */
   0x68A8, /* PCI_CHIP_JUNIPER_68A8 */
   0x68A9, /* PCI_CHIP_JUNIPER_68A9 */
   0x68B0, /* PCI_CHIP_JUNIPER_68B0 */
   0x68B8, /* PCI_CHIP_JUNIPER_68B8 */
   0x68B9, /* PCI_CHIP_JUNIPER_68B9 */
   0x68BE, /* PCI_CHIP_JUNIPER_68BE */
   0x6880, /* PCI_CHIP_CYPRESS_6880 */
   0x6888, /* PCI_CHIP_CYPRESS_6888 */
   0x6889, /* PCI_CHIP_CYPRESS_6889 */
   0x688A, /* PCI_CHIP_CYPRESS_688A */
   0x6898, /* PCI_CHIP_CYPRESS_6898 */
   0x6899, /* PCI_CHIP_CYPRESS_6899 */
   0x689E, /* PCI_CHIP_CYPRESS_689E */
   0x689C, /* PCI_CHIP_HEMLOCK_689C */
   0x689D, /* PCI_CHIP_HEMLOCK_689D */
};

const struct dri2_driver_map driver_map[] = {
   { 0x8086, "i915", i915_chip_ids, ARRAY_SIZE(i915_chip_ids) },
   { 0x8086, "i965", i965_chip_ids, ARRAY_SIZE(i965_chip_ids) },
   { 0x1002, "radeon", r100_chip_ids, ARRAY_SIZE(r100_chip_ids) },
   { 0x1002, "r200", r200_chip_ids, ARRAY_SIZE(r200_chip_ids) },
   { 0x1002, "r300", r300_chip_ids, ARRAY_SIZE(r300_chip_ids) },
   { 0x1002, "r600", r600_chip_ids, ARRAY_SIZE(r600_chip_ids) },
};

static char *
dri2_get_driver_for_fd(int fd)
{
   struct udev *udev;
   struct udev_device *device, *parent;
   struct stat buf;
   const char *pci_id;
   char *driver = NULL;
   int vendor_id, chip_id, i, j;

   udev = udev_new();
   if (fstat(fd, &buf) < 0) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: failed to stat fd %d", fd);
      goto out;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      _eglLog(_EGL_WARNING,
	      "EGL-DRI2: could not create udev device for fd %d", fd);
      goto out;
   }

   parent = udev_device_get_parent(device);
   if (parent == NULL) {
      _eglLog(_EGL_WARNING, "DRI2: could not get parent device");
      goto out;
   }

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (pci_id == NULL || sscanf(pci_id, "%x:%x", &vendor_id, &chip_id) != 2) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: malformed or no PCI ID");
      goto out;
   }

   for (i = 0; i < ARRAY_SIZE(driver_map); i++) {
      if (vendor_id != driver_map[i].vendor_id)
	 continue;
      for (j = 0; j < driver_map[i].num_chips_ids; j++)
	 if (driver_map[i].chip_ids[j] == chip_id) {
	    driver = strdup(driver_map[i].driver);
	    _eglLog(_EGL_DEBUG, "pci id for %d: %04x:%04x, driver %s",
		    fd, vendor_id, chip_id, driver);
	    goto out;
	 }
   }

 out:
   udev_device_unref(device);
   udev_unref(udev);

   return driver;
}

static EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   int i;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   disp->DriverData = (void *) dri2_dpy;
   dri2_dpy->fd = (int) disp->PlatformDisplay;

   dri2_dpy->driver_name = dri2_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL)
      return _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");

   if (!dri2_load_driver(disp))
      goto cleanup_driver_name;

   dri2_dpy->extensions[0] = &image_lookup_extension.base;
   dri2_dpy->extensions[1] = &use_invalidate.base;
   dri2_dpy->extensions[2] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_driver;

   for (i = 0; dri2_dpy->driver_configs[i]; i++)
      dri2_add_config(disp, dri2_dpy->driver_configs[i], i + 1, 0, 0);

   disp->Extensions.MESA_drm_image = EGL_TRUE;
   disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   disp->Extensions.KHR_gl_texture_2D_image = EGL_TRUE;

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_driver_name:
   free(dri2_dpy->driver_name);

   return EGL_FALSE;
}

#endif

/**
 * Called via eglInitialize(), GLX_drv->API.Initialize().
 */
static EGLBoolean
dri2_initialize(_EGLDriver *drv, _EGLDisplay *disp)
{
   /* not until swrast_dri is supported */
   if (disp->Options.UseFallback)
      return EGL_FALSE;

   switch (disp->Platform) {
   case _EGL_PLATFORM_X11:
      if (disp->Options.TestOnly)
         return EGL_TRUE;
      return dri2_initialize_x11(drv, disp);

#ifdef HAVE_LIBUDEV
   case _EGL_PLATFORM_DRM:
      if (disp->Options.TestOnly)
         return EGL_TRUE;
      return dri2_initialize_drm(drv, disp);
#endif

   default:
      return EGL_FALSE;
   }
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
   if (disp->PlatformDisplay == NULL)
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
   const __DRIconfig *dri_config;
   int api;

   (void) drv;

   dri2_ctx = malloc(sizeof *dri2_ctx);
   if (!dri2_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&dri2_ctx->base, disp, conf, attrib_list))
      goto cleanup;

   switch (dri2_ctx->base.ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (dri2_ctx->base.ClientVersion) {
      case 1:
         api = __DRI_API_GLES;
         break;
      case 2:
         api = __DRI_API_GLES2;
         break;
      default:
	 _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
	 return NULL;
      }
      break;
   case EGL_OPENGL_API:
      api = __DRI_API_OPENGL;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
      return NULL;
   }

   if (conf != NULL)
      dri_config = dri2_config->dri_config;
   else
      dri_config = NULL;

   if (dri2_dpy->dri2->base.version >= 2) {
      dri2_ctx->dri_context =
	 dri2_dpy->dri2->createNewContextForAPI(dri2_dpy->dri_screen,
						api,
						dri_config,
						dri2_ctx_shared ? 
						dri2_ctx_shared->dri_context : NULL,
						dri2_ctx);
   } else if (api == __DRI_API_OPENGL) {
      dri2_ctx->dri_context =
	 dri2_dpy->dri2->createNewContext(dri2_dpy->dri_screen,
					  dri2_config->dri_config,
					  dri2_ctx_shared ? 
					  dri2_ctx_shared->dri_context : NULL,
					  dri2_ctx);
   } else {
      /* fail */
   }

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

   (void) drv;

   if (!_eglPutSurface(surf))
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
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_dsurf = dri2_egl_surface(dsurf);
   struct dri2_egl_surface *dri2_rsurf = dri2_egl_surface(rsurf);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   __DRIdrawable *ddraw, *rdraw;
   __DRIcontext *cctx;

   /* make new bindings */
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
      return EGL_FALSE;

   /* flush before context switch */
   if (old_ctx && dri2_drv->glFlush)
      dri2_drv->glFlush();

   ddraw = (dri2_dsurf) ? dri2_dsurf->dri_drawable : NULL;
   rdraw = (dri2_rsurf) ? dri2_rsurf->dri_drawable : NULL;
   cctx = (dri2_ctx) ? dri2_ctx->dri_context : NULL;

   if ((cctx == NULL && ddraw == NULL && rdraw == NULL) ||
       dri2_dpy->core->bindContext(cctx, ddraw, rdraw)) {
      dri2_destroy_surface(drv, disp, old_dsurf);
      dri2_destroy_surface(drv, disp, old_rsurf);
      if (old_ctx) {
         /* unbind the old context only when there is no new context bound */
         if (!ctx) {
            __DRIcontext *old_cctx = dri2_egl_context(old_ctx)->dri_context;
            dri2_dpy->core->unbindContext(old_cctx);
         }
         /* no destroy? */
         _eglPutContext(old_ctx);
      }

      return EGL_TRUE;
   } else {
      /* undo the previous _eglBindContext */
      _eglBindContext(old_ctx, old_dsurf, old_rsurf, &ctx, &dsurf, &rsurf);
      assert(&dri2_ctx->base == ctx &&
             &dri2_dsurf->base == dsurf &&
             &dri2_rsurf->base == rsurf);

      _eglPutSurface(dsurf);
      _eglPutSurface(rsurf);
      _eglPutContext(ctx);

      _eglPutSurface(old_dsurf);
      _eglPutSurface(old_rsurf);
      _eglPutContext(old_ctx);

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

   (void) drv;

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
      xcb_create_pixmap(dri2_dpy->conn, conf->BufferSize,
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
dri2_copy_region(_EGLDriver *drv, _EGLDisplay *disp,
		 _EGLSurface *draw, xcb_xfixes_region_t region)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   _EGLContext *ctx;
   xcb_dri2_copy_region_cookie_t cookie;

   if (dri2_drv->glFlush) {
      ctx = _eglGetCurrentContext();
      if (ctx && ctx->DrawSurface == &dri2_surf->base)
         dri2_drv->glFlush();
   }

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
					   region,
					   XCB_DRI2_ATTACHMENT_BUFFER_FRONT_LEFT,
					   XCB_DRI2_ATTACHMENT_BUFFER_FAKE_FRONT_LEFT);
   free(xcb_dri2_copy_region_reply(dri2_dpy->conn, cookie, NULL));

   return EGL_TRUE;
}

static EGLBoolean
dri2_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   return dri2_copy_region(drv, disp, draw, dri2_surf->region);
}

static EGLBoolean
dri2_swap_buffers_region(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw,
			 EGLint numRects, const EGLint *rects)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   EGLBoolean ret;
   xcb_xfixes_region_t region;
   xcb_rectangle_t rectangles[16];
   int i;

   if (numRects > (int)ARRAY_SIZE(rectangles))
      return dri2_copy_region(drv, disp, draw, dri2_surf->region);

   /* FIXME: Invert y here? */
   for (i = 0; i < numRects; i++) {
      rectangles[i].x = rects[i * 4];
      rectangles[i].y = rects[i * 4 + 1];
      rectangles[i].width = rects[i * 4 + 2];
      rectangles[i].height = rects[i * 4 + 3];
   }

   region = xcb_generate_id(dri2_dpy->conn);
   xcb_xfixes_create_region(dri2_dpy->conn, region, numRects, rectangles);
   ret = dri2_copy_region(drv, disp, draw, region);
   xcb_xfixes_destroy_region(dri2_dpy->conn, region);

   return ret;
}

/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
dri2_get_proc_address(_EGLDriver *drv, const char *procname)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);

   return dri2_drv->get_proc_address(procname);
}

static EGLBoolean
dri2_wait_client(_EGLDriver *drv, _EGLDisplay *disp, _EGLContext *ctx)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(ctx->DrawSurface);

   (void) drv;

   /* FIXME: If EGL allows frontbuffer rendering for window surfaces,
    * we need to copy fake to real here.*/

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static EGLBoolean
dri2_wait_native(_EGLDriver *drv, _EGLDisplay *disp, EGLint engine)
{
   (void) drv;
   (void) disp;

   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");
   /* glXWaitX(); */

   return EGL_TRUE;
}

static EGLBoolean
dri2_copy_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf,
		  EGLNativePixmapType target)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   xcb_gcontext_t gc;

   (void) drv;

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

   if (!_eglBindTexImage(drv, disp, surf, buffer))
      return EGL_FALSE;

   switch (dri2_surf->base.TextureFormat) {
   case EGL_TEXTURE_RGB:
      format = __DRI_TEXTURE_FORMAT_RGB;
      break;
   case EGL_TEXTURE_RGBA:
      format = __DRI_TEXTURE_FORMAT_RGBA;
      break;
   default:
      assert(0);
   }

   switch (dri2_surf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   default:
      assert(0);
   }

   (*dri2_dpy->tex_buffer->setTexBuffer2)(dri2_ctx->dri_context,
					  target, format,
					  dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static EGLBoolean
dri2_release_tex_image(_EGLDriver *drv,
		       _EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
{
#if __DRI_TEX_BUFFER_VERSION >= 3
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   struct dri2_egl_context *dri2_ctx;
   _EGLContext *ctx;
   GLint  target;

   ctx = _eglGetCurrentContext();
   dri2_ctx = dri2_egl_context(ctx);

   if (!_eglReleaseTexImage(drv, disp, surf, buffer))
      return EGL_FALSE;

   switch (dri2_surf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   default:
      assert(0);
   }
   if (dri2_dpy->tex_buffer->releaseTexBuffer!=NULL)
    (*dri2_dpy->tex_buffer->releaseTexBuffer)(dri2_ctx->dri_context,
                                             target,
                                             dri2_surf->dri_drawable);
#endif

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
			     EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   unsigned int attachments[1];
   xcb_drawable_t drawable;
   xcb_dri2_get_buffers_cookie_t buffers_cookie;
   xcb_dri2_get_buffers_reply_t *buffers_reply;
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_get_geometry_cookie_t geometry_cookie;
   xcb_get_geometry_reply_t *geometry_reply;
   xcb_generic_error_t *error;
   int stride, format;

   (void) ctx;

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
      return NULL;
   }

   geometry_reply = xcb_get_geometry_reply (dri2_dpy->conn,
					    geometry_cookie, &error);
   if (geometry_reply == NULL || error != NULL) {
      _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
      free(error);
      free(buffers_reply);
   }

   switch (geometry_reply->depth) {
   case 16:
      format = __DRI_IMAGE_FORMAT_RGB565;
      break;
   case 24:
      format = __DRI_IMAGE_FORMAT_XRGB8888;
      break;
   case 32:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER,
		"dri2_create_image_khr: unsupported pixmap depth");
      free(buffers_reply);
      free(geometry_reply);
      return NULL;
   }

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      free(buffers_reply);
      free(geometry_reply);
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(buffers_reply);
      free(geometry_reply);
      return EGL_NO_IMAGE_KHR;
   }

   stride = buffers[0].pitch / buffers[0].cpp;
   dri2_img->dri_image =
      dri2_dpy->image->createImageFromName(dri2_dpy->dri_screen,
					   buffers_reply->width,
					   buffers_reply->height,
					   format,
					   buffers[0].name,
					   stride,
					   dri2_img);

   free(buffers_reply);
   free(geometry_reply);

   return &dri2_img->base;
}

static _EGLImage *
dri2_create_image_khr_renderbuffer(_EGLDisplay *disp, _EGLContext *ctx,
				   EGLClientBuffer buffer,
				   const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   struct dri2_egl_image *dri2_img;
   GLuint renderbuffer = (GLuint) buffer;

   if (renderbuffer == 0) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   if (!_eglInitImage(&dri2_img->base, disp))
      return EGL_NO_IMAGE_KHR;

   dri2_img->dri_image = 
      dri2_dpy->image->createImageFromRenderbuffer(dri2_ctx->dri_context,
						   renderbuffer,
						   dri2_img);

   return &dri2_img->base;
}

static _EGLImage *
dri2_create_image_mesa_drm_buffer(_EGLDisplay *disp, _EGLContext *ctx,
				  EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   EGLint format, name, pitch, err;
   _EGLImageAttribs attrs;

   (void) ctx;

   name = (EGLint) buffer;

   err = _eglParseImageAttribList(&attrs, disp, attr_list);
   if (err != EGL_SUCCESS)
      return NULL;

   if (attrs.Width <= 0 || attrs.Height <= 0 ||
       attrs.DRMBufferStrideMESA <= 0) {
      _eglError(EGL_BAD_PARAMETER,
		"bad width, height or stride");
      return NULL;
   }

   switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      pitch = attrs.DRMBufferStrideMESA;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER,
		"dri2_create_image_khr: unsupported pixmap depth");
      return NULL;
   }

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_mesa_drm");
      return NULL;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(dri2_img);
      return NULL;
   }

   dri2_img->dri_image =
      dri2_dpy->image->createImageFromName(dri2_dpy->dri_screen,
					   attrs.Width,
					   attrs.Height,
					   format,
					   name,
					   pitch,
					   dri2_img);
   if (dri2_img->dri_image == NULL) {
      free(dri2_img);
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_mesa_drm");
      return NULL;
   }

   return &dri2_img->base;
}

static _EGLImage *
dri2_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		      _EGLContext *ctx, EGLenum target,
		      EGLClientBuffer buffer, const EGLint *attr_list)
{
   (void) drv;

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      return dri2_create_image_khr_pixmap(disp, ctx, buffer, attr_list);
   case EGL_GL_RENDERBUFFER_KHR:
      return dri2_create_image_khr_renderbuffer(disp, ctx, buffer, attr_list);
   case EGL_DRM_BUFFER_MESA:
      return dri2_create_image_mesa_drm_buffer(disp, ctx, buffer, attr_list);
   default:
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }
}

static EGLBoolean
dri2_destroy_image_khr(_EGLDriver *drv, _EGLDisplay *disp, _EGLImage *image)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(image);

   (void) drv;

   dri2_dpy->image->destroyImage(dri2_img->dri_image);
   free(dri2_img);

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_drm_image_mesa(_EGLDriver *drv, _EGLDisplay *disp,
			   const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   _EGLImageAttribs attrs;
   unsigned int dri_use, valid_mask;
   int format;
   EGLint err = EGL_SUCCESS;

   (void) drv;

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   if (!attr_list) {
      err = EGL_BAD_PARAMETER;
      goto cleanup_img;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      err = EGL_BAD_PARAMETER;
      goto cleanup_img;
   }

   err = _eglParseImageAttribList(&attrs, disp, attr_list);
   if (err != EGL_SUCCESS)
      goto cleanup_img;

   if (attrs.Width <= 0 || attrs.Height <= 0) {
      _eglLog(_EGL_WARNING, "bad width or height (%dx%d)",
            attrs.Width, attrs.Height);
      goto cleanup_img;
   }

   switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   default:
      _eglLog(_EGL_WARNING, "bad image format value 0x%04x",
            attrs.DRMBufferFormatMESA);
      goto cleanup_img;
   }

   valid_mask =
      EGL_DRM_BUFFER_USE_SCANOUT_MESA |
      EGL_DRM_BUFFER_USE_SHARE_MESA; 
   if (attrs.DRMBufferUseMESA & ~valid_mask) {
      _eglLog(_EGL_WARNING, "bad image use bit 0x%04x",
            attrs.DRMBufferUseMESA & ~valid_mask);
      goto cleanup_img;
   }

   dri_use = 0;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SHARE_MESA)
      dri_use |= __DRI_IMAGE_USE_SHARE;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SCANOUT_MESA)
      dri_use |= __DRI_IMAGE_USE_SCANOUT;

   dri2_img->dri_image = 
      dri2_dpy->image->createImage(dri2_dpy->dri_screen,
				   attrs.Width, attrs.Height,
                                   format, dri_use, dri2_img);
   if (dri2_img->dri_image == NULL) {
      err = EGL_BAD_ALLOC;
      goto cleanup_img;
   }

   return &dri2_img->base;

 cleanup_img:
   free(dri2_img);
   _eglError(err, "dri2_create_drm_image_mesa");

   return EGL_NO_IMAGE_KHR;
}

static EGLBoolean
dri2_export_drm_image_mesa(_EGLDriver *drv, _EGLDisplay *disp, _EGLImage *img,
			  EGLint *name, EGLint *handle, EGLint *stride)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);

   (void) drv;

   if (name && !dri2_dpy->image->queryImage(dri2_img->dri_image,
					    __DRI_IMAGE_ATTRIB_NAME, name)) {
      _eglError(EGL_BAD_ALLOC, "dri2_export_drm_image_mesa");
      return EGL_FALSE;
   }

   if (handle)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
				  __DRI_IMAGE_ATTRIB_HANDLE, handle);

   if (stride)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
				  __DRI_IMAGE_ATTRIB_STRIDE, stride);

   return EGL_TRUE;
}

static void
dri2_unload(_EGLDriver *drv)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);

   if (dri2_drv->handle)
      dlclose(dri2_drv->handle);
   free(dri2_drv);
}

static EGLBoolean
dri2_load(_EGLDriver *drv)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
#ifdef HAVE_SHARED_GLAPI
   const char *libname = "libglapi.so.0";
#else
   /*
    * Both libGL.so and libglapi.so are glapi providers.  There is no way to
    * tell which one to load.
    */
   const char *libname = NULL;
#endif
   void *handle;

   /* RTLD_GLOBAL to make sure glapi symbols are visible to DRI drivers */
   handle = dlopen(libname, RTLD_LAZY | RTLD_GLOBAL);
   if (handle) {
      dri2_drv->get_proc_address = (_EGLProc (*)(const char *))
         dlsym(handle, "_glapi_get_proc_address");
      if (!dri2_drv->get_proc_address || !libname) {
         /* no need to keep a reference */
         dlclose(handle);
         handle = NULL;
      }
   }

   /* if glapi is not available, loading DRI drivers will fail */
   if (!dri2_drv->get_proc_address) {
      _eglLog(_EGL_WARNING, "DRI2: failed to find _glapi_get_proc_address");
      return EGL_FALSE;
   }

   dri2_drv->glFlush = (void (*)(void))
      dri2_drv->get_proc_address("glFlush");

   dri2_drv->handle = handle;

   return EGL_TRUE;
}

/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_EGL_MAIN(const char *args)
{
   struct dri2_egl_driver *dri2_drv;

   (void) args;

   dri2_drv = malloc(sizeof *dri2_drv);
   if (!dri2_drv)
      return NULL;

   memset(dri2_drv, 0, sizeof *dri2_drv);

   if (!dri2_load(&dri2_drv->base))
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
   dri2_drv->base.API.SwapBuffersRegionNOK = dri2_swap_buffers_region;
   dri2_drv->base.API.CreateDRMImageMESA = dri2_create_drm_image_mesa;
   dri2_drv->base.API.ExportDRMImageMESA = dri2_export_drm_image_mesa;

   dri2_drv->base.Name = "DRI2";
   dri2_drv->base.Unload = dri2_unload;

   return &dri2_drv->base;
}
