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

#include "brw_batchbuffer.h"
#include "intel_buffer_objects.h"

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
static GLuint brw_set_prim(struct brw_context *brw, GLenum prim)
{

   if (INTEL_DEBUG & DEBUG_PRIMS)
      _mesa_printf("PRIM: %s\n", _mesa_lookup_enum_by_nr(prim));
   
   /* Slight optimization to avoid the GS program when not needed:
    */
   if (prim == GL_QUAD_STRIP &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL)
      prim = GL_TRIANGLE_STRIP;

   if (prim != brw->primitive) {
      brw->primitive = prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[prim] != brw->intel.reduced_primitive) {
	 brw->intel.reduced_primitive = reduced_prim[prim];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }
   }

   return prim_to_hw_prim[prim];
}


static GLuint trim(GLenum prim, GLuint length)
{
   if (prim == GL_QUAD_STRIP)
      return length > 3 ? (length - length % 2) : 0;
   else if (prim == GL_QUADS)
      return length - length % 4;
   else 
      return length;
}


static void brw_emit_prim(struct brw_context *brw,
			  const struct _mesa_prim *prim,
			  uint32_t hw_prim)
{
   struct brw_3d_primitive prim_packet;

   if (INTEL_DEBUG & DEBUG_PRIMS)
      _mesa_printf("PRIM: %s %d %d\n", _mesa_lookup_enum_by_nr(prim->mode), 
		   prim->start, prim->count);

   prim_packet.header.opcode = CMD_3D_PRIM;
   prim_packet.header.length = sizeof(prim_packet)/4 - 2;
   prim_packet.header.pad = 0;
   prim_packet.header.topology = hw_prim;
   prim_packet.header.indexed = prim->indexed;

   prim_packet.verts_per_instance = trim(prim->mode, prim->count);
   prim_packet.start_vert_location = prim->start;
   if (prim->indexed)
      prim_packet.start_vert_location += brw->ib.start_vertex_offset;
   prim_packet.instance_count = 1;
   prim_packet.start_instance_location = 0;
   prim_packet.base_vert_location = prim->basevertex;

   /* Can't wrap here, since we rely on the validated state. */
   brw->no_batch_wrap = GL_TRUE;

   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (intel->always_flush_cache) {
      BEGIN_BATCH(1, IGNORE_CLIPRECTS);
      OUT_BATCH(intel->vtbl.flush_cmd());
      ADVANCE_BATCH();
   }
   if (prim_packet.verts_per_instance) {
      brw_batchbuffer_data( brw->intel.batch, &prim_packet,
			      sizeof(prim_packet), LOOP_CLIPRECTS);
   }
   if (intel->always_flush_cache) {
      BEGIN_BATCH(1, IGNORE_CLIPRECTS);
      OUT_BATCH(intel->vtbl.flush_cmd());
      ADVANCE_BATCH();
   }

   brw->no_batch_wrap = GL_FALSE;
}

static void brw_merge_inputs( struct brw_context *brw,
		       const struct gl_client_array *arrays[])
{
   struct brw_vertex_info old = brw->vb.info;
   GLuint i;

   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      brw->sws->bo_unreference(brw->vb.inputs[i].bo);

   memset(&brw->vb.inputs, 0, sizeof(brw->vb.inputs));
   memset(&brw->vb.info, 0, sizeof(brw->vb.info));

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      brw->vb.inputs[i].glarray = arrays[i];
      brw->vb.inputs[i].attrib = (gl_vert_attrib) i;

      if (arrays[i]->StrideB != 0)
	 brw->vb.info.sizes[i/16] |= (brw->vb.inputs[i].glarray->Size - 1) <<
	    ((i%16) * 2);
   }

   /* Raise statechanges if input sizes have changed. */
   if (memcmp(brw->vb.info.sizes, old.sizes, sizeof(old.sizes)) != 0)
      brw->state.dirty.brw |= BRW_NEW_INPUT_DIMENSIONS;
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

void brw_draw_prims( struct brw_context *brw,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     GLboolean index_bounds_valid,
		     GLuint min_index,
		     GLuint max_index )
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
}

void brw_draw_init( struct brw_context *brw )
{
   struct vbo_context *vbo = vbo_context(ctx);

   /* Register our drawing function: 
    */
   vbo->draw_prims = brw_draw_prims;
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
