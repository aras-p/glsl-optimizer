/* $Id: buffers.c,v 1.30 2001/06/18 17:26:08 brianp Exp $ */

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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "accum.h"
#include "buffers.h"
#include "colormac.h"
#include "context.h"
#include "depth.h"
#include "enums.h"
#include "macros.h"
#include "mem.h"
#include "stencil.h"
#include "state.h"
#include "mtypes.h"
#endif



void
_mesa_ClearIndex( GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Color.ClearIndex == (GLuint) c)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.ClearIndex = (GLuint) c;

   if (!ctx->Visual.rgbMode && ctx->Driver.ClearIndex) {
      /* it's OK to call glClearIndex in RGBA mode but it should be a NOP */
      (*ctx->Driver.ClearIndex)( ctx, ctx->Color.ClearIndex );
   }
}



void
_mesa_ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
   GLchan tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   UNCLAMPED_FLOAT_TO_CHAN(tmp[0], red);
   UNCLAMPED_FLOAT_TO_CHAN(tmp[1], green);
   UNCLAMPED_FLOAT_TO_CHAN(tmp[2], blue);
   UNCLAMPED_FLOAT_TO_CHAN(tmp[3], alpha);

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor))
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_CHAN4(ctx->Color.ClearColor, tmp);

   if (ctx->Visual.rgbMode && ctx->Driver.ClearColor) {
      /* it's OK to call glClearColor in CI mode but it should be a NOP */
      (*ctx->Driver.ClearColor)(ctx, ctx->Color.ClearColor);
   }
}



void
_mesa_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glClear 0x%x\n", mask);

   if (ctx->NewState) {
      _mesa_update_state( ctx );	/* update _Xmin, etc */
   }

   if (ctx->RenderMode==GL_RENDER) {
      const GLint x = ctx->DrawBuffer->_Xmin;
      const GLint y = ctx->DrawBuffer->_Ymin;
      const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
      const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
      GLbitfield ddMask;

      /* don't clear depth buffer if depth writing disabled */
      if (!ctx->Depth.Mask)
         CLEAR_BITS(mask, GL_DEPTH_BUFFER_BIT);

      /* Build bitmask to send to driver Clear function */
      ddMask = mask & (GL_DEPTH_BUFFER_BIT |
                       GL_STENCIL_BUFFER_BIT |
                       GL_ACCUM_BUFFER_BIT);
      if (mask & GL_COLOR_BUFFER_BIT) {
         ddMask |= ctx->Color.DrawDestMask;
      }

      ASSERT(ctx->Driver.Clear);
      ctx->Driver.Clear( ctx, ddMask, !ctx->Scissor.Enabled,
			 x, y, width, height );
   }
}


void
_mesa_DrawBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex... */


   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glDrawBuffer %s\n", _mesa_lookup_enum_by_nr(mode));

   switch (mode) {
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
         /* AUX buffers not implemented in Mesa at this time */
         _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
         return;
      case GL_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;}
         if (ctx->Visual.doubleBufferMode)
            ctx->Color.DrawDestMask = FRONT_RIGHT_BIT | BACK_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_RIGHT_BIT;
         break;
      case GL_FRONT_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawDestMask = FRONT_RIGHT_BIT;
         break;
      case GL_BACK_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawDestMask = BACK_RIGHT_BIT;
         break;
      case GL_BACK_LEFT:
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawDestMask = BACK_LEFT_BIT;
         break;
      case GL_FRONT_AND_BACK:
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (ctx->Visual.stereoMode)
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | BACK_LEFT_BIT
                                    | FRONT_RIGHT_BIT | BACK_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | BACK_LEFT_BIT;
         break;
      case GL_BACK:
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (ctx->Visual.stereoMode)
            ctx->Color.DrawDestMask = BACK_LEFT_BIT | BACK_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = BACK_LEFT_BIT;
         break;
      case GL_LEFT:
         /* never an error */
         if (ctx->Visual.doubleBufferMode)
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | BACK_LEFT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT;
         break;
      case GL_FRONT_LEFT:
         /* never an error */
         ctx->Color.DrawDestMask = FRONT_LEFT_BIT;
         break;
      case GL_FRONT:
         /* never an error */
         if (ctx->Visual.stereoMode)
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | FRONT_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT;
         break;
      case GL_NONE:
         /* never an error */
         ctx->Color.DrawDestMask = 0;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
         return;
   }

   /*
    * Make the dest buffer mode more precise if possible
    */
   if (mode == GL_LEFT && !ctx->Visual.doubleBufferMode)
      ctx->Color.DriverDrawBuffer = GL_FRONT_LEFT;
   else if (mode == GL_RIGHT && !ctx->Visual.doubleBufferMode)
      ctx->Color.DriverDrawBuffer = GL_FRONT_RIGHT;
   else if (mode == GL_FRONT && !ctx->Visual.stereoMode)
      ctx->Color.DriverDrawBuffer = GL_FRONT_LEFT;
   else if (mode == GL_BACK && !ctx->Visual.stereoMode)
      ctx->Color.DriverDrawBuffer = GL_BACK_LEFT;
   else
      ctx->Color.DriverDrawBuffer = mode;

   /*
    * Set current alpha buffer pointer
    */
   if (ctx->DrawBuffer->UseSoftwareAlphaBuffers) {
      if (ctx->Color.DriverDrawBuffer == GL_FRONT_LEFT)
         ctx->DrawBuffer->Alpha = ctx->DrawBuffer->FrontLeftAlpha;
      else if (ctx->Color.DriverDrawBuffer == GL_BACK_LEFT)
         ctx->DrawBuffer->Alpha = ctx->DrawBuffer->BackLeftAlpha;
      else if (ctx->Color.DriverDrawBuffer == GL_FRONT_RIGHT)
         ctx->DrawBuffer->Alpha = ctx->DrawBuffer->FrontRightAlpha;
      else if (ctx->Color.DriverDrawBuffer == GL_BACK_RIGHT)
         ctx->DrawBuffer->Alpha = ctx->DrawBuffer->BackRightAlpha;
   }

   /*
    * If we get here there can't have been an error.
    * Now see if device driver can implement the drawing to the target
    * buffer(s).  The driver may not be able to do GL_FRONT_AND_BACK mode
    * for example.  We'll take care of that in the core code by looping
    * over the individual buffers.
    */
   ASSERT(ctx->Driver.SetDrawBuffer);
   if ( (*ctx->Driver.SetDrawBuffer)(ctx, ctx->Color.DriverDrawBuffer) ) {
      /* All OK, the driver will do all buffer writes */
      ctx->Color.MultiDrawBuffer = GL_FALSE;
   }
   else {
      /* We'll have to loop over the multiple draw buffer targets */
      ctx->Color.MultiDrawBuffer = GL_TRUE;
      /* Set drawing buffer to front for now */
      (void) (*ctx->Driver.SetDrawBuffer)(ctx, GL_FRONT_LEFT);
   }

   ctx->Color.DrawBuffer = mode;
   ctx->NewState |= _NEW_COLOR;
}



void
_mesa_ReadBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glReadBuffer %s\n", _mesa_lookup_enum_by_nr(mode));

   switch (mode) {
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
         /* AUX buffers not implemented in Mesa at this time */
         _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
         return;
      case GL_LEFT:
      case GL_FRONT:
      case GL_FRONT_LEFT:
         /* Front-Left buffer, always exists */
         ctx->Pixel.DriverReadBuffer = GL_FRONT_LEFT;
         break;
      case GL_BACK:
      case GL_BACK_LEFT:
         /* Back-Left buffer, requires double buffering */
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.DriverReadBuffer = GL_BACK_LEFT;
         break;
      case GL_FRONT_RIGHT:
      case GL_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.DriverReadBuffer = GL_FRONT_RIGHT;
         break;
      case GL_BACK_RIGHT:
         if (!ctx->Visual.stereoMode || !ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.DriverReadBuffer = GL_BACK_RIGHT;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
         return;
   }

   ctx->Pixel.ReadBuffer = mode;
   ctx->NewState |= _NEW_PIXEL;
}


/*
 * GL_MESA_resize_buffers extension
 */
void
_mesa_ResizeBuffersMESA( void )
{
   GLcontext *ctx = _mesa_get_current_context();
   GLuint buf_width, buf_height;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );


   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glResizeBuffersMESA\n");

   /* ask device driver for size of output buffer */
   (*ctx->Driver.GetBufferSize)( ctx, &buf_width, &buf_height );

   /* see if size of device driver's color buffer (window) has changed */
   if (ctx->DrawBuffer->Width == (GLint) buf_width &&
       ctx->DrawBuffer->Height == (GLint) buf_height)
      return;

   ctx->NewState |= _NEW_BUFFERS;  /* to update scissor / window bounds */

   /* save buffer size */
   ctx->DrawBuffer->Width = buf_width;
   ctx->DrawBuffer->Height = buf_height;

   ctx->Driver.ResizeBuffersMESA( ctx );
}


void
_mesa_Scissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glScissor" );
      return;
   }

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glScissor %d %d %d %d\n", x, y, width, height);

   if (x == ctx->Scissor.X &&
       y == ctx->Scissor.Y &&
       width == ctx->Scissor.Width &&
       height == ctx->Scissor.Height)
      return;

   FLUSH_VERTICES(ctx, _NEW_SCISSOR);
   ctx->Scissor.X = x;
   ctx->Scissor.Y = y;
   ctx->Scissor.Width = width;
   ctx->Scissor.Height = height;

   if (ctx->Driver.Scissor)
      ctx->Driver.Scissor( ctx, x, y, width, height );
}


/*
 * XXX move somewhere else someday?
 */
void
_mesa_SampleCoverageARB(GLclampf value, GLboolean invert)
{
   GLcontext *ctx = _mesa_get_current_context();

   if (!ctx->Extensions.ARB_multisample) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleCoverageARB");
      return;
   }

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );
   ctx->Multisample.SampleCoverageValue = CLAMP(value, 0.0, 1.0);
   ctx->Multisample.SampleCoverageInvert = invert;
   ctx->NewState |= _NEW_MULTISAMPLE;
}
			   
