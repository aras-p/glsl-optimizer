/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */



#include "brw_context.h"
#include "brw_state.h"
#include "drivers/common/meta.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"

static const struct brw_tracked_state *gen4_atoms[] =
{
   &brw_vs_prog, /* must do before GS prog, state base address. */
   &brw_ff_gs_prog, /* must do before state base address */

   &brw_interpolation_map,

   &brw_clip_prog, /* must do before state base address */
   &brw_sf_prog, /* must do before state base address */
   &brw_wm_prog, /* must do before state base address */

   /* Once all the programs are done, we know how large urb entry
    * sizes need to be and can decide if we need to change the urb
    * layout.
    */
   &brw_curbe_offsets,
   &brw_recalculate_urb_fence,

   &brw_cc_vp,
   &brw_cc_unit,

   /* Surface state setup.  Must come before the VS/WM unit.  The binding
    * table upload must be last.
    */
   &brw_vs_pull_constants,
   &brw_wm_pull_constants,
   &brw_renderbuffer_surfaces,
   &brw_texture_surfaces,
   &brw_vs_binding_table,
   &brw_wm_binding_table,

   &brw_fs_samplers,
   &brw_vs_samplers,

   /* These set up state for brw_psp_urb_cbs */
   &brw_wm_unit,
   &brw_sf_vp,
   &brw_sf_unit,
   &brw_vs_unit,		/* always required, enabled or not */
   &brw_clip_unit,
   &brw_gs_unit,

   /* Command packets:
    */
   &brw_invariant_state,
   &brw_state_base_address,

   &brw_binding_table_pointers,
   &brw_blend_constant_color,

   &brw_depthbuffer,

   &brw_polygon_stipple,
   &brw_polygon_stipple_offset,

   &brw_line_stipple,
   &brw_aa_line_parameters,

   &brw_psp_urb_cbs,

   &brw_drawing_rect,
   &brw_indices,
   &brw_index_buffer,
   &brw_vertices,

   &brw_constant_buffer
};

static const struct brw_tracked_state *gen6_atoms[] =
{
   &brw_vs_prog, /* must do before state base address */
   &brw_ff_gs_prog, /* must do before state base address */
   &brw_wm_prog, /* must do before state base address */

   &gen6_clip_vp,
   &gen6_sf_vp,

   /* Command packets: */

   /* must do before binding table pointers, cc state ptrs */
   &brw_state_base_address,

   &brw_cc_vp,
   &gen6_viewport_state,	/* must do after *_vp stages */

   &gen6_urb,
   &gen6_blend_state,		/* must do before cc unit */
   &gen6_color_calc_state,	/* must do before cc unit */
   &gen6_depth_stencil_state,	/* must do before cc unit */

   &gen6_vs_push_constants, /* Before vs_state */
   &gen6_wm_push_constants, /* Before wm_state */

   /* Surface state setup.  Must come before the VS/WM unit.  The binding
    * table upload must be last.
    */
   &brw_vs_pull_constants,
   &brw_vs_ubo_surfaces,
   &brw_wm_pull_constants,
   &brw_wm_ubo_surfaces,
   &gen6_renderbuffer_surfaces,
   &brw_texture_surfaces,
   &gen6_sol_surface,
   &brw_vs_binding_table,
   &gen6_gs_binding_table,
   &brw_wm_binding_table,

   &brw_fs_samplers,
   &brw_vs_samplers,
   &gen6_sampler_state,
   &gen6_multisample_state,

   &gen6_vs_state,
   &gen6_gs_state,
   &gen6_clip_state,
   &gen6_sf_state,
   &gen6_wm_state,

   &gen6_scissor_state,

   &gen6_binding_table_pointers,

   &brw_depthbuffer,

   &brw_polygon_stipple,
   &brw_polygon_stipple_offset,

   &brw_line_stipple,
   &brw_aa_line_parameters,

   &brw_drawing_rect,

   &brw_indices,
   &brw_index_buffer,
   &brw_vertices,
};

static const struct brw_tracked_state *gen7_atoms[] =
{
   &brw_vs_prog,
   &brw_gs_prog,
   &brw_wm_prog,

   /* Command packets: */

   /* must do before binding table pointers, cc state ptrs */
   &brw_state_base_address,

   &brw_cc_vp,
   &gen7_cc_viewport_state_pointer, /* must do after brw_cc_vp */
   &gen7_sf_clip_viewport,

   &gen7_push_constant_space,
   &gen7_urb,
   &gen6_blend_state,		/* must do before cc unit */
   &gen6_color_calc_state,	/* must do before cc unit */
   &gen6_depth_stencil_state,	/* must do before cc unit */

   &gen6_vs_push_constants, /* Before vs_state */
   &gen7_gs_push_constants, /* Before gs_state */
   &gen6_wm_push_constants, /* Before wm_surfaces and constant_buffer */

   /* Surface state setup.  Must come before the VS/WM unit.  The binding
    * table upload must be last.
    */
   &brw_vs_pull_constants,
   &brw_vs_ubo_surfaces,
   &brw_vs_abo_surfaces,
   &brw_gs_pull_constants,
   &brw_gs_ubo_surfaces,
   &brw_gs_abo_surfaces,
   &brw_wm_pull_constants,
   &brw_wm_ubo_surfaces,
   &brw_wm_abo_surfaces,
   &gen6_renderbuffer_surfaces,
   &brw_texture_surfaces,
   &brw_vs_binding_table,
   &brw_gs_binding_table,
   &brw_wm_binding_table,

   &brw_fs_samplers,
   &brw_vs_samplers,
   &brw_gs_samplers,
   &gen6_multisample_state,

   &gen7_disable_stages,
   &gen7_vs_state,
   &gen7_gs_state,
   &gen7_sol_state,
   &gen7_clip_state,
   &gen7_sbe_state,
   &gen7_sf_state,
   &gen7_wm_state,
   &gen7_ps_state,

   &gen6_scissor_state,

   &gen7_depthbuffer,

   &brw_polygon_stipple,
   &brw_polygon_stipple_offset,

   &brw_line_stipple,
   &brw_aa_line_parameters,

   &brw_drawing_rect,

   &brw_indices,
   &brw_index_buffer,
   &brw_vertices,

   &haswell_cut_index,
};

static const struct brw_tracked_state *gen8_atoms[] =
{
   &brw_vs_prog,
   &brw_gs_prog,
   &brw_wm_prog,

   /* Command packets: */
   &gen8_state_base_address,

   &brw_cc_vp,
   &gen7_cc_viewport_state_pointer, /* must do after brw_cc_vp */
   &gen8_sf_clip_viewport,

   &gen7_push_constant_space,
   &gen7_urb,
   &gen8_blend_state,
   &gen6_color_calc_state,

   &gen6_vs_push_constants, /* Before vs_state */
   &gen7_gs_push_constants, /* Before gs_state */
   &gen6_wm_push_constants, /* Before wm_surfaces and constant_buffer */

   /* Surface state setup.  Must come before the VS/WM unit.  The binding
    * table upload must be last.
    */
   &brw_vs_pull_constants,
   &brw_vs_ubo_surfaces,
   &brw_vs_abo_surfaces,
   &brw_gs_pull_constants,
   &brw_gs_ubo_surfaces,
   &brw_gs_abo_surfaces,
   &brw_wm_pull_constants,
   &brw_wm_ubo_surfaces,
   &brw_wm_abo_surfaces,
   &gen6_renderbuffer_surfaces,
   &brw_texture_surfaces,
   &brw_vs_binding_table,
   &brw_gs_binding_table,
   &brw_wm_binding_table,

   &brw_fs_samplers,
   &brw_vs_samplers,
   &brw_gs_samplers,
   &gen8_multisample_state,

   &gen8_disable_stages,
   &gen8_vs_state,
   &gen8_gs_state,
   &gen8_sol_state,
   &gen6_clip_state,
   &gen8_raster_state,
   &gen8_sbe_state,
   &gen8_sf_state,
   &gen8_ps_blend,
   &gen8_ps_extra,
   &gen8_ps_state,
   &gen8_wm_depth_stencil,
   &gen8_wm_state,

   &gen6_scissor_state,

   &gen7_depthbuffer,

   &brw_polygon_stipple,
   &brw_polygon_stipple_offset,

   &brw_line_stipple,
   &brw_aa_line_parameters,

   &brw_drawing_rect,

   &gen8_vf_topology,

   &brw_indices,
   &gen8_index_buffer,
   &gen8_vertices,

   &haswell_cut_index,
};

static void
brw_upload_initial_gpu_state(struct brw_context *brw)
{
   /* On platforms with hardware contexts, we can set our initial GPU state
    * right away rather than doing it via state atoms.  This saves a small
    * amount of overhead on every draw call.
    */
   if (!brw->hw_ctx)
      return;

   brw_upload_invariant_state(brw);

   if (brw->gen >= 8) {
      gen8_emit_3dstate_sample_pattern(brw);
   }
}

void brw_init_state( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_tracked_state **atoms;
   int num_atoms;

   brw_init_caches(brw);

   if (brw->gen >= 8) {
      atoms = gen8_atoms;
      num_atoms = ARRAY_SIZE(gen8_atoms);
   } else if (brw->gen == 7) {
      atoms = gen7_atoms;
      num_atoms = ARRAY_SIZE(gen7_atoms);
   } else if (brw->gen == 6) {
      atoms = gen6_atoms;
      num_atoms = ARRAY_SIZE(gen6_atoms);
   } else {
      atoms = gen4_atoms;
      num_atoms = ARRAY_SIZE(gen4_atoms);
   }

   brw->atoms = atoms;
   brw->num_atoms = num_atoms;

   while (num_atoms--) {
      assert((*atoms)->dirty.mesa |
	     (*atoms)->dirty.brw |
	     (*atoms)->dirty.cache);
      assert((*atoms)->emit);
      atoms++;
   }

   brw_upload_initial_gpu_state(brw);

   brw->state.dirty.mesa = ~0;
   brw->state.dirty.brw = ~0;

   /* Make sure that brw->state.dirty.brw has enough bits to hold all possible
    * dirty flags.
    */
   STATIC_ASSERT(BRW_NUM_STATE_BITS <= 8 * sizeof(brw->state.dirty.brw));

   ctx->DriverFlags.NewTransformFeedback = BRW_NEW_TRANSFORM_FEEDBACK;
   ctx->DriverFlags.NewTransformFeedbackProg = BRW_NEW_TRANSFORM_FEEDBACK;
   ctx->DriverFlags.NewRasterizerDiscard = BRW_NEW_RASTERIZER_DISCARD;
   ctx->DriverFlags.NewUniformBuffer = BRW_NEW_UNIFORM_BUFFER;
   ctx->DriverFlags.NewAtomicBuffer = BRW_NEW_ATOMIC_BUFFER;
}


void brw_destroy_state( struct brw_context *brw )
{
   brw_destroy_caches(brw);
}

/***********************************************************************
 */

static bool
check_state(const struct brw_state_flags *a, const struct brw_state_flags *b)
{
   return ((a->mesa & b->mesa) |
	   (a->brw & b->brw) |
	   (a->cache & b->cache)) != 0;
}

static void accumulate_state( struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   a->mesa |= b->mesa;
   a->brw |= b->brw;
   a->cache |= b->cache;
}


static void xor_states( struct brw_state_flags *result,
			     const struct brw_state_flags *a,
			      const struct brw_state_flags *b )
{
   result->mesa = a->mesa ^ b->mesa;
   result->brw = a->brw ^ b->brw;
   result->cache = a->cache ^ b->cache;
}

struct dirty_bit_map {
   uint32_t bit;
   char *name;
   uint32_t count;
};

#define DEFINE_BIT(name) {name, #name, 0}

static struct dirty_bit_map mesa_bits[] = {
   DEFINE_BIT(_NEW_MODELVIEW),
   DEFINE_BIT(_NEW_PROJECTION),
   DEFINE_BIT(_NEW_TEXTURE_MATRIX),
   DEFINE_BIT(_NEW_COLOR),
   DEFINE_BIT(_NEW_DEPTH),
   DEFINE_BIT(_NEW_EVAL),
   DEFINE_BIT(_NEW_FOG),
   DEFINE_BIT(_NEW_HINT),
   DEFINE_BIT(_NEW_LIGHT),
   DEFINE_BIT(_NEW_LINE),
   DEFINE_BIT(_NEW_PIXEL),
   DEFINE_BIT(_NEW_POINT),
   DEFINE_BIT(_NEW_POLYGON),
   DEFINE_BIT(_NEW_POLYGONSTIPPLE),
   DEFINE_BIT(_NEW_SCISSOR),
   DEFINE_BIT(_NEW_STENCIL),
   DEFINE_BIT(_NEW_TEXTURE),
   DEFINE_BIT(_NEW_TRANSFORM),
   DEFINE_BIT(_NEW_VIEWPORT),
   DEFINE_BIT(_NEW_ARRAY),
   DEFINE_BIT(_NEW_RENDERMODE),
   DEFINE_BIT(_NEW_BUFFERS),
   DEFINE_BIT(_NEW_CURRENT_ATTRIB),
   DEFINE_BIT(_NEW_MULTISAMPLE),
   DEFINE_BIT(_NEW_TRACK_MATRIX),
   DEFINE_BIT(_NEW_PROGRAM),
   DEFINE_BIT(_NEW_PROGRAM_CONSTANTS),
   DEFINE_BIT(_NEW_BUFFER_OBJECT),
   DEFINE_BIT(_NEW_FRAG_CLAMP),
   DEFINE_BIT(_NEW_VARYING_VP_INPUTS),
   {0, 0, 0}
};

static struct dirty_bit_map brw_bits[] = {
   DEFINE_BIT(BRW_NEW_URB_FENCE),
   DEFINE_BIT(BRW_NEW_FRAGMENT_PROGRAM),
   DEFINE_BIT(BRW_NEW_GEOMETRY_PROGRAM),
   DEFINE_BIT(BRW_NEW_VERTEX_PROGRAM),
   DEFINE_BIT(BRW_NEW_CURBE_OFFSETS),
   DEFINE_BIT(BRW_NEW_REDUCED_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_CONTEXT),
   DEFINE_BIT(BRW_NEW_PSP),
   DEFINE_BIT(BRW_NEW_SURFACES),
   DEFINE_BIT(BRW_NEW_VS_BINDING_TABLE),
   DEFINE_BIT(BRW_NEW_GS_BINDING_TABLE),
   DEFINE_BIT(BRW_NEW_PS_BINDING_TABLE),
   DEFINE_BIT(BRW_NEW_INDICES),
   DEFINE_BIT(BRW_NEW_VERTICES),
   DEFINE_BIT(BRW_NEW_BATCH),
   DEFINE_BIT(BRW_NEW_INDEX_BUFFER),
   DEFINE_BIT(BRW_NEW_VS_CONSTBUF),
   DEFINE_BIT(BRW_NEW_GS_CONSTBUF),
   DEFINE_BIT(BRW_NEW_PROGRAM_CACHE),
   DEFINE_BIT(BRW_NEW_STATE_BASE_ADDRESS),
   DEFINE_BIT(BRW_NEW_VUE_MAP_VS),
   DEFINE_BIT(BRW_NEW_VUE_MAP_GEOM_OUT),
   DEFINE_BIT(BRW_NEW_TRANSFORM_FEEDBACK),
   DEFINE_BIT(BRW_NEW_RASTERIZER_DISCARD),
   DEFINE_BIT(BRW_NEW_STATS_WM),
   DEFINE_BIT(BRW_NEW_UNIFORM_BUFFER),
   DEFINE_BIT(BRW_NEW_ATOMIC_BUFFER),
   DEFINE_BIT(BRW_NEW_META_IN_PROGRESS),
   DEFINE_BIT(BRW_NEW_INTERPOLATION_MAP),
   DEFINE_BIT(BRW_NEW_PUSH_CONSTANT_ALLOCATION),
   DEFINE_BIT(BRW_NEW_NUM_SAMPLES),
   {0, 0, 0}
};

static struct dirty_bit_map cache_bits[] = {
   DEFINE_BIT(CACHE_NEW_CC_VP),
   DEFINE_BIT(CACHE_NEW_CC_UNIT),
   DEFINE_BIT(CACHE_NEW_WM_PROG),
   DEFINE_BIT(CACHE_NEW_BLORP_BLIT_PROG),
   DEFINE_BIT(CACHE_NEW_BLORP_CONST_COLOR_PROG),
   DEFINE_BIT(CACHE_NEW_SAMPLER),
   DEFINE_BIT(CACHE_NEW_WM_UNIT),
   DEFINE_BIT(CACHE_NEW_SF_PROG),
   DEFINE_BIT(CACHE_NEW_SF_VP),
   DEFINE_BIT(CACHE_NEW_SF_UNIT),
   DEFINE_BIT(CACHE_NEW_VS_UNIT),
   DEFINE_BIT(CACHE_NEW_VS_PROG),
   DEFINE_BIT(CACHE_NEW_FF_GS_UNIT),
   DEFINE_BIT(CACHE_NEW_FF_GS_PROG),
   DEFINE_BIT(CACHE_NEW_GS_PROG),
   DEFINE_BIT(CACHE_NEW_CLIP_VP),
   DEFINE_BIT(CACHE_NEW_CLIP_UNIT),
   DEFINE_BIT(CACHE_NEW_CLIP_PROG),
   {0, 0, 0}
};


static void
brw_update_dirty_count(struct dirty_bit_map *bit_map, int32_t bits)
{
   int i;

   for (i = 0; i < 32; i++) {
      if (bit_map[i].bit == 0)
	 return;

      if (bit_map[i].bit & bits)
	 bit_map[i].count++;
   }
}

static void
brw_print_dirty_count(struct dirty_bit_map *bit_map)
{
   int i;

   for (i = 0; i < 32; i++) {
      if (bit_map[i].bit == 0)
	 return;

      fprintf(stderr, "0x%08x: %12d (%s)\n",
	      bit_map[i].bit, bit_map[i].count, bit_map[i].name);
   }
}

/***********************************************************************
 * Emit all state:
 */
void brw_upload_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_state_flags *state = &brw->state.dirty;
   int i;
   static int dirty_count = 0;

   state->mesa |= brw->NewGLState;
   brw->NewGLState = 0;

   state->brw |= ctx->NewDriverState;
   ctx->NewDriverState = 0;

   if (0) {
      /* Always re-emit all state. */
      state->mesa |= ~0;
      state->brw |= ~0;
      state->cache |= ~0;
   }

   if (brw->fragment_program != ctx->FragmentProgram._Current) {
      brw->fragment_program = ctx->FragmentProgram._Current;
      brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
   }

   if (brw->geometry_program != ctx->GeometryProgram._Current) {
      brw->geometry_program = ctx->GeometryProgram._Current;
      brw->state.dirty.brw |= BRW_NEW_GEOMETRY_PROGRAM;
   }

   if (brw->vertex_program != ctx->VertexProgram._Current) {
      brw->vertex_program = ctx->VertexProgram._Current;
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
   }

   if (brw->meta_in_progress != _mesa_meta_in_progress(ctx)) {
      brw->meta_in_progress = _mesa_meta_in_progress(ctx);
      brw->state.dirty.brw |= BRW_NEW_META_IN_PROGRESS;
   }

   if (brw->num_samples != ctx->DrawBuffer->Visual.samples) {
      brw->num_samples = ctx->DrawBuffer->Visual.samples;
      brw->state.dirty.brw |= BRW_NEW_NUM_SAMPLES;
   }

   if ((state->mesa | state->cache | state->brw) == 0)
      return;

   if (unlikely(INTEL_DEBUG)) {
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      struct brw_state_flags examined, prev;
      memset(&examined, 0, sizeof(examined));
      prev = *state;

      for (i = 0; i < brw->num_atoms; i++) {
	 const struct brw_tracked_state *atom = brw->atoms[i];
	 struct brw_state_flags generated;

	 if (check_state(state, &atom->dirty)) {
	    atom->emit(brw);
	 }

	 accumulate_state(&examined, &atom->dirty);

	 /* generated = (prev ^ state)
	  * if (examined & generated)
	  *     fail;
	  */
	 xor_states(&generated, &prev, state);
	 assert(!check_state(&examined, &generated));
	 prev = *state;
      }
   }
   else {
      for (i = 0; i < brw->num_atoms; i++) {
	 const struct brw_tracked_state *atom = brw->atoms[i];

	 if (check_state(state, &atom->dirty)) {
	    atom->emit(brw);
	 }
      }
   }

   if (unlikely(INTEL_DEBUG & DEBUG_STATE)) {
      STATIC_ASSERT(ARRAY_SIZE(brw_bits) == BRW_NUM_STATE_BITS + 1);
      STATIC_ASSERT(ARRAY_SIZE(cache_bits) == BRW_MAX_CACHE + 1);

      brw_update_dirty_count(mesa_bits, state->mesa);
      brw_update_dirty_count(brw_bits, state->brw);
      brw_update_dirty_count(cache_bits, state->cache);
      if (dirty_count++ % 1000 == 0) {
	 brw_print_dirty_count(mesa_bits);
	 brw_print_dirty_count(brw_bits);
	 brw_print_dirty_count(cache_bits);
	 fprintf(stderr, "\n");
      }
   }
}


/**
 * Clear dirty bits to account for the fact that the state emitted by
 * brw_upload_state() has been committed to the hardware.  This is a separate
 * call from brw_upload_state() because it's possible that after the call to
 * brw_upload_state(), we will discover that we've run out of aperture space,
 * and need to rewind the batch buffer to the state it had before the
 * brw_upload_state() call.
 */
void
brw_clear_dirty_bits(struct brw_context *brw)
{
   struct brw_state_flags *state = &brw->state.dirty;
   memset(state, 0, sizeof(*state));
}
