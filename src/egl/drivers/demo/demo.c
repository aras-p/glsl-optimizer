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
demoInitialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
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

   drv->Initialized = EGL_TRUE;

   *major = 1;
   *minor = 0;

   return EGL_TRUE;
}


static EGLBoolean
demoTerminate(_EGLDriver *drv, EGLDisplay dpy)
{
   /*DemoDriver *demo = DEMO_DRIVER(dpy);*/
   free(drv);
   return EGL_TRUE;
}


static DemoContext *
LookupDemoContext(EGLContext ctx)
{
   _EGLContext *c = _eglLookupContext(ctx);
   return (DemoContext *) c;
}


static DemoSurface *
LookupDemoSurface(EGLSurface surf)
{
   _EGLSurface *s = _eglLookupSurface(surf);
   return (DemoSurface *) s;
}



static EGLContext
demoCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   DemoContext *c;
   int i;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreateContext");
      return EGL_NO_CONTEXT;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs defined for now */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
         return EGL_NO_CONTEXT;
      }
   }

   c = (DemoContext *) calloc(1, sizeof(DemoContext));
   if (!c)
      return EGL_NO_CONTEXT;

   _eglInitContext(drv, dpy, &c->Base, config, attrib_list);
   c->DemoStuff = 1;
   printf("demoCreateContext\n");

   /* generate handle and insert into hash table */
   _eglSaveContext(&c->Base);
   assert(_eglGetContextHandle(&c->Base));

   return _eglGetContextHandle(&c->Base);
}


static EGLSurface
demoCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
   int i;
   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs at this time */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateWindowSurface");
         return EGL_NO_SURFACE;
      }
   }
   printf("eglCreateWindowSurface()\n");
   /* XXX unfinished */

   return EGL_NO_SURFACE;
}


static EGLSurface
demoCreatePixmapSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   EGLint i;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreatePixmapSurface");
      return EGL_NO_SURFACE;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs at this time */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreatePixmapSurface");
         return EGL_NO_SURFACE;
      }
   }

   if (conf->Attrib[EGL_SURFACE_TYPE - FIRST_ATTRIB] == 0) {
      _eglError(EGL_BAD_MATCH, "eglCreatePixmapSurface");
      return EGL_NO_SURFACE;
   }

   printf("eglCreatePixmapSurface()\n");
   return EGL_NO_SURFACE;
}


static EGLSurface
demoCreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                         const EGLint *attrib_list)
{
   DemoSurface *surf = (DemoSurface *) calloc(1, sizeof(DemoSurface));
   if (!surf)
      return EGL_NO_SURFACE;

   if (!_eglInitSurface(drv, dpy, &surf->Base, EGL_PBUFFER_BIT,
                        config, attrib_list)) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   /* a real driver would allocate the pbuffer memory here */

   return surf->Base.Handle;
}


static EGLBoolean
demoDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   DemoSurface *fs = LookupDemoSurface(surface);
   _eglRemoveSurface(&fs->Base);
   if (fs->Base.IsBound) {
      fs->Base.DeletePending = EGL_TRUE;
   }
   else {
      free(fs);
   }
   return EGL_TRUE;
}


static EGLBoolean
demoDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext context)
{
   DemoContext *fc = LookupDemoContext(context);
   _eglRemoveContext(&fc->Base);
   if (fc->Base.IsBound) {
      fc->Base.DeletePending = EGL_TRUE;
   }
   else {
      free(fc);
   }
   return EGL_TRUE;
}


static EGLBoolean
demoMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext context)
{
   /*DemoDriver *demo = DEMO_DRIVER(dpy);*/
   DemoSurface *readSurf = LookupDemoSurface(read);
   DemoSurface *drawSurf = LookupDemoSurface(draw);
   DemoContext *ctx = LookupDemoContext(context);
   EGLBoolean b;

   b = _eglMakeCurrent(drv, dpy, draw, read, context);
   if (!b)
      return EGL_FALSE;

   /* XXX this is where we'd do the hardware context switch */
   (void) drawSurf;
   (void) readSurf;
   (void) ctx;

   printf("eglMakeCurrent()\n");
   return EGL_TRUE;
}


/**
 * The bootstrap function.  Return a new DemoDriver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy, const char *args)
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

   /* enable supported extensions */
   demo->Base.Extensions.MESA_screen_surface = EGL_TRUE;
   demo->Base.Extensions.MESA_copy_context = EGL_TRUE;

   return &demo->Base;
}
