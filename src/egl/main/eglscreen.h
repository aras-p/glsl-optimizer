#ifndef EGLSCREEN_INCLUDED
#define EGLSCREEN_INCLUDED


/**
 * Per-screen information.
 * Note that an EGL screen doesn't have a size.  A screen may be set to
 * one of several display modes (width/height/scanrate).  The screen
 * then displays a drawing surface.  The drawing surface must be at least
 * as large as the display mode's resolution.  If it's larger, the
 * OriginX and OriginY fields control what part of the surface is visible
 * on the screen.
 */
struct _egl_screen
{
   EGLScreenMESA Handle; /* The public/opaque handle which names this object */

   _EGLMode *CurrentMode;
   _EGLSurface *CurrentSurface;

   EGLint OriginX, OriginY; /**< Origin of scan-out region w.r.t. surface */
   EGLint StepX, StepY;     /**< Screen position/origin granularity */

   EGLint NumModes;
   _EGLMode *Modes;  /**< array [NumModes] */
};


extern EGLScreenMESA
_eglAllocScreenHandle(void);


extern void
_eglInitScreen(_EGLScreen *screen);


extern _EGLScreen *
_eglLookupScreen(EGLDisplay dpy, EGLScreenMESA screen);


extern void
_eglAddScreen(_EGLDisplay *display, _EGLScreen *screen);


extern EGLBoolean
_eglGetScreensMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens);


extern EGLSurface
_eglCreateScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);


extern EGLBoolean
_eglShowScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLSurface surface, EGLModeMESA mode);


extern EGLBoolean
_eglScreenModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA mode);


extern EGLBoolean
_eglScreenPositionMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLint x, EGLint y);


extern EGLBoolean
_eglQueryDisplayMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglQueryScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy,
                           EGLScreenMESA screen, EGLSurface *surface);


extern EGLBoolean
_eglQueryScreenModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *mode);


extern EGLBoolean
_eglQueryScreenMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLint attribute, EGLint *value);


extern void
_eglDestroyScreenModes(_EGLScreen *scrn);


extern void
_eglDestroyScreen(_EGLScreen *scrn);


#endif /* EGLSCREEN_INCLUDED */
