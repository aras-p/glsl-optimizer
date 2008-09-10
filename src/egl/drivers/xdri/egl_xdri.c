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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dlfcn.h"
#include <X11/Xlib.h>
#include <GL/gl.h>
#include "xf86dri.h"
#include "glxclient.h"
#include "dri_util.h"
#include "drm_sarea.h"

#define _EGL_PLATFORM_X

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglhash.h"
#include "egllog.h"
#include "eglsurface.h"

#include <GL/gl.h>

typedef void (*glGetIntegerv_t)(GLenum, GLint *);
typedef void (*glBindTexture_t)(GLenum, GLuint);
typedef void (*glCopyTexImage2D_t)(GLenum, GLint, GLenum, GLint, GLint,
                                   GLint, GLint, GLint);


#define CALLOC_STRUCT(T)   (struct T *) calloc(1, sizeof(struct T))


/** subclass of _EGLDriver */
struct xdri_egl_driver
{
   _EGLDriver Base;   /**< base class */

   const char *dri_driver_name;  /**< name of DRI driver to load */
   void *dri_driver_handle;      /**< returned by dlopen(dri_driver_name) */

   __GLXdisplayPrivate *glx_priv;


   /* XXX we're not actually using these at this time: */
   int chipset;
   int minor;
   int drmFD;

   __DRIframebuffer framebuffer;
   drm_handle_t hSAREA;
   drmAddress pSAREA;
   char *busID;
   drm_magic_t magic;
};


/** subclass of _EGLContext */
struct xdri_egl_context
{
   _EGLContext Base;   /**< base class */

   __DRIcontext driContext;

   GLint bound_tex_object;
};


/** subclass of _EGLSurface */
struct xdri_egl_surface
{
   _EGLSurface Base;   /**< base class */

   __DRIid driDrawable;  /**< DRI surface */
   drm_drawable_t hDrawable;
};


/** subclass of _EGLConfig */
struct xdri_egl_config
{
   _EGLConfig Base;   /**< base class */

   const __GLcontextModes *mode;  /**< corresponding GLX mode */
};



/** cast wrapper */
static struct xdri_egl_driver *
xdri_egl_driver(_EGLDriver *drv)
{
   return (struct xdri_egl_driver *) drv;
}


/** Map EGLSurface handle to xdri_egl_surface object */
static struct xdri_egl_surface *
lookup_surface(EGLSurface surf)
{
   _EGLSurface *surface = _eglLookupSurface(surf);
   return (struct xdri_egl_surface *) surface;
}


/** Map EGLContext handle to xdri_egl_context object */
static struct xdri_egl_context *
lookup_context(EGLContext c)
{
   _EGLContext *context = _eglLookupContext(c);
   return (struct xdri_egl_context *) context;
}

static struct xdri_egl_context *
current_context(void)
{
   return (struct xdri_egl_context *) _eglGetCurrentContext();
}

/** Map EGLConfig handle to xdri_egl_config object */
static struct xdri_egl_config *
lookup_config(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config)
{
   _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);
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


/**
 * Produce a set of EGL configs.
 * Note that we get the list of GLcontextModes from the GLX library.
 * This dependency on GLX lib will be removed someday.
 */
static void
create_configs(_EGLDisplay *disp, __GLXdisplayPrivate *glx_priv)
{
   static const EGLint all_apis = (EGL_OPENGL_ES_BIT |
                                   EGL_OPENGL_ES2_BIT |
                                   EGL_OPENVG_BIT |
                                   EGL_OPENGL_BIT);
   __GLXscreenConfigs *scrn = glx_priv->screenConfigs;
   const __GLcontextModes *m;
   int id = 1;

   for (m = scrn->configs; m; m = m->next) {
      /* EGL requires double-buffered configs */
      if (m->doubleBufferMode) {
         struct xdri_egl_config *config = CALLOC_STRUCT(xdri_egl_config);

         _eglInitConfig(&config->Base, id++);

         SET_CONFIG_ATTRIB(&config->Base, EGL_BUFFER_SIZE, m->rgbBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_RED_SIZE, m->redBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_GREEN_SIZE, m->greenBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_BLUE_SIZE, m->blueBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_ALPHA_SIZE, m->alphaBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_DEPTH_SIZE, m->depthBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_STENCIL_SIZE, m->stencilBits);
         SET_CONFIG_ATTRIB(&config->Base, EGL_SAMPLES, m->samples);
         SET_CONFIG_ATTRIB(&config->Base, EGL_SAMPLE_BUFFERS, m->sampleBuffers);
         SET_CONFIG_ATTRIB(&config->Base, EGL_NATIVE_VISUAL_ID, m->visualID);
         SET_CONFIG_ATTRIB(&config->Base, EGL_NATIVE_VISUAL_TYPE, m->visualType);
         SET_CONFIG_ATTRIB(&config->Base, EGL_CONFORMANT, all_apis);
         SET_CONFIG_ATTRIB(&config->Base, EGL_RENDERABLE_TYPE, all_apis);
         /* XXX only window rendering allowed ATM */
         SET_CONFIG_ATTRIB(&config->Base, EGL_SURFACE_TYPE,
                           (EGL_WINDOW_BIT | EGL_PBUFFER_BIT));

         /* XXX possibly other things to init... */

         /* Ptr from EGL config to GLcontextMode.  Used in CreateContext(). */
         config->mode = m;

         _eglAddConfig(disp, &config->Base);
      }
   }
}


/**
 * Called via __DRIinterfaceMethods object
 */
static __DRIfuncPtr
dri_get_proc_address(const char * proc_name)
{
   return NULL;
}


static void
dri_context_modes_destroy(__GLcontextModes *modes)
{
   _eglLog(_EGL_DEBUG, "%s", __FUNCTION__);

   while (modes) {
      __GLcontextModes * const next = modes->next;
      free(modes);
      modes = next;
   }
}


/**
 * Create a linked list of 'count' GLcontextModes.
 * These are used during the client/server visual negotiation phase,
 * then discarded.
 */
static __GLcontextModes *
dri_context_modes_create(unsigned count, size_t minimum_size)
{
   /* This code copied from libGLX, and modified */
   const size_t size = (minimum_size > sizeof(__GLcontextModes))
       ? minimum_size : sizeof(__GLcontextModes);
   __GLcontextModes * head = NULL;
   __GLcontextModes ** next;
   unsigned   i;

   next = & head;
   for (i = 0 ; i < count ; i++) {
      *next = (__GLcontextModes *) calloc(1, size);
      if (*next == NULL) {
	 dri_context_modes_destroy(head);
	 head = NULL;
	 break;
      }
      
      (*next)->doubleBufferMode = 1;
      (*next)->visualID = GLX_DONT_CARE;
      (*next)->visualType = GLX_DONT_CARE;
      (*next)->visualRating = GLX_NONE;
      (*next)->transparentPixel = GLX_NONE;
      (*next)->transparentRed = GLX_DONT_CARE;
      (*next)->transparentGreen = GLX_DONT_CARE;
      (*next)->transparentBlue = GLX_DONT_CARE;
      (*next)->transparentAlpha = GLX_DONT_CARE;
      (*next)->transparentIndex = GLX_DONT_CARE;
      (*next)->xRenderable = GLX_DONT_CARE;
      (*next)->fbconfigID = GLX_DONT_CARE;
      (*next)->swapMethod = GLX_SWAP_UNDEFINED_OML;
      (*next)->bindToTextureRgb = GLX_DONT_CARE;
      (*next)->bindToTextureRgba = GLX_DONT_CARE;
      (*next)->bindToMipmapTexture = GLX_DONT_CARE;
      (*next)->bindToTextureTargets = 0;
      (*next)->yInverted = GLX_DONT_CARE;

      next = & ((*next)->next);
   }

   return head;
}


static __DRIscreen *
dri_find_dri_screen(__DRInativeDisplay *ndpy, int scrn)
{
   __GLXdisplayPrivate *priv = __glXInitialize(ndpy);
   __GLXscreenConfigs *scrnConf = priv->screenConfigs;
   return &scrnConf->driScreen;
}


static GLboolean
dri_window_exists(__DRInativeDisplay *ndpy, __DRIid draw)
{
   return EGL_TRUE;
}


static GLboolean
dri_create_context(__DRInativeDisplay *ndpy, int screenNum, int configID,
                   void * contextID, drm_context_t * hw_context)
{
   assert(configID >= 0);
   return XF86DRICreateContextWithConfig(ndpy, screenNum,
                                         configID, contextID, hw_context);
}


static GLboolean
dri_destroy_context(__DRInativeDisplay * ndpy, int screen, __DRIid context)
{
   return XF86DRIDestroyContext(ndpy, screen, context);
}


static GLboolean
dri_create_drawable(__DRInativeDisplay * ndpy, int screen,
                    __DRIid drawable, drm_drawable_t * hHWDrawable)
{
   _eglLog(_EGL_DEBUG, "XDRI: %s", __FUNCTION__);

   /* Create DRI drawable for given window ID (drawable) */
   if (!XF86DRICreateDrawable(ndpy, screen, drawable, hHWDrawable))
      return EGL_FALSE;

   return EGL_TRUE;
}


static GLboolean
dri_destroy_drawable(__DRInativeDisplay * ndpy, int screen, __DRIid drawable)
{
   _eglLog(_EGL_DEBUG, "XDRI: %s", __FUNCTION__);
   return XF86DRIDestroyDrawable(ndpy, screen, drawable);
}


static GLboolean
dri_get_drawable_info(__DRInativeDisplay *ndpy, int scrn,
                      __DRIid draw, unsigned int * index, unsigned int * stamp,
                      int * x, int * y, int * width, int * height,
                      int * numClipRects, drm_clip_rect_t ** pClipRects,
                      int * backX, int * backY,
                      int * numBackClipRects,
                      drm_clip_rect_t ** pBackClipRects)
{
   _eglLog(_EGL_DEBUG, "XDRI: %s", __FUNCTION__);

   if (!XF86DRIGetDrawableInfo(ndpy, scrn, draw, index, stamp,
                               x, y, width, height,
                               numClipRects, pClipRects,
                               backX, backY,
                               numBackClipRects, pBackClipRects)) {
      return EGL_FALSE;
   }

   return EGL_TRUE;
}


/**
 * Table of functions exported by the loader to the driver.
 */
static const __DRIinterfaceMethods interface_methods = {
   dri_get_proc_address,

   dri_context_modes_create,
   dri_context_modes_destroy,

   dri_find_dri_screen,
   dri_window_exists,

   dri_create_context,
   dri_destroy_context,

   dri_create_drawable,
   dri_destroy_drawable,
   dri_get_drawable_info,

   NULL,/*__eglGetUST,*/
   NULL,/*__eglGetMSCRate,*/
};



static EGLBoolean
init_drm(struct xdri_egl_driver *xdri_drv, _EGLDisplay *disp)
{
   __DRIversion ddx_version;
   __DRIversion dri_version;
   __DRIversion drm_version;
   drmVersionPtr version;
   drm_handle_t  hFB;
   int newlyopened;
   int status;
   int scrn = DefaultScreen(disp->Xdpy);

#if 0
   createNewScreen = (PFNCREATENEWSCREENFUNC)
      dlsym(xdri_drv->dri_driver_handle, createNewScreenName);
   if (!createNewScreen) {
      _eglLog(_EGL_WARNING, "XDRI: Couldn't find %s function in the driver.",
              createNewScreenName);
      return EGL_FALSE;
   }
   else {
      _eglLog(_EGL_DEBUG, "XDRI: Found %s", createNewScreenName);
   }
#endif

   /*
    * Get the DRI X extension version.
    */
   dri_version.major = 4;
   dri_version.minor = 0;
   dri_version.patch = 0;

   if (!XF86DRIOpenConnection(disp->Xdpy, scrn,
                              &xdri_drv->hSAREA, &xdri_drv->busID)) {
      _eglLog(_EGL_WARNING, "XF86DRIOpenConnection failed");
   }

   xdri_drv->drmFD = drmOpenOnce(NULL, xdri_drv->busID, &newlyopened);
   if (xdri_drv->drmFD < 0) {
      perror("drmOpenOnce failed: ");
      return EGL_FALSE;
   }
   else {
      _eglLog(_EGL_DEBUG, "XDRI: drmOpenOnce returned %d", xdri_drv->drmFD);
   }


   if (drmGetMagic(xdri_drv->drmFD, &xdri_drv->magic)) {
      perror("drmGetMagic failed: ");
      return EGL_FALSE;
   }

   version = drmGetVersion(xdri_drv->drmFD);
   if (version) {
      drm_version.major = version->version_major;
      drm_version.minor = version->version_minor;
      drm_version.patch = version->version_patchlevel;
      drmFreeVersion(version);
      _eglLog(_EGL_DEBUG, "XDRI: Got DRM version %d.%d.%d",
              drm_version.major,
              drm_version.minor,
              drm_version.patch);
   }
   else {
      drm_version.major = -1;
      drm_version.minor = -1;
      drm_version.patch = -1;
      _eglLog(_EGL_WARNING, "XDRI: drmGetVersion() failed");
      return EGL_FALSE;
   }

   /* Authenticate w/ server.
    */
   if (!XF86DRIAuthConnection(disp->Xdpy, scrn, xdri_drv->magic)) {
      _eglLog(_EGL_WARNING, "XDRI: XF86DRIAuthConnection() failed");
      return EGL_FALSE;
   }
   else {
      _eglLog(_EGL_DEBUG, "XDRI: XF86DRIAuthConnection() success");
   }

   /* Get ddx version.
    */
   {
      char *driverName;

      /*
       * Get device name (like "tdfx") and the ddx version
       * numbers.  We'll check the version in each DRI driver's
       * "createNewScreen" function.
       */
      if (!XF86DRIGetClientDriverName(disp->Xdpy, scrn,
                                     &ddx_version.major,
                                     &ddx_version.minor,
                                     &ddx_version.patch,
                                     &driverName)) {
         _eglLog(_EGL_WARNING, "XDRI: XF86DRIGetClientDriverName failed");
         return EGL_FALSE;
      }
      else {
         _eglLog(_EGL_DEBUG, "XDRI: XF86DRIGetClientDriverName returned %s", driverName);
      }
   }

   /* Get framebuffer info.
    */
   {
      int junk;
      if (!XF86DRIGetDeviceInfo(disp->Xdpy, scrn,
                                &hFB,
                                &junk,
                                &xdri_drv->framebuffer.size,
                                &xdri_drv->framebuffer.stride,
                                &xdri_drv->framebuffer.dev_priv_size,
                                &xdri_drv->framebuffer.dev_priv)) {
         _eglLog(_EGL_WARNING, "XDRI: XF86DRIGetDeviceInfo() failed");
         return EGL_FALSE;
      }
      else {
         _eglLog(_EGL_DEBUG, "XDRI: XF86DRIGetDeviceInfo() success");
      }
      xdri_drv->framebuffer.width = DisplayWidth(disp->Xdpy, scrn);
      xdri_drv->framebuffer.height = DisplayHeight(disp->Xdpy, scrn);
   }

   /* Map the framebuffer region. (this may not be needed)
    */
   status = drmMap(xdri_drv->drmFD, hFB, xdri_drv->framebuffer.size, 
                   (drmAddressPtr) &xdri_drv->framebuffer.base);
   if (status != 0) {
      _eglLog(_EGL_WARNING, "XDRI: drmMap(framebuffer) failed");
      return EGL_FALSE;
   }
   else {
      _eglLog(_EGL_DEBUG, "XDRI: drmMap(framebuffer) success");
   }

   /* Map the SAREA region.
    */
   status = drmMap(xdri_drv->drmFD, xdri_drv->hSAREA, SAREA_MAX, &xdri_drv->pSAREA);
   if (status != 0) {
      _eglLog(_EGL_WARNING, "XDRI: drmMap(sarea) failed");
      return EGL_FALSE;
   }
   else {
      _eglLog(_EGL_DEBUG, "XDRI: drmMap(sarea) success");
   }

   return EGL_TRUE;
}


/**
 * Load the DRI driver named by "xdri_drv->dri_driver_name".
 * Basically, dlopen() the library to set "xdri_drv->dri_driver_handle".
 *
 * Later, we'll call dlsym(createNewScreenName) to get a pointer to
 * the driver's createNewScreen() function which is the bootstrap function.
 *
 * \return EGL_TRUE for success, EGL_FALSE for failure
 */
static EGLBoolean
load_dri_driver(struct xdri_egl_driver *xdri_drv)
{
   char filename[100];
   int flags = RTLD_NOW;

   /* try "egl_xxx_dri.so" first */
   snprintf(filename, sizeof(filename), "egl_%s.so", xdri_drv->dri_driver_name);
   _eglLog(_EGL_DEBUG, "XDRI: dlopen(%s)", filename);
   xdri_drv->dri_driver_handle = dlopen(filename, flags);
   if (xdri_drv->dri_driver_handle) {
      _eglLog(_EGL_DEBUG, "XDRI: dlopen(%s) OK", filename);
      return EGL_TRUE;
   }
   else {
      _eglLog(_EGL_DEBUG, "XDRI: dlopen(%s) fail (%s)", filename, dlerror());
   }

   /* try regular "xxx_dri.so" next */
   snprintf(filename, sizeof(filename), "%s.so", xdri_drv->dri_driver_name);
   _eglLog(_EGL_DEBUG, "XDRI: dlopen(%s)", filename);
   xdri_drv->dri_driver_handle = dlopen(filename, flags);
   if (xdri_drv->dri_driver_handle) {
      _eglLog(_EGL_DEBUG, "XDRI: dlopen(%s) OK", filename);
      return EGL_TRUE;
   }

   _eglLog(_EGL_WARNING, "XDRI Could not open %s (%s)", filename, dlerror());
   return EGL_FALSE;
}


/**
 * Called via eglInitialize(), xdri_drv->API.Initialize().
 */
static EGLBoolean
xdri_eglInitialize(_EGLDriver *drv, EGLDisplay dpy,
                   EGLint *minor, EGLint *major)
{
   struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   static char name[100];

   _eglLog(_EGL_DEBUG, "XDRI: eglInitialize");

   if (!disp->Xdpy) {
      disp->Xdpy = XOpenDisplay(NULL);
      if (!disp->Xdpy) {
         _eglLog(_EGL_WARNING, "XDRI: XOpenDisplay failed");
         return EGL_FALSE;
      }
   }

#if 0
   /* choose the DRI driver to load */
   xdri_drv->dri_driver_name = _eglChooseDRMDriver(0);
   if (!load_dri_driver(xdri_drv))
      return EGL_FALSE;
#else
   (void) load_dri_driver;
#endif

#if 0
   if (!init_drm(xdri_drv, disp))
      return EGL_FALSE;
#else
   (void) init_drm;
#endif

   /*
    * NOTE: this call to __glXInitialize() bootstraps the whole GLX/DRI
    * interface, loads the DRI driver, etc.
    * This replaces the load_dri_driver()  and init_drm() code above.
    */
   xdri_drv->glx_priv = __glXInitialize(disp->Xdpy);

   create_configs(disp, xdri_drv->glx_priv);

   xdri_drv->Base.Initialized = EGL_TRUE;

   snprintf(name, sizeof(name), "X/DRI:%s", xdri_drv->dri_driver_name);
   xdri_drv->Base.Name = name;

   /* we're supporting EGL 1.4 */
   *minor = 1;
   *major = 4;

   return EGL_TRUE;
}


/*
 * Do some clean-up that normally occurs in XCloseDisplay().
 * We do this here because we're about to unload a dynamic library
 * that has added some per-display extension data and callbacks.
 * If we don't do this here we'll crash in XCloseDisplay() because it'll
 * try to call functions that went away when the driver library was unloaded.
 */
static void
FreeDisplayExt(Display *dpy)
{
   _XExtension *ext, *next;

   for (ext = dpy->ext_procs; ext; ext = next) {
      next = ext->next;
      if (ext->close_display) {
         ext->close_display(dpy, &ext->codes);
         ext->close_display = NULL;
      }
      if (ext->name)
         Xfree(ext->name);
      Xfree(ext);
   }
   dpy->ext_procs = NULL;

   _XFreeExtData (dpy->ext_data);
   dpy->ext_data = NULL;
}


/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
xdri_eglTerminate(_EGLDriver *drv, EGLDisplay dpy)
{
   struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   _eglLog(_EGL_DEBUG, "XDRI: eglTerminate");

   _eglLog(_EGL_DEBUG, "XDRI: Closing %s", xdri_drv->dri_driver_name);

   FreeDisplayExt(disp->Xdpy);

#if 0
   /* this causes a segfault for some reason */
   dlclose(xdri_drv->dri_driver_handle);
#endif
   xdri_drv->dri_driver_handle = NULL;

   free((void*) xdri_drv->dri_driver_name);

   return EGL_TRUE;
}


/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
xdri_eglGetProcAddress(const char *procname)
{
#if 0
   _EGLDriver *drv = NULL;

   struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
   /*_EGLDisplay *disp = _eglLookupDisplay(dpy);*/
   _EGLProc *proc = xdri_drv->driScreen.getProcAddress(procname);
   return proc;
#elif 1
   /* This is a bit of a hack to get at the gallium/Mesa state tracker
    * function st_get_proc_address().  This will probably change at
    * some point.
    */
   _EGLProc (*st_get_proc_addr)(const char *procname);
   st_get_proc_addr = dlsym(NULL, "st_get_proc_address");
   if (st_get_proc_addr) {
      return st_get_proc_addr(procname);
   }
   return NULL;   
#else
   return NULL;
#endif
}


/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static EGLContext
xdri_eglCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                      EGLContext share_list, const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   struct xdri_egl_config *xdri_config = lookup_config(drv, dpy, config);
   void *shared = NULL;
   int renderType = GLX_RGBA_BIT;

   struct xdri_egl_context *xdri_ctx = CALLOC_STRUCT(xdri_egl_context);
   if (!xdri_ctx)
      return EGL_NO_CONTEXT;

   if (!_eglInitContext(drv, dpy, &xdri_ctx->Base, config, attrib_list)) {
      free(xdri_ctx);
      return EGL_NO_CONTEXT;
   }

   assert(xdri_config);

   {
      struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
      __GLXscreenConfigs *scrnConf = xdri_drv->glx_priv->screenConfigs;
      xdri_ctx->driContext.private = 
         scrnConf->driScreen.createNewContext(disp->Xdpy,
                                              xdri_config->mode, renderType,
                                              shared, &xdri_ctx->driContext);
   }

   if (!xdri_ctx->driContext.private) {
      _eglLog(_EGL_DEBUG, "driScreen.createNewContext failed");
      free(xdri_ctx);
      return EGL_NO_CONTEXT;
   }

   xdri_ctx->driContext.mode = xdri_config->mode;

   return _eglGetContextHandle(&xdri_ctx->Base);
}


/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
xdri_eglMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface d,
                    EGLSurface r, EGLContext context)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   struct xdri_egl_context *xdri_ctx = lookup_context(context);
   struct xdri_egl_surface *xdri_draw = lookup_surface(d);
   struct xdri_egl_surface *xdri_read = lookup_surface(r);
   __DRIid draw = xdri_draw ? xdri_draw->driDrawable : 0;
   __DRIid read = xdri_read ? xdri_read->driDrawable : 0;
   int scrn = DefaultScreen(disp->Xdpy);

   if (!_eglMakeCurrent(drv, dpy, d, r, context))
      return EGL_FALSE;


   if (xdri_ctx &&
       !xdri_ctx->driContext.bindContext(disp->Xdpy, scrn, draw, read,
                                         &xdri_ctx->driContext)) {
      return EGL_FALSE;
   }

   return EGL_TRUE;
}


/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static EGLSurface
xdri_eglCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                            NativeWindowType window, const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   struct xdri_egl_surface *xdri_surf;
   int scrn = DefaultScreen(disp->Xdpy);
   uint width, height;

   xdri_surf = CALLOC_STRUCT(xdri_egl_surface);
   if (!xdri_surf)
      return EGL_NO_SURFACE;

   if (!_eglInitSurface(drv, dpy, &xdri_surf->Base, EGL_WINDOW_BIT,
                        config, attrib_list)) {
      free(xdri_surf);
      return EGL_FALSE;
   }

   if (!XF86DRICreateDrawable(disp->Xdpy, scrn, window, &xdri_surf->hDrawable)) {
      free(xdri_surf);
      return EGL_FALSE;
   }

   xdri_surf->driDrawable = window;

   _eglSaveSurface(&xdri_surf->Base);

   get_drawable_size(disp->Xdpy, window, &width, &height);
   xdri_surf->Base.Width = width;
   xdri_surf->Base.Height = height;

   _eglLog(_EGL_DEBUG,
           "XDRI: CreateWindowSurface win 0x%x  handle %d  hDrawable %d",
           (int) window, _eglGetSurfaceHandle(&xdri_surf->Base),
           (int) xdri_surf->hDrawable);

   return _eglGetSurfaceHandle(&xdri_surf->Base);
}


/**
 * Called via eglCreatePbufferSurface(), drv->API.CreatePbufferSurface().
 */
static EGLSurface
xdri_eglCreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                             const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   struct xdri_egl_surface *xdri_surf;
   struct xdri_egl_config *xdri_config = lookup_config(drv, dpy, config);
   int scrn = DefaultScreen(disp->Xdpy);
   Window window;

   xdri_surf = CALLOC_STRUCT(xdri_egl_surface);
   if (!xdri_surf)
      return EGL_NO_SURFACE;

   if (!_eglInitSurface(drv, dpy, &xdri_surf->Base, EGL_PBUFFER_BIT,
                        config, attrib_list)) {
      free(xdri_surf);
      return EGL_FALSE;
   }

   /* Create a dummy X window */
   {
      Window root = RootWindow(disp->Xdpy, scrn);
      XSetWindowAttributes attr;
      XVisualInfo *visInfo, visTemplate;
      unsigned mask;
      int nvis;

      visTemplate.visualid = xdri_config->mode->visualID;
      visInfo = XGetVisualInfo(disp->Xdpy, VisualIDMask, &visTemplate, &nvis);
      if (!visInfo) {
         return EGL_NO_SURFACE;
      }

      attr.background_pixel = 0;
      attr.border_pixel = 0;
      attr.colormap = XCreateColormap(disp->Xdpy, root,
                                      visInfo->visual, AllocNone);
      attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
      mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
      
      window = XCreateWindow(disp->Xdpy, root, 0, 0,
                             xdri_surf->Base.Width, xdri_surf->Base.Height,
                             0, visInfo->depth, InputOutput,
                             visInfo->visual, mask, &attr);

      /*XMapWindow(disp->Xdpy, window);*/
      XFree(visInfo);

      /* set hints and properties */
      /*
      sizehints.width = xdri_surf->Base.Width;
      sizehints.height = xdri_surf->Base.Height;
      sizehints.flags = USPosition;
      XSetNormalHints(disp->Xdpy, window, &sizehints);
      */
   }

   if (!XF86DRICreateDrawable(disp->Xdpy, scrn, window, &xdri_surf->hDrawable)) {
      free(xdri_surf);
      return EGL_FALSE;
   }

   xdri_surf->driDrawable = window;

   _eglSaveSurface(&xdri_surf->Base);

   _eglLog(_EGL_DEBUG,
           "XDRI: CreatePbufferSurface handle %d  hDrawable %d",
           _eglGetSurfaceHandle(&xdri_surf->Base),
           (int) xdri_surf->hDrawable);

   return _eglGetSurfaceHandle(&xdri_surf->Base);
}



static EGLBoolean
xdri_eglDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   struct xdri_egl_surface *xdri_surf = lookup_surface(surface);
   if (xdri_surf) {
      _eglHashRemove(_eglGlobal.Surfaces, (EGLuint) surface);
      if (xdri_surf->Base.IsBound) {
         xdri_surf->Base.DeletePending = EGL_TRUE;
      }
      else {
         /*
         st_unreference_framebuffer(&surf->Framebuffer);
         */
         free(xdri_surf);
      }
      return EGL_TRUE;
   }
   else {
      _eglError(EGL_BAD_SURFACE, "eglDestroySurface");
      return EGL_FALSE;
   }
}


static EGLBoolean
xdri_eglBindTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface,
                     EGLint buffer)
{
   typedef int (*bind_teximage)(__DRInativeDisplay *dpy,
                                __DRIid surface, __DRIscreen *psc,
                                int buffer, int target, int format,
                                int level, int mipmap);

   bind_teximage egl_dri_bind_teximage;

   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   struct xdri_egl_context *xdri_ctx = current_context();
   struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
   struct xdri_egl_surface *xdri_surf = lookup_surface(surface);

   __DRIid dri_surf = xdri_surf ? xdri_surf->driDrawable : 0;

   __GLXscreenConfigs *scrnConf = xdri_drv->glx_priv->screenConfigs;
   __DRIscreen *psc = &scrnConf->driScreen;

   /* this call just does error checking */
   if (!_eglBindTexImage(drv, dpy, surface, buffer)) {
      return EGL_FALSE;
   }

   egl_dri_bind_teximage =
      (bind_teximage) dlsym(NULL, "egl_dri_bind_teximage");
   if (egl_dri_bind_teximage) {
      return egl_dri_bind_teximage(disp->Xdpy, dri_surf, psc,
                                   buffer,
                                   xdri_surf->Base.TextureTarget,
                                   xdri_surf->Base.TextureFormat,
                                   xdri_surf->Base.MipmapLevel,
                                   xdri_surf->Base.MipmapTexture);
   }
   else {
      /* fallback path based on glCopyTexImage() */
      /* Get/save currently bound 2D texobj name */
      glGetIntegerv_t glGetIntegerv_func = 
         (glGetIntegerv_t) dlsym(NULL, "glGetIntegerv");
      GLint curTexObj = 0;
      if (glGetIntegerv_func) {
         (*glGetIntegerv_func)(GL_TEXTURE_BINDING_2D, &curTexObj);
      }
      xdri_ctx->bound_tex_object = curTexObj;
   }

   return EGL_FALSE;
}


static EGLBoolean
xdri_eglReleaseTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface,
                        EGLint buffer)
{
   typedef int (*release_teximage)(__DRInativeDisplay *dpy,
                                   __DRIid surface, __DRIscreen *psc,
                                   int buffer, int target, int format,
                                   int level, int mipmap);
   release_teximage egl_dri_release_teximage;

   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   struct xdri_egl_context *xdri_ctx = current_context();
   struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
   struct xdri_egl_surface *xdri_surf = lookup_surface(surface);

   __DRIid dri_surf = xdri_surf ? xdri_surf->driDrawable : 0;

   __GLXscreenConfigs *scrnConf = xdri_drv->glx_priv->screenConfigs;
   __DRIscreen *psc = &scrnConf->driScreen;

   /* this call just does error checking */
   if (!_eglReleaseTexImage(drv, dpy, surface, buffer)) {
      return EGL_FALSE;
   }

   egl_dri_release_teximage =
      (release_teximage) dlsym(NULL, "egl_dri_release_teximage");
   if (egl_dri_release_teximage) {
      return egl_dri_release_teximage(disp->Xdpy, dri_surf, psc,
                                      buffer,
                                      xdri_surf->Base.TextureTarget,
                                      xdri_surf->Base.TextureFormat,
                                      xdri_surf->Base.MipmapLevel,
                                      xdri_surf->Base.MipmapTexture);
   }
   else {
      /* fallback path based on glCopyTexImage() */
      glGetIntegerv_t glGetIntegerv_func = 
         (glGetIntegerv_t) dlsym(NULL, "glGetIntegerv");
      glBindTexture_t glBindTexture_func = 
         (glBindTexture_t) dlsym(NULL, "glBindTexture");
      glCopyTexImage2D_t glCopyTexImage2D_func = 
         (glCopyTexImage2D_t) dlsym(NULL, "glCopyTexImage2D");
      GLint curTexObj;
      GLenum intFormat;
      GLint level, width, height;

      if (xdri_surf->Base.TextureFormat == EGL_TEXTURE_RGBA)
         intFormat = GL_RGBA;
      else
         intFormat = GL_RGB;
      level = xdri_surf->Base.MipmapLevel;
      width = xdri_surf->Base.Width >> level;
      height = xdri_surf->Base.Height >> level;

      if (width > 0 && height > 0 &&
          glGetIntegerv_func && glBindTexture_func && glCopyTexImage2D_func) {
         glGetIntegerv_func(GL_TEXTURE_BINDING_2D, &curTexObj);
         /* restore texobj from time of eglBindTexImage() call */
         if (curTexObj != xdri_ctx->bound_tex_object)
            glBindTexture_func(GL_TEXTURE_2D, xdri_ctx->bound_tex_object);
         /* copy pbuffer image to texture */
         glCopyTexImage2D_func(GL_TEXTURE_2D,
                               level,
                               intFormat,
                               0, 0, width, height, 0);
         /* restore current texture */
         if (curTexObj != xdri_ctx->bound_tex_object)
            glBindTexture_func(GL_TEXTURE_2D, curTexObj);
      }
      xdri_ctx->bound_tex_object = -1;
   }

   return EGL_FALSE;
}


static EGLBoolean
xdri_eglSwapBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   _eglLog(_EGL_DEBUG, "XDRI: EGL SwapBuffers");

   /* error checking step: */
   if (!_eglSwapBuffers(drv, dpy, draw))
      return EGL_FALSE;

   {
      struct xdri_egl_surface *xdri_surf = lookup_surface(draw);
      struct xdri_egl_driver *xdri_drv = xdri_egl_driver(drv);
      __GLXscreenConfigs *scrnConf = xdri_drv->glx_priv->screenConfigs;
      __DRIscreen *psc = &scrnConf->driScreen;
      __DRIdrawable * const pdraw = psc->getDrawable(disp->Xdpy,
                                                     xdri_surf->driDrawable,
                                                     psc->private);

      if (pdraw)
         pdraw->swapBuffers(disp->Xdpy, pdraw->private);
      else
         _eglLog(_EGL_WARNING, "pdraw is null in SwapBuffers");
   }

   return EGL_TRUE;
}


/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_eglMain(_EGLDisplay *disp, const char *args)
{
   struct xdri_egl_driver *xdri_drv = CALLOC_STRUCT(xdri_egl_driver);
   if (!xdri_drv)
      return NULL;

   /* Tell libGL to prefer the EGL drivers over regular DRI drivers */
   __glXPreferEGL(1);

   _eglInitDriverFallbacks(&xdri_drv->Base);
   xdri_drv->Base.API.Initialize = xdri_eglInitialize;
   xdri_drv->Base.API.Terminate = xdri_eglTerminate;

   xdri_drv->Base.API.GetProcAddress = xdri_eglGetProcAddress;

   xdri_drv->Base.API.CreateContext = xdri_eglCreateContext;
   xdri_drv->Base.API.MakeCurrent = xdri_eglMakeCurrent;
   xdri_drv->Base.API.CreateWindowSurface = xdri_eglCreateWindowSurface;
   xdri_drv->Base.API.CreatePbufferSurface = xdri_eglCreatePbufferSurface;
   xdri_drv->Base.API.DestroySurface = xdri_eglDestroySurface;
   xdri_drv->Base.API.BindTexImage = xdri_eglBindTexImage;
   xdri_drv->Base.API.ReleaseTexImage = xdri_eglReleaseTexImage;
   xdri_drv->Base.API.SwapBuffers = xdri_eglSwapBuffers;

   xdri_drv->Base.ClientAPIsMask = (EGL_OPENGL_BIT |
                                    EGL_OPENGL_ES_BIT |
                                    EGL_OPENGL_ES2_BIT |
                                    EGL_OPENVG_BIT);
   xdri_drv->Base.Name = "X/DRI";

   _eglLog(_EGL_DEBUG, "XDRI: main(%s)", args);

   return &xdri_drv->Base;
}
