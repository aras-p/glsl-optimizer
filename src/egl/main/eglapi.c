/**
 * Public EGL API entrypoints
 *
 * Generally, we use the EGLDisplay parameter as a key to lookup the
 * appropriate device driver handle, then jump though the driver's
 * dispatch table to handle the function.
 *
 * That allows us the option of supporting multiple, simultaneous,
 * heterogeneous hardware devices in the future.
 *
 * The EGLDisplay, EGLConfig, EGLContext and EGLSurface types are
 * opaque handles implemented with 32-bit unsigned integers.
 * It's up to the driver function or fallback function to look up the
 * handle and get an object.
 * By using opaque handles, we leave open the possibility of having
 * indirect rendering in the future, like GLX.
 *
 *
 * Notes on naming conventions:
 *
 * eglFooBar    - public EGL function
 * EGL_FOO_BAR  - public EGL token
 * EGLDatatype  - public EGL datatype
 *
 * _eglFooBar   - private EGL function
 * _EGLDatatype - private EGL datatype, typedef'd struct
 * _egl_struct  - private EGL struct, non-typedef'd
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "egldisplay.h"
#include "egltypedefs.h"
#include "eglglobals.h"
#include "egldriver.h"
#include "eglsurface.h"



/**
 * This is typically the first EGL function that an application calls.
 * We initialize our global vars and create a private _EGLDisplay object.
 */
EGLDisplay EGLAPIENTRY
eglGetDisplay(NativeDisplayType nativeDisplay)
{
   _EGLDisplay *dpy;
   _eglInitGlobals();
   dpy = _eglNewDisplay(nativeDisplay);
   return _eglGetDisplayHandle(dpy);
}


/**
 * This is typically the second EGL function that an application calls.
 * Here we load/initialize the actual hardware driver.
 */
EGLBoolean EGLAPIENTRY
eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   EGLint major_int, minor_int;

   if (dpy) {
      EGLBoolean retVal;
      _EGLDisplay *dpyPriv = _eglLookupDisplay(dpy);
      if (!dpyPriv) {
         return EGL_FALSE;
      }
      dpyPriv->Driver = _eglOpenDriver(dpyPriv,
                                       dpyPriv->DriverName,
                                       dpyPriv->DriverArgs);
      if (!dpyPriv->Driver) {
         return EGL_FALSE;
      }
      /* Initialize the particular driver now */
      retVal = dpyPriv->Driver->API.Initialize(dpyPriv->Driver, dpy,
                                               &major_int, &minor_int);

      dpyPriv->Driver->APImajor = major_int;
      dpyPriv->Driver->APIminor = minor_int;
      snprintf(dpyPriv->Driver->Version, sizeof(dpyPriv->Driver->Version),
               "%d.%d (%s)", major_int, minor_int, dpyPriv->Driver->Name);

      /* Update applications version of major and minor if not NULL */
      if((major != NULL) && (minor != NULL))
      {
         *major = major_int;
         *minor = minor_int;
      }

      return retVal;
   }
   return EGL_FALSE;
}


EGLBoolean EGLAPIENTRY
eglTerminate(EGLDisplay dpy)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return _eglCloseDriver(drv, dpy);
   else
      return EGL_FALSE;
}


const char * EGLAPIENTRY
eglQueryString(EGLDisplay dpy, EGLint name)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.QueryString(drv, dpy, name);
   else
      return NULL;
}


EGLBoolean EGLAPIENTRY
eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   /* XXX check drv for null in remaining functions */
   return drv->API.GetConfigs(drv, dpy, configs, config_size, num_config);
}


EGLBoolean EGLAPIENTRY
eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ChooseConfig(drv, dpy, attrib_list, configs, config_size, num_config);
}


EGLBoolean EGLAPIENTRY
eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.GetConfigAttrib(drv, dpy, config, attribute, value);
}


EGLContext EGLAPIENTRY
eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreateContext(drv, dpy, config, share_list, attrib_list);
}


EGLBoolean EGLAPIENTRY
eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.DestroyContext(drv, dpy, ctx);
}


EGLBoolean EGLAPIENTRY
eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.MakeCurrent(drv, dpy, draw, read, ctx);
}


EGLBoolean EGLAPIENTRY
eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QueryContext(drv, dpy, ctx, attribute, value);
}


EGLSurface EGLAPIENTRY
eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreateWindowSurface(drv, dpy, config, window, attrib_list);
}


EGLSurface EGLAPIENTRY
eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreatePixmapSurface(drv, dpy, config, pixmap, attrib_list);
}


EGLSurface EGLAPIENTRY
eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreatePbufferSurface(drv, dpy, config, attrib_list);
}


EGLBoolean EGLAPIENTRY
eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.DestroySurface(drv, dpy, surface);
}


EGLBoolean EGLAPIENTRY
eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QuerySurface(drv, dpy, surface, attribute, value);
}


EGLBoolean EGLAPIENTRY
eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.SurfaceAttrib(drv, dpy, surface, attribute, value);
}


EGLBoolean EGLAPIENTRY
eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.BindTexImage(drv, dpy, surface, buffer);
}


EGLBoolean EGLAPIENTRY
eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ReleaseTexImage(drv, dpy, surface, buffer);
}


EGLBoolean EGLAPIENTRY
eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.SwapInterval(drv, dpy, interval);
}


EGLBoolean EGLAPIENTRY
eglSwapBuffers(EGLDisplay dpy, EGLSurface draw)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.SwapBuffers(drv, dpy, draw);
}


EGLBoolean EGLAPIENTRY
eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CopyBuffers(drv, dpy, surface, target);
}


EGLBoolean EGLAPIENTRY
eglWaitGL(void)
{
   EGLDisplay dpy = eglGetCurrentDisplay();
   if (dpy != EGL_NO_DISPLAY) {
      _EGLDriver *drv = _eglLookupDriver(dpy);
      return drv->API.WaitGL(drv, dpy);
   }
   else
      return EGL_FALSE;
}


EGLBoolean EGLAPIENTRY
eglWaitNative(EGLint engine)
{
   EGLDisplay dpy = eglGetCurrentDisplay();
   if (dpy != EGL_NO_DISPLAY) {
      _EGLDriver *drv = _eglLookupDriver(dpy);
      return drv->API.WaitNative(drv, dpy, engine);
   }
   else
      return EGL_FALSE;
}


EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay(void)
{
   _EGLDisplay *dpy = _eglGetCurrentDisplay();
   return _eglGetDisplayHandle(dpy);
}


EGLContext EGLAPIENTRY
eglGetCurrentContext(void)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   return _eglGetContextHandle(ctx);
}


EGLSurface EGLAPIENTRY
eglGetCurrentSurface(EGLint readdraw)
{
   _EGLSurface *s = _eglGetCurrentSurface(readdraw);
   return _eglGetSurfaceHandle(s);
}


EGLint EGLAPIENTRY
eglGetError(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLint e = t->LastError;
   t->LastError = EGL_SUCCESS;
   return e;
}


void (* EGLAPIENTRY eglGetProcAddress(const char *procname))()
{
   typedef void (*genericFunc)();
   struct name_function {
      const char *name;
      _EGLProc function;
   };
   static struct name_function egl_functions[] = {
      /* alphabetical order */
      { "eglBindTexImage", (_EGLProc) eglBindTexImage },
      { "eglChooseConfig", (_EGLProc) eglChooseConfig },
      { "eglCopyBuffers", (_EGLProc) eglCopyBuffers },
      { "eglCreateContext", (_EGLProc) eglCreateContext },
      { "eglCreatePbufferSurface", (_EGLProc) eglCreatePbufferSurface },
      { "eglCreatePixmapSurface", (_EGLProc) eglCreatePixmapSurface },
      { "eglCreateWindowSurface", (_EGLProc) eglCreateWindowSurface },
      { "eglDestroyContext", (_EGLProc) eglDestroyContext },
      { "eglDestroySurface", (_EGLProc) eglDestroySurface },
      { "eglGetConfigAttrib", (_EGLProc) eglGetConfigAttrib },
      { "eglGetConfigs", (_EGLProc) eglGetConfigs },
      { "eglGetCurrentContext", (_EGLProc) eglGetCurrentContext },
      { "eglGetCurrentDisplay", (_EGLProc) eglGetCurrentDisplay },
      { "eglGetCurrentSurface", (_EGLProc) eglGetCurrentSurface },
      { "eglGetDisplay", (_EGLProc) eglGetDisplay },
      { "eglGetError", (_EGLProc) eglGetError },
      { "eglGetProcAddress", (_EGLProc) eglGetProcAddress },
      { "eglInitialize", (_EGLProc) eglInitialize },
      { "eglMakeCurrent", (_EGLProc) eglMakeCurrent },
      { "eglQueryContext", (_EGLProc) eglQueryContext },
      { "eglQueryString", (_EGLProc) eglQueryString },
      { "eglQuerySurface", (_EGLProc) eglQuerySurface },
      { "eglReleaseTexImage", (_EGLProc) eglReleaseTexImage },
      { "eglSurfaceAttrib", (_EGLProc) eglSurfaceAttrib },
      { "eglSwapBuffers", (_EGLProc) eglSwapBuffers },
      { "eglSwapInterval", (_EGLProc) eglSwapInterval },
      { "eglTerminate", (_EGLProc) eglTerminate },
      { "eglWaitGL", (_EGLProc) eglWaitGL },
      { "eglWaitNative", (_EGLProc) eglWaitNative },
      /* Extensions */
#ifdef EGL_MESA_screen_surface
      { "eglChooseModeMESA", (_EGLProc) eglChooseModeMESA },
      { "eglGetModesMESA", (_EGLProc) eglGetModesMESA },
      { "eglGetModeAttribMESA", (_EGLProc) eglGetModeAttribMESA },
      { "eglCopyContextMESA", (_EGLProc) eglCopyContextMESA },
      { "eglGetScreensMESA", (_EGLProc) eglGetScreensMESA },
      { "eglCreateScreenSurfaceMESA", (_EGLProc) eglCreateScreenSurfaceMESA },
      { "eglShowScreenSurfaceMESA", (_EGLProc) eglShowScreenSurfaceMESA },
      { "eglScreenPositionMESA", (_EGLProc) eglScreenPositionMESA },
      { "eglQueryScreenMESA", (_EGLProc) eglQueryScreenMESA },
      { "eglQueryScreenSurfaceMESA", (_EGLProc) eglQueryScreenSurfaceMESA },
      { "eglQueryScreenModeMESA", (_EGLProc) eglQueryScreenModeMESA },
      { "eglQueryModeStringMESA", (_EGLProc) eglQueryModeStringMESA },
#endif /* EGL_MESA_screen_surface */
#ifdef EGL_VERSION_1_2
      { "eglBindAPI", (_EGLProc) eglBindAPI },
      { "eglCreatePbufferFromClientBuffer", (_EGLProc) eglCreatePbufferFromClientBuffer },
      { "eglQueryAPI", (_EGLProc) eglQueryAPI },
      { "eglReleaseThread", (_EGLProc) eglReleaseThread },
      { "eglWaitClient", (_EGLProc) eglWaitClient },
#endif /* EGL_VERSION_1_2 */
      { NULL, NULL }
   };
   EGLint i;
   for (i = 0; egl_functions[i].name; i++) {
      if (strcmp(egl_functions[i].name, procname) == 0) {
         return (genericFunc) egl_functions[i].function;
      }
   }

   /* now loop over drivers to query their procs */
   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLProc p = _eglGlobal.Drivers[i]->API.GetProcAddress(procname);
      if (p)
         return p;
   }

   return NULL;
}


/*
 * EGL_MESA_screen extension
 */

EGLBoolean EGLAPIENTRY
eglChooseModeMESA(EGLDisplay dpy, EGLScreenMESA screen,
                  const EGLint *attrib_list, EGLModeMESA *modes,
                  EGLint modes_size, EGLint *num_modes)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.ChooseModeMESA(drv, dpy, screen, attrib_list, modes, modes_size, num_modes);
   else
      return EGL_FALSE;
}


EGLBoolean EGLAPIENTRY
eglGetModesMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *modes, EGLint mode_size, EGLint *num_mode)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.GetModesMESA(drv, dpy, screen, modes, mode_size, num_mode);
   else
      return EGL_FALSE;
}


EGLBoolean EGLAPIENTRY
eglGetModeAttribMESA(EGLDisplay dpy, EGLModeMESA mode, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.GetModeAttribMESA(drv, dpy, mode, attribute, value);
   else
      return EGL_FALSE;
}


EGLBoolean EGLAPIENTRY
eglCopyContextMESA(EGLDisplay dpy, EGLContext source, EGLContext dest, EGLint mask)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.CopyContextMESA(drv, dpy, source, dest, mask);
   else
      return EGL_FALSE;
}


EGLBoolean
eglGetScreensMESA(EGLDisplay dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.GetScreensMESA(drv, dpy, screens, max_screens, num_screens);
   else
      return EGL_FALSE;
}


EGLSurface
eglCreateScreenSurfaceMESA(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreateScreenSurfaceMESA(drv, dpy, config, attrib_list);
}


EGLBoolean
eglShowScreenSurfaceMESA(EGLDisplay dpy, EGLint screen, EGLSurface surface, EGLModeMESA mode)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ShowScreenSurfaceMESA(drv, dpy, screen, surface, mode);
}


EGLBoolean
eglScreenPositionMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLint x, EGLint y)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ScreenPositionMESA(drv, dpy, screen, x, y);
}


EGLBoolean
eglQueryScreenMESA( EGLDisplay dpy, EGLScreenMESA screen, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QueryScreenMESA(drv, dpy, screen, attribute, value);
}


EGLBoolean
eglQueryScreenSurfaceMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLSurface *surface)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QueryScreenSurfaceMESA(drv, dpy, screen, surface);
}


EGLBoolean
eglQueryScreenModeMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *mode)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QueryScreenModeMESA(drv, dpy, screen, mode);
}


const char *
eglQueryModeStringMESA(EGLDisplay dpy, EGLModeMESA mode)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QueryModeStringMESA(drv, dpy, mode);
}


/**
 ** EGL 1.2
 **/

#ifdef EGL_VERSION_1_2


/**
 * Specify the client API to use for subsequent calls including:
 *  eglCreateContext()
 *  eglGetCurrentContext()
 *  eglGetCurrentDisplay()
 *  eglGetCurrentSurface()
 *  eglMakeCurrent(when the ctx parameter is EGL NO CONTEXT)
 *  eglWaitClient()
 *  eglWaitNative()
 * See section 3.7 "Rendering Context" in the EGL specification for details.
 */
EGLBoolean
eglBindAPI(EGLenum api)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();

   switch (api) {
#ifdef EGL_VERSION_1_4
   case EGL_OPENGL_API:
      if (_eglGlobal.ClientAPIsMask & EGL_OPENGL_BIT) {
         t->CurrentAPI = api;
         return EGL_TRUE;
      }
      _eglError(EGL_BAD_PARAMETER, "eglBindAPI");
      return EGL_FALSE;
#endif
   case EGL_OPENGL_ES_API:
      if (_eglGlobal.ClientAPIsMask & (EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT)) {
         t->CurrentAPI = api;
         return EGL_TRUE;
      }
      _eglError(EGL_BAD_PARAMETER, "eglBindAPI");
      return EGL_FALSE;
   case EGL_OPENVG_API:
      if (_eglGlobal.ClientAPIsMask & EGL_OPENVG_BIT) {
         t->CurrentAPI = api;
         return EGL_TRUE;
      }
      _eglError(EGL_BAD_PARAMETER, "eglBindAPI");
      return EGL_FALSE;
   default:
      return EGL_FALSE;
   }
   return EGL_TRUE;
}


/**
 * Return the last value set with eglBindAPI().
 */
EGLenum
eglQueryAPI(void)
{
   /* returns one of EGL_OPENGL_API, EGL_OPENGL_ES_API or EGL_OPENVG_API */
   _EGLThreadInfo *t = _eglGetCurrentThread();
   return t->CurrentAPI;
}


EGLSurface
eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype,
                                 EGLClientBuffer buffer, EGLConfig config,
                                 const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreatePbufferFromClientBuffer(drv, dpy, buftype, buffer,
                                                 config, attrib_list);
}


EGLBoolean
eglReleaseThread(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLDisplay dpy = eglGetCurrentDisplay();
   if (dpy) {
      _EGLDriver *drv = _eglLookupDriver(dpy);
      /* unbind context */
      (void) drv->API.MakeCurrent(drv, dpy, EGL_NO_SURFACE,
                                  EGL_NO_SURFACE, EGL_NO_CONTEXT);
   }
   _eglDeleteThreadData(t);
   return EGL_TRUE;
}


EGLBoolean
eglWaitClient(void)
{
   EGLDisplay dpy = eglGetCurrentDisplay();
   if (dpy != EGL_NO_DISPLAY) {
      _EGLDriver *drv = _eglLookupDriver(dpy);
      return drv->API.WaitClient(drv, dpy);
   }
   else
      return EGL_FALSE;
}

#endif /* EGL_VERSION_1_2 */
