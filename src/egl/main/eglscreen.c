/*
 * Ideas for screen management extension to EGL.
 *
 * Each EGLDisplay has one or more screens (CRTs, Flat Panels, etc).
 * The number of screens can be queried with eglQueryDisplay(EGL_SCREEN_COUNT).
 *
 * A new kind of EGLSurface is possible- one which can be directly scanned
 * out on a screen.  Such a surface is created with eglCreateScreenSurface().
 *
 * To actually display a screen surface on a screen, the eglShowSurface()
 * function is called.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include "egldisplay.h"
#include "eglglobals.h"
#include "eglmode.h"
#include "eglsurface.h"
#include "eglscreen.h"


_EGLScreen *
_eglLookupScreen(EGLDisplay dpy, GLint screenNumber)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   if (!disp || screenNumber < 0 || screenNumber >= disp->NumScreens) {
      return NULL;
   }
   else {
      return disp->Screens + screenNumber;
   }
}


/**
 * Create a drawing surface which can be directly displayed on a screen.
 */
EGLSurface
_eglCreateScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                            const EGLint *attrib_list)
{
   _EGLSurface *surf;
   EGLint width = 0, height = 0;
   EGLint i;

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_WIDTH:
         width = attrib_list[++i];
         break;
      case EGL_HEIGHT:
         height = attrib_list[++i];
         break;
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateScreenSurfaceMESA");
         return EGL_NO_SURFACE;
      }
   }

   if (width <= 0 || height <= 0) {
      _eglError(EGL_BAD_ATTRIBUTE,
                "eglCreateScreenSurfaceMESA(width or height)");
      return EGL_NO_SURFACE;
   }

   surf = (_EGLSurface *) malloc(sizeof(_EGLSurface));
   _eglInitSurface(surf);
   surf->Width = width;
   surf->Height = height;
   surf->Type = EGL_SCREEN_BIT_MESA;

   /* insert into hash table */
   _eglSaveSurface(surf);
   assert(surf->Handle);

   return surf->Handle;
}


/**
 * Show the given surface on the named screen.
 * If surface is EGL_NO_SURFACE, disable the screen's output.
 * 
 * This is just a placeholder function; drivers will always override
 * this with code that _really_ shows the surface.
 */
EGLBoolean
_eglShowSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                    EGLSurface surface)
{
   _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);
   _EGLMode *mode;

   if (!scrn) {
      _eglError(EGL_BAD_SCREEN_MESA, "eglShowSurfaceMESA");
      return EGL_FALSE;
   }

   /*
    * XXX: Check if the surface's configuration is compatible with the
    * current screen mode.
    */

   mode = scrn->CurrentMode;
   if (mode == EGL_NO_MODE_MESA) {
      _eglError(EGL_BAD_MODE_MESA, "eglShowSurfaceMESA(no current mode)");
      return EGL_FALSE;
   }

   if (surface == EGL_NO_SURFACE) {
      scrn->CurrentSurface = NULL;
   }
   else {
      _EGLSurface *surf = _eglLookupSurface(surface);
      if (!surf || surf->Type != EGL_SCREEN_BIT_MESA) {
         _eglError(EGL_BAD_SURFACE, "eglShowSurfaceMESA");
         return EGL_FALSE;
      }
      if (surf->Width < mode->Width || surf->Height < mode->Height) {
         _eglError(EGL_BAD_SURFACE,
                   "eglShowSurfaceMESA(surface smaller than screen size)");
         return EGL_FALSE;
      }

      scrn->CurrentSurface = surf;
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
_eglScreenModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                   EGLModeMESA mode)
{
   _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);

   if (!scrn) {
      _eglError(EGL_BAD_SCREEN_MESA, "eglScreenModeMESA");
      return EGL_FALSE;
   }

   scrn->CurrentMode = _eglLookupMode(dpy, mode);

   return EGL_TRUE;
}


/**
 * Set a screen's surface origin.
 */
EGLBoolean
_eglScreenPositionMESA(_EGLDriver *drv, EGLDisplay dpy,
                       EGLint screen_number, EGLint x, EGLint y)
{
   _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);
   if (!scrn) {
      _eglError(EGL_BAD_SCREEN_MESA, "eglScreenPositionMESA");
      return EGL_FALSE;
   }

   scrn->OriginX = x;
   scrn->OriginY = y;

   return EGL_TRUE;
}


EGLBoolean
_eglQueryDisplayMESA(_EGLDriver *drv, EGLDisplay dpy,
                     EGLint attribute, EGLint *value)
{
   const _EGLDisplay *display = _eglLookupDisplay(dpy);
   switch (attribute) {
   case EGL_SCREEN_COUNT_MESA:
      *value = display->NumScreens;
      break;
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQueryDisplayMESA");
      return EGL_FALSE;
   }
   return EGL_TRUE;
}


/**
 * Query a screen's current surface.
 */
EGLBoolean
_eglQueryScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy,
                           EGLint screen_number, EGLSurface *surface)
{
   const _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);
   if (scrn->CurrentSurface)
      *surface = scrn->CurrentSurface->Handle;
   else
      *surface = EGL_NO_SURFACE;
   return EGL_TRUE;
}


/**
 * Query a screen's current mode.
 */
EGLBoolean
_eglQueryScreenModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                        EGLModeMESA *mode)
{
   const _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);
   if (scrn->CurrentMode)
      *mode = scrn->CurrentMode->Handle;
   else
      *mode = EGL_NO_MODE_MESA;
   return EGL_TRUE;
}


EGLBoolean
_eglQueryScreenMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint screen_number,
                    EGLint attribute, EGLint *value)
{
   const _EGLScreen *scrn = _eglLookupScreen(dpy, screen_number);

   if (!scrn) {
      _eglError(EGL_BAD_SCREEN_MESA, "eglQueryScreenMESA");
      return EGL_FALSE;
   }

   switch (attribute) {
   case EGL_SCREEN_POSITION_MESA:
      value[0] = scrn->OriginX;
      value[1] = scrn->OriginY;
      break;
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQueryScreenMESA");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}



void
_eglDeleteScreen(_EGLScreen *scrn)
{
   free(scrn->Modes);
   free(scrn);
}

