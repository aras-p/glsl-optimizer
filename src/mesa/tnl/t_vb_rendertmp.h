/* $Id: t_vb_rendertmp.h,v 1.2 2000/12/27 19:57:37 keithw Exp $ */

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


#ifndef POSTFIX
#define POSTFIX
#endif

#ifndef INIT
#define INIT(x)  
#endif

#ifndef NEED_EDGEFLAG_SETUP
#define NEED_EDGEFLAG_SETUP 0
#define EDGEFLAG_GET(a) 0
#define EDGEFLAG_SET(a,b) 
#endif

#ifndef RESET_STIPPLE
#define RESET_STIPPLE
#endif

#ifndef RESET_OCCLUSION
#define RESET_OCCLUSION
#endif

#ifndef TEST_PRIM_END
#define TEST_PRIM_END(flags) (flags & PRIM_END)
#define TEST_PRIM_BEGIN(flags) (flags & PRIM_BEGIN)
#define TEST_PRIM_PARITY(flags) (flags & PRIM_PARITY)
#endif

#ifndef ELT
#define ELT(x) x
#endif

static void TAG(render_points)( GLcontext *ctx,
				GLuint start,
				GLuint count,
				GLuint flags )
{
   LOCAL_VARS;
   (void) flags;

   RESET_OCCLUSION;
   INIT(GL_POINTS);
   RENDER_POINTS( start, count );
   POSTFIX;
}

static void TAG(render_lines)( GLcontext *ctx,
			       GLuint start,
			       GLuint count,
			       GLuint flags )
{
   GLuint j;
   LOCAL_VARS;
   (void) flags;

   RESET_OCCLUSION;
   INIT(GL_LINES);
   for (j=start+1; j<count; j+=2 ) {
      RENDER_LINE( ELT(j-1), ELT(j) );
      RESET_STIPPLE;
   }
   POSTFIX;
}


static void TAG(render_line_strip)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   GLuint j;
   LOCAL_VARS;
   (void) flags;

   RESET_OCCLUSION;
   INIT(GL_LINES);

   for (j=start+1; j<count; j++ ) 
      RENDER_LINE( ELT(j-1), ELT(j) );

   if (TEST_PRIM_END(flags))
      RESET_STIPPLE;

   POSTFIX;
}


static void TAG(render_line_loop)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   GLuint i;	 
   LOCAL_VARS;

   (void) flags;

   RESET_OCCLUSION;
   INIT(GL_LINES);

   if (start+1 < count) {
      if (TEST_PRIM_BEGIN(flags)) {
	 RENDER_LINE( ELT(start), ELT(start+1) );
      }

      for ( i = start+2 ; i < count ; i++) {
	 RENDER_LINE( ELT(i-1), ELT(i) );
      }

      if ( TEST_PRIM_END(flags)) {
	 RENDER_LINE( ELT(count-1), ELT(start) );
	 RESET_STIPPLE;
      }
   }

   POSTFIX;
}


static void TAG(render_triangles)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   GLuint j;
   LOCAL_VARS;
   (void) flags;

   INIT(GL_POLYGON);
   if (NEED_EDGEFLAG_SETUP) {
      for (j=start+2; j<count; j+=3) {
	 /* Leave the edgeflags as supplied by the user.
	  */
	 RENDER_TRI( ELT(j-2), ELT(j-1), ELT(j), ELT(j), 0 );
	 RESET_STIPPLE;
      }
   } else {
      for (j=start+2; j<count; j+=3) {
	 RENDER_TRI( ELT(j-2), ELT(j-1), ELT(j), ELT(j), 0 );
      }
   }
   POSTFIX;
}



static void TAG(render_tri_strip)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   GLuint j;
   GLuint parity = 0;
   LOCAL_VARS;
   
   if (TEST_PRIM_PARITY(flags))
      parity = 1;
   
   INIT(GL_POLYGON);
   if (NEED_EDGEFLAG_SETUP) {
      for (j=start+2;j<count;j++,parity^=1) {
	 /* All edges are boundary.  Set edgeflags to 1, draw the
	  * triangle, and restore them to the original values.
	  */
	 GLuint ej2 = ELT(j-2);
	 GLuint ej1 = ELT(j-1);
	 GLuint ej = ELT(j);
	 GLboolean ef2 = EDGEFLAG_GET( ej2 );
	 GLboolean ef1 = EDGEFLAG_GET( ej1 );
	 GLboolean ef = EDGEFLAG_GET( ej );
	 EDGEFLAG_SET( ej2, GL_TRUE );
	 EDGEFLAG_SET( ej1, GL_TRUE );
	 EDGEFLAG_SET( ej, GL_TRUE );
	 RENDER_TRI( ej2, ej1, ej, ej, parity );
	 EDGEFLAG_SET( ej2, ef2 );
	 EDGEFLAG_SET( ej1, ef1 );
	 EDGEFLAG_SET( ej, ef );
	 RESET_STIPPLE;
      }
   } else {
      for (j=start+2;j<count;j++,parity^=1) {
	 RENDER_TRI( ELT(j-2), ELT(j-1), ELT(j), ELT(j), parity );
      }
   }
   POSTFIX;
}


static void TAG(render_tri_fan)( GLcontext *ctx,
				 GLuint start,
				 GLuint count,
				 GLuint flags )
{
   GLuint j;
   LOCAL_VARS;
   (void) flags;

   INIT(GL_POLYGON);
   if (NEED_EDGEFLAG_SETUP) {
      for (j=start+2;j<count;j++) {
	 /* For trifans, all edges are boundary.
	  */
	 GLuint ejs = ELT(start);
	 GLuint ej1 = ELT(j-1);
	 GLuint ej = ELT(j);
	 GLboolean efs = EDGEFLAG_GET( ejs );
	 GLboolean ef1 = EDGEFLAG_GET( ej1 );
	 GLboolean ef = EDGEFLAG_GET( ej );
	 EDGEFLAG_SET( ejs, GL_TRUE );
	 EDGEFLAG_SET( ej1, GL_TRUE );
	 EDGEFLAG_SET( ej, GL_TRUE );
	 RENDER_TRI( ejs, ej1, ej, ej, 0);
	 EDGEFLAG_SET( ejs, efs );
	 EDGEFLAG_SET( ej1, ef1 );
	 EDGEFLAG_SET( ej, ef );
	 RESET_STIPPLE;
      }
   } else {
      for (j=start+2;j<count;j++) {
	 RENDER_TRI( ELT(start), ELT(j-1), ELT(j), ELT(j), 0 );
      }
   }

   POSTFIX;
}


/* This is a bit of a hack.  Clipping produces polygons and really
 * wants to use this function to render them (in particular to get the
 * edgeflags right).  However, the rule that pv==start for polys
 * doens't hold there, hence the extra arg and the wrapper below.  
 */
static void TAG(render_poly_pv)( GLcontext *ctx,
				 GLuint start,
				 GLuint count,
				 GLuint flags,
				 GLuint pv )
{
   GLuint j = start+2;
   LOCAL_VARS;
   (void) flags;

   INIT(GL_POLYGON);
   if (NEED_EDGEFLAG_SETUP) {
      GLboolean efstart = EDGEFLAG_GET( ELT(start) );
      GLboolean efcount = EDGEFLAG_GET( ELT(count-1) );

      /* If the primitive does not begin here, the first edge
       * is non-boundary.
       */
      if (!TEST_PRIM_BEGIN(flags)) 
	 EDGEFLAG_SET( ELT(start), GL_FALSE );

      /* If the primitive does not end here, the final edge is
       * non-boundary.
       */
      if (!TEST_PRIM_END(flags)) 
	 EDGEFLAG_SET( ELT(count-1), GL_FALSE );

      /* Draw the first triangles (possibly zero)
       */
      if (j<count-1) {
	 GLboolean ef = EDGEFLAG_GET( ELT(j) );
	 EDGEFLAG_SET( ELT(j), GL_FALSE );
	 RENDER_TRI( ELT(start), ELT(j-1), ELT(j), ELT(pv), 0 );
	 EDGEFLAG_SET( ELT(j), ef );
	 j++;
	    
	 /* Don't render the first edge again:
	  */
	 EDGEFLAG_SET( ELT(start), GL_FALSE );

	 for (;j<count-1;j++) {
	    GLboolean efj = EDGEFLAG_GET( ELT(j) );
	    EDGEFLAG_SET( ELT(j), GL_FALSE );
	    RENDER_TRI( ELT(start), ELT(j-1), ELT(j), ELT(pv), 0 );
	    EDGEFLAG_SET( ELT(j), efj );
	 }
      }

      /* Draw the last or only triangle
       */
      if (j < count)
	 RENDER_TRI( ELT(start), ELT(j-1), ELT(j), ELT(pv), 0 );

      /* Restore the first and last edgeflags:
       */
      EDGEFLAG_SET( ELT(count-1), efcount );
      EDGEFLAG_SET( ELT(start), efstart );
	 
      if (TEST_PRIM_END(flags)) {
	 RESET_STIPPLE;
      }
   }
   else {
      for (j=start+2;j<count;j++) {
	 RENDER_TRI( ELT(start), ELT(j-1), ELT(j), ELT(start), 0 );
      }
   }
   POSTFIX;
}

static void TAG(render_poly)( GLcontext *ctx,
			      GLuint start,
			      GLuint count,
			      GLuint flags )
{
   TAG(render_poly_pv)( ctx, start, count, flags, start );
}

static void TAG(render_quads)( GLcontext *ctx,
			       GLuint start,
			       GLuint count,
			       GLuint flags )
{
   GLuint j;
   LOCAL_VARS;
   (void) flags;

   INIT(GL_POLYGON);
   if (NEED_EDGEFLAG_SETUP) {
      for (j=start+3; j<count; j+=4) {
	 /* Use user-specified edgeflags for quads.
	  */
	 RENDER_QUAD( ELT(j-3), ELT(j-2), ELT(j-1), ELT(j), ELT(j) );
	 RESET_STIPPLE;
      }
   } else {
      for (j=start+3; j<count; j+=4) {
	 RENDER_QUAD( ELT(j-3), ELT(j-2), ELT(j-1), ELT(j), ELT(j) );
      }
   }
   POSTFIX;
}

static void TAG(render_quad_strip)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   GLuint j;
   LOCAL_VARS;
   (void) flags;

   INIT(GL_POLYGON);
   if (NEED_EDGEFLAG_SETUP) {
      for (j=start+3;j<count;j+=2) {
	 /* All edges are boundary.  Set edgeflags to 1, draw the
	  * quad, and restore them to the original values.
	  */
	 GLboolean ef3 = EDGEFLAG_GET( ELT(j-3) );
	 GLboolean ef2 = EDGEFLAG_GET( ELT(j-2) );
	 GLboolean ef1 = EDGEFLAG_GET( ELT(j-1) );
	 GLboolean ef = EDGEFLAG_GET( ELT(j) );
	 EDGEFLAG_SET( ELT(j-3), GL_TRUE );
	 EDGEFLAG_SET( ELT(j-2), GL_TRUE );
	 EDGEFLAG_SET( ELT(j-1), GL_TRUE );
	 EDGEFLAG_SET( ELT(j), GL_TRUE );
	 RENDER_QUAD( ELT(j-3), ELT(j-2), ELT(j), ELT(j-1), ELT(j) );
	 EDGEFLAG_SET( ELT(j-3), ef3 );
	 EDGEFLAG_SET( ELT(j-2), ef2 );
	 EDGEFLAG_SET( ELT(j-1), ef1 );
	 EDGEFLAG_SET( ELT(j), ef );
	 RESET_STIPPLE;
      }
   } else {
      for (j=start+3;j<count;j+=2) {
	 RENDER_QUAD( ELT(j-3), ELT(j-2), ELT(j), ELT(j-1), ELT(j) );
      }
   }
   POSTFIX;
}

static void TAG(render_noop)( GLcontext *ctx,
			      GLuint start,
			      GLuint count,
			      GLuint flags )
{
   (void)(ctx && start && count && flags);
}

static render_func TAG(render_tab)[GL_POLYGON+2] = {
   TAG(render_points),
   TAG(render_lines),
   TAG(render_line_loop),
   TAG(render_line_strip),
   TAG(render_triangles),
   TAG(render_tri_strip),
   TAG(render_tri_fan),
   TAG(render_quads),
   TAG(render_quad_strip),
   TAG(render_poly),
   TAG(render_noop),
};



#ifndef PRESERVE_VB_DEFS
#undef RENDER_TRI
#undef RENDER_QUAD
#undef RENDER_LINE
#undef RENDER_POINTS
#undef LOCAL_VARS
#undef INIT
#undef POSTFIX
#undef RESET_STIPPLE
#undef DBG
#undef ELT
#endif

#ifndef PRESERVE_TAG
#undef TAG
#endif

#undef PRESERVE_VB_DEFS
#undef PRESERVE_TAG

