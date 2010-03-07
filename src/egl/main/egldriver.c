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
#include "eglimage.h"

#if defined(_EGL_PLATFORM_POSIX)
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
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


static const char *
library_suffix(void)
{
   return ".dll";
}


#elif defined(_EGL_PLATFORM_POSIX)


static const char DefaultDriverName[] = "egl_glx";

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


static const char *
library_suffix(void)
{
   return ".so";
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


static const char *
library_suffix(void)
{
   return NULL;
}


#endif


#define NUM_PROBE_CACHE_SLOTS 8
static struct {
   EGLint keys[NUM_PROBE_CACHE_SLOTS];
   const void *values[NUM_PROBE_CACHE_SLOTS];
} _eglProbeCache;


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
#elif defined(_EGL_PLATFORM_POSIX)
   if (lib) {
      union {
         _EGLMain_t func;
         void *ptr;
      } tmp = { NULL };
      /* direct cast gives a warning when compiled with -pedantic */
      tmp.ptr = dlsym(lib, "_eglMain");
      mainFunc = tmp.func;
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
 * Load the named driver.
 */
static _EGLDriver *
_eglLoadDriver(const char *path, const char *args)
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

   drv->Path = _eglstrdup(path);
   drv->Args = (args) ? _eglstrdup(args) : NULL;
   if (!drv->Path || (args && !drv->Args)) {
      if (drv->Path)
         free((char *) drv->Path);
      if (drv->Args)
         free((char *) drv->Args);
      drv->Unload(drv);
      if (lib)
         close_library(lib);
      return NULL;
   }

   drv->LibHandle = lib;

   return drv;
}


/**
 * Match a display to a preloaded driver.
 *
 * The matching is done by finding the driver with the highest score.
 */
_EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy)
{
   _EGLDriver *best_drv = NULL;
   EGLint best_score = -1, i;

   /*
    * this function is called after preloading and the drivers never change
    * after preloading.
    */
   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLDriver *drv = _eglGlobal.Drivers[i];
      EGLint score;

      score = (drv->Probe) ? drv->Probe(drv, dpy) : 0;
      if (score > best_score) {
         if (best_drv) {
            _eglLog(_EGL_DEBUG, "driver %s has higher score than %s",
                  drv->Name, best_drv->Name);
         }

         best_drv = drv;
         best_score = score;
         /* perfect match */
         if (score >= 100)
            break;
      }
   }

   return best_drv;
}


/**
 * A loader function for use with _eglPreloadForEach.  The loader data is the
 * filename of the driver.   This function stops on the first valid driver.
 */
static EGLBoolean
_eglLoaderFile(const char *dir, size_t len, void *loader_data)
{
   _EGLDriver *drv;
   char path[1024];
   const char *filename = (const char *) loader_data;
   size_t flen = strlen(filename);

   /* make a full path */
   if (len + flen + 2 > sizeof(path))
      return EGL_TRUE;
   if (len) {
      memcpy(path, dir, len);
      path[len++] = '/';
   }
   memcpy(path + len, filename, flen);
   len += flen;
   path[len] = '\0';

   drv = _eglLoadDriver(path, NULL);
   /* fix the path and load again */
   if (!drv && library_suffix()) {
      const char *suffix = library_suffix();
      size_t slen = strlen(suffix);
      const char *p;
      EGLBoolean need_suffix;

      p = filename + flen - slen;
      need_suffix = (p < filename || strcmp(p, suffix) != 0);
      if (need_suffix && len + slen + 1 <= sizeof(path)) {
         strcpy(path + len, suffix);
         drv = _eglLoadDriver(path, NULL);
      }
   }
   if (!drv)
      return EGL_TRUE;

   /* remember the driver and stop */
   _eglGlobal.Drivers[_eglGlobal.NumDrivers++] = drv;
   return EGL_FALSE;
}


/**
 * A loader function for use with _eglPreloadForEach.  The loader data is the
 * pattern (prefix) of the files to look for.
 */
static EGLBoolean
_eglLoaderPattern(const char *dir, size_t len, void *loader_data)
{
#if defined(_EGL_PLATFORM_POSIX)
   const char *prefix, *suffix;
   size_t prefix_len, suffix_len;
   DIR *dirp;
   struct dirent *dirent;
   char path[1024];

   if (len + 2 > sizeof(path))
      return EGL_TRUE;
   if (len) {
      memcpy(path, dir, len);
      path[len++] = '/';
   }
   path[len] = '\0';

   dirp = opendir(path);
   if (!dirp)
      return EGL_TRUE;

   prefix = (const char *) loader_data;
   prefix_len = strlen(prefix);
   suffix = library_suffix();
   suffix_len = (suffix) ? strlen(suffix) : 0;

   while ((dirent = readdir(dirp))) {
      _EGLDriver *drv;
      size_t dirent_len = strlen(dirent->d_name);
      const char *p;

      /* match the prefix */
      if (strncmp(dirent->d_name, prefix, prefix_len) != 0)
         continue;
      /* match the suffix */
      if (suffix) {
         p = dirent->d_name + dirent_len - suffix_len;
         if (p < dirent->d_name || strcmp(p, suffix) != 0)
            continue;
      }

      /* make a full path and load the driver */
      if (len + dirent_len + 1 <= sizeof(path)) {
         strcpy(path + len, dirent->d_name);
         drv = _eglLoadDriver(path, NULL);
         if (drv)
            _eglGlobal.Drivers[_eglGlobal.NumDrivers++] = drv;
      }
   }

   closedir(dirp);

   return EGL_TRUE;
#else /* _EGL_PLATFORM_POSIX */
   /* stop immediately */
   return EGL_FALSE;
#endif
}


/**
 * Run the preload function on each driver directory and return the number of
 * drivers loaded.
 *
 * The process may end prematurely if the callback function returns false.
 */
static EGLint
_eglPreloadForEach(const char *search_path,
                   EGLBoolean (*loader)(const char *, size_t, void *),
                   void *loader_data)
{
   const char *cur, *next;
   size_t len;
   EGLint num_drivers = _eglGlobal.NumDrivers;

   cur = search_path;
   while (cur) {
      next = strchr(cur, ':');
      len = (next) ? next - cur : strlen(cur);

      if (!loader(cur, len, loader_data))
         break;

      cur = (next) ? next + 1 : NULL;
   }

   return (_eglGlobal.NumDrivers - num_drivers);
}


/**
 * Return a list of colon-separated driver directories.
 */
static const char *
_eglGetSearchPath(void)
{
   static const char *search_path;

#if defined(_EGL_PLATFORM_POSIX) || defined(_EGL_PLATFORM_WINDOWS)
   if (!search_path) {
      static char buffer[1024];
      const char *p;
      int ret;

      p = getenv("EGL_DRIVERS_PATH");
#if defined(_EGL_PLATFORM_POSIX)
      if (p && (geteuid() != getuid() || getegid() != getgid())) {
         _eglLog(_EGL_DEBUG,
               "ignore EGL_DRIVERS_PATH for setuid/setgid binaries");
         p = NULL;
      }
#endif /* _EGL_PLATFORM_POSIX */

      if (p) {
         ret = snprintf(buffer, sizeof(buffer),
               "%s:%s", p, _EGL_DRIVER_SEARCH_DIR);
         if (ret > 0 && ret < sizeof(buffer))
            search_path = buffer;
      }
   }
   if (!search_path)
      search_path = _EGL_DRIVER_SEARCH_DIR;
#else
   search_path = "";
#endif

   return search_path;
}


/**
 * Preload a user driver.
 *
 * A user driver can be specified by EGL_DRIVER.
 */
static EGLBoolean
_eglPreloadUserDriver(void)
{
   const char *search_path = _eglGetSearchPath();
   char *env;

   env = getenv("EGL_DRIVER");
#if defined(_EGL_PLATFORM_POSIX)
   if (env && strchr(env, '/')) {
      search_path = "";
      if ((geteuid() != getuid() || getegid() != getgid())) {
         _eglLog(_EGL_DEBUG,
               "ignore EGL_DRIVER for setuid/setgid binaries");
         env = NULL;
      }
   }
#endif /* _EGL_PLATFORM_POSIX */
   if (!env)
      return EGL_FALSE;

   if (!_eglPreloadForEach(search_path, _eglLoaderFile, (void *) env)) {
      _eglLog(_EGL_WARNING, "EGL_DRIVER is set to an invalid driver");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}


/**
 * Preload display drivers.
 *
 * Display drivers are a set of drivers that support a certain display system.
 * The display system may be specified by EGL_DISPLAY.
 *
 * FIXME This makes libEGL a memory hog if an user driver is not specified and
 * there are many display drivers.
 */
static EGLBoolean
_eglPreloadDisplayDrivers(void)
{
   const char *dpy;
   char prefix[32];
   int ret;

   dpy = getenv("EGL_DISPLAY");
   if (!dpy || !dpy[0])
      dpy = _EGL_DEFAULT_DISPLAY;
   if (!dpy || !dpy[0])
      return EGL_FALSE;

   ret = snprintf(prefix, sizeof(prefix), "egl_%s_", dpy);
   if (ret < 0 || ret >= sizeof(prefix))
      return EGL_FALSE;

   return (_eglPreloadForEach(_eglGetSearchPath(),
            _eglLoaderPattern, (void *) prefix) > 0);
}


/**
 * Preload the default driver.
 */
static EGLBoolean
_eglPreloadDefaultDriver(void)
{
   return (_eglPreloadForEach(_eglGetSearchPath(),
            _eglLoaderFile, (void *) DefaultDriverName) > 0);
}


/**
 * Preload drivers.
 *
 * This function loads the driver modules and creates the corresponding
 * _EGLDriver objects.
 */
EGLBoolean
_eglPreloadDrivers(void)
{
   EGLBoolean loaded;

   /* protect the preloading process */
   _eglLockMutex(_eglGlobal.Mutex);

   /* already preloaded */
   if (_eglGlobal.NumDrivers) {
      _eglUnlockMutex(_eglGlobal.Mutex);
      return EGL_TRUE;
   }

   loaded = (_eglPreloadUserDriver() ||
             _eglPreloadDisplayDrivers() ||
             _eglPreloadDefaultDriver());

   _eglUnlockMutex(_eglGlobal.Mutex);

   return loaded;
}


/**
 * Unload preloaded drivers.
 */
void
_eglUnloadDrivers(void)
{
   EGLint i;

   /* this is called at atexit time */
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

#ifdef EGL_KHR_image_base
   drv->API.CreateImageKHR = _eglCreateImageKHR;
   drv->API.DestroyImageKHR = _eglDestroyImageKHR;
#endif /* EGL_KHR_image_base */
}


/**
 * Set the probe cache at the given key.
 *
 * A key, instead of a _EGLDriver, is used to allow the probe cache to be share
 * by multiple drivers.
 */
void
_eglSetProbeCache(EGLint key, const void *val)
{
   EGLint idx;

   for (idx = 0; idx < NUM_PROBE_CACHE_SLOTS; idx++) {
      if (!_eglProbeCache.keys[idx] || _eglProbeCache.keys[idx] == key)
         break;
   }
   assert(key > 0);
   assert(idx < NUM_PROBE_CACHE_SLOTS);

   _eglProbeCache.keys[idx] = key;
   _eglProbeCache.values[idx] = val;
}


/**
 * Return the probe cache at the given key.
 */
const void *
_eglGetProbeCache(EGLint key)
{
   EGLint idx;

   for (idx = 0; idx < NUM_PROBE_CACHE_SLOTS; idx++) {
      if (!_eglProbeCache.keys[idx] || _eglProbeCache.keys[idx] == key)
         break;
   }

   return (idx < NUM_PROBE_CACHE_SLOTS && _eglProbeCache.keys[idx] == key) ?
      _eglProbeCache.values[idx] : NULL;
}
