/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

#ifndef S_TRI_H
#define S_TRI_H

/* Vertices are just an array of floats, with all the attributes
 * packed.  We currently assume a layout like:
 *
 * attr[0][0..3] - window position
 * attr[1..n][0..3] - remaining attributes.
 *
 * Attributes are assumed to be 4 floats wide but are packed so that
 * all the enabled attributes run contiguously.
 */

#include "glheader.h"
#include "imports.h"
#if 0
#include "s_tri_public.h"
#endif
#include "s_context.h"


extern struct prim_stage *prim_setup( struct softpipe_context *softpipe );


#if 0 /* UNUSED? */
struct tri_context;
struct fp_context;
struct be_context;

/* Note the rasterizer does not take a GLcontext argument.  This is
 * deliberate.
 */
struct tri_context *tri_create_context( GLcontext *ctx );

void tri_destroy_context( struct tri_context *tri );

void tri_set_fp_context( struct tri_context *tri,
			 struct fp_context *fp,
			 void (*fp_run)( struct fp_context *fp,
					 const struct fp_inputs *,
					 struct fp_outputs * ));


void tri_set_be_context( struct tri_context *tri,
			 struct be_context *be,
			 void (*be_run)( struct be_context *be,
					 const struct fp_outputs * ));

void tri_set_attribs( struct tri_context *tri,
		      const struct attr_info *info,
		      GLuint nr_attrib );

void tri_set_backface( struct tri_context *tri,
		       GLfloat backface );
					       
void tri_set_scissor( struct tri_context *tri,
		      GLint x,
		      GLint y,
		      GLuint width,
		      GLuint height,
		      GLboolean enabled );

void tri_set_stipple( struct tri_context *tri,
		      const GLuint *pattern,
		      GLboolean enabled );

/* Unfilled triangles will be handled elsewhere (higher in the
 * pipeline), as will things like stipple (lower in the pipeline).
 */

void tri_triangle( struct tri_context *tri,
		   const struct vertex *v0,
		   const struct vertex *v1,
		   const struct vertex *v2 );

/* TODO: rasterize_line, rasterize_point?? 
 * How will linestipple work?
 */


#ifdef SETUP_PRIVATE






GLboolean tri_setup( struct tri_context *tri,
		       const struct vertex *v0,
		       const struct vertex *v1,
		       const struct vertex *v2 );

void tri_rasterize( struct tri_context *tri );
void tri_rasterize_spans( struct tri_context *tri );






#endif
#endif
#endif
