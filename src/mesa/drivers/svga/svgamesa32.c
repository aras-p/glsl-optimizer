/* $Id: svgamesa32.c,v 1.2 2000/01/22 20:08:36 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.2
 * Copyright (C) 1995-2000  Brian Paul
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
 * SVGA driver for Mesa.
 * Original author:  Brian Paul
 * Additional authors:  Slawomir Szczyrba <steev@hot.pl>  (Mesa 3.2)
 */


#include "svgapix.h"

GLint * intBuffer;

inline int RGB2BGR32(int c)
{
	asm("rorw  $8, %0\n"	 
	    "rorl $16, %0\n"	 
	    "rorw  $8, %0\n"	 
	    "shrl  $8, %0\n"	 
      : "=q"(c):"0"(c));
    return c;
}

int __svga_drawpixel32(int x, int y, unsigned long c)
{
    unsigned long offset;

    intBuffer=(void *)SVGABuffer.BackBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    intBuffer[offset]=c;
    return 0;
}

unsigned long __svga_getpixel32(int x, int y)
{
    unsigned long offset;

    intBuffer=(void *)SVGABuffer.BackBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    return intBuffer[offset];
}

void __set_color32( GLcontext *ctx,
                    GLubyte red, GLubyte green,
                    GLubyte blue, GLubyte alpha )
{
   SVGAMesa->red = red;
   SVGAMesa->green = green;
   SVGAMesa->blue = blue;
   SVGAMesa->truecolor = red<<16 | green<<8 | blue;
}

void __clear_color32( GLcontext *ctx,
                      GLubyte red, GLubyte green,
                      GLubyte blue, GLubyte alpha )
{
   SVGAMesa->clear_truecolor = red<<16 | green<<8 | blue;
}

GLbitfield __clear32( GLcontext *ctx, GLbitfield mask, GLboolean all,
                        GLint x, GLint y, GLint width, GLint height )
{
   int i,j;
   
   if (mask & GL_COLOR_BUFFER_BIT) {
    if (all) {
     intBuffer=(void *)SVGABuffer.BackBuffer;
     for (i=0;i<SVGABuffer.BufferSize / 4;i++) intBuffer[i]=SVGAMesa->clear_truecolor;
    } else {
    for (i=x;i<width;i++)    
     for (j=y;j<height;j++)    
      __svga_drawpixel32(i,j,SVGAMesa->clear_truecolor);
    }	
   }
   return mask & (~GL_COLOR_BUFFER_BIT);
}

void __write_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   if (mask) {
      /* draw some pixels */
      for (i=0; i<n; i++, x++) {
         if (mask[i]) {
         __svga_drawpixel32( x, y, RGB2BGR32(*((GLint*)rgba[i])));
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0; i<n; i++, x++) {
         __svga_drawpixel32( x, y, RGB2BGR32(*((GLint*)rgba[i])));
      }
   }
}

void __write_mono_rgba_span32( const GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
                               const GLubyte mask[])
{
   int i;
   for (i=0; i<n; i++, x++) {
      if (mask[i]) {
         __svga_drawpixel32( x, y, SVGAMesa->truecolor);
      }
   }
}

void __read_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4] )
{
   int i;
   for (i=0; i<n; i++, x++) {
     *((GLint*)rgba[i]) = RGB2BGR32(__svga_getpixel32( x, y ));
   }
}

void __write_rgba_pixels32( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel32( x[i], y[i], RGB2BGR32(*((GLint*)rgba[i])));
      }
   }
}

void __write_mono_rgba_pixels32( const GLcontext *ctx,
                                 GLuint n,
                                 const GLint x[], const GLint y[],
                                 const GLubyte mask[] )
{
   int i;
   /* use current rgb color */
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel32( x[i], y[i], SVGAMesa->truecolor );
      }
   }
}

void __read_rgba_pixels32( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++,x++) {
    *((GLint*)rgba[i]) = RGB2BGR32(__svga_getpixel32( x[i], y[i] ));
   }
}
