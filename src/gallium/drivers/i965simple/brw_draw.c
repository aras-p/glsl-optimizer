/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <stdlib.h>

#include "brw_batch.h"
#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "pipe/p_context.h"
#include "pipe/p_winsys.h"

static unsigned hw_prim[PIPE_PRIM_POLYGON+1] = {
   _3DPRIM_POINTLIST,
   _3DPRIM_LINELIST,
   _3DPRIM_LINELOOP,
   _3DPRIM_LINESTRIP,
   _3DPRIM_TRILIST,
   _3DPRIM_TRISTRIP,
   _3DPRIM_TRIFAN,
   _3DPRIM_QUADLIST,
   _3DPRIM_QUADSTRIP,
   _3DPRIM_POLYGON
};


static const int reduced_prim[PIPE_PRIM_POLYGON+1] = {
   PIPE_PRIM_POINTS,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES
};


/* When the primitive changes, set a state bit and re-validate.  Not
 * the nicest and would rather deal with this by having all the
 * programs be immune to the active primitive (ie. cope with all
 * possibilities).  That may not be realistic however.
 */
static void brw_set_prim(struct brw_context *brw, int prim)
{
   PRINT("PRIM: %d\n", prim);

   /* Slight optimization to avoid the GS program when not needed:
    */
   if (prim == PIPE_PRIM_QUAD_STRIP &&
       brw->attribs.Raster->flatshade &&
       brw->attribs.Raster->fill_cw == PIPE_POLYGON_MODE_FILL &&
       brw->attribs.Raster->fill_ccw == PIPE_POLYGON_MODE_FILL)
      prim = PIPE_PRIM_TRIANGLE_STRIP;

   if (prim != brw->primitive) {
      brw->primitive = prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[prim] != brw->reduced_primitive) {
	 brw->reduced_primitive = reduced_prim[prim];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }

      brw_validate_state(brw);
   }

}


static unsigned trim(int prim, unsigned length)
{
   if (prim == PIPE_PRIM_QUAD_STRIP)
      return length > 3 ? (length - length % 2) : 0;
   else if (prim == PIPE_PRIM_QUADS)
      return length - length % 4;
   else
      return length;
}



static boolean brw_emit_prim( struct brw_context *brw,
			      boolean indexed,
			      unsigned start,
			      unsigned count )

{
   struct brw_3d_primitive prim_packet;

   if (BRW_DEBUG & DEBUG_PRIMS)
      PRINT("PRIM: %d %d %d\n",  brw->primitive, start, count);

   prim_packet.header.opcode = CMD_3D_PRIM;
   prim_packet.header.length = sizeof(prim_packet)/4 - 2;
   prim_packet.header.pad = 0;
   prim_packet.header.topology = hw_prim[brw->primitive];
   prim_packet.header.indexed = indexed;

   prim_packet.verts_per_instance = trim(brw->primitive, count);
   prim_packet.start_vert_location = start;
   prim_packet.instance_count = 1;
   prim_packet.start_instance_location = 0;
   prim_packet.base_vert_location = 0;

   if (prim_packet.verts_per_instance == 0)
      return TRUE;

   return brw_batchbuffer_data( brw->winsys,
                                &prim_packet,
                                sizeof(prim_packet) );
}


/* May fail if out of video memory for texture or vbo upload, or on
 * fallback conditions.
 */
static boolean brw_try_draw_elements( struct pipe_context *pipe,
				      struct pipe_buffer *index_buffer,
				      unsigned index_size,
				      unsigned mode,
				      unsigned start,
				      unsigned count )
{
   struct brw_context *brw = brw_context(pipe);

   /* Set the first primitive ahead of validate_state:
    */
   brw_set_prim(brw, mode);

   /* Upload index, vertex data:
    */
   if (index_buffer &&
       !brw_upload_indices( brw, index_buffer, index_size, start, count ))
      return FALSE;

   if (!brw_upload_vertex_buffers(brw))
      return FALSE;

   if (!brw_upload_vertex_elements( brw ))
      return FALSE;

   /* XXX:  Need to separate validate and upload of state.
    */
   if (brw->state.dirty.brw)
      brw_validate_state( brw );

   if (!brw_emit_prim(brw, index_buffer != NULL,
                      start, count))
      return FALSE;

   return TRUE;
}



static boolean brw_draw_elements( struct pipe_context *pipe,
				  struct pipe_buffer *indexBuffer,
				  unsigned indexSize,
				  unsigned mode,
				  unsigned start,
				  unsigned count )
{
   if (!brw_try_draw_elements( pipe,
			       indexBuffer,
			       indexSize,
			       mode, start, count ))
   {
      /* flush ? */

      if (!brw_try_draw_elements( pipe,
				  indexBuffer,
				  indexSize,
				  mode, start,
				  count )) {
	 assert(0);
	 return FALSE;
      }
   }

   return TRUE;
}



static boolean brw_draw_arrays( struct pipe_context *pipe,
				    unsigned mode,
				    unsigned start,
				    unsigned count )
{
   if (!brw_try_draw_elements( pipe, NULL, 0, mode, start, count )) {
      /* flush ? */

      if (!brw_try_draw_elements( pipe, NULL, 0, mode, start, count )) {
	 assert(0);
	 return FALSE;
      }
   }
   
   return TRUE;
}



void brw_init_draw_functions( struct brw_context *brw )
{
   brw->pipe.draw_arrays = brw_draw_arrays;
   brw->pipe.draw_elements = brw_draw_elements;
}


