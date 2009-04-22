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

   Display *Dpy;  /**< The X Display of the window */
   Window Win;    /**< The user-created window ID */
   GC Gc;
   XVisualInfo VisInfo;

   struct pipe_winsys *winsys;

   struct st_framebuffer *Framebuffer;
};


/** cast wrapper */
static INLINE struct xlib_egl_driver *
xlib_egl_driver(_EGLDriver *drv)
{
   return (struct xlib_egl_driver *) drv;
}


static struct xlib_egl_surface *
lookup_surface(EGLSurface surf)
{
   _EGLSurface *surface = _eglLookupSurface(surf);
   return (struct xlib_egl_surface *) surface;
}


static struct xlib_egl_context *
lookup_context(EGLContext surf)
{
   _EGLContext *context = _eglLookupContext(surf);
   return (struct xlib_egl_context *) context;
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
create_configs(_EGLDriver *drv, EGLDisplay dpy)
{
   static const EGLint all_apis = (EGL_OPENGL_ES_BIT |
                                   EGL_OPENGL_ES2_BIT |
                                   EGL_OPENVG_BIT |
                                   EGL_OPENGL_BIT);
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   XVisualInfo *visInfo, visTemplate;
   int num_visuals, i;

   /* get list of all X visuals, create an EGL config for each */
   visTemplate.screen = DefaultScreen(disp->Xdpy);
   visInfo = XGetVisualInfo(disp->Xdpy, VisualScreenMask,
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
      SET_CONFIG_ATTRIB(config, EGL_SURFACE_TYPE, EGL_WINDOW_BIT);

      _eglAddConfig(disp, config);
   }
}


/**
 * Called via eglInitialize(), drv->API.Initialize().
 */
static EGLBoolean
xlib_eglInitialize(_EGLDriver *drv, EGLDisplay dpy,
                   EGLint *minor, EGLint *major)
{
   create_configs(drv, dpy);

   drv->Initialized = EGL_TRUE;

   /* we're supporting EGL 1.4 */
   *minor = 1;
   *major = 4;

   return EGL_TRUE;
}


/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
xlib_eglTerminate(_EGLDriver *drv, EGLDisplay dpy)
{
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
   get_drawable_size(surface->Dpy, surface->Win, &width, &height);
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
static EGLContext
xlib_eglCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                      EGLContext share_list, const EGLint *attrib_list)
{
   struct xlib_egl_driver *xdrv = xlib_egl_driver(drv);
   _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);
   struct xlib_egl_context *ctx;
   struct st_context *share_ctx = NULL; /* XXX fix */
   __GLcontextModes visual;

   ctx = CALLOC_STRUCT(xlib_egl_context);
   if (!ctx)
      return EGL_NO_CONTEXT;

   /* let EGL lib init the common stuff */
   if (!_eglInitContext(drv, dpy, &ctx->Base, config, attrib_list)) {
      free(ctx);
      return EGL_NO_CONTEXT;
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
      ctx->pipe = softpipe_create(xdrv->screen);
      /* Now do xlib / state tracker inits here */
      _eglConfigToContextModesRec(conf, &visual);
      ctx->Context = st_create_context(ctx->pipe, &visual, share_ctx);
      break;
   default:
      _eglError(EGL_BAD_MATCH, "eglCreateContext(unsupported API)");
      free(ctx);
      return EGL_NO_CONTEXT;
   }

   _eglSaveContext(&ctx->Base);

   return _eglGetContextHandle(&ctx->Base);
}


static EGLBoolean
xlib_eglDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx)
{
   struct xlib_egl_context *context = lookup_context(ctx);
   if (context) {
      if (context->Base.IsBound) {
         context->Base.DeletePending = EGL_TRUE;
      }
      else {
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
   else {
      _eglError(EGL_BAD_CONTEXT, "eglDestroyContext");
      return EGL_TRUE;
   }
}


/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
xlib_eglMakeCurrent(_EGLDriver *drv, EGLDisplay dpy,
                    EGLSurface draw, EGLSurface read, EGLContext ctx)
{
   struct xlib_egl_context *context = lookup_context(ctx);
   struct xlib_egl_surface *draw_surf = lookup_surface(draw);
   struct xlib_egl_surface *read_surf = lookup_surface(read);

   if (!_eglMakeCurrent(drv, dpy, draw, read, context))
      return EGL_FALSE;

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
static EGLSurface
xlib_eglCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                            NativeWindowType window, const EGLint *attrib_list)
{
   struct xlib_egl_driver *xdrv = xlib_egl_driver(drv);
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);

   struct xlib_egl_surface *surf;
   __GLcontextModes visual;
   uint width, height;

   surf = CALLOC_STRUCT(xlib_egl_surface);
   if (!surf)
      return EGL_NO_SURFACE;

   /* Let EGL lib init the common stuff */
   if (!_eglInitSurface(drv, dpy, &surf->Base, EGL_WINDOW_BIT,
                        config, attrib_list)) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   _eglSaveSurface(&surf->Base);

   /*
    * Now init the Xlib and gallium stuff
    */
   surf->Win = (Window) window;  /* The X window ID */
   surf->Dpy = disp->Xdpy;  /* The X display */
   surf->Gc = XCreateGC(surf->Dpy, surf->Win, 0, NULL);

   surf->winsys = xdrv->winsys;

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

   return _eglGetSurfaceHandle(&surf->Base);
}


static EGLBoolean
xlib_eglDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   struct xlib_egl_surface *surf = lookup_surface(surface);
   if (surf) {
      _eglHashRemove(_eglGlobal.Surfaces, (EGLuint) surface);
      if (surf->Base.IsBound) {
         surf->Base.DeletePending = EGL_TRUE;
      }
      else {
         XFreeGC(surf->Dpy, surf->Gc);
         st_unreference_framebuffer(surf->Framebuffer);
         free(surf);
      }
      return EGL_TRUE;
   }
   else {
      _eglError(EGL_BAD_SURFACE, "eglDestroySurface");
      return EGL_FALSE;
   }
}


static EGLBoolean
xlib_eglSwapBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
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

   handle = dlopen(NULL, 0);

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


/**
 * This is the main entrypoint into the driver.
 * Called by libEGL to instantiate an _EGLDriver object.
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy, const char *args)
{
   struct xlib_egl_driver *xdrv;

   _eglLog(_EGL_INFO, "Entering EGL/Xlib _eglMain(%s)", args);

   xdrv = CALLOC_STRUCT(xlib_egl_driver);
   if (!xdrv)
      return NULL;

   if (!dpy->Xdpy) {
      dpy->Xdpy = XOpenDisplay(NULL);
   }

   _eglInitDriverFallbacks(&xdrv->Base);
   xdrv->Base.API.Initialize = xlib_eglInitialize;
   xdrv->Base.API.Terminate = xlib_eglTerminate;
   xdrv->Base.API.GetProcAddress = xlib_eglGetProcAddress;
   xdrv->Base.API.CreateContext = xlib_eglCreateContext;
   xdrv->Base.API.DestroyContext = xlib_eglDestroyContext;
   xdrv->Base.API.CreateWindowSurface = xlib_eglCreateWindowSurface;
   xdrv->Base.API.DestroySurface = xlib_eglDestroySurface;
   xdrv->Base.API.MakeCurrent = xlib_eglMakeCurrent;
   xdrv->Base.API.SwapBuffers = xlib_eglSwapBuffers;

   xdrv->Base.ClientAPIsMask = find_supported_apis();
   if (xdrv->Base.ClientAPIsMask == 0x0) {
      /* the app isn't directly linked with any EGL-supprted APIs
       * (such as libGLESv2.so) so use an EGL utility to see what
       * APIs might be loaded dynamically on this system.
       */
      xdrv->Base.ClientAPIsMask = _eglFindAPIs();
   }      

   xdrv->Base.Name = "Xlib/softpipe";

   /* create one winsys and use it for all contexts/surfaces */
   xdrv->winsys = create_sw_winsys();
   xdrv->winsys->flush_frontbuffer = flush_frontbuffer;

   xdrv->screen = softpipe_create_screen(xdrv->winsys);

   return &xdrv->Base;
}

