#ifndef EGLMODE_INCLUDED
#define EGLMODE_INCLUDED

#include "egltypedefs.h"


#define EGL_NO_MODE_MESA 0


/**
 * Data structure which corresponds to an EGLModeMESA.
 */
struct _egl_mode
{
   EGLModeMESA Handle;     /* the public/opaque handle which names this mode */
   EGLint Width, Height;   /* size in pixels */
   EGLint RefreshRate;     /* rate * 1000.0 */
   EGLint Optimal;
   EGLint Interlaced;
   const char *Name;

   /* Other possible attributes */
   /* interlaced */
   /* external sync */
};


extern _EGLMode *
_eglLookupMode(EGLModeMESA mode, _EGLDisplay *dpy);


PUBLIC _EGLMode *
_eglAddNewMode(_EGLScreen *screen, EGLint width, EGLint height,
               EGLint refreshRate, const char *name);


extern EGLBoolean
_eglChooseModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                   const EGLint *attrib_list, EGLModeMESA *modes,
                   EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModesMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                 EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModeAttribMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *m,
                      EGLint attribute, EGLint *value);


extern const char *
_eglQueryModeStringMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *m);


#endif /* EGLMODE_INCLUDED */
