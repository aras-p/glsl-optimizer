/* $Id: api_eval.c,v 1.1 2001/06/04 13:57:35 keithw Exp $ */

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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "api_eval.h"
#include "context.h"
#include "macros.h"
#include "mmath.h"
#include "math/m_eval.h"

static void do_EvalCoord1f(GLcontext* ctx, GLfloat u)
{

   /** Color Index **/
   if (ctx->Eval.Map1Index) 
   {
      GLfloat findex;
      struct gl_1d_map *map = &ctx->EvalMap.Map1Index;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, &findex, uu, 1, map->Order);
      glIndexi( (GLint) findex );
   }

   /** Color **/
   if (ctx->Eval.Map1Color4) {
      GLfloat fcolor[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Color4;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, fcolor, uu, 4, map->Order);
      glColor4fv( fcolor );
   }

   /** Normal Vector **/
   if (ctx->Eval.Map1Normal) {
      GLfloat normal[3];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Normal;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, normal, uu, 3, map->Order);
      glNormal3fv( normal );
   }

   /** Texture Coordinates **/
   if (ctx->Eval.Map1TextureCoord4) {
      GLfloat texcoord[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Texture4;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, texcoord, uu, 4, map->Order);
      glTexCoord4fv( texcoord );
   }
   else if (ctx->Eval.Map1TextureCoord3) {
      GLfloat texcoord[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Texture3;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, texcoord, uu, 3, map->Order);
      glTexCoord3fv( texcoord );
   }
   else if (ctx->Eval.Map1TextureCoord2) {
      GLfloat texcoord[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Texture2;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, texcoord, uu, 2, map->Order);
      glTexCoord2fv( texcoord );
   }
   else if (ctx->Eval.Map1TextureCoord1) {
      GLfloat texcoord[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Texture1;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, texcoord, uu, 1, map->Order);
      glTexCoord1fv( texcoord );
   }
  
   /** Vertex **/
   if (ctx->Eval.Map1Vertex4) 
   {
      GLfloat vertex[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Vertex4;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, vertex, uu, 4, map->Order);
      glVertex4fv( vertex );
   }
   else if (ctx->Eval.Map1Vertex3) 
   {
      GLfloat vertex[4];
      struct gl_1d_map *map = &ctx->EvalMap.Map1Vertex3;
      GLfloat uu = (u - map->u1) * map->du;
      _math_horner_bezier_curve(map->Points, vertex, uu, 3, map->Order);
      glVertex3fv( vertex );
   }
}

#define CROSS_PROD(n, u, v) \
  (n)[0] = (u)[1]*(v)[2] - (u)[2]*(v)[1]; \
  (n)[1] = (u)[2]*(v)[0] - (u)[0]*(v)[2]; \
  (n)[2] = (u)[0]*(v)[1] - (u)[1]*(v)[0]


static void do_EvalCoord2f( GLcontext* ctx, GLfloat u, GLfloat v )
{   
   /** Color Index **/
   if (ctx->Eval.Map2Index) {
      GLfloat findex;
      struct gl_2d_map *map = &ctx->EvalMap.Map2Index;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, &findex, uu, vv, 1,
                         map->Uorder, map->Vorder);
      glIndexi( (GLuint) (GLint) findex );
   }

   /** Color **/
   if (ctx->Eval.Map2Color4) {
      GLfloat fcolor[4];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Color4;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, fcolor, uu, vv, 4,
                         map->Uorder, map->Vorder);
      glColor4fv( fcolor );
   }

   /** Normal **/
   if (ctx->Eval.Map2Normal &&
       (!ctx->Eval.AutoNormal || (!ctx->Eval.Map2Vertex3 && 
				  !ctx->Eval.Map2Vertex4))) {
      GLfloat normal[3];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Normal;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, normal, uu, vv, 3,
			 map->Uorder, map->Vorder);
      glNormal3fv( normal );
   }

   /** Texture Coordinates **/
   if (ctx->Eval.Map2TextureCoord4) {
      GLfloat texcoord[4];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture4;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, texcoord, uu, vv, 4,
                         map->Uorder, map->Vorder);
      glTexCoord4fv( texcoord );
   }
   else if (ctx->Eval.Map2TextureCoord3) {
      GLfloat texcoord[4];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture3;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, texcoord, uu, vv, 3,
                         map->Uorder, map->Vorder);
      glTexCoord3fv( texcoord );
   }
   else if (ctx->Eval.Map2TextureCoord2) {
      GLfloat texcoord[4];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture2;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, texcoord, uu, vv, 2,
                         map->Uorder, map->Vorder);
      glTexCoord2fv( texcoord );
   }
   else if (ctx->Eval.Map2TextureCoord1) {
      GLfloat texcoord[4];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Texture1;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      _math_horner_bezier_surf(map->Points, texcoord, uu, vv, 1,
                         map->Uorder, map->Vorder);
      glTexCoord1fv( texcoord );
   }

   /** Vertex **/
   if(ctx->Eval.Map2Vertex4) {
      GLfloat vertex[4];
      GLfloat normal[3];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Vertex4;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;

      if (ctx->Eval.AutoNormal) {
         GLfloat du[4], dv[4];

         _math_de_casteljau_surf(map->Points, vertex, du, dv, uu, vv, 4,
				 map->Uorder, map->Vorder);

         CROSS_PROD(normal, du, dv);
         NORMALIZE_3FV(normal);
      }
      else {
         _math_horner_bezier_surf(map->Points, vertex, uu, vv, 4,
                            map->Uorder, map->Vorder);
      }
   }
   else if (ctx->Eval.Map2Vertex3) {
      GLfloat vertex[4];
      struct gl_2d_map *map = &ctx->EvalMap.Map2Vertex3;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      if (ctx->Eval.AutoNormal) {
         GLfloat du[3], dv[3];
	 GLfloat normal[3];
         _math_de_casteljau_surf(map->Points, vertex, du, dv, uu, vv, 3,
				 map->Uorder, map->Vorder);
         CROSS_PROD(normal, du, dv);
         NORMALIZE_3FV(normal);
	 glNormal3fv( normal );
	 glVertex3fv( vertex );
      }
      else {
         _math_horner_bezier_surf(map->Points, vertex, uu, vv, 3,
                            map->Uorder, map->Vorder);
	 glVertex3fv( vertex );
      }
   }
}


void _mesa_EvalPoint1( GLint i )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1) /
		 (GLfloat) ctx->Eval.MapGrid1un);
   GLfloat u = i * du + ctx->Eval.MapGrid1u1;

   glEvalCoord1f( u );
}


void _mesa_EvalPoint2( GLint i, GLint j )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1) / 
		 (GLfloat) ctx->Eval.MapGrid2un);
   GLfloat dv = ((ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1) / 
		 (GLfloat) ctx->Eval.MapGrid2vn);
   GLfloat u = i * du + ctx->Eval.MapGrid2u1;
   GLfloat v = j * dv + ctx->Eval.MapGrid2v1;

   glEvalCoord2f( u, v );
}

/* Wierd thing about eval is that it doesn't affect 'current' values.
 * This technique of saving and resetting current values requires
 * that: 
 *
 *   1) Current values are updated immediately in the glColor,
 *   etc. functions.
 *
 *   2) Hardware color values are stored seperately from ctx->Current, 
 *      for example in dma buffers, or direct emit to registers.
 */
void _mesa_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat normal[3], texcoord[4], color[4];
   GLuint index;

   COPY_3FV( normal, ctx->Current.Normal );
   COPY_4FV( texcoord, ctx->Current.Texcoord[0] );
   COPY_4FV( color, ctx->Current.Color );
   index = ctx->Current.Index;

   do_EvalCoord1f( ctx, u );

   COPY_3FV( ctx->Current.Normal, normal );
   COPY_4FV( ctx->Current.Texcoord[0], texcoord );
   COPY_4FV( ctx->Current.Color, color );
   ctx->Current.Index = index;
}

void _mesa_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat normal[3], texcoord[4], color[4];
   GLuint index;

   COPY_3FV( normal, ctx->Current.Normal );
   COPY_4FV( texcoord, ctx->Current.Texcoord[0] );
   COPY_4FV( color, ctx->Current.Color );
   index = ctx->Current.Index;

   do_EvalCoord2f( ctx, u, v );

   COPY_3FV( ctx->Current.Normal, normal );
   COPY_4FV( ctx->Current.Texcoord[0], texcoord );
   COPY_4FV( ctx->Current.Color, color );
   ctx->Current.Index = index;
}

void _mesa_EvalCoord1fv( const GLfloat *u )
{
   glEvalCoord1f( u[0] );
}

void _mesa_EvalCoord2fv( const GLfloat *u )
{
   glEvalCoord2f( u[0], u[1] );
}
