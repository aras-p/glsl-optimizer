/* $Id: matrix.c,v 1.32 2001/02/13 23:51:34 brianp Exp $ */

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


/**********************************************************************/
/*                        API functions                               */
/**********************************************************************/


#define GET_ACTIVE_MATRIX(ctx, mat, flags, where)		\
do {									\
   if (MESA_VERBOSE&VERBOSE_API) fprintf(stderr, "%s\n", where);	\
   switch (ctx->Transform.MatrixMode) {					\
      case GL_MODELVIEW:						\
	 mat = &ctx->ModelView;						\
	 flags |= _NEW_MODELVIEW;					\
	 break;								\
      case GL_PROJECTION:						\
	 mat = &ctx->ProjectionMatrix;					\
	 flags |= _NEW_PROJECTION;					\
	 break;								\
      case GL_TEXTURE:							\
	 mat = &ctx->TextureMatrix[ctx->Texture.CurrentTransformUnit];	\
	 flags |= _NEW_TEXTURE_MATRIX;					\
	 break;								\
      case GL_COLOR:							\
	 mat = &ctx->ColorMatrix;					\
	 flags |= _NEW_COLOR_MATRIX;					\
	 break;								\
      default:								\
         gl_problem(ctx, where);					\
   }									\
} while (0)


void
_mesa_Frustum( GLdouble left, GLdouble right,
               GLdouble bottom, GLdouble top,
               GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   GET_ACTIVE_MATRIX( ctx,  mat, ctx->NewState, "glFrustrum" );

   if (nearval <= 0.0 ||
       farval <= 0.0 ||
       nearval == farval ||
       left == right ||
       top == bottom)
   {
      gl_error( ctx,  GL_INVALID_VALUE, "glFrustum" );
      return;
   }

   _math_matrix_frustum( mat, left, right, bottom, top, nearval, farval );
}


void
_mesa_Ortho( GLdouble left, GLdouble right,
             GLdouble bottom, GLdouble top,
             GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   GET_ACTIVE_MATRIX( ctx,  mat, ctx->NewState, "glOrtho" );

   if (left == right ||
       bottom == top ||
       nearval == farval)
   {
      gl_error( ctx,  GL_INVALID_VALUE, "glOrtho" );
      return;
   }

   _math_matrix_ortho( mat, left, right, bottom, top, nearval, farval );
}


void
_mesa_MatrixMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (mode) {
      case GL_MODELVIEW:
      case GL_PROJECTION:
      case GL_TEXTURE:
      case GL_COLOR:
	 if (ctx->Transform.MatrixMode == mode)
	    return;
         ctx->Transform.MatrixMode = mode;
	 FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         break;
      default:
         gl_error( ctx,  GL_INVALID_ENUM, "glMatrixMode" );
   }
}



void
_mesa_PushMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      fprintf(stderr, "glPushMatrix %s\n",
	      gl_lookup_enum_by_nr(ctx->Transform.MatrixMode));

   switch (ctx->Transform.MatrixMode) {
      case GL_MODELVIEW:
         if (ctx->ModelViewStackDepth >= MAX_MODELVIEW_STACK_DEPTH - 1) {
            gl_error( ctx,  GL_STACK_OVERFLOW, "glPushMatrix");
            return;
         }
         _math_matrix_copy( &ctx->ModelViewStack[ctx->ModelViewStackDepth++],
                      &ctx->ModelView );
         break;
      case GL_PROJECTION:
         if (ctx->ProjectionStackDepth >= MAX_PROJECTION_STACK_DEPTH - 1) {
            gl_error( ctx,  GL_STACK_OVERFLOW, "glPushMatrix");
            return;
         }
         _math_matrix_copy( &ctx->ProjectionStack[ctx->ProjectionStackDepth++],
                      &ctx->ProjectionMatrix );
         break;
      case GL_TEXTURE:
         {
            GLuint t = ctx->Texture.CurrentTransformUnit;
            if (ctx->TextureStackDepth[t] >= MAX_TEXTURE_STACK_DEPTH - 1) {
               gl_error( ctx,  GL_STACK_OVERFLOW, "glPushMatrix");
               return;
            }
	    _math_matrix_copy( &ctx->TextureStack[t][ctx->TextureStackDepth[t]++],
                         &ctx->TextureMatrix[t] );
         }
         break;
      case GL_COLOR:
         if (ctx->ColorStackDepth >= MAX_COLOR_STACK_DEPTH - 1) {
            gl_error( ctx,  GL_STACK_OVERFLOW, "glPushMatrix");
            return;
         }
         _math_matrix_copy( &ctx->ColorStack[ctx->ColorStackDepth++],
                      &ctx->ColorMatrix );
         break;
      default:
         gl_problem(ctx, "Bad matrix mode in gl_PushMatrix");
   }
}



void
_mesa_PopMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      fprintf(stderr, "glPopMatrix %s\n",
	      gl_lookup_enum_by_nr(ctx->Transform.MatrixMode));

   switch (ctx->Transform.MatrixMode) {
      case GL_MODELVIEW:
         if (ctx->ModelViewStackDepth==0) {
            gl_error( ctx,  GL_STACK_UNDERFLOW, "glPopMatrix");
            return;
         }
         _math_matrix_copy( &ctx->ModelView,
			    &ctx->ModelViewStack[--ctx->ModelViewStackDepth] );
	 ctx->NewState |= _NEW_MODELVIEW;
         break;
      case GL_PROJECTION:
         if (ctx->ProjectionStackDepth==0) {
            gl_error( ctx,  GL_STACK_UNDERFLOW, "glPopMatrix");
            return;
         }

         _math_matrix_copy( &ctx->ProjectionMatrix,
			    &ctx->ProjectionStack[--ctx->ProjectionStackDepth] );
	 ctx->NewState |= _NEW_PROJECTION;
         break;
      case GL_TEXTURE:
         {
            GLuint t = ctx->Texture.CurrentTransformUnit;
            if (ctx->TextureStackDepth[t]==0) {
               gl_error( ctx,  GL_STACK_UNDERFLOW, "glPopMatrix");
               return;
            }
	    _math_matrix_copy(&ctx->TextureMatrix[t],
			      &ctx->TextureStack[t][--ctx->TextureStackDepth[t]]);
	    ctx->NewState |= _NEW_TEXTURE_MATRIX;
         }
         break;
      case GL_COLOR:
         if (ctx->ColorStackDepth==0) {
            gl_error( ctx,  GL_STACK_UNDERFLOW, "glPopMatrix");
            return;
         }
         _math_matrix_copy(&ctx->ColorMatrix,
			   &ctx->ColorStack[--ctx->ColorStackDepth]);
	 ctx->NewState |= _NEW_COLOR_MATRIX;
         break;
      default:
         gl_problem(ctx, "Bad matrix mode in gl_PopMatrix");
   }
}



void
_mesa_LoadIdentity( void )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   GET_ACTIVE_MATRIX(ctx, mat, ctx->NewState, "glLoadIdentity");
   _math_matrix_set_identity( mat );
}


void
_mesa_LoadMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   GET_ACTIVE_MATRIX(ctx, mat, ctx->NewState, "glLoadMatrix");
   _math_matrix_loadf( mat, m );
}


void
_mesa_LoadMatrixd( const GLdouble *m )
{
   GLint i;
   GLfloat f[16];
   for (i = 0; i < 16; i++)
      f[i] = m[i];
   _mesa_LoadMatrixf(f);
}



/*
 * Multiply the active matrix by an arbitary matrix.
 */
void
_mesa_MultMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   GET_ACTIVE_MATRIX( ctx,  mat, ctx->NewState, "glMultMatrix" );
   _math_matrix_mul_floats( mat, m );
}


/*
 * Multiply the active matrix by an arbitary matrix.
 */
void
_mesa_MultMatrixd( const GLdouble *m )
{
   GLint i;
   GLfloat f[16];
   for (i = 0; i < 16; i++)
      f[i] = m[i];
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
      GLmatrix *mat = 0;
      GET_ACTIVE_MATRIX( ctx,  mat, ctx->NewState, "glRotate" );
      _math_matrix_rotate( mat, angle, x, y, z );
   }
}

void
_mesa_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Rotatef(angle, x, y, z);
}


/*
 * Execute a glScale call
 */
void
_mesa_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   GET_ACTIVE_MATRIX(ctx, mat, ctx->NewState, "glScale");
   _math_matrix_scale( mat, x, y, z );
}


void
_mesa_Scaled( GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Scalef(x, y, z);
}


/*
 * Execute a glTranslate call
 */
void
_mesa_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   GLmatrix *mat = 0;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   GET_ACTIVE_MATRIX(ctx, mat, ctx->NewState, "glTranslate");
   _math_matrix_translate( mat, x, y, z );
}


void
_mesa_Translated( GLdouble x, GLdouble y, GLdouble z )
{
   _mesa_Translatef(x, y, z);
}


void
_mesa_LoadTransposeMatrixfARB( const GLfloat *m )
{
   GLfloat tm[16];
   _math_transposef(tm, m);
   _mesa_LoadMatrixf(tm);
}


void
_mesa_LoadTransposeMatrixdARB( const GLdouble *m )
{
   GLfloat tm[16];
   _math_transposefd(tm, m);
   _mesa_LoadMatrixf(tm);
}


void
_mesa_MultTransposeMatrixfARB( const GLfloat *m )
{
   GLfloat tm[16];
   _math_transposef(tm, m);
   _mesa_MultMatrixf(tm);
}


void
_mesa_MultTransposeMatrixdARB( const GLdouble *m )
{
   GLfloat tm[16];
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
   gl_Viewport(ctx, x, y, width, height);
}



/*
 * Define a new viewport and reallocate auxillary buffers if the size of
 * the window (color buffer) has changed.
 *
 * XXX This is directly called by device drivers, BUT this function
 * may be renamed _mesa_Viewport (without ctx arg) in the future so
 * use of _mesa_Viewport is encouraged.
 */
void
gl_Viewport( GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height )
{
   if (width<0 || height<0) {
      gl_error( ctx,  GL_INVALID_VALUE, "glViewport" );
      return;
   }

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glViewport %d %d %d %d\n", x, y, width, height);

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
   ctx->Viewport._WindowMap.m[MAT_SZ] = 0.5 * ctx->DepthMaxF;
   ctx->Viewport._WindowMap.m[MAT_TZ] = 0.5 * ctx->DepthMaxF;
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
      fprintf(stderr, "glDepthRange %f %f\n", nearval, farval);

   n = (GLfloat) CLAMP( nearval, 0.0, 1.0 );
   f = (GLfloat) CLAMP( farval, 0.0, 1.0 );

   ctx->Viewport.Near = n;
   ctx->Viewport.Far = f;
   ctx->Viewport._WindowMap.m[MAT_SZ] = ctx->DepthMaxF * ((f - n) / 2.0);
   ctx->Viewport._WindowMap.m[MAT_TZ] = ctx->DepthMaxF * ((f - n) / 2.0 + n);
   ctx->NewState |= _NEW_VIEWPORT;

   if (ctx->Driver.DepthRange) {
      (*ctx->Driver.DepthRange)( ctx, nearval, farval );
   }
}
