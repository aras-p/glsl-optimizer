/* $Id: s_imaging.c,v 1.1 2000/10/31 18:00:04 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.4
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


/*
 * Histogram, Min/max and convolution for GL_ARB_imaging subset
 *
 */


#include "glheader.h"
#include "colormac.h"
#include "image.h"
#include "mmath.h"

#include "s_imaging.h"
#include "s_span.h"



/*
 * Update the min/max values from an array of fragment colors.
 */
void
_mesa_update_minmax(GLcontext *ctx, GLuint n, const GLfloat rgba[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      /* update mins */
      if (rgba[i][RCOMP] < ctx->MinMax.Min[RCOMP])
         ctx->MinMax.Min[RCOMP] = rgba[i][RCOMP];
      if (rgba[i][GCOMP] < ctx->MinMax.Min[GCOMP])
         ctx->MinMax.Min[GCOMP] = rgba[i][GCOMP];
      if (rgba[i][BCOMP] < ctx->MinMax.Min[BCOMP])
         ctx->MinMax.Min[BCOMP] = rgba[i][BCOMP];
      if (rgba[i][ACOMP] < ctx->MinMax.Min[ACOMP])
         ctx->MinMax.Min[ACOMP] = rgba[i][ACOMP];

      /* update maxs */
      if (rgba[i][RCOMP] > ctx->MinMax.Max[RCOMP])
         ctx->MinMax.Max[RCOMP] = rgba[i][RCOMP];
      if (rgba[i][GCOMP] > ctx->MinMax.Max[GCOMP])
         ctx->MinMax.Max[GCOMP] = rgba[i][GCOMP];
      if (rgba[i][BCOMP] > ctx->MinMax.Max[BCOMP])
         ctx->MinMax.Max[BCOMP] = rgba[i][BCOMP];
      if (rgba[i][ACOMP] > ctx->MinMax.Max[ACOMP])
         ctx->MinMax.Max[ACOMP] = rgba[i][ACOMP];
   }
}


/*
 * Update the histogram values from an array of fragment colors.
 */
void
_mesa_update_histogram(GLcontext *ctx, GLuint n, const GLfloat rgba[][4])
{
   const GLint max = ctx->Histogram.Width - 1;
   GLfloat w = (GLfloat) max;
   GLuint i;

   if (ctx->Histogram.Width == 0)
      return;
   
   for (i = 0; i < n; i++) {
      GLint ri = (GLint) (rgba[i][RCOMP] * w + 0.5F);
      GLint gi = (GLint) (rgba[i][GCOMP] * w + 0.5F);
      GLint bi = (GLint) (rgba[i][BCOMP] * w + 0.5F);
      GLint ai = (GLint) (rgba[i][ACOMP] * w + 0.5F);
      ri = CLAMP(ri, 0, max);
      gi = CLAMP(gi, 0, max);
      bi = CLAMP(bi, 0, max);
      ai = CLAMP(ai, 0, max);
      ctx->Histogram.Count[ri][RCOMP]++;
      ctx->Histogram.Count[gi][GCOMP]++;
      ctx->Histogram.Count[bi][BCOMP]++;
      ctx->Histogram.Count[ai][ACOMP]++;
   }
}



