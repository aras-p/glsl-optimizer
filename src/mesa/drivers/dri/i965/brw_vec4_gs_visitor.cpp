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
 * \file brw_vec4_gs_visitor.cpp
 *
 * Geometry-shader-specific code derived from the vec4_visitor class.
 */

#include "brw_vec4_gs_visitor.h"

const unsigned MAX_GS_INPUT_VERTICES = 6;

namespace brw {

vec4_gs_visitor::vec4_gs_visitor(struct brw_context *brw,
                                 struct brw_vec4_gs_compile *c,
                                 struct gl_shader_program *prog,
                                 struct brw_shader *shader,
                                 void *mem_ctx)
   : vec4_visitor(brw, &c->base, &c->gp->program.Base, &c->key.base,
                  &c->prog_data.base, prog, shader, mem_ctx,
                  INTEL_DEBUG & DEBUG_GS),
     c(c)
{
}


dst_reg *
vec4_gs_visitor::make_reg_for_system_value(ir_variable *ir)
{
   /* Geometry shaders don't use any system values. */
   assert(!"Unreached");
   return NULL;
}


int
vec4_gs_visitor::setup_varying_inputs(int payload_reg, int *attribute_map)
{
   /* For geometry shaders there are N copies of the input attributes, where N
    * is the number of input vertices.  attribute_map[BRW_VARYING_SLOT_COUNT *
    * i + j] represents attribute j for vertex i.
    *
    * Note that GS inputs are read from the VUE 256 bits (2 vec4's) at a time,
    * so the total number of input slots that will be delivered to the GS (and
    * thus the stride of the input arrays) is urb_read_length * 2.
    */
   const unsigned num_input_vertices = c->gp->program.VerticesIn;
   assert(num_input_vertices <= MAX_GS_INPUT_VERTICES);
   unsigned input_array_stride = c->prog_data.base.urb_read_length * 2;

   for (int slot = 0; slot < c->key.input_vue_map.num_slots; slot++) {
      int varying = c->key.input_vue_map.slot_to_varying[slot];
      for (unsigned vertex = 0; vertex < num_input_vertices; vertex++) {
         attribute_map[BRW_VARYING_SLOT_COUNT * vertex + varying] =
            payload_reg + input_array_stride * vertex + slot;
      }
   }

   return payload_reg + input_array_stride * num_input_vertices;
}


void
vec4_gs_visitor::setup_payload()
{
   int attribute_map[BRW_VARYING_SLOT_COUNT * MAX_GS_INPUT_VERTICES];

   /* If a geometry shader tries to read from an input that wasn't written by
    * the vertex shader, that produces undefined results, but it shouldn't
    * crash anything.  So initialize attribute_map to zeros--that ensures that
    * these undefined results are read from r0.
    */
   memset(attribute_map, 0, sizeof(attribute_map));

   int reg = 0;

   /* The payload always contains important data in r0, which contains
    * the URB handles that are passed on to the URB write at the end
    * of the thread.
    */
   reg++;

   reg = setup_uniforms(reg);

   reg = setup_varying_inputs(reg, attribute_map);

   lower_attributes_to_hw_regs(attribute_map);

   this->first_non_payload_grf = reg;
}


void
vec4_gs_visitor::emit_prolog()
{
   /* In vertex shaders, r0.2 is guaranteed to be initialized to zero.  In
    * geometry shaders, it isn't (it contains a bunch of information we don't
    * need, like the input primitive type).  We need r0.2 to be zero in order
    * to build scratch read/write messages correctly (otherwise this value
    * will be interpreted as a global offset, causing us to do our scratch
    * reads/writes to garbage memory).  So just set it to zero at the top of
    * the shader.
    */
   this->current_annotation = "clear r0.2";
   dst_reg r0(retype(brw_vec4_grf(0, 0), BRW_REGISTER_TYPE_UD));
   emit(GS_OPCODE_SET_DWORD_2_IMMED, r0, 0u);

   /* Create a virtual register to hold the vertex count */
   this->vertex_count = src_reg(this, glsl_type::uint_type);

   /* Initialize the vertex_count register to 0 */
   this->current_annotation = "initialize vertex_count";
   vec4_instruction *inst = emit(MOV(dst_reg(this->vertex_count), 0u));
   inst->force_writemask_all = true;

   this->current_annotation = NULL;
}


void
vec4_gs_visitor::emit_program_code()
{
   /* We don't support NV_geometry_program4. */
   assert(!"Unreached");
}


void
vec4_gs_visitor::emit_thread_end()
{
   /* MRF 0 is reserved for the debugger, so start with message header
    * in MRF 1.
    */
   int base_mrf = 1;

   current_annotation = "thread end";
   dst_reg mrf_reg(MRF, base_mrf);
   src_reg r0(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   vec4_instruction *inst = emit(MOV(mrf_reg, r0));
   inst->force_writemask_all = true;
   emit(GS_OPCODE_SET_VERTEX_COUNT, mrf_reg, this->vertex_count);
   if (INTEL_DEBUG & DEBUG_SHADER_TIME)
      emit_shader_time_end();
   inst = emit(GS_OPCODE_THREAD_END);
   inst->base_mrf = base_mrf;
   inst->mlen = 1;
}


void
vec4_gs_visitor::emit_urb_write_header(int mrf)
{
   /* The SEND instruction that writes the vertex data to the VUE will use
    * per_slot_offset=true, which means that DWORDs 3 and 4 of the message
    * header specify an offset (in multiples of 256 bits) into the URB entry
    * at which the write should take place.
    *
    * So we have to prepare a message header with the appropriate offset
    * values.
    */
   dst_reg mrf_reg(MRF, mrf);
   src_reg r0(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   this->current_annotation = "URB write header";
   vec4_instruction *inst = emit(MOV(mrf_reg, r0));
   inst->force_writemask_all = true;
   emit(GS_OPCODE_SET_WRITE_OFFSET, mrf_reg, this->vertex_count,
        (uint32_t) c->prog_data.output_vertex_size_hwords);
}


vec4_instruction *
vec4_gs_visitor::emit_urb_write_opcode(bool complete)
{
   /* We don't care whether the vertex is complete, because in general
    * geometry shaders output multiple vertices, and we don't terminate the
    * thread until all vertices are complete.
    */
   (void) complete;

   vec4_instruction *inst = emit(GS_OPCODE_URB_WRITE);
   inst->urb_write_flags = BRW_URB_WRITE_PER_SLOT_OFFSET;
   return inst;
}


int
vec4_gs_visitor::compute_array_stride(ir_dereference_array *ir)
{
   /* Geometry shader inputs are arrays, but they use an unusual array layout:
    * instead of all array elements for a given geometry shader input being
    * stored consecutively, all geometry shader inputs are interleaved into
    * one giant array.  At this stage of compilation, we assume that the
    * stride of the array is BRW_VARYING_SLOT_COUNT.  Later,
    * setup_attributes() will remap our accesses to the actual input array.
    */
   ir_dereference_variable *deref_var = ir->array->as_dereference_variable();
   if (deref_var && deref_var->var->mode == ir_var_shader_in)
      return BRW_VARYING_SLOT_COUNT;
   else
      return vec4_visitor::compute_array_stride(ir);
}


void
vec4_gs_visitor::visit(ir_emit_vertex *)
{
   this->current_annotation = "emit vertex: safety check";

   /* To ensure that we don't output more vertices than the shader specified
    * using max_vertices, do the logic inside a conditional of the form "if
    * (vertex_count < MAX)"
    */
   unsigned num_output_vertices = c->gp->program.VerticesOut;
   emit(CMP(dst_null_d(), this->vertex_count,
            src_reg(num_output_vertices), BRW_CONDITIONAL_L));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      this->current_annotation = "emit vertex: vertex data";
      emit_vertex();

      this->current_annotation = "emit vertex: increment vertex count";
      emit(ADD(dst_reg(this->vertex_count), this->vertex_count,
               src_reg(1u)));
   }
   emit(BRW_OPCODE_ENDIF);

   this->current_annotation = NULL;
}

void
vec4_gs_visitor::visit(ir_end_primitive *)
{
   assert(!"Not implemented yet");
}


} /* namespace brw */
