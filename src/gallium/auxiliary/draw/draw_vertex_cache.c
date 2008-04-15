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


void draw_vertex_cache_invalidate( struct draw_context *draw )
{
   assert(draw->pq.queue_nr == 0);
   assert(draw->vs.queue_nr == 0);
   assert(draw->vcache.referenced == 0);

   /* There's an error somewhere in the vcache code that requires this
    * memset.  The bug is exposed in q3demo demo001, but probably
    * elsewhere as well.  Will track it down later.
    */
   memset(draw->vcache.idx, ~0, sizeof(draw->vcache.idx));
}


/**
 * Check if vertex is in cache, otherwise add it.  It won't go through
 * VS yet, not until there is a flush operation or the VS queue fills up.  
 *
 * Note that cache entries are basically just two pointers: the first
 * an index into the user's vertex arrays, the second a location in
 * the vertex shader cache for the post-transformed vertex.
 *
 * \return pointer to location of (post-transformed) vertex header in the cache
 */
static struct vertex_header *get_vertex( struct draw_context *draw,
					 unsigned i )
{
   unsigned slot = (i + (i>>5)) % VCACHE_SIZE;
   
   assert(slot < 32); /* so we don't exceed the bitfield size below */

   if (draw->vcache.referenced & (1<<slot))
   {
      /* Cache hit?
       */
      if (draw->vcache.idx[slot].in == i) {
	 /*debug_printf("HIT %d %d\n", slot, i);*/
	 assert(draw->vcache.idx[slot].out < draw->vs.queue_nr);
	 return draw_header_from_block(draw->vs.vertex_cache,
                                       MAX_VERTEX_ALLOCATION,
                                       draw->vcache.idx[slot].out);
      }

      /* Otherwise a collision
       */
      slot = VCACHE_SIZE + draw->vcache.overflow++;
      /*debug_printf("XXX %d --> %d\n", i, slot);*/
   }

   /* Deal with the cache miss: 
    */
   {
      unsigned out;
      struct vertex_header *header;

      assert(slot < Elements(draw->vcache.idx));

      /*debug_printf("NEW %d %d\n", slot, i);*/
      draw->vcache.idx[slot].in = i;
      draw->vcache.idx[slot].out = out = draw->vs.queue_nr++;
      draw->vcache.referenced |= (1 << slot);


      /* Add to vertex shader queue:
       */
      assert(draw->vs.queue_nr < VS_QUEUE_LENGTH);

      header = draw_header_from_block(draw->vs.vertex_cache, MAX_VERTEX_ALLOCATION,
                                      out);
      draw->vs.elts[out] = i;
      header->clipmask = 0;
      header->edgeflag = draw_get_edgeflag(draw, i);
      header->pad = 0;
      header->vertex_id = UNDEFINED_VERTEX_ID;

      /* Need to set the vertex's edge flag here.  If we're being called
       * by do_ef_triangle(), that function needs edge flag info!
       */

      return draw_header_from_block(draw->vs.vertex_cache,
                                    MAX_VERTEX_ALLOCATION,
                                    draw->vcache.idx[slot].out);
   }
}


static struct vertex_header *get_uint_elt_vertex( struct draw_context *draw,
                                                  unsigned i )
{
   const unsigned *elts = (const unsigned *) draw->user.elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ushort_elt_vertex( struct draw_context *draw,
						    unsigned i )
{
   const ushort *elts = (const ushort *) draw->user.elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ubyte_elt_vertex( struct draw_context *draw,
                                                   unsigned i )
{
   const ubyte *elts = (const ubyte *) draw->user.elts;
   return get_vertex( draw, elts[i] );
}


void draw_vertex_cache_reset_vertex_ids( struct draw_context *draw )
{
   unsigned i;

   for (i = 0; i < draw->vs.post_nr; i++) {
      struct vertex_header * header =
         draw_header_from_block(draw->vs.vertex_cache,
                                MAX_VERTEX_ALLOCATION, i);
      header->vertex_id = UNDEFINED_VERTEX_ID;
   }
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
//   draw_statechange( draw );

   /* choose the get_vertex() function to use */
   switch (eltSize) {
   case 0:
      draw->vcache.get_vertex = get_vertex;
      break;
   case 1:
      draw->vcache.get_vertex = get_ubyte_elt_vertex;
      break;
   case 2:
      draw->vcache.get_vertex = get_ushort_elt_vertex;
      break;
   case 4:
      draw->vcache.get_vertex = get_uint_elt_vertex;
      break;
   default:
      assert(0);
   }
   draw->user.elts = elements;
   draw->user.eltSize = eltSize;
}

