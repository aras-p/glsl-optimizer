/*
 * Ideas for screen management extension to EGL.
 *
 * Each EGLDisplay has one or more screens (CRTs, Flat Panels, etc).
 * The screens' handles can be obtained with eglGetScreensMESA().
 *
 * A new kind of EGLSurface is possible- one which can be directly scanned
 * out on a screen.  Such a surface is created with eglCreateScreenSurface().
 *
 * To actually display a screen surface on a screen, the eglShowSurface()
 * function is called.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "egldisplay.h"
#include "eglcurrent.h"
#include "eglmode.h"
#include "eglconfig.h"
#include "eglsurface.h"
#include "eglscreen.h"
#include "eglmutex.h"


#ifdef EGL_MESA_screen_surface


/* ugh, no atomic op? */
static _EGL_DECLARE_MUTEX(_eglNextScreenHandleMutex);
static EGLScreenMESA _eglNextScreenHandle = 1;


/**
 * Return a new screen handle/ID.
 * NOTE: we never reuse these!
 */
static EGLScreenMESA
_eglAllocScreenHandle(void)
{
   EGLScreenMESA s;

   _eglLockMutex(&_eglNextScreenHandleMutex);
   s = _eglNextScreenHandle++;
   _eglUnlockMutex(&_eglNextScreenHandleMutex);

   return s;
}


/**
 * Initialize an _EGLScreen object to default values.
 */
void
_eglInitScreen(_EGLScreen *screen)
{
   memset(screen, 0, sizeof(_EGLScreen));
   screen->StepX = 1;
   screen->StepY = 1;
}


/**
 * Given a public screen handle, return the internal _EGLScreen object.
 */
_EGLScreen *
_eglLookupScreen(EGLScreenMESA screen, _EGLDisplay *display)
{
   EGLint i;

   if (!display || !display->Screens)
      return NULL;

   for (i = 0; i < display->Screens->Size; i++) {
      _EGLScreen *scr = (_EGLScreen *) display->Screens->Elements[i];
      if (scr->Handle == screen)
         return scr;
   }
   return NULL;
}


/**
 * Add the given _EGLScreen to the display's list of screens.
 */
void
_eglAddScreen(_EGLDisplay *display, _EGLScreen *screen)
{
   assert(display);
   assert(screen);

   if (!display->Screens) {
      display->Screens = _eglCreateArray("Screen", 4);
      if (!display->Screens)
         return;
   }
   screen->Handle = _eglAllocScreenHandle();
   _eglAppendArray(display->Screens, (void *) screen);
}



static EGLBoolean
_eglFlattenScreen(void *elem, void *buffer)
{
   _EGLScreen *scr = (_EGLScreen *) elem;
   EGLScreenMESA *handle = (EGLScreenMESA *) buffer;
   *handle = scr->Handle;
   return EGL_TRUE;
}


EGLBoolean
_eglGetScreensMESA(_EGLDriver *drv, _EGLDisplay *display, EGLScreenMESA *screens,
                   EGLint max_screens, EGLint *num_screens)
{
   *num_screens = _eglFlattenArray(display->Screens, (void *) screens,
         sizeof(screens[0]), max_screens, _eglFlattenScreen);

   return EGL_TRUE;
}


/**
 * Drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreateScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                            const EGLint *attrib_list)
{
   return NULL;
}


/**
 * Show the given surface on the named screen.
 * If surface is EGL_NO_SURFACE, disable the screen's output.
 * 
 * This is just a placeholder function; drivers will always override
 * this with code that _really_ shows the surface.
 */
EGLBoolean
_eglShowScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy,
                          _EGLScreen *scrn, _EGLSurface *surf,
                          _EGLMode *mode)
{
   if (!surf) {
      scrn->CurrentSurface = NULL;
   }
   else {
      if (surf->Type != EGL_SCREEN_BIT_MESA) {
         _eglError(EGL_BAD_SURFACE, "eglShowSurfaceMESA");
         return EGL_FALSE;
      }
      if (surf->Width < mode->Width || surf->Height < mode->Height) {
         _eglError(EGL_BAD_SURFACE,
                   "eglShowSurfaceMESA(surface smaller than screen size)");
         return EGL_FALSE;
      }

      scrn->CurrentSurface = surf;
      scrn->CurrentMode = mode;
   }
   return EGL_TRUE;
}


/**
 * Set a screen's current display mode.
 * Note: mode = EGL_NO_MODE is valid (turns off the screen)
 *
 * This is just a placeholder function; drivers will always override
 * this with code that _really_ sets the mode.
 */
EGLBoolean
_eglScreenModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                   _EGLMode *m)
{
   scrn->CurrentMode = m;
   return EGL_TRUE;
}


/**
 * Set a screen's surface origin.
 */
EGLBoolean
_eglScreenPositionMESA(_EGLDriver *drv, _EGLDisplay *dpy,
                       _EGLScreen *scrn, EGLint x, EGLint y)
{
   scrn->OriginX = x;
   scrn->OriginY = y;

   return EGL_TRUE;
}


/**
 * Query a screen's current surface.
 */
EGLBoolean
_eglQueryScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy,
                           _EGLScreen *scrn, _EGLSurface **surf)
{
   *surf = scrn->CurrentSurface;
   return EGL_TRUE;
}


/**
 * Query a screen's current mode.
 */
EGLBoolean
_eglQueryScreenModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                        _EGLMode **m)
{
   *m = scrn->CurrentMode;
   return EGL_TRUE;
}


EGLBoolean
_eglQueryScreenMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                    EGLint attribute, EGLint *value)
{
   switch (attribute) {
   case EGL_SCREEN_POSITION_MESA:
      value[0] = scrn->OriginX;
      value[1] = scrn->OriginY;
      break;
   case EGL_SCREEN_POSITION_GRANULARITY_MESA:
      value[0] = scrn->StepX;
      value[1] = scrn->StepY;
      break;
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQueryScreenMESA");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}


/**
 * Delete the modes associated with given screen.
 */
void
_eglDestroyScreenModes(_EGLScreen *scrn)
{
   EGLint i;
   for (i = 0; i < scrn->NumModes; i++) {
      if (scrn->Modes[i].Name)
         free((char *) scrn->Modes[i].Name); /* cast away const */
   }
   if (scrn->Modes)
      free(scrn->Modes);
   scrn->Modes = NULL;
   scrn->NumModes = 0;
}

      
/**
 * Default fallback routine - drivers should usually override this.
 */
void
_eglDestroyScreen(_EGLScreen *scrn)
{
   _eglDestroyScreenModes(scrn);
   free(scrn);
}


#endif /* EGL_MESA_screen_surface */
