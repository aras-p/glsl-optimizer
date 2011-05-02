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

#ifndef BRW_VEC4_H
#define BRW_VEC4_H

#include <stdint.h>
#include "brw_shader.h"
#include "main/compiler.h"
#include "program/hash_table.h"

extern "C" {
#include "brw_vs.h"
#include "brw_context.h"
#include "brw_eu.h"
};

#include "../glsl/ir.h"

namespace brw {

class dst_reg;

/**
 * Common helper for constructing swizzles.  When only a subset of
 * channels of a vec4 are used, we don't want to reference the other
 * channels, as that will tell optimization passes that those other
 * channels are used.
 */
static int
swizzle_for_size(int size)
{
   int size_swizzles[4] = {
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z),
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
   };

   assert((size >= 1) && (size <= 4));
   return size_swizzles[size - 1];
}

enum register_file {
   ARF = BRW_ARCHITECTURE_REGISTER_FILE,
   GRF = BRW_GENERAL_REGISTER_FILE,
   MRF = BRW_MESSAGE_REGISTER_FILE,
   IMM = BRW_IMMEDIATE_VALUE,
   HW_REG, /* a struct brw_reg */
   ATTR,
   UNIFORM, /* prog_data->params[hw_reg] */
   BAD_FILE
};

class reg
{
public:
   /** Register file: ARF, GRF, MRF, IMM. */
   enum register_file file;
   /** virtual register number.  0 = fixed hw reg */
   int reg;
   /** Offset within the virtual register. */
   int reg_offset;
   /** Register type.  BRW_REGISTER_TYPE_* */
   int type;
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

class src_reg : public reg
{
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

      this->file = BAD_FILE;
   }

   src_reg(register_file file, int reg, const glsl_type *type)
   {
      init();

      this->file = file;
      this->reg = reg;
      if (type && (type->is_scalar() || type->is_vector() || type->is_matrix()))
	 this->swizzle = swizzle_for_size(type->vector_elements);
      else
	 this->swizzle = SWIZZLE_XYZW;
   }

   /** Generic unset register constructor. */
   src_reg()
   {
      init();
   }

   src_reg(float f)
   {
      init();

      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_F;
      this->imm.f = f;
   }

   src_reg(uint32_t u)
   {
      init();

      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_UD;
      this->imm.f = u;
   }

   src_reg(int32_t i)
   {
      init();

      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_D;
      this->imm.i = i;
   }

   src_reg(class vec4_visitor *v, const struct glsl_type *type);

   explicit src_reg(dst_reg reg);

   GLuint swizzle; /**< SWIZZLE_XYZW swizzles from Mesa. */
   bool negate;
   bool abs;
};

class dst_reg : public reg
{
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
      this->file = BAD_FILE;
      this->writemask = WRITEMASK_XYZW;
   }

   dst_reg()
   {
      init();
   }

   dst_reg(register_file file, int reg)
   {
      init();

      this->file = file;
      this->reg = reg;
   }

   dst_reg(struct brw_reg reg)
   {
      init();

      this->file = HW_REG;
      this->fixed_hw_reg = reg;
   }

   dst_reg(class vec4_visitor *v, const struct glsl_type *type);

   explicit dst_reg(src_reg reg);

   int writemask; /**< Bitfield of WRITEMASK_[XYZW] */
};

class vec4_instruction : public exec_node {
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

   struct brw_reg get_dst(void);
   struct brw_reg get_src(int i);

   enum opcode opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   dst_reg dst;
   src_reg src[3];

   bool saturate;
   bool predicate_inverse;
   uint32_t predicate;

   int conditional_mod; /**< BRW_CONDITIONAL_* */

   int sampler;
   int target; /**< MRT target. */
   bool shadow_compare;

   bool eot;
   bool header_present;
   int mlen; /**< SEND message length */
   int base_mrf; /**< First MRF in the SEND message, if mlen is nonzero. */

   uint32_t offset; /* spill/unspill offset */
   /** @{
    * Annotation for the generated IR.  One of the two can be set.
    */
   ir_instruction *ir;
   const char *annotation;
};

class vec4_visitor : public ir_visitor
{
public:
   vec4_visitor(struct brw_vs_compile *c,
		struct gl_shader_program *prog, struct brw_shader *shader);
   ~vec4_visitor();

   dst_reg dst_null_f()
   {
      return dst_reg(brw_null_reg());
   }

   dst_reg dst_null_d()
   {
      return dst_reg(retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   }

   dst_reg dst_null_cmp()
   {
      if (intel->gen > 4)
	 return dst_null_d();
      else
	 return dst_null_f();
   }

   struct brw_context *brw;
   const struct gl_vertex_program *vp;
   struct intel_context *intel;
   struct gl_context *ctx;
   struct brw_vs_compile *c;
   struct brw_vs_prog_data *prog_data;
   struct brw_compile *p;
   struct brw_shader *shader;
   struct gl_shader_program *prog;
   void *mem_ctx;
   exec_list instructions;

   char *fail_msg;
   bool failed;

   /**
    * GLSL IR currently being processed, which is associated with our
    * driver IR instructions for debugging purposes.
    */
   ir_instruction *base_ir;
   const char *current_annotation;

   int *virtual_grf_sizes;
   int virtual_grf_count;
   int virtual_grf_array_size;
   int first_non_payload_grf;

   dst_reg *variable_storage(ir_variable *var);

   void reladdr_to_temp(ir_instruction *ir, src_reg *reg, int *num_reladdr);

   src_reg src_reg_for_float(float val);

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_variable *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference_variable  *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_if *);
   /*@}*/

   src_reg result;

   /* Regs for vertex results.  Generated at ir_variable visiting time
    * for the ir->location's used.
    */
   dst_reg output_reg[VERT_RESULT_MAX];

   struct hash_table *variable_ht;

   bool run(void);
   void fail(const char *msg, ...);

   int virtual_grf_alloc(int size);
   int setup_attributes(int payload_reg);
   void setup_payload();
   void reg_allocate_trivial();
   void reg_allocate();

   vec4_instruction *emit(enum opcode opcode);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst, src_reg src0);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst,
			  src_reg src0, src_reg src1);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst,
			  src_reg src0, src_reg src1, src_reg src2);

   /** Walks an exec_list of ir_instruction and sends it through this visitor. */
   void visit_instructions(const exec_list *list);

   void emit_bool_to_cond_code(ir_rvalue *ir);
   void emit_bool_comparison(unsigned int op, dst_reg dst, src_reg src0, src_reg src1);
   void emit_if_gen6(ir_if *ir);

   void emit_block_move(ir_assignment *ir);

   /**
    * Emit the correct dot-product instruction for the type of arguments
    */
   void emit_dp(dst_reg dst, src_reg src0, src_reg src1, unsigned elements);

   void emit_scalar(ir_instruction *ir, enum prog_opcode op,
		    dst_reg dst, src_reg src0);

   void emit_scalar(ir_instruction *ir, enum prog_opcode op,
		    dst_reg dst, src_reg src0, src_reg src1);

   void emit_scs(ir_instruction *ir, enum prog_opcode op,
		 dst_reg dst, const src_reg &src);

   void emit_math1_gen6(enum opcode opcode, dst_reg dst, src_reg src);
   void emit_math1_gen4(enum opcode opcode, dst_reg dst, src_reg src);
   void emit_math(enum opcode opcode, dst_reg dst, src_reg src);
   void emit_math2_gen6(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1);
   void emit_math2_gen4(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1);
   void emit_math(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1);

   int emit_vue_header_gen6(int header_mrf);
   int emit_vue_header_gen4(int header_mrf);
   void emit_urb_writes(void);

   GLboolean try_emit_sat(ir_expression *ir);

   bool process_move_condition(ir_rvalue *ir);

   void generate_code();
   void generate_vs_instruction(vec4_instruction *inst,
				struct brw_reg dst,
				struct brw_reg *src);
   void generate_math1_gen4(vec4_instruction *inst,
			    struct brw_reg dst,
			    struct brw_reg src);
   void generate_math1_gen6(vec4_instruction *inst,
			    struct brw_reg dst,
			    struct brw_reg src);
   void generate_urb_write(vec4_instruction *inst);
};

} /* namespace brw */

#endif /* BRW_VEC4_H */
