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
 */

/* these routines are written to access graphics memory directly, not using mesa
   to render the cursor, this is faster, it would be good to use a hardware
   cursor if it exists instead */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <linux/fb.h>

#include <GL/glut.h>

#include "internal.h"
#include "cursors.h"

int CurrentCursor = GLUT_CURSOR_LEFT_ARROW;

static int LastMouseX, LastMouseY;
static unsigned char *MouseBuffer;

void InitializeCursor(void)
{
   if(!MouseBuffer && (MouseBuffer = malloc(CURSOR_WIDTH * CURSOR_HEIGHT
			    * VarInfo.bits_per_pixel / 8)) == NULL) {
      sprintf(exiterror, "malloc failure\n");
      exit(0);
   }

   MouseX = VarInfo.xres / 2;
   MouseY = VarInfo.yres / 2;
}

void EraseCursor(void)
{
   int off = LastMouseY * FixedInfo.line_length 
      + LastMouseX * VarInfo.bits_per_pixel / 8;
   int stride = CURSOR_WIDTH * VarInfo.bits_per_pixel / 8;
   int i;

   unsigned char *src = MouseBuffer;

   if(!MouseVisible || CurrentCursor < 0 || CurrentCursor >= NUM_CURSORS)
      return;

   for(i = 0; i<CURSOR_HEIGHT; i++) {
      memcpy(BackBuffer + off, src, stride);
      src += stride;
      off += FixedInfo.line_length;
   }
}

static void SaveCursor(int x, int y)
{
   int bypp, off, stride, i;
   unsigned char *src = MouseBuffer;

   if(x < 0)
      LastMouseX = 0;
   else
      if(x > (int)VarInfo.xres - CURSOR_WIDTH)
	 LastMouseX = VarInfo.xres - CURSOR_WIDTH;
      else
	 LastMouseX = x;

   if(y < 0)
      LastMouseY = 0;
   else
      if(y > (int)VarInfo.yres - CURSOR_HEIGHT)
	 LastMouseY = VarInfo.yres - CURSOR_HEIGHT;
      else
	 LastMouseY = y;

   bypp = VarInfo.bits_per_pixel / 8;
   off = LastMouseY * FixedInfo.line_length + LastMouseX * bypp;
   stride = CURSOR_WIDTH * bypp;
   for(i = 0; i<CURSOR_HEIGHT; i++) {
      memcpy(src, BackBuffer + off, stride);
      src += stride;
      off += FixedInfo.line_length;
   }
}

void DrawCursor(void)
{
   int i, j, px, py, xoff, xlen, yoff, ylen, bypp, cstride, dstride;
   unsigned char *c;
   const unsigned char *d;

   if(!MouseVisible || CurrentCursor < 0 || CurrentCursor >= NUM_CURSORS)
      return;

   px = MouseX - CursorsXOffset[CurrentCursor];
   py = MouseY - CursorsYOffset[CurrentCursor];

   SaveCursor(px, py);

   xoff = 0;
   if(px < 0)
      xoff = -px;

   xlen = CURSOR_WIDTH;
   if(px + CURSOR_WIDTH > VarInfo.xres)
      xlen = VarInfo.xres - px;

   yoff = 0;
   if(py < 0)
      yoff = -py;

   ylen = CURSOR_HEIGHT;
   if(py + CURSOR_HEIGHT > VarInfo.yres)
      ylen = VarInfo.yres - py;

   bypp = VarInfo.bits_per_pixel / 8;

   c = BackBuffer + FixedInfo.line_length * (py + yoff) + (px + xoff) * bypp;
   cstride = FixedInfo.line_length - bypp * (xlen - xoff);

   d = Cursors[CurrentCursor] + (CURSOR_WIDTH * yoff + xoff)*4;
   dstride = (CURSOR_WIDTH - xlen + xoff) * 4;

   switch(bypp) {
   case 1:
      {
	 const int shift = 8 - REVERSECMAPSIZELOG;
	 for(i = yoff; i < ylen; i++) {
	    for(j = xoff; j < xlen; j++) {
	       if(d[3] < 220)
		  *c = ReverseColorMap
		     [(d[0]+(((int)(RedColorMap[c[0]]>>8)*d[3])>>8))>>shift]
		     [(d[1]+(((int)(GreenColorMap[c[0]]>>8)*d[3])>>8))>>shift]
		     [(d[2]+(((int)(BlueColorMap[c[0]]>>8)*d[3])>>8))>>shift];
	       c++;
	       d+=4;
	    }
	    d += dstride;
	    c += cstride;
	 } 
      } break;
   case 2:
      {
	 uint16_t *e = (void*)c;
	 cstride /= 2;
	 for(i = yoff; i < ylen; i++) {
	    for(j = xoff; j < xlen; j++) {
	       if(d[3] < 220)
		  e[0] = ((((d[0] + (((int)(((e[0] >> 8) & 0xf8) 
		     	 | ((c[0] >> 11) & 0x7)) * d[3]) >> 8)) & 0xf8) << 8)
		     | (((d[1] + (((int)(((e[0] >> 3) & 0xfc)
	   		 | ((e[0] >> 5) & 0x3)) * d[3]) >> 8)) & 0xfc) << 3)
		     | ((d[2] + (((int)(((e[0] << 3) & 0xf8)
		      	 | (e[0] & 0x7)) * d[3]) >> 8)) >> 3));
		
	       e++;
	       d+=4;
	    }
	    d += dstride;
	    e += cstride;
	 }
      } break;
   case 3:
   case 4:
      for(i = yoff; i < ylen; i++) {
	 for(j = xoff; j < xlen; j++) {
	    if(d[3] < 220) {
	       c[0] = d[0] + (((int)c[0] * d[3]) >> 8);
	       c[1] = d[1] + (((int)c[1] * d[3]) >> 8);
	       c[2] = d[2] + (((int)c[2] * d[3]) >> 8);
	    }
		
	    c+=bypp;
	    d+=4;
	 }
	 d += dstride;
	 c += cstride;
      } break;
   }
}

#define MIN(x, y) x < y ? x : y
void SwapCursor(void)
{
   int px = MouseX - CursorsXOffset[CurrentCursor];
   int py = MouseY - CursorsYOffset[CurrentCursor];

   int minx = MIN(px, LastMouseX);
   int sizex = abs(px - LastMouseX);

   int miny = MIN(py, LastMouseY);
   int sizey = abs(py - LastMouseY);

   if(MouseVisible)
      DrawCursor();

   /* now update the portion of the screen that has changed, this is also
      used to hide the mouse if MouseVisible is 0 */
   if(DisplayMode & GLUT_DOUBLE && ((sizex || sizey) || !MouseVisible)) {
      int off, stride, i;
      if(minx < 0)
	 minx = 0;
      if(miny < 0)
	 miny = 0;
	
      if(minx + sizex > VarInfo.xres - CURSOR_WIDTH)
	 sizex = VarInfo.xres - CURSOR_WIDTH - minx;
      if(miny + sizey > VarInfo.yres - CURSOR_HEIGHT)
	 sizey = VarInfo.yres - CURSOR_HEIGHT - miny;
      off = FixedInfo.line_length * miny
	 + minx * VarInfo.bits_per_pixel / 8;
      stride = (sizex + CURSOR_WIDTH) * VarInfo.bits_per_pixel / 8;

      for(i = 0; i < sizey + CURSOR_HEIGHT; i++) {
	 memcpy(FrameBuffer+off, BackBuffer+off, stride);
	 off += FixedInfo.line_length;
      }
   }
}

void glutWarpPointer(int x, int y) 
{
   if(x < 0)
      x = 0;
   if(x >= VarInfo.xres) 
      x = VarInfo.xres - 1;
   MouseX = x;

   if(y < 0)
      y = 0;
   if(y >= VarInfo.yres) 
      y = VarInfo.yres - 1;
   MouseY = y;

   EraseCursor();
   SwapCursor();
}

void glutSetCursor(int cursor)
{
   if(cursor == GLUT_CURSOR_FULL_CROSSHAIR)
      cursor = GLUT_CURSOR_CROSSHAIR;

   EraseCursor();
   MouseVisible = 1;
   CurrentCursor = cursor;
   SwapCursor();
}
