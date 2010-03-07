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


#include "util/u_inlines.h"
#include "util/u_prim.h"
#include "util/u_upload_mgr.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_debug.h"

#include "brw_batchbuffer.h"


static uint32_t prim_to_hw_prim[PIPE_PRIM_POLYGON+1] = {
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



/* When the primitive changes, set a state bit and re-validate.  Not
 * the nicest and would rather deal with this by having all the
 * programs be immune to the active primitive (ie. cope with all
 * possibilities).  That may not be realistic however.
 */
static int brw_set_prim(struct brw_context *brw, unsigned prim )
{

   if (BRW_DEBUG & DEBUG_PRIMS)
      debug_printf("PRIM: %s\n", u_prim_name(prim));
   
   if (prim != brw->primitive) {
      unsigned reduced_prim;

      brw->primitive = prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      reduced_prim = u_reduced_prim(prim);
      if (reduced_prim != brw->reduced_primitive) {
	 brw->reduced_primitive = reduced_prim;
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }
   }

   return prim_to_hw_prim[prim];
}



static int brw_emit_prim(struct brw_context *brw,
			 unsigned start,
			 unsigned count,
			 boolean indexed,
			 uint32_t hw_prim)
{
   struct brw_3d_primitive prim_packet;
   int ret;

   if (BRW_DEBUG & DEBUG_PRIMS)
      debug_printf("%s start %d count %d indexed %d hw_prim %d\n",
                   __FUNCTION__, start, count, indexed, hw_prim); 

   prim_packet.header.opcode = CMD_3D_PRIM;
   prim_packet.header.length = sizeof(prim_packet)/4 - 2;
   prim_packet.header.pad = 0;
   prim_packet.header.topology = hw_prim;
   prim_packet.header.indexed = indexed;

   prim_packet.verts_per_instance = count;
   prim_packet.start_vert_location = start;
   if (indexed)
      prim_packet.start_vert_location += brw->ib.start_vertex_offset;
   prim_packet.instance_count = 1;
   prim_packet.start_instance_location = 0;
   prim_packet.base_vert_location = 0; /* prim->basevertex; XXX: add this to gallium */


   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (0) {
      BEGIN_BATCH(1, IGNORE_CLIPRECTS);
      OUT_BATCH((CMD_MI_FLUSH << 16) | BRW_FLUSH_STATE_CACHE);
      ADVANCE_BATCH();
   }
   if (prim_packet.verts_per_instance) {
      ret = brw_batchbuffer_data( brw->batch, &prim_packet,
				  sizeof(prim_packet), LOOP_CLIPRECTS);
      if (ret)
	 return ret;
   }
   if (0) {
      BEGIN_BATCH(1, IGNORE_CLIPRECTS);
      OUT_BATCH((CMD_MI_FLUSH << 16) | BRW_FLUSH_STATE_CACHE);
      ADVANCE_BATCH();
   }

   return 0;
}


/* May fail if out of video memory for texture or vbo upload, or on
 * fallback conditions.
 */
static int
try_draw_range_elements(struct brw_context *brw,
			struct pipe_buffer *index_buffer,
			unsigned hw_prim, 
			unsigned start, unsigned count)
{
   int ret;

   ret = brw_validate_state(brw);
   if (ret)
      return ret;

   /* Check that we can fit our state in with our existing batchbuffer, or
    * flush otherwise.
    */
   ret = brw->sws->check_aperture_space(brw->sws,
					brw->state.validated_bos,
					brw->state.validated_bo_count);
   if (ret)
      return ret;

   ret = brw_upload_state(brw);
   if (ret)
      return ret;
   
   ret = brw_emit_prim(brw, start, count, index_buffer != NULL, hw_prim);
   if (ret)
      return ret;

   if (brw->flags.always_flush_batch)
      brw_context_flush( brw );

   return 0;
}


static void
brw_draw_range_elements(struct pipe_context *pipe,
			struct pipe_buffer *index_buffer,
			unsigned index_size,
			unsigned min_index,
			unsigned max_index,
			unsigned mode, unsigned start, unsigned count)
{
   struct brw_context *brw = brw_context(pipe);
   int ret;
   uint32_t hw_prim;

   hw_prim = brw_set_prim(brw, mode);

   if (BRW_DEBUG & DEBUG_PRIMS)
      debug_printf("PRIM: %s start %d count %d index_buffer %p\n",
                   u_prim_name(mode), start, count, (void *)index_buffer);

   /* Potentially trigger upload of new index buffer.
    *
    * XXX: do we need to go through state validation to achieve this?
    * Could just call upload code directly.
    */
   if (brw->curr.index_buffer != index_buffer ||
       brw->curr.index_size != index_size) {
      pipe_buffer_reference( &brw->curr.index_buffer, index_buffer );
      brw->curr.index_size = index_size;
      brw->state.dirty.mesa |= PIPE_NEW_INDEX_BUFFER;
   }

   /* XXX: do we really care?
    */
   if (brw->curr.min_index != min_index ||
       brw->curr.max_index != max_index) 
   { 
      brw->curr.min_index = min_index;
      brw->curr.max_index = max_index;
      brw->state.dirty.mesa |= PIPE_NEW_INDEX_RANGE;
   }


   /* Make a first attempt at drawing:
    */
   ret = try_draw_range_elements(brw, index_buffer, hw_prim, start, count );

   /* Otherwise, flush and retry:
    */
   if (ret != 0) {
      brw_context_flush( brw );
      ret = try_draw_range_elements(brw, index_buffer, hw_prim, start, count );
      assert(ret == 0);
   }
}

static void
brw_draw_elements(struct pipe_context *pipe,
		  struct pipe_buffer *index_buffer,
		  unsigned index_size,
		  unsigned mode, 
		  unsigned start, unsigned count)
{
   brw_draw_range_elements( pipe, index_buffer,
                            index_size,
                            0, 0xffffffff,
                            mode, 
                            start, count );
}

static void
brw_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   brw_draw_elements(pipe, NULL, 0, mode, start, count);
}



boolean brw_draw_init( struct brw_context *brw )
{
   /* Register our drawing function: 
    */
   brw->base.draw_arrays = brw_draw_arrays;
   brw->base.draw_elements = brw_draw_elements;
   brw->base.draw_range_elements = brw_draw_range_elements;

   /* Create helpers for uploading data in user buffers:
    */
   brw->vb.upload_vertex = u_upload_create( brw->base.screen,
					    128 * 1024,
					    64,
					    PIPE_BUFFER_USAGE_VERTEX );
   if (brw->vb.upload_vertex == NULL)
      return FALSE;

   brw->vb.upload_index = u_upload_create( brw->base.screen,
					   32 * 1024,
					   64,
					   PIPE_BUFFER_USAGE_INDEX );
   if (brw->vb.upload_index == NULL)
      return FALSE;

   return TRUE;
}

void brw_draw_cleanup( struct brw_context *brw )
{
   u_upload_destroy( brw->vb.upload_vertex );
   u_upload_destroy( brw->vb.upload_index );

   bo_reference(&brw->ib.bo, NULL);
}
