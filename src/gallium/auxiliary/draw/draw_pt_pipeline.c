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

#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"


/**
 * Add a point to the primitive queue.
 * \param i0  index into user's vertex arrays
 */
static void do_point( struct draw_context *draw,
		      const char *v0 )
{
   struct prim_header prim;
   
   prim.reset_line_stipple = 0;
   prim.edgeflags = 1;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;

   draw->pipeline.first->point( draw->pipeline.first, &prim );
}


/**
 * Add a line to the primitive queue.
 * \param i0  index into user's vertex arrays
 * \param i1  index into user's vertex arrays
 */
static void do_line( struct draw_context *draw,
		     const char *v0,
		     const char *v1 )
{
   struct prim_header prim;
   
   prim.reset_line_stipple = 1; /* fixme */
   prim.edgeflags = 1;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;

   draw->pipeline.first->line( draw->pipeline.first, &prim );
}

/**
 * Add a triangle to the primitive queue.
 */
static void do_triangle( struct draw_context *draw,
			 char *v0,
			 char *v1,
			 char *v2 )
{
   struct prim_header prim;
   
   prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;
   prim.v[2] = (struct vertex_header *)v2;
   prim.reset_line_stipple = 1;
   prim.edgeflags = ((prim.v[0]->edgeflag)      |
                     (prim.v[1]->edgeflag << 1) |
                     (prim.v[2]->edgeflag << 2));
   prim.pad = 0;

   if (0) debug_printf("tri ef: %d %d %d\n", 
                       prim.v[0]->edgeflag,
                       prim.v[1]->edgeflag,
                       prim.v[2]->edgeflag);
   
   draw->pipeline.first->tri( draw->pipeline.first, &prim );
}



void draw_pt_reset_vertex_ids( struct draw_context *draw )
{
   unsigned i;
   char *verts = draw->pt.pipeline.verts;
   unsigned stride = draw->pt.pipeline.vertex_stride;

   for (i = 0; i < draw->pt.pipeline.vertex_count; i++) {
      ((struct vertex_header *)verts)->vertex_id = UNDEFINED_VERTEX_ID;
      verts += stride;
   }
}


/* Code to run the pipeline on a fairly arbitary collection of vertices.
 *
 * Vertex headers must be pre-initialized with the
 * UNDEFINED_VERTEX_ID, this code will cause that id to become
 * overwritten, so it may have to be reset if there is the intention
 * to reuse the vertices.
 *
 * This code provides a callback to reset the vertex id's which the
 * draw_vbuf.c code uses when it has to perform a flush.
 */
void draw_pt_run_pipeline( struct draw_context *draw,
                           unsigned prim,
                           char *verts,
                           unsigned stride,
                           unsigned vertex_count,
                           const ushort *elts,
                           unsigned count )
{
   unsigned i;

   draw->pt.pipeline.verts = verts;
   draw->pt.pipeline.vertex_stride = stride;
   draw->pt.pipeline.vertex_count = vertex_count;
   
   switch (prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i++) 
         do_point( draw, 
                   verts + stride * elts[i] );
      break;
   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) 
         do_line( draw, 
                  verts + stride * elts[i+0],
                  verts + stride * elts[i+1]);
      break;
   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3)
         do_triangle( draw, 
                      verts + stride * elts[i+0],
                      verts + stride * elts[i+1],
                      verts + stride * elts[i+2]);
      break;
   }
   
   draw->pt.pipeline.verts = NULL;
   draw->pt.pipeline.vertex_count = 0;
}

