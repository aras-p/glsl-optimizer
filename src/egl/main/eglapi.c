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
 * opaque handles. Internal objects are linked to a display to
 * create the handles.
 *
 * For each public API entry point, the opaque handles are looked up
 * before being dispatched to the drivers.  When it fails to look up
 * a handle, one of
 *
 * EGL_BAD_DISPLAY
 * EGL_BAD_CONFIG
 * EGL_BAD_CONTEXT
 * EGL_BAD_SURFACE
 * EGL_BAD_SCREEN_MESA
 * EGL_BAD_MODE_MESA
 *
 * is generated and the driver function is not called. An
 * uninitialized EGLDisplay has no driver associated with it. When
 * such display is detected,
 *
 * EGL_NOT_INITIALIZED
 *
 * is generated.
 *
 * Some of the entry points use current display, context, or surface
 * implicitly.  For such entry points, the implicit objects are also
 * checked before calling the driver function.  Other than the
 * errors listed above,
 *
 * EGL_BAD_CURRENT_SURFACE
 *
 * may also be generated.
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
#include "eglcurrent.h"
#include "egldriver.h"
#include "eglsurface.h"
#include "eglconfig.h"
#include "eglscreen.h"
#include "eglmode.h"
#include "eglimage.h"


/**
 * Macros to help return an API entrypoint.
 */
#define _EGL_ERROR(disp, err, ret)              \
   ({                                           \
      /* EGL error codes are non-zero */        \
      if (err)                                  \
         _eglError(err, __FUNCTION__);          \
      ret;                                      \
   })
#define _EGL_SUCCESS(disp, ret)  _EGL_ERROR(disp, EGL_SUCCESS, ret)
/* record EGL_SUCCESS only when ret evaluates to true */
#define _EGL_EVAL(disp, ret)  _EGL_ERROR(disp, (ret) ? EGL_SUCCESS : 0, ret)


/**
 * A bunch of macros and checks to simplify error checking.
 */
#define _EGL_CHECK_DISPLAY(disp, ret)                                   \
   ({                                                                   \
      _EGLDriver *__drv = _eglCheckDisplay(disp, __FUNCTION__);         \
      if (!__drv)                                                       \
         return _EGL_ERROR(disp, 0, ret);                               \
      __drv;                                                            \
   })


#define _EGL_CHECK_OBJECT(disp, type, obj, ret)                         \
   ({                                                                   \
      _EGLDriver *__drv = _eglCheck ## type(disp, obj, __FUNCTION__);   \
      if (!__drv)                                                       \
         return _EGL_ERROR(disp, 0, ret);                               \
      __drv;                                                            \
   })
#define _EGL_CHECK_SURFACE(disp, surf, ret)      \
   _EGL_CHECK_OBJECT(disp, Surface, surf, ret)
#define _EGL_CHECK_CONTEXT(disp, context, ret)   \
   _EGL_CHECK_OBJECT(disp, Context, context, ret)
#define _EGL_CHECK_CONFIG(disp, conf, ret)       \
   _EGL_CHECK_OBJECT(disp, Config, conf, ret)
#define _EGL_CHECK_SCREEN(disp, scrn, ret)       \
   _EGL_CHECK_OBJECT(disp, Screen, scrn, ret)
#define _EGL_CHECK_MODE(disp, m, ret)            \
   _EGL_CHECK_OBJECT(disp, Mode, m, ret)


static INLINE _EGLDriver *
_eglCheckDisplay(_EGLDisplay *disp, const char *msg)
{
   if (!disp) {
      _eglError(EGL_BAD_DISPLAY, msg);
      return NULL;
   }
   if (!disp->Initialized) {
      _eglError(EGL_NOT_INITIALIZED, msg);
      return NULL;
   }
   return disp->Driver;
}


static INLINE _EGLDriver *
_eglCheckSurface(_EGLDisplay *disp, _EGLSurface *surf, const char *msg)
{
   _EGLDriver *drv = _eglCheckDisplay(disp, msg);
   if (!drv)
      return NULL;
   if (!surf) {
      _eglError(EGL_BAD_SURFACE, msg);
      return NULL;
   }
   return drv;
}


static INLINE _EGLDriver *
_eglCheckContext(_EGLDisplay *disp, _EGLContext *context, const char *msg)
{
   _EGLDriver *drv = _eglCheckDisplay(disp, msg);
   if (!drv)
      return NULL;
   if (!context) {
      _eglError(EGL_BAD_CONTEXT, msg);
      return NULL;
   }
   return drv;
}


static INLINE _EGLDriver *
_eglCheckConfig(_EGLDisplay *disp, _EGLConfig *conf, const char *msg)
{
   _EGLDriver *drv = _eglCheckDisplay(disp, msg);
   if (!drv)
      return NULL;
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, msg);
      return NULL;
   }
   return drv;
}


#ifdef EGL_MESA_screen_surface


static INLINE _EGLDriver *
_eglCheckScreen(_EGLDisplay *disp, _EGLScreen *scrn, const char *msg)
{
   _EGLDriver *drv = _eglCheckDisplay(disp, msg);
   if (!drv)
      return NULL;
   if (!scrn) {
      _eglError(EGL_BAD_SCREEN_MESA, msg);
      return NULL;
   }
   return drv;
}


static INLINE _EGLDriver *
_eglCheckMode(_EGLDisplay *disp, _EGLMode *m, const char *msg)
{
   _EGLDriver *drv = _eglCheckDisplay(disp, msg);
   if (!drv)
      return NULL;
   if (!m) {
      _eglError(EGL_BAD_MODE_MESA, msg);
      return NULL;
   }
   return drv;
}


#endif /* EGL_MESA_screen_surface */


/**
 * This is typically the first EGL function that an application calls.
 * We initialize our global vars and create a private _EGLDisplay object.
 */
EGLDisplay EGLAPIENTRY
eglGetDisplay(EGLNativeDisplayType nativeDisplay)
{
   _EGLDisplay *dpy;
   dpy = _eglFindDisplay(nativeDisplay);
   if (!dpy) {
      dpy = _eglNewDisplay(nativeDisplay);
      if (dpy)
         _eglLinkDisplay(dpy);
   }
   return _eglGetDisplayHandle(dpy);
}


/**
 * This is typically the second EGL function that an application calls.
 * Here we load/initialize the actual hardware driver.
 */
EGLBoolean EGLAPIENTRY
eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   EGLint major_int, minor_int;

   if (!disp)
      return _EGL_ERROR(NULL, EGL_BAD_DISPLAY, EGL_FALSE);

   if (!disp->Initialized) {
      _EGLDriver *drv = disp->Driver;

      if (!drv) {
         _eglPreloadDrivers();
         drv = _eglMatchDriver(disp);
         if (!drv)
            return _EGL_ERROR(disp, EGL_NOT_INITIALIZED, EGL_FALSE);
      }

      /* Initialize the particular display now */
      if (!drv->API.Initialize(drv, disp, &major_int, &minor_int))
         return _EGL_ERROR(disp, EGL_NOT_INITIALIZED, EGL_FALSE);

      disp->APImajor = major_int;
      disp->APIminor = minor_int;
      snprintf(disp->Version, sizeof(disp->Version),
               "%d.%d (%s)", major_int, minor_int, drv->Name);

      /* limit to APIs supported by core */
      disp->ClientAPIsMask &= _EGL_API_ALL_BITS;

      disp->Driver = drv;
      disp->Initialized = EGL_TRUE;
   } else {
      major_int = disp->APImajor;
      minor_int = disp->APIminor;
   }

   /* Update applications version of major and minor if not NULL */
   if ((major != NULL) && (minor != NULL)) {
      *major = major_int;
      *minor = minor_int;
   }

   return _EGL_SUCCESS(disp, EGL_TRUE);
}


EGLBoolean EGLAPIENTRY
eglTerminate(EGLDisplay dpy)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   if (!disp)
      return _EGL_ERROR(NULL, EGL_BAD_DISPLAY, EGL_FALSE);

   if (disp->Initialized) {
      _EGLDriver *drv = disp->Driver;

      drv->API.Terminate(drv, disp);
      /* do not reset disp->Driver */
      disp->Initialized = EGL_FALSE;
   }

   return _EGL_SUCCESS(disp, EGL_TRUE);
}


const char * EGLAPIENTRY
eglQueryString(EGLDisplay dpy, EGLint name)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLDriver *drv;
   const char *ret;

   drv = _EGL_CHECK_DISPLAY(disp, NULL);
   ret = drv->API.QueryString(drv, disp, name);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
              EGLint config_size, EGLint *num_config)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
   ret = drv->API.GetConfigs(drv, disp, configs, config_size, num_config);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs,
                EGLint config_size, EGLint *num_config)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
   ret = drv->API.ChooseConfig(drv, disp, attrib_list, configs,
                                config_size, num_config);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                   EGLint attribute, EGLint *value)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_FALSE);
   ret = drv->API.GetConfigAttrib(drv, disp, conf, attribute, value);

   return _EGL_EVAL(disp, ret);
}


EGLContext EGLAPIENTRY
eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list,
                 const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLContext *share = _eglLookupContext(share_list, disp);
   _EGLDriver *drv;
   _EGLContext *context;
   EGLContext ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_NO_CONTEXT);
   if (!share && share_list != EGL_NO_CONTEXT)
      return _EGL_ERROR(disp, EGL_BAD_CONTEXT, EGL_NO_CONTEXT);

   context = drv->API.CreateContext(drv, disp, conf, share, attrib_list);
   ret = (context) ? _eglLinkContext(context, disp) : EGL_NO_CONTEXT;

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_CONTEXT(disp, context, EGL_FALSE);
   _eglUnlinkContext(context);
   ret = drv->API.DestroyContext(drv, disp, context);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read,
               EGLContext ctx)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
   _EGLSurface *draw_surf = _eglLookupSurface(draw, disp);
   _EGLSurface *read_surf = _eglLookupSurface(read, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   if (!disp)
      return _EGL_ERROR(disp, EGL_BAD_DISPLAY, EGL_FALSE);
   drv = disp->Driver;

   /* display is allowed to be uninitialized under certain condition */
   if (!disp->Initialized) {
      if (draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE ||
          ctx != EGL_NO_CONTEXT)
         return _EGL_ERROR(disp, EGL_BAD_DISPLAY, EGL_FALSE);
   }
   if (!drv)
      return _EGL_SUCCESS(disp, EGL_TRUE);

   if (!context && ctx != EGL_NO_CONTEXT)
      return _EGL_ERROR(disp, EGL_BAD_CONTEXT, EGL_FALSE);
   if ((!draw_surf && draw != EGL_NO_SURFACE) ||
       (!read_surf && read != EGL_NO_SURFACE))
      return _EGL_ERROR(disp, EGL_BAD_SURFACE, EGL_FALSE);

   ret = drv->API.MakeCurrent(drv, disp, draw_surf, read_surf, context);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglQueryContext(EGLDisplay dpy, EGLContext ctx,
                EGLint attribute, EGLint *value)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_CONTEXT(disp, context, EGL_FALSE);
   ret = drv->API.QueryContext(drv, disp, context, attribute, value);

   return _EGL_EVAL(disp, ret);
}


EGLSurface EGLAPIENTRY
eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                       EGLNativeWindowType window, const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLDriver *drv;
   _EGLSurface *surf;
   EGLSurface ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_NO_SURFACE);

   surf = drv->API.CreateWindowSurface(drv, disp, conf, window, attrib_list);
   ret = (surf) ? _eglLinkSurface(surf, disp) : EGL_NO_SURFACE;

   return _EGL_EVAL(disp, ret);
}


EGLSurface EGLAPIENTRY
eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
                       EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLDriver *drv;
   _EGLSurface *surf;
   EGLSurface ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_NO_SURFACE);

   surf = drv->API.CreatePixmapSurface(drv, disp, conf, pixmap, attrib_list);
   ret = (surf) ? _eglLinkSurface(surf, disp) : EGL_NO_SURFACE;

   return _EGL_EVAL(disp, ret);
}


EGLSurface EGLAPIENTRY
eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                        const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLDriver *drv;
   _EGLSurface *surf;
   EGLSurface ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_NO_SURFACE);

   surf = drv->API.CreatePbufferSurface(drv, disp, conf, attrib_list);
   ret = (surf) ? _eglLinkSurface(surf, disp) : EGL_NO_SURFACE;

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   _eglUnlinkSurface(surf);
   ret = drv->API.DestroySurface(drv, disp, surf);

   return _EGL_EVAL(disp, ret);
}

EGLBoolean EGLAPIENTRY
eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
                EGLint attribute, EGLint *value)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   ret = drv->API.QuerySurface(drv, disp, surf, attribute, value);

   return _EGL_EVAL(disp, ret);
}

EGLBoolean EGLAPIENTRY
eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
                 EGLint attribute, EGLint value)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   ret = drv->API.SurfaceAttrib(drv, disp, surf, attribute, value);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   ret = drv->API.BindTexImage(drv, disp, surf, buffer);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   ret = drv->API.ReleaseTexImage(drv, disp, surf, buffer);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLContext *ctx = _eglGetCurrentContext();
   _EGLSurface *surf;
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_DISPLAY(disp, EGL_FALSE);

   if (!ctx || !_eglIsContextLinked(ctx) || ctx->Resource.Display != disp)
      return _EGL_ERROR(disp, EGL_BAD_CONTEXT, EGL_FALSE);

   surf = ctx->DrawSurface;
   if (!_eglIsSurfaceLinked(surf))
      return _EGL_ERROR(disp, EGL_BAD_SURFACE, EGL_FALSE);

   ret = drv->API.SwapInterval(drv, disp, surf, interval);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);

   /* surface must be bound to current context in EGL 1.4 */
   if (!ctx || !_eglIsContextLinked(ctx) || surf != ctx->DrawSurface)
      return _EGL_ERROR(disp, EGL_BAD_SURFACE, EGL_FALSE);

   ret = drv->API.SwapBuffers(drv, disp, surf);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   ret = drv->API.CopyBuffers(drv, disp, surf, target);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglWaitClient(void)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp;
   _EGLDriver *drv;
   EGLBoolean ret;

   if (!ctx)
      return _EGL_SUCCESS(NULL, EGL_TRUE);
   /* let bad current context imply bad current surface */
   if (!_eglIsContextLinked(ctx) || !_eglIsSurfaceLinked(ctx->DrawSurface))
      return _EGL_ERROR(NULL, EGL_BAD_CURRENT_SURFACE, EGL_FALSE);

   disp = ctx->Resource.Display;

   /* a valid current context implies an initialized current display */
   assert(disp->Initialized);
   drv = disp->Driver;
   ret = drv->API.WaitClient(drv, disp, ctx);

   return _EGL_EVAL(NULL, ret);
}


EGLBoolean EGLAPIENTRY
eglWaitGL(void)
{
#ifdef EGL_VERSION_1_2
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLint api_index = t->CurrentAPIIndex;
   EGLint es_index = _eglConvertApiToIndex(EGL_OPENGL_ES_API);
   EGLBoolean ret;

   if (api_index != es_index && _eglIsCurrentThreadDummy())
      return _EGL_ERROR(NULL, EGL_BAD_ALLOC, EGL_FALSE);

   t->CurrentAPIIndex = es_index;
   ret = eglWaitClient();
   t->CurrentAPIIndex = api_index;
   return ret;
#else
   return eglWaitClient();
#endif
}


EGLBoolean EGLAPIENTRY
eglWaitNative(EGLint engine)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp;
   _EGLDriver *drv;
   EGLBoolean ret;

   if (!ctx)
      return _EGL_SUCCESS(NULL, EGL_TRUE);

   /* let bad current context imply bad current surface */
   if (!_eglIsContextLinked(ctx) || !_eglIsSurfaceLinked(ctx->DrawSurface))
      return _EGL_ERROR(NULL, EGL_BAD_CURRENT_SURFACE, EGL_FALSE);

   disp = ctx->Resource.Display;

   /* a valid current context implies an initialized current display */
   assert(disp->Initialized);
   drv = disp->Driver;
   ret = drv->API.WaitNative(drv, disp, engine);

   return _EGL_EVAL(NULL, ret);
}


EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay(void)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   EGLDisplay ret;

   ret = (ctx) ? _eglGetDisplayHandle(ctx->Resource.Display) : EGL_NO_DISPLAY;

   return _EGL_SUCCESS(NULL, ret);
}


EGLContext EGLAPIENTRY
eglGetCurrentContext(void)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   EGLContext ret;

   ret = _eglGetContextHandle(ctx);

   return _EGL_SUCCESS(NULL, ret);
}


EGLSurface EGLAPIENTRY
eglGetCurrentSurface(EGLint readdraw)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   EGLint err = EGL_SUCCESS;
   _EGLSurface *surf;
   EGLSurface ret;

   if (!ctx)
      return _EGL_SUCCESS(NULL, EGL_NO_SURFACE);

   switch (readdraw) {
   case EGL_DRAW:
      surf = ctx->DrawSurface;
      break;
   case EGL_READ:
      surf = ctx->ReadSurface;
      break;
   default:
      surf = NULL;
      err = EGL_BAD_PARAMETER;
      break;
   }

   ret = _eglGetSurfaceHandle(surf);

   return _EGL_ERROR(NULL, err, ret);
}


EGLint EGLAPIENTRY
eglGetError(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLint e = t->LastError;
   if (!_eglIsCurrentThreadDummy())
      t->LastError = EGL_SUCCESS;
   return e;
}


__eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char *procname)
{
   static const struct {
      const char *name;
      _EGLProc function;
   } egl_functions[] = {
      /* extensions only */
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
#ifdef EGL_KHR_image_base
      { "eglCreateImageKHR", (_EGLProc) eglCreateImageKHR },
      { "eglDestroyImageKHR", (_EGLProc) eglDestroyImageKHR },
#endif /* EGL_KHR_image_base */
      { NULL, NULL }
   };
   EGLint i;
   _EGLProc ret;

   if (!procname)
      return _EGL_SUCCESS(NULL, NULL);

   ret = NULL;
   if (strncmp(procname, "egl", 3) == 0) {
      for (i = 0; egl_functions[i].name; i++) {
         if (strcmp(egl_functions[i].name, procname) == 0) {
            ret = egl_functions[i].function;
            break;
         }
      }
   }
   if (ret)
      return _EGL_SUCCESS(NULL, ret);

   _eglPreloadDrivers();

   /* now loop over drivers to query their procs */
   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLDriver *drv = _eglGlobal.Drivers[i];
      ret = drv->API.GetProcAddress(drv, procname);
      if (ret)
         break;
   }

   return _EGL_SUCCESS(NULL, ret);
}


#ifdef EGL_MESA_screen_surface


/*
 * EGL_MESA_screen extension
 */

EGLBoolean EGLAPIENTRY
eglChooseModeMESA(EGLDisplay dpy, EGLScreenMESA screen,
                  const EGLint *attrib_list, EGLModeMESA *modes,
                  EGLint modes_size, EGLint *num_modes)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen(screen, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   ret = drv->API.ChooseModeMESA(drv, disp, scrn, attrib_list,
         modes, modes_size, num_modes);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglGetModesMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *modes,
                EGLint mode_size, EGLint *num_mode)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen(screen, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   ret = drv->API.GetModesMESA(drv, disp, scrn, modes, mode_size, num_mode);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglGetModeAttribMESA(EGLDisplay dpy, EGLModeMESA mode,
                     EGLint attribute, EGLint *value)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLMode *m = _eglLookupMode(mode, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_MODE(disp, m, EGL_FALSE);
   ret = drv->API.GetModeAttribMESA(drv, disp, m, attribute, value);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean EGLAPIENTRY
eglCopyContextMESA(EGLDisplay dpy, EGLContext source, EGLContext dest,
                   EGLint mask)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLContext *source_context = _eglLookupContext(source, disp);
   _EGLContext *dest_context = _eglLookupContext(dest, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_CONTEXT(disp, source_context, EGL_FALSE);
   if (!dest_context)
      return _EGL_ERROR(disp, EGL_BAD_CONTEXT, EGL_FALSE);

   ret = drv->API.CopyContextMESA(drv, disp,
         source_context, dest_context, mask);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglGetScreensMESA(EGLDisplay dpy, EGLScreenMESA *screens,
                  EGLint max_screens, EGLint *num_screens)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
   ret = drv->API.GetScreensMESA(drv, disp, screens, max_screens, num_screens);

   return _EGL_EVAL(disp, ret);
}


EGLSurface
eglCreateScreenSurfaceMESA(EGLDisplay dpy, EGLConfig config,
                           const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLDriver *drv;
   _EGLSurface *surf;
   EGLSurface ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_NO_SURFACE);

   surf = drv->API.CreateScreenSurfaceMESA(drv, disp, conf, attrib_list);
   ret = (surf) ? _eglLinkSurface(surf, disp) : EGL_NO_SURFACE;

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglShowScreenSurfaceMESA(EGLDisplay dpy, EGLint screen,
                         EGLSurface surface, EGLModeMESA mode)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen((EGLScreenMESA) screen, disp);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGLMode *m = _eglLookupMode(mode, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   if (!surf && surface != EGL_NO_SURFACE)
      return _EGL_ERROR(disp, EGL_BAD_SURFACE, EGL_FALSE);
   if (!m && mode != EGL_NO_MODE_MESA)
      return _EGL_ERROR(disp, EGL_BAD_MODE_MESA, EGL_FALSE);

   ret = drv->API.ShowScreenSurfaceMESA(drv, disp, scrn, surf, m);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglScreenPositionMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLint x, EGLint y)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen(screen, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   ret = drv->API.ScreenPositionMESA(drv, disp, scrn, x, y);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglQueryScreenMESA(EGLDisplay dpy, EGLScreenMESA screen,
                   EGLint attribute, EGLint *value)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen(screen, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   ret = drv->API.QueryScreenMESA(drv, disp, scrn, attribute, value);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglQueryScreenSurfaceMESA(EGLDisplay dpy, EGLScreenMESA screen,
                          EGLSurface *surface)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen((EGLScreenMESA) screen, disp);
   _EGLDriver *drv;
   _EGLSurface *surf;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   ret = drv->API.QueryScreenSurfaceMESA(drv, disp, scrn, &surf);
   if (ret && surface)
      *surface = _eglGetSurfaceHandle(surf);

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglQueryScreenModeMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *mode)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLScreen *scrn = _eglLookupScreen((EGLScreenMESA) screen, disp);
   _EGLDriver *drv;
   _EGLMode *m;
   EGLBoolean ret;

   drv = _EGL_CHECK_SCREEN(disp, scrn, EGL_FALSE);
   ret = drv->API.QueryScreenModeMESA(drv, disp, scrn, &m);
   if (ret && mode)
      *mode = m->Handle;

   return _EGL_EVAL(disp, ret);
}


const char *
eglQueryModeStringMESA(EGLDisplay dpy, EGLModeMESA mode)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLMode *m = _eglLookupMode(mode, disp);
   _EGLDriver *drv;
   const char *ret;

   drv = _EGL_CHECK_MODE(disp, m, NULL);
   ret = drv->API.QueryModeStringMESA(drv, disp, m);

   return _EGL_EVAL(disp, ret);
}


#endif /* EGL_MESA_screen_surface */


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

   if (_eglIsCurrentThreadDummy())
      return _EGL_ERROR(NULL, EGL_BAD_ALLOC, EGL_FALSE);

   if (!_eglIsApiValid(api))
      return _EGL_ERROR(NULL, EGL_BAD_PARAMETER, EGL_FALSE);

   t->CurrentAPIIndex = _eglConvertApiToIndex(api);

   return _EGL_SUCCESS(NULL, EGL_TRUE);
}


/**
 * Return the last value set with eglBindAPI().
 */
EGLenum
eglQueryAPI(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLenum ret;

   /* returns one of EGL_OPENGL_API, EGL_OPENGL_ES_API or EGL_OPENVG_API */
   ret = _eglConvertApiFromIndex(t->CurrentAPIIndex);

   return _EGL_SUCCESS(NULL, ret);
}


EGLSurface
eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype,
                                 EGLClientBuffer buffer, EGLConfig config,
                                 const EGLint *attrib_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLDriver *drv;
   _EGLSurface *surf;
   EGLSurface ret;

   drv = _EGL_CHECK_CONFIG(disp, conf, EGL_NO_SURFACE);

   surf = drv->API.CreatePbufferFromClientBuffer(drv, disp, buftype, buffer,
                                                 conf, attrib_list);
   ret = (surf) ? _eglLinkSurface(surf, disp) : EGL_NO_SURFACE;

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglReleaseThread(void)
{
   /* unbind current contexts */
   if (!_eglIsCurrentThreadDummy()) {
      _EGLThreadInfo *t = _eglGetCurrentThread();
      EGLint api_index = t->CurrentAPIIndex;
      EGLint i;

      for (i = 0; i < _EGL_API_NUM_APIS; i++) {
         _EGLContext *ctx = t->CurrentContexts[i];
         if (ctx) {
            _EGLDisplay *disp = ctx->Resource.Display;
            _EGLDriver *drv = disp->Driver;
            t->CurrentAPIIndex = i;
            (void) drv->API.MakeCurrent(drv, disp, NULL, NULL, NULL);
         }
      }

      t->CurrentAPIIndex = api_index;
   }

   _eglDestroyCurrentThread();

   return _EGL_SUCCESS(NULL, EGL_TRUE);
}


#endif /* EGL_VERSION_1_2 */


#ifdef EGL_KHR_image_base


EGLImageKHR
eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                  EGLClientBuffer buffer, const EGLint *attr_list)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
   _EGLDriver *drv;
   _EGLImage *img;
   EGLImageKHR ret;

   drv = _EGL_CHECK_DISPLAY(disp, EGL_NO_IMAGE_KHR);
   if (!context && ctx != EGL_NO_CONTEXT)
      return _EGL_ERROR(disp, EGL_BAD_CONTEXT, EGL_NO_IMAGE_KHR);

   img = drv->API.CreateImageKHR(drv,
         disp, context, target, buffer, attr_list);
   ret = (img) ? _eglLinkImage(img, disp) : EGL_NO_IMAGE_KHR;

   return _EGL_EVAL(disp, ret);
}


EGLBoolean
eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLImage *img = _eglLookupImage(image, disp);
   _EGLDriver *drv;
   EGLBoolean ret;

   drv = _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
   if (!img)
      return _EGL_ERROR(disp, EGL_BAD_PARAMETER, EGL_FALSE);

   _eglUnlinkImage(img);
   ret = drv->API.DestroyImageKHR(drv, disp, img);

   return _EGL_EVAL(disp, ret);
}


#endif /* EGL_KHR_image_base */
