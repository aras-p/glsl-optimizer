
/*
 * Mesa 3-D graphics library
 * Version:  4.0.3
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


/* Template for render stages which build and emit vertices directly
 * to fixed-size dma buffers.  Useful for rendering strips and other
 * native primitives where clipping and per-vertex tweaks such as
 * those in t_dd_tritmp.h are not required.
 *
 */

#if !HAVE_TRIANGLES || !HAVE_POINTS || !HAVE_LINES
#error "must have points, lines & triangles to use render template"
#endif

#if !HAVE_TRI_STRIPS || !HAVE_TRI_FANS
#error "must have tri strip and fans to use render template"
#endif

#if !HAVE_LINE_STRIPS
#error "must have line strips to use render template"
#endif

#if !HAVE_POLYGONS
#error "must have polygons to use render template"
#endif

#if !HAVE_ELTS
#error "must have elts to use render template"
#endif


#ifndef EMIT_TWO_ELTS
#define EMIT_TWO_ELTS( offset, elt0, elt1 )	\
do { 						\
   EMIT_ELT( offset, elt0 ); 			\
   EMIT_ELT( offset+1, elt1 ); 			\
} while (0)
#endif


/**********************************************************************/
/*                  Render whole begin/end objects                    */
/**********************************************************************/


static void TAG(emit_elts)( GLcontext *ctx, GLuint *elts, GLuint nr )
{
   GLint i;
   LOCAL_VARS;
   ELTS_VARS;

   ALLOC_ELTS( nr );

   for ( i = 0 ; i < nr ; i+=2, elts += 2 ) {
      EMIT_TWO_ELTS( 0, elts[0], elts[1] );
      INCR_ELTS( 2 );
   }
}

static void TAG(emit_consecutive_elts)( GLcontext *ctx, GLuint start, GLuint nr )
{
   GLint i;
   LOCAL_VARS;
   ELTS_VARS;

   ALLOC_ELTS( nr );

   for ( i = 0 ; i+1 < nr ; i+=2, start += 2 ) {
      EMIT_TWO_ELTS( 0, start, start+1 );
      INCR_ELTS( 2 );
   }
   if (i < nr) {
      EMIT_ELT( 0, start );
      INCR_ELTS( 1 );
   }
}

/***********************************************************************
 *                    Render non-indexed primitives.
 ***********************************************************************/



static void TAG(render_points_verts)( GLcontext *ctx,
				      GLuint start,
				      GLuint count,
				      GLuint flags )
{
   if (start < count) {
      LOCAL_VARS;
      if (0) fprintf(stderr, "%s\n", __FUNCTION__);
      EMIT_PRIM( ctx, GL_POINTS, HW_POINTS, start, count );
   }
}

static void TAG(render_lines_verts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);
   count -= (count-start) & 1;

   if (start+1 >= count)
      return;

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag) {
      RESET_STIPPLE();
      AUTO_STIPPLE( GL_TRUE );
   }
      
   EMIT_PRIM( ctx, GL_LINES, HW_LINES, start, count );

   if ((flags & PRIM_END) && ctx->Line.StippleFlag)
      AUTO_STIPPLE( GL_FALSE );
}


static void TAG(render_line_strip_verts)( GLcontext *ctx,
					  GLuint start,
					  GLuint count,
					  GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start+1 >= count)
      return;

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag)
      RESET_STIPPLE();


   if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_LINES ))
   {   
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      ELT_INIT( GL_LINES, HW_LINES );

      /* Emit whole number of lines in each full buffer.
       */
      dmasz = dmasz/2;
      currentsz = GET_CURRENT_VB_MAX_ELTS();
      currentsz = currentsz/2;

      if (currentsz < 4) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start; j + 1 < count; j += nr - 1 ) {
	 GLint i;
	 ELTS_VARS;
	 nr = MIN2( currentsz, count - j );
	    
	 ALLOC_ELTS( (nr-1)*2 );
	    
	 for ( i = j ; i+1 < j+nr ; i+=1 ) {
	    EMIT_TWO_ELTS( 0, (i+0), (i+1) );
	    INCR_ELTS( 2 );
	 }

	 if (nr == currentsz) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }
      }
   }
   else
      EMIT_PRIM( ctx, GL_LINE_STRIP, HW_LINE_STRIP, start, count );
}


static void TAG(render_line_loop_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   GLuint j, nr;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (flags & PRIM_BEGIN) {
      j = start;
      if (ctx->Line.StippleFlag)
	 RESET_STIPPLE( );
   }
   else
      j = start + 1;

   if (flags & PRIM_END) {

      if (start+1 >= count)
	 return;

      if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_LINES )) {
	 int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
	 int currentsz;

	 ELT_INIT( GL_LINES, HW_LINES );

	 /* Emit whole number of lines in each full buffer.
	  */
	 dmasz = dmasz/2;
	 currentsz = GET_CURRENT_VB_MAX_ELTS();
	 currentsz = currentsz/2;

	 if (currentsz < 4) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }

	 /* Ensure last vertex doesn't wrap:
	  */
	 currentsz--;
	 dmasz--;

	 for (; j + 1 < count;  ) {
	    GLint i;
	    ELTS_VARS;
	    nr = MIN2( currentsz, count - j );
	    
	    ALLOC_ELTS( (nr-1)*2 );	    
	    for ( i = j ; i+1 < j+nr ; i+=1 ) {
	       EMIT_TWO_ELTS( 0, (i+0), (i+1) );
	       INCR_ELTS( 2 );
	    }

	    j += nr - 1;
	    if (j + 1 < count) {
	       NEW_BUFFER();
	       currentsz = dmasz;
	    }
 	    else { 
 	       ALLOC_ELTS( 2 ); 
 	       EMIT_TWO_ELTS( 0, (j), (start) ); 
 	       INCR_ELTS( 2 ); 
 	    } 
	 }
      }
      else
      {
	 int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
	 int currentsz;

	 ELT_INIT( GL_LINE_STRIP, HW_LINE_STRIP );

	 currentsz = GET_CURRENT_VB_MAX_ELTS();

	 if (currentsz < 8) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }

	 /* Ensure last vertex doesn't wrap:
	  */
	 currentsz--;
	 dmasz--;

	 for ( ; j + 1 < count;  ) {
	    nr = MIN2( currentsz, count - j );
	    if (j + nr < count) {
	       TAG(emit_consecutive_elts)( ctx, j, nr );
	       currentsz = dmasz;
	       j += nr - 1;
	       NEW_BUFFER();
	    }
	    else if (nr) {
	       ELTS_VARS;
	       int i;

	       ALLOC_ELTS( nr + 1 );
	       for ( i = 0 ; i+1 < nr ; i+=2, j += 2 ) {
		  EMIT_TWO_ELTS( 0, j, j+1 );
		  INCR_ELTS( 2 );
	       }
	       if (i < nr) {
		  EMIT_ELT( 0, j ); j++;
		  INCR_ELTS( 1 );
	       }
	       EMIT_ELT( 0, start );
	       INCR_ELTS( 1 );
	       NEW_BUFFER();
	    }
	    else {
	       fprintf(stderr, "warining nr==0\n");
	    }
	 }   
      }
   } else {
      TAG(render_line_strip_verts)( ctx, j, count, flags );
   }
}


static void TAG(render_triangles_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   count -= (count-start)%3;

   if (start+2 >= count) {
      return;
   }

   /* need a PREFER_DISCRETE_ELT_PRIM here too..
    */
   EMIT_PRIM( ctx, GL_TRIANGLES, HW_TRIANGLES, start, count );
}



static void TAG(render_tri_strip_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start + 2 >= count)
      return;

   if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_TRIANGLES ))
   {   
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      int parity = 0;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      if (flags & PRIM_PARITY)
	 parity = 1;

      /* Emit even number of tris in each full buffer.
       */
      dmasz = dmasz/3;
      dmasz -= dmasz & 1;
      currentsz = GET_CURRENT_VB_MAX_ELTS();
      currentsz = currentsz/3;
      currentsz -= currentsz & 1;

      if (currentsz < 4) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start; j + 2 < count; j += nr - 2 ) {
	 GLint i;
	 ELTS_VARS;
	 nr = MIN2( currentsz, count - j );
	    
	 ALLOC_ELTS( (nr-2)*3 );
	    
	 for ( i = j ; i+2 < j+nr ; i++, parity^=1 ) {
	    EMIT_ELT( 0, (i+0+parity) );
	    EMIT_ELT( 1, (i+1-parity) );
	    EMIT_ELT( 2, (i+2) );
	    INCR_ELTS( 3 );
	 }

	 if (nr == currentsz) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }
      }
   }
   else if ((flags & PRIM_PARITY) == 0)  
      EMIT_PRIM( ctx, GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0, start, count );
   else if (HAVE_TRI_STRIP_1)
      EMIT_PRIM( ctx, GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_1, start, count );
   else {
      /* Emit the first triangle with elts, then the rest as a regular strip.
       * TODO:  Make this unlikely in t_imm_api.c
       */
      ELTS_VARS;
      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );
      ALLOC_ELTS( 3 );
      EMIT_ELT( 0, (start+1) );
      EMIT_ELT( 1, (start+0) );
      EMIT_ELT( 2, (start+2) );
      INCR_ELTS( 3 );
      NEW_PRIMITIVE();

      start++;
      if (start + 2 >= count)
	 return;

      EMIT_PRIM( ctx, GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0, start, 
		 count );
   }
}

static void TAG(render_tri_fan_verts)( GLcontext *ctx,
				       GLuint start,
				       GLuint count,
				       GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start+2 >= count) 
      return;

   if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_TRIANGLES ))
   {   
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      dmasz = dmasz/3;
      currentsz = GET_CURRENT_VB_MAX_ELTS();
      currentsz = currentsz/3;

      if (currentsz < 4) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start + 1; j + 1 < count; j += nr - 1 ) {
	 GLint i;
	 ELTS_VARS;
	 nr = MIN2( currentsz, count - j );
	    
	 ALLOC_ELTS( (nr-1)*3 );
	    
	 for ( i = j ; i+1 < j+nr ; i++ ) {
	    EMIT_ELT( 0, (start) );
	    EMIT_ELT( 1, (i) );
	    EMIT_ELT( 2, (i+1) );
	    INCR_ELTS( 3 );
	 }

	 if (nr == currentsz) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }
      }
   }
   else {
      EMIT_PRIM( ctx, GL_TRIANGLE_FAN, HW_TRIANGLE_FAN, start, count );
   }
}


static void TAG(render_poly_verts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start+2 >= count) 
      return;

   EMIT_PRIM( ctx, GL_POLYGON, HW_POLYGON, start, count );
}

static void TAG(render_quad_strip_verts)( GLcontext *ctx,
					  GLuint start,
					  GLuint count,
					  GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   count -= (count-start) & 1;

   if (start+3 >= count) 
      return;

   if (HAVE_QUAD_STRIPS) {
      EMIT_PRIM( ctx, GL_QUAD_STRIP, HW_QUAD_STRIP, start, count );
   } 
   else if (ctx->_TriangleCaps & DD_FLATSHADE) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      currentsz = (currentsz/6)*2;
      dmasz = (dmasz/6)*2;

      if (currentsz < 4) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start; j + 3 < count; j += nr - 2 ) {
	 ELTS_VARS;
	 GLint quads, i;

	 nr = MIN2( currentsz, count - j );
	 quads = (nr/2)-1;
	    
	 ALLOC_ELTS( quads*6 );
	    
	 for ( i = j ; i < j+quads*2 ; i+=2 ) {
	    EMIT_TWO_ELTS( 0, (i+0), (i+1) );
	    EMIT_TWO_ELTS( 2, (i+2), (i+1) );
	    EMIT_TWO_ELTS( 4, (i+3), (i+2) );
	    INCR_ELTS( 6 );
	 }

	 if (nr == currentsz) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }
      }
   }
   else {
      EMIT_PRIM( ctx, GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0, start, count );
   }
}


static void TAG(render_quads_verts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);
   count -= (count-start)%4;

   if (start+3 >= count) 
      return;

   if (HAVE_QUADS) {
      EMIT_PRIM( ctx, HW_QUADS, GL_QUADS, start, count );
   } 
   else {
      /* Hardware doesn't have a quad primitive type -- simulate it
       * using indexed vertices and the triangle primitive: 
       */
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Adjust for rendering as triangles:
       */
      currentsz = (currentsz/6)*4;
      dmasz = (dmasz/6)*4;

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start; j < count; j += nr ) {
	 ELTS_VARS;
	 GLint quads, i;
	 nr = MIN2( currentsz, count - j );
	 quads = nr/4;

	 ALLOC_ELTS( quads*6 );

	 for ( i = j ; i < j+quads*4 ; i+=4 ) {
	    EMIT_TWO_ELTS( 0, (i+0), (i+1) );
	    EMIT_TWO_ELTS( 2, (i+3), (i+1) );
	    EMIT_TWO_ELTS( 4, (i+2), (i+3) );
	    INCR_ELTS( 6 );
	 }

	 if (nr == currentsz) {
	    NEW_BUFFER();
	    currentsz = dmasz;
	 }
      }
   }
}

static void TAG(render_noop)( GLcontext *ctx,
			      GLuint start,
			      GLuint count,
			      GLuint flags )
{
}




static render_func TAG(render_tab_verts)[GL_POLYGON+2] =
{
   TAG(render_points_verts),
   TAG(render_lines_verts),
   TAG(render_line_loop_verts),
   TAG(render_line_strip_verts),
   TAG(render_triangles_verts),
   TAG(render_tri_strip_verts),
   TAG(render_tri_fan_verts),
   TAG(render_quads_verts),
   TAG(render_quad_strip_verts),
   TAG(render_poly_verts),
   TAG(render_noop),
};


/****************************************************************************
 *                 Render elts using hardware indexed verts                 *
 ****************************************************************************/

static void TAG(render_points_elts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;
   GLuint *elts = GET_ELTS();
   GLuint j, nr;

   ELT_INIT( GL_POINTS, HW_POINTS );

   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j < count; j += nr ) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}



static void TAG(render_lines_elts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;
   GLuint *elts = GET_ELTS();
   GLuint j, nr;

   if (start+1 >= count)
      return;

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag) {
      RESET_STIPPLE();
      AUTO_STIPPLE( GL_TRUE );
   }

   ELT_INIT( GL_LINES, HW_LINES );

   /* Emit whole number of lines in total and in each buffer:
    */
   count -= (count-start) & 1;
   currentsz -= currentsz & 1;
   dmasz -= dmasz & 1;

   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j < count; j += nr ) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }

   if ((flags & PRIM_END) && ctx->Line.StippleFlag)
      AUTO_STIPPLE( GL_FALSE );
}


static void TAG(render_line_strip_elts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;
   GLuint *elts = GET_ELTS();
   GLuint j, nr;

   if (start+1 >= count)
      return;

   ELT_INIT( GL_LINE_STRIP, HW_LINE_STRIP );

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag)
      RESET_STIPPLE();

   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j + 1 < count; j += nr - 1 ) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}


static void TAG(render_line_loop_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;
   GLuint *elts = GET_ELTS();
   GLuint j, nr;

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (flags & PRIM_BEGIN)
      j = start;
   else
      j = start + 1;

   
   if (flags & PRIM_END) {
      if (start+1 >= count)
	 return;
   } 
   else {
      if (j+1 >= count)
	 return;
   }

   ELT_INIT( GL_LINE_STRIP, HW_LINE_STRIP );

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag)
      RESET_STIPPLE();

   
   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8) {
      NEW_BUFFER();
      currentsz = dmasz;
   }

   /* Ensure last vertex doesn't wrap:
    */
   currentsz--;
   dmasz--;

   for ( ; j + 1 < count; j += nr - 1 ) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      currentsz = dmasz;
   }

   if (flags & PRIM_END)
      TAG(emit_elts)( ctx, elts+start, 1 );

   NEW_PRIMITIVE();
}


static void TAG(render_triangles_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = GET_ELTS();
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS()/3*3;
   int currentsz;
   GLuint j, nr;

   if (start+2 >= count)
      return;

/*     NEW_PRIMITIVE(); */
   ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

   currentsz = GET_CURRENT_VB_MAX_ELTS();

   /* Emit whole number of tris in total.  dmasz is already a multiple
    * of 3.
    */
   count -= (count-start)%3;
   currentsz -= currentsz%3;
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j < count; j += nr) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}



static void TAG(render_tri_strip_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   GLuint j, nr;
   GLuint *elts = GET_ELTS();
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0 );

   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8) {
      NEW_BUFFER();
      currentsz = dmasz;
   }

   if ((flags & PRIM_PARITY) && count - start > 2) {
      TAG(emit_elts)( ctx, elts+start, 1 );
      currentsz--;
   }

   /* Keep the same winding over multiple buffers:
    */
   dmasz -= (dmasz & 1);
   currentsz -= (currentsz & 1);

   for (j = start ; j + 2 < count; j += nr - 2 ) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}

static void TAG(render_tri_fan_elts)( GLcontext *ctx,
				      GLuint start,
				      GLuint count,
				      GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = GET_ELTS();
   GLuint j, nr;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_TRIANGLE_FAN, HW_TRIANGLE_FAN );

   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8) {
      NEW_BUFFER();
      currentsz = dmasz;
   }

   for (j = start + 1 ; j + 1 < count; j += nr - 1 ) {
      nr = MIN2( currentsz, count - j + 1 );
      TAG(emit_elts)( ctx, elts+start, 1 );
      TAG(emit_elts)( ctx, elts+j, nr - 1 );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}


static void TAG(render_poly_elts)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = GET_ELTS();
   GLuint j, nr;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
   int currentsz;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_POLYGON, HW_POLYGON );

   currentsz = GET_CURRENT_VB_MAX_ELTS();
   if (currentsz < 8) {
      NEW_BUFFER();
      currentsz = dmasz;
   }

   for (j = start + 1 ; j + 1 < count ; j += nr - 1 ) {
      nr = MIN2( currentsz, count - j + 1 );
      TAG(emit_elts)( ctx, elts+start, 1 );
      TAG(emit_elts)( ctx, elts+j, nr - 1 );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}

static void TAG(render_quad_strip_elts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   if (start+3 >= count)
      return;

   if (HAVE_QUAD_STRIPS && 0) {
   }
   else {
      LOCAL_VARS;
      GLuint *elts = GET_ELTS();
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      NEW_PRIMITIVE();
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 1;
      count -= (count-start) & 1;
      currentsz -= currentsz & 1;

      if (currentsz < 12)
	 currentsz = dmasz;

      if (ctx->_TriangleCaps & DD_FLATSHADE) {
	 ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

	 currentsz = currentsz/6*2;
	 dmasz = dmasz/6*2;

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );

	    if (nr >= 4)
	    {
	       GLint i;
	       GLint quads = (nr/2)-1;
	       ELTS_VARS;

	       ALLOC_ELTS( quads*6 );

	       for ( i = j-start ; i < j-start+quads ; i++, elts += 2 ) {
		  EMIT_TWO_ELTS( 0, elts[0], elts[1] );
		  EMIT_TWO_ELTS( 2, elts[2], elts[1] );
		  EMIT_TWO_ELTS( 4, elts[3], elts[2] );
		  INCR_ELTS( 6 );
	       }

	       NEW_PRIMITIVE();
	    }

	    currentsz = dmasz;
	 }
      }
      else {
	 ELT_INIT( GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0 );

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );
	    TAG(emit_elts)( ctx, elts+j, nr );
	    NEW_PRIMITIVE();
	    currentsz = dmasz;
	 }
      }
   }
}


static void TAG(render_quads_elts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   if (start+3 >= count)
      return;

   if (HAVE_QUADS && 0) {
   } else {
      LOCAL_VARS;
      GLuint *elts = GET_ELTS();
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 3;
      count -= (count-start) & 3;
      currentsz -= currentsz & 3;

      /* Adjust for rendering as triangles:
       */
      currentsz = currentsz/6*4;
      dmasz = dmasz/6*4;

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j + 3 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );

	 if (nr >= 4)
	 {
	    GLint quads = nr/4;
	    GLint i;
	    ELTS_VARS;
	    ALLOC_ELTS( quads * 6 );

	    for ( i = j-start ; i < j-start+quads ; i++, elts += 4 ) {
	       EMIT_TWO_ELTS( 0, elts[0], elts[1] );
	       EMIT_TWO_ELTS( 2, elts[3], elts[1] );
	       EMIT_TWO_ELTS( 4, elts[2], elts[3] );
	       INCR_ELTS( 6 );
	    }
	 }

	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   }
}



static render_func TAG(render_tab_elts)[GL_POLYGON+2] =
{
   TAG(render_points_elts),
   TAG(render_lines_elts),
   TAG(render_line_loop_elts),
   TAG(render_line_strip_elts),
   TAG(render_triangles_elts),
   TAG(render_tri_strip_elts),
   TAG(render_tri_fan_elts),
   TAG(render_quads_elts),
   TAG(render_quad_strip_elts),
   TAG(render_poly_elts),
   TAG(render_noop),
};
