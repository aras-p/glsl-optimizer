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

#include "util/hash_table.h"
#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/fbobject.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "util/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
}
#include "brw_fs.h"
#include "brw_cfg.h"
#include "brw_dead_control_flow.h"
#include "main/uniforms.h"
#include "brw_fs_live_variables.h"
#include "glsl/glsl_types.h"

void
fs_inst::init(enum opcode opcode, const fs_reg &dst, fs_reg *src, int sources)
{
   memset(this, 0, sizeof(*this));

   this->opcode = opcode;
   this->dst = dst;
   this->src = src;
   this->sources = sources;

   this->conditional_mod = BRW_CONDITIONAL_NONE;

   /* This will be the case for almost all instructions. */
   this->regs_written = 1;

   this->writes_accumulator = false;
}

fs_inst::fs_inst(enum opcode opcode, const fs_reg &dst)
{
   fs_reg *src = ralloc_array(this, fs_reg, 3);
   init(opcode, dst, src, 0);
}

fs_inst::fs_inst(enum opcode opcode, const fs_reg &dst, const fs_reg &src0)
{
   fs_reg *src = ralloc_array(this, fs_reg, 3);
   src[0] = src0;
   init(opcode, dst, src, 1);
}

fs_inst::fs_inst(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
                 const fs_reg &src1)
{
   fs_reg *src = ralloc_array(this, fs_reg, 3);
   src[0] = src0;
   src[1] = src1;
   init(opcode, dst, src, 2);
}

fs_inst::fs_inst(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
                 const fs_reg &src1, const fs_reg &src2)
{
   fs_reg *src = ralloc_array(this, fs_reg, 3);
   src[0] = src0;
   src[1] = src1;
   src[2] = src2;
   init(opcode, dst, src, 3);
}

fs_inst::fs_inst(enum opcode opcode, const fs_reg &dst, fs_reg src[], int sources)
{
   init(opcode, dst, src, sources);
}

fs_inst::fs_inst(const fs_inst &that)
{
   memcpy(this, &that, sizeof(that));

   this->src = ralloc_array(this, fs_reg, that.sources);

   for (int i = 0; i < that.sources; i++)
      this->src[i] = that.src[i];
}

void
fs_inst::resize_sources(uint8_t num_sources)
{
   if (this->sources != num_sources) {
      this->src = reralloc(this, this->src, fs_reg, num_sources);
      this->sources = num_sources;
   }
}

#define ALU1(op)                                                        \
   fs_inst *                                                            \
   fs_visitor::op(const fs_reg &dst, const fs_reg &src0)                \
   {                                                                    \
      return new(mem_ctx) fs_inst(BRW_OPCODE_##op, dst, src0);          \
   }

#define ALU2(op)                                                        \
   fs_inst *                                                            \
   fs_visitor::op(const fs_reg &dst, const fs_reg &src0,                \
                  const fs_reg &src1)                                   \
   {                                                                    \
      return new(mem_ctx) fs_inst(BRW_OPCODE_##op, dst, src0, src1);    \
   }

#define ALU2_ACC(op)                                                    \
   fs_inst *                                                            \
   fs_visitor::op(const fs_reg &dst, const fs_reg &src0,                \
                  const fs_reg &src1)                                   \
   {                                                                    \
      fs_inst *inst = new(mem_ctx) fs_inst(BRW_OPCODE_##op, dst, src0, src1);\
      inst->writes_accumulator = true;                                  \
      return inst;                                                      \
   }

#define ALU3(op)                                                        \
   fs_inst *                                                            \
   fs_visitor::op(const fs_reg &dst, const fs_reg &src0,                \
                  const fs_reg &src1, const fs_reg &src2)               \
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
ALU2_ACC(MACH)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
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
ALU2(SEL)
ALU2(MAC)

/** Gen4 predicated IF. */
fs_inst *
fs_visitor::IF(enum brw_predicate predicate)
{
   fs_inst *inst = new(mem_ctx) fs_inst(BRW_OPCODE_IF);
   inst->predicate = predicate;
   return inst;
}

/** Gen6 IF with embedded comparison. */
fs_inst *
fs_visitor::IF(const fs_reg &src0, const fs_reg &src1,
               enum brw_conditional_mod condition)
{
   assert(brw->gen == 6);
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
fs_visitor::CMP(fs_reg dst, fs_reg src0, fs_reg src1,
                enum brw_conditional_mod condition)
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
   if (brw->gen == 4) {
      dst.type = src0.type;
      if (dst.file == HW_REG)
	 dst.fixed_hw_reg.type = dst.type;
   }

   resolve_ud_negate(&src0);
   resolve_ud_negate(&src1);

   inst = new(mem_ctx) fs_inst(BRW_OPCODE_CMP, dst, src0, src1);
   inst->conditional_mod = condition;

   return inst;
}

fs_inst *
fs_visitor::LOAD_PAYLOAD(const fs_reg &dst, fs_reg *src, int sources)
{
   fs_inst *inst = new(mem_ctx) fs_inst(SHADER_OPCODE_LOAD_PAYLOAD, dst, src,
                                        sources);
   inst->regs_written = sources;

   return inst;
}

exec_list
fs_visitor::VARYING_PULL_CONSTANT_LOAD(const fs_reg &dst,
                                       const fs_reg &surf_index,
                                       const fs_reg &varying_offset,
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
   if (brw->gen == 4 && dispatch_width == 8) {
      /* Pre-gen5, we can either use a SIMD8 message that requires (header,
       * u, v, r) as parameters, or we can just use the SIMD16 message
       * consisting of (header, u).  We choose the second, at the cost of a
       * longer return length.
       */
      scale = 2;
   }

   enum opcode op;
   if (brw->gen >= 7)
      op = FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7;
   else
      op = FS_OPCODE_VARYING_PULL_CONSTANT_LOAD;
   fs_reg vec4_result = fs_reg(GRF, virtual_grf_alloc(4 * scale), dst.type);
   inst = new(mem_ctx) fs_inst(op, vec4_result, surf_index, vec4_offset);
   inst->regs_written = 4 * scale;
   instructions.push_tail(inst);

   if (brw->gen < 7) {
      inst->base_mrf = 13;
      inst->header_present = true;
      if (brw->gen == 4)
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
fs_inst::equals(fs_inst *inst) const
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
           target == inst->target &&
           eot == inst->eot &&
           header_present == inst->header_present &&
           shadow_compare == inst->shadow_compare &&
           offset == inst->offset);
}

bool
fs_inst::overwrites_reg(const fs_reg &reg) const
{
   return (reg.file == dst.file &&
           reg.reg == dst.reg &&
           reg.reg_offset >= dst.reg_offset  &&
           reg.reg_offset < dst.reg_offset + regs_written);
}

bool
fs_inst::is_send_from_grf() const
{
   return (opcode == FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7 ||
           opcode == SHADER_OPCODE_SHADER_TIME_ADD ||
           opcode == FS_OPCODE_INTERPOLATE_AT_CENTROID ||
           opcode == FS_OPCODE_INTERPOLATE_AT_SAMPLE ||
           opcode == FS_OPCODE_INTERPOLATE_AT_SHARED_OFFSET ||
           opcode == FS_OPCODE_INTERPOLATE_AT_PER_SLOT_OFFSET ||
           (opcode == FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD &&
            src[1].file == GRF) ||
           (is_tex() && src[0].file == GRF));
}

bool
fs_inst::can_do_source_mods(struct brw_context *brw)
{
   if (brw->gen == 6 && is_math())
      return false;

   if (is_send_from_grf())
      return false;

   if (!backend_instruction::can_do_source_mods())
      return false;

   return true;
}

void
fs_reg::init()
{
   memset(this, 0, sizeof(*this));
   stride = 1;
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
   this->fixed_hw_reg.dw1.f = f;
}

/** Immediate value constructor. */
fs_reg::fs_reg(int32_t i)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_D;
   this->fixed_hw_reg.dw1.d = i;
}

/** Immediate value constructor. */
fs_reg::fs_reg(uint32_t u)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_UD;
   this->fixed_hw_reg.dw1.ud = u;
}

/** Fixed brw_reg. */
fs_reg::fs_reg(struct brw_reg fixed_hw_reg)
{
   init();
   this->file = HW_REG;
   this->fixed_hw_reg = fixed_hw_reg;
   this->type = fixed_hw_reg.type;
}

bool
fs_reg::equals(const fs_reg &r) const
{
   return (file == r.file &&
           reg == r.reg &&
           reg_offset == r.reg_offset &&
           subreg_offset == r.subreg_offset &&
           type == r.type &&
           negate == r.negate &&
           abs == r.abs &&
           !reladdr && !r.reladdr &&
           memcmp(&fixed_hw_reg, &r.fixed_hw_reg,
                  sizeof(fixed_hw_reg)) == 0 &&
           stride == r.stride);
}

fs_reg &
fs_reg::apply_stride(unsigned stride)
{
   assert((this->stride * stride) <= 4 &&
          (is_power_of_two(stride) || stride == 0) &&
          file != HW_REG && file != IMM);
   this->stride *= stride;
   return *this;
}

fs_reg &
fs_reg::set_smear(unsigned subreg)
{
   assert(file != HW_REG && file != IMM);
   subreg_offset = subreg * type_sz(type);
   stride = 0;
   return *this;
}

bool
fs_reg::is_contiguous() const
{
   return stride == 1;
}

bool
fs_reg::is_valid_3src() const
{
   return file == GRF || file == UNIFORM;
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
   case GLSL_TYPE_ATOMIC_UINT:
      return 0;
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_INTERFACE:
      unreachable("not reached");
   }

   return 0;
}

fs_reg
fs_visitor::get_timestamp()
{
   assert(brw->gen >= 7);

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
   dst.set_smear(0);

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
   reset.set_smear(2);
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
   int shader_time_index =
      brw_get_shader_time_index(brw, shader_prog, prog, type);
   fs_reg offset = fs_reg(shader_time_index * SHADER_TIME_STRIDE);

   fs_reg payload;
   if (dispatch_width == 8)
      payload = fs_reg(this, glsl_type::uvec2_type);
   else
      payload = fs_reg(this, glsl_type::uint_type);

   emit(new(mem_ctx) fs_inst(SHADER_OPCODE_SHADER_TIME_ADD,
                             fs_reg(), payload, offset, value));
}

void
fs_visitor::vfail(const char *format, va_list va)
{
   char *msg;

   if (failed)
      return;

   failed = true;

   msg = ralloc_vasprintf(mem_ctx, format, va);
   msg = ralloc_asprintf(mem_ctx, "FS compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (INTEL_DEBUG & DEBUG_WM) {
      fprintf(stderr, "%s",  msg);
   }
}

void
fs_visitor::fail(const char *format, ...)
{
   va_list va;

   va_start(va, format);
   vfail(format, va);
   va_end(va);
}

/**
 * Mark this program as impossible to compile in SIMD16 mode.
 *
 * During the SIMD8 compile (which happens first), we can detect and flag
 * things that are unsupported in SIMD16 mode, so the compiler can skip
 * the SIMD16 compile altogether.
 *
 * During a SIMD16 compile (if one happens anyway), this just calls fail().
 */
void
fs_visitor::no16(const char *format, ...)
{
   va_list va;

   va_start(va, format);

   if (dispatch_width == 16) {
      vfail(format, va);
   } else {
      simd16_unsupported = true;

      if (brw->perf_debug) {
         if (no16_msg)
            ralloc_vasprintf_append(&no16_msg, format, va);
         else
            no16_msg = ralloc_vasprintf(mem_ctx, format, va);
      }
   }

   va_end(va);
}

fs_inst *
fs_visitor::emit(enum opcode opcode)
{
   return emit(new(mem_ctx) fs_inst(opcode));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, const fs_reg &dst)
{
   return emit(new(mem_ctx) fs_inst(opcode, dst));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, const fs_reg &dst, const fs_reg &src0)
{
   return emit(new(mem_ctx) fs_inst(opcode, dst, src0));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
                 const fs_reg &src1)
{
   return emit(new(mem_ctx) fs_inst(opcode, dst, src0, src1));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
                 const fs_reg &src1, const fs_reg &src2)
{
   return emit(new(mem_ctx) fs_inst(opcode, dst, src0, src1, src2));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, const fs_reg &dst,
                 fs_reg src[], int sources)
{
   return emit(new(mem_ctx) fs_inst(opcode, dst, src, sources));
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

/**
 * Returns true if the instruction has a flag that means it won't
 * update an entire destination register.
 *
 * For example, dead code elimination and live variable analysis want to know
 * when a write to a variable screens off any preceding values that were in
 * it.
 */
bool
fs_inst::is_partial_write() const
{
   return ((this->predicate && this->opcode != BRW_OPCODE_SEL) ||
           this->force_uncompressed ||
           this->force_sechalf || !this->dst.is_contiguous());
}

int
fs_inst::regs_read(fs_visitor *v, int arg) const
{
   if (is_tex() && arg == 0 && src[0].file == GRF) {
      if (v->dispatch_width == 16)
	 return (mlen + 1) / 2;
      else
	 return mlen;
   }
   return 1;
}

bool
fs_inst::reads_flag() const
{
   return predicate;
}

bool
fs_inst::writes_flag() const
{
   return (conditional_mod && opcode != BRW_OPCODE_SEL) ||
          opcode == FS_OPCODE_MOV_DISPATCH_TO_FLAGS;
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

   if (inst->base_mrf == -1)
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
   case SHADER_OPCODE_TXF_CMS:
   case SHADER_OPCODE_TXF_MCS:
   case SHADER_OPCODE_TG4:
   case SHADER_OPCODE_TG4_OFFSET:
   case SHADER_OPCODE_TXL:
   case SHADER_OPCODE_TXS:
   case SHADER_OPCODE_LOD:
      return 1;
   case FS_OPCODE_FB_WRITE:
      return 2;
   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
   case SHADER_OPCODE_GEN4_SCRATCH_READ:
      return 1;
   case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD:
      return inst->mlen;
   case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
      return 2;
   case SHADER_OPCODE_UNTYPED_ATOMIC:
   case SHADER_OPCODE_UNTYPED_SURFACE_READ:
   case FS_OPCODE_INTERPOLATE_AT_CENTROID:
   case FS_OPCODE_INTERPOLATE_AT_SAMPLE:
   case FS_OPCODE_INTERPOLATE_AT_SHARED_OFFSET:
   case FS_OPCODE_INTERPOLATE_AT_PER_SLOT_OFFSET:
      return 0;
   default:
      unreachable("not reached");
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
fs_reg::fs_reg(enum register_file file, int reg, enum brw_reg_type type)
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

/* For SIMD16, we need to follow from the uniform setup of SIMD8 dispatch.
 * This brings in those uniform definitions
 */
void
fs_visitor::import_uniforms(fs_visitor *v)
{
   hash_table_call_foreach(v->variable_ht,
			   import_uniforms_callback,
			   variable_ht);
   this->push_constant_loc = v->push_constant_loc;
   this->pull_constant_loc = v->pull_constant_loc;
   this->uniforms = v->uniforms;
   this->param_size = v->param_size;
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
   unsigned params_before = uniforms;
   for (unsigned u = 0; u < shader_prog->NumUserUniformStorage; u++) {
      struct gl_uniform_storage *storage = &shader_prog->UniformStorage[u];

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
         stage_prog_data->param[uniforms++] = &storage->storage[i];
      }
   }

   /* Make sure we actually initialized the right amount of stuff here. */
   assert(params_before + ir->type->component_slots() == uniforms);
   (void)params_before;
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
      int index = _mesa_add_state_reference(this->prog->Parameters,
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

         stage_prog_data->param[uniforms++] =
            &prog->Parameters->ParameterValues[index][swiz];
      }
   }
}

fs_reg *
fs_visitor::emit_fragcoord_interpolation(ir_variable *ir)
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   fs_reg wpos = *reg;
   bool flip = !ir->data.origin_upper_left ^ key->render_to_fbo;

   /* gl_FragCoord.x */
   if (ir->data.pixel_center_integer) {
      emit(MOV(wpos, this->pixel_x));
   } else {
      emit(ADD(wpos, this->pixel_x, fs_reg(0.5f)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.y */
   if (!flip && ir->data.pixel_center_integer) {
      emit(MOV(wpos, this->pixel_y));
   } else {
      fs_reg pixel_y = this->pixel_y;
      float offset = (ir->data.pixel_center_integer ? 0.0 : 0.5);

      if (flip) {
	 pixel_y.negate = true;
	 offset += key->drawable_height - 1.0;
      }

      emit(ADD(wpos, pixel_y, fs_reg(offset)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.z */
   if (brw->gen >= 6) {
      emit(MOV(wpos, fs_reg(brw_vec8_grf(payload.source_depth_reg, 0))));
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
                         bool is_centroid, bool is_sample)
{
   brw_wm_barycentric_interp_mode barycoord_mode;
   if (brw->gen >= 6) {
      if (is_centroid) {
         if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
            barycoord_mode = BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC;
         else
            barycoord_mode = BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC;
      } else if (is_sample) {
          if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
            barycoord_mode = BRW_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC;
         else
            barycoord_mode = BRW_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC;
      } else {
         if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
            barycoord_mode = BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
         else
            barycoord_mode = BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC;
      }
   } else {
      /* On Ironlake and below, there is only one interpolation mode.
       * Centroid interpolation doesn't mean anything on this hardware --
       * there is no multisampling.
       */
      barycoord_mode = BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
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

   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;

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
      ir->determine_interpolation_mode(key->flat_shade);

   int location = ir->data.location;
   for (unsigned int i = 0; i < array_elements; i++) {
      for (unsigned int j = 0; j < type->matrix_columns; j++) {
	 if (prog_data->urb_setup[location] == -1) {
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
               struct brw_reg interp = interp_reg(location, k);
               if (brw->needs_unlit_centroid_workaround && ir->data.centroid) {
                  /* Get the pixel/sample mask into f0 so that we know
                   * which pixels are lit.  Then, for each channel that is
                   * unlit, replace the centroid data with non-centroid
                   * data.
                   */
                  emit(FS_OPCODE_MOV_DISPATCH_TO_FLAGS);

                  fs_inst *inst;
                  inst = emit_linterp(attr, fs_reg(interp), interpolation_mode,
                                      false, false);
                  inst->predicate = BRW_PREDICATE_NORMAL;
                  inst->predicate_inverse = true;
                  if (brw->has_pln)
                     inst->no_dd_clear = true;

                  inst = emit_linterp(attr, fs_reg(interp), interpolation_mode,
                                      ir->data.centroid && !key->persample_shading,
                                      ir->data.sample || key->persample_shading);
                  inst->predicate = BRW_PREDICATE_NORMAL;
                  inst->predicate_inverse = false;
                  if (brw->has_pln)
                     inst->no_dd_check = true;

               } else {
                  emit_linterp(attr, fs_reg(interp), interpolation_mode,
                               ir->data.centroid && !key->persample_shading,
                               ir->data.sample || key->persample_shading);
               }
               if (brw->gen < 6 && interpolation_mode == INTERP_QUALIFIER_SMOOTH) {
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
fs_visitor::emit_frontfacing_interpolation()
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, glsl_type::bool_type);

   if (brw->gen >= 6) {
      /* Bit 15 of g0.0 is 0 if the polygon is front facing. We want to create
       * a boolean result from this (~0/true or 0/false).
       *
       * We can use the fact that bit 15 is the MSB of g0.0:W to accomplish
       * this task in only one instruction:
       *    - a negation source modifier will flip the bit; and
       *    - a W -> D type conversion will sign extend the bit into the high
       *      word of the destination.
       *
       * An ASR 15 fills the low word of the destination.
       */
      fs_reg g0 = fs_reg(retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_W));
      g0.negate = true;

      emit(ASR(*reg, g0, fs_reg(15)));
   } else {
      /* Bit 31 of g1.6 is 0 if the polygon is front facing. We want to create
       * a boolean result from this (1/true or 0/false).
       *
       * Like in the above case, since the bit is the MSB of g1.6:UD we can use
       * the negation source modifier to flip it. Unfortunately the SHR
       * instruction only operates on UD (or D with an abs source modifier)
       * sources without negation.
       *
       * Instead, use ASR (which will give ~0/true or 0/false) followed by an
       * AND 1.
       */
      fs_reg asr = fs_reg(this, glsl_type::bool_type);
      fs_reg g1_6 = fs_reg(retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_D));
      g1_6.negate = true;

      emit(ASR(asr, g1_6, fs_reg(31)));
      emit(AND(*reg, asr, fs_reg(1)));
   }

   return reg;
}

void
fs_visitor::compute_sample_position(fs_reg dst, fs_reg int_sample_pos)
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;
   assert(dst.type == BRW_REGISTER_TYPE_F);

   if (key->compute_pos_offset) {
      /* Convert int_sample_pos to floating point */
      emit(MOV(dst, int_sample_pos));
      /* Scale to the range [0, 1] */
      emit(MUL(dst, dst, fs_reg(1 / 16.0f)));
   }
   else {
      /* From ARB_sample_shading specification:
       * "When rendering to a non-multisample buffer, or if multisample
       *  rasterization is disabled, gl_SamplePosition will always be
       *  (0.5, 0.5).
       */
      emit(MOV(dst, fs_reg(0.5f)));
   }
}

fs_reg *
fs_visitor::emit_samplepos_setup()
{
   assert(brw->gen >= 6);

   this->current_annotation = "compute sample position";
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, glsl_type::vec2_type);
   fs_reg pos = *reg;
   fs_reg int_sample_x = fs_reg(this, glsl_type::int_type);
   fs_reg int_sample_y = fs_reg(this, glsl_type::int_type);

   /* WM will be run in MSDISPMODE_PERSAMPLE. So, only one of SIMD8 or SIMD16
    * mode will be enabled.
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 344:
    * R31.1:0         Position Offset X/Y for Slot[3:0]
    * R31.3:2         Position Offset X/Y for Slot[7:4]
    * .....
    *
    * The X, Y sample positions come in as bytes in  thread payload. So, read
    * the positions using vstride=16, width=8, hstride=2.
    */
   struct brw_reg sample_pos_reg =
      stride(retype(brw_vec1_grf(payload.sample_pos_reg, 0),
                    BRW_REGISTER_TYPE_B), 16, 8, 2);

   fs_inst *inst = emit(MOV(int_sample_x, fs_reg(sample_pos_reg)));
   if (dispatch_width == 16) {
      inst->force_uncompressed = true;
      inst = emit(MOV(half(int_sample_x, 1),
                      fs_reg(suboffset(sample_pos_reg, 16))));
      inst->force_sechalf = true;
   }
   /* Compute gl_SamplePosition.x */
   compute_sample_position(pos, int_sample_x);
   pos.reg_offset++;
   inst = emit(MOV(int_sample_y, fs_reg(suboffset(sample_pos_reg, 1))));
   if (dispatch_width == 16) {
      inst->force_uncompressed = true;
      inst = emit(MOV(half(int_sample_y, 1),
                      fs_reg(suboffset(sample_pos_reg, 17))));
      inst->force_sechalf = true;
   }
   /* Compute gl_SamplePosition.y */
   compute_sample_position(pos, int_sample_y);
   return reg;
}

fs_reg *
fs_visitor::emit_sampleid_setup(ir_variable *ir)
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;
   assert(brw->gen >= 6);

   this->current_annotation = "compute sample id";
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);

   if (key->compute_sample_id) {
      fs_reg t1 = fs_reg(this, glsl_type::int_type);
      fs_reg t2 = fs_reg(this, glsl_type::int_type);
      t2.type = BRW_REGISTER_TYPE_UW;

      /* The PS will be run in MSDISPMODE_PERSAMPLE. For example with
       * 8x multisampling, subspan 0 will represent sample N (where N
       * is 0, 2, 4 or 6), subspan 1 will represent sample 1, 3, 5 or
       * 7. We can find the value of N by looking at R0.0 bits 7:6
       * ("Starting Sample Pair Index (SSPI)") and multiplying by two
       * (since samples are always delivered in pairs). That is, we
       * compute 2*((R0.0 & 0xc0) >> 6) == (R0.0 & 0xc0) >> 5. Then
       * we need to add N to the sequence (0, 0, 0, 0, 1, 1, 1, 1) in
       * case of SIMD8 and sequence (0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
       * 2, 3, 3, 3, 3) in case of SIMD16. We compute this sequence by
       * populating a temporary variable with the sequence (0, 1, 2, 3),
       * and then reading from it using vstride=1, width=4, hstride=0.
       * These computations hold good for 4x multisampling as well.
       *
       * For 2x MSAA and SIMD16, we want to use the sequence (0, 1, 0, 1):
       * the first four slots are sample 0 of subspan 0; the next four
       * are sample 1 of subspan 0; the third group is sample 0 of
       * subspan 1, and finally sample 1 of subspan 1.
       */
      fs_inst *inst;
      inst = emit(BRW_OPCODE_AND, t1,
                  fs_reg(retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UD)),
                  fs_reg(0xc0));
      inst->force_writemask_all = true;
      inst = emit(BRW_OPCODE_SHR, t1, t1, fs_reg(5));
      inst->force_writemask_all = true;
      /* This works for both SIMD8 and SIMD16 */
      inst = emit(MOV(t2, brw_imm_v(key->persample_2x ? 0x1010 : 0x3210)));
      inst->force_writemask_all = true;
      /* This special instruction takes care of setting vstride=1,
       * width=4, hstride=0 of t2 during an ADD instruction.
       */
      emit(FS_OPCODE_SET_SAMPLE_ID, *reg, t1, t2);
   } else {
      /* As per GL_ARB_sample_shading specification:
       * "When rendering to a non-multisample buffer, or if multisample
       *  rasterization is disabled, gl_SampleID will always be zero."
       */
      emit(BRW_OPCODE_MOV, *reg, fs_reg(0));
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
   if (brw->gen == 6 && src.file != UNIFORM && src.file != IMM &&
       !src.abs && !src.negate)
      return src;

   /* Gen7 relaxes most of the above restrictions, but still can't use IMM
    * operands to math
    */
   if (brw->gen >= 7 && src.file != IMM)
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
      unreachable("not reached: bad math opcode");
   }

   /* Can't do hstride == 0 args to gen6 math, so expand it out.  We
    * might be able to do better by doing execsize = 1 math and then
    * expanding that result out, but we would need to be careful with
    * masking.
    *
    * Gen 6 hardware ignores source modifiers (negate and abs) on math
    * instructions, so we also move to a temp to set those up.
    */
   if (brw->gen == 6 || brw->gen == 7)
      src = fix_math_operand(src);

   fs_inst *inst = emit(opcode, dst, src);

   if (brw->gen < 6) {
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

   if (brw->gen >= 8) {
      inst = emit(opcode, dst, src0, src1);
   } else if (brw->gen >= 6) {
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
   if (dispatch_width == 8) {
      prog_data->dispatch_grf_start_reg = payload.num_regs;
   } else {
      assert(stage == MESA_SHADER_FRAGMENT);
      brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;
      prog_data->dispatch_grf_start_reg_16 = payload.num_regs;
   }

   prog_data->curb_read_length = ALIGN(stage_prog_data->nr_params, 8) / 8;

   /* Map the offsets in the UNIFORM file to fixed HW regs. */
   foreach_in_list(fs_inst, inst, &instructions) {
      for (unsigned int i = 0; i < inst->sources; i++) {
	 if (inst->src[i].file == UNIFORM) {
            int uniform_nr = inst->src[i].reg + inst->src[i].reg_offset;
            int constant_nr;
            if (uniform_nr >= 0 && uniform_nr < (int) uniforms) {
               constant_nr = push_constant_loc[uniform_nr];
            } else {
               /* Section 5.11 of the OpenGL 4.1 spec says:
                * "Out-of-bounds reads return undefined values, which include
                *  values from other variables of the active program or zero."
                * Just return the first push constant.
                */
               constant_nr = 0;
            }

	    struct brw_reg brw_reg = brw_vec1_grf(payload.num_regs +
						  constant_nr / 8,
						  constant_nr % 8);

	    inst->src[i].file = HW_REG;
	    inst->src[i].fixed_hw_reg = byte_offset(
               retype(brw_reg, inst->src[i].type),
               inst->src[i].subreg_offset);
	 }
      }
   }
}

void
fs_visitor::calculate_urb_setup()
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;

   for (unsigned int i = 0; i < VARYING_SLOT_MAX; i++) {
      prog_data->urb_setup[i] = -1;
   }

   int urb_next = 0;
   /* Figure out where each of the incoming setup attributes lands. */
   if (brw->gen >= 6) {
      if (_mesa_bitcount_64(prog->InputsRead &
                            BRW_FS_VARYING_INPUT_MASK) <= 16) {
         /* The SF/SBE pipeline stage can do arbitrary rearrangement of the
          * first 16 varying inputs, so we can put them wherever we want.
          * Just put them in order.
          *
          * This is useful because it means that (a) inputs not used by the
          * fragment shader won't take up valuable register space, and (b) we
          * won't have to recompile the fragment shader if it gets paired with
          * a different vertex (or geometry) shader.
          */
         for (unsigned int i = 0; i < VARYING_SLOT_MAX; i++) {
            if (prog->InputsRead & BRW_FS_VARYING_INPUT_MASK &
                BITFIELD64_BIT(i)) {
               prog_data->urb_setup[i] = urb_next++;
            }
         }
      } else {
         /* We have enough input varyings that the SF/SBE pipeline stage can't
          * arbitrarily rearrange them to suit our whim; we have to put them
          * in an order that matches the output of the previous pipeline stage
          * (geometry or vertex shader).
          */
         struct brw_vue_map prev_stage_vue_map;
         brw_compute_vue_map(brw, &prev_stage_vue_map,
                             key->input_slots_valid);
         int first_slot = 2 * BRW_SF_URB_ENTRY_READ_OFFSET;
         assert(prev_stage_vue_map.num_slots <= first_slot + 32);
         for (int slot = first_slot; slot < prev_stage_vue_map.num_slots;
              slot++) {
            int varying = prev_stage_vue_map.slot_to_varying[slot];
            /* Note that varying == BRW_VARYING_SLOT_COUNT when a slot is
             * unused.
             */
            if (varying != BRW_VARYING_SLOT_COUNT &&
                (prog->InputsRead & BRW_FS_VARYING_INPUT_MASK &
                 BITFIELD64_BIT(varying))) {
               prog_data->urb_setup[varying] = slot - first_slot;
            }
         }
         urb_next = prev_stage_vue_map.num_slots - first_slot;
      }
   } else {
      /* FINISHME: The sf doesn't map VS->FS inputs for us very well. */
      for (unsigned int i = 0; i < VARYING_SLOT_MAX; i++) {
         /* Point size is packed into the header, not as a general attribute */
         if (i == VARYING_SLOT_PSIZ)
            continue;

	 if (key->input_slots_valid & BITFIELD64_BIT(i)) {
	    /* The back color slot is skipped when the front color is
	     * also written to.  In addition, some slots can be
	     * written in the vertex shader and not read in the
	     * fragment shader.  So the register number must always be
	     * incremented, mapped or not.
	     */
	    if (_mesa_varying_slot_in_fs((gl_varying_slot) i))
	       prog_data->urb_setup[i] = urb_next;
            urb_next++;
	 }
      }

      /*
       * It's a FS only attribute, and we did interpolation for this attribute
       * in SF thread. So, count it here, too.
       *
       * See compile_sf_prog() for more info.
       */
      if (prog->InputsRead & BITFIELD64_BIT(VARYING_SLOT_PNTC))
         prog_data->urb_setup[VARYING_SLOT_PNTC] = urb_next++;
   }

   prog_data->num_varying_inputs = urb_next;
}

void
fs_visitor::assign_urb_setup()
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;

   int urb_start = payload.num_regs + prog_data->base.curb_read_length;

   /* Offset all the urb_setup[] index by the actual position of the
    * setup regs, now that the location of the constants has been chosen.
    */
   foreach_in_list(fs_inst, inst, &instructions) {
      if (inst->opcode == FS_OPCODE_LINTERP) {
	 assert(inst->src[2].file == HW_REG);
	 inst->src[2].fixed_hw_reg.nr += urb_start;
      }

      if (inst->opcode == FS_OPCODE_CINTERP) {
	 assert(inst->src[0].file == HW_REG);
	 inst->src[0].fixed_hw_reg.nr += urb_start;
      }
   }

   /* Each attribute is 4 setup channels, each of which is half a reg. */
   this->first_non_payload_grf =
      urb_start + prog_data->num_varying_inputs * 2;
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

   foreach_in_list(fs_inst, inst, &instructions) {
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
         for (int i = 0; i < inst->sources; i++) {
            if (inst->src[i].file == GRF) {
               split_grf[inst->src[i].reg] = false;
            }
         }
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

   foreach_in_list(fs_inst, inst, &instructions) {
      if (inst->dst.file == GRF &&
	  split_grf[inst->dst.reg] &&
	  inst->dst.reg_offset != 0) {
	 inst->dst.reg = (new_virtual_grf[inst->dst.reg] +
			  inst->dst.reg_offset - 1);
	 inst->dst.reg_offset = 0;
      }
      for (int i = 0; i < inst->sources; i++) {
	 if (inst->src[i].file == GRF &&
	     split_grf[inst->src[i].reg] &&
	     inst->src[i].reg_offset != 0) {
	    inst->src[i].reg = (new_virtual_grf[inst->src[i].reg] +
				inst->src[i].reg_offset - 1);
	    inst->src[i].reg_offset = 0;
	 }
      }
   }
   invalidate_live_intervals(false);
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
   if (unlikely(INTEL_DEBUG & DEBUG_OPTIMIZER))
      return;

   /* Mark which virtual GRFs are used, and count how many. */
   int remap_table[this->virtual_grf_count];
   memset(remap_table, -1, sizeof(remap_table));

   foreach_in_list(const fs_inst, inst, &instructions) {
      if (inst->dst.file == GRF)
         remap_table[inst->dst.reg] = 0;

      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file == GRF)
            remap_table[inst->src[i].reg] = 0;
      }
   }

   /* Compact the GRF arrays. */
   int new_index = 0;
   for (int i = 0; i < this->virtual_grf_count; i++) {
      if (remap_table[i] != -1) {
         remap_table[i] = new_index;
         virtual_grf_sizes[new_index] = virtual_grf_sizes[i];
         invalidate_live_intervals(false);
         ++new_index;
      }
   }

   this->virtual_grf_count = new_index;

   /* Patch all the instructions to use the newly renumbered registers */
   foreach_in_list(fs_inst, inst, &instructions) {
      if (inst->dst.file == GRF)
         inst->dst.reg = remap_table[inst->dst.reg];

      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file == GRF)
            inst->src[i].reg = remap_table[inst->src[i].reg];
      }
   }

   /* Patch all the references to delta_x/delta_y, since they're used in
    * register allocation.  If they're unused, switch them to BAD_FILE so
    * we don't think some random VGRF is delta_x/delta_y.
    */
   for (unsigned i = 0; i < ARRAY_SIZE(delta_x); i++) {
      if (delta_x[i].file == GRF) {
         if (remap_table[delta_x[i].reg] != -1) {
            delta_x[i].reg = remap_table[delta_x[i].reg];
         } else {
            delta_x[i].file = BAD_FILE;
         }
      }
   }
   for (unsigned i = 0; i < ARRAY_SIZE(delta_y); i++) {
      if (delta_y[i].file == GRF) {
         if (remap_table[delta_y[i].reg] != -1) {
            delta_y[i].reg = remap_table[delta_y[i].reg];
         } else {
            delta_y[i].file = BAD_FILE;
         }
      }
   }
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
   if (dispatch_width != 8)
      return;

   pull_constant_loc = ralloc_array(mem_ctx, int, uniforms);

   for (unsigned int i = 0; i < uniforms; i++) {
      pull_constant_loc[i] = -1;
   }

   /* Walk through and find array access of uniforms.  Put a copy of that
    * uniform in the pull constant buffer.
    *
    * Note that we don't move constant-indexed accesses to arrays.  No
    * testing has been done of the performance impact of this choice.
    */
   foreach_in_list_safe(fs_inst, inst, &instructions) {
      for (int i = 0 ; i < inst->sources; i++) {
         if (inst->src[i].file != UNIFORM || !inst->src[i].reladdr)
            continue;

         int uniform = inst->src[i].reg;

         /* If this array isn't already present in the pull constant buffer,
          * add it.
          */
         if (pull_constant_loc[uniform] == -1) {
            const gl_constant_value **values = &stage_prog_data->param[uniform];

            assert(param_size[uniform]);

            for (int j = 0; j < param_size[uniform]; j++) {
               pull_constant_loc[uniform + j] = stage_prog_data->nr_pull_params;

               stage_prog_data->pull_param[stage_prog_data->nr_pull_params++] =
                  values[j];
            }
         }
      }
   }
}

/**
 * Assign UNIFORM file registers to either push constants or pull constants.
 *
 * We allow a fragment shader to have more than the specified minimum
 * maximum number of fragment shader uniform components (64).  If
 * there are too many of these, they'd fill up all of register space.
 * So, this will push some of them out to the pull constant buffer and
 * update the program to load them.
 */
void
fs_visitor::assign_constant_locations()
{
   /* Only the first compile (SIMD8 mode) gets to decide on locations. */
   if (dispatch_width != 8)
      return;

   /* Find which UNIFORM registers are still in use. */
   bool is_live[uniforms];
   for (unsigned int i = 0; i < uniforms; i++) {
      is_live[i] = false;
   }

   foreach_in_list(fs_inst, inst, &instructions) {
      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file != UNIFORM)
            continue;

         int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;
         if (constant_nr >= 0 && constant_nr < (int) uniforms)
            is_live[constant_nr] = true;
      }
   }

   /* Only allow 16 registers (128 uniform components) as push constants.
    *
    * Just demote the end of the list.  We could probably do better
    * here, demoting things that are rarely used in the program first.
    *
    * If changing this value, note the limitation about total_regs in
    * brw_curbe.c.
    */
   unsigned int max_push_components = 16 * 8;
   unsigned int num_push_constants = 0;

   push_constant_loc = ralloc_array(mem_ctx, int, uniforms);

   for (unsigned int i = 0; i < uniforms; i++) {
      if (!is_live[i] || pull_constant_loc[i] != -1) {
         /* This UNIFORM register is either dead, or has already been demoted
          * to a pull const.  Mark it as no longer living in the param[] array.
          */
         push_constant_loc[i] = -1;
         continue;
      }

      if (num_push_constants < max_push_components) {
         /* Retain as a push constant.  Record the location in the params[]
          * array.
          */
         push_constant_loc[i] = num_push_constants++;
      } else {
         /* Demote to a pull constant. */
         push_constant_loc[i] = -1;

         int pull_index = stage_prog_data->nr_pull_params++;
         stage_prog_data->pull_param[pull_index] = stage_prog_data->param[i];
         pull_constant_loc[i] = pull_index;
      }
   }

   stage_prog_data->nr_params = num_push_constants;

   /* Up until now, the param[] array has been indexed by reg + reg_offset
    * of UNIFORM registers.  Condense it to only contain the uniforms we
    * chose to upload as push constants.
    */
   for (unsigned int i = 0; i < uniforms; i++) {
      int remapped = push_constant_loc[i];

      if (remapped == -1)
         continue;

      assert(remapped <= (int)i);
      stage_prog_data->param[remapped] = stage_prog_data->param[i];
   }
}

/**
 * Replace UNIFORM register file access with either UNIFORM_PULL_CONSTANT_LOAD
 * or VARYING_PULL_CONSTANT_LOAD instructions which load values into VGRFs.
 */
void
fs_visitor::demote_pull_constants()
{
   calculate_cfg();

   foreach_block_and_inst (block, fs_inst, inst, cfg) {
      for (int i = 0; i < inst->sources; i++) {
	 if (inst->src[i].file != UNIFORM)
	    continue;

         int pull_index = pull_constant_loc[inst->src[i].reg +
                                            inst->src[i].reg_offset];
         if (pull_index == -1)
	    continue;

         /* Set up the annotation tracking for new generated instructions. */
         base_ir = inst->ir;
         current_annotation = inst->annotation;

         fs_reg surf_index(stage_prog_data->binding_table.pull_constants_start);
         fs_reg dst = fs_reg(this, glsl_type::float_type);

         /* Generate a pull load into dst. */
         if (inst->src[i].reladdr) {
            exec_list list = VARYING_PULL_CONSTANT_LOAD(dst,
                                                        surf_index,
                                                        *inst->src[i].reladdr,
                                                        pull_index);
            inst->insert_before(block, &list);
            inst->src[i].reladdr = NULL;
         } else {
            fs_reg offset = fs_reg((unsigned)(pull_index * 4) & ~15);
            fs_inst *pull =
               new(mem_ctx) fs_inst(FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD,
                                    dst, surf_index, offset);
            inst->insert_before(block, pull);
            inst->src[i].set_smear(pull_index & 3);
         }

         /* Rewrite the instruction to use the temporary VGRF. */
         inst->src[i].file = GRF;
         inst->src[i].reg = dst.reg;
         inst->src[i].reg_offset = 0;
      }
   }
   invalidate_live_intervals(false);
}

bool
fs_visitor::opt_algebraic()
{
   bool progress = false;

   foreach_in_list(fs_inst, inst, &instructions) {
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
      case BRW_OPCODE_OR:
         if (inst->src[0].equals(inst->src[1])) {
            inst->opcode = BRW_OPCODE_MOV;
            inst->src[1] = reg_undef;
            progress = true;
            break;
         }
         break;
      case BRW_OPCODE_LRP:
         if (inst->src[1].equals(inst->src[2])) {
            inst->opcode = BRW_OPCODE_MOV;
            inst->src[0] = inst->src[1];
            inst->src[1] = reg_undef;
            inst->src[2] = reg_undef;
            progress = true;
            break;
         }
         break;
      case BRW_OPCODE_SEL:
         if (inst->src[0].equals(inst->src[1])) {
            inst->opcode = BRW_OPCODE_MOV;
            inst->src[1] = reg_undef;
            inst->predicate = BRW_PREDICATE_NONE;
            inst->predicate_inverse = false;
            progress = true;
         } else if (inst->saturate && inst->src[1].file == IMM) {
            switch (inst->conditional_mod) {
            case BRW_CONDITIONAL_LE:
            case BRW_CONDITIONAL_L:
               switch (inst->src[1].type) {
               case BRW_REGISTER_TYPE_F:
                  if (inst->src[1].fixed_hw_reg.dw1.f >= 1.0f) {
                     inst->opcode = BRW_OPCODE_MOV;
                     inst->src[1] = reg_undef;
                     progress = true;
                  }
                  break;
               default:
                  break;
               }
               break;
            case BRW_CONDITIONAL_GE:
            case BRW_CONDITIONAL_G:
               switch (inst->src[1].type) {
               case BRW_REGISTER_TYPE_F:
                  if (inst->src[1].fixed_hw_reg.dw1.f <= 0.0f) {
                     inst->opcode = BRW_OPCODE_MOV;
                     inst->src[1] = reg_undef;
                     inst->conditional_mod = BRW_CONDITIONAL_NONE;
                     progress = true;
                  }
                  break;
               default:
                  break;
               }
            default:
               break;
            }
         }
         break;
      default:
	 break;
      }
   }

   return progress;
}

bool
fs_visitor::opt_register_renaming()
{
   bool progress = false;
   int depth = 0;

   int remap[virtual_grf_count];
   memset(remap, -1, sizeof(int) * virtual_grf_count);

   foreach_in_list(fs_inst, inst, &this->instructions) {
      if (inst->opcode == BRW_OPCODE_IF || inst->opcode == BRW_OPCODE_DO) {
         depth++;
      } else if (inst->opcode == BRW_OPCODE_ENDIF ||
                 inst->opcode == BRW_OPCODE_WHILE) {
         depth--;
      }

      /* Rewrite instruction sources. */
      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file == GRF &&
             remap[inst->src[i].reg] != -1 &&
             remap[inst->src[i].reg] != inst->src[i].reg) {
            inst->src[i].reg = remap[inst->src[i].reg];
            progress = true;
         }
      }

      const int dst = inst->dst.reg;

      if (depth == 0 &&
          inst->dst.file == GRF &&
          virtual_grf_sizes[inst->dst.reg] == 1 &&
          !inst->is_partial_write()) {
         if (remap[dst] == -1) {
            remap[dst] = dst;
         } else {
            remap[dst] = virtual_grf_alloc(1);
            inst->dst.reg = remap[dst];
            progress = true;
         }
      } else if (inst->dst.file == GRF &&
                 remap[dst] != -1 &&
                 remap[dst] != dst) {
         inst->dst.reg = remap[dst];
         progress = true;
      }
   }

   if (progress) {
      invalidate_live_intervals(false);

      for (unsigned i = 0; i < ARRAY_SIZE(delta_x); i++) {
         if (delta_x[i].file == GRF && remap[delta_x[i].reg] != -1) {
            delta_x[i].reg = remap[delta_x[i].reg];
         }
      }
      for (unsigned i = 0; i < ARRAY_SIZE(delta_y); i++) {
         if (delta_y[i].file == GRF && remap[delta_y[i].reg] != -1) {
            delta_y[i].reg = remap[delta_y[i].reg];
         }
      }
   }

   return progress;
}

bool
fs_visitor::compute_to_mrf()
{
   bool progress = false;
   int next_ip = 0;

   calculate_live_intervals();

   foreach_block_and_inst_safe(block, fs_inst, inst, cfg) {
      int ip = next_ip;
      next_ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->is_partial_write() ||
	  inst->dst.file != MRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  inst->src[0].abs || inst->src[0].negate ||
          !inst->src[0].is_contiguous() ||
          inst->src[0].subreg_offset)
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
      if (this->virtual_grf_end[inst->src[0].reg] > ip)
	 continue;

      /* Found a move of a GRF to a MRF.  Let's see if we can go
       * rewrite the thing that made this GRF to write into the MRF.
       */
      fs_inst *scan_inst;
      for (scan_inst = (fs_inst *)inst->prev;
           !scan_inst->is_head_sentinel();
	   scan_inst = (fs_inst *)scan_inst->prev) {
	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->src[0].reg) {
	    /* Found the last thing to write our reg we want to turn
	     * into a compute-to-MRF.
	     */

	    /* If this one instruction didn't populate all the
	     * channels, bail.  We might be able to rewrite everything
	     * that writes that reg, but it would require smarter
	     * tracking to delay the rewriting until complete success.
	     */
	    if (scan_inst->is_partial_write())
	       break;

            /* Things returning more than one register would need us to
             * understand coalescing out more than one MOV at a time.
             */
            if (scan_inst->regs_written > 1)
               break;

	    /* SEND instructions can't have MRF as a destination. */
	    if (scan_inst->mlen)
	       break;

	    if (brw->gen == 6) {
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
	       inst->remove(block);
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
	 for (int i = 0; i < scan_inst->sources; i++) {
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

	 if (scan_inst->mlen > 0 && scan_inst->base_mrf != -1) {
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
      invalidate_live_intervals(false);

   return progress;
}

/**
 * Once we've generated code, try to convert normal FS_OPCODE_FB_WRITE
 * instructions to FS_OPCODE_REP_FB_WRITE.
 */
void
fs_visitor::try_rep_send()
{
   int i, count;
   fs_inst *start = NULL;
   bblock_t *mov_block;

   /* From the Ivybridge PRM, Volume 4 Part 1, section 3.9.11.2
    * ("Message Descriptor - Render Target Write"):
    *
    * "SIMD16_REPDATA message must not be used in SIMD8 pixel-shaders."
    */
   if (dispatch_width != 16)
      return;

   /* The constant color write message can't handle anything but the 4 color
    * values.  We could do MRT, but the loops below would need to understand
    * handling the header being enabled or disabled on different messages.  It
    * also requires that the render target be tiled, which might not be the
    * case for some EGLImage paths or if we some day do rendering to PBOs.
    */
   if (prog->OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH) ||
       payload.aa_dest_stencil_reg ||
       payload.dest_depth_reg ||
       dual_src_output.file != BAD_FILE)
      return;

   /* The optimization is implemented as one pass through the instruction
    * list.  We keep track of the most recent block of MOVs into sequential
    * MRFs from single, sequential float registers (ie uniforms).  Then when
    * we find an FB_WRITE opcode, we see if the payload registers match the
    * destination registers in our block of MOVs.
    */
   count = 0;
   foreach_block_and_inst_safe(block, fs_inst, inst, cfg) {
      if (count == 0) {
         start = inst;
         mov_block = block;
      }
      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF &&
          inst->dst.reg == start->dst.reg + 2 * count &&
          inst->src[0].file == HW_REG &&
          inst->src[0].reg_offset == start->src[0].reg_offset + count) {
         if (count == 0) {
            start = inst;
            mov_block = block;
         }
         count++;
      }

      if (inst->opcode == FS_OPCODE_FB_WRITE &&
          count == 4 &&
          (inst->base_mrf == start->dst.reg ||
           (inst->base_mrf + 2 == start->dst.reg && inst->header_present))) {
         fs_inst *mov = MOV(start->dst, start->src[0]);

         /* Make a MOV that moves the four floats into the replicated write
          * payload.  Since we're running at the very end of code generation
          * we can use hw registers and generate the stride and offsets we
          * need for this MOV.  We use the first of the eight registers
          * allocated for the SIMD16 payload for the four floats.
          */
         mov->dst.fixed_hw_reg =
            brw_vec4_reg(BRW_MESSAGE_REGISTER_FILE,
                         start->dst.reg, 0);
         mov->dst.file = HW_REG;
         mov->dst.type = mov->dst.fixed_hw_reg.type;

         mov->src[0].fixed_hw_reg =
            brw_vec4_grf(mov->src[0].fixed_hw_reg.nr, 0);
         mov->src[0].file = HW_REG;
         mov->src[0].type = mov->src[0].fixed_hw_reg.type;
         mov->force_writemask_all = true;
         mov->dst.type = BRW_REGISTER_TYPE_F;

         /* Replace the four MOVs with the new vec4 MOV. */
         start->insert_before(mov_block, mov);
         for (i = 0; i < 4; i++)
            ((fs_inst *) mov->next)->remove(mov_block);

         /* Finally, adjust the message length and set the opcode to
          * REP_FB_WRITE for the send, so that the generator will use the
          * replicated data mesage type.  Then reset count so we'll start
          * looking for a new block in case we're in a MRT shader.
          */
         inst->opcode = FS_OPCODE_REP_FB_WRITE;
         inst->mlen -= 7;
         count = 0;
      }
   }

   return;
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

   calculate_cfg();

   foreach_block_and_inst_safe (block, fs_inst, inst, cfg) {
      if (inst->is_control_flow()) {
	 memset(last_mrf_move, 0, sizeof(last_mrf_move));
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF) {
	 fs_inst *prev_inst = last_mrf_move[inst->dst.reg];
	 if (prev_inst && inst->equals(prev_inst)) {
	    inst->remove(block);
	    progress = true;
	    continue;
	 }
      }

      /* Clear out the last-write records for MRFs that were overwritten. */
      if (inst->dst.file == MRF) {
	 last_mrf_move[inst->dst.reg] = NULL;
      }

      if (inst->mlen > 0 && inst->base_mrf != -1) {
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
	  !inst->is_partial_write()) {
	 last_mrf_move[inst->dst.reg] = inst;
      }
   }

   if (progress)
      invalidate_live_intervals(false);

   return progress;
}

static void
clear_deps_for_inst_src(fs_inst *inst, int dispatch_width, bool *deps,
                        int first_grf, int grf_len)
{
   bool inst_simd16 = (dispatch_width > 8 &&
                       !inst->force_uncompressed &&
                       !inst->force_sechalf);

   /* Clear the flag for registers that actually got read (as expected). */
   for (int i = 0; i < inst->sources; i++) {
      int grf;
      if (inst->src[i].file == GRF) {
         grf = inst->src[i].reg;
      } else if (inst->src[i].file == HW_REG &&
                 inst->src[i].fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
         grf = inst->src[i].fixed_hw_reg.nr;
      } else {
         continue;
      }

      if (grf >= first_grf &&
          grf < first_grf + grf_len) {
         deps[grf - first_grf] = false;
         if (inst_simd16)
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
fs_visitor::insert_gen4_pre_send_dependency_workarounds(bblock_t *block,
                                                        fs_inst *inst)
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
        !scan_inst->is_head_sentinel();
        scan_inst = (fs_inst *)scan_inst->prev) {

      /* If we hit control flow, assume that there *are* outstanding
       * dependencies, and force their cleanup before our instruction.
       */
      if (scan_inst->is_control_flow()) {
         for (int i = 0; i < write_len; i++) {
            if (needs_dep[i]) {
               inst->insert_before(block, DEP_RESOLVE_MOV(first_write_grf + i));
            }
         }
         return;
      }

      bool scan_inst_simd16 = (dispatch_width > 8 &&
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
               inst->insert_before(block, DEP_RESOLVE_MOV(reg));
               needs_dep[reg - first_write_grf] = false;
               if (scan_inst_simd16)
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
fs_visitor::insert_gen4_post_send_dependency_workarounds(bblock_t *block, fs_inst *inst)
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
               scan_inst->insert_before(block,
                                        DEP_RESOLVE_MOV(first_write_grf + i));
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
         scan_inst->insert_before(block, DEP_RESOLVE_MOV(scan_inst->dst.reg));
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
         last_inst->insert_before(block, DEP_RESOLVE_MOV(first_write_grf + i));
   }
}

void
fs_visitor::insert_gen4_send_dependency_workarounds()
{
   if (brw->gen != 4 || brw->is_g4x)
      return;

   bool progress = false;

   /* Note that we're done with register allocation, so GRF fs_regs always
    * have a .reg_offset of 0.
    */

   calculate_cfg();

   foreach_block_and_inst(block, fs_inst, inst, cfg) {
      if (inst->mlen != 0 && inst->dst.file == GRF) {
         insert_gen4_pre_send_dependency_workarounds(block, inst);
         insert_gen4_post_send_dependency_workarounds(block, inst);
         progress = true;
      }
   }

   if (progress)
      invalidate_live_intervals(false);
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
   calculate_cfg();

   foreach_block_and_inst (block, fs_inst, inst, cfg) {
      if (inst->opcode != FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD)
         continue;

      if (brw->gen >= 7) {
         /* The offset arg before was a vec4-aligned byte offset.  We need to
          * turn it into a dword offset.
          */
         fs_reg const_offset_reg = inst->src[1];
         assert(const_offset_reg.file == IMM &&
                const_offset_reg.type == BRW_REGISTER_TYPE_UD);
         const_offset_reg.fixed_hw_reg.dw1.ud /= 4;
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
         inst->insert_before(block, setup);

         /* Similarly, this will only populate the first 4 channels of the
          * result register (since we only use smear values from 0-3), but we
          * don't tell the optimizer.
          */
         inst->opcode = FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD_GEN7;
         inst->src[1] = payload;

         invalidate_live_intervals(false);
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

bool
fs_visitor::lower_load_payload()
{
   bool progress = false;

   calculate_cfg();

   foreach_block_and_inst_safe (block, fs_inst, inst, cfg) {
      if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
         fs_reg dst = inst->dst;

         /* src[0] represents the (optional) message header. */
         if (inst->src[0].file != BAD_FILE) {
            inst->insert_before(block, MOV(dst, inst->src[0]));
         }
         dst.reg_offset++;

         for (int i = 1; i < inst->sources; i++) {
            inst->insert_before(block, MOV(dst, inst->src[i]));
            dst.reg_offset++;
         }

         inst->remove(block);
         progress = true;
      }
   }

   if (progress)
      invalidate_live_intervals(false);

   return progress;
}

void
fs_visitor::dump_instructions()
{
   dump_instructions(NULL);
}

void
fs_visitor::dump_instructions(const char *name)
{
   calculate_register_pressure();
   FILE *file = stderr;
   if (name && geteuid() != 0) {
      file = fopen(name, "w");
      if (!file)
         file = stderr;
   }

   int ip = 0, max_pressure = 0;
   foreach_in_list(backend_instruction, inst, &instructions) {
      max_pressure = MAX2(max_pressure, regs_live_at_ip[ip]);
      fprintf(file, "{%3d} %4d: ", regs_live_at_ip[ip], ip);
      dump_instruction(inst, file);
      ++ip;
   }
   fprintf(file, "Maximum %3d registers live at once.\n", max_pressure);

   if (file != stderr) {
      fclose(file);
   }
}

void
fs_visitor::dump_instruction(backend_instruction *be_inst)
{
   dump_instruction(be_inst, stderr);
}

void
fs_visitor::dump_instruction(backend_instruction *be_inst, FILE *file)
{
   fs_inst *inst = (fs_inst *)be_inst;

   if (inst->predicate) {
      fprintf(file, "(%cf0.%d) ",
             inst->predicate_inverse ? '-' : '+',
             inst->flag_subreg);
   }

   fprintf(file, "%s", brw_instruction_name(inst->opcode));
   if (inst->saturate)
      fprintf(file, ".sat");
   if (inst->conditional_mod) {
      fprintf(file, "%s", conditional_modifier[inst->conditional_mod]);
      if (!inst->predicate &&
          (brw->gen < 5 || (inst->opcode != BRW_OPCODE_SEL &&
                              inst->opcode != BRW_OPCODE_IF &&
                              inst->opcode != BRW_OPCODE_WHILE))) {
         fprintf(file, ".f0.%d", inst->flag_subreg);
      }
   }
   fprintf(file, " ");


   switch (inst->dst.file) {
   case GRF:
      fprintf(file, "vgrf%d", inst->dst.reg);
      if (virtual_grf_sizes[inst->dst.reg] != 1 ||
          inst->dst.subreg_offset)
         fprintf(file, "+%d.%d",
                 inst->dst.reg_offset, inst->dst.subreg_offset);
      break;
   case MRF:
      fprintf(file, "m%d", inst->dst.reg);
      break;
   case BAD_FILE:
      fprintf(file, "(null)");
      break;
   case UNIFORM:
      fprintf(file, "***u%d***", inst->dst.reg + inst->dst.reg_offset);
      break;
   case HW_REG:
      if (inst->dst.fixed_hw_reg.file == BRW_ARCHITECTURE_REGISTER_FILE) {
         switch (inst->dst.fixed_hw_reg.nr) {
         case BRW_ARF_NULL:
            fprintf(file, "null");
            break;
         case BRW_ARF_ADDRESS:
            fprintf(file, "a0.%d", inst->dst.fixed_hw_reg.subnr);
            break;
         case BRW_ARF_ACCUMULATOR:
            fprintf(file, "acc%d", inst->dst.fixed_hw_reg.subnr);
            break;
         case BRW_ARF_FLAG:
            fprintf(file, "f%d.%d", inst->dst.fixed_hw_reg.nr & 0xf,
                             inst->dst.fixed_hw_reg.subnr);
            break;
         default:
            fprintf(file, "arf%d.%d", inst->dst.fixed_hw_reg.nr & 0xf,
                               inst->dst.fixed_hw_reg.subnr);
            break;
         }
      } else {
         fprintf(file, "hw_reg%d", inst->dst.fixed_hw_reg.nr);
      }
      if (inst->dst.fixed_hw_reg.subnr)
         fprintf(file, "+%d", inst->dst.fixed_hw_reg.subnr);
      break;
   default:
      fprintf(file, "???");
      break;
   }
   fprintf(file, ":%s, ", brw_reg_type_letters(inst->dst.type));

   for (int i = 0; i < inst->sources && inst->src[i].file != BAD_FILE; i++) {
      if (inst->src[i].negate)
         fprintf(file, "-");
      if (inst->src[i].abs)
         fprintf(file, "|");
      switch (inst->src[i].file) {
      case GRF:
         fprintf(file, "vgrf%d", inst->src[i].reg);
         if (virtual_grf_sizes[inst->src[i].reg] != 1 ||
             inst->src[i].subreg_offset)
            fprintf(file, "+%d.%d", inst->src[i].reg_offset,
                    inst->src[i].subreg_offset);
         break;
      case MRF:
         fprintf(file, "***m%d***", inst->src[i].reg);
         break;
      case UNIFORM:
         fprintf(file, "u%d", inst->src[i].reg + inst->src[i].reg_offset);
         if (inst->src[i].reladdr) {
            fprintf(file, "+reladdr");
         } else if (inst->src[i].subreg_offset) {
            fprintf(file, "+%d.%d", inst->src[i].reg_offset,
                    inst->src[i].subreg_offset);
         }
         break;
      case BAD_FILE:
         fprintf(file, "(null)");
         break;
      case IMM:
         switch (inst->src[i].type) {
         case BRW_REGISTER_TYPE_F:
            fprintf(file, "%ff", inst->src[i].fixed_hw_reg.dw1.f);
            break;
         case BRW_REGISTER_TYPE_D:
            fprintf(file, "%dd", inst->src[i].fixed_hw_reg.dw1.d);
            break;
         case BRW_REGISTER_TYPE_UD:
            fprintf(file, "%uu", inst->src[i].fixed_hw_reg.dw1.ud);
            break;
         default:
            fprintf(file, "???");
            break;
         }
         break;
      case HW_REG:
         if (inst->src[i].fixed_hw_reg.negate)
            fprintf(file, "-");
         if (inst->src[i].fixed_hw_reg.abs)
            fprintf(file, "|");
         if (inst->src[i].fixed_hw_reg.file == BRW_ARCHITECTURE_REGISTER_FILE) {
            switch (inst->src[i].fixed_hw_reg.nr) {
            case BRW_ARF_NULL:
               fprintf(file, "null");
               break;
            case BRW_ARF_ADDRESS:
               fprintf(file, "a0.%d", inst->src[i].fixed_hw_reg.subnr);
               break;
            case BRW_ARF_ACCUMULATOR:
               fprintf(file, "acc%d", inst->src[i].fixed_hw_reg.subnr);
               break;
            case BRW_ARF_FLAG:
               fprintf(file, "f%d.%d", inst->src[i].fixed_hw_reg.nr & 0xf,
                                inst->src[i].fixed_hw_reg.subnr);
               break;
            default:
               fprintf(file, "arf%d.%d", inst->src[i].fixed_hw_reg.nr & 0xf,
                                  inst->src[i].fixed_hw_reg.subnr);
               break;
            }
         } else {
            fprintf(file, "hw_reg%d", inst->src[i].fixed_hw_reg.nr);
         }
         if (inst->src[i].fixed_hw_reg.subnr)
            fprintf(file, "+%d", inst->src[i].fixed_hw_reg.subnr);
         if (inst->src[i].fixed_hw_reg.abs)
            fprintf(file, "|");
         break;
      default:
         fprintf(file, "???");
         break;
      }
      if (inst->src[i].abs)
         fprintf(file, "|");

      if (inst->src[i].file != IMM) {
         fprintf(file, ":%s", brw_reg_type_letters(inst->src[i].type));
      }

      if (i < inst->sources - 1 && inst->src[i + 1].file != BAD_FILE)
         fprintf(file, ", ");
   }

   fprintf(file, " ");

   if (inst->force_uncompressed)
      fprintf(file, "1sthalf ");

   if (inst->force_sechalf)
      fprintf(file, "2ndhalf ");

   fprintf(file, "\n");
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
					   const fs_reg &reg)
{
   if (end == start ||
       end->is_partial_write() ||
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
   bool uses_depth =
      (prog->InputsRead & (1 << VARYING_SLOT_POS)) != 0;
   unsigned barycentric_interp_modes =
      (stage == MESA_SHADER_FRAGMENT) ?
      ((brw_wm_prog_data*) this->prog_data)->barycentric_interp_modes : 0;

   assert(brw->gen >= 6);

   /* R0-1: masks, pixel X/Y coordinates. */
   payload.num_regs = 2;
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
         payload.barycentric_coord_reg[i] = payload.num_regs;
         payload.num_regs += 2;
         if (dispatch_width == 16) {
            payload.num_regs += 2;
         }
      }
   }

   /* R27: interpolated depth if uses source depth */
   if (uses_depth) {
      payload.source_depth_reg = payload.num_regs;
      payload.num_regs++;
      if (dispatch_width == 16) {
         /* R28: interpolated depth if not SIMD8. */
         payload.num_regs++;
      }
   }
   /* R29: interpolated W set if GEN6_WM_USES_SOURCE_W. */
   if (uses_depth) {
      payload.source_w_reg = payload.num_regs;
      payload.num_regs++;
      if (dispatch_width == 16) {
         /* R30: interpolated W if not SIMD8. */
         payload.num_regs++;
      }
   }

   if (stage == MESA_SHADER_FRAGMENT) {
      brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;
      brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;
      prog_data->uses_pos_offset = key->compute_pos_offset;
      /* R31: MSAA position offsets. */
      if (prog_data->uses_pos_offset) {
         payload.sample_pos_reg = payload.num_regs;
         payload.num_regs++;
      }
   }

   /* R32: MSAA input coverage mask */
   if (prog->SystemValuesRead & SYSTEM_BIT_SAMPLE_MASK_IN) {
      assert(brw->gen >= 7);
      payload.sample_mask_in_reg = payload.num_regs;
      payload.num_regs++;
      if (dispatch_width == 16) {
         /* R33: input coverage mask if not SIMD8. */
         payload.num_regs++;
      }
   }

   /* R34-: bary for 32-pixel. */
   /* R58-59: interp W for 32-pixel. */

   if (prog->OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      source_depth_to_render_target = true;
   }
}

void
fs_visitor::assign_binding_table_offsets()
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;
   uint32_t next_binding_table_offset = 0;

   /* If there are no color regions, we still perform an FB write to a null
    * renderbuffer, which we place at surface index 0.
    */
   prog_data->binding_table.render_target_start = next_binding_table_offset;
   next_binding_table_offset += MAX2(key->nr_color_regions, 1);

   assign_common_binding_table_offsets(next_binding_table_offset);
}

void
fs_visitor::calculate_register_pressure()
{
   invalidate_live_intervals(false);
   calculate_live_intervals();

   unsigned num_instructions = instructions.length();

   regs_live_at_ip = rzalloc_array(mem_ctx, int, num_instructions);

   for (int reg = 0; reg < virtual_grf_count; reg++) {
      for (int ip = virtual_grf_start[reg]; ip <= virtual_grf_end[reg]; ip++)
         regs_live_at_ip[ip] += virtual_grf_sizes[reg];
   }
}

/**
 * Look for repeated FS_OPCODE_MOV_DISPATCH_TO_FLAGS and drop the later ones.
 *
 * The needs_unlit_centroid_workaround ends up producing one of these per
 * channel of centroid input, so it's good to clean them up.
 *
 * An assumption here is that nothing ever modifies the dispatched pixels
 * value that FS_OPCODE_MOV_DISPATCH_TO_FLAGS reads from, but the hardware
 * dictates that anyway.
 */
void
fs_visitor::opt_drop_redundant_mov_to_flags()
{
   bool flag_mov_found[2] = {false};

   foreach_block_and_inst_safe(block, fs_inst, inst, cfg) {
      if (inst->is_control_flow()) {
         memset(flag_mov_found, 0, sizeof(flag_mov_found));
      } else if (inst->opcode == FS_OPCODE_MOV_DISPATCH_TO_FLAGS) {
         if (!flag_mov_found[inst->flag_subreg])
            flag_mov_found[inst->flag_subreg] = true;
         else
            inst->remove(block);
      } else if (inst->writes_flag()) {
         flag_mov_found[inst->flag_subreg] = false;
      }
   }
}

bool
fs_visitor::run()
{
   sanity_param_count = prog->Parameters->NumParameters;
   bool allocated_without_spills;

   assign_binding_table_offsets();

   if (brw->gen >= 6)
      setup_payload_gen6();
   else
      setup_payload_gen4();

   if (0) {
      emit_dummy_fs();
   } else {
      if (INTEL_DEBUG & DEBUG_SHADER_TIME)
         emit_shader_time_begin();

      calculate_urb_setup();
      if (prog->InputsRead > 0) {
         if (brw->gen < 6)
            emit_interpolation_setup_gen4();
         else
            emit_interpolation_setup_gen6();
      }

      /* We handle discards by keeping track of the still-live pixels in f0.1.
       * Initialize it with the dispatched pixels.
       */
      bool uses_kill =
         (stage == MESA_SHADER_FRAGMENT) &&
         ((brw_wm_prog_data*) this->prog_data)->uses_kill;
      bool alpha_test_func =
         (stage == MESA_SHADER_FRAGMENT) &&
         ((brw_wm_prog_key*) this->key)->alpha_test_func;
      if (uses_kill || alpha_test_func) {
         fs_inst *discard_init = emit(FS_OPCODE_MOV_DISPATCH_TO_FLAGS);
         discard_init->flag_subreg = 1;
      }

      /* Generate FS IR for main().  (the visitor only descends into
       * functions called "main").
       */
      if (shader) {
         foreach_in_list(ir_instruction, ir, shader->base.ir) {
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

      if (alpha_test_func)
         emit_alpha_test();

      emit_fb_writes();

      split_virtual_grfs();

      move_uniform_array_access_to_pull_constants();
      assign_constant_locations();
      demote_pull_constants();

      opt_drop_redundant_mov_to_flags();

#define OPT(pass, args...) do {                                            \
      pass_num++;                                                          \
      bool this_progress = pass(args);                                     \
                                                                           \
      if (unlikely(INTEL_DEBUG & DEBUG_OPTIMIZER) && this_progress) {      \
         char filename[64];                                                \
         snprintf(filename, 64, "fs%d-%04d-%02d-%02d-" #pass,              \
                  dispatch_width, shader_prog ? shader_prog->Name : 0, iteration, pass_num); \
                                                                           \
         backend_visitor::dump_instructions(filename);                     \
      }                                                                    \
                                                                           \
      progress = progress || this_progress;                                \
   } while (false)

      if (unlikely(INTEL_DEBUG & DEBUG_OPTIMIZER)) {
         char filename[64];
         snprintf(filename, 64, "fs%d-%04d-00-start",
                  dispatch_width, shader_prog ? shader_prog->Name : 0);

         backend_visitor::dump_instructions(filename);
      }

      bool progress;
      int iteration = 0;
      do {
	 progress = false;
         iteration++;
         int pass_num = 0;

         compact_virtual_grfs();

         OPT(remove_duplicate_mrf_writes);

         OPT(opt_algebraic);
         OPT(opt_cse);
         OPT(opt_copy_propagate);
         OPT(opt_peephole_predicated_break);
         OPT(dead_code_eliminate);
         OPT(opt_peephole_sel);
         OPT(dead_control_flow_eliminate, this);
         OPT(opt_register_renaming);
         OPT(opt_saturate_propagation);
         OPT(register_coalesce);
         OPT(compute_to_mrf);
      } while (progress);

      if (lower_load_payload()) {
         register_coalesce();
         dead_code_eliminate();
      }

      lower_uniform_pull_constant_loads();

      assign_curb_setup();
      assign_urb_setup();

      static enum instruction_scheduler_mode pre_modes[] = {
         SCHEDULE_PRE,
         SCHEDULE_PRE_NON_LIFO,
         SCHEDULE_PRE_LIFO,
      };

      /* Try each scheduling heuristic to see if it can successfully register
       * allocate without spilling.  They should be ordered by decreasing
       * performance but increasing likelihood of allocating.
       */
      for (unsigned i = 0; i < ARRAY_SIZE(pre_modes); i++) {
         schedule_instructions(pre_modes[i]);

         if (0) {
            assign_regs_trivial();
            allocated_without_spills = true;
         } else {
            allocated_without_spills = assign_regs(false);
         }
         if (allocated_without_spills)
            break;
      }

      if (!allocated_without_spills) {
         /* We assume that any spilling is worse than just dropping back to
          * SIMD8.  There's probably actually some intermediate point where
          * SIMD16 with a couple of spills is still better.
          */
         if (dispatch_width == 16) {
            fail("Failure to register allocate.  Reduce number of "
                 "live scalar values to avoid this.");
         } else {
            perf_debug("Fragment shader triggered register spilling.  "
                       "Try reducing the number of live scalar values to "
                       "improve performance.\n");
         }

         /* Since we're out of heuristics, just go spill registers until we
          * get an allocation.
          */
         while (!assign_regs(true)) {
            if (failed)
               break;
         }
      }
   }
   assert(force_uncompressed_stack == 0);

   /* This must come after all optimization and register allocation, since
    * it inserts dead code that happens to have side effects, and it does
    * so based on the actual physical registers in use.
    */
   insert_gen4_send_dependency_workarounds();

   if (failed)
      return false;

   if (!allocated_without_spills)
      schedule_instructions(SCHEDULE_POST);

   if (last_scratch > 0) {
      prog_data->total_scratch = brw_get_scratch_size(last_scratch);
   }

   if (brw->use_rep_send)
      try_rep_send();

   if (stage == MESA_SHADER_FRAGMENT) {
      brw_wm_prog_data *prog_data = (brw_wm_prog_data*) this->prog_data;
      if (dispatch_width == 8)
         prog_data->reg_blocks = brw_register_blocks(grf_used);
      else
         prog_data->reg_blocks_16 = brw_register_blocks(grf_used);
   }

   /* If any state parameters were appended, then ParameterValues could have
    * been realloced, in which case the driver uniform storage set up by
    * _mesa_associate_uniform_storage() would point to freed memory.  Make
    * sure that didn't happen.
    */
   assert(sanity_param_count == prog->Parameters->NumParameters);

   calculate_cfg();

   return !failed;
}

const unsigned *
brw_wm_fs_emit(struct brw_context *brw,
               void *mem_ctx,
               const struct brw_wm_prog_key *key,
               struct brw_wm_prog_data *prog_data,
               struct gl_fragment_program *fp,
               struct gl_shader_program *prog,
               unsigned *final_assembly_size)
{
   bool start_busy = false;
   double start_time = 0;

   if (unlikely(brw->perf_debug)) {
      start_busy = (brw->batch.last_bo &&
                    drm_intel_bo_busy(brw->batch.last_bo));
      start_time = get_time();
   }

   struct brw_shader *shader = NULL;
   if (prog)
      shader = (brw_shader *) prog->_LinkedShaders[MESA_SHADER_FRAGMENT];

   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      brw_dump_ir(brw, "fragment", prog, &shader->base, &fp->Base);

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   fs_visitor v(brw, mem_ctx, key, prog_data, prog, fp, 8);
   if (!v.run()) {
      if (prog) {
         prog->LinkStatus = false;
         ralloc_strcat(&prog->InfoLog, v.fail_msg);
      }

      _mesa_problem(NULL, "Failed to compile fragment shader: %s\n",
                    v.fail_msg);

      return NULL;
   }

   cfg_t *simd16_cfg = NULL;
   fs_visitor v2(brw, mem_ctx, key, prog_data, prog, fp, 16);
   if (brw->gen >= 5 && likely(!(INTEL_DEBUG & DEBUG_NO16) ||
                               brw->use_rep_send)) {
      if (!v.simd16_unsupported) {
         /* Try a SIMD16 compile */
         v2.import_uniforms(&v);
         if (!v2.run()) {
            perf_debug("SIMD16 shader failed to compile, falling back to "
                       "SIMD8 at a 10-20%% performance cost: %s", v2.fail_msg);
         } else {
            simd16_cfg = v2.cfg;
         }
      } else {
         perf_debug("SIMD16 shader unsupported, falling back to "
                    "SIMD8 at a 10-20%% performance cost: %s", v.no16_msg);
      }
   }

   cfg_t *simd8_cfg;
   int no_simd8 = (INTEL_DEBUG & DEBUG_NO8) || brw->no_simd8;
   if (no_simd8 && simd16_cfg) {
      simd8_cfg = NULL;
      prog_data->no_8 = true;
   } else {
      simd8_cfg = v.cfg;
      prog_data->no_8 = false;
   }

   const unsigned *assembly = NULL;
   fs_generator g(brw, mem_ctx, key, prog_data, prog, fp,
                  v.runtime_check_aads_emit, INTEL_DEBUG & DEBUG_WM);
   assembly = g.generate_assembly(simd8_cfg, simd16_cfg,
                                  final_assembly_size);

   if (unlikely(brw->perf_debug) && shader) {
      if (shader->compiled_once)
         brw_wm_debug_recompile(brw, prog, key);
      shader->compiled_once = true;

      if (start_busy && !drm_intel_bo_busy(brw->batch.last_bo)) {
         perf_debug("FS compile took %.03f ms and stalled the GPU\n",
                    (get_time() - start_time) * 1000);
      }
   }

   return assembly;
}

bool
brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_wm_prog_key key;

   if (!prog->_LinkedShaders[MESA_SHADER_FRAGMENT])
      return true;

   struct gl_fragment_program *fp = (struct gl_fragment_program *)
      prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;
   struct brw_fragment_program *bfp = brw_fragment_program(fp);
   bool program_uses_dfdy = fp->UsesDFdy;

   memset(&key, 0, sizeof(key));

   if (brw->gen < 6) {
      if (fp->UsesKill)
         key.iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

      if (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
         key.iz_lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

      /* Just assume depth testing. */
      key.iz_lookup |= IZ_DEPTH_TEST_ENABLE_BIT;
      key.iz_lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;
   }

   if (brw->gen < 6 || _mesa_bitcount_64(fp->Base.InputsRead &
                                         BRW_FS_VARYING_INPUT_MASK) > 16)
      key.input_slots_valid = fp->Base.InputsRead | VARYING_BIT_POS;

   unsigned sampler_count = _mesa_fls(fp->Base.SamplersUsed);
   for (unsigned i = 0; i < sampler_count; i++) {
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

   key.nr_color_regions = _mesa_bitcount_64(fp->Base.OutputsWritten &
         ~(BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
         BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)));

   if ((fp->Base.InputsRead & VARYING_BIT_POS) || program_uses_dfdy) {
      key.render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer) ||
                          key.nr_color_regions > 1;
   }

   /* GL_FRAGMENT_SHADER_DERIVATIVE_HINT is almost always GL_DONT_CARE.  The
    * quality of the derivatives is likely to be determined by the driconf
    * option.
    */
   key.high_quality_derivatives = brw->disable_derivative_optimization;

   key.program_string_id = bfp->id;

   uint32_t old_prog_offset = brw->wm.base.prog_offset;
   struct brw_wm_prog_data *old_prog_data = brw->wm.prog_data;

   bool success = do_wm_prog(brw, prog, bfp, &key);

   brw->wm.base.prog_offset = old_prog_offset;
   brw->wm.prog_data = old_prog_data;

   return success;
}
