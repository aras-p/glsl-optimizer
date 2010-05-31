/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"

#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_string.h"

#include "egl_g3d.h"
#include "egl_g3d_api.h"
#include "egl_g3d_st.h"
#include "native.h"

/**
 * Initialize the state trackers.
 */
static void
egl_g3d_init_st(_EGLDriver *drv)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   EGLint i;

   /* already initialized */
   if (gdrv->api_mask)
      return;

   egl_g3d_init_st_apis(gdrv->stapis);
   for (i = 0; i < ST_API_COUNT; i++) {
      if (gdrv->stapis[i])
         gdrv->api_mask |= egl_g3d_st_api_bit(i);
   }

   if (gdrv->api_mask)
      _eglLog(_EGL_DEBUG, "Driver API mask: 0x%x", gdrv->api_mask);
   else
      _eglLog(_EGL_WARNING, "No supported client API");
}

/**
 * Get the probe object of the display.
 *
 * Note that this function may be called before the display is initialized.
 */
static struct native_probe *
egl_g3d_get_probe(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct native_probe *nprobe;

   nprobe = (struct native_probe *) _eglGetProbeCache(gdrv->probe_key);
   if (!nprobe || nprobe->display != dpy->NativeDisplay) {
      if (nprobe)
         nprobe->destroy(nprobe);
      nprobe = native_create_probe(dpy->NativeDisplay);
      _eglSetProbeCache(gdrv->probe_key, (void *) nprobe);
   }

   return nprobe;
}

/**
 * Destroy the probe object of the display.  The display may be NULL.
 *
 * Note that this function may be called before the display is initialized.
 */
static void
egl_g3d_destroy_probe(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct native_probe *nprobe;

   nprobe = (struct native_probe *) _eglGetProbeCache(gdrv->probe_key);
   if (nprobe && (!dpy || nprobe->display == dpy->NativeDisplay)) {
      nprobe->destroy(nprobe);
      _eglSetProbeCache(gdrv->probe_key, NULL);
   }
}

#ifdef EGL_MESA_screen_surface

static void
egl_g3d_add_screens(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   const struct native_connector **native_connectors;
   EGLint num_connectors, i;

   native_connectors =
      gdpy->native->modeset->get_connectors(gdpy->native, &num_connectors, NULL);
   if (!num_connectors) {
      if (native_connectors)
         FREE(native_connectors);
      return;
   }

   for (i = 0; i < num_connectors; i++) {
      const struct native_connector *nconn = native_connectors[i];
      struct egl_g3d_screen *gscr;
      const struct native_mode **native_modes;
      EGLint num_modes, j;

      /* TODO support for hotplug */
      native_modes =
         gdpy->native->modeset->get_modes(gdpy->native, nconn, &num_modes);
      if (!num_modes) {
         if (native_modes)
            FREE(native_modes);
         continue;
      }

      gscr = CALLOC_STRUCT(egl_g3d_screen);
      if (!gscr) {
         FREE(native_modes);
         continue;
      }

      _eglInitScreen(&gscr->base);

      for (j = 0; j < num_modes; j++) {
         const struct native_mode *nmode = native_modes[j];
         _EGLMode *mode;

         mode = _eglAddNewMode(&gscr->base, nmode->width, nmode->height,
               nmode->refresh_rate, nmode->desc);
         if (!mode)
            break;
         /* gscr->native_modes and gscr->base.Modes should be consistent */
         assert(mode == &gscr->base.Modes[j]);
      }

      gscr->native = nconn;
      gscr->native_modes = native_modes;

      _eglAddScreen(dpy, &gscr->base);
   }

   FREE(native_connectors);
}

#endif /* EGL_MESA_screen_surface */

/**
 * Initialize and validate the EGL config attributes.
 */
static EGLBoolean
init_config_attributes(_EGLConfig *conf, const struct native_config *nconf,
                       EGLint api_mask, enum pipe_format depth_stencil_format)
{
   uint rgba[4], depth_stencil[2], buffer_size;
   EGLint surface_type;
   EGLint i;

   /* get the color and depth/stencil component sizes */
   assert(nconf->color_format != PIPE_FORMAT_NONE);
   buffer_size = 0;
   for (i = 0; i < 4; i++) {
      rgba[i] = util_format_get_component_bits(nconf->color_format,
            UTIL_FORMAT_COLORSPACE_RGB, i);
      buffer_size += rgba[i];
   }
   for (i = 0; i < 2; i++) {
      if (depth_stencil_format != PIPE_FORMAT_NONE) {
         depth_stencil[i] =
            util_format_get_component_bits(depth_stencil_format,
               UTIL_FORMAT_COLORSPACE_ZS, i);
      }
      else {
         depth_stencil[i] = 0;
      }
   }

   surface_type = 0x0;
   if (nconf->window_bit)
      surface_type |= EGL_WINDOW_BIT;
   if (nconf->pixmap_bit)
      surface_type |= EGL_PIXMAP_BIT;
#ifdef EGL_MESA_screen_surface
   if (nconf->scanout_bit)
      surface_type |= EGL_SCREEN_BIT_MESA;
#endif

   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_LEFT))
      surface_type |= EGL_PBUFFER_BIT;

   SET_CONFIG_ATTRIB(conf, EGL_CONFORMANT, api_mask);
   SET_CONFIG_ATTRIB(conf, EGL_RENDERABLE_TYPE, api_mask);

   SET_CONFIG_ATTRIB(conf, EGL_RED_SIZE, rgba[0]);
   SET_CONFIG_ATTRIB(conf, EGL_GREEN_SIZE, rgba[1]);
   SET_CONFIG_ATTRIB(conf, EGL_BLUE_SIZE, rgba[2]);
   SET_CONFIG_ATTRIB(conf, EGL_ALPHA_SIZE, rgba[3]);
   SET_CONFIG_ATTRIB(conf, EGL_BUFFER_SIZE, buffer_size);

   SET_CONFIG_ATTRIB(conf, EGL_DEPTH_SIZE, depth_stencil[0]);
   SET_CONFIG_ATTRIB(conf, EGL_STENCIL_SIZE, depth_stencil[1]);

   SET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE, surface_type);

   SET_CONFIG_ATTRIB(conf, EGL_NATIVE_RENDERABLE, EGL_TRUE);
   if (surface_type & EGL_WINDOW_BIT) {
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_ID, nconf->native_visual_id);
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_TYPE,
            nconf->native_visual_type);
   }

   if (surface_type & EGL_PBUFFER_BIT) {
      SET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE);
      if (rgba[3])
         SET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE);

      SET_CONFIG_ATTRIB(conf, EGL_MAX_PBUFFER_WIDTH, 4096);
      SET_CONFIG_ATTRIB(conf, EGL_MAX_PBUFFER_HEIGHT, 4096);
      SET_CONFIG_ATTRIB(conf, EGL_MAX_PBUFFER_PIXELS, 4096 * 4096);
   }

   SET_CONFIG_ATTRIB(conf, EGL_LEVEL, nconf->level);
   SET_CONFIG_ATTRIB(conf, EGL_SAMPLES, nconf->samples);
   SET_CONFIG_ATTRIB(conf, EGL_SAMPLE_BUFFERS, 1);

   if (nconf->slow_config)
      SET_CONFIG_ATTRIB(conf, EGL_CONFIG_CAVEAT, EGL_SLOW_CONFIG);

   if (nconf->transparent_rgb) {
      rgba[0] = nconf->transparent_rgb_values[0];
      rgba[1] = nconf->transparent_rgb_values[1];
      rgba[2] = nconf->transparent_rgb_values[2];

      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_TYPE, EGL_TRANSPARENT_RGB);
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_RED_VALUE, rgba[0]);
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_GREEN_VALUE, rgba[1]);
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_BLUE_VALUE, rgba[2]);
   }

   return _eglValidateConfig(conf, EGL_FALSE);
}

/**
 * Initialize an EGL config from the native config.
 */
static EGLBoolean
egl_g3d_init_config(_EGLDriver *drv, _EGLDisplay *dpy,
                    _EGLConfig *conf, const struct native_config *nconf,
                    enum pipe_format depth_stencil_format)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   EGLint buffer_mask, api_mask;
   EGLBoolean valid;
   EGLint i;

   buffer_mask = 0x0;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_FRONT_LEFT))
      buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_LEFT))
      buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_FRONT_RIGHT))
      buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
   if (nconf->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_RIGHT))
      buffer_mask |= ST_ATTACHMENT_BACK_RIGHT_MASK;

   gconf->stvis.buffer_mask = buffer_mask;
   gconf->stvis.color_format = nconf->color_format;
   gconf->stvis.depth_stencil_format = depth_stencil_format;
   gconf->stvis.accum_format = PIPE_FORMAT_NONE;
   gconf->stvis.samples = nconf->samples;

   gconf->stvis.render_buffer = (buffer_mask & ST_ATTACHMENT_BACK_LEFT_MASK) ?
      ST_ATTACHMENT_BACK_LEFT : ST_ATTACHMENT_FRONT_LEFT;

   api_mask = 0;
   for (i = 0; i < ST_API_COUNT; i++) {
      struct st_api *stapi = gdrv->stapis[i];
      if (stapi) {
         if (stapi->is_visual_supported(stapi, &gconf->stvis))
            api_mask |= egl_g3d_st_api_bit(i);
      }
   }
   /* this is required by EGL, not by OpenGL ES */
   if (nconf->window_bit &&
       gconf->stvis.render_buffer != ST_ATTACHMENT_BACK_LEFT)
      api_mask &= ~(EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT);

   if (!api_mask) {
      _eglLog(_EGL_DEBUG, "no state tracker supports config 0x%x",
            nconf->native_visual_id);
   }

   valid = init_config_attributes(&gconf->base,
         nconf, api_mask, depth_stencil_format);
   if (!valid) {
      _eglLog(_EGL_DEBUG, "skip invalid config 0x%x", nconf->native_visual_id);
      return EGL_FALSE;
   }

   gconf->native = nconf;

   return EGL_TRUE;
}

/**
 * Get all interested depth/stencil formats of a display.
 */
static EGLint
egl_g3d_fill_depth_stencil_formats(_EGLDisplay *dpy,
                                   enum pipe_format formats[8])
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_screen *screen = gdpy->native->screen;
   const EGLint candidates[] = {
      1, PIPE_FORMAT_Z16_UNORM,
      1, PIPE_FORMAT_Z32_UNORM,
      2, PIPE_FORMAT_Z24_UNORM_S8_USCALED, PIPE_FORMAT_S8_USCALED_Z24_UNORM,
      2, PIPE_FORMAT_Z24X8_UNORM, PIPE_FORMAT_X8Z24_UNORM,
      0
   };
   const EGLint *fmt = candidates;
   EGLint count;

   count = 0;
   formats[count++] = PIPE_FORMAT_NONE;

   while (*fmt) {
      EGLint i, n = *fmt++;

      /* pick the first supported format */
      for (i = 0; i < n; i++) {
         if (screen->is_format_supported(screen, fmt[i],
                  PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL, 0)) {
            formats[count++] = fmt[i];
            break;
         }
      }

      fmt += n;
   }

   return count;
}

/**
 * Add configs to display and return the next config ID.
 */
static EGLint
egl_g3d_add_configs(_EGLDriver *drv, _EGLDisplay *dpy, EGLint id)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   const struct native_config **native_configs;
   enum pipe_format depth_stencil_formats[8];
   int num_formats, num_configs, i, j;

   native_configs = gdpy->native->get_configs(gdpy->native, &num_configs);
   if (!num_configs) {
      if (native_configs)
         FREE(native_configs);
      return id;
   }

   num_formats = egl_g3d_fill_depth_stencil_formats(dpy,
         depth_stencil_formats);

   for (i = 0; i < num_configs; i++) {
      for (j = 0; j < num_formats; j++) {
         struct egl_g3d_config *gconf;

         gconf = CALLOC_STRUCT(egl_g3d_config);
         if (gconf) {
            _eglInitConfig(&gconf->base, dpy, id);
            if (!egl_g3d_init_config(drv, dpy, &gconf->base,
                     native_configs[i], depth_stencil_formats[j])) {
               FREE(gconf);
               break;
            }

            _eglAddConfig(dpy, &gconf->base);
            id++;
         }
      }
   }

   FREE(native_configs);
   return id;
}

static void
egl_g3d_invalid_surface(struct native_display *ndpy,
                        struct native_surface *nsurf,
                        unsigned int seq_num)
{
   /* XXX not thread safe? */
   struct egl_g3d_surface *gsurf = egl_g3d_surface(nsurf->user_data);
   struct egl_g3d_context *gctx;
   
   /*
    * Some functions such as egl_g3d_copy_buffers create a temporary native
    * surface.  There is no gsurf associated with it.
    */
   gctx = (gsurf) ? egl_g3d_context(gsurf->base.CurrentContext) : NULL;
   if (gctx)
      gctx->stctxi->notify_invalid_framebuffer(gctx->stctxi, gsurf->stfbi);
}

static struct native_event_handler egl_g3d_native_event_handler = {
   egl_g3d_invalid_surface
};

static EGLBoolean
egl_g3d_terminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   EGLint i;

   _eglReleaseDisplayResources(drv, dpy);
   _eglCleanupDisplay(dpy);

   if (gdpy->pipe)
      gdpy->pipe->destroy(gdpy->pipe);

   if (dpy->Screens) {
      for (i = 0; i < dpy->NumScreens; i++) {
         struct egl_g3d_screen *gscr = egl_g3d_screen(dpy->Screens[i]);
         FREE(gscr->native_modes);
         FREE(gscr);
      }
      FREE(dpy->Screens);
   }

   if (gdpy->smapi)
      egl_g3d_destroy_st_manager(gdpy->smapi);

   if (gdpy->native)
      gdpy->native->destroy(gdpy->native);

   FREE(gdpy);
   dpy->DriverData = NULL;

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_initialize(_EGLDriver *drv, _EGLDisplay *dpy,
                   EGLint *major, EGLint *minor)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct egl_g3d_display *gdpy;

   /* the probe object is unlikely to be needed again */
   egl_g3d_destroy_probe(drv, dpy);

   gdpy = CALLOC_STRUCT(egl_g3d_display);
   if (!gdpy) {
      _eglError(EGL_BAD_ALLOC, "eglInitialize");
      goto fail;
   }
   dpy->DriverData = gdpy;

   gdpy->native = native_create_display(dpy->NativeDisplay,
         &egl_g3d_native_event_handler);
   if (!gdpy->native) {
      _eglError(EGL_NOT_INITIALIZED, "eglInitialize(no usable display)");
      goto fail;
   }

   gdpy->native->user_data = (void *) dpy;

   egl_g3d_init_st(&gdrv->base);
   dpy->ClientAPIsMask = gdrv->api_mask;

   gdpy->smapi = egl_g3d_create_st_manager(dpy);
   if (!gdpy->smapi) {
      _eglError(EGL_NOT_INITIALIZED,
            "eglInitialize(failed to create st manager)");
      goto fail;
   }

#ifdef EGL_MESA_screen_surface
   /* enable MESA_screen_surface before adding (and validating) configs */
   if (gdpy->native->modeset) {
      dpy->Extensions.MESA_screen_surface = EGL_TRUE;
      egl_g3d_add_screens(drv, dpy);
   }
#endif

   dpy->Extensions.KHR_image_base = EGL_TRUE;
   if (gdpy->native->get_param(gdpy->native, NATIVE_PARAM_USE_NATIVE_BUFFER))
      dpy->Extensions.KHR_image_pixmap = EGL_TRUE;

   if (egl_g3d_add_configs(drv, dpy, 1) == 1) {
      _eglError(EGL_NOT_INITIALIZED, "eglInitialize(unable to add configs)");
      goto fail;
   }

   *major = 1;
   *minor = 4;

   return EGL_TRUE;

fail:
   if (gdpy)
      egl_g3d_terminate(drv, dpy);
   return EGL_FALSE;
}

static _EGLProc
egl_g3d_get_proc_address(_EGLDriver *drv, const char *procname)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   _EGLProc proc;
   EGLint i;

   /* in case this is called before a display is initialized */
   egl_g3d_init_st(&gdrv->base);

   for (i = 0; i < ST_API_COUNT; i++) {
      struct st_api *stapi = gdrv->stapis[i];
      if (stapi) {
         proc = (_EGLProc) stapi->get_proc_address(stapi, procname);
         if (proc)
            return proc;
      }
   }

   return (_EGLProc) NULL;
}

static EGLint
egl_g3d_probe(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct native_probe *nprobe;
   enum native_probe_result res;
   EGLint score;

   nprobe = egl_g3d_get_probe(drv, dpy);
   res = native_get_probe_result(nprobe);

   switch (res) {
   case NATIVE_PROBE_UNKNOWN:
   default:
      score = 0;
      break;
   case NATIVE_PROBE_FALLBACK:
      score = 40;
      break;
   case NATIVE_PROBE_SUPPORTED:
      score = 50;
      break;
   case NATIVE_PROBE_EXACT:
      score = 100;
      break;
   }

   return score;
}

static void
egl_g3d_unload(_EGLDriver *drv)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);

   egl_g3d_destroy_st_apis();
   egl_g3d_destroy_probe(drv, NULL);
   FREE(gdrv);
}

_EGLDriver *
_eglMain(const char *args)
{
   static char driver_name[64];
   struct egl_g3d_driver *gdrv;

   util_snprintf(driver_name, sizeof(driver_name),
         "Gallium/%s", native_get_name());

   gdrv = CALLOC_STRUCT(egl_g3d_driver);
   if (!gdrv)
      return NULL;

   egl_g3d_init_driver_api(&gdrv->base);
   gdrv->base.API.Initialize = egl_g3d_initialize;
   gdrv->base.API.Terminate = egl_g3d_terminate;
   gdrv->base.API.GetProcAddress = egl_g3d_get_proc_address;

   gdrv->base.Name = driver_name;
   gdrv->base.Probe = egl_g3d_probe;
   gdrv->base.Unload = egl_g3d_unload;

   /* the key is " EGL G3D" */
   gdrv->probe_key = 0x0E61063D;

   return &gdrv->base;
}
