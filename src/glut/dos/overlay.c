/*
 * DOS/DJGPP Mesa Utility Toolkit
 * Version:  1.0
 *
 * Copyright (C) 2005  Daniel Borca   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DANIEL BORCA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "internal.h"


int APIENTRY
glutLayerGet (GLenum info)
{
   switch (info) {
      case GLUT_OVERLAY_POSSIBLE:
      case GLUT_HAS_OVERLAY:
         return GL_FALSE;
      case GLUT_LAYER_IN_USE:
         return GLUT_NORMAL;
      case GLUT_NORMAL_DAMAGED:
         return GL_FALSE;
      case GLUT_OVERLAY_DAMAGED:
      case GLUT_TRANSPARENT_INDEX:
      default:
         return -1;
   }
}


void APIENTRY
glutOverlayDisplayFunc (GLUTdisplayCB func)
{
}


void APIENTRY
glutEstablishOverlay (void)
{
}


void APIENTRY
glutRemoveOverlay (void)
{
}


void APIENTRY
glutUseLayer (GLenum layer)
{
}


void APIENTRY
glutPostOverlayRedisplay (void)
{
}


void APIENTRY
glutShowOverlay (void)
{
}


void APIENTRY
glutHideOverlay (void)
{
}


void APIENTRY
glutPostWindowOverlayRedisplay (int win)
{
}
