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
                                 struct brw_gs_compile *c,
                                 struct gl_shader_program *prog,
                                 void *mem_ctx,
                                 bool no_spills)
   : vec4_visitor(brw, &c->base, &c->gp->program.Base, &c->key.base,
                  &c->prog_data.base, prog, MESA_SHADER_GEOMETRY, mem_ctx,
                  INTEL_DEBUG & DEBUG_GS, no_spills,
                  ST_GS, ST_GS_WRITTEN, ST_GS_RESET),
     c(c)
{
}


dst_reg *
vec4_gs_visitor::make_reg_for_system_value(ir_variable *ir)
{
   dst_reg *reg = new(mem_ctx) dst_reg(this, ir->type);

   switch (ir->data.location) {
   case SYSTEM_VALUE_INVOCATION_ID:
      this->current_annotation = "initialize gl_InvocationID";
      emit(GS_OPCODE_GET_INSTANCE_ID, *reg);
      break;
   default:
      assert(!"not reached");
      break;
   }

   return reg;
}


int
vec4_gs_visitor::setup_varying_inputs(int payload_reg, int *attribute_map,
                                      int attributes_per_reg)
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

   for (int slot = 0; slot < c->input_vue_map.num_slots; slot++) {
      int varying = c->input_vue_map.slot_to_varying[slot];
      for (unsigned vertex = 0; vertex < num_input_vertices; vertex++) {
         attribute_map[BRW_VARYING_SLOT_COUNT * vertex + varying] =
            attributes_per_reg * payload_reg + input_array_stride * vertex +
            slot;
      }
   }

   int regs_used = ALIGN(input_array_stride * num_input_vertices,
                         attributes_per_reg) / attributes_per_reg;
   return payload_reg + regs_used;
}


void
vec4_gs_visitor::setup_payload()
{
   int attribute_map[BRW_VARYING_SLOT_COUNT * MAX_GS_INPUT_VERTICES];

   /* If we are in dual instanced mode, then attributes are going to be
    * interleaved, so one register contains two attribute slots.
    */
   int attributes_per_reg = c->prog_data.dual_instanced_dispatch ? 2 : 1;

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

   /* If the shader uses gl_PrimitiveIDIn, that goes in r1. */
   if (c->prog_data.include_primitive_id)
      attribute_map[VARYING_SLOT_PRIMITIVE_ID] = attributes_per_reg * reg++;

   reg = setup_uniforms(reg);

   reg = setup_varying_inputs(reg, attribute_map, attributes_per_reg);

   lower_attributes_to_hw_regs(attribute_map,
                               c->prog_data.dual_instanced_dispatch);

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
   vec4_instruction *inst = emit(GS_OPCODE_SET_DWORD_2_IMMED, r0, 0u);
   inst->force_writemask_all = true;

   /* Create a virtual register to hold the vertex count */
   this->vertex_count = src_reg(this, glsl_type::uint_type);

   /* Initialize the vertex_count register to 0 */
   this->current_annotation = "initialize vertex_count";
   inst = emit(MOV(dst_reg(this->vertex_count), 0u));
   inst->force_writemask_all = true;

   if (c->control_data_header_size_bits > 0) {
      /* Create a virtual register to hold the current set of control data
       * bits.
       */
      this->control_data_bits = src_reg(this, glsl_type::uint_type);

      /* If we're outputting more than 32 control data bits, then EmitVertex()
       * will set control_data_bits to 0 after emitting the first vertex.
       * Otherwise, we need to initialize it to 0 here.
       */
      if (c->control_data_header_size_bits <= 32) {
         this->current_annotation = "initialize control data bits";
         inst = emit(MOV(dst_reg(this->control_data_bits), 0u));
         inst->force_writemask_all = true;
      }
   }

   /* If the geometry shader uses the gl_PointSize input, we need to fix it up
    * to account for the fact that the vertex shader stored it in the w
    * component of VARYING_SLOT_PSIZ.
    */
   if (c->gp->program.Base.InputsRead & VARYING_BIT_PSIZ) {
      this->current_annotation = "swizzle gl_PointSize input";
      for (int vertex = 0; vertex < c->gp->program.VerticesIn; vertex++) {
         dst_reg dst(ATTR,
                     BRW_VARYING_SLOT_COUNT * vertex + VARYING_SLOT_PSIZ);
         dst.type = BRW_REGISTER_TYPE_F;
         src_reg src(dst);
         dst.writemask = WRITEMASK_X;
         src.swizzle = BRW_SWIZZLE_WWWW;
         inst = emit(MOV(dst, src));

         /* In dual instanced dispatch mode, dst has a width of 4, so we need
          * to make sure the MOV happens regardless of which channels are
          * enabled.
          */
         inst->force_writemask_all = true;
      }
   }

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
   if (c->control_data_header_size_bits > 0) {
      /* During shader execution, we only ever call emit_control_data_bits()
       * just prior to outputting a vertex.  Therefore, the control data bits
       * corresponding to the most recently output vertex still need to be
       * emitted.
       */
      current_annotation = "thread end: emit control data bits";
      emit_control_data_bits();
   }

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
   inst->offset = c->prog_data.control_data_header_size_hwords;

   /* We need to increment Global Offset by 1 to make room for Broadwell's
    * extra "Vertex Count" payload at the beginning of the URB entry.
    */
   if (brw->gen >= 8)
      inst->offset++;

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
   if (deref_var && deref_var->var->data.mode == ir_var_shader_in)
      return BRW_VARYING_SLOT_COUNT;
   else
      return vec4_visitor::compute_array_stride(ir);
}


/**
 * Write out a batch of 32 control data bits from the control_data_bits
 * register to the URB.
 *
 * The current value of the vertex_count register determines which DWORD in
 * the URB receives the control data bits.  The control_data_bits register is
 * assumed to contain the correct data for the vertex that was most recently
 * output, and all previous vertices that share the same DWORD.
 *
 * This function takes care of ensuring that if no vertices have been output
 * yet, no control bits are emitted.
 */
void
vec4_gs_visitor::emit_control_data_bits()
{
   assert(c->control_data_bits_per_vertex != 0);

   /* Since the URB_WRITE_OWORD message operates with 128-bit (vec4 sized)
    * granularity, we need to use two tricks to ensure that the batch of 32
    * control data bits is written to the appropriate DWORD in the URB.  To
    * select which vec4 we are writing to, we use the "slot {0,1} offset"
    * fields of the message header.  To select which DWORD in the vec4 we are
    * writing to, we use the channel mask fields of the message header.  To
    * avoid penalizing geometry shaders that emit a small number of vertices
    * with extra bookkeeping, we only do each of these tricks when
    * c->prog_data.control_data_header_size_bits is large enough to make it
    * necessary.
    *
    * Note: this means that if we're outputting just a single DWORD of control
    * data bits, we'll actually replicate it four times since we won't do any
    * channel masking.  But that's not a problem since in this case the
    * hardware only pays attention to the first DWORD.
    */
   enum brw_urb_write_flags urb_write_flags = BRW_URB_WRITE_OWORD;
   if (c->control_data_header_size_bits > 32)
      urb_write_flags = urb_write_flags | BRW_URB_WRITE_USE_CHANNEL_MASKS;
   if (c->control_data_header_size_bits > 128)
      urb_write_flags = urb_write_flags | BRW_URB_WRITE_PER_SLOT_OFFSET;

   /* If vertex_count is 0, then no control data bits have been accumulated
    * yet, so we should do nothing.
    */
   emit(CMP(dst_null_d(), this->vertex_count, 0u, BRW_CONDITIONAL_NEQ));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      /* If we are using either channel masks or a per-slot offset, then we
       * need to figure out which DWORD we are trying to write to, using the
       * formula:
       *
       *     dword_index = (vertex_count - 1) * bits_per_vertex / 32
       *
       * Since bits_per_vertex is a power of two, and is known at compile
       * time, this can be optimized to:
       *
       *     dword_index = (vertex_count - 1) >> (6 - log2(bits_per_vertex))
       */
      src_reg dword_index(this, glsl_type::uint_type);
      if (urb_write_flags) {
         src_reg prev_count(this, glsl_type::uint_type);
         emit(ADD(dst_reg(prev_count), this->vertex_count, 0xffffffffu));
         unsigned log2_bits_per_vertex =
            _mesa_fls(c->control_data_bits_per_vertex);
         emit(SHR(dst_reg(dword_index), prev_count,
                  (uint32_t) (6 - log2_bits_per_vertex)));
      }

      /* Start building the URB write message.  The first MRF gets a copy of
       * R0.
       */
      int base_mrf = 1;
      dst_reg mrf_reg(MRF, base_mrf);
      src_reg r0(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
      vec4_instruction *inst = emit(MOV(mrf_reg, r0));
      inst->force_writemask_all = true;

      if (urb_write_flags & BRW_URB_WRITE_PER_SLOT_OFFSET) {
         /* Set the per-slot offset to dword_index / 4, to that we'll write to
          * the appropriate OWORD within the control data header.
          */
         src_reg per_slot_offset(this, glsl_type::uint_type);
         emit(SHR(dst_reg(per_slot_offset), dword_index, 2u));
         emit(GS_OPCODE_SET_WRITE_OFFSET, mrf_reg, per_slot_offset, 1u);
      }

      if (urb_write_flags & BRW_URB_WRITE_USE_CHANNEL_MASKS) {
         /* Set the channel masks to 1 << (dword_index % 4), so that we'll
          * write to the appropriate DWORD within the OWORD.  We need to do
          * this computation with force_writemask_all, otherwise garbage data
          * from invocation 0 might clobber the mask for invocation 1 when
          * GS_OPCODE_PREPARE_CHANNEL_MASKS tries to OR the two masks
          * together.
          */
         src_reg channel(this, glsl_type::uint_type);
         inst = emit(AND(dst_reg(channel), dword_index, 3u));
         inst->force_writemask_all = true;
         src_reg one(this, glsl_type::uint_type);
         inst = emit(MOV(dst_reg(one), 1u));
         inst->force_writemask_all = true;
         src_reg channel_mask(this, glsl_type::uint_type);
         inst = emit(SHL(dst_reg(channel_mask), one, channel));
         inst->force_writemask_all = true;
         emit(GS_OPCODE_PREPARE_CHANNEL_MASKS, dst_reg(channel_mask),
                                               channel_mask);
         emit(GS_OPCODE_SET_CHANNEL_MASKS, mrf_reg, channel_mask);
      }

      /* Store the control data bits in the message payload and send it. */
      dst_reg mrf_reg2(MRF, base_mrf + 1);
      inst = emit(MOV(mrf_reg2, this->control_data_bits));
      inst->force_writemask_all = true;
      inst = emit(GS_OPCODE_URB_WRITE);
      inst->urb_write_flags = urb_write_flags;
      /* We need to increment Global Offset by 256-bits to make room for
       * Broadwell's extra "Vertex Count" payload at the beginning of the
       * URB entry.  Since this is an OWord message, Global Offset is counted
       * in 128-bit units, so we must set it to 2.
       */
      if (brw->gen >= 8)
         inst->offset = 2;
      inst->base_mrf = base_mrf;
      inst->mlen = 2;
   }
   emit(BRW_OPCODE_ENDIF);
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
      /* If we're outputting 32 control data bits or less, then we can wait
       * until the shader is over to output them all.  Otherwise we need to
       * output them as we go.  Now is the time to do it, since we're about to
       * output the vertex_count'th vertex, so it's guaranteed that the
       * control data bits associated with the (vertex_count - 1)th vertex are
       * correct.
       */
      if (c->control_data_header_size_bits > 32) {
         this->current_annotation = "emit vertex: emit control data bits";
         /* Only emit control data bits if we've finished accumulating a batch
          * of 32 bits.  This is the case when:
          *
          *     (vertex_count * bits_per_vertex) % 32 == 0
          *
          * (in other words, when the last 5 bits of vertex_count *
          * bits_per_vertex are 0).  Assuming bits_per_vertex == 2^n for some
          * integer n (which is always the case, since bits_per_vertex is
          * always 1 or 2), this is equivalent to requiring that the last 5-n
          * bits of vertex_count are 0:
          *
          *     vertex_count & (2^(5-n) - 1) == 0
          *
          * 2^(5-n) == 2^5 / 2^n == 32 / bits_per_vertex, so this is
          * equivalent to:
          *
          *     vertex_count & (32 / bits_per_vertex - 1) == 0
          */
         vec4_instruction *inst =
            emit(AND(dst_null_d(), this->vertex_count,
                     (uint32_t) (32 / c->control_data_bits_per_vertex - 1)));
         inst->conditional_mod = BRW_CONDITIONAL_Z;
         emit(IF(BRW_PREDICATE_NORMAL));
         {
            emit_control_data_bits();

            /* Reset control_data_bits to 0 so we can start accumulating a new
             * batch.
             *
             * Note: in the case where vertex_count == 0, this neutralizes the
             * effect of any call to EndPrimitive() that the shader may have
             * made before outputting its first vertex.
             */
            inst = emit(MOV(dst_reg(this->control_data_bits), 0u));
            inst->force_writemask_all = true;
         }
         emit(BRW_OPCODE_ENDIF);
      }

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
   /* We can only do EndPrimitive() functionality when the control data
    * consists of cut bits.  Fortunately, the only time it isn't is when the
    * output type is points, in which case EndPrimitive() is a no-op.
    */
   if (c->prog_data.control_data_format !=
       GEN7_GS_CONTROL_DATA_FORMAT_GSCTL_CUT) {
      return;
   }

   /* Cut bits use one bit per vertex. */
   assert(c->control_data_bits_per_vertex == 1);

   /* Cut bit n should be set to 1 if EndPrimitive() was called after emitting
    * vertex n, 0 otherwise.  So all we need to do here is mark bit
    * (vertex_count - 1) % 32 in the cut_bits register to indicate that
    * EndPrimitive() was called after emitting vertex (vertex_count - 1);
    * vec4_gs_visitor::emit_control_data_bits() will take care of the rest.
    *
    * Note that if EndPrimitve() is called before emitting any vertices, this
    * will cause us to set bit 31 of the control_data_bits register to 1.
    * That's fine because:
    *
    * - If max_vertices < 32, then vertex number 31 (zero-based) will never be
    *   output, so the hardware will ignore cut bit 31.
    *
    * - If max_vertices == 32, then vertex number 31 is guaranteed to be the
    *   last vertex, so setting cut bit 31 has no effect (since the primitive
    *   is automatically ended when the GS terminates).
    *
    * - If max_vertices > 32, then the ir_emit_vertex visitor will reset the
    *   control_data_bits register to 0 when the first vertex is emitted.
    */

   /* control_data_bits |= 1 << ((vertex_count - 1) % 32) */
   src_reg one(this, glsl_type::uint_type);
   emit(MOV(dst_reg(one), 1u));
   src_reg prev_count(this, glsl_type::uint_type);
   emit(ADD(dst_reg(prev_count), this->vertex_count, 0xffffffffu));
   src_reg mask(this, glsl_type::uint_type);
   /* Note: we're relying on the fact that the GEN SHL instruction only pays
    * attention to the lower 5 bits of its second source argument, so on this
    * architecture, 1 << (vertex_count - 1) is equivalent to 1 <<
    * ((vertex_count - 1) % 32).
    */
   emit(SHL(dst_reg(mask), one, prev_count));
   emit(OR(dst_reg(this->control_data_bits), this->control_data_bits, mask));
}

static const unsigned *
generate_assembly(struct brw_context *brw,
                  struct gl_shader_program *shader_prog,
                  struct gl_program *prog,
                  struct brw_vec4_prog_data *prog_data,
                  void *mem_ctx,
                  exec_list *instructions,
                  unsigned *final_assembly_size)
{
   if (brw->gen >= 8) {
      gen8_vec4_generator g(brw, shader_prog, prog, prog_data, mem_ctx,
                            INTEL_DEBUG & DEBUG_GS);
      return g.generate_assembly(instructions, final_assembly_size);
   } else {
      vec4_generator g(brw, shader_prog, prog, prog_data, mem_ctx,
                       INTEL_DEBUG & DEBUG_GS);
      return g.generate_assembly(instructions, final_assembly_size);
   }
}

extern "C" const unsigned *
brw_gs_emit(struct brw_context *brw,
            struct gl_shader_program *prog,
            struct brw_gs_compile *c,
            void *mem_ctx,
            unsigned *final_assembly_size)
{
   if (unlikely(INTEL_DEBUG & DEBUG_GS)) {
      struct brw_shader *shader =
         (brw_shader *) prog->_LinkedShaders[MESA_SHADER_GEOMETRY];

      brw_dump_ir(brw, "geometry", prog, &shader->base, NULL);
   }

   /* Compile the geometry shader in DUAL_OBJECT dispatch mode, if we can do
    * so without spilling. If the GS invocations count > 1, then we can't use
    * dual object mode.
    */
   if (c->prog_data.invocations <= 1 &&
       likely(!(INTEL_DEBUG & DEBUG_NO_DUAL_OBJECT_GS))) {
      c->prog_data.dual_instanced_dispatch = false;

      vec4_gs_visitor v(brw, c, prog, mem_ctx, true /* no_spills */);
      if (v.run()) {
         return generate_assembly(brw, prog, &c->gp->program.Base,
                                  &c->prog_data.base, mem_ctx, &v.instructions,
                                  final_assembly_size);
      }
   }

   /* Either we failed to compile in DUAL_OBJECT mode (probably because it
    * would have required spilling) or DUAL_OBJECT mode is disabled.  So fall
    * back to DUAL_INSTANCED mode, which consumes fewer registers.
    *
    * FIXME: In an ideal world we'd fall back to SINGLE mode, which would
    * allow us to interleave general purpose registers (resulting in even less
    * likelihood of spilling).  But at the moment, the vec4 generator and
    * visitor classes don't have the infrastructure to interleave general
    * purpose registers, so DUAL_INSTANCED is the best we can do.
    */
   c->prog_data.dual_instanced_dispatch = true;

   vec4_gs_visitor v(brw, c, prog, mem_ctx, false /* no_spills */);
   if (!v.run()) {
      prog->LinkStatus = false;
      ralloc_strcat(&prog->InfoLog, v.fail_msg);
      return NULL;
   }

   return generate_assembly(brw, prog, &c->gp->program.Base, &c->prog_data.base,
                            mem_ctx, &v.instructions, final_assembly_size);
}


} /* namespace brw */
