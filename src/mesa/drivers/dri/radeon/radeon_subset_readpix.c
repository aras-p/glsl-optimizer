/**
 * \file radeon_subset_readpix.c
 * \brief Pixel reading.
 * 
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * \author Brian Paul <brian@tungstengraphics.com>
 */

/*
 * Copyright 2003       ATI Technologies Inc., Ontario, Canada, and
 *                      Tungsten Graphics Inc., Cedar Park, Texas.
 * 
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* $XFree86$ */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "imports.h"
/*#include "mmath.h" */
#include "macros.h"
#include "state.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_subset.h"

/**
 * \brief Read pixel in RGBA format on a Radeon 16bpp frame buffer.
 *
 * \param rgba destination pointer.
 * \param ptr pointer to the pixel in the frame buffer.
 */
#define READ_RGBA_16( rgba, ptr )		\
do {						\
    GLushort p = *(GLushort *)ptr;		\
    rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;	\
    rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;	\
    rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;	\
    rgba[3] = 0xff;				\
} while (0)

/**
 * \brief Read pixel in RGBA format on a Radeon 32bpp frame buffer.
 *
 * \param rgba destination pointer.
 * \param ptr pointer to the pixel in the frame buffer.
 */
#define READ_RGBA_32( rgba, ptr )		\
do {						\
   GLuint p = *(GLuint *)ptr;			\
   rgba[0] = (p >> 16) & 0xff;			\
   rgba[1] = (p >>  8) & 0xff;			\
   rgba[2] = (p >>  0) & 0xff;			\
   rgba[3] = (p >> 24) & 0xff;			\
} while (0)

/**
 * \brief Read a span in RGBA format.
 * 
 * \param ctx GL context.
 * \param n number of pixels in the span.
 * \param x x position of the span start.
 * \param y y position of the span.
 * \param rgba destination buffer.
 *
 * Calculates the pointer to the span start in the frame buffer and uses either
 * #READ_RGBA_16 or #READ_RGBA_32 macros to copy the values.
 */
static void ReadRGBASpan( const GLcontext *ctx,
			       GLuint n, GLint x, GLint y,
			       GLubyte rgba[][4])
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonScreenPtr radeonScreen = rmesa->radeonScreen;
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   GLuint cpp = radeonScreen->cpp;
   GLuint pitch = radeonScreen->frontPitch * cpp;
   GLint i;

   if (ctx->_RotateMode) {
      char *ptr = (char *)(rmesa->dri.screen->pFB +
			   rmesa->state.pixel.readOffset +
			   ((dPriv->x + (dPriv->w - y - 1)) * cpp) +
			   ((dPriv->y + (dPriv->h - x - 1)) * pitch));

      if (cpp == 4)
	 for (i = 0; i < n; i++, ptr -= pitch)
	    READ_RGBA_32( rgba[i], ptr );
      else
	 for (i = 0; i < n; i++, ptr -= pitch)
	    READ_RGBA_16( rgba[i], ptr );
   }
   else {
      char *ptr = (char *)(rmesa->dri.screen->pFB +
			   rmesa->state.pixel.readOffset +
			   ((dPriv->x + x) * cpp) +
			   ((dPriv->y + (dPriv->h - y - 1)) * pitch));

      if (cpp == 4)
	 for (i = 0; i < n; i++, ptr += cpp)
	    READ_RGBA_32( rgba[i], ptr );
      else
	 for (i = 0; i < n; i++, ptr += cpp)
	    READ_RGBA_16( rgba[i], ptr );
   }
}


/**
 * \brief Optimized glReadPixels().
 *
 * To be used with particular pixel formats GL_UNSIGNED_BYTE and GL_RGBA, when pixel
 * scaling, biasing and mapping are disabled.
 *
 * \param x x start position of the reading rectangle.
 * \param y y start position of the reading rectangle.
 * \param width width of the reading rectangle.
 * \param height height of the reading rectangle.
 * \param format pixel format. Must be GL_RGBA.
 * \param type pixel type. Must be GL_UNSIGNED_BYTE.
 * \param pixels pixel data.
 * 
 * After asserting the above conditions, compensates for clipping and calls
 * ReadRGBASpan() to read each row.
 */
void radeonReadPixels( GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint srcX = x;
   GLint srcY = y;
   GLint readWidth = width;           /* actual width read */
   GLint readHeight = height;         /* actual height read */
   const struct gl_pixelstore_attrib *packing = &ctx->Pack;
   GLint skipRows = packing->SkipRows;
   GLint skipPixels = packing->SkipPixels;
   GLint rowLength;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   {
      GLint tmp, tmps;
      tmp = x; x = y; y = tmp;
      tmps = width; width = height; height = tmps;
   }

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glReadPixels(width=%d height=%d)", width, height );
      return;
   }

   if (!pixels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glReadPixels(pixels)" );
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);


   /* can't do scale, bias, mapping, etc */
   assert(!ctx->_ImageTransferState);

   /* can't do fancy pixel packing */
   assert (packing->Alignment == 1 &&
	   !packing->SwapBytes &&
	   !packing->LsbFirst);


   if (packing->RowLength > 0)
      rowLength = packing->RowLength;
   else
      rowLength = width;

   /* horizontal clipping */
   if (srcX < 0) {
      skipPixels -= srcX;
      readWidth += srcX;
      srcX = 0;
   }
   if (srcX + readWidth > (GLint) ctx->ReadBuffer->Width)
      readWidth -= (srcX + readWidth - (GLint) ctx->ReadBuffer->Width);
   if (readWidth <= 0)
      return;

   /* vertical clipping */
   if (srcY < 0) {
      skipRows -= srcY;
      readHeight += srcY;
      srcY = 0;
   }
   if (srcY + readHeight > (GLint) ctx->ReadBuffer->Height)
      readHeight -= (srcY + readHeight - (GLint) ctx->ReadBuffer->Height);
   if (readHeight <= 0)
      return;

   /*
    * Ready to read!
    * The window region at (destX, destY) of size (readWidth, readHeight)
    * will be read back.
    * We'll write pixel data to buffer pointed to by "pixels" but we'll
    * skip "skipRows" rows and skip "skipPixels" pixels/row.
    */
   if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
      GLchan *dest = (GLchan *) pixels 
	 + (skipRows * rowLength + skipPixels) * 4;
      GLint row;

      for (row=0; row<readHeight; row++) {
	 ReadRGBASpan(ctx, readWidth, srcX, srcY, (GLchan (*)[4]) dest);
	 dest += rowLength * 4;
	 srcY++;
      }
   }
   else {
      /* can't do this format/type combination */
      assert(0);
   }
}
