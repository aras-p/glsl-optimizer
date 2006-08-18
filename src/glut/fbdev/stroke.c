/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * Copyright (C) 1995-2006  Brian Paul
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
 * Library for glut using mesa fbdev driver
 *
 * Written by Sean D'Epagnier (c) 2006
 * 
 * To improve on this library, maybe support subwindows or overlays,
 * I (sean at depagnier dot com) will do my best to help.
 */

#include <GL/glut.h>
#include "glutstroke.h"

void glutStrokeCharacter(GLUTstrokeFont font, int c)
{
   const StrokeCharRec *ch;
   const StrokeRec *stroke;
   const CoordRec *coord;
   StrokeFontPtr fontinfo = (StrokeFontPtr) font;
   int i, j;

   if (c < 0 || c >= fontinfo->num_chars)
      return;
   ch = &(fontinfo->ch[c]);
   if (ch) {
      for (i = ch->num_strokes, stroke = ch->stroke;
	   i > 0; i--, stroke++) {
	 glBegin(GL_LINE_STRIP);
	 for (j = stroke->num_coords, coord = stroke->coord;
	      j > 0; j--, coord++) {
	    glVertex2f(coord->x, coord->y);
	 }
	 glEnd();
      }
      glTranslatef(ch->right, 0.0, 0.0);
   }
}

int glutStrokeWidth(GLUTstrokeFont font, int c)
{
   StrokeFontPtr fontinfo;
   const StrokeCharRec *ch;

   fontinfo = (StrokeFontPtr) font;

   if (c < 0 || c >= fontinfo->num_chars)
      return 0;
   ch = &(fontinfo->ch[c]);
   if (ch)
      return ch->right;

   return 0;
}

int glutStrokeLength(GLUTstrokeFont font, const unsigned char *string)
{
   int length = 0;

   for (; *string; string++)
      length += glutStrokeWidth(font, *string);
   return length;
}
