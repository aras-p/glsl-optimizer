/* $Id: matrix.c,v 1.42 2002/06/15 02:38:16 brianp Exp $ */

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
 * Matrix operations
 *
 * NOTES:
 * 1. 4x4 transformation matrices are stored in memory in column major order.
 * 2. Points/vertices are to be thought of as column vectors.
 * 3. Transformation of a point p by a matrix M is: p' = M * p
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "buffers.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"

#include "math/m_matrix.h"
#endif


void
_mesa_Frustum( GLdouble left, GLdouble right,
               GLdouble bottom, GLdouble top,
               GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (nearval <= 0.0 ||
       farval <= 0.0 ||
       nearval == farval ||
       left == right ||
       top == bottom)
   {
      _mesa_error( ctx,  GL_INVALID_VALUE, "glFrustum" );
      return;
   }

   _math_matrix_frustum( ctx->CurrentStack->Top,
                         (GLfloat) left, (GLfloat) right, 
			 (GLfloat) bottom, (GLfloat) top, 
			 (GLfloat) nearval, (GLfloat) farval );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


void
_mesa_Ortho( GLdouble left, GLdouble right,
             GLdouble bottom, GLdouble top,
             GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (left == right ||
       bottom == top ||
       nearval == farval)
   {
      _mesa_error( ctx,  GL_INVALID_VALUE, "glOrtho" );
      return;
   }

   _math_matrix_ortho( ctx->CurrentStack->Top,
                       (GLfloat) left, (GLfloat) right, 
		       (GLfloat) bottom, (GLfloat) top, 
		       (GLfloat) nearval, (GLfloat) farval );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


void
_mesa_MatrixMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Transform.MatrixMode == mode && mode != GL_TEXTURE)
      return;
   FLUSH_VERTICES(ctx, _NEW_TRANSFORM);

   switch (mode) {
   case GL_MODELVIEW:
      ctx->CurrentStack = &ctx->ModelviewMatrixStack;
      break;
   case GL_PROJECTION:
      ctx->CurrentStack = &ctx->ProjectionMatrixStack;
      break;
   case GL_TEXTURE:
      ctx->CurrentStack = &ctx->TextureMatrixStack[ctx->Texture.CurrentUnit];
      break;
   case GL_COLOR:
      ctx->CurrentStack = &ctx->ColorMatrixStack;
      break;
   case GL_MATRIX0_NV:
   case GL_MATRIX1_NV:
   case GL_MATRIX2_NV:
   case GL_MATRIX3_NV:
   case GL_MATRIX4_NV:
   case GL_MATRIX5_NV:
   case GL_MATRIX6_NV:
   case GL_MATRIX7_NV:
      if (!ctx->Extensions.NV_vertex_program) {
         _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode" );
         return;
      }
      ctx->CurrentStack = &ctx->ProgramMatrixStack[mode - GL_MATRIX0_NV];
      break;
   default:
      _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode" );
      return;
   }

   ctx->Transform.MatrixMode = mode;
}



void
_mesa_PushMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   struct matrix_stack *stack = ctx->CurrentStack;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPushMatrix %s\n",
                  _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));

   if (stack->Depth + 1 >= stack->MaxDepth) {
      _mesa_error( ctx,  GL_STACK_OVERFLOW, "glPushMatrix" );
      return;
   }
   _math_matrix_copy( &stack->Stack[stack->Depth + 1],
                      &stack->Stack[stack->Depth] );
   stack->Depth++;
   stack->Top = &(stack->Stack[stack->Depth]);
   ctx->NewState |= stack->DirtyFlag;
}



void
_mesa_PopMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   struct matrix_stack *stack = ctx->CurrentStack;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPopMatrix %s\n",
                  _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));

   if (stack->Depth == 0) {
      _mesa_error( ctx,  GL_STACK_UNDERFLOW, "glPopMatrix" );
      return;
   }
   stack->Depth--;
   stack->Top = &(stack->Stack[stack->Depth]);
   ctx->NewState |= stack->DirtyFlag;
}



void
_mesa_LoadIdentity( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_set_identity( ctx->CurrentStack->Top );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


void
_mesa_LoadMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   if (!m) return;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_loadf( ctx->CurrentStack->Top, m );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


void
_mesa_LoadMatrixd( const GLdouble *m )
{
   GLint i;
   GLfloat f[16];
   if (!m) return;
   for (i = 0; i < 16; i++)
      f[i] = (GLfloat) m[i];
   _mesa_LoadMatrixf(f);
}



/*
 * Multiply the active matrix by an arbitary matrix.
 */
void
_mesa_MultMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   if (!m) return;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_mul_floats( ctx->CurrentStack->Top, m );
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/*
 * Multiply the active matrix by an arbitary matrix.
 */
void
_mesa_MultMatrixd( const GLdouble *m )
{
   GLint i;
   GLfloat f[16];
   if (!m) return;
   for (i = 0; i < 16; i++)
      f[i] = (GLfloat) m[i];
   _mesa_MultMatrixf( f );
}




/*
 * Execute a glRotate call
 */
void
_mesa_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   if (angle != 0.0F) {
      _math_matrix_rotate( ctx->CurrentStack->Top, angle, x, y, z);
      ctx->NewState |= ctx->CurrentStack->DirtyFlag;
   }
}

void
_mesa_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Rotatef((GLfloat) angle, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}


/*
 * Execute a glScale call
 */
void
_mesa_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_scale( ctx->CurrentStack->Top, x, y, z);
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


void
_mesa_Scaled( GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Scalef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}


/*
 * Execute a glTranslate call
 */
void
_mesa_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _math_matrix_translate( ctx->CurrentStack->Top, x, y, z);
   ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


void
_mesa_Translated( GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Translatef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}


void
_mesa_LoadTransposeMatrixfARB( const GLfloat *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposef(tm, m);
   _mesa_LoadMatrixf(tm);
}


void
_mesa_LoadTransposeMatrixdARB( const GLdouble *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposefd(tm, m);
   _mesa_LoadMatrixf(tm);
}


void
_mesa_MultTransposeMatrixfARB( const GLfloat *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposef(tm, m);
   _mesa_MultMatrixf(tm);
}


void
_mesa_MultTransposeMatrixdARB( const GLdouble *m )
{
   GLfloat tm[16];
   if (!m) return;
   _math_transposefd(tm, m);
   _mesa_MultMatrixf(tm);
}


/*
 * Called via glViewport or display list execution.
 */
void
_mesa_Viewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_set_viewport(ctx, x, y, width, height);
}


/*
 * Define a new viewport and reallocate auxillary buffers if the size of
 * the window (color buffer) has changed.
 */
void
_mesa_set_viewport( GLcontext *ctx, GLint x, GLint y,
                    GLsizei width, GLsizei height )
{
   const GLfloat n = ctx->Viewport.Near;
   const GLfloat f = ctx->Viewport.Far;

   if (width < 0 || height < 0) {
      _mesa_error( ctx,  GL_INVALID_VALUE, "glViewport" );
      return;
   }

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glViewport %d %d %d %d\n", x, y, width, height);

   /* clamp width, and height to implementation dependent range */
   width  = CLAMP( width,  1, MAX_WIDTH );
   height = CLAMP( height, 1, MAX_HEIGHT );

   /* Save viewport */
   ctx->Viewport.X = x;
   ctx->Viewport.Width = width;
   ctx->Viewport.Y = y;
   ctx->Viewport.Height = height;

   /* compute scale and bias values :: This is really driver-specific
    * and should be maintained elsewhere if at all.
    */
   ctx->Viewport._WindowMap.m[MAT_SX] = (GLfloat) width / 2.0F;
   ctx->Viewport._WindowMap.m[MAT_TX] = ctx->Viewport._WindowMap.m[MAT_SX] + x;
   ctx->Viewport._WindowMap.m[MAT_SY] = (GLfloat) height / 2.0F;
   ctx->Viewport._WindowMap.m[MAT_TY] = ctx->Viewport._WindowMap.m[MAT_SY] + y;
   ctx->Viewport._WindowMap.m[MAT_SZ] = ctx->DepthMaxF * ((f - n) / 2.0F);
   ctx->Viewport._WindowMap.m[MAT_TZ] = ctx->DepthMaxF * ((f - n) / 2.0F + n);
   ctx->Viewport._WindowMap.flags = MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION;
   ctx->Viewport._WindowMap.type = MATRIX_3D_NO_ROT;
   ctx->NewState |= _NEW_VIEWPORT;

   /* Check if window/buffer has been resized and if so, reallocate the
    * ancillary buffers.
    */
   _mesa_ResizeBuffersMESA();

   if (ctx->Driver.Viewport) {
      (*ctx->Driver.Viewport)( ctx, x, y, width, height );
   }
}



void
_mesa_DepthRange( GLclampd nearval, GLclampd farval )
{
   /*
    * nearval - specifies mapping of the near clipping plane to window
    *   coordinates, default is 0
    * farval - specifies mapping of the far clipping plane to window
    *   coordinates, default is 1
    *
    * After clipping and div by w, z coords are in -1.0 to 1.0,
    * corresponding to near and far clipping planes.  glDepthRange
    * specifies a linear mapping of the normalized z coords in
    * this range to window z coords.
    */
   GLfloat n, f;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glDepthRange %f %f\n", nearval, farval);

   n = (GLfloat) CLAMP( nearval, 0.0, 1.0 );
   f = (GLfloat) CLAMP( farval, 0.0, 1.0 );

   ctx->Viewport.Near = n;
   ctx->Viewport.Far = f;
   ctx->Viewport._WindowMap.m[MAT_SZ] = ctx->DepthMaxF * ((f - n) / 2.0F);
   ctx->Viewport._WindowMap.m[MAT_TZ] = ctx->DepthMaxF * ((f - n) / 2.0F + n);
   ctx->NewState |= _NEW_VIEWPORT;

   if (ctx->Driver.DepthRange) {
      (*ctx->Driver.DepthRange)( ctx, nearval, farval );
   }
}
