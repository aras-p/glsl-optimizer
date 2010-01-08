/**
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
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
#include <dlfcn.h>
#endif


/**
 * Wrappers for dlopen/dlclose()
 */
#if defined(_EGL_PLATFORM_WINDOWS)


/* XXX Need to decide how to do dynamic name lookup on Windows */
static const char DefaultDriverName[] = "TBD";

typedef HMODULE lib_handle;

static HMODULE
open_library(const char *filename)
{
   return LoadLibrary(filename);
}

static void
close_library(HMODULE lib)
{
   FreeLibrary(lib);
}


#elif defined(_EGL_PLATFORM_X)


static const char DefaultDriverName[] = "egl_softpipe";

typedef void * lib_handle;

static void *
open_library(const char *filename)
{
   return dlopen(filename, RTLD_LAZY);
}

static void
close_library(void *lib)
{
   dlclose(lib);
}

#else /* _EGL_PLATFORM_NO_OS */

static const char DefaultDriverName[] = "builtin";

typedef void *lib_handle;

static INLINE void *
open_library(const char *filename)
{
   return (void *) filename;
}

static INLINE void
close_library(void *lib)
{
}


#endif


/**
 * Choose a driver for a given display.
 * The caller may free() the returned strings.
 */
static char *
_eglChooseDriver(_EGLDisplay *dpy, char **argsRet)
{
   char *path = NULL;
   const char *args = NULL;
   const char *suffix = NULL;
   const char *p;

   path = getenv("EGL_DRIVER");
   if (path)
      path = _eglstrdup(path);

#if defined(_EGL_PLATFORM_X)
   if (!path && dpy && dpy->NativeDisplay) {
      /* assume (wrongly!) that the native display is a display string */
      path = _eglSplitDisplayString((const char *) dpy->NativeDisplay, &args);
   }
   suffix = "so";
#elif defined(_EGL_PLATFORM_WINDOWS)
   suffix = "dll";
#else /* _EGL_PLATFORM_NO_OS */
   if (path) {
      /* force the use of the default driver */
      _eglLog(_EGL_DEBUG, "ignore EGL_DRIVER");
      free(path);
      path = NULL;
   }
   suffix = NULL;
#endif

   if (!path)
      path = _eglstrdup(DefaultDriverName);

   /* append suffix if there isn't */
   p = strrchr(path, '.');
   if (!p && suffix) {
      size_t len = strlen(path);
      char *tmp = malloc(len + strlen(suffix) + 2);
      if (tmp) {
         memcpy(tmp, path, len);
         tmp[len++] = '.';
         tmp[len] = '\0';
         strcat(tmp + len, suffix);

         free(path);
         path = tmp;
      }
   }

   if (argsRet)
      *argsRet = (args) ? _eglstrdup(args) : NULL;

   return path;
}


/**
 * Open the named driver and find its bootstrap function: _eglMain().
 */
static _EGLMain_t
_eglOpenLibrary(const char *driverPath, lib_handle *handle)
{
   lib_handle lib;
   _EGLMain_t mainFunc = NULL;
   const char *error = "unknown error";

   assert(driverPath);

   _eglLog(_EGL_DEBUG, "dlopen(%s)", driverPath);
   lib = open_library(driverPath);

#if defined(_EGL_PLATFORM_WINDOWS)
   /* XXX untested */
   if (lib)
      mainFunc = (_EGLMain_t) GetProcAddress(lib, "_eglMain");
#elif defined(_EGL_PLATFORM_X)
   if (lib) {
      mainFunc = (_EGLMain_t) dlsym(lib, "_eglMain");
      if (!mainFunc)
         error = dlerror();
   }
   else {
      error = dlerror();
   }
#else /* _EGL_PLATFORM_NO_OS */
   /* must be the default driver name */
   if (strcmp(driverPath, DefaultDriverName) == 0)
      mainFunc = (_EGLMain_t) _eglMain;
   else
      error = "not builtin driver";
#endif

   if (!lib) {
      _eglLog(_EGL_WARNING, "Could not open driver %s (%s)",
              driverPath, error);
      if (!getenv("EGL_DRIVER"))
         _eglLog(_EGL_WARNING,
                 "The driver can be overridden by setting EGL_DRIVER");
      return NULL;
   }

   if (!mainFunc) {
      _eglLog(_EGL_WARNING, "_eglMain not found in %s (%s)",
              driverPath, error);
      if (lib)
         close_library(lib);
      return NULL;
   }

   *handle = lib;
   return mainFunc;
}


/**
 * Load the named driver.  The path and args passed will be
 * owned by the driver and freed.
 */
static _EGLDriver *
_eglLoadDriver(char *path, char *args)
{
   _EGLMain_t mainFunc;
   lib_handle lib;
   _EGLDriver *drv = NULL;

   mainFunc = _eglOpenLibrary(path, &lib);
   if (!mainFunc)
      return NULL;

   drv = mainFunc(args);
   if (!drv) {
      if (lib)
         close_library(lib);
      return NULL;
   }

   if (!drv->Name) {
      _eglLog(_EGL_WARNING, "Driver loaded from %s has no name", path);
      drv->Name = "UNNAMED";
   }

   drv->Path = path;
   drv->Args = args;
   drv->LibHandle = lib;

   return drv;
}


/**
 * Match a display to a preloaded driver.
 */
static _EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy)
{
   _EGLDriver *defaultDriver = NULL;
   EGLint i;

   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLDriver *drv = _eglGlobal.Drivers[i];

      /* display specifies a driver */
      if (dpy->DriverName) {
         if (strcmp(dpy->DriverName, drv->Name) == 0)
            return drv;
      }
      else if (drv->Probe) {
         if (drv->Probe(drv, dpy))
            return drv;
      }
      else {
         if (!defaultDriver)
            defaultDriver = drv;
      }
   }

   return defaultDriver;
}


/**
 * Load a driver and save it.
 */
const char *
_eglPreloadDriver(_EGLDisplay *dpy)
{
   char *path, *args;
   _EGLDriver *drv;
   EGLint i;

   path = _eglChooseDriver(dpy, &args);
   if (!path)
      return NULL;

   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      drv = _eglGlobal.Drivers[i];
      if (strcmp(drv->Path, path) == 0) {
         _eglLog(_EGL_DEBUG, "Driver %s is already preloaded",
                 drv->Name);
         free(path);
         if (args)
            free(args);
         return drv->Name;
      }
   }

   drv = _eglLoadDriver(path, args);
   if (!drv)
      return NULL;

   _eglGlobal.Drivers[_eglGlobal.NumDrivers++] = drv;

   return drv->Name;
}


/**
 * Open a preloaded driver.
 */
_EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy)
{
   _EGLDriver *drv = _eglMatchDriver(dpy);
   return drv;
}


/**
 * Close a preloaded driver.
 */
EGLBoolean
_eglCloseDriver(_EGLDriver *drv, _EGLDisplay *dpy)
{
   return EGL_TRUE;
}


/**
 * Unload preloaded drivers.
 */
void
_eglUnloadDrivers(void)
{
   EGLint i;
   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLDriver *drv = _eglGlobal.Drivers[i];
      lib_handle handle = drv->LibHandle;

      if (drv->Path)
         free((char *) drv->Path);
      if (drv->Args)
         free((char *) drv->Args);

      /* destroy driver */
      if (drv->Unload)
         drv->Unload(drv);

      if (handle)
         close_library(handle);
      _eglGlobal.Drivers[i] = NULL;
   }

   _eglGlobal.NumDrivers = 0;
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
   drv->API.WaitClient = _eglWaitClient;
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
 * Try to determine which EGL APIs (OpenGL, OpenGL ES, OpenVG, etc)
 * are supported on the system by looking for standard library names.
 */
EGLint
_eglFindAPIs(void)
{
   EGLint mask = 0x0;
   lib_handle lib;
#if defined(_EGL_PLATFORM_WINDOWS)
   /* XXX not sure about these names */
   const char *es1_libname = "libGLESv1_CM.dll";
   const char *es2_libname = "libGLESv2.dll";
   const char *gl_libname = "OpenGL32.dll";
   const char *vg_libname = "libOpenVG.dll";
#elif defined(_EGL_PLATFORM_X)
   const char *es1_libname = "libGLESv1_CM.so";
   const char *es2_libname = "libGLESv2.so";
   const char *gl_libname = "libGL.so";
   const char *vg_libname = "libOpenVG.so";
#else /* _EGL_PLATFORM_NO_OS */
   const char *es1_libname = NULL;
   const char *es2_libname = NULL;
   const char *gl_libname = NULL;
   const char *vg_libname = NULL;
#endif

   if ((lib = open_library(es1_libname))) {
      close_library(lib);
      mask |= EGL_OPENGL_ES_BIT;
   }

   if ((lib = open_library(es2_libname))) {
      close_library(lib);
      mask |= EGL_OPENGL_ES2_BIT;
   }

   if ((lib = open_library(gl_libname))) {
      close_library(lib);
      mask |= EGL_OPENGL_BIT;
   }

   if ((lib = open_library(vg_libname))) {
      close_library(lib);
      mask |= EGL_OPENVG_BIT;
   }

   return mask;
}
