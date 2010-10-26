#include <string.h>
#include "egltypedefs.h"
#include "egldriver.h"
#include "eglconfig.h"
#include "eglcontext.h"
#include "eglsurface.h"
#include "eglmisc.h"
#include "eglscreen.h"
#include "eglmode.h"
#include "eglsync.h"


static EGLBoolean
_eglReturnFalse(void)
{
   return EGL_FALSE;
}


/**
 * Plug all the available fallback routines into the given driver's
 * dispatch table.
 */
void
_eglInitDriverFallbacks(_EGLDriver *drv)
{
   memset(&drv->API, 0, sizeof(drv->API));

   /* the driver has to implement these */
   drv->API.Initialize = NULL;
   drv->API.Terminate = NULL;

   drv->API.GetConfigs = _eglGetConfigs;
   drv->API.ChooseConfig = _eglChooseConfig;
   drv->API.GetConfigAttrib = _eglGetConfigAttrib;

   drv->API.CreateContext = (CreateContext_t) _eglReturnFalse;
   drv->API.DestroyContext = (DestroyContext_t) _eglReturnFalse;
   drv->API.MakeCurrent = (MakeCurrent_t) _eglReturnFalse;
   drv->API.QueryContext = _eglQueryContext;

   drv->API.CreateWindowSurface = (CreateWindowSurface_t) _eglReturnFalse;
   drv->API.CreatePixmapSurface = (CreatePixmapSurface_t) _eglReturnFalse;
   drv->API.CreatePbufferSurface = (CreatePbufferSurface_t) _eglReturnFalse;
   drv->API.CreatePbufferFromClientBuffer =
      (CreatePbufferFromClientBuffer_t) _eglReturnFalse;
   drv->API.DestroySurface = (DestroySurface_t) _eglReturnFalse;
   drv->API.QuerySurface = _eglQuerySurface;
   drv->API.SurfaceAttrib = _eglSurfaceAttrib;

   drv->API.BindTexImage = (BindTexImage_t) _eglReturnFalse;
   drv->API.ReleaseTexImage = (ReleaseTexImage_t) _eglReturnFalse;
   drv->API.CopyBuffers = (CopyBuffers_t) _eglReturnFalse;
   drv->API.SwapBuffers = (SwapBuffers_t) _eglReturnFalse;
   drv->API.SwapInterval = _eglSwapInterval;

   drv->API.WaitClient = (WaitClient_t) _eglReturnFalse;
   drv->API.WaitNative = (WaitNative_t) _eglReturnFalse;
   drv->API.GetProcAddress = (GetProcAddress_t) _eglReturnFalse;
   drv->API.QueryString = _eglQueryString;

#ifdef EGL_MESA_screen_surface
   drv->API.CopyContextMESA = (CopyContextMESA_t) _eglReturnFalse;
   drv->API.CreateScreenSurfaceMESA =
      (CreateScreenSurfaceMESA_t) _eglReturnFalse;
   drv->API.ShowScreenSurfaceMESA = (ShowScreenSurfaceMESA_t) _eglReturnFalse;
   drv->API.ChooseModeMESA = _eglChooseModeMESA;
   drv->API.GetModesMESA = _eglGetModesMESA;
   drv->API.GetModeAttribMESA = _eglGetModeAttribMESA;
   drv->API.GetScreensMESA = _eglGetScreensMESA;
   drv->API.ScreenPositionMESA = _eglScreenPositionMESA;
   drv->API.QueryScreenMESA = _eglQueryScreenMESA;
   drv->API.QueryScreenSurfaceMESA = _eglQueryScreenSurfaceMESA;
   drv->API.QueryScreenModeMESA = _eglQueryScreenModeMESA;
   drv->API.QueryModeStringMESA = _eglQueryModeStringMESA;
#endif /* EGL_MESA_screen_surface */

#ifdef EGL_KHR_image_base
   drv->API.CreateImageKHR = NULL;
   drv->API.DestroyImageKHR = NULL;
#endif /* EGL_KHR_image_base */

#ifdef EGL_KHR_reusable_sync
   drv->API.CreateSyncKHR = NULL;
   drv->API.DestroySyncKHR = NULL;
   drv->API.ClientWaitSyncKHR = NULL;
   drv->API.SignalSyncKHR = NULL;
   drv->API.GetSyncAttribKHR = _eglGetSyncAttribKHR;
#endif /* EGL_KHR_reusable_sync */

#ifdef EGL_MESA_drm_image
   drv->API.CreateDRMImageMESA = NULL;
   drv->API.ExportDRMImageMESA = NULL;
#endif

#ifdef EGL_NOK_swap_region
   drv->API.SwapBuffersRegionNOK = NULL;
#endif
}
