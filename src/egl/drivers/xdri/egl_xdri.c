/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * Code to interface a DRI driver to libEGL.
 * Note that unlike previous DRI/EGL interfaces, this one is meant to
 * be used _with_ X.  Applications will use eglCreateWindowSurface()
 * to render into X-created windows.
 *
 * This is an EGL driver that, in turn, loads a regular DRI driver.
 * There are some dependencies on code in libGL, but those could be
 * removed with some effort.
 *
 * Authors: Brian Paul
 */

#include <assert.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "glxinit.h"
#include "driinit.h"
#include "glapi/glapi.h" /* for glapi functions */

#include "eglconfig.h"
#include "eglconfigutil.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"

#define CALLOC_STRUCT(T)   (struct T *) calloc(1, sizeof(struct T))

/** subclass of _EGLDriver */
struct xdri_egl_driver
{
   _EGLDriver Base;   /**< base class */
   void (*FlushCurrentContext)(void);
};


/** driver data of _EGLDisplay */
struct xdri_egl_display
{
   Display *dpy;
   __GLXdisplayPrivate *dpyPriv;
   __GLXDRIdisplay *driDisplay;
   int driVersion;

   __GLXscreenConfigs *psc;
   EGLint scr;
};


/** subclass of _EGLContext */
struct xdri_egl_context
{
   _EGLContext Base;   /**< base class */

   /* just enough info to create dri contexts */
   GLXContext dummy_gc;

   __GLXDRIcontext *driContext;
};


/** subclass of _EGLSurface */
struct xdri_egl_surface
{
   _EGLSurface Base;   /**< base class */

   Drawable drawable;
   __GLXDRIdrawable *driDrawable;
};


/** subclass of _EGLConfig */
struct xdri_egl_config
{
   _EGLConfig Base;   /**< base class */

   const __GLcontextModes *mode;  /**< corresponding GLX mode */
   EGLint window_render_buffer;
};



/** cast wrapper */
static INLINE struct xdri_egl_driver *
xdri_egl_driver(_EGLDriver *drv)
{
   return (struct xdri_egl_driver *) drv;
}


static INLINE struct xdri_egl_display *
lookup_display(_EGLDisplay *dpy)
{
   return (struct xdri_egl_display *) dpy->DriverData;
}


/** Map EGLSurface handle to xdri_egl_surface object */
static INLINE struct xdri_egl_surface *
lookup_surface(_EGLSurface *surface)
{
   return (struct xdri_egl_surface *) surface;
}


/** Map EGLContext handle to xdri_egl_context object */
static INLINE struct xdri_egl_context *
lookup_context(_EGLContext *context)
{
   return (struct xdri_egl_context *) context;
}


/** Map EGLConfig handle to xdri_egl_config object */
static INLINE struct xdri_egl_config *
lookup_config(_EGLConfig *conf)
{
   return (struct xdri_egl_config *) conf;
}


/** Get size of given window */
static Status
get_drawable_size(Display *dpy, Drawable d, uint *width, uint *height)
{
   Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
   return stat;
}


static EGLBoolean
convert_config(_EGLConfig *conf, EGLint id, const __GLcontextModes *m)
{
   EGLint val;

   _eglInitConfig(conf, id);
   if (!_eglConfigFromContextModesRec(conf, m, EGL_OPENGL_BIT, EGL_OPENGL_BIT))
      return EGL_FALSE;

   if (m->doubleBufferMode) {
      /* pixmap and pbuffer surfaces are always single-buffered */
      val = GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE);
      val &= ~(EGL_PIXMAP_BIT | EGL_PBUFFER_BIT);
      SET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE, val);
   }
   else {
      /* EGL requires OpenGL ES context to be double-buffered */
      val = GET_CONFIG_ATTRIB(conf, EGL_RENDERABLE_TYPE);
      val &= ~(EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT);
      SET_CONFIG_ATTRIB(conf, EGL_RENDERABLE_TYPE, val);
   }
   /* skip "empty" config */
   if (!val)
      return EGL_FALSE;

   val = GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE);
   if (!(val & EGL_PBUFFER_BIT)) {
      /* bind-to-texture cannot be EGL_TRUE without pbuffer bit */
      SET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGB, EGL_FALSE);
      SET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGBA, EGL_FALSE);
   }

   /* EGL_NATIVE_RENDERABLE is a boolean */
   val = GET_CONFIG_ATTRIB(conf, EGL_NATIVE_RENDERABLE);
   if (val != EGL_TRUE)
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_RENDERABLE, EGL_FALSE);

   return _eglValidateConfig(conf, EGL_FALSE);
}


/**
 * Produce a set of EGL configs.
 */
static EGLint
create_configs(_EGLDisplay *disp, const __GLcontextModes *m, EGLint first_id)
{
   struct xdri_egl_display *xdri_dpy = lookup_display(disp);
   int id = first_id;

   for (; m; m = m->next) {
      struct xdri_egl_config *xdri_conf;
      _EGLConfig conf;
      EGLint rb;

      if (!convert_config(&conf, id, m))
         continue;
      if (m->doubleBufferMode) {
         rb = EGL_BACK_BUFFER;
      }
      else {
         /* ignore single-buffered mode for DRISW */
         if (xdri_dpy->driVersion == 0)
            continue;
         rb = EGL_SINGLE_BUFFER;
      }

      xdri_conf = CALLOC_STRUCT(xdri_egl_config);
      if (xdri_conf) {
         memcpy(&xdri_conf->Base, &conf, sizeof(conf));
         xdri_conf->mode = m;
         xdri_conf->window_render_buffer = rb;
         _eglAddConfig(disp, &xdri_conf->Base);
         id++;
      }
   }

   return id;
}


/**
 * Called via eglInitialize(), xdri_dpy->API.Initialize().
 */
static EGLBoolean
xdri_eglInitialize(_EGLDriver *drv, _EGLDisplay *dpy,
                   EGLint *minor, EGLint *major)
{
   struct xdri_egl_display *xdri_dpy;
   __GLXdisplayPrivate *dpyPriv;
   __GLXDRIdisplay *driDisplay;
   __GLXscreenConfigs *psc;
   EGLint first_id = 1;
   int scr;

   xdri_dpy = CALLOC_STRUCT(xdri_egl_display);
   if (!xdri_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   xdri_dpy->dpy = (Display *) dpy->NativeDisplay;
   if (!xdri_dpy->dpy) {
      xdri_dpy->dpy = XOpenDisplay(NULL);
      if (!xdri_dpy->dpy) {
         free(xdri_dpy);
         return _eglError(EGL_NOT_INITIALIZED, "eglInitialize");
      }
   }

   dpyPriv = __glXInitialize(xdri_dpy->dpy);
   if (!dpyPriv) {
      _eglLog(_EGL_WARNING, "failed to create GLX display");
      free(xdri_dpy);
      return _eglError(EGL_NOT_INITIALIZED, "eglInitialize");
   }

   driDisplay = __driCreateDisplay(dpyPriv, &xdri_dpy->driVersion);
   if (!driDisplay) {
      _eglLog(_EGL_WARNING, "failed to create DRI display");
      free(xdri_dpy);
      return _eglError(EGL_NOT_INITIALIZED, "eglInitialize");
   }

   scr = DefaultScreen(xdri_dpy->dpy);
   psc = &dpyPriv->screenConfigs[scr];

   xdri_dpy->dpyPriv = dpyPriv;
   xdri_dpy->driDisplay = driDisplay;
   xdri_dpy->psc = psc;
   xdri_dpy->scr = scr;

   psc->driScreen = driDisplay->createScreen(psc, scr, dpyPriv);
   if (!psc->driScreen) {
      _eglLog(_EGL_WARNING, "failed to create DRI screen #%d", scr);
      free(xdri_dpy);
      return _eglError(EGL_NOT_INITIALIZED, "eglInitialize");
   }

   dpy->DriverData = xdri_dpy;
   dpy->ClientAPIsMask = EGL_OPENGL_BIT;

   /* add visuals and fbconfigs */
   first_id = create_configs(dpy, psc->visuals, first_id);
   create_configs(dpy, psc->configs, first_id);

   /* we're supporting EGL 1.4 */
   *minor = 1;
   *major = 4;

   return EGL_TRUE;
}


/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
xdri_eglTerminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct xdri_egl_display *xdri_dpy = lookup_display(dpy);
   __GLXscreenConfigs *psc;

   _eglReleaseDisplayResources(drv, dpy);
   _eglCleanupDisplay(dpy);

   psc = xdri_dpy->psc;
   if (psc->driver_configs) {
      unsigned int i;
      for (i = 0; psc->driver_configs[i]; i++)
         free((__DRIconfig *) psc->driver_configs[i]);
      free(psc->driver_configs);
      psc->driver_configs = NULL;
   }
   if (psc->driScreen) {
      psc->driScreen->destroyScreen(psc);
      free(psc->driScreen);
      psc->driScreen = NULL;
   }

   xdri_dpy->driDisplay->destroyDisplay(xdri_dpy->driDisplay);

   free(xdri_dpy);
   dpy->DriverData = NULL;

   return EGL_TRUE;
}


/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
xdri_eglGetProcAddress(_EGLDriver *drv, const char *procname)
{
   /* the symbol is defined in libGL.so */
   return (_EGLProc) _glapi_get_proc_address(procname);
}


/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static _EGLContext *
xdri_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                      _EGLContext *share_list, const EGLint *attrib_list)
{
   struct xdri_egl_display *xdri_dpy = lookup_display(dpy);
   struct xdri_egl_config *xdri_config = lookup_config(conf);
   struct xdri_egl_context *shared = lookup_context(share_list);
   __GLXscreenConfigs *psc = xdri_dpy->psc;
   int renderType = GLX_RGBA_BIT;
   struct xdri_egl_context *xdri_ctx;

   xdri_ctx = CALLOC_STRUCT(xdri_egl_context);
   if (!xdri_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   xdri_ctx->dummy_gc = CALLOC_STRUCT(__GLXcontextRec);
   if (!xdri_ctx->dummy_gc) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      free(xdri_ctx);
      return NULL;
   }

   if (!_eglInitContext(drv, &xdri_ctx->Base, &xdri_config->Base, attrib_list)) {
      free(xdri_ctx->dummy_gc);
      free(xdri_ctx);
      return NULL;
   }

   /* the config decides the render buffer for the context */
   xdri_ctx->Base.WindowRenderBuffer = xdri_config->window_render_buffer;

   xdri_ctx->driContext =
      psc->driScreen->createContext(psc,
                                    xdri_config->mode,
                                    xdri_ctx->dummy_gc,
                                    (shared) ? shared->dummy_gc : NULL,
                                    renderType);
   if (!xdri_ctx->driContext) {
      free(xdri_ctx->dummy_gc);
      free(xdri_ctx);
      return NULL;
   }

   /* fill in the required field */
   xdri_ctx->dummy_gc->driContext = xdri_ctx->driContext;

   return &xdri_ctx->Base;
}


/**
 * Destroy a context.
 */
static void
destroy_context(_EGLDisplay *dpy, _EGLContext *ctx)
{
   struct xdri_egl_display *xdri_dpy = lookup_display(dpy);
   struct xdri_egl_context *xdri_ctx = lookup_context(ctx);

   /* FIXME a context might live longer than its display */
   if (!dpy->Initialized)
      _eglLog(_EGL_FATAL, "destroy a context with an unitialized display");

   xdri_ctx->driContext->destroyContext(xdri_ctx->driContext,
         xdri_dpy->psc, xdri_dpy->dpy);
   free(xdri_ctx->dummy_gc);
   free(xdri_ctx);
}


/**
 * Destroy a surface.
 */
static void
destroy_surface(_EGLDisplay *dpy, _EGLSurface *surf)
{
   struct xdri_egl_surface *xdri_surf = lookup_surface(surf);

   if (!dpy->Initialized)
      _eglLog(_EGL_FATAL, "destroy a surface with an unitialized display");

   xdri_surf->driDrawable->destroyDrawable(xdri_surf->driDrawable);
   free(xdri_surf);
}


static EGLBoolean
xdri_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   if (!_eglIsContextBound(ctx))
      destroy_context(dpy, ctx);
   return EGL_TRUE;
}


/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
xdri_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *d,
                    _EGLSurface *r, _EGLContext *context)
{
   struct xdri_egl_driver *xdri_driver = xdri_egl_driver(drv);
   struct xdri_egl_context *xdri_ctx = lookup_context(context);
   struct xdri_egl_surface *draw = lookup_surface(d);
   struct xdri_egl_surface *read = lookup_surface(r);

   /* bind the new context and return the "orphaned" one */
   if (!_eglBindContext(&context, &d, &r))
      return EGL_FALSE;

   /* flush before context switch */
   if (context && xdri_driver->FlushCurrentContext)
      xdri_driver->FlushCurrentContext();

   /* the symbol is defined in libGL.so */
   _glapi_check_multithread();

   if (xdri_ctx) {
      if (!xdri_ctx->driContext->bindContext(xdri_ctx->driContext,
                                             draw->driDrawable,
                                             read->driDrawable)) {
         return EGL_FALSE;
      }
   }
   else if (context) {
      xdri_ctx = lookup_context(context);
      xdri_ctx->driContext->unbindContext(xdri_ctx->driContext);
   }

   if (context && !_eglIsContextLinked(context))
      destroy_context(dpy, context);
   if (d && !_eglIsSurfaceLinked(d))
      destroy_surface(dpy, d);
   if (r && r != d && !_eglIsSurfaceLinked(r))
      destroy_surface(dpy, r);

   return EGL_TRUE;
}


/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
xdri_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                            EGLNativeWindowType window,
                            const EGLint *attrib_list)
{
   struct xdri_egl_display *xdri_dpy = lookup_display(dpy);
   struct xdri_egl_config *xdri_config = lookup_config(conf);
   struct xdri_egl_surface *xdri_surf;
   uint width, height;

   xdri_surf = CALLOC_STRUCT(xdri_egl_surface);
   if (!xdri_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreateWindowSurface");
      return NULL;
   }

   if (!_eglInitSurface(drv, &xdri_surf->Base, EGL_WINDOW_BIT,
                        &xdri_config->Base, attrib_list)) {
      free(xdri_surf);
      return NULL;
   }

   xdri_surf->driDrawable =
      xdri_dpy->psc->driScreen->createDrawable(xdri_dpy->psc,
                                               (XID) window,
                                               (GLXDrawable) window,
                                               xdri_config->mode);
   if (!xdri_surf->driDrawable) {
      free(xdri_surf);
      return NULL;
   }

   xdri_surf->drawable = (Drawable) window;

   get_drawable_size(xdri_dpy->dpy, window, &width, &height);
   xdri_surf->Base.Width = width;
   xdri_surf->Base.Height = height;

   return &xdri_surf->Base;
}


/**
 * Called via eglCreatePbufferSurface(), drv->API.CreatePbufferSurface().
 */
static _EGLSurface *
xdri_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                             const EGLint *attrib_list)
{
   return NULL;
}



static EGLBoolean
xdri_eglDestroySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface)
{
   if (!_eglIsSurfaceBound(surface))
      destroy_surface(dpy, surface);
   return EGL_TRUE;
}


static EGLBoolean
xdri_eglBindTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                     EGLint buffer)
{
   return EGL_FALSE;
}


static EGLBoolean
xdri_eglReleaseTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                        EGLint buffer)
{
   return EGL_FALSE;
}


static EGLBoolean
xdri_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw)
{
   struct xdri_egl_driver *xdri_driver = xdri_egl_driver(drv);
   struct xdri_egl_display *xdri_dpy = lookup_display(dpy);
   struct xdri_egl_surface *xdri_surf = lookup_surface(draw);

   /* swapBuffers does not flush commands */
   if (draw->Binding && xdri_driver->FlushCurrentContext)
      xdri_driver->FlushCurrentContext();
 
   xdri_dpy->psc->driScreen->swapBuffers(xdri_surf->driDrawable, 0, 0, 0);

   return EGL_TRUE;
}


static void
xdri_Unload(_EGLDriver *drv)
{
   struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
   free(xdri_drv);
}


/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_eglMain(const char *args)
{
   struct xdri_egl_driver *xdri_drv = CALLOC_STRUCT(xdri_egl_driver);
   if (!xdri_drv)
      return NULL;

   _eglInitDriverFallbacks(&xdri_drv->Base);
   xdri_drv->Base.API.Initialize = xdri_eglInitialize;
   xdri_drv->Base.API.Terminate = xdri_eglTerminate;

   xdri_drv->Base.API.GetProcAddress = xdri_eglGetProcAddress;

   xdri_drv->Base.API.CreateContext = xdri_eglCreateContext;
   xdri_drv->Base.API.DestroyContext = xdri_eglDestroyContext;
   xdri_drv->Base.API.MakeCurrent = xdri_eglMakeCurrent;
   xdri_drv->Base.API.CreateWindowSurface = xdri_eglCreateWindowSurface;
   xdri_drv->Base.API.CreatePbufferSurface = xdri_eglCreatePbufferSurface;
   xdri_drv->Base.API.DestroySurface = xdri_eglDestroySurface;
   xdri_drv->Base.API.BindTexImage = xdri_eglBindTexImage;
   xdri_drv->Base.API.ReleaseTexImage = xdri_eglReleaseTexImage;
   xdri_drv->Base.API.SwapBuffers = xdri_eglSwapBuffers;

   xdri_drv->Base.Name = "X/DRI";
   xdri_drv->Base.Unload = xdri_Unload;

   /* we need a way to flush commands */
   xdri_drv->FlushCurrentContext =
      (void (*)(void)) xdri_eglGetProcAddress(&xdri_drv->Base, "glFlush");

   return &xdri_drv->Base;
}
