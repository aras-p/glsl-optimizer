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
_eglInitScreen(_EGLScreen *screen, _EGLDisplay *dpy)
{
   memset(screen, 0, sizeof(_EGLScreen));
   screen->Display = dpy;
   screen->StepX = 1;
   screen->StepY = 1;
}


/**
 * Link a screen to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
EGLScreenMESA
_eglLinkScreen(_EGLScreen *screen)
{
   _EGLDisplay *display;

   assert(screen && screen->Display);
   display = screen->Display;

   if (!display->Screens) {
      display->Screens = _eglCreateArray("Screen", 4);
      if (!display->Screens)
         return (EGLScreenMESA) 0;
   }
   screen->Handle = _eglAllocScreenHandle();
   _eglAppendArray(display->Screens, (void *) screen);

   return screen->Handle;
}


/**
 * Lookup a handle to find the linked config.
 * Return NULL if the handle has no corresponding linked config.
 */
_EGLScreen *
_eglLookupScreen(EGLScreenMESA screen, _EGLDisplay *display)
{
   EGLint i;

   if (!display || !display->Screens)
      return NULL;

   for (i = 0; i < display->Screens->Size; i++) {
      _EGLScreen *scr = (_EGLScreen *) display->Screens->Elements[i];
      if (scr->Handle == screen) {
         assert(scr->Display == display);
         return scr;
      }
   }
   return NULL;
}


static EGLBoolean
_eglFlattenScreen(void *elem, void *buffer)
{
   _EGLScreen *scr = (_EGLScreen *) elem;
   EGLScreenMESA *handle = (EGLScreenMESA *) buffer;
   *handle = _eglGetScreenHandle(scr);
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


#endif /* EGL_MESA_screen_surface */
