/* $Id: s_masking.c,v 1.6 2002/02/02 17:24:11 brianp Exp $ */

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
 * Implement the effect of glColorMask and glIndexMask in software.
 */


#include "glheader.h"
#include "enums.h"
#include "macros.h"

#include "s_alphabuf.h"
#include "s_context.h"
#include "s_masking.h"
#include "s_pb.h"
#include "s_span.h"



void
_mesa_mask_rgba_span( GLcontext *ctx, const struct sw_span *span,
                     GLchan rgba[][4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan dest[MAX_WIDTH][4];
#if CHAN_BITS == 8
   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;
#else
   const GLboolean rMask = ctx->Color.ColorMask[RCOMP];
   const GLboolean gMask = ctx->Color.ColorMask[GCOMP];
   const GLboolean bMask = ctx->Color.ColorMask[BCOMP];
   const GLboolean aMask = ctx->Color.ColorMask[ACOMP];
#endif
   const GLuint n = span->end;
   GLuint i;

   ASSERT(n < MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_RGBA);

   if (span->arrayMask & SPAN_XY) {
      (*swrast->Driver.ReadRGBAPixels)(ctx, n, span->xArray, span->yArray,
                                       dest, span->mask);
      if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
         _mesa_read_alpha_pixels(ctx, n, span->xArray, span->yArray,
                                 dest, span->mask );
      }
   }
   else {
      _mesa_read_rgba_span(ctx, ctx->DrawBuffer, n, span->x, span->y, dest);
   }

#if CHAN_BITS == 8
   for (i = 0; i < n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }
#else
   for (i = 0; i < n; i++) {
      if (!rMask)  rgba[i][RCOMP] = dest[i][RCOMP];
      if (!gMask)  rgba[i][GCOMP] = dest[i][GCOMP];
      if (!bMask)  rgba[i][BCOMP] = dest[i][BCOMP];
      if (!aMask)  rgba[i][ACOMP] = dest[i][ACOMP];
   }
#endif
}




/*
 * Apply glColorMask to a span of RGBA pixels.
 */
void
_mesa_mask_rgba_array( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLchan rgba[][4] )
{
   GLchan dest[MAX_WIDTH][4];
   GLuint i;

#if CHAN_BITS == 8

   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;

   _mesa_read_rgba_span( ctx, ctx->DrawBuffer, n, x, y, dest );
   for (i = 0; i < n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }

#else

   const GLint rMask = ctx->Color.ColorMask[RCOMP];
   const GLint gMask = ctx->Color.ColorMask[GCOMP];
   const GLint bMask = ctx->Color.ColorMask[BCOMP];
   const GLint aMask = ctx->Color.ColorMask[ACOMP];

   _mesa_read_rgba_span( ctx, ctx->DrawBuffer, n, x, y, dest );
   for (i = 0; i < n; i++) {
      if (!rMask)  rgba[i][RCOMP] = dest[i][RCOMP];
      if (!gMask)  rgba[i][GCOMP] = dest[i][GCOMP];
      if (!bMask)  rgba[i][BCOMP] = dest[i][BCOMP];
      if (!aMask)  rgba[i][ACOMP] = dest[i][ACOMP];
   }

#endif
}



/*
 * Apply glColorMask to an array of RGBA pixels.
 */
void
_mesa_mask_rgba_pixels( GLcontext *ctx,
                        GLuint n, const GLint x[], const GLint y[],
                        GLchan rgba[][4], const GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan dest[PB_SIZE][4];
   GLuint i;

#if CHAN_BITS == 8

   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;

   (*swrast->Driver.ReadRGBAPixels)( ctx, n, x, y, dest, mask );
   if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
      _mesa_read_alpha_pixels( ctx, n, x, y, dest, mask );
   }

   for (i=0; i<n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }

#else

   const GLint rMask = ctx->Color.ColorMask[RCOMP];
   const GLint gMask = ctx->Color.ColorMask[GCOMP];
   const GLint bMask = ctx->Color.ColorMask[BCOMP];
   const GLint aMask = ctx->Color.ColorMask[ACOMP];

   (*swrast->Driver.ReadRGBAPixels)( ctx, n, x, y, dest, mask );
   if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
      _mesa_read_alpha_pixels( ctx, n, x, y, dest, mask );
   }

   for (i = 0; i < n; i++) {
      if (!rMask)  rgba[i][RCOMP] = dest[i][RCOMP];
      if (!gMask)  rgba[i][GCOMP] = dest[i][GCOMP];
      if (!bMask)  rgba[i][BCOMP] = dest[i][BCOMP];
      if (!aMask)  rgba[i][ACOMP] = dest[i][ACOMP];
   }

#endif
}



void
_mesa_mask_index_span( GLcontext *ctx, const struct sw_span *span,
                       GLuint index[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint msrc = ctx->Color.IndexMask;
   const GLuint mdest = ~msrc;
   GLuint fbindexes[MAX_WIDTH];
   GLuint i;

   ASSERT(span->arrayMask & SPAN_INDEX);
   ASSERT(span->end < MAX_WIDTH);

   if (span->arrayMask & SPAN_XY) {

      (*swrast->Driver.ReadCI32Pixels)(ctx, span->end, span->xArray,
                                       span->yArray, fbindexes, span->mask);

      for (i = 0; i < span->end; i++) {
         index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
      }
   }
   else {
      _mesa_read_index_span(ctx, ctx->DrawBuffer, span->end, span->x, span->y,
                            fbindexes );

      for (i = 0; i < span->end; i++) {
         index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
      }
   }
}



/*
 * Apply glIndexMask to a span of CI pixels.
 */
void
_mesa_mask_index_array( GLcontext *ctx,
                        GLuint n, GLint x, GLint y, GLuint index[] )
{
   GLuint i;
   GLuint fbindexes[MAX_WIDTH];
   GLuint msrc, mdest;

   _mesa_read_index_span( ctx, ctx->DrawBuffer, n, x, y, fbindexes );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}



/*
 * Apply glIndexMask to an array of CI pixels.
 */
void
_mesa_mask_index_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLuint index[], const GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;
   GLuint fbindexes[PB_SIZE];
   GLuint msrc, mdest;

   (*swrast->Driver.ReadCI32Pixels)( ctx, n, x, y, fbindexes, mask );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}

