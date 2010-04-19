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

#include "tgsi/tgsi_scan.h"


struct pipe_context;
struct draw_vertex_shader;
struct draw_context;
struct draw_stage;
struct vbuf_render;
struct tgsi_exec_machine;
struct tgsi_sampler;


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

   /* This will probably become float (*data)[4] soon:
    */
   float data[][4];
};

/* NOTE: It should match vertex_id size above */
#define UNDEFINED_VERTEX_ID 0xffff


/**
 * Private context for the drawing module.
 */
struct draw_context
{
   struct pipe_context *pipe;

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

      float wide_point_threshold; /**< convert pnts to tris if larger than this */
      float wide_line_threshold;  /**< convert lines to tris if wider than this */
      boolean line_stipple;       /**< do line stipple? */
      boolean point_sprite;       /**< convert points to quads for sprites? */

      /* Temporary storage while the pipeline is being run:
       */
      char *verts;
      unsigned vertex_stride;
      unsigned vertex_count;
   } pipeline;


   struct vbuf_render *render;

   /* Support prototype passthrough path:
    */
   struct {
      struct {
         struct draw_pt_middle_end *fetch_emit;
         struct draw_pt_middle_end *fetch_shade_emit;
         struct draw_pt_middle_end *general;
      } middle;

      struct {
         struct draw_pt_front_end *vcache;
         struct draw_pt_front_end *varray;
      } front;

      struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
      unsigned nr_vertex_buffers;

      struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
      unsigned nr_vertex_elements;

      /* user-space vertex data, buffers */
      struct {
         /** vertex element/index buffer (ex: glDrawElements) */
         const void *elts;
         /** bytes per index (0, 1, 2 or 4) */
         unsigned eltSize;
         unsigned min_index;
         unsigned max_index;
         
         /** vertex arrays */
         const void *vbuffer[PIPE_MAX_ATTRIBS];
         
         /** constant buffer (for vertex/geometry shader) */
         const void *vs_constants[PIPE_MAX_CONSTANT_BUFFERS];
         const void *gs_constants[PIPE_MAX_CONSTANT_BUFFERS];
      } user;

      boolean test_fse;         /* enable FSE even though its not correct (eg for softpipe) */
      boolean no_fse;           /* disable FSE even when it is correct */
   } pt;

   struct {
      boolean bypass_clipping;
      boolean bypass_vs;
   } driver;

   boolean flushing;         /**< debugging/sanity */
   boolean suspend_flushing; /**< internally set */
   boolean bypass_clipping;  /**< set if either api or driver bypass_clipping true */

   boolean force_passthrough; /**< never clip or shade */

   boolean dump_vs;

   double mrd;  /**< minimum resolvable depth value, for polygon offset */

   /** Current rasterizer state given to us by the driver */
   const struct pipe_rasterizer_state *rasterizer;
   /** Driver CSO handle for the current rasterizer state */
   void *rast_handle;

   /** Rasterizer CSOs without culling/stipple/etc */
   void *rasterizer_no_cull[2][2];

   struct pipe_viewport_state viewport;
   boolean identity_viewport;

   struct {
      struct draw_vertex_shader *vertex_shader;
      uint num_vs_outputs;  /**< convenience, from vertex_shader */
      uint position_output;
      uint edgeflag_output;

      /** TGSI program interpreter runtime state */
      struct tgsi_exec_machine *machine;

      uint num_samplers;
      struct tgsi_sampler **samplers;

      /* Here's another one:
       */
      struct aos_machine *aos_machine; 


      const void *aligned_constants[PIPE_MAX_CONSTANT_BUFFERS];

      const void *aligned_constant_storage[PIPE_MAX_CONSTANT_BUFFERS];
      unsigned const_storage_size[PIPE_MAX_CONSTANT_BUFFERS];


      struct translate *fetch;
      struct translate_cache *fetch_cache;
      struct translate *emit;
      struct translate_cache *emit_cache;
   } vs;

   struct {
      struct draw_geometry_shader *geometry_shader;
      uint num_gs_outputs;  /**< convenience, from geometry_shader */
      uint position_output;

      /** TGSI program interpreter runtime state */
      struct tgsi_exec_machine *machine;

      uint num_samplers;
      struct tgsi_sampler **samplers;
   } gs;

   /* Clip derived state:
    */
   float plane[12][4];
   unsigned nr_planes;

   /* If a prim stage introduces new vertex attributes, they'll be stored here
    */
   struct {
      uint semantic_name;
      uint semantic_index;
      int slot;
   } extra_shader_outputs;

   unsigned reduced_prim;

   unsigned instance_id;

   void *driver_private;
};


/*******************************************************************************
 * Vertex shader code:
 */
boolean draw_vs_init( struct draw_context *draw );
void draw_vs_destroy( struct draw_context *draw );

void draw_vs_set_viewport( struct draw_context *, 
                           const struct pipe_viewport_state * );

void
draw_vs_set_constants(struct draw_context *,
                      unsigned slot,
                      const void *constants,
                      unsigned size);



/*******************************************************************************
 * Geometry shading code:
 */
boolean draw_gs_init( struct draw_context *draw );

void
draw_gs_set_constants(struct draw_context *,
                      unsigned slot,
                      const void *constants,
                      unsigned size);

void draw_gs_destroy( struct draw_context *draw );

/*******************************************************************************
 * Common shading code:
 */
uint draw_current_shader_outputs(const struct draw_context *draw);
uint draw_current_shader_position_output(const struct draw_context *draw);

/*******************************************************************************
 * Vertex processing (was passthrough) code:
 */
boolean draw_pt_init( struct draw_context *draw );
void draw_pt_destroy( struct draw_context *draw );
void draw_pt_reset_vertex_ids( struct draw_context *draw );


/*******************************************************************************
 * Primitive processing (pipeline) code: 
 */

boolean draw_pipeline_init( struct draw_context *draw );
void draw_pipeline_destroy( struct draw_context *draw );





/* We use the top few bits in the elts[] parameter to convey a little
 * API information.  This limits the number of vertices we can address
 * to only 4096 -- if that becomes a problem, we can switch to 32-bit
 * draw indices.
 *
 * These flags expected at first vertex of lines & triangles when
 * unfilled and/or line stipple modes are operational.
 */
#define DRAW_PIPE_MAX_VERTICES  (0x1<<12)
#define DRAW_PIPE_EDGE_FLAG_0   (0x1<<12)
#define DRAW_PIPE_EDGE_FLAG_1   (0x2<<12)
#define DRAW_PIPE_EDGE_FLAG_2   (0x4<<12)
#define DRAW_PIPE_EDGE_FLAG_ALL (0x7<<12)
#define DRAW_PIPE_RESET_STIPPLE (0x8<<12)
#define DRAW_PIPE_FLAG_MASK     (0xf<<12)

void draw_pipeline_run( struct draw_context *draw,
                        unsigned prim,
                        struct vertex_header *vertices,
                        unsigned vertex_count,
                        unsigned stride,
                        const ushort *elts,
                        unsigned count );

void draw_pipeline_run_linear( struct draw_context *draw,
                               unsigned prim,
                               struct vertex_header *vertices,
                               unsigned count,
                               unsigned stride );



void draw_pipeline_flush( struct draw_context *draw, 
                          unsigned flags );



/*******************************************************************************
 * Flushing 
 */

#define DRAW_FLUSH_STATE_CHANGE              0x8
#define DRAW_FLUSH_BACKEND                   0x10


void draw_do_flush( struct draw_context *draw, unsigned flags );



void *
draw_get_rasterizer_no_cull( struct draw_context *draw,
                             boolean scissor,
                             boolean flatshade );


#endif /* DRAW_PRIVATE_H */
