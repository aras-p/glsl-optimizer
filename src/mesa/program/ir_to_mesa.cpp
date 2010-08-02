/*
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  VMware, Inc.   All Rights Reserved.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_to_mesa.cpp
 *
 * Translates the IR to ARB_fragment_program text if possible,
 * printing the result
 */

#include <stdio.h>
#include "ir.h"
#include "ir_visitor.h"
#include "ir_print_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "../glsl/program.h"
#include "ir_optimization.h"
#include "ast.h"

extern "C" {
#include "main/mtypes.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_instruction.h"
#include "program/prog_optimize.h"
#include "program/prog_print.h"
#include "program/program.h"
#include "program/prog_uniform.h"
#include "program/prog_parameter.h"
}

static int swizzle_for_size(int size);

/**
 * This struct is a corresponding struct to Mesa prog_src_register, with
 * wider fields.
 */
typedef struct ir_to_mesa_src_reg {
   ir_to_mesa_src_reg(int file, int index, const glsl_type *type)
   {
      this->file = file;
      this->index = index;
      if (type && (type->is_scalar() || type->is_vector() || type->is_matrix()))
	 this->swizzle = swizzle_for_size(type->vector_elements);
      else
	 this->swizzle = SWIZZLE_XYZW;
      this->negate = 0;
      this->reladdr = NULL;
   }

   ir_to_mesa_src_reg()
   {
      this->file = PROGRAM_UNDEFINED;
   }

   int file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   GLuint swizzle; /**< SWIZZLE_XYZWONEZERO swizzles from Mesa. */
   int negate; /**< NEGATE_XYZW mask from mesa */
   /** Register index should be offset by the integer in this reg. */
   ir_to_mesa_src_reg *reladdr;
} ir_to_mesa_src_reg;

typedef struct ir_to_mesa_dst_reg {
   int file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   int writemask; /**< Bitfield of WRITEMASK_[XYZW] */
   GLuint cond_mask:4;
   /** Register index should be offset by the integer in this reg. */
   ir_to_mesa_src_reg *reladdr;
} ir_to_mesa_dst_reg;

extern ir_to_mesa_src_reg ir_to_mesa_undef;

class ir_to_mesa_instruction : public exec_node {
public:
   enum prog_opcode op;
   ir_to_mesa_dst_reg dst_reg;
   ir_to_mesa_src_reg src_reg[3];
   /** Pointer to the ir source this tree came from for debugging */
   ir_instruction *ir;
   GLboolean cond_update;
   int sampler; /**< sampler index */
   int tex_target; /**< One of TEXTURE_*_INDEX */
   GLboolean tex_shadow;

   class function_entry *function; /* Set on OPCODE_CAL or OPCODE_BGNSUB */
};

class variable_storage : public exec_node {
public:
   variable_storage(ir_variable *var, int file, int index)
      : file(file), index(index), var(var)
   {
      /* empty */
   }

   int file;
   int index;
   ir_variable *var; /* variable that maps to this, if any */
};

class function_entry : public exec_node {
public:
   ir_function_signature *sig;

   /**
    * identifier of this function signature used by the program.
    *
    * At the point that Mesa instructions for function calls are
    * generated, we don't know the address of the first instruction of
    * the function body.  So we make the BranchTarget that is called a
    * small integer and rewrite them during set_branchtargets().
    */
   int sig_id;

   /**
    * Pointer to first instruction of the function body.
    *
    * Set during function body emits after main() is processed.
    */
   ir_to_mesa_instruction *bgn_inst;

   /**
    * Index of the first instruction of the function body in actual
    * Mesa IR.
    *
    * Set after convertion from ir_to_mesa_instruction to prog_instruction.
    */
   int inst;

   /** Storage for the return value. */
   ir_to_mesa_src_reg return_reg;
};

class ir_to_mesa_visitor : public ir_visitor {
public:
   ir_to_mesa_visitor();

   function_entry *current_function;

   GLcontext *ctx;
   struct gl_program *prog;

   int next_temp;

   variable_storage *find_variable_storage(ir_variable *var);

   function_entry *get_function_signature(ir_function_signature *sig);

   ir_to_mesa_src_reg get_temp(const glsl_type *type);
   void reladdr_to_temp(ir_instruction *ir,
			ir_to_mesa_src_reg *reg, int *num_reladdr);

   struct ir_to_mesa_src_reg src_reg_for_float(float val);

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

   struct ir_to_mesa_src_reg result;

   /** List of variable_storage */
   exec_list variables;

   /** List of function_entry */
   exec_list function_signatures;
   int next_signature_id;

   /** List of ir_to_mesa_instruction */
   exec_list instructions;

   ir_to_mesa_instruction *ir_to_mesa_emit_op0(ir_instruction *ir,
					       enum prog_opcode op);

   ir_to_mesa_instruction *ir_to_mesa_emit_op1(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0);

   ir_to_mesa_instruction *ir_to_mesa_emit_op2(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0,
					       ir_to_mesa_src_reg src1);

   ir_to_mesa_instruction *ir_to_mesa_emit_op3(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0,
					       ir_to_mesa_src_reg src1,
					       ir_to_mesa_src_reg src2);

   void ir_to_mesa_emit_scalar_op1(ir_instruction *ir,
				   enum prog_opcode op,
				   ir_to_mesa_dst_reg dst,
				   ir_to_mesa_src_reg src0);

   void ir_to_mesa_emit_scalar_op2(ir_instruction *ir,
				   enum prog_opcode op,
				   ir_to_mesa_dst_reg dst,
				   ir_to_mesa_src_reg src0,
				   ir_to_mesa_src_reg src1);

   GLboolean try_emit_mad(ir_expression *ir,
			  int mul_operand);

   int *sampler_map;
   int sampler_map_size;

   void map_sampler(int location, int sampler);
   int get_sampler_number(int location);

   void *mem_ctx;
};

ir_to_mesa_src_reg ir_to_mesa_undef = ir_to_mesa_src_reg(PROGRAM_UNDEFINED, 0, NULL);

ir_to_mesa_dst_reg ir_to_mesa_undef_dst = {
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP, COND_TR, NULL,
};

ir_to_mesa_dst_reg ir_to_mesa_address_reg = {
   PROGRAM_ADDRESS, 0, WRITEMASK_X, COND_TR, NULL
};

static int swizzle_for_size(int size)
{
   int size_swizzles[4] = {
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
   };

   return size_swizzles[size - 1];
}

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op3(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0,
					ir_to_mesa_src_reg src1,
					ir_to_mesa_src_reg src2)
{
   ir_to_mesa_instruction *inst = new(mem_ctx) ir_to_mesa_instruction();
   int num_reladdr = 0;

   /* If we have to do relative addressing, we want to load the ARL
    * reg directly for one of the regs, and preload the other reladdr
    * sources into temps.
    */
   num_reladdr += dst.reladdr != NULL;
   num_reladdr += src0.reladdr != NULL;
   num_reladdr += src1.reladdr != NULL;
   num_reladdr += src2.reladdr != NULL;

   reladdr_to_temp(ir, &src2, &num_reladdr);
   reladdr_to_temp(ir, &src1, &num_reladdr);
   reladdr_to_temp(ir, &src0, &num_reladdr);

   if (dst.reladdr) {
      ir_to_mesa_emit_op1(ir, OPCODE_ARL, ir_to_mesa_address_reg,
                          *dst.reladdr);

      num_reladdr--;
   }
   assert(num_reladdr == 0);

   inst->op = op;
   inst->dst_reg = dst;
   inst->src_reg[0] = src0;
   inst->src_reg[1] = src1;
   inst->src_reg[2] = src2;
   inst->ir = ir;

   inst->function = NULL;

   this->instructions.push_tail(inst);

   return inst;
}


ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op2(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0,
					ir_to_mesa_src_reg src1)
{
   return ir_to_mesa_emit_op3(ir, op, dst, src0, src1, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op1(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0)
{
   return ir_to_mesa_emit_op3(ir, op, dst,
			      src0, ir_to_mesa_undef, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op0(ir_instruction *ir,
					enum prog_opcode op)
{
   return ir_to_mesa_emit_op3(ir, op, ir_to_mesa_undef_dst,
			      ir_to_mesa_undef,
			      ir_to_mesa_undef,
			      ir_to_mesa_undef);
}

void
ir_to_mesa_visitor::map_sampler(int location, int sampler)
{
   if (this->sampler_map_size <= location) {
      this->sampler_map = talloc_realloc(this->mem_ctx, this->sampler_map,
					 int, location + 1);
      this->sampler_map_size = location + 1;
   }

   this->sampler_map[location] = sampler;
}

int
ir_to_mesa_visitor::get_sampler_number(int location)
{
   assert(location < this->sampler_map_size);
   return this->sampler_map[location];
}

inline ir_to_mesa_dst_reg
ir_to_mesa_dst_reg_from_src(ir_to_mesa_src_reg reg)
{
   ir_to_mesa_dst_reg dst_reg;

   dst_reg.file = reg.file;
   dst_reg.index = reg.index;
   dst_reg.writemask = WRITEMASK_XYZW;
   dst_reg.cond_mask = COND_TR;
   dst_reg.reladdr = reg.reladdr;

   return dst_reg;
}

inline ir_to_mesa_src_reg
ir_to_mesa_src_reg_from_dst(ir_to_mesa_dst_reg reg)
{
   return ir_to_mesa_src_reg(reg.file, reg.index, NULL);
}

/**
 * Emits Mesa scalar opcodes to produce unique answers across channels.
 *
 * Some Mesa opcodes are scalar-only, like ARB_fp/vp.  The src X
 * channel determines the result across all channels.  So to do a vec4
 * of this operation, we want to emit a scalar per source channel used
 * to produce dest channels.
 */
void
ir_to_mesa_visitor::ir_to_mesa_emit_scalar_op2(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg orig_src0,
					       ir_to_mesa_src_reg orig_src1)
{
   int i, j;
   int done_mask = ~dst.writemask;

   /* Mesa RCP is a scalar operation splatting results to all channels,
    * like ARB_fp/vp.  So emit as many RCPs as necessary to cover our
    * dst channels.
    */
   for (i = 0; i < 4; i++) {
      GLuint this_mask = (1 << i);
      ir_to_mesa_instruction *inst;
      ir_to_mesa_src_reg src0 = orig_src0;
      ir_to_mesa_src_reg src1 = orig_src1;

      if (done_mask & this_mask)
	 continue;

      GLuint src0_swiz = GET_SWZ(src0.swizzle, i);
      GLuint src1_swiz = GET_SWZ(src1.swizzle, i);
      for (j = i + 1; j < 4; j++) {
	 if (!(done_mask & (1 << j)) &&
	     GET_SWZ(src0.swizzle, j) == src0_swiz &&
	     GET_SWZ(src1.swizzle, j) == src1_swiz) {
	    this_mask |= (1 << j);
	 }
      }
      src0.swizzle = MAKE_SWIZZLE4(src0_swiz, src0_swiz,
				   src0_swiz, src0_swiz);
      src1.swizzle = MAKE_SWIZZLE4(src1_swiz, src1_swiz,
				  src1_swiz, src1_swiz);

      inst = ir_to_mesa_emit_op2(ir, op,
				 dst,
				 src0,
				 src1);
      inst->dst_reg.writemask = this_mask;
      done_mask |= this_mask;
   }
}

void
ir_to_mesa_visitor::ir_to_mesa_emit_scalar_op1(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0)
{
   ir_to_mesa_src_reg undef = ir_to_mesa_undef;

   undef.swizzle = SWIZZLE_XXXX;

   ir_to_mesa_emit_scalar_op2(ir, op, dst, src0, undef);
}

struct ir_to_mesa_src_reg
ir_to_mesa_visitor::src_reg_for_float(float val)
{
   ir_to_mesa_src_reg src_reg(PROGRAM_CONSTANT, -1, NULL);

   src_reg.index = _mesa_add_unnamed_constant(this->prog->Parameters,
					      &val, 1, &src_reg.swizzle);

   return src_reg;
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
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   default:
      assert(0);
   }
}

/**
 * In the initial pass of codegen, we assign temporary numbers to
 * intermediate results.  (not SSA -- variable assignments will reuse
 * storage).  Actual register allocation for the Mesa VM occurs in a
 * pass over the Mesa IR later.
 */
ir_to_mesa_src_reg
ir_to_mesa_visitor::get_temp(const glsl_type *type)
{
   ir_to_mesa_src_reg src_reg;
   int swizzle[4];
   int i;

   src_reg.file = PROGRAM_TEMPORARY;
   src_reg.index = next_temp;
   src_reg.reladdr = NULL;
   next_temp += type_size(type);

   if (type->is_array() || type->is_record()) {
      src_reg.swizzle = SWIZZLE_NOOP;
   } else {
      for (i = 0; i < type->vector_elements; i++)
	 swizzle[i] = i;
      for (; i < 4; i++)
	 swizzle[i] = type->vector_elements - 1;
      src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
				      swizzle[2], swizzle[3]);
   }
   src_reg.negate = 0;

   return src_reg;
}

variable_storage *
ir_to_mesa_visitor::find_variable_storage(ir_variable *var)
{
   
   variable_storage *entry;

   foreach_iter(exec_list_iterator, iter, this->variables) {
      entry = (variable_storage *)iter.get();

      if (entry->var == var)
	 return entry;
   }

   return NULL;
}

void
ir_to_mesa_visitor::visit(ir_variable *ir)
{
   if (strcmp(ir->name, "gl_FragCoord") == 0) {
      struct gl_fragment_program *fp = (struct gl_fragment_program *)this->prog;

      fp->OriginUpperLeft = ir->origin_upper_left;
      fp->PixelCenterInteger = ir->pixel_center_integer;
   }
}

void
ir_to_mesa_visitor::visit(ir_loop *ir)
{
   assert(!ir->from);
   assert(!ir->to);
   assert(!ir->increment);
   assert(!ir->counter);

   ir_to_mesa_emit_op0(NULL, OPCODE_BGNLOOP);
   visit_exec_list(&ir->body_instructions, this);
   ir_to_mesa_emit_op0(NULL, OPCODE_ENDLOOP);
}

void
ir_to_mesa_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      ir_to_mesa_emit_op0(NULL, OPCODE_BRK);
      break;
   case ir_loop_jump::jump_continue:
      ir_to_mesa_emit_op0(NULL, OPCODE_CONT);
      break;
   }
}


void
ir_to_mesa_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
}

void
ir_to_mesa_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined before we get to ir_to_mesa.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      foreach_iter(exec_list_iterator, iter, sig->body) {
	 ir_instruction *ir = (ir_instruction *)iter.get();

	 ir->accept(this);
      }
   }
}

GLboolean
ir_to_mesa_visitor::try_emit_mad(ir_expression *ir, int mul_operand)
{
   int nonmul_operand = 1 - mul_operand;
   ir_to_mesa_src_reg a, b, c;

   ir_expression *expr = ir->operands[mul_operand]->as_expression();
   if (!expr || expr->operation != ir_binop_mul)
      return false;

   expr->operands[0]->accept(this);
   a = this->result;
   expr->operands[1]->accept(this);
   b = this->result;
   ir->operands[nonmul_operand]->accept(this);
   c = this->result;

   this->result = get_temp(ir->type);
   ir_to_mesa_emit_op3(ir, OPCODE_MAD,
		       ir_to_mesa_dst_reg_from_src(this->result), a, b, c);

   return true;
}

void
ir_to_mesa_visitor::reladdr_to_temp(ir_instruction *ir,
				    ir_to_mesa_src_reg *reg, int *num_reladdr)
{
   if (!reg->reladdr)
      return;

   ir_to_mesa_emit_op1(ir, OPCODE_ARL, ir_to_mesa_address_reg, *reg->reladdr);

   if (*num_reladdr != 1) {
      ir_to_mesa_src_reg temp = get_temp(glsl_type::vec4_type);

      ir_to_mesa_emit_op1(ir, OPCODE_MOV,
			  ir_to_mesa_dst_reg_from_src(temp), *reg);
      *reg = temp;
   }

   (*num_reladdr)--;
}

void
ir_to_mesa_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   struct ir_to_mesa_src_reg op[2];
   struct ir_to_mesa_src_reg result_src;
   struct ir_to_mesa_dst_reg result_dst;
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);

   /* Quick peephole: Emit OPCODE_MAD(a, b, c) instead of ADD(MUL(a, b), c)
    */
   if (ir->operation == ir_binop_add) {
      if (try_emit_mad(ir, 1))
	 return;
      if (try_emit_mad(ir, 0))
	 return;
   }

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result.file = PROGRAM_UNDEFINED;
      ir->operands[operand]->accept(this);
      if (this->result.file == PROGRAM_UNDEFINED) {
	 ir_print_visitor v;
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
	 exit(1);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
   }

   this->result.file = PROGRAM_UNDEFINED;

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = get_temp(ir->type);
   /* convenience for the emit functions below. */
   result_dst = ir_to_mesa_dst_reg_from_src(result_src);
   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   switch (ir->operation) {
   case ir_unop_logic_not:
      ir_to_mesa_emit_op2(ir, OPCODE_SEQ, result_dst,
			  op[0], src_reg_for_float(0.0));
      break;
   case ir_unop_neg:
      op[0].negate = ~op[0].negate;
      result_src = op[0];
      break;
   case ir_unop_abs:
      ir_to_mesa_emit_op1(ir, OPCODE_ABS, result_dst, op[0]);
      break;
   case ir_unop_sign:
      ir_to_mesa_emit_op1(ir, OPCODE_SSG, result_dst, op[0]);
      break;
   case ir_unop_rcp:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RCP, result_dst, op[0]);
      break;

   case ir_unop_exp:
      ir_to_mesa_emit_scalar_op2(ir, OPCODE_POW, result_dst,
				 src_reg_for_float(M_E), op[0]);
      break;
   case ir_unop_exp2:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_EX2, result_dst, op[0]);
      break;
   case ir_unop_log:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_LOG, result_dst, op[0]);
      break;
   case ir_unop_log2:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_LG2, result_dst, op[0]);
      break;
   case ir_unop_sin:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_COS, result_dst, op[0]);
      break;

   case ir_unop_dFdx:
      ir_to_mesa_emit_op1(ir, OPCODE_DDX, result_dst, op[0]);
      break;
   case ir_unop_dFdy:
      ir_to_mesa_emit_op1(ir, OPCODE_DDY, result_dst, op[0]);
      break;

   case ir_binop_add:
      ir_to_mesa_emit_op2(ir, OPCODE_ADD, result_dst, op[0], op[1]);
      break;
   case ir_binop_sub:
      ir_to_mesa_emit_op2(ir, OPCODE_SUB, result_dst, op[0], op[1]);
      break;

   case ir_binop_mul:
      ir_to_mesa_emit_op2(ir, OPCODE_MUL, result_dst, op[0], op[1]);
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
      ir_to_mesa_emit_op2(ir, OPCODE_SLT, result_dst, op[0], op[1]);
      break;
   case ir_binop_greater:
      ir_to_mesa_emit_op2(ir, OPCODE_SGT, result_dst, op[0], op[1]);
      break;
   case ir_binop_lequal:
      ir_to_mesa_emit_op2(ir, OPCODE_SLE, result_dst, op[0], op[1]);
      break;
   case ir_binop_gequal:
      ir_to_mesa_emit_op2(ir, OPCODE_SGE, result_dst, op[0], op[1]);
      break;
   case ir_binop_equal:
      ir_to_mesa_emit_op2(ir, OPCODE_SEQ, result_dst, op[0], op[1]);
      break;
   case ir_binop_logic_xor:
   case ir_binop_nequal:
      ir_to_mesa_emit_op2(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      /* This could be a saturated add and skip the SNE. */
      ir_to_mesa_emit_op2(ir, OPCODE_ADD,
			  result_dst,
			  op[0], op[1]);

      ir_to_mesa_emit_op2(ir, OPCODE_SNE,
			  result_dst,
			  result_src, src_reg_for_float(0.0));
      break;

   case ir_binop_logic_and:
      /* the bool args are stored as float 0.0 or 1.0, so "mul" gives us "and". */
      ir_to_mesa_emit_op2(ir, OPCODE_MUL,
			  result_dst,
			  op[0], op[1]);
      break;

   case ir_binop_dot:
      if (ir->operands[0]->type == vec4_type) {
	 assert(ir->operands[1]->type == vec4_type);
	 ir_to_mesa_emit_op2(ir, OPCODE_DP4,
			     result_dst,
			     op[0], op[1]);
      } else if (ir->operands[0]->type == vec3_type) {
	 assert(ir->operands[1]->type == vec3_type);
	 ir_to_mesa_emit_op2(ir, OPCODE_DP3,
			     result_dst,
			     op[0], op[1]);
      } else if (ir->operands[0]->type == vec2_type) {
	 assert(ir->operands[1]->type == vec2_type);
	 ir_to_mesa_emit_op2(ir, OPCODE_DP2,
			     result_dst,
			     op[0], op[1]);
      }
      break;

   case ir_binop_cross:
      ir_to_mesa_emit_op2(ir, OPCODE_XPD, result_dst, op[0], op[1]);
      break;

   case ir_unop_sqrt:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RSQ, result_dst, op[0]);
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RCP, result_dst, result_src);
      /* For incoming channels < 0, set the result to 0. */
      ir_to_mesa_emit_op3(ir, OPCODE_CMP, result_dst,
			  op[0], src_reg_for_float(0.0), result_src);
      break;
   case ir_unop_rsq:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RSQ, result_dst, op[0]);
      break;
   case ir_unop_i2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
      /* Mesa IR lacks types, ints are stored as truncated floats. */
      result_src = op[0];
      break;
   case ir_unop_f2i:
      ir_to_mesa_emit_op1(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_f2b:
   case ir_unop_i2b:
      ir_to_mesa_emit_op2(ir, OPCODE_SNE, result_dst,
			  result_src, src_reg_for_float(0.0));
      break;
   case ir_unop_trunc:
      ir_to_mesa_emit_op1(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_ceil:
      op[0].negate = ~op[0].negate;
      ir_to_mesa_emit_op1(ir, OPCODE_FLR, result_dst, op[0]);
      result_src.negate = ~result_src.negate;
      break;
   case ir_unop_floor:
      ir_to_mesa_emit_op1(ir, OPCODE_FLR, result_dst, op[0]);
      break;
   case ir_unop_fract:
      ir_to_mesa_emit_op1(ir, OPCODE_FRC, result_dst, op[0]);
      break;

   case ir_binop_min:
      ir_to_mesa_emit_op2(ir, OPCODE_MIN, result_dst, op[0], op[1]);
      break;
   case ir_binop_max:
      ir_to_mesa_emit_op2(ir, OPCODE_MAX, result_dst, op[0], op[1]);
      break;
   case ir_binop_pow:
      ir_to_mesa_emit_scalar_op2(ir, OPCODE_POW, result_dst, op[0], op[1]);
      break;

   case ir_unop_bit_not:
   case ir_unop_u2f:
   case ir_binop_lshift:
   case ir_binop_rshift:
   case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }

   this->result = result_src;
}


void
ir_to_mesa_visitor::visit(ir_swizzle *ir)
{
   ir_to_mesa_src_reg src_reg;
   int i;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   src_reg = this->result;
   assert(src_reg.file != PROGRAM_UNDEFINED);

   for (i = 0; i < 4; i++) {
      if (i < ir->type->vector_elements) {
	 switch (i) {
	 case 0:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.x);
	    break;
	 case 1:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.y);
	    break;
	 case 2:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.z);
	    break;
	 case 3:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.w);
	    break;
	 }
      } else {
	 /* If the type is smaller than a vec4, replicate the last
	  * channel out.
	  */
	 swizzle[i] = swizzle[ir->type->vector_elements - 1];
      }
   }

   src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0],
				   swizzle[1],
				   swizzle[2],
				   swizzle[3]);

   this->result = src_reg;
}

static const struct {
   const char *name;
   const char *field;
   int tokens[STATE_LENGTH];
   int swizzle;
   bool array_indexed;
} statevars[] = {
   {"gl_DepthRange", "near",
    {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_XXXX},
   {"gl_DepthRange", "far",
    {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_YYYY},
   {"gl_DepthRange", "diff",
    {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_ZZZZ},

   {"gl_ClipPlane", NULL,
    {STATE_CLIPPLANE, 0, 0}, SWIZZLE_XYZW, true}
,
   {"gl_Point", "size",
    {STATE_POINT_SIZE}, SWIZZLE_XXXX},
   {"gl_Point", "sizeMin",
    {STATE_POINT_SIZE}, SWIZZLE_YYYY},
   {"gl_Point", "sizeMax",
    {STATE_POINT_SIZE}, SWIZZLE_ZZZZ},
   {"gl_Point", "fadeThresholdSize",
    {STATE_POINT_SIZE}, SWIZZLE_WWWW},
   {"gl_Point", "distanceConstantAttenuation",
    {STATE_POINT_ATTENUATION}, SWIZZLE_XXXX},
   {"gl_Point", "distanceLinearAttenuation",
    {STATE_POINT_ATTENUATION}, SWIZZLE_YYYY},
   {"gl_Point", "distanceQuadraticAttenuation",
    {STATE_POINT_ATTENUATION}, SWIZZLE_ZZZZ},

   {"gl_FrontMaterial", "emission",
    {STATE_MATERIAL, 0, STATE_EMISSION}, SWIZZLE_XYZW},
   {"gl_FrontMaterial", "ambient",
    {STATE_MATERIAL, 0, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"gl_FrontMaterial", "diffuse",
    {STATE_MATERIAL, 0, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"gl_FrontMaterial", "specular",
    {STATE_MATERIAL, 0, STATE_SPECULAR}, SWIZZLE_XYZW},
   {"gl_FrontMaterial", "shininess",
    {STATE_MATERIAL, 0, STATE_SHININESS}, SWIZZLE_XXXX},

   {"gl_BackMaterial", "emission",
    {STATE_MATERIAL, 1, STATE_EMISSION}, SWIZZLE_XYZW},
   {"gl_BackMaterial", "ambient",
    {STATE_MATERIAL, 1, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"gl_BackMaterial", "diffuse",
    {STATE_MATERIAL, 1, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"gl_BackMaterial", "specular",
    {STATE_MATERIAL, 1, STATE_SPECULAR}, SWIZZLE_XYZW},
   {"gl_BackMaterial", "shininess",
    {STATE_MATERIAL, 1, STATE_SHININESS}, SWIZZLE_XXXX},

   {"gl_LightSource", "ambient",
    {STATE_LIGHT, 0, STATE_AMBIENT}, SWIZZLE_XYZW, true},
   {"gl_LightSource", "diffuse",
    {STATE_LIGHT, 0, STATE_DIFFUSE}, SWIZZLE_XYZW, true},
   {"gl_LightSource", "specular",
    {STATE_LIGHT, 0, STATE_SPECULAR}, SWIZZLE_XYZW, true},
   {"gl_LightSource", "position",
    {STATE_LIGHT, 0, STATE_POSITION}, SWIZZLE_XYZW, true},
   {"gl_LightSource", "halfVector",
    {STATE_LIGHT, 0, STATE_HALF_VECTOR}, SWIZZLE_XYZW, true},
   {"gl_LightSource", "spotDirection",
    {STATE_LIGHT, 0, STATE_SPOT_DIRECTION}, SWIZZLE_XYZW, true},
   {"gl_LightSource", "spotCosCutoff",
    {STATE_LIGHT, 0, STATE_SPOT_DIRECTION}, SWIZZLE_WWWW, true},
   {"gl_LightSource", "spotCutoff",
    {STATE_LIGHT, 0, STATE_SPOT_CUTOFF}, SWIZZLE_XXXX, true},
   {"gl_LightSource", "spotExponent",
    {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_WWWW, true},
   {"gl_LightSource", "constantAttenuation",
    {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_XXXX, true},
   {"gl_LightSource", "linearAttenuation",
    {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_YYYY, true},
   {"gl_LightSource", "quadraticAttenuation",
    {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_ZZZZ, true},

   {"gl_LightModel", NULL,
    {STATE_LIGHTMODEL_AMBIENT, 0}, SWIZZLE_XYZW},

   {"gl_FrontLightModelProduct", NULL,
    {STATE_LIGHTMODEL_SCENECOLOR, 0}, SWIZZLE_XYZW},
   {"gl_BackLightModelProduct", NULL,
    {STATE_LIGHTMODEL_SCENECOLOR, 1}, SWIZZLE_XYZW},

   {"gl_FrontLightProduct", "ambient",
    {STATE_LIGHTPROD, 0, 0, STATE_AMBIENT}, SWIZZLE_XYZW, true},
   {"gl_FrontLightProduct", "diffuse",
    {STATE_LIGHTPROD, 0, 0, STATE_DIFFUSE}, SWIZZLE_XYZW, true},
   {"gl_FrontLightProduct", "specular",
    {STATE_LIGHTPROD, 0, 0, STATE_SPECULAR}, SWIZZLE_XYZW, true},

   {"gl_BackLightProduct", "ambient",
    {STATE_LIGHTPROD, 0, 1, STATE_AMBIENT}, SWIZZLE_XYZW, true},
   {"gl_BackLightProduct", "diffuse",
    {STATE_LIGHTPROD, 0, 1, STATE_DIFFUSE}, SWIZZLE_XYZW, true},
   {"gl_BackLightProduct", "specular",
    {STATE_LIGHTPROD, 0, 1, STATE_SPECULAR}, SWIZZLE_XYZW, true},

   {"gl_TextureEnvColor", "ambient",
    {STATE_TEXENV_COLOR, 0}, SWIZZLE_XYZW, true},

   {"gl_EyePlaneS", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_S}, SWIZZLE_XYZW, true},
   {"gl_EyePlaneT", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_T}, SWIZZLE_XYZW, true},
   {"gl_EyePlaneR", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_R}, SWIZZLE_XYZW, true},
   {"gl_EyePlaneQ", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_EYE_Q}, SWIZZLE_XYZW, true},

   {"gl_ObjectPlaneS", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_S}, SWIZZLE_XYZW, true},
   {"gl_ObjectPlaneT", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_T}, SWIZZLE_XYZW, true},
   {"gl_ObjectPlaneR", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_R}, SWIZZLE_XYZW, true},
   {"gl_ObjectPlaneQ", NULL,
    {STATE_TEXGEN, 0, STATE_TEXGEN_OBJECT_Q}, SWIZZLE_XYZW, true},

   {"gl_Fog", "color",
    {STATE_FOG_COLOR}, SWIZZLE_XYZW},
   {"gl_Fog", "density",
    {STATE_FOG_PARAMS}, SWIZZLE_XXXX},
   {"gl_Fog", "start",
    {STATE_FOG_PARAMS}, SWIZZLE_YYYY},
   {"gl_Fog", "end",
    {STATE_FOG_PARAMS}, SWIZZLE_ZZZZ},
   {"gl_Fog", "scale",
    {STATE_FOG_PARAMS}, SWIZZLE_WWWW},
};

static ir_to_mesa_src_reg
get_builtin_uniform_reg(struct gl_program *prog,
			const char *name, int array_index, const char *field)
{
   unsigned int i;
   ir_to_mesa_src_reg src_reg;
   int tokens[STATE_LENGTH];

   for (i = 0; i < Elements(statevars); i++) {
      if (strcmp(statevars[i].name, name) != 0)
	 continue;
      if (!field && statevars[i].field) {
	 assert(!"FINISHME: whole-structure state var dereference");
      }
      if (field && strcmp(statevars[i].field, field) != 0)
	 continue;
      break;
   }

   if (i ==  Elements(statevars)) {
      printf("builtin uniform %s%s%s not found\n",
	     name,
	     field ? "." : "",
	     field ? field : "");
      abort();
   }

   memcpy(&tokens, statevars[i].tokens, sizeof(tokens));
   if (statevars[i].array_indexed)
      tokens[1] = array_index;

   src_reg.file = PROGRAM_STATE_VAR;
   src_reg.index = _mesa_add_state_reference(prog->Parameters,
					     (gl_state_index *)tokens);
   src_reg.swizzle = statevars[i].swizzle;
   src_reg.negate = 0;
   src_reg.reladdr = false;

   return src_reg;
}

static int
add_matrix_ref(struct gl_program *prog, int *tokens)
{
   int base_pos = -1;
   int i;

   /* Add a ref for each column.  It looks like the reason we do
    * it this way is that _mesa_add_state_reference doesn't work
    * for things that aren't vec4s, so the tokens[2]/tokens[3]
    * range has to be equal.
    */
   for (i = 0; i < 4; i++) {
      tokens[2] = i;
      tokens[3] = i;
      int pos = _mesa_add_state_reference(prog->Parameters,
					  (gl_state_index *)tokens);
      if (base_pos == -1)
	 base_pos = pos;
      else
	 assert(base_pos + i == pos);
   }

   return base_pos;
}

static variable_storage *
get_builtin_matrix_ref(void *mem_ctx, struct gl_program *prog, ir_variable *var,
		       ir_rvalue *array_index)
{
   /*
    * NOTE: The ARB_vertex_program extension specified that matrices get
    * loaded in registers in row-major order.  With GLSL, we want column-
    * major order.  So, we need to transpose all matrices here...
    */
   static const struct {
      const char *name;
      int matrix;
      int modifier;
   } matrices[] = {
      { "gl_ModelViewMatrix", STATE_MODELVIEW_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_ModelViewMatrixInverse", STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_ModelViewMatrixTranspose", STATE_MODELVIEW_MATRIX, 0 },
      { "gl_ModelViewMatrixInverseTranspose", STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_ProjectionMatrix", STATE_PROJECTION_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_ProjectionMatrixInverse", STATE_PROJECTION_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_ProjectionMatrixTranspose", STATE_PROJECTION_MATRIX, 0 },
      { "gl_ProjectionMatrixInverseTranspose", STATE_PROJECTION_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_ModelViewProjectionMatrix", STATE_MVP_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_ModelViewProjectionMatrixInverse", STATE_MVP_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_ModelViewProjectionMatrixTranspose", STATE_MVP_MATRIX, 0 },
      { "gl_ModelViewProjectionMatrixInverseTranspose", STATE_MVP_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_TextureMatrix", STATE_TEXTURE_MATRIX, STATE_MATRIX_TRANSPOSE },
      { "gl_TextureMatrixInverse", STATE_TEXTURE_MATRIX, STATE_MATRIX_INVTRANS },
      { "gl_TextureMatrixTranspose", STATE_TEXTURE_MATRIX, 0 },
      { "gl_TextureMatrixInverseTranspose", STATE_TEXTURE_MATRIX, STATE_MATRIX_INVERSE },

      { "gl_NormalMatrix", STATE_MODELVIEW_MATRIX, STATE_MATRIX_INVERSE },

   };
   unsigned int i;
   variable_storage *entry;

   /* C++ gets angry when we try to use an int as a gl_state_index, so we use
    * ints for gl_state_index.  Make sure they're compatible.
    */
   assert(sizeof(gl_state_index) == sizeof(int));

   for (i = 0; i < Elements(matrices); i++) {
      if (strcmp(var->name, matrices[i].name) == 0) {
	 int tokens[STATE_LENGTH];
	 int base_pos = -1;

	 tokens[0] = matrices[i].matrix;
	 tokens[4] = matrices[i].modifier;
	 if (matrices[i].matrix == STATE_TEXTURE_MATRIX) {
	    ir_constant *index = array_index->constant_expression_value();
	    if (index) {
	       tokens[1] = index->value.i[0];
	       base_pos = add_matrix_ref(prog, tokens);
	    } else {
	       for (i = 0; i < var->type->length; i++) {
		  tokens[1] = i;
		  int pos = add_matrix_ref(prog, tokens);
		  if (base_pos == -1)
		     base_pos = pos;
		  else
		     assert(base_pos + (int)i * 4 == pos);
	       }
	    }
	 } else {
	    tokens[1] = 0; /* unused array index */
	    base_pos = add_matrix_ref(prog, tokens);
	 }
	 tokens[4] = matrices[i].modifier;

	 entry = new(mem_ctx) variable_storage(var,
					       PROGRAM_STATE_VAR,
					       base_pos);

	 return entry;
      }
   }

   return NULL;
}

void
ir_to_mesa_visitor::visit(ir_dereference_variable *ir)
{
   variable_storage *entry = find_variable_storage(ir->var);
   unsigned int loc;
   int len;

   if (!entry) {
      switch (ir->var->mode) {
      case ir_var_uniform:
	 entry = get_builtin_matrix_ref(this->mem_ctx, this->prog, ir->var,
					NULL);
	 if (entry)
	    break;

	 /* FINISHME: Fix up uniform name for arrays and things */
	 if (ir->var->type->base_type == GLSL_TYPE_SAMPLER) {
	    /* FINISHME: we whack the location of the var here, which
	     * is probably not expected.  But we need to communicate
	     * mesa's sampler number to the tex instruction.
	     */
	    int sampler = _mesa_add_sampler(this->prog->Parameters,
					    ir->var->name,
					    ir->var->type->gl_type);
	    map_sampler(ir->var->location, sampler);

	    entry = new(mem_ctx) variable_storage(ir->var, PROGRAM_SAMPLER,
						  sampler);
	    this->variables.push_tail(entry);
	    break;
	 }

	 assert(ir->var->type->gl_type != 0 &&
		ir->var->type->gl_type != GL_INVALID_ENUM);

	 if (ir->var->type->is_vector() ||
	     ir->var->type->is_scalar()) {
	    len = ir->var->type->vector_elements;
	 } else {
	    len = type_size(ir->var->type) * 4;
	 }

	 loc = _mesa_add_uniform(this->prog->Parameters,
				 ir->var->name,
				 len,
				 ir->var->type->gl_type,
				 NULL);

	 /* Always mark the uniform used at this point.  If it isn't
	  * used, dead code elimination should have nuked the decl already.
	  */
	 this->prog->Parameters->Parameters[loc].Used = GL_TRUE;

	 entry = new(mem_ctx) variable_storage(ir->var, PROGRAM_UNIFORM, loc);
	 this->variables.push_tail(entry);
	 break;
      case ir_var_in:
      case ir_var_out:
      case ir_var_inout:
	 /* The linker assigns locations for varyings and attributes,
	  * including deprecated builtins (like gl_Color), user-assign
	  * generic attributes (glBindVertexLocation), and
	  * user-defined varyings.
	  *
	  * FINISHME: We would hit this path for function arguments.  Fix!
	  */
	 assert(ir->var->location != -1);
	 if (ir->var->mode == ir_var_in ||
	     ir->var->mode == ir_var_inout) {
	    entry = new(mem_ctx) variable_storage(ir->var,
						  PROGRAM_INPUT,
						  ir->var->location);

	    if (this->prog->Target == GL_VERTEX_PROGRAM_ARB &&
		ir->var->location >= VERT_ATTRIB_GENERIC0) {
	       _mesa_add_attribute(prog->Attributes,
				   ir->var->name,
				   type_size(ir->var->type) * 4,
				   ir->var->type->gl_type,
				   ir->var->location - VERT_ATTRIB_GENERIC0);
	    }
	 } else {
	    entry = new(mem_ctx) variable_storage(ir->var,
						  PROGRAM_OUTPUT,
						  ir->var->location);
	 }

	 break;
      case ir_var_auto:
      case ir_var_temporary:
	 entry = new(mem_ctx) variable_storage(ir->var, PROGRAM_TEMPORARY,
					       this->next_temp);
	 this->variables.push_tail(entry);

	 next_temp += type_size(ir->var->type);
	 break;
      }

      if (!entry) {
	 printf("Failed to make storage for %s\n", ir->var->name);
	 exit(1);
      }
   }

   this->result = ir_to_mesa_src_reg(entry->file, entry->index, ir->var->type);
}

void
ir_to_mesa_visitor::visit(ir_dereference_array *ir)
{
   ir_variable *var = ir->variable_referenced();
   ir_constant *index;
   ir_to_mesa_src_reg src_reg;
   ir_dereference_variable *deref_var = ir->array->as_dereference_variable();
   int element_size = type_size(ir->type);

   index = ir->array_index->constant_expression_value();

   if (deref_var && strncmp(deref_var->var->name,
			    "gl_TextureMatrix",
			    strlen("gl_TextureMatrix")) == 0) {
      struct variable_storage *entry;

      entry = get_builtin_matrix_ref(this->mem_ctx, this->prog, deref_var->var,
				     ir->array_index);
      assert(entry);

      ir_to_mesa_src_reg src_reg(entry->file, entry->index, ir->type);

      if (index) {
	 src_reg.reladdr = NULL;
      } else {
	 ir_to_mesa_src_reg index_reg = get_temp(glsl_type::float_type);

	 ir->array_index->accept(this);
	 ir_to_mesa_emit_op2(ir, OPCODE_MUL,
			     ir_to_mesa_dst_reg_from_src(index_reg),
			     this->result, src_reg_for_float(element_size));

	 src_reg.reladdr = talloc(mem_ctx, ir_to_mesa_src_reg);
	 memcpy(src_reg.reladdr, &index_reg, sizeof(index_reg));
      }

      this->result = src_reg;
      return;
   }

   if (strncmp(var->name, "gl_", 3) == 0 && var->mode == ir_var_uniform &&
       !var->type->is_matrix()) {
      ir_dereference_record *record = NULL;
      if (ir->array->ir_type == ir_type_dereference_record)
	 record = (ir_dereference_record *)ir->array;

      assert(index || !"FINISHME: variable-indexed builtin uniform access");

      this->result = get_builtin_uniform_reg(prog,
					     var->name,
					     index->value.i[0],
					     record ? record->field : NULL);
   }

   ir->array->accept(this);
   src_reg = this->result;

   if (index) {
      src_reg.index += index->value.i[0] * element_size;
   } else {
      ir_to_mesa_src_reg array_base = this->result;
      /* Variable index array dereference.  It eats the "vec4" of the
       * base of the array and an index that offsets the Mesa register
       * index.
       */
      ir->array_index->accept(this);

      ir_to_mesa_src_reg index_reg;

      if (element_size == 1) {
	 index_reg = this->result;
      } else {
	 index_reg = get_temp(glsl_type::float_type);

	 ir_to_mesa_emit_op2(ir, OPCODE_MUL,
			     ir_to_mesa_dst_reg_from_src(index_reg),
			     this->result, src_reg_for_float(element_size));
      }

      src_reg.reladdr = talloc(mem_ctx, ir_to_mesa_src_reg);
      memcpy(src_reg.reladdr, &index_reg, sizeof(index_reg));
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector())
      src_reg.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      src_reg.swizzle = SWIZZLE_NOOP;

   this->result = src_reg;
}

void
ir_to_mesa_visitor::visit(ir_dereference_record *ir)
{
   unsigned int i;
   const glsl_type *struct_type = ir->record->type;
   int offset = 0;
   ir_variable *var = ir->record->variable_referenced();

   if (strncmp(var->name, "gl_", 3) == 0 && var->mode == ir_var_uniform) {
      assert(var);

      this->result = get_builtin_uniform_reg(prog,
					     var->name,
					     0,
					     ir->field);
      return;
   }

   ir->record->accept(this);

   for (i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }
   this->result.swizzle = swizzle_for_size(ir->type->vector_elements);
   this->result.index += offset;
}

/**
 * We want to be careful in assignment setup to hit the actual storage
 * instead of potentially using a temporary like we might with the
 * ir_dereference handler.
 *
 * Thanks to ir_swizzle_swizzle, and ir_vec_index_to_swizzle, we
 * should only see potentially one variable array index of a vector,
 * and one swizzle, before getting to actual vec4 storage.  So handle
 * those, then go use ir_dereference to handle the rest.
 */
static struct ir_to_mesa_dst_reg
get_assignment_lhs(ir_instruction *ir, ir_to_mesa_visitor *v,
		   ir_to_mesa_src_reg *r)
{
   struct ir_to_mesa_dst_reg dst_reg;
   ir_swizzle *swiz;

   ir_dereference_array *deref_array = ir->as_dereference_array();
   /* This should have been handled by ir_vec_index_to_cond_assign */
   if (deref_array) {
      assert(!deref_array->array->type->is_vector());
   }

   /* Use the rvalue deref handler for the most part.  We'll ignore
    * swizzles in it and write swizzles using writemask, though.
    */
   ir->accept(v);
   dst_reg = ir_to_mesa_dst_reg_from_src(v->result);

   if ((swiz = ir->as_swizzle())) {
      int swizzles[4] = {
	 swiz->mask.x,
	 swiz->mask.y,
	 swiz->mask.z,
	 swiz->mask.w
      };
      int new_r_swizzle[4];
      int orig_r_swizzle = r->swizzle;
      int i;

      for (i = 0; i < 4; i++) {
	 new_r_swizzle[i] = GET_SWZ(orig_r_swizzle, 0);
      }

      dst_reg.writemask = 0;
      for (i = 0; i < 4; i++) {
	 if (i < swiz->mask.num_components) {
	    dst_reg.writemask |= 1 << swizzles[i];
	    new_r_swizzle[swizzles[i]] = GET_SWZ(orig_r_swizzle, i);
	 }
      }

      r->swizzle = MAKE_SWIZZLE4(new_r_swizzle[0],
				 new_r_swizzle[1],
				 new_r_swizzle[2],
				 new_r_swizzle[3]);
   }

   return dst_reg;
}

void
ir_to_mesa_visitor::visit(ir_assignment *ir)
{
   struct ir_to_mesa_dst_reg l;
   struct ir_to_mesa_src_reg r;
   int i;

   ir->rhs->accept(this);
   r = this->result;

   l = get_assignment_lhs(ir->lhs, this, &r);

   assert(l.file != PROGRAM_UNDEFINED);
   assert(r.file != PROGRAM_UNDEFINED);

   if (ir->condition) {
      ir_to_mesa_src_reg condition;

      ir->condition->accept(this);
      condition = this->result;

      /* We use the OPCODE_CMP (a < 0 ? b : c) for conditional moves,
       * and the condition we produced is 0.0 or 1.0.  By flipping the
       * sign, we can choose which value OPCODE_CMP produces without
       * an extra computing the condition.
       */
      condition.negate = ~condition.negate;
      for (i = 0; i < type_size(ir->lhs->type); i++) {
	 ir_to_mesa_emit_op3(ir, OPCODE_CMP, l,
			     condition, r, ir_to_mesa_src_reg_from_dst(l));
	 l.index++;
	 r.index++;
      }
   } else {
      for (i = 0; i < type_size(ir->lhs->type); i++) {
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	 l.index++;
	 r.index++;
      }
   }
}


void
ir_to_mesa_visitor::visit(ir_constant *ir)
{
   ir_to_mesa_src_reg src_reg;
   GLfloat stack_vals[4];
   GLfloat *values = stack_vals;
   unsigned int i;

   /* Unfortunately, 4 floats is all we can get into
    * _mesa_add_unnamed_constant.  So, make a temp to store an
    * aggregate constant and move each constant value into it.  If we
    * get lucky, copy propagation will eliminate the extra moves.
    */

   if (ir->type->base_type == GLSL_TYPE_STRUCT) {
      ir_to_mesa_src_reg temp_base = get_temp(ir->type);
      ir_to_mesa_dst_reg temp = ir_to_mesa_dst_reg_from_src(temp_base);

      foreach_iter(exec_list_iterator, iter, ir->components) {
	 ir_constant *field_value = (ir_constant *)iter.get();
	 int size = type_size(field_value->type);

	 assert(size > 0);

	 field_value->accept(this);
	 src_reg = this->result;

	 for (i = 0; i < (unsigned int)size; i++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, temp, src_reg);

	    src_reg.index++;
	    temp.index++;
	 }
      }
      this->result = temp_base;
      return;
   }

   if (ir->type->is_array()) {
      ir_to_mesa_src_reg temp_base = get_temp(ir->type);
      ir_to_mesa_dst_reg temp = ir_to_mesa_dst_reg_from_src(temp_base);
      int size = type_size(ir->type->fields.array);

      assert(size > 0);

      for (i = 0; i < ir->type->length; i++) {
	 ir->array_elements[i]->accept(this);
	 src_reg = this->result;
	 for (int j = 0; j < size; j++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, temp, src_reg);

	    src_reg.index++;
	    temp.index++;
	 }
      }
      this->result = temp_base;
      return;
   }

   if (ir->type->is_matrix()) {
      ir_to_mesa_src_reg mat = get_temp(ir->type);
      ir_to_mesa_dst_reg mat_column = ir_to_mesa_dst_reg_from_src(mat);

      for (i = 0; i < ir->type->matrix_columns; i++) {
	 assert(ir->type->base_type == GLSL_TYPE_FLOAT);
	 values = &ir->value.f[i * ir->type->vector_elements];

	 src_reg = ir_to_mesa_src_reg(PROGRAM_CONSTANT, -1, NULL);
	 src_reg.index = _mesa_add_unnamed_constant(this->prog->Parameters,
						values,
						ir->type->vector_elements,
						&src_reg.swizzle);
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, mat_column, src_reg);

	 mat_column.index++;
      }

      this->result = mat;
   }

   src_reg.file = PROGRAM_CONSTANT;
   switch (ir->type->base_type) {
   case GLSL_TYPE_FLOAT:
      values = &ir->value.f[0];
      break;
   case GLSL_TYPE_UINT:
      for (i = 0; i < ir->type->vector_elements; i++) {
	 values[i] = ir->value.u[i];
      }
      break;
   case GLSL_TYPE_INT:
      for (i = 0; i < ir->type->vector_elements; i++) {
	 values[i] = ir->value.i[i];
      }
      break;
   case GLSL_TYPE_BOOL:
      for (i = 0; i < ir->type->vector_elements; i++) {
	 values[i] = ir->value.b[i];
      }
      break;
   default:
      assert(!"Non-float/uint/int/bool constant");
   }

   this->result = ir_to_mesa_src_reg(PROGRAM_CONSTANT, -1, ir->type);
   this->result.index = _mesa_add_unnamed_constant(this->prog->Parameters,
						   values,
						   ir->type->vector_elements,
						   &this->result.swizzle);
}

function_entry *
ir_to_mesa_visitor::get_function_signature(ir_function_signature *sig)
{
   function_entry *entry;

   foreach_iter(exec_list_iterator, iter, this->function_signatures) {
      entry = (function_entry *)iter.get();

      if (entry->sig == sig)
	 return entry;
   }

   entry = talloc(mem_ctx, function_entry);
   entry->sig = sig;
   entry->sig_id = this->next_signature_id++;
   entry->bgn_inst = NULL;

   /* Allocate storage for all the parameters. */
   foreach_iter(exec_list_iterator, iter, sig->parameters) {
      ir_variable *param = (ir_variable *)iter.get();
      variable_storage *storage;

      storage = find_variable_storage(param);
      assert(!storage);

      storage = new(mem_ctx) variable_storage(param, PROGRAM_TEMPORARY,
					      this->next_temp);
      this->variables.push_tail(storage);

      this->next_temp += type_size(param->type);
   }

   if (!sig->return_type->is_void()) {
      entry->return_reg = get_temp(sig->return_type);
   } else {
      entry->return_reg = ir_to_mesa_undef;
   }

   this->function_signatures.push_tail(entry);
   return entry;
}

void
ir_to_mesa_visitor::visit(ir_call *ir)
{
   ir_to_mesa_instruction *call_inst;
   ir_function_signature *sig = ir->get_callee();
   function_entry *entry = get_function_signature(sig);
   int i;

   /* Process in parameters. */
   exec_list_iterator sig_iter = sig->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param_rval = (ir_rvalue *)iter.get();
      ir_variable *param = (ir_variable *)sig_iter.get();

      if (param->mode == ir_var_in ||
	  param->mode == ir_var_inout) {
	 variable_storage *storage = find_variable_storage(param);
	 assert(storage);

	 param_rval->accept(this);
	 ir_to_mesa_src_reg r = this->result;

	 ir_to_mesa_dst_reg l;
	 l.file = storage->file;
	 l.index = storage->index;
	 l.reladdr = NULL;
	 l.writemask = WRITEMASK_XYZW;
	 l.cond_mask = COND_TR;

	 for (i = 0; i < type_size(param->type); i++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	    l.index++;
	    r.index++;
	 }
      }

      sig_iter.next();
   }
   assert(!sig_iter.has_next());

   /* Emit call instruction */
   call_inst = ir_to_mesa_emit_op1(ir, OPCODE_CAL,
				   ir_to_mesa_undef_dst, ir_to_mesa_undef);
   call_inst->function = entry;

   /* Process out parameters. */
   sig_iter = sig->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param_rval = (ir_rvalue *)iter.get();
      ir_variable *param = (ir_variable *)sig_iter.get();

      if (param->mode == ir_var_out ||
	  param->mode == ir_var_inout) {
	 variable_storage *storage = find_variable_storage(param);
	 assert(storage);

	 ir_to_mesa_src_reg r;
	 r.file = storage->file;
	 r.index = storage->index;
	 r.reladdr = NULL;
	 r.swizzle = SWIZZLE_NOOP;
	 r.negate = 0;

	 param_rval->accept(this);
	 ir_to_mesa_dst_reg l = ir_to_mesa_dst_reg_from_src(this->result);

	 for (i = 0; i < type_size(param->type); i++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	    l.index++;
	    r.index++;
	 }
      }

      sig_iter.next();
   }
   assert(!sig_iter.has_next());

   /* Process return value. */
   this->result = entry->return_reg;
}


void
ir_to_mesa_visitor::visit(ir_texture *ir)
{
   ir_to_mesa_src_reg result_src, coord, lod_info, projector;
   ir_to_mesa_dst_reg result_dst, coord_dst;
   ir_to_mesa_instruction *inst = NULL;
   prog_opcode opcode = OPCODE_NOP;

   ir->coordinate->accept(this);

   /* Put our coords in a temp.  We'll need to modify them for shadow,
    * projection, or LOD, so the only case we'd use it as is is if
    * we're doing plain old texturing.  Mesa IR optimization should
    * handle cleaning up our mess in that case.
    */
   coord = get_temp(glsl_type::vec4_type);
   coord_dst = ir_to_mesa_dst_reg_from_src(coord);
   ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst,
		       this->result);

   if (ir->projector) {
      ir->projector->accept(this);
      projector = this->result;
   }

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = get_temp(glsl_type::vec4_type);
   result_dst = ir_to_mesa_dst_reg_from_src(result_src);

   switch (ir->op) {
   case ir_tex:
      opcode = OPCODE_TEX;
      break;
   case ir_txb:
      opcode = OPCODE_TXB;
      ir->lod_info.bias->accept(this);
      lod_info = this->result;
      break;
   case ir_txl:
      opcode = OPCODE_TXL;
      ir->lod_info.lod->accept(this);
      lod_info = this->result;
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }

   if (ir->projector) {
      if (opcode == OPCODE_TEX) {
	 /* Slot the projector in as the last component of the coord. */
	 coord_dst.writemask = WRITEMASK_W;
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst, projector);
	 coord_dst.writemask = WRITEMASK_XYZW;
	 opcode = OPCODE_TXP;
      } else {
	 ir_to_mesa_src_reg coord_w = coord;
	 coord_w.swizzle = SWIZZLE_WWWW;

	 /* For the other TEX opcodes there's no projective version
	  * since the last slot is taken up by lod info.  Do the
	  * projective divide now.
	  */
	 coord_dst.writemask = WRITEMASK_W;
	 ir_to_mesa_emit_op1(ir, OPCODE_RCP, coord_dst, projector);

	 coord_dst.writemask = WRITEMASK_XYZ;
	 ir_to_mesa_emit_op2(ir, OPCODE_MUL, coord_dst, coord, coord_w);

	 coord_dst.writemask = WRITEMASK_XYZW;
	 coord.swizzle = SWIZZLE_XYZW;
      }
   }

   if (ir->shadow_comparitor) {
      /* Slot the shadow value in as the second to last component of the
       * coord.
       */
      ir->shadow_comparitor->accept(this);
      coord_dst.writemask = WRITEMASK_Z;
      ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst, this->result);
      coord_dst.writemask = WRITEMASK_XYZW;
   }

   if (opcode == OPCODE_TXL || opcode == OPCODE_TXB) {
      /* Mesa IR stores lod or lod bias in the last channel of the coords. */
      coord_dst.writemask = WRITEMASK_W;
      ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst, lod_info);
      coord_dst.writemask = WRITEMASK_XYZW;
   }

   inst = ir_to_mesa_emit_op1(ir, opcode, result_dst, coord);

   if (ir->shadow_comparitor)
      inst->tex_shadow = GL_TRUE;

   ir_dereference_variable *sampler = ir->sampler->as_dereference_variable();
   assert(sampler); /* FINISHME: sampler arrays */
   /* generate the mapping, remove when we generate storage at
    * declaration time
    */
   sampler->accept(this);

   inst->sampler = get_sampler_number(sampler->var->location);

   switch (sampler->type->sampler_dimensionality) {
   case GLSL_SAMPLER_DIM_1D:
      inst->tex_target = TEXTURE_1D_INDEX;
      break;
   case GLSL_SAMPLER_DIM_2D:
      inst->tex_target = TEXTURE_2D_INDEX;
      break;
   case GLSL_SAMPLER_DIM_3D:
      inst->tex_target = TEXTURE_3D_INDEX;
      break;
   case GLSL_SAMPLER_DIM_CUBE:
      inst->tex_target = TEXTURE_CUBE_INDEX;
      break;
   default:
      assert(!"FINISHME: other texture targets");
   }

   this->result = result_src;
}

void
ir_to_mesa_visitor::visit(ir_return *ir)
{
   assert(current_function);

   if (ir->get_value()) {
      ir_to_mesa_dst_reg l;
      int i;

      ir->get_value()->accept(this);
      ir_to_mesa_src_reg r = this->result;

      l = ir_to_mesa_dst_reg_from_src(current_function->return_reg);

      for (i = 0; i < type_size(current_function->sig->return_type); i++) {
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	 l.index++;
	 r.index++;
      }
   }

   ir_to_mesa_emit_op0(ir, OPCODE_RET);
}

void
ir_to_mesa_visitor::visit(ir_discard *ir)
{
   assert(ir->condition == NULL); /* FINISHME */

   ir_to_mesa_emit_op0(ir, OPCODE_KIL_NV);
}

void
ir_to_mesa_visitor::visit(ir_if *ir)
{
   ir_to_mesa_instruction *cond_inst, *if_inst, *else_inst = NULL;
   ir_to_mesa_instruction *prev_inst;

   prev_inst = (ir_to_mesa_instruction *)this->instructions.get_tail();

   ir->condition->accept(this);
   assert(this->result.file != PROGRAM_UNDEFINED);

   if (ctx->Shader.EmitCondCodes) {
      cond_inst = (ir_to_mesa_instruction *)this->instructions.get_tail();

      /* See if we actually generated any instruction for generating
       * the condition.  If not, then cook up a move to a temp so we
       * have something to set cond_update on.
       */
      if (cond_inst == prev_inst) {
	 ir_to_mesa_src_reg temp = get_temp(glsl_type::bool_type);
	 cond_inst = ir_to_mesa_emit_op1(ir->condition, OPCODE_MOV,
					 ir_to_mesa_dst_reg_from_src(temp),
					 result);
      }
      cond_inst->cond_update = GL_TRUE;

      if_inst = ir_to_mesa_emit_op0(ir->condition, OPCODE_IF);
      if_inst->dst_reg.cond_mask = COND_NE;
   } else {
      if_inst = ir_to_mesa_emit_op1(ir->condition,
				    OPCODE_IF, ir_to_mesa_undef_dst,
				    this->result);
   }

   this->instructions.push_tail(if_inst);

   visit_exec_list(&ir->then_instructions, this);

   if (!ir->else_instructions.is_empty()) {
      else_inst = ir_to_mesa_emit_op0(ir->condition, OPCODE_ELSE);
      visit_exec_list(&ir->else_instructions, this);
   }

   if_inst = ir_to_mesa_emit_op1(ir->condition, OPCODE_ENDIF,
				 ir_to_mesa_undef_dst, ir_to_mesa_undef);
}

ir_to_mesa_visitor::ir_to_mesa_visitor()
{
   result.file = PROGRAM_UNDEFINED;
   next_temp = 1;
   next_signature_id = 1;
   sampler_map = NULL;
   sampler_map_size = 0;
   current_function = NULL;
}

static struct prog_src_register
mesa_src_reg_from_ir_src_reg(ir_to_mesa_src_reg reg)
{
   struct prog_src_register mesa_reg;

   mesa_reg.File = reg.file;
   assert(reg.index < (1 << INST_INDEX_BITS) - 1);
   mesa_reg.Index = reg.index;
   mesa_reg.Swizzle = reg.swizzle;
   mesa_reg.RelAddr = reg.reladdr != NULL;
   mesa_reg.Negate = reg.negate;
   mesa_reg.Abs = 0;

   return mesa_reg;
}

static void
set_branchtargets(ir_to_mesa_visitor *v,
		  struct prog_instruction *mesa_instructions,
		  int num_instructions)
{
   int if_count = 0, loop_count = 0;
   int *if_stack, *loop_stack;
   int if_stack_pos = 0, loop_stack_pos = 0;
   int i, j;

   for (i = 0; i < num_instructions; i++) {
      switch (mesa_instructions[i].Opcode) {
      case OPCODE_IF:
	 if_count++;
	 break;
      case OPCODE_BGNLOOP:
	 loop_count++;
	 break;
      case OPCODE_BRK:
      case OPCODE_CONT:
	 mesa_instructions[i].BranchTarget = -1;
	 break;
      default:
	 break;
      }
   }

   if_stack = (int *)calloc(if_count, sizeof(*if_stack));
   loop_stack = (int *)calloc(loop_count, sizeof(*loop_stack));

   for (i = 0; i < num_instructions; i++) {
      switch (mesa_instructions[i].Opcode) {
      case OPCODE_IF:
	 if_stack[if_stack_pos] = i;
	 if_stack_pos++;
	 break;
      case OPCODE_ELSE:
	 mesa_instructions[if_stack[if_stack_pos - 1]].BranchTarget = i;
	 if_stack[if_stack_pos - 1] = i;
	 break;
      case OPCODE_ENDIF:
	 mesa_instructions[if_stack[if_stack_pos - 1]].BranchTarget = i;
	 if_stack_pos--;
	 break;
      case OPCODE_BGNLOOP:
	 loop_stack[loop_stack_pos] = i;
	 loop_stack_pos++;
	 break;
      case OPCODE_ENDLOOP:
	 loop_stack_pos--;
	 /* Rewrite any breaks/conts at this nesting level (haven't
	  * already had a BranchTarget assigned) to point to the end
	  * of the loop.
	  */
	 for (j = loop_stack[loop_stack_pos]; j < i; j++) {
	    if (mesa_instructions[j].Opcode == OPCODE_BRK ||
		mesa_instructions[j].Opcode == OPCODE_CONT) {
	       if (mesa_instructions[j].BranchTarget == -1) {
		  mesa_instructions[j].BranchTarget = i;
	       }
	    }
	 }
	 /* The loop ends point at each other. */
	 mesa_instructions[i].BranchTarget = loop_stack[loop_stack_pos];
	 mesa_instructions[loop_stack[loop_stack_pos]].BranchTarget = i;
	 break;
      case OPCODE_CAL:
	 foreach_iter(exec_list_iterator, iter, v->function_signatures) {
	    function_entry *entry = (function_entry *)iter.get();

	    if (entry->sig_id == mesa_instructions[i].BranchTarget) {
	       mesa_instructions[i].BranchTarget = entry->inst;
	       break;
	    }
	 }
	 break;
      default:
	 break;
      }
   }

   free(if_stack);
}

static void
print_program(struct prog_instruction *mesa_instructions,
	      ir_instruction **mesa_instruction_annotation,
	      int num_instructions)
{
   ir_instruction *last_ir = NULL;
   int i;
   int indent = 0;

   for (i = 0; i < num_instructions; i++) {
      struct prog_instruction *mesa_inst = mesa_instructions + i;
      ir_instruction *ir = mesa_instruction_annotation[i];

      fprintf(stdout, "%3d: ", i);

      if (last_ir != ir && ir) {
	 int j;

	 for (j = 0; j < indent; j++) {
	    fprintf(stdout, " ");
	 }
	 ir->print();
	 printf("\n");
	 last_ir = ir;

	 fprintf(stdout, "     "); /* line number spacing. */
      }

      indent = _mesa_fprint_instruction_opt(stdout, mesa_inst, indent,
					    PROG_PRINT_DEBUG, NULL);
   }
}

static void
mark_input(struct gl_program *prog,
	   int index,
	   GLboolean reladdr)
{
   prog->InputsRead |= BITFIELD64_BIT(index);
   int i;

   if (reladdr) {
      if (index >= FRAG_ATTRIB_TEX0 && index <= FRAG_ATTRIB_TEX7) {
	 for (i = 0; i < 8; i++) {
	    prog->InputsRead |= BITFIELD64_BIT(FRAG_ATTRIB_TEX0 + i);
	 }
      } else {
	 assert(!"FINISHME: Mark InputsRead for varying arrays");
      }
   }
}

static void
mark_output(struct gl_program *prog,
	   int index,
	   GLboolean reladdr)
{
   prog->OutputsWritten |= BITFIELD64_BIT(index);
   int i;

   if (reladdr) {
      if (index >= VERT_RESULT_TEX0 && index <= VERT_RESULT_TEX7) {
	 for (i = 0; i < 8; i++) {
	    prog->OutputsWritten |= BITFIELD64_BIT(FRAG_ATTRIB_TEX0 + i);
	 }
      } else {
	 assert(!"FINISHME: Mark OutputsWritten for varying arrays");
      }
   }
}

static void
count_resources(struct gl_program *prog)
{
   unsigned int i;

   prog->InputsRead = 0;
   prog->OutputsWritten = 0;
   prog->SamplersUsed = 0;

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = &prog->Instructions[i];
      unsigned int reg;

      switch (inst->DstReg.File) {
      case PROGRAM_OUTPUT:
	 mark_output(prog, inst->DstReg.Index, inst->DstReg.RelAddr);
	 break;
      case PROGRAM_INPUT:
	 mark_input(prog, inst->DstReg.Index, inst->DstReg.RelAddr);
	 break;
      default:
	 break;
      }

      for (reg = 0; reg < _mesa_num_inst_src_regs(inst->Opcode); reg++) {
	 switch (inst->SrcReg[reg].File) {
	 case PROGRAM_OUTPUT:
	    mark_output(prog, inst->SrcReg[reg].Index,
			inst->SrcReg[reg].RelAddr);
	    break;
	 case PROGRAM_INPUT:
	    mark_input(prog, inst->SrcReg[reg].Index, inst->SrcReg[reg].RelAddr);
	    break;
	 default:
	    break;
	 }
      }

      /* Instead of just using the uniform's value to map to a
       * sampler, Mesa first allocates a separate number for the
       * sampler (_mesa_add_sampler), then we reindex it down to a
       * small integer (sampler_map[], SamplersUsed), then that gets
       * mapped to the uniform's value, and we get an actual sampler.
       */
      if (_mesa_is_tex_instruction(inst->Opcode)) {
	 prog->SamplerTargets[inst->TexSrcUnit] =
	    (gl_texture_index)inst->TexSrcTarget;
	 prog->SamplersUsed |= 1 << inst->TexSrcUnit;
	 if (inst->TexShadow) {
	    prog->ShadowSamplers |= 1 << inst->TexSrcUnit;
	 }
      }
   }

   _mesa_update_shader_textures_used(prog);
}

/* Each stage has some uniforms in its Parameters list.  The Uniforms
 * list for the linked shader program has a pointer to these uniforms
 * in each of the stage's Parameters list, so that their values can be
 * updated when a uniform is set.
 */
static void
link_uniforms_to_shared_uniform_list(struct gl_uniform_list *uniforms,
				     struct gl_program *prog)
{
   unsigned int i;

   for (i = 0; i < prog->Parameters->NumParameters; i++) {
      const struct gl_program_parameter *p = prog->Parameters->Parameters + i;

      if (p->Type == PROGRAM_UNIFORM || p->Type == PROGRAM_SAMPLER) {
	 struct gl_uniform *uniform =
	    _mesa_append_uniform(uniforms, p->Name, prog->Target, i);
	 if (uniform)
	    uniform->Initialized = p->Initialized;
      }
   }
}

struct gl_program *
get_mesa_program(GLcontext *ctx, struct gl_shader_program *shader_program,
		 struct gl_shader *shader)
{
   void *mem_ctx = shader_program;
   ir_to_mesa_visitor v;
   struct prog_instruction *mesa_instructions, *mesa_inst;
   ir_instruction **mesa_instruction_annotation;
   int i;
   struct gl_program *prog;
   GLenum target;
   const char *target_string;
   GLboolean progress;

   switch (shader->Type) {
   case GL_VERTEX_SHADER:
      target = GL_VERTEX_PROGRAM_ARB;
      target_string = "vertex";
      break;
   case GL_FRAGMENT_SHADER:
      target = GL_FRAGMENT_PROGRAM_ARB;
      target_string = "fragment";
      break;
   default:
      assert(!"should not be reached");
      break;
   }

   validate_ir_tree(shader->ir);

   prog = ctx->Driver.NewProgram(ctx, target, shader_program->Name);
   if (!prog)
      return NULL;
   prog->Parameters = _mesa_new_parameter_list();
   prog->Varying = _mesa_new_parameter_list();
   prog->Attributes = _mesa_new_parameter_list();
   v.ctx = ctx;
   v.prog = prog;

   v.mem_ctx = talloc_new(NULL);

   /* Emit Mesa IR for main(). */
   visit_exec_list(shader->ir, &v);
   v.ir_to_mesa_emit_op0(NULL, OPCODE_END);

   /* Now emit bodies for any functions that were used. */
   do {
      progress = GL_FALSE;

      foreach_iter(exec_list_iterator, iter, v.function_signatures) {
	 function_entry *entry = (function_entry *)iter.get();

	 if (!entry->bgn_inst) {
	    v.current_function = entry;

	    entry->bgn_inst = v.ir_to_mesa_emit_op0(NULL, OPCODE_BGNSUB);
	    entry->bgn_inst->function = entry;

	    visit_exec_list(&entry->sig->body, &v);

	    ir_to_mesa_instruction *last;
	    last = (ir_to_mesa_instruction *)v.instructions.get_tail();
	    if (last->op != OPCODE_RET)
	       v.ir_to_mesa_emit_op0(NULL, OPCODE_RET);

	    ir_to_mesa_instruction *end;
	    end = v.ir_to_mesa_emit_op0(NULL, OPCODE_ENDSUB);
	    end->function = entry;

	    progress = GL_TRUE;
	 }
      }
   } while (progress);

   prog->NumTemporaries = v.next_temp;

   int num_instructions = 0;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      num_instructions++;
   }

   mesa_instructions =
      (struct prog_instruction *)calloc(num_instructions,
					sizeof(*mesa_instructions));
   mesa_instruction_annotation = talloc_array(mem_ctx, ir_instruction *,
					      num_instructions);

   mesa_inst = mesa_instructions;
   i = 0;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      ir_to_mesa_instruction *inst = (ir_to_mesa_instruction *)iter.get();

      mesa_inst->Opcode = inst->op;
      mesa_inst->CondUpdate = inst->cond_update;
      mesa_inst->DstReg.File = inst->dst_reg.file;
      mesa_inst->DstReg.Index = inst->dst_reg.index;
      mesa_inst->DstReg.CondMask = inst->dst_reg.cond_mask;
      mesa_inst->DstReg.WriteMask = inst->dst_reg.writemask;
      mesa_inst->DstReg.RelAddr = inst->dst_reg.reladdr != NULL;
      mesa_inst->SrcReg[0] = mesa_src_reg_from_ir_src_reg(inst->src_reg[0]);
      mesa_inst->SrcReg[1] = mesa_src_reg_from_ir_src_reg(inst->src_reg[1]);
      mesa_inst->SrcReg[2] = mesa_src_reg_from_ir_src_reg(inst->src_reg[2]);
      mesa_inst->TexSrcUnit = inst->sampler;
      mesa_inst->TexSrcTarget = inst->tex_target;
      mesa_inst->TexShadow = inst->tex_shadow;
      mesa_instruction_annotation[i] = inst->ir;

      if (ctx->Shader.EmitNoIfs && mesa_inst->Opcode == OPCODE_IF) {
	 shader_program->InfoLog =
	    talloc_asprintf_append(shader_program->InfoLog,
				   "Couldn't flatten if statement\n");
	 shader_program->LinkStatus = false;
      }

      switch (mesa_inst->Opcode) {
      case OPCODE_BGNSUB:
	 inst->function->inst = i;
	 mesa_inst->Comment = strdup(inst->function->sig->function_name());
	 break;
      case OPCODE_ENDSUB:
	 mesa_inst->Comment = strdup(inst->function->sig->function_name());
	 break;
      case OPCODE_CAL:
	 mesa_inst->BranchTarget = inst->function->sig_id; /* rewritten later */
	 break;
      case OPCODE_ARL:
	 prog->NumAddressRegs = 1;
	 break;
      default:
	 break;
      }

      mesa_inst++;
      i++;
   }

   set_branchtargets(&v, mesa_instructions, num_instructions);
   if (ctx->Shader.Flags & GLSL_DUMP) {
      printf("Mesa %s program:\n", target_string);
      print_program(mesa_instructions, mesa_instruction_annotation,
		    num_instructions);
   }

   prog->Instructions = mesa_instructions;
   prog->NumInstructions = num_instructions;

   _mesa_reference_program(ctx, &shader->Program, prog);

   if ((ctx->Shader.Flags & GLSL_NO_OPT) == 0) {
      _mesa_optimize_program(ctx, prog);
   }

   return prog;
}

extern "C" {

void
_mesa_glsl_compile_shader(GLcontext *ctx, struct gl_shader *shader)
{
   struct _mesa_glsl_parse_state *state =
      new(shader) _mesa_glsl_parse_state(ctx, shader->Type, shader);

   const char *source = shader->Source;
   state->error = preprocess(state, &source, &state->info_log,
			     &ctx->Extensions);

   if (!state->error) {
     _mesa_glsl_lexer_ctor(state, source);
     _mesa_glsl_parse(state);
     _mesa_glsl_lexer_dtor(state);
   }

   shader->ir = new(shader) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
      _mesa_ast_to_hir(shader->ir, state);

   if (!state->error && !shader->ir->is_empty()) {
      validate_ir_tree(shader->ir);

      /* Lowering */
      do_mat_op_to_vec(shader->ir);
      do_mod_to_fract(shader->ir);
      do_div_to_mul_rcp(shader->ir);

      /* Optimization passes */
      bool progress;
      do {
	 progress = false;

	 progress = do_function_inlining(shader->ir) || progress;
	 progress = do_if_simplification(shader->ir) || progress;
	 progress = do_copy_propagation(shader->ir) || progress;
	 progress = do_dead_code_local(shader->ir) || progress;
	 progress = do_dead_code_unlinked(shader->ir) || progress;
	 progress = do_tree_grafting(shader->ir) || progress;
	 progress = do_constant_variable_unlinked(shader->ir) || progress;
	 progress = do_constant_folding(shader->ir) || progress;
	 progress = do_algebraic(shader->ir) || progress;
	 progress = do_if_return(shader->ir) || progress;
	 if (ctx->Shader.EmitNoIfs)
	    progress = do_if_to_cond_assign(shader->ir) || progress;

	 progress = do_vec_index_to_swizzle(shader->ir) || progress;
	 /* Do this one after the previous to let the easier pass handle
	  * constant vector indexing.
	  */
	 progress = do_vec_index_to_cond_assign(shader->ir) || progress;

	 progress = do_swizzle_swizzle(shader->ir) || progress;
      } while (progress);

      validate_ir_tree(shader->ir);
   }

   shader->symbols = state->symbols;

   shader->CompileStatus = !state->error;
   shader->InfoLog = state->info_log;
   shader->Version = state->language_version;
   memcpy(shader->builtins_to_link, state->builtins_to_link,
	  sizeof(shader->builtins_to_link[0]) * state->num_builtins_to_link);
   shader->num_builtins_to_link = state->num_builtins_to_link;

   if (ctx->Shader.Flags & GLSL_LOG) {
      _mesa_write_shader_to_file(shader);
   }

   /* Retain any live IR, but trash the rest. */
   reparent_ir(shader->ir, shader);

   talloc_free(state);
 }

void
_mesa_glsl_link_shader(GLcontext *ctx, struct gl_shader_program *prog)
{
   unsigned int i;

   _mesa_clear_shader_program_data(ctx, prog);

   prog->LinkStatus = GL_TRUE;

   for (i = 0; i < prog->NumShaders; i++) {
      if (!prog->Shaders[i]->CompileStatus) {
	 prog->InfoLog =
	    talloc_asprintf_append(prog->InfoLog,
				   "linking with uncompiled shader");
	 prog->LinkStatus = GL_FALSE;
      }
   }

   prog->Varying = _mesa_new_parameter_list();
   _mesa_reference_vertprog(ctx, &prog->VertexProgram, NULL);
   _mesa_reference_fragprog(ctx, &prog->FragmentProgram, NULL);

   if (prog->LinkStatus) {
      link_shaders(prog);

      /* We don't use the linker's uniforms list, and cook up our own at
       * generate time.
       */
      free(prog->Uniforms);
      prog->Uniforms = _mesa_new_uniform_list();
   }

   if (prog->LinkStatus) {
      for (i = 0; i < prog->_NumLinkedShaders; i++) {
	 struct gl_program *linked_prog;
	 bool ok = true;

	 linked_prog = get_mesa_program(ctx, prog,
					prog->_LinkedShaders[i]);
	 count_resources(linked_prog);

	 link_uniforms_to_shared_uniform_list(prog->Uniforms, linked_prog);

	 switch (prog->_LinkedShaders[i]->Type) {
	 case GL_VERTEX_SHADER:
	    _mesa_reference_vertprog(ctx, &prog->VertexProgram,
				     (struct gl_vertex_program *)linked_prog);
	    ok = ctx->Driver.ProgramStringNotify(ctx, GL_VERTEX_PROGRAM_ARB,
						 linked_prog);
	    break;
	 case GL_FRAGMENT_SHADER:
	    _mesa_reference_fragprog(ctx, &prog->FragmentProgram,
				     (struct gl_fragment_program *)linked_prog);
	    ok = ctx->Driver.ProgramStringNotify(ctx, GL_FRAGMENT_PROGRAM_ARB,
						 linked_prog);
	    break;
	 }
	 if (!ok) {
	    prog->LinkStatus = GL_FALSE;
	 }
      }
   }
}

} /* extern "C" */
