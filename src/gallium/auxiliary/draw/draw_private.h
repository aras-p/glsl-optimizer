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

#include "tgsi/exec/tgsi_exec.h"
#include "tgsi/util/tgsi_scan.h"


struct pipe_context;
struct gallivm_prog;
struct gallivm_cpu_engine;
struct draw_vertex_shader;
struct draw_context;
struct draw_stage;
struct vbuf_render;


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
 * Basic info for a point/line/triangle primitive.
 */
struct prim_header {
   float det;                 /**< front/back face determinant */
   unsigned reset_line_stipple:1;
   unsigned edgeflags:3;
   unsigned pad:28;
   struct vertex_header *v[3];  /**< 1 to 3 vertex pointers */
};




#define PT_SHADE      0x1
#define PT_CLIPTEST   0x2
#define PT_PIPELINE   0x4
#define PT_MAX_MIDDLE 0x8

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
         struct draw_pt_middle_end *general;
      } middle;

      struct {
         struct draw_pt_front_end *vcache;
      } front;

      struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
      unsigned nr_vertex_buffers;

      struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
      unsigned nr_vertex_elements;

      /* user-space vertex data, buffers */
      struct {
         const unsigned *edgeflag;

         /** vertex element/index buffer (ex: glDrawElements) */
         const void *elts;
         /** bytes per index (0, 1, 2 or 4) */
         unsigned eltSize;
         
         /** vertex arrays */
         const void *vbuffer[PIPE_MAX_ATTRIBS];
         
         /** constant buffer (for vertex shader) */
         const void *constants;
      } user;

   } pt;

   struct {
      boolean bypass_clipping;
   } driver;

   boolean flushing;         /**< debugging/sanity */
   boolean suspend_flushing; /**< internally set */
   boolean bypass_clipping;  /**< set if either api or driver bypass_clipping true */

   /* pipe state that we need: */
   const struct pipe_rasterizer_state *rasterizer;
   struct pipe_viewport_state viewport;

   struct draw_vertex_shader *vertex_shader;

   boolean identity_viewport;

   uint num_vs_outputs;  /**< convenience, from vertex_shader */


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
   } extra_vp_outputs;

   unsigned reduced_prim;

   /** TGSI program interpreter runtime state */
   struct tgsi_exec_machine machine;

   /* This (and the tgsi_exec_machine struct) probably need to be moved somewhere private.
    */
   struct gallivm_cpu_engine *engine;   
   void *driver_private;
};







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

void draw_pipeline_run( struct draw_context *draw,
                        unsigned prim,
                        struct vertex_header *vertices,
                        unsigned vertex_count,
                        unsigned stride,
                        const ushort *elts,
                        unsigned count );

void draw_pipeline_flush( struct draw_context *draw, 
                          unsigned flags );



/*******************************************************************************
 * Flushing 
 */

#define DRAW_FLUSH_STATE_CHANGE              0x8
#define DRAW_FLUSH_BACKEND                   0x10


void draw_do_flush( struct draw_context *draw, unsigned flags );




#endif /* DRAW_PRIVATE_H */
