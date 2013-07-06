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

#include <sys/errno.h>

#include "main/glheader.h"
#include "main/context.h"
#include "main/condrender.h"
#include "main/samplerobj.h"
#include "main/state.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/transformfeedback.h"
#include "tnl/tnl.h"
#include "vbo/vbo_context.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "drivers/common/meta.h"

#include "brw_blorp.h"
#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"

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
static void brw_set_prim(struct brw_context *brw,
                         const struct _mesa_prim *prim)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t hw_prim = prim_to_hw_prim[prim->mode];

   DBG("PRIM: %s\n", _mesa_lookup_enum_by_nr(prim->mode));

   /* Slight optimization to avoid the GS program when not needed:
    */
   if (prim->mode == GL_QUAD_STRIP &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL)
      hw_prim = _3DPRIM_TRISTRIP;

   if (prim->mode == GL_QUADS && prim->count == 4 &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL) {
      hw_prim = _3DPRIM_TRIFAN;
   }

   if (hw_prim != brw->primitive) {
      brw->primitive = hw_prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[prim->mode] != brw->reduced_primitive) {
	 brw->reduced_primitive = reduced_prim[prim->mode];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }
   }
}

static void gen6_set_prim(struct brw_context *brw,
                          const struct _mesa_prim *prim)
{
   uint32_t hw_prim;

   DBG("PRIM: %s\n", _mesa_lookup_enum_by_nr(prim->mode));

   hw_prim = prim_to_hw_prim[prim->mode];

   if (hw_prim != brw->primitive) {
      brw->primitive = hw_prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;
   }
}


/**
 * The hardware is capable of removing dangling vertices on its own; however,
 * prior to Gen6, we sometimes convert quads into trifans (and quad strips
 * into tristrips), since pre-Gen6 hardware requires a GS to render quads.
 * This function manually trims dangling vertices from a draw call involving
 * quads so that those dangling vertices won't get drawn when we convert to
 * trifans/tristrips.
 */
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

   /* We only need to trim the primitive count on pre-Gen6. */
   if (brw->gen < 6)
      verts_per_instance = trim(prim->mode, prim->count);
   else
      verts_per_instance = prim->count;

   /* If nothing to emit, just return. */
   if (verts_per_instance == 0)
      return;

   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (brw->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(brw);
   }

   BEGIN_BATCH(6);
   OUT_BATCH(CMD_3D_PRIM << 16 | (6 - 2) |
	     hw_prim << GEN4_3DPRIM_TOPOLOGY_TYPE_SHIFT |
	     vertex_access_type);
   OUT_BATCH(verts_per_instance);
   OUT_BATCH(start_vertex_location);
   OUT_BATCH(prim->num_instances);
   OUT_BATCH(prim->base_instance);
   OUT_BATCH(base_vertex_location);
   ADVANCE_BATCH();

   brw->batch.need_workaround_flush = true;

   if (brw->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(brw);
   }
}

static void gen7_emit_prim(struct brw_context *brw,
			   const struct _mesa_prim *prim,
			   uint32_t hw_prim)
{
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

   verts_per_instance = prim->count;

   /* If nothing to emit, just return. */
   if (verts_per_instance == 0)
      return;

   /* If we're set to always flush, do it before and after the primitive emit.
    * We want to catch both missed flushes that hurt instruction/state cache
    * and missed flushes of the render cache as it heads to other parts of
    * the besides the draw code.
    */
   if (brw->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(brw);
   }

   BEGIN_BATCH(7);
   OUT_BATCH(CMD_3D_PRIM << 16 | (7 - 2));
   OUT_BATCH(hw_prim | vertex_access_type);
   OUT_BATCH(verts_per_instance);
   OUT_BATCH(start_vertex_location);
   OUT_BATCH(prim->num_instances);
   OUT_BATCH(prim->base_instance);
   OUT_BATCH(base_vertex_location);
   ADVANCE_BATCH();

   if (brw->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(brw);
   }
}


static void brw_merge_inputs( struct brw_context *brw,
		       const struct gl_client_array *arrays[])
{
   GLuint i;

   for (i = 0; i < brw->vb.nr_buffers; i++) {
      drm_intel_bo_unreference(brw->vb.buffers[i].bo);
      brw->vb.buffers[i].bo = NULL;
   }
   brw->vb.nr_buffers = 0;

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      brw->vb.inputs[i].buffer = -1;
      brw->vb.inputs[i].glarray = arrays[i];
      brw->vb.inputs[i].attrib = (gl_vert_attrib) i;
   }
}

/*
 * \brief Resolve buffers before drawing.
 *
 * Resolve the depth buffer's HiZ buffer and resolve the depth buffer of each
 * enabled depth texture.
 *
 * (In the future, this will also perform MSAA resolves).
 */
static void
brw_predraw_resolve_buffers(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct intel_renderbuffer *depth_irb;
   struct intel_texture_object *tex_obj;

   /* Resolve the depth buffer's HiZ buffer. */
   depth_irb = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);
   if (depth_irb)
      intel_renderbuffer_resolve_hiz(brw, depth_irb);

   /* Resolve depth buffer of each enabled depth texture, and color buffer of
    * each fast-clear-enabled color texture.
    */
   for (int i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      if (!ctx->Texture.Unit[i]._ReallyEnabled)
	 continue;
      tex_obj = intel_texture_object(ctx->Texture.Unit[i]._Current);
      if (!tex_obj || !tex_obj->mt)
	 continue;
      intel_miptree_all_slices_resolve_depth(brw, tex_obj->mt);
      intel_miptree_resolve_color(brw, tex_obj->mt);
   }
}

/**
 * \brief Call this after drawing to mark which buffers need resolving
 *
 * If the depth buffer was written to and if it has an accompanying HiZ
 * buffer, then mark that it needs a depth resolve.
 *
 * If the color buffer is a multisample window system buffer, then
 * mark that it needs a downsample.
 */
static void brw_postdraw_set_buffers_need_resolve(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   struct intel_renderbuffer *front_irb = NULL;
   struct intel_renderbuffer *back_irb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
   struct intel_renderbuffer *depth_irb = intel_get_renderbuffer(fb, BUFFER_DEPTH);

   if (brw->is_front_buffer_rendering)
      front_irb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);

   if (front_irb)
      intel_renderbuffer_set_needs_downsample(front_irb);
   if (back_irb)
      intel_renderbuffer_set_needs_downsample(back_irb);
   if (depth_irb && ctx->Depth.Mask)
      intel_renderbuffer_set_needs_depth_resolve(depth_irb);
}

/* May fail if out of video memory for texture or vbo upload, or on
 * fallback conditions.
 */
static bool brw_try_draw_prims( struct gl_context *ctx,
				     const struct gl_client_array *arrays[],
				     const struct _mesa_prim *prim,
				     GLuint nr_prims,
				     const struct _mesa_index_buffer *ib,
				     GLuint min_index,
				     GLuint max_index )
{
   struct brw_context *brw = brw_context(ctx);
   bool retval = true;
   GLuint i;
   bool fail_next = false;

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

   intel_prepare_render(brw);

   /* This workaround has to happen outside of brw_upload_state() because it
    * may flush the batchbuffer for a blit, affecting the state flags.
    */
   brw_workaround_depthstencil_alignment(brw, 0);

   /* Resolves must occur after updating renderbuffers, updating context state,
    * and finalizing textures but before setting up any hardware state for
    * this draw call.
    */
   brw_predraw_resolve_buffers(brw);

   /* Bind all inputs, derive varying and size information:
    */
   brw_merge_inputs( brw, arrays );

   brw->ib.ib = ib;
   brw->state.dirty.brw |= BRW_NEW_INDICES;

   brw->vb.min_index = min_index;
   brw->vb.max_index = max_index;
   brw->state.dirty.brw |= BRW_NEW_VERTICES;

   for (i = 0; i < nr_prims; i++) {
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
      intel_batchbuffer_require_space(brw, estimated_max_prim_size, false);
      intel_batchbuffer_save_state(brw);

      if (brw->num_instances != prim->num_instances) {
         brw->num_instances = prim->num_instances;
         brw->state.dirty.brw |= BRW_NEW_VERTICES;
      }
      if (brw->basevertex != prim->basevertex) {
         brw->basevertex = prim->basevertex;
         brw->state.dirty.brw |= BRW_NEW_VERTICES;
      }
      if (brw->gen < 6)
	 brw_set_prim(brw, &prim[i]);
      else
	 gen6_set_prim(brw, &prim[i]);

retry:
      /* Note that before the loop, brw->state.dirty.brw was set to != 0, and
       * that the state updated in the loop outside of this block is that in
       * *_set_prim or intel_batchbuffer_flush(), which only impacts
       * brw->state.dirty.brw.
       */
      if (brw->state.dirty.brw) {
	 brw->no_batch_wrap = true;
	 brw_upload_state(brw);
      }

      if (brw->gen >= 7)
	 gen7_emit_prim(brw, &prim[i], brw->primitive);
      else
	 brw_emit_prim(brw, &prim[i], brw->primitive);

      brw->no_batch_wrap = false;

      if (dri_bufmgr_check_aperture_space(&brw->batch.bo, 1)) {
	 if (!fail_next) {
	    intel_batchbuffer_reset_to_saved(brw);
	    intel_batchbuffer_flush(brw);
	    fail_next = true;
	    goto retry;
	 } else {
	    if (intel_batchbuffer_flush(brw) == -ENOSPC) {
	       static bool warned = false;

	       if (!warned) {
		  fprintf(stderr, "i965: Single primitive emit exceeded"
			  "available aperture space\n");
		  warned = true;
	       }

	       retval = false;
	    }
	 }
      }
   }

   if (brw->always_flush_batch)
      intel_batchbuffer_flush(brw);

   brw_state_cache_check_size(brw);
   brw_postdraw_set_buffers_need_resolve(brw);

   return retval;
}

void brw_draw_prims( struct gl_context *ctx,
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     GLboolean index_bounds_valid,
		     GLuint min_index,
		     GLuint max_index,
		     struct gl_transform_feedback_object *tfb_vertcount )
{
   struct brw_context *brw = brw_context(ctx);
   const struct gl_client_array **arrays = ctx->Array._DrawArrays;

   if (!_mesa_check_conditional_render(ctx))
      return;

   /* Handle primitive restart if needed */
   if (brw_handle_primitive_restart(ctx, prim, nr_prims, ib)) {
      /* The draw was handled, so we can exit now */
      return;
   }

   /* If we're going to have to upload any of the user's vertex arrays, then
    * get the minimum and maximum of their index buffer so we know what range
    * to upload.
    */
   if (!vbo_all_varyings_in_vbos(arrays) && !index_bounds_valid)
      vbo_get_minmax_indices(ctx, prim, ib, &min_index, &max_index, nr_prims);

   /* Do GL_SELECT and GL_FEEDBACK rendering using swrast, even though it
    * won't support all the extensions we support.
    */
   if (ctx->RenderMode != GL_RENDER) {
      perf_debug("%s render mode not supported in hardware\n",
                 _mesa_lookup_enum_by_nr(ctx->RenderMode));
      _swsetup_Wakeup(ctx);
      _tnl_wakeup(ctx);
      _tnl_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
      return;
   }

   /* Try drawing with the hardware, but don't do anything else if we can't
    * manage it.  swrast doesn't support our featureset, so we can't fall back
    * to it.
    */
   brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
}

void brw_draw_init( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->ctx;
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
