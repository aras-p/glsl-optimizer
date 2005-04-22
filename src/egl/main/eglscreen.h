#ifndef EGLSCREEN_INCLUDED
#define EGLSCREEN_INCLUDED


/* NOTE: there is no public EGLScreen type, we refers to screens with
 * an integer.
 */

struct _egl_screen
{
   _EGLMode *CurrentMode;
   _EGLSurface *CurrentSurface;
   EGLint OriginX, OriginY;

   EGLint NumModes;
   _EGLMode *Modes;  /* array [NumModes] */
};


extern _EGLScreen *
_eglLookupScreen(EGLDisplay dpy, GLint screenNumber);


extern EGLSurface
_eglCreateScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);


extern EGLBoolean
_eglShowSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen, EGLSurface surface);


extern EGLBoolean
_eglScreenModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number, EGLModeMESA mode);


extern EGLBoolean
_eglScreenPositionMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number, EGLint x, EGLint y);


extern EGLBoolean
_eglQueryDisplayMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglQueryScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy,
                           EGLint screen_number, EGLSurface *surface);


extern EGLBoolean
_eglQueryScreenModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number, EGLModeMESA *mode);


extern EGLBoolean
_eglQueryScreenMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number, EGLint attribute, EGLint *value);


extern void
_eglDeleteScreen(_EGLScreen *scrn);


#endif /* EGLSCREEN_INCLUDED */
