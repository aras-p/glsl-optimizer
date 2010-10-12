#include <string.h>

#include "eglsync.h"
#include "eglcurrent.h"
#include "egllog.h"


#ifdef EGL_KHR_reusable_sync


/**
 * Parse the list of sync attributes and return the proper error code.
 */
static EGLint
_eglParseSyncAttribList(_EGLSync *sync, const EGLint *attrib_list)
{
   EGLint i, err = EGL_SUCCESS;

   if (!attrib_list)
      return EGL_SUCCESS;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      default:
         (void) val;
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad sync attribute 0x%04x", attr);
         break;
      }
   }

   return err;
}


EGLBoolean
_eglInitSync(_EGLSync *sync, _EGLDisplay *dpy, EGLenum type,
             const EGLint *attrib_list)
{
   EGLint err;

   if (!(type == EGL_SYNC_REUSABLE_KHR && dpy->Extensions.KHR_reusable_sync) &&
       !(type == EGL_SYNC_FENCE_KHR && dpy->Extensions.KHR_fence_sync))
      return _eglError(EGL_BAD_ATTRIBUTE, "eglCreateSyncKHR");

   memset(sync, 0, sizeof(*sync));

   sync->Resource.Display = dpy;

   sync->Type = type;
   sync->SyncStatus = EGL_UNSIGNALED_KHR;
   sync->SyncCondition = EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR;

   err = _eglParseSyncAttribList(sync, attrib_list);
   if (err != EGL_SUCCESS)
      return _eglError(err, "eglCreateSyncKHR");

   return EGL_TRUE;
}


_EGLSync *
_eglCreateSyncKHR(_EGLDriver *drv, _EGLDisplay *dpy,
                  EGLenum type, const EGLint *attrib_list)
{
   return NULL;
}


EGLBoolean
_eglDestroySyncKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync)
{
   return EGL_TRUE;
}


EGLint
_eglClientWaitSyncKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                      EGLint flags, EGLTimeKHR timeout)
{
   return EGL_FALSE;
}


EGLBoolean
_eglSignalSyncKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                  EGLenum mode)
{
   return EGL_FALSE;
}


EGLBoolean
_eglGetSyncAttribKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                     EGLint attribute, EGLint *value)
{
   if (!value)
      return _eglError(EGL_BAD_PARAMETER, "eglGetConfigs");

   switch (attribute) {
   case EGL_SYNC_TYPE_KHR:
      *value = sync->Type;
      break;
   case EGL_SYNC_STATUS_KHR:
      *value = sync->SyncStatus;
      break;
   case EGL_SYNC_CONDITION_KHR:
      if (sync->Type != EGL_SYNC_FENCE_KHR)
         return _eglError(EGL_BAD_ATTRIBUTE, "eglGetSyncAttribKHR");
      *value = sync->SyncCondition;
      break;
   default:
      return _eglError(EGL_BAD_ATTRIBUTE, "eglGetSyncAttribKHR");
      break;
   }

   return EGL_TRUE;
}


#endif /* EGL_KHR_reusable_sync */
