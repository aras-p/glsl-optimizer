/* $Id: svgamesa8.c,v 1.2 2000/01/22 20:08:36 brianp Exp $ */

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

int __svga_drawpixel8(int x, int y, unsigned long c)
{
    unsigned long offset;

    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->linewidth + x;
    SVGABuffer.BackBuffer[offset]=c;
    
    return 0;
}

unsigned long __svga_getpixel8(int x, int y)
{
    unsigned long offset;

    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->linewidth + x;
    return SVGABuffer.BackBuffer[offset];
}

void __set_index8( GLcontext *ctx, GLuint index )
{
   SVGAMesa->index = index;
}

void __clear_index8( GLcontext *ctx, GLuint index )
{
   SVGAMesa->clear_index = index;
}

GLbitfield __clear8( GLcontext *ctx, GLbitfield mask, GLboolean all,
                     GLint x, GLint y, GLint width, GLint height )
{
   int i,j;
   
   if (mask & GL_COLOR_BUFFER_BIT) {
   
    if (all) 
    { 
     memset(SVGABuffer.BackBuffer,SVGAMesa->clear_index,SVGABuffer.BufferSize);
    } else {
    for (i=x;i<width;i++)    
     for (j=y;j<height;j++)    
      __svga_drawpixel8(i,j,SVGAMesa->clear_index);
    }
   }    
   return mask & (~GL_COLOR_BUFFER_BIT);
}

void __write_ci32_span8( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         const GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         __svga_drawpixel8( x, y, index[i]);
      }
   }
}

void __write_ci8_span8( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLubyte index[], const GLubyte mask[] )
{
   int i;

   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         __svga_drawpixel8( x, y, index[i]);
      }
   }
}

void __write_mono_ci_span8( const GLcontext *ctx, GLuint n,
                            GLint x, GLint y, const GLubyte mask[] )
{
   int i;
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         __svga_drawpixel8( x, y, SVGAMesa->index);
      }
   }
}

void __read_ci32_span8( const GLcontext *ctx,
                        GLuint n, GLint x, GLint y, GLuint index[])
{
   int i;
   for (i=0; i<n; i++,x++) {
      index[i] = __svga_getpixel8( x, y);
   }
}

void __write_ci32_pixels8( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel8( x[i], y[i], index[i]);
      }
   }
}


void __write_mono_ci_pixels8( const GLcontext *ctx, GLuint n,
                              const GLint x[], const GLint y[],
                              const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel8( x[i], y[i], SVGAMesa->index);
      }
   }
}

void __read_ci32_pixels8( const GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++,x++) {
      index[i] = __svga_getpixel8( x[i], y[i]);
   }
}

