/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatexcnv.c,v 1.3 2002/10/30 12:51:36 alanh Exp $ */
/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "mm.h"
#include "mgacontext.h"
#include "mgatex.h"


/*
 * mgaConvertTexture
 * Converts a mesa format texture to the appropriate hardware format
 * Note that sometimes width may be larger than the texture, like 64x1
 * for an 8x8 texture.  This happens when we have to crutch the pitch
 * limits of the mga by uploading a block of texels as a single line.
 */
void mgaConvertTexture( GLuint *destPtr, int texelBytes,
			struct gl_texture_image *image,
			int x, int y, int width, int height )
{
   register int		i, j;
   GLubyte		*src;
   int stride;

   if (0)
      fprintf(stderr, "texture image %p\n", image->Data);

   if (image->Data == 0)
      return;

   /* FIXME: g400 luminance_alpha internal format */
   switch (texelBytes) {
   case 1:
      switch (image->Format) {
      case GL_COLOR_INDEX:
      case GL_INTENSITY:
      case GL_LUMINANCE:
      case GL_ALPHA:
	 src = (GLubyte *)image->Data + ( y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 2  ; j ; j-- ) {

	       *destPtr++ = src[0] | ( src[1] << 8 ) | ( src[2] << 16 ) | ( src[3] << 24 );
	       src += 4;
	    }
	    src += stride;
	 }
	 break;
      default:
	 goto format_error;
      }
      break;
   case 2:
      switch (image->Format) {
      case GL_RGB:
	 src = (GLubyte *)image->Data + ( y * image->Width + x ) * 3;
	 stride = (image->Width - width) * 3;
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 1  ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR565(src[0],src[1],src[2]) |
		  ( MGAPACKCOLOR565(src[3],src[4],src[5]) << 16 );
	       src += 6;
	    }
	    src += stride;
	 }
	 break;
      case GL_RGBA:
	 src = (GLubyte *)image->Data + ( y * image->Width + x ) * 4;
	 stride = (image->Width - width) * 4;
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 1  ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR4444(src[0],src[1],src[2],src[3]) |
		  ( MGAPACKCOLOR4444(src[4],src[5],src[6],src[7]) << 16 );
	       src += 8;
	    }
	    src += stride;
	 }
	 break;
      case GL_LUMINANCE:
	 src = (GLubyte *)image->Data + ( y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 1  ; j ; j-- ) {
	       /* FIXME: should probably use 555 texture to get true grey */
	       *destPtr++ = MGAPACKCOLOR565(src[0],src[0],src[0]) |
		  ( MGAPACKCOLOR565(src[1],src[1],src[1]) << 16 );
	       src += 2;
	    }
	    src += stride;
	 }
	 break;
      case GL_INTENSITY:
	 src = (GLubyte *)image->Data + ( y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 1  ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR4444(src[0],src[0],src[0],src[0]) |
		  ( MGAPACKCOLOR4444(src[1],src[1],src[1],src[1]) << 16 );
	       src += 2;
	    }
	    src += stride;
	 }
	 break;
      case GL_ALPHA:
	 src = (GLubyte *)image->Data + ( y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 1  ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR4444(255,255,255,src[0]) |
		  ( MGAPACKCOLOR4444(255,255,255,src[1]) << 16 );
	       src += 2;
	    }
	    src += stride;
	 }
	 break;
      case GL_LUMINANCE_ALPHA:
	 src = (GLubyte *)image->Data + ( y * image->Width + x ) * 2;
	 stride = (image->Width - width) * 2;
	 for ( i = height ; i ; i-- ) {
	    for ( j = width >> 1  ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR4444(src[0],src[0],src[0],src[1]) |
		  ( MGAPACKCOLOR4444(src[2],src[2],src[2],src[3]) << 16 );
	       src += 4;
	    }
	    src += stride;
	 }
	 break;
      default:
	 goto format_error;
      }
      break;
   case 4:
      switch (image->Format) {
      case GL_RGB:
	 src = (GLubyte *)image->Data + (  y * image->Width + x ) * 3;
	 stride = (image->Width - width) * 3;
	 for ( i = height ; i ; i-- ) {
	    for ( j = width ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR8888(src[0],src[1],src[2], 255);
	       src += 3;
	    }
	    src += stride;
	 }
	 break;
      case GL_RGBA:
	 src = (GLubyte *)image->Data + (  y * image->Width + x ) * 4;
	 stride = (image->Width - width) * 4;
	 for ( i = height ; i ; i-- ) {
	    for ( j = width  ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR8888(src[0],src[1],src[2],src[3]);
	       src += 4;
	    }
	    src += stride;
	 }
	 break;
      case GL_LUMINANCE:
	 src = (GLubyte *)image->Data + ( y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR8888(src[0],src[0],src[0], 255);
	       src += 1;
	    }
	    src += stride;
	 }
	 break;
      case GL_INTENSITY:
	 src = (GLubyte *)image->Data + (  y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR8888(src[0],src[0],src[0],src[0]);
	       src += 1;
	    }
	    src += stride;
	 }
	 break;
      case GL_ALPHA:
	 src = (GLubyte *)image->Data + ( y * image->Width + x );
	 stride = (image->Width - width);
	 for ( i = height ; i ; i-- ) {
	    for ( j = width ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR8888(255,255,255,src[0]);
	       src += 1;
	    }
	    src += stride;
	 }
	 break;
      case GL_LUMINANCE_ALPHA:
	 src = (GLubyte *)image->Data + ( y * image->Width + x ) * 2;
	 stride = (image->Width - width) * 2;
	 for ( i = height ; i ; i-- ) {
	    for ( j = width ; j ; j-- ) {

	       *destPtr++ = MGAPACKCOLOR8888(src[0],src[0],
					     src[0],src[1]);
	       src += 2;
	    }
	    src += stride;
	 }
	 break;
      default:
	 goto format_error;
      }
      break;
   default:
      goto format_error;
   }

   return;

 format_error:

   fprintf(stderr, "Unsupported texelBytes %i, image->Format %i\n",
	   (int)texelBytes, (int)image->Format );
}
