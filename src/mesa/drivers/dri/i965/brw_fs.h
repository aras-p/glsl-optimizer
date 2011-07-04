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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
}
#include "../glsl/glsl_types.h"
#include "../glsl/ir.h"

enum register_file {
   ARF = BRW_ARCHITECTURE_REGISTER_FILE,
   GRF = BRW_GENERAL_REGISTER_FILE,
   MRF = BRW_MESSAGE_REGISTER_FILE,
   IMM = BRW_IMMEDIATE_VALUE,
   FIXED_HW_REG, /* a struct brw_reg */
   UNIFORM, /* prog_data->params[hw_reg] */
   BAD_FILE
};

enum fs_opcodes {
   FS_OPCODE_FB_WRITE = 256,
   FS_OPCODE_RCP,
   FS_OPCODE_RSQ,
   FS_OPCODE_SQRT,
   FS_OPCODE_EXP2,
   FS_OPCODE_LOG2,
   FS_OPCODE_POW,
   FS_OPCODE_SIN,
   FS_OPCODE_COS,
   FS_OPCODE_DDX,
   FS_OPCODE_DDY,
   FS_OPCODE_PIXEL_X,
   FS_OPCODE_PIXEL_Y,
   FS_OPCODE_CINTERP,
   FS_OPCODE_LINTERP,
   FS_OPCODE_TEX,
   FS_OPCODE_TXB,
   FS_OPCODE_TXD,
   FS_OPCODE_TXL,
   FS_OPCODE_DISCARD,
   FS_OPCODE_SPILL,
   FS_OPCODE_UNSPILL,
   FS_OPCODE_PULL_CONSTANT_LOAD,
};


class fs_reg {
public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = ralloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init()
   {
      memset(this, 0, sizeof(*this));
      this->hw_reg = -1;
      this->smear = -1;
   }

   /** Generic unset register constructor. */
   fs_reg()
   {
      init();
      this->file = BAD_FILE;
   }

   /** Immediate value constructor. */
   fs_reg(float f)
   {
      init();
      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_F;
      this->imm.f = f;
   }

   /** Immediate value constructor. */
   fs_reg(int32_t i)
   {
      init();
      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_D;
      this->imm.i = i;
   }

   /** Immediate value constructor. */
   fs_reg(uint32_t u)
   {
      init();
      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_UD;
      this->imm.u = u;
   }

   /** Fixed brw_reg Immediate value constructor. */
   fs_reg(struct brw_reg fixed_hw_reg)
   {
      init();
      this->file = FIXED_HW_REG;
      this->fixed_hw_reg = fixed_hw_reg;
      this->type = fixed_hw_reg.type;
   }

   fs_reg(enum register_file file, int hw_reg);
   fs_reg(enum register_file file, int hw_reg, uint32_t type);
   fs_reg(class fs_visitor *v, const struct glsl_type *type);

   bool equals(fs_reg *r)
   {
      return (file == r->file &&
	      reg == r->reg &&
	      reg_offset == r->reg_offset &&
	      hw_reg == r->hw_reg &&
	      type == r->type &&
	      negate == r->negate &&
	      abs == r->abs &&
	      memcmp(&fixed_hw_reg, &r->fixed_hw_reg,
		     sizeof(fixed_hw_reg)) == 0 &&
	      smear == r->smear &&
	      imm.u == r->imm.u);
   }

   /** Register file: ARF, GRF, MRF, IMM. */
   enum register_file file;
   /** virtual register number.  0 = fixed hw reg */
   int reg;
   /** Offset within the virtual register. */
   int reg_offset;
   /** HW register number.  Generally unset until register allocation. */
   int hw_reg;
   /** Register type.  BRW_REGISTER_TYPE_* */
   int type;
   bool negate;
   bool abs;
   bool sechalf;
   struct brw_reg fixed_hw_reg;
   int smear; /* -1, or a channel of the reg to smear to all channels. */

   /** Value for file == BRW_IMMMEDIATE_FILE */
   union {
      int32_t i;
      uint32_t u;
      float f;
   } imm;
};

static const fs_reg reg_undef;
static const fs_reg reg_null_f(ARF, BRW_ARF_NULL, BRW_REGISTER_TYPE_F);
static const fs_reg reg_null_d(ARF, BRW_ARF_NULL, BRW_REGISTER_TYPE_D);

class fs_inst : public exec_node {
public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init()
   {
      memset(this, 0, sizeof(*this));
      this->opcode = BRW_OPCODE_NOP;
      this->conditional_mod = BRW_CONDITIONAL_NONE;

      this->dst = reg_undef;
      this->src[0] = reg_undef;
      this->src[1] = reg_undef;
      this->src[2] = reg_undef;
   }

   fs_inst()
   {
      init();
   }

   fs_inst(int opcode)
   {
      init();
      this->opcode = opcode;
   }

   fs_inst(int opcode, fs_reg dst)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;

      if (dst.file == GRF)
	 assert(dst.reg_offset >= 0);
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0)
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

   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1)
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

   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1, fs_reg src2)
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

   bool equals(fs_inst *inst)
   {
      return (opcode == inst->opcode &&
	      dst.equals(&inst->dst) &&
	      src[0].equals(&inst->src[0]) &&
	      src[1].equals(&inst->src[1]) &&
	      src[2].equals(&inst->src[2]) &&
	      saturate == inst->saturate &&
	      predicated == inst->predicated &&
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

   bool is_tex()
   {
      return (opcode == FS_OPCODE_TEX ||
	      opcode == FS_OPCODE_TXB ||
	      opcode == FS_OPCODE_TXD ||
	      opcode == FS_OPCODE_TXL);
   }

   bool is_math()
   {
      return (opcode == FS_OPCODE_RCP ||
	      opcode == FS_OPCODE_RSQ ||
	      opcode == FS_OPCODE_SQRT ||
	      opcode == FS_OPCODE_EXP2 ||
	      opcode == FS_OPCODE_LOG2 ||
	      opcode == FS_OPCODE_SIN ||
	      opcode == FS_OPCODE_COS ||
	      opcode == FS_OPCODE_POW);
   }

   int opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   fs_reg dst;
   fs_reg src[3];
   bool saturate;
   bool predicated;
   bool predicate_inverse;
   int conditional_mod; /**< BRW_CONDITIONAL_* */

   int mlen; /**< SEND message length */
   int base_mrf; /**< First MRF in the SEND message, if mlen is nonzero. */
   int sampler;
   int target; /**< MRT target. */
   bool eot;
   bool header_present;
   bool shadow_compare;
   bool force_uncompressed;
   bool force_sechalf;
   uint32_t offset; /* spill/unspill offset */

   /** @{
    * Annotation for the generated IR.  One of the two can be set.
    */
   ir_instruction *ir;
   const char *annotation;
   /** @} */
};

class fs_visitor : public ir_visitor
{
public:

   fs_visitor(struct brw_wm_compile *c, struct gl_shader_program *prog,
	      struct brw_shader *shader)
   {
      this->c = c;
      this->p = &c->func;
      this->brw = p->brw;
      this->fp = prog->FragmentProgram;
      this->prog = prog;
      this->intel = &brw->intel;
      this->ctx = &intel->ctx;
      this->mem_ctx = ralloc_context(NULL);
      this->shader = shader;
      this->failed = false;
      this->variable_ht = hash_table_ctor(0,
					  hash_table_pointer_hash,
					  hash_table_pointer_compare);

      /* There's a question that appears to be left open in the spec:
       * How do implicit dst conversions interact with the CMP
       * instruction or conditional mods?  On gen6, the instruction:
       *
       * CMP null<d> src0<f> src1<f>
       *
       * will do src1 - src0 and compare that result as if it was an
       * integer.  On gen4, it will do src1 - src0 as float, convert
       * the result to int, and compare as int.  In between, it
       * appears that it does src1 - src0 and does the compare in the
       * execution type so dst type doesn't matter.
       */
      if (this->intel->gen > 4)
	 this->reg_null_cmp = reg_null_d;
      else
	 this->reg_null_cmp = reg_null_f;

      this->frag_color = NULL;
      this->frag_data = NULL;
      this->frag_depth = NULL;
      this->first_non_payload_grf = 0;

      this->current_annotation = NULL;
      this->base_ir = NULL;

      this->virtual_grf_sizes = NULL;
      this->virtual_grf_next = 1;
      this->virtual_grf_array_size = 0;
      this->virtual_grf_def = NULL;
      this->virtual_grf_use = NULL;
      this->live_intervals_valid = false;

      this->kill_emitted = false;
      this->force_uncompressed_stack = 0;
      this->force_sechalf_stack = 0;
   }

   ~fs_visitor()
   {
      ralloc_free(this->mem_ctx);
      hash_table_dtor(this->variable_ht);
   }

   fs_reg *variable_storage(ir_variable *var);
   int virtual_grf_alloc(int size);
   void import_uniforms(struct hash_table *src_variable_ht);

   void visit(ir_variable *ir);
   void visit(ir_assignment *ir);
   void visit(ir_dereference_variable *ir);
   void visit(ir_dereference_record *ir);
   void visit(ir_dereference_array *ir);
   void visit(ir_expression *ir);
   void visit(ir_texture *ir);
   void visit(ir_if *ir);
   void visit(ir_constant *ir);
   void visit(ir_swizzle *ir);
   void visit(ir_return *ir);
   void visit(ir_loop *ir);
   void visit(ir_loop_jump *ir);
   void visit(ir_discard *ir);
   void visit(ir_call *ir);
   void visit(ir_function *ir);
   void visit(ir_function_signature *ir);

   void swizzle_result(ir_texture *ir, fs_reg orig_val, int sampler);

   fs_inst *emit(fs_inst inst);

   fs_inst *emit(int opcode)
   {
      return emit(fs_inst(opcode));
   }

   fs_inst *emit(int opcode, fs_reg dst)
   {
      return emit(fs_inst(opcode, dst));
   }

   fs_inst *emit(int opcode, fs_reg dst, fs_reg src0)
   {
      return emit(fs_inst(opcode, dst, src0));
   }

   fs_inst *emit(int opcode, fs_reg dst, fs_reg src0, fs_reg src1)
   {
      return emit(fs_inst(opcode, dst, src0, src1));
   }

   fs_inst *emit(int opcode, fs_reg dst, fs_reg src0, fs_reg src1, fs_reg src2)
   {
      return emit(fs_inst(opcode, dst, src0, src1, src2));
   }

   int type_size(const struct glsl_type *type);

   bool run();
   void setup_paramvalues_refs();
   void assign_curb_setup();
   void calculate_urb_setup();
   void assign_urb_setup();
   bool assign_regs();
   void assign_regs_trivial();
   int choose_spill_reg(struct ra_graph *g);
   void spill_reg(int spill_reg);
   void split_virtual_grfs();
   void setup_pull_constants();
   void calculate_live_intervals();
   bool propagate_constants();
   bool register_coalesce();
   bool compute_to_mrf();
   bool dead_code_eliminate();
   bool remove_duplicate_mrf_writes();
   bool virtual_grf_interferes(int a, int b);
   void schedule_instructions();
   void fail(const char *msg, ...);

   void push_force_uncompressed();
   void pop_force_uncompressed();
   void push_force_sechalf();
   void pop_force_sechalf();

   void generate_code();
   void generate_fb_write(fs_inst *inst);
   void generate_pixel_xy(struct brw_reg dst, bool is_x);
   void generate_linterp(fs_inst *inst, struct brw_reg dst,
			 struct brw_reg *src);
   void generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_math(fs_inst *inst, struct brw_reg dst, struct brw_reg *src);
   void generate_discard(fs_inst *inst);
   void generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_spill(fs_inst *inst, struct brw_reg src);
   void generate_unspill(fs_inst *inst, struct brw_reg dst);
   void generate_pull_constant_load(fs_inst *inst, struct brw_reg dst);

   void emit_dummy_fs();
   fs_reg *emit_fragcoord_interpolation(ir_variable *ir);
   fs_reg *emit_frontfacing_interpolation(ir_variable *ir);
   fs_reg *emit_general_interpolation(ir_variable *ir);
   void emit_interpolation_setup_gen4();
   void emit_interpolation_setup_gen6();
   fs_inst *emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      int sampler);
   fs_inst *emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      int sampler);
   fs_inst *emit_texture_gen7(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      int sampler);
   fs_inst *emit_math(fs_opcodes op, fs_reg dst, fs_reg src0);
   fs_inst *emit_math(fs_opcodes op, fs_reg dst, fs_reg src0, fs_reg src1);
   bool try_emit_saturate(ir_expression *ir);
   void emit_bool_to_cond_code(ir_rvalue *condition);
   void emit_if_gen6(ir_if *ir);
   void emit_unspill(fs_inst *inst, fs_reg reg, uint32_t spill_offset);

   void emit_color_write(int index, int first_color_mrf, fs_reg color);
   void emit_fb_writes();
   void emit_assignment_writes(fs_reg &l, fs_reg &r,
			       const glsl_type *type, bool predicated);

   struct brw_reg interp_reg(int location, int channel);
   int setup_uniform_values(int loc, const glsl_type *type);
   void setup_builtin_uniform_values(ir_variable *ir);
   int implied_mrf_writes(fs_inst *inst);

   struct brw_context *brw;
   const struct gl_fragment_program *fp;
   struct intel_context *intel;
   struct gl_context *ctx;
   struct brw_wm_compile *c;
   struct brw_compile *p;
   struct brw_shader *shader;
   struct gl_shader_program *prog;
   void *mem_ctx;
   exec_list instructions;

   /* Delayed setup of c->prog_data.params[] due to realloc of
    * ParamValues[] during compile.
    */
   int param_index[MAX_UNIFORMS * 4];
   int param_offset[MAX_UNIFORMS * 4];

   int *virtual_grf_sizes;
   int virtual_grf_next;
   int virtual_grf_array_size;
   int *virtual_grf_def;
   int *virtual_grf_use;
   bool live_intervals_valid;

   struct hash_table *variable_ht;
   ir_variable *frag_color, *frag_data, *frag_depth;
   int first_non_payload_grf;
   int urb_setup[FRAG_ATTRIB_MAX];
   bool kill_emitted;

   /** @{ debug annotation info */
   const char *current_annotation;
   ir_instruction *base_ir;
   /** @} */

   bool failed;
   char *fail_msg;

   /* On entry to a visit() method, this is the storage for the
    * result.  On exit, the visit() called may have changed it, in
    * which case the parent must use the new storage instead.
    */
   fs_reg result;

   fs_reg pixel_x;
   fs_reg pixel_y;
   fs_reg wpos_w;
   fs_reg pixel_w;
   fs_reg delta_x;
   fs_reg delta_y;
   fs_reg reg_null_cmp;

   int grf_used;

   int force_uncompressed_stack;
   int force_sechalf_stack;
};

GLboolean brw_do_channel_expressions(struct exec_list *instructions);
GLboolean brw_do_vector_splitting(struct exec_list *instructions);
bool brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog);
