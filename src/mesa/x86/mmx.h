
/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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





#ifndef ASM_MMX_H
#define ASM_MMX_H


extern void
gl_mmx_blend_transparency( GLcontext *ctx, GLuint n, const GLubyte mask[],
                           GLubyte rgba[][4], const GLubyte dest[][4] );


void gl_mmx_set_blend_function( GLcontext *ctx )
{
   const GLenum eq = ctx->Color.BlendEquation;
   const GLenum srcRGB = ctx->Color.BlendSrcRGB;
   const GLenum dstRGB = ctx->Color.BlendDstRGB;
   const GLenum srcA = ctx->Color.BlendSrcA;
   const GLenum dstA = ctx->Color.BlendDstA;


   if (srcRGB != srcA || dstRGB != dstA) {
      ctx->Color.BlendFunc = blend_general;
   }
   else if (eq==GL_FUNC_ADD_EXT && srcRGB==GL_SRC_ALPHA
       && dstRGB==GL_ONE_MINUS_SRC_ALPHA) {
      ctx->Color.BlendFunc = gl_mmx_blend_transparency;
   }
   else if (eq==GL_FUNC_ADD_EXT && srcRGB==GL_ONE && dstRGB==GL_ONE) {
      ctx->Color.BlendFunc = blend_add;
   }
   else if (((eq==GL_FUNC_ADD_EXT || eq==GL_FUNC_REVERSE_SUBTRACT_EXT)
             && (srcRGB==GL_ZERO && dstRGB==GL_SRC_COLOR))
            ||
            ((eq==GL_FUNC_ADD_EXT || eq==GL_FUNC_SUBTRACT_EXT)
             && (srcRGB==GL_DST_COLOR && dstRGB==GL_ZERO))) {
      ctx->Color.BlendFunc = blend_modulate;
   }
   else if (eq==GL_MIN_EXT) {
      ctx->Color.BlendFunc = blend_min;
   }
   else if (eq==GL_MAX_EXT) {
      ctx->Color.BlendFunc = blend_max;
   }
   else {
      ctx->Color.BlendFunc = blend_general;
   }
}


#endif
