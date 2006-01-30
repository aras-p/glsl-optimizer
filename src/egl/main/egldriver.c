#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglsurface.h"


const char *DefaultDriverName = "demodriver";


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
      /* XXX probe hardware here to determine which driver to open */
      driverName = "libEGLdri";
   }
   else if (name[0] == '!') {
      /* use specified driver name */
      driverName = name + 1;
   }
   else {
      /* Maybe display was returned by XOpenDisplay? */
      _eglLog(_EGL_FATAL, "eglChooseDriver() bad name");
   }

   _eglLog(_EGL_INFO, "eglChooseDriver() choosing %s", driverName);

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
   _EGLDriver *drv;
   _EGLMain_t mainFunc;
   void *lib;
   char driverFilename[1000];

   /* XXX also prepend a directory path??? */
   sprintf(driverFilename, "%s.so", driverName);

   _eglLog(_EGL_DEBUG, "dlopen(%s)", driverFilename);
   lib = dlopen(driverFilename, RTLD_NOW);
   if (!lib) {
      _eglLog(_EGL_WARNING, "Could not open %s (%s)",
              driverFilename, dlerror());
      return NULL;
   }

   mainFunc = (_EGLMain_t) dlsym(lib, "_eglMain");
   if (!mainFunc) {
      _eglLog(_EGL_WARNING, "_eglMain not found in %s", driverFilename);
      dlclose(lib);
      return NULL;
   }

   drv = mainFunc(dpy);
   if (!drv) {
      dlclose(lib);
      return NULL;
   }
   /* with a recurvise open you want the inner most handle */
   if (!drv->LibHandle)
      drv->LibHandle = lib;
   else
      dlclose(lib);

   drv->Display = dpy;
   return drv;
}


EGLBoolean
_eglCloseDriver(_EGLDriver *drv, EGLDisplay dpy)
{
   void *handle = drv->LibHandle;
   EGLBoolean b;

   _eglLog(_EGL_INFO, "Closing driver");

   /*
    * XXX check for currently bound context/surfaces and delete them?
    */

   b = drv->API.Terminate(drv, dpy);
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
   drv->API.Initialize = NULL;
   drv->API.Terminate = NULL;

   drv->API.GetConfigs = _eglGetConfigs;
   drv->API.ChooseConfig = _eglChooseConfig;
   drv->API.GetConfigAttrib = _eglGetConfigAttrib;

   drv->API.CreateContext = _eglCreateContext;
   drv->API.DestroyContext = _eglDestroyContext;
   drv->API.MakeCurrent = _eglMakeCurrent;
   drv->API.QueryContext = _eglQueryContext;

   drv->API.CreateWindowSurface = _eglCreateWindowSurface;
   drv->API.CreatePixmapSurface = _eglCreatePixmapSurface;
   drv->API.CreatePbufferSurface = _eglCreatePbufferSurface;
   drv->API.DestroySurface = _eglDestroySurface;
   drv->API.QuerySurface = _eglQuerySurface;
   drv->API.SurfaceAttrib = _eglSurfaceAttrib;
   drv->API.BindTexImage = _eglBindTexImage;
   drv->API.ReleaseTexImage = _eglReleaseTexImage;
   drv->API.SwapInterval = _eglSwapInterval;
   drv->API.SwapBuffers = _eglSwapBuffers;
   drv->API.CopyBuffers = _eglCopyBuffers;

   drv->API.QueryString = _eglQueryString;
   drv->API.WaitGL = _eglWaitGL;
   drv->API.WaitNative = _eglWaitNative;

#ifdef EGL_MESA_screen_surface
   drv->API.ChooseModeMESA = _eglChooseModeMESA; 
   drv->API.GetModesMESA = _eglGetModesMESA;
   drv->API.GetModeAttribMESA = _eglGetModeAttribMESA;
   drv->API.GetScreensMESA = _eglGetScreensMESA;
   drv->API.CreateScreenSurfaceMESA = _eglCreateScreenSurfaceMESA;
   drv->API.ShowScreenSurfaceMESA = _eglShowScreenSurfaceMESA;
   drv->API.ScreenPositionMESA = _eglScreenPositionMESA;
   drv->API.QueryScreenMESA = _eglQueryScreenMESA;
   drv->API.QueryScreenSurfaceMESA = _eglQueryScreenSurfaceMESA;
   drv->API.QueryScreenModeMESA = _eglQueryScreenModeMESA;
   drv->API.QueryModeStringMESA = _eglQueryModeStringMESA;
#endif /* EGL_MESA_screen_surface */

#ifdef EGL_VERSION_1_2
   drv->API.CreatePbufferFromClientBuffer = _eglCreatePbufferFromClientBuffer;
#endif /* EGL_VERSION_1_2 */
}


/**
 * Examine the individual extension enable/disable flags and recompute
 * the driver's Extensions string.
 */
static void
_eglUpdateExtensionsString(_EGLDriver *drv)
{
   drv->Extensions.String[0] = 0;

   if (drv->Extensions.MESA_screen_surface)
      strcat(drv->Extensions.String, "EGL_MESA_screen_surface ");
   if (drv->Extensions.MESA_copy_context)
      strcat(drv->Extensions.String, "EGL_MESA_copy_context ");
   assert(strlen(drv->Extensions.String) < MAX_EXTENSIONS_LEN);
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
      _eglUpdateExtensionsString(drv);
      return drv->Extensions.String;
#ifdef EGL_VERSION_1_2
   case EGL_CLIENT_APIS:
      /* XXX need to initialize somewhere */
      return drv->ClientAPIs;
#endif
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
   switch (engine) {
   case EGL_CORE_NATIVE_ENGINE:
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglWaitNative(engine)");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}
