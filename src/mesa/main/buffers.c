/* $Id: buffers.c,v 1.1 2000/02/02 19:15:03 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#include "alphabuf.h"
#include "buffers.h"
#include "context.h"
#include "depth.h"
#include "enums.h"
#include "macros.h"
#include "masking.h"
#include "mem.h"
#include "stencil.h"
#include "state.h"
#include "types.h"
#endif



void
_mesa_ClearIndex( GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClearIndex");
   ctx->Color.ClearIndex = (GLuint) c;
   if (!ctx->Visual->RGBAflag) {
      /* it's OK to call glClearIndex in RGBA mode but it should be a NOP */
      (*ctx->Driver.ClearIndex)( ctx, ctx->Color.ClearIndex );
   }
}



void
_mesa_ClearColor( GLclampf red, GLclampf green,
                  GLclampf blue, GLclampf alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClearColor");

   ctx->Color.ClearColor[0] = CLAMP( red,   0.0F, 1.0F );
   ctx->Color.ClearColor[1] = CLAMP( green, 0.0F, 1.0F );
   ctx->Color.ClearColor[2] = CLAMP( blue,  0.0F, 1.0F );
   ctx->Color.ClearColor[3] = CLAMP( alpha, 0.0F, 1.0F );

   if (ctx->Visual->RGBAflag) {
      GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
      GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
      GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);
      GLubyte a = (GLint) (ctx->Color.ClearColor[3] * 255.0F);
      (*ctx->Driver.ClearColor)( ctx, r, g, b, a );
   }
}




/*
 * Clear the color buffer when glColorMask or glIndexMask is in effect.
 */
static void
clear_color_buffer_with_masking( GLcontext *ctx )
{
   const GLint x = ctx->DrawBuffer->Xmin;
   const GLint y = ctx->DrawBuffer->Ymin;
   const GLint height = ctx->DrawBuffer->Ymax - ctx->DrawBuffer->Ymin + 1;
   const GLint width  = ctx->DrawBuffer->Xmax - ctx->DrawBuffer->Xmin + 1;

   if (ctx->Visual->RGBAflag) {
      /* RGBA mode */
      const GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
      const GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
      const GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);
      const GLubyte a = (GLint) (ctx->Color.ClearColor[3] * 255.0F);
      GLint i;
      for (i = 0; i < height; i++) {
         GLubyte rgba[MAX_WIDTH][4];
         GLint j;
         for (j=0; j<width; j++) {
            rgba[j][RCOMP] = r;
            rgba[j][GCOMP] = g;
            rgba[j][BCOMP] = b;
            rgba[j][ACOMP] = a;
         }
         gl_mask_rgba_span( ctx, width, x, y + i, rgba );
         (*ctx->Driver.WriteRGBASpan)( ctx, width, x, y + i,
				       (CONST GLubyte (*)[4])rgba, NULL );
      }
   }
   else {
      /* Color index mode */
      GLuint span[MAX_WIDTH];
      GLubyte mask[MAX_WIDTH];
      GLint i, j;
      MEMSET( mask, 1, width );
      for (i=0;i<height;i++) {
         for (j=0;j<width;j++) {
            span[j] = ctx->Color.ClearIndex;
         }
         gl_mask_index_span( ctx, width, x, y + i, span );
         (*ctx->Driver.WriteCI32Span)( ctx, width, x, y + i, span, mask );
      }
   }
}



/*
 * Clear a color buffer without index/channel masking.
 */
static void
clear_color_buffer(GLcontext *ctx)
{
   const GLint x = ctx->DrawBuffer->Xmin;
   const GLint y = ctx->DrawBuffer->Ymin;
   const GLint height = ctx->DrawBuffer->Ymax - ctx->DrawBuffer->Ymin + 1;
   const GLint width  = ctx->DrawBuffer->Xmax - ctx->DrawBuffer->Xmin + 1;

   if (ctx->Visual->RGBAflag) {
      /* RGBA mode */
      const GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
      const GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
      const GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);
      const GLubyte a = (GLint) (ctx->Color.ClearColor[3] * 255.0F);
      GLubyte span[MAX_WIDTH][4];
      GLint i;
      ASSERT(ctx->Color.ColorMask[0] &&
             ctx->Color.ColorMask[1] &&
             ctx->Color.ColorMask[2] &&
             ctx->Color.ColorMask[3]);
      for (i = 0; i < width; i++) {
         span[i][RCOMP] = r;
         span[i][GCOMP] = g;
         span[i][BCOMP] = b;
         span[i][ACOMP] = a;
      }
      for (i = 0; i < height; i++) {
         (*ctx->Driver.WriteRGBASpan)( ctx, width, x, y + i,
                                       (CONST GLubyte (*)[4]) span, NULL );
      }
   }
   else {
      /* Color index mode */
      ASSERT(ctx->Color.IndexMask == ~0);
      if (ctx->Visual->IndexBits == 8) {
         /* 8-bit clear */
         GLubyte span[MAX_WIDTH];
         GLint i;
         MEMSET(span, ctx->Color.ClearIndex, width);
         for (i = 0; i < height; i++) {
            (*ctx->Driver.WriteCI8Span)( ctx, width, x, y + i, span, NULL );
         }
      }
      else {
         /* non 8-bit clear */
         GLuint span[MAX_WIDTH];
         GLint i;
         for (i = 0; i < width; i++) {
            span[i] = ctx->Color.ClearIndex;
         }
         for (i = 0; i < height; i++) {
            (*ctx->Driver.WriteCI32Span)( ctx, width, x, y + i, span, NULL );
         }
      }
   }
}



/*
 * Clear the front/back/left/right color buffers.
 * This function is usually only called if we need to clear the
 * buffers with masking.
 */
static void
clear_color_buffers(GLcontext *ctx)
{
   GLuint bufferBit;

   /* loop over four possible dest color buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit = bufferBit << 1) {
      if (bufferBit & ctx->Color.DrawDestMask) {
         if (bufferBit == FRONT_LEFT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_LEFT);
         }
         else if (bufferBit == FRONT_RIGHT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_RIGHT);
         }
         else if (bufferBit == BACK_LEFT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_LEFT);
         }
         else {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_RIGHT);
         }
         
         if (ctx->Color.SWmasking) {
            clear_color_buffer_with_masking(ctx);
         }
         else {
            clear_color_buffer(ctx);
         }
      }
   }

   /* restore default dest buffer */
   (void) (*ctx->Driver.SetDrawBuffer)( ctx, ctx->Color.DriverDrawBuffer );
}



void 
_mesa_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
#ifdef PROFILE
   GLdouble t0 = gl_time();
#endif
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClear");

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glClear 0x%x\n", mask);

   if (ctx->NewState) {
      gl_update_state( ctx );
   }

   if (ctx->RenderMode==GL_RENDER) {
      const GLint x = ctx->DrawBuffer->Xmin;
      const GLint y = ctx->DrawBuffer->Ymin;
      const GLint height = ctx->DrawBuffer->Ymax - ctx->DrawBuffer->Ymin + 1;
      const GLint width  = ctx->DrawBuffer->Xmax - ctx->DrawBuffer->Xmin + 1;
      GLbitfield ddMask;
      GLbitfield newMask;

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
      newMask = (*ctx->Driver.Clear)( ctx, ddMask, !ctx->Scissor.Enabled,
                                      x, y, width, height );

#ifdef DEBUG
      {
         GLbitfield legalBits = DD_FRONT_LEFT_BIT |
                                DD_FRONT_RIGHT_BIT |
                                DD_BACK_LEFT_BIT |
                                DD_BACK_RIGHT_BIT |
                                DD_DEPTH_BIT |
                                DD_STENCIL_BIT |
                                DD_ACCUM_BIT;
         assert((newMask & (~legalBits)) == 0);
      }
#endif

      /* do software clearing here */
      if (newMask) {
         if (newMask & ctx->Color.DrawDestMask)   clear_color_buffers( ctx );
         if (newMask & GL_DEPTH_BUFFER_BIT)    gl_clear_depth_buffer( ctx );
         if (newMask & GL_ACCUM_BUFFER_BIT)    gl_clear_accum_buffer( ctx );
         if (newMask & GL_STENCIL_BUFFER_BIT)  gl_clear_stencil_buffer( ctx );
      }

      /* clear software-based alpha buffer(s) */
      if ( (mask & GL_COLOR_BUFFER_BIT) && ctx->Visual->SoftwareAlpha
           && ctx->Color.ColorMask[RCOMP]) {
         gl_clear_alpha_buffers( ctx );
      }

#ifdef PROFILE
      ctx->ClearTime += gl_time() - t0;
      ctx->ClearCount++;
#endif
   }
}


void
_mesa_DrawBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDrawBuffer");

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glDrawBuffer %s\n", gl_lookup_enum_by_nr(mode));

   switch (mode) {
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
         /* AUX buffers not implemented in Mesa at this time */
         gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
         return;
      case GL_RIGHT:
         if (!ctx->Visual->StereoFlag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (ctx->Visual->DBflag)
            ctx->Color.DrawDestMask = FRONT_RIGHT_BIT | BACK_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_RIGHT_BIT;
         break;
      case GL_FRONT_RIGHT:
         if (!ctx->Visual->StereoFlag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawDestMask = FRONT_RIGHT_BIT;
         break;
      case GL_BACK_RIGHT:
         if (!ctx->Visual->StereoFlag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (!ctx->Visual->DBflag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawDestMask = BACK_RIGHT_BIT;
         break;
      case GL_BACK_LEFT:
         if (!ctx->Visual->DBflag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawDestMask = BACK_LEFT_BIT;
         break;
      case GL_FRONT_AND_BACK:
         if (!ctx->Visual->DBflag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (ctx->Visual->StereoFlag)
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | BACK_LEFT_BIT
                                    | FRONT_RIGHT_BIT | BACK_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | BACK_LEFT_BIT;
         break;
      case GL_BACK:
         if (!ctx->Visual->DBflag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
            return;
         }
         if (ctx->Visual->StereoFlag)
            ctx->Color.DrawDestMask = BACK_LEFT_BIT | BACK_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = BACK_LEFT_BIT;
         break;
      case GL_LEFT:
         /* never an error */
         if (ctx->Visual->DBflag)
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
         if (ctx->Visual->StereoFlag)
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT | FRONT_RIGHT_BIT;
         else
            ctx->Color.DrawDestMask = FRONT_LEFT_BIT;
         break;
      case GL_NONE:
         /* never an error */
         ctx->Color.DrawDestMask = 0;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
         return;
   }

   /*
    * Make the dest buffer mode more precise if possible
    */
   if (mode == GL_LEFT && !ctx->Visual->DBflag)
      ctx->Color.DriverDrawBuffer = GL_FRONT_LEFT;
   else if (mode == GL_RIGHT && !ctx->Visual->DBflag)
      ctx->Color.DriverDrawBuffer = GL_FRONT_RIGHT;
   else if (mode == GL_FRONT && !ctx->Visual->StereoFlag)
      ctx->Color.DriverDrawBuffer = GL_FRONT_LEFT;
   else if (mode == GL_BACK && !ctx->Visual->StereoFlag)
      ctx->Color.DriverDrawBuffer = GL_BACK_LEFT;
   else
      ctx->Color.DriverDrawBuffer = mode;

   /*
    * Set current alpha buffer pointer
    */
   if (ctx->Visual->SoftwareAlpha) {
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
   ctx->NewState |= NEW_RASTER_OPS;
}



void
_mesa_ReadBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glReadBuffer");

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glReadBuffer %s\n", gl_lookup_enum_by_nr(mode));

   switch (mode) {
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
         /* AUX buffers not implemented in Mesa at this time */
         gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
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
         if (!ctx->Visual->DBflag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.DriverReadBuffer = GL_BACK_LEFT;
         break;
      case GL_FRONT_RIGHT:
      case GL_RIGHT:
         if (!ctx->Visual->StereoFlag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.DriverReadBuffer = GL_FRONT_RIGHT;
         break;
      case GL_BACK_RIGHT:
         if (!ctx->Visual->StereoFlag || !ctx->Visual->DBflag) {
            gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.DriverReadBuffer = GL_BACK_RIGHT;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
         return;
   }

   ctx->Pixel.ReadBuffer = mode;
   ctx->NewState |= NEW_RASTER_OPS;
}


/*
 * GL_MESA_resize_buffers extension
 */
void
_mesa_ResizeBuffersMESA( void )
{
   GLcontext *ctx = gl_get_current_context();

   GLuint buf_width, buf_height;

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glResizeBuffersMESA\n");

   /* ask device driver for size of output buffer */
   (*ctx->Driver.GetBufferSize)( ctx, &buf_width, &buf_height );

   /* see if size of device driver's color buffer (window) has changed */
   if (ctx->DrawBuffer->Width == (GLint) buf_width &&
       ctx->DrawBuffer->Height == (GLint) buf_height)
      return;

   ctx->NewState |= NEW_RASTER_OPS;  /* to update scissor / window bounds */

   /* save buffer size */
   ctx->DrawBuffer->Width = buf_width;
   ctx->DrawBuffer->Height = buf_height;

   /* Reallocate other buffers if needed. */
   if (ctx->DrawBuffer->UseSoftwareDepthBuffer) {
      gl_alloc_depth_buffer( ctx );
   }
   if (ctx->DrawBuffer->UseSoftwareStencilBuffer) {
      gl_alloc_stencil_buffer( ctx );
   }
   if (ctx->DrawBuffer->UseSoftwareAccumBuffer) {
      gl_alloc_accum_buffer( ctx );
   }
   if (ctx->Visual->SoftwareAlpha) {
      gl_alloc_alpha_buffers( ctx );
   }
}
