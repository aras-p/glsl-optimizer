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

#include "rtasm/rtasm_x86sse.h"
#include "tgsi/exec/tgsi_exec.h"
#include "tgsi/util/tgsi_scan.h"


struct pipe_context;
struct gallivm_prog;
struct gallivm_cpu_engine;

struct draw_pt_middle_end;
struct draw_pt_front_end;

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

/* NOTE: It should match vertex_id size above */
#define UNDEFINED_VERTEX_ID 0xffff

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

   struct vertex_header **tmp;  /**< temp vert storage, such as for clipping */
   unsigned nr_tmps;

   void (*point)( struct draw_stage *,
		  struct prim_header * );

   void (*line)( struct draw_stage *,
		 struct prim_header * );

   void (*tri)( struct draw_stage *,
		struct prim_header * );

   void (*flush)( struct draw_stage *,
		  unsigned flags );

   void (*reset_stipple_counter)( struct draw_stage * );

   void (*destroy)( struct draw_stage * );
};


#define PRIM_QUEUE_LENGTH      32
#define VCACHE_SIZE            32
#define VCACHE_OVERFLOW        4
#define VS_QUEUE_LENGTH        (VCACHE_SIZE + VCACHE_OVERFLOW + 1)	/* can never fill up */

/**
 * Private version of the compiled vertex_shader
 */
struct draw_vertex_shader {

   /* This member will disappear shortly:
    */
   struct pipe_shader_state   state;

   struct tgsi_shader_info info;

   void (*prepare)( struct draw_vertex_shader *shader,
		    struct draw_context *draw );

   /* Run the shader - this interface will get cleaned up in the
    * future:
    */
   void (*run)( struct draw_vertex_shader *shader,
		struct draw_context *draw,
		const unsigned *elts, 
		unsigned count,
		struct vertex_header *vOut[] );
		    

   void (*delete)( struct draw_vertex_shader * );
};


/* Internal function for vertex fetch.
 */
typedef void (*fetch_func)(const void *ptr, float *attrib);
typedef void (*full_fetch_func)( struct draw_context *draw,
				 struct tgsi_exec_machine *machine,
				 const unsigned *elts,
				 unsigned count );

typedef void (*pt_fetch_func)( struct draw_context *draw,
			      float *out,
			      unsigned start,
			       unsigned count );


struct vbuf_render;

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
      struct draw_stage *flatshade;
      struct draw_stage *clip;
      struct draw_stage *cull;
      struct draw_stage *twoside;
      struct draw_stage *offset;
      struct draw_stage *unfilled;
      struct draw_stage *stipple;
      struct draw_stage *aapoint;
      struct draw_stage *aaline;
      struct draw_stage *pstipple;
      struct draw_stage *wide_line;
      struct draw_stage *wide_point;
      struct draw_stage *rasterize;
   } pipeline;


   struct vbuf_render *render;

   /* Support prototype passthrough path:
    */
   struct {
      unsigned prim;           /* XXX: to be removed */
      unsigned hw_vertex_size; /* XXX: to be removed */

      struct {
         struct draw_pt_middle_end *fetch_emit;
         struct draw_pt_middle_end *fetch_shade_emit;
         struct draw_pt_middle_end *fetch_shade_cliptest_pipeline_or_emit;
      } middle;

      struct {
         struct draw_pt_front_end *noop;
         struct draw_pt_front_end *split_arrays;
         struct draw_pt_front_end *vcache;
      } front;

   } pt;


   /* pipe state that we need: */
   const struct pipe_rasterizer_state *rasterizer;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_ATTRIB_MAX];
   struct pipe_vertex_element vertex_element[PIPE_ATTRIB_MAX];
   struct draw_vertex_shader *vertex_shader;

   uint num_vs_outputs;  /**< convenience, from vertex_shader */

   /* user-space vertex data, buffers */
   struct {
      /** vertex element/index buffer (ex: glDrawElements) */
      const void *elts;
      /** bytes per index (0, 1, 2 or 4) */
      unsigned eltSize;

      /** vertex arrays */
      const void *vbuffer[PIPE_ATTRIB_MAX];

      /** constant buffer (for vertex shader) */
      const void *constants;
   } user;

   /* Clip derived state:
    */
   float plane[12][4];
   unsigned nr_planes;

   float wide_point_threshold; /**< convert pnts to tris if larger than this */
   float wide_line_threshold;  /**< convert lines to tris if wider than this */
   boolean line_stipple;       /**< do line stipple? */
   boolean point_sprite;       /**< convert points to quads for sprites? */
   boolean use_sse;

   /* If a prim stage introduces new vertex attributes, they'll be stored here
    */
   struct {
      uint semantic_name;
      uint semantic_index;
      int slot;
   } extra_vp_outputs;

   unsigned reduced_prim;

   /** TGSI program interpreter runtime state */
   struct tgsi_exec_machine machine;

   /* Vertex fetch internal state
    */
   struct {
      const ubyte *src_ptr[PIPE_ATTRIB_MAX];
      unsigned pitch[PIPE_ATTRIB_MAX];
      fetch_func fetch[PIPE_ATTRIB_MAX];
      unsigned nr_attrs;
      full_fetch_func fetch_func;
      pt_fetch_func pt_fetch;
   } vertex_fetch;

   /* Post-tnl vertex cache:
    */
   struct {
      unsigned referenced;  /**< bitfield */

      struct {
	 unsigned in;		/* client array element */
	 unsigned out;		/* index in vs queue/array */
      } idx[VCACHE_SIZE + VCACHE_OVERFLOW];

      unsigned overflow;

      /** To find space in the vertex cache: */
      struct vertex_header *(*get_vertex)( struct draw_context *draw,
                                           unsigned i );
   } vcache;

   /* Vertex shader queue:
    */
   struct {
      struct {
	 unsigned elt;   /**< index into the user's vertex arrays */
	 struct vertex_header *vertex;
      } queue[VS_QUEUE_LENGTH];
      unsigned queue_nr;
      unsigned post_nr;
   } vs;

   /**
    * Run the vertex shader on all vertices in the vertex queue.
    */
   void (*shader_queue_flush)(struct draw_context *draw);

   /* Prim pipeline queue:
    */
   struct {
      /* Need to queue up primitives until their vertices have been
       * transformed by a vs queue flush.
       */
      struct prim_header queue[PRIM_QUEUE_LENGTH];
      unsigned queue_nr;
   } pq;


   /* This (and the tgsi_exec_machine struct) probably need to be moved somewhere private.
    */
   struct gallivm_cpu_engine *engine;   
   void *driver_private;
};



extern struct draw_stage *draw_unfilled_stage( struct draw_context *context );
extern struct draw_stage *draw_twoside_stage( struct draw_context *context );
extern struct draw_stage *draw_offset_stage( struct draw_context *context );
extern struct draw_stage *draw_clip_stage( struct draw_context *context );
extern struct draw_stage *draw_flatshade_stage( struct draw_context *context );
extern struct draw_stage *draw_cull_stage( struct draw_context *context );
extern struct draw_stage *draw_stipple_stage( struct draw_context *context );
extern struct draw_stage *draw_wide_line_stage( struct draw_context *context );
extern struct draw_stage *draw_wide_point_stage( struct draw_context *context );
extern struct draw_stage *draw_validate_stage( struct draw_context *context );


extern void draw_free_temp_verts( struct draw_stage *stage );

extern void draw_alloc_temp_verts( struct draw_stage *stage, unsigned nr );

extern void draw_reset_vertex_ids( struct draw_context *draw );


extern int draw_vertex_cache_check_space( struct draw_context *draw, 
					  unsigned nr_verts );

extern void draw_vertex_cache_invalidate( struct draw_context *draw );
extern void draw_vertex_cache_unreference( struct draw_context *draw );
extern void draw_vertex_cache_reset_vertex_ids( struct draw_context *draw );

extern void draw_vertex_shader_queue_flush( struct draw_context *draw );

extern void draw_update_vertex_fetch( struct draw_context *draw );

extern boolean draw_need_pipeline(const struct draw_context *draw,
                                  unsigned prim );


/* Passthrough mode (second attempt):
 */
boolean draw_pt_init( struct draw_context *draw );
void draw_pt_destroy( struct draw_context *draw );
boolean draw_pt_arrays( struct draw_context *draw,
                        unsigned prim,
                        unsigned start,
                        unsigned count );



/* Prototype/hack (DEPRECATED)
 */
boolean
draw_passthrough_arrays(struct draw_context *draw, 
                        unsigned prim,
                        unsigned start, 
                        unsigned count);


#define DRAW_FLUSH_SHADER_QUEUE              0x1 /* sized not to overflow, never raised */
#define DRAW_FLUSH_PRIM_QUEUE                0x2
#define DRAW_FLUSH_VERTEX_CACHE              0x4
#define DRAW_FLUSH_STATE_CHANGE              0x8
#define DRAW_FLUSH_BACKEND                   0x10


void draw_do_flush( struct draw_context *draw, unsigned flags );



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
   const uint vsize = sizeof(struct vertex_header)
      + stage->draw->num_vs_outputs * 4 * sizeof(float);
   memcpy(tmp, vert, vsize);
   tmp->vertex_id = UNDEFINED_VERTEX_ID;
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
