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

#include "pipe/p_debug.h"
#include "pipe/p_util.h"

#include "draw_private.h"
#include "draw_context.h"



#define RP_NONE  0
#define RP_POINT 1
#define RP_LINE  2
#define RP_TRI   3


static unsigned reduced_prim[PIPE_PRIM_POLYGON + 1] = {
   RP_POINT,
   RP_LINE,
   RP_LINE,
   RP_LINE,
   RP_TRI,
   RP_TRI,
   RP_TRI,
   RP_TRI,
   RP_TRI,
   RP_TRI
};


static void draw_prim_queue_flush( struct draw_context *draw )
{
   unsigned i;

   if (0)
      debug_printf("Flushing with %d prims, %d verts\n",
                   draw->pq.queue_nr, draw->vs.queue_nr);

   assert (draw->pq.queue_nr != 0);

   /* NOTE: we cannot save draw->pipeline->first in a local var because
    * draw->pipeline->first is often changed by the first call to tri(),
    * line(), etc.
    */
   if (draw->rasterizer->line_stipple_enable) {
      switch (draw->reduced_prim) {
      case RP_TRI:
	 for (i = 0; i < draw->pq.queue_nr; i++) {
	    if (draw->pq.queue[i].reset_line_stipple)
	       draw->pipeline.first->reset_stipple_counter( draw->pipeline.first );
	    
	    draw->pipeline.first->tri( draw->pipeline.first, &draw->pq.queue[i] );
	 }
	 break;
      case RP_LINE:
	 for (i = 0; i < draw->pq.queue_nr; i++) {
	    if (draw->pq.queue[i].reset_line_stipple)
	       draw->pipeline.first->reset_stipple_counter( draw->pipeline.first );
	    
	    draw->pipeline.first->line( draw->pipeline.first, &draw->pq.queue[i] );
	 }
	 break;
      case RP_POINT:
	 draw->pipeline.first->reset_stipple_counter( draw->pipeline.first );
	 for (i = 0; i < draw->pq.queue_nr; i++)
	    draw->pipeline.first->point( draw->pipeline.first, &draw->pq.queue[i] );
	 break;
      }
   }
   else {
      switch (draw->reduced_prim) {
      case RP_TRI:
	 for (i = 0; i < draw->pq.queue_nr; i++) 
	    draw->pipeline.first->tri( draw->pipeline.first, &draw->pq.queue[i] );
	 break;
      case RP_LINE:
	 for (i = 0; i < draw->pq.queue_nr; i++) 
	    draw->pipeline.first->line( draw->pipeline.first, &draw->pq.queue[i] );
	 break;
      case RP_POINT:
	 for (i = 0; i < draw->pq.queue_nr; i++)
	    draw->pipeline.first->point( draw->pipeline.first, &draw->pq.queue[i] );
	 break;
      }
   }

   draw->pq.queue_nr = 0;   
   draw->vs.post_nr = 0;   
   draw_vertex_cache_unreference( draw );
}

static INLINE void fetch_and_store(struct draw_context *draw)
{
   /* run vertex shader on vertex cache entries, four per invokation */
#if 0
   {
      const struct vertex_info *vinfo = draw->render->get_vertex_info(draw->render);
      memcpy(draw->vs.queue[0].vertex, draw->vs.queue[i + j].elt,
             count * vinfo->size);
   }
#elif 0
   unsigned i;

   draw_update_vertex_fetch(draw);
   for (i = 0; i < draw->vs.queue_nr; i += 4) {
      struct vertex_header *dests[4];
      unsigned elts[4];
      struct tgsi_exec_machine *machine = &draw->machine;
      int j, n = MIN2(4, draw->vs.queue_nr - i);

      for (j = 0; j < n; j++) {
         elts[j] = draw->vs.queue[i + j].elt;
         dests[j] = draw->vs.queue[i + j].vertex;
      }

      for ( ; j < 4; j++) {
	 elts[j] = elts[0];
         dests[j] = draw->vs.queue[i + j].vertex;
      }
      //fetch directly into dests
      draw->vertex_fetch.fetch_func(draw, machine, dests, count);
   }
#endif

   draw->vs.post_nr = draw->vs.queue_nr;
   draw->vs.queue_nr = 0;
}

void draw_do_flush( struct draw_context *draw, unsigned flags )
{
   static boolean flushing = FALSE;

   if (0)
      debug_printf("Flushing with %d verts, %d prims\n",
                   draw->vs.queue_nr,
                   draw->pq.queue_nr );

   if (!flushing) {
      flushing = TRUE;

   if (flags >= DRAW_FLUSH_SHADER_QUEUE) {
      if (draw->vs.queue_nr) {
         (*draw->shader_queue_flush)(draw);
      }

      if (flags >= DRAW_FLUSH_PRIM_QUEUE) {
	 if (draw->pq.queue_nr)
	    draw_prim_queue_flush(draw);

	 if (flags >= DRAW_FLUSH_VERTEX_CACHE) {
            draw_vertex_cache_invalidate(draw);

	    if (flags >= DRAW_FLUSH_STATE_CHANGE) {
               draw->pipeline.first->flush( draw->pipeline.first, flags );
               draw->pipeline.first = draw->pipeline.validate;
               draw->reduced_prim = ~0;
	    }
	 }
      }    
   }

      flushing = FALSE;
   }
}



/* Return a pointer to a freshly queued primitive header.  Ensure that
 * there is room in the vertex cache for a maximum of "nr_verts" new
 * vertices.  Flush primitive and/or vertex queues if necessary to
 * make space.
 */
static struct prim_header *get_queued_prim( struct draw_context *draw,
					    unsigned nr_verts )
{
   if (!draw_vertex_cache_check_space( draw, nr_verts )) {
//      debug_printf("v");
      draw_do_flush( draw, DRAW_FLUSH_VERTEX_CACHE );
   }
   else if (draw->pq.queue_nr == PRIM_QUEUE_LENGTH) {
//      debug_printf("p");
      draw_do_flush( draw, DRAW_FLUSH_PRIM_QUEUE );
   }

   assert(draw->pq.queue_nr < PRIM_QUEUE_LENGTH);

   return &draw->pq.queue[draw->pq.queue_nr++];
}



/**
 * Add a point to the primitive queue.
 * \param i0  index into user's vertex arrays
 */
static void do_point( struct draw_context *draw,
		      unsigned i0 )
{
   struct prim_header *prim = get_queued_prim( draw, 1 );
   
   prim->reset_line_stipple = 0;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->vcache.get_vertex( draw, i0 );
}


/**
 * Add a line to the primitive queue.
 * \param i0  index into user's vertex arrays
 * \param i1  index into user's vertex arrays
 */
static void do_line( struct draw_context *draw,
		     boolean reset_stipple,
		     unsigned i0,
		     unsigned i1 )
{
   struct prim_header *prim = get_queued_prim( draw, 2 );
   
   prim->reset_line_stipple = reset_stipple;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->vcache.get_vertex( draw, i0 );
   prim->v[1] = draw->vcache.get_vertex( draw, i1 );
}

/**
 * Add a triangle to the primitive queue.
 */
static void do_triangle( struct draw_context *draw,
			 unsigned i0,
			 unsigned i1,
			 unsigned i2 )
{
   struct prim_header *prim = get_queued_prim( draw, 3 );
   
//   _mesa_printf("tri %d %d %d\n", i0, i1, i2);
   prim->reset_line_stipple = 1;
   prim->edgeflags = ~0;
   prim->pad = 0;
   prim->v[0] = draw->vcache.get_vertex( draw, i0 );
   prim->v[1] = draw->vcache.get_vertex( draw, i1 );
   prim->v[2] = draw->vcache.get_vertex( draw, i2 );
}
			  
static void do_ef_triangle( struct draw_context *draw,
			    boolean reset_stipple,
			    unsigned ef_mask,
			    unsigned i0,
			    unsigned i1,
			    unsigned i2 )
{
   struct prim_header *prim = get_queued_prim( draw, 3 );
   struct vertex_header *v0 = draw->vcache.get_vertex( draw, i0 );
   struct vertex_header *v1 = draw->vcache.get_vertex( draw, i1 );
   struct vertex_header *v2 = draw->vcache.get_vertex( draw, i2 );

   prim->reset_line_stipple = reset_stipple;

   prim->edgeflags = ef_mask & ((v0->edgeflag << 0) | 
				(v1->edgeflag << 1) | 
				(v2->edgeflag << 2));
   prim->pad = 0;
   prim->v[0] = v0;
   prim->v[1] = v1;
   prim->v[2] = v2;
}


static void do_ef_quad( struct draw_context *draw,
		     unsigned v0,
		     unsigned v1,
		     unsigned v2,
		     unsigned v3 )
{
   const unsigned omitEdge2 = ~(1 << 1);
   const unsigned omitEdge3 = ~(1 << 2);
   do_ef_triangle( draw, 1, omitEdge2, v0, v1, v3 );
   do_ef_triangle( draw, 0, omitEdge3, v1, v2, v3 );
}

static void do_quad( struct draw_context *draw,
		     unsigned v0,
		     unsigned v1,
		     unsigned v2,
		     unsigned v3 )
{
   do_triangle( draw, v0, v1, v3 );
   do_triangle( draw, v1, v2, v3 );
}


/**
 * Main entrypoint to draw some number of points/lines/triangles
 */
static void
draw_prim( struct draw_context *draw, 
	   unsigned prim, unsigned start, unsigned count )
{
   unsigned i;
   boolean unfilled = (draw->rasterizer->fill_cw != PIPE_POLYGON_MODE_FILL ||
		       draw->rasterizer->fill_ccw != PIPE_POLYGON_MODE_FILL);

//   debug_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count );

   switch (prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
	 do_point( draw,
		   start + i );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
	 do_line( draw, 
		  TRUE,
		  start + i + 0,
		  start + i + 1);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:  
      if (count >= 2) {
	 for (i = 1; i < count; i++) {
	    do_line( draw, 
		     i == 1, 	/* XXX: only if vb not split */
		     start + i - 1,
		     start + i );
	 }

	 do_line( draw, 
		  0,
		  start + count - 1,
		  start + 0 );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < count; i++) {
	 do_line( draw,
		  i == 1,
		  start + i - 1,
		  start + i );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      if (unfilled) {
	 for (i = 0; i+2 < count; i += 3) {
	    do_ef_triangle( draw,
			    1, 
			    ~0,
			    start + i + 0,
			    start + i + 1,
			    start + i + 2 );
	 }
      } 
      else {
	 for (i = 0; i+2 < count; i += 3) {
	    do_triangle( draw,
			 start + i + 0,
			 start + i + 1,
			 start + i + 2 );
	 }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      for (i = 0; i+2 < count; i++) {
	 if (i & 1) {
	    do_triangle( draw,
			 start + i + 1,
			 start + i + 0,
			 start + i + 2 );
	 }
	 else {
	    do_triangle( draw,
			 start + i + 0,
			 start + i + 1,
			 start + i + 2 );
	 }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
	 for (i = 0; i+2 < count; i++) {
	    do_triangle( draw,
			 start + 0,
			 start + i + 1,
			 start + i + 2 );
	 }
      }
      break;


   case PIPE_PRIM_QUADS:
      if (unfilled) {
	 for (i = 0; i+3 < count; i += 4) {
	    do_ef_quad( draw,
			start + i + 0,
			start + i + 1,
			start + i + 2,
			start + i + 3);
	 }
      }
      else {
	 for (i = 0; i+3 < count; i += 4) {
	    do_quad( draw,
		     start + i + 0,
		     start + i + 1,
		     start + i + 2,
		     start + i + 3);
	 }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (unfilled) {
	 for (i = 0; i+3 < count; i += 2) {
	    do_ef_quad( draw,
			start + i + 2,
			start + i + 0,
			start + i + 1,
			start + i + 3);
	 }
      }
      else {
	 for (i = 0; i+3 < count; i += 2) {
	    do_quad( draw,
		     start + i + 2,
		     start + i + 0,
		     start + i + 1,
		     start + i + 3);
	 }
      }
      break;

   case PIPE_PRIM_POLYGON:
      if (unfilled) {
	 unsigned ef_mask = (1<<2) | (1<<0);

	 for (i = 0; i+2 < count; i++) {

            if (i + 3 >= count)
	       ef_mask |= (1<<1);

	    do_ef_triangle( draw,
			    i == 0,
			    ef_mask,
			    start + i + 1,
			    start + i + 2,
			    start + 0);

	    ef_mask &= ~(1<<2);
	 }
      }
      else {
	 for (i = 0; i+2 < count; i++) {
	    do_triangle( draw,
			 start + i + 1,
			 start + i + 2,
			 start + 0);
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
}




/**
 * Draw vertex arrays
 * This is the main entrypoint into the drawing module.
 * \param prim  one of PIPE_PRIM_x
 * \param start  index of first vertex to draw
 * \param count  number of vertices to draw
 */
void
draw_arrays(struct draw_context *draw, unsigned prim,
            unsigned start, unsigned count)
{
   if (reduced_prim[prim] != draw->reduced_prim) {
      draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
      draw->reduced_prim = reduced_prim[prim];
   }

   /* drawing done here: */
   if (!draw->rasterizer->bypass_vs ||
       !draw_pt_arrays(draw, prim, start, count)) {
      /* we have to run the whole pipeline */
      draw_prim(draw, prim, start, count);
   }
}


