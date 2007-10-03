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

#include "draw_vertex.h"

#include "x86/rtasm/x86sse.h"
#include "pipe/tgsi/exec/tgsi_core.h"


/**
 * Basic vertex info.
 * Carry some useful information around with the vertices in the prim pipe.  
 */
struct vertex_header {
   unsigned clipmask:12;
   unsigned edgeflag:1;
   unsigned pad:3;
   unsigned vertex_id:16;

   float clip[4];

   float data[][4];		/* Note variable size */
};

/* XXX This is too large */
#define MAX_VERTEX_SIZE ((2 + PIPE_MAX_SHADER_OUTPUTS) * 4 * sizeof(float))



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
 * Private version of the compiled vertex_shader
 */
struct draw_vertex_shader {
   const struct pipe_shader_state *state;
#if defined(__i386__) || defined(__386__)
   struct x86_function  sse2_program;
#endif
};

/**
 * Private context for the drawing module.
 */
struct draw_context
{
   /** Drawing/primitive pipeline stages */
   struct {
      struct draw_stage *first;  /**< one of the following */

      struct draw_stage *validate; 

      /* stages (in logical order) */
      struct draw_stage *feedback;
      struct draw_stage *flatshade;
      struct draw_stage *clip;
      struct draw_stage *cull;
      struct draw_stage *twoside;
      struct draw_stage *offset;
      struct draw_stage *unfilled;
      struct draw_stage *rasterize;
   } pipeline;

   /* pipe state that we need: */
   const struct pipe_rasterizer_state *rasterizer;
   struct pipe_feedback_state feedback;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_ATTRIB_MAX];
   struct pipe_vertex_element vertex_element[PIPE_ATTRIB_MAX];
   const struct draw_vertex_shader *vertex_shader;
   struct pipe_vertex_buffer feedback_buffer[PIPE_ATTRIB_MAX];
   struct pipe_vertex_element feedback_element[PIPE_ATTRIB_MAX];

   /** The mapped vertex element/index buffer */
   const void *mapped_elts;
   unsigned eltSize;  /**< bytes per index (0, 1, 2 or 4) */
   /** The mapped vertex arrays */
   const void *mapped_vbuffer[PIPE_ATTRIB_MAX];
   /** The mapped constant buffers (for vertex shader) */
   const void *mapped_constants;

   /** The mapped vertex element/index buffer */
   void *mapped_feedback_buffer[PIPE_MAX_FEEDBACK_ATTRIBS];
   uint mapped_feedback_buffer_size[PIPE_MAX_FEEDBACK_ATTRIBS]; /* in bytes */

   /* Clip derived state:
    */
   float plane[12][4];
   unsigned nr_planes;

   /** Describes the layout of post-transformation vertices */
   struct vertex_info vertex_info;
   /** Two-sided attributes: */
   uint attrib_front0, attrib_back0;
   uint attrib_front1, attrib_back1;

   unsigned drawing;
   unsigned prim;   /**< current prim type: PIPE_PRIM_x */
   unsigned reduced_prim;

   /** TGSI program interpreter runtime state */
   struct tgsi_exec_machine machine;

   /* Post-tnl vertex cache:
    */
   struct {
      unsigned referenced;
      unsigned idx[VCACHE_SIZE + VCACHE_OVERFLOW];
      struct vertex_header *vertex[VCACHE_SIZE + VCACHE_OVERFLOW];
      unsigned overflow;

      struct vertex_header *(*get_vertex)( struct draw_context *draw,
                                           unsigned i );
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

   int use_sse : 1;
};



extern struct draw_stage *draw_feedback_stage( struct draw_context *context );
extern struct draw_stage *draw_unfilled_stage( struct draw_context *context );
extern struct draw_stage *draw_twoside_stage( struct draw_context *context );
extern struct draw_stage *draw_offset_stage( struct draw_context *context );
extern struct draw_stage *draw_clip_stage( struct draw_context *context );
extern struct draw_stage *draw_flatshade_stage( struct draw_context *context );
extern struct draw_stage *draw_cull_stage( struct draw_context *context );
extern struct draw_stage *draw_validate_stage( struct draw_context *context );


extern void draw_free_tmps( struct draw_stage *stage );
extern void draw_alloc_tmps( struct draw_stage *stage, unsigned nr );


extern int draw_vertex_cache_check_space( struct draw_context *draw, 
					  unsigned nr_verts );

extern void draw_vertex_cache_invalidate( struct draw_context *draw );
extern void draw_vertex_cache_unreference( struct draw_context *draw );
extern void draw_vertex_cache_reset_vertex_ids( struct draw_context *draw );


extern void draw_vertex_shader_queue_flush( struct draw_context *draw );

struct tgsi_exec_machine;

extern void draw_vertex_fetch( struct draw_context *draw,
			       struct tgsi_exec_machine *machine,
			       const unsigned *elts,
			       unsigned count );


#define DRAW_FLUSH_PRIM_QUEUE                0x1
#define DRAW_FLUSH_VERTEX_CACHE_INVALIDATE   0x2
#define DRAW_FLUSH_DRAW                      0x4


void draw_do_flush( struct draw_context *draw,
                    unsigned flags );




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
   memcpy(tmp, vert, stage->draw->vertex_info.size * sizeof(float) );
   tmp->vertex_id = ~0;
   return tmp;
}

static INLINE float
dot4(const float *a, const float *b)
{
   float result = (a[0]*b[0] +
                   a[1]*b[1] +
                   a[2]*b[2] +
                   a[3]*b[3]);

   return result;
}

#endif /* DRAW_PRIVATE_H */
