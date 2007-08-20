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


#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#ifdef MESA
#include "vf/vf.h"
#else
/* XXX these are temporary */
struct vf_attr_map {
   unsigned attrib;
   unsigned format;
   unsigned offset;
};
#define VF_ATTRIB_POS 0
#define VF_ATTRIB_COLOR0 3
#define VF_ATTRIB_COLOR1 4
#define VF_ATTRIB_BFC0 25
#define VF_ATTRIB_BFC1 26

#define VF_ATTRIB_CLIP_POS 27
#define VF_ATTRIB_VERTEX_HEADER 28
#define VF_ATTRIB_MAX 29
#define EMIT_1F 0
#define EMIT_4F 3
#define EMIT_4F_VIEWPORT 6
#define FRAG_ATTRIB_MAX 13
#endif


/**
 * Basic vertex info.
 * Carry some useful information around with the vertices in the prim pipe.  
 */
struct vertex_header {
   unsigned clipmask:12;
   unsigned edgeflag:1;
   unsigned pad:19;

   float clip[4];

   float data[][4];		/* Note variable size */
};

#define MAX_VERTEX_SIZE ((2 + FRAG_ATTRIB_MAX) * 4 * sizeof(float))



/**
 * Basic info for a point/line/triangle primitive.
 */
struct prim_header {
   float det;                 /**< front/back face determinant */
   unsigned reset_line_stipple:1;
   unsigned edgeflags:3;
   unsigned pad:28;
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
   unsigned nr_tmps;

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
   /** Drawing/primitive pipeline stages */
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
   struct pipe_vertex_buffer vertex_buffer[PIPE_ATTRIB_MAX];
   struct pipe_vertex_element vertex_element[PIPE_ATTRIB_MAX];
   struct pipe_shader_state vertex_shader;

   const void *mapped_vbuffer[PIPE_ATTRIB_MAX];

   /* Clip derived state:
    */
   float plane[12][4];
   unsigned nr_planes;

   /*
    * Vertex attribute info
    */
   unsigned vf_attr_to_slot[PIPE_ATTRIB_MAX];
   struct vf_attr_map attrs[VF_ATTRIB_MAX];
   unsigned nr_attrs;

   unsigned vertex_size;       /**< in bytes */
   unsigned nr_vertices;

   /** Pointer to vertex element/index buffer */
   unsigned eltSize;  /**< bytes per index (0, 1, 2 or 4) */
   void *elts;

   unsigned prim;   /**< current prim type: PIPE_PRIM_x */
   unsigned reduced_prim;

   void (*vs_flush)( struct draw_context *draw );

   struct vertex_header *(*get_vertex)( struct draw_context *draw,
					unsigned i );

   /* Post-tnl vertex cache:
    */
   struct {
      unsigned referenced;
      unsigned idx[VCACHE_SIZE + VCACHE_OVERFLOW];
      struct vertex_header *vertex[VCACHE_SIZE + VCACHE_OVERFLOW];
      unsigned overflow;
   } vcache;

   /* Vertex shader queue:
    */
   struct {
      struct {
	 unsigned elt;
	 struct vertex_header *dest;
      } queue[VS_QUEUE_LENGTH];
      unsigned queue_nr;
   } vs;

   /* Prim pipeline queue:
    */
   struct {

      /* Need to queue up primitives until their vertices have been
       * transformed by a vs queue flush.
       */
      struct prim_header queue[PRIM_QUEUE_LENGTH];
      unsigned queue_nr;
   } pq;


#if 0
   ubyte *verts;
   boolean in_vb;
   struct vertex_fetch *vf;
#endif
};



extern struct draw_stage *draw_unfilled_stage( struct draw_context *context );
extern struct draw_stage *draw_twoside_stage( struct draw_context *context );
extern struct draw_stage *draw_offset_stage( struct draw_context *context );
extern struct draw_stage *draw_clip_stage( struct draw_context *context );
extern struct draw_stage *draw_flatshade_stage( struct draw_context *context );
extern struct draw_stage *draw_cull_stage( struct draw_context *context );


extern void draw_free_tmps( struct draw_stage *stage );
extern void draw_alloc_tmps( struct draw_stage *stage, unsigned nr );



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
	  unsigned idx )
{   
   struct vertex_header *tmp = stage->tmp[idx];
   memcpy(tmp, vert, stage->draw->vertex_size );
   return tmp;
}


#endif /* DRAW_PRIVATE_H */
