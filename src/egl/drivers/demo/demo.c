/*
 * Sample driver: Demo
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglsurface.h"


/**
 * Demo driver-specific driver class derived from _EGLDriver
 */
typedef struct demo_driver
{
   _EGLDriver Base;  /* base class/object */
   unsigned DemoStuff;
} DemoDriver;

#define DEMO_DRIVER(D) ((DemoDriver *) (D))


/**
 * Demo driver-specific surface class derived from _EGLSurface
 */
typedef struct demo_surface
{
   _EGLSurface Base;  /* base class/object */
   unsigned DemoStuff;
} DemoSurface;


/**
 * Demo driver-specific context class derived from _EGLContext
 */
typedef struct demo_context
{
   _EGLContext Base;  /* base class/object */
   unsigned DemoStuff;
} DemoContext;



static EGLBoolean
demoInitialize(_EGLDriver *drv, _EGLDisplay *disp, EGLint *major, EGLint *minor)
{
   _EGLScreen *scrn;
   EGLint i;

   /* Create a screen */
   scrn = calloc(1, sizeof(*scrn));
   _eglAddScreen(disp, scrn);

   /* Create the screen's modes - silly example */
   _eglAddNewMode(scrn, 1600, 1200, 72 * 1000, "1600x1200-72");
   _eglAddNewMode(scrn, 1280, 1024, 72 * 1000, "1280x1024-70");
   _eglAddNewMode(scrn, 1280, 1024, 70 * 1000, "1280x1024-70");
   _eglAddNewMode(scrn, 1024,  768, 72 * 1000, "1024x768-72");

   /* Create the display's visual configs - silly example */
   for (i = 0; i < 4; i++) {
      _EGLConfig *config = calloc(1, sizeof(_EGLConfig));
      _eglInitConfig(config, i + 1);
      _eglSetConfigAttrib(config, EGL_RED_SIZE, 8);
      _eglSetConfigAttrib(config, EGL_GREEN_SIZE, 8);
      _eglSetConfigAttrib(config, EGL_BLUE_SIZE, 8);
      _eglSetConfigAttrib(config, EGL_ALPHA_SIZE, 8);
      _eglSetConfigAttrib(config, EGL_BUFFER_SIZE, 32);
      if (i & 1) {
         _eglSetConfigAttrib(config, EGL_DEPTH_SIZE, 32);
      }
      if (i & 2) {
         _eglSetConfigAttrib(config, EGL_STENCIL_SIZE, 8);
      }
      _eglSetConfigAttrib(config, EGL_SURFACE_TYPE,
                          (EGL_WINDOW_BIT | EGL_PIXMAP_BIT | EGL_PBUFFER_BIT));
      _eglAddConfig(disp, config);
   }

   /* enable supported extensions */
   disp->Extensions.MESA_screen_surface = EGL_TRUE;
   disp->Extensions.MESA_copy_context = EGL_TRUE;

   *major = 1;
   *minor = 0;

   return EGL_TRUE;
}


static EGLBoolean
demoTerminate(_EGLDriver *drv, _EGLDisplay *dpy)
{
   /*DemoDriver *demo = DEMO_DRIVER(dpy);*/
   return EGL_TRUE;
}


static DemoContext *
LookupDemoContext(_EGLContext *c)
{
   return (DemoContext *) c;
}


static DemoSurface *
LookupDemoSurface(_EGLSurface *s)
{
   return (DemoSurface *) s;
}


static _EGLContext *
demoCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, _EGLContext *share_list, const EGLint *attrib_list)
{
   DemoContext *c;
   int i;

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs defined for now */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
         return NULL;
      }
   }

   c = (DemoContext *) calloc(1, sizeof(DemoContext));
   if (!c)
      return NULL;

   _eglInitContext(drv, &c->Base, conf, attrib_list);
   c->DemoStuff = 1;
   printf("demoCreateContext\n");

   return &c->Base;
}


static _EGLSurface *
demoCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativeWindowType window, const EGLint *attrib_list)
{
   int i;
   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs at this time */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateWindowSurface");
         return NULL;
      }
   }
   printf("eglCreateWindowSurface()\n");
   /* XXX unfinished */

   return NULL;
}


static _EGLSurface *
demoCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativePixmapType pixmap, const EGLint *attrib_list)
{
   EGLint i;

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs at this time */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreatePixmapSurface");
         return NULL;
      }
   }

   if (GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE) == 0) {
      _eglError(EGL_BAD_MATCH, "eglCreatePixmapSurface");
      return NULL;
   }

   printf("eglCreatePixmapSurface()\n");
   return NULL;
}


static _EGLSurface *
demoCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                         const EGLint *attrib_list)
{
   DemoSurface *surf = (DemoSurface *) calloc(1, sizeof(DemoSurface));

   if (!surf)
      return NULL;

   if (!_eglInitSurface(drv, &surf->Base, EGL_PBUFFER_BIT,
                        conf, attrib_list)) {
      free(surf);
      return NULL;
   }

   /* a real driver would allocate the pbuffer memory here */

   return &surf->Base;
}


static EGLBoolean
demoDestroySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface)
{
   DemoSurface *fs = LookupDemoSurface(surface);
   if (!_eglIsSurfaceBound(&fs->Base))
      free(fs);
   return EGL_TRUE;
}


static EGLBoolean
demoDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *context)
{
   DemoContext *fc = LookupDemoContext(context);
   if (!_eglIsContextBound(&fc->Base))
      free(fc);
   return EGL_TRUE;
}


static EGLBoolean
demoMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *drawSurf, _EGLSurface *readSurf, _EGLContext *ctx)
{
   /*DemoDriver *demo = DEMO_DRIVER(dpy);*/
   EGLBoolean b;

   b = _eglMakeCurrent(drv, dpy, drawSurf, readSurf, ctx);
   if (!b)
      return EGL_FALSE;

   /* XXX this is where we'd do the hardware context switch */
   (void) drawSurf;
   (void) readSurf;
   (void) ctx;

   printf("eglMakeCurrent()\n");
   return EGL_TRUE;
}


static void
demoUnload(_EGLDriver *drv)
{
   free(drv);
}


/**
 * The bootstrap function.  Return a new DemoDriver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(const char *args)
{
   DemoDriver *demo;

   demo = (DemoDriver *) calloc(1, sizeof(DemoDriver));
   if (!demo) {
      return NULL;
   }

   /* First fill in the dispatch table with defaults */
   _eglInitDriverFallbacks(&demo->Base);
   /* then plug in our Demo-specific functions */
   demo->Base.API.Initialize = demoInitialize;
   demo->Base.API.Terminate = demoTerminate;
   demo->Base.API.CreateContext = demoCreateContext;
   demo->Base.API.MakeCurrent = demoMakeCurrent;
   demo->Base.API.CreateWindowSurface = demoCreateWindowSurface;
   demo->Base.API.CreatePixmapSurface = demoCreatePixmapSurface;
   demo->Base.API.CreatePbufferSurface = demoCreatePbufferSurface;
   demo->Base.API.DestroySurface = demoDestroySurface;
   demo->Base.API.DestroyContext = demoDestroyContext;

   demo->Base.Name = "egl/demo";
   demo->Base.Unload = demoUnload;

   return &demo->Base;
}
