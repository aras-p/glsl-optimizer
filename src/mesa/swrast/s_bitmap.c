/* $Id: s_bitmap.c,v 1.11 2001/06/18 23:55:18 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "image.h"
#include "macros.h"
#include "pixel.h"

#include "s_context.h"
#include "s_fog.h"
#include "s_pb.h"



/*
 * Render a bitmap.
 */
void
_swrast_Bitmap( GLcontext *ctx, GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct pixel_buffer *PB = swrast->PB;
   GLint row, col;
   GLdepth fragZ;
   GLfloat fogCoord;

   ASSERT(ctx->RenderMode == GL_RENDER);
   ASSERT(bitmap);

   RENDER_START(swrast,ctx);

   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived( ctx );

   /* Set bitmap drawing color */
   if (ctx->Visual.rgbMode) {
      GLint r, g, b, a;
      r = (GLint) (ctx->Current.RasterColor[0] * CHAN_MAXF);
      g = (GLint) (ctx->Current.RasterColor[1] * CHAN_MAXF);
      b = (GLint) (ctx->Current.RasterColor[2] * CHAN_MAXF);
      a = (GLint) (ctx->Current.RasterColor[3] * CHAN_MAXF);
      PB_SET_COLOR( PB, r, g, b, a );
   }
   else {
      PB_SET_INDEX( PB, ctx->Current.RasterIndex );
   }

   fragZ = (GLdepth) ( ctx->Current.RasterPos[2] * ctx->DepthMaxF);

   if (ctx->Fog.Enabled) {
      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT) {
         fogCoord = ctx->Current.FogCoord;
      }
      else {
         fogCoord = _mesa_z_to_fogfactor(ctx, ctx->Current.RasterDistance);
      }
   }
   else {
      fogCoord = 0.0;
   }

   for (row=0; row<height; row++) {
      const GLubyte *src = (const GLubyte *) _mesa_image_address( unpack,
                 bitmap, width, height, GL_COLOR_INDEX, GL_BITMAP, 0, row, 0 );

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            if (*src & mask) {
               PB_WRITE_PIXEL( PB, px+col, py+row, fragZ, fogCoord );
            }
            if (mask == 128U) {
               src++;
               mask = 1U;
            }
            else {
               mask = mask << 1;
            }
         }

         PB_CHECK_FLUSH( ctx, PB );

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            if (*src & mask) {
               PB_WRITE_PIXEL( PB, px+col, py+row, fragZ, fogCoord );
            }
            if (mask == 1U) {
               src++;
               mask = 128U;
            }
            else {
               mask = mask >> 1;
            }
         }

         PB_CHECK_FLUSH( ctx, PB );

         /* get ready for next row */
         if (mask != 128)
            src++;
      }
   }

   _mesa_flush_pb(ctx);

   RENDER_FINISH(swrast,ctx);
}
