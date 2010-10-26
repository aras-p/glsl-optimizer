/**
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "eglstring.h"
#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "egllog.h"
#include "eglmutex.h"

#if defined(_EGL_OS_UNIX)
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif


typedef struct _egl_module {
   char *Path;
   void *Handle;
   _EGLDriver *Driver;
} _EGLModule;

static _EGL_DECLARE_MUTEX(_eglModuleMutex);
static _EGLArray *_eglModules;


/**
 * Wrappers for dlopen/dlclose()
 */
#if defined(_EGL_OS_WINDOWS)


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


#elif defined(_EGL_OS_UNIX)


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


#endif


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

#if defined(_EGL_OS_WINDOWS)
   /* XXX untested */
   if (lib)
      mainFunc = (_EGLMain_t) GetProcAddress(lib, "_eglMain");
#elif defined(_EGL_OS_UNIX)
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
 * Load a module and create the driver object.
 */
static EGLBoolean
_eglLoadModule(_EGLModule *mod)
{
   _EGLMain_t mainFunc;
   lib_handle lib;
   _EGLDriver *drv;

   mainFunc = _eglOpenLibrary(mod->Path, &lib);
   if (!mainFunc)
      return EGL_FALSE;

   drv = mainFunc(NULL);
   if (!drv) {
      if (lib)
         close_library(lib);
      return EGL_FALSE;
   }

   if (!drv->Name) {
      _eglLog(_EGL_WARNING, "Driver loaded from %s has no name", mod->Path);
      drv->Name = "UNNAMED";
   }

   mod->Handle = (void *) lib;
   mod->Driver = drv;

   return EGL_TRUE;
}


/**
 * Unload a module.
 */
static void
_eglUnloadModule(_EGLModule *mod)
{
   /* destroy the driver */
   if (mod->Driver && mod->Driver->Unload)
      mod->Driver->Unload(mod->Driver);
   if (mod->Handle)
      close_library(mod->Handle);

   mod->Driver = NULL;
   mod->Handle = NULL;
}


/**
 * Add a module to the module array.
 */
static _EGLModule *
_eglAddModule(const char *path)
{
   _EGLModule *mod;
   EGLint i;

   if (!_eglModules) {
      _eglModules = _eglCreateArray("Module", 8);
      if (!_eglModules)
         return NULL;
   }

   /* find duplicates */
   for (i = 0; i < _eglModules->Size; i++) {
      mod = _eglModules->Elements[i];
      if (strcmp(mod->Path, path) == 0)
         return mod;
   }

   /* allocate a new one */
   mod = calloc(1, sizeof(*mod));
   if (mod) {
      mod->Path = _eglstrdup(path);
      if (!mod->Path) {
         free(mod);
         mod = NULL;
      }
   }
   if (mod) {
      _eglAppendArray(_eglModules, (void *) mod);
      _eglLog(_EGL_DEBUG, "added %s to module array", mod->Path);
   }

   return mod;
}


/**
 * Free a module.
 */
static void
_eglFreeModule(void *module)
{
   _EGLModule *mod = (_EGLModule *) module;

   _eglUnloadModule(mod);
   free(mod->Path);
   free(mod);
}


/**
 * A loader function for use with _eglPreloadForEach.  The loader data is the
 * filename of the driver.   This function stops on the first valid driver.
 */
static EGLBoolean
_eglLoaderFile(const char *dir, size_t len, void *loader_data)
{
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

   if (library_suffix()) {
      const char *suffix = library_suffix();
      size_t slen = strlen(suffix);
      const char *p;
      EGLBoolean need_suffix;

      p = filename + flen - slen;
      need_suffix = (p < filename || strcmp(p, suffix) != 0);
      if (need_suffix) {
         /* overflow */
         if (len + slen + 1 > sizeof(path))
            return EGL_TRUE;
         strcpy(path + len, suffix);
      }
   }

#if defined(_EGL_OS_UNIX)
   /* check if the file exists */
   if (access(path, F_OK))
      return EGL_TRUE;
#endif

   _eglAddModule(path);

   return EGL_TRUE;
}


/**
 * A loader function for use with _eglPreloadForEach.  The loader data is the
 * pattern (prefix) of the files to look for.
 */
static EGLBoolean
_eglLoaderPattern(const char *dir, size_t len, void *loader_data)
{
#if defined(_EGL_OS_UNIX)
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

      /* make a full path and add it to the module array */
      if (len + dirent_len + 1 <= sizeof(path)) {
         strcpy(path + len, dirent->d_name);
         _eglAddModule(path);
      }
   }

   closedir(dirp);

   return EGL_TRUE;
#else /* _EGL_OS_UNIX */
   /* stop immediately */
   return EGL_FALSE;
#endif
}


/**
 * Run the callback function on each driver directory.
 *
 * The process may end prematurely if the callback function returns false.
 */
static void
_eglPreloadForEach(const char *search_path,
                   EGLBoolean (*loader)(const char *, size_t, void *),
                   void *loader_data)
{
   const char *cur, *next;
   size_t len;

   cur = search_path;
   while (cur) {
      next = strchr(cur, ':');
      len = (next) ? next - cur : strlen(cur);

      if (!loader(cur, len, loader_data))
         break;

      cur = (next) ? next + 1 : NULL;
   }
}


/**
 * Return a list of colon-separated driver directories.
 */
static const char *
_eglGetSearchPath(void)
{
   static const char *search_path;

#if defined(_EGL_OS_UNIX) || defined(_EGL_OS_WINDOWS)
   if (!search_path) {
      static char buffer[1024];
      const char *p;
      int ret;

      p = getenv("EGL_DRIVERS_PATH");
#if defined(_EGL_OS_UNIX)
      if (p && (geteuid() != getuid() || getegid() != getgid())) {
         _eglLog(_EGL_DEBUG,
               "ignore EGL_DRIVERS_PATH for setuid/setgid binaries");
         p = NULL;
      }
#endif /* _EGL_OS_UNIX */

      if (p) {
         ret = _eglsnprintf(buffer, sizeof(buffer),
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
 * Add the user driver to the module array.
 *
 * The user driver is specified by EGL_DRIVER.
 */
static void
_eglAddUserDriver(void)
{
   const char *search_path = _eglGetSearchPath();
   char *env;

   env = getenv("EGL_DRIVER");
#if defined(_EGL_OS_UNIX)
   if (env && strchr(env, '/')) {
      search_path = "";
      if ((geteuid() != getuid() || getegid() != getgid())) {
         _eglLog(_EGL_DEBUG,
               "ignore EGL_DRIVER for setuid/setgid binaries");
         env = NULL;
      }
   }
#endif /* _EGL_OS_UNIX */
   if (env)
      _eglPreloadForEach(search_path, _eglLoaderFile, (void *) env);
}


/**
 * Add default drivers to the module array.
 */
static void
_eglAddDefaultDrivers(void)
{
   const char *search_path = _eglGetSearchPath();
   EGLint i;
#if defined(_EGL_OS_WINDOWS)
   const char *DefaultDriverNames[] = {
      "egl_gallium"
   };
#elif defined(_EGL_OS_UNIX)
   const char *DefaultDriverNames[] = {
      "egl_gallium",
      "egl_dri2",
      "egl_glx"
   };
#endif

   for (i = 0; i < ARRAY_SIZE(DefaultDriverNames); i++) {
      void *name = (void *) DefaultDriverNames[i];
      _eglPreloadForEach(search_path, _eglLoaderFile, name);
   }
}


/**
 * Add drivers to the module array.  Drivers will be loaded as they are matched
 * to displays.
 */
static EGLBoolean
_eglAddDrivers(void)
{
   if (_eglModules)
      return EGL_TRUE;

   /* the order here decides the priorities of the drivers */
   _eglAddUserDriver();
   _eglAddDefaultDrivers();
   _eglPreloadForEach(_eglGetSearchPath(), _eglLoaderPattern, (void *) "egl_");

   return (_eglModules != NULL);
}


/**
 * Match a display to a driver.  The display is initialized unless use_probe is
 * true.
 *
 * The matching is done by finding the first driver that can initialize the
 * display, or when use_probe is true, the driver with highest score.
 */
_EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy, EGLBoolean use_probe)
{
   _EGLModule *mod;
   _EGLDriver *best_drv = NULL;
   EGLint best_score = 0;
   EGLint major, minor, i;

   _eglLockMutex(&_eglModuleMutex);

   if (!_eglAddDrivers()) {
      _eglUnlockMutex(&_eglModuleMutex);
      return EGL_FALSE;
   }

   /* match the loaded modules */
   for (i = 0; i < _eglModules->Size; i++) {
      mod = (_EGLModule *) _eglModules->Elements[i];
      if (!mod->Driver)
         break;

      if (use_probe) {
         EGLint score = (mod->Driver->Probe) ?
            mod->Driver->Probe(mod->Driver, dpy) : 1;
         if (score > best_score) {
            best_drv = mod->Driver;
            best_score = score;
         }
      }
      else {
         if (mod->Driver->API.Initialize(mod->Driver, dpy, &major, &minor)) {
            best_drv = mod->Driver;
            best_score = 100;
         }
      }
      /* perfect match */
      if (best_score >= 100)
         break;
   }

   /* load more modules */
   if (!best_drv) {
      EGLint first_unloaded = i;

      while (i < _eglModules->Size) {
         mod = (_EGLModule *) _eglModules->Elements[i];
         assert(!mod->Driver);

         if (!_eglLoadModule(mod)) {
            /* remove invalid modules */
            _eglEraseArray(_eglModules, i, _eglFreeModule);
            continue;
         }

         if (use_probe) {
            best_score = (mod->Driver->Probe) ?
               mod->Driver->Probe(mod->Driver, dpy) : 1;
         }
         else {
            if (mod->Driver->API.Initialize(mod->Driver, dpy, &major, &minor))
               best_score = 100;
         }

         if (best_score > 0) {
            best_drv = mod->Driver;
            /* loaded modules come before unloaded ones */
            if (first_unloaded != i) {
               void *tmp = _eglModules->Elements[i];
               _eglModules->Elements[i] =
                  _eglModules->Elements[first_unloaded];
               _eglModules->Elements[first_unloaded] = tmp;
            }
            break;
         }
         else {
            _eglUnloadModule(mod);
            i++;
         }
      }
   }

   _eglUnlockMutex(&_eglModuleMutex);

   if (best_drv) {
      _eglLog(_EGL_DEBUG, "the best driver is %s (score %d)",
            best_drv->Name, best_score);
      if (!use_probe) {
         dpy->Driver = best_drv;
         dpy->Initialized = EGL_TRUE;
         dpy->APImajor = major;
         dpy->APIminor = minor;
      }
   }

   return best_drv;
}


__eglMustCastToProperFunctionPointerType
_eglGetDriverProc(const char *procname)
{
   EGLint i;
   _EGLProc proc = NULL;

   if (!_eglModules) {
      /* load the driver for the default display */
      EGLDisplay egldpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      _EGLDisplay *dpy = _eglLookupDisplay(egldpy);
      if (!dpy || !_eglMatchDriver(dpy, EGL_TRUE))
         return NULL;
   }

   for (i = 0; i < _eglModules->Size; i++) {
      _EGLModule *mod = (_EGLModule *) _eglModules->Elements[i];

      if (!mod->Driver)
         break;
      proc = mod->Driver->API.GetProcAddress(mod->Driver, procname);
      if (proc)
         break;
   }

   return proc;
}


/**
 * Unload all drivers.
 */
void
_eglUnloadDrivers(void)
{
   /* this is called at atexit time */
   if (_eglModules) {
#if defined(_EGL_OS_UNIX)
      _eglDestroyArray(_eglModules, _eglFreeModule);
#elif defined(_EGL_OS_WINDOWS)
      /* XXX Windows unloads DLLs before atexit */
      _eglDestroyArray(_eglModules, NULL);
#endif
      _eglModules = NULL;
   }
}


/**
 * Invoke a callback function on each EGL search path.
 *
 * The first argument of the callback function is the name of the search path.
 * The second argument is the length of the name.
 */
void
_eglSearchPathForEach(EGLBoolean (*callback)(const char *, size_t, void *),
                      void *callback_data)
{
   const char *search_path = _eglGetSearchPath();
   _eglPreloadForEach(search_path, callback, callback_data);
}
