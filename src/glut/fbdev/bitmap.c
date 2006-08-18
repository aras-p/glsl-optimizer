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


#include "glutbitmap.h"

void glutBitmapCharacter(GLUTbitmapFont font, int c)
{
   const BitmapCharRec *ch;
   BitmapFontPtr fi = (BitmapFontPtr) font;

   if (c < fi->first ||
       c >= fi->first + fi->num_chars)
      return;
   ch = fi->ch[c - fi->first];
   if (!ch)
      return;

   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

   glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
   glBitmap(ch->width, ch->height, ch->xorig, ch->yorig,
	    ch->advance, 0, ch->bitmap);
   glPopClientAttrib();
}

int glutBitmapWidth (GLUTbitmapFont font, int c)
{
   const BitmapCharRec *ch;
   BitmapFontPtr fi = (BitmapFontPtr) font;

   if (c < fi->first || c >= fi->first + fi->num_chars)
      return 0;
   ch = fi->ch[c - fi->first];
   if (ch)
      return ch->advance;
   return 0;
}

int glutBitmapLength(GLUTbitmapFont font, const unsigned char *string)
{
   int length = 0;

   for (; *string; string++)
      length += glutBitmapWidth(font, *string);
   return length;
}
