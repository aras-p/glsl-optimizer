/* $Id: rastpos.c,v 1.31 2001/09/18 23:06:14 kschultz Exp $ */

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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "clip.h"
#include "colormac.h"
#include "context.h"
#include "feedback.h"
#include "light.h"
#include "macros.h"
#include "mmath.h"
#include "rastpos.h"
#include "state.h"
#include "simple_list.h"
#include "mtypes.h"

#include "math/m_matrix.h"
#include "math/m_xform.h"
#endif


/*
 * Clip a point against the view volume.
 * Input:  v - vertex-vector describing the point to clip
 * Return:  0 = outside view volume
 *          1 = inside view volume
 */
static GLuint
viewclip_point( const GLfloat v[] )
{
   if (   v[0] > v[3] || v[0] < -v[3]
       || v[1] > v[3] || v[1] < -v[3]
       || v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}


/* As above, but only clip test against far/near Z planes */
static GLuint
viewclip_point_z( const GLfloat v[] )
{
   if (v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}



/*
 * Clip a point against the user clipping planes.
 * Input:  v - vertex-vector describing the point to clip.
 * Return:  0 = point was clipped
 *          1 = point not clipped
 */
static GLuint
userclip_point( GLcontext* ctx, const GLfloat v[] )
{
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 GLfloat dot = v[0] * ctx->Transform._ClipUserPlane[p][0]
		     + v[1] * ctx->Transform._ClipUserPlane[p][1]
		     + v[2] * ctx->Transform._ClipUserPlane[p][2]
		     + v[3] * ctx->Transform._ClipUserPlane[p][3];
         if (dot < 0.0F) {
            return 0;
         }
      }
   }

   return 1;
}


/* This has been split off to allow the normal shade routines to
 * get a little closer to the vertex buffer, and to use the
 * GLvector objects directly.
 * Input: ctx - the context
 *        vertex - vertex location
 *        normal - normal vector
 * Output: Rcolor - returned color
 *         Rspec  - returned specular color (if separate specular enabled)
 *         Rindex - returned color index
 */
static void
shade_rastpos(GLcontext *ctx,
              const GLfloat vertex[4],
              const GLfloat normal[3],
              GLfloat Rcolor[4],
              GLfloat Rspec[4],
              GLuint *Rindex)
{
   GLfloat (*base)[3] = ctx->Light._BaseColor;
   struct gl_light *light;
   GLfloat diffuseColor[4], specularColor[4];
   GLfloat diffuse = 0, specular = 0;

   if (!ctx->_ShineTable[0] || !ctx->_ShineTable[1])
      _mesa_validate_all_lighting_tables( ctx );

   COPY_3V(diffuseColor, base[0]);
   diffuseColor[3] = CLAMP( ctx->Light.Material[0].Diffuse[3], 0.0F, 1.0F );
   ASSIGN_4V(specularColor, 0.0, 0.0, 0.0, 0.0);

   foreach (light, &ctx->Light.EnabledList) {
      GLfloat n_dot_h;
      GLfloat attenuation = 1.0;
      GLfloat VP[3];
      GLfloat n_dot_VP;
      GLfloat *h;
      GLfloat diffuseContrib[3], specularContrib[3];
      GLboolean normalized;

      if (!(light->_Flags & LIGHT_POSITIONAL)) {
	 COPY_3V(VP, light->_VP_inf_norm);
	 attenuation = light->_VP_inf_spot_attenuation;
      }
      else {
	 GLfloat d;

	 SUB_3V(VP, light->_Position, vertex);
	 d = (GLfloat) LEN_3FV( VP );

	 if ( d > 1e-6) {
	    GLfloat invd = 1.0F / d;
	    SELF_SCALE_SCALAR_3V(VP, invd);
	 }
	 attenuation = 1.0F / (light->ConstantAttenuation + d *
			       (light->LinearAttenuation + d *
				light->QuadraticAttenuation));

	 if (light->_Flags & LIGHT_SPOT) {
	    GLfloat PV_dot_dir = - DOT3(VP, light->_NormDirection);

	    if (PV_dot_dir<light->_CosCutoff) {
	       continue;
	    }
	    else {
	       double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
	       int k = (int) x;
	       GLfloat spot = (GLfloat) (light->_SpotExpTable[k][0]
			       + (x-k)*light->_SpotExpTable[k][1]);
	       attenuation *= spot;
	    }
	 }
      }

      if (attenuation < 1e-3)
	 continue;

      n_dot_VP = DOT3( normal, VP );

      if (n_dot_VP < 0.0F) {
	 ACC_SCALE_SCALAR_3V(diffuseColor, attenuation, light->_MatAmbient[0]);
	 continue;
      }

      COPY_3V(diffuseContrib, light->_MatAmbient[0]);
      ACC_SCALE_SCALAR_3V(diffuseContrib, n_dot_VP, light->_MatDiffuse[0]);
      diffuse += n_dot_VP * light->_dli * attenuation;
      ASSIGN_3V(specularContrib, 0.0, 0.0, 0.0);

      {
	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);
	    h = VP;
	    normalized = 0;
	 }
	 else if (light->_Flags & LIGHT_POSITIONAL) {
	    h = VP;
	    ACC_3V(h, ctx->_EyeZDir);
	    normalized = 0;
	 }
         else {
	    h = light->_h_inf_norm;
	    normalized = 1;
	 }

	 n_dot_h = DOT3(normal, h);

	 if (n_dot_h > 0.0F) {
	    const struct gl_material *mat = &ctx->Light.Material[0];
	    GLfloat spec_coef;
	    GLfloat shininess = mat->Shininess;

	    if (!normalized) {
	       n_dot_h *= n_dot_h;
	       n_dot_h /= LEN_SQUARED_3FV( h );
	       shininess *= .5;
	    }

	    GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec_coef );

	    if (spec_coef > 1.0e-10) {
               if (ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) {
                  ACC_SCALE_SCALAR_3V( specularContrib, spec_coef,
                                       light->_MatSpecular[0]);
               }
               else {
                  ACC_SCALE_SCALAR_3V( diffuseContrib, spec_coef,
                                       light->_MatSpecular[0]);
               }
	       specular += spec_coef * light->_sli * attenuation;
	    }
	 }
      }

      ACC_SCALE_SCALAR_3V( diffuseColor, attenuation, diffuseContrib );
      ACC_SCALE_SCALAR_3V( specularColor, attenuation, specularContrib );
   }

   if (ctx->Visual.rgbMode) {
      Rcolor[0] = CLAMP(diffuseColor[0], 0.0F, 1.0F);
      Rcolor[1] = CLAMP(diffuseColor[1], 0.0F, 1.0F);
      Rcolor[2] = CLAMP(diffuseColor[2], 0.0F, 1.0F);
      Rcolor[3] = CLAMP(diffuseColor[3], 0.0F, 1.0F);
      Rspec[0] = CLAMP(specularColor[0], 0.0F, 1.0F);
      Rspec[1] = CLAMP(specularColor[1], 0.0F, 1.0F);
      Rspec[2] = CLAMP(specularColor[2], 0.0F, 1.0F);
      Rspec[3] = CLAMP(specularColor[3], 0.0F, 1.0F);
   }
   else {
      struct gl_material *mat = &ctx->Light.Material[0];
      GLfloat d_a = mat->DiffuseIndex - mat->AmbientIndex;
      GLfloat s_a = mat->SpecularIndex - mat->AmbientIndex;
      GLfloat ind = mat->AmbientIndex
                  + diffuse * (1.0F-specular) * d_a
                  + specular * s_a;
      if (ind > mat->SpecularIndex) {
	 ind = mat->SpecularIndex;
      }
      *Rindex = (GLuint) (GLint) ind;
   }

}

/*
 * Caller:  context->API.RasterPos4f
 */
static void
raster_pos4f(GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GLfloat v[4], eye[4], clip[4], ndc[3], d;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   FLUSH_CURRENT(ctx, 0);

   if (ctx->NewState)
      _mesa_update_state( ctx );

   ASSIGN_4V( v, x, y, z, w );
   TRANSFORM_POINT( eye, ctx->ModelView.m, v );

   /* raster color */
   if (ctx->Light.Enabled) {
      GLfloat *norm, eyenorm[3];
      GLfloat *objnorm = ctx->Current.Normal;

      if (ctx->_NeedEyeCoords) {
	 GLfloat *inv = ctx->ModelView.inv;
	 TRANSFORM_NORMAL( eyenorm, objnorm, inv );
	 norm = eyenorm;
      }
      else {
	 norm = objnorm;
      }

      shade_rastpos( ctx, v, norm,
                     ctx->Current.RasterColor,
                     ctx->Current.RasterSecondaryColor,
                     &ctx->Current.RasterIndex );

   }
   else {
      /* use current color or index */
      if (ctx->Visual.rgbMode) {
         COPY_4FV(ctx->Current.RasterColor, ctx->Current.Color);
         COPY_4FV(ctx->Current.RasterSecondaryColor,
                  ctx->Current.SecondaryColor);
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
   if (ctx->Transform.RasterPositionUnclipped) {
      /* GL_IBM_rasterpos_clip: only clip against Z */
      if (viewclip_point_z(clip) == 0)
         ctx->Current.RasterPosValid = GL_FALSE;
   }
   else if (viewclip_point(clip) == 0) {
      /* Normal OpenGL behaviour */
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* clip to user clipping planes */
   if (ctx->Transform._AnyClip &&
       userclip_point(ctx, clip) == 0) {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* ndc = clip / W */
   ASSERT( clip[3]!=0.0 );
   d = 1.0F / clip[3];
   ndc[0] = clip[0] * d;
   ndc[1] = clip[1] * d;
   ndc[2] = clip[2] * d;

   ctx->Current.RasterPos[0] = (ndc[0] * ctx->Viewport._WindowMap.m[MAT_SX] +
				ctx->Viewport._WindowMap.m[MAT_TX]);
   ctx->Current.RasterPos[1] = (ndc[1] * ctx->Viewport._WindowMap.m[MAT_SY] +
				ctx->Viewport._WindowMap.m[MAT_TY]);
   ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport._WindowMap.m[MAT_SZ] +
				ctx->Viewport._WindowMap.m[MAT_TZ]) / ctx->DepthMaxF;
   ctx->Current.RasterPos[3] = clip[3];
   ctx->Current.RasterPosValid = GL_TRUE;

   ctx->Current.RasterFogCoord = ctx->Current.FogCoord;

   {
      GLuint texSet;
      for (texSet = 0; texSet < ctx->Const.MaxTextureUnits; texSet++) {
         COPY_4FV( ctx->Current.RasterMultiTexCoord[texSet],
                  ctx->Current.Texcoord[texSet] );
      }
   }

   if (ctx->RenderMode==GL_SELECT) {
      _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}



void
_mesa_RasterPos2d(GLdouble x, GLdouble y)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void
_mesa_RasterPos2f(GLfloat x, GLfloat y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

void
_mesa_RasterPos2i(GLint x, GLint y)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void
_mesa_RasterPos2s(GLshort x, GLshort y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

void
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

void
_mesa_RasterPos3i(GLint x, GLint y, GLint z)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

void
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
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
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   _mesa_RasterPos4f(x, y, z, w);
}

void
_mesa_RasterPos2dv(const GLdouble *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos2fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos2iv(const GLint *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos2sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_RasterPos3dv(const GLdouble *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void
_mesa_RasterPos3fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

void
_mesa_RasterPos3iv(const GLint *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void
_mesa_RasterPos3sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

void
_mesa_RasterPos4dv(const GLdouble *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

void
_mesa_RasterPos4fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}

void
_mesa_RasterPos4iv(const GLint *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

void
_mesa_RasterPos4sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}



/**********************************************************************/
/***                     GL_MESA_window_pos                         ***/
/**********************************************************************/


/*
 * This is a MESA extension function.  Pretty much just like glRasterPos
 * except we don't apply the modelview or projection matrices; specify a
 * window coordinate directly.
 * Caller:  context->API.WindowPos4fMESA pointer.
 */
void
_mesa_WindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   FLUSH_CURRENT(ctx, 0);

   /* set raster position */
   ctx->Current.RasterPos[0] = x;
   ctx->Current.RasterPos[1] = y;
   ctx->Current.RasterPos[2] = CLAMP( z, 0.0F, 1.0F );
   ctx->Current.RasterPos[3] = w;

   ctx->Current.RasterPosValid = GL_TRUE;
   ctx->Current.RasterDistance = 0.0F;
   ctx->Current.RasterFogCoord = 0.0F;

   /* raster color = current color or index */
   if (ctx->Visual.rgbMode) {
      ctx->Current.RasterColor[0] = (ctx->Current.Color[0]);
      ctx->Current.RasterColor[1] = (ctx->Current.Color[1]);
      ctx->Current.RasterColor[2] = (ctx->Current.Color[2]);
      ctx->Current.RasterColor[3] = (ctx->Current.Color[3]);
   }
   else {
      ctx->Current.RasterIndex = ctx->Current.Index;
   }

   /* raster texcoord = current texcoord */
   {
      GLuint texSet;
      for (texSet = 0; texSet < ctx->Const.MaxTextureUnits; texSet++) {
         COPY_4FV( ctx->Current.RasterMultiTexCoord[texSet],
                  ctx->Current.Texcoord[texSet] );
      }
   }

   if (ctx->RenderMode==GL_SELECT) {
      _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}




void
_mesa_WindowPos2dMESA(GLdouble x, GLdouble y)
{
   _mesa_WindowPos4fMESA((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void
_mesa_WindowPos2fMESA(GLfloat x, GLfloat y)
{
   _mesa_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

void
_mesa_WindowPos2iMESA(GLint x, GLint y)
{
   _mesa_WindowPos4fMESA((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void
_mesa_WindowPos2sMESA(GLshort x, GLshort y)
{
   _mesa_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

void
_mesa_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   _mesa_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void
_mesa_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   _mesa_WindowPos4fMESA(x, y, z, 1.0F);
}

void
_mesa_WindowPos3iMESA(GLint x, GLint y, GLint z)
{
   _mesa_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void
_mesa_WindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   _mesa_WindowPos4fMESA(x, y, z, 1.0F);
}

void
_mesa_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void
_mesa_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   _mesa_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void
_mesa_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   _mesa_WindowPos4fMESA(x, y, z, w);
}

void
_mesa_WindowPos2dvMESA(const GLdouble *v)
{
   _mesa_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void
_mesa_WindowPos2fvMESA(const GLfloat *v)
{
   _mesa_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_WindowPos2ivMESA(const GLint *v)
{
   _mesa_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void
_mesa_WindowPos2svMESA(const GLshort *v)
{
   _mesa_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

void
_mesa_WindowPos3dvMESA(const GLdouble *v)
{
   _mesa_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void
_mesa_WindowPos3fvMESA(const GLfloat *v)
{
   _mesa_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

void
_mesa_WindowPos3ivMESA(const GLint *v)
{
   _mesa_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void
_mesa_WindowPos3svMESA(const GLshort *v)
{
   _mesa_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

void
_mesa_WindowPos4dvMESA(const GLdouble *v)
{
   _mesa_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 
			 (GLfloat) v[2], (GLfloat) v[3]);
}

void
_mesa_WindowPos4fvMESA(const GLfloat *v)
{
   _mesa_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}

void
_mesa_WindowPos4ivMESA(const GLint *v)
{
   _mesa_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 
			 (GLfloat) v[2], (GLfloat) v[3]);
}

void
_mesa_WindowPos4svMESA(const GLshort *v)
{
   _mesa_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}



#if 0

/*
 * OpenGL implementation of glWindowPos*MESA()
 */
void glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GLfloat fx, fy;

   /* Push current matrix mode and viewport attributes */
   glPushAttrib( GL_TRANSFORM_BIT | GL_VIEWPORT_BIT );

   /* Setup projection parameters */
   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glLoadIdentity();

   glDepthRange( z, z );
   glViewport( (int) x - 1, (int) y - 1, 2, 2 );

   /* set the raster (window) position */
   fx = x - (int) x;
   fy = y - (int) y;
   glRasterPos4f( fx, fy, 0.0, w );

   /* restore matrices, viewport and matrix mode */
   glPopMatrix();
   glMatrixMode( GL_PROJECTION );
   glPopMatrix();

   glPopAttrib();
}

#endif
