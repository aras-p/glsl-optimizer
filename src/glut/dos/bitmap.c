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


void APIENTRY
glutBitmapCharacter (void *font, int c)
{
   const GLUTBitmapFont *bfp = _glut_font(font);
   const GLUTBitmapChar *bcp;

   if (c >= bfp->num || !(bcp = bfp->table[c]))
      return;

   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

   glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glBitmap(bcp->width, bcp->height, bcp->xorig, bcp->yorig,
            bcp->xmove, 0, bcp->bitmap);

   glPopClientAttrib();
}


void APIENTRY
glutBitmapString (void *font, const unsigned char *string)
{
   const GLUTBitmapFont *bfp = _glut_font(font);
   const GLUTBitmapChar *bcp;
   unsigned char c;

   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

   glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   while ((c = *(string++))) {
      if (c < bfp->num && (bcp = bfp->table[c]))
         glBitmap(bcp->width, bcp->height, bcp->xorig,
                  bcp->yorig, bcp->xmove, 0, bcp->bitmap);
   }

   glPopClientAttrib();
}


int APIENTRY
glutBitmapWidth (void *font, int c)
{
   const GLUTBitmapFont *bfp = _glut_font(font);
   const GLUTBitmapChar *bcp;

   if (c >= bfp->num || !(bcp = bfp->table[c]))
      return 0;

   return bcp->xmove;
}


int APIENTRY
glutBitmapLength (void *font, const unsigned char *string)
{
   const GLUTBitmapFont *bfp = _glut_font(font);
   const GLUTBitmapChar *bcp;
   unsigned char c;
   int length = 0;

   while ((c = *(string++))) {
      if (c < bfp->num && (bcp = bfp->table[c]))
         length += bcp->xmove;
   }

   return length;
}


int APIENTRY
glutBitmapHeight (void *font)
{
   const GLUTBitmapFont *bfp = _glut_font(font);

   return bfp->height;
}
