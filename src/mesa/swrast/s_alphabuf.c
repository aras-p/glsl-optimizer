/* $Id: s_alphabuf.c,v 1.10 2002/03/19 16:47:05 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0.2
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
 * Software alpha planes.  Many frame buffers don't have alpha bits so
 * we simulate them in software.
 */


#include "glheader.h"
#include "context.h"
#include "mem.h"

#include "s_alphabuf.h"


#define ALPHA_DRAW_ADDR(X,Y) \
   (ctx->DrawBuffer->Alpha + (Y) * ctx->DrawBuffer->Width + (X))

#define ALPHA_READ_ADDR(X,Y) \
   (ctx->ReadBuffer->Alpha + (Y) * ctx->ReadBuffer->Width + (X))


/*
 * Allocate a new front and back alpha buffer.
 */
void
_mesa_alloc_alpha_buffers( GLframebuffer *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint bytes = buffer->Width * buffer->Height * sizeof(GLchan);

   ASSERT(buffer->UseSoftwareAlphaBuffers);

   if (buffer->FrontLeftAlpha) {
      MESA_PBUFFER_FREE( buffer->FrontLeftAlpha );
   }
   buffer->FrontLeftAlpha = (GLchan *) MESA_PBUFFER_ALLOC( bytes );
   if (!buffer->FrontLeftAlpha) {
      /* out of memory */
      _mesa_error( NULL, GL_OUT_OF_MEMORY,
                   "Couldn't allocate front-left alpha buffer" );
   }

   if (buffer->Visual.doubleBufferMode) {
      if (buffer->BackLeftAlpha) {
         MESA_PBUFFER_FREE( buffer->BackLeftAlpha );
      }
      buffer->BackLeftAlpha = (GLchan *) MESA_PBUFFER_ALLOC( bytes );
      if (!buffer->BackLeftAlpha) {
         /* out of memory */
         _mesa_error( NULL, GL_OUT_OF_MEMORY,
                      "Couldn't allocate back-left alpha buffer" );
      }
   }

   if (buffer->Visual.stereoMode) {
      if (buffer->FrontRightAlpha) {
         MESA_PBUFFER_FREE( buffer->FrontRightAlpha );
      }
      buffer->FrontRightAlpha = (GLchan *) MESA_PBUFFER_ALLOC( bytes );
      if (!buffer->FrontRightAlpha) {
         /* out of memory */
         _mesa_error( NULL, GL_OUT_OF_MEMORY,
                      "Couldn't allocate front-right alpha buffer" );
      }

      if (buffer->Visual.doubleBufferMode) {
         if (buffer->BackRightAlpha) {
            MESA_PBUFFER_FREE( buffer->BackRightAlpha );
         }
         buffer->BackRightAlpha = (GLchan *) MESA_PBUFFER_ALLOC( bytes );
         if (!buffer->BackRightAlpha) {
            /* out of memory */
            _mesa_error( NULL, GL_OUT_OF_MEMORY,
                         "Couldn't allocate back-right alpha buffer" );
         }
      }
   }

   if (ctx) {
      if (ctx->Color.DriverDrawBuffer == GL_FRONT_LEFT)
         buffer->Alpha = buffer->FrontLeftAlpha;
      else if (ctx->Color.DriverDrawBuffer == GL_BACK_LEFT)
         buffer->Alpha = buffer->BackLeftAlpha;
      else if (ctx->Color.DriverDrawBuffer == GL_FRONT_RIGHT)
         buffer->Alpha = buffer->FrontRightAlpha;
      else if (ctx->Color.DriverDrawBuffer == GL_BACK_RIGHT)
         buffer->Alpha = buffer->BackRightAlpha;
   }
}


/*
 * Clear all the alpha buffers
 */
void
_mesa_clear_alpha_buffers( GLcontext *ctx )
{
   const GLchan aclear = ctx->Color.ClearColor[3];
   GLuint bufferBit;

   ASSERT(ctx->DrawBuffer->UseSoftwareAlphaBuffers);
   ASSERT(ctx->Color.ColorMask[ACOMP]);

   /* loop over four possible alpha buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit = bufferBit << 1) {
      if (bufferBit & ctx->Color.DrawDestMask) {
         GLchan *buffer;
         if (bufferBit == FRONT_LEFT_BIT) {
            buffer = ctx->DrawBuffer->FrontLeftAlpha;
         }
         else if (bufferBit == FRONT_RIGHT_BIT) {
            buffer = ctx->DrawBuffer->FrontRightAlpha;
         }
         else if (bufferBit == BACK_LEFT_BIT) {
            buffer = ctx->DrawBuffer->BackLeftAlpha;
         }
         else {
            buffer = ctx->DrawBuffer->BackRightAlpha;
         }

         if (ctx->Scissor.Enabled) {
            /* clear scissor region */
            GLint j;
            GLint rowLen = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
            GLint rows = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
            GLint width = ctx->DrawBuffer->Width;
            GLchan *aptr = buffer
                          + ctx->DrawBuffer->_Ymin * ctx->DrawBuffer->Width
                          + ctx->DrawBuffer->_Xmin;
            for (j = 0; j < rows; j++) {
#if CHAN_BITS == 8
               MEMSET( aptr, aclear, rowLen );
#elif CHAN_BITS == 16
               MEMSET16( aptr, aclear, rowLen );
#else
               GLint i;
               for (i = 0; i < rowLen; i++) {
                  aptr[i] = aclear;
               }
#endif
               aptr += width;
            }
         }
         else {
            /* clear whole buffer */
            GLuint pixels = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
#if CHAN_BITS == 8
            MEMSET(buffer, aclear, pixels);
#elif CHAN_BITS == 16
            MEMSET16(buffer, aclear, pixels);
#else
            GLuint i;
            for (i = 0; i < pixels; i++) {
               buffer[i] = aclear;
            }
#endif
         }
      }
   }
}



void
_mesa_write_alpha_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                        CONST GLchan rgba[][4], const GLubyte mask[] )
{
   GLchan *aptr = ALPHA_DRAW_ADDR( x, y );
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *aptr = rgba[i][ACOMP];
         }
         aptr++;
      }
   }
   else {
      for (i=0;i<n;i++) {
         *aptr++ = rgba[i][ACOMP];
      }
   }
}


void
_mesa_write_mono_alpha_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                             GLchan alpha, const GLubyte mask[] )
{
   GLchan *aptr = ALPHA_DRAW_ADDR( x, y );
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *aptr = alpha;
         }
         aptr++;
      }
   }
   else {
      for (i=0;i<n;i++) {
         *aptr++ = alpha;
      }
   }
}


void
_mesa_write_alpha_pixels( GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          CONST GLchan rgba[][4], const GLubyte mask[] )
{
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLchan *aptr = ALPHA_DRAW_ADDR( x[i], y[i] );
            *aptr = rgba[i][ACOMP];
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         GLchan *aptr = ALPHA_DRAW_ADDR( x[i], y[i] );
         *aptr = rgba[i][ACOMP];
      }
   }
}


void
_mesa_write_mono_alpha_pixels( GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               GLchan alpha, const GLubyte mask[] )
{
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLchan *aptr = ALPHA_DRAW_ADDR( x[i], y[i] );
            *aptr = alpha;
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         GLchan *aptr = ALPHA_DRAW_ADDR( x[i], y[i] );
         *aptr = alpha;
      }
   }
}



void
_mesa_read_alpha_span( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLchan rgba[][4] )
{
   GLchan *aptr = ALPHA_READ_ADDR( x, y );
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][ACOMP] = *aptr++;
   }
}


void
_mesa_read_alpha_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLchan rgba[][4], const GLubyte mask[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         GLchan *aptr = ALPHA_READ_ADDR( x[i], y[i] );
         rgba[i][ACOMP] = *aptr;
      }
   }
}
