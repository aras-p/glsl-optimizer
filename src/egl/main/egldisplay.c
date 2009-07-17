
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
#include "eglhash.h"
#include "eglstring.h"


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
#if defined(_EGL_PLATFORM_X)
      dpy->Xdpy = (Display *) nativeDisplay;
#endif

      dpy->DriverName = _eglChooseDriver(dpy);
      if (!dpy->DriverName) {
         free(dpy);
         return NULL;
      }
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
   EGLuint key;
   key = _eglHashGenKey(_eglGlobal.Displays);
   assert(key);
   /* "link" the display to the hash table */
   _eglHashInsert(_eglGlobal.Displays, key, dpy);
   dpy->Handle = (EGLDisplay) key;

   return dpy->Handle;
}


/**
 * Unlink a linked display from itself.
 * Accessing an unlinked display should generate EGL_BAD_DISPLAY error.
 */
void
_eglUnlinkDisplay(_EGLDisplay *dpy)
{
   _eglHashRemove(_eglGlobal.Displays, (EGLuint) dpy->Handle);
   dpy->Handle = EGL_NO_DISPLAY;
}


/**
 * Return the handle of a linked display, or EGL_NO_DISPLAY.
 */
EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *display)
{
   if (display)
      return display->Handle;
   else
      return EGL_NO_DISPLAY;
}

 
/**
 * Lookup a handle to find the linked display.
 * Return NULL if the handle has no corresponding linked display.
 */
_EGLDisplay *
_eglLookupDisplay(EGLDisplay dpy)
{
   EGLuint key = (EGLuint) dpy;
   return (_EGLDisplay *) _eglHashLookup(_eglGlobal.Displays, key);
}


/**
 * Find the display corresponding to the specified native display id in all
 * linked displays.
 */
_EGLDisplay *
_eglFindDisplay(NativeDisplayType nativeDisplay)
{
   EGLuint key = _eglHashFirstEntry(_eglGlobal.Displays);

   /* Walk the hash table.  Should switch to list if it is a problem. */
   while (key) {
      _EGLDisplay *dpy = (_EGLDisplay *)
            _eglHashLookup(_eglGlobal.Displays, key);
      assert(dpy);

      if (dpy->NativeDisplay == nativeDisplay)
         return dpy;
      key = _eglHashNextEntry(_eglGlobal.Displays, key);
   }

   return NULL;
}


/**
 * Destroy the contexts and surfaces that are linked to the display.
 */
void
_eglReleaseDisplayResources(_EGLDriver *drv, EGLDisplay dpy)
{
   _EGLDisplay *display;
   _EGLContext *contexts;
   _EGLSurface *surfaces;

   display = _eglLookupDisplay(dpy);
   if (!display)
      return;
   contexts = display->ContextList;
   surfaces = display->SurfaceList;

   while (contexts) {
      EGLContext handle = _eglGetContextHandle(contexts);
      contexts = contexts->Next;
      drv->API.DestroyContext(drv, dpy, handle);
   }
   assert(!display->ContextList);

   while (surfaces) {
      EGLSurface handle = _eglGetSurfaceHandle(surfaces);
      surfaces = surfaces->Next;
      drv->API.DestroySurface(drv, dpy, handle);
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

   for (i = 0; i < disp->NumConfigs; i++) {
      free(disp->Configs[i]);
   }
   free(disp->Configs);
   disp->Configs = NULL;

   /* XXX incomplete */

   free((void *) disp->DriverName);
   disp->DriverName = NULL;

   /* driver deletes the _EGLDisplay object */
}


/**
 * Link a context to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
EGLContext
_eglLinkContext(_EGLContext *ctx, _EGLDisplay *dpy)
{
   ctx->Display = dpy;
   ctx->Next = dpy->ContextList;
   dpy->ContextList = ctx;
   return (EGLContext) ctx;
}


/**
 * Unlink a linked context from its display.
 * Accessing an unlinked context should generate EGL_BAD_CONTEXT error.
 */
void
_eglUnlinkContext(_EGLContext *ctx)
{
   _EGLContext *prev;

   prev = ctx->Display->ContextList;
   if (prev != ctx) {
      while (prev) {
         if (prev->Next == ctx)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = ctx->Next;
   }
   else {
      ctx->Display->ContextList = ctx->Next;
   }

   ctx->Next = NULL;
   ctx->Display = NULL;
}


/**
 * Return the handle of a linked context, or EGL_NO_CONTEXT.
 */
EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   return (EGLContext) (ctx && ctx->Display) ? ctx : EGL_NO_CONTEXT;
}


/**
 * Lookup a handle to find the linked context.
 * Return NULL if the handle has no corresponding linked context.
 */
_EGLContext *
_eglLookupContext(EGLContext ctx)
{
   _EGLContext *context = (_EGLContext *) ctx;
   return (context && context->Display) ? context : NULL;
}


/**
 * Link a surface to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
EGLSurface
_eglLinkSurface(_EGLSurface *surf, _EGLDisplay *dpy)
{
   EGLuint key;

   surf->Display = dpy;
   surf->Next = dpy->SurfaceList;
   dpy->SurfaceList = surf;

   key = _eglHashGenKey(_eglGlobal.Surfaces);
   assert(key);
   _eglHashInsert(_eglGlobal.Surfaces, key, surf);

   surf->Handle = (EGLSurface) key;
   return surf->Handle;
}


/**
 * Unlink a linked surface from its display.
 * Accessing an unlinked surface should generate EGL_BAD_SURFACE error.
 */
void
_eglUnlinkSurface(_EGLSurface *surf)
{
   _EGLSurface *prev;

   _eglHashRemove(_eglGlobal.Surfaces, (EGLuint) surf->Handle);
   surf->Handle = EGL_NO_SURFACE;

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


/**
 * Return the handle of a linked surface, or EGL_NO_SURFACE.
 */
EGLSurface
_eglGetSurfaceHandle(_EGLSurface *surface)
{
   if (surface)
      return surface->Handle;
   else
      return EGL_NO_SURFACE;
}


/**
 * Lookup a handle to find the linked surface.
 * Return NULL if the handle has no corresponding linked surface.
 */
_EGLSurface *
_eglLookupSurface(EGLSurface surf)
{
   _EGLSurface *c = (_EGLSurface *) _eglHashLookup(_eglGlobal.Surfaces,
                                                   (EGLuint) surf);
   return c;
}
