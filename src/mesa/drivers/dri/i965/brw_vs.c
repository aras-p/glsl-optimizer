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


#include "main/compiler.h"
#include "brw_context.h"
#include "brw_vs.h"
#include "brw_util.h"
#include "brw_state.h"
#include "program/prog_print.h"
#include "program/prog_parameter.h"

#include "util/ralloc.h"

static inline void assign_vue_slot(struct brw_vue_map *vue_map,
                                   int varying)
{
   /* Make sure this varying hasn't been assigned a slot already */
   assert (vue_map->varying_to_slot[varying] == -1);

   vue_map->varying_to_slot[varying] = vue_map->num_slots;
   vue_map->slot_to_varying[vue_map->num_slots++] = varying;
}

/**
 * Compute the VUE map for vertex shader program.
 */
void
brw_compute_vue_map(struct brw_context *brw, struct brw_vue_map *vue_map,
                    GLbitfield64 slots_valid)
{
   vue_map->slots_valid = slots_valid;
   int i;

   /* gl_Layer and gl_ViewportIndex don't get their own varying slots -- they
    * are stored in the first VUE slot (VARYING_SLOT_PSIZ).
    */
   slots_valid &= ~(VARYING_BIT_LAYER | VARYING_BIT_VIEWPORT);

   /* Make sure that the values we store in vue_map->varying_to_slot and
    * vue_map->slot_to_varying won't overflow the signed chars that are used
    * to store them.  Note that since vue_map->slot_to_varying sometimes holds
    * values equal to BRW_VARYING_SLOT_COUNT, we need to ensure that
    * BRW_VARYING_SLOT_COUNT is <= 127, not 128.
    */
   STATIC_ASSERT(BRW_VARYING_SLOT_COUNT <= 127);

   vue_map->num_slots = 0;
   for (i = 0; i < BRW_VARYING_SLOT_COUNT; ++i) {
      vue_map->varying_to_slot[i] = -1;
      vue_map->slot_to_varying[i] = BRW_VARYING_SLOT_COUNT;
   }

   /* VUE header: format depends on chip generation and whether clipping is
    * enabled.
    */
   if (brw->gen < 6) {
      /* There are 8 dwords in VUE header pre-Ironlake:
       * dword 0-3 is indices, point width, clip flags.
       * dword 4-7 is ndc position
       * dword 8-11 is the first vertex data.
       *
       * On Ironlake the VUE header is nominally 20 dwords, but the hardware
       * will accept the same header layout as Gen4 [and should be a bit faster]
       */
      assign_vue_slot(vue_map, VARYING_SLOT_PSIZ);
      assign_vue_slot(vue_map, BRW_VARYING_SLOT_NDC);
      assign_vue_slot(vue_map, VARYING_SLOT_POS);
   } else {
      /* There are 8 or 16 DWs (D0-D15) in VUE header on Sandybridge:
       * dword 0-3 of the header is indices, point width, clip flags.
       * dword 4-7 is the 4D space position
       * dword 8-15 of the vertex header is the user clip distance if
       * enabled.
       * dword 8-11 or 16-19 is the first vertex element data we fill.
       */
      assign_vue_slot(vue_map, VARYING_SLOT_PSIZ);
      assign_vue_slot(vue_map, VARYING_SLOT_POS);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0))
         assign_vue_slot(vue_map, VARYING_SLOT_CLIP_DIST0);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1))
         assign_vue_slot(vue_map, VARYING_SLOT_CLIP_DIST1);

      /* front and back colors need to be consecutive so that we can use
       * ATTRIBUTE_SWIZZLE_INPUTATTR_FACING to swizzle them when doing
       * two-sided color.
       */
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_COL0))
         assign_vue_slot(vue_map, VARYING_SLOT_COL0);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         assign_vue_slot(vue_map, VARYING_SLOT_BFC0);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_COL1))
         assign_vue_slot(vue_map, VARYING_SLOT_COL1);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_BFC1))
         assign_vue_slot(vue_map, VARYING_SLOT_BFC1);
   }

   /* The hardware doesn't care about the rest of the vertex outputs, so just
    * assign them contiguously.  Don't reassign outputs that already have a
    * slot.
    *
    * We generally don't need to assign a slot for VARYING_SLOT_CLIP_VERTEX,
    * since it's encoded as the clip distances by emit_clip_distances().
    * However, it may be output by transform feedback, and we'd rather not
    * recompute state when TF changes, so we just always include it.
    */
   for (int i = 0; i < VARYING_SLOT_MAX; ++i) {
      if ((slots_valid & BITFIELD64_BIT(i)) &&
          vue_map->varying_to_slot[i] == -1) {
         assign_vue_slot(vue_map, i);
      }
   }
}


/**
 * Decide which set of clip planes should be used when clipping via
 * gl_Position or gl_ClipVertex.
 */
gl_clip_plane *brw_select_clip_planes(struct gl_context *ctx)
{
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX]) {
      /* There is currently a GLSL vertex shader, so clip according to GLSL
       * rules, which means compare gl_ClipVertex (or gl_Position, if
       * gl_ClipVertex wasn't assigned) against the eye-coordinate clip planes
       * that were stored in EyeUserPlane at the time the clip planes were
       * specified.
       */
      return ctx->Transform.EyeUserPlane;
   } else {
      /* Either we are using fixed function or an ARB vertex program.  In
       * either case the clip planes are going to be compared against
       * gl_Position (which is in clip coordinates) so we have to clip using
       * _ClipUserPlane, which was transformed into clip coordinates by Mesa
       * core.
       */
      return ctx->Transform._ClipUserPlane;
   }
}


bool
brw_vs_prog_data_compare(const void *in_a, const void *in_b)
{
   const struct brw_vs_prog_data *a = in_a;
   const struct brw_vs_prog_data *b = in_b;

   /* Compare the base structure. */
   if (!brw_stage_prog_data_compare(&a->base.base, &b->base.base))
      return false;

   /* Compare the rest of the struct. */
   const unsigned offset = sizeof(struct brw_stage_prog_data);
   if (memcmp(((char *) a) + offset, ((char *) b) + offset,
              sizeof(struct brw_vs_prog_data) - offset)) {
      return false;
   }

   return true;
}

static bool
do_vs_prog(struct brw_context *brw,
	   struct gl_shader_program *prog,
	   struct brw_vertex_program *vp,
	   struct brw_vs_prog_key *key)
{
   GLuint program_size;
   const GLuint *program;
   struct brw_vs_compile c;
   struct brw_vs_prog_data prog_data;
   struct brw_stage_prog_data *stage_prog_data = &prog_data.base.base;
   void *mem_ctx;
   int i;
   struct gl_shader *vs = NULL;

   if (prog)
      vs = prog->_LinkedShaders[MESA_SHADER_VERTEX];

   memset(&c, 0, sizeof(c));
   memcpy(&c.key, key, sizeof(*key));
   memset(&prog_data, 0, sizeof(prog_data));

   mem_ctx = ralloc_context(NULL);

   c.vp = vp;

   /* Allocate the references to the uniforms that will end up in the
    * prog_data associated with the compiled program, and which will be freed
    * by the state cache.
    */
   int param_count;
   if (vs) {
      /* We add padding around uniform values below vec4 size, with the worst
       * case being a float value that gets blown up to a vec4, so be
       * conservative here.
       */
      param_count = vs->num_uniform_components * 4;

   } else {
      param_count = vp->program.Base.Parameters->NumParameters * 4;
   }
   /* vec4_visitor::setup_uniform_clipplane_values() also uploads user clip
    * planes as uniforms.
    */
   param_count += c.key.base.nr_userclip_plane_consts * 4;

   stage_prog_data->param =
      rzalloc_array(NULL, const gl_constant_value *, param_count);
   stage_prog_data->pull_param =
      rzalloc_array(NULL, const gl_constant_value *, param_count);

   /* Setting nr_params here NOT to the size of the param and pull_param
    * arrays, but to the number of uniform components vec4_visitor
    * needs. vec4_visitor::setup_uniforms() will set it back to a proper value.
    */
   stage_prog_data->nr_params = ALIGN(param_count, 4) / 4;
   if (vs) {
      stage_prog_data->nr_params += vs->num_samplers;
   }

   GLbitfield64 outputs_written = vp->program.Base.OutputsWritten;
   prog_data.inputs_read = vp->program.Base.InputsRead;

   if (c.key.copy_edgeflag) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_EDGE);
      prog_data.inputs_read |= VERT_BIT_EDGEFLAG;
   }

   if (brw->gen < 6) {
      /* Put dummy slots into the VUE for the SF to put the replaced
       * point sprite coords in.  We shouldn't need these dummy slots,
       * which take up precious URB space, but it would mean that the SF
       * doesn't get nice aligned pairs of input coords into output
       * coords, which would be a pain to handle.
       */
      for (i = 0; i < 8; i++) {
         if (c.key.point_coord_replace & (1 << i))
            outputs_written |= BITFIELD64_BIT(VARYING_SLOT_TEX0 + i);
      }

      /* if back colors are written, allocate slots for front colors too */
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         outputs_written |= BITFIELD64_BIT(VARYING_SLOT_COL0);
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC1))
         outputs_written |= BITFIELD64_BIT(VARYING_SLOT_COL1);
   }

   /* In order for legacy clipping to work, we need to populate the clip
    * distance varying slots whenever clipping is enabled, even if the vertex
    * shader doesn't write to gl_ClipDistance.
    */
   if (c.key.base.userclip_active) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0);
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1);
   }

   brw_compute_vue_map(brw, &prog_data.base.vue_map, outputs_written);

   if (0) {
      _mesa_fprint_program_opt(stderr, &c.vp->program.Base, PROG_PRINT_DEBUG,
			       true);
   }

   /* Emit GEN4 code.
    */
   program = brw_vs_emit(brw, prog, &c, &prog_data, mem_ctx, &program_size);
   if (program == NULL) {
      ralloc_free(mem_ctx);
      return false;
   }

   /* Scratch space is used for register spilling */
   if (c.base.last_scratch) {
      perf_debug("Vertex shader triggered register spilling.  "
                 "Try reducing the number of live vec4 values to "
                 "improve performance.\n");

      prog_data.base.base.total_scratch
         = brw_get_scratch_size(c.base.last_scratch*REG_SIZE);

      brw_get_scratch_bo(brw, &brw->vs.base.scratch_bo,
			 prog_data.base.base.total_scratch *
                         brw->max_vs_threads);
   }

   brw_upload_cache(&brw->cache, BRW_VS_PROG,
		    &c.key, sizeof(c.key),
		    program, program_size,
		    &prog_data, sizeof(prog_data),
		    &brw->vs.base.prog_offset, &brw->vs.prog_data);
   ralloc_free(mem_ctx);

   return true;
}

static bool
key_debug(struct brw_context *brw, const char *name, int a, int b)
{
   if (a != b) {
      perf_debug("  %s %d->%d\n", name, a, b);
      return true;
   }
   return false;
}

void
brw_vs_debug_recompile(struct brw_context *brw,
                       struct gl_shader_program *prog,
                       const struct brw_vs_prog_key *key)
{
   struct brw_cache_item *c = NULL;
   const struct brw_vs_prog_key *old_key = NULL;
   bool found = false;

   perf_debug("Recompiling vertex shader for program %d\n", prog->Name);

   for (unsigned int i = 0; i < brw->cache.size; i++) {
      for (c = brw->cache.items[i]; c; c = c->next) {
         if (c->cache_id == BRW_VS_PROG) {
            old_key = c->key;

            if (old_key->base.program_string_id == key->base.program_string_id)
               break;
         }
      }
      if (c)
         break;
   }

   if (!c) {
      perf_debug("  Didn't find previous compile in the shader cache for "
                 "debug\n");
      return;
   }

   for (unsigned int i = 0; i < VERT_ATTRIB_MAX; i++) {
      found |= key_debug(brw, "Vertex attrib w/a flags",
                         old_key->gl_attrib_wa_flags[i],
                         key->gl_attrib_wa_flags[i]);
   }

   found |= key_debug(brw, "user clip flags",
                      old_key->base.userclip_active, key->base.userclip_active);

   found |= key_debug(brw, "user clipping planes as push constants",
                      old_key->base.nr_userclip_plane_consts,
                      key->base.nr_userclip_plane_consts);

   found |= key_debug(brw, "copy edgeflag",
                      old_key->copy_edgeflag, key->copy_edgeflag);
   found |= key_debug(brw, "PointCoord replace",
                      old_key->point_coord_replace, key->point_coord_replace);
   found |= key_debug(brw, "vertex color clamping",
                      old_key->base.clamp_vertex_color, key->base.clamp_vertex_color);

   found |= brw_debug_recompile_sampler_key(brw, &old_key->base.tex,
                                            &key->base.tex);

   if (!found) {
      perf_debug("  Something else\n");
   }
}


void
brw_setup_vec4_key_clip_info(struct brw_context *brw,
                             struct brw_vec4_prog_key *key,
                             bool program_uses_clip_distance)
{
   struct gl_context *ctx = &brw->ctx;

   key->userclip_active = (ctx->Transform.ClipPlanesEnabled != 0);
   if (key->userclip_active && !program_uses_clip_distance) {
      key->nr_userclip_plane_consts
         = _mesa_logbase2(ctx->Transform.ClipPlanesEnabled) + 1;
   }
}


static void brw_upload_vs_prog(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_vs_prog_key key;
   /* BRW_NEW_VERTEX_PROGRAM */
   struct brw_vertex_program *vp =
      (struct brw_vertex_program *)brw->vertex_program;
   struct gl_program *prog = (struct gl_program *) brw->vertex_program;
   int i;

   memset(&key, 0, sizeof(key));

   /* Just upload the program verbatim for now.  Always send it all
    * the inputs it asks for, whether they are varying or not.
    */
   key.base.program_string_id = vp->id;
   brw_setup_vec4_key_clip_info(brw, &key.base,
                                vp->program.Base.UsesClipDistanceOut);

   /* _NEW_POLYGON */
   if (brw->gen < 6) {
      key.copy_edgeflag = (ctx->Polygon.FrontMode != GL_FILL ||
                           ctx->Polygon.BackMode != GL_FILL);
   }

   /* _NEW_LIGHT | _NEW_BUFFERS */
   key.base.clamp_vertex_color = ctx->Light._ClampVertexColor;

   /* _NEW_POINT */
   if (brw->gen < 6 && ctx->Point.PointSprite) {
      for (i = 0; i < 8; i++) {
	 if (ctx->Point.CoordReplace[i])
	    key.point_coord_replace |= (1 << i);
      }
   }

   /* _NEW_TEXTURE */
   brw_populate_sampler_prog_key_data(ctx, prog, brw->vs.base.sampler_count,
                                      &key.base.tex);

   /* BRW_NEW_VERTICES */
   if (brw->gen < 8 && !brw->is_haswell) {
      /* Prior to Haswell, the hardware can't natively support GL_FIXED or
       * 2_10_10_10_REV vertex formats.  Set appropriate workaround flags.
       */
      for (i = 0; i < VERT_ATTRIB_MAX; i++) {
         if (!(vp->program.Base.InputsRead & BITFIELD64_BIT(i)))
            continue;

         uint8_t wa_flags = 0;

         switch (brw->vb.inputs[i].glarray->Type) {

         case GL_FIXED:
            wa_flags = brw->vb.inputs[i].glarray->Size;
            break;

         case GL_INT_2_10_10_10_REV:
            wa_flags |= BRW_ATTRIB_WA_SIGN;
            /* fallthough */

         case GL_UNSIGNED_INT_2_10_10_10_REV:
            if (brw->vb.inputs[i].glarray->Format == GL_BGRA)
               wa_flags |= BRW_ATTRIB_WA_BGRA;

            if (brw->vb.inputs[i].glarray->Normalized)
               wa_flags |= BRW_ATTRIB_WA_NORMALIZE;
            else if (!brw->vb.inputs[i].glarray->Integer)
               wa_flags |= BRW_ATTRIB_WA_SCALE;

            break;
         }

         key.gl_attrib_wa_flags[i] = wa_flags;
      }
   }

   if (!brw_search_cache(&brw->cache, BRW_VS_PROG,
			 &key, sizeof(key),
			 &brw->vs.base.prog_offset, &brw->vs.prog_data)) {
      bool success =
         do_vs_prog(brw, ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX], vp,
                    &key);
      (void) success;
      assert(success);
   }
   brw->vs.base.prog_data = &brw->vs.prog_data->base.base;

   if (memcmp(&brw->vs.prog_data->base.vue_map, &brw->vue_map_geom_out,
              sizeof(brw->vue_map_geom_out)) != 0) {
      brw->vue_map_vs = brw->vs.prog_data->base.vue_map;
      brw->state.dirty.brw |= BRW_NEW_VUE_MAP_VS;
      if (brw->gen < 6) {
         /* No geometry shader support, so the VS VUE map is the VUE map for
          * the output of the "geometry" portion of the pipeline.
          */
         brw->vue_map_geom_out = brw->vue_map_vs;
         brw->state.dirty.brw |= BRW_NEW_VUE_MAP_GEOM_OUT;
      }
   }
}

/* See brw_vs.c:
 */
const struct brw_tracked_state brw_vs_prog = {
   .dirty = {
      .mesa  = (_NEW_TRANSFORM | _NEW_POLYGON | _NEW_POINT | _NEW_LIGHT |
		_NEW_TEXTURE |
		_NEW_BUFFERS),
      .brw   = (BRW_NEW_VERTEX_PROGRAM |
		BRW_NEW_VERTICES),
      .cache = 0
   },
   .emit = brw_upload_vs_prog
};

bool
brw_vs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_vs_prog_key key;
   uint32_t old_prog_offset = brw->vs.base.prog_offset;
   struct brw_vs_prog_data *old_prog_data = brw->vs.prog_data;
   bool success;

   if (!prog->_LinkedShaders[MESA_SHADER_VERTEX])
      return true;

   struct gl_vertex_program *vp = (struct gl_vertex_program *)
      prog->_LinkedShaders[MESA_SHADER_VERTEX]->Program;
   struct brw_vertex_program *bvp = brw_vertex_program(vp);

   memset(&key, 0, sizeof(key));

   brw_vec4_setup_prog_key_for_precompile(ctx, &key.base, bvp->id, &vp->Base);

   success = do_vs_prog(brw, prog, bvp, &key);

   brw->vs.base.prog_offset = old_prog_offset;
   brw->vs.prog_data = old_prog_data;

   return success;
}
