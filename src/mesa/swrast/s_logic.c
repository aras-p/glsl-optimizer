/* $Id: s_logic.c,v 1.7 2001/05/10 16:54:12 brianp Exp $ */

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
#include "context.h"
#include "macros.h"

#include "s_alphabuf.h"
#include "s_context.h"
#include "s_logic.h"
#include "s_pb.h"
#include "s_span.h"



/*
 * Apply logic op to array of CI pixels.
 */
static void index_logicop( GLcontext *ctx, GLuint n,
                           GLuint index[], const GLuint dest[],
                           const GLubyte mask[] )
{
   GLuint i;
   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = 0;
	    }
	 }
	 break;
      case GL_SET:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~0;
	    }
	 }
	 break;
      case GL_COPY:
	 /* do nothing */
	 break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i];
	    }
	 }
	 break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = dest[i];
	    }
	 }
	 break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~dest[i];
	    }
	 }
	 break;
      case GL_AND:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] &= dest[i];
	    }
	 }
	 break;
      case GL_NAND:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] & dest[i]);
	    }
	 }
	 break;
      case GL_OR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] |= dest[i];
	    }
	 }
	 break;
      case GL_NOR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] | dest[i]);
	    }
	 }
	 break;
      case GL_XOR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] ^= dest[i];
	    }
	 }
	 break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] ^ dest[i]);
	    }
	 }
	 break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = index[i] & ~dest[i];
	    }
	 }
	 break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i] & dest[i];
	    }
	 }
	 break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = index[i] | ~dest[i];
	    }
	 }
	 break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i] | dest[i];
	    }
	 }
	 break;
      default:
	 _mesa_problem(ctx, "bad mode in index_logic()");
   }
}



/*
 * Apply the current logic operator to a span of CI pixels.  This is only
 * used if the device driver can't do logic ops.
 */
void
_mesa_logicop_ci_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       GLuint index[], const GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint dest[MAX_WIDTH];
   /* Read dest values from frame buffer */
   (*swrast->Driver.ReadCI32Span)( ctx, n, x, y, dest );
   index_logicop( ctx, n, index, dest, mask );
}



/*
 * Apply the current logic operator to an array of CI pixels.  This is only
 * used if the device driver can't do logic ops.
 */
void
_mesa_logicop_ci_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLuint index[], const GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint dest[PB_SIZE];
   /* Read dest values from frame buffer */
   (*swrast->Driver.ReadCI32Pixels)( ctx, n, x, y, dest, mask );
   index_logicop( ctx, n, index, dest, mask );
}



/*
 * Apply logic operator to rgba pixels.
 * Input:  ctx - the context
 *         n - number of pixels
 *         mask - pixel mask array
 * In/Out:  src - incoming pixels which will be modified
 * Input:  dest - frame buffer values
 *
 * Note:  Since the R, G, B, and A channels are all treated the same we
 * process them as 4-byte GLuints instead of four GLubytes.
 */
static void rgba_logicop_ui( const GLcontext *ctx, GLuint n,
                             const GLubyte mask[],
                             GLuint src[], const GLuint dest[] )
{
   GLuint i;
   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = 0;
            }
         }
         break;
      case GL_SET:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~0;
            }
         }
         break;
      case GL_COPY:
         /* do nothing */
         break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~src[i];
            }
         }
         break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = dest[i];
            }
         }
         break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~dest[i];
            }
         }
         break;
      case GL_AND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] &= dest[i];
            }
         }
         break;
      case GL_NAND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~(src[i] & dest[i]);
            }
         }
         break;
      case GL_OR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i]|= dest[i];
            }
         }
         break;
      case GL_NOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~(src[i] | dest[i]);
            }
         }
         break;
      case GL_XOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] ^= dest[i];
            }
         }
         break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~(src[i] ^ dest[i]);
            }
         }
         break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = src[i] & ~dest[i];
            }
         }
         break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~src[i] & dest[i];
            }
         }
         break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = src[i] | ~dest[i];
            }
         }
         break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~src[i] | dest[i];
            }
         }
         break;
      default:
         /* should never happen */
         _mesa_problem(ctx, "Bad function in rgba_logicop");
   }
}


/*
 * As above, but operate on GLchan values
 * Note: need to pass n = numPixels * 4.
 */
static void rgba_logicop_chan( const GLcontext *ctx, GLuint n,
                               const GLubyte mask[],
                               GLchan src[], const GLchan dest[] )
{
   GLuint i;
   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = 0;
            }
         }
         break;
      case GL_SET:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~0;
            }
         }
         break;
      case GL_COPY:
         /* do nothing */
         break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~src[i];
            }
         }
         break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = dest[i];
            }
         }
         break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~dest[i];
            }
         }
         break;
      case GL_AND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] &= dest[i];
            }
         }
         break;
      case GL_NAND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~(src[i] & dest[i]);
            }
         }
         break;
      case GL_OR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i]|= dest[i];
            }
         }
         break;
      case GL_NOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~(src[i] | dest[i]);
            }
         }
         break;
      case GL_XOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] ^= dest[i];
            }
         }
         break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~(src[i] ^ dest[i]);
            }
         }
         break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = src[i] & ~dest[i];
            }
         }
         break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~src[i] & dest[i];
            }
         }
         break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = src[i] | ~dest[i];
            }
         }
         break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               src[i] = ~src[i] | dest[i];
            }
         }
         break;
      default:
         /* should never happen */
         _mesa_problem(ctx, "Bad function in rgba_logicop");
   }
}



/*
 * Apply the current logic operator to a span of RGBA pixels.
 * This is only used if the device driver can't do logic ops.
 */
void
_mesa_logicop_rgba_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         GLchan rgba[][4], const GLubyte mask[] )
{
   GLchan dest[MAX_WIDTH][4];
   _mesa_read_rgba_span( ctx, ctx->DrawBuffer, n, x, y, dest );
   if (sizeof(GLchan) * 4 == sizeof(GLuint)) {
      rgba_logicop_ui(ctx, n, mask, (GLuint *) rgba, (const GLuint *) dest);
   }
   else {
      rgba_logicop_chan(ctx, 4 * n, mask,
                        (GLchan *) rgba, (const GLchan *) dest);
   }
}



/*
 * Apply the current logic operator to an array of RGBA pixels.
 * This is only used if the device driver can't do logic ops.
 */
void
_mesa_logicop_rgba_pixels( GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLchan rgba[][4], const GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan dest[PB_SIZE][4];
   (*swrast->Driver.ReadRGBAPixels)( ctx, n, x, y, dest, mask );
   if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
      _mesa_read_alpha_pixels( ctx, n, x, y, dest, mask );
   }
   if (sizeof(GLchan) * 4 == sizeof(GLuint)) {
      rgba_logicop_ui(ctx, n, mask, (GLuint *) rgba, (const GLuint *) dest);
   }
   else {
      rgba_logicop_chan(ctx, 4 * n, mask,
                        (GLchan *) rgba, (const GLchan *) dest);
   }
}
