/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file brw_vec4_gs.c
 *
 * State atom for client-programmable geometry shaders, and support code.
 */

#include "brw_vec4_gs.h"
#include "brw_context.h"
#include "brw_vec4_gs_visitor.h"
#include "brw_state.h"


static bool
do_gs_prog(struct brw_context *brw,
           struct gl_shader_program *prog,
           struct brw_geometry_program *gp,
           struct brw_gs_prog_key *key)
{
   struct brw_stage_state *stage_state = &brw->gs.base;
   struct brw_gs_compile c;
   memset(&c, 0, sizeof(c));
   c.key = *key;
   c.gp = gp;

   c.prog_data.include_primitive_id =
      (gp->program.Base.InputsRead & VARYING_BIT_PRIMITIVE_ID) != 0;

   c.prog_data.invocations = gp->program.Invocations;

   /* Allocate the references to the uniforms that will end up in the
    * prog_data associated with the compiled program, and which will be freed
    * by the state cache.
    *
    * Note: param_count needs to be num_uniform_components * 4, since we add
    * padding around uniform values below vec4 size, so the worst case is that
    * every uniform is a float which gets padded to the size of a vec4.
    */
   struct gl_shader *gs = prog->_LinkedShaders[MESA_SHADER_GEOMETRY];
   int param_count = gs->num_uniform_components * 4;

   /* We also upload clip plane data as uniforms */
   param_count += MAX_CLIP_PLANES * 4;

   c.prog_data.base.base.param =
      rzalloc_array(NULL, const float *, param_count);
   c.prog_data.base.base.pull_param =
      rzalloc_array(NULL, const float *, param_count);
   /* Setting nr_params here NOT to the size of the param and pull_param
    * arrays, but to the number of uniform components vec4_visitor
    * needs. vec4_visitor::setup_uniforms() will set it back to a proper value.
    */
   c.prog_data.base.base.nr_params = ALIGN(param_count, 4) / 4 + gs->num_samplers;

   if (gp->program.OutputType == GL_POINTS) {
      /* When the output type is points, the geometry shader may output data
       * to multiple streams, and EndPrimitive() has no effect.  So we
       * configure the hardware to interpret the control data as stream ID.
       */
      c.prog_data.control_data_format = GEN7_GS_CONTROL_DATA_FORMAT_GSCTL_SID;

      /* However, StreamID is not yet supported, so we output zero bits of
       * control data per vertex.
       */
      c.control_data_bits_per_vertex = 0;
   } else {
      /* When the output type is triangle_strip or line_strip, EndPrimitive()
       * may be used to terminate the current strip and start a new one
       * (similar to primitive restart), and outputting data to multiple
       * streams is not supported.  So we configure the hardware to interpret
       * the control data as EndPrimitive information (a.k.a. "cut bits").
       */
      c.prog_data.control_data_format = GEN7_GS_CONTROL_DATA_FORMAT_GSCTL_CUT;

      /* We only need to output control data if the shader actually calls
       * EndPrimitive().
       */
      c.control_data_bits_per_vertex = gp->program.UsesEndPrimitive ? 1 : 0;
   }
   c.control_data_header_size_bits =
      gp->program.VerticesOut * c.control_data_bits_per_vertex;

   /* 1 HWORD = 32 bytes = 256 bits */
   c.prog_data.control_data_header_size_hwords =
      ALIGN(c.control_data_header_size_bits, 256) / 256;

   GLbitfield64 outputs_written = gp->program.Base.OutputsWritten;

   /* In order for legacy clipping to work, we need to populate the clip
    * distance varying slots whenever clipping is enabled, even if the vertex
    * shader doesn't write to gl_ClipDistance.
    */
   if (c.key.base.userclip_active) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0);
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1);
   }

   brw_compute_vue_map(brw, &c.prog_data.base.vue_map, outputs_written);

   /* Compute the output vertex size.
    *
    * From the Ivy Bridge PRM, Vol2 Part1 7.2.1.1 STATE_GS - Output Vertex
    * Size (p168):
    *
    *     [0,62] indicating [1,63] 16B units
    *
    *     Specifies the size of each vertex stored in the GS output entry
    *     (following any Control Header data) as a number of 128-bit units
    *     (minus one).
    *
    *     Programming Restrictions: The vertex size must be programmed as a
    *     multiple of 32B units with the following exception: Rendering is
    *     disabled (as per SOL stage state) and the vertex size output by the
    *     GS thread is 16B.
    *
    *     If rendering is enabled (as per SOL state) the vertex size must be
    *     programmed as a multiple of 32B units. In other words, the only time
    *     software can program a vertex size with an odd number of 16B units
    *     is when rendering is disabled.
    *
    * Note: B=bytes in the above text.
    *
    * It doesn't seem worth the extra trouble to optimize the case where the
    * vertex size is 16B (especially since this would require special-casing
    * the GEN assembly that writes to the URB).  So we just set the vertex
    * size to a multiple of 32B (2 vec4's) in all cases.
    *
    * The maximum output vertex size is 62*16 = 992 bytes (31 hwords).  We
    * budget that as follows:
    *
    *   512 bytes for varyings (a varying component is 4 bytes and
    *             gl_MaxGeometryOutputComponents = 128)
    *    16 bytes overhead for VARYING_SLOT_PSIZ (each varying slot is 16
    *             bytes)
    *    16 bytes overhead for gl_Position (we allocate it a slot in the VUE
    *             even if it's not used)
    *    32 bytes overhead for gl_ClipDistance (we allocate it 2 VUE slots
    *             whenever clip planes are enabled, even if the shader doesn't
    *             write to gl_ClipDistance)
    *    16 bytes overhead since the VUE size must be a multiple of 32 bytes
    *             (see below)--this causes up to 1 VUE slot to be wasted
    *   400 bytes available for varying packing overhead
    *
    * Worst-case varying packing overhead is 3/4 of a varying slot (12 bytes)
    * per interpolation type, so this is plenty.
    *
    */
   unsigned output_vertex_size_bytes = c.prog_data.base.vue_map.num_slots * 16;
   assert(output_vertex_size_bytes <= GEN7_MAX_GS_OUTPUT_VERTEX_SIZE_BYTES);
   c.prog_data.output_vertex_size_hwords =
      ALIGN(output_vertex_size_bytes, 32) / 32;

   /* Compute URB entry size.  The maximum allowed URB entry size is 32k.
    * That divides up as follows:
    *
    *     64 bytes for the control data header (cut indices or StreamID bits)
    *   4096 bytes for varyings (a varying component is 4 bytes and
    *              gl_MaxGeometryTotalOutputComponents = 1024)
    *   4096 bytes overhead for VARYING_SLOT_PSIZ (each varying slot is 16
    *              bytes/vertex and gl_MaxGeometryOutputVertices is 256)
    *   4096 bytes overhead for gl_Position (we allocate it a slot in the VUE
    *              even if it's not used)
    *   8192 bytes overhead for gl_ClipDistance (we allocate it 2 VUE slots
    *              whenever clip planes are enabled, even if the shader doesn't
    *              write to gl_ClipDistance)
    *   4096 bytes overhead since the VUE size must be a multiple of 32
    *              bytes (see above)--this causes up to 1 VUE slot to be wasted
    *   8128 bytes available for varying packing overhead
    *
    * Worst-case varying packing overhead is 3/4 of a varying slot per
    * interpolation type, which works out to 3072 bytes, so this would allow
    * us to accommodate 2 interpolation types without any danger of running
    * out of URB space.
    *
    * In practice, the risk of running out of URB space is very small, since
    * the above figures are all worst-case, and most of them scale with the
    * number of output vertices.  So we'll just calculate the amount of space
    * we need, and if it's too large, fail to compile.
    */
   unsigned output_size_bytes =
      c.prog_data.output_vertex_size_hwords * 32 * gp->program.VerticesOut;
   output_size_bytes += 32 * c.prog_data.control_data_header_size_hwords;

   /* Broadwell stores "Vertex Count" as a full 8 DWord (32 byte) URB output,
    * which comes before the control header.
    */
   if (brw->gen >= 8)
      output_size_bytes += 32;

   assert(output_size_bytes >= 1);
   if (output_size_bytes > GEN7_MAX_GS_URB_ENTRY_SIZE_BYTES)
      return false;

   /* URB entry sizes are stored as a multiple of 64 bytes. */
   c.prog_data.base.urb_entry_size = ALIGN(output_size_bytes, 64) / 64;

   c.prog_data.output_topology = prim_to_hw_prim[gp->program.OutputType];

   brw_compute_vue_map(brw, &c.input_vue_map, c.key.input_varyings);

   /* GS inputs are read from the VUE 256 bits (2 vec4's) at a time, so we
    * need to program a URB read length of ceiling(num_slots / 2).
    */
   c.prog_data.base.urb_read_length = (c.input_vue_map.num_slots + 1) / 2;

   void *mem_ctx = ralloc_context(NULL);
   unsigned program_size;
   const unsigned *program =
      brw_gs_emit(brw, prog, &c, mem_ctx, &program_size);
   if (program == NULL) {
      ralloc_free(mem_ctx);
      return false;
   }

   /* Scratch space is used for register spilling */
   if (c.base.last_scratch) {
      perf_debug("Geometry shader triggered register spilling.  "
                 "Try reducing the number of live vec4 values to "
                 "improve performance.\n");

      c.prog_data.base.total_scratch
         = brw_get_scratch_size(c.base.last_scratch*REG_SIZE);

      brw_get_scratch_bo(brw, &stage_state->scratch_bo,
			 c.prog_data.base.total_scratch * brw->max_gs_threads);
   }

   brw_upload_cache(&brw->cache, BRW_GS_PROG,
                    &c.key, sizeof(c.key),
                    program, program_size,
                    &c.prog_data, sizeof(c.prog_data),
                    &stage_state->prog_offset, &brw->gs.prog_data);
   ralloc_free(mem_ctx);

   return true;
}


static void
brw_upload_gs_prog(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_stage_state *stage_state = &brw->gs.base;
   struct brw_gs_prog_key key;
   /* BRW_NEW_GEOMETRY_PROGRAM */
   struct brw_geometry_program *gp =
      (struct brw_geometry_program *) brw->geometry_program;

   if (gp == NULL) {
      /* No geometry shader.  Vertex data just passes straight through. */
      if (brw->state.dirty.brw & BRW_NEW_VUE_MAP_VS) {
         brw->vue_map_geom_out = brw->vue_map_vs;
         brw->state.dirty.brw |= BRW_NEW_VUE_MAP_GEOM_OUT;
      }

      /* Other state atoms had better not try to access prog_data, since
       * there's no GS program.
       */
      brw->gs.prog_data = NULL;
      brw->gs.base.prog_data = NULL;

      return;
   }

   struct gl_program *prog = &gp->program.Base;

   memset(&key, 0, sizeof(key));

   key.base.program_string_id = gp->id;
   brw_setup_vec4_key_clip_info(brw, &key.base,
                                gp->program.Base.UsesClipDistanceOut);

   /* _NEW_LIGHT | _NEW_BUFFERS */
   key.base.clamp_vertex_color = ctx->Light._ClampVertexColor;

   /* _NEW_TEXTURE */
   brw_populate_sampler_prog_key_data(ctx, prog, stage_state->sampler_count,
                                      &key.base.tex);

   /* BRW_NEW_VUE_MAP_VS */
   key.input_varyings = brw->vue_map_vs.slots_valid;

   if (!brw_search_cache(&brw->cache, BRW_GS_PROG,
                         &key, sizeof(key),
                         &stage_state->prog_offset, &brw->gs.prog_data)) {
      bool success =
         do_gs_prog(brw, ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY], gp,
                    &key);
      assert(success);
   }
   brw->gs.base.prog_data = &brw->gs.prog_data->base.base;

   if (memcmp(&brw->vs.prog_data->base.vue_map, &brw->vue_map_geom_out,
              sizeof(brw->vue_map_geom_out)) != 0) {
      brw->vue_map_geom_out = brw->gs.prog_data->base.vue_map;
      brw->state.dirty.brw |= BRW_NEW_VUE_MAP_GEOM_OUT;
   }
}


const struct brw_tracked_state brw_gs_prog = {
   .dirty = {
      .mesa  = (_NEW_LIGHT | _NEW_BUFFERS | _NEW_TEXTURE),
      .brw   = BRW_NEW_GEOMETRY_PROGRAM | BRW_NEW_VUE_MAP_VS,
   },
   .emit = brw_upload_gs_prog
};


bool
brw_gs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_gs_prog_key key;
   uint32_t old_prog_offset = brw->gs.base.prog_offset;
   struct brw_gs_prog_data *old_prog_data = brw->gs.prog_data;
   bool success;

   if (!prog->_LinkedShaders[MESA_SHADER_GEOMETRY])
      return true;

   struct gl_geometry_program *gp = (struct gl_geometry_program *)
      prog->_LinkedShaders[MESA_SHADER_GEOMETRY]->Program;
   struct brw_geometry_program *bgp = brw_geometry_program(gp);

   memset(&key, 0, sizeof(key));

   brw_vec4_setup_prog_key_for_precompile(ctx, &key.base, bgp->id, &gp->Base);

   /* Assume that the set of varyings coming in from the vertex shader exactly
    * matches what the geometry shader requires.
    */
   key.input_varyings = gp->Base.InputsRead;

   success = do_gs_prog(brw, prog, bgp, &key);

   brw->gs.base.prog_offset = old_prog_offset;
   brw->gs.prog_data = old_prog_data;

   return success;
}


bool
brw_gs_prog_data_compare(const void *in_a, const void *in_b)
{
   const struct brw_gs_prog_data *a = in_a;
   const struct brw_gs_prog_data *b = in_b;

   /* Compare the base structure. */
   if (!brw_stage_prog_data_compare(&a->base.base, &b->base.base))
      return false;

   /* Compare the rest of the struct. */
   const unsigned offset = sizeof(struct brw_stage_prog_data);
   if (memcmp(((char *) a) + offset, ((char *) b) + offset,
              sizeof(struct brw_gs_prog_data) - offset)) {
      return false;
   }

   return true;
}
