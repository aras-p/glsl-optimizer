/* $Id: rastpos.c,v 1.6 2000/03/03 17:47:39 brianp Exp $ */

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
#include "clip.h"
#include "context.h"
#include "feedback.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "rastpos.h"
#include "shade.h"
#include "state.h"
#include "types.h"
#include "xform.h"
#endif


/*
 * Caller:  context->API.RasterPos4f
 */
static void raster_pos4f( GLcontext *ctx,
                          GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GLfloat v[4], eye[4], clip[4], ndc[3], d;

   /* KW: Added this test, which is in the spec.  We can't do this
    *     outside begin/end any more because the ctx->Current values
    *     aren't uptodate during that period. 
    */
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx, "glRasterPos" );

   if (ctx->NewState)
      gl_update_state( ctx );

   ASSIGN_4V( v, x, y, z, w );
   TRANSFORM_POINT( eye, ctx->ModelView.m, v );

   /* raster color */
   if (ctx->Light.Enabled) 
   {
      /*GLfloat *vert;*/
      GLfloat *norm, eyenorm[3];
      GLfloat *objnorm = ctx->Current.Normal;

	  /* Not needed???
      vert = (ctx->NeedEyeCoords ? eye : v);
	  */

      if (ctx->NeedEyeNormals) {
	 GLfloat *inv = ctx->ModelView.inv;
	 TRANSFORM_NORMAL( eyenorm, objnorm, inv );
	 norm = eyenorm;
      } else {
	 norm = objnorm;
      }

      gl_shade_rastpos( ctx, v, norm, 
			ctx->Current.RasterColor,
			&ctx->Current.RasterIndex );

   }
   else {
      /* use current color or index */
      if (ctx->Visual->RGBAflag) {
	 UBYTE_RGBA_TO_FLOAT_RGBA(ctx->Current.RasterColor, 
				  ctx->Current.ByteColor);
      }
      else {
	 ctx->Current.RasterIndex = ctx->Current.Index;
      }
   }

   /* compute raster distance */
   ctx->Current.RasterDistance = (GLfloat)
                      GL_SQRT( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );

   /* apply projection matrix:  clip = Proj * eye */
   TRANSFORM_POINT( clip, ctx->ProjectionMatrix.m, eye );

   /* clip to view volume */
   if (gl_viewclip_point( clip )==0) {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* clip to user clipping planes */
   if ( ctx->Transform.AnyClip &&
	gl_userclip_point(ctx, clip) == 0) 
   {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* ndc = clip / W */
   ASSERT( clip[3]!=0.0 );
   d = 1.0F / clip[3];
   ndc[0] = clip[0] * d;
   ndc[1] = clip[1] * d;
   ndc[2] = clip[2] * d;

   ctx->Current.RasterPos[0] = (ndc[0] * ctx->Viewport.WindowMap.m[MAT_SX] + 
				ctx->Viewport.WindowMap.m[MAT_TX]);
   ctx->Current.RasterPos[1] = (ndc[1] * ctx->Viewport.WindowMap.m[MAT_SY] + 
				ctx->Viewport.WindowMap.m[MAT_TY]);
   ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport.WindowMap.m[MAT_SZ] + 
				ctx->Viewport.WindowMap.m[MAT_TZ]) / ctx->Visual->DepthMaxF;
   ctx->Current.RasterPos[3] = clip[3];
   ctx->Current.RasterPosValid = GL_TRUE;

   /* FOG??? */

   {
      GLuint texSet;
      for (texSet=0; texSet<MAX_TEXTURE_UNITS; texSet++) {
         COPY_4FV( ctx->Current.RasterMultiTexCoord[texSet],
                  ctx->Current.Texcoord[texSet] );
      }
   }

   if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}



void
_mesa_RasterPos2d(GLdouble x, GLdouble y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

void
_mesa_RasterPos2f(GLfloat x, GLfloat y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

void
_mesa_RasterPos2i(GLint x, GLint y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

void
_mesa_RasterPos2s(GLshort x, GLshort y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

void
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

void
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

void
_mesa_RasterPos3i(GLint x, GLint y, GLint z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

void
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

void
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_RasterPos4f(x, y, z, w);
}

void
_mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   raster_pos4f(ctx, x, y, z, w);
}

void
_mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   _mesa_RasterPos4f(x, y, z, w);
}

void
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   _mesa_RasterPos4f(x, y, z, w);
}

void
_mesa_RasterPos2dv(const GLdouble *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos2fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos2iv(const GLint *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos2sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos3dv(const GLdouble *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

void
_mesa_RasterPos3fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

void
_mesa_RasterPos3iv(const GLint *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

void
_mesa_RasterPos3sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

void
_mesa_RasterPos4dv(const GLdouble *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}

void
_mesa_RasterPos4fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}

void
_mesa_RasterPos4iv(const GLint *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}

void
_mesa_RasterPos4sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}
