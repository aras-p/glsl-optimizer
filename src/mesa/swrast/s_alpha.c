/* $Id: s_alpha.c,v 1.6 2002/01/27 18:32:03 brianp Exp $ */

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


#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "mmath.h"

#include "s_alpha.h"


/*
 * Apply the alpha test to a span of pixels.
 * In:  rgba - array of pixels
 * In/Out:  span -
 * Return:  0 = all pixels in the span failed the alpha test.
 *          1 = one or more pixels passed the alpha test.
 */
GLint
_mesa_alpha_test( const GLcontext *ctx, struct sw_span *span,
                  CONST GLchan rgba[][4])
{
   GLuint i;
   const GLchan ref = ctx->Color.AlphaRef;
   GLubyte *mask = span->mask;

   ASSERT (span->filledAlpha == GL_TRUE || (span->arrayMask & SPAN_RGBA));

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Color.AlphaFunc) {
      case GL_LESS:
         for (i=span->start; i<span->end; i++) {
	    mask[i] &= (rgba[i][ACOMP] < ref);
	 }
	 break;
      case GL_LEQUAL:
         for (i=span->start; i<span->end; i++)
	    mask[i] &= (rgba[i][ACOMP] <= ref);
	 break;
      case GL_GEQUAL:
         for (i=span->start; i<span->end; i++) {
	    mask[i] &= (rgba[i][ACOMP] >= ref);
	 }
	 break;
      case GL_GREATER:
         for (i=span->start; i<span->end; i++) {
	    mask[i] &= (rgba[i][ACOMP] > ref);
	 }
	 break;
      case GL_NOTEQUAL:
         for (i=span->start; i<span->end; i++) {
	    mask[i] &= (rgba[i][ACOMP] != ref);
	 }
	 break;
      case GL_EQUAL:
         for (i=span->start; i<span->end; i++) {
	    mask[i] &= (rgba[i][ACOMP] == ref);
	 }
	 break;
      case GL_ALWAYS:
	 /* do nothing */
	 return 1;
      case GL_NEVER:
         /* caller should check for zero! */
	 return 0;
      default:
	 _mesa_problem( ctx, "Invalid alpha test in gl_alpha_test" );
         return 0;
   }

#if 0
   /* XXXX This causes conformance failures!!!! */
   while ((span->start <= span->end)  &&
	  (mask[span->start] == 0))
     span->start ++;

   while ((span->end >= span->start)  &&
	  (mask[span->end] == 0))
     span->end --;
#endif
   span->writeAll = GL_FALSE;

   if (span->start >= span->end)
     return 0;
   else
     return 1;
}


/*
 * Apply the alpha test to a span of pixels.
 * In:  rgba - array of pixels
 * In/Out:  mask - current pixel mask.  Pixels which fail the alpha test
 *                 will set the corresponding mask flag to 0.
 * Return:  0 = all pixels in the span failed the alpha test.
 *          1 = one or more pixels passed the alpha test.
 */
GLint
_old_alpha_test( const GLcontext *ctx,
		 GLuint n, CONST GLchan rgba[][4], GLubyte mask[] )
{
   GLuint i;
   const GLchan ref = ctx->Color.AlphaRef;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Color.AlphaFunc) {
      case GL_LESS:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] < ref);
	 }
	 return 1;
      case GL_LEQUAL:
         for (i=0;i<n;i++)
	    mask[i] &= (rgba[i][ACOMP] <= ref);
	 return 1;
      case GL_GEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] >= ref);
	 }
	 return 1;
      case GL_GREATER:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] > ref);
	 }
	 return 1;
      case GL_NOTEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] != ref);
	 }
	 return 1;
      case GL_EQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] == ref);
	 }
	 return 1;
      case GL_ALWAYS:
	 /* do nothing */
	 return 1;
      case GL_NEVER:
         /* caller should check for zero! */
	 return 0;
      default:
	 _mesa_problem( ctx, "Invalid alpha test in gl_alpha_test" );
         return 0;
   }
   /* Never get here */
   /*return 1;*/
}
