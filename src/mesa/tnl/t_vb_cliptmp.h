/* $Id: t_vb_cliptmp.h,v 1.12 2001/05/09 12:25:40 keithw Exp $ */

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


#define CLIP_DOTPROD(K, A, B, C, D) X(K)*A + Y(K)*B + Z(K)*C + W(K)*D

#define POLY_CLIP( PLANE, A, B, C, D )					\
do {									\
   if (mask & PLANE) {							\
      GLuint idxPrev = inlist[0];					\
      GLfloat dpPrev = CLIP_DOTPROD(idxPrev, A, B, C, D );		\
      GLuint outcount = 0;						\
      GLuint i;								\
									\
      inlist[n] = inlist[0]; /* prevent rotation of vertices */		\
      for (i = 1; i <= n; i++) {					\
	 GLuint idx = inlist[i];					\
	 GLfloat dp = CLIP_DOTPROD(idx, A, B, C, D );			\
									\
         clipmask[idxPrev] |= PLANE;					\
	 if (!NEGATIVE(dpPrev)) {					\
	    outlist[outcount++] = idxPrev;				\
 	    clipmask[idxPrev] &= ~PLANE;				\
	 }								\
									\
	 if (DIFFERENT_SIGNS(dp, dpPrev)) {				\
            GLuint newvert = VB->LastClipped++;				\
            VB->ClipMask[newvert] = 0;					\
            outlist[outcount++] = newvert;				\
	    if (NEGATIVE(dp)) {						\
	       /* Going out of bounds.  Avoid division by zero as we	\
		* know dp != dpPrev from DIFFERENT_SIGNS, above.	\
		*/							\
	       GLfloat t = dp / (dp - dpPrev);				\
               INTERP_4F( t, coord[newvert], coord[idx], coord[idxPrev]); \
      	       interp( ctx, t, newvert, idx, idxPrev, GL_TRUE );	\
	    } else {							\
	       /* Coming back in.					\
		*/							\
	       GLfloat t = dpPrev / (dpPrev - dp);			\
               INTERP_4F( t, coord[newvert], coord[idxPrev], coord[idx]); \
	       interp( ctx, t, newvert, idxPrev, idx, GL_FALSE );	\
	    }								\
	 }								\
									\
	 idxPrev = idx;							\
	 dpPrev = dp;							\
      }									\
									\
      if (outcount < 3)							\
	 return;							\
									\
      {									\
	 GLuint *tmp = inlist;						\
	 inlist = outlist;						\
	 outlist = tmp;							\
	 n = outcount;							\
      }									\
   }									\
} while (0)


#define LINE_CLIP(PLANE, A, B, C, D )					\
do {									\
   if (mask & PLANE) {							\
      GLfloat dpI = CLIP_DOTPROD( ii, A, B, C, D );			\
      GLfloat dpJ = CLIP_DOTPROD( jj, A, B, C, D );			\
									\
      if (DIFFERENT_SIGNS(dpI, dpJ)) {					\
         GLuint newvert = VB->LastClipped++;				\
         VB->ClipMask[newvert] = 0;					\
	 if (NEGATIVE(dpJ)) {						\
	    GLfloat t = dpI / (dpI - dpJ);				\
            VB->ClipMask[jj] |= PLANE;					\
            INTERP_4F( t, coord[newvert], coord[ii], coord[jj] );	\
	    interp( ctx, t, newvert, ii, jj, GL_FALSE );		\
            jj = newvert;						\
	 } else {							\
  	    GLfloat t = dpJ / (dpJ - dpI);				\
            VB->ClipMask[ii] |= PLANE;					\
            INTERP_4F( t, coord[newvert], coord[jj], coord[ii] );	\
	    interp( ctx, t, newvert, jj, ii, GL_FALSE );		\
            ii = newvert;						\
	 }								\
      }									\
      else if (NEGATIVE(dpI))						\
	 return;							\
  }									\
} while (0)



/* Clip a line against the viewport and user clip planes.
 */
static __inline void TAG(clip_line)( GLcontext *ctx,
				     GLuint i, GLuint j,
				     GLubyte mask )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   interp_func interp = tnl->Driver.RenderInterp;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint ii = i, jj = j, p;

   VB->LastClipped = VB->FirstClipped;

   if (mask & 0x3f) {
      LINE_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
      LINE_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
      LINE_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
      LINE_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
      LINE_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
      LINE_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );
   }

   if (mask & CLIP_USER_BIT) {
      for (p=0;p<MAX_CLIP_PLANES;p++) {
	 if (ctx->Transform.ClipEnabled[p]) {
	    GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	    GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	    GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	    GLfloat d = ctx->Transform._ClipUserPlane[p][3];
	    LINE_CLIP( CLIP_USER_BIT, a, b, c, d );
	 }
      }
   }

   if ((ctx->_TriangleCaps & DD_FLATSHADE) && j != jj)
      tnl->Driver.RenderCopyPV( ctx, jj, j );

   tnl->Driver.RenderClippedLine( ctx, ii, jj );
}


/* Clip a triangle against the viewport and user clip planes.
 */
static __inline void TAG(clip_tri)( GLcontext *ctx,
				    GLuint v0, GLuint v1, GLuint v2,
				    GLubyte mask )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   interp_func interp = tnl->Driver.RenderInterp;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint pv = v2;
   GLuint vlist[2][MAX_CLIPPED_VERTICES];
   GLuint *inlist = vlist[0], *outlist = vlist[1];
   GLuint p;
   GLubyte *clipmask = VB->ClipMask;
   GLuint n = 3;

   ASSIGN_3V(inlist, v2, v0, v1 ); /* pv rotated to slot zero */

   VB->LastClipped = VB->FirstClipped;

   if (mask & 0x3f) {
      POLY_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
      POLY_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
      POLY_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
      POLY_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
      POLY_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
      POLY_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );
   }

   if (mask & CLIP_USER_BIT) {
      for (p=0;p<MAX_CLIP_PLANES;p++) {
	 if (ctx->Transform.ClipEnabled[p]) {
	    GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	    GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	    GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	    GLfloat d = ctx->Transform._ClipUserPlane[p][3];
	    POLY_CLIP( CLIP_USER_BIT, a, b, c, d );
	 }
      }
   }

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      if (pv != inlist[0]) {
	 ASSERT( inlist[0] >= VB->FirstClipped );
	 tnl->Driver.RenderCopyPV( ctx, inlist[0], pv );
      }
   }

   tnl->Driver.RenderClippedPolygon( ctx, inlist, n );
}


/* Clip a quad against the viewport and user clip planes.
 */
static __inline void TAG(clip_quad)( GLcontext *ctx,
				     GLuint v0, GLuint v1, GLuint v2, GLuint v3,
				     GLubyte mask )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   interp_func interp = tnl->Driver.RenderInterp;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint pv = v3;
   GLuint vlist[2][MAX_CLIPPED_VERTICES];
   GLuint *inlist = vlist[0], *outlist = vlist[1];
   GLuint p;
   GLubyte *clipmask = VB->ClipMask;
   GLuint n = 4;

   ASSIGN_4V(inlist, v3, v0, v1, v2 ); /* pv rotated to slot zero */

   VB->LastClipped = VB->FirstClipped;

   if (mask & 0x3f) {
      POLY_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
      POLY_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
      POLY_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
      POLY_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
      POLY_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
      POLY_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );
   }

   if (mask & CLIP_USER_BIT) {
      for (p=0;p<MAX_CLIP_PLANES;p++) {
	 if (ctx->Transform.ClipEnabled[p]) {
	    GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	    GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	    GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	    GLfloat d = ctx->Transform._ClipUserPlane[p][3];
	    POLY_CLIP( CLIP_USER_BIT, a, b, c, d );
	 }
      }
   }

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      if (pv != inlist[0]) {
	 ASSERT( inlist[0] >= VB->FirstClipped );
	 tnl->Driver.RenderCopyPV( ctx, inlist[0], pv );
      }
   }

   tnl->Driver.RenderClippedPolygon( ctx, inlist, n );
}

#undef W
#undef Z
#undef Y
#undef X
#undef SIZE
#undef TAG
#undef POLY_CLIP
#undef LINE_CLIP
