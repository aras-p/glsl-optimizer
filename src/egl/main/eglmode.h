#ifndef EGLMODE_INCLUDED
#define EGLMODE_INCLUDED

#include "egltypedefs.h"

struct _egl_mode
{
   EGLConfig Handle;       /* the public/opaque handle which names this mode */
   EGLint Width, Height;   /* size in pixels */
   EGLint Depth;           /* bits per pixel */
   EGLint RefreshRate;     /* rate * 1000.0 */
   EGLBoolean Stereo;

   /* Other possible attributes */
   /* interlaced */
   /* external sync */
};


extern _EGLMode *
_eglLookupMode(EGLDisplay dpy, EGLModeMESA mode);


extern EGLBoolean
_eglChooseModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                   const EGLint *attrib_list, EGLModeMESA *modes,
                   EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModesMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                 EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModeAttribMESA(_EGLDriver *drv, EGLDisplay dpy, EGLModeMESA mode,
                      EGLint attribute, EGLint *value);



#endif /* EGLMODE_INCLUDED */
