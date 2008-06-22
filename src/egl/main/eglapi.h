#ifndef EGLAPI_INCLUDED
#define EGLAPI_INCLUDED

/**
 * A generic function ptr type
 */
typedef void (*_EGLProc)();


/**
 * Typedefs for all EGL API entrypoint functions.
 */

/* driver funcs */
typedef EGLBoolean (*Initialize_t)(_EGLDriver *, EGLDisplay dpy, EGLint *major, EGLint *minor);
typedef EGLBoolean (*Terminate_t)(_EGLDriver *, EGLDisplay dpy);

/* config funcs */
typedef EGLBoolean (*GetConfigs_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLBoolean (*ChooseConfig_t)(_EGLDriver *drv, EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLBoolean (*GetConfigAttrib_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);

/* context funcs */
typedef EGLContext (*CreateContext_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list);
typedef EGLBoolean (*DestroyContext_t)(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx);
typedef EGLBoolean (*MakeCurrent_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
typedef EGLBoolean (*QueryContext_t)(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);

/* surface funcs */
typedef EGLSurface (*CreateWindowSurface_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list);
typedef EGLSurface (*CreatePixmapSurface_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list);
typedef EGLSurface (*CreatePbufferSurface_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
typedef EGLBoolean (*DestroySurface_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface);
typedef EGLBoolean (*QuerySurface_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
typedef EGLBoolean (*SurfaceAttrib_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
typedef EGLBoolean (*BindTexImage_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint buffer);
typedef EGLBoolean (*ReleaseTexImage_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint buffer);
typedef EGLBoolean (*SwapInterval_t)(_EGLDriver *drv, EGLDisplay dpy, EGLint interval);
typedef EGLBoolean (*SwapBuffers_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw);
typedef EGLBoolean (*CopyBuffers_t)(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, NativePixmapType target);

/* misc funcs */
typedef const char *(*QueryString_t)(_EGLDriver *drv, EGLDisplay dpy, EGLint name);
typedef EGLBoolean (*WaitGL_t)(_EGLDriver *drv, EGLDisplay dpy);
typedef EGLBoolean (*WaitNative_t)(_EGLDriver *drv, EGLDisplay dpy, EGLint engine);

typedef _EGLProc (*GetProcAddress_t)(const char *procname);



#ifdef EGL_MESA_screen_surface
typedef EGLBoolean (*ChooseModeMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, const EGLint *attrib_list, EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);
typedef EGLBoolean (*GetModesMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *modes, EGLint mode_size, EGLint *num_mode);
typedef EGLBoolean (*GetModeAttribMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLModeMESA mode, EGLint attribute, EGLint *value);
typedef EGLBoolean (*CopyContextMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLContext source, EGLContext dest, EGLint mask);
typedef EGLBoolean (*GetScreensMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens);
typedef EGLSurface (*CreateScreenSurfaceMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
typedef EGLBoolean (*ShowScreenSurfaceMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLSurface surface, EGLModeMESA mode);
typedef EGLBoolean (*ScreenPositionMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLint x, EGLint y);
typedef EGLBoolean (*QueryScreenMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLint attribute, EGLint *value);
typedef EGLBoolean (*QueryScreenSurfaceMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLSurface *surface);
typedef EGLBoolean (*QueryScreenModeMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *mode);
typedef const char * (*QueryModeStringMESA_t)(_EGLDriver *drv, EGLDisplay dpy, EGLModeMESA mode);
#endif /* EGL_MESA_screen_surface */


#ifdef EGL_VERSION_1_2
typedef EGLBoolean (*WaitClient_t)(_EGLDriver *drv, EGLDisplay dpy);
typedef EGLSurface (*CreatePbufferFromClientBuffer_t)(_EGLDriver *drv, EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
#endif /* EGL_VERSION_1_2 */



/**
 * The API dispatcher jumps through these functions
 */
struct _egl_api
{
   Initialize_t Initialize;
   Terminate_t Terminate;

   GetConfigs_t GetConfigs;
   ChooseConfig_t ChooseConfig;
   GetConfigAttrib_t GetConfigAttrib;

   CreateContext_t CreateContext;
   DestroyContext_t DestroyContext;
   MakeCurrent_t MakeCurrent;
   QueryContext_t QueryContext;

   CreateWindowSurface_t CreateWindowSurface;
   CreatePixmapSurface_t CreatePixmapSurface;
   CreatePbufferSurface_t CreatePbufferSurface;
   DestroySurface_t DestroySurface;
   QuerySurface_t QuerySurface;
   SurfaceAttrib_t SurfaceAttrib;
   BindTexImage_t BindTexImage;
   ReleaseTexImage_t ReleaseTexImage;
   SwapInterval_t SwapInterval;
   SwapBuffers_t SwapBuffers;
   CopyBuffers_t CopyBuffers;

   QueryString_t QueryString;
   WaitGL_t WaitGL;
   WaitNative_t WaitNative;
   GetProcAddress_t GetProcAddress;

   /* EGL_MESA_screen extension */
   ChooseModeMESA_t ChooseModeMESA;
   GetModesMESA_t GetModesMESA;
   GetModeAttribMESA_t GetModeAttribMESA;
   CopyContextMESA_t CopyContextMESA;
   GetScreensMESA_t GetScreensMESA;
   CreateScreenSurfaceMESA_t CreateScreenSurfaceMESA;
   ShowScreenSurfaceMESA_t ShowScreenSurfaceMESA;
   ScreenPositionMESA_t ScreenPositionMESA;
   QueryScreenMESA_t QueryScreenMESA;
   QueryScreenSurfaceMESA_t QueryScreenSurfaceMESA;
   QueryScreenModeMESA_t QueryScreenModeMESA;
   QueryModeStringMESA_t QueryModeStringMESA;

#ifdef EGL_VERSION_1_2
   WaitClient_t WaitClient;
   CreatePbufferFromClientBuffer_t CreatePbufferFromClientBuffer;
#endif
};

#endif /* EGLAPI_INCLUDED */
