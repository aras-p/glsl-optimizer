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
#include "GL/dmesa.h"


#define CLAMP(i) ((i) > 1.0F ? 1.0F : ((i) < 0.0F ? 0.0F : (i)))


void APIENTRY
glutSetColor (int ndx, GLfloat red, GLfloat green, GLfloat blue)
{
   if (g_display_mode & GLUT_INDEX) {
      if ((ndx >= 0) && (ndx < (256 - RESERVED_COLORS))) {
         DMesaSetCI(ndx, CLAMP(red), CLAMP(green), CLAMP(blue));
      }
   }
}


GLfloat APIENTRY
glutGetColor (int ndx, int component)
{
   return 0.0;
}


void APIENTRY
glutCopyColormap (int win)
{
}
