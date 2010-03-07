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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "draw/draw_private.h"
#include "draw/draw_pipe.h"
#include "util/u_debug.h"



boolean draw_pipeline_init( struct draw_context *draw )
{
   /* create pipeline stages */
   draw->pipeline.wide_line  = draw_wide_line_stage( draw );
   draw->pipeline.wide_point = draw_wide_point_stage( draw );
   draw->pipeline.stipple   = draw_stipple_stage( draw );
   draw->pipeline.unfilled  = draw_unfilled_stage( draw );
   draw->pipeline.twoside   = draw_twoside_stage( draw );
   draw->pipeline.offset    = draw_offset_stage( draw );
   draw->pipeline.clip      = draw_clip_stage( draw );
   draw->pipeline.flatshade = draw_flatshade_stage( draw );
   draw->pipeline.cull      = draw_cull_stage( draw );
   draw->pipeline.validate  = draw_validate_stage( draw );
   draw->pipeline.first     = draw->pipeline.validate;

   if (!draw->pipeline.wide_line ||
       !draw->pipeline.wide_point ||
       !draw->pipeline.stipple ||
       !draw->pipeline.unfilled ||
       !draw->pipeline.twoside ||
       !draw->pipeline.offset ||
       !draw->pipeline.clip ||
       !draw->pipeline.flatshade ||
       !draw->pipeline.cull ||
       !draw->pipeline.validate)
      return FALSE;

   /* these defaults are oriented toward the needs of softpipe */
   draw->pipeline.wide_point_threshold = 1000000.0; /* infinity */
   draw->pipeline.wide_line_threshold = 1.0;
   draw->pipeline.line_stipple = TRUE;
   draw->pipeline.point_sprite = TRUE;

   return TRUE;
}


void draw_pipeline_destroy( struct draw_context *draw )
{
   if (draw->pipeline.wide_line)
      draw->pipeline.wide_line->destroy( draw->pipeline.wide_line );
   if (draw->pipeline.wide_point)
      draw->pipeline.wide_point->destroy( draw->pipeline.wide_point );
   if (draw->pipeline.stipple)
      draw->pipeline.stipple->destroy( draw->pipeline.stipple );
   if (draw->pipeline.unfilled)
      draw->pipeline.unfilled->destroy( draw->pipeline.unfilled );
   if (draw->pipeline.twoside)
      draw->pipeline.twoside->destroy( draw->pipeline.twoside );
   if (draw->pipeline.offset)
      draw->pipeline.offset->destroy( draw->pipeline.offset );
   if (draw->pipeline.clip)
      draw->pipeline.clip->destroy( draw->pipeline.clip );
   if (draw->pipeline.flatshade)
      draw->pipeline.flatshade->destroy( draw->pipeline.flatshade );
   if (draw->pipeline.cull)
      draw->pipeline.cull->destroy( draw->pipeline.cull );
   if (draw->pipeline.validate)
      draw->pipeline.validate->destroy( draw->pipeline.validate );
   if (draw->pipeline.aaline)
      draw->pipeline.aaline->destroy( draw->pipeline.aaline );
   if (draw->pipeline.aapoint)
      draw->pipeline.aapoint->destroy( draw->pipeline.aapoint );
   if (draw->pipeline.pstipple)
      draw->pipeline.pstipple->destroy( draw->pipeline.pstipple );
   if (draw->pipeline.rasterize)
      draw->pipeline.rasterize->destroy( draw->pipeline.rasterize );
}



/**
 * Build primitive to render a point with vertex at v0.
 */
static void do_point( struct draw_context *draw,
		      const char *v0 )
{
   struct prim_header prim;
   
   prim.flags = 0;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;

   draw->pipeline.first->point( draw->pipeline.first, &prim );
}


/**
 * Build primitive to render a line with vertices at v0, v1.
 * \param flags  bitmask of DRAW_PIPE_EDGE_x, DRAW_PIPE_RESET_STIPPLE
 */
static void do_line( struct draw_context *draw,
                     ushort flags,
		     const char *v0,
		     const char *v1 )
{
   struct prim_header prim;
   
   prim.flags = flags;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;

   draw->pipeline.first->line( draw->pipeline.first, &prim );
}


/**
 * Build primitive to render a triangle with vertices at v0, v1, v2.
 * \param flags  bitmask of DRAW_PIPE_EDGE_x, DRAW_PIPE_RESET_STIPPLE
 */
static void do_triangle( struct draw_context *draw,
                         ushort flags,
			 char *v0,
			 char *v1,
			 char *v2 )
{
   struct prim_header prim;
   
   prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;
   prim.v[2] = (struct vertex_header *)v2;
   prim.flags = flags;
   prim.pad = 0;

   draw->pipeline.first->tri( draw->pipeline.first, &prim );
}


/*
 * Set up macros for draw_pt_decompose.h template code.
 * This code uses vertex indexes / elements.
 */
#define QUAD(i0,i1,i2,i3)                       \
   do_triangle( draw,                           \
                ( DRAW_PIPE_RESET_STIPPLE |     \
                  DRAW_PIPE_EDGE_FLAG_0 |       \
                  DRAW_PIPE_EDGE_FLAG_2 ),      \
                verts + stride * elts[i0],      \
                verts + stride * elts[i1],      \
                verts + stride * elts[i3]);     \
   do_triangle( draw,                           \
                ( DRAW_PIPE_EDGE_FLAG_0 |       \
                  DRAW_PIPE_EDGE_FLAG_1 ),      \
                verts + stride * elts[i1],      \
                verts + stride * elts[i2],      \
                verts + stride * elts[i3])

#define TRIANGLE(flags,i0,i1,i2)                                        \
   do_triangle( draw,                                                   \
                elts[i0],  /* flags */                                  \
                verts + stride * (elts[i0] & ~DRAW_PIPE_FLAG_MASK),     \
                verts + stride * (elts[i1] & ~DRAW_PIPE_FLAG_MASK),     \
                verts + stride * (elts[i2] & ~DRAW_PIPE_FLAG_MASK) );

#define LINE(flags,i0,i1)                                       \
   do_line( draw,                                               \
            elts[i0],                                           \
            verts + stride * (elts[i0] & ~DRAW_PIPE_FLAG_MASK), \
            verts + stride * (elts[i1] & ~DRAW_PIPE_FLAG_MASK) );

#define POINT(i0)                               \
   do_point( draw,                              \
             verts + stride * elts[i0] )

#define FUNC pipe_run
#define ARGS                                    \
    struct draw_context *draw,                  \
    unsigned prim,                              \
    struct vertex_header *vertices,             \
    unsigned stride,                            \
    const ushort *elts

#define LOCAL_VARS                                           \
   char *verts = (char *)vertices;                           \
   boolean flatfirst = (draw->rasterizer->flatshade &&       \
                        draw->rasterizer->flatshade_first);  \
   unsigned i;                                               \
   ushort flags

#define FLUSH

#include "draw_pt_decompose.h"
#undef ARGS
#undef LOCAL_VARS



/**
 * Code to run the pipeline on a fairly arbitary collection of vertices.
 * For drawing indexed primitives.
 *
 * Vertex headers must be pre-initialized with the
 * UNDEFINED_VERTEX_ID, this code will cause that id to become
 * overwritten, so it may have to be reset if there is the intention
 * to reuse the vertices.
 *
 * This code provides a callback to reset the vertex id's which the
 * draw_vbuf.c code uses when it has to perform a flush.
 */
void draw_pipeline_run( struct draw_context *draw,
                        unsigned prim,
                        struct vertex_header *vertices,
                        unsigned vertex_count,
                        unsigned stride,
                        const ushort *elts,
                        unsigned count )
{
   char *verts = (char *)vertices;

   draw->pipeline.verts = verts;
   draw->pipeline.vertex_stride = stride;
   draw->pipeline.vertex_count = vertex_count;
   
   pipe_run(draw, prim, vertices, stride, elts, count);
   
   draw->pipeline.verts = NULL;
   draw->pipeline.vertex_count = 0;
}



/*
 * Set up macros for draw_pt_decompose.h template code.
 * This code is for non-indexed rendering (no elts).
 */
#define QUAD(i0,i1,i2,i3)                                        \
   do_triangle( draw,                                            \
                ( DRAW_PIPE_RESET_STIPPLE |                      \
                  DRAW_PIPE_EDGE_FLAG_0 |                        \
                  DRAW_PIPE_EDGE_FLAG_2 ),                       \
                verts + stride * ((i0) & ~DRAW_PIPE_FLAG_MASK),  \
                verts + stride * (i1),                           \
                verts + stride * (i3));                          \
   do_triangle( draw,                                            \
                ( DRAW_PIPE_EDGE_FLAG_0 |                        \
                  DRAW_PIPE_EDGE_FLAG_1 ),                       \
                verts + stride * ((i1) & ~DRAW_PIPE_FLAG_MASK),  \
                verts + stride * (i2),                           \
                verts + stride * (i3))

#define TRIANGLE(flags,i0,i1,i2)                                 \
   do_triangle( draw,                                            \
                flags,  /* flags */                              \
                verts + stride * ((i0) & ~DRAW_PIPE_FLAG_MASK),  \
                verts + stride * (i1),                           \
                verts + stride * (i2))

#define LINE(flags,i0,i1)                                   \
   do_line( draw,                                           \
            flags,                                          \
            verts + stride * ((i0) & ~DRAW_PIPE_FLAG_MASK), \
            verts + stride * (i1))

#define POINT(i0)                               \
   do_point( draw,                              \
             verts + stride * i0 )

#define FUNC pipe_run_linear
#define ARGS                                    \
    struct draw_context *draw,                  \
    unsigned prim,                              \
    struct vertex_header *vertices,             \
    unsigned stride

#define LOCAL_VARS                                           \
   char *verts = (char *)vertices;                           \
   boolean flatfirst = (draw->rasterizer->flatshade &&       \
                        draw->rasterizer->flatshade_first);  \
   unsigned i;                                               \
   ushort flags

#define FLUSH

#include "draw_pt_decompose.h"


/*
 * For drawing non-indexed primitives.
 */
void draw_pipeline_run_linear( struct draw_context *draw,
                               unsigned prim,
                               struct vertex_header *vertices,
                               unsigned count,
                               unsigned stride )
{
   char *verts = (char *)vertices;
   draw->pipeline.verts = verts;
   draw->pipeline.vertex_stride = stride;
   draw->pipeline.vertex_count = count;

   pipe_run_linear(draw, prim, vertices, stride, count);

   draw->pipeline.verts = NULL;
   draw->pipeline.vertex_count = 0;
}


void draw_pipeline_flush( struct draw_context *draw, 
                          unsigned flags )
{
   draw->pipeline.first->flush( draw->pipeline.first, flags );
   draw->pipeline.first = draw->pipeline.validate;
}
