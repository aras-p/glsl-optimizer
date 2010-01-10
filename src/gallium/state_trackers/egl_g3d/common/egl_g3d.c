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
#include <string.h>
#include "util/u_memory.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "eglconfigutil.h"
#include "egllog.h"

#include "native.h"
#include "egl_g3d.h"
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
   EGLint num_surfaces;
   EGLint s, i;

   /* validate draw and/or read buffers */
   num_surfaces = (gctx->base.ReadSurface == gctx->base.DrawSurface) ? 1 : 2;
   for (s = 0; s < num_surfaces; s++) {
      struct pipe_texture *textures[NUM_NATIVE_ATTACHMENTS];
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

      if (!gctx->force_validate) {
         EGLint cur_w, cur_h;

         cur_w = gsurf->base.Width;
         cur_h = gsurf->base.Height;
         gsurf->native->validate(gsurf->native,
               gbuf->native_atts, gbuf->num_atts,
               NULL,
               &gsurf->base.Width, &gsurf->base.Height);
         /* validate only when the geometry changed */
         if (gsurf->base.Width == cur_w && gsurf->base.Height == cur_h)
            continue;
      }

      gsurf->native->validate(gsurf->native,
            gbuf->native_atts, gbuf->num_atts,
            (struct pipe_texture **) textures,
            &gsurf->base.Width, &gsurf->base.Height);
      for (i = 0; i < gbuf->num_atts; i++) {
         struct pipe_texture *pt = textures[i];
         struct pipe_surface *ps;

         if (pt) {
            ps = screen->get_tex_surface(screen, pt, 0, 0, 0,
                  PIPE_BUFFER_USAGE_GPU_READ |
                  PIPE_BUFFER_USAGE_GPU_WRITE);
            gctx->stapi->st_set_framebuffer_surface(gbuf->st_fb,
                  gbuf->st_atts[i], ps);

            if (gbuf->native_atts[i] == gsurf->render_att)
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
   const uint st_att_map[NUM_NATIVE_ATTACHMENTS] = {
      ST_SURFACE_FRONT_LEFT,
      ST_SURFACE_BACK_LEFT,
      ST_SURFACE_FRONT_RIGHT,
      ST_SURFACE_BACK_RIGHT,
   };
   EGLint s, i;

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

      gbuf->native_atts[0] = gsurf->render_att;
      gbuf->num_atts = 1;

      for (i = 0; i < gbuf->num_atts; i++)
         gbuf->st_atts[i] = st_att_map[gbuf->native_atts[i]];

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
         gctx->draw.num_atts = 0;
      }

      if (is_equal) {
         gctx->read.st_fb = NULL;
         gctx->draw.num_atts = 0;
      }
      else {
         priv = gctx->stapi->st_framebuffer_private(gctx->read.st_fb);
         if (!gread || priv != (void *) &gread->base) {
            gctx->stapi->st_unreference_framebuffer(gctx->read.st_fb);
            gctx->read.st_fb = NULL;
            gctx->draw.num_atts = 0;
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
 * Return the current context of the given API.
 */
static struct egl_g3d_context *
egl_g3d_get_current_context(EGLint api)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLint api_index = _eglConvertApiToIndex(api);
   return egl_g3d_context(t->CurrentContexts[api_index]);
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

      api_mask = get_mode_api_mask(&native_configs[i]->mode, gdrv->api_mask);
      if (!api_mask) {
         _eglLog(_EGL_DEBUG, "no state tracker supports config 0x%x",
               native_configs[i]->mode.visualID);
         continue;
      }

      gconf = CALLOC_STRUCT(egl_g3d_config);
      if (!gconf)
         continue;

      _eglInitConfig(&gconf->base, id);
      valid = _eglConfigFromContextModesRec(&gconf->base,
            &native_configs[i]->mode, api_mask, api_mask);
      if (valid)
         valid = _eglValidateConfig(&gconf->base, EGL_FALSE);
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
egl_g3d_flush_frontbuffer(void *dummy, struct pipe_surface *surf,
                          void *context_private)
{
   struct egl_g3d_context *gctx = egl_g3d_context(context_private);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(gctx->base.DrawSurface);

   if (gsurf) {
      gsurf->native->flush_frontbuffer(gsurf->native);
      egl_g3d_validate_context(gctx->base.Display, &gctx->base);
   }
}

static EGLBoolean
egl_g3d_terminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);

   _eglReleaseDisplayResources(drv, dpy);
   _eglCleanupDisplay(dpy);

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

   gdpy = CALLOC_STRUCT(egl_g3d_display);
   if (!gdpy) {
      _eglError(EGL_BAD_ALLOC, "eglInitialize");
      goto fail;
   }
   dpy->DriverData = gdpy;

   gdpy->native =
      native_create_display(dpy->NativeDisplay, egl_g3d_flush_frontbuffer);
   if (!gdpy->native) {
      _eglError(EGL_NOT_INITIALIZED, "eglInitialize(no usable display)");
      goto fail;
   }

   dpy->ClientAPIsMask = gdrv->api_mask;

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
   struct egl_g3d_context *xshare = egl_g3d_context(share);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_context *gctx;
   const __GLcontextModes *mode;

   gctx = CALLOC_STRUCT(egl_g3d_context);
   if (!gctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(drv, &gctx->base, conf, attribs)) {
      free(gctx);
      return NULL;
   }

   gctx->stapi = egl_g3d_choose_st(drv, &gctx->base);
   if (!gctx->stapi) {
      free(gctx);
      return NULL;
   }

   mode = &gconf->native->mode;
   gctx->pipe =
      gdpy->native->create_context(gdpy->native, (void *) &gctx->base);
   gctx->st_ctx = gctx->stapi->st_create_context(gctx->pipe, mode,
                        (xshare) ? xshare->st_ctx : NULL);

   return &gctx->base;
}

static EGLBoolean
egl_g3d_destroy_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);

   if (_eglIsContextBound(&gctx->base))
      return EGL_TRUE;

   egl_g3d_realloc_context(dpy, &gctx->base);

   /* it will destroy pipe context */
   gctx->stapi->st_destroy_context(gctx->st_ctx);

   free(gctx);

   return EGL_TRUE;
}

static _EGLSurface *
egl_g3d_create_window_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, EGLNativeWindowType win,
                              const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_surface *gsurf;

   gsurf = CALLOC_STRUCT(egl_g3d_surface);
   if (!gsurf) {
      _eglError(EGL_BAD_ALLOC, "eglCreateWindowSurface");
      return NULL;
   }

   if (!_eglInitSurface(drv, &gsurf->base, EGL_WINDOW_BIT, conf, attribs)) {
      free(gsurf);
      return NULL;
   }

   gsurf->native =
      gdpy->native->create_window_surface(gdpy->native, win, gconf->native);
   if (!gsurf->native) {
      free(gsurf);
      return NULL;
   }

   if (!gsurf->native->validate(gsurf->native, NULL, 0, NULL,
            &gsurf->base.Width, &gsurf->base.Height)) {
      gsurf->native->destroy(gsurf->native);
      free(gsurf);
      return NULL;
   }

   gsurf->render_att = (gsurf->base.RenderBuffer == EGL_SINGLE_BUFFER ||
                        !gconf->native->mode.doubleBufferMode) ?
      NATIVE_ATTACHMENT_FRONT_LEFT : NATIVE_ATTACHMENT_BACK_LEFT;

   return &gsurf->base;
}

static _EGLSurface *
egl_g3d_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, EGLNativePixmapType pix,
                              const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_surface *gsurf;

   gsurf = CALLOC_STRUCT(egl_g3d_surface);
   if (!gsurf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePixmapSurface");
      return NULL;
   }

   if (!_eglInitSurface(drv, &gsurf->base, EGL_PIXMAP_BIT, conf, attribs)) {
      free(gsurf);
      return NULL;
   }

   gsurf->native =
      gdpy->native->create_pixmap_surface(gdpy->native, pix, gconf->native);
   if (!gsurf->native) {
      free(gsurf);
      return NULL;
   }

   if (!gsurf->native->validate(gsurf->native, NULL, 0, NULL,
            &gsurf->base.Width, &gsurf->base.Height)) {
      gsurf->native->destroy(gsurf->native);
      free(gsurf);
      return NULL;
   }

   gsurf->render_att = NATIVE_ATTACHMENT_FRONT_LEFT;

   return &gsurf->base;
}

static _EGLSurface *
egl_g3d_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLConfig *conf, const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_surface *gsurf;

   gsurf = CALLOC_STRUCT(egl_g3d_surface);
   if (!gsurf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePbufferSurface");
      return NULL;
   }

   if (!_eglInitSurface(drv, &gsurf->base, EGL_PBUFFER_BIT, conf, attribs)) {
      free(gsurf);
      return NULL;
   }

   gsurf->native =
      gdpy->native->create_pbuffer_surface(gdpy->native, gconf->native,
            gsurf->base.Width, gsurf->base.Height);
   if (!gsurf->native) {
      free(gsurf);
      return NULL;
   }

   gsurf->render_att = (!gconf->native->mode.doubleBufferMode) ?
      NATIVE_ATTACHMENT_FRONT_LEFT : NATIVE_ATTACHMENT_BACK_LEFT;

   return &gsurf->base;
}

static EGLBoolean
egl_g3d_destroy_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   if (_eglIsSurfaceBound(&gsurf->base))
         return EGL_TRUE;

   pipe_surface_reference(&gsurf->render_surface, NULL);
   gsurf->native->destroy(gsurf->native);
   free(gsurf);
   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_make_current(_EGLDriver *drv, _EGLDisplay *dpy,
                     _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   struct egl_g3d_context *old_gctx;
   EGLint api;
   EGLBoolean ok = EGL_TRUE;

   /* find the old context */
   api = (gctx) ? gctx->base.ClientAPI : eglQueryAPI();
   old_gctx = egl_g3d_get_current_context(api);
   if (old_gctx && !_eglIsContextLinked(&old_gctx->base))
         old_gctx = NULL;

   if (!_eglMakeCurrent(drv, dpy, draw, read, ctx))
      return EGL_FALSE;

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
      struct egl_g3d_surface *gdraw = egl_g3d_surface(draw);

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

   /*
    * We drew on the back buffer, unless there was no back buffer.
    * In that case, we drew on the front buffer.  Either case, we call
    * swap_buffers.
    */
   if (!gsurf->native->swap_buffers(gsurf->native))
      return EGL_FALSE;

   if (gctx) {
      struct egl_g3d_config *gconf = egl_g3d_config(gsurf->base.Config);

      /* force validation if the swap method is not copy */
      if (gconf->native->mode.swapMethod != GLX_SWAP_COPY_OML)
         gctx->force_validate = EGL_TRUE;
      egl_g3d_validate_context(dpy, &gctx->base);
   }

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
   _EGLSurface *surf = _eglGetCurrentSurface(EGL_DRAW);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");

   if (gsurf)
      gsurf->native->wait(gsurf->native);

   return EGL_TRUE;
}

static _EGLProc
egl_g3d_get_proc_address(const char *procname)
{
   /* FIXME how come _EGLDriver is not passed? */
   const struct egl_g3d_st *stapi;
   _EGLProc proc;
   EGLint i;

   for (i = 0; i < NUM_EGL_G3D_STS; i++) {
      stapi = egl_g3d_get_st(i);
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
      target_format = PIPE_FORMAT_A8R8G8B8_UNORM;
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

   /* flush properly if the surface is bound */
   if (gsurf->base.Binding) {
      gctx = egl_g3d_context(gsurf->base.Binding);
      gctx->stapi->st_flush(gctx->st_ctx,
            PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME, NULL);
   }

   /* XXX change to EGL_OPENGL_ES_API once OpenGL ES is merged */
   gctx = egl_g3d_get_current_context(EGL_OPENGL_API);
   if (gctx) {
      if (!gsurf->render_surface)
         return EGL_FALSE;

      gctx->stapi->st_bind_texture_surface(gsurf->render_surface,
            target, gsurf->base.MipmapLevel, target_format);
      gsurf->base.BoundToTexture = EGL_TRUE;
   }

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
      _EGLThreadInfo *t = _eglGetCurrentThread();
      /* XXX change to EGL_OPENGL_ES_API once OpenGL ES is merged */
      struct egl_g3d_context *gctx = egl_g3d_context(
            t->CurrentContexts[_eglConvertApiToIndex(EGL_OPENGL_API)]);

      /* what if the context the surface binds to is no longer current? */
      if (gctx)
         gctx->stapi->st_unbind_texture_surface(gsurf->render_surface,
               ST_TEXTURE_2D, gsurf->base.MipmapLevel);
   }

   gsurf->base.BoundToTexture = EGL_FALSE;

   return EGL_TRUE;
}

static void
egl_g3d_unload(_EGLDriver *drv)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);
   free(gdrv);
}

_EGLDriver *
_eglMain(const char *args)
{
   static char driver_name[64];
   struct egl_g3d_driver *gdrv;
   EGLint i;

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
   gdrv->base.API.WaitClient = egl_g3d_wait_client;
   gdrv->base.API.WaitNative = egl_g3d_wait_native;
   gdrv->base.API.GetProcAddress = egl_g3d_get_proc_address;

   gdrv->base.API.BindTexImage = egl_g3d_bind_tex_image;
   gdrv->base.API.ReleaseTexImage = egl_g3d_release_tex_image;

   gdrv->base.Name = driver_name;
   gdrv->base.Unload = egl_g3d_unload;

   for (i = 0; i < NUM_EGL_G3D_STS; i++) {
      gdrv->stapis[i] = egl_g3d_get_st(i);
      if (gdrv->stapis[i])
         gdrv->api_mask |= gdrv->stapis[i]->api_bit;
   }

   if (gdrv->api_mask)
      _eglLog(_EGL_DEBUG, "Driver API mask: 0x%x", gdrv->api_mask);
   else
      _eglLog(_EGL_WARNING, "No supported client API");

   return &gdrv->base;
}
