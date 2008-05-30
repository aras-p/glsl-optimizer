/**
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglmisc.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglstring.h"
#include "eglsurface.h"

#if defined(_EGL_PLATFORM_X)
#include "eglx.h"
#elif defined(_EGL_PLATFORM_WINDOWS)
/* XXX to do */
#elif defined(_EGL_PLATFORM_WINCE)
/* XXX to do */
#endif


const char *DefaultDriverName = ":0";
const char *SysFS = "/sys/class";




/**
 * Given a card number, use sysfs to determine the DRI driver name.
 */
static const char *
_eglChooseDRMDriver(int card)
{
#if 0
   return _eglstrdup("libEGLdri");
#else
   char path[2000], driverName[2000];
   FILE *f;
   int length;

   snprintf(path, sizeof(path), "%s/drm/card%d/dri_library_name", SysFS, card);

   f = fopen(path, "r");
   if (!f)
      return NULL;

   fgets(driverName, sizeof(driverName), f);
   fclose(f);

   if ((length = strlen(driverName)) > 1) {
      /* remove the trailing newline from sysfs */
      driverName[length - 1] = '\0';
      strncat(driverName, "_dri", sizeof(driverName));
      return _eglstrdup(driverName);
   }
   else {
      return NULL;
   }   
#endif
}


/**
 * Determine/return the name of the driver to use for the given _EGLDisplay.
 *
 * Try to be clever and determine if nativeDisplay is an Xlib Display
 * ptr or a string (naming a driver or screen number, etc).
 *
 * If the first character is ':' we interpret it as a screen or card index
 * number (i.e. ":0" or ":1", etc)
 * Else if the first character is '!' we interpret it as specific driver name
 * (i.e. "!r200" or "!i830".
 *
 * Whatever follows ':' is copied and put into dpy->DriverArgs.
 *
 * The caller may free() the returned string.
 */
const char *
_eglChooseDriver(_EGLDisplay *dpy)
{
   const char *displayString = (const char *) dpy->NativeDisplay;
   const char *driverName = NULL;

   /* First, if the EGL_DRIVER env var is set, use that */
   driverName = getenv("EGL_DRIVER");
   if (driverName)
      return _eglstrdup(driverName);

#if 0
   if (!displayString) {
      /* choose a default */
      displayString = DefaultDriverName;
   }
#endif
   /* extract default DriverArgs = whatever follows ':' */
   if (displayString &&
       (displayString[0] == '!' ||
        displayString[0] == ':')) {
      const char *args = strchr(displayString, ':');
      if (args)
         dpy->DriverArgs = _eglstrdup(args + 1);
   }

   /* determine driver name now */
   if (displayString && displayString[0] == ':' &&
       (displayString[1] >= '0' && displayString[1] <= '9') &&
       !displayString[2]) {
      int card = atoi(displayString + 1);
      driverName = _eglChooseDRMDriver(card);
   }
   else if (displayString && displayString[0] == '!') {
      /* use user-specified driver name */
      driverName = _eglstrdup(displayString + 1);
      /* truncate driverName at ':' if present */
      {
         char *args = strchr(driverName, ':');
         if (args) {
            *args = 0;
         }
      }
   }
   else {
      /* NativeDisplay is not a string! */
#if defined(_EGL_PLATFORM_X)
      driverName = _xeglChooseDriver(dpy);
#elif defined(_EGL_PLATFORM_WINDOWS)
      /* XXX to do */
      driverName = _weglChooseDriver(dpy);
#elif defined(_EGL_PLATFORM_WINCE)
      /* XXX to do */
#endif
   }

   return driverName;
}


/**
 * Open/load the named driver and call its bootstrap function: _eglMain().
 * By the time this function is called, the dpy->DriverName should have
 * been determined.
 *
 * \return  new _EGLDriver object.
 */
_EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy, const char *driverName, const char *args)
{
   _EGLDriver *drv;
   _EGLMain_t mainFunc;
   void *lib;
   char driverFilename[1000];

   assert(driverName);

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

   drv = mainFunc(dpy, args);
   if (!drv) {
      dlclose(lib);
      return NULL;
   }
   /* with a recurvise open you want the inner most handle */
   if (!drv->LibHandle)
      drv->LibHandle = lib;
   else
      dlclose(lib);

   /* update the global notion of supported APIs */
   _eglGlobal.ClientAPIsMask |= drv->ClientAPIsMask;

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
