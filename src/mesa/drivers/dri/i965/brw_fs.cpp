/*
 * Copyright © 2010 Intel Corporation
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

/** @file brw_fs.cpp
 *
 * This file drives the GLSL IR -> LIR translation, contains the
 * optimizations on the LIR, and drives the generation of native code
 * from the LIR.
 */

extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "main/fbobject.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
}
#include "brw_fs.h"
#include "glsl/glsl_types.h"
#include "glsl/ir_print_visitor.h"

void
fs_inst::init()
{
   memset(this, 0, sizeof(*this));
   this->opcode = BRW_OPCODE_NOP;
   this->conditional_mod = BRW_CONDITIONAL_NONE;

   this->dst = reg_undef;
   this->src[0] = reg_undef;
   this->src[1] = reg_undef;
   this->src[2] = reg_undef;

   /* This will be the case for almost all instructions. */
   this->regs_written = 1;
}

fs_inst::fs_inst()
{
   init();
}

fs_inst::fs_inst(enum opcode opcode)
{
   init();
   this->opcode = opcode;
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst)
{
   init();
   this->opcode = opcode;
   this->dst = dst;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst, fs_reg src0)
{
   init();
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
   if (src[0].file == GRF)
      assert(src[0].reg_offset >= 0);
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   init();
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;
   this->src[1] = src1;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
   if (src[0].file == GRF)
      assert(src[0].reg_offset >= 0);
   if (src[1].file == GRF)
      assert(src[1].reg_offset >= 0);
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst,
		 fs_reg src0, fs_reg src1, fs_reg src2)
{
   init();
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;
   this->src[1] = src1;
   this->src[2] = src2;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
   if (src[0].file == GRF)
      assert(src[0].reg_offset >= 0);
   if (src[1].file == GRF)
      assert(src[1].reg_offset >= 0);
   if (src[2].file == GRF)
      assert(src[2].reg_offset >= 0);
}

#define ALU1(op)                                                        \
   fs_inst *                                                            \
   fs_visitor::op(fs_reg dst, fs_reg src0)                              \
   {                                                                    \
      return new(mem_ctx) fs_inst(BRW_OPCODE_##op, dst, src0);          \
   }

#define ALU2(op)                                                        \
   fs_inst *                                                            \
   fs_visitor::op(fs_reg dst, fs_reg src0, fs_reg src1)                 \
   {                                                                    \
      return new(mem_ctx) fs_inst(BRW_OPCODE_##op, dst, src0, src1);    \
   }

#define ALU3(op)                                                        \
   fs_inst *                                                            \
   fs_visitor::op(fs_reg dst, fs_reg src0, fs_reg src1, fs_reg src2)    \
   {                                                                    \
      return new(mem_ctx) fs_inst(BRW_OPCODE_##op, dst, src0, src1, src2);\
   }

ALU1(NOT)
ALU1(MOV)
ALU1(FRC)
ALU1(RNDD)
ALU1(RNDE)
ALU1(RNDZ)
ALU2(ADD)
ALU2(MUL)
ALU2(MACH)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHL)
ALU2(SHR)
ALU2(ASR)
ALU3(LRP)

/** Gen4 predicated IF. */
fs_inst *
fs_visitor::IF(uint32_t predicate)
{
   fs_inst *inst = new(mem_ctx) fs_inst(BRW_OPCODE_IF);
   inst->predicate = predicate;
   return inst;
}

/** Gen6+ IF with embedded comparison. */
fs_inst *
fs_visitor::IF(fs_reg src0, fs_reg src1, uint32_t condition)
{
   assert(intel->gen >= 6);
   fs_inst *inst = new(mem_ctx) fs_inst(BRW_OPCODE_IF,
                                        reg_null_d, src0, src1);
   inst->conditional_mod = condition;
   return inst;
}

/**
 * CMP: Sets the low bit of the destination channels with the result
 * of the comparison, while the upper bits are undefined, and updates
 * the flag register with the packed 16 bits of the result.
 */
fs_inst *
fs_visitor::CMP(fs_reg dst, fs_reg src0, fs_reg src1, uint32_t condition)
{
   fs_inst *inst;

   /* Take the instruction:
    *
    * CMP null<d> src0<f> src1<f>
    *
    * Original gen4 does type conversion to the destination type before
    * comparison, producing garbage results for floating point comparisons.
    * gen5 does the comparison on the execution type (resolved source types),
    * so dst type doesn't matter.  gen6 does comparison and then uses the
    * result as if it was the dst type with no conversion, which happens to
    * mostly work out for float-interpreted-as-int since our comparisons are
    * for >0, =0, <0.
    */
   if (intel->gen == 4) {
      dst.type = src0.type;
      if (dst.file == FIXED_HW_REG)
	 dst.fixed_hw_reg.type = dst.type;
   }

   resolve_ud_negate(&src0);
   resolve_ud_negate(&src1);

   inst = new(mem_ctx) fs_inst(BRW_OPCODE_CMP, dst, src0, src1);
   inst->conditional_mod = condition;

   return inst;
}

exec_list
fs_visitor::VARYING_PULL_CONSTANT_LOAD(fs_reg dst, fs_reg surf_index,
                                       fs_reg varying_offset,
                                       uint32_t const_offset)
{
   exec_list instructions;
   fs_inst *inst;

   /* We have our constant surface use a pitch of 4 bytes, so our index can
    * be any component of a vector, and then we load 4 contiguous
    * components starting from that.
    *
    * We break down the const_offset to a portion added to the variable
    * offset and a portion done using reg_offset, which means that if you
    * have GLSL using something like "uniform vec4 a[20]; gl_FragColor =
    * a[i]", we'll temporarily generate 4 vec4 loads from offset i * 4, and
    * CSE can later notice that those loads are all the same and eliminate
    * the redundant ones.
    */
   fs_reg vec4_offset = fs_reg(this, glsl_type::int_type);
   instructions.push_tail(ADD(vec4_offset,
                              varying_offset, const_offset & ~3));

   int scale = 1;
   if (intel->gen == 4 && dispatch_width == 8) {
      /* Pre-gen5, we can either use a SIMD8 message that requires (header,
       * u, v, r) as parameters, or we can just use the SIMD16 message
       * consisting of (header, u).  We choose the second, at the cost of a
       * longer return length.
       */
      scale = 2;
   }

   enum opcode op;
   if (intel->gen >= 7)
      op = FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7;
   else
      op = FS_OPCODE_VARYING_PULL_CONSTANT_LOAD;
   fs_reg vec4_result = fs_reg(GRF, virtual_grf_alloc(4 * scale), dst.type);
   inst = new(mem_ctx) fs_inst(op, vec4_result, surf_index, vec4_offset);
   inst->regs_written = 4 * scale;
   instructions.push_tail(inst);

   if (intel->gen < 7) {
      inst->base_mrf = 13;
      inst->header_present = true;
      if (intel->gen == 4)
         inst->mlen = 3;
      else
         inst->mlen = 1 + dispatch_width / 8;
   }

   vec4_result.reg_offset += (const_offset & 3) * scale;
   instructions.push_tail(MOV(dst, vec4_result));

   return instructions;
}

/**
 * A helper for MOV generation for fixing up broken hardware SEND dependency
 * handling.
 */
fs_inst *
fs_visitor::DEP_RESOLVE_MOV(int grf)
{
   fs_inst *inst = MOV(brw_null_reg(), fs_reg(GRF, grf, BRW_REGISTER_TYPE_F));

   inst->ir = NULL;
   inst->annotation = "send dependency resolve";

   /* The caller always wants uncompressed to emit the minimal extra
    * dependencies, and to avoid having to deal with aligning its regs to 2.
    */
   inst->force_uncompressed = true;

   return inst;
}

bool
fs_inst::equals(fs_inst *inst)
{
   return (opcode == inst->opcode &&
           dst.equals(inst->dst) &&
           src[0].equals(inst->src[0]) &&
           src[1].equals(inst->src[1]) &&
           src[2].equals(inst->src[2]) &&
           saturate == inst->saturate &&
           predicate == inst->predicate &&
           conditional_mod == inst->conditional_mod &&
           mlen == inst->mlen &&
           base_mrf == inst->base_mrf &&
           sampler == inst->sampler &&
           target == inst->target &&
           eot == inst->eot &&
           header_present == inst->header_present &&
           shadow_compare == inst->shadow_compare &&
           offset == inst->offset);
}

bool
fs_inst::overwrites_reg(const fs_reg &reg)
{
   return (reg.file == dst.file &&
           reg.reg == dst.reg &&
           reg.reg_offset >= dst.reg_offset  &&
           reg.reg_offset < dst.reg_offset + regs_written);
}

bool
fs_inst::is_tex()
{
   return (opcode == SHADER_OPCODE_TEX ||
           opcode == FS_OPCODE_TXB ||
           opcode == SHADER_OPCODE_TXD ||
           opcode == SHADER_OPCODE_TXF ||
           opcode == SHADER_OPCODE_TXF_MS ||
           opcode == SHADER_OPCODE_TXL ||
           opcode == SHADER_OPCODE_TXS ||
           opcode == SHADER_OPCODE_LOD);
}

bool
fs_inst::is_math()
{
   return (opcode == SHADER_OPCODE_RCP ||
           opcode == SHADER_OPCODE_RSQ ||
           opcode == SHADER_OPCODE_SQRT ||
           opcode == SHADER_OPCODE_EXP2 ||
           opcode == SHADER_OPCODE_LOG2 ||
           opcode == SHADER_OPCODE_SIN ||
           opcode == SHADER_OPCODE_COS ||
           opcode == SHADER_OPCODE_INT_QUOTIENT ||
           opcode == SHADER_OPCODE_INT_REMAINDER ||
           opcode == SHADER_OPCODE_POW);
}

bool
fs_inst::is_control_flow()
{
   switch (opcode) {
   case BRW_OPCODE_DO:
   case BRW_OPCODE_WHILE:
   case BRW_OPCODE_IF:
   case BRW_OPCODE_ELSE:
   case BRW_OPCODE_ENDIF:
   case BRW_OPCODE_BREAK:
   case BRW_OPCODE_CONTINUE:
      return true;
   default:
      return false;
   }
}

bool
fs_inst::is_send_from_grf()
{
   return (opcode == FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7 ||
           opcode == SHADER_OPCODE_SHADER_TIME_ADD ||
           (opcode == FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD &&
            src[1].file == GRF));
}

bool
fs_visitor::can_do_source_mods(fs_inst *inst)
{
   if (intel->gen == 6 && inst->is_math())
      return false;

   if (inst->is_send_from_grf())
      return false;

   return true;
}

void
fs_reg::init()
{
   memset(this, 0, sizeof(*this));
   this->smear = -1;
}

/** Generic unset register constructor. */
fs_reg::fs_reg()
{
   init();
   this->file = BAD_FILE;
}

/** Immediate value constructor. */
fs_reg::fs_reg(float f)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_F;
   this->imm.f = f;
}

/** Immediate value constructor. */
fs_reg::fs_reg(int32_t i)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_D;
   this->imm.i = i;
}

/** Immediate value constructor. */
fs_reg::fs_reg(uint32_t u)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_UD;
   this->imm.u = u;
}

/** Fixed brw_reg Immediate value constructor. */
fs_reg::fs_reg(struct brw_reg fixed_hw_reg)
{
   init();
   this->file = FIXED_HW_REG;
   this->fixed_hw_reg = fixed_hw_reg;
   this->type = fixed_hw_reg.type;
}

bool
fs_reg::equals(const fs_reg &r) const
{
   return (file == r.file &&
           reg == r.reg &&
           reg_offset == r.reg_offset &&
           type == r.type &&
           negate == r.negate &&
           abs == r.abs &&
           !reladdr && !r.reladdr &&
           memcmp(&fixed_hw_reg, &r.fixed_hw_reg,
                  sizeof(fixed_hw_reg)) == 0 &&
           smear == r.smear &&
           imm.u == r.imm.u);
}

bool
fs_reg::is_zero() const
{
   if (file != IMM)
      return false;

   return type == BRW_REGISTER_TYPE_F ? imm.f == 0.0 : imm.i == 0;
}

bool
fs_reg::is_one() const
{
   if (file != IMM)
      return false;

   return type == BRW_REGISTER_TYPE_F ? imm.f == 1.0 : imm.i == 1;
}

int
fs_visitor::type_size(const struct glsl_type *type)
{
   unsigned int size, i;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return type->components();
   case GLSL_TYPE_ARRAY:
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up no register space, since they're baked in at
       * link time.
       */
      return 0;
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_INTERFACE:
      assert(!"not reached");
      break;
   }

   return 0;
}

fs_reg
fs_visitor::get_timestamp()
{
   assert(intel->gen >= 7);

   fs_reg ts = fs_reg(retype(brw_vec1_reg(BRW_ARCHITECTURE_REGISTER_FILE,
                                          BRW_ARF_TIMESTAMP,
                                          0),
                             BRW_REGISTER_TYPE_UD));

   fs_reg dst = fs_reg(this, glsl_type::uint_type);

   fs_inst *mov = emit(MOV(dst, ts));
   /* We want to read the 3 fields we care about (mostly field 0, but also 2)
    * even if it's not enabled in the dispatch.
    */
   mov->force_writemask_all = true;
   mov->force_uncompressed = true;

   /* The caller wants the low 32 bits of the timestamp.  Since it's running
    * at the GPU clock rate of ~1.2ghz, it will roll over every ~3 seconds,
    * which is plenty of time for our purposes.  It is identical across the
    * EUs, but since it's tracking GPU core speed it will increment at a
    * varying rate as render P-states change.
    *
    * The caller could also check if render P-states have changed (or anything
    * else that might disrupt timing) by setting smear to 2 and checking if
    * that field is != 0.
    */
   dst.smear = 0;

   return dst;
}

void
fs_visitor::emit_shader_time_begin()
{
   current_annotation = "shader time start";
   shader_start_time = get_timestamp();
}

void
fs_visitor::emit_shader_time_end()
{
   current_annotation = "shader time end";

   enum shader_time_shader_type type, written_type, reset_type;
   if (dispatch_width == 8) {
      type = ST_FS8;
      written_type = ST_FS8_WRITTEN;
      reset_type = ST_FS8_RESET;
   } else {
      assert(dispatch_width == 16);
      type = ST_FS16;
      written_type = ST_FS16_WRITTEN;
      reset_type = ST_FS16_RESET;
   }

   fs_reg shader_end_time = get_timestamp();

   /* Check that there weren't any timestamp reset events (assuming these
    * were the only two timestamp reads that happened).
    */
   fs_reg reset = shader_end_time;
   reset.smear = 2;
   fs_inst *test = emit(AND(reg_null_d, reset, fs_reg(1u)));
   test->conditional_mod = BRW_CONDITIONAL_Z;
   emit(IF(BRW_PREDICATE_NORMAL));

   push_force_uncompressed();
   fs_reg start = shader_start_time;
   start.negate = true;
   fs_reg diff = fs_reg(this, glsl_type::uint_type);
   emit(ADD(diff, start, shader_end_time));

   /* If there were no instructions between the two timestamp gets, the diff
    * is 2 cycles.  Remove that overhead, so I can forget about that when
    * trying to determine the time taken for single instructions.
    */
   emit(ADD(diff, diff, fs_reg(-2u)));

   emit_shader_time_write(type, diff);
   emit_shader_time_write(written_type, fs_reg(1u));
   emit(BRW_OPCODE_ELSE);
   emit_shader_time_write(reset_type, fs_reg(1u));
   emit(BRW_OPCODE_ENDIF);

   pop_force_uncompressed();
}

void
fs_visitor::emit_shader_time_write(enum shader_time_shader_type type,
                                   fs_reg value)
{
   int shader_time_index = brw_get_shader_time_index(brw, prog, &fp->Base,
                                                     type);
   fs_reg offset = fs_reg(shader_time_index * SHADER_TIME_STRIDE);

   fs_reg payload;
   if (dispatch_width == 8)
      payload = fs_reg(this, glsl_type::uvec2_type);
   else
      payload = fs_reg(this, glsl_type::uint_type);

   emit(fs_inst(SHADER_OPCODE_SHADER_TIME_ADD,
                fs_reg(), payload, offset, value));
}

void
fs_visitor::fail(const char *format, ...)
{
   va_list va;
   char *msg;

   if (failed)
      return;

   failed = true;

   va_start(va, format);
   msg = ralloc_vasprintf(mem_ctx, format, va);
   va_end(va);
   msg = ralloc_asprintf(mem_ctx, "FS compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (INTEL_DEBUG & DEBUG_WM) {
      fprintf(stderr, "%s",  msg);
   }
}

fs_inst *
fs_visitor::emit(enum opcode opcode)
{
   return emit(fs_inst(opcode));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst)
{
   return emit(fs_inst(opcode, dst));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst, fs_reg src0)
{
   return emit(fs_inst(opcode, dst, src0));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   return emit(fs_inst(opcode, dst, src0, src1));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst,
                 fs_reg src0, fs_reg src1, fs_reg src2)
{
   return emit(fs_inst(opcode, dst, src0, src1, src2));
}

void
fs_visitor::push_force_uncompressed()
{
   force_uncompressed_stack++;
}

void
fs_visitor::pop_force_uncompressed()
{
   force_uncompressed_stack--;
   assert(force_uncompressed_stack >= 0);
}

void
fs_visitor::push_force_sechalf()
{
   force_sechalf_stack++;
}

void
fs_visitor::pop_force_sechalf()
{
   force_sechalf_stack--;
   assert(force_sechalf_stack >= 0);
}

/**
 * Returns how many MRFs an FS opcode will write over.
 *
 * Note that this is not the 0 or 1 implied writes in an actual gen
 * instruction -- the FS opcodes often generate MOVs in addition.
 */
int
fs_visitor::implied_mrf_writes(fs_inst *inst)
{
   if (inst->mlen == 0)
      return 0;

   switch (inst->opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      return 1 * dispatch_width / 8;
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      return 2 * dispatch_width / 8;
   case SHADER_OPCODE_TEX:
   case FS_OPCODE_TXB:
   case SHADER_OPCODE_TXD:
   case SHADER_OPCODE_TXF:
   case SHADER_OPCODE_TXF_MS:
   case SHADER_OPCODE_TXL:
   case SHADER_OPCODE_TXS:
   case SHADER_OPCODE_LOD:
      return 1;
   case FS_OPCODE_FB_WRITE:
      return 2;
   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
   case FS_OPCODE_UNSPILL:
      return 1;
   case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD:
      return inst->mlen;
   case FS_OPCODE_SPILL:
      return 2;
   default:
      assert(!"not reached");
      return inst->mlen;
   }
}

int
fs_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_count) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = reralloc(mem_ctx, virtual_grf_sizes, int,
				   virtual_grf_array_size);
   }
   virtual_grf_sizes[virtual_grf_count] = size;
   return virtual_grf_count++;
}

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int reg)
{
   init();
   this->file = file;
   this->reg = reg;
   this->type = BRW_REGISTER_TYPE_F;
}

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int reg, uint32_t type)
{
   init();
   this->file = file;
   this->reg = reg;
   this->type = type;
}

/** Automatic reg constructor. */
fs_reg::fs_reg(class fs_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(v->type_size(type));
   this->reg_offset = 0;
   this->type = brw_type_for_base_type(type);
}

fs_reg *
fs_visitor::variable_storage(ir_variable *var)
{
   return (fs_reg *)hash_table_find(this->variable_ht, var);
}

void
import_uniforms_callback(const void *key,
			 void *data,
			 void *closure)
{
   struct hash_table *dst_ht = (struct hash_table *)closure;
   const fs_reg *reg = (const fs_reg *)data;

   if (reg->file != UNIFORM)
      return;

   hash_table_insert(dst_ht, data, key);
}

/* For 16-wide, we need to follow from the uniform setup of 8-wide dispatch.
 * This brings in those uniform definitions
 */
void
fs_visitor::import_uniforms(fs_visitor *v)
{
   hash_table_call_foreach(v->variable_ht,
			   import_uniforms_callback,
			   variable_ht);
   this->params_remap = v->params_remap;
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
void
fs_visitor::setup_uniform_values(ir_variable *ir)
{
   int namelen = strlen(ir->name);

   /* The data for our (non-builtin) uniforms is stored in a series of
    * gl_uniform_driver_storage structs for each subcomponent that
    * glGetUniformLocation() could name.  We know it's been set up in the same
    * order we'd walk the type, so walk the list of storage and find anything
    * with our name, or the prefix of a component that starts with our name.
    */
   unsigned params_before = c->prog_data.nr_params;
   for (unsigned u = 0; u < prog->NumUserUniformStorage; u++) {
      struct gl_uniform_storage *storage = &prog->UniformStorage[u];

      if (strncmp(ir->name, storage->name, namelen) != 0 ||
          (storage->name[namelen] != 0 &&
           storage->name[namelen] != '.' &&
           storage->name[namelen] != '[')) {
         continue;
      }

      unsigned slots = storage->type->component_slots();
      if (storage->array_elements)
         slots *= storage->array_elements;

      for (unsigned i = 0; i < slots; i++) {
         c->prog_data.param[c->prog_data.nr_params++] =
            &storage->storage[i].f;
      }
   }

   /* Make sure we actually initialized the right amount of stuff here. */
   assert(params_before + ir->type->component_slots() ==
          c->prog_data.nr_params);
}


/* Our support for builtin uniforms is even scarier than non-builtin.
 * It sits on top of the PROG_STATE_VAR parameters that are
 * automatically updated from GL context state.
 */
void
fs_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const ir_state_slot *const slots = ir->state_slots;
   assert(ir->state_slots != NULL);

   for (unsigned int i = 0; i < ir->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa, but we'll
       * get the same index back here.
       */
      int index = _mesa_add_state_reference(this->fp->Base.Parameters,
					    (gl_state_index *)slots[i].tokens);

      /* Add each of the unique swizzles of the element as a parameter.
       * This'll end up matching the expected layout of the
       * array/matrix/structure we're trying to fill in.
       */
      int last_swiz = -1;
      for (unsigned int j = 0; j < 4; j++) {
	 int swiz = GET_SWZ(slots[i].swizzle, j);
	 if (swiz == last_swiz)
	    break;
	 last_swiz = swiz;

	 c->prog_data.param[c->prog_data.nr_params++] =
            &fp->Base.Parameters->ParameterValues[index][swiz].f;
      }
   }
}

fs_reg *
fs_visitor::emit_fragcoord_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   fs_reg wpos = *reg;
   bool flip = !ir->origin_upper_left ^ c->key.render_to_fbo;

   /* gl_FragCoord.x */
   if (ir->pixel_center_integer) {
      emit(MOV(wpos, this->pixel_x));
   } else {
      emit(ADD(wpos, this->pixel_x, fs_reg(0.5f)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.y */
   if (!flip && ir->pixel_center_integer) {
      emit(MOV(wpos, this->pixel_y));
   } else {
      fs_reg pixel_y = this->pixel_y;
      float offset = (ir->pixel_center_integer ? 0.0 : 0.5);

      if (flip) {
	 pixel_y.negate = true;
	 offset += c->key.drawable_height - 1.0;
      }

      emit(ADD(wpos, pixel_y, fs_reg(offset)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.z */
   if (intel->gen >= 6) {
      emit(MOV(wpos, fs_reg(brw_vec8_grf(c->source_depth_reg, 0))));
   } else {
      emit(FS_OPCODE_LINTERP, wpos,
           this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
           this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
           interp_reg(VARYING_SLOT_POS, 2));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.w: Already set up in emit_interpolation */
   emit(BRW_OPCODE_MOV, wpos, this->wpos_w);

   return reg;
}

fs_inst *
fs_visitor::emit_linterp(const fs_reg &attr, const fs_reg &interp,
                         glsl_interp_qualifier interpolation_mode,
                         bool is_centroid)
{
   brw_wm_barycentric_interp_mode barycoord_mode;
   if (is_centroid) {
      if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
         barycoord_mode = BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC;
      else
         barycoord_mode = BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC;
   } else {
      if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
         barycoord_mode = BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
      else
         barycoord_mode = BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC;
   }
   return emit(FS_OPCODE_LINTERP, attr,
               this->delta_x[barycoord_mode],
               this->delta_y[barycoord_mode], interp);
}

fs_reg *
fs_visitor::emit_general_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   reg->type = brw_type_for_base_type(ir->type->get_scalar_type());
   fs_reg attr = *reg;

   unsigned int array_elements;
   const glsl_type *type;

   if (ir->type->is_array()) {
      array_elements = ir->type->length;
      if (array_elements == 0) {
	 fail("dereferenced array '%s' has length 0\n", ir->name);
      }
      type = ir->type->fields.array;
   } else {
      array_elements = 1;
      type = ir->type;
   }

   glsl_interp_qualifier interpolation_mode =
      ir->determine_interpolation_mode(c->key.flat_shade);

   int location = ir->location;
   for (unsigned int i = 0; i < array_elements; i++) {
      for (unsigned int j = 0; j < type->matrix_columns; j++) {
	 if (urb_setup[location] == -1) {
	    /* If there's no incoming setup data for this slot, don't
	     * emit interpolation for it.
	     */
	    attr.reg_offset += type->vector_elements;
	    location++;
	    continue;
	 }

	 if (interpolation_mode == INTERP_QUALIFIER_FLAT) {
	    /* Constant interpolation (flat shading) case. The SF has
	     * handed us defined values in only the constant offset
	     * field of the setup reg.
	     */
	    for (unsigned int k = 0; k < type->vector_elements; k++) {
	       struct brw_reg interp = interp_reg(location, k);
	       interp = suboffset(interp, 3);
               interp.type = reg->type;
	       emit(FS_OPCODE_CINTERP, attr, fs_reg(interp));
	       attr.reg_offset++;
	    }
	 } else {
	    /* Smooth/noperspective interpolation case. */
	    for (unsigned int k = 0; k < type->vector_elements; k++) {
	       /* FINISHME: At some point we probably want to push
		* this farther by giving similar treatment to the
		* other potentially constant components of the
		* attribute, as well as making brw_vs_constval.c
		* handle varyings other than gl_TexCoord.
		*/
               struct brw_reg interp = interp_reg(location, k);
               emit_linterp(attr, fs_reg(interp), interpolation_mode,
                            ir->centroid);
               if (brw->needs_unlit_centroid_workaround && ir->centroid) {
                  /* Get the pixel/sample mask into f0 so that we know
                   * which pixels are lit.  Then, for each channel that is
                   * unlit, replace the centroid data with non-centroid
                   * data.
                   */
                  emit(FS_OPCODE_MOV_DISPATCH_TO_FLAGS, attr);
                  fs_inst *inst = emit_linterp(attr, fs_reg(interp),
                                               interpolation_mode, false);
                  inst->predicate = BRW_PREDICATE_NORMAL;
                  inst->predicate_inverse = true;
               }
               if (intel->gen < 6) {
                  emit(BRW_OPCODE_MUL, attr, attr, this->pixel_w);
               }
	       attr.reg_offset++;
	    }

	 }
	 location++;
      }
   }

   return reg;
}

fs_reg *
fs_visitor::emit_frontfacing_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);

   /* The frontfacing comes in as a bit in the thread payload. */
   if (intel->gen >= 6) {
      emit(BRW_OPCODE_ASR, *reg,
	   fs_reg(retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_D)),
	   fs_reg(15));
      emit(BRW_OPCODE_NOT, *reg, *reg);
      emit(BRW_OPCODE_AND, *reg, *reg, fs_reg(1));
   } else {
      struct brw_reg r1_6ud = retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_UD);
      /* bit 31 is "primitive is back face", so checking < (1 << 31) gives
       * us front face
       */
      emit(CMP(*reg, fs_reg(r1_6ud), fs_reg(1u << 31), BRW_CONDITIONAL_L));
      emit(BRW_OPCODE_AND, *reg, *reg, fs_reg(1u));
   }

   return reg;
}

fs_reg
fs_visitor::fix_math_operand(fs_reg src)
{
   /* Can't do hstride == 0 args on gen6 math, so expand it out. We
    * might be able to do better by doing execsize = 1 math and then
    * expanding that result out, but we would need to be careful with
    * masking.
    *
    * The hardware ignores source modifiers (negate and abs) on math
    * instructions, so we also move to a temp to set those up.
    */
   if (intel->gen == 6 && src.file != UNIFORM && src.file != IMM &&
       !src.abs && !src.negate)
      return src;

   /* Gen7 relaxes most of the above restrictions, but still can't use IMM
    * operands to math
    */
   if (intel->gen >= 7 && src.file != IMM)
      return src;

   fs_reg expanded = fs_reg(this, glsl_type::float_type);
   expanded.type = src.type;
   emit(BRW_OPCODE_MOV, expanded, src);
   return expanded;
}

fs_inst *
fs_visitor::emit_math(enum opcode opcode, fs_reg dst, fs_reg src)
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
      return NULL;
   }

   /* Can't do hstride == 0 args to gen6 math, so expand it out.  We
    * might be able to do better by doing execsize = 1 math and then
    * expanding that result out, but we would need to be careful with
    * masking.
    *
    * Gen 6 hardware ignores source modifiers (negate and abs) on math
    * instructions, so we also move to a temp to set those up.
    */
   if (intel->gen >= 6)
      src = fix_math_operand(src);

   fs_inst *inst = emit(opcode, dst, src);

   if (intel->gen < 6) {
      inst->base_mrf = 2;
      inst->mlen = dispatch_width / 8;
   }

   return inst;
}

fs_inst *
fs_visitor::emit_math(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   int base_mrf = 2;
   fs_inst *inst;

   switch (opcode) {
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      if (intel->gen >= 7 && dispatch_width == 16)
	 fail("16-wide INTDIV unsupported\n");
      break;
   case SHADER_OPCODE_POW:
      break;
   default:
      assert(!"not reached: unsupported binary math opcode.");
      return NULL;
   }

   if (intel->gen >= 6) {
      src0 = fix_math_operand(src0);
      src1 = fix_math_operand(src1);

      inst = emit(opcode, dst, src0, src1);
   } else {
      /* From the Ironlake PRM, Volume 4, Part 1, Section 6.1.13
       * "Message Payload":
       *
       * "Operand0[7].  For the INT DIV functions, this operand is the
       *  denominator."
       *  ...
       * "Operand1[7].  For the INT DIV functions, this operand is the
       *  numerator."
       */
      bool is_int_div = opcode != SHADER_OPCODE_POW;
      fs_reg &op0 = is_int_div ? src1 : src0;
      fs_reg &op1 = is_int_div ? src0 : src1;

      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + 1, op1.type), op1);
      inst = emit(opcode, dst, op0, reg_null_f);

      inst->base_mrf = base_mrf;
      inst->mlen = 2 * dispatch_width / 8;
   }
   return inst;
}

void
fs_visitor::assign_curb_setup()
{
   c->prog_data.curb_read_length = ALIGN(c->prog_data.nr_params, 8) / 8;
   if (dispatch_width == 8) {
      c->prog_data.first_curbe_grf = c->nr_payload_regs;
   } else {
      c->prog_data.first_curbe_grf_16 = c->nr_payload_regs;
   }

   /* Map the offsets in the UNIFORM file to fixed HW regs. */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == UNIFORM) {
	    int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;
	    struct brw_reg brw_reg = brw_vec1_grf(c->nr_payload_regs +
						  constant_nr / 8,
						  constant_nr % 8);

	    inst->src[i].file = FIXED_HW_REG;
	    inst->src[i].fixed_hw_reg = retype(brw_reg, inst->src[i].type);
	 }
      }
   }
}

void
fs_visitor::calculate_urb_setup()
{
   for (unsigned int i = 0; i < VARYING_SLOT_MAX; i++) {
      urb_setup[i] = -1;
   }

   int urb_next = 0;
   /* Figure out where each of the incoming setup attributes lands. */
   if (intel->gen >= 6) {
      for (unsigned int i = 0; i < VARYING_SLOT_MAX; i++) {
	 if (fp->Base.InputsRead & BITFIELD64_BIT(i)) {
	    urb_setup[i] = urb_next++;
	 }
      }
   } else {
      /* FINISHME: The sf doesn't map VS->FS inputs for us very well. */
      for (unsigned int i = 0; i < VARYING_SLOT_MAX; i++) {
         /* Point size is packed into the header, not as a general attribute */
         if (i == VARYING_SLOT_PSIZ)
            continue;

	 if (c->key.input_slots_valid & BITFIELD64_BIT(i)) {
	    /* The back color slot is skipped when the front color is
	     * also written to.  In addition, some slots can be
	     * written in the vertex shader and not read in the
	     * fragment shader.  So the register number must always be
	     * incremented, mapped or not.
	     */
	    if (_mesa_varying_slot_in_fs((gl_varying_slot) i))
	       urb_setup[i] = urb_next;
            urb_next++;
	 }
      }

      /*
       * It's a FS only attribute, and we did interpolation for this attribute
       * in SF thread. So, count it here, too.
       *
       * See compile_sf_prog() for more info.
       */
      if (fp->Base.InputsRead & BITFIELD64_BIT(VARYING_SLOT_PNTC))
         urb_setup[VARYING_SLOT_PNTC] = urb_next++;
   }

   /* Each attribute is 4 setup channels, each of which is half a reg. */
   c->prog_data.urb_read_length = urb_next * 2;
}

void
fs_visitor::assign_urb_setup()
{
   int urb_start = c->nr_payload_regs + c->prog_data.curb_read_length;

   /* Offset all the urb_setup[] index by the actual position of the
    * setup regs, now that the location of the constants has been chosen.
    */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode == FS_OPCODE_LINTERP) {
	 assert(inst->src[2].file == FIXED_HW_REG);
	 inst->src[2].fixed_hw_reg.nr += urb_start;
      }

      if (inst->opcode == FS_OPCODE_CINTERP) {
	 assert(inst->src[0].file == FIXED_HW_REG);
	 inst->src[0].fixed_hw_reg.nr += urb_start;
      }
   }

   this->first_non_payload_grf = urb_start + c->prog_data.urb_read_length;
}

/**
 * Split large virtual GRFs into separate components if we can.
 *
 * This is mostly duplicated with what brw_fs_vector_splitting does,
 * but that's really conservative because it's afraid of doing
 * splitting that doesn't result in real progress after the rest of
 * the optimization phases, which would cause infinite looping in
 * optimization.  We can do it once here, safely.  This also has the
 * opportunity to split interpolated values, or maybe even uniforms,
 * which we don't have at the IR level.
 *
 * We want to split, because virtual GRFs are what we register
 * allocate and spill (due to contiguousness requirements for some
 * instructions), and they're what we naturally generate in the
 * codegen process, but most virtual GRFs don't actually need to be
 * contiguous sets of GRFs.  If we split, we'll end up with reduced
 * live intervals and better dead code elimination and coalescing.
 */
void
fs_visitor::split_virtual_grfs()
{
   int num_vars = this->virtual_grf_count;
   bool split_grf[num_vars];
   int new_virtual_grf[num_vars];

   /* Try to split anything > 0 sized. */
   for (int i = 0; i < num_vars; i++) {
      if (this->virtual_grf_sizes[i] != 1)
	 split_grf[i] = true;
      else
	 split_grf[i] = false;
   }

   if (brw->has_pln &&
       this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].file == GRF) {
      /* PLN opcodes rely on the delta_xy being contiguous.  We only have to
       * check this for BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC, because prior to
       * Gen6, that was the only supported interpolation mode, and since Gen6,
       * delta_x and delta_y are in fixed hardware registers.
       */
      split_grf[this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].reg] =
         false;
   }

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      /* If there's a SEND message that requires contiguous destination
       * registers, no splitting is allowed.
       */
      if (inst->regs_written > 1) {
	 split_grf[inst->dst.reg] = false;
      }

      /* If we're sending from a GRF, don't split it, on the assumption that
       * the send is reading the whole thing.
       */
      if (inst->is_send_from_grf()) {
         split_grf[inst->src[0].reg] = false;
      }
   }

   /* Allocate new space for split regs.  Note that the virtual
    * numbers will be contiguous.
    */
   for (int i = 0; i < num_vars; i++) {
      if (split_grf[i]) {
	 new_virtual_grf[i] = virtual_grf_alloc(1);
	 for (int j = 2; j < this->virtual_grf_sizes[i]; j++) {
	    int reg = virtual_grf_alloc(1);
	    assert(reg == new_virtual_grf[i] + j - 1);
	    (void) reg;
	 }
	 this->virtual_grf_sizes[i] = 1;
      }
   }

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->dst.file == GRF &&
	  split_grf[inst->dst.reg] &&
	  inst->dst.reg_offset != 0) {
	 inst->dst.reg = (new_virtual_grf[inst->dst.reg] +
			  inst->dst.reg_offset - 1);
	 inst->dst.reg_offset = 0;
      }
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF &&
	     split_grf[inst->src[i].reg] &&
	     inst->src[i].reg_offset != 0) {
	    inst->src[i].reg = (new_virtual_grf[inst->src[i].reg] +
				inst->src[i].reg_offset - 1);
	    inst->src[i].reg_offset = 0;
	 }
      }
   }
   this->live_intervals_valid = false;
}

/**
 * Remove unused virtual GRFs and compact the virtual_grf_* arrays.
 *
 * During code generation, we create tons of temporary variables, many of
 * which get immediately killed and are never used again.  Yet, in later
 * optimization and analysis passes, such as compute_live_intervals, we need
 * to loop over all the virtual GRFs.  Compacting them can save a lot of
 * overhead.
 */
void
fs_visitor::compact_virtual_grfs()
{
   /* Mark which virtual GRFs are used, and count how many. */
   int remap_table[this->virtual_grf_count];
   memset(remap_table, -1, sizeof(remap_table));

   foreach_list(node, &this->instructions) {
      const fs_inst *inst = (const fs_inst *) node;

      if (inst->dst.file == GRF)
         remap_table[inst->dst.reg] = 0;

      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == GRF)
            remap_table[inst->src[i].reg] = 0;
      }
   }

   /* In addition to registers used in instructions, fs_visitor keeps
    * direct references to certain special values which must be patched:
    */
   fs_reg *special[] = {
      &frag_depth, &pixel_x, &pixel_y, &pixel_w, &wpos_w, &dual_src_output,
      &outputs[0], &outputs[1], &outputs[2], &outputs[3],
      &outputs[4], &outputs[5], &outputs[6], &outputs[7],
      &delta_x[0], &delta_x[1], &delta_x[2],
      &delta_x[3], &delta_x[4], &delta_x[5],
      &delta_y[0], &delta_y[1], &delta_y[2],
      &delta_y[3], &delta_y[4], &delta_y[5],
   };
   STATIC_ASSERT(BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT == 6);
   STATIC_ASSERT(BRW_MAX_DRAW_BUFFERS == 8);

   /* Treat all special values as used, to be conservative */
   for (unsigned i = 0; i < ARRAY_SIZE(special); i++) {
      if (special[i]->file == GRF)
	 remap_table[special[i]->reg] = 0;
   }

   /* Compact the GRF arrays. */
   int new_index = 0;
   for (int i = 0; i < this->virtual_grf_count; i++) {
      if (remap_table[i] != -1) {
         remap_table[i] = new_index;
         virtual_grf_sizes[new_index] = virtual_grf_sizes[i];
         if (live_intervals_valid) {
            virtual_grf_use[new_index] = virtual_grf_use[i];
            virtual_grf_def[new_index] = virtual_grf_def[i];
         }
         ++new_index;
      }
   }

   this->virtual_grf_count = new_index;

   /* Patch all the instructions to use the newly renumbered registers */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *) node;

      if (inst->dst.file == GRF)
         inst->dst.reg = remap_table[inst->dst.reg];

      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == GRF)
            inst->src[i].reg = remap_table[inst->src[i].reg];
      }
   }

   /* Patch all the references to special values */
   for (unsigned i = 0; i < ARRAY_SIZE(special); i++) {
      if (special[i]->file == GRF && remap_table[special[i]->reg] != -1)
	 special[i]->reg = remap_table[special[i]->reg];
   }
}

bool
fs_visitor::remove_dead_constants()
{
   if (dispatch_width == 8) {
      this->params_remap = ralloc_array(mem_ctx, int, c->prog_data.nr_params);

      for (unsigned int i = 0; i < c->prog_data.nr_params; i++)
	 this->params_remap[i] = -1;

      /* Find which params are still in use. */
      foreach_list(node, &this->instructions) {
	 fs_inst *inst = (fs_inst *)node;

	 for (int i = 0; i < 3; i++) {
	    int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;

	    if (inst->src[i].file != UNIFORM)
	       continue;

	    assert(constant_nr < (int)c->prog_data.nr_params);

	    /* For now, set this to non-negative.  We'll give it the
	     * actual new number in a moment, in order to keep the
	     * register numbers nicely ordered.
	     */
	    this->params_remap[constant_nr] = 0;
	 }
      }

      /* Figure out what the new numbers for the params will be.  At some
       * point when we're doing uniform array access, we're going to want
       * to keep the distinction between .reg and .reg_offset, but for
       * now we don't care.
       */
      unsigned int new_nr_params = 0;
      for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
	 if (this->params_remap[i] != -1) {
	    this->params_remap[i] = new_nr_params++;
	 }
      }

      /* Update the list of params to be uploaded to match our new numbering. */
      for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
	 int remapped = this->params_remap[i];

	 if (remapped == -1)
	    continue;

	 c->prog_data.param[remapped] = c->prog_data.param[i];
      }

      c->prog_data.nr_params = new_nr_params;
   } else {
      /* This should have been generated in the 8-wide pass already. */
      assert(this->params_remap);
   }

   /* Now do the renumbering of the shader to remove unused params. */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (int i = 0; i < 3; i++) {
	 int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;

	 if (inst->src[i].file != UNIFORM)
	    continue;

	 assert(this->params_remap[constant_nr] != -1);
	 inst->src[i].reg = this->params_remap[constant_nr];
	 inst->src[i].reg_offset = 0;
      }
   }

   return true;
}

/*
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
fs_visitor::move_uniform_array_access_to_pull_constants()
{
   int pull_constant_loc[c->prog_data.nr_params];

   for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
      pull_constant_loc[i] = -1;
   }

   /* Walk through and find array access of uniforms.  Put a copy of that
    * uniform in the pull constant buffer.
    *
    * Note that we don't move constant-indexed accesses to arrays.  No
    * testing has been done of the performance impact of this choice.
    */
   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (int i = 0 ; i < 3; i++) {
         if (inst->src[i].file != UNIFORM || !inst->src[i].reladdr)
            continue;

         int uniform = inst->src[i].reg;

         /* If this array isn't already present in the pull constant buffer,
          * add it.
          */
         if (pull_constant_loc[uniform] == -1) {
            const float **values = &c->prog_data.param[uniform];

            pull_constant_loc[uniform] = c->prog_data.nr_pull_params;

            assert(param_size[uniform]);

            for (int j = 0; j < param_size[uniform]; j++) {
               c->prog_data.pull_param[c->prog_data.nr_pull_params++] =
                  values[j];
            }
         }

         /* Set up the annotation tracking for new generated instructions. */
         base_ir = inst->ir;
         current_annotation = inst->annotation;

         fs_reg surf_index = fs_reg((unsigned)SURF_INDEX_FRAG_CONST_BUFFER);
         fs_reg temp = fs_reg(this, glsl_type::float_type);
         exec_list list = VARYING_PULL_CONSTANT_LOAD(temp,
                                                     surf_index,
                                                     *inst->src[i].reladdr,
                                                     pull_constant_loc[uniform] +
                                                     inst->src[i].reg_offset);
         inst->insert_before(&list);

         inst->src[i].file = temp.file;
         inst->src[i].reg = temp.reg;
         inst->src[i].reg_offset = temp.reg_offset;
         inst->src[i].reladdr = NULL;
      }
   }
}

/**
 * Choose accesses from the UNIFORM file to demote to using the pull
 * constant buffer.
 *
 * We allow a fragment shader to have more than the specified minimum
 * maximum number of fragment shader uniform components (64).  If
 * there are too many of these, they'd fill up all of register space.
 * So, this will push some of them out to the pull constant buffer and
 * update the program to load them.
 */
void
fs_visitor::setup_pull_constants()
{
   /* Only allow 16 registers (128 uniform components) as push constants. */
   unsigned int max_uniform_components = 16 * 8;
   if (c->prog_data.nr_params <= max_uniform_components)
      return;

   if (dispatch_width == 16) {
      fail("Pull constants not supported in 16-wide\n");
      return;
   }

   /* Just demote the end of the list.  We could probably do better
    * here, demoting things that are rarely used in the program first.
    */
   unsigned int pull_uniform_base = max_uniform_components;

   int pull_constant_loc[c->prog_data.nr_params];
   for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
      if (i < pull_uniform_base) {
         pull_constant_loc[i] = -1;
      } else {
         pull_constant_loc[i] = -1;
         /* If our constant is already being uploaded for reladdr purposes,
          * reuse it.
          */
         for (unsigned int j = 0; j < c->prog_data.nr_pull_params; j++) {
            if (c->prog_data.pull_param[j] == c->prog_data.param[i]) {
               pull_constant_loc[i] = j;
               break;
            }
         }
         if (pull_constant_loc[i] == -1) {
            int pull_index = c->prog_data.nr_pull_params++;
            c->prog_data.pull_param[pull_index] = c->prog_data.param[i];
            pull_constant_loc[i] = pull_index;;
         }
      }
   }
   c->prog_data.nr_params = pull_uniform_base;

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM)
	    continue;

         int pull_index = pull_constant_loc[inst->src[i].reg +
                                            inst->src[i].reg_offset];
         if (pull_index == -1)
	    continue;

         assert(!inst->src[i].reladdr);

	 fs_reg dst = fs_reg(this, glsl_type::float_type);
	 fs_reg index = fs_reg((unsigned)SURF_INDEX_FRAG_CONST_BUFFER);
	 fs_reg offset = fs_reg((unsigned)(pull_index * 4) & ~15);
	 fs_inst *pull =
            new(mem_ctx) fs_inst(FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD,
                                 dst, index, offset);
	 pull->ir = inst->ir;
	 pull->annotation = inst->annotation;

	 inst->insert_before(pull);

	 inst->src[i].file = GRF;
	 inst->src[i].reg = dst.reg;
	 inst->src[i].reg_offset = 0;
	 inst->src[i].smear = pull_index & 3;
      }
   }
}

bool
fs_visitor::opt_algebraic()
{
   bool progress = false;

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      switch (inst->opcode) {
      case BRW_OPCODE_MUL:
	 if (inst->src[1].file != IMM)
	    continue;

	 /* a * 1.0 = a */
	 if (inst->src[1].is_one()) {
	    inst->opcode = BRW_OPCODE_MOV;
	    inst->src[1] = reg_undef;
	    progress = true;
	    break;
	 }

         /* a * 0.0 = 0.0 */
         if (inst->src[1].is_zero()) {
            inst->opcode = BRW_OPCODE_MOV;
            inst->src[0] = inst->src[1];
            inst->src[1] = reg_undef;
            progress = true;
            break;
         }

	 break;
      case BRW_OPCODE_ADD:
         if (inst->src[1].file != IMM)
            continue;

         /* a + 0.0 = a */
         if (inst->src[1].is_zero()) {
            inst->opcode = BRW_OPCODE_MOV;
            inst->src[1] = reg_undef;
            progress = true;
            break;
         }
         break;
      default:
	 break;
      }
   }

   return progress;
}

/**
 * Must be called after calculate_live_intervales() to remove unused
 * writes to registers -- register allocation will fail otherwise
 * because something deffed but not used won't be considered to
 * interfere with other regs.
 */
bool
fs_visitor::dead_code_eliminate()
{
   bool progress = false;
   int pc = 0;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->dst.file == GRF && this->virtual_grf_use[inst->dst.reg] <= pc) {
	 inst->remove();
	 progress = true;
      }

      pc++;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

/**
 * Implements a second type of register coalescing: This one checks if
 * the two regs involved in a raw move don't interfere, in which case
 * they can both by stored in the same place and the MOV removed.
 */
bool
fs_visitor::register_coalesce_2()
{
   bool progress = false;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicate ||
	  inst->saturate ||
	  inst->src[0].file != GRF ||
	  inst->src[0].negate ||
	  inst->src[0].abs ||
	  inst->src[0].smear != -1 ||
	  inst->dst.file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  virtual_grf_sizes[inst->src[0].reg] != 1 ||
	  virtual_grf_interferes(inst->dst.reg, inst->src[0].reg)) {
	 continue;
      }

      int reg_from = inst->src[0].reg;
      assert(inst->src[0].reg_offset == 0);
      int reg_to = inst->dst.reg;
      int reg_to_offset = inst->dst.reg_offset;

      foreach_list(node, &this->instructions) {
	 fs_inst *scan_inst = (fs_inst *)node;

	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == reg_from) {
	    scan_inst->dst.reg = reg_to;
	    scan_inst->dst.reg_offset = reg_to_offset;
	 }
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == reg_from) {
	       scan_inst->src[i].reg = reg_to;
	       scan_inst->src[i].reg_offset = reg_to_offset;
	    }
	 }
      }

      inst->remove();

      /* We don't need to recalculate live intervals inside the loop despite
       * flagging live_intervals_valid because we only use live intervals for
       * the interferes test, and we must have had a situation where the
       * intervals were:
       *
       *  from  to
       *  ^
       *  |
       *  v
       *        ^
       *        |
       *        v
       *
       * Some register R that might get coalesced with one of these two could
       * only be referencing "to", otherwise "from"'s range would have been
       * longer.  R's range could also only start at the end of "to" or later,
       * otherwise it will conflict with "to" when we try to coalesce "to"
       * into Rw anyway.
       */
      live_intervals_valid = false;

      progress = true;
      continue;
   }

   return progress;
}

bool
fs_visitor::register_coalesce()
{
   bool progress = false;
   int if_depth = 0;
   int loop_depth = 0;

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      /* Make sure that we dominate the instructions we're going to
       * scan for interfering with our coalescing, or we won't have
       * scanned enough to see if anything interferes with our
       * coalescing.  We don't dominate the following instructions if
       * we're in a loop or an if block.
       */
      switch (inst->opcode) {
      case BRW_OPCODE_DO:
	 loop_depth++;
	 break;
      case BRW_OPCODE_WHILE:
	 loop_depth--;
	 break;
      case BRW_OPCODE_IF:
	 if_depth++;
	 break;
      case BRW_OPCODE_ENDIF:
	 if_depth--;
	 break;
      default:
	 break;
      }
      if (loop_depth || if_depth)
	 continue;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicate ||
	  inst->saturate ||
	  inst->dst.file != GRF || (inst->src[0].file != GRF &&
				    inst->src[0].file != UNIFORM)||
	  inst->dst.type != inst->src[0].type)
	 continue;

      bool has_source_modifiers = (inst->src[0].abs ||
                                   inst->src[0].negate ||
                                   inst->src[0].smear != -1 ||
                                   inst->src[0].file == UNIFORM);

      /* Found a move of a GRF to a GRF.  Let's see if we can coalesce
       * them: check for no writes to either one until the exit of the
       * program.
       */
      bool interfered = false;

      for (fs_inst *scan_inst = (fs_inst *)inst->next;
	   !scan_inst->is_tail_sentinel();
	   scan_inst = (fs_inst *)scan_inst->next) {
	 if (scan_inst->dst.file == GRF) {
	    if (scan_inst->overwrites_reg(inst->dst) ||
                scan_inst->overwrites_reg(inst->src[0])) {
	       interfered = true;
	       break;
	    }
	 }

	 /* The gen6 MATH instruction can't handle source modifiers or
	  * unusual register regions, so avoid coalescing those for
	  * now.  We should do something more specific.
	  */
	 if (has_source_modifiers && !can_do_source_mods(scan_inst)) {
            interfered = true;
	    break;
	 }

	 /* The accumulator result appears to get used for the
	  * conditional modifier generation.  When negating a UD
	  * value, there is a 33rd bit generated for the sign in the
	  * accumulator value, so now you can't check, for example,
	  * equality with a 32-bit value.  See piglit fs-op-neg-uint.
	  */
	 if (scan_inst->conditional_mod &&
	     inst->src[0].negate &&
	     inst->src[0].type == BRW_REGISTER_TYPE_UD) {
	    interfered = true;
	    break;
	 }
      }
      if (interfered) {
	 continue;
      }

      /* Rewrite the later usage to point at the source of the move to
       * be removed.
       */
      for (fs_inst *scan_inst = inst;
	   !scan_inst->is_tail_sentinel();
	   scan_inst = (fs_inst *)scan_inst->next) {
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->dst.reg &&
		scan_inst->src[i].reg_offset == inst->dst.reg_offset) {
	       fs_reg new_src = inst->src[0];
               if (scan_inst->src[i].abs) {
                  new_src.negate = 0;
                  new_src.abs = 1;
               }
	       new_src.negate ^= scan_inst->src[i].negate;
	       scan_inst->src[i] = new_src;
	    }
	 }
      }

      inst->remove();
      progress = true;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}


bool
fs_visitor::compute_to_mrf()
{
   bool progress = false;
   int next_ip = 0;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      int ip = next_ip;
      next_ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicate ||
	  inst->dst.file != MRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  inst->src[0].abs || inst->src[0].negate || inst->src[0].smear != -1)
	 continue;

      /* Work out which hardware MRF registers are written by this
       * instruction.
       */
      int mrf_low = inst->dst.reg & ~BRW_MRF_COMPR4;
      int mrf_high;
      if (inst->dst.reg & BRW_MRF_COMPR4) {
	 mrf_high = mrf_low + 4;
      } else if (dispatch_width == 16 &&
		 (!inst->force_uncompressed && !inst->force_sechalf)) {
	 mrf_high = mrf_low + 1;
      } else {
	 mrf_high = mrf_low;
      }

      /* Can't compute-to-MRF this GRF if someone else was going to
       * read it later.
       */
      if (this->virtual_grf_use[inst->src[0].reg] > ip)
	 continue;

      /* Found a move of a GRF to a MRF.  Let's see if we can go
       * rewrite the thing that made this GRF to write into the MRF.
       */
      fs_inst *scan_inst;
      for (scan_inst = (fs_inst *)inst->prev;
	   scan_inst->prev != NULL;
	   scan_inst = (fs_inst *)scan_inst->prev) {
	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->src[0].reg) {
	    /* Found the last thing to write our reg we want to turn
	     * into a compute-to-MRF.
	     */

	    /* If it's predicated, it (probably) didn't populate all
	     * the channels.  We might be able to rewrite everything
	     * that writes that reg, but it would require smarter
	     * tracking to delay the rewriting until complete success.
	     */
	    if (scan_inst->predicate)
	       break;

	    /* If it's half of register setup and not the same half as
	     * our MOV we're trying to remove, bail for now.
	     */
	    if (scan_inst->force_uncompressed != inst->force_uncompressed ||
		scan_inst->force_sechalf != inst->force_sechalf) {
	       break;
	    }

            /* Things returning more than one register would need us to
             * understand coalescing out more than one MOV at a time.
             */
            if (scan_inst->regs_written > 1)
               break;

	    /* SEND instructions can't have MRF as a destination. */
	    if (scan_inst->mlen)
	       break;

	    if (intel->gen == 6) {
	       /* gen6 math instructions must have the destination be
		* GRF, so no compute-to-MRF for them.
		*/
	       if (scan_inst->is_math()) {
		  break;
	       }
	    }

	    if (scan_inst->dst.reg_offset == inst->src[0].reg_offset) {
	       /* Found the creator of our MRF's source value. */
	       scan_inst->dst.file = MRF;
	       scan_inst->dst.reg = inst->dst.reg;
	       scan_inst->saturate |= inst->saturate;
	       inst->remove();
	       progress = true;
	    }
	    break;
	 }

	 /* We don't handle control flow here.  Most computation of
	  * values that end up in MRFs are shortly before the MRF
	  * write anyway.
	  */
	 if (scan_inst->is_control_flow() && scan_inst->opcode != BRW_OPCODE_IF)
	    break;

	 /* You can't read from an MRF, so if someone else reads our
	  * MRF's source GRF that we wanted to rewrite, that stops us.
	  */
	 bool interfered = false;
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->src[0].reg &&
		scan_inst->src[i].reg_offset == inst->src[0].reg_offset) {
	       interfered = true;
	    }
	 }
	 if (interfered)
	    break;

	 if (scan_inst->dst.file == MRF) {
	    /* If somebody else writes our MRF here, we can't
	     * compute-to-MRF before that.
	     */
	    int scan_mrf_low = scan_inst->dst.reg & ~BRW_MRF_COMPR4;
	    int scan_mrf_high;

	    if (scan_inst->dst.reg & BRW_MRF_COMPR4) {
	       scan_mrf_high = scan_mrf_low + 4;
	    } else if (dispatch_width == 16 &&
		       (!scan_inst->force_uncompressed &&
			!scan_inst->force_sechalf)) {
	       scan_mrf_high = scan_mrf_low + 1;
	    } else {
	       scan_mrf_high = scan_mrf_low;
	    }

	    if (mrf_low == scan_mrf_low ||
		mrf_low == scan_mrf_high ||
		mrf_high == scan_mrf_low ||
		mrf_high == scan_mrf_high) {
	       break;
	    }
	 }

	 if (scan_inst->mlen > 0) {
	    /* Found a SEND instruction, which means that there are
	     * live values in MRFs from base_mrf to base_mrf +
	     * scan_inst->mlen - 1.  Don't go pushing our MRF write up
	     * above it.
	     */
	    if (mrf_low >= scan_inst->base_mrf &&
		mrf_low < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	    if (mrf_high >= scan_inst->base_mrf &&
		mrf_high < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	 }
      }
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

/**
 * Walks through basic blocks, looking for repeated MRF writes and
 * removing the later ones.
 */
bool
fs_visitor::remove_duplicate_mrf_writes()
{
   fs_inst *last_mrf_move[16];
   bool progress = false;

   /* Need to update the MRF tracking for compressed instructions. */
   if (dispatch_width == 16)
      return false;

   memset(last_mrf_move, 0, sizeof(last_mrf_move));

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->is_control_flow()) {
	 memset(last_mrf_move, 0, sizeof(last_mrf_move));
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF) {
	 fs_inst *prev_inst = last_mrf_move[inst->dst.reg];
	 if (prev_inst && inst->equals(prev_inst)) {
	    inst->remove();
	    progress = true;
	    continue;
	 }
      }

      /* Clear out the last-write records for MRFs that were overwritten. */
      if (inst->dst.file == MRF) {
	 last_mrf_move[inst->dst.reg] = NULL;
      }

      if (inst->mlen > 0) {
	 /* Found a SEND instruction, which will include two or fewer
	  * implied MRF writes.  We could do better here.
	  */
	 for (int i = 0; i < implied_mrf_writes(inst); i++) {
	    last_mrf_move[inst->base_mrf + i] = NULL;
	 }
      }

      /* Clear out any MRF move records whose sources got overwritten. */
      if (inst->dst.file == GRF) {
	 for (unsigned int i = 0; i < Elements(last_mrf_move); i++) {
	    if (last_mrf_move[i] &&
		last_mrf_move[i]->src[0].reg == inst->dst.reg) {
	       last_mrf_move[i] = NULL;
	    }
	 }
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF &&
	  inst->src[0].file == GRF &&
	  !inst->predicate) {
	 last_mrf_move[inst->dst.reg] = inst;
      }
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

static void
clear_deps_for_inst_src(fs_inst *inst, int dispatch_width, bool *deps,
                        int first_grf, int grf_len)
{
   bool inst_16wide = (dispatch_width > 8 &&
                       !inst->force_uncompressed &&
                       !inst->force_sechalf);

   /* Clear the flag for registers that actually got read (as expected). */
   for (int i = 0; i < 3; i++) {
      int grf;
      if (inst->src[i].file == GRF) {
         grf = inst->src[i].reg;
      } else if (inst->src[i].file == FIXED_HW_REG &&
                 inst->src[i].fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
         grf = inst->src[i].fixed_hw_reg.nr;
      } else {
         continue;
      }

      if (grf >= first_grf &&
          grf < first_grf + grf_len) {
         deps[grf - first_grf] = false;
         if (inst_16wide)
            deps[grf - first_grf + 1] = false;
      }
   }
}

/**
 * Implements this workaround for the original 965:
 *
 *     "[DevBW, DevCL] Implementation Restrictions: As the hardware does not
 *      check for post destination dependencies on this instruction, software
 *      must ensure that there is no destination hazard for the case of ‘write
 *      followed by a posted write’ shown in the following example.
 *
 *      1. mov r3 0
 *      2. send r3.xy <rest of send instruction>
 *      3. mov r2 r3
 *
 *      Due to no post-destination dependency check on the ‘send’, the above
 *      code sequence could have two instructions (1 and 2) in flight at the
 *      same time that both consider ‘r3’ as the target of their final writes.
 */
void
fs_visitor::insert_gen4_pre_send_dependency_workarounds(fs_inst *inst)
{
   int reg_size = dispatch_width / 8;
   int write_len = inst->regs_written * reg_size;
   int first_write_grf = inst->dst.reg;
   bool needs_dep[BRW_MAX_MRF];
   assert(write_len < (int)sizeof(needs_dep) - 1);

   memset(needs_dep, false, sizeof(needs_dep));
   memset(needs_dep, true, write_len);

   clear_deps_for_inst_src(inst, dispatch_width,
                           needs_dep, first_write_grf, write_len);

   /* Walk backwards looking for writes to registers we're writing which
    * aren't read since being written.  If we hit the start of the program,
    * we assume that there are no outstanding dependencies on entry to the
    * program.
    */
   for (fs_inst *scan_inst = (fs_inst *)inst->prev;
        scan_inst != NULL;
        scan_inst = (fs_inst *)scan_inst->prev) {

      /* If we hit control flow, assume that there *are* outstanding
       * dependencies, and force their cleanup before our instruction.
       */
      if (scan_inst->is_control_flow()) {
         for (int i = 0; i < write_len; i++) {
            if (needs_dep[i]) {
               inst->insert_before(DEP_RESOLVE_MOV(first_write_grf + i));
            }
         }
         return;
      }

      bool scan_inst_16wide = (dispatch_width > 8 &&
                               !scan_inst->force_uncompressed &&
                               !scan_inst->force_sechalf);

      /* We insert our reads as late as possible on the assumption that any
       * instruction but a MOV that might have left us an outstanding
       * dependency has more latency than a MOV.
       */
      if (scan_inst->dst.file == GRF) {
         for (int i = 0; i < scan_inst->regs_written; i++) {
            int reg = scan_inst->dst.reg + i * reg_size;

            if (reg >= first_write_grf &&
                reg < first_write_grf + write_len &&
                needs_dep[reg - first_write_grf]) {
               inst->insert_before(DEP_RESOLVE_MOV(reg));
               needs_dep[reg - first_write_grf] = false;
               if (scan_inst_16wide)
                  needs_dep[reg - first_write_grf + 1] = false;
            }
         }
      }

      /* Clear the flag for registers that actually got read (as expected). */
      clear_deps_for_inst_src(scan_inst, dispatch_width,
                              needs_dep, first_write_grf, write_len);

      /* Continue the loop only if we haven't resolved all the dependencies */
      int i;
      for (i = 0; i < write_len; i++) {
         if (needs_dep[i])
            break;
      }
      if (i == write_len)
         return;
   }
}

/**
 * Implements this workaround for the original 965:
 *
 *     "[DevBW, DevCL] Errata: A destination register from a send can not be
 *      used as a destination register until after it has been sourced by an
 *      instruction with a different destination register.
 */
void
fs_visitor::insert_gen4_post_send_dependency_workarounds(fs_inst *inst)
{
   int write_len = inst->regs_written * dispatch_width / 8;
   int first_write_grf = inst->dst.reg;
   bool needs_dep[BRW_MAX_MRF];
   assert(write_len < (int)sizeof(needs_dep) - 1);

   memset(needs_dep, false, sizeof(needs_dep));
   memset(needs_dep, true, write_len);
   /* Walk forwards looking for writes to registers we're writing which aren't
    * read before being written.
    */
   for (fs_inst *scan_inst = (fs_inst *)inst->next;
        !scan_inst->is_tail_sentinel();
        scan_inst = (fs_inst *)scan_inst->next) {
      /* If we hit control flow, force resolve all remaining dependencies. */
      if (scan_inst->is_control_flow()) {
         for (int i = 0; i < write_len; i++) {
            if (needs_dep[i])
               scan_inst->insert_before(DEP_RESOLVE_MOV(first_write_grf + i));
         }
         return;
      }

      /* Clear the flag for registers that actually got read (as expected). */
      clear_deps_for_inst_src(scan_inst, dispatch_width,
                              needs_dep, first_write_grf, write_len);

      /* We insert our reads as late as possible since they're reading the
       * result of a SEND, which has massive latency.
       */
      if (scan_inst->dst.file == GRF &&
          scan_inst->dst.reg >= first_write_grf &&
          scan_inst->dst.reg < first_write_grf + write_len &&
          needs_dep[scan_inst->dst.reg - first_write_grf]) {
         scan_inst->insert_before(DEP_RESOLVE_MOV(scan_inst->dst.reg));
         needs_dep[scan_inst->dst.reg - first_write_grf] = false;
      }

      /* Continue the loop only if we haven't resolved all the dependencies */
      int i;
      for (i = 0; i < write_len; i++) {
         if (needs_dep[i])
            break;
      }
      if (i == write_len)
         return;
   }

   /* If we hit the end of the program, resolve all remaining dependencies out
    * of paranoia.
    */
   fs_inst *last_inst = (fs_inst *)this->instructions.get_tail();
   assert(last_inst->eot);
   for (int i = 0; i < write_len; i++) {
      if (needs_dep[i])
         last_inst->insert_before(DEP_RESOLVE_MOV(first_write_grf + i));
   }
}

void
fs_visitor::insert_gen4_send_dependency_workarounds()
{
   if (intel->gen != 4 || intel->is_g4x)
      return;

   /* Note that we're done with register allocation, so GRF fs_regs always
    * have a .reg_offset of 0.
    */

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->mlen != 0 && inst->dst.file == GRF) {
         insert_gen4_pre_send_dependency_workarounds(inst);
         insert_gen4_post_send_dependency_workarounds(inst);
      }
   }
}

/**
 * Turns the generic expression-style uniform pull constant load instruction
 * into a hardware-specific series of instructions for loading a pull
 * constant.
 *
 * The expression style allows the CSE pass before this to optimize out
 * repeated loads from the same offset, and gives the pre-register-allocation
 * scheduling full flexibility, while the conversion to native instructions
 * allows the post-register-allocation scheduler the best information
 * possible.
 *
 * Note that execution masking for setting up pull constant loads is special:
 * the channels that need to be written are unrelated to the current execution
 * mask, since a later instruction will use one of the result channels as a
 * source operand for all 8 or 16 of its channels.
 */
void
fs_visitor::lower_uniform_pull_constant_loads()
{
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode != FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD)
         continue;

      if (intel->gen >= 7) {
         /* The offset arg before was a vec4-aligned byte offset.  We need to
          * turn it into a dword offset.
          */
         fs_reg const_offset_reg = inst->src[1];
         assert(const_offset_reg.file == IMM &&
                const_offset_reg.type == BRW_REGISTER_TYPE_UD);
         const_offset_reg.imm.u /= 4;
         fs_reg payload = fs_reg(this, glsl_type::uint_type);

         /* This is actually going to be a MOV, but since only the first dword
          * is accessed, we have a special opcode to do just that one.  Note
          * that this needs to be an operation that will be considered a def
          * by live variable analysis, or register allocation will explode.
          */
         fs_inst *setup = new(mem_ctx) fs_inst(FS_OPCODE_SET_SIMD4X2_OFFSET,
                                               payload, const_offset_reg);
         setup->force_writemask_all = true;

         setup->ir = inst->ir;
         setup->annotation = inst->annotation;
         inst->insert_before(setup);

         /* Similarly, this will only populate the first 4 channels of the
          * result register (since we only use smear values from 0-3), but we
          * don't tell the optimizer.
          */
         inst->opcode = FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD_GEN7;
         inst->src[1] = payload;

         this->live_intervals_valid = false;
      } else {
         /* Before register allocation, we didn't tell the scheduler about the
          * MRF we use.  We know it's safe to use this MRF because nothing
          * else does except for register spill/unspill, which generates and
          * uses its MRF within a single IR instruction.
          */
         inst->base_mrf = 14;
         inst->mlen = 1;
      }
   }
}

void
fs_visitor::dump_instruction(fs_inst *inst)
{
   if (inst->predicate) {
      printf("(%cf0.%d) ",
             inst->predicate_inverse ? '-' : '+',
             inst->flag_subreg);
   }

   printf("%s", brw_instruction_name(inst->opcode));
   if (inst->saturate)
      printf(".sat");
   if (inst->conditional_mod) {
      printf(".cmod");
      if (!inst->predicate &&
          (intel->gen < 5 || (inst->opcode != BRW_OPCODE_SEL &&
                              inst->opcode != BRW_OPCODE_IF &&
                              inst->opcode != BRW_OPCODE_WHILE))) {
         printf(".f0.%d\n", inst->flag_subreg);
      }
   }
   printf(" ");


   switch (inst->dst.file) {
   case GRF:
      printf("vgrf%d", inst->dst.reg);
      if (inst->dst.reg_offset)
         printf("+%d", inst->dst.reg_offset);
      break;
   case MRF:
      printf("m%d", inst->dst.reg);
      break;
   case BAD_FILE:
      printf("(null)");
      break;
   case UNIFORM:
      printf("***u%d***", inst->dst.reg);
      break;
   default:
      printf("???");
      break;
   }
   printf(", ");

   for (int i = 0; i < 3; i++) {
      if (inst->src[i].negate)
         printf("-");
      if (inst->src[i].abs)
         printf("|");
      switch (inst->src[i].file) {
      case GRF:
         printf("vgrf%d", inst->src[i].reg);
         if (inst->src[i].reg_offset)
            printf("+%d", inst->src[i].reg_offset);
         break;
      case MRF:
         printf("***m%d***", inst->src[i].reg);
         break;
      case UNIFORM:
         printf("u%d", inst->src[i].reg);
         if (inst->src[i].reg_offset)
            printf(".%d", inst->src[i].reg_offset);
         break;
      case BAD_FILE:
         printf("(null)");
         break;
      case IMM:
         switch (inst->src[i].type) {
         case BRW_REGISTER_TYPE_F:
            printf("%ff", inst->src[i].imm.f);
            break;
         case BRW_REGISTER_TYPE_D:
            printf("%dd", inst->src[i].imm.i);
            break;
         case BRW_REGISTER_TYPE_UD:
            printf("%uu", inst->src[i].imm.u);
            break;
         default:
            printf("???");
            break;
         }
         break;
      default:
         printf("???");
         break;
      }
      if (inst->src[i].abs)
         printf("|");

      if (i < 3)
         printf(", ");
   }

   printf(" ");

   if (inst->force_uncompressed)
      printf("1sthalf ");

   if (inst->force_sechalf)
      printf("2ndhalf ");

   printf("\n");
}

void
fs_visitor::dump_instructions()
{
   int ip = 0;
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;
      printf("%d: ", ip++);
      dump_instruction(inst);
   }
}

/**
 * Possibly returns an instruction that set up @param reg.
 *
 * Sometimes we want to take the result of some expression/variable
 * dereference tree and rewrite the instruction generating the result
 * of the tree.  When processing the tree, we know that the
 * instructions generated are all writing temporaries that are dead
 * outside of this tree.  So, if we have some instructions that write
 * a temporary, we're free to point that temp write somewhere else.
 *
 * Note that this doesn't guarantee that the instruction generated
 * only reg -- it might be the size=4 destination of a texture instruction.
 */
fs_inst *
fs_visitor::get_instruction_generating_reg(fs_inst *start,
					   fs_inst *end,
					   fs_reg reg)
{
   if (end == start ||
       end->predicate ||
       end->force_uncompressed ||
       end->force_sechalf ||
       reg.reladdr ||
       !reg.equals(end->dst)) {
      return NULL;
   } else {
      return end;
   }
}

void
fs_visitor::setup_payload_gen6()
{
   struct intel_context *intel = &brw->intel;
   bool uses_depth =
      (fp->Base.InputsRead & (1 << VARYING_SLOT_POS)) != 0;
   unsigned barycentric_interp_modes = c->prog_data.barycentric_interp_modes;

   assert(intel->gen >= 6);

   /* R0-1: masks, pixel X/Y coordinates. */
   c->nr_payload_regs = 2;
   /* R2: only for 32-pixel dispatch.*/

   /* R3-26: barycentric interpolation coordinates.  These appear in the
    * same order that they appear in the brw_wm_barycentric_interp_mode
    * enum.  Each set of coordinates occupies 2 registers if dispatch width
    * == 8 and 4 registers if dispatch width == 16.  Coordinates only
    * appear if they were enabled using the "Barycentric Interpolation
    * Mode" bits in WM_STATE.
    */
   for (int i = 0; i < BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT; ++i) {
      if (barycentric_interp_modes & (1 << i)) {
         c->barycentric_coord_reg[i] = c->nr_payload_regs;
         c->nr_payload_regs += 2;
         if (dispatch_width == 16) {
            c->nr_payload_regs += 2;
         }
      }
   }

   /* R27: interpolated depth if uses source depth */
   if (uses_depth) {
      c->source_depth_reg = c->nr_payload_regs;
      c->nr_payload_regs++;
      if (dispatch_width == 16) {
         /* R28: interpolated depth if not 8-wide. */
         c->nr_payload_regs++;
      }
   }
   /* R29: interpolated W set if GEN6_WM_USES_SOURCE_W. */
   if (uses_depth) {
      c->source_w_reg = c->nr_payload_regs;
      c->nr_payload_regs++;
      if (dispatch_width == 16) {
         /* R30: interpolated W if not 8-wide. */
         c->nr_payload_regs++;
      }
   }
   /* R31: MSAA position offsets. */
   /* R32-: bary for 32-pixel. */
   /* R58-59: interp W for 32-pixel. */

   if (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      c->source_depth_to_render_target = true;
   }
}

bool
fs_visitor::run()
{
   sanity_param_count = fp->Base.Parameters->NumParameters;
   uint32_t orig_nr_params = c->prog_data.nr_params;

   if (intel->gen >= 6)
      setup_payload_gen6();
   else
      setup_payload_gen4();

   if (0) {
      emit_dummy_fs();
   } else {
      if (INTEL_DEBUG & DEBUG_SHADER_TIME)
         emit_shader_time_begin();

      calculate_urb_setup();
      if (intel->gen < 6)
	 emit_interpolation_setup_gen4();
      else
	 emit_interpolation_setup_gen6();

      /* We handle discards by keeping track of the still-live pixels in f0.1.
       * Initialize it with the dispatched pixels.
       */
      if (fp->UsesKill) {
         fs_inst *discard_init = emit(FS_OPCODE_MOV_DISPATCH_TO_FLAGS);
         discard_init->flag_subreg = 1;
      }

      /* Generate FS IR for main().  (the visitor only descends into
       * functions called "main").
       */
      if (shader) {
         foreach_list(node, &*shader->ir) {
            ir_instruction *ir = (ir_instruction *)node;
            base_ir = ir;
            this->result = reg_undef;
            ir->accept(this);
         }
      } else {
         emit_fragment_program_code();
      }
      base_ir = NULL;
      if (failed)
	 return false;

      emit(FS_OPCODE_PLACEHOLDER_HALT);

      emit_fb_writes();

      split_virtual_grfs();

      move_uniform_array_access_to_pull_constants();
      setup_pull_constants();

      bool progress;
      do {
	 progress = false;

         compact_virtual_grfs();

	 progress = remove_duplicate_mrf_writes() || progress;

	 progress = opt_algebraic() || progress;
	 progress = opt_cse() || progress;
	 progress = opt_copy_propagate() || progress;
	 progress = dead_code_eliminate() || progress;
	 progress = register_coalesce() || progress;
	 progress = register_coalesce_2() || progress;
	 progress = compute_to_mrf() || progress;
      } while (progress);

      remove_dead_constants();

      schedule_instructions(false);

      lower_uniform_pull_constant_loads();

      assign_curb_setup();
      assign_urb_setup();

      if (0) {
	 /* Debug of register spilling: Go spill everything. */
	 for (int i = 0; i < virtual_grf_count; i++) {
	    spill_reg(i);
	 }
      }

      if (0)
	 assign_regs_trivial();
      else {
	 while (!assign_regs()) {
	    if (failed)
	       break;
	 }
      }
   }
   assert(force_uncompressed_stack == 0);
   assert(force_sechalf_stack == 0);

   /* This must come after all optimization and register allocation, since
    * it inserts dead code that happens to have side effects, and it does
    * so based on the actual physical registers in use.
    */
   insert_gen4_send_dependency_workarounds();

   if (failed)
      return false;

   schedule_instructions(true);

   if (dispatch_width == 8) {
      c->prog_data.reg_blocks = brw_register_blocks(grf_used);
   } else {
      c->prog_data.reg_blocks_16 = brw_register_blocks(grf_used);

      /* Make sure we didn't try to sneak in an extra uniform */
      assert(orig_nr_params == c->prog_data.nr_params);
      (void) orig_nr_params;
   }

   /* If any state parameters were appended, then ParameterValues could have
    * been realloced, in which case the driver uniform storage set up by
    * _mesa_associate_uniform_storage() would point to freed memory.  Make
    * sure that didn't happen.
    */
   assert(sanity_param_count == fp->Base.Parameters->NumParameters);

   return !failed;
}

const unsigned *
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c,
               struct gl_fragment_program *fp,
               struct gl_shader_program *prog,
               unsigned *final_assembly_size)
{
   struct intel_context *intel = &brw->intel;
   bool start_busy = false;
   float start_time = 0;

   if (unlikely(intel->perf_debug)) {
      start_busy = (intel->batch.last_bo &&
                    drm_intel_bo_busy(intel->batch.last_bo));
      start_time = get_time();
   }

   struct brw_shader *shader = NULL;
   if (prog)
      shader = (brw_shader *) prog->_LinkedShaders[MESA_SHADER_FRAGMENT];

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      if (shader) {
         printf("GLSL IR for native fragment shader %d:\n", prog->Name);
         _mesa_print_ir(shader->ir, NULL);
         printf("\n\n");
      } else {
         printf("ARB_fragment_program %d ir for native fragment shader\n",
                fp->Base.Id);
         _mesa_print_program(&fp->Base);
      }
   }

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   fs_visitor v(brw, c, prog, fp, 8);
   if (!v.run()) {
      prog->LinkStatus = false;
      ralloc_strcat(&prog->InfoLog, v.fail_msg);

      _mesa_problem(NULL, "Failed to compile fragment shader: %s\n",
		    v.fail_msg);

      return NULL;
   }

   exec_list *simd16_instructions = NULL;
   fs_visitor v2(brw, c, prog, fp, 16);
   bool no16 = INTEL_DEBUG & DEBUG_NO16;
   if (intel->gen >= 5 && c->prog_data.nr_pull_params == 0 && likely(!no16)) {
      v2.import_uniforms(&v);
      if (!v2.run()) {
         perf_debug("16-wide shader failed to compile, falling back to "
                    "8-wide at a 10-20%% performance cost: %s", v2.fail_msg);
      } else {
         simd16_instructions = &v2.instructions;
      }
   }

   c->prog_data.dispatch_width = 8;

   fs_generator g(brw, c, prog, fp, v.dual_src_output.file != BAD_FILE);
   const unsigned *generated = g.generate_assembly(&v.instructions,
                                                   simd16_instructions,
                                                   final_assembly_size);

   if (unlikely(intel->perf_debug) && shader) {
      if (shader->compiled_once)
         brw_wm_debug_recompile(brw, prog, &c->key);
      shader->compiled_once = true;

      if (start_busy && !drm_intel_bo_busy(intel->batch.last_bo)) {
         perf_debug("FS compile took %.03f ms and stalled the GPU\n",
                    (get_time() - start_time) * 1000);
      }
   }

   return generated;
}

bool
brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = &brw->intel;
   struct brw_wm_prog_key key;

   if (!prog->_LinkedShaders[MESA_SHADER_FRAGMENT])
      return true;

   struct gl_fragment_program *fp = (struct gl_fragment_program *)
      prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;
   struct brw_fragment_program *bfp = brw_fragment_program(fp);
   bool program_uses_dfdy = fp->UsesDFdy;

   memset(&key, 0, sizeof(key));

   if (intel->gen < 6) {
      if (fp->UsesKill)
         key.iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

      if (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
         key.iz_lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

      /* Just assume depth testing. */
      key.iz_lookup |= IZ_DEPTH_TEST_ENABLE_BIT;
      key.iz_lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;
   }

   if (intel->gen < 6)
      key.input_slots_valid |= BITFIELD64_BIT(VARYING_SLOT_POS);

   for (int i = 0; i < VARYING_SLOT_MAX; i++) {
      if (!(fp->Base.InputsRead & BITFIELD64_BIT(i)))
	 continue;

      if (intel->gen < 6) {
         if (_mesa_varying_slot_in_fs((gl_varying_slot) i))
            key.input_slots_valid |= BITFIELD64_BIT(i);
      }
   }

   key.clamp_fragment_color = true;

   for (int i = 0; i < MAX_SAMPLERS; i++) {
      if (fp->Base.ShadowSamplers & (1 << i)) {
         /* Assume DEPTH_TEXTURE_MODE is the default: X, X, X, 1 */
         key.tex.swizzles[i] =
            MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_ONE);
      } else {
         /* Color sampler: assume no swizzling. */
         key.tex.swizzles[i] = SWIZZLE_XYZW;
      }
   }

   if (fp->Base.InputsRead & VARYING_BIT_POS) {
      key.drawable_height = ctx->DrawBuffer->Height;
   }

   if ((fp->Base.InputsRead & VARYING_BIT_POS) || program_uses_dfdy) {
      key.render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   }

   key.nr_color_regions = 1;

   key.program_string_id = bfp->id;

   uint32_t old_prog_offset = brw->wm.prog_offset;
   struct brw_wm_prog_data *old_prog_data = brw->wm.prog_data;

   bool success = do_wm_prog(brw, prog, bfp, &key);

   brw->wm.prog_offset = old_prog_offset;
   brw->wm.prog_data = old_prog_data;

   return success;
}
