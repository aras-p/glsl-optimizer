/* $Id: fog.c,v 1.35 2001/05/30 14:43:17 brianp Exp $ */

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
#include "colormac.h"
#include "context.h"
#include "fog.h"
#include "mtypes.h"
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
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_FOG_MODE:
         m = (GLenum) (GLint) *params;
	 switch (m) {
	 case GL_LINEAR:
	 case GL_EXP:
	 case GL_EXP2:
	    break;
	 default:
	    _mesa_error( ctx, GL_INVALID_ENUM, "glFog" );
            return;
	 }
	 if (ctx->Fog.Mode == m)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Mode = m;
	 break;
      case GL_FOG_DENSITY:
	 if (*params<0.0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glFog" );
            return;
	 }
	 if (ctx->Fog.Density == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Density = *params;
	 break;
      case GL_FOG_START:
	 if (ctx->Fog.Start == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Start = *params;
	 break;
      case GL_FOG_END:
	 if (ctx->Fog.End == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.End = *params;
	 break;
      case GL_FOG_INDEX:
 	 if (ctx->Fog.Index == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
 	 ctx->Fog.Index = *params;
	 break;
      case GL_FOG_COLOR:
	 if (TEST_EQ_4V(ctx->Fog.Color, params))
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Color[0] = params[0];
	 ctx->Fog.Color[1] = params[1];
	 ctx->Fog.Color[2] = params[2];
	 ctx->Fog.Color[3] = params[3];
         break;
      case GL_FOG_COORDINATE_SOURCE_EXT: {
	 GLenum p = (GLenum) (GLint) *params;
         if (!ctx->Extensions.EXT_fog_coord ||
             (p != GL_FOG_COORDINATE_EXT && p != GL_FRAGMENT_DEPTH_EXT)) {
	    _mesa_error(ctx, GL_INVALID_ENUM, "glFog");
	    return;
	 }
	 if (ctx->Fog.FogCoordinateSource == p)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.FogCoordinateSource = p;
	 break;
      }
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glFog" );
         return;
   }

   if (ctx->Driver.Fogfv) {
      (*ctx->Driver.Fogfv)( ctx, pname, params );
   }
}
