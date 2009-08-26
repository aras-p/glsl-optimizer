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
 * EGL / softpipe / xlib winsys module
 *
 * Authors: Brian Paul
 */


#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "pipe/internal/p_winsys_screen.h"
#include "util/u_memory.h"
#include "softpipe/sp_winsys.h"
#include "softpipe/sp_texture.h"

#include "eglconfig.h"
#include "eglconfigutil.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglsurface.h"

#include "state_tracker/st_public.h"

#include "sw_winsys.h"


/** subclass of _EGLDriver */
struct xlib_egl_driver
{
   _EGLDriver Base;   /**< base class */
   EGLint apis;
};


/** driver data of _EGLDisplay */
struct xlib_egl_display
{
   Display *Dpy;

   struct pipe_winsys *winsys;
   struct pipe_screen *screen;
};


/** subclass of _EGLContext */
struct xlib_egl_context
{
   _EGLContext Base;   /**< base class */

   struct pipe_context *pipe;   /**< Gallium driver context */
   struct st_context *Context;  /**< Mesa/gallium state tracker context */
};


/** subclass of _EGLSurface */
struct xlib_egl_surface
{
   _EGLSurface Base;   /**< base class */

   /* These are set for window surface */
   Display *Dpy;  /**< The X Display of the window */
   Window Win;    /**< The user-created window ID */
   GC Gc;
   XVisualInfo VisInfo;

   struct pipe_winsys *winsys;

   struct st_framebuffer *Framebuffer;
};


static void
flush_frontbuffer(struct pipe_winsys *pws,
                  struct pipe_surface *psurf,
                  void *context_private);


/** cast wrapper */
static INLINE struct xlib_egl_driver *
xlib_egl_driver(_EGLDriver *drv)
{
   return (struct xlib_egl_driver *) drv;
}


static INLINE struct xlib_egl_display *
xlib_egl_display(_EGLDisplay *dpy)
{
   return (struct xlib_egl_display *) dpy->DriverData;
}


static INLINE struct xlib_egl_surface *
lookup_surface(_EGLSurface *surf)
{
   return (struct xlib_egl_surface *) surf;
}


static INLINE struct xlib_egl_context *
lookup_context(_EGLContext *ctx)
{
   return (struct xlib_egl_context *) ctx;
}


static unsigned int
bitcount(unsigned int n)
{
   unsigned int bits;
   for (bits = 0; n > 0; n = n >> 1) {
      bits += (n & 1);
   }
   return bits;
}


/**
 * Create the EGLConfigs.  (one per X visual)
 */
static void
create_configs(struct xlib_egl_display *xdpy, _EGLDisplay *disp)
{
   static const EGLint all_apis = (EGL_OPENGL_ES_BIT |
                                   EGL_OPENGL_ES2_BIT |
                                   EGL_OPENVG_BIT |
                                   EGL_OPENGL_BIT);
   XVisualInfo *visInfo, visTemplate;
   int num_visuals, i;

   /* get list of all X visuals, create an EGL config for each */
   visTemplate.screen = DefaultScreen(xdpy->Dpy);
   visInfo = XGetVisualInfo(xdpy->Dpy, VisualScreenMask,
                            &visTemplate, &num_visuals);
   if (!visInfo) {
      printf("egl_xlib.c: couldn't get any X visuals\n");
      abort();
   }

   for (i = 0; i < num_visuals; i++) {
      _EGLConfig *config = calloc(1, sizeof(_EGLConfig));
      int id = i + 1;
      int rbits = bitcount(visInfo[i].red_mask);
      int gbits = bitcount(visInfo[i].green_mask);
      int bbits = bitcount(visInfo[i].blue_mask);
      int abits = bbits == 8 ? 8 : 0;
      int zbits = 24;
      int sbits = 8;
      int visid = visInfo[i].visualid;
#if defined(__cplusplus) || defined(c_plusplus)
      int vistype = visInfo[i].c_class;
#else
      int vistype = visInfo[i].class;
#endif

      _eglInitConfig(config, id);
      SET_CONFIG_ATTRIB(config, EGL_BUFFER_SIZE, rbits + gbits + bbits + abits);
      SET_CONFIG_ATTRIB(config, EGL_RED_SIZE, rbits);
      SET_CONFIG_ATTRIB(config, EGL_GREEN_SIZE, gbits);
      SET_CONFIG_ATTRIB(config, EGL_BLUE_SIZE, bbits);
      SET_CONFIG_ATTRIB(config, EGL_ALPHA_SIZE, abits);
      SET_CONFIG_ATTRIB(config, EGL_DEPTH_SIZE, zbits);
      SET_CONFIG_ATTRIB(config, EGL_STENCIL_SIZE, sbits);
      SET_CONFIG_ATTRIB(config, EGL_NATIVE_VISUAL_ID, visid);
      SET_CONFIG_ATTRIB(config, EGL_NATIVE_VISUAL_TYPE, vistype);
      SET_CONFIG_ATTRIB(config, EGL_NATIVE_RENDERABLE, EGL_FALSE);
      SET_CONFIG_ATTRIB(config, EGL_CONFORMANT, all_apis);
      SET_CONFIG_ATTRIB(config, EGL_RENDERABLE_TYPE, all_apis);
      SET_CONFIG_ATTRIB(config, EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT);
      SET_CONFIG_ATTRIB(config, EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE);
      SET_CONFIG_ATTRIB(config, EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE);

      _eglAddConfig(disp, config);
   }

   XFree(visInfo);
}


/**
 * Called via eglInitialize(), drv->API.Initialize().
 */
static EGLBoolean
xlib_eglInitialize(_EGLDriver *drv, _EGLDisplay *dpy,
                   EGLint *major, EGLint *minor)
{
   struct xlib_egl_driver *xdrv = xlib_egl_driver(drv);
   struct xlib_egl_display *xdpy;

   xdpy = CALLOC_STRUCT(xlib_egl_display);
   if (!xdpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   xdpy->Dpy = (Display *) dpy->NativeDisplay;
   if (!xdpy->Dpy) {
      xdpy->Dpy = XOpenDisplay(NULL);
      if (!xdpy->Dpy) {
         free(xdpy);
         return EGL_FALSE;
      }
   }

   /* create winsys and pipe screen */
   xdpy->winsys = create_sw_winsys();
   if (!xdpy->winsys) {
      free(xdpy);
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");
   }
   xdpy->winsys->flush_frontbuffer = flush_frontbuffer;
   xdpy->screen = softpipe_create_screen(xdpy->winsys);
   if (!xdpy->screen) {
      free(xdpy->winsys);
      free(xdpy);
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");
   }

   dpy->DriverData = (void *) xdpy;
   dpy->ClientAPIsMask = xdrv->apis;

   create_configs(xdpy, dpy);

   /* we're supporting EGL 1.4 */
   *major = 1;
   *minor = 4;

   return EGL_TRUE;
}


/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
xlib_eglTerminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct xlib_egl_display *xdpy = xlib_egl_display(dpy);

   _eglReleaseDisplayResources(drv, dpy);
   _eglCleanupDisplay(dpy);

   xdpy->screen->destroy(xdpy->screen);
   free(xdpy->winsys);

   if (!dpy->NativeDisplay)
      XCloseDisplay(xdpy->Dpy);
   free(xdpy);

   return EGL_TRUE;
}


static _EGLProc
xlib_eglGetProcAddress(const char *procname)
{
   return (_EGLProc) st_get_proc_address(procname);
}


static void
get_drawable_visual_info(Display *dpy, Drawable d, XVisualInfo *visInfo)
{
   XWindowAttributes attr;
   XVisualInfo visTemp, *vis;
   int num_visuals;

   XGetWindowAttributes(dpy, d, &attr);

   visTemp.screen = DefaultScreen(dpy);
   visTemp.visualid = attr.visual->visualid;
   vis = XGetVisualInfo(dpy,
                        (VisualScreenMask | VisualIDMask),
                        &visTemp, &num_visuals);
   if (vis)
      *visInfo = *vis;

   XFree(vis);
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


static void
check_and_update_buffer_size(struct xlib_egl_surface *surface)
{
   uint width, height;
   if (surface->Base.Type == EGL_PBUFFER_BIT) {
      width = surface->Base.Width;
      height = surface->Base.Height;
   }
   else {
      get_drawable_size(surface->Dpy, surface->Win, &width, &height);
   }
   st_resize_framebuffer(surface->Framebuffer, width, height);
   surface->Base.Width = width;
   surface->Base.Height = height;
}



static void
display_surface(struct pipe_winsys *pws,
                struct pipe_surface *psurf,
                struct xlib_egl_surface *xsurf)
{
   struct softpipe_texture *spt = softpipe_texture(psurf->texture);
   XImage *ximage;
   void *data;

   if (xsurf->Base.Type == EGL_PBUFFER_BIT)
      return;

   ximage = XCreateImage(xsurf->Dpy,
                         xsurf->VisInfo.visual,
                         xsurf->VisInfo.depth,
                         ZPixmap, 0,   /* format, offset */
                         NULL,         /* data */
                         0, 0,         /* size */
                         32,           /* bitmap_pad */
                         0);           /* bytes_per_line */


   assert(ximage->format);
   assert(ximage->bitmap_unit);

   data = pws->buffer_map(pws, spt->buffer, 0);

   /* update XImage's fields */
   ximage->data = data;
   ximage->width = psurf->width;
   ximage->height = psurf->height;
   ximage->bytes_per_line = spt->stride[psurf->level];
   
   XPutImage(xsurf->Dpy, xsurf->Win, xsurf->Gc,
             ximage, 0, 0, 0, 0, psurf->width, psurf->height);

   XSync(xsurf->Dpy, 0);

   ximage->data = NULL;
   XDestroyImage(ximage);

   pws->buffer_unmap(pws, spt->buffer);
}



/** Display gallium surface in X window */
static void
flush_frontbuffer(struct pipe_winsys *pws,
                  struct pipe_surface *psurf,
                  void *context_private)
{
   struct xlib_egl_surface *xsurf = (struct xlib_egl_surface *) context_private;
   display_surface(pws, psurf, xsurf);
}



/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static _EGLContext *
xlib_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                      _EGLContext *share_list, const EGLint *attrib_list)
{
   struct xlib_egl_display *xdpy = xlib_egl_display(dpy);
   struct xlib_egl_context *ctx;
   struct st_context *share_ctx = NULL; /* XXX fix */
   __GLcontextModes visual;

   ctx = CALLOC_STRUCT(xlib_egl_context);
   if (!ctx)
      return NULL;

   /* let EGL lib init the common stuff */
   if (!_eglInitContext(drv, &ctx->Base, conf, attrib_list)) {
      free(ctx);
      return NULL;
   }

   /* API-dependent context creation */
   switch (ctx->Base.ClientAPI) {
   case EGL_OPENVG_API:
   case EGL_OPENGL_ES_API:
      _eglLog(_EGL_DEBUG, "Create Context for ES version %d\n",
              ctx->Base.ClientVersion);
      /* fall-through */
   case EGL_OPENGL_API:
      /* create a softpipe context */
      ctx->pipe = softpipe_create(xdpy->screen);
      /* Now do xlib / state tracker inits here */
      _eglConfigToContextModesRec(conf, &visual);
      ctx->Context = st_create_context(ctx->pipe, &visual, share_ctx);
      break;
   default:
      _eglError(EGL_BAD_MATCH, "eglCreateContext(unsupported API)");
      free(ctx);
      return NULL;
   }

   return &ctx->Base;
}


static EGLBoolean
xlib_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   struct xlib_egl_context *context = lookup_context(ctx);

   if (!_eglIsContextBound(&context->Base)) {
      /* API-dependent clean-up */
      switch (context->Base.ClientAPI) {
      case EGL_OPENGL_ES_API:
      case EGL_OPENVG_API:
         /* fall-through */
      case EGL_OPENGL_API:
         st_destroy_context(context->Context);
         break;
      default:
         assert(0);
      }
      free(context);
   }
   return EGL_TRUE;
}


/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
xlib_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy,
                    _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx)
{
   struct xlib_egl_context *context = lookup_context(ctx);
   struct xlib_egl_surface *draw_surf = lookup_surface(draw);
   struct xlib_egl_surface *read_surf = lookup_surface(read);
   struct st_context *oldcontext = NULL;
   _EGLContext *oldctx;

   oldctx = _eglGetCurrentContext();
   if (oldctx && _eglIsContextLinked(oldctx))
      oldcontext = st_get_current();

   if (!_eglMakeCurrent(drv, dpy, draw, read, ctx))
      return EGL_FALSE;

   /* Flush before switching context.  Check client API? */
   if (oldcontext)
      st_flush(oldcontext, PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME, NULL);
   st_make_current((context ? context->Context : NULL),
                   (draw_surf ? draw_surf->Framebuffer : NULL),
                   (read_surf ? read_surf->Framebuffer : NULL));

   if (draw_surf)
      check_and_update_buffer_size(draw_surf);
   if (read_surf && read_surf != draw_surf)
      check_and_update_buffer_size(draw_surf);

   return EGL_TRUE;
}


static enum pipe_format
choose_color_format(const __GLcontextModes *visual)
{
   if (visual->redBits == 8 &&
       visual->greenBits == 8 &&
       visual->blueBits == 8 &&
       visual->alphaBits == 8) {
      /* XXX this really also depends on the ordering of R,G,B,A */
      return PIPE_FORMAT_A8R8G8B8_UNORM;
   }
   else {
      assert(0);
      return PIPE_FORMAT_NONE;
   }
}


static enum pipe_format
choose_depth_format(const __GLcontextModes *visual)
{
   if (visual->depthBits > 0)
      return PIPE_FORMAT_S8Z24_UNORM;
   else
      return PIPE_FORMAT_NONE;
}


static enum pipe_format
choose_stencil_format(const __GLcontextModes *visual)
{
   if (visual->stencilBits > 0)
      return PIPE_FORMAT_S8Z24_UNORM;
   else
      return PIPE_FORMAT_NONE;
}


/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
xlib_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
                            NativeWindowType window, const EGLint *attrib_list)
{
   struct xlib_egl_display *xdpy = xlib_egl_display(disp);
   struct xlib_egl_surface *surf;
   __GLcontextModes visual;
   uint width, height;

   surf = CALLOC_STRUCT(xlib_egl_surface);
   if (!surf)
      return NULL;

   /* Let EGL lib init the common stuff */
   if (!_eglInitSurface(drv, &surf->Base, EGL_WINDOW_BIT,
                        conf, attrib_list)) {
      free(surf);
      return NULL;
   }

   /*
    * Now init the Xlib and gallium stuff
    */
   surf->Win = (Window) window;  /* The X window ID */
   surf->Dpy = xdpy->Dpy;  /* The X display */
   surf->Gc = XCreateGC(surf->Dpy, surf->Win, 0, NULL);

   surf->winsys = xdpy->winsys;

   _eglConfigToContextModesRec(conf, &visual);
   get_drawable_size(surf->Dpy, surf->Win, &width, &height);
   get_drawable_visual_info(surf->Dpy, surf->Win, &surf->VisInfo);

   surf->Base.Width = width;
   surf->Base.Height = height;

   /* Create GL statetracker framebuffer */
   surf->Framebuffer = st_create_framebuffer(&visual,
                                             choose_color_format(&visual),
                                             choose_depth_format(&visual),
                                             choose_stencil_format(&visual),
                                             width, height,
                                             (void *) surf);

   st_resize_framebuffer(surf->Framebuffer, width, height);

   return &surf->Base;
}


static _EGLSurface *
xlib_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
                             const EGLint *attrib_list)
{
   struct xlib_egl_display *xdpy = xlib_egl_display(disp);
   struct xlib_egl_surface *surf;
   __GLcontextModes visual;
   uint width, height;
   EGLBoolean bind_texture;

   surf = CALLOC_STRUCT(xlib_egl_surface);
   if (!surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePbufferSurface");
      return NULL;
   }

   if (!_eglInitSurface(drv, &surf->Base, EGL_PBUFFER_BIT,
                        conf, attrib_list)) {
      free(surf);
      return NULL;
   }
   if (surf->Base.Width < 0 || surf->Base.Height < 0) {
      _eglError(EGL_BAD_PARAMETER, "eglCreatePbufferSurface");
      free(surf);
      return NULL;
   }

   bind_texture = (surf->Base.TextureFormat != EGL_NO_TEXTURE);
   width = (uint) surf->Base.Width;
   height = (uint) surf->Base.Height;
   if ((surf->Base.TextureTarget == EGL_NO_TEXTURE && bind_texture) ||
       (surf->Base.TextureTarget != EGL_NO_TEXTURE && !bind_texture)) {
      _eglError(EGL_BAD_MATCH, "eglCreatePbufferSurface");
      free(surf);
      return NULL;
   }
   /* a framebuffer of zero width or height confuses st */
   if (width == 0 || height == 0) {
      _eglError(EGL_BAD_MATCH, "eglCreatePbufferSurface");
      free(surf);
      return NULL;
   }
   /* no mipmap generation */
   if (surf->Base.MipmapTexture) {
      _eglError(EGL_BAD_MATCH, "eglCreatePbufferSurface");
      free(surf);
      return NULL;
   }

   surf->winsys = xdpy->winsys;

   _eglConfigToContextModesRec(conf, &visual);

   /* Create GL statetracker framebuffer */
   surf->Framebuffer = st_create_framebuffer(&visual,
                                             choose_color_format(&visual),
                                             choose_depth_format(&visual),
                                             choose_stencil_format(&visual),
                                             width, height,
                                             (void *) surf);
   st_resize_framebuffer(surf->Framebuffer, width, height);

   return &surf->Base;
}


static EGLBoolean
xlib_eglDestroySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface)
{
   struct xlib_egl_surface *surf = lookup_surface(surface);
   if (!_eglIsSurfaceBound(&surf->Base)) {
      if (surf->Base.Type != EGL_PBUFFER_BIT)
         XFreeGC(surf->Dpy, surf->Gc);
      st_unreference_framebuffer(surf->Framebuffer);
      free(surf);
   }
   return EGL_TRUE;
}


static EGLBoolean
xlib_eglBindTexImage(_EGLDriver *drv, _EGLDisplay *dpy,
                     _EGLSurface *surface, EGLint buffer)
{
   struct xlib_egl_surface *xsurf = lookup_surface(surface);
   struct xlib_egl_context *xctx;
   struct pipe_surface *psurf;
   enum pipe_format format;
   int target;

   if (!xsurf || xsurf->Base.Type != EGL_PBUFFER_BIT)
      return _eglError(EGL_BAD_SURFACE, "eglBindTexImage");
   if (buffer != EGL_BACK_BUFFER)
      return _eglError(EGL_BAD_PARAMETER, "eglBindTexImage");
   if (xsurf->Base.BoundToTexture)
      return _eglError(EGL_BAD_ACCESS, "eglBindTexImage");

   /* this should be updated when choose_color_format is */
   switch (xsurf->Base.TextureFormat) {
   case EGL_TEXTURE_RGB:
      format = PIPE_FORMAT_R8G8B8_UNORM;
      break;
   case EGL_TEXTURE_RGBA:
      format = PIPE_FORMAT_A8R8G8B8_UNORM;
      break;
   default:
      return _eglError(EGL_BAD_MATCH, "eglBindTexImage");
   }

   switch (xsurf->Base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = ST_TEXTURE_2D;
      break;
   default:
      return _eglError(EGL_BAD_MATCH, "eglBindTexImage");
   }

   /* flush properly */
   if (eglGetCurrentSurface(EGL_DRAW) == surface) {
      xctx = lookup_context(_eglGetCurrentContext());
      st_flush(xctx->Context, PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_FRAME,
               NULL);
   }
   else if (_eglIsSurfaceBound(&xsurf->Base)) {
      xctx = lookup_context(xsurf->Base.Binding);
      if (xctx)
         st_finish(xctx->Context);
   }

   st_get_framebuffer_surface(xsurf->Framebuffer, ST_SURFACE_BACK_LEFT,
                              &psurf);
   st_bind_texture_surface(psurf, target, xsurf->Base.MipmapLevel, format);
   xsurf->Base.BoundToTexture = EGL_TRUE;

   return EGL_TRUE;
}


static EGLBoolean
xlib_eglReleaseTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface,
                        EGLint buffer)
{
   struct xlib_egl_surface *xsurf = lookup_surface(surface);
   struct pipe_surface *psurf;

   if (!xsurf || xsurf->Base.Type != EGL_PBUFFER_BIT ||
       !xsurf->Base.BoundToTexture)
      return _eglError(EGL_BAD_SURFACE, "eglReleaseTexImage");
   if (buffer != EGL_BACK_BUFFER)
      return _eglError(EGL_BAD_PARAMETER, "eglReleaseTexImage");

   st_get_framebuffer_surface(xsurf->Framebuffer, ST_SURFACE_BACK_LEFT,
                              &psurf);
   st_unbind_texture_surface(psurf, ST_TEXTURE_2D, xsurf->Base.MipmapLevel);
   xsurf->Base.BoundToTexture = EGL_FALSE;

   return EGL_TRUE;
}


static EGLBoolean
xlib_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw)
{
   /* error checking step: */
   if (!_eglSwapBuffers(drv, dpy, draw))
      return EGL_FALSE;

   {
      struct xlib_egl_surface *xsurf = lookup_surface(draw);
      struct pipe_winsys *pws = xsurf->winsys;
      struct pipe_surface *psurf;

      st_get_framebuffer_surface(xsurf->Framebuffer, ST_SURFACE_BACK_LEFT,
                                 &psurf);

      st_notify_swapbuffers(xsurf->Framebuffer);

      display_surface(pws, psurf, xsurf);

      check_and_update_buffer_size(xsurf);
   }

   return EGL_TRUE;
}


/**
 * Determine which API(s) is(are) present by looking for some specific
 * global symbols.
 */
static EGLint
find_supported_apis(void)
{
   EGLint mask = 0;
   void *handle;

   handle = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL);
   if(!handle)
      return mask;

   if (dlsym(handle, "st_api_OpenGL_ES1"))
      mask |= EGL_OPENGL_ES_BIT;

   if (dlsym(handle, "st_api_OpenGL_ES2"))
      mask |= EGL_OPENGL_ES2_BIT;

   if (dlsym(handle, "st_api_OpenGL"))
      mask |= EGL_OPENGL_BIT;

   if (dlsym(handle, "st_api_OpenVG"))
      mask |= EGL_OPENVG_BIT;

   dlclose(handle);

   return mask;
}


static void
xlib_Unload(_EGLDriver *drv)
{
   struct xlib_egl_driver *xdrv = xlib_egl_driver(drv);
   free(xdrv);
}


/**
 * This is the main entrypoint into the driver.
 * Called by libEGL to instantiate an _EGLDriver object.
 */
_EGLDriver *
_eglMain(const char *args)
{
   struct xlib_egl_driver *xdrv;

   _eglLog(_EGL_INFO, "Entering EGL/Xlib _eglMain(%s)", args);

   xdrv = CALLOC_STRUCT(xlib_egl_driver);
   if (!xdrv)
      return NULL;

   _eglInitDriverFallbacks(&xdrv->Base);
   xdrv->Base.API.Initialize = xlib_eglInitialize;
   xdrv->Base.API.Terminate = xlib_eglTerminate;
   xdrv->Base.API.GetProcAddress = xlib_eglGetProcAddress;
   xdrv->Base.API.CreateContext = xlib_eglCreateContext;
   xdrv->Base.API.DestroyContext = xlib_eglDestroyContext;
   xdrv->Base.API.CreateWindowSurface = xlib_eglCreateWindowSurface;
   xdrv->Base.API.CreatePbufferSurface = xlib_eglCreatePbufferSurface;
   xdrv->Base.API.DestroySurface = xlib_eglDestroySurface;
   xdrv->Base.API.BindTexImage = xlib_eglBindTexImage;
   xdrv->Base.API.ReleaseTexImage = xlib_eglReleaseTexImage;
   xdrv->Base.API.MakeCurrent = xlib_eglMakeCurrent;
   xdrv->Base.API.SwapBuffers = xlib_eglSwapBuffers;

   xdrv->apis = find_supported_apis();
   if (xdrv->apis == 0x0) {
      /* the app isn't directly linked with any EGL-supprted APIs
       * (such as libGLESv2.so) so use an EGL utility to see what
       * APIs might be loaded dynamically on this system.
       */
      xdrv->apis = _eglFindAPIs();
   }

   xdrv->Base.Name = "Xlib/softpipe";
   xdrv->Base.Unload = xlib_Unload;

   return &xdrv->Base;
}
