/**
 * Functions related to EGLDisplay.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "eglsurface.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglstring.h"
#include "eglmutex.h"
#include "egllog.h"


/**
 * Finish display management.
 */
void
_eglFiniDisplay(void)
{
   _EGLDisplay *dpyList, *dpy;

   /* atexit function is called with global mutex locked */
   dpyList = _eglGlobal.DisplayList;
   while (dpyList) {
      /* pop list head */
      dpy = dpyList;
      dpyList = dpyList->Next;

      if (dpy->ContextList || dpy->SurfaceList)
         _eglLog(_EGL_DEBUG, "Display %p is destroyed with resources", dpy);

      free(dpy);
   }
   _eglGlobal.DisplayList = NULL;
}


/**
 * Allocate a new _EGLDisplay object for the given nativeDisplay handle.
 * We'll also try to determine the device driver name at this time.
 *
 * Note that nativeDisplay may be an X Display ptr, or a string.
 */
_EGLDisplay *
_eglNewDisplay(NativeDisplayType nativeDisplay)
{
   _EGLDisplay *dpy = (_EGLDisplay *) calloc(1, sizeof(_EGLDisplay));
   if (dpy) {
      dpy->NativeDisplay = nativeDisplay;
   }
   return dpy;
}


/**
 * Link a display to itself and return the handle of the link.
 * The handle can be passed to client directly.
 */
EGLDisplay
_eglLinkDisplay(_EGLDisplay *dpy)
{
   _eglLockMutex(_eglGlobal.Mutex);

   dpy->Next = _eglGlobal.DisplayList;
   _eglGlobal.DisplayList = dpy;

   _eglUnlockMutex(_eglGlobal.Mutex);

   return (EGLDisplay) dpy;
}


/**
 * Unlink a linked display from itself.
 * Accessing an unlinked display should generate EGL_BAD_DISPLAY error.
 */
void
_eglUnlinkDisplay(_EGLDisplay *dpy)
{
   _EGLDisplay *prev;

   _eglLockMutex(_eglGlobal.Mutex);

   prev = _eglGlobal.DisplayList;
   if (prev != dpy) {
      while (prev) {
         if (prev->Next == dpy)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = dpy->Next;
   }
   else {
      _eglGlobal.DisplayList = dpy->Next;
   }

   _eglUnlockMutex(_eglGlobal.Mutex);
}


/**
 * Find the display corresponding to the specified native display id in all
 * linked displays.
 */
_EGLDisplay *
_eglFindDisplay(NativeDisplayType nativeDisplay)
{
   _EGLDisplay *dpy;

   _eglLockMutex(_eglGlobal.Mutex);

   dpy = _eglGlobal.DisplayList;
   while (dpy) {
      if (dpy->NativeDisplay == nativeDisplay) {
         _eglUnlockMutex(_eglGlobal.Mutex);
         return dpy;
      }
      dpy = dpy->Next;
   }

   _eglUnlockMutex(_eglGlobal.Mutex);

   return NULL;
}


/**
 * Destroy the contexts and surfaces that are linked to the display.
 */
void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *display)
{
   _EGLContext *contexts;
   _EGLSurface *surfaces;

   contexts = display->ContextList;
   surfaces = display->SurfaceList;

   while (contexts) {
      _EGLContext *ctx = contexts;
      contexts = contexts->Next;

      _eglUnlinkContext(ctx);
      drv->API.DestroyContext(drv, display, ctx);
   }
   assert(!display->ContextList);

   while (surfaces) {
      _EGLSurface *surf = surfaces;
      surfaces = surfaces->Next;

      _eglUnlinkSurface(surf);
      drv->API.DestroySurface(drv, display, surf);
   }
   assert(!display->SurfaceList);
}


/**
 * Free all the data hanging of an _EGLDisplay object, but not
 * the object itself.
 */
void
_eglCleanupDisplay(_EGLDisplay *disp)
{
   EGLint i;

   if (disp->Configs) {
      for (i = 0; i < disp->NumConfigs; i++)
         free(disp->Configs[i]);
      free(disp->Configs);
      disp->Configs = NULL;
      disp->NumConfigs = 0;
   }

   /* XXX incomplete */
}


/**
 * Link a surface to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
EGLSurface
_eglLinkSurface(_EGLSurface *surf, _EGLDisplay *dpy)
{
   surf->Display = dpy;
   surf->Next = dpy->SurfaceList;
   dpy->SurfaceList = surf;
   return (EGLSurface) surf;
}


/**
 * Unlink a linked surface from its display.
 * Accessing an unlinked surface should generate EGL_BAD_SURFACE error.
 */
void
_eglUnlinkSurface(_EGLSurface *surf)
{
   _EGLSurface *prev;

   prev = surf->Display->SurfaceList;
   if (prev != surf) {
      while (prev) {
         if (prev->Next == surf)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = surf->Next;
   }
   else {
      prev = NULL;
      surf->Display->SurfaceList = surf->Next;
   }

   surf->Next = NULL;
   surf->Display = NULL;
}


#ifndef _EGL_SKIP_HANDLE_CHECK


/**
 * Return EGL_TRUE if the given handle is a valid handle to a display.
 */
EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy)
{
   _EGLDisplay *cur;

   _eglLockMutex(_eglGlobal.Mutex);
   cur = _eglGlobal.DisplayList;
   while (cur) {
      if (cur == (_EGLDisplay *) dpy)
         break;
      cur = cur->Next;
   }
   _eglUnlockMutex(_eglGlobal.Mutex);
   return (cur != NULL);
}


/**
 * Return EGL_TRUE if the given handle is a valid handle to a surface.
 */
EGLBoolean
_eglCheckSurfaceHandle(EGLSurface surf, _EGLDisplay *dpy)
{
   _EGLSurface *cur = NULL;

   if (dpy)
      cur = dpy->SurfaceList;
   while (cur) {
      if (cur == (_EGLSurface *) surf) {
         assert(cur->Display == dpy);
         break;
      }
      cur = cur->Next;
   }
   return (cur != NULL);
}


#endif /* !_EGL_SKIP_HANDLE_CHECK */
