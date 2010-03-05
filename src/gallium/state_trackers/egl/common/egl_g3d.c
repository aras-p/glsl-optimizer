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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "eglconfigutil.h"
#include "egllog.h"

#include "native.h"
#include "egl_g3d.h"
#include "egl_g3d_image.h"
#include "egl_st.h"

/**
 * Validate the draw/read surfaces of the context.
 */
static void
egl_g3d_validate_context(_EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_screen *screen = gdpy->native->screen;
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   const uint st_att_map[NUM_NATIVE_ATTACHMENTS] = {
      ST_SURFACE_FRONT_LEFT,
      ST_SURFACE_BACK_LEFT,
      ST_SURFACE_FRONT_RIGHT,
      ST_SURFACE_BACK_RIGHT,
   };
   EGLint num_surfaces, s;

   /* validate draw and/or read buffers */
   num_surfaces = (gctx->base.ReadSurface == gctx->base.DrawSurface) ? 1 : 2;
   for (s = 0; s < num_surfaces; s++) {
      struct pipe_texture *textures[NUM_NATIVE_ATTACHMENTS];
      struct egl_g3d_surface *gsurf;
      struct egl_g3d_buffer *gbuf;
      EGLint att;

      if (s == 0) {
         gsurf = egl_g3d_surface(gctx->base.DrawSurface);
         gbuf = &gctx->draw;
      }
      else {
         gsurf = egl_g3d_surface(gctx->base.ReadSurface);
         gbuf = &gctx->read;
      }

      if (!gctx->force_validate) {
         unsigned int seq_num;

         gsurf->native->validate(gsurf->native, gbuf->attachment_mask,
               &seq_num, NULL, NULL, NULL);
         /* skip validation */
         if (gsurf->sequence_number == seq_num)
            continue;
      }

      pipe_surface_reference(&gsurf->render_surface, NULL);
      memset(textures, 0, sizeof(textures));

      gsurf->native->validate(gsurf->native, gbuf->attachment_mask,
            &gsurf->sequence_number, textures,
            &gsurf->base.Width, &gsurf->base.Height);
      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         struct pipe_texture *pt = textures[att];
         struct pipe_surface *ps;

         if (native_attachment_mask_test(gbuf->attachment_mask, att) && pt) {
            ps = screen->get_tex_surface(screen, pt, 0, 0, 0,
                  PIPE_BUFFER_USAGE_GPU_READ |
                  PIPE_BUFFER_USAGE_GPU_WRITE);
            gctx->stapi->st_set_framebuffer_surface(gbuf->st_fb,
                  st_att_map[att], ps);

            if (gsurf->render_att == att)
               pipe_surface_reference(&gsurf->render_surface, ps);

            pipe_surface_reference(&ps, NULL);
            pipe_texture_reference(&pt, NULL);
         }
      }

      gctx->stapi->st_resize_framebuffer(gbuf->st_fb,
            gsurf->base.Width, gsurf->base.Height);
   }

   gctx->force_validate = EGL_FALSE;

}

/**
 * Create a st_framebuffer.
 */
static struct st_framebuffer *
create_framebuffer(_EGLContext *ctx, _EGLSurface *surf)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct egl_g3d_config *gconf = egl_g3d_config(gsurf->base.Config);

   return gctx->stapi->st_create_framebuffer(&gconf->native->mode,
         gconf->native->color_format, gconf->native->depth_format,
         gconf->native->stencil_format,
         gsurf->base.Width, gsurf->base.Height, &gsurf->base);
}

/**
 * Update the attachments of draw/read surfaces.
 */
static void
egl_g3d_route_context(_EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   EGLint s;

   /* route draw and read buffers' attachments */
   for (s = 0; s < 2; s++) {
      struct egl_g3d_surface *gsurf;
      struct egl_g3d_buffer *gbuf;

      if (s == 0) {
         gsurf = egl_g3d_surface(gctx->base.DrawSurface);
         gbuf = &gctx->draw;
      }
      else {
         gsurf = egl_g3d_surface(gctx->base.ReadSurface);
         gbuf = &gctx->read;
      }

      gbuf->attachment_mask = (1 << gsurf->render_att);

      /* FIXME OpenGL defaults to draw the front or back buffer when the
       * context is single-buffered or double-buffered respectively.  In EGL,
       * however, the buffer to be drawn is determined by the surface, instead
       * of the context.  As a result, rendering to a pixmap surface with a
       * double-buffered context does not work as expected.
       *
       * gctx->stapi->st_draw_front_buffer(gctx->st_ctx, natt ==
       *    NATIVE_ATTACHMENT_FRONT_LEFT);
       */

      /*
       * FIXME If the back buffer is asked for here, and the front buffer is
       * later needed by the client API (e.g. glDrawBuffer is called to draw
       * the front buffer), it will create a new pipe texture and draw there.
       * One fix is to ask for both buffers here, but it would be a waste if
       * the front buffer is never used.  A better fix is to add a callback to
       * the pipe screen with context private (just like flush_frontbuffer).
       */
   }
}

/**
 * Reallocate the context's framebuffers after draw/read surfaces change.
 */
static EGLBoolean
egl_g3d_realloc_context(_EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   struct egl_g3d_surface *gdraw = egl_g3d_surface(gctx->base.DrawSurface);
   struct egl_g3d_surface *gread = egl_g3d_surface(gctx->base.ReadSurface);

   /* unreference the old framebuffers */
   if (gctx->draw.st_fb) {
      EGLBoolean is_equal = (gctx->draw.st_fb == gctx->read.st_fb);
      void *priv;

      priv = gctx->stapi->st_framebuffer_private(gctx->draw.st_fb);
      if (!gdraw || priv != (void *) &gdraw->base) {
         gctx->stapi->st_unreference_framebuffer(gctx->draw.st_fb);
         gctx->draw.st_fb = NULL;
         gctx->draw.attachment_mask = 0x0;
      }

      if (is_equal) {
         gctx->read.st_fb = NULL;
         gctx->draw.attachment_mask = 0x0;
      }
      else {
         priv = gctx->stapi->st_framebuffer_private(gctx->read.st_fb);
         if (!gread || priv != (void *) &gread->base) {
            gctx->stapi->st_unreference_framebuffer(gctx->read.st_fb);
            gctx->read.st_fb = NULL;
            gctx->draw.attachment_mask = 0x0;
         }
      }
   }

   if (!gdraw)
      return EGL_TRUE;

   /* create the draw fb */
   if (!gctx->draw.st_fb) {
      gctx->draw.st_fb = create_framebuffer(&gctx->base, &gdraw->base);
      if (!gctx->draw.st_fb)
         return EGL_FALSE;
   }

   /* create the read fb */
   if (!gctx->read.st_fb) {
      if (gread != gdraw) {
         gctx->read.st_fb = create_framebuffer(&gctx->base, &gread->base);
         if (!gctx->read.st_fb) {
            gctx->stapi->st_unreference_framebuffer(gctx->draw.st_fb);
            gctx->draw.st_fb = NULL;
            return EGL_FALSE;
         }
      }
      else {
         /* there is no st_reference_framebuffer... */
         gctx->read.st_fb = gctx->draw.st_fb;
      }
   }

   egl_g3d_route_context(dpy, &gctx->base);
   gctx->force_validate = EGL_TRUE;

   return EGL_TRUE;
}

/**
 * Return the state tracker for the given context.
 */
static const struct egl_g3d_st *
egl_g3d_choose_st(_EGLDriver *drv, _EGLContext *ctx)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   const struct egl_g3d_st *stapi;
   EGLint idx = -1;

   switch (ctx->ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (ctx->ClientVersion) {
      case 1:
         idx = EGL_G3D_ST_OPENGL_ES;
         break;
      case 2:
         idx = EGL_G3D_ST_OPENGL_ES2;
         break;
      default:
         _eglLog(_EGL_WARNING, "unknown client version %d",
               ctx->ClientVersion);
         break;
      }
      break;
   case EGL_OPENVG_API:
      idx = EGL_G3D_ST_OPENVG;
      break;
   case EGL_OPENGL_API:
      idx = EGL_G3D_ST_OPENGL;
      break;
   default:
      _eglLog(_EGL_WARNING, "unknown client API 0x%04x", ctx->ClientAPI);
      break;
   }

   stapi = (idx >= 0) ? gdrv->stapis[idx] : NULL;
   return stapi;
}

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

   for (i = 0; i < NUM_EGL_G3D_STS; i++) {
      gdrv->stapis[i] = egl_g3d_get_st(i);
      if (gdrv->stapis[i])
         gdrv->api_mask |= gdrv->stapis[i]->api_bit;
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

/**
 * Return an API mask that consists of the state trackers that supports the
 * given mode.
 *
 * FIXME add st_is_mode_supported()?
 */
static EGLint
get_mode_api_mask(const __GLcontextModes *mode, EGLint api_mask)
{
   EGLint check;

   /* OpenGL ES 1.x and 2.x are checked together */
   check = EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT;
   if (api_mask & check) {
      /* this is required by EGL, not by OpenGL ES */
      if (mode->drawableType & GLX_WINDOW_BIT && !mode->doubleBufferMode)
         api_mask &= ~check;
   }

   check = EGL_OPENVG_BIT;
   if (api_mask & check) {
      /* vega st needs the depth/stencil rb */
      if (!mode->depthBits && !mode->stencilBits)
         api_mask &= ~check;
   }

   return api_mask;
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
         free(native_connectors);
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
            free(native_modes);
         continue;
      }

      gscr = CALLOC_STRUCT(egl_g3d_screen);
      if (!gscr) {
         free(native_modes);
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

   free(native_connectors);
}

#endif /* EGL_MESA_screen_surface */

/**
 * Add configs to display and return the next config ID.
 */
static EGLint
egl_g3d_add_configs(_EGLDriver *drv, _EGLDisplay *dpy, EGLint id)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   const struct native_config **native_configs;
   int num_configs, i;

   native_configs = gdpy->native->get_configs(gdpy->native,
         &num_configs);
   if (!num_configs) {
      if (native_configs)
         free(native_configs);
      return id;
   }

   for (i = 0; i < num_configs; i++) {
      EGLint api_mask;
      struct egl_g3d_config *gconf;
      EGLBoolean valid;

      gconf = CALLOC_STRUCT(egl_g3d_config);
      if (!gconf)
         continue;

      _eglInitConfig(&gconf->base, dpy, id);

      api_mask = get_mode_api_mask(&native_configs[i]->mode, gdrv->api_mask);
      if (!api_mask) {
         _eglLog(_EGL_DEBUG, "no state tracker supports config 0x%x",
               native_configs[i]->mode.visualID);
      }

      valid = _eglConfigFromContextModesRec(&gconf->base,
            &native_configs[i]->mode, api_mask, api_mask);
      if (valid) {
#ifdef EGL_MESA_screen_surface
         /* check if scanout surface bit is set */
         if (native_configs[i]->scanout_bit) {
            EGLint val = GET_CONFIG_ATTRIB(&gconf->base, EGL_SURFACE_TYPE);
            val |= EGL_SCREEN_BIT_MESA;
            SET_CONFIG_ATTRIB(&gconf->base, EGL_SURFACE_TYPE, val);
         }
#endif
         valid = _eglValidateConfig(&gconf->base, EGL_FALSE);
      }
      if (!valid) {
         _eglLog(_EGL_DEBUG, "skip invalid config 0x%x",
               native_configs[i]->mode.visualID);
         free(gconf);
         continue;
      }

      gconf->native = native_configs[i];
      _eglAddConfig(dpy, &gconf->base);
      id++;
   }

   free(native_configs);
   return id;
}

/**
 * Flush the front buffer of the context's draw surface.
 */
static void
egl_g3d_flush_frontbuffer(struct pipe_screen *screen,
                          struct pipe_surface *surf, void *context_private)
{
   struct egl_g3d_context *gctx = egl_g3d_context(context_private);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(gctx->base.DrawSurface);

   if (gsurf)
      gsurf->native->flush_frontbuffer(gsurf->native);
}

/**
 * Re-validate the context.
 */
static void
egl_g3d_update_buffer(struct pipe_screen *screen, void *context_private)
{
   struct egl_g3d_context *gctx = egl_g3d_context(context_private);
   egl_g3d_validate_context(gctx->base.Resource.Display, &gctx->base);
}

static void
egl_g3d_invalid_surface(struct native_display *ndpy,
                        struct native_surface *nsurf,
                        unsigned int seq_num)
{
   /* XXX not thread safe? */
   struct egl_g3d_surface *gsurf = egl_g3d_surface(nsurf->user_data);
   struct egl_g3d_context *gctx = egl_g3d_context(gsurf->base.CurrentContext);

   /* set force_validate to skip an unnecessary check */
   if (gctx)
      gctx->force_validate = TRUE;
}

static struct native_event_handler egl_g3d_native_event_handler = {
   .invalid_surface = egl_g3d_invalid_surface
};

static EGLBoolean
egl_g3d_terminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   EGLint i;

   _eglReleaseDisplayResources(drv, dpy);
   _eglCleanupDisplay(dpy);

   if (dpy->Screens) {
      for (i = 0; i < dpy->NumScreens; i++) {
         struct egl_g3d_screen *gscr = egl_g3d_screen(dpy->Screens[i]);
         free(gscr->native_modes);
         free(gscr);
      }
      free(dpy->Screens);
   }

   if (gdpy->native)
      gdpy->native->destroy(gdpy->native);

   free(gdpy);
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
   gdpy->native->screen->flush_frontbuffer = egl_g3d_flush_frontbuffer;
   gdpy->native->screen->update_buffer = egl_g3d_update_buffer;

   egl_g3d_init_st(&gdrv->base);
   dpy->ClientAPIsMask = gdrv->api_mask;

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

static _EGLContext *
egl_g3d_create_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                       _EGLContext *share, const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_context *gshare = egl_g3d_context(share);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_context *gctx;
   const __GLcontextModes *mode;

   gctx = CALLOC_STRUCT(egl_g3d_context);
   if (!gctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&gctx->base, dpy, conf, attribs)) {
      free(gctx);
      return NULL;
   }

   gctx->stapi = egl_g3d_choose_st(drv, &gctx->base);
   if (!gctx->stapi) {
      free(gctx);
      return NULL;
   }

   mode = &gconf->native->mode;

   gctx->pipe = gdpy->native->screen->context_create(
      gdpy->native->screen,
      (void *) &gctx->base);

   if (!gctx->pipe) {
      free(gctx);
      return NULL;
   }

   gctx->st_ctx = gctx->stapi->st_create_context(gctx->pipe, mode,
                        (gshare) ? gshare->st_ctx : NULL);
   if (!gctx->st_ctx) {
      gctx->pipe->destroy(gctx->pipe);
      free(gctx);
      return NULL;
   }

   return &gctx->base;
}

/**
 * Destroy a context.
 */
static void
destroy_context(_EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);

   /* FIXME a context might live longer than its display */
   if (!dpy->Initialized)
      _eglLog(_EGL_FATAL, "destroy a context with an unitialized display");

   egl_g3d_realloc_context(dpy, &gctx->base);
   /* it will destroy the associated pipe context */
   gctx->stapi->st_destroy_context(gctx->st_ctx);

   free(gctx);
}

static EGLBoolean
egl_g3d_destroy_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   if (!_eglIsContextBound(ctx))
      destroy_context(dpy, ctx);
   return EGL_TRUE;
}

struct egl_g3d_create_surface_arg {
   EGLint type;
   union {
      EGLNativeWindowType win;
      EGLNativePixmapType pix;
   } u;
};

static _EGLSurface *
egl_g3d_create_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                       struct egl_g3d_create_surface_arg *arg,
                       const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_surface *gsurf;
   struct native_surface *nsurf;
   const char *err;

   switch (arg->type) {
   case EGL_WINDOW_BIT:
      err = "eglCreateWindowSurface";
      break;
   case EGL_PIXMAP_BIT:
      err = "eglCreatePixmapSurface";
      break;
   case EGL_PBUFFER_BIT:
      err = "eglCreatePBufferSurface";
      break;
#ifdef EGL_MESA_screen_surface
   case EGL_SCREEN_BIT_MESA:
      err = "eglCreateScreenSurface";
      break;
#endif
   default:
      err = "eglCreateUnknownSurface";
      break;
   }

   gsurf = CALLOC_STRUCT(egl_g3d_surface);
   if (!gsurf) {
      _eglError(EGL_BAD_ALLOC, err);
      return NULL;
   }

   if (!_eglInitSurface(&gsurf->base, dpy, arg->type, conf, attribs)) {
      free(gsurf);
      return NULL;
   }

   /* create the native surface */
   switch (arg->type) {
   case EGL_WINDOW_BIT:
      nsurf = gdpy->native->create_window_surface(gdpy->native,
            arg->u.win, gconf->native);
      break;
   case EGL_PIXMAP_BIT:
      nsurf = gdpy->native->create_pixmap_surface(gdpy->native,
            arg->u.pix, gconf->native);
      break;
   case EGL_PBUFFER_BIT:
      nsurf = gdpy->native->create_pbuffer_surface(gdpy->native,
            gconf->native, gsurf->base.Width, gsurf->base.Height);
      break;
#ifdef EGL_MESA_screen_surface
   case EGL_SCREEN_BIT_MESA:
      /* prefer back buffer (move to _eglInitSurface?) */
      gsurf->base.RenderBuffer = EGL_BACK_BUFFER;
      nsurf = gdpy->native->modeset->create_scanout_surface(gdpy->native,
            gconf->native, gsurf->base.Width, gsurf->base.Height);
      break;
#endif
   default:
      nsurf = NULL;
      break;
   }

   if (!nsurf) {
      free(gsurf);
      return NULL;
   }
   /* initialize the geometry */
   if (!nsurf->validate(nsurf, 0x0, &gsurf->sequence_number, NULL,
            &gsurf->base.Width, &gsurf->base.Height)) {
      nsurf->destroy(nsurf);
      free(gsurf);
      return NULL;
   }

   nsurf->user_data = &gsurf->base;
   gsurf->native = nsurf;

   gsurf->render_att = (gsurf->base.RenderBuffer == EGL_SINGLE_BUFFER) ?
      NATIVE_ATTACHMENT_FRONT_LEFT : NATIVE_ATTACHMENT_BACK_LEFT;
   if (!gconf->native->mode.doubleBufferMode)
      gsurf->render_att = NATIVE_ATTACHMENT_FRONT_LEFT;

   return &gsurf->base;
}

static _EGLSurface *
egl_g3d_create_window_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, EGLNativeWindowType win,
                              const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_WINDOW_BIT;
   arg.u.win = win;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

static _EGLSurface *
egl_g3d_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, EGLNativePixmapType pix,
                              const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_PIXMAP_BIT;
   arg.u.pix = pix;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

static _EGLSurface *
egl_g3d_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLConfig *conf, const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_PBUFFER_BIT;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

/**
 * Destroy a surface.
 */
static void
destroy_surface(_EGLDisplay *dpy, _EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   /* FIXME a surface might live longer than its display */
   if (!dpy->Initialized)
      _eglLog(_EGL_FATAL, "destroy a surface with an unitialized display");

   pipe_surface_reference(&gsurf->render_surface, NULL);
   gsurf->native->destroy(gsurf->native);
   free(gsurf);
}

static EGLBoolean
egl_g3d_destroy_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   if (!_eglIsSurfaceBound(surf))
      destroy_surface(dpy, surf);
   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_make_current(_EGLDriver *drv, _EGLDisplay *dpy,
                     _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   struct egl_g3d_surface *gdraw = egl_g3d_surface(draw);
   struct egl_g3d_context *old_gctx;
   EGLBoolean ok = EGL_TRUE;

   /* bind the new context and return the "orphaned" one */
   if (!_eglBindContext(&ctx, &draw, &read))
      return EGL_FALSE;
   old_gctx = egl_g3d_context(ctx);

   if (old_gctx) {
      /* flush old context */
      old_gctx->stapi->st_flush(old_gctx->st_ctx,
            PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME, NULL);

      /*
       * The old context is no longer current, and egl_g3d_realloc_context()
       * should be called to destroy the framebuffers.  However, it is possible
       * that it will be made current again with the same draw/read surfaces.
       * It might be better to keep it around.
       */
   }

   if (gctx) {
      ok = egl_g3d_realloc_context(dpy, &gctx->base);
      if (ok) {
         ok = gctx->stapi->st_make_current(gctx->st_ctx,
               gctx->draw.st_fb, gctx->read.st_fb);
         if (ok) {
            egl_g3d_validate_context(dpy, &gctx->base);
            if (gdraw->base.Type == EGL_WINDOW_BIT) {
               gctx->base.WindowRenderBuffer =
                  (gdraw->render_att == NATIVE_ATTACHMENT_FRONT_LEFT) ?
                  EGL_SINGLE_BUFFER : EGL_BACK_BUFFER;
            }
         }
      }
   }
   else if (old_gctx) {
      ok = old_gctx->stapi->st_make_current(NULL, NULL, NULL);
      old_gctx->base.WindowRenderBuffer = EGL_NONE;
   }

   if (ctx && !_eglIsContextLinked(ctx))
      destroy_context(dpy, ctx);
   if (draw && !_eglIsSurfaceLinked(draw))
      destroy_surface(dpy, draw);
   if (read && read != draw && !_eglIsSurfaceLinked(read))
      destroy_surface(dpy, read);

   return ok;
}

static EGLBoolean
egl_g3d_swap_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   _EGLContext *ctx = _eglGetCurrentContext();
   struct egl_g3d_context *gctx = NULL;

   /* no-op for pixmap or pbuffer surface */
   if (gsurf->base.Type == EGL_PIXMAP_BIT ||
       gsurf->base.Type == EGL_PBUFFER_BIT)
      return EGL_TRUE;

   /* or when the surface is single-buffered */
   if (gsurf->render_att == NATIVE_ATTACHMENT_FRONT_LEFT)
      return EGL_TRUE;

   if (ctx && ctx->DrawSurface == surf)
      gctx = egl_g3d_context(ctx);

   /* flush if the surface is current */
   if (gctx)
      gctx->stapi->st_notify_swapbuffers(gctx->draw.st_fb);

   return gsurf->native->swap_buffers(gsurf->native);
}

/**
 * Find a config that supports the pixmap.
 */
_EGLConfig *
egl_g3d_find_pixmap_config(_EGLDisplay *dpy, EGLNativePixmapType pix)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf;
   EGLint i;

   for (i = 0; i < dpy->NumConfigs; i++) {
      gconf = egl_g3d_config(dpy->Configs[i]);
      if (gdpy->native->is_pixmap_supported(gdpy->native, pix, gconf->native))
         break;
   }

   return (i < dpy->NumConfigs) ? &gconf->base : NULL;
}

/**
 * Get the pipe surface of the given attachment of the native surface.
 */
static struct pipe_surface *
get_pipe_surface(struct native_display *ndpy, struct native_surface *nsurf,
                 enum native_attachment natt)
{
   struct pipe_texture *textures[NUM_NATIVE_ATTACHMENTS];
   struct pipe_surface *psurf;

   textures[natt] = NULL;
   nsurf->validate(nsurf, 1 << natt, NULL, textures, NULL, NULL);
   if (!textures[natt])
      return NULL;

   psurf = ndpy->screen->get_tex_surface(ndpy->screen, textures[natt],
         0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE);
   pipe_texture_reference(&textures[natt], NULL);

   return psurf;
}

static EGLBoolean
egl_g3d_copy_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                     EGLNativePixmapType target)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   _EGLContext *ctx = _eglGetCurrentContext();
   struct egl_g3d_config *gconf;
   struct native_surface *nsurf;
   struct pipe_screen *screen = gdpy->native->screen;
   struct pipe_surface *psurf;

   if (!gsurf->render_surface)
      return EGL_TRUE;

   gconf = egl_g3d_config(egl_g3d_find_pixmap_config(dpy, target));
   if (!gconf)
      return _eglError(EGL_BAD_NATIVE_PIXMAP, "eglCopyBuffers");

   nsurf = gdpy->native->create_pixmap_surface(gdpy->native,
         target, gconf->native);
   if (!nsurf)
      return _eglError(EGL_BAD_NATIVE_PIXMAP, "eglCopyBuffers");

   /* flush if the surface is current */
   if (ctx && ctx->DrawSurface == &gsurf->base) {
      struct egl_g3d_context *gctx = egl_g3d_context(ctx);
      gctx->stapi->st_flush(gctx->st_ctx,
            PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME, NULL);
   }

   psurf = get_pipe_surface(gdpy->native, nsurf, NATIVE_ATTACHMENT_FRONT_LEFT);
   if (psurf) {
      struct pipe_context pipe;

      /**
       * XXX This is hacky.  If we might allow the EGLDisplay to create a pipe
       * context of its own and use the blitter context for this.
       */
      memset(&pipe, 0, sizeof(pipe));
      pipe.screen = screen;

      util_surface_copy(&pipe, FALSE, psurf, 0, 0,
            gsurf->render_surface, 0, 0, psurf->width, psurf->height);

      pipe_surface_reference(&psurf, NULL);
      nsurf->flush_frontbuffer(nsurf);
   }

   nsurf->destroy(nsurf);

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_wait_client(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   gctx->stapi->st_finish(gctx->st_ctx);
   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_wait_native(_EGLDriver *drv, _EGLDisplay *dpy, EGLint engine)
{
   _EGLContext *ctx = _eglGetCurrentContext();

   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");

   if (ctx && ctx->DrawSurface) {
      struct egl_g3d_surface *gsurf = egl_g3d_surface(ctx->DrawSurface);
      gsurf->native->wait(gsurf->native);
   }

   return EGL_TRUE;
}

static _EGLProc
egl_g3d_get_proc_address(_EGLDriver *drv, const char *procname)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   _EGLProc proc;
   EGLint i;

   /* in case this is called before a display is initialized */
   egl_g3d_init_st(&gdrv->base);

   for (i = 0; i < NUM_EGL_G3D_STS; i++) {
      const struct egl_g3d_st *stapi = gdrv->stapis[i];
      if (stapi) {
         proc = (_EGLProc) stapi->st_get_proc_address(procname);
         if (proc)
            return proc;
      }
   }

   return (_EGLProc) NULL;
}

static EGLBoolean
egl_g3d_bind_tex_image(_EGLDriver *drv, _EGLDisplay *dpy,
                       _EGLSurface *surf, EGLint buffer)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   _EGLContext *es1 = _eglGetAPIContext(EGL_OPENGL_ES_API);
   struct egl_g3d_context *gctx;
   enum pipe_format target_format;
   int target;

   if (!gsurf || gsurf->base.Type != EGL_PBUFFER_BIT)
      return _eglError(EGL_BAD_SURFACE, "eglBindTexImage");
   if (buffer != EGL_BACK_BUFFER)
      return _eglError(EGL_BAD_PARAMETER, "eglBindTexImage");
   if (gsurf->base.BoundToTexture)
      return _eglError(EGL_BAD_ACCESS, "eglBindTexImage");

   switch (gsurf->base.TextureFormat) {
   case EGL_TEXTURE_RGB:
      target_format = PIPE_FORMAT_R8G8B8_UNORM;
      break;
   case EGL_TEXTURE_RGBA:
      target_format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      return _eglError(EGL_BAD_MATCH, "eglBindTexImage");
   }

   switch (gsurf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = ST_TEXTURE_2D;
      break;
   default:
      return _eglError(EGL_BAD_MATCH, "eglBindTexImage");
   }

   if (!es1)
      return EGL_TRUE;
   if (!gsurf->render_surface)
      return EGL_FALSE;

   /* flush properly if the surface is bound */
   if (gsurf->base.CurrentContext) {
      gctx = egl_g3d_context(gsurf->base.CurrentContext);
      gctx->stapi->st_flush(gctx->st_ctx,
            PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME, NULL);
   }

   gctx = egl_g3d_context(es1);
   gctx->stapi->st_bind_texture_surface(gsurf->render_surface,
         target, gsurf->base.MipmapLevel, target_format);

   gsurf->base.BoundToTexture = EGL_TRUE;

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_release_tex_image(_EGLDriver *drv, _EGLDisplay *dpy,
                          _EGLSurface *surf, EGLint buffer)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   if (!gsurf || gsurf->base.Type != EGL_PBUFFER_BIT ||
       !gsurf->base.BoundToTexture)
      return _eglError(EGL_BAD_SURFACE, "eglReleaseTexImage");
   if (buffer != EGL_BACK_BUFFER)
      return _eglError(EGL_BAD_PARAMETER, "eglReleaseTexImage");

   if (gsurf->render_surface) {
      _EGLContext *ctx = _eglGetAPIContext(EGL_OPENGL_ES_API);
      struct egl_g3d_context *gctx = egl_g3d_context(ctx);

      /* what if the context the surface binds to is no longer current? */
      if (gctx)
         gctx->stapi->st_unbind_texture_surface(gsurf->render_surface,
               ST_TEXTURE_2D, gsurf->base.MipmapLevel);
   }

   gsurf->base.BoundToTexture = EGL_FALSE;

   return EGL_TRUE;
}

#ifdef EGL_MESA_screen_surface

static _EGLSurface *
egl_g3d_create_screen_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_SCREEN_BIT_MESA;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

static EGLBoolean
egl_g3d_show_screen_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                            _EGLScreen *scr, _EGLSurface *surf,
                            _EGLMode *mode)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_screen *gscr = egl_g3d_screen(scr);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct native_surface *nsurf;
   const struct native_mode *nmode;
   EGLBoolean changed;

   if (gsurf) {
      EGLint idx;

      if (!mode)
         return _eglError(EGL_BAD_MATCH, "eglShowSurfaceMESA");
      if (gsurf->base.Type != EGL_SCREEN_BIT_MESA)
         return _eglError(EGL_BAD_SURFACE, "eglShowScreenSurfaceMESA");
      if (gsurf->base.Width < mode->Width || gsurf->base.Height < mode->Height)
         return _eglError(EGL_BAD_MATCH,
               "eglShowSurfaceMESA(surface smaller than mode size)");

      /* find the index of the mode */
      for (idx = 0; idx < gscr->base.NumModes; idx++)
         if (mode == &gscr->base.Modes[idx])
            break;
      if (idx >= gscr->base.NumModes) {
         return _eglError(EGL_BAD_MODE_MESA,
               "eglShowSurfaceMESA(unknown mode)");
      }

      nsurf = gsurf->native;
      nmode = gscr->native_modes[idx];
   }
   else {
      if (mode)
         return _eglError(EGL_BAD_MATCH, "eglShowSurfaceMESA");

      /* disable the screen */
      nsurf = NULL;
      nmode = NULL;
   }

   /* TODO surface panning by CRTC choosing */
   changed = gdpy->native->modeset->program(gdpy->native, 0, nsurf,
         gscr->base.OriginX, gscr->base.OriginY, &gscr->native, 1, nmode);
   if (changed) {
      gscr->base.CurrentSurface = &gsurf->base;
      gscr->base.CurrentMode = mode;
   }

   return changed;
}

#endif /* EGL_MESA_screen_surface */

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

   egl_g3d_destroy_probe(drv, NULL);
   free(gdrv);
}

_EGLDriver *
_eglMain(const char *args)
{
   static char driver_name[64];
   struct egl_g3d_driver *gdrv;

   snprintf(driver_name, sizeof(driver_name),
         "Gallium/%s", native_get_name());

   gdrv = CALLOC_STRUCT(egl_g3d_driver);
   if (!gdrv)
      return NULL;

   _eglInitDriverFallbacks(&gdrv->base);

   gdrv->base.API.Initialize = egl_g3d_initialize;
   gdrv->base.API.Terminate = egl_g3d_terminate;
   gdrv->base.API.CreateContext = egl_g3d_create_context;
   gdrv->base.API.DestroyContext = egl_g3d_destroy_context;
   gdrv->base.API.CreateWindowSurface = egl_g3d_create_window_surface;
   gdrv->base.API.CreatePixmapSurface = egl_g3d_create_pixmap_surface;
   gdrv->base.API.CreatePbufferSurface = egl_g3d_create_pbuffer_surface;
   gdrv->base.API.DestroySurface = egl_g3d_destroy_surface;
   gdrv->base.API.MakeCurrent = egl_g3d_make_current;
   gdrv->base.API.SwapBuffers = egl_g3d_swap_buffers;
   gdrv->base.API.CopyBuffers = egl_g3d_copy_buffers;
   gdrv->base.API.WaitClient = egl_g3d_wait_client;
   gdrv->base.API.WaitNative = egl_g3d_wait_native;
   gdrv->base.API.GetProcAddress = egl_g3d_get_proc_address;

   gdrv->base.API.BindTexImage = egl_g3d_bind_tex_image;
   gdrv->base.API.ReleaseTexImage = egl_g3d_release_tex_image;

   gdrv->base.API.CreateImageKHR = egl_g3d_create_image;
   gdrv->base.API.DestroyImageKHR = egl_g3d_destroy_image;

#ifdef EGL_MESA_screen_surface
   gdrv->base.API.CreateScreenSurfaceMESA = egl_g3d_create_screen_surface;
   gdrv->base.API.ShowScreenSurfaceMESA = egl_g3d_show_screen_surface;
#endif

   gdrv->base.Name = driver_name;
   gdrv->base.Probe = egl_g3d_probe;
   gdrv->base.Unload = egl_g3d_unload;

   /* the key is " EGL G3D" */
   gdrv->probe_key = 0x0E61063D;

   return &gdrv->base;
}
