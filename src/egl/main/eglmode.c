#include "egldisplay.h"
#include "egldriver.h"
#include "eglmode.h"
#include "eglglobals.h"
#include "eglscreen.h"


#define MIN2(A, B)  (((A) < (B)) ? (A) : (B))


_EGLMode *
_eglLookupMode(EGLDisplay dpy, EGLModeMESA mode)
{
   const _EGLDisplay *disp = _eglLookupDisplay(dpy);
   EGLint scrnum;

   for (scrnum = 0; scrnum < disp->NumScreens; scrnum++) {
      const _EGLScreen *scrn = disp->Screens + scrnum;
      EGLint i;
      for (i = 0; i < scrn->NumModes; i++) {
         if (scrn->Modes[i].Handle == mode) {
            return scrn->Modes + i;
         }
      }
   }
   return NULL;
}


/**
 * Search for the EGLMode that best matches the given attribute list.
 */
EGLBoolean
_eglChooseModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                   const EGLint *attrib_list, EGLModeMESA *modes,
                   EGLint modes_size, EGLint *num_modes)
{
   EGLint i;

   /* XXX incomplete */

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_WIDTH:
         i++;
         break;
      case EGL_HEIGHT:
         i++;
         break;
      case EGL_REFRESH_RATE_MESA:
         i++;
         break;
#if 0
      case EGL_STEREO_MESA:
         i++;
         break;
#endif
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglChooseMode");
         return EGL_FALSE;
      }
   }

   return EGL_TRUE;
}



/**
 * Return all possible modes for the given screen
 */
EGLBoolean
_eglGetModesMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                 EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes)
{
   _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);
   EGLint i;

   if (!scrn) {
      _eglError(EGL_BAD_SCREEN_MESA, "eglGetModes");
      return EGL_FALSE;
   }

   *num_modes = MIN2(modes_size, scrn->NumModes);
   for (i = 0; i < *num_modes; i++) {
      modes[i] = scrn->Modes[i].Handle;
   }

   return EGL_TRUE;
}


/**
 * Query an attribute of a mode.
 */
EGLBoolean
_eglGetModeAttribMESA(_EGLDriver *drv, EGLDisplay dpy,
                      EGLModeMESA mode, EGLint attribute, EGLint *value)
{
   _EGLMode *m = _eglLookupMode(dpy, mode);

   switch (attribute) {
   case EGL_MODE_ID_MESA:
      *value = m->Handle;
      break;
   case EGL_WIDTH:
      *value = m->Width;
      break;
   case EGL_HEIGHT:
      *value = m->Height;
      break;
#if 0
   case EGL_DEPTH_MESA:
      *value = m->Depth;
      break;
#endif
   case EGL_REFRESH_RATE_MESA:
      *value = m->RefreshRate;
      break;
#if 0
   case EGL_STEREO_MESA:
      *value = m->Stereo;
      break;
#endif
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetModeAttrib");
      return EGL_FALSE;
   }
   return EGL_TRUE;
}
