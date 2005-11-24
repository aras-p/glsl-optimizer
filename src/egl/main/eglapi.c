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
#include <string.h>
#include "eglcontext.h"
#include "egldisplay.h"
#include "egltypedefs.h"
#include "eglglobals.h"
#include "egldriver.h"
#include "eglsurface.h"



/**
 * NOTE: displayName is treated as a string in _eglChooseDriver()!!!
 * This will probably change!
 * See _eglChooseDriver() for details!
 */
EGLDisplay APIENTRY
eglGetDisplay(NativeDisplayType displayName)
{
   _EGLDisplay *dpy;
   _eglInitGlobals();
   dpy = _eglNewDisplay(displayName);
   if (dpy)
      return dpy->Handle;
   else
      return EGL_NO_DISPLAY;
}


EGLBoolean APIENTRY
eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   if (dpy) {
      _EGLDriver *drv = _eglChooseDriver(dpy);
      if (drv)
         return drv->API.Initialize(drv, dpy, major, minor);
   }
   return EGL_FALSE;
}


EGLBoolean APIENTRY
eglTerminate(EGLDisplay dpy)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return _eglCloseDriver(drv, dpy);
   else
      return EGL_FALSE;
}


const char * APIENTRY
eglQueryString(EGLDisplay dpy, EGLint name)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.QueryString(drv, dpy, name);
   else
      return NULL;
}


EGLBoolean APIENTRY
eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   /* XXX check drv for null in remaining functions */
   return drv->API.GetConfigs(drv, dpy, configs, config_size, num_config);
}


EGLBoolean APIENTRY
eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ChooseConfig(drv, dpy, attrib_list, configs, config_size, num_config);
}


EGLBoolean APIENTRY
eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.GetConfigAttrib(drv, dpy, config, attribute, value);
}


EGLContext APIENTRY
eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreateContext(drv, dpy, config, share_list, attrib_list);
}


EGLBoolean APIENTRY
eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.DestroyContext(drv, dpy, ctx);
}


EGLBoolean APIENTRY
eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.MakeCurrent(drv, dpy, draw, read, ctx);
}


EGLBoolean APIENTRY
eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QueryContext(drv, dpy, ctx, attribute, value);
}


EGLSurface APIENTRY
eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreateWindowSurface(drv, dpy, config, window, attrib_list);
}


EGLSurface APIENTRY
eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreatePixmapSurface(drv, dpy, config, pixmap, attrib_list);
}


EGLSurface APIENTRY
eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CreatePbufferSurface(drv, dpy, config, attrib_list);
}


EGLBoolean APIENTRY
eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.DestroySurface(drv, dpy, surface);
}


EGLBoolean APIENTRY
eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.QuerySurface(drv, dpy, surface, attribute, value);
}


EGLBoolean APIENTRY
eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.SurfaceAttrib(drv, dpy, surface, attribute, value);
}


EGLBoolean APIENTRY
eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.BindTexImage(drv, dpy, surface, buffer);
}


EGLBoolean APIENTRY
eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ReleaseTexImage(drv, dpy, surface, buffer);
}


EGLBoolean APIENTRY
eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.SwapInterval(drv, dpy, interval);
}


EGLBoolean APIENTRY
eglSwapBuffers(EGLDisplay dpy, EGLSurface draw)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.SwapBuffers(drv, dpy, draw);
}


EGLBoolean APIENTRY
eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.CopyBuffers(drv, dpy, surface, target);
}


EGLBoolean APIENTRY
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


EGLBoolean APIENTRY
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


EGLDisplay APIENTRY
eglGetCurrentDisplay(void)
{
   _EGLDisplay *dpy = _eglGetCurrentDisplay();
   if (dpy)
      return dpy->Handle;
   else
      return EGL_NO_DISPLAY;
}


EGLContext APIENTRY
eglGetCurrentContext(void)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   if (ctx)
      return ctx->Handle;
   else
      return EGL_NO_CONTEXT;
}


EGLSurface APIENTRY
eglGetCurrentSurface(EGLint readdraw)
{
   _EGLSurface *s = _eglGetCurrentSurface(readdraw);
   if (s)
      return s->Handle;
   else
      return EGL_NO_SURFACE;
}


EGLint APIENTRY
eglGetError(void)
{
   EGLint e = _eglGlobal.LastError;
   _eglGlobal.LastError = EGL_SUCCESS;
   return e;
}


void (* APIENTRY eglGetProcAddress(const char *procname))()
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
      { "eglChooseModeMESA", (_EGLProc) eglChooseModeMESA },
      { "eglGetModesMESA", (_EGLProc) eglGetModesMESA },
      { "eglGetModeAttribMESA", (_EGLProc) eglGetModeAttribMESA },
      { "eglCopyContextMESA", (_EGLProc) eglCopyContextMESA },
      { "eglGetScreensMESA", (_EGLProc) eglGetScreensMESA },
      { "eglCreateScreenSurfaceMESA", (_EGLProc) eglCreateScreenSurfaceMESA },
      { "eglShowSurfaceMESA", (_EGLProc) eglShowSurfaceMESA },
      { "eglScreenPositionMESA", (_EGLProc) eglScreenPositionMESA },
      { "eglQueryScreenMESA", (_EGLProc) eglQueryScreenMESA },
      { "eglQueryScreenSurfaceMESA", (_EGLProc) eglQueryScreenSurfaceMESA },
      { "eglQueryScreenModeMESA", (_EGLProc) eglQueryScreenModeMESA },
      { "eglQueryModeStringMESA", (_EGLProc) eglQueryModeStringMESA },
      { NULL, NULL }
   };
   EGLint i;
   for (i = 0; egl_functions[i].name; i++) {
      if (strcmp(egl_functions[i].name, procname) == 0) {
         return (genericFunc) egl_functions[i].function;
      }
   }
#if 0
   /* XXX enable this code someday */
   return (genericFunc) _glapi_get_proc_address(procname);
#else
   return NULL;
#endif
}


/*
 * EGL_MESA_screen extension
 */

EGLBoolean APIENTRY
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


EGLBoolean APIENTRY
eglGetModesMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *modes, EGLint mode_size, EGLint *num_mode)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.GetModesMESA(drv, dpy, screen, modes, mode_size, num_mode);
   else
      return EGL_FALSE;
}


EGLBoolean APIENTRY
eglGetModeAttribMESA(EGLDisplay dpy, EGLModeMESA mode, EGLint attribute, EGLint *value)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   if (drv)
      return drv->API.GetModeAttribMESA(drv, dpy, mode, attribute, value);
   else
      return EGL_FALSE;
}


EGLBoolean APIENTRY
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
eglShowSurfaceMESA(EGLDisplay dpy, EGLint screen, EGLSurface surface, EGLModeMESA mode)
{
   _EGLDriver *drv = _eglLookupDriver(dpy);
   return drv->API.ShowSurfaceMESA(drv, dpy, screen, surface, mode);
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





