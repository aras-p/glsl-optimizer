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


#define CLAMP(i) ((i) > 1.0F ? 1.0F : ((i) < 0.0F ? 0.0F : (i)))


void APIENTRY
glutSetColor (int ndx, GLfloat red, GLfloat green, GLfloat blue)
{
   if (_glut_default.mode & GLUT_INDEX) {
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
