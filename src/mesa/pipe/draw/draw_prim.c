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
#include "draw_private.h"
#include "draw_context.h"
#include "draw_prim.h"


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


void draw_flush( struct draw_context *draw )
{
   struct draw_stage *first = draw->pipeline.first;
   unsigned i;

   /* Make sure all vertices are available:
    */
   assert(draw->vs_flush);
   draw->vs_flush( draw );


   switch (draw->reduced_prim) {
   case RP_TRI:
      for (i = 0; i < draw->pq.queue_nr; i++) {
	 if (draw->pq.queue[i].reset_line_stipple)
	    first->reset_stipple_counter( first );

	 first->tri( first, &draw->pq.queue[i] );
      }
      break;
   case RP_LINE:
      for (i = 0; i < draw->pq.queue_nr; i++) {
	 if (draw->pq.queue[i].reset_line_stipple)
	    first->reset_stipple_counter( first );

	 first->line( first, &draw->pq.queue[i] );
      }
      break;
   case RP_POINT:
      first->reset_stipple_counter( first );
      for (i = 0; i < draw->pq.queue_nr; i++)
	 first->point( first, &draw->pq.queue[i] );
      break;
   }

   draw->pq.queue_nr = 0;
   draw->vcache.referenced = 0;
   draw->vcache.overflow = 0;
}


void draw_invalidate_vcache( struct draw_context *draw )
{
   unsigned i;

   assert(draw->pq.queue_nr == 0);
   assert(draw->vs.queue_nr == 0);
   assert(draw->vcache.referenced == 0);
   
   for (i = 0; i < Elements( draw->vcache.idx ); i++)
      draw->vcache.idx[i] = ~0;
}


/* Return a pointer to a freshly queued primitive header.  Ensure that
 * there is room in the vertex cache for a maximum of "nr_verts" new
 * vertices.  Flush primitive and/or vertex queues if necessary to
 * make space.
 */
static struct prim_header *get_queued_prim( struct draw_context *draw,
					    unsigned nr_verts )
{
   if (draw->pq.queue_nr + 1 >= PRIM_QUEUE_LENGTH ||
       draw->vcache.overflow + nr_verts >= VCACHE_OVERFLOW) 
      draw_flush( draw );

   /* The vs queue is sized so that this can never happen:
    */
   assert(draw->vs.queue_nr + nr_verts < VS_QUEUE_LENGTH);

   return &draw->pq.queue[draw->pq.queue_nr++];
}


/* Check if vertex is in cache, otherwise add it.  It won't go through
 * VS yet, not until there is a flush operation or the VS queue fills up.  
 */
static struct vertex_header *get_vertex( struct draw_context *draw,
					 unsigned i )
{
   unsigned slot = (i + (i>>5)) & 31;
   
   /* Cache miss?
    */
   if (draw->vcache.idx[slot] != i) {

      /* If slot is in use, use the overflow area:
       */
      if (draw->vcache.referenced & (1 << slot))
	 slot = VCACHE_SIZE + draw->vcache.overflow++;

      draw->vcache.idx[slot] = i;

      /* Add to vertex shader queue:
       */
      draw->vs.queue[draw->vs.queue_nr].dest = draw->vcache.vertex[slot];
      draw->vs.queue[draw->vs.queue_nr].elt = i;
      draw->vs.queue_nr++;
   }

   /* Mark slot as in-use:
    */
   if (slot < VCACHE_SIZE)
      draw->vcache.referenced |= (1 << slot);
   return draw->vcache.vertex[slot];
}


static struct vertex_header *get_uint_elt_vertex( struct draw_context *draw,
                                                  unsigned i )
{
   const unsigned *elts = (const unsigned *)draw->elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ushort_elt_vertex( struct draw_context *draw,
						    unsigned i )
{
   const ushort *elts = (const ushort *)draw->elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ubyte_elt_vertex( struct draw_context *draw,
                                                   unsigned i )
{
   const ubyte *elts = (const ubyte *)draw->elts;
   return get_vertex( draw, elts[i] );
}


static void do_point( struct draw_context *draw,
		      unsigned i0 )
{
   struct prim_header *prim = get_queued_prim( draw, 1 );
   
   prim->reset_line_stipple = 0;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
}


static void do_line( struct draw_context *draw,
		     boolean reset_stipple,
		     unsigned i0,
		     unsigned i1 )
{
   struct prim_header *prim = get_queued_prim( draw, 2 );
   
   prim->reset_line_stipple = reset_stipple;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
   prim->v[1] = draw->get_vertex( draw, i1 );
}

static void do_triangle( struct draw_context *draw,
			 unsigned i0,
			 unsigned i1,
			 unsigned i2 )
{
   struct prim_header *prim = get_queued_prim( draw, 3 );
   
   prim->reset_line_stipple = 1;
   prim->edgeflags = ~0;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
   prim->v[1] = draw->get_vertex( draw, i1 );
   prim->v[2] = draw->get_vertex( draw, i2 );
}
			  
static void do_ef_triangle( struct draw_context *draw,
			    boolean reset_stipple,
			    unsigned ef_mask,
			    unsigned i0,
			    unsigned i1,
			    unsigned i2 )
{
   struct prim_header *prim = get_queued_prim( draw, 3 );
   struct vertex_header *v0 = draw->get_vertex( draw, i0 );
   struct vertex_header *v1 = draw->get_vertex( draw, i1 );
   struct vertex_header *v2 = draw->get_vertex( draw, i2 );
   
   prim->reset_line_stipple = reset_stipple;

   prim->edgeflags = ef_mask & ((v0->edgeflag << 0) | 
				(v1->edgeflag << 1) | 
				(v2->edgeflag << 2));
   prim->pad = 0;
   prim->v[0] = v0;
   prim->v[1] = v1;
   prim->v[2] = v2;
}


static void do_quad( struct draw_context *draw,
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


/**
 * Main entrypoint to draw some number of points/lines/triangles
 */
void
draw_prim( struct draw_context *draw, unsigned start, unsigned count )
{
   unsigned i;

//   _mesa_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count );

   switch (draw->prim) {
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
      if (count >= 2) {
	 for (i = 1; i < count; i++) {
	    do_line( draw,
		     i == 1,
		     start + i - 1,
		     start + i );
	 }
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
	 do_ef_triangle( draw,
			 1, 
			 ~0,
			 start + i + 0,
			 start + i + 1,
			 start + i + 2 );
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
      for (i = 0; i+3 < count; i += 4) {
	 do_quad( draw,
		  start + i + 0,
		  start + i + 1,
		  start + i + 2,
		  start + i + 3);
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
	 do_quad( draw,
		  start + i + 2,
		  start + i + 0,
		  start + i + 1,
		  start + i + 3);
      }
      break;

   case PIPE_PRIM_POLYGON:
      if (count >= 3) {
	 unsigned ef_mask = (1<<2) | (1<<0);

	 for (i = 0; i+2 < count; i++) {

            if (i + 3 >= count)
	       ef_mask |= (1<<1);

	    do_ef_triangle( draw,
			    i == 0,
			    ef_mask,
			    start + i + 1,
			    start + i + 2,
			    start + i + 0);

	    ef_mask &= ~(1<<2);
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
}


void
draw_set_prim( struct draw_context *draw, unsigned prim )
{
   assert(prim >= PIPE_PRIM_POINTS);
   assert(prim <= PIPE_PRIM_POLYGON);

   if (reduced_prim[prim] != draw->reduced_prim) {
      draw_flush( draw );
      draw->reduced_prim = reduced_prim[prim];
   }

   draw->prim = prim;
}


/**
 * Tell the drawing context about the index/element buffer to use
 * (ala glDrawElements)
 * If no element buffer is to be used (i.e. glDrawArrays) then this
 * should be called with eltSize=0 and elements=NULL.
 *
 * \param draw  the drawing context
 * \param eltSize  size of each element (1, 2 or 4 bytes)
 * \param elements  the element buffer ptr
 */
void
draw_set_element_buffer( struct draw_context *draw,
                         unsigned eltSize, void *elements )
{
   /* choose the get_vertex() function to use */
   switch (eltSize) {
   case 0:
      draw->get_vertex = get_vertex;
      break;
   case 1:
      draw->get_vertex = get_ubyte_elt_vertex;
      break;
   case 2:
      draw->get_vertex = get_ushort_elt_vertex;
      break;
   case 4:
      draw->get_vertex = get_uint_elt_vertex;
      break;
   default:
      assert(0);
   }
   draw->elts = elements;
   draw->eltSize = eltSize;


}

unsigned
draw_prim_info(unsigned prim, unsigned *first, unsigned *incr)
{
   assert(prim >= PIPE_PRIM_POINTS);
   assert(prim <= PIPE_PRIM_POLYGON);

   switch (prim) {
   case PIPE_PRIM_POINTS:
      *first = 1;
      *incr = 1;
      return 0;
   case PIPE_PRIM_LINES:
      *first = 2;
      *incr = 2;
      return 0;
   case PIPE_PRIM_LINE_STRIP:
      *first = 2;
      *incr = 1;
      return 0;
   case PIPE_PRIM_LINE_LOOP:
      *first = 2;
      *incr = 1;
      return 1;
   case PIPE_PRIM_TRIANGLES:
      *first = 3;
      *incr = 3;
      return 0;
   case PIPE_PRIM_TRIANGLE_STRIP:
      *first = 3;
      *incr = 1;
      return 0;
   case PIPE_PRIM_TRIANGLE_FAN:
   case PIPE_PRIM_POLYGON:
      *first = 3;
      *incr = 1;
      return 1;
   case PIPE_PRIM_QUADS:
      *first = 4;
      *incr = 4;
      return 0;
   case PIPE_PRIM_QUAD_STRIP:
      *first = 4;
      *incr = 2;
      return 0;
   default:
      assert(0);
      *first = 1;
      *incr = 1;
      return 0;
   }
}


unsigned
draw_trim( unsigned count, unsigned first, unsigned incr )
{
   if (count < first)
      return 0;
   else
      return count - (count - first) % incr; 
}


/**
 * Allocate space for temporary post-transform vertices, such as for clipping.
 */
void draw_alloc_tmps( struct draw_stage *stage, unsigned nr )
{
   stage->nr_tmps = nr;

   if (nr) {
      ubyte *store = (ubyte *) malloc(MAX_VERTEX_SIZE * nr);
      unsigned i;

      stage->tmp = (struct vertex_header **) malloc(sizeof(struct vertex_header *) * nr);
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}

void draw_free_tmps( struct draw_stage *stage )
{
   if (stage->tmp) {
      free(stage->tmp[0]);
      free(stage->tmp);
   }
}
