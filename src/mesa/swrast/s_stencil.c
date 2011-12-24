/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "main/glheader.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/format_pack.h"
#include "main/format_unpack.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_stencil.h"
#include "s_span.h"



/* Stencil Logic:

IF stencil test fails THEN
   Apply fail-op to stencil value
   Don't write the pixel (RGBA,Z)
ELSE
   IF doing depth test && depth test fails THEN
      Apply zfail-op to stencil value
      Write RGBA and Z to appropriate buffers
   ELSE
      Apply zpass-op to stencil value
ENDIF

*/


/**
 * Return the address of a stencil value in a renderbuffer.
 */
static inline GLubyte *
get_stencil_address(struct gl_renderbuffer *rb, GLint x, GLint y)
{
   const GLint bpp = _mesa_get_format_bytes(rb->Format);
   const GLint rowStride = rb->RowStride * bpp;
   assert(rb->Data);
   return (GLubyte *) rb->Data + y * rowStride + x * bpp;
}



/**
 * Apply the given stencil operator to the array of stencil values.
 * Don't touch stencil[i] if mask[i] is zero.
 * Input:  n - size of stencil array
 *         oper - the stencil buffer operator
 *         face - 0 or 1 for front or back face operation
 *         stencil - array of stencil values
 *         mask - array [n] of flag:  1=apply operator, 0=don't apply operator
 * Output:  stencil - modified values
 */
static void
apply_stencil_op( const struct gl_context *ctx, GLenum oper, GLuint face,
                  GLuint n, GLubyte stencil[], const GLubyte mask[] )
{
   const GLubyte ref = ctx->Stencil.Ref[face];
   const GLubyte wrtmask = ctx->Stencil.WriteMask[face];
   const GLubyte invmask = (GLubyte) (~wrtmask);
   const GLubyte stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   GLuint i;

   switch (oper) {
      case GL_KEEP:
         /* do nothing */
         break;
      case GL_ZERO:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  stencil[i] = 0;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  stencil[i] = (GLubyte) (stencil[i] & invmask);
	       }
	    }
	 }
	 break;
      case GL_REPLACE:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  stencil[i] = ref;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLubyte s = stencil[i];
		  stencil[i] = (GLubyte) ((invmask & s ) | (wrtmask & ref));
	       }
	    }
	 }
	 break;
      case GL_INCR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLubyte s = stencil[i];
		  if (s < stencilMax) {
		     stencil[i] = (GLubyte) (s+1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  /* VERIFY logic of adding 1 to a write-masked value */
		  GLubyte s = stencil[i];
		  if (s < stencilMax) {
		     stencil[i] = (GLubyte) ((invmask & s) | (wrtmask & (s+1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_DECR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLubyte s = stencil[i];
		  if (s>0) {
		     stencil[i] = (GLubyte) (s-1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  /* VERIFY logic of subtracting 1 to a write-masked value */
		  GLubyte s = stencil[i];
		  if (s>0) {
		     stencil[i] = (GLubyte) ((invmask & s) | (wrtmask & (s-1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_INCR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  stencil[i]++;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte s = stencil[i];
                  stencil[i] = (GLubyte) ((invmask & s) | (wrtmask & (s+1)));
	       }
	    }
	 }
	 break;
      case GL_DECR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  stencil[i]--;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte s = stencil[i];
                  stencil[i] = (GLubyte) ((invmask & s) | (wrtmask & (s-1)));
	       }
	    }
	 }
	 break;
      case GL_INVERT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLubyte s = stencil[i];
		  stencil[i] = (GLubyte) ~s;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  GLubyte s = stencil[i];
		  stencil[i] = (GLubyte) ((invmask & s) | (wrtmask & ~s));
	       }
	    }
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencil op in apply_stencil_op");
   }
}




/**
 * Apply stencil test to an array of stencil values (before depth buffering).
 * Input:  face - 0 or 1 for front or back-face polygons
 *         n - number of pixels in the array
 *         stencil - array of [n] stencil values
 *         mask - array [n] of flag:  0=skip the pixel, 1=stencil the pixel
 * Output:  mask - pixels which fail the stencil test will have their
 *                 mask flag set to 0.
 *          stencil - updated stencil values (where the test passed)
 * Return:  GL_FALSE = all pixels failed, GL_TRUE = zero or more pixels passed.
 */
static GLboolean
do_stencil_test( struct gl_context *ctx, GLuint face, GLuint n, GLubyte stencil[],
                 GLubyte mask[] )
{
   GLubyte fail[MAX_WIDTH];
   GLboolean allfail = GL_FALSE;
   GLuint i;
   const GLuint valueMask = ctx->Stencil.ValueMask[face];
   const GLubyte r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
   GLubyte s;

   ASSERT(n <= MAX_WIDTH);

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */
   switch (ctx->Stencil.Function[face]) {
      case GL_NEVER:
         /* never pass; always fail */
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       mask[i] = 0;
	       fail[i] = 1;
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 allfail = GL_TRUE;
	 break;
      case GL_LESS:
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLubyte) (stencil[i] & valueMask);
	       if (r < s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_LEQUAL:
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLubyte) (stencil[i] & valueMask);
	       if (r <= s) {
		  /* pass */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GREATER:
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLubyte) (stencil[i] & valueMask);
	       if (r > s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GEQUAL:
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLubyte) (stencil[i] & valueMask);
	       if (r >= s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_EQUAL:
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLubyte) (stencil[i] & valueMask);
	       if (r == s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
	       s = (GLubyte) (stencil[i] & valueMask);
	       if (r != s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_ALWAYS:
	 /* always pass */
	 for (i=0;i<n;i++) {
	    fail[i] = 0;
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencil func in gl_stencil_span");
         return 0;
   }

   if (ctx->Stencil.FailFunc[face] != GL_KEEP) {
      apply_stencil_op( ctx, ctx->Stencil.FailFunc[face], face, n, stencil, fail );
   }

   return !allfail;
}


/**
 * Compute the zpass/zfail masks by comparing the pre- and post-depth test
 * masks.
 */
static inline void
compute_pass_fail_masks(GLuint n, const GLubyte origMask[],
                        const GLubyte newMask[],
                        GLubyte passMask[], GLubyte failMask[])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      ASSERT(newMask[i] == 0 || newMask[i] == 1);
      passMask[i] = origMask[i] & newMask[i];
      failMask[i] = origMask[i] & (newMask[i] ^ 1);
   }
}


/**
 * Apply stencil and depth testing to the span of pixels.
 * Both software and hardware stencil buffers are acceptable.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in span
 *         z - array [n] of z values
 *         mask - array [n] of flags  (1=test this pixel, 0=skip the pixel)
 * Output:  mask - array [n] of flags (1=stencil and depth test passed)
 * Return: GL_FALSE - all fragments failed the testing
 *         GL_TRUE - one or more fragments passed the testing
 *
 */
static GLboolean
stencil_and_ztest_span(struct gl_context *ctx, SWspan *span, GLuint face)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLubyte stencilRow[MAX_WIDTH];
   GLubyte *stencil;
   const GLuint n = span->end;
   const GLint x = span->x;
   const GLint y = span->y;
   GLubyte *mask = span->array->mask;

   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(ctx->Stencil.Enabled);
   ASSERT(n <= MAX_WIDTH);
#ifdef DEBUG
   if (ctx->Depth.Test) {
      ASSERT(span->arrayMask & SPAN_Z);
   }
#endif

   stencil = (GLubyte *) rb->GetPointer(ctx, rb, x, y);
   if (!stencil) {
      rb->GetRow(ctx, rb, n, x, y, stencilRow);
      stencil = stencilRow;
   }

   /*
    * Apply the stencil test to the fragments.
    * failMask[i] is 1 if the stencil test failed.
    */
   if (do_stencil_test( ctx, face, n, stencil, mask ) == GL_FALSE) {
      /* all fragments failed the stencil test, we're done. */
      span->writeAll = GL_FALSE;
      if (!rb->GetPointer(ctx, rb, 0, 0)) {
         /* put updated stencil values into buffer */
         rb->PutRow(ctx, rb, n, x, y, stencil, NULL);
      }
      return GL_FALSE;
   }

   /*
    * Some fragments passed the stencil test, apply depth test to them
    * and apply Zpass and Zfail stencil ops.
    */
   if (ctx->Depth.Test == GL_FALSE ||
       ctx->DrawBuffer->_DepthBuffer == NULL) {
      /*
       * No depth buffer, just apply zpass stencil function to active pixels.
       */
      apply_stencil_op( ctx, ctx->Stencil.ZPassFunc[face], face, n, stencil, mask );
   }
   else {
      /*
       * Perform depth buffering, then apply zpass or zfail stencil function.
       */
      GLubyte passMask[MAX_WIDTH], failMask[MAX_WIDTH], origMask[MAX_WIDTH];

      /* save the current mask bits */
      memcpy(origMask, mask, n * sizeof(GLubyte));

      /* apply the depth test */
      _swrast_depth_test_span(ctx, span);

      compute_pass_fail_masks(n, origMask, mask, passMask, failMask);

      /* apply the pass and fail operations */
      if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
         apply_stencil_op( ctx, ctx->Stencil.ZFailFunc[face], face,
                           n, stencil, failMask );
      }
      if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
         apply_stencil_op( ctx, ctx->Stencil.ZPassFunc[face], face,
                           n, stencil, passMask );
      }
   }

   /*
    * Write updated stencil values back into hardware stencil buffer.
    */
   if (!rb->GetPointer(ctx, rb, 0, 0)) {
      rb->PutRow(ctx, rb, n, x, y, stencil, NULL);
   }
   
   span->writeAll = GL_FALSE;
   
   return GL_TRUE;  /* one or more fragments passed both tests */
}



/*
 * Return the address of a stencil buffer value given the window coords:
 */
#define STENCIL_ADDRESS(X, Y)  (stencilStart + (Y) * stride + (X))



/**
 * Apply the given stencil operator for each pixel in the array whose
 * mask flag is set.
 * \note  This is for software stencil buffers only.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels
 *         operator - the stencil buffer operator
 *         mask - array [n] of flag:  1=apply operator, 0=don't apply operator
 */
static void
apply_stencil_op_to_pixels( struct gl_context *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            GLenum oper, GLuint face, const GLubyte mask[] )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   const GLubyte stencilMax = (1 << fb->Visual.stencilBits) - 1;
   const GLubyte ref = ctx->Stencil.Ref[face];
   const GLubyte wrtmask = ctx->Stencil.WriteMask[face];
   const GLubyte invmask = (GLubyte) (~wrtmask);
   GLuint i;
   GLubyte *stencilStart = (GLubyte *) rb->Data;
   const GLuint stride = rb->Width;

   ASSERT(rb->GetPointer(ctx, rb, 0, 0));

   switch (oper) {
      case GL_KEEP:
         /* do nothing */
         break;
      case GL_ZERO:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = 0;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  *sptr = (GLubyte) (invmask & *sptr);
	       }
	    }
	 }
	 break;
      case GL_REPLACE:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = ref;
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  *sptr = (GLubyte) ((invmask & *sptr ) | (wrtmask & ref));
	       }
	    }
	 }
	 break;
      case GL_INCR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr < stencilMax) {
		     *sptr = (GLubyte) (*sptr + 1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr < stencilMax) {
		     *sptr = (GLubyte) ((invmask & *sptr) | (wrtmask & (*sptr+1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_DECR:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr>0) {
		     *sptr = (GLubyte) (*sptr - 1);
		  }
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
		  if (*sptr>0) {
		     *sptr = (GLubyte) ((invmask & *sptr) | (wrtmask & (*sptr-1)));
		  }
	       }
	    }
	 }
	 break;
      case GL_INCR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLubyte) (*sptr + 1);
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLubyte) ((invmask & *sptr) | (wrtmask & (*sptr+1)));
	       }
	    }
	 }
	 break;
      case GL_DECR_WRAP_EXT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLubyte) (*sptr - 1);
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLubyte) ((invmask & *sptr) | (wrtmask & (*sptr-1)));
	       }
	    }
	 }
	 break;
      case GL_INVERT:
	 if (invmask==0) {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLubyte) (~*sptr);
	       }
	    }
	 }
	 else {
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLubyte *sptr = STENCIL_ADDRESS( x[i], y[i] );
                  *sptr = (GLubyte) ((invmask & *sptr) | (wrtmask & ~*sptr));
	       }
	    }
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencilop in apply_stencil_op_to_pixels");
   }
}



/**
 * Apply stencil test to an array of pixels before depth buffering.
 *
 * \note Used for software stencil buffer only.
 * Input:  n - number of pixels in the span
 *         x, y - array of [n] pixels to stencil
 *         mask - array [n] of flag:  0=skip the pixel, 1=stencil the pixel
 * Output:  mask - pixels which fail the stencil test will have their
 *                 mask flag set to 0.
 * \return  GL_FALSE = all pixels failed, GL_TRUE = zero or more pixels passed.
 */
static GLboolean
stencil_test_pixels( struct gl_context *ctx, GLuint face, GLuint n,
                     const GLint x[], const GLint y[], GLubyte mask[] )
{
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLubyte fail[MAX_WIDTH];
   GLubyte r, s;
   GLuint i;
   GLboolean allfail = GL_FALSE;
   const GLuint valueMask = ctx->Stencil.ValueMask[face];
   const GLubyte *stencilStart = (GLubyte *) rb->Data;
   const GLuint stride = rb->Width;

   ASSERT(rb->GetPointer(ctx, rb, 0, 0));

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */

   switch (ctx->Stencil.Function[face]) {
      case GL_NEVER:
         /* always fail */
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       mask[i] = 0;
	       fail[i] = 1;
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 allfail = GL_TRUE;
	 break;
      case GL_LESS:
	 r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLubyte *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLubyte) (*sptr & valueMask);
	       if (r < s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_LEQUAL:
	 r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLubyte *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLubyte) (*sptr & valueMask);
	       if (r <= s) {
		  /* pass */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GREATER:
	 r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLubyte *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLubyte) (*sptr & valueMask);
	       if (r > s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_GEQUAL:
	 r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLubyte *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLubyte) (*sptr & valueMask);
	       if (r >= s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_EQUAL:
	 r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLubyte *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLubyte) (*sptr & valueMask);
	       if (r == s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 r = (GLubyte) (ctx->Stencil.Ref[face] & valueMask);
	 for (i=0;i<n;i++) {
	    if (mask[i]) {
               const GLubyte *sptr = STENCIL_ADDRESS(x[i],y[i]);
	       s = (GLubyte) (*sptr & valueMask);
	       if (r != s) {
		  /* passed */
		  fail[i] = 0;
	       }
	       else {
		  fail[i] = 1;
		  mask[i] = 0;
	       }
	    }
	    else {
	       fail[i] = 0;
	    }
	 }
	 break;
      case GL_ALWAYS:
	 /* always pass */
	 for (i=0;i<n;i++) {
	    fail[i] = 0;
	 }
	 break;
      default:
         _mesa_problem(ctx, "Bad stencil func in gl_stencil_pixels");
         return 0;
   }

   if (ctx->Stencil.FailFunc[face] != GL_KEEP) {
      apply_stencil_op_to_pixels( ctx, n, x, y, ctx->Stencil.FailFunc[face],
                                  face, fail );
   }

   return !allfail;
}



static void
get_s8_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
              GLuint count, const GLint x[], const GLint y[],
              GLubyte stencil[])
{
   const GLint w = rb->Width, h = rb->Height;
   const GLubyte *map = (const GLubyte *) rb->Data;
   GLuint i;

   if (rb->Format == MESA_FORMAT_S8) {
      const GLuint rowStride = rb->RowStride;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            stencil[i] = *(map + y[i] * rowStride + x[i]);
         }
      }
   }
   else {
      const GLuint bpp = _mesa_get_format_bytes(rb->Format);
      const GLuint rowStride = rb->RowStride * bpp;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            const GLubyte *src = map + y[i] * rowStride + x[i] * bpp;
            _mesa_unpack_ubyte_stencil_row(rb->Format, 1, src, &stencil[i]);
         }
      }
   }
}


/**
 * Apply stencil and depth testing to an array of pixels.
 * This is used both for software and hardware stencil buffers.
 *
 * The comments in this function are a bit sparse but the code is
 * almost identical to stencil_and_ztest_span(), which is well
 * commented.
 *
 * Input:  n - number of pixels in the array
 *         x, y - array of [n] pixel positions
 *         z - array [n] of z values
 *         mask - array [n] of flags  (1=test this pixel, 0=skip the pixel)
 * Output: mask - array [n] of flags (1=stencil and depth test passed)
 * Return: GL_FALSE - all fragments failed the testing
 *         GL_TRUE - one or more fragments passed the testing
 */
static GLboolean
stencil_and_ztest_pixels( struct gl_context *ctx, SWspan *span, GLuint face )
{
   GLubyte passMask[MAX_WIDTH], failMask[MAX_WIDTH], origMask[MAX_WIDTH];
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   const GLuint n = span->end;
   const GLint *x = span->array->x;
   const GLint *y = span->array->y;
   GLubyte *mask = span->array->mask;

   ASSERT(span->arrayMask & SPAN_XY);
   ASSERT(ctx->Stencil.Enabled);
   ASSERT(n <= MAX_WIDTH);

   if (!rb->GetPointer(ctx, rb, 0, 0)) {
      /* No direct access */
      GLubyte stencil[MAX_WIDTH];

      ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
      get_s8_values(ctx, rb, n, x, y, stencil);

      memcpy(origMask, mask, n * sizeof(GLubyte));          

      (void) do_stencil_test(ctx, face, n, stencil, mask);

      if (ctx->Depth.Test == GL_FALSE) {
         apply_stencil_op(ctx, ctx->Stencil.ZPassFunc[face], face,
                          n, stencil, mask);
      }
      else {
         GLubyte tmpMask[MAX_WIDTH]; 
         memcpy(tmpMask, mask, n * sizeof(GLubyte));

         _swrast_depth_test_span(ctx, span);

         compute_pass_fail_masks(n, tmpMask, mask, passMask, failMask);

         if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
            apply_stencil_op(ctx, ctx->Stencil.ZFailFunc[face], face,
                             n, stencil, failMask);
         }
         if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
            apply_stencil_op(ctx, ctx->Stencil.ZPassFunc[face], face,
                             n, stencil, passMask);
         }
      }

      /* Write updated stencil values into hardware stencil buffer */
      rb->PutValues(ctx, rb, n, x, y, stencil, origMask);

      return GL_TRUE;
   }
   else {
      /* Direct access to stencil buffer */

      if (stencil_test_pixels(ctx, face, n, x, y, mask) == GL_FALSE) {
         /* all fragments failed the stencil test, we're done. */
         return GL_FALSE;
      }

      if (ctx->Depth.Test==GL_FALSE) {
         apply_stencil_op_to_pixels(ctx, n, x, y,
                                    ctx->Stencil.ZPassFunc[face], face, mask);
      }
      else {
         memcpy(origMask, mask, n * sizeof(GLubyte));

         _swrast_depth_test_span(ctx, span);

         compute_pass_fail_masks(n, origMask, mask, passMask, failMask);

         if (ctx->Stencil.ZFailFunc[face] != GL_KEEP) {
            apply_stencil_op_to_pixels(ctx, n, x, y,
                                       ctx->Stencil.ZFailFunc[face],
                                       face, failMask);
         }
         if (ctx->Stencil.ZPassFunc[face] != GL_KEEP) {
            apply_stencil_op_to_pixels(ctx, n, x, y,
                                       ctx->Stencil.ZPassFunc[face],
                                       face, passMask);
         }
      }

      return GL_TRUE;  /* one or more fragments passed both tests */
   }
}


/**
 * /return GL_TRUE = one or more fragments passed,
 * GL_FALSE = all fragments failed.
 */
GLboolean
_swrast_stencil_and_ztest_span(struct gl_context *ctx, SWspan *span)
{
   const GLuint face = (span->facing == 0) ? 0 : ctx->Stencil._BackFace;

   if (span->arrayMask & SPAN_XY)
      return stencil_and_ztest_pixels(ctx, span, face);
   else
      return stencil_and_ztest_span(ctx, span, face);
}


#if 0
GLuint
clip_span(GLuint bufferWidth, GLuint bufferHeight,
          GLint x, GLint y, GLuint *count)
{
   GLuint n = *count;
   GLuint skipPixels = 0;

   if (y < 0 || y >= bufferHeight || x + n <= 0 || x >= bufferWidth) {
      /* totally out of bounds */
      n = 0;
   }
   else {
      /* left clip */
      if (x < 0) {
         skipPixels = -x;
         x = 0;
         n -= skipPixels;
      }
      /* right clip */
      if (x + n > bufferWidth) {
         GLint dx = x + n - bufferWidth;
         n -= dx;
      }
   }

   *count = n;

   return skipPixels;
}
#endif


/**
 * Return a span of stencil values from the stencil buffer.
 * Used for glRead/CopyPixels
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  stencil - the array of stencil values
 */
void
_swrast_read_stencil_span(struct gl_context *ctx, struct gl_renderbuffer *rb,
                          GLint n, GLint x, GLint y, GLubyte stencil[])
{
   GLubyte *src;
   const GLuint bpp = _mesa_get_format_bytes(rb->Format);
   const GLuint rowStride = rb->RowStride * bpp;

   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }

   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - rb->Width;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   src = get_stencil_address(rb, x, y);
   _mesa_unpack_ubyte_stencil_row(rb->Format, n, src, stencil);
}



/**
 * Write a span of stencil values to the stencil buffer.  This function
 * applies the stencil write mask when needed.
 * Used for glDraw/CopyPixels
 * Input:  n - how many pixels
 *         x, y - location of first pixel
 *         stencil - the array of stencil values
 */
void
_swrast_write_stencil_span(struct gl_context *ctx, GLint n, GLint x, GLint y,
                           const GLubyte stencil[] )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   const GLuint stencilMax = (1 << fb->Visual.stencilBits) - 1;
   const GLuint stencilMask = ctx->Stencil.WriteMask[0];
   GLubyte *stencilBuf;

   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }
   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - rb->Width;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   stencilBuf = get_stencil_address(rb, x, y);

   if ((stencilMask & stencilMax) != stencilMax) {
      /* need to apply writemask */
      GLubyte destVals[MAX_WIDTH], newVals[MAX_WIDTH];
      GLint i;

      _mesa_unpack_ubyte_stencil_row(rb->Format, n, stencilBuf, destVals);
      for (i = 0; i < n; i++) {
         newVals[i]
            = (stencil[i] & stencilMask) | (destVals[i] & ~stencilMask);
      }
      _mesa_pack_ubyte_stencil_row(rb->Format, n, newVals, stencilBuf);
   }
   else {
      _mesa_pack_ubyte_stencil_row(rb->Format, n, stencil, stencilBuf);
   }
}



/**
 * Clear the stencil buffer.  If the buffer is a combined
 * depth+stencil buffer, only the stencil bits will be touched.
 */
void
_swrast_clear_stencil_buffer(struct gl_context *ctx)
{
   struct gl_renderbuffer *rb =
      ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   const GLubyte stencilBits = ctx->DrawBuffer->Visual.stencilBits;
   const GLuint writeMask = ctx->Stencil.WriteMask[0];
   const GLuint stencilMax = (1 << stencilBits) - 1;
   GLint x, y, width, height;
   GLubyte *map;
   GLint rowStride, i, j;
   GLbitfield mapMode;

   if (!rb || writeMask == 0)
      return;

   /* compute region to clear */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   mapMode = GL_MAP_WRITE_BIT;
   if ((writeMask & stencilMax) != stencilMax) {
      /* need to mask stencil values */
      mapMode |= GL_MAP_READ_BIT;
   }
   else if (_mesa_get_format_bits(rb->Format, GL_DEPTH_BITS) > 0) {
      /* combined depth+stencil, need to mask Z values */
      mapMode |= GL_MAP_READ_BIT;
   }

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               mapMode, &map, &rowStride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClear(stencil)");
      return;
   }

   switch (rb->Format) {
   case MESA_FORMAT_S8:
      {
         GLubyte clear = ctx->Stencil.Clear & writeMask & 0xff;
         GLubyte mask = (~writeMask) & 0xff;
         if (mask != 0) {
            /* masked clear */
            for (i = 0; i < height; i++) {
               GLubyte *row = map;
               for (j = 0; j < width; j++) {
                  row[j] = (row[j] & mask) | clear;
               }
               map += rowStride;
            }
         }
         else if (rowStride == width) {
            /* clear whole buffer */
            memset(map, clear, width * height);
         }
         else {
            /* clear scissored */
            for (i = 0; i < height; i++) {
               memset(map, clear, width);
               map += rowStride;
            }
         }
      }
      break;
   case MESA_FORMAT_S8_Z24:
      {
         GLuint clear = (ctx->Stencil.Clear & writeMask & 0xff) << 24;
         GLuint mask = (((~writeMask) & 0xff) << 24) | 0xffffff;
         for (i = 0; i < height; i++) {
            GLuint *row = (GLuint *) map;
            for (j = 0; j < width; j++) {
               row[j] = (row[j] & mask) | clear;
            }
            map += rowStride;
         }
      }
      break;
   case MESA_FORMAT_Z24_S8:
      {
         GLuint clear = ctx->Stencil.Clear & writeMask & 0xff;
         GLuint mask = 0xffffff00 | ((~writeMask) & 0xff);
         for (i = 0; i < height; i++) {
            GLuint *row = (GLuint *) map;
            for (j = 0; j < width; j++) {
               row[j] = (row[j] & mask) | clear;
            }
            map += rowStride;
         }
      }
      break;
   default:
      _mesa_problem(ctx, "Unexpected stencil buffer format %s"
                    " in _swrast_clear_stencil_buffer()",
                    _mesa_get_format_name(rb->Format));
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}
