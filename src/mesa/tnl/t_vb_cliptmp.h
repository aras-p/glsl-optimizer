/* $Id: t_vb_cliptmp.h,v 1.2 2000/12/27 19:57:37 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 *
 * Author:
 *    Keith Whitwell <keithw@valinux.com>
 */


#define INSIDE( J ) !NEGATIVE(J)
#define OUTSIDE( J ) NEGATIVE(J)




static GLuint TAG(userclip_line)( GLcontext *ctx, 
				  GLuint *i, GLuint *j,
				  interp_func interp )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint ii = *i;
   GLuint jj = *j;
   GLuint p;

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	 GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	 GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	 GLfloat d = ctx->Transform._ClipUserPlane[p][3];

	 GLfloat dpI = d*W(ii) + c*Z(ii) + b*Y(ii) + a*X(ii);
	 GLfloat dpJ = d*W(jj) + c*Z(jj) + b*Y(jj) + a*X(jj);

         GLuint flagI = OUTSIDE( dpI );
         GLuint flagJ = OUTSIDE( dpJ );

	 if (flagI ^ flagJ) {
	    if (flagJ) {
	       GLfloat t = dpI / (dpI - dpJ);
	       VB->ClipMask[jj] |= CLIP_USER_BIT;
	       jj = interp( ctx, t, ii, jj, GL_FALSE );
	       VB->ClipMask[jj] = 0;
	    } else {
	       GLfloat t = dpJ / (dpJ - dpI);
	       VB->ClipMask[ii] |= CLIP_USER_BIT;
	       ii = interp( ctx, t, jj, ii, GL_FALSE );
	       VB->ClipMask[ii] = 0;
	    }
	 }
	 else if (flagI) 
	    return 0;
      }
   }

   *i = ii;
   *j = jj;
   return 1;
}


static GLuint TAG(userclip_polygon)( GLcontext *ctx, 
				     GLuint n, 
				     GLuint vlist[],
				     interp_func interp )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint vlist2[MAX_CLIPPED_VERTICES];
   GLuint *inlist = vlist, *outlist = vlist2;
   GLubyte *clipmask = VB->ClipMask;
   GLuint p;
   
#define CLIP_DOTPROD(xx) d*W(xx) + c*Z(xx) + b*Y(xx) + a*X(xx)
   
   /* Can be speeded up to if vertex_stage actually saves the
    * UserClipMask, and if it is used in this loop (after computing a
    * UserClipOrMask).
    */
   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 register float a = ctx->Transform._ClipUserPlane[p][0];
	 register float b = ctx->Transform._ClipUserPlane[p][1];
	 register float c = ctx->Transform._ClipUserPlane[p][2];
	 register float d = ctx->Transform._ClipUserPlane[p][3];

	 /* initialize prev to be last in the input list */
	 GLuint idxPrev = inlist[n-1];
	 GLfloat dpPrev = CLIP_DOTPROD(idxPrev);
	 GLuint outcount = 0;
	 GLuint i;
	 
         for (i = 0 ; i < n ; i++) {	    
	    GLuint idx = inlist[i];
	    GLfloat dp = CLIP_DOTPROD(idx);

	    if (!NEGATIVE(dpPrev)) {
	       outlist[outcount++] = idxPrev;
	       clipmask[idxPrev] &= ~CLIP_USER_BIT;
	    } else {
	       clipmask[idxPrev] |= CLIP_USER_BIT;
	    }


	    if (DIFFERENT_SIGNS(dp, dpPrev)) {
	       GLuint newvert;
	       if (NEGATIVE(dp)) {
		  /* Going out of bounds.  Avoid division by zero as we
		   * know dp != dpPrev from DIFFERENT_SIGNS, above.
		   */
		  GLfloat t = dp / (dp - dpPrev);
		  newvert = interp( ctx, t, idx, idxPrev, GL_TRUE );
/*  		  fprintf(stderr,"Goint out: in: %d/%d out: %d/%d new: %d/%d\n", */
/*  			  idxPrev, VB->EdgeFlagPtr->data[idxPrev], */
/*  			  idx, VB->EdgeFlagPtr->data[idx], */
/*  			  newvert, VB->EdgeFlagPtr->data[newvert]); */
	       } else {
		  /* Coming back in.
		   */
		  GLfloat t = dpPrev / (dpPrev - dp);
		  newvert = interp( ctx, t, idxPrev, idx, GL_FALSE );
/*  		  fprintf(stderr,"coming in: in: %d/%d out: %d/%d new: %d/%d\n", */
/*  			  idx, VB->EdgeFlagPtr->data[idx], */
/*  			  idxPrev, VB->EdgeFlagPtr->data[idxPrev], */
/*  			  newvert, VB->EdgeFlagPtr->data[newvert]); */
	       }
	       clipmask[newvert] = 0;
	       outlist[outcount++] = newvert;
	    }
	    
	    idxPrev = idx;
	    dpPrev = dp;
	 }

	 if (outcount < 3)
	    return 0;

	 {
            GLuint *tmp;
            tmp = inlist;
            inlist = outlist;
            outlist = tmp;
            n = outcount;
         }
      } /* if */
   } /* for p */

   if (inlist!=vlist) {
      GLuint i;
      for (i = 0 ; i < n ; i++) 
	 vlist[i] = inlist[i];
   }

/*     fprintf(stderr, "%s: finally:\n", __FUNCTION__); */
/*     for (i = 0 ; i < n ; i++)  */
/*        fprintf(stderr, "%d: %d\n", vlist[i], VB->EdgeFlagPtr->data[vlist[i]]); */

   return n;
}


/* This now calls the user clip functions if required.
 */
static void TAG(viewclip_line)( GLcontext *ctx,
				GLuint i, GLuint j,
				GLubyte mask )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   interp_func interp = (interp_func) VB->interpfunc;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint ii = i, jj = j;
   GLuint vlist[2];
   GLuint n;

   VB->LastClipped = VB->FirstClipped;

/*
 * We use 6 instances of this code to clip against the 6 planes.
 */
#define GENERAL_CLIP					\
   if (mask & PLANE) {					\
      GLfloat dpI = CLIP_DOTPROD( ii );			\
      GLfloat dpJ = CLIP_DOTPROD( jj );			\
							\
      if (DIFFERENT_SIGNS(dpI, dpJ)) {			\
	 if (NEGATIVE(dpJ)) {				\
	    GLfloat t = dpI / (dpI - dpJ);		\
            VB->ClipMask[jj] |= PLANE;			\
	    jj = interp( ctx, t, ii, jj, GL_FALSE );	\
            VB->ClipMask[jj] = 0;			\
	 } else {					\
  	    GLfloat t = dpJ / (dpJ - dpI);		\
            VB->ClipMask[ii] |= PLANE;			\
	    ii = interp( ctx, t, jj, ii, GL_FALSE );	\
            VB->ClipMask[ii] = 0;			\
	 }						\
      }							\
      else if (NEGATIVE(dpI))				\
	 return;					\
  }

#undef CLIP_DOTPROD
#define PLANE CLIP_RIGHT_BIT
#define CLIP_DOTPROD(K) (- X(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE
#define PLANE CLIP_LEFT_BIT
#define CLIP_DOTPROD(K) (X(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE
#define PLANE CLIP_TOP_BIT
#define CLIP_DOTPROD(K) (- Y(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE
#define PLANE CLIP_BOTTOM_BIT
#define CLIP_DOTPROD(K) (Y(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE
#define PLANE CLIP_FAR_BIT
#define CLIP_DOTPROD(K) (- Z(K) + W(K))

   if (SIZE >= 3) { 
      GENERAL_CLIP
   }

#undef CLIP_DOTPROD
#undef PLANE
#define PLANE CLIP_NEAR_BIT
#define CLIP_DOTPROD(K) (Z(K) + W(K))

   if (SIZE >=3 ) { 
      GENERAL_CLIP
   }

#undef CLIP_DOTPROD
#undef PLANE
#undef GENERAL_CLIP


   if (mask & CLIP_USER_BIT) {
      if ( TAG(userclip_line)( ctx, &ii, &jj, interp ) == 0 )
	 return;
   }

   vlist[0] = ii;
   vlist[1] = jj;
   n = 2;

   /* If necessary, project new vertices.
    */
   {
      GLuint i, j;
      GLfloat (*proj)[4] = VB->ProjectedClipPtr->data;
      GLuint start = VB->FirstClipped;

      for (i = 0; i < n; i++) {
	 j = vlist[i];
	 if (j >= start) {
	    if (SIZE == 4 && W(j) != 0.0F) {
	       GLfloat wInv = 1.0F / W(j);
	       proj[j][0] = X(j) * wInv;
	       proj[j][1] = Y(j) * wInv;
	       proj[j][2] = Z(j) * wInv;
	       proj[j][3] = wInv;
	    } else {
	       proj[j][0] = X(j);
	       proj[j][1] = Y(j);
	       proj[j][2] = Z(j);
	       proj[j][3] = W(j);
	    }
	 }
      }
   }

   if (ctx->Driver.BuildProjectedVertices) 
      ctx->Driver.BuildProjectedVertices(ctx, 
					 VB->FirstClipped, 
					 VB->LastClipped,
					 ~0);
   

   /* Render the new line.
    */
   ctx->Driver.LineFunc( ctx, ii, jj, j );
}

/* We now clip polygon triangles individually.  This is necessary to
 * avoid artifacts dependent on where the boundary of the VB falls
 * within the polygon.  As a result, there is an upper bound on the
 * number of vertices which may be created, and the test against VB_SIZE
 * is redundant.  
 */
static void TAG(viewclip_polygon)( GLcontext *ctx, 
				   GLuint n, GLuint vlist[], GLuint pv,
				   GLubyte mask )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   interp_func interp = (interp_func) VB->interpfunc;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint vlist2[MAX_CLIPPED_VERTICES];
   GLuint *inlist = vlist, *outlist = vlist2;
   GLuint i;
   GLubyte *clipmask = VB->ClipMask;

   VB->LastClipped = VB->FirstClipped;

   if (mask & CLIP_ALL_BITS) {

#define GENERAL_CLIP							\
   if (mask & PLANE) {							\
      GLuint idxPrev = inlist[n-1];					\
      GLfloat dpPrev = CLIP_DOTPROD(idxPrev);				\
      GLuint outcount = 0;						\
      GLuint i;								\
									\
      mask &= ~PLANE;							\
									\
      for (i = 0; i < n; i++) {  					\
	 GLuint idx = inlist[i];					\
	 GLfloat dp = CLIP_DOTPROD(idx);				\
									\
	 if (!NEGATIVE(dpPrev)) {					\
	    outlist[outcount++] = idxPrev;				\
 	    clipmask[idxPrev] &= ~PLANE;				\
	 }								\
									\
	 if (DIFFERENT_SIGNS(dp, dpPrev)) {				\
            GLuint newvert;						\
	    if (NEGATIVE(dp)) {						\
	       /* Going out of bounds.  Avoid division by zero as we	\
		* know dp != dpPrev from DIFFERENT_SIGNS, above.	\
		*/							\
	       GLfloat t = dp / (dp - dpPrev);				\
      	       newvert = interp( ctx, t, idx, idxPrev, GL_TRUE );	\
	    } else {							\
	       /* Coming back in.					\
		*/							\
	       GLfloat t = dpPrev / (dpPrev - dp);			\
	       newvert = interp( ctx, t, idxPrev, idx, GL_FALSE );	\
	    }								\
            clipmask[newvert] = mask;					\
            outlist[outcount++] = newvert;				\
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
   }


#define PLANE CLIP_RIGHT_BIT
#define CLIP_DOTPROD(K) (- X(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE


#define PLANE CLIP_LEFT_BIT
#define CLIP_DOTPROD(K) (X(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE

#define PLANE CLIP_TOP_BIT
#define CLIP_DOTPROD(K) (- Y(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE

#define PLANE CLIP_BOTTOM_BIT
#define CLIP_DOTPROD(K) (Y(K) + W(K))

   GENERAL_CLIP

#undef CLIP_DOTPROD
#undef PLANE

#define PLANE CLIP_FAR_BIT
#define CLIP_DOTPROD(K) (- Z(K) + W(K))

   if (SIZE >= 3) { 
      GENERAL_CLIP
   }

#undef CLIP_DOTPROD
#undef PLANE

#define PLANE CLIP_NEAR_BIT
#define CLIP_DOTPROD(K) (Z(K) + W(K))

   if (SIZE >=3 ) { 
      GENERAL_CLIP
   }

#undef CLIP_DOTPROD
#undef PLANE
#undef GENERAL_CLIP

      if (inlist != vlist) 
	 for (i = 0 ; i < n ; i++)
	    vlist[i] = inlist[i];
   }

   /* Clip against user clipping planes in clip space. 
    */
   if (mask & CLIP_USER_BIT) {
      n = TAG(userclip_polygon)( ctx, n, vlist, interp );
      if (n < 3) return;
   }

   /* Project if necessary.
    */
   {
      GLuint i;
      GLfloat (*proj)[4] = VB->ProjectedClipPtr->data;
      GLuint first = VB->FirstClipped;

      for (i = 0; i < n; i++) {
	 GLuint j = vlist[i];
	 if (j >= first) {
	    if (SIZE == 4 && W(j) != 0.0F) {
	       GLfloat wInv = 1.0F / W(j);
	       proj[j][0] = X(j) * wInv;
	       proj[j][1] = Y(j) * wInv;
	       proj[j][2] = Z(j) * wInv;
	       proj[j][3] = wInv;
	    } else {
	       proj[j][0] = X(j);
	       proj[j][1] = Y(j);
	       proj[j][2] = Z(j);
	       proj[j][3] = W(j);
	    }
	 }
      }
   }

   if (ctx->Driver.BuildProjectedVertices)
      ctx->Driver.BuildProjectedVertices(ctx, 
					 VB->FirstClipped, 
					 VB->LastClipped,
					 ~0);

   /* Render the new vertices as an unclipped polygon. 
    * Argh - need to pass in pv...
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = vlist;
      render_poly_pv_raw_elts( ctx, 0, n, PRIM_BEGIN|PRIM_END, pv );
      VB->Elts = tmp;
   }
}



#undef W
#undef Z
#undef Y
#undef X
#undef SIZE
#undef TAG
#undef INSIDE
#undef OUTSIDE
