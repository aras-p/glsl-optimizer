/* $Id: light.c,v 1.24 2000/11/13 20:02:56 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mmath.h"
#include "simple_list.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#endif


/* XXX this is a bit of a hack needed for compilation within XFree86 */
#ifndef FLT_MIN
#define FLT_MIN 1e-37
#endif


void
_mesa_ShadeModel( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glShadeModel");

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glShadeModel %s\n", gl_lookup_enum_by_nr(mode));

   if (mode == GL_FLAT || mode == GL_SMOOTH) {
      if (ctx->Light.ShadeModel != mode) {
         ctx->Light.ShadeModel = mode;
         if (ctx->Light.ShadeModel == GL_FLAT)
            SET_BITS(ctx->_TriangleCaps, DD_FLATSHADE);
         else
            CLEAR_BITS(ctx->_TriangleCaps, DD_FLATSHADE);

         ctx->NewState |= _NEW_LIGHT;

         if (ctx->Driver.ShadeModel) 
            (*ctx->Driver.ShadeModel)( ctx, mode );
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glShadeModel" );
   }
}



void
_mesa_Lightf( GLenum light, GLenum pname, GLfloat param )
{
   _mesa_Lightfv( light, pname, &param );
}


void
_mesa_Lightfv( GLenum light, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i = (GLint) (light - GL_LIGHT0);
   struct gl_light *l = &ctx->Light.Light[i];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLight");

   if (i < 0 || i >= MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glLight" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( l->Ambient, params );
         break;
      case GL_DIFFUSE:
         COPY_4V( l->Diffuse, params );
         break;
      case GL_SPECULAR:
         COPY_4V( l->Specular, params );
         break;
      case GL_POSITION:
	 /* transform position by ModelView matrix */
	 TRANSFORM_POINT( l->EyePosition, ctx->ModelView.m, params );
	 if (l->EyePosition[3] != 0.0F) 
	    l->_Flags |= LIGHT_POSITIONAL;
	 else
	    l->_Flags &= ~LIGHT_POSITIONAL;
         break;
      case GL_SPOT_DIRECTION:
	 /* transform direction by inverse modelview */
	 if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
	    gl_matrix_analyze( &ctx->ModelView );
	 }
	 TRANSFORM_NORMAL( l->EyeDirection, params, ctx->ModelView.inv );
         break;
      case GL_SPOT_EXPONENT:
         if (params[0]<0.0 || params[0]>128.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         if (l->SpotExponent != params[0]) {
            l->SpotExponent = params[0];
            gl_compute_spot_exp_table( l );
         }
         break;
      case GL_SPOT_CUTOFF:
         if ((params[0]<0.0 || params[0]>90.0) && params[0]!=180.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         l->SpotCutoff = params[0];
         l->_CosCutoff = cos(params[0]*DEG2RAD);
         if (l->_CosCutoff < 0) 
	    l->_CosCutoff = 0;
	 if (l->SpotCutoff != 180.0F)
	    l->_Flags |= LIGHT_SPOT;
	 else
	    l->_Flags &= ~LIGHT_SPOT;
         break;
      case GL_CONSTANT_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         l->ConstantAttenuation = params[0];
         break;
      case GL_LINEAR_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         l->LinearAttenuation = params[0];
         break;
      case GL_QUADRATIC_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         l->QuadraticAttenuation = params[0];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLight" );
         return;
   }

   if (ctx->Driver.Lightfv)
      ctx->Driver.Lightfv( ctx, light, pname, params );

   ctx->NewState |= _NEW_LIGHT;
}


void
_mesa_Lighti( GLenum light, GLenum pname, GLint param )
{
   _mesa_Lightiv( light, pname, &param );
}


void
_mesa_Lightiv( GLenum light, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];

   switch (pname) {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_POSITION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         fparam[3] = (GLfloat) params[3];
         break;
      case GL_SPOT_DIRECTION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         break;
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* error will be caught later in gl_Lightfv */
         ;
   }

   _mesa_Lightfv( light, pname, fparam );
}



void
_mesa_GetLightfv( GLenum light, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint l = (GLint) (light - GL_LIGHT0);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetLight");

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( params, ctx->Light.Light[l].Ambient );
         break;
      case GL_DIFFUSE:
         COPY_4V( params, ctx->Light.Light[l].Diffuse );
         break;
      case GL_SPECULAR:
         COPY_4V( params, ctx->Light.Light[l].Specular );
         break;
      case GL_POSITION:
         COPY_4V( params, ctx->Light.Light[l].EyePosition );
         break;
      case GL_SPOT_DIRECTION:
         COPY_3V( params, ctx->Light.Light[l].EyeDirection );
         break;
      case GL_SPOT_EXPONENT:
         params[0] = ctx->Light.Light[l].SpotExponent;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = ctx->Light.Light[l].SpotCutoff;
         break;
      case GL_CONSTANT_ATTENUATION:
         params[0] = ctx->Light.Light[l].ConstantAttenuation;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = ctx->Light.Light[l].LinearAttenuation;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = ctx->Light.Light[l].QuadraticAttenuation;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
         break;
   }
}



void
_mesa_GetLightiv( GLenum light, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint l = (GLint) (light - GL_LIGHT0);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetLight");

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[3]);
         break;
      case GL_DIFFUSE:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[3]);
         break;
      case GL_SPECULAR:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[3]);
         break;
      case GL_POSITION:
         params[0] = (GLint) ctx->Light.Light[l].EyePosition[0];
         params[1] = (GLint) ctx->Light.Light[l].EyePosition[1];
         params[2] = (GLint) ctx->Light.Light[l].EyePosition[2];
         params[3] = (GLint) ctx->Light.Light[l].EyePosition[3];
         break;
      case GL_SPOT_DIRECTION:
         params[0] = (GLint) ctx->Light.Light[l].EyeDirection[0];
         params[1] = (GLint) ctx->Light.Light[l].EyeDirection[1];
         params[2] = (GLint) ctx->Light.Light[l].EyeDirection[2];
         break;
      case GL_SPOT_EXPONENT:
         params[0] = (GLint) ctx->Light.Light[l].SpotExponent;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = (GLint) ctx->Light.Light[l].SpotCutoff;
         break;
      case GL_CONSTANT_ATTENUATION:
         params[0] = (GLint) ctx->Light.Light[l].ConstantAttenuation;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = (GLint) ctx->Light.Light[l].LinearAttenuation;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = (GLint) ctx->Light.Light[l].QuadraticAttenuation;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
         break;
   }
}



/**********************************************************************/
/***                        Light Model                             ***/
/**********************************************************************/


void
_mesa_LightModelfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLightModelfv");

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         COPY_4V( ctx->Light.Model.Ambient, params );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
         if (params[0]==0.0)
            ctx->Light.Model.LocalViewer = GL_FALSE;
         else
            ctx->Light.Model.LocalViewer = GL_TRUE;
         break;
      case GL_LIGHT_MODEL_TWO_SIDE:
         if (params[0]==0.0) 
            ctx->Light.Model.TwoSide = GL_FALSE;
         else
            ctx->Light.Model.TwoSide = GL_TRUE;
         break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         if (params[0] == (GLfloat) GL_SINGLE_COLOR) {
            ctx->Light.Model.ColorControl = GL_SINGLE_COLOR;
	    if (!ctx->Fog.ColorSumEnabled)
	       CLEAR_BITS(ctx->_TriangleCaps, DD_SEPERATE_SPECULAR);
         }
         else if (params[0] == (GLfloat) GL_SEPARATE_SPECULAR_COLOR) {
            ctx->Light.Model.ColorControl = GL_SEPARATE_SPECULAR_COLOR;
	    SET_BITS(ctx->_TriangleCaps, DD_SEPERATE_SPECULAR);
	 }
         else {
            gl_error( ctx, GL_INVALID_ENUM, "glLightModel(param)" );
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLightModel" );
         break;
   }

   if (ctx->Driver.LightModelfv) 
      ctx->Driver.LightModelfv( ctx, pname, params );

   ctx->NewState |= _NEW_LIGHT;
}


void
_mesa_LightModeliv( GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLightModeliv");

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      case GL_LIGHT_MODEL_TWO_SIDE:
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* Error will be caught later in gl_LightModelfv */
         ;
   }
   _mesa_LightModelfv( pname, fparam );
}


void
_mesa_LightModeli( GLenum pname, GLint param )
{
   _mesa_LightModeliv( pname, &param );
}


void
_mesa_LightModelf( GLenum pname, GLfloat param )
{
   _mesa_LightModelfv( pname, &param );
}



/********** MATERIAL **********/


/*
 * Given a face and pname value (ala glColorMaterial), compute a bitmask
 * of the targeted material values.
 */
GLuint gl_material_bitmask( GLcontext *ctx, GLenum face, GLenum pname, 
			    GLuint legal,
			    const char *where )
{
   GLuint bitmask = 0;

   /* Make a bitmask indicating what material attribute(s) we're updating */
   switch (pname) {
      case GL_EMISSION:
         bitmask |= FRONT_EMISSION_BIT | BACK_EMISSION_BIT;
         break;
      case GL_AMBIENT:
         bitmask |= FRONT_AMBIENT_BIT | BACK_AMBIENT_BIT;
         break;
      case GL_DIFFUSE:
         bitmask |= FRONT_DIFFUSE_BIT | BACK_DIFFUSE_BIT;
         break;
      case GL_SPECULAR:
         bitmask |= FRONT_SPECULAR_BIT | BACK_SPECULAR_BIT;
         break;
      case GL_SHININESS:
         bitmask |= FRONT_SHININESS_BIT | BACK_SHININESS_BIT;
         break;
      case GL_AMBIENT_AND_DIFFUSE:
         bitmask |= FRONT_AMBIENT_BIT | BACK_AMBIENT_BIT;
         bitmask |= FRONT_DIFFUSE_BIT | BACK_DIFFUSE_BIT;
         break;
      case GL_COLOR_INDEXES:
         bitmask |= FRONT_INDEXES_BIT  | BACK_INDEXES_BIT;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, where );
         return 0;
   }

   if (face==GL_FRONT) {
      bitmask &= FRONT_MATERIAL_BITS;
   }
   else if (face==GL_BACK) {
      bitmask &= BACK_MATERIAL_BITS;
   }
   else if (face != GL_FRONT_AND_BACK) {
      gl_error( ctx, GL_INVALID_ENUM, where );
      return 0;
   }
   
   if (bitmask & ~legal) {
      gl_error( ctx, GL_INVALID_ENUM, where );
      return 0;
   }

   return bitmask;
}



/*
 * Check if the global material has to be updated with info that was
 * associated with a vertex via glMaterial.
 * This function is used when any material values get changed between
 * glBegin/glEnd either by calling glMaterial() or by calling glColor()
 * when GL_COLOR_MATERIAL is enabled.
 *
 * src[0] is front material, src[1] is back material
 *
 * KW: Added code here to keep the precomputed variables uptodate.
 *     This means we can use the faster shade functions when using
 *     GL_COLOR_MATERIAL, and we can also now use the precomputed 
 *     values in the slower shading functions, which further offsets
 *     the cost of doing this here.
 */
void gl_update_material( GLcontext *ctx, 
			 const struct gl_material src[2], 
			 GLuint bitmask )
{
   struct gl_light *light, *list = &ctx->Light.EnabledList;

   if (ctx->Light.ColorMaterialEnabled)
      bitmask &= ~ctx->Light.ColorMaterialBitmask;

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      fprintf(stderr, "gl_update_material, mask 0x%x\n", bitmask);

   if (!bitmask) 
      return;

   /* update material emission */
   if (bitmask & FRONT_EMISSION_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      COPY_4FV( mat->Emission, src[0].Emission );
   }
   if (bitmask & BACK_EMISSION_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      COPY_4FV( mat->Emission, src[1].Emission );
   }

   /* update material ambience */
   if (bitmask & FRONT_AMBIENT_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      COPY_4FV( mat->Ambient, src[0].Ambient );
      foreach (light, list) {
         SCALE_3V( light->_MatAmbient[0], light->Ambient, src[0].Ambient);
      }
   }
   if (bitmask & BACK_AMBIENT_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      COPY_4FV( mat->Ambient, src[1].Ambient );
      foreach (light, list) {
         SCALE_3V( light->_MatAmbient[1], light->Ambient, src[1].Ambient);
      }
   }

   /* update BaseColor = emission + scene's ambience * material's ambience */
   if (bitmask & (FRONT_EMISSION_BIT | FRONT_AMBIENT_BIT)) {
      struct gl_material *mat = &ctx->Light.Material[0];
      COPY_3V( ctx->Light._BaseColor[0], mat->Emission );
      ACC_SCALE_3V( ctx->Light._BaseColor[0], mat->Ambient, ctx->Light.Model.Ambient );
   }
   if (bitmask & (BACK_EMISSION_BIT | BACK_AMBIENT_BIT)) {
      struct gl_material *mat = &ctx->Light.Material[1];
      COPY_3V( ctx->Light._BaseColor[1], mat->Emission );
      ACC_SCALE_3V( ctx->Light._BaseColor[1], mat->Ambient, ctx->Light.Model.Ambient );
   }

   /* update material diffuse values */
   if (bitmask & FRONT_DIFFUSE_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      GLfloat tmp[4];
      SUB_3V( tmp, src[0].Diffuse, mat->Diffuse );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatDiffuse[0], light->Diffuse, tmp );
      }
      COPY_4FV( mat->Diffuse, src[0].Diffuse );
      FLOAT_COLOR_TO_CHAN(ctx->Light._BaseAlpha[0], mat->Diffuse[3]);
   }
   if (bitmask & BACK_DIFFUSE_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      GLfloat tmp[4];
      SUB_3V( tmp, src[1].Diffuse, mat->Diffuse );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatDiffuse[1], light->Diffuse, tmp );
      }
      COPY_4FV( mat->Diffuse, src[1].Diffuse );
      FLOAT_COLOR_TO_CHAN(ctx->Light._BaseAlpha[1], mat->Diffuse[3]);
   }

   /* update material specular values */
   if (bitmask & FRONT_SPECULAR_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      GLfloat tmp[4];
      SUB_3V( tmp, src[0].Specular, mat->Specular );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatSpecular[0], light->Specular, tmp );
	 light->_IsMatSpecular[0] = 
	    (LEN_SQUARED_3FV(light->_MatSpecular[0]) > 1e-16);
      }
      COPY_4FV( mat->Specular, src[0].Specular );
   }
   if (bitmask & BACK_SPECULAR_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      GLfloat tmp[4];
      SUB_3V( tmp, src[1].Specular, mat->Specular );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatSpecular[1], light->Specular, tmp );
	 light->_IsMatSpecular[1] = 
	    (LEN_SQUARED_3FV(light->_MatSpecular[1]) > 1e-16);
      }
      COPY_4FV( mat->Specular, src[1].Specular );
   }

   if (bitmask & FRONT_SHININESS_BIT) {
      GLfloat shininess = ctx->Light.Material[0].Shininess = src[0].Shininess;
      gl_compute_shine_table( ctx, 0, shininess );
      gl_compute_shine_table( ctx, 2, shininess * .5 );
   }
   if (bitmask & BACK_SHININESS_BIT) {
      GLfloat shininess = ctx->Light.Material[1].Shininess = src[1].Shininess;
      gl_compute_shine_table( ctx, 1, shininess );
      gl_compute_shine_table( ctx, 3, shininess * .5 );
   }

   if (bitmask & FRONT_INDEXES_BIT) {
      ctx->Light.Material[0].AmbientIndex = src[0].AmbientIndex;
      ctx->Light.Material[0].DiffuseIndex = src[0].DiffuseIndex;
      ctx->Light.Material[0].SpecularIndex = src[0].SpecularIndex;
   }
   if (bitmask & BACK_INDEXES_BIT) {
      ctx->Light.Material[1].AmbientIndex = src[1].AmbientIndex;
      ctx->Light.Material[1].DiffuseIndex = src[1].DiffuseIndex;
      ctx->Light.Material[1].SpecularIndex = src[1].SpecularIndex;
   }

   if (0)
   {
      struct gl_material *mat = &ctx->Light.Material[0];
      fprintf(stderr, "update_mat  emission : %f %f %f\n",
	      mat->Emission[0],
	      mat->Emission[1],
	      mat->Emission[2]);
      fprintf(stderr, "update_mat  specular : %f %f %f\n",
	      mat->Specular[0],
	      mat->Specular[1],
	      mat->Specular[2]);
      fprintf(stderr, "update_mat  diffuse : %f %f %f\n",
	      mat->Diffuse[0],
	      mat->Diffuse[1],
	      mat->Diffuse[2]);
      fprintf(stderr, "update_mat  ambient : %f %f %f\n",
	      mat->Ambient[0],
	      mat->Ambient[1],
	      mat->Ambient[2]);
   }
}




/*
 * Update the current materials from the given rgba color
 * according to the bitmask in ColorMaterialBitmask, which is
 * set by glColorMaterial().
 */
void gl_update_color_material( GLcontext *ctx, 
			       const GLchan rgba[4] )
{
   struct gl_light *light, *list = &ctx->Light.EnabledList;
   GLuint bitmask = ctx->Light.ColorMaterialBitmask;
   GLfloat color[4];

   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      fprintf(stderr, "gl_update_color_material, mask 0x%x\n", bitmask);

   /* update emissive colors */
   if (bitmask & FRONT_EMISSION_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      COPY_4FV( mat->Emission, color );
   }

   if (bitmask & BACK_EMISSION_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      COPY_4FV( mat->Emission, color );
   }

   /* update light->_MatAmbient = light's ambient * material's ambient */
   if (bitmask & FRONT_AMBIENT_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      foreach (light, list) {
         SCALE_3V( light->_MatAmbient[0], light->Ambient, color);
      }
      COPY_4FV( mat->Ambient, color );
   }

   if (bitmask & BACK_AMBIENT_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      foreach (light, list) {
         SCALE_3V( light->_MatAmbient[1], light->Ambient, color);
      }
      COPY_4FV( mat->Ambient, color );
   }

   /* update BaseColor = emission + scene's ambience * material's ambience */
   if (bitmask & (FRONT_EMISSION_BIT | FRONT_AMBIENT_BIT)) {
      struct gl_material *mat = &ctx->Light.Material[0];
      COPY_3V( ctx->Light._BaseColor[0], mat->Emission );
      ACC_SCALE_3V( ctx->Light._BaseColor[0], mat->Ambient, ctx->Light.Model.Ambient );
   }

   if (bitmask & (BACK_EMISSION_BIT | BACK_AMBIENT_BIT)) {
      struct gl_material *mat = &ctx->Light.Material[1];
      COPY_3V( ctx->Light._BaseColor[1], mat->Emission );
      ACC_SCALE_3V( ctx->Light._BaseColor[1], mat->Ambient, ctx->Light.Model.Ambient );
   }

   /* update light->_MatDiffuse = light's diffuse * material's diffuse */
   if (bitmask & FRONT_DIFFUSE_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      GLfloat tmp[4];
      SUB_3V( tmp, color, mat->Diffuse );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatDiffuse[0], light->Diffuse, tmp );
      }
      COPY_4FV( mat->Diffuse, color );
      FLOAT_COLOR_TO_CHAN(ctx->Light._BaseAlpha[0], mat->Diffuse[3]);
   }

   if (bitmask & BACK_DIFFUSE_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      GLfloat tmp[4];
      SUB_3V( tmp, color, mat->Diffuse );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatDiffuse[1], light->Diffuse, tmp );
      }
      COPY_4FV( mat->Diffuse, color );
      FLOAT_COLOR_TO_CHAN(ctx->Light._BaseAlpha[1], mat->Diffuse[3]);
   }

   /* update light->_MatSpecular = light's specular * material's specular */
   if (bitmask & FRONT_SPECULAR_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      GLfloat tmp[4];
      SUB_3V( tmp, color, mat->Specular );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatSpecular[0], light->Specular, tmp );
	 light->_IsMatSpecular[0] = 
	    (LEN_SQUARED_3FV(light->_MatSpecular[0]) > 1e-16);
      }
      COPY_4FV( mat->Specular, color );
   }

   if (bitmask & BACK_SPECULAR_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      GLfloat tmp[4];
      SUB_3V( tmp, color, mat->Specular );
      foreach (light, list) {
	 ACC_SCALE_3V( light->_MatSpecular[1], light->Specular, tmp );
	 light->_IsMatSpecular[1] = 
	    (LEN_SQUARED_3FV(light->_MatSpecular[1]) > 1e-16);
      }
      COPY_4FV( mat->Specular, color );
   }

   if (0)
   {
      struct gl_material *mat = &ctx->Light.Material[0];
      fprintf(stderr, "update_color_mat  emission : %f %f %f\n",
	      mat->Emission[0],
	      mat->Emission[1],
	      mat->Emission[2]);
      fprintf(stderr, "update_color_mat  specular : %f %f %f\n",
	      mat->Specular[0],
	      mat->Specular[1],
	      mat->Specular[2]);
      fprintf(stderr, "update_color_mat  diffuse : %f %f %f\n",
	      mat->Diffuse[0],
	      mat->Diffuse[1],
	      mat->Diffuse[2]);
      fprintf(stderr, "update_color_mat  ambient : %f %f %f\n",
	      mat->Ambient[0],
	      mat->Ambient[1],
	      mat->Ambient[2]);
   }
}




void
_mesa_ColorMaterial( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint bitmask;
   GLuint legal = (FRONT_EMISSION_BIT | BACK_EMISSION_BIT |
		   FRONT_SPECULAR_BIT | BACK_SPECULAR_BIT |
		   FRONT_DIFFUSE_BIT  | BACK_DIFFUSE_BIT  |
		   FRONT_AMBIENT_BIT  | BACK_AMBIENT_BIT);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glColorMaterial");

   if (MESA_VERBOSE&VERBOSE_API) 
      fprintf(stderr, "glColorMaterial %s %s\n", 
	      gl_lookup_enum_by_nr(face),
	      gl_lookup_enum_by_nr(mode));

   bitmask = gl_material_bitmask( ctx, face, mode, legal, "glColorMaterial" );

   if (bitmask != 0) {
      ctx->Light.ColorMaterialBitmask = bitmask;
      ctx->Light.ColorMaterialFace = face;
      ctx->Light.ColorMaterialMode = mode;
   }

   if (ctx->Light.ColorMaterialEnabled)
      gl_update_color_material( ctx, ctx->Current.Color );

   ctx->NewState |= _NEW_LIGHT;
}




void
_mesa_Materialf( GLenum face, GLenum pname, GLfloat param )
{
   _mesa_Materialfv( face, pname, &param );
}


/* KW:  This is now called directly (ie by name) from the glMaterial* 
 *      API functions.
 */
void
_mesa_Materialfv( GLenum face, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct immediate *IM;
   struct gl_material *mat;
   GLuint bitmask;
   GLuint count;

   bitmask = gl_material_bitmask( ctx, face, pname, ~0, "gl_Materialfv" );
   if (bitmask == 0)
      return;

   IM = ctx->input;
   count = IM->Count;

   if (!IM->Material) {
      IM->Material = 
	 (struct gl_material (*)[2]) MALLOC( sizeof(struct gl_material) * 
					     VB_SIZE * 2 );
      IM->MaterialMask = (GLuint *) MALLOC( sizeof(GLuint) * VB_SIZE );
   }


   if (!(IM->Flag[count] & VERT_MATERIAL)) {
      IM->Flag[count] |= VERT_MATERIAL;
      IM->MaterialMask[count] = 0;      
   }


   IM->MaterialMask[count] |= bitmask;
   mat = IM->Material[count];

   if (bitmask & FRONT_AMBIENT_BIT) {
      COPY_4FV( mat[0].Ambient, params );
   }
   if (bitmask & BACK_AMBIENT_BIT) {
      COPY_4FV( mat[1].Ambient, params );
   }
   if (bitmask & FRONT_DIFFUSE_BIT) {
      COPY_4FV( mat[0].Diffuse, params );
   }
   if (bitmask & BACK_DIFFUSE_BIT) {
      COPY_4FV( mat[1].Diffuse, params );
   }
   if (bitmask & FRONT_SPECULAR_BIT) {
      COPY_4FV( mat[0].Specular, params );
   }
   if (bitmask & BACK_SPECULAR_BIT) {
      COPY_4FV( mat[1].Specular, params );
   }
   if (bitmask & FRONT_EMISSION_BIT) {
      COPY_4FV( mat[0].Emission, params );
   }
   if (bitmask & BACK_EMISSION_BIT) {
      COPY_4FV( mat[1].Emission, params );
   }
   if (bitmask & FRONT_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      mat[0].Shininess = shininess;
   }
   if (bitmask & BACK_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      mat[1].Shininess = shininess;
   }
   if (bitmask & FRONT_INDEXES_BIT) {
      mat[0].AmbientIndex = params[0];
      mat[0].DiffuseIndex = params[1];
      mat[0].SpecularIndex = params[2];
   }
   if (bitmask & BACK_INDEXES_BIT) {
      mat[1].AmbientIndex = params[0];
      mat[1].DiffuseIndex = params[1];
      mat[1].SpecularIndex = params[2];
   }
}


void
_mesa_Materiali(GLenum face, GLenum pname, GLint param )
{
   _mesa_Materialiv(face, pname, &param);
}


void
_mesa_Materialiv(GLenum face, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_EMISSION:
      case GL_AMBIENT_AND_DIFFUSE:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_SHININESS:
         fparam[0] = (GLfloat) params[0];
         break;
      case GL_COLOR_INDEXES:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         break;
      default:
         /* Error will be caught later in gl_Materialfv */
         ;
   }
   _mesa_Materialfv(face, pname, fparam);
}


void
_mesa_GetMaterialfv( GLenum face, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint f;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetMaterialfv");

   if (face==GL_FRONT) {
      f = 0;
   }
   else if (face==GL_BACK) {
      f = 1;
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(face)" );
      return;
   }
   switch (pname) {
      case GL_AMBIENT:
         COPY_4FV( params, ctx->Light.Material[f].Ambient );
         break;
      case GL_DIFFUSE:
         COPY_4FV( params, ctx->Light.Material[f].Diffuse );
	 break;
      case GL_SPECULAR:
         COPY_4FV( params, ctx->Light.Material[f].Specular );
	 break;
      case GL_EMISSION:
	 COPY_4FV( params, ctx->Light.Material[f].Emission );
	 break;
      case GL_SHININESS:
	 *params = ctx->Light.Material[f].Shininess;
	 break;
      case GL_COLOR_INDEXES:
	 params[0] = ctx->Light.Material[f].AmbientIndex;
	 params[1] = ctx->Light.Material[f].DiffuseIndex;
	 params[2] = ctx->Light.Material[f].SpecularIndex;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   }
}



void
_mesa_GetMaterialiv( GLenum face, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint f;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetMaterialiv");

   if (face==GL_FRONT) {
      f = 0;
   }
   else if (face==GL_BACK) {
      f = 1;
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialiv(face)" );
      return;
   }
   switch (pname) {
      case GL_AMBIENT:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[3] );
         break;
      case GL_DIFFUSE:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[3] );
	 break;
      case GL_SPECULAR:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[3] );
	 break;
      case GL_EMISSION:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[3] );
	 break;
      case GL_SHININESS:
         *params = ROUNDF( ctx->Light.Material[f].Shininess );
	 break;
      case GL_COLOR_INDEXES:
	 params[0] = ROUNDF( ctx->Light.Material[f].AmbientIndex );
	 params[1] = ROUNDF( ctx->Light.Material[f].DiffuseIndex );
	 params[2] = ROUNDF( ctx->Light.Material[f].SpecularIndex );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   }
}




/**********************************************************************/
/*****                  Lighting computation                      *****/
/**********************************************************************/


/*
 * Notes:
 *   When two-sided lighting is enabled we compute the color (or index)
 *   for both the front and back side of the primitive.  Then, when the
 *   orientation of the facet is later learned, we can determine which
 *   color (or index) to use for rendering.
 *
 *   KW: We now know orientation in advance and only shade for 
 *       the side or sides which are actually required.
 *
 * Variables:
 *   n = normal vector
 *   V = vertex position
 *   P = light source position
 *   Pe = (0,0,0,1)
 *
 * Precomputed:
 *   IF P[3]==0 THEN
 *       // light at infinity
 *       IF local_viewer THEN
 *           _VP_inf_norm = unit vector from V to P      // Precompute
 *       ELSE 
 *           // eye at infinity
 *           _h_inf_norm = Normalize( VP + <0,0,1> )     // Precompute
 *       ENDIF
 *   ENDIF
 *
 * Functions:
 *   Normalize( v ) = normalized vector v
 *   Magnitude( v ) = length of vector v
 */



/*
 * Whenever the spotlight exponent for a light changes we must call
 * this function to recompute the exponent lookup table.
 */
void
gl_compute_spot_exp_table( struct gl_light *l )
{
   GLint i;
   GLdouble exponent = l->SpotExponent;
   GLdouble tmp = 0;
   GLint clamp = 0;

   l->_SpotExpTable[0][0] = 0.0;

   for (i = EXP_TABLE_SIZE - 1; i > 0 ;i--) {
      if (clamp == 0) {
         tmp = pow(i / (GLdouble) (EXP_TABLE_SIZE - 1), exponent);
         if (tmp < FLT_MIN * 100.0) {
            tmp = 0.0;
            clamp = 1;
         }
      }
      l->_SpotExpTable[i][0] = tmp;
   }
   for (i = 0; i < EXP_TABLE_SIZE - 1; i++) {
      l->_SpotExpTable[i][1] = l->_SpotExpTable[i+1][0] - l->_SpotExpTable[i][0];
   }
   l->_SpotExpTable[EXP_TABLE_SIZE-1][1] = 0.0;
}




/* Calculate a new shine table.  Doing this here saves a branch in
 * lighting, and the cost of doing it early may be partially offset
 * by keeping a MRU cache of shine tables for various shine values.
 */
static void
compute_shine_table( struct gl_shine_tab *tab, GLfloat shininess )
{
   GLint i;
   GLfloat *m = tab->tab;

   m[0] = 0.0;
   if (shininess == 0.0) {
      for (i = 1 ; i <= SHINE_TABLE_SIZE ; i++)
	 m[i] = 1.0;
   }
   else {
      for (i = 1 ; i < SHINE_TABLE_SIZE ; i++) {
	 GLdouble t = pow(i / (GLfloat) (SHINE_TABLE_SIZE - 1), shininess);
	 if (t > 1e-20)
            m[i] = t;
         else
            m[i] = 0.0;
      }      
      m[SHINE_TABLE_SIZE] = 1.0;
   }

   tab->shininess = shininess;
}


void
gl_compute_shine_table( GLcontext *ctx, GLuint i, GLfloat shininess )
{
#define DISTSQR(a,b) ((a-b)*(a-b))
   struct gl_shine_tab *list = ctx->_ShineTabList;
   struct gl_shine_tab *s;

   foreach(s, list) 
      if ( DISTSQR(s->shininess, shininess) < 1e-4 ) 
	 break;

   if (s == list) {
      foreach(s, list) 
	 if (s->refcount == 0)
            break;

      compute_shine_table( s, shininess );
   }

   ctx->_ShineTable[i]->refcount--;
   ctx->_ShineTable[i] = s;
   move_to_tail( list, s );
   s->refcount++;
#undef DISTSQR
}




/*
 * Examine current lighting parameters to determine if the optimized lighting
 * function can be used.
 * Also, precompute some lighting values such as the products of light
 * source and material ambient, diffuse and specular coefficients.
 */
void
gl_update_lighting( GLcontext *ctx )
{
   struct gl_light *light;
   ctx->_TriangleCaps &= ~(DD_TRI_LIGHT_TWOSIDE|DD_LIGHTING_CULL);
   ctx->_NeedEyeCoords &= ~NEED_EYE_LIGHT;
   ctx->_NeedNormals &= ~NEED_NORMALS_LIGHT;
   ctx->Light._Flags = 0;
   
   if (!ctx->Light.Enabled) 
      return;

   ctx->_NeedNormals |= NEED_NORMALS_LIGHT;

   if (ctx->Light.Model.TwoSide)
      ctx->_TriangleCaps |= (DD_TRI_LIGHT_TWOSIDE|DD_LIGHTING_CULL);

   foreach(light, &ctx->Light.EnabledList) {
      ctx->Light._Flags |= light->_Flags;
   }

   ctx->Light._NeedVertices = 
      ((ctx->Light._Flags & (LIGHT_POSITIONAL|LIGHT_SPOT)) ||
       ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR ||
       ctx->Light.Model.LocalViewer);
   
   if ((ctx->Light._Flags & LIGHT_POSITIONAL) || 
       ctx->Light.Model.LocalViewer) 
      ctx->_NeedEyeCoords |= NEED_EYE_LIGHT;
      
   
   /* XXX: This test is overkill & needs to be fixed both for software and
    * hardware t&l drivers.  The above should be sufficient & should
    * be tested to verify this.  
    */
   if (ctx->Light._NeedVertices)
      ctx->_NeedEyeCoords |= NEED_EYE_LIGHT;
  

   /* Precompute some shading values.
    */
   if (ctx->Visual.RGBAflag) {
      GLuint sides = ctx->Light.Model.TwoSide ? 2 : 1;
      GLuint side;
      for (side=0; side < sides; side++) {
	 struct gl_material *mat = &ctx->Light.Material[side];

	 COPY_3V(ctx->Light._BaseColor[side], mat->Emission);
	 ACC_SCALE_3V(ctx->Light._BaseColor[side], 
		      ctx->Light.Model.Ambient,
		      mat->Ambient);

	 FLOAT_COLOR_TO_CHAN(ctx->Light._BaseAlpha[side],
                             ctx->Light.Material[side].Diffuse[3] );
      }
      
      foreach (light, &ctx->Light.EnabledList) {	 
	 for (side=0; side< sides; side++) {
	    const struct gl_material *mat = &ctx->Light.Material[side];
	    SCALE_3V( light->_MatDiffuse[side], light->Diffuse, mat->Diffuse );
	    SCALE_3V( light->_MatAmbient[side], light->Ambient, mat->Ambient );
	    SCALE_3V( light->_MatSpecular[side], light->Specular,
		      mat->Specular);
	    light->_IsMatSpecular[side] = 
	       (LEN_SQUARED_3FV(light->_MatSpecular[side]) > 1e-16);
	 }
      }
   } 
   else {
      static const GLfloat ci[3] = { .30, .59, .11 };
      foreach(light, &ctx->Light.EnabledList) {
	 light->_dli = DOT3(ci, light->Diffuse);
	 light->_sli = DOT3(ci, light->Specular);
      }
   }

   gl_update_lighting_function(ctx);
}


/* _NEW_MODELVIEW
 * _NEW_LIGHT
 * _TNL_NEW_NEED_EYE_COORDS
 *
 * Update on (_NEW_MODELVIEW | _NEW_LIGHT) when lighting is enabled.
 * Also update on lighting space changes.
 */
void
gl_compute_light_positions( GLcontext *ctx )
{
   struct gl_light *light;
   static const GLfloat eye_z[3] = { 0, 0, 1 };

   if (!ctx->Light.Enabled)
      return;

   if (ctx->_NeedEyeCoords) {
      COPY_3V( ctx->_EyeZDir, eye_z );
   }
   else {
      TRANSFORM_NORMAL( ctx->_EyeZDir, eye_z, ctx->ModelView.m );
   }
   
   foreach (light, &ctx->Light.EnabledList) {

      if (ctx->_NeedEyeCoords) {
	 COPY_4FV( light->_Position, light->EyePosition );
      }
      else {
	 TRANSFORM_POINT( light->_Position, ctx->ModelView.inv, 
			  light->EyePosition );
      }

      if (!(light->_Flags & LIGHT_POSITIONAL)) {
	 /* VP (VP) = Normalize( Position ) */
	 COPY_3V( light->_VP_inf_norm, light->_Position );
	 NORMALIZE_3FV( light->_VP_inf_norm );

	 if (!ctx->Light.Model.LocalViewer) {
	    /* _h_inf_norm = Normalize( V_to_P + <0,0,1> ) */
	    ADD_3V( light->_h_inf_norm, light->_VP_inf_norm, ctx->_EyeZDir);
	    NORMALIZE_3FV( light->_h_inf_norm );
	 }
	 light->_VP_inf_spot_attenuation = 1.0;
      }
      
      if (light->_Flags & LIGHT_SPOT) {
	 if (ctx->_NeedEyeCoords) {
	    COPY_3V( light->_NormDirection, light->EyeDirection );
	 }
         else {
	    TRANSFORM_NORMAL( light->_NormDirection, 
			      light->EyeDirection,
			      ctx->ModelView.m);
	 }

	 NORMALIZE_3FV( light->_NormDirection );

	 if (!(light->_Flags & LIGHT_POSITIONAL)) {
	    GLfloat PV_dot_dir = - DOT3(light->_VP_inf_norm, 
					light->_NormDirection);

	    if (PV_dot_dir > light->_CosCutoff) {
	       double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
	       int k = (int) x;
	       light->_VP_inf_spot_attenuation = 
		  (light->_SpotExpTable[k][0] + 
		   (x-k)*light->_SpotExpTable[k][1]);
	    }
	    else {
	       light->_VP_inf_spot_attenuation = 0;
            }
	 }
      }
   }
}


/* _NEW_TRANSFORM
 * _NEW_MODELVIEW
 * _TNL_NEW_NEED_NORMALS    
 * _TNL_NEW_NEED_EYE_COORDS
 *
 * Update on (_NEW_TRANSFORM|_NEW_MODELVIEW)
 * And also on NewLightingSpaces() callback.
 */
void
gl_update_normal_transform( GLcontext *ctx )
{

   if (!ctx->_NeedNormals) {
      ctx->_NormalTransform = 0;
      return;
   }

   if (ctx->_NeedEyeCoords) {
      GLuint transform = NORM_TRANSFORM_NO_ROT;

      if (ctx->ModelView.flags & (MAT_FLAG_GENERAL |
				  MAT_FLAG_ROTATION |
				  MAT_FLAG_GENERAL_3D |
				  MAT_FLAG_PERSPECTIVE)) 
	 transform = NORM_TRANSFORM;
	    
	       
      if (ctx->Transform.Normalize) {
	 ctx->_NormalTransform = gl_normal_tab[transform | NORM_NORMALIZE];
      } 
      else if (ctx->Transform.RescaleNormals &&
	       ctx->_ModelViewInvScale != 1.0) {
	 ctx->_NormalTransform = gl_normal_tab[transform | NORM_RESCALE];
      }
      else {
	 ctx->_NormalTransform = gl_normal_tab[transform];
      }
   }
   else {
      if (ctx->Transform.Normalize) {
	 ctx->_NormalTransform = gl_normal_tab[NORM_NORMALIZE];
      }
      else if (!ctx->Transform.RescaleNormals &&
	       ctx->_ModelViewInvScale != 1.0) {
	 ctx->_NormalTransform = gl_normal_tab[NORM_RESCALE];
      }
      else {
	 ctx->_NormalTransform = 0;
      }
   }
}
