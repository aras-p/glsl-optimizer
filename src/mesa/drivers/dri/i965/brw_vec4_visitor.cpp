/*
 * Copyright © 2011 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_vec4.h"
#include "glsl/ir_uniform.h"
extern "C" {
#include "program/sampler.h"
}

namespace brw {

vec4_instruction::vec4_instruction(vec4_visitor *v,
				   enum opcode opcode, dst_reg dst,
				   src_reg src0, src_reg src1, src_reg src2)
{
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;
   this->src[1] = src1;
   this->src[2] = src2;
   this->saturate = false;
   this->force_writemask_all = false;
   this->no_dd_clear = false;
   this->no_dd_check = false;
   this->writes_accumulator = false;
   this->conditional_mod = BRW_CONDITIONAL_NONE;
   this->sampler = 0;
   this->texture_offset = 0;
   this->target = 0;
   this->shadow_compare = false;
   this->ir = v->base_ir;
   this->urb_write_flags = BRW_URB_WRITE_NO_FLAGS;
   this->header_present = false;
   this->mlen = 0;
   this->base_mrf = 0;
   this->offset = 0;
   this->annotation = v->current_annotation;
}

vec4_instruction *
vec4_visitor::emit(vec4_instruction *inst)
{
   this->instructions.push_tail(inst);

   return inst;
}

vec4_instruction *
vec4_visitor::emit_before(vec4_instruction *inst, vec4_instruction *new_inst)
{
   new_inst->ir = inst->ir;
   new_inst->annotation = inst->annotation;

   inst->insert_before(new_inst);

   return inst;
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst,
		   src_reg src0, src_reg src1, src_reg src2)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst,
					     src0, src1, src2));
}


vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst, src0, src1));
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst, src_reg src0)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst, src0));
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst));
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst_reg()));
}

#define ALU1(op)							\
   vec4_instruction *							\
   vec4_visitor::op(dst_reg dst, src_reg src0)				\
   {									\
      return new(mem_ctx) vec4_instruction(this, BRW_OPCODE_##op, dst,	\
					   src0);			\
   }

#define ALU2(op)							\
   vec4_instruction *							\
   vec4_visitor::op(dst_reg dst, src_reg src0, src_reg src1)		\
   {									\
      return new(mem_ctx) vec4_instruction(this, BRW_OPCODE_##op, dst,	\
					   src0, src1);			\
   }

#define ALU2_ACC(op)							\
   vec4_instruction *							\
   vec4_visitor::op(dst_reg dst, src_reg src0, src_reg src1)		\
   {									\
      vec4_instruction *inst = new(mem_ctx) vec4_instruction(this,     \
                       BRW_OPCODE_##op, dst, src0, src1);		\
      inst->writes_accumulator = true;                                 \
      return inst;                                                     \
   }

#define ALU3(op)							\
   vec4_instruction *							\
   vec4_visitor::op(dst_reg dst, src_reg src0, src_reg src1, src_reg src2)\
   {									\
      assert(brw->gen >= 6);						\
      return new(mem_ctx) vec4_instruction(this, BRW_OPCODE_##op, dst,	\
					   src0, src1, src2);		\
   }

ALU1(NOT)
ALU1(MOV)
ALU1(FRC)
ALU1(RNDD)
ALU1(RNDE)
ALU1(RNDZ)
ALU1(F32TO16)
ALU1(F16TO32)
ALU2(ADD)
ALU2(MUL)
ALU2_ACC(MACH)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(DP3)
ALU2(DP4)
ALU2(DPH)
ALU2(SHL)
ALU2(SHR)
ALU2(ASR)
ALU3(LRP)
ALU1(BFREV)
ALU3(BFE)
ALU2(BFI1)
ALU3(BFI2)
ALU1(FBH)
ALU1(FBL)
ALU1(CBIT)
ALU3(MAD)
ALU2_ACC(ADDC)
ALU2_ACC(SUBB)
ALU2(MAC)

/** Gen4 predicated IF. */
vec4_instruction *
vec4_visitor::IF(uint32_t predicate)
{
   vec4_instruction *inst;

   inst = new(mem_ctx) vec4_instruction(this, BRW_OPCODE_IF);
   inst->predicate = predicate;

   return inst;
}

/** Gen6 IF with embedded comparison. */
vec4_instruction *
vec4_visitor::IF(src_reg src0, src_reg src1, uint32_t condition)
{
   assert(brw->gen == 6);

   vec4_instruction *inst;

   resolve_ud_negate(&src0);
   resolve_ud_negate(&src1);

   inst = new(mem_ctx) vec4_instruction(this, BRW_OPCODE_IF, dst_null_d(),
					src0, src1);
   inst->conditional_mod = condition;

   return inst;
}

/**
 * CMP: Sets the low bit of the destination channels with the result
 * of the comparison, while the upper bits are undefined, and updates
 * the flag register with the packed 16 bits of the result.
 */
vec4_instruction *
vec4_visitor::CMP(dst_reg dst, src_reg src0, src_reg src1, uint32_t condition)
{
   vec4_instruction *inst;

   /* original gen4 does type conversion to the destination type
    * before before comparison, producing garbage results for floating
    * point comparisons.
    */
   if (brw->gen == 4) {
      dst.type = src0.type;
      if (dst.file == HW_REG)
	 dst.fixed_hw_reg.type = dst.type;
   }

   resolve_ud_negate(&src0);
   resolve_ud_negate(&src1);

   inst = new(mem_ctx) vec4_instruction(this, BRW_OPCODE_CMP, dst, src0, src1);
   inst->conditional_mod = condition;

   return inst;
}

vec4_instruction *
vec4_visitor::SCRATCH_READ(dst_reg dst, src_reg index)
{
   vec4_instruction *inst;

   inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_GEN4_SCRATCH_READ,
					dst, index);
   inst->base_mrf = 14;
   inst->mlen = 2;

   return inst;
}

vec4_instruction *
vec4_visitor::SCRATCH_WRITE(dst_reg dst, src_reg src, src_reg index)
{
   vec4_instruction *inst;

   inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_GEN4_SCRATCH_WRITE,
					dst, src, index);
   inst->base_mrf = 13;
   inst->mlen = 3;

   return inst;
}

void
vec4_visitor::emit_dp(dst_reg dst, src_reg src0, src_reg src1, unsigned elements)
{
   static enum opcode dot_opcodes[] = {
      BRW_OPCODE_DP2, BRW_OPCODE_DP3, BRW_OPCODE_DP4
   };

   emit(dot_opcodes[elements - 2], dst, src0, src1);
}

src_reg
vec4_visitor::fix_3src_operand(src_reg src)
{
   /* Using vec4 uniforms in SIMD4x2 programs is difficult. You'd like to be
    * able to use vertical stride of zero to replicate the vec4 uniform, like
    *
    *    g3<0;4,1>:f - [0, 4][1, 5][2, 6][3, 7]
    *
    * But you can't, since vertical stride is always four in three-source
    * instructions. Instead, insert a MOV instruction to do the replication so
    * that the three-source instruction can consume it.
    */

   /* The MOV is only needed if the source is a uniform or immediate. */
   if (src.file != UNIFORM && src.file != IMM)
      return src;

   if (src.file == UNIFORM && brw_is_single_value_swizzle(src.swizzle))
      return src;

   dst_reg expanded = dst_reg(this, glsl_type::vec4_type);
   expanded.type = src.type;
   emit(MOV(expanded, src));
   return src_reg(expanded);
}

src_reg
vec4_visitor::fix_math_operand(src_reg src)
{
   /* The gen6 math instruction ignores the source modifiers --
    * swizzle, abs, negate, and at least some parts of the register
    * region description.
    *
    * Rather than trying to enumerate all these cases, *always* expand the
    * operand to a temp GRF for gen6.
    *
    * For gen7, keep the operand as-is, except if immediate, which gen7 still
    * can't use.
    */

   if (brw->gen == 7 && src.file != IMM)
      return src;

   dst_reg expanded = dst_reg(this, glsl_type::vec4_type);
   expanded.type = src.type;
   emit(MOV(expanded, src));
   return src_reg(expanded);
}

void
vec4_visitor::emit_math1_gen6(enum opcode opcode, dst_reg dst, src_reg src)
{
   src = fix_math_operand(src);

   if (dst.writemask != WRITEMASK_XYZW) {
      /* The gen6 math instruction must be align1, so we can't do
       * writemasks.
       */
      dst_reg temp_dst = dst_reg(this, glsl_type::vec4_type);

      emit(opcode, temp_dst, src);

      emit(MOV(dst, src_reg(temp_dst)));
   } else {
      emit(opcode, dst, src);
   }
}

void
vec4_visitor::emit_math1_gen4(enum opcode opcode, dst_reg dst, src_reg src)
{
   vec4_instruction *inst = emit(opcode, dst, src);
   inst->base_mrf = 1;
   inst->mlen = 1;
}

void
vec4_visitor::emit_math(opcode opcode, dst_reg dst, src_reg src)
{
   switch (opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      break;
   default:
      assert(!"not reached: bad math opcode");
      return;
   }

   if (brw->gen >= 6) {
      return emit_math1_gen6(opcode, dst, src);
   } else {
      return emit_math1_gen4(opcode, dst, src);
   }
}

void
vec4_visitor::emit_math2_gen6(enum opcode opcode,
			      dst_reg dst, src_reg src0, src_reg src1)
{
   src0 = fix_math_operand(src0);
   src1 = fix_math_operand(src1);

   if (dst.writemask != WRITEMASK_XYZW) {
      /* The gen6 math instruction must be align1, so we can't do
       * writemasks.
       */
      dst_reg temp_dst = dst_reg(this, glsl_type::vec4_type);
      temp_dst.type = dst.type;

      emit(opcode, temp_dst, src0, src1);

      emit(MOV(dst, src_reg(temp_dst)));
   } else {
      emit(opcode, dst, src0, src1);
   }
}

void
vec4_visitor::emit_math2_gen4(enum opcode opcode,
			      dst_reg dst, src_reg src0, src_reg src1)
{
   vec4_instruction *inst = emit(opcode, dst, src0, src1);
   inst->base_mrf = 1;
   inst->mlen = 2;
}

void
vec4_visitor::emit_math(enum opcode opcode,
			dst_reg dst, src_reg src0, src_reg src1)
{
   switch (opcode) {
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      break;
   default:
      assert(!"not reached: unsupported binary math opcode");
      return;
   }

   if (brw->gen >= 6) {
      return emit_math2_gen6(opcode, dst, src0, src1);
   } else {
      return emit_math2_gen4(opcode, dst, src0, src1);
   }
}

void
vec4_visitor::emit_pack_half_2x16(dst_reg dst, src_reg src0)
{
   if (brw->gen < 7)
      assert(!"ir_unop_pack_half_2x16 should be lowered");

   assert(dst.type == BRW_REGISTER_TYPE_UD);
   assert(src0.type == BRW_REGISTER_TYPE_F);

   /* From the Ivybridge PRM, Vol4, Part3, Section 6.27 f32to16:
    *
    *   Because this instruction does not have a 16-bit floating-point type,
    *   the destination data type must be Word (W).
    *
    *   The destination must be DWord-aligned and specify a horizontal stride
    *   (HorzStride) of 2. The 16-bit result is stored in the lower word of
    *   each destination channel and the upper word is not modified.
    *
    * The above restriction implies that the f32to16 instruction must use
    * align1 mode, because only in align1 mode is it possible to specify
    * horizontal stride.  We choose here to defy the hardware docs and emit
    * align16 instructions.
    *
    * (I [chadv] did attempt to emit align1 instructions for VS f32to16
    * instructions. I was partially successful in that the code passed all
    * tests.  However, the code was dubiously correct and fragile, and the
    * tests were not harsh enough to probe that frailty. Not trusting the
    * code, I chose instead to remain in align16 mode in defiance of the hw
    * docs).
    *
    * I've [chadv] experimentally confirmed that, on gen7 hardware and the
    * simulator, emitting a f32to16 in align16 mode with UD as destination
    * data type is safe. The behavior differs from that specified in the PRM
    * in that the upper word of each destination channel is cleared to 0.
    */

   dst_reg tmp_dst(this, glsl_type::uvec2_type);
   src_reg tmp_src(tmp_dst);

#if 0
   /* Verify the undocumented behavior on which the following instructions
    * rely.  If f32to16 fails to clear the upper word of the X and Y channels,
    * then the result of the bit-or instruction below will be incorrect.
    *
    * You should inspect the disasm output in order to verify that the MOV is
    * not optimized away.
    */
   emit(MOV(tmp_dst, src_reg(0x12345678u)));
#endif

   /* Give tmp the form below, where "." means untouched.
    *
    *     w z          y          x w z          y          x
    *   |.|.|0x0000hhhh|0x0000llll|.|.|0x0000hhhh|0x0000llll|
    *
    * That the upper word of each write-channel be 0 is required for the
    * following bit-shift and bit-or instructions to work. Note that this
    * relies on the undocumented hardware behavior mentioned above.
    */
   tmp_dst.writemask = WRITEMASK_XY;
   emit(F32TO16(tmp_dst, src0));

   /* Give the write-channels of dst the form:
    *   0xhhhh0000
    */
   tmp_src.swizzle = BRW_SWIZZLE_YYYY;
   emit(SHL(dst, tmp_src, src_reg(16u)));

   /* Finally, give the write-channels of dst the form of packHalf2x16's
    * output:
    *   0xhhhhllll
    */
   tmp_src.swizzle = BRW_SWIZZLE_XXXX;
   emit(OR(dst, src_reg(dst), tmp_src));
}

void
vec4_visitor::emit_unpack_half_2x16(dst_reg dst, src_reg src0)
{
   if (brw->gen < 7)
      assert(!"ir_unop_unpack_half_2x16 should be lowered");

   assert(dst.type == BRW_REGISTER_TYPE_F);
   assert(src0.type == BRW_REGISTER_TYPE_UD);

   /* From the Ivybridge PRM, Vol4, Part3, Section 6.26 f16to32:
    *
    *   Because this instruction does not have a 16-bit floating-point type,
    *   the source data type must be Word (W). The destination type must be
    *   F (Float).
    *
    * To use W as the source data type, we must adjust horizontal strides,
    * which is only possible in align1 mode. All my [chadv] attempts at
    * emitting align1 instructions for unpackHalf2x16 failed to pass the
    * Piglit tests, so I gave up.
    *
    * I've verified that, on gen7 hardware and the simulator, it is safe to
    * emit f16to32 in align16 mode with UD as source data type.
    */

   dst_reg tmp_dst(this, glsl_type::uvec2_type);
   src_reg tmp_src(tmp_dst);

   tmp_dst.writemask = WRITEMASK_X;
   emit(AND(tmp_dst, src0, src_reg(0xffffu)));

   tmp_dst.writemask = WRITEMASK_Y;
   emit(SHR(tmp_dst, src0, src_reg(16u)));

   dst.writemask = WRITEMASK_XY;
   emit(F16TO32(dst, tmp_src));
}

void
vec4_visitor::visit_instructions(const exec_list *list)
{
   foreach_list(node, list) {
      ir_instruction *ir = (ir_instruction *)node;

      base_ir = ir;
      ir->accept(this);
   }
}


static int
type_size(const struct glsl_type *type)
{
   unsigned int i;
   int size;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      if (type->is_matrix()) {
	 return type->matrix_columns;
      } else {
	 /* Regardless of size of vector, it gets a vec4. This is bad
	  * packing for things like floats, but otherwise arrays become a
	  * mess.  Hopefully a later pass over the code can pack scalars
	  * down if appropriate.
	  */
	 return 1;
      }
   case GLSL_TYPE_ARRAY:
      assert(type->length > 0);
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up one slot in UNIFORMS[], but they're baked in
       * at link time.
       */
      return 1;
   case GLSL_TYPE_ATOMIC_UINT:
      return 0;
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_INTERFACE:
      assert(0);
      break;
   }

   return 0;
}

int
vec4_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_count) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = reralloc(mem_ctx, virtual_grf_sizes, int,
				   virtual_grf_array_size);
      virtual_grf_reg_map = reralloc(mem_ctx, virtual_grf_reg_map, int,
				     virtual_grf_array_size);
   }
   virtual_grf_reg_map[virtual_grf_count] = virtual_grf_reg_count;
   virtual_grf_reg_count += size;
   virtual_grf_sizes[virtual_grf_count] = size;
   return virtual_grf_count++;
}

src_reg::src_reg(class vec4_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));

   if (type->is_array() || type->is_record()) {
      this->swizzle = BRW_SWIZZLE_NOOP;
   } else {
      this->swizzle = swizzle_for_size(type->vector_elements);
   }

   this->type = brw_type_for_base_type(type);
}

dst_reg::dst_reg(class vec4_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));

   if (type->is_array() || type->is_record()) {
      this->writemask = WRITEMASK_XYZW;
   } else {
      this->writemask = (1 << type->vector_elements) - 1;
   }

   this->type = brw_type_for_base_type(type);
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
void
vec4_visitor::setup_uniform_values(ir_variable *ir)
{
   int namelen = strlen(ir->name);

   /* The data for our (non-builtin) uniforms is stored in a series of
    * gl_uniform_driver_storage structs for each subcomponent that
    * glGetUniformLocation() could name.  We know it's been set up in the same
    * order we'd walk the type, so walk the list of storage and find anything
    * with our name, or the prefix of a component that starts with our name.
    */
   for (unsigned u = 0; u < shader_prog->NumUserUniformStorage; u++) {
      struct gl_uniform_storage *storage = &shader_prog->UniformStorage[u];

      if (strncmp(ir->name, storage->name, namelen) != 0 ||
          (storage->name[namelen] != 0 &&
           storage->name[namelen] != '.' &&
           storage->name[namelen] != '[')) {
         continue;
      }

      gl_constant_value *components = storage->storage;
      unsigned vector_count = (MAX2(storage->array_elements, 1) *
                               storage->type->matrix_columns);

      for (unsigned s = 0; s < vector_count; s++) {
         assert(uniforms < uniform_array_size);
         uniform_vector_size[uniforms] = storage->type->vector_elements;

         int i;
         for (i = 0; i < uniform_vector_size[uniforms]; i++) {
            stage_prog_data->param[uniforms * 4 + i] = &components->f;
            components++;
         }
         for (; i < 4; i++) {
            static float zero = 0;
            stage_prog_data->param[uniforms * 4 + i] = &zero;
         }

         uniforms++;
      }
   }
}

void
vec4_visitor::setup_uniform_clipplane_values()
{
   gl_clip_plane *clip_planes = brw_select_clip_planes(ctx);

   for (int i = 0; i < key->nr_userclip_plane_consts; ++i) {
      assert(this->uniforms < uniform_array_size);
      this->uniform_vector_size[this->uniforms] = 4;
      this->userplane[i] = dst_reg(UNIFORM, this->uniforms);
      this->userplane[i].type = BRW_REGISTER_TYPE_F;
      for (int j = 0; j < 4; ++j) {
         stage_prog_data->param[this->uniforms * 4 + j] = &clip_planes[i][j];
      }
      ++this->uniforms;
   }
}

/* Our support for builtin uniforms is even scarier than non-builtin.
 * It sits on top of the PROG_STATE_VAR parameters that are
 * automatically updated from GL context state.
 */
void
vec4_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const ir_state_slot *const slots = ir->state_slots;
   assert(ir->state_slots != NULL);

   for (unsigned int i = 0; i < ir->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa,
       * but we'll get the same index back here.  We can reference
       * ParameterValues directly, since unlike brw_fs.cpp, we never
       * add new state references during compile.
       */
      int index = _mesa_add_state_reference(this->prog->Parameters,
					    (gl_state_index *)slots[i].tokens);
      float *values = &this->prog->Parameters->ParameterValues[index][0].f;

      assert(this->uniforms < uniform_array_size);
      this->uniform_vector_size[this->uniforms] = 0;
      /* Add each of the unique swizzled channels of the element.
       * This will end up matching the size of the glsl_type of this field.
       */
      int last_swiz = -1;
      for (unsigned int j = 0; j < 4; j++) {
	 int swiz = GET_SWZ(slots[i].swizzle, j);
	 last_swiz = swiz;

	 stage_prog_data->param[this->uniforms * 4 + j] = &values[swiz];
	 assert(this->uniforms < uniform_array_size);
	 if (swiz <= last_swiz)
	    this->uniform_vector_size[this->uniforms]++;
      }
      this->uniforms++;
   }
}

dst_reg *
vec4_visitor::variable_storage(ir_variable *var)
{
   return (dst_reg *)hash_table_find(this->variable_ht, var);
}

void
vec4_visitor::emit_bool_to_cond_code(ir_rvalue *ir, uint32_t *predicate)
{
   ir_expression *expr = ir->as_expression();

   *predicate = BRW_PREDICATE_NORMAL;

   if (expr) {
      src_reg op[2];
      vec4_instruction *inst;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 expr->operands[i]->accept(this);
	 op[i] = this->result;

	 resolve_ud_negate(&op[i]);
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(AND(dst_null_d(), op[0], src_reg(1)));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;

      case ir_binop_logic_xor:
	 inst = emit(XOR(dst_null_d(), op[0], op[1]));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_or:
	 inst = emit(OR(dst_null_d(), op[0], op[1]));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_and:
	 inst = emit(AND(dst_null_d(), op[0], op[1]));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_f2b:
	 if (brw->gen >= 6) {
	    emit(CMP(dst_null_d(), op[0], src_reg(0.0f), BRW_CONDITIONAL_NZ));
	 } else {
	    inst = emit(MOV(dst_null_f(), op[0]));
	    inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 }
	 break;

      case ir_unop_i2b:
	 if (brw->gen >= 6) {
	    emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 } else {
	    inst = emit(MOV(dst_null_d(), op[0]));
	    inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 }
	 break;

      case ir_binop_all_equal:
	 inst = emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_Z));
	 *predicate = BRW_PREDICATE_ALIGN16_ALL4H;
	 break;

      case ir_binop_any_nequal:
	 inst = emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_NZ));
	 *predicate = BRW_PREDICATE_ALIGN16_ANY4H;
	 break;

      case ir_unop_any:
	 inst = emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 *predicate = BRW_PREDICATE_ALIGN16_ANY4H;
	 break;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_nequal:
	 emit(CMP(dst_null_d(), op[0], op[1],
		  brw_conditional_for_comparison(expr->operation)));
	 break;

      default:
	 assert(!"not reached");
	 break;
      }
      return;
   }

   ir->accept(this);

   resolve_ud_negate(&this->result);

   if (brw->gen >= 6) {
      vec4_instruction *inst = emit(AND(dst_null_d(),
					this->result, src_reg(1)));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   } else {
      vec4_instruction *inst = emit(MOV(dst_null_d(), this->result));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   }
}

/**
 * Emit a gen6 IF statement with the comparison folded into the IF
 * instruction.
 */
void
vec4_visitor::emit_if_gen6(ir_if *ir)
{
   ir_expression *expr = ir->condition->as_expression();

   if (expr) {
      src_reg op[2];
      dst_reg temp;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_Z));
	 return;

      case ir_binop_logic_xor:
	 emit(IF(op[0], op[1], BRW_CONDITIONAL_NZ));
	 return;

      case ir_binop_logic_or:
	 temp = dst_reg(this, glsl_type::bool_type);
	 emit(OR(temp, op[0], op[1]));
	 emit(IF(src_reg(temp), src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_binop_logic_and:
	 temp = dst_reg(this, glsl_type::bool_type);
	 emit(AND(temp, op[0], op[1]));
	 emit(IF(src_reg(temp), src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_unop_f2b:
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_unop_i2b:
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_nequal:
	 emit(IF(op[0], op[1],
		 brw_conditional_for_comparison(expr->operation)));
	 return;

      case ir_binop_all_equal:
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_Z));
	 emit(IF(BRW_PREDICATE_ALIGN16_ALL4H));
	 return;

      case ir_binop_any_nequal:
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_NZ));
	 emit(IF(BRW_PREDICATE_ALIGN16_ANY4H));
	 return;

      case ir_unop_any:
	 emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 emit(IF(BRW_PREDICATE_ALIGN16_ANY4H));
	 return;

      default:
	 assert(!"not reached");
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 return;
      }
      return;
   }

   ir->condition->accept(this);

   emit(IF(this->result, src_reg(0), BRW_CONDITIONAL_NZ));
}

void
vec4_visitor::visit(ir_variable *ir)
{
   dst_reg *reg = NULL;

   if (variable_storage(ir))
      return;

   switch (ir->data.mode) {
   case ir_var_shader_in:
      reg = new(mem_ctx) dst_reg(ATTR, ir->data.location);
      break;

   case ir_var_shader_out:
      reg = new(mem_ctx) dst_reg(this, ir->type);

      for (int i = 0; i < type_size(ir->type); i++) {
	 output_reg[ir->data.location + i] = *reg;
	 output_reg[ir->data.location + i].reg_offset = i;
	 output_reg[ir->data.location + i].type =
            brw_type_for_base_type(ir->type->get_scalar_type());
	 output_reg_annotation[ir->data.location + i] = ir->name;
      }
      break;

   case ir_var_auto:
   case ir_var_temporary:
      reg = new(mem_ctx) dst_reg(this, ir->type);
      break;

   case ir_var_uniform:
      reg = new(this->mem_ctx) dst_reg(UNIFORM, this->uniforms);

      /* Thanks to the lower_ubo_reference pass, we will see only
       * ir_binop_ubo_load expressions and not ir_dereference_variable for UBO
       * variables, so no need for them to be in variable_ht.
       *
       * Atomic counters take no uniform storage, no need to do
       * anything here.
       */
      if (ir->is_in_uniform_block() || ir->type->contains_atomic())
         return;

      /* Track how big the whole uniform variable is, in case we need to put a
       * copy of its data into pull constants for array access.
       */
      assert(this->uniforms < uniform_array_size);
      this->uniform_size[this->uniforms] = type_size(ir->type);

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir);
      }
      break;

   case ir_var_system_value:
      reg = make_reg_for_system_value(ir);
      break;

   default:
      assert(!"not reached");
   }

   reg->type = brw_type_for_base_type(ir->type);
   hash_table_insert(this->variable_ht, reg, ir);
}

void
vec4_visitor::visit(ir_loop *ir)
{
   /* We don't want debugging output to print the whole body of the
    * loop as the annotation.
    */
   this->base_ir = NULL;

   emit(BRW_OPCODE_DO);

   visit_instructions(&ir->body_instructions);

   emit(BRW_OPCODE_WHILE);
}

void
vec4_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      emit(BRW_OPCODE_BREAK);
      break;
   case ir_loop_jump::jump_continue:
      emit(BRW_OPCODE_CONTINUE);
      break;
   }
}


void
vec4_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
}

void
vec4_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(NULL, &empty);

      assert(sig);

      visit_instructions(&sig->body);
   }
}

bool
vec4_visitor::try_emit_sat(ir_expression *ir)
{
   ir_rvalue *sat_src = ir->as_rvalue_to_saturate();
   if (!sat_src)
      return false;

   sat_src->accept(this);
   src_reg src = this->result;

   this->result = src_reg(this, ir->type);
   vec4_instruction *inst;
   inst = emit(MOV(dst_reg(this->result), src));
   inst->saturate = true;

   return true;
}

bool
vec4_visitor::try_emit_mad(ir_expression *ir)
{
   /* 3-src instructions were introduced in gen6. */
   if (brw->gen < 6)
      return false;

   /* MAD can only handle floating-point data. */
   if (ir->type->base_type != GLSL_TYPE_FLOAT)
      return false;

   ir_rvalue *nonmul = ir->operands[1];
   ir_expression *mul = ir->operands[0]->as_expression();

   if (!mul || mul->operation != ir_binop_mul) {
      nonmul = ir->operands[0];
      mul = ir->operands[1]->as_expression();

      if (!mul || mul->operation != ir_binop_mul)
         return false;
   }

   nonmul->accept(this);
   src_reg src0 = fix_3src_operand(this->result);

   mul->operands[0]->accept(this);
   src_reg src1 = fix_3src_operand(this->result);

   mul->operands[1]->accept(this);
   src_reg src2 = fix_3src_operand(this->result);

   this->result = src_reg(this, ir->type);
   emit(BRW_OPCODE_MAD, dst_reg(this->result), src0, src1, src2);

   return true;
}

void
vec4_visitor::emit_bool_comparison(unsigned int op,
				 dst_reg dst, src_reg src0, src_reg src1)
{
   /* original gen4 does destination conversion before comparison. */
   if (brw->gen < 5)
      dst.type = src0.type;

   emit(CMP(dst, src0, src1, brw_conditional_for_comparison(op)));

   dst.type = BRW_REGISTER_TYPE_D;
   emit(AND(dst, src_reg(dst), src_reg(0x1)));
}

void
vec4_visitor::emit_minmax(uint32_t conditionalmod, dst_reg dst,
                          src_reg src0, src_reg src1)
{
   vec4_instruction *inst;

   if (brw->gen >= 6) {
      inst = emit(BRW_OPCODE_SEL, dst, src0, src1);
      inst->conditional_mod = conditionalmod;
   } else {
      emit(CMP(dst, src0, src1, conditionalmod));

      inst = emit(BRW_OPCODE_SEL, dst, src0, src1);
      inst->predicate = BRW_PREDICATE_NORMAL;
   }
}

void
vec4_visitor::emit_lrp(const dst_reg &dst,
                       const src_reg &x, const src_reg &y, const src_reg &a)
{
   if (brw->gen >= 6) {
      /* Note that the instruction's argument order is reversed from GLSL
       * and the IR.
       */
      emit(LRP(dst,
               fix_3src_operand(a), fix_3src_operand(y), fix_3src_operand(x)));
   } else {
      /* Earlier generations don't support three source operations, so we
       * need to emit x*(1-a) + y*a.
       */
      dst_reg y_times_a           = dst_reg(this, glsl_type::vec4_type);
      dst_reg one_minus_a         = dst_reg(this, glsl_type::vec4_type);
      dst_reg x_times_one_minus_a = dst_reg(this, glsl_type::vec4_type);
      y_times_a.writemask           = dst.writemask;
      one_minus_a.writemask         = dst.writemask;
      x_times_one_minus_a.writemask = dst.writemask;

      emit(MUL(y_times_a, y, a));
      emit(ADD(one_minus_a, negate(a), src_reg(1.0f)));
      emit(MUL(x_times_one_minus_a, x, src_reg(one_minus_a)));
      emit(ADD(dst, src_reg(x_times_one_minus_a), src_reg(y_times_a)));
   }
}

void
vec4_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   src_reg op[Elements(ir->operands)];
   src_reg result_src;
   dst_reg result_dst;
   vec4_instruction *inst;

   if (try_emit_sat(ir))
      return;

   if (ir->operation == ir_binop_add) {
      if (try_emit_mad(ir))
	 return;
   }

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result.file = BAD_FILE;
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 fprintf(stderr, "Failed to get tree for expression operand:\n");
	 ir->operands[operand]->fprint(stderr);
	 exit(1);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
   }

   int vector_elements = ir->operands[0]->type->vector_elements;
   if (ir->operands[1]) {
      vector_elements = MAX2(vector_elements,
			     ir->operands[1]->type->vector_elements);
   }

   this->result.file = BAD_FILE;

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = src_reg(this, ir->type);
   /* convenience for the emit functions below. */
   result_dst = dst_reg(result_src);
   /* If nothing special happens, this is the result. */
   this->result = result_src;
   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   switch (ir->operation) {
   case ir_unop_logic_not:
      /* Note that BRW_OPCODE_NOT is not appropriate here, since it is
       * ones complement of the whole register, not just bit 0.
       */
      emit(XOR(result_dst, op[0], src_reg(1)));
      break;
   case ir_unop_neg:
      op[0].negate = !op[0].negate;
      emit(MOV(result_dst, op[0]));
      break;
   case ir_unop_abs:
      op[0].abs = true;
      op[0].negate = false;
      emit(MOV(result_dst, op[0]));
      break;

   case ir_unop_sign:
      if (ir->type->is_float()) {
         /* AND(val, 0x80000000) gives the sign bit.
          *
          * Predicated OR ORs 1.0 (0x3f800000) with the sign bit if val is not
          * zero.
          */
         emit(CMP(dst_null_f(), op[0], src_reg(0.0f), BRW_CONDITIONAL_NZ));

         op[0].type = BRW_REGISTER_TYPE_UD;
         result_dst.type = BRW_REGISTER_TYPE_UD;
         emit(AND(result_dst, op[0], src_reg(0x80000000u)));

         inst = emit(OR(result_dst, src_reg(result_dst), src_reg(0x3f800000u)));
         inst->predicate = BRW_PREDICATE_NORMAL;

         this->result.type = BRW_REGISTER_TYPE_F;
      } else {
         /*  ASR(val, 31) -> negative val generates 0xffffffff (signed -1).
          *               -> non-negative val generates 0x00000000.
          *  Predicated OR sets 1 if val is positive.
          */
         emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_G));

         emit(ASR(result_dst, op[0], src_reg(31)));

         inst = emit(OR(result_dst, src_reg(result_dst), src_reg(1)));
         inst->predicate = BRW_PREDICATE_NORMAL;
      }
      break;

   case ir_unop_rcp:
      emit_math(SHADER_OPCODE_RCP, result_dst, op[0]);
      break;

   case ir_unop_exp2:
      emit_math(SHADER_OPCODE_EXP2, result_dst, op[0]);
      break;
   case ir_unop_log2:
      emit_math(SHADER_OPCODE_LOG2, result_dst, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_sin:
   case ir_unop_sin_reduced:
      emit_math(SHADER_OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos:
   case ir_unop_cos_reduced:
      emit_math(SHADER_OPCODE_COS, result_dst, op[0]);
      break;

   case ir_unop_dFdx:
   case ir_unop_dFdy:
      assert(!"derivatives not valid in vertex shader");
      break;

   case ir_unop_bitfield_reverse:
      emit(BFREV(result_dst, op[0]));
      break;
   case ir_unop_bit_count:
      emit(CBIT(result_dst, op[0]));
      break;
   case ir_unop_find_msb: {
      src_reg temp = src_reg(this, glsl_type::uint_type);

      inst = emit(FBH(dst_reg(temp), op[0]));
      inst->dst.writemask = WRITEMASK_XYZW;

      /* FBH counts from the MSB side, while GLSL's findMSB() wants the count
       * from the LSB side. If FBH didn't return an error (0xFFFFFFFF), then
       * subtract the result from 31 to convert the MSB count into an LSB count.
       */

      /* FBH only supports UD type for dst, so use a MOV to convert UD to D. */
      temp.swizzle = BRW_SWIZZLE_NOOP;
      emit(MOV(result_dst, temp));

      src_reg src_tmp = src_reg(result_dst);
      emit(CMP(dst_null_d(), src_tmp, src_reg(-1), BRW_CONDITIONAL_NZ));

      src_tmp.negate = true;
      inst = emit(ADD(result_dst, src_tmp, src_reg(31)));
      inst->predicate = BRW_PREDICATE_NORMAL;
      break;
   }
   case ir_unop_find_lsb:
      emit(FBL(result_dst, op[0]));
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
      break;

   case ir_binop_add:
      emit(ADD(result_dst, op[0], op[1]));
      break;
   case ir_binop_sub:
      assert(!"not reached: should be handled by ir_sub_to_add_neg");
      break;

   case ir_binop_mul:
      if (brw->gen < 8 && ir->type->is_integer()) {
	 /* For integer multiplication, the MUL uses the low 16 bits of one of
	  * the operands (src0 through SNB, src1 on IVB and later).  The MACH
	  * accumulates in the contribution of the upper 16 bits of that
	  * operand.  If we can determine that one of the args is in the low
	  * 16 bits, though, we can just emit a single MUL.
          */
         if (ir->operands[0]->is_uint16_constant()) {
            if (brw->gen < 7)
               emit(MUL(result_dst, op[0], op[1]));
            else
               emit(MUL(result_dst, op[1], op[0]));
         } else if (ir->operands[1]->is_uint16_constant()) {
            if (brw->gen < 7)
               emit(MUL(result_dst, op[1], op[0]));
            else
               emit(MUL(result_dst, op[0], op[1]));
         } else {
            struct brw_reg acc = retype(brw_acc_reg(), result_dst.type);

            emit(MUL(acc, op[0], op[1]));
            emit(MACH(dst_null_d(), op[0], op[1]));
            emit(MOV(result_dst, src_reg(acc)));
         }
      } else {
	 emit(MUL(result_dst, op[0], op[1]));
      }
      break;
   case ir_binop_imul_high: {
      struct brw_reg acc = retype(brw_acc_reg(), result_dst.type);

      emit(MUL(acc, op[0], op[1]));
      emit(MACH(result_dst, op[0], op[1]));
      break;
   }
   case ir_binop_div:
      /* Floating point should be lowered by DIV_TO_MUL_RCP in the compiler. */
      assert(ir->type->is_integer());
      emit_math(SHADER_OPCODE_INT_QUOTIENT, result_dst, op[0], op[1]);
      break;
   case ir_binop_carry: {
      struct brw_reg acc = retype(brw_acc_reg(), BRW_REGISTER_TYPE_UD);

      emit(ADDC(dst_null_ud(), op[0], op[1]));
      emit(MOV(result_dst, src_reg(acc)));
      break;
   }
   case ir_binop_borrow: {
      struct brw_reg acc = retype(brw_acc_reg(), BRW_REGISTER_TYPE_UD);

      emit(SUBB(dst_null_ud(), op[0], op[1]));
      emit(MOV(result_dst, src_reg(acc)));
      break;
   }
   case ir_binop_mod:
      /* Floating point should be lowered by MOD_TO_FRACT in the compiler. */
      assert(ir->type->is_integer());
      emit_math(SHADER_OPCODE_INT_REMAINDER, result_dst, op[0], op[1]);
      break;

   case ir_binop_less:
   case ir_binop_greater:
   case ir_binop_lequal:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_nequal: {
      emit(CMP(result_dst, op[0], op[1],
	       brw_conditional_for_comparison(ir->operation)));
      emit(AND(result_dst, result_src, src_reg(0x1)));
      break;
   }

   case ir_binop_all_equal:
      /* "==" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_Z));
	 emit(MOV(result_dst, src_reg(0)));
	 inst = emit(MOV(result_dst, src_reg(1)));
	 inst->predicate = BRW_PREDICATE_ALIGN16_ALL4H;
      } else {
	 emit(CMP(result_dst, op[0], op[1], BRW_CONDITIONAL_Z));
	 emit(AND(result_dst, result_src, src_reg(0x1)));
      }
      break;
   case ir_binop_any_nequal:
      /* "!=" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_NZ));

	 emit(MOV(result_dst, src_reg(0)));
	 inst = emit(MOV(result_dst, src_reg(1)));
	 inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      } else {
	 emit(CMP(result_dst, op[0], op[1], BRW_CONDITIONAL_NZ));
	 emit(AND(result_dst, result_src, src_reg(0x1)));
      }
      break;

   case ir_unop_any:
      emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
      emit(MOV(result_dst, src_reg(0)));

      inst = emit(MOV(result_dst, src_reg(1)));
      inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      break;

   case ir_binop_logic_xor:
      emit(XOR(result_dst, op[0], op[1]));
      break;

   case ir_binop_logic_or:
      emit(OR(result_dst, op[0], op[1]));
      break;

   case ir_binop_logic_and:
      emit(AND(result_dst, op[0], op[1]));
      break;

   case ir_binop_dot:
      assert(ir->operands[0]->type->is_vector());
      assert(ir->operands[0]->type == ir->operands[1]->type);
      emit_dp(result_dst, op[0], op[1], ir->operands[0]->type->vector_elements);
      break;

   case ir_unop_sqrt:
      emit_math(SHADER_OPCODE_SQRT, result_dst, op[0]);
      break;
   case ir_unop_rsq:
      emit_math(SHADER_OPCODE_RSQ, result_dst, op[0]);
      break;

   case ir_unop_bitcast_i2f:
   case ir_unop_bitcast_u2f:
      this->result = op[0];
      this->result.type = BRW_REGISTER_TYPE_F;
      break;

   case ir_unop_bitcast_f2i:
      this->result = op[0];
      this->result.type = BRW_REGISTER_TYPE_D;
      break;

   case ir_unop_bitcast_f2u:
      this->result = op[0];
      this->result.type = BRW_REGISTER_TYPE_UD;
      break;

   case ir_unop_i2f:
   case ir_unop_i2u:
   case ir_unop_u2i:
   case ir_unop_u2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
   case ir_unop_f2i:
   case ir_unop_f2u:
      emit(MOV(result_dst, op[0]));
      break;
   case ir_unop_f2b:
   case ir_unop_i2b: {
      emit(CMP(result_dst, op[0], src_reg(0.0f), BRW_CONDITIONAL_NZ));
      emit(AND(result_dst, result_src, src_reg(1)));
      break;
   }

   case ir_unop_trunc:
      emit(RNDZ(result_dst, op[0]));
      break;
   case ir_unop_ceil:
      op[0].negate = !op[0].negate;
      inst = emit(RNDD(result_dst, op[0]));
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(RNDD(result_dst, op[0]));
      break;
   case ir_unop_fract:
      inst = emit(FRC(result_dst, op[0]));
      break;
   case ir_unop_round_even:
      emit(RNDE(result_dst, op[0]));
      break;

   case ir_binop_min:
      emit_minmax(BRW_CONDITIONAL_L, result_dst, op[0], op[1]);
      break;
   case ir_binop_max:
      emit_minmax(BRW_CONDITIONAL_G, result_dst, op[0], op[1]);
      break;

   case ir_binop_pow:
      emit_math(SHADER_OPCODE_POW, result_dst, op[0], op[1]);
      break;

   case ir_unop_bit_not:
      inst = emit(NOT(result_dst, op[0]));
      break;
   case ir_binop_bit_and:
      inst = emit(AND(result_dst, op[0], op[1]));
      break;
   case ir_binop_bit_xor:
      inst = emit(XOR(result_dst, op[0], op[1]));
      break;
   case ir_binop_bit_or:
      inst = emit(OR(result_dst, op[0], op[1]));
      break;

   case ir_binop_lshift:
      inst = emit(SHL(result_dst, op[0], op[1]));
      break;

   case ir_binop_rshift:
      if (ir->type->base_type == GLSL_TYPE_INT)
         inst = emit(ASR(result_dst, op[0], op[1]));
      else
         inst = emit(SHR(result_dst, op[0], op[1]));
      break;

   case ir_binop_bfm:
      emit(BFI1(result_dst, op[0], op[1]));
      break;

   case ir_binop_ubo_load: {
      ir_constant *uniform_block = ir->operands[0]->as_constant();
      ir_constant *const_offset_ir = ir->operands[1]->as_constant();
      unsigned const_offset = const_offset_ir ? const_offset_ir->value.u[0] : 0;
      src_reg offset;

      /* Now, load the vector from that offset. */
      assert(ir->type->is_vector() || ir->type->is_scalar());

      src_reg packed_consts = src_reg(this, glsl_type::vec4_type);
      packed_consts.type = result.type;
      src_reg surf_index =
         src_reg(prog_data->base.binding_table.ubo_start + uniform_block->value.u[0]);
      if (const_offset_ir) {
         if (brw->gen >= 8) {
            /* Store the offset in a GRF so we can send-from-GRF. */
            offset = src_reg(this, glsl_type::int_type);
            emit(MOV(dst_reg(offset), src_reg(const_offset / 16)));
         } else {
            /* Immediates are fine on older generations since they'll be moved
             * to a (potentially fake) MRF at the generator level.
             */
            offset = src_reg(const_offset / 16);
         }
      } else {
         offset = src_reg(this, glsl_type::uint_type);
         emit(SHR(dst_reg(offset), op[1], src_reg(4)));
      }

      if (brw->gen >= 7) {
         dst_reg grf_offset = dst_reg(this, glsl_type::int_type);
         grf_offset.type = offset.type;

         emit(MOV(grf_offset, offset));

         emit(new(mem_ctx) vec4_instruction(this,
                                            VS_OPCODE_PULL_CONSTANT_LOAD_GEN7,
                                            dst_reg(packed_consts),
                                            surf_index,
                                            src_reg(grf_offset)));
      } else {
         vec4_instruction *pull =
            emit(new(mem_ctx) vec4_instruction(this,
                                               VS_OPCODE_PULL_CONSTANT_LOAD,
                                               dst_reg(packed_consts),
                                               surf_index,
                                               offset));
         pull->base_mrf = 14;
         pull->mlen = 1;
      }

      packed_consts.swizzle = swizzle_for_size(ir->type->vector_elements);
      packed_consts.swizzle += BRW_SWIZZLE4(const_offset % 16 / 4,
                                            const_offset % 16 / 4,
                                            const_offset % 16 / 4,
                                            const_offset % 16 / 4);

      /* UBO bools are any nonzero int.  We store bools as either 0 or 1. */
      if (ir->type->base_type == GLSL_TYPE_BOOL) {
         emit(CMP(result_dst, packed_consts, src_reg(0u),
                  BRW_CONDITIONAL_NZ));
         emit(AND(result_dst, result, src_reg(0x1)));
      } else {
         emit(MOV(result_dst, packed_consts));
      }
      break;
   }

   case ir_binop_vector_extract:
      assert(!"should have been lowered by vec_index_to_cond_assign");
      break;

   case ir_triop_fma:
      op[0] = fix_3src_operand(op[0]);
      op[1] = fix_3src_operand(op[1]);
      op[2] = fix_3src_operand(op[2]);
      /* Note that the instruction's argument order is reversed from GLSL
       * and the IR.
       */
      emit(MAD(result_dst, op[2], op[1], op[0]));
      break;

   case ir_triop_lrp:
      emit_lrp(result_dst, op[0], op[1], op[2]);
      break;

   case ir_triop_csel:
      emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
      inst = emit(BRW_OPCODE_SEL, result_dst, op[1], op[2]);
      inst->predicate = BRW_PREDICATE_NORMAL;
      break;

   case ir_triop_bfi:
      op[0] = fix_3src_operand(op[0]);
      op[1] = fix_3src_operand(op[1]);
      op[2] = fix_3src_operand(op[2]);
      emit(BFI2(result_dst, op[0], op[1], op[2]));
      break;

   case ir_triop_bitfield_extract:
      op[0] = fix_3src_operand(op[0]);
      op[1] = fix_3src_operand(op[1]);
      op[2] = fix_3src_operand(op[2]);
      /* Note that the instruction's argument order is reversed from GLSL
       * and the IR.
       */
      emit(BFE(result_dst, op[2], op[1], op[0]));
      break;

   case ir_triop_vector_insert:
      assert(!"should have been lowered by lower_vector_insert");
      break;

   case ir_quadop_bitfield_insert:
      assert(!"not reached: should be handled by "
              "bitfield_insert_to_bfm_bfi\n");
      break;

   case ir_quadop_vector:
      assert(!"not reached: should be handled by lower_quadop_vector");
      break;

   case ir_unop_pack_half_2x16:
      emit_pack_half_2x16(result_dst, op[0]);
      break;
   case ir_unop_unpack_half_2x16:
      emit_unpack_half_2x16(result_dst, op[0]);
      break;
   case ir_unop_pack_snorm_2x16:
   case ir_unop_pack_snorm_4x8:
   case ir_unop_pack_unorm_2x16:
   case ir_unop_pack_unorm_4x8:
   case ir_unop_unpack_snorm_2x16:
   case ir_unop_unpack_snorm_4x8:
   case ir_unop_unpack_unorm_2x16:
   case ir_unop_unpack_unorm_4x8:
      assert(!"not reached: should be handled by lower_packing_builtins");
      break;
   case ir_unop_unpack_half_2x16_split_x:
   case ir_unop_unpack_half_2x16_split_y:
   case ir_binop_pack_half_2x16_split:
      assert(!"not reached: should not occur in vertex shader");
      break;
   case ir_binop_ldexp:
      assert(!"not reached: should be handled by ldexp_to_arith()");
      break;
   }
}


void
vec4_visitor::visit(ir_swizzle *ir)
{
   src_reg src;
   int i = 0;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   src = this->result;
   assert(src.file != BAD_FILE);

   for (i = 0; i < ir->type->vector_elements; i++) {
      switch (i) {
      case 0:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.x);
	 break;
      case 1:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.y);
	 break;
      case 2:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.z);
	 break;
      case 3:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.w);
	    break;
      }
   }
   for (; i < 4; i++) {
      /* Replicate the last channel out. */
      swizzle[i] = swizzle[ir->type->vector_elements - 1];
   }

   src.swizzle = BRW_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);

   this->result = src;
}

void
vec4_visitor::visit(ir_dereference_variable *ir)
{
   const struct glsl_type *type = ir->type;
   dst_reg *reg = variable_storage(ir->var);

   if (!reg) {
      fail("Failed to find variable storage for %s\n", ir->var->name);
      this->result = src_reg(brw_null_reg());
      return;
   }

   this->result = src_reg(*reg);

   /* System values get their swizzle from the dst_reg writemask */
   if (ir->var->data.mode == ir_var_system_value)
      return;

   if (type->is_scalar() || type->is_vector() || type->is_matrix())
      this->result.swizzle = swizzle_for_size(type->vector_elements);
}


int
vec4_visitor::compute_array_stride(ir_dereference_array *ir)
{
   /* Under normal circumstances array elements are stored consecutively, so
    * the stride is equal to the size of the array element.
    */
   return type_size(ir->type);
}


void
vec4_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *constant_index;
   src_reg src;
   int array_stride = compute_array_stride(ir);

   constant_index = ir->array_index->constant_expression_value();

   ir->array->accept(this);
   src = this->result;

   if (constant_index) {
      src.reg_offset += constant_index->value.i[0] * array_stride;
   } else {
      /* Variable index array dereference.  It eats the "vec4" of the
       * base of the array and an index that offsets the Mesa register
       * index.
       */
      ir->array_index->accept(this);

      src_reg index_reg;

      if (array_stride == 1) {
	 index_reg = this->result;
      } else {
	 index_reg = src_reg(this, glsl_type::int_type);

	 emit(MUL(dst_reg(index_reg), this->result, src_reg(array_stride)));
      }

      if (src.reladdr) {
	 src_reg temp = src_reg(this, glsl_type::int_type);

	 emit(ADD(dst_reg(temp), *src.reladdr, index_reg));

	 index_reg = temp;
      }

      src.reladdr = ralloc(mem_ctx, src_reg);
      memcpy(src.reladdr, &index_reg, sizeof(index_reg));
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector() || ir->type->is_matrix())
      src.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      src.swizzle = BRW_SWIZZLE_NOOP;
   src.type = brw_type_for_base_type(ir->type);

   this->result = src;
}

void
vec4_visitor::visit(ir_dereference_record *ir)
{
   unsigned int i;
   const glsl_type *struct_type = ir->record->type;
   int offset = 0;

   ir->record->accept(this);

   for (i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector() || ir->type->is_matrix())
      this->result.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      this->result.swizzle = BRW_SWIZZLE_NOOP;
   this->result.type = brw_type_for_base_type(ir->type);

   this->result.reg_offset += offset;
}

/**
 * We want to be careful in assignment setup to hit the actual storage
 * instead of potentially using a temporary like we might with the
 * ir_dereference handler.
 */
static dst_reg
get_assignment_lhs(ir_dereference *ir, vec4_visitor *v)
{
   /* The LHS must be a dereference.  If the LHS is a variable indexed array
    * access of a vector, it must be separated into a series conditional moves
    * before reaching this point (see ir_vec_index_to_cond_assign).
    */
   assert(ir->as_dereference());
   ir_dereference_array *deref_array = ir->as_dereference_array();
   if (deref_array) {
      assert(!deref_array->array->type->is_vector());
   }

   /* Use the rvalue deref handler for the most part.  We'll ignore
    * swizzles in it and write swizzles using writemask, though.
    */
   ir->accept(v);
   return dst_reg(v->result);
}

void
vec4_visitor::emit_block_move(dst_reg *dst, src_reg *src,
			      const struct glsl_type *type, uint32_t predicate)
{
   if (type->base_type == GLSL_TYPE_STRUCT) {
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_block_move(dst, src, type->fields.structure[i].type, predicate);
      }
      return;
   }

   if (type->is_array()) {
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_block_move(dst, src, type->fields.array, predicate);
      }
      return;
   }

   if (type->is_matrix()) {
      const struct glsl_type *vec_type;

      vec_type = glsl_type::get_instance(GLSL_TYPE_FLOAT,
					 type->vector_elements, 1);

      for (int i = 0; i < type->matrix_columns; i++) {
	 emit_block_move(dst, src, vec_type, predicate);
      }
      return;
   }

   assert(type->is_scalar() || type->is_vector());

   dst->type = brw_type_for_base_type(type);
   src->type = dst->type;

   dst->writemask = (1 << type->vector_elements) - 1;

   src->swizzle = swizzle_for_size(type->vector_elements);

   vec4_instruction *inst = emit(MOV(*dst, *src));
   inst->predicate = predicate;

   dst->reg_offset++;
   src->reg_offset++;
}


/* If the RHS processing resulted in an instruction generating a
 * temporary value, and it would be easy to rewrite the instruction to
 * generate its result right into the LHS instead, do so.  This ends
 * up reliably removing instructions where it can be tricky to do so
 * later without real UD chain information.
 */
bool
vec4_visitor::try_rewrite_rhs_to_dst(ir_assignment *ir,
				     dst_reg dst,
				     src_reg src,
				     vec4_instruction *pre_rhs_inst,
				     vec4_instruction *last_rhs_inst)
{
   /* This could be supported, but it would take more smarts. */
   if (ir->condition)
      return false;

   if (pre_rhs_inst == last_rhs_inst)
      return false; /* No instructions generated to work with. */

   /* Make sure the last instruction generated our source reg. */
   if (src.file != GRF ||
       src.file != last_rhs_inst->dst.file ||
       src.reg != last_rhs_inst->dst.reg ||
       src.reg_offset != last_rhs_inst->dst.reg_offset ||
       src.reladdr ||
       src.abs ||
       src.negate ||
       last_rhs_inst->predicate != BRW_PREDICATE_NONE)
      return false;

   /* Check that that last instruction fully initialized the channels
    * we want to use, in the order we want to use them.  We could
    * potentially reswizzle the operands of many instructions so that
    * we could handle out of order channels, but don't yet.
    */

   for (unsigned i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i)) {
	 if (!(last_rhs_inst->dst.writemask & (1 << i)))
	    return false;

	 if (BRW_GET_SWZ(src.swizzle, i) != i)
	    return false;
      }
   }

   /* Success!  Rewrite the instruction. */
   last_rhs_inst->dst.file = dst.file;
   last_rhs_inst->dst.reg = dst.reg;
   last_rhs_inst->dst.reg_offset = dst.reg_offset;
   last_rhs_inst->dst.reladdr = dst.reladdr;
   last_rhs_inst->dst.writemask &= dst.writemask;

   return true;
}

void
vec4_visitor::visit(ir_assignment *ir)
{
   dst_reg dst = get_assignment_lhs(ir->lhs, this);
   uint32_t predicate = BRW_PREDICATE_NONE;

   if (!ir->lhs->type->is_scalar() &&
       !ir->lhs->type->is_vector()) {
      ir->rhs->accept(this);
      src_reg src = this->result;

      if (ir->condition) {
	 emit_bool_to_cond_code(ir->condition, &predicate);
      }

      /* emit_block_move doesn't account for swizzles in the source register.
       * This should be ok, since the source register is a structure or an
       * array, and those can't be swizzled.  But double-check to be sure.
       */
      assert(src.swizzle ==
             (ir->rhs->type->is_matrix()
              ? swizzle_for_size(ir->rhs->type->vector_elements)
              : BRW_SWIZZLE_NOOP));

      emit_block_move(&dst, &src, ir->rhs->type, predicate);
      return;
   }

   /* Now we're down to just a scalar/vector with writemasks. */
   int i;

   vec4_instruction *pre_rhs_inst, *last_rhs_inst;
   pre_rhs_inst = (vec4_instruction *)this->instructions.get_tail();

   ir->rhs->accept(this);

   last_rhs_inst = (vec4_instruction *)this->instructions.get_tail();

   src_reg src = this->result;

   int swizzles[4];
   int first_enabled_chan = 0;
   int src_chan = 0;

   assert(ir->lhs->type->is_vector() ||
	  ir->lhs->type->is_scalar());
   dst.writemask = ir->write_mask;

   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i)) {
	 first_enabled_chan = BRW_GET_SWZ(src.swizzle, i);
	 break;
      }
   }

   /* Swizzle a small RHS vector into the channels being written.
    *
    * glsl ir treats write_mask as dictating how many channels are
    * present on the RHS while in our instructions we need to make
    * those channels appear in the slots of the vec4 they're written to.
    */
   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i))
	 swizzles[i] = BRW_GET_SWZ(src.swizzle, src_chan++);
      else
	 swizzles[i] = first_enabled_chan;
   }
   src.swizzle = BRW_SWIZZLE4(swizzles[0], swizzles[1],
			      swizzles[2], swizzles[3]);

   if (try_rewrite_rhs_to_dst(ir, dst, src, pre_rhs_inst, last_rhs_inst)) {
      return;
   }

   if (ir->condition) {
      emit_bool_to_cond_code(ir->condition, &predicate);
   }

   for (i = 0; i < type_size(ir->lhs->type); i++) {
      vec4_instruction *inst = emit(MOV(dst, src));
      inst->predicate = predicate;

      dst.reg_offset++;
      src.reg_offset++;
   }
}

void
vec4_visitor::emit_constant_values(dst_reg *dst, ir_constant *ir)
{
   if (ir->type->base_type == GLSL_TYPE_STRUCT) {
      foreach_list(node, &ir->components) {
	 ir_constant *field_value = (ir_constant *)node;

	 emit_constant_values(dst, field_value);
      }
      return;
   }

   if (ir->type->is_array()) {
      for (unsigned int i = 0; i < ir->type->length; i++) {
	 emit_constant_values(dst, ir->array_elements[i]);
      }
      return;
   }

   if (ir->type->is_matrix()) {
      for (int i = 0; i < ir->type->matrix_columns; i++) {
	 float *vec = &ir->value.f[i * ir->type->vector_elements];

	 for (int j = 0; j < ir->type->vector_elements; j++) {
	    dst->writemask = 1 << j;
	    dst->type = BRW_REGISTER_TYPE_F;

	    emit(MOV(*dst, src_reg(vec[j])));
	 }
	 dst->reg_offset++;
      }
      return;
   }

   int remaining_writemask = (1 << ir->type->vector_elements) - 1;

   for (int i = 0; i < ir->type->vector_elements; i++) {
      if (!(remaining_writemask & (1 << i)))
	 continue;

      dst->writemask = 1 << i;
      dst->type = brw_type_for_base_type(ir->type);

      /* Find other components that match the one we're about to
       * write.  Emits fewer instructions for things like vec4(0.5,
       * 1.5, 1.5, 1.5).
       */
      for (int j = i + 1; j < ir->type->vector_elements; j++) {
	 if (ir->type->base_type == GLSL_TYPE_BOOL) {
	    if (ir->value.b[i] == ir->value.b[j])
	       dst->writemask |= (1 << j);
	 } else {
	    /* u, i, and f storage all line up, so no need for a
	     * switch case for comparing each type.
	     */
	    if (ir->value.u[i] == ir->value.u[j])
	       dst->writemask |= (1 << j);
	 }
      }

      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 emit(MOV(*dst, src_reg(ir->value.f[i])));
	 break;
      case GLSL_TYPE_INT:
	 emit(MOV(*dst, src_reg(ir->value.i[i])));
	 break;
      case GLSL_TYPE_UINT:
	 emit(MOV(*dst, src_reg(ir->value.u[i])));
	 break;
      case GLSL_TYPE_BOOL:
	 emit(MOV(*dst, src_reg(ir->value.b[i])));
	 break;
      default:
	 assert(!"Non-float/uint/int/bool constant");
	 break;
      }

      remaining_writemask &= ~dst->writemask;
   }
   dst->reg_offset++;
}

void
vec4_visitor::visit(ir_constant *ir)
{
   dst_reg dst = dst_reg(this, ir->type);
   this->result = src_reg(dst);

   emit_constant_values(&dst, ir);
}

void
vec4_visitor::visit_atomic_counter_intrinsic(ir_call *ir)
{
   ir_dereference *deref = static_cast<ir_dereference *>(
      ir->actual_parameters.get_head());
   ir_variable *location = deref->variable_referenced();
   unsigned surf_index = (prog_data->base.binding_table.abo_start +
                          location->data.atomic.buffer_index);

   /* Calculate the surface offset */
   src_reg offset(this, glsl_type::uint_type);
   ir_dereference_array *deref_array = deref->as_dereference_array();
   if (deref_array) {
      deref_array->array_index->accept(this);

      src_reg tmp(this, glsl_type::uint_type);
      emit(MUL(dst_reg(tmp), this->result, ATOMIC_COUNTER_SIZE));
      emit(ADD(dst_reg(offset), tmp, location->data.atomic.offset));
   } else {
      offset = location->data.atomic.offset;
   }

   /* Emit the appropriate machine instruction */
   const char *callee = ir->callee->function_name();
   dst_reg dst = get_assignment_lhs(ir->return_deref, this);

   if (!strcmp("__intrinsic_atomic_read", callee)) {
      emit_untyped_surface_read(surf_index, dst, offset);

   } else if (!strcmp("__intrinsic_atomic_increment", callee)) {
      emit_untyped_atomic(BRW_AOP_INC, surf_index, dst, offset,
                          src_reg(), src_reg());

   } else if (!strcmp("__intrinsic_atomic_predecrement", callee)) {
      emit_untyped_atomic(BRW_AOP_PREDEC, surf_index, dst, offset,
                          src_reg(), src_reg());
   }
}

void
vec4_visitor::visit(ir_call *ir)
{
   const char *callee = ir->callee->function_name();

   if (!strcmp("__intrinsic_atomic_read", callee) ||
       !strcmp("__intrinsic_atomic_increment", callee) ||
       !strcmp("__intrinsic_atomic_predecrement", callee)) {
      visit_atomic_counter_intrinsic(ir);
   } else {
      assert(!"Unsupported intrinsic.");
   }
}

src_reg
vec4_visitor::emit_mcs_fetch(ir_texture *ir, src_reg coordinate, int sampler)
{
   vec4_instruction *inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXF_MCS);
   inst->base_mrf = 2;
   inst->mlen = 1;
   inst->sampler = sampler;
   inst->dst = dst_reg(this, glsl_type::uvec4_type);
   inst->dst.writemask = WRITEMASK_XYZW;

   /* parameters are: u, v, r, lod; lod will always be zero due to api restrictions */
   int param_base = inst->base_mrf;
   int coord_mask = (1 << ir->coordinate->type->vector_elements) - 1;
   int zero_mask = 0xf & ~coord_mask;

   emit(MOV(dst_reg(MRF, param_base, ir->coordinate->type, coord_mask),
            coordinate));

   emit(MOV(dst_reg(MRF, param_base, ir->coordinate->type, zero_mask),
            src_reg(0)));

   emit(inst);
   return src_reg(inst->dst);
}

void
vec4_visitor::visit(ir_texture *ir)
{
   int sampler =
      _mesa_get_sampler_uniform_value(ir->sampler, shader_prog, prog);

   /* When tg4 is used with the degenerate ZERO/ONE swizzles, don't bother
    * emitting anything other than setting up the constant result.
    */
   if (ir->op == ir_tg4) {
      ir_constant *chan = ir->lod_info.component->as_constant();
      int swiz = GET_SWZ(key->tex.swizzles[sampler], chan->value.i[0]);
      if (swiz == SWIZZLE_ZERO || swiz == SWIZZLE_ONE) {
         dst_reg result(this, ir->type);
         this->result = src_reg(result);
         emit(MOV(result, src_reg(swiz == SWIZZLE_ONE ? 1.0f : 0.0f)));
         return;
      }
   }

   /* Should be lowered by do_lower_texture_projection */
   assert(!ir->projector);

   /* Should be lowered */
   assert(!ir->offset || !ir->offset->type->is_array());

   /* Generate code to compute all the subexpression trees.  This has to be
    * done before loading any values into MRFs for the sampler message since
    * generating these values may involve SEND messages that need the MRFs.
    */
   src_reg coordinate;
   if (ir->coordinate) {
      ir->coordinate->accept(this);
      coordinate = this->result;
   }

   src_reg shadow_comparitor;
   if (ir->shadow_comparitor) {
      ir->shadow_comparitor->accept(this);
      shadow_comparitor = this->result;
   }

   bool has_nonconstant_offset = ir->offset && !ir->offset->as_constant();
   src_reg offset_value;
   if (has_nonconstant_offset) {
      ir->offset->accept(this);
      offset_value = src_reg(this->result);
   }

   const glsl_type *lod_type = NULL, *sample_index_type = NULL;
   src_reg lod, dPdx, dPdy, sample_index, mcs;
   switch (ir->op) {
   case ir_tex:
      lod = src_reg(0.0f);
      lod_type = glsl_type::float_type;
      break;
   case ir_txf:
   case ir_txl:
   case ir_txs:
      ir->lod_info.lod->accept(this);
      lod = this->result;
      lod_type = ir->lod_info.lod->type;
      break;
   case ir_query_levels:
      lod = src_reg(0);
      lod_type = glsl_type::int_type;
      break;
   case ir_txf_ms:
      ir->lod_info.sample_index->accept(this);
      sample_index = this->result;
      sample_index_type = ir->lod_info.sample_index->type;

      if (brw->gen >= 7 && key->tex.compressed_multisample_layout_mask & (1<<sampler))
         mcs = emit_mcs_fetch(ir, coordinate, sampler);
      else
         mcs = src_reg(0u);
      break;
   case ir_txd:
      ir->lod_info.grad.dPdx->accept(this);
      dPdx = this->result;

      ir->lod_info.grad.dPdy->accept(this);
      dPdy = this->result;

      lod_type = ir->lod_info.grad.dPdx->type;
      break;
   case ir_txb:
   case ir_lod:
   case ir_tg4:
      break;
   }

   vec4_instruction *inst = NULL;
   switch (ir->op) {
   case ir_tex:
   case ir_txl:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXL);
      break;
   case ir_txd:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXD);
      break;
   case ir_txf:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXF);
      break;
   case ir_txf_ms:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXF_CMS);
      break;
   case ir_txs:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXS);
      break;
   case ir_tg4:
      if (has_nonconstant_offset)
         inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TG4_OFFSET);
      else
         inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TG4);
      break;
   case ir_query_levels:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXS);
      break;
   case ir_txb:
      assert(!"TXB is not valid for vertex shaders.");
      break;
   case ir_lod:
      assert(!"LOD is not valid for vertex shaders.");
      break;
   default:
      assert(!"Unrecognized tex op");
   }

   if (ir->offset != NULL && ir->op != ir_txf)
      inst->texture_offset = brw_texture_offset(ctx, ir->offset->as_constant());

   /* Stuff the channel select bits in the top of the texture offset */
   if (ir->op == ir_tg4)
      inst->texture_offset |= gather_channel(ir, sampler) << 16;

   /* The message header is necessary for:
    * - Gen4 (always)
    * - Texel offsets
    * - Gather channel selection
    * - Sampler indices too large to fit in a 4-bit value.
    */
   inst->header_present =
      brw->gen < 5 || inst->texture_offset != 0 || ir->op == ir_tg4 ||
      sampler >= 16;
   inst->base_mrf = 2;
   inst->mlen = inst->header_present + 1; /* always at least one */
   inst->sampler = sampler;
   inst->dst = dst_reg(this, ir->type);
   inst->dst.writemask = WRITEMASK_XYZW;
   inst->shadow_compare = ir->shadow_comparitor != NULL;

   /* MRF for the first parameter */
   int param_base = inst->base_mrf + inst->header_present;

   if (ir->op == ir_txs || ir->op == ir_query_levels) {
      int writemask = brw->gen == 4 ? WRITEMASK_W : WRITEMASK_X;
      emit(MOV(dst_reg(MRF, param_base, lod_type, writemask), lod));
   } else {
      /* Load the coordinate */
      /* FINISHME: gl_clamp_mask and saturate */
      int coord_mask = (1 << ir->coordinate->type->vector_elements) - 1;
      int zero_mask = 0xf & ~coord_mask;

      emit(MOV(dst_reg(MRF, param_base, ir->coordinate->type, coord_mask),
               coordinate));

      if (zero_mask != 0) {
         emit(MOV(dst_reg(MRF, param_base, ir->coordinate->type, zero_mask),
                  src_reg(0)));
      }
      /* Load the shadow comparitor */
      if (ir->shadow_comparitor && ir->op != ir_txd && (ir->op != ir_tg4 || !has_nonconstant_offset)) {
	 emit(MOV(dst_reg(MRF, param_base + 1, ir->shadow_comparitor->type,
			  WRITEMASK_X),
		  shadow_comparitor));
	 inst->mlen++;
      }

      /* Load the LOD info */
      if (ir->op == ir_tex || ir->op == ir_txl) {
	 int mrf, writemask;
	 if (brw->gen >= 5) {
	    mrf = param_base + 1;
	    if (ir->shadow_comparitor) {
	       writemask = WRITEMASK_Y;
	       /* mlen already incremented */
	    } else {
	       writemask = WRITEMASK_X;
	       inst->mlen++;
	    }
	 } else /* brw->gen == 4 */ {
	    mrf = param_base;
	    writemask = WRITEMASK_W;
	 }
	 emit(MOV(dst_reg(MRF, mrf, lod_type, writemask), lod));
      } else if (ir->op == ir_txf) {
         emit(MOV(dst_reg(MRF, param_base, lod_type, WRITEMASK_W), lod));
      } else if (ir->op == ir_txf_ms) {
         emit(MOV(dst_reg(MRF, param_base + 1, sample_index_type, WRITEMASK_X),
                  sample_index));
         if (brw->gen >= 7)
            /* MCS data is in the first channel of `mcs`, but we need to get it into
             * the .y channel of the second vec4 of params, so replicate .x across
             * the whole vec4 and then mask off everything except .y
             */
            mcs.swizzle = BRW_SWIZZLE_XXXX;
            emit(MOV(dst_reg(MRF, param_base + 1, glsl_type::uint_type, WRITEMASK_Y),
                     mcs));
         inst->mlen++;
      } else if (ir->op == ir_txd) {
	 const glsl_type *type = lod_type;

	 if (brw->gen >= 5) {
	    dPdx.swizzle = BRW_SWIZZLE4(SWIZZLE_X,SWIZZLE_X,SWIZZLE_Y,SWIZZLE_Y);
	    dPdy.swizzle = BRW_SWIZZLE4(SWIZZLE_X,SWIZZLE_X,SWIZZLE_Y,SWIZZLE_Y);
	    emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_XZ), dPdx));
	    emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_YW), dPdy));
	    inst->mlen++;

	    if (ir->type->vector_elements == 3 || ir->shadow_comparitor) {
	       dPdx.swizzle = BRW_SWIZZLE_ZZZZ;
	       dPdy.swizzle = BRW_SWIZZLE_ZZZZ;
	       emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_X), dPdx));
	       emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_Y), dPdy));
	       inst->mlen++;

               if (ir->shadow_comparitor) {
                  emit(MOV(dst_reg(MRF, param_base + 2,
                                   ir->shadow_comparitor->type, WRITEMASK_Z),
                           shadow_comparitor));
               }
	    }
	 } else /* brw->gen == 4 */ {
	    emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_XYZ), dPdx));
	    emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_XYZ), dPdy));
	    inst->mlen += 2;
	 }
      } else if (ir->op == ir_tg4 && has_nonconstant_offset) {
         if (ir->shadow_comparitor) {
            emit(MOV(dst_reg(MRF, param_base, ir->shadow_comparitor->type, WRITEMASK_W),
                     shadow_comparitor));
         }

         emit(MOV(dst_reg(MRF, param_base + 1, glsl_type::ivec2_type, WRITEMASK_XY),
                  offset_value));
         inst->mlen++;
      }
   }

   emit(inst);

   /* fixup num layers (z) for cube arrays: hardware returns faces * layers;
    * spec requires layers.
    */
   if (ir->op == ir_txs) {
      glsl_type const *type = ir->sampler->type;
      if (type->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE &&
          type->sampler_array) {
         emit_math(SHADER_OPCODE_INT_QUOTIENT,
                   writemask(inst->dst, WRITEMASK_Z),
                   src_reg(inst->dst), src_reg(6));
      }
   }

   if (brw->gen == 6 && ir->op == ir_tg4) {
      emit_gen6_gather_wa(key->tex.gen6_gather_wa[sampler], inst->dst);
   }

   swizzle_result(ir, src_reg(inst->dst), sampler);
}

/**
 * Apply workarounds for Gen6 gather with UINT/SINT
 */
void
vec4_visitor::emit_gen6_gather_wa(uint8_t wa, dst_reg dst)
{
   if (!wa)
      return;

   int width = (wa & WA_8BIT) ? 8 : 16;
   dst_reg dst_f = dst;
   dst_f.type = BRW_REGISTER_TYPE_F;

   /* Convert from UNORM to UINT */
   emit(MUL(dst_f, src_reg(dst_f), src_reg((float)((1 << width) - 1))));
   emit(MOV(dst, src_reg(dst_f)));

   if (wa & WA_SIGN) {
      /* Reinterpret the UINT value as a signed INT value by
       * shifting the sign bit into place, then shifting back
       * preserving sign.
       */
      emit(SHL(dst, src_reg(dst), src_reg(32 - width)));
      emit(ASR(dst, src_reg(dst), src_reg(32 - width)));
   }
}

/**
 * Set up the gather channel based on the swizzle, for gather4.
 */
uint32_t
vec4_visitor::gather_channel(ir_texture *ir, int sampler)
{
   ir_constant *chan = ir->lod_info.component->as_constant();
   int swiz = GET_SWZ(key->tex.swizzles[sampler], chan->value.i[0]);
   switch (swiz) {
      case SWIZZLE_X: return 0;
      case SWIZZLE_Y:
         /* gather4 sampler is broken for green channel on RG32F --
          * we must ask for blue instead.
          */
         if (key->tex.gather_channel_quirk_mask & (1<<sampler))
            return 2;
         return 1;
      case SWIZZLE_Z: return 2;
      case SWIZZLE_W: return 3;
      default:
         assert(!"Not reached"); /* zero, one swizzles handled already */
         return 0;
   }
}

void
vec4_visitor::swizzle_result(ir_texture *ir, src_reg orig_val, int sampler)
{
   int s = key->tex.swizzles[sampler];

   this->result = src_reg(this, ir->type);
   dst_reg swizzled_result(this->result);

   if (ir->op == ir_query_levels) {
      /* # levels is in .w */
      orig_val.swizzle = BRW_SWIZZLE4(SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W);
      emit(MOV(swizzled_result, orig_val));
      return;
   }

   if (ir->op == ir_txs || ir->type == glsl_type::float_type
			|| s == SWIZZLE_NOOP || ir->op == ir_tg4) {
      emit(MOV(swizzled_result, orig_val));
      return;
   }


   int zero_mask = 0, one_mask = 0, copy_mask = 0;
   int swizzle[4] = {0};

   for (int i = 0; i < 4; i++) {
      switch (GET_SWZ(s, i)) {
      case SWIZZLE_ZERO:
	 zero_mask |= (1 << i);
	 break;
      case SWIZZLE_ONE:
	 one_mask |= (1 << i);
	 break;
      default:
	 copy_mask |= (1 << i);
	 swizzle[i] = GET_SWZ(s, i);
	 break;
      }
   }

   if (copy_mask) {
      orig_val.swizzle = BRW_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
      swizzled_result.writemask = copy_mask;
      emit(MOV(swizzled_result, orig_val));
   }

   if (zero_mask) {
      swizzled_result.writemask = zero_mask;
      emit(MOV(swizzled_result, src_reg(0.0f)));
   }

   if (one_mask) {
      swizzled_result.writemask = one_mask;
      emit(MOV(swizzled_result, src_reg(1.0f)));
   }
}

void
vec4_visitor::visit(ir_return *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_discard *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_if *ir)
{
   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   if (brw->gen == 6) {
      emit_if_gen6(ir);
   } else {
      uint32_t predicate;
      emit_bool_to_cond_code(ir->condition, &predicate);
      emit(IF(predicate));
   }

   visit_instructions(&ir->then_instructions);

   if (!ir->else_instructions.is_empty()) {
      this->base_ir = ir->condition;
      emit(BRW_OPCODE_ELSE);

      visit_instructions(&ir->else_instructions);
   }

   this->base_ir = ir->condition;
   emit(BRW_OPCODE_ENDIF);
}

void
vec4_visitor::visit(ir_emit_vertex *)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_end_primitive *)
{
   assert(!"not reached");
}

void
vec4_visitor::emit_untyped_atomic(unsigned atomic_op, unsigned surf_index,
                                  dst_reg dst, src_reg offset,
                                  src_reg src0, src_reg src1)
{
   unsigned mlen = 0;

   /* Set the atomic operation offset. */
   emit(MOV(brw_writemask(brw_uvec_mrf(8, mlen, 0), WRITEMASK_X), offset));
   mlen++;

   /* Set the atomic operation arguments. */
   if (src0.file != BAD_FILE) {
      emit(MOV(brw_writemask(brw_uvec_mrf(8, mlen, 0), WRITEMASK_X), src0));
      mlen++;
   }

   if (src1.file != BAD_FILE) {
      emit(MOV(brw_writemask(brw_uvec_mrf(8, mlen, 0), WRITEMASK_X), src1));
      mlen++;
   }

   /* Emit the instruction.  Note that this maps to the normal SIMD8
    * untyped atomic message on Ivy Bridge, but that's OK because
    * unused channels will be masked out.
    */
   vec4_instruction *inst = emit(SHADER_OPCODE_UNTYPED_ATOMIC, dst,
                                 src_reg(atomic_op), src_reg(surf_index));
   inst->base_mrf = 0;
   inst->mlen = mlen;
}

void
vec4_visitor::emit_untyped_surface_read(unsigned surf_index, dst_reg dst,
                                        src_reg offset)
{
   /* Set the surface read offset. */
   emit(MOV(brw_writemask(brw_uvec_mrf(8, 0, 0), WRITEMASK_X), offset));

   /* Emit the instruction.  Note that this maps to the normal SIMD8
    * untyped surface read message, but that's OK because unused
    * channels will be masked out.
    */
   vec4_instruction *inst = emit(SHADER_OPCODE_UNTYPED_SURFACE_READ,
                                 dst, src_reg(surf_index));
   inst->base_mrf = 0;
   inst->mlen = 1;
}

void
vec4_visitor::emit_ndc_computation()
{
   /* Get the position */
   src_reg pos = src_reg(output_reg[VARYING_SLOT_POS]);

   /* Build ndc coords, which are (x/w, y/w, z/w, 1/w) */
   dst_reg ndc = dst_reg(this, glsl_type::vec4_type);
   output_reg[BRW_VARYING_SLOT_NDC] = ndc;

   current_annotation = "NDC";
   dst_reg ndc_w = ndc;
   ndc_w.writemask = WRITEMASK_W;
   src_reg pos_w = pos;
   pos_w.swizzle = BRW_SWIZZLE4(SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W);
   emit_math(SHADER_OPCODE_RCP, ndc_w, pos_w);

   dst_reg ndc_xyz = ndc;
   ndc_xyz.writemask = WRITEMASK_XYZ;

   emit(MUL(ndc_xyz, pos, src_reg(ndc_w)));
}

void
vec4_visitor::emit_psiz_and_flags(struct brw_reg reg)
{
   if (brw->gen < 6 &&
       ((prog_data->vue_map.slots_valid & VARYING_BIT_PSIZ) ||
        key->userclip_active || brw->has_negative_rhw_bug)) {
      dst_reg header1 = dst_reg(this, glsl_type::uvec4_type);
      dst_reg header1_w = header1;
      header1_w.writemask = WRITEMASK_W;

      emit(MOV(header1, 0u));

      if (prog_data->vue_map.slots_valid & VARYING_BIT_PSIZ) {
	 src_reg psiz = src_reg(output_reg[VARYING_SLOT_PSIZ]);

	 current_annotation = "Point size";
	 emit(MUL(header1_w, psiz, src_reg((float)(1 << 11))));
	 emit(AND(header1_w, src_reg(header1_w), 0x7ff << 8));
      }

      if (key->userclip_active) {
         current_annotation = "Clipping flags";
         dst_reg flags0 = dst_reg(this, glsl_type::uint_type);
         dst_reg flags1 = dst_reg(this, glsl_type::uint_type);

         emit(CMP(dst_null_f(), src_reg(output_reg[VARYING_SLOT_CLIP_DIST0]), src_reg(0.0f), BRW_CONDITIONAL_L));
         emit(VS_OPCODE_UNPACK_FLAGS_SIMD4X2, flags0, src_reg(0));
         emit(OR(header1_w, src_reg(header1_w), src_reg(flags0)));

         emit(CMP(dst_null_f(), src_reg(output_reg[VARYING_SLOT_CLIP_DIST1]), src_reg(0.0f), BRW_CONDITIONAL_L));
         emit(VS_OPCODE_UNPACK_FLAGS_SIMD4X2, flags1, src_reg(0));
         emit(SHL(flags1, src_reg(flags1), src_reg(4)));
         emit(OR(header1_w, src_reg(header1_w), src_reg(flags1)));
      }

      /* i965 clipping workaround:
       * 1) Test for -ve rhw
       * 2) If set,
       *      set ndc = (0,0,0,0)
       *      set ucp[6] = 1
       *
       * Later, clipping will detect ucp[6] and ensure the primitive is
       * clipped against all fixed planes.
       */
      if (brw->has_negative_rhw_bug) {
         src_reg ndc_w = src_reg(output_reg[BRW_VARYING_SLOT_NDC]);
         ndc_w.swizzle = BRW_SWIZZLE_WWWW;
         emit(CMP(dst_null_f(), ndc_w, src_reg(0.0f), BRW_CONDITIONAL_L));
         vec4_instruction *inst;
         inst = emit(OR(header1_w, src_reg(header1_w), src_reg(1u << 6)));
         inst->predicate = BRW_PREDICATE_NORMAL;
         inst = emit(MOV(output_reg[BRW_VARYING_SLOT_NDC], src_reg(0.0f)));
         inst->predicate = BRW_PREDICATE_NORMAL;
      }

      emit(MOV(retype(reg, BRW_REGISTER_TYPE_UD), src_reg(header1)));
   } else if (brw->gen < 6) {
      emit(MOV(retype(reg, BRW_REGISTER_TYPE_UD), 0u));
   } else {
      emit(MOV(retype(reg, BRW_REGISTER_TYPE_D), src_reg(0)));
      if (prog_data->vue_map.slots_valid & VARYING_BIT_PSIZ) {
         emit(MOV(brw_writemask(reg, WRITEMASK_W),
                  src_reg(output_reg[VARYING_SLOT_PSIZ])));
      }
      if (prog_data->vue_map.slots_valid & VARYING_BIT_LAYER) {
         emit(MOV(retype(brw_writemask(reg, WRITEMASK_Y), BRW_REGISTER_TYPE_D),
                  src_reg(output_reg[VARYING_SLOT_LAYER])));
      }
      if (prog_data->vue_map.slots_valid & VARYING_BIT_VIEWPORT) {
         emit(MOV(retype(brw_writemask(reg, WRITEMASK_Z), BRW_REGISTER_TYPE_D),
                  src_reg(output_reg[VARYING_SLOT_VIEWPORT])));
      }
   }
}

void
vec4_visitor::emit_clip_distances(dst_reg reg, int offset)
{
   /* From the GLSL 1.30 spec, section 7.1 (Vertex Shader Special Variables):
    *
    *     "If a linked set of shaders forming the vertex stage contains no
    *     static write to gl_ClipVertex or gl_ClipDistance, but the
    *     application has requested clipping against user clip planes through
    *     the API, then the coordinate written to gl_Position is used for
    *     comparison against the user clip planes."
    *
    * This function is only called if the shader didn't write to
    * gl_ClipDistance.  Accordingly, we use gl_ClipVertex to perform clipping
    * if the user wrote to it; otherwise we use gl_Position.
    */
   gl_varying_slot clip_vertex = VARYING_SLOT_CLIP_VERTEX;
   if (!(prog_data->vue_map.slots_valid & VARYING_BIT_CLIP_VERTEX)) {
      clip_vertex = VARYING_SLOT_POS;
   }

   for (int i = 0; i + offset < key->nr_userclip_plane_consts && i < 4;
        ++i) {
      reg.writemask = 1 << i;
      emit(DP4(reg,
               src_reg(output_reg[clip_vertex]),
               src_reg(this->userplane[i + offset])));
   }
}

void
vec4_visitor::emit_generic_urb_slot(dst_reg reg, int varying)
{
   assert (varying < VARYING_SLOT_MAX);
   reg.type = output_reg[varying].type;
   current_annotation = output_reg_annotation[varying];
   /* Copy the register, saturating if necessary */
   vec4_instruction *inst = emit(MOV(reg,
                                     src_reg(output_reg[varying])));
   if ((varying == VARYING_SLOT_COL0 ||
        varying == VARYING_SLOT_COL1 ||
        varying == VARYING_SLOT_BFC0 ||
        varying == VARYING_SLOT_BFC1) &&
       key->clamp_vertex_color) {
      inst->saturate = true;
   }
}

void
vec4_visitor::emit_urb_slot(int mrf, int varying)
{
   struct brw_reg hw_reg = brw_message_reg(mrf);
   dst_reg reg = dst_reg(MRF, mrf);
   reg.type = BRW_REGISTER_TYPE_F;

   switch (varying) {
   case VARYING_SLOT_PSIZ:
      /* PSIZ is always in slot 0, and is coupled with other flags. */
      current_annotation = "indices, point width, clip flags";
      emit_psiz_and_flags(hw_reg);
      break;
   case BRW_VARYING_SLOT_NDC:
      current_annotation = "NDC";
      emit(MOV(reg, src_reg(output_reg[BRW_VARYING_SLOT_NDC])));
      break;
   case VARYING_SLOT_POS:
      current_annotation = "gl_Position";
      emit(MOV(reg, src_reg(output_reg[VARYING_SLOT_POS])));
      break;
   case VARYING_SLOT_EDGE:
      /* This is present when doing unfilled polygons.  We're supposed to copy
       * the edge flag from the user-provided vertex array
       * (glEdgeFlagPointer), or otherwise we'll copy from the current value
       * of that attribute (starts as 1.0f).  This is then used in clipping to
       * determine which edges should be drawn as wireframe.
       */
      current_annotation = "edge flag";
      emit(MOV(reg, src_reg(dst_reg(ATTR, VERT_ATTRIB_EDGEFLAG,
                                    glsl_type::float_type, WRITEMASK_XYZW))));
      break;
   case BRW_VARYING_SLOT_PAD:
      /* No need to write to this slot */
      break;
   default:
      emit_generic_urb_slot(reg, varying);
      break;
   }
}

static int
align_interleaved_urb_mlen(struct brw_context *brw, int mlen)
{
   if (brw->gen >= 6) {
      /* URB data written (does not include the message header reg) must
       * be a multiple of 256 bits, or 2 VS registers.  See vol5c.5,
       * section 5.4.3.2.2: URB_INTERLEAVED.
       *
       * URB entries are allocated on a multiple of 1024 bits, so an
       * extra 128 bits written here to make the end align to 256 is
       * no problem.
       */
      if ((mlen % 2) != 1)
	 mlen++;
   }

   return mlen;
}


/**
 * Generates the VUE payload plus the necessary URB write instructions to
 * output it.
 *
 * The VUE layout is documented in Volume 2a.
 */
void
vec4_visitor::emit_vertex()
{
   /* MRF 0 is reserved for the debugger, so start with message header
    * in MRF 1.
    */
   int base_mrf = 1;
   int mrf = base_mrf;
   /* In the process of generating our URB write message contents, we
    * may need to unspill a register or load from an array.  Those
    * reads would use MRFs 14-15.
    */
   int max_usable_mrf = 13;

   /* The following assertion verifies that max_usable_mrf causes an
    * even-numbered amount of URB write data, which will meet gen6's
    * requirements for length alignment.
    */
   assert ((max_usable_mrf - base_mrf) % 2 == 0);

   /* First mrf is the g0-based message header containing URB handles and
    * such.
    */
   emit_urb_write_header(mrf++);

   if (brw->gen < 6) {
      emit_ndc_computation();
   }

   /* Lower legacy ff and ClipVertex clipping to clip distances */
   if (key->userclip_active && !prog->UsesClipDistanceOut) {
      current_annotation = "user clip distances";

      output_reg[VARYING_SLOT_CLIP_DIST0] = dst_reg(this, glsl_type::vec4_type);
      output_reg[VARYING_SLOT_CLIP_DIST1] = dst_reg(this, glsl_type::vec4_type);

      emit_clip_distances(output_reg[VARYING_SLOT_CLIP_DIST0], 0);
      emit_clip_distances(output_reg[VARYING_SLOT_CLIP_DIST1], 4);
   }

   /* We may need to split this up into several URB writes, so do them in a
    * loop.
    */
   int slot = 0;
   bool complete = false;
   do {
      /* URB offset is in URB row increments, and each of our MRFs is half of
       * one of those, since we're doing interleaved writes.
       */
      int offset = slot / 2;

      mrf = base_mrf + 1;
      for (; slot < prog_data->vue_map.num_slots; ++slot) {
         emit_urb_slot(mrf++, prog_data->vue_map.slot_to_varying[slot]);

         /* If this was max_usable_mrf, we can't fit anything more into this
          * URB WRITE.
          */
         if (mrf > max_usable_mrf) {
            slot++;
            break;
         }
      }

      complete = slot >= prog_data->vue_map.num_slots;
      current_annotation = "URB write";
      vec4_instruction *inst = emit_urb_write_opcode(complete);
      inst->base_mrf = base_mrf;
      inst->mlen = align_interleaved_urb_mlen(brw, mrf - base_mrf);
      inst->offset += offset;
   } while(!complete);
}


src_reg
vec4_visitor::get_scratch_offset(vec4_instruction *inst,
				 src_reg *reladdr, int reg_offset)
{
   /* Because we store the values to scratch interleaved like our
    * vertex data, we need to scale the vec4 index by 2.
    */
   int message_header_scale = 2;

   /* Pre-gen6, the message header uses byte offsets instead of vec4
    * (16-byte) offset units.
    */
   if (brw->gen < 6)
      message_header_scale *= 16;

   if (reladdr) {
      src_reg index = src_reg(this, glsl_type::int_type);

      emit_before(inst, ADD(dst_reg(index), *reladdr, src_reg(reg_offset)));
      emit_before(inst, MUL(dst_reg(index),
			    index, src_reg(message_header_scale)));

      return index;
   } else {
      return src_reg(reg_offset * message_header_scale);
   }
}

src_reg
vec4_visitor::get_pull_constant_offset(vec4_instruction *inst,
				       src_reg *reladdr, int reg_offset)
{
   if (reladdr) {
      src_reg index = src_reg(this, glsl_type::int_type);

      emit_before(inst, ADD(dst_reg(index), *reladdr, src_reg(reg_offset)));

      /* Pre-gen6, the message header uses byte offsets instead of vec4
       * (16-byte) offset units.
       */
      if (brw->gen < 6) {
	 emit_before(inst, MUL(dst_reg(index), index, src_reg(16)));
      }

      return index;
   } else if (brw->gen >= 8) {
      /* Store the offset in a GRF so we can send-from-GRF. */
      src_reg offset = src_reg(this, glsl_type::int_type);
      emit_before(inst, MOV(dst_reg(offset), src_reg(reg_offset)));
      return offset;
   } else {
      int message_header_scale = brw->gen < 6 ? 16 : 1;
      return src_reg(reg_offset * message_header_scale);
   }
}

/**
 * Emits an instruction before @inst to load the value named by @orig_src
 * from scratch space at @base_offset to @temp.
 *
 * @base_offset is measured in 32-byte units (the size of a register).
 */
void
vec4_visitor::emit_scratch_read(vec4_instruction *inst,
				dst_reg temp, src_reg orig_src,
				int base_offset)
{
   int reg_offset = base_offset + orig_src.reg_offset;
   src_reg index = get_scratch_offset(inst, orig_src.reladdr, reg_offset);

   emit_before(inst, SCRATCH_READ(temp, index));
}

/**
 * Emits an instruction after @inst to store the value to be written
 * to @orig_dst to scratch space at @base_offset, from @temp.
 *
 * @base_offset is measured in 32-byte units (the size of a register).
 */
void
vec4_visitor::emit_scratch_write(vec4_instruction *inst, int base_offset)
{
   int reg_offset = base_offset + inst->dst.reg_offset;
   src_reg index = get_scratch_offset(inst, inst->dst.reladdr, reg_offset);

   /* Create a temporary register to store *inst's result in.
    *
    * We have to be careful in MOVing from our temporary result register in
    * the scratch write.  If we swizzle from channels of the temporary that
    * weren't initialized, it will confuse live interval analysis, which will
    * make spilling fail to make progress.
    */
   src_reg temp = src_reg(this, glsl_type::vec4_type);
   temp.type = inst->dst.type;
   int first_writemask_chan = ffs(inst->dst.writemask) - 1;
   int swizzles[4];
   for (int i = 0; i < 4; i++)
      if (inst->dst.writemask & (1 << i))
         swizzles[i] = i;
      else
         swizzles[i] = first_writemask_chan;
   temp.swizzle = BRW_SWIZZLE4(swizzles[0], swizzles[1],
                               swizzles[2], swizzles[3]);

   dst_reg dst = dst_reg(brw_writemask(brw_vec8_grf(0, 0),
				       inst->dst.writemask));
   vec4_instruction *write = SCRATCH_WRITE(dst, temp, index);
   write->predicate = inst->predicate;
   write->ir = inst->ir;
   write->annotation = inst->annotation;
   inst->insert_after(write);

   inst->dst.file = temp.file;
   inst->dst.reg = temp.reg;
   inst->dst.reg_offset = temp.reg_offset;
   inst->dst.reladdr = NULL;
}

/**
 * We can't generally support array access in GRF space, because a
 * single instruction's destination can only span 2 contiguous
 * registers.  So, we send all GRF arrays that get variable index
 * access to scratch space.
 */
void
vec4_visitor::move_grf_array_access_to_scratch()
{
   int scratch_loc[this->virtual_grf_count];

   for (int i = 0; i < this->virtual_grf_count; i++) {
      scratch_loc[i] = -1;
   }

   /* First, calculate the set of virtual GRFs that need to be punted
    * to scratch due to having any array access on them, and where in
    * scratch.
    */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      if (inst->dst.file == GRF && inst->dst.reladdr &&
	  scratch_loc[inst->dst.reg] == -1) {
	 scratch_loc[inst->dst.reg] = c->last_scratch;
	 c->last_scratch += this->virtual_grf_sizes[inst->dst.reg];
      }

      for (int i = 0 ; i < 3; i++) {
	 src_reg *src = &inst->src[i];

	 if (src->file == GRF && src->reladdr &&
	     scratch_loc[src->reg] == -1) {
	    scratch_loc[src->reg] = c->last_scratch;
	    c->last_scratch += this->virtual_grf_sizes[src->reg];
	 }
      }
   }

   /* Now, for anything that will be accessed through scratch, rewrite
    * it to load/store.  Note that this is a _safe list walk, because
    * we may generate a new scratch_write instruction after the one
    * we're processing.
    */
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      /* Set up the annotation tracking for new generated instructions. */
      base_ir = inst->ir;
      current_annotation = inst->annotation;

      if (inst->dst.file == GRF && scratch_loc[inst->dst.reg] != -1) {
	 emit_scratch_write(inst, scratch_loc[inst->dst.reg]);
      }

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != GRF || scratch_loc[inst->src[i].reg] == -1)
	    continue;

	 dst_reg temp = dst_reg(this, glsl_type::vec4_type);

	 emit_scratch_read(inst, temp, inst->src[i],
			   scratch_loc[inst->src[i].reg]);

	 inst->src[i].file = temp.file;
	 inst->src[i].reg = temp.reg;
	 inst->src[i].reg_offset = temp.reg_offset;
	 inst->src[i].reladdr = NULL;
      }
   }
}

/**
 * Emits an instruction before @inst to load the value named by @orig_src
 * from the pull constant buffer (surface) at @base_offset to @temp.
 */
void
vec4_visitor::emit_pull_constant_load(vec4_instruction *inst,
				      dst_reg temp, src_reg orig_src,
				      int base_offset)
{
   int reg_offset = base_offset + orig_src.reg_offset;
   src_reg index = src_reg(prog_data->base.binding_table.pull_constants_start);
   src_reg offset = get_pull_constant_offset(inst, orig_src.reladdr, reg_offset);
   vec4_instruction *load;

   if (brw->gen >= 7) {
      dst_reg grf_offset = dst_reg(this, glsl_type::int_type);
      grf_offset.type = offset.type;
      emit_before(inst, MOV(grf_offset, offset));

      load = new(mem_ctx) vec4_instruction(this,
                                           VS_OPCODE_PULL_CONSTANT_LOAD_GEN7,
                                           temp, index, src_reg(grf_offset));
   } else {
      load = new(mem_ctx) vec4_instruction(this, VS_OPCODE_PULL_CONSTANT_LOAD,
                                           temp, index, offset);
      load->base_mrf = 14;
      load->mlen = 1;
   }
   emit_before(inst, load);
}

/**
 * Implements array access of uniforms by inserting a
 * PULL_CONSTANT_LOAD instruction.
 *
 * Unlike temporary GRF array access (where we don't support it due to
 * the difficulty of doing relative addressing on instruction
 * destinations), we could potentially do array access of uniforms
 * that were loaded in GRF space as push constants.  In real-world
 * usage we've seen, though, the arrays being used are always larger
 * than we could load as push constants, so just always move all
 * uniform array access out to a pull constant buffer.
 */
void
vec4_visitor::move_uniform_array_access_to_pull_constants()
{
   int pull_constant_loc[this->uniforms];

   for (int i = 0; i < this->uniforms; i++) {
      pull_constant_loc[i] = -1;
   }

   /* Walk through and find array access of uniforms.  Put a copy of that
    * uniform in the pull constant buffer.
    *
    * Note that we don't move constant-indexed accesses to arrays.  No
    * testing has been done of the performance impact of this choice.
    */
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM || !inst->src[i].reladdr)
	    continue;

	 int uniform = inst->src[i].reg;

	 /* If this array isn't already present in the pull constant buffer,
	  * add it.
	  */
	 if (pull_constant_loc[uniform] == -1) {
	    const float **values = &stage_prog_data->param[uniform * 4];

	    pull_constant_loc[uniform] = stage_prog_data->nr_pull_params / 4;

	    assert(uniform < uniform_array_size);
	    for (int j = 0; j < uniform_size[uniform] * 4; j++) {
	       stage_prog_data->pull_param[stage_prog_data->nr_pull_params++]
                  = values[j];
	    }
	 }

	 /* Set up the annotation tracking for new generated instructions. */
	 base_ir = inst->ir;
	 current_annotation = inst->annotation;

	 dst_reg temp = dst_reg(this, glsl_type::vec4_type);

	 emit_pull_constant_load(inst, temp, inst->src[i],
				 pull_constant_loc[uniform]);

	 inst->src[i].file = temp.file;
	 inst->src[i].reg = temp.reg;
	 inst->src[i].reg_offset = temp.reg_offset;
	 inst->src[i].reladdr = NULL;
      }
   }

   /* Now there are no accesses of the UNIFORM file with a reladdr, so
    * no need to track them as larger-than-vec4 objects.  This will be
    * relied on in cutting out unused uniform vectors from push
    * constants.
    */
   split_uniform_registers();
}

void
vec4_visitor::resolve_ud_negate(src_reg *reg)
{
   if (reg->type != BRW_REGISTER_TYPE_UD ||
       !reg->negate)
      return;

   src_reg temp = src_reg(this, glsl_type::uvec4_type);
   emit(BRW_OPCODE_MOV, dst_reg(temp), *reg);
   *reg = temp;
}

vec4_visitor::vec4_visitor(struct brw_context *brw,
                           struct brw_vec4_compile *c,
                           struct gl_program *prog,
                           const struct brw_vec4_prog_key *key,
                           struct brw_vec4_prog_data *prog_data,
			   struct gl_shader_program *shader_prog,
                           gl_shader_stage stage,
			   void *mem_ctx,
                           bool debug_flag,
                           bool no_spills,
                           shader_time_shader_type st_base,
                           shader_time_shader_type st_written,
                           shader_time_shader_type st_reset)
   : backend_visitor(brw, shader_prog, prog, &prog_data->base, stage),
     c(c),
     key(key),
     prog_data(prog_data),
     sanity_param_count(0),
     fail_msg(NULL),
     first_non_payload_grf(0),
     need_all_constants_in_pull_buffer(false),
     debug_flag(debug_flag),
     no_spills(no_spills),
     st_base(st_base),
     st_written(st_written),
     st_reset(st_reset)
{
   this->mem_ctx = mem_ctx;
   this->failed = false;

   this->base_ir = NULL;
   this->current_annotation = NULL;
   memset(this->output_reg_annotation, 0, sizeof(this->output_reg_annotation));

   this->variable_ht = hash_table_ctor(0,
				       hash_table_pointer_hash,
				       hash_table_pointer_compare);

   this->virtual_grf_start = NULL;
   this->virtual_grf_end = NULL;
   this->virtual_grf_sizes = NULL;
   this->virtual_grf_count = 0;
   this->virtual_grf_reg_map = NULL;
   this->virtual_grf_reg_count = 0;
   this->virtual_grf_array_size = 0;
   this->live_intervals_valid = false;

   this->max_grf = brw->gen >= 7 ? GEN7_MRF_HACK_START : BRW_MAX_GRF;

   this->uniforms = 0;

   /* Initialize uniform_array_size to at least 1 because pre-gen6 VS requires
    * at least one. See setup_uniforms() in brw_vec4.cpp.
    */
   this->uniform_array_size = 1;
   if (prog_data) {
      this->uniform_array_size = MAX2(stage_prog_data->nr_params, 1);
   }

   this->uniform_size = rzalloc_array(mem_ctx, int, this->uniform_array_size);
   this->uniform_vector_size = rzalloc_array(mem_ctx, int, this->uniform_array_size);
}

vec4_visitor::~vec4_visitor()
{
   hash_table_dtor(this->variable_ht);
}


void
vec4_visitor::fail(const char *format, ...)
{
   va_list va;
   char *msg;

   if (failed)
      return;

   failed = true;

   va_start(va, format);
   msg = ralloc_vasprintf(mem_ctx, format, va);
   va_end(va);
   msg = ralloc_asprintf(mem_ctx, "vec4 compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (debug_flag) {
      fprintf(stderr, "%s",  msg);
   }
}

} /* namespace brw */
