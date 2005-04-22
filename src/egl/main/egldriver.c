#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglmode.h"
#include "eglsurface.h"


const char *DefaultDriverName = "demo";


/**
 * Choose and open/init the hardware driver for the given EGLDisplay.
 * Previously, the EGLDisplay was created with _eglNewDisplay() where
 * we recorded the user's NativeDisplayType parameter.
 *
 * Now we'll use the NativeDisplayType value.
 *
 * Currently, the native display value is treated as a string.
 * If the first character is ':' we interpret it as a screen or card index
 * number (i.e. ":0" or ":1", etc)
 * Else if the first character is '!' we interpret it as specific driver name
 * (i.e. "!r200" or "!i830".
 */
_EGLDriver *
_eglChooseDriver(EGLDisplay display)
{
   _EGLDisplay *dpy = _eglLookupDisplay(display);
   _EGLDriver *drv;
   const char *driverName = DefaultDriverName;
   const char *name;

   assert(dpy);

   name = dpy->Name;
   if (!name) {
      /* use default */
   }
   else if (name[0] == ':' && (name[1] >= '0' && name[1] <= '9') && !name[2]) {
      printf("EGL: Use driver for screen: %s\n", name);
      /* XXX probe hardware here to determine which driver to open */
      /* driverName = "something"; */
   }
   else if (name[0] == '!') {
      /* use specified driver name */
      driverName = name + 1;
      printf("EGL: Use driver named %s\n", driverName);
   }
   else {
      /* Maybe display was returned by XOpenDisplay? */
      printf("EGL: can't parse display pointer\n");
   }

   drv = _eglOpenDriver(dpy, driverName);
   dpy->Driver = drv;

   return drv;
}


/**
 * Open/load the named driver and call its bootstrap function: _eglMain().
 * \return  new _EGLDriver object.
 */
_EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy, const char *driverName)
{
   void *lib;
   char driverFilename[1000];

   /* XXX also prepend a directory path??? */
   sprintf(driverFilename, "%sdriver.so", driverName);

#if 1
   lib = dlopen(driverFilename, RTLD_NOW);
   if (lib) {
      _EGLDriver *drv;
      _EGLMain_t mainFunc;

      mainFunc = (_EGLMain_t) dlsym(lib, "_eglMain");
      if (!mainFunc) {
         fprintf(stderr, "_eglMain not found in %s", (char *) driverFilename);
         dlclose(lib);
         return NULL;
      }

      drv = mainFunc(dpy);
      if (!drv) {
         dlclose(lib);
         return NULL;
      }

      drv->LibHandle = lib;
      drv->Display = dpy;
      return drv;
   }
   else {
      fprintf(stderr, "EGLdebug: Error opening %s: %s\n",
              driverFilename, dlerror());
      return NULL;
   }
#else
   /* use built-in driver */
   return _eglDefaultMain(d);
#endif
}


EGLBoolean
_eglCloseDriver(_EGLDriver *drv, EGLDisplay dpy)
{
   void *handle = drv->LibHandle;
   EGLBoolean b;
   fprintf(stderr, "EGL debug: Closing driver\n");

   /*
    * XXX check for currently bound context/surfaces and delete them?
    */

   b = drv->Terminate(drv, dpy);
   dlclose(handle);
   return b;
}


/**
 * Given a display handle, return the _EGLDriver for that display.
 */
_EGLDriver *
_eglLookupDriver(EGLDisplay dpy)
{
   _EGLDisplay *d = _eglLookupDisplay(dpy);
   if (d)
      return d->Driver;
   else
      return NULL;
}


/**
 * Plug all the available fallback routines into the given driver's
 * dispatch table.
 */
void
_eglInitDriverFallbacks(_EGLDriver *drv)
{
   /* If a pointer is set to NULL, then the device driver _really_ has
    * to implement it.
    */
   drv->Initialize = NULL;
   drv->Terminate = NULL;

   drv->GetConfigs = _eglGetConfigs;
   drv->ChooseConfig = _eglChooseConfig;
   drv->GetConfigAttrib = _eglGetConfigAttrib;

   drv->CreateContext = _eglCreateContext;
   drv->DestroyContext = _eglDestroyContext;
   drv->MakeCurrent = _eglMakeCurrent;
   drv->QueryContext = _eglQueryContext;

   drv->CreateWindowSurface = _eglCreateWindowSurface;
   drv->CreatePixmapSurface = _eglCreatePixmapSurface;
   drv->CreatePbufferSurface = _eglCreatePbufferSurface;
   drv->DestroySurface = _eglDestroySurface;
   drv->QuerySurface = _eglQuerySurface;
   drv->SurfaceAttrib = _eglSurfaceAttrib;
   drv->BindTexImage = _eglBindTexImage;
   drv->ReleaseTexImage = _eglReleaseTexImage;
   drv->SwapInterval = _eglSwapInterval;
   drv->SwapBuffers = _eglSwapBuffers;
   drv->CopyBuffers = _eglCopyBuffers;

   drv->QueryString = _eglQueryString;
   drv->WaitGL = _eglWaitGL;
   drv->WaitNative = _eglWaitNative;

   /* EGL_MESA_screen */
   drv->GetModesMESA = _eglGetModesMESA;
   drv->GetModeAttribMESA = _eglGetModeAttribMESA;
}


const char *
_eglQueryString(_EGLDriver *drv, EGLDisplay dpy, EGLint name)
{
   (void) drv;
   (void) dpy;
   switch (name) {
   case EGL_VENDOR:
      return "Mesa Project";
   case EGL_VERSION:
      return "1.0";
   case EGL_EXTENSIONS:
      return "";
   default:
      _eglError(EGL_BAD_PARAMETER, "eglQueryString");
      return NULL;
   }
}


EGLBoolean
_eglWaitGL(_EGLDriver *drv, EGLDisplay dpy)
{
   /* just a placeholder */
   (void) drv;
   (void) dpy;
   return EGL_TRUE;
}


EGLBoolean
_eglWaitNative(_EGLDriver *drv, EGLDisplay dpy, EGLint engine)
{
   /* just a placeholder */
   (void) drv;
   (void) dpy;
   (void) engine;
   return EGL_TRUE;
}
