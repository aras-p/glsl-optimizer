/*
 * Mesa 3-D graphics library
 * Version:  4.0
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
 * DOS/DJGPP glut driver v1.0 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "GL/glut.h"
#include "internal.h"


void APIENTRY glutInit (int *argcp, char **argv)
{
 glutGet(GLUT_ELAPSED_TIME);
}


void APIENTRY glutInitDisplayMode (unsigned int mode)
{
 g_display_mode = mode;
}


void APIENTRY glutInitWindowPosition (int x, int y)
{
 g_xpos = x;
 g_ypos = y;
}


void APIENTRY glutInitWindowSize (int width, int height)
{
 g_width  = width;
 g_height = height;
}


