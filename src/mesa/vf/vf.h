/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */

#ifndef VF_VERTEX_H
#define VF_VERTEX_H

#include "main/mtypes.h"
#include "math/m_vector.h"

enum {
   VF_ATTRIB_POS = 0,
   VF_ATTRIB_WEIGHT = 1,
   VF_ATTRIB_NORMAL = 2,
   VF_ATTRIB_COLOR0 = 3,
   VF_ATTRIB_COLOR1 = 4,
   VF_ATTRIB_FOG = 5,
   VF_ATTRIB_COLOR_INDEX = 6,
   VF_ATTRIB_EDGEFLAG = 7,
   VF_ATTRIB_TEX0 = 8,
   VF_ATTRIB_TEX1 = 9,
   VF_ATTRIB_TEX2 = 10,
   VF_ATTRIB_TEX3 = 11,
   VF_ATTRIB_TEX4 = 12,
   VF_ATTRIB_TEX5 = 13,
   VF_ATTRIB_TEX6 = 14,
   VF_ATTRIB_TEX7 = 15,
   VF_ATTRIB_VAR0 = 16,
   VF_ATTRIB_VAR1 = 17,
   VF_ATTRIB_VAR2 = 18,
   VF_ATTRIB_VAR3 = 19,
   VF_ATTRIB_VAR4 = 20,
   VF_ATTRIB_VAR5 = 21,
   VF_ATTRIB_VAR6 = 22,
   VF_ATTRIB_VAR7 = 23,
   VF_ATTRIB_POINTSIZE = 24,
   VF_ATTRIB_BFC0 = 25,
   VF_ATTRIB_BFC1 = 26,
   VF_ATTRIB_CLIP_POS = 27,
   VF_ATTRIB_VERTEX_HEADER = 28,
   VF_ATTRIB_MAX = 29
};


enum vf_attr_format {
   EMIT_1F,
   EMIT_2F,
   EMIT_3F,
   EMIT_4F,
   EMIT_2F_VIEWPORT,		/* do viewport transform and emit */
   EMIT_3F_VIEWPORT,		/* do viewport transform and emit */
   EMIT_4F_VIEWPORT,		/* do viewport transform and emit */
   EMIT_3F_XYW,			/* for projective texture */
   EMIT_1UB_1F,			/* for fog coordinate */
   EMIT_3UB_3F_RGB,		/* for specular color */
   EMIT_3UB_3F_BGR,		/* for specular color */
   EMIT_4UB_4F_RGBA,		/* for color */
   EMIT_4UB_4F_BGRA,		/* for color */
   EMIT_4UB_4F_ARGB,		/* for color */
   EMIT_4UB_4F_ABGR,		/* for color */
   EMIT_4CHAN_4F_RGBA,		/* for swrast color */
   EMIT_PAD,			/* leave a hole of 'offset' bytes */
   EMIT_MAX
};

struct vf_attr_map {
   GLuint attrib;
   enum vf_attr_format format;
   GLuint offset;
};

struct vertex_fetch;

void vf_set_vp_matrix( struct vertex_fetch *vf,
		      const GLfloat *viewport );

void vf_set_vp_scale_translate( struct vertex_fetch *vf,
				const GLfloat *scale,
				const GLfloat *translate );

GLuint vf_set_vertex_attributes( struct vertex_fetch *vf,
				 const struct vf_attr_map *map,
				 GLuint nr, 
				 GLuint vertex_stride );

void vf_set_sources( struct vertex_fetch *vf,
		     GLvector4f * const attrib[],
		     GLuint start ); 

void vf_emit_vertices( struct vertex_fetch *vf,
		       GLuint count,
		       void *dest );

void vf_get_attr( struct vertex_fetch *vf,
		  const void *vertex,
		  GLenum attr, 
		  const GLfloat *dflt,
		  GLfloat *dest );

struct vertex_fetch *vf_create( GLboolean allow_viewport_emits );

void vf_destroy( struct vertex_fetch *vf );



/***********************************************************************
 * Internal functions and structs:
 */

struct vf_attr;

typedef void (*vf_extract_func)( const struct vf_attr *a, 
				 GLfloat *out, 
				 const GLubyte *v );

typedef void (*vf_insert_func)( const struct vf_attr *a, 
				GLubyte *v, 
				const GLfloat *in );

typedef void (*vf_emit_func)( struct vertex_fetch *vf,
			      GLuint count, 
			      GLubyte *dest );



/* Describes how to convert/move a vertex attribute from a vertex
 * array to a vertex structure.
 */
struct vf_attr
{
   struct vertex_fetch *vf;

   GLuint format;
   GLuint inputsize;
   GLuint inputstride;
   GLuint vertoffset;      /* position of the attrib in the vertex struct */

   GLuint attrib;          /* which vertex attrib (0=position, etc) */
   GLuint vertattrsize;    /* size of the attribute in bytes */

   GLubyte *inputptr;
   const vf_insert_func *insert;
   vf_insert_func do_insert;
   vf_extract_func extract;
};

struct vertex_fetch
{
   struct vf_attr attr[VF_ATTRIB_MAX];
   GLuint attr_count;
   GLuint vertex_stride;

   struct vf_attr *lookup[VF_ATTRIB_MAX];
   
   vf_emit_func emit;

   /* Parameters and constants for codegen:
    */
   GLboolean allow_viewport_emits;
   GLfloat vp[8];		
   GLfloat chan_scale[4];
   GLfloat identity[4];

   struct vf_fastpath *fastpath;
   
   void (*codegen_emit)( struct vertex_fetch *vf );
};


struct vf_attr_type {
   GLuint format;
   GLuint size;
   GLuint stride;
   GLuint offset;
};

struct vf_fastpath {
   GLuint vertex_stride;
   GLuint attr_count;
   GLboolean match_strides;

   struct vf_attr_type *attr;

   vf_emit_func func;
   struct vf_fastpath *next;
};


void vf_register_fastpath( struct vertex_fetch *vtx,
			     GLboolean match_strides );

void vf_generic_emit( struct vertex_fetch *vf,
			GLuint count,
			GLubyte *v );

void vf_generate_hardwired_emit( struct vertex_fetch *vf );

void vf_generate_sse_emit( struct vertex_fetch *vf );


struct vf_format_info {
   const char *name;
   vf_extract_func extract;
   vf_insert_func insert[4];
   const GLuint attrsize;
};

const struct vf_format_info vf_format_info[EMIT_MAX];


#endif
