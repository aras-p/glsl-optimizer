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


extern GLUTStrokeFont glutStrokeRoman, glutStrokeMonoRoman;
extern GLUTBitmapFont glutBitmap8By13, glutBitmap9By15, glutBitmapTimesRoman10, glutBitmapTimesRoman24, glutBitmapHelvetica10, glutBitmapHelvetica12, glutBitmapHelvetica18;

/* To get around the fact that DJGPP DXEs only allow functions
   to be exported and no data addresses (as Unix DSOs support), the
   GLUT API constants such as GLUT_STROKE_ROMAN have to get passed
   through a case statement to get mapped to the actual data structure
   address. */
void *
_glut_font (void *font)
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
      default:
         if ((font == &glutStrokeRoman) ||
             (font == &glutStrokeMonoRoman) ||
             (font == &glutBitmap9By15) ||
             (font == &glutBitmap8By13) ||
             (font == &glutBitmapTimesRoman10) ||
             (font == &glutBitmapTimesRoman24) ||
             (font == &glutBitmapHelvetica10) ||
             (font == &glutBitmapHelvetica12) ||
             (font == &glutBitmapHelvetica18)) {
            return font;
         }
         _glut_fatal("bad font!");
         return NULL;
   }
}
