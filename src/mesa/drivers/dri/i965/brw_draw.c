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


#include "main/glheader.h"
#include "main/context.h"
#include "main/condrender.h"
#include "main/samplerobj.h"
#include "main/state.h"
#include "main/enums.h"
#include "tnl/tnl.h"
#include "vbo/vbo_context.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_PRIMS

static GLuint prim_to_hw_prim[GL_POLYGON+1] = {
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


static const GLenum reduced_prim[GL_POLYGON+1] = {  
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};


/* When the primitive changes, set a state bit and re-validate.  Not
 * the nicest and would rather deal with this by having all the
 * programs be immune to the active primitive (ie. cope with all
 * possibilities).  That may not be realistic however.
 */
static GLuint brw_set_prim(struct brw_context *brw,
			   const struct _mesa_prim *prim)
{
   struct gl_context *ctx = &brw->intel.ctx;
   GLenum mode = prim->mode;

   DBG("PRIM: %s\n", _mesa_lookup_enum_by_nr(prim->mode));

   /* Slight optimization to avoid the GS program when not needed:
    */
   if (mode == GL_QUAD_STRIP &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL)
      mode = GL_TRIANGLE_STRIP;

   if (prim->mode == GL_QUADS && prim->count == 4 &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL) {
      mode = GL_TRIANGLE_FAN;
   }

   if (mode != brw->primitive) {
      brw->primitive = mode;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[mode] != brw->intel.reduced_primitive) {
	 brw->intel.reduced_primitive = reduced_prim[mode];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }
   }

   return prim_to_hw_prim[mode];
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
   struct intel_context *intel = &brw->intel;
   int verts_per_instance;
   int vertex_access_type;
   int start_vertex_location;
   int base_vertex_location;

   DBG("PRIM: %s %d %d\n", _mesa_lookup_enum_by_nr(prim->mode),
       prim->start, prim->count);

   start_vertex_location = prim->start;
   base_vertex_location = prim->basevertex;
   if (prim->indexed) {
      vertex_access_type = GEN4_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM;
      start_vertex_location += brw->ib.start_vertex_offset;
      base_vertex_location += brw->vb.start_vertex_bias;
   } else {
      vertex_access_type = GEN4_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL;
      start_vertex_location += brw->vb.start_vertex_bias;
   }

   verts_per_instance = trim(prim->mode, prim->count);

   /* If nothing to emit, just return. */
   if (verts_per_instance == 0)
      return;

   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (intel->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(intel);
   }

   BEGIN_BATCH(6);
   OUT_BATCH(CMD_3D_PRIM << 16 | (6 - 2) |
	     hw_prim << GEN4_3DPRIM_TOPOLOGY_TYPE_SHIFT |
	     vertex_access_type);
   OUT_BATCH(verts_per_instance);
   OUT_BATCH(start_vertex_location);
   OUT_BATCH(1); // instance count
   OUT_BATCH(0); // start instance location
   OUT_BATCH(base_vertex_location);
   ADVANCE_BATCH();

   intel->batch.need_workaround_flush = true;

   if (intel->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(intel);
   }
}

static void gen7_emit_prim(struct brw_context *brw,
			   const struct _mesa_prim *prim,
			   uint32_t hw_prim)
{
   struct intel_context *intel = &brw->intel;
   int verts_per_instance;
   int vertex_access_type;
   int start_vertex_location;
   int base_vertex_location;

   DBG("PRIM: %s %d %d\n", _mesa_lookup_enum_by_nr(prim->mode),
       prim->start, prim->count);

   start_vertex_location = prim->start;
   base_vertex_location = prim->basevertex;
   if (prim->indexed) {
      vertex_access_type = GEN7_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM;
      start_vertex_location += brw->ib.start_vertex_offset;
      base_vertex_location += brw->vb.start_vertex_bias;
   } else {
      vertex_access_type = GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL;
      start_vertex_location += brw->vb.start_vertex_bias;
   }

   verts_per_instance = trim(prim->mode, prim->count);

   /* If nothing to emit, just return. */
   if (verts_per_instance == 0)
      return;

   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (intel->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(intel);
   }

   BEGIN_BATCH(7);
   OUT_BATCH(CMD_3D_PRIM << 16 | (7 - 2));
   OUT_BATCH(hw_prim | vertex_access_type);
   OUT_BATCH(verts_per_instance);
   OUT_BATCH(start_vertex_location);
   OUT_BATCH(1); // instance count
   OUT_BATCH(0); // start instance location
   OUT_BATCH(base_vertex_location);
   ADVANCE_BATCH();

   if (intel->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(intel);
   }
}


static void brw_merge_inputs( struct brw_context *brw,
		       const struct gl_client_array *arrays[])
{
   struct brw_vertex_info old = brw->vb.info;
   GLuint i;

   for (i = 0; i < brw->vb.nr_buffers; i++) {
      drm_intel_bo_unreference(brw->vb.buffers[i].bo);
      brw->vb.buffers[i].bo = NULL;
   }
   brw->vb.nr_buffers = 0;

   memset(&brw->vb.info, 0, sizeof(brw->vb.info));

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      brw->vb.inputs[i].buffer = -1;
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
static GLboolean brw_try_draw_prims( struct gl_context *ctx,
				     const struct gl_client_array *arrays[],
				     const struct _mesa_prim *prim,
				     GLuint nr_prims,
				     const struct _mesa_index_buffer *ib,
				     GLuint min_index,
				     GLuint max_index )
{
   struct intel_context *intel = intel_context(ctx);
   struct brw_context *brw = brw_context(ctx);
   GLboolean retval = GL_FALSE;
   GLboolean warn = GL_FALSE;
   GLuint i;

   if (ctx->NewState)
      _mesa_update_state( ctx );

   /* We have to validate the textures *before* checking for fallbacks;
    * otherwise, the software fallback won't be able to rely on the
    * texture state, the firstLevel and lastLevel fields won't be
    * set in the intel texture object (they'll both be 0), and the 
    * software fallback will segfault if it attempts to access any
    * texture level other than level 0.
    */
   brw_validate_textures( brw );

   /* Bind all inputs, derive varying and size information:
    */
   brw_merge_inputs( brw, arrays );

   brw->ib.ib = ib;
   brw->state.dirty.brw |= BRW_NEW_INDICES;

   brw->vb.min_index = min_index;
   brw->vb.max_index = max_index;
   brw->state.dirty.brw |= BRW_NEW_VERTICES;

   /* Have to validate state quite late.  Will rebuild tnl_program,
    * which depends on varying information.  
    * 
    * Note this is where brw->vs->prog_data.inputs_read is calculated,
    * so can't access it earlier.
    */

   intel_prepare_render(intel);

   for (i = 0; i < nr_prims; i++) {
      uint32_t hw_prim;
      int estimated_max_prim_size;

      estimated_max_prim_size = 512; /* batchbuffer commands */
      estimated_max_prim_size += (BRW_MAX_TEX_UNIT *
				  (sizeof(struct brw_sampler_state) +
				   sizeof(struct gen5_sampler_default_color)));
      estimated_max_prim_size += 1024; /* gen6 VS push constants */
      estimated_max_prim_size += 1024; /* gen6 WM push constants */
      estimated_max_prim_size += 512; /* misc. pad */

      /* Flush the batch if it's approaching full, so that we don't wrap while
       * we've got validated state that needs to be in the same batch as the
       * primitives.
       */
      intel_batchbuffer_require_space(intel, estimated_max_prim_size, false);

      hw_prim = brw_set_prim(brw, &prim[i]);
      if (brw->state.dirty.brw) {
	 brw_validate_state(brw);

	 /* Various fallback checks:  */
	 if (brw->intel.Fallback)
	    goto out;

	 /* Check that we can fit our state in with our existing batchbuffer, or
	  * flush otherwise.
	  */
	 if (dri_bufmgr_check_aperture_space(brw->state.validated_bos,
					     brw->state.validated_bo_count)) {
	    static GLboolean warned;
	    intel_batchbuffer_flush(intel);

	    /* Validate the state after we flushed the batch (which would have
	     * changed the set of dirty state).  If we still fail to
	     * check_aperture, warn of what's happening, but attempt to continue
	     * on since it may succeed anyway, and the user would probably rather
	     * see a failure and a warning than a fallback.
	     */
	    brw_validate_state(brw);
	    if (!warned &&
		dri_bufmgr_check_aperture_space(brw->state.validated_bos,
						brw->state.validated_bo_count)) {
	       warn = GL_TRUE;
	       warned = GL_TRUE;
	    }
	 }

	 intel->no_batch_wrap = GL_TRUE;
	 brw_upload_state(brw);
      }

      if (intel->gen >= 7)
	 gen7_emit_prim(brw, &prim[i], hw_prim);
      else
	 brw_emit_prim(brw, &prim[i], hw_prim);

      intel->no_batch_wrap = GL_FALSE;

      retval = GL_TRUE;
   }

   if (intel->always_flush_batch)
      intel_batchbuffer_flush(intel);
 out:

   brw_state_cache_check_size(brw);

   if (warn)
      fprintf(stderr, "i965: Single primitive emit potentially exceeded "
	      "available aperture space\n");

   if (!retval)
      DBG("%s failed\n", __FUNCTION__);

   return retval;
}

void brw_draw_prims( struct gl_context *ctx,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     GLboolean index_bounds_valid,
		     GLuint min_index,
		     GLuint max_index )
{
   GLboolean retval;

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (!vbo_all_varyings_in_vbos(arrays)) {
      if (!index_bounds_valid)
	 vbo_get_minmax_index(ctx, prim, ib, &min_index, &max_index);

      /* Decide if we want to rebase.  If so we end up recursing once
       * only into this function.
       */
      if (min_index != 0 && !vbo_any_varyings_in_vbos(arrays)) {
	 vbo_rebase_prims(ctx, arrays,
			  prim, nr_prims,
			  ib, min_index, max_index,
			  brw_draw_prims );
	 return;
      }
   }

   /* Make a first attempt at drawing:
    */
   retval = brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);

   /* Otherwise, we really are out of memory.  Pass the drawing
    * command to the software tnl module and which will in turn call
    * swrast to do the drawing.
    */
   if (!retval) {
       _swsetup_Wakeup(ctx);
       _tnl_wakeup(ctx);
      _tnl_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
   }

}

void brw_draw_init( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   int i;

   /* Register our drawing function: 
    */
   vbo->draw_prims = brw_draw_prims;

   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      brw->vb.inputs[i].buffer = -1;
   brw->vb.nr_buffers = 0;
   brw->vb.nr_enabled = 0;
}

void brw_draw_destroy( struct brw_context *brw )
{
   int i;

   for (i = 0; i < brw->vb.nr_buffers; i++) {
      drm_intel_bo_unreference(brw->vb.buffers[i].bo);
      brw->vb.buffers[i].bo = NULL;
   }
   brw->vb.nr_buffers = 0;

   for (i = 0; i < brw->vb.nr_enabled; i++) {
      brw->vb.enabled[i]->buffer = -1;
   }
   brw->vb.nr_enabled = 0;

   drm_intel_bo_unreference(brw->ib.bo);
   brw->ib.bo = NULL;
}
