/* $Id: clip.c,v 1.11 2000/10/28 20:41:13 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "clip.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#endif




#define CLIP_RGBA0    0x1
#define CLIP_RGBA1    0x2
#define CLIP_TEX0     0x4
#define CLIP_TEX1     0x8
#define CLIP_INDEX0   0x10
#define CLIP_INDEX1   0x20
#define CLIP_FOG_COORD 0x40


/* Linear interpolation between A and B: */
#define LINTERP( T, A, B )   ( (A) + (T) * ( (B) - (A) ) )



#define INTERP_SZ( t, vec, to, a, b, sz )			\
do {								\
   switch (sz) {						\
   case 4: vec[to][3] = LINTERP( t, vec[a][3], vec[b][3] );	\
   case 3: vec[to][2] = LINTERP( t, vec[a][2], vec[b][2] );	\
   case 2: vec[to][1] = LINTERP( t, vec[a][1], vec[b][1] );	\
   case 1: vec[to][0] = LINTERP( t, vec[a][0], vec[b][0] );	\
   }								\
} while(0)
   

static clip_interp_func clip_interp_tab[0x80]; 

#define IND 0
#define NAME clip_nil
#include "interp_tmp.h"

#define IND (CLIP_RGBA0)
#define NAME clipRGBA0
#include "interp_tmp.h"

#define IND (CLIP_RGBA0|CLIP_RGBA1)
#define NAME clipRGBA0_RGBA1
#include "interp_tmp.h"

#define IND (CLIP_TEX0|CLIP_RGBA0)
#define NAME clipTEX0_RGBA0
#include "interp_tmp.h"

#define IND (CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1)
#define NAME clipTEX0_RGBA0_RGBA1
#include "interp_tmp.h"

#define IND (CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0)
#define NAME clipTEX1_TEX0_RGBA0
#include "interp_tmp.h"

#define IND (CLIP_TEX0)
#define NAME clipTEX0
#include "interp_tmp.h"

#define IND (CLIP_TEX1|CLIP_TEX0)
#define NAME clipTEX1_TEX0
#include "interp_tmp.h"

#define IND (CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1)
#define NAME clipTEX1_TEX0_RGBA0_RGBA1
#include "interp_tmp.h"

#define IND (CLIP_INDEX0)
#define NAME clipINDEX0
#include "interp_tmp.h"

#define IND (CLIP_INDEX0|CLIP_INDEX1)
#define NAME clipINDEX0_INDEX1
#include "interp_tmp.h"

#define IND (CLIP_FOG_COORD)
#define NAME clip_FOG
#include "interp_tmp.h"

#define IND (CLIP_RGBA0|CLIP_FOG_COORD)
#define NAME clipRGBA0_FOG
#include "interp_tmp.h"

#define IND (CLIP_RGBA0|CLIP_RGBA1|CLIP_FOG_COORD)
#define NAME clipRGBA0_RGBA1_FOG
#include "interp_tmp.h"

#define IND (CLIP_TEX0|CLIP_RGBA0|CLIP_FOG_COORD)
#define NAME clipTEX0_RGBA0_FOG
#include "interp_tmp.h"

#define IND (CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1|CLIP_FOG_COORD)
#define NAME clipTEX0_RGBA0_RGBA1_FOG
#include "interp_tmp.h"

#define IND (CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0|CLIP_FOG_COORD)
#define NAME clipTEX1_TEX0_RGBA0_FOG
#include "interp_tmp.h"

#define IND (CLIP_TEX0|CLIP_FOG_COORD)
#define NAME clipTEX0_FOG
#include "interp_tmp.h"

#define IND (CLIP_TEX1|CLIP_TEX0|CLIP_FOG_COORD)
#define NAME clipTEX1_TEX0_FOG
#include "interp_tmp.h"

#define IND (CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1|CLIP_FOG_COORD)
#define NAME clipTEX1_TEX0_RGBA0_RGBA1_FOG
#include "interp_tmp.h"

#define IND (CLIP_INDEX0|CLIP_FOG_COORD)
#define NAME clipINDEX0_FOG
#include "interp_tmp.h"

#define IND (CLIP_INDEX0|CLIP_INDEX1|CLIP_FOG_COORD)
#define NAME clipINDEX0_INDEX1_FOG
#include "interp_tmp.h"




/**********************************************************************/
/*                     Get/Set User clip-planes.                      */
/**********************************************************************/



void
_mesa_ClipPlane( GLenum plane, const GLdouble *eq )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint p;
   GLfloat equation[4];

   equation[0] = eq[0];
   equation[1] = eq[1];
   equation[2] = eq[2];
   equation[3] = eq[3];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClipPlane");

   p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
   if (p<0 || p>=MAX_CLIP_PLANES) {
      gl_error( ctx, GL_INVALID_ENUM, "glClipPlane" );
      return;
   }

   /*
    * The equation is transformed by the transpose of the inverse of the
    * current modelview matrix and stored in the resulting eye coordinates.
    *
    * KW: Eqn is then transformed to the current clip space, where user
    * clipping now takes place.  The clip-space equations are recalculated
    * whenever the projection matrix changes.
    */
   if (ctx->ModelView.flags & MAT_DIRTY_ALL_OVER) {
      gl_matrix_analyze( &ctx->ModelView );
   }
   gl_transform_vector( ctx->Transform.EyeUserPlane[p], equation,
		        ctx->ModelView.inv );


   if (ctx->Transform.ClipEnabled[p]) {
      ctx->NewState |= NEW_USER_CLIP;

      if (ctx->ProjectionMatrix.flags & MAT_DIRTY_ALL_OVER) {
	 gl_matrix_analyze( &ctx->ProjectionMatrix );
      }
      gl_transform_vector( ctx->Transform.ClipUserPlane[p], 
			   ctx->Transform.EyeUserPlane[p], 
			   ctx->ProjectionMatrix.inv );
   }
}


void gl_update_userclip( GLcontext *ctx )
{
   GLuint p;
   
   for (p = 0 ; p < MAX_CLIP_PLANES ; p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 gl_transform_vector( ctx->Transform.ClipUserPlane[p],
			      ctx->Transform.EyeUserPlane[p],
			      ctx->ProjectionMatrix.inv );
      }
   }
}

void
_mesa_GetClipPlane( GLenum plane, GLdouble *equation )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint p;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetClipPlane");


   p = (GLint) (plane - GL_CLIP_PLANE0);
   if (p<0 || p>=MAX_CLIP_PLANES) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetClipPlane" );
      return;
   }

   equation[0] = (GLdouble) ctx->Transform.EyeUserPlane[p][0];
   equation[1] = (GLdouble) ctx->Transform.EyeUserPlane[p][1];
   equation[2] = (GLdouble) ctx->Transform.EyeUserPlane[p][2];
   equation[3] = (GLdouble) ctx->Transform.EyeUserPlane[p][3];
}




/**********************************************************************/
/*                         View volume clipping.                      */
/**********************************************************************/


/*
 * Clip a point against the view volume.
 * Input:  v - vertex-vector describing the point to clip
 * Return:  0 = outside view volume
 *          1 = inside view volume
 */
GLuint gl_viewclip_point( const GLfloat v[] )
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

/*
 * Clip a point against the user clipping planes.
 * Input:  v - vertex-vector describing the point to clip.
 * Return:  0 = point was clipped
 *          1 = point not clipped
 */
GLuint gl_userclip_point( GLcontext* ctx, const GLfloat v[] )
{
   GLuint p;

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 GLfloat dot = v[0] * ctx->Transform.ClipUserPlane[p][0]
		     + v[1] * ctx->Transform.ClipUserPlane[p][1]
		     + v[2] * ctx->Transform.ClipUserPlane[p][2]
		     + v[3] * ctx->Transform.ClipUserPlane[p][3];
         if (dot < 0.0F) {
            return 0;
         }
      }
   }

   return 1;
}




#if 0
#define NEGATIVE(x) ((*(int *)&x)<0)
#define DIFFERENT_SIGNS(a,b) ((a*b) < 0)
#else
#define NEGATIVE(x) (x < 0)
#define DIFFERENT_SIGNS(a,b) ((a*b) < 0)
#endif


static clip_poly_func gl_poly_clip_tab[2][5];
static clip_line_func gl_line_clip_tab[2][5];

#define W(i) coord[i][3]
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 4
#define IND 0
#define TAG(x) x##_4
#include "clip_funcs.h"

#define W(i) 1.0
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 3
#define IND 0
#define TAG(x) x##_3
#include "clip_funcs.h"

#define W(i) 1.0
#define Z(i) 0.0
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 2
#define IND 0
#define TAG(x) x##_2
#include "clip_funcs.h"

#define W(i) coord[i][3]
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 4
#define IND CLIP_TAB_EDGEFLAG
#define TAG(x) x##_4_edgeflag
#include "clip_funcs.h"

#define W(i) 1.0
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 3
#define IND CLIP_TAB_EDGEFLAG
#define TAG(x) x##_3_edgeflag
#include "clip_funcs.h"

#define W(i) 1.0
#define Z(i) 0.0
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 2
#define IND CLIP_TAB_EDGEFLAG
#define TAG(x) x##_2_edgeflag
#include "clip_funcs.h"




void gl_update_clipmask( GLcontext *ctx )
{
   GLuint mask = 0;

   if (ctx->Visual.RGBAflag) 
   {
      mask |= CLIP_RGBA0;
      
      if (ctx->TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_SEPERATE_SPECULAR))
         mask |= CLIP_RGBA1;

      if (ctx->Texture.ReallyEnabled & 0xf0)
	 mask |= CLIP_TEX1|CLIP_TEX0;

      if (ctx->Texture.ReallyEnabled & 0xf)
	 mask |= CLIP_TEX0;
   }
   else if (ctx->Light.ShadeModel==GL_SMOOTH) 
   {
      mask |= CLIP_INDEX0;
      
      if (ctx->TriangleCaps & DD_TRI_LIGHT_TWOSIDE) 
	 mask |= CLIP_INDEX1;
   }

   if (ctx->Fog.Enabled)
      mask |= CLIP_FOG_COORD;
   
   ctx->ClipInterpFunc = clip_interp_tab[mask];
   ctx->poly_clip_tab = gl_poly_clip_tab[0];
   ctx->line_clip_tab = gl_line_clip_tab[0];

   if (ctx->TriangleCaps & DD_TRI_UNFILLED) {
      ctx->poly_clip_tab = gl_poly_clip_tab[1];
      ctx->line_clip_tab = gl_line_clip_tab[0];
   } 
}


#define USER_CLIPTEST(NAME, SZ)						\
static void NAME( struct vertex_buffer *VB )				\
{									\
   GLcontext *ctx = VB->ctx;						\
   GLubyte *clipMask = VB->ClipMask;					\
   GLubyte *userClipMask = VB->UserClipMask;				\
   GLuint start = VB->Start;						\
   GLuint count = VB->Count;						\
   GLuint p, i;								\
   GLubyte bit;								\
									\
									\
   for (bit = 1, p = 0; p < MAX_CLIP_PLANES ; p++, bit *=2)		\
      if (ctx->Transform.ClipEnabled[p]) {				\
	 GLuint nr = 0;							\
	 const GLfloat a = ctx->Transform.ClipUserPlane[p][0];		\
	 const GLfloat b = ctx->Transform.ClipUserPlane[p][1];		\
	 const GLfloat c = ctx->Transform.ClipUserPlane[p][2];		\
	 const GLfloat d = ctx->Transform.ClipUserPlane[p][3];		\
         GLfloat *coord = VB->ClipPtr->start;				\
         GLuint stride = VB->ClipPtr->stride;				\
									\
	 for (i = start ; i < count ; i++, STRIDE_F(coord, stride)) {	\
	    GLfloat dp = coord[0] * a + coord[1] * b;			\
	    if (SZ > 2) dp += coord[2] * c;				\
	    if (SZ > 3) dp += coord[3] * d; else dp += d;		\
									\
	    if (dp < 0) {						\
	       clipMask[i] |= CLIP_USER_BIT;				\
	       userClipMask[i] |= bit;					\
	       nr++;							\
	    }								\
	 }								\
									\
	 if (nr > 0) {							\
	    VB->ClipOrMask |= CLIP_USER_BIT;				\
	    VB->CullMode |= CLIP_MASK_ACTIVE;				\
	    if (nr == count - start) {					\
	       VB->ClipAndMask |= CLIP_USER_BIT;			\
	       VB->Culled = 1;						\
	       return;							\
	    }								\
	 }								\
      }									\
}


USER_CLIPTEST(userclip2, 2)		       
USER_CLIPTEST(userclip3, 3)		       
USER_CLIPTEST(userclip4, 4)		       

static void (*(usercliptab[5]))( struct vertex_buffer * ) = {
   0,
   0,
   userclip2,
   userclip3,
   userclip4
};

void gl_user_cliptest( struct vertex_buffer *VB )
{
   usercliptab[VB->ClipPtr->size]( VB );
}


void gl_init_clip(void)
{
   init_clip_funcs_4();
   init_clip_funcs_3();
   init_clip_funcs_2();

   init_clip_funcs_4_edgeflag();
   init_clip_funcs_3_edgeflag();
   init_clip_funcs_2_edgeflag();

   clip_interp_tab[0] = clip_nil;
   clip_interp_tab[CLIP_RGBA0] = clipRGBA0;
   clip_interp_tab[CLIP_RGBA0|CLIP_RGBA1] = clipRGBA0_RGBA1;
   clip_interp_tab[CLIP_TEX0|CLIP_RGBA0] = clipTEX0_RGBA0;
   clip_interp_tab[CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1] = clipTEX0_RGBA0_RGBA1;
   clip_interp_tab[CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0] = clipTEX1_TEX0_RGBA0;
   clip_interp_tab[CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1] = 
      clipTEX1_TEX0_RGBA0_RGBA1;
   clip_interp_tab[CLIP_TEX0] = clipTEX0;
   clip_interp_tab[CLIP_TEX1|CLIP_TEX0] = clipTEX1_TEX0;
   clip_interp_tab[CLIP_INDEX0] = clipINDEX0;
   clip_interp_tab[CLIP_INDEX0|CLIP_INDEX1] = clipINDEX0_INDEX1;

   clip_interp_tab[CLIP_FOG_COORD] = clip_FOG;
   clip_interp_tab[CLIP_RGBA0|CLIP_FOG_COORD] = clipRGBA0_FOG;
   clip_interp_tab[CLIP_RGBA0|CLIP_RGBA1|CLIP_FOG_COORD] = clipRGBA0_RGBA1_FOG;
   clip_interp_tab[CLIP_TEX0|CLIP_RGBA0|CLIP_FOG_COORD] = clipTEX0_RGBA0_FOG;
   clip_interp_tab[CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1|CLIP_FOG_COORD] = clipTEX0_RGBA0_RGBA1_FOG;
   clip_interp_tab[CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0|CLIP_FOG_COORD] = clipTEX1_TEX0_RGBA0_FOG;
   clip_interp_tab[CLIP_TEX1|CLIP_TEX0|CLIP_RGBA0|CLIP_RGBA1|CLIP_FOG_COORD] = 
      clipTEX1_TEX0_RGBA0_RGBA1_FOG;
   clip_interp_tab[CLIP_TEX0|CLIP_FOG_COORD] = clipTEX0_FOG;
   clip_interp_tab[CLIP_TEX1|CLIP_TEX0|CLIP_FOG_COORD] = clipTEX1_TEX0_FOG;
   clip_interp_tab[CLIP_INDEX0|CLIP_FOG_COORD] = clipINDEX0_FOG;
   clip_interp_tab[CLIP_INDEX0|CLIP_INDEX1|CLIP_FOG_COORD] = clipINDEX0_INDEX1_FOG;
}

