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
#include "talloc.h"
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
   FS_OPCODE_LINTERP,
   FS_OPCODE_TEX,
   FS_OPCODE_TXB,
   FS_OPCODE_TXL,
   FS_OPCODE_DISCARD_NOT,
   FS_OPCODE_DISCARD_AND,
};


class fs_reg {
public:
   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = talloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init()
   {
      this->reg = 0;
      this->reg_offset = 0;
      this->negate = 0;
      this->abs = 0;
      this->hw_reg = -1;
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
   fs_reg(class fs_visitor *v, const struct glsl_type *type);

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
   struct brw_reg fixed_hw_reg;

   /** Value for file == BRW_IMMMEDIATE_FILE */
   union {
      int32_t i;
      uint32_t u;
      float f;
   } imm;
};

class fs_inst : public exec_node {
public:
   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = talloc_zero_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init()
   {
      this->opcode = BRW_OPCODE_NOP;
      this->saturate = false;
      this->conditional_mod = BRW_CONDITIONAL_NONE;
      this->predicated = false;
      this->sampler = 0;
      this->target = 0;
      this->eot = false;
      this->header_present = false;
      this->shadow_compare = false;
      this->mlen = 0;
      this->base_mrf = 0;
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
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
      this->src[1] = src1;
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1, fs_reg src2)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
      this->src[1] = src1;
      this->src[2] = src2;
   }

   int opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   fs_reg dst;
   fs_reg src[3];
   bool saturate;
   bool predicated;
   int conditional_mod; /**< BRW_CONDITIONAL_* */

   int mlen; /**< SEND message length */
   int base_mrf; /**< First MRF in the SEND message, if mlen is nonzero. */
   int sampler;
   int target; /**< MRT target. */
   bool eot;
   bool header_present;
   bool shadow_compare;

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

   fs_visitor(struct brw_wm_compile *c, struct brw_shader *shader)
   {
      this->c = c;
      this->p = &c->func;
      this->brw = p->brw;
      this->fp = brw->fragment_program;
      this->intel = &brw->intel;
      this->ctx = &intel->ctx;
      this->mem_ctx = talloc_new(NULL);
      this->shader = shader;
      this->fail = false;
      this->variable_ht = hash_table_ctor(0,
					  hash_table_pointer_hash,
					  hash_table_pointer_compare);

      this->frag_color = NULL;
      this->frag_data = NULL;
      this->frag_depth = NULL;
      this->first_non_payload_grf = 0;

      this->current_annotation = NULL;
      this->annotation_string = NULL;
      this->annotation_ir = NULL;
      this->base_ir = NULL;

      this->virtual_grf_sizes = NULL;
      this->virtual_grf_next = 1;
      this->virtual_grf_array_size = 0;
      this->virtual_grf_def = NULL;
      this->virtual_grf_use = NULL;

      this->kill_emitted = false;
   }

   ~fs_visitor()
   {
      talloc_free(this->mem_ctx);
      hash_table_dtor(this->variable_ht);
   }

   fs_reg *variable_storage(ir_variable *var);
   int virtual_grf_alloc(int size);

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

   fs_inst *emit(fs_inst inst);
   void assign_curb_setup();
   void calculate_urb_setup();
   void assign_urb_setup();
   void assign_regs();
   void assign_regs_trivial();
   void calculate_live_intervals();
   bool propagate_constants();
   bool register_coalesce();
   bool dead_code_eliminate();
   bool virtual_grf_interferes(int a, int b);
   void generate_code();
   void generate_fb_write(fs_inst *inst);
   void generate_linterp(fs_inst *inst, struct brw_reg dst,
			 struct brw_reg *src);
   void generate_tex(fs_inst *inst, struct brw_reg dst);
   void generate_math(fs_inst *inst, struct brw_reg dst, struct brw_reg *src);
   void generate_discard_not(fs_inst *inst, struct brw_reg temp);
   void generate_discard_and(fs_inst *inst, struct brw_reg temp);
   void generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src);

   void emit_dummy_fs();
   fs_reg *emit_fragcoord_interpolation(ir_variable *ir);
   fs_reg *emit_frontfacing_interpolation(ir_variable *ir);
   fs_reg *emit_general_interpolation(ir_variable *ir);
   void emit_interpolation_setup_gen4();
   void emit_interpolation_setup_gen6();
   fs_inst *emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate);
   fs_inst *emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate);
   fs_inst *emit_math(fs_opcodes op, fs_reg dst, fs_reg src0);
   fs_inst *emit_math(fs_opcodes op, fs_reg dst, fs_reg src0, fs_reg src1);

   void emit_fb_writes();
   void emit_assignment_writes(fs_reg &l, fs_reg &r,
			       const glsl_type *type, bool predicated);

   struct brw_reg interp_reg(int location, int channel);
   int setup_uniform_values(int loc, const glsl_type *type);
   void setup_builtin_uniform_values(ir_variable *ir);

   struct brw_context *brw;
   const struct gl_fragment_program *fp;
   struct intel_context *intel;
   GLcontext *ctx;
   struct brw_wm_compile *c;
   struct brw_compile *p;
   struct brw_shader *shader;
   void *mem_ctx;
   exec_list instructions;

   int *virtual_grf_sizes;
   int virtual_grf_next;
   int virtual_grf_array_size;
   int *virtual_grf_def;
   int *virtual_grf_use;

   struct hash_table *variable_ht;
   ir_variable *frag_color, *frag_data, *frag_depth;
   int first_non_payload_grf;
   int urb_setup[FRAG_ATTRIB_MAX];
   bool kill_emitted;

   /** @{ debug annotation info */
   const char *current_annotation;
   ir_instruction *base_ir;
   const char **annotation_string;
   ir_instruction **annotation_ir;
   /** @} */

   bool fail;

   /* Result of last visit() method. */
   fs_reg result;

   fs_reg pixel_x;
   fs_reg pixel_y;
   fs_reg wpos_w;
   fs_reg pixel_w;
   fs_reg delta_x;
   fs_reg delta_y;

   int grf_used;
};

GLboolean brw_do_channel_expressions(struct exec_list *instructions);
GLboolean brw_do_vector_splitting(struct exec_list *instructions);
