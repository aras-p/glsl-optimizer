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


#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_debug.h"

#include "brw_batchbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_BATCH

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
static GLuint brw_set_prim(struct brw_context *brw, unsigned prim)
{

   if (BRW_DEBUG & DEBUG_PRIMS)
      debug_printf("PRIM: %s\n", u_prim_name(prim));
   
   if (prim != brw->primitive) {
      brw->primitive = prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[prim] != brw->reduced_primitive) {
	 brw->reduced_primitive = reduced_prim[prim];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }
   }

   return prim_to_hw_prim[prim];
}



static enum pipe_error brw_emit_prim(struct brw_context *brw,
				     unsigned prim,
				     unsigned start,
				     unsigned count,
				     boolean indexed,
				     uint32_t hw_prim)
{
   struct brw_3d_primitive prim_packet;

   if (INTEL_DEBUG & DEBUG_PRIMS)
      debug_printf("PRIM: %s %d %d\n", u_prim_name(prim), start, count);

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
   prim_packet.base_vert_location = prim->basevertex;


   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (intel->always_flush_cache) {
      BEGIN_BATCH(1, IGNORE_CLIPRECTS)
      OUT_BATCH(intel->vtbl.flush_cmd());
      ADVANCE_BATCH();
   }
   if (prim_packet.verts_per_instance) {
      ret = brw_batchbuffer_data( brw->intel.batch, &prim_packet,
				  sizeof(prim_packet), LOOP_CLIPRECTS);
      if (ret)
	 return ret;
   }
   if (intel->always_flush_cache) {
      BEGIN_BATCH(1, IGNORE_CLIPRECTS);
      OUT_BATCH(intel->vtbl.flush_cmd());
      ADVANCE_BATCH();
   }

   return 0;
}


/* May fail if out of video memory for texture or vbo upload, or on
 * fallback conditions.
 */
static GLboolean brw_try_draw_prims( struct brw_context *brw,
				     const struct gl_client_array *arrays[],
				     const struct _mesa_prim *prim,
				     GLuint nr_prims,
				     const struct _mesa_index_buffer *ib,
				     GLuint min_index,
				     GLuint max_index )
{
   struct brw_context *brw = brw_context(ctx);
   GLboolean retval = GL_FALSE;
   GLboolean warn = GL_FALSE;
   GLboolean first_time = GL_TRUE;
   uint32_t hw_prim;
   GLuint i;

   if (ctx->NewState)
      _mesa_update_state( ctx );

   /* Bind all inputs, derive varying and size information:
    */
   brw_merge_inputs( brw, arrays );

   brw->ib.ib = ib;
   brw->state.dirty.brw |= BRW_NEW_INDICES;

   brw->vb.min_index = min_index;
   brw->vb.max_index = max_index;
   brw->state.dirty.brw |= BRW_NEW_VERTICES;

   hw_prim = brw_set_prim(brw, prim[i].mode);

   brw_validate_state(brw);

   /* Check that we can fit our state in with our existing batchbuffer, or
    * flush otherwise.
    */
   ret = dri_bufmgr_check_aperture_space(brw->state.validated_bos,
					 brw->state.validated_bo_count);
   if (ret)
      return ret;

   ret = brw_upload_state(brw);
   if (ret)
      return ret;
   
   ret = brw_emit_prim(brw, &prim[i], hw_prim);
   if (ret)
      return ret;

   if (intel->always_flush_batch)
      brw_batchbuffer_flush(intel->batch);

   return 0;
}


static boolean
brw_draw_range_elements(struct pipe_context *pipe,
			struct pipe_buffer *index_buffer,
			unsigned index_size,
			unsigned min_index,
			unsigned max_index,
			unsigned mode, unsigned start, unsigned count)
{
   enum pipe_error ret;

   if (!vbo_all_varyings_in_vbos(arrays)) {
      if (!index_bounds_valid)
	 vbo_get_minmax_index(ctx, prim, ib, &min_index, &max_index);
   }

   /* Make a first attempt at drawing:
    */
   ret = brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);

   /* Otherwise, flush and retry:
    */
   if (ret != 0) {
      brw_batchbuffer_flush(intel->batch);
      ret = brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
      assert(ret == 0);
   }

   return TRUE;
}

static boolean
brw_draw_elements(struct pipe_context *pipe,
		  struct pipe_buffer *index_buffer,
		  unsigned index_size,
		  unsigned mode, 
		  unsigned start, unsigned count)
{
   return brw_draw_range_elements( pipe, index_buffer,
				   index_size,
				   0, 0xffffffff,
				   mode, 
				   start, count );
}

static boolean
brw_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   return brw_draw_elements(pipe, NULL, 0, mode, start, count);
}



void brw_draw_init( struct brw_context *brw )
{
   /* Register our drawing function: 
    */
   brw->base.draw_arrays = brw_draw_arrays;
   brw->base.draw_elements = brw_draw_elements;
   brw->base.draw_range_elements = brw_draw_range_elements;
}

void brw_draw_destroy( struct brw_context *brw )
{
   int i;

   if (brw->vb.upload.bo != NULL) {
      brw->sws->bo_unreference(brw->vb.upload.bo);
      brw->vb.upload.bo = NULL;
   }

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      brw->sws->bo_unreference(brw->vb.inputs[i].bo);
      brw->vb.inputs[i].bo = NULL;
   }

   brw->sws->bo_unreference(brw->ib.bo);
   brw->ib.bo = NULL;
}
