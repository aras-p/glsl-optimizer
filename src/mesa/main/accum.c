/* $Id: accum.c,v 1.22 2000/07/14 04:15:37 brianp Exp $ */

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
#include "context.h"
#include "mem.h"
#include "masking.h"
#include "span.h"
#include "state.h"
#include "types.h"
#endif


/*
 * Accumulation buffer notes
 *
 * Normally, accumulation buffer values are GLshorts with values in
 * [-32767, 32767] which represent floating point colors in [-1, 1],
 * as suggested by the OpenGL specification.
 *
 * We optimize for the common case used for full-scene antialiasing:
 *    // start with accum buffer cleared to zero
 *    glAccum(GL_LOAD, w);   // or GL_ACCUM the first image
 *    glAccum(GL_ACCUM, w);
 *    ...
 *    glAccum(GL_ACCUM, w);
 *    glAccum(GL_RETURN, 1.0);
 * That is, we start with an empty accumulation buffer and accumulate
 * n images, each with weight w = 1/n.
 * In this scenario, we can simply store unscaled integer values in
 * the accum buffer instead of scaled integers.  We'll also keep track
 * of the w value so when we do GL_RETURN we simply divide the accumulated
 * values by n (=1/w).
 * This lets us avoid _many_ int->float->int conversions.
 */


#define USE_OPTIMIZED_ACCUM   /* enable the optimization */



void
_mesa_alloc_accum_buffer( GLcontext *ctx )
{
   GLint n;

   if (ctx->DrawBuffer->Accum) {
      FREE( ctx->DrawBuffer->Accum );
      ctx->DrawBuffer->Accum = NULL;
   }

   /* allocate accumulation buffer if not already present */
   n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height * 4 * sizeof(GLaccum);
   ctx->DrawBuffer->Accum = (GLaccum *) MALLOC( n );
   if (!ctx->DrawBuffer->Accum) {
      /* unable to setup accumulation buffer */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glAccum" );
   }
#ifdef USE_OPTIMIZED_ACCUM
   ctx->IntegerAccumMode = GL_TRUE;
#else
   ctx->IntegerAccumMode = GL_FALSE;
#endif
   ctx->IntegerAccumScaler = 0.0;
}



void
_mesa_ClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glAccum");

   ctx->Accum.ClearColor[0] = CLAMP( red, -1.0, 1.0 );
   ctx->Accum.ClearColor[1] = CLAMP( green, -1.0, 1.0 );
   ctx->Accum.ClearColor[2] = CLAMP( blue, -1.0, 1.0 );
   ctx->Accum.ClearColor[3] = CLAMP( alpha, -1.0, 1.0 );
}



/*
 * This is called when we fall out of optimized/unscaled accum buffer mode.
 * That is, we convert each unscaled accum buffer value into a scaled value
 * representing the range[-1, 1].
 */
static void rescale_accum( GLcontext *ctx )
{
   const GLuint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height * 4;
   const GLfloat fChanMax = (1 << (sizeof(GLchan) * 8)) - 1;
   const GLfloat s = ctx->IntegerAccumScaler * (32767.0 / fChanMax);
   GLaccum *accum = ctx->DrawBuffer->Accum;
   GLuint i;

   assert(ctx->IntegerAccumMode);
   assert(accum);

   for (i = 0; i < n; i++) {
      accum[i] = (GLaccum) (accum[i] * s);
   }

   ctx->IntegerAccumMode = GL_FALSE;
}



void
_mesa_Accum( GLenum op, GLfloat value )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint xpos, ypos, width, height, width4;
   GLfloat acc_scale;
   GLubyte rgba[MAX_WIDTH][4];
   const GLint iChanMax = (1 << (sizeof(GLchan) * 8)) - 1;
   const GLfloat fChanMax = (1 << (sizeof(GLchan) * 8)) - 1;
   
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glAccum");

   if (ctx->Visual->AccumRedBits == 0 || ctx->DrawBuffer != ctx->ReadBuffer) {
      gl_error(ctx, GL_INVALID_OPERATION, "glAccum");
      return;
   }

   if (!ctx->DrawBuffer->Accum) {
      gl_warning(ctx, "Calling glAccum() without an accumulation buffer (low memory?)");
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      /* sizeof(GLaccum) > 2 (Cray) */
      acc_scale = (float) SHRT_MAX;
   }

   if (ctx->NewState)
      gl_update_state( ctx );

   /* Determine region to operate upon. */
   if (ctx->Scissor.Enabled) {
      xpos = ctx->Scissor.X;
      ypos = ctx->Scissor.Y;
      width = ctx->Scissor.Width;
      height = ctx->Scissor.Height;
   }
   else {
      /* whole window */
      xpos = 0;
      ypos = 0;
      width = ctx->DrawBuffer->Width;
      height = ctx->DrawBuffer->Height;
   }

   width4 = 4 * width;

   switch (op) {
      case GL_ADD:
         if (value != 0.0F) {
	    const GLaccum intVal = (GLaccum) (value * acc_scale);
	    GLuint j;
            /* Leave optimized accum buffer mode */
            if (ctx->IntegerAccumMode)
               rescale_accum(ctx);
	    for (j = 0; j < height; j++) {
	       GLaccum * acc = ctx->DrawBuffer->Accum + ypos * width4 + 4 * xpos;
               GLuint i;
	       for (i = 0; i < width4; i++) {
                  acc[i] += intVal;
	       }
	       ypos++;
	    }
	 }
	 break;

      case GL_MULT:
         if (value != 1.0F) {
	    GLuint j;
            /* Leave optimized accum buffer mode */
            if (ctx->IntegerAccumMode)
               rescale_accum(ctx);
	    for (j = 0; j < height; j++) {
	       GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + 4 * xpos;
               GLuint i;
	       for (i = 0; i < width4; i++) {
                  acc[i] = (GLaccum) ( (GLfloat) acc[i] * value );
	       }
	       ypos++;
	    }
	 }
	 break;

      case GL_ACCUM:
         if (value == 0.0F)
            return;

         (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                       ctx->Pixel.DriverReadBuffer );

         /* May have to leave optimized accum buffer mode */
         if (ctx->IntegerAccumScaler == 0.0 && value > 0.0 && value <= 1.0)
            ctx->IntegerAccumScaler = value;
         if (ctx->IntegerAccumMode && value != ctx->IntegerAccumScaler)
            rescale_accum(ctx);
            
         if (ctx->IntegerAccumMode) {
            /* simply add integer color values into accum buffer */
            GLuint j;
            GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
            assert(ctx->IntegerAccumScaler > 0.0);
            assert(ctx->IntegerAccumScaler <= 1.0);
            for (j = 0; j < height; j++) {
               
               GLuint i, i4;
               gl_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i = i4 = 0; i < width; i++, i4+=4) {
                  acc[i4+0] += rgba[i][RCOMP];
                  acc[i4+1] += rgba[i][GCOMP];
                  acc[i4+2] += rgba[i][BCOMP];
                  acc[i4+3] += rgba[i][ACOMP];
               }
               acc += width4;
               ypos++;
            }
         }
         else {
            /* scaled integer accum buffer */
            const GLfloat rscale = value * acc_scale / fChanMax;
            const GLfloat gscale = value * acc_scale / fChanMax;
            const GLfloat bscale = value * acc_scale / fChanMax;
            const GLfloat ascale = value * acc_scale / fChanMax;
            GLuint j;
            for (j=0;j<height;j++) {
               GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
               GLuint i;
               gl_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i=0;i<width;i++) {
                  *acc += (GLaccum) ( (GLfloat) rgba[i][RCOMP] * rscale );  acc++;
                  *acc += (GLaccum) ( (GLfloat) rgba[i][GCOMP] * gscale );  acc++;
                  *acc += (GLaccum) ( (GLfloat) rgba[i][BCOMP] * bscale );  acc++;
                  *acc += (GLaccum) ( (GLfloat) rgba[i][ACOMP] * ascale );  acc++;
               }
               ypos++;
            }
         }
         /* restore read buffer = draw buffer (the default) */
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                       ctx->Color.DriverDrawBuffer );
	 break;

      case GL_LOAD:
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                       ctx->Pixel.DriverReadBuffer );

         /* This is a change to go into optimized accum buffer mode */
         if (value > 0.0 && value <= 1.0) {
#ifdef USE_OPTIMIZED_ACCUM
            ctx->IntegerAccumMode = GL_TRUE;
#else
            ctx->IntegerAccumMode = GL_FALSE;
#endif
            ctx->IntegerAccumScaler = value;
         }
         else {
            ctx->IntegerAccumMode = GL_FALSE;
            ctx->IntegerAccumScaler = 0.0;
         }

         if (ctx->IntegerAccumMode) {
            /* just copy values into accum buffer */
            GLuint j;
            GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
            assert(ctx->IntegerAccumScaler > 0.0);
            assert(ctx->IntegerAccumScaler <= 1.0);
            for (j = 0; j < height; j++) {
               GLuint i, i4;
               gl_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i = i4 = 0; i < width; i++, i4 += 4) {
                  acc[i4+0] = rgba[i][RCOMP];
                  acc[i4+1] = rgba[i][GCOMP];
                  acc[i4+2] = rgba[i][BCOMP];
                  acc[i4+3] = rgba[i][ACOMP];
               }
               acc += width4;
               ypos++;
            }
         }
         else {
            /* scaled integer accum buffer */
            const GLfloat rscale = value * acc_scale / fChanMax;
            const GLfloat gscale = value * acc_scale / fChanMax;
            const GLfloat bscale = value * acc_scale / fChanMax;
            const GLfloat ascale = value * acc_scale / fChanMax;
            const GLfloat d = 3.0 / acc_scale;
            GLuint i, j;
            for (j = 0; j < height; j++) {
               GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
               gl_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i=0;i<width;i++) {
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][RCOMP] * rscale + d);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][GCOMP] * gscale + d);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][BCOMP] * bscale + d);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][ACOMP] * ascale + d);
               }
               ypos++;
            }
         }

         /* restore read buffer = draw buffer (the default) */
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                       ctx->Color.DriverDrawBuffer );
	 break;

      case GL_RETURN:
         /* May have to leave optimized accum buffer mode */
         if (ctx->IntegerAccumMode && value != 1.0)
            rescale_accum(ctx);

         if (ctx->IntegerAccumMode && ctx->IntegerAccumScaler > 0
             && (GLint) (256 / ctx->IntegerAccumScaler) <= 32768) {
            /* build lookup table to avoid many floating point multiplies */
            const GLfloat mult = ctx->IntegerAccumScaler;
            static GLchan multTable[32768];
            static GLfloat prevMult = 0.0;
            GLuint j;
            const GLint max = (GLint) (256 / mult);
            if (mult != prevMult) {
               assert(max <= 32768);
               for (j = 0; j < max; j++)
                  multTable[j] = (GLint) ((GLfloat) j * mult + 0.5F);
               prevMult = mult;
            }

            assert(ctx->IntegerAccumScaler > 0.0);
            assert(ctx->IntegerAccumScaler <= 1.0);
            for (j = 0; j < height; j++) {
               const GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos*4;
               GLuint i, i4;
               for (i = i4 = 0; i < width; i++, i4 += 4) {
                  ASSERT(acc[i4+0] < max);
                  ASSERT(acc[i4+1] < max);
                  ASSERT(acc[i4+2] < max);
                  ASSERT(acc[i4+3] < max);
                  rgba[i][RCOMP] = multTable[acc[i4+0]];
                  rgba[i][GCOMP] = multTable[acc[i4+1]];
                  rgba[i][BCOMP] = multTable[acc[i4+2]];
                  rgba[i][ACOMP] = multTable[acc[i4+3]];
               }
               if (ctx->Color.SWmasking) {
                  _mesa_mask_rgba_span( ctx, width, xpos, ypos, rgba );
               }
               (*ctx->Driver.WriteRGBASpan)( ctx, width, xpos, ypos, 
                                             (const GLubyte (*)[4])rgba, NULL );
               ypos++;
            }
         }
         else {
            const GLfloat rscale = value / acc_scale * fChanMax;
            const GLfloat gscale = value / acc_scale * fChanMax;
            const GLfloat bscale = value / acc_scale * fChanMax;
            const GLfloat ascale = value / acc_scale * fChanMax;
            GLuint i, j;
            for (j=0;j<height;j++) {
               const GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos*4;
               for (i=0;i<width;i++) {
                  GLint r, g, b, a;
                  r = (GLint) ( (GLfloat) (*acc++) * rscale + 0.5F );
                  g = (GLint) ( (GLfloat) (*acc++) * gscale + 0.5F );
                  b = (GLint) ( (GLfloat) (*acc++) * bscale + 0.5F );
                  a = (GLint) ( (GLfloat) (*acc++) * ascale + 0.5F );
                  rgba[i][RCOMP] = CLAMP( r, 0, iChanMax );
                  rgba[i][GCOMP] = CLAMP( g, 0, iChanMax );
                  rgba[i][BCOMP] = CLAMP( b, 0, iChanMax );
                  rgba[i][ACOMP] = CLAMP( a, 0, iChanMax );
               }
               if (ctx->Color.SWmasking) {
                  _mesa_mask_rgba_span( ctx, width, xpos, ypos, rgba );
               }
               (*ctx->Driver.WriteRGBASpan)( ctx, width, xpos, ypos, 
                                             (const GLubyte (*)[4])rgba, NULL );
               ypos++;
            }
	 }
	 break;

      default:
         gl_error( ctx, GL_INVALID_ENUM, "glAccum" );
   }
}



/*
 * Clear the accumulation Buffer.
 */
void
_mesa_clear_accum_buffer( GLcontext *ctx )
{
   GLuint buffersize;
   GLfloat acc_scale;

   if (ctx->Visual->AccumRedBits==0) {
      /* No accumulation buffer! */
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      /* sizeof(GLaccum) > 2 (Cray) */
      acc_scale = (float) SHRT_MAX;
   }

   /* number of pixels */
   buffersize = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;

   if (!ctx->DrawBuffer->Accum) {
      /* try to alloc accumulation buffer */
      ctx->DrawBuffer->Accum = (GLaccum *)
	                   MALLOC( buffersize * 4 * sizeof(GLaccum) );
   }

   if (ctx->DrawBuffer->Accum) {
      if (ctx->Scissor.Enabled) {
	 /* Limit clear to scissor box */
	 GLaccum r, g, b, a;
	 GLint i, j;
         GLint width, height;
         GLaccum *row;
	 r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	 g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	 b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	 a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
         /* size of region to clear */
         width = 4 * (ctx->DrawBuffer->Xmax - ctx->DrawBuffer->Xmin + 1);
         height = ctx->DrawBuffer->Ymax - ctx->DrawBuffer->Ymin + 1;
         /* ptr to first element to clear */
         row = ctx->DrawBuffer->Accum
               + 4 * (ctx->DrawBuffer->Ymin * ctx->DrawBuffer->Width
                      + ctx->DrawBuffer->Xmin);
         for (j=0;j<height;j++) {
            for (i=0;i<width;i+=4) {
               row[i+0] = r;
               row[i+1] = g;
               row[i+2] = b;
               row[i+3] = a;
	    }
            row += 4 * ctx->DrawBuffer->Width;
	 }
      }
      else {
	 /* clear whole buffer */
	 if (ctx->Accum.ClearColor[0]==0.0 &&
	     ctx->Accum.ClearColor[1]==0.0 &&
	     ctx->Accum.ClearColor[2]==0.0 &&
	     ctx->Accum.ClearColor[3]==0.0) {
	    /* Black */
	    BZERO( ctx->DrawBuffer->Accum, buffersize * 4 * sizeof(GLaccum) );
	 }
	 else {
	    /* Not black */
	    GLaccum *acc, r, g, b, a;
	    GLuint i;

	    acc = ctx->DrawBuffer->Accum;
	    r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	    g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	    b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	    a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
	    for (i=0;i<buffersize;i++) {
	       *acc++ = r;
	       *acc++ = g;
	       *acc++ = b;
	       *acc++ = a;
	    }
	 }
      }

      /* update optimized accum state vars */
      if (ctx->Accum.ClearColor[0] == 0.0 && ctx->Accum.ClearColor[1] == 0.0 &&
          ctx->Accum.ClearColor[2] == 0.0 && ctx->Accum.ClearColor[3] == 0.0) {
#ifdef USE_OPTIMIZED_ACCUM
         ctx->IntegerAccumMode = GL_TRUE;
#else
         ctx->IntegerAccumMode = GL_FALSE;
#endif
         ctx->IntegerAccumScaler = 0.0;  /* denotes empty accum buffer */
      }
      else {
         ctx->IntegerAccumMode = GL_FALSE;
      }
   }
}
