/*
 * Mesa 3-D graphics library
 * Version:  3.4
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * DOS/DJGPP glut driver v1.3 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "glutint.h"


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
