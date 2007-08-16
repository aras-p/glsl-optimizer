/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * Private data structures, etc for the draw module.
 */


/**
 * Authors:
 * Keith Whitwell <keith@tungstengraphics.com>
 * Brian Paul
 */


#ifndef DRAW_PRIVATE_H
#define DRAW_PRIVATE_H


#include "main/glheader.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "vf/vf.h"


/**
 * Basic vertex info.
 * Carry some useful information around with the vertices in the prim pipe.  
 */
struct vertex_header {
   GLuint clipmask:12;
   GLuint edgeflag:1;
   GLuint pad:19;

   GLfloat clip[4];

   GLfloat data[][4];		/* Note variable size */
};

#define MAX_VERTEX_SIZE ((2 + FRAG_ATTRIB_MAX) * 4 * sizeof(GLfloat))



/**
 * Basic info for a point/line/triangle primitive.
 */
struct prim_header {
   GLfloat det;                 /**< front/back face determinant */
   GLuint reset_line_stipple:1;
   GLuint edgeflags:3;
   GLuint pad:28;
   struct vertex_header *v[3];  /**< 1 to 3 vertex pointers */
};



struct draw_context;

/**
 * Base class for all primitive drawing stages.
 */
struct draw_stage
{
   struct draw_context *draw;   /**< parent context */

   struct draw_stage *next;     /**< next stage in pipeline */

   struct vertex_header **tmp;
   GLuint nr_tmps;

   void (*begin)( struct draw_stage * );

   void (*point)( struct draw_stage *,
		  struct prim_header * );

   void (*line)( struct draw_stage *,
		 struct prim_header * );

   void (*tri)( struct draw_stage *,
		struct prim_header * );
   
   void (*end)( struct draw_stage * );

   void (*reset_stipple_counter)( struct draw_stage * );
};


#define PRIM_QUEUE_LENGTH      16
#define VCACHE_SIZE            32
#define VCACHE_OVERFLOW        4
#define VS_QUEUE_LENGTH        (VCACHE_SIZE + VCACHE_OVERFLOW + 1)	/* can never fill up */


/**
 * Private context for the drawing module.
 */
struct draw_context
{
   struct {
      struct draw_stage *first;  /**< one of the following */

      /* stages (in logical order) */
      struct draw_stage *flatshade;
      struct draw_stage *clip;
      struct draw_stage *cull;
      struct draw_stage *twoside;
      struct draw_stage *offset;
      struct draw_stage *unfilled;
      struct draw_stage *setup;  /* aka render/rasterize */
   } pipeline;

   /* pipe state that we need: */
   struct pipe_setup_state setup;
   struct pipe_viewport_state viewport;

   const struct pipe_vertex_buffer *vertex_buffer;  /**< note: pointer */
   const struct pipe_vertex_element *vertex_element; /**< note: pointer */


   /* Clip derived state:
    */
   GLfloat plane[12][4];
   GLuint nr_planes;

   GLuint vf_attr_to_slot[PIPE_ATTRIB_MAX];

   struct vf_attr_map attrs[VF_ATTRIB_MAX];
   GLuint nr_attrs;
   GLuint vertex_size;       /**< in bytes */
   struct vertex_fetch *vf;

   GLubyte *verts;
   GLuint nr_vertices;
   GLboolean in_vb;

   /** Pointer to vertex element/index buffer */
   unsigned eltSize;  /**< bytes per index (0, 1, 2 or 4) */
   void *elts;

   struct vertex_header *(*get_vertex)( struct draw_context *draw,
					GLuint i );

   /* Post-tnl vertex cache:
    */
   struct {
      GLuint referenced;
      GLuint idx[VCACHE_SIZE + VCACHE_OVERFLOW];
      struct vertex_header *vertex[VCACHE_SIZE + VCACHE_OVERFLOW];
      GLuint overflow;
   } vcache;

   /* Vertex shader queue:
    */
   struct {
      struct {
	 GLuint elt;
	 struct vertex_header *dest;
      } queue[VS_QUEUE_LENGTH];
      GLuint queue_nr;
   } vs;

   /* Prim pipeline queue:
    */
   struct {

      /* Need to queue up primitives until their vertices have been
       * transformed by a vs queue flush.
       */
      struct prim_header queue[PRIM_QUEUE_LENGTH];
      GLuint queue_nr;
   } pq;



   GLenum prim;   /**< GL_POINTS, GL_LINE_STRIP, GL_QUADS, etc */
   unsigned reduced_prim;

   void (*vs_flush)( struct draw_context *draw );

   /* Helper for tnl:
    */
   GLvector4f header;   

   /* helper for sp_draw_arrays.c - temporary? */
   void *mapped_vbuffer;
};



extern struct draw_stage *draw_unfilled_stage( struct draw_context *context );
extern struct draw_stage *draw_twoside_stage( struct draw_context *context );
extern struct draw_stage *draw_offset_stage( struct draw_context *context );
extern struct draw_stage *draw_clip_stage( struct draw_context *context );
extern struct draw_stage *draw_flatshade_stage( struct draw_context *context );
extern struct draw_stage *draw_cull_stage( struct draw_context *context );


extern void draw_free_tmps( struct draw_stage *stage );
extern void draw_alloc_tmps( struct draw_stage *stage, GLuint nr );



/**
 * Get a writeable copy of a vertex.
 * \param stage  drawing stage info
 * \param vert  the vertex to copy (source)
 * \param idx  index into stage's tmp[] array to put the copy (dest)
 * \return  pointer to the copied vertex
 */
static INLINE struct vertex_header *
dup_vert( struct draw_stage *stage,
	  const struct vertex_header *vert,
	  GLuint idx )
{   
   struct vertex_header *tmp = stage->tmp[idx];
   memcpy(tmp, vert, stage->draw->vertex_size );
   return tmp;
}


#endif /* DRAW_PRIVATE_H */
