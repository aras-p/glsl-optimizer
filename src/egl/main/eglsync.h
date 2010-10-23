#ifndef EGLSYNC_INCLUDED
#define EGLSYNC_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


#ifdef EGL_KHR_reusable_sync


/**
 * "Base" class for device driver syncs.
 */
struct _egl_sync
{
   /* A sync is a display resource */
   _EGLResource Resource;

   EGLenum Type;
   EGLenum SyncStatus;
   EGLenum SyncCondition;
};


PUBLIC EGLBoolean
_eglInitSync(_EGLSync *sync, _EGLDisplay *dpy, EGLenum type,
             const EGLint *attrib_list);


extern EGLBoolean
_eglGetSyncAttribKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                     EGLint attribute, EGLint *value);


/**
 * Increment reference count for the sync.
 */
static INLINE _EGLSync *
_eglGetSync(_EGLSync *sync)
{
   if (sync)
      _eglGetResource(&sync->Resource);
   return sync;
}


/**
 * Decrement reference count for the sync.
 */
static INLINE EGLBoolean
_eglPutSync(_EGLSync *sync)
{
   return (sync) ? _eglPutResource(&sync->Resource) : EGL_FALSE;
}


/**
 * Link a sync to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLSyncKHR
_eglLinkSync(_EGLSync *sync)
{
   _eglLinkResource(&sync->Resource, _EGL_RESOURCE_SYNC);
   return (EGLSyncKHR) sync;
}


/**
 * Unlink a linked sync from its display.
 */
static INLINE void
_eglUnlinkSync(_EGLSync *sync)
{
   _eglUnlinkResource(&sync->Resource, _EGL_RESOURCE_SYNC);
}


/**
 * Lookup a handle to find the linked sync.
 * Return NULL if the handle has no corresponding linked sync.
 */
static INLINE _EGLSync *
_eglLookupSync(EGLSyncKHR handle, _EGLDisplay *dpy)
{
   _EGLSync *sync = (_EGLSync *) handle;
   if (!dpy || !_eglCheckResource((void *) sync, _EGL_RESOURCE_SYNC, dpy))
      sync = NULL;
   return sync;
}


/**
 * Return the handle of a linked sync, or EGL_NO_SYNC_KHR.
 */
static INLINE EGLSyncKHR
_eglGetSyncHandle(_EGLSync *sync)
{
   _EGLResource *res = (_EGLResource *) sync;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLSyncKHR) sync : EGL_NO_SYNC_KHR;
}


#endif /* EGL_KHR_reusable_sync */


#endif /* EGLSYNC_INCLUDED */
