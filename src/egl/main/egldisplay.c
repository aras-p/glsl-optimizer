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
      EGLint i;

      /* pop list head */
      dpy = dpyList;
      dpyList = dpyList->Next;

      for (i = 0; i < _EGL_NUM_RESOURCES; i++) {
         if (dpy->ResourceLists[i]) {
            _eglLog(_EGL_DEBUG, "Display %p is destroyed with resources", dpy);
            break;
         }
      }

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
_eglNewDisplay(EGLNativeDisplayType nativeDisplay)
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
_eglFindDisplay(EGLNativeDisplayType nativeDisplay)
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
   _EGLResource *list;

   list = display->ResourceLists[_EGL_RESOURCE_CONTEXT];
   while (list) {
      _EGLContext *ctx = (_EGLContext *) list;
      list = list->Next;

      _eglUnlinkContext(ctx);
      drv->API.DestroyContext(drv, display, ctx);
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_CONTEXT]);

   list = display->ResourceLists[_EGL_RESOURCE_SURFACE];
   while (list) {
      _EGLSurface *surf = (_EGLSurface *) list;
      list = list->Next;

      _eglUnlinkSurface(surf);
      drv->API.DestroySurface(drv, display, surf);
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_SURFACE]);
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
 * Return EGL_TRUE if the given resource is valid.  That is, the display does
 * own the resource.
 */
EGLBoolean
_eglCheckResource(void *res, _EGLResourceType type, _EGLDisplay *dpy)
{
   _EGLResource *list = dpy->ResourceLists[type];
   
   if (!res)
      return EGL_FALSE;

   while (list) {
      if (res == (void *) list) {
         assert(list->Display == dpy);
         break;
      }
      list = list->Next;
   }

   return (list != NULL);
}


#endif /* !_EGL_SKIP_HANDLE_CHECK */


/**
 * Link a resource to a display.
 */
void
_eglLinkResource(_EGLResource *res, _EGLResourceType type, _EGLDisplay *dpy)
{
   assert(!res->Display || res->Display == dpy);

   res->Display = dpy;
   res->IsLinked = EGL_TRUE;
   res->Next = dpy->ResourceLists[type];
   dpy->ResourceLists[type] = res;
}


/**
 * Unlink a linked resource from its display.
 */
void
_eglUnlinkResource(_EGLResource *res, _EGLResourceType type)
{
   _EGLResource *prev;

   prev = res->Display->ResourceLists[type];
   if (prev != res) {
      while (prev) {
         if (prev->Next == res)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = res->Next;
   }
   else {
      res->Display->ResourceLists[type] = res->Next;
   }

   res->Next = NULL;
   /* do not reset res->Display */
   res->IsLinked = EGL_FALSE;
}
