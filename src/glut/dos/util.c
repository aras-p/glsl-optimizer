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
 * DOS/DJGPP glut driver v1.4 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "glutint.h"
#include "glutbitmap.h"
#include "glutstroke.h"


#ifdef GLUT_IMPORT_LIB
extern StrokeFontRec glutStrokeRoman, glutStrokeMonoRoman;
extern BitmapFontRec glutBitmap8By13, glutBitmap9By15, glutBitmapTimesRoman10, glutBitmapTimesRoman24, glutBitmapHelvetica10, glutBitmapHelvetica12, glutBitmapHelvetica18;

/* To get around the fact that DJGPP DXEs only allow functions
   to be exported and no data addresses (as Unix DSOs support), the
   GLUT API constants such as GLUT_STROKE_ROMAN have to get passed
   through a case statement to get mapped to the actual data structure
   address. */
void *
__glutFont (void *font)
{
   switch ((int)font) {
      case (int)GLUT_STROKE_ROMAN:
         return &glutStrokeRoman;
      case (int)GLUT_STROKE_MONO_ROMAN:
         return &glutStrokeMonoRoman;
      case (int)GLUT_BITMAP_9_BY_15:
         return &glutBitmap9By15;
      case (int)GLUT_BITMAP_8_BY_13:
         return &glutBitmap8By13;
      case (int)GLUT_BITMAP_TIMES_ROMAN_10:
         return &glutBitmapTimesRoman10;
      case (int)GLUT_BITMAP_TIMES_ROMAN_24:
         return &glutBitmapTimesRoman24;
      case (int)GLUT_BITMAP_HELVETICA_10:
         return &glutBitmapHelvetica10;
      case (int)GLUT_BITMAP_HELVETICA_12:
         return &glutBitmapHelvetica12;
      case (int)GLUT_BITMAP_HELVETICA_18:
         return &glutBitmapHelvetica18;
      default: /* NOTREACHED */
         __glutFatalError("bad font!");
         return NULL;
   }
}
#endif
