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

#ifndef DRAW_VF_H
#define DRAW_VF_H


#include "math/m_vector.h"

#include "pipe/p_compiler.h"
#include "draw_vertex.h"


enum {
   DRAW_VF_ATTRIB_POS = 0,
   DRAW_VF_ATTRIB_WEIGHT = 1,
   DRAW_VF_ATTRIB_NORMAL = 2,
   DRAW_VF_ATTRIB_COLOR0 = 3,
   DRAW_VF_ATTRIB_COLOR1 = 4,
   DRAW_VF_ATTRIB_FOG = 5,
   DRAW_VF_ATTRIB_COLOR_INDEX = 6,
   DRAW_VF_ATTRIB_EDGEFLAG = 7,
   DRAW_VF_ATTRIB_TEX0 = 8,
   DRAW_VF_ATTRIB_TEX1 = 9,
   DRAW_VF_ATTRIB_TEX2 = 10,
   DRAW_VF_ATTRIB_TEX3 = 11,
   DRAW_VF_ATTRIB_TEX4 = 12,
   DRAW_VF_ATTRIB_TEX5 = 13,
   DRAW_VF_ATTRIB_TEX6 = 14,
   DRAW_VF_ATTRIB_TEX7 = 15,
   DRAW_VF_ATTRIB_VAR0 = 16,
   DRAW_VF_ATTRIB_VAR1 = 17,
   DRAW_VF_ATTRIB_VAR2 = 18,
   DRAW_VF_ATTRIB_VAR3 = 19,
   DRAW_VF_ATTRIB_VAR4 = 20,
   DRAW_VF_ATTRIB_VAR5 = 21,
   DRAW_VF_ATTRIB_VAR6 = 22,
   DRAW_VF_ATTRIB_VAR7 = 23,
   DRAW_VF_ATTRIB_POINTSIZE = 24,
   DRAW_VF_ATTRIB_BFC0 = 25,
   DRAW_VF_ATTRIB_BFC1 = 26,
   DRAW_VF_ATTRIB_CLIP_POS = 27,
   DRAW_VF_ATTRIB_VERTEX_HEADER = 28,
   DRAW_VF_ATTRIB_MAX = 29
};

enum draw_vf_attr_format {
   DRAW_EMIT_1F,
   DRAW_EMIT_2F,
   DRAW_EMIT_3F,
   DRAW_EMIT_4F,
   DRAW_EMIT_3F_XYW,			/**< for projective texture */
   DRAW_EMIT_1UB_1F,			/**< for fog coordinate */
   DRAW_EMIT_3UB_3F_RGB,		/**< for specular color */
   DRAW_EMIT_3UB_3F_BGR,		/**< for specular color */
   DRAW_EMIT_4UB_4F_RGBA,		/**< for color */
   DRAW_EMIT_4UB_4F_BGRA,		/**< for color */
   DRAW_EMIT_4UB_4F_ARGB,		/**< for color */
   DRAW_EMIT_4UB_4F_ABGR,		/**< for color */
   DRAW_EMIT_PAD,			/**< leave a hole of 'offset' bytes */
   DRAW_EMIT_MAX
};

struct draw_vf_attr_map {
   unsigned attrib;
   enum draw_vf_attr_format format;
   unsigned offset;
};

struct draw_vertex_fetch;



unsigned 
draw_vf_set_vertex_attributes( struct draw_vertex_fetch *vf,
                               const struct draw_vf_attr_map *map,
                               unsigned nr, 
                               unsigned vertex_stride );

void 
draw_vf_set_sources( struct draw_vertex_fetch *vf,
		     GLvector4f * const attrib[],
		     unsigned start ); 

void 
draw_vf_set_data( struct draw_vertex_fetch *vf,
                  float data[][4]);

void 
draw_vf_emit_vertices( struct draw_vertex_fetch *vf,
		       unsigned count,
		       void *dest );

void 
draw_vf_get_attr( struct draw_vertex_fetch *vf,
		  const void *vertex,
		  GLenum attr, 
		  const float *dflt,
		  float *dest );

struct draw_vertex_fetch *
draw_vf_create( void );

void 
draw_vf_destroy( struct draw_vertex_fetch *vf );



/***********************************************************************
 * Internal functions and structs:
 */

struct draw_vf_attr;

typedef void (*draw_vf_extract_func)( const struct draw_vf_attr *a, 
				      float *out, 
				      const uint8_t *v );

typedef void (*draw_vf_insert_func)( const struct draw_vf_attr *a, 
				     uint8_t *v, 
				     const float *in );

typedef void (*draw_vf_emit_func)( struct draw_vertex_fetch *vf,
      				   unsigned count, 
      				   uint8_t *dest );



/**
 * Describes how to convert/move a vertex attribute from a vertex
 * array to a vertex structure.
 */
struct draw_vf_attr
{
   struct draw_vertex_fetch *vf;

   unsigned format;
   unsigned inputsize;
   unsigned inputstride;
   unsigned vertoffset;      /**< position of the attrib in the vertex struct */

   unsigned attrib;          /**< which vertex attrib (0=position, etc) */
   unsigned vertattrsize;    /**< size of the attribute in bytes */

   uint8_t *inputptr;
   const draw_vf_insert_func *insert;
   draw_vf_insert_func do_insert;
   draw_vf_extract_func extract;
};

struct draw_vertex_fetch
{
   struct draw_vf_attr attr[DRAW_VF_ATTRIB_MAX];
   unsigned attr_count;
   unsigned vertex_stride;

   struct draw_vf_attr *lookup[DRAW_VF_ATTRIB_MAX];
   
   draw_vf_emit_func emit;

   /* Parameters and constants for codegen:
    */
   float identity[4];

   struct draw_vf_fastpath *fastpath;
   
   void (*codegen_emit)( struct draw_vertex_fetch *vf );
};


struct draw_vf_attr_type {
   unsigned format;
   unsigned size;
   unsigned stride;
   unsigned offset;
};

struct draw_vf_fastpath {
   unsigned vertex_stride;
   unsigned attr_count;
   boolean match_strides;

   struct draw_vf_attr_type *attr;

   draw_vf_emit_func func;
   struct draw_vf_fastpath *next;
};


void 
draw_vf_register_fastpath( struct draw_vertex_fetch *vtx,
                           boolean match_strides );

void 
draw_vf_generic_emit( struct draw_vertex_fetch *vf,
                      unsigned count,
                      uint8_t *v );

void 
draw_vf_generate_hardwired_emit( struct draw_vertex_fetch *vf );

void 
draw_vf_generate_sse_emit( struct draw_vertex_fetch *vf );


struct draw_vf_format_info {
   const char *name;
   draw_vf_insert_func insert[4];
   const unsigned attrsize;
};

const struct draw_vf_format_info draw_vf_format_info[DRAW_EMIT_MAX];


#endif
