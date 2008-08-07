
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include "glutint.h"

/* CENTRY */
void GLUTAPIENTRY
glutSetWindowTitle(const char *title)
{
#if defined(__OS2PM__)
   __glutSetWindowText(__glutCurrentWindow->win, (char *)title);

#else
  XTextProperty textprop;

  assert(!__glutCurrentWindow->parent);
  IGNORE_IN_GAME_MODE();
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  XSetWMName(__glutDisplay,
    __glutCurrentWindow->win, &textprop);
  XFlush(__glutDisplay);
#endif
}

void GLUTAPIENTRY
glutSetIconTitle(const char *title)
{
#if defined(__OS2PM__)
//todo ?
#else

  XTextProperty textprop;

  assert(!__glutCurrentWindow->parent);
  IGNORE_IN_GAME_MODE();
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  XSetWMIconName(__glutDisplay,
    __glutCurrentWindow->win, &textprop);
  XFlush(__glutDisplay);
#endif
}

void GLUTAPIENTRY
glutPositionWindow(int x, int y)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredX = x;
  __glutCurrentWindow->desiredY = y;
  __glutCurrentWindow->desiredConfMask |= CWX | CWY;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void GLUTAPIENTRY
glutReshapeWindow(int w, int h)
{
  IGNORE_IN_GAME_MODE();
  if (w <= 0 || h <= 0)
    __glutWarning("glutReshapeWindow: non-positive width or height not allowed");

  __glutCurrentWindow->desiredWidth = w;
  __glutCurrentWindow->desiredHeight = h;
  __glutCurrentWindow->desiredConfMask |= CWWidth | CWHeight;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void GLUTAPIENTRY
glutPopWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredStack = Above;
  __glutCurrentWindow->desiredConfMask |= CWStackMode;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void GLUTAPIENTRY
glutPushWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredStack = Below;
  __glutCurrentWindow->desiredConfMask |= CWStackMode;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void GLUTAPIENTRY
glutIconifyWindow(void)
{
  IGNORE_IN_GAME_MODE();
  assert(!__glutCurrentWindow->parent);
  __glutCurrentWindow->desiredMapState = IconicState;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

void GLUTAPIENTRY
glutShowWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredMapState = NormalState;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

void GLUTAPIENTRY
glutHideWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredMapState = WithdrawnState;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

/* ENDCENTRY */
