/* $Id: s_pixeltex.c,v 1.4 2002/01/10 16:54:29 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This file implements both the GL_SGIX_pixel_texture and
 * GL_SIGS_pixel_texture extensions. Luckily, they pretty much
 * overlap in functionality so we use the same state variables
 * and execution code for both.
 */


#include "glheader.h"
#include "colormac.h"

#include "s_context.h"
#include "s_pixeltex.h"


/*
 * Convert RGBA values into strq texture coordinates.
 */
void
_mesa_pixeltexgen(GLcontext *ctx, GLuint n, const GLchan rgba[][4],
                  GLfloat texcoord[][4])
{
   if (ctx->Pixel.FragmentRgbSource == GL_CURRENT_RASTER_COLOR) {
      GLuint i;
      for (i = 0; i < n; i++) {
         texcoord[i][0] = ctx->Current.RasterColor[RCOMP];
         texcoord[i][1] = ctx->Current.RasterColor[GCOMP];
         texcoord[i][2] = ctx->Current.RasterColor[BCOMP];
      }
   }
   else {
      GLuint i;
      ASSERT(ctx->Pixel.FragmentRgbSource == GL_PIXEL_GROUP_COLOR_SGIS);
      for (i = 0; i < n; i++) {
         texcoord[i][0] = CHAN_TO_FLOAT(rgba[i][RCOMP]);
         texcoord[i][1] = CHAN_TO_FLOAT(rgba[i][GCOMP]);
         texcoord[i][2] = CHAN_TO_FLOAT(rgba[i][BCOMP]);
      }
   }

   if (ctx->Pixel.FragmentAlphaSource == GL_CURRENT_RASTER_COLOR) {
      GLuint i;
      for (i = 0; i < n; i++) {
         texcoord[i][3] = ctx->Current.RasterColor[ACOMP];
      }
   }
   else {
      GLuint i;
      ASSERT(ctx->Pixel.FragmentAlphaSource == GL_PIXEL_GROUP_COLOR_SGIS);
      for (i = 0; i < n; i++) {
         texcoord[i][3] = CHAN_TO_FLOAT(rgba[i][ACOMP]);
      }
   }
}
