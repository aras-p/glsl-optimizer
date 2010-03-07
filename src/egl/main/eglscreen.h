#ifndef EGLSCREEN_INCLUDED
#define EGLSCREEN_INCLUDED


#include "egltypedefs.h"


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


PUBLIC void
_eglInitScreen(_EGLScreen *screen);


extern _EGLScreen *
_eglLookupScreen(EGLScreenMESA screen, _EGLDisplay *dpy);


PUBLIC void
_eglAddScreen(_EGLDisplay *display, _EGLScreen *screen);


extern EGLBoolean
_eglGetScreensMESA(_EGLDriver *drv, _EGLDisplay *dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens);


extern _EGLSurface *
_eglCreateScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, const EGLint *attrib_list);


extern EGLBoolean
_eglShowScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, _EGLSurface *surf, _EGLMode *m);


extern EGLBoolean
_eglScreenModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, _EGLMode *m);


extern EGLBoolean
_eglScreenPositionMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, EGLint x, EGLint y);


extern EGLBoolean
_eglQueryDisplayMESA(_EGLDriver *drv, _EGLDisplay *dpy, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglQueryScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy,
                           _EGLScreen *scrn, _EGLSurface **surface);


extern EGLBoolean
_eglQueryScreenModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, _EGLMode **m);


extern EGLBoolean
_eglQueryScreenMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, EGLint attribute, EGLint *value);


extern void
_eglDestroyScreenModes(_EGLScreen *scrn);


PUBLIC void
_eglDestroyScreen(_EGLScreen *scrn);


#endif /* EGLSCREEN_INCLUDED */
