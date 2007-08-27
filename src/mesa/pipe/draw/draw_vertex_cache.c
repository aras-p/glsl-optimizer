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
#include "draw_vertex.h"


void draw_vertex_cache_invalidate( struct draw_context *draw )
{
   unsigned i;

   assert(draw->pq.queue_nr == 0);
   assert(draw->vs.queue_nr == 0);
   assert(draw->vcache.referenced == 0);
   
   for (i = 0; i < Elements( draw->vcache.idx ); i++)
      draw->vcache.idx[i] = ~0;
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
      else
         draw->vcache.referenced |= (1 << slot);  /* slot now in use */

      draw->vcache.idx[slot] = i;

      /* Add to vertex shader queue:
       */
      draw->vs.queue[draw->vs.queue_nr].dest = draw->vcache.vertex[slot];
      draw->vs.queue[draw->vs.queue_nr].elt = i;
      draw->vs.queue_nr++;

      /* Need to set the vertex's edge flag here.  If we're being called
       * by do_ef_triangle(), that function needs edge flag info!
       */
      draw->vcache.vertex[slot]->edgeflag = 1; /*XXX use user's edge flag! */
   }

   return draw->vcache.vertex[slot];
}


static struct vertex_header *get_uint_elt_vertex( struct draw_context *draw,
                                                  unsigned i )
{
   const unsigned *elts = (const unsigned *) draw->mapped_elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ushort_elt_vertex( struct draw_context *draw,
						    unsigned i )
{
   const ushort *elts = (const ushort *) draw->mapped_elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ubyte_elt_vertex( struct draw_context *draw,
                                                   unsigned i )
{
   const ubyte *elts = (const ubyte *) draw->mapped_elts;
   return get_vertex( draw, elts[i] );
}



void draw_vertex_cache_validate( struct draw_context *draw )
{
   draw_vertex_shader_queue_flush( draw );
}

void draw_vertex_cache_unreference( struct draw_context *draw )
{
   draw->vcache.referenced = 0;
   draw->vcache.overflow = 0;
}


int draw_vertex_cache_check_space( struct draw_context *draw,
				    unsigned nr_verts )
{
   if (draw->vcache.overflow + nr_verts < VCACHE_OVERFLOW) {
      /* The vs queue is sized so that this can never happen:
       */
      assert(draw->vs.queue_nr + nr_verts < VS_QUEUE_LENGTH);
      return TRUE;
   }
   else
      return FALSE;
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
draw_set_mapped_element_buffer( struct draw_context *draw,
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
   draw->mapped_elts = elements;
   draw->eltSize = eltSize;
}

