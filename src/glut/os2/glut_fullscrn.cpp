
/* Copyright (c) Mark J. Kilgard, 1995, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <assert.h>

#include "glutint.h"

/* CENTRY */
void GLUTAPIENTRY
glutFullScreen(void)
{
  assert(!__glutCurrentWindow->parent);
  IGNORE_IN_GAME_MODE();
#if !defined(_WIN32) && !defined(__OS2PM__)
  if (__glutMotifHints == None) {
    __glutMotifHints = XSGIFastInternAtom(__glutDisplay, "_MOTIF_WM_HINTS",
      SGI_XA__MOTIF_WM_HINTS, 0);
    if (__glutMotifHints == None) {
      __glutWarning("Could not intern X atom for _MOTIF_WM_HINTS.");
    }
  }
#endif

  __glutCurrentWindow->desiredX = 0;
  __glutCurrentWindow->desiredY = 0;
  __glutCurrentWindow->desiredWidth = __glutScreenWidth;
  __glutCurrentWindow->desiredHeight = __glutScreenHeight;
  __glutCurrentWindow->desiredConfMask |= CWX | CWY | CWWidth | CWHeight;

  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_CONFIGURE_WORK | GLUT_FULL_SCREEN_WORK);
}

/* ENDCENTRY */
