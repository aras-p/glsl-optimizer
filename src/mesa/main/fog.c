/* $Id: fog.c,v 1.3 1999/11/08 07:36:44 brianp Exp $ */

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


/* $XFree86: xc/lib/GL/mesa/src/fog.c,v 1.4 1999/04/04 00:20:24 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <math.h>
#include <stdlib.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "fog.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#endif



void gl_Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   GLenum m;

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
#if 0
         /* Prior to OpenGL 1.1, this was an error */
         if (*params<0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glFog(GL_FOG_START)" );
            return;
         }
#endif
	 ctx->Fog.Start = *params;
	 break;
      case GL_FOG_END:
#if 0
         /* Prior to OpenGL 1.1, this was an error */
         if (*params<0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glFog(GL_FOG_END)" );
            return;
         }
#endif
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
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glFog" );
         return;
   }

   if (ctx->Driver.Fogfv) {
      (*ctx->Driver.Fogfv)( ctx, pname, params );
   }

   ctx->NewState |= NEW_FOG;
}


typedef void (*fog_func)( struct vertex_buffer *VB, GLuint side, 
			  GLubyte flag );


static fog_func fog_ci_tab[2];
static fog_func fog_rgba_tab[2];

/*
 * Compute the fogged color for an array of vertices.
 * Input:  n - number of vertices
 *         v - array of vertices
 *         color - the original vertex colors
 * Output:  color - the fogged colors
 * 
 */
#define TAG(x) x##_raw
#define CULLCHECK
#define IDX 0
#include "fog_tmp.h"

#define TAG(x) x##_masked
#define CULLCHECK if (cullmask[i]&flag)
#define IDX 1
#include "fog_tmp.h"

void gl_init_fog( void )
{
   init_fog_tab_masked();
   init_fog_tab_raw();
}

/*
 * Compute fog for the vertices in the vertex buffer.
 */
void gl_fog_vertices( struct vertex_buffer *VB )
{
   GLcontext *ctx = VB->ctx;
   GLuint i = VB->CullMode & 1;

   if (ctx->Visual->RGBAflag) {
      /* Fog RGB colors */
      if (ctx->TriangleCaps & DD_TRI_LIGHT_TWOSIDE) {
	 fog_rgba_tab[i]( VB, 0, VERT_FACE_FRONT );
	 fog_rgba_tab[i]( VB, 1, VERT_FACE_REAR );
      } else {
	 fog_rgba_tab[i]( VB, 0, VERT_FACE_FRONT|VERT_FACE_REAR );
      }
   }
   else {
      /* Fog color indexes */
      if (ctx->TriangleCaps & DD_TRI_LIGHT_TWOSIDE) {
	 fog_ci_tab[i]( VB, 0, VERT_FACE_FRONT );
         fog_ci_tab[i]( VB, 1, VERT_FACE_REAR );
      } else {
	 fog_ci_tab[i]( VB, 0, VERT_FACE_FRONT|VERT_FACE_REAR );
      }
   }
}

/*
 * Apply fog to an array of RGBA pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         red, green, blue, alpha - pixel colors
 * Output:  red, green, blue, alpha - fogged pixel colors
 */
void gl_fog_rgba_pixels( const GLcontext *ctx,
                         GLuint n, const GLdepth z[], GLubyte rgba[][4] )
{
   GLfloat c = ctx->ProjectionMatrix.m[10];
   GLfloat d = ctx->ProjectionMatrix.m[14];
   GLuint i;

   GLfloat rFog = ctx->Fog.Color[0] * 255.0F;
   GLfloat gFog = ctx->Fog.Color[1] * 255.0F;
   GLfloat bFog = ctx->Fog.Color[2] * 255.0F;

   GLfloat tz = ctx->Viewport.WindowMap.m[MAT_TZ];
   GLfloat szInv = 1.0F / ctx->Viewport.WindowMap.m[MAT_SZ];

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f, g;
               if (eyez < 0.0)  eyez = -eyez;
               f = (fogEnd - eyez) * fogScale;
               f = CLAMP( f, 0.0F, 1.0F );
               g = 1.0F - f;
               rgba[i][RCOMP] = (GLint) (f * rgba[i][RCOMP] + g * rFog);
               rgba[i][GCOMP] = (GLint) (f * rgba[i][GCOMP] + g * gFog);
               rgba[i][BCOMP] = (GLint) (f * rgba[i][BCOMP] + g * bFog);
            }
         }
	 break;
      case GL_EXP:
	 for (i=0;i<n;i++) {
	    GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
	    GLfloat eyez = d / (c+ndcz);
            GLfloat f, g;
	    if (eyez < 0.0)
               eyez = -eyez;
	    f = exp( -ctx->Fog.Density * eyez );
            g = 1.0F - f;
            rgba[i][RCOMP] = (GLint) (f * rgba[i][RCOMP] + g * rFog);
            rgba[i][GCOMP] = (GLint) (f * rgba[i][GCOMP] + g * gFog);
            rgba[i][BCOMP] = (GLint) (f * rgba[i][BCOMP] + g * bFog);
	 }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = d / (c+ndcz);
               GLfloat f, g;
               GLfloat tmp = negDensitySquared * eyez * eyez;
#ifdef __alpha__
               /* XXX this underflow check may be needed for other systems */
               if (tmp < FLT_MIN_10_EXP)
                  f = exp( FLT_MIN_10_EXP );
               else
#endif
                  f = exp( tmp );
               g = 1.0F - f;
               rgba[i][RCOMP] = (GLint) (f * rgba[i][RCOMP] + g * rFog);
               rgba[i][GCOMP] = (GLint) (f * rgba[i][GCOMP] + g * gFog);
               rgba[i][BCOMP] = (GLint) (f * rgba[i][BCOMP] + g * bFog);
            }
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_rgba_pixels");
         return;
   }
}




/*
 * Apply fog to an array of color index pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         index - pixel color indexes
 * Output:  index - fogged pixel color indexes
 */
void gl_fog_ci_pixels( const GLcontext *ctx,
                       GLuint n, const GLdepth z[], GLuint index[] )
{
   GLfloat c = ctx->ProjectionMatrix.m[10];
   GLfloat d = ctx->ProjectionMatrix.m[14];
   GLuint i;

   GLfloat tz = ctx->Viewport.WindowMap.m[MAT_TZ];
   GLfloat szInv = 1.0F / ctx->Viewport.WindowMap.m[MAT_SZ];

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f;
               if (eyez < 0.0)  eyez = -eyez;
               f = (fogEnd - eyez) * fogScale;
               f = CLAMP( f, 0.0F, 1.0F );
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
            }
	 }
	 break;
      case GL_EXP:
         for (i=0;i<n;i++) {
	    GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
	    GLfloat eyez = -d / (c+ndcz);
            GLfloat f;
	    if (eyez < 0.0)
               eyez = -eyez;
	    f = exp( -ctx->Fog.Density * eyez );
	    f = CLAMP( f, 0.0F, 1.0F );
	    index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
	 }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat tmp, f;
               if (eyez < 0.0)
                  eyez = -eyez;
               tmp = negDensitySquared * eyez * eyez;
#ifdef __alpha__
               /* XXX this underflow check may be needed for other systems */
               if (tmp < FLT_MIN_10_EXP)
                  f = exp( FLT_MIN_10_EXP );
               else
#endif
               f = exp( tmp );
               f = CLAMP( f, 0.0F, 1.0F );
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
            }
	 }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_ci_pixels");
         return;
   }
}

