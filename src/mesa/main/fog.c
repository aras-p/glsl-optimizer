/* $Id: fog.c,v 1.27 2000/10/31 18:09:44 keithw Exp $ */

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
#include "fog.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#include "xform.h"
#endif



void
_mesa_Fogf(GLenum pname, GLfloat param)
{
   _mesa_Fogfv(pname, &param);
}


void
_mesa_Fogi(GLenum pname, GLint param )
{
   GLfloat fparam = (GLfloat) param;
   _mesa_Fogfv(pname, &fparam);
}


void
_mesa_Fogiv(GLenum pname, const GLint *params )
{
   GLfloat p[4];
   switch (pname) {
      case GL_FOG_MODE:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_INDEX:
      case GL_FOG_COORDINATE_SOURCE_EXT:
	 p[0] = (GLfloat) *params;
	 break;
      case GL_FOG_COLOR:
	 p[0] = INT_TO_FLOAT( params[0] );
	 p[1] = INT_TO_FLOAT( params[1] );
	 p[2] = INT_TO_FLOAT( params[2] );
	 p[3] = INT_TO_FLOAT( params[3] );
	 break;
      default:
         /* Error will be caught later in _mesa_Fogfv */
         ;
   }
   _mesa_Fogfv(pname, p);
}


void 
_mesa_Fogfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLenum m;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glFog");

   switch (pname) {
      case GL_FOG_MODE:
         m = (GLenum) (GLint) *params;
	 if (m==GL_LINEAR || m==GL_EXP || m==GL_EXP2) {
	    ctx->Fog.Mode = m;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glFog" );
            return;
	 }
	 break;
      case GL_FOG_DENSITY:
	 if (*params<0.0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glFog" );
            return;
	 }
	 else {
	    ctx->Fog.Density = *params;
	 }
	 break;
      case GL_FOG_START:
	 ctx->Fog.Start = *params;
	 break;
      case GL_FOG_END:
	 ctx->Fog.End = *params;
	 break;
      case GL_FOG_INDEX:
	 ctx->Fog.Index = *params;
	 break;
      case GL_FOG_COLOR:
	 ctx->Fog.Color[0] = params[0];
	 ctx->Fog.Color[1] = params[1];
	 ctx->Fog.Color[2] = params[2];
	 ctx->Fog.Color[3] = params[3];
         break;
      case GL_FOG_COORDINATE_SOURCE_EXT: {
	 GLenum p = (GLenum)(GLint) *params;
	 if (p == GL_FOG_COORDINATE_EXT || p == GL_FRAGMENT_DEPTH_EXT)
	    ctx->Fog.FogCoordinateSource = p;
	 else
	    gl_error( ctx, GL_INVALID_ENUM, "glFog" );
	 break;
      }
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glFog" );
         return;
   }

   if (ctx->Driver.Fogfv) {
      (*ctx->Driver.Fogfv)( ctx, pname, params );
   }

   ctx->NewState |= _NEW_FOG;
}





static GLvector1f *get_fogcoord_ptr( GLcontext *ctx, GLvector1f *tmp )
{
   struct vertex_buffer *VB = ctx->VB;

   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT) {      
      if (!ctx->NeedEyeCoords) {
	 GLfloat *m = ctx->ModelView.m;
	 GLfloat plane[4];

	 plane[0] = m[2];
	 plane[1] = m[6];
	 plane[2] = m[10];
	 plane[3] = m[14];

	 /* Full eye coords weren't required, just calculate the
	  * eye Z values.  
	  */
	 gl_dotprod_tab[0][VB->ObjPtr->size](&VB->Eye, 2,
					     VB->ObjPtr, plane, 0 );

	 tmp->data = &(VB->Eye.data[0][2]);
	 tmp->start = VB->Eye.start+2;
	 tmp->stride = VB->Eye.stride;
	 return tmp;
      }
      else 
      {
	 if (VB->EyePtr->size < 2)
	    gl_vector4f_clean_elem( &VB->Eye, VB->Count, 2 );

	 tmp->data = &(VB->EyePtr->data[0][2]);
	 tmp->start = VB->EyePtr->start+2;
	 tmp->stride = VB->EyePtr->stride;
	 return tmp;
      }
   } else
      return VB->FogCoordPtr;
}


/* Use lookup table & interpolation?
 */
static void 
make_win_fog_coords( struct vertex_buffer *VB,
		     GLvector1f *fogcoord)
{
   const GLcontext *ctx = VB->ctx;
   GLfloat end  = ctx->Fog.End;
   GLfloat *v = fogcoord->start;
   GLuint stride = fogcoord->stride;
   GLuint n = VB->Count - VB->Start;
   GLfloat *out;		
   GLfloat d;
   GLuint i;

   VB->FogCoordPtr = VB->store.FogCoord;
   out = VB->FogCoordPtr->data + VB->Start;

   switch (ctx->Fog.Mode) {
   case GL_LINEAR:
      d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride)) {
	 out[i] = (end - ABSF(*v)) * d;
	 if (0) fprintf(stderr, "z %f out %f\n", *v, out[i]);
      }
      break;
   case GL_EXP:
      d = -ctx->Fog.Density;
      for ( i = 0 ; i < n ; i++, STRIDE_F(v,stride)) {
	 out[i] = exp( d*ABSF(*v) );
	 if (0) fprintf(stderr, "z %f out %f\n", *v, out[i]);
      }
      break;
   case GL_EXP2:
      d = -(ctx->Fog.Density*ctx->Fog.Density);
      for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride)) {
	 GLfloat z = *v;
	 out[i] = exp( d*z*z ); 
	 if (0) fprintf(stderr, "z %f out %f\n", *v, out[i]);
      }
      break;
   default:
      gl_problem(ctx, "Bad fog mode in make_fog_coord");
      return;
   }
}


void 
_mesa_make_win_fog_coords( struct vertex_buffer *VB )
{
   GLvector1f tmp;

   make_win_fog_coords( VB, get_fogcoord_ptr( VB->ctx, &tmp ) );
}

