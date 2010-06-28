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
#include "shader/prog_instruction.h"
#include "shader/prog_print.h"
#include "shader/program.h"
#include "shader/prog_uniform.h"
#include "shader/prog_parameter.h"
#include "shader/shader_api.h"
}

/**
 * This struct is a corresponding struct to Mesa prog_src_register, with
 * wider fields.
 */
typedef struct ir_to_mesa_src_reg {
   int file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   GLuint swizzle; /**< SWIZZLE_XYZWONEZERO swizzles from Mesa. */
   int negate; /**< NEGATE_XYZW mask from mesa */
   bool reladdr; /**< Register index should be offset by address reg. */
} ir_to_mesa_src_reg;

typedef struct ir_to_mesa_dst_reg {
   int file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   int writemask; /**< Bitfield of WRITEMASK_[XYZW] */
} ir_to_mesa_dst_reg;

extern ir_to_mesa_src_reg ir_to_mesa_undef;

class ir_to_mesa_instruction : public exec_node {
public:
   enum prog_opcode op;
   ir_to_mesa_dst_reg dst_reg;
   ir_to_mesa_src_reg src_reg[3];
   /** Pointer to the ir source this tree came from for debugging */
   ir_instruction *ir;
};

class temp_entry : public exec_node {
public:
   temp_entry(ir_variable *var, int file, int index)
      : file(file), index(index), var(var)
   {
      /* empty */
   }

   int file;
   int index;
   ir_variable *var; /* variable that maps to this, if any */
};

class ir_to_mesa_visitor : public ir_visitor {
public:
   ir_to_mesa_visitor();

   GLcontext *ctx;
   struct gl_program *prog;

   int next_temp;

   temp_entry *find_variable_storage(ir_variable *var);

   ir_to_mesa_src_reg get_temp(const glsl_type *type);

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
   virtual void visit(ir_texture *);
   virtual void visit(ir_if *);
   /*@}*/

   struct ir_to_mesa_src_reg result;

   /** List of temp_entry */
   exec_list variable_storage;

   /** List of ir_to_mesa_instruction */
   exec_list instructions;

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

   void *mem_ctx;
};

ir_to_mesa_src_reg ir_to_mesa_undef = {
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP, NEGATE_NONE, false,
};

ir_to_mesa_dst_reg ir_to_mesa_undef_dst = {
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP
};

ir_to_mesa_dst_reg ir_to_mesa_address_reg = {
   PROGRAM_ADDRESS, 0, WRITEMASK_X
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

/* This list should match up with builtin_variables.h */
static const struct {
   const char *name;
   int file;
   int index;
} builtin_var_to_mesa_reg[] = {
   /* core_vs */
   {"gl_Position", PROGRAM_OUTPUT, VERT_RESULT_HPOS},
   {"gl_PointSize", PROGRAM_OUTPUT, VERT_RESULT_PSIZ},

   /* core_fs */
   {"gl_FragCoord", PROGRAM_INPUT, FRAG_ATTRIB_WPOS},
   {"gl_FrontFacing", PROGRAM_INPUT, FRAG_ATTRIB_FACE},
   {"gl_FragColor", PROGRAM_OUTPUT, FRAG_ATTRIB_COL0},
   {"gl_FragDepth", PROGRAM_UNDEFINED, FRAG_ATTRIB_WPOS}, /* FINISHME: WPOS.z */

   /* 110_deprecated_fs */
   {"gl_Color", PROGRAM_INPUT, FRAG_ATTRIB_COL0},
   {"gl_SecondaryColor", PROGRAM_INPUT, FRAG_ATTRIB_COL1},
   {"gl_FogFragCoord", PROGRAM_INPUT, FRAG_ATTRIB_FOGC},
   {"gl_TexCoord", PROGRAM_INPUT, FRAG_ATTRIB_TEX0}, /* array */

   /* 110_deprecated_vs */
   {"gl_Vertex", PROGRAM_INPUT, VERT_ATTRIB_POS},
   {"gl_Normal", PROGRAM_INPUT, VERT_ATTRIB_NORMAL},
   {"gl_Color", PROGRAM_INPUT, VERT_ATTRIB_COLOR0},
   {"gl_SecondaryColor", PROGRAM_INPUT, VERT_ATTRIB_COLOR1},
   {"gl_MultiTexCoord0", PROGRAM_INPUT, VERT_ATTRIB_TEX0},
   {"gl_MultiTexCoord1", PROGRAM_INPUT, VERT_ATTRIB_TEX1},
   {"gl_MultiTexCoord2", PROGRAM_INPUT, VERT_ATTRIB_TEX2},
   {"gl_MultiTexCoord3", PROGRAM_INPUT, VERT_ATTRIB_TEX3},
   {"gl_MultiTexCoord4", PROGRAM_INPUT, VERT_ATTRIB_TEX4},
   {"gl_MultiTexCoord5", PROGRAM_INPUT, VERT_ATTRIB_TEX5},
   {"gl_MultiTexCoord6", PROGRAM_INPUT, VERT_ATTRIB_TEX6},
   {"gl_MultiTexCoord7", PROGRAM_INPUT, VERT_ATTRIB_TEX7},
   {"gl_TexCoord", PROGRAM_OUTPUT, VERT_RESULT_TEX0}, /* array */
   {"gl_FogCoord", PROGRAM_INPUT, VERT_RESULT_FOGC},
   /*{"gl_ClipVertex", PROGRAM_OUTPUT, VERT_ATTRIB_FOGC},*/ /* FINISHME */
   {"gl_FrontColor", PROGRAM_OUTPUT, VERT_RESULT_COL0},
   {"gl_BackColor", PROGRAM_OUTPUT, VERT_RESULT_BFC0},
   {"gl_FrontSecondaryColor", PROGRAM_OUTPUT, VERT_RESULT_COL1},
   {"gl_BackSecondaryColor", PROGRAM_OUTPUT, VERT_RESULT_BFC1},
   {"gl_FogFragCoord", PROGRAM_OUTPUT, VERT_RESULT_FOGC},

   /* 130_vs */
   /*{"gl_VertexID", PROGRAM_INPUT, VERT_ATTRIB_FOGC},*/ /* FINISHME */

   {"gl_FragData", PROGRAM_OUTPUT, FRAG_RESULT_DATA0}, /* array */
};

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op3(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0,
					ir_to_mesa_src_reg src1,
					ir_to_mesa_src_reg src2)
{
   ir_to_mesa_instruction *inst = new(mem_ctx) ir_to_mesa_instruction();

   inst->op = op;
   inst->dst_reg = dst;
   inst->src_reg[0] = src0;
   inst->src_reg[1] = src1;
   inst->src_reg[2] = src2;
   inst->ir = ir;

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

inline ir_to_mesa_dst_reg
ir_to_mesa_dst_reg_from_src(ir_to_mesa_src_reg reg)
{
   ir_to_mesa_dst_reg dst_reg;

   dst_reg.file = reg.file;
   dst_reg.index = reg.index;
   dst_reg.writemask = WRITEMASK_XYZW;

   return dst_reg;
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
ir_to_mesa_visitor::ir_to_mesa_emit_scalar_op1(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0)
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
      ir_to_mesa_src_reg src = src0;

      if (done_mask & this_mask)
	 continue;

      GLuint src_swiz = GET_SWZ(src.swizzle, i);
      for (j = i + 1; j < 4; j++) {
	 if (!(done_mask & (1 << j)) && GET_SWZ(src.swizzle, j) == src_swiz) {
	    this_mask |= (1 << j);
	 }
      }
      src.swizzle = MAKE_SWIZZLE4(src_swiz, src_swiz,
				  src_swiz, src_swiz);

      inst = ir_to_mesa_emit_op1(ir, op,
				 dst,
				 src);
      inst->dst_reg.writemask = this_mask;
      done_mask |= this_mask;
   }
}

struct ir_to_mesa_src_reg
ir_to_mesa_visitor::src_reg_for_float(float val)
{
   ir_to_mesa_src_reg src_reg;

   src_reg.file = PROGRAM_CONSTANT;
   src_reg.index = _mesa_add_unnamed_constant(this->prog->Parameters,
					      &val, 1, &src_reg.swizzle);

   return src_reg;
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

   assert(!type->is_array());

   src_reg.file = PROGRAM_TEMPORARY;
   src_reg.index = type->matrix_columns;
   src_reg.reladdr = false;

   for (i = 0; i < type->vector_elements; i++)
      swizzle[i] = i;
   for (; i < 4; i++)
      swizzle[i] = type->vector_elements - 1;
   src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
				   swizzle[2], swizzle[3]);

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
	 return 4; /* FINISHME: Not all matrices are 4x4. */
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

temp_entry *
ir_to_mesa_visitor::find_variable_storage(ir_variable *var)
{
   
   temp_entry *entry;

   foreach_iter(exec_list_iterator, iter, this->variable_storage) {
      entry = (temp_entry *)iter.get();

      if (entry->var == var)
	 return entry;
   }

   return NULL;
}

void
ir_to_mesa_visitor::visit(ir_variable *ir)
{
   (void)ir;
}

void
ir_to_mesa_visitor::visit(ir_loop *ir)
{
   assert(!ir->from);
   assert(!ir->to);
   assert(!ir->increment);
   assert(!ir->counter);

   ir_to_mesa_emit_op1(NULL, OPCODE_BGNLOOP,
		       ir_to_mesa_undef_dst, ir_to_mesa_undef);

   visit_exec_list(&ir->body_instructions, this);

   ir_to_mesa_emit_op1(NULL, OPCODE_ENDLOOP,
		       ir_to_mesa_undef_dst, ir_to_mesa_undef);
}

void
ir_to_mesa_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      ir_to_mesa_emit_op1(NULL, OPCODE_BRK,
			  ir_to_mesa_undef_dst, ir_to_mesa_undef);
      break;
   case ir_loop_jump::jump_continue:
      ir_to_mesa_emit_op1(NULL, OPCODE_CONT,
			  ir_to_mesa_undef_dst, ir_to_mesa_undef);
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

      /* Only expression implemented for matrices yet */
      assert(!ir->operands[operand]->type->is_matrix() ||
	     ir->operation == ir_binop_mul);
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
   case ir_unop_exp:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_EXP, result_dst, op[0]);
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
   case ir_binop_add:
      ir_to_mesa_emit_op2(ir, OPCODE_ADD, result_dst, op[0], op[1]);
      break;
   case ir_binop_sub:
      ir_to_mesa_emit_op2(ir, OPCODE_SUB, result_dst, op[0], op[1]);
      break;
   case ir_binop_mul:
      if (ir->operands[0]->type->is_matrix() &&
	  !ir->operands[1]->type->is_matrix()) {
	 if (ir->operands[1]->type->is_scalar()) {
	    ir_to_mesa_dst_reg dst_column = result_dst;
	    ir_to_mesa_src_reg src_column = op[0];
	    for (int i = 0; i < ir->operands[0]->type->matrix_columns; i++) {
	       ir_to_mesa_emit_op2(ir, OPCODE_MUL,
				   dst_column, src_column, op[1]);
	       dst_column.index++;
	       src_column.index++;
	    }
	 } else {
	    ir_to_mesa_src_reg src_column = op[0];
	    ir_to_mesa_src_reg src_chan = op[1];
	    assert(!ir->operands[1]->type->is_matrix() ||
		    !"FINISHME: matrix * matrix");
	     for (int i = 0; i < ir->operands[0]->type->matrix_columns; i++) {
		src_chan.swizzle = MAKE_SWIZZLE4(i, i, i, i);
		if (i == 0) {
		   ir_to_mesa_emit_op2(ir, OPCODE_MUL,
				       result_dst, src_column, src_chan);
		} else {
		   ir_to_mesa_emit_op3(ir, OPCODE_MAD,
				       result_dst, src_column, src_chan,
				       result_src);
		}
		src_column.index++;
	    }
	 }
      } else {
	 assert(!ir->operands[0]->type->is_matrix());
	 assert(!ir->operands[1]->type->is_matrix());
	 ir_to_mesa_emit_op2(ir, OPCODE_MUL, result_dst, op[0], op[1]);
      }
      break;
   case ir_binop_div:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RCP, result_dst, op[1]);
      ir_to_mesa_emit_op2(ir, OPCODE_MUL, result_dst, op[0], result_src);
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
   case ir_unop_sqrt:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RSQ, result_dst, op[0]);
      ir_to_mesa_emit_op1(ir, OPCODE_RCP, result_dst, result_src);
      break;
   case ir_unop_rsq:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RSQ, result_dst, op[0]);
      break;
   case ir_unop_i2f:
      /* Mesa IR lacks types, ints are stored as truncated floats. */
      result_src = op[0];
      break;
   case ir_unop_f2i:
      ir_to_mesa_emit_op1(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_f2b:
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
   case ir_binop_min:
      ir_to_mesa_emit_op2(ir, OPCODE_MIN, result_dst, op[0], op[1]);
      break;
   case ir_binop_max:
      ir_to_mesa_emit_op2(ir, OPCODE_MAX, result_dst, op[0], op[1]);
      break;
   default:
      ir_print_visitor v;
      printf("Failed to get tree for expression:\n");
      ir->accept(&v);
      exit(1);
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
	    swizzle[i] = ir->mask.x;
	    break;
	 case 1:
	    swizzle[i] = ir->mask.y;
	    break;
	 case 2:
	    swizzle[i] = ir->mask.z;
	    break;
	 case 3:
	    swizzle[i] = ir->mask.w;
	    break;
	 }
      } else {
	 /* If the type is smaller than a vec4, replicate the last
	  * channel out.
	  */
	 swizzle[i] = ir->type->vector_elements - 1;
      }
   }

   src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0],
				   swizzle[1],
				   swizzle[2],
				   swizzle[3]);

   this->result = src_reg;
}

static temp_entry *
get_builtin_matrix_ref(void *mem_ctx, struct gl_program *prog, ir_variable *var)
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
   temp_entry *entry;

   /* C++ gets angry when we try to use an int as a gl_state_index, so we use
    * ints for gl_state_index.  Make sure they're compatible.
    */
   assert(sizeof(gl_state_index) == sizeof(int));

   for (i = 0; i < Elements(matrices); i++) {
      if (strcmp(var->name, matrices[i].name) == 0) {
	 int j;
	 int last_pos = -1, base_pos = -1;
	 int tokens[STATE_LENGTH];

	 tokens[0] = matrices[i].matrix;
	 tokens[1] = 0; /* array index! */
	 tokens[4] = matrices[i].modifier;

	 /* Add a ref for each column.  It looks like the reason we do
	  * it this way is that _mesa_add_state_reference doesn't work
	  * for things that aren't vec4s, so the tokens[2]/tokens[3]
	  * range has to be equal.
	  */
	 for (j = 0; j < 4; j++) {
	    tokens[2] = j;
	    tokens[3] = j;
	    int pos = _mesa_add_state_reference(prog->Parameters,
						(gl_state_index *)tokens);
	    assert(last_pos == -1 || last_pos == base_pos + j);
	    if (base_pos == -1)
	       base_pos = pos;
	 }

	 entry = new(mem_ctx) temp_entry(var,
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
   ir_to_mesa_src_reg src_reg;
   temp_entry *entry = find_variable_storage(ir->var);
   unsigned int i, loc;
   bool var_in;

   if (!entry) {
      switch (ir->var->mode) {
      case ir_var_uniform:
	 entry = get_builtin_matrix_ref(this->mem_ctx, this->prog, ir->var);
	 if (entry)
	    break;

	 /* FINISHME: Fix up uniform name for arrays and things */
	 assert(ir->var->type->gl_type != 0 &&
		ir->var->type->gl_type != GL_INVALID_ENUM);
	 loc = _mesa_add_uniform(this->prog->Parameters,
				 ir->var->name,
				 type_size(ir->var->type) * 4,
				 ir->var->type->gl_type,
				 NULL);
	 /* Always mark the uniform used at this point.  If it isn't
	  * used, dead code elimination should have nuked the decl already.
	  */
	 this->prog->Parameters->Parameters[loc].Used = GL_TRUE;

	 entry = new(mem_ctx) temp_entry(ir->var, PROGRAM_UNIFORM, loc);
	 this->variable_storage.push_tail(entry);
	 break;
      case ir_var_in:
      case ir_var_out:
      case ir_var_inout:
	 var_in = (ir->var->mode == ir_var_in ||
		   ir->var->mode == ir_var_inout);

	 for (i = 0; i < ARRAY_SIZE(builtin_var_to_mesa_reg); i++) {
	    bool in = builtin_var_to_mesa_reg[i].file == PROGRAM_INPUT;

	    if (strcmp(ir->var->name, builtin_var_to_mesa_reg[i].name) == 0 &&
		!(var_in ^ in))
	       break;
	 }
	 if (i == ARRAY_SIZE(builtin_var_to_mesa_reg)) {
	    printf("Failed to find builtin for %s variable %s\n",
		   var_in ? "in" : "out",
		   ir->var->name);
	    abort();
	 }
	 entry = new(mem_ctx) temp_entry(ir->var,
					 builtin_var_to_mesa_reg[i].file,
					 builtin_var_to_mesa_reg[i].index);
	 break;
      case ir_var_auto:
	 entry = new(mem_ctx) temp_entry(ir->var, PROGRAM_TEMPORARY,
					 this->next_temp);
	 this->variable_storage.push_tail(entry);

	 next_temp += type_size(ir->var->type);
	 break;
      }

      if (!entry) {
	 printf("Failed to make storage for %s\n", ir->var->name);
	 exit(1);
      }
   }

   src_reg.file = entry->file;
   src_reg.index = entry->index;
   /* If the type is smaller than a vec4, replicate the last channel out. */
   src_reg.swizzle = swizzle_for_size(ir->var->type->vector_elements);
   src_reg.reladdr = false;
   src_reg.negate = 0;

   this->result = src_reg;
}

void
ir_to_mesa_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *index;
   ir_to_mesa_src_reg src_reg;

   index = ir->array_index->constant_expression_value();

   /* By the time we make it to this stage, matrices should be broken down
    * to vectors.
    */
   assert(!ir->type->is_matrix());

   ir->array->accept(this);
   src_reg = this->result;

   if (src_reg.file == PROGRAM_INPUT ||
       src_reg.file == PROGRAM_OUTPUT) {
      assert(index); /* FINISHME: Handle variable indexing of builtins. */

      src_reg.index += index->value.i[0];
   } else {
      if (index) {
	 src_reg.index += index->value.i[0];
      } else {
	 ir_to_mesa_src_reg array_base = this->result;
	 /* Variable index array dereference.  It eats the "vec4" of the
	  * base of the array and an index that offsets the Mesa register
	  * index.
	  */
	 ir->array_index->accept(this);

	 /* FINISHME: This doesn't work when we're trying to do the LHS
	  * of an assignment.
	  */
	 src_reg.reladdr = true;
	 ir_to_mesa_emit_op1(ir, OPCODE_ARL, ir_to_mesa_address_reg,
			     this->result);

	 this->result = get_temp(ir->type);
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV,
			     ir_to_mesa_dst_reg_from_src(this->result),
			     src_reg);
      }
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   src_reg.swizzle = swizzle_for_size(ir->type->vector_elements);

   this->result = src_reg;
}

void
ir_to_mesa_visitor::visit(ir_dereference_record *ir)
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
get_assignment_lhs(ir_instruction *ir, ir_to_mesa_visitor *v)
{
   struct ir_to_mesa_dst_reg dst_reg;
   ir_dereference *deref;
   ir_swizzle *swiz;

   /* Use the rvalue deref handler for the most part.  We'll ignore
    * swizzles in it and write swizzles using writemask, though.
    */
   ir->accept(v);
   dst_reg = ir_to_mesa_dst_reg_from_src(v->result);

   if ((deref = ir->as_dereference())) {
      ir_dereference_array *deref_array = ir->as_dereference_array();
      assert(!deref_array || deref_array->array->type->is_array());

      ir->accept(v);
   } else if ((swiz = ir->as_swizzle())) {
      dst_reg.writemask = 0;
      if (swiz->mask.num_components >= 1)
	 dst_reg.writemask |= (1 << swiz->mask.x);
      if (swiz->mask.num_components >= 2)
	 dst_reg.writemask |= (1 << swiz->mask.y);
      if (swiz->mask.num_components >= 3)
	 dst_reg.writemask |= (1 << swiz->mask.z);
      if (swiz->mask.num_components >= 4)
	 dst_reg.writemask |= (1 << swiz->mask.w);
   }

   return dst_reg;
}

void
ir_to_mesa_visitor::visit(ir_assignment *ir)
{
   struct ir_to_mesa_dst_reg l;
   struct ir_to_mesa_src_reg r;

   assert(!ir->lhs->type->is_matrix());
   assert(!ir->lhs->type->is_array());
   assert(ir->lhs->type->base_type != GLSL_TYPE_STRUCT);

   l = get_assignment_lhs(ir->lhs, this);

   ir->rhs->accept(this);
   r = this->result;
   assert(l.file != PROGRAM_UNDEFINED);
   assert(r.file != PROGRAM_UNDEFINED);

   if (ir->condition) {
	 ir_constant *condition_constant;

	 condition_constant = ir->condition->constant_expression_value();

	 assert(condition_constant && condition_constant->value.b[0]);
   }

   ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
}


void
ir_to_mesa_visitor::visit(ir_constant *ir)
{
   ir_to_mesa_src_reg src_reg;
   GLfloat stack_vals[4];
   GLfloat *values = stack_vals;
   unsigned int i;

   if (ir->type->is_matrix() || ir->type->is_array()) {
      assert(!"FINISHME: array/matrix constants");
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

   src_reg.index = _mesa_add_unnamed_constant(this->prog->Parameters,
					      values, ir->type->vector_elements,
					      &src_reg.swizzle);
   src_reg.reladdr = false;
   src_reg.negate = 0;

   this->result = src_reg;
}


void
ir_to_mesa_visitor::visit(ir_call *ir)
{
   printf("Can't support call to %s\n", ir->callee_name());
   exit(1);
}


void
ir_to_mesa_visitor::visit(ir_texture *ir)
{
   assert(0);

   ir->coordinate->accept(this);
}

void
ir_to_mesa_visitor::visit(ir_return *ir)
{
   assert(0);

   ir->get_value()->accept(this);
}


void
ir_to_mesa_visitor::visit(ir_if *ir)
{
   ir_to_mesa_instruction *if_inst, *else_inst = NULL;

   ir->condition->accept(this);
   assert(this->result.file != PROGRAM_UNDEFINED);

   if_inst = ir_to_mesa_emit_op1(ir->condition,
				 OPCODE_IF, ir_to_mesa_undef_dst,
				 this->result);

   this->instructions.push_tail(if_inst);

   visit_exec_list(&ir->then_instructions, this);

   if (!ir->else_instructions.is_empty()) {
      else_inst = ir_to_mesa_emit_op1(ir->condition, OPCODE_ELSE,
				      ir_to_mesa_undef_dst,
				      ir_to_mesa_undef);
      visit_exec_list(&ir->else_instructions, this);
   }

   if_inst = ir_to_mesa_emit_op1(ir->condition, OPCODE_ENDIF,
				 ir_to_mesa_undef_dst, ir_to_mesa_undef);
}

ir_to_mesa_visitor::ir_to_mesa_visitor()
{
   result.file = PROGRAM_UNDEFINED;
   next_temp = 1;
}

static struct prog_src_register
mesa_src_reg_from_ir_src_reg(ir_to_mesa_src_reg reg)
{
   struct prog_src_register mesa_reg;

   mesa_reg.File = reg.file;
   assert(reg.index < (1 << INST_INDEX_BITS) - 1);
   mesa_reg.Index = reg.index;
   mesa_reg.Swizzle = reg.swizzle;
   mesa_reg.RelAddr = reg.reladdr;

   return mesa_reg;
}

static void
set_branchtargets(struct prog_instruction *mesa_instructions,
		  int num_instructions)
{
   int if_count = 0, loop_count;
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

   for (i = 0; i < num_instructions; i++) {
      struct prog_instruction *mesa_inst = mesa_instructions + i;
      ir_instruction *ir = mesa_instruction_annotation[i];

      if (last_ir != ir && ir) {
	 ir_print_visitor print;
	 ir->accept(&print);
	 printf("\n");
	 last_ir = ir;
      }

      _mesa_print_instruction(mesa_inst);
   }
}

static void
count_resources(struct gl_program *prog)
{
   prog->InputsRead = 0;
   prog->OutputsWritten = 0;
   unsigned int i;

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = &prog->Instructions[i];
      unsigned int reg;

      switch (inst->DstReg.File) {
      case PROGRAM_OUTPUT:
	 prog->OutputsWritten |= BITFIELD64_BIT(inst->DstReg.Index);
	 break;
      case PROGRAM_INPUT:
	 prog->InputsRead |= BITFIELD64_BIT(inst->DstReg.Index);
	 break;
      default:
	 break;
      }

      for (reg = 0; reg < _mesa_num_inst_src_regs(inst->Opcode); reg++) {
	 switch (inst->SrcReg[reg].File) {
	 case PROGRAM_OUTPUT:
	    prog->OutputsWritten |= BITFIELD64_BIT(inst->SrcReg[reg].Index);
	    break;
	 case PROGRAM_INPUT:
	    prog->InputsRead |= BITFIELD64_BIT(inst->SrcReg[reg].Index);
	    break;
	 default:
	    break;
	 }
      }
   }
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
get_mesa_program(GLcontext *ctx, void *mem_ctx, struct glsl_shader *shader)
{
   ir_to_mesa_visitor v;
   struct prog_instruction *mesa_instructions, *mesa_inst;
   ir_instruction **mesa_instruction_annotation;
   int i;
   exec_list *instructions = &shader->ir;
   struct gl_program *prog;
   GLenum target;

   switch (shader->Type) {
   case GL_VERTEX_SHADER:   target = GL_VERTEX_PROGRAM_ARB; break;
   case GL_FRAGMENT_SHADER: target = GL_FRAGMENT_PROGRAM_ARB; break;
   default: assert(!"should not be reached"); break;
   }

   prog = ctx->Driver.NewProgram(ctx, target, 1);
   if (!prog)
      return NULL;
   prog->Parameters = _mesa_new_parameter_list();
   prog->Varying = _mesa_new_parameter_list();
   prog->Attributes = _mesa_new_parameter_list();
   v.ctx = ctx;
   v.prog = prog;

   v.mem_ctx = talloc_new(NULL);
   visit_exec_list(instructions, &v);
   v.ir_to_mesa_emit_op1(NULL, OPCODE_END,
			 ir_to_mesa_undef_dst, ir_to_mesa_undef);

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
      mesa_inst->DstReg.File = inst->dst_reg.file;
      mesa_inst->DstReg.Index = inst->dst_reg.index;
      mesa_inst->DstReg.CondMask = COND_TR;
      mesa_inst->DstReg.WriteMask = inst->dst_reg.writemask;
      mesa_inst->SrcReg[0] = mesa_src_reg_from_ir_src_reg(inst->src_reg[0]);
      mesa_inst->SrcReg[1] = mesa_src_reg_from_ir_src_reg(inst->src_reg[1]);
      mesa_inst->SrcReg[2] = mesa_src_reg_from_ir_src_reg(inst->src_reg[2]);
      mesa_instruction_annotation[i] = inst->ir;

      mesa_inst++;
      i++;
   }

   set_branchtargets(mesa_instructions, num_instructions);
   if (0) {
      print_program(mesa_instructions, mesa_instruction_annotation,
		    num_instructions);
   }

   prog->Instructions = mesa_instructions;
   prog->NumInstructions = num_instructions;

   _mesa_reference_program(ctx, &shader->mesa_shader->Program, prog);

   return prog;
}

/* Takes a Mesa gl shader structure and compiles it, returning our Mesa-like
 * structure with the IR and such attached.
 */
static struct glsl_shader *
_mesa_get_glsl_shader(GLcontext *ctx, void *mem_ctx, struct gl_shader *sh)
{
   struct glsl_shader *shader = talloc_zero(mem_ctx, struct glsl_shader);
   struct _mesa_glsl_parse_state *state;

   shader->Type = sh->Type;
   shader->Name = sh->Name;
   shader->RefCount = 1;
   shader->Source = sh->Source;
   shader->SourceLen = strlen(sh->Source);
   shader->mesa_shader = sh;

   state = talloc_zero(shader, struct _mesa_glsl_parse_state);
   switch (shader->Type) {
   case GL_VERTEX_SHADER:   state->target = vertex_shader; break;
   case GL_FRAGMENT_SHADER: state->target = fragment_shader; break;
   case GL_GEOMETRY_SHADER: state->target = geometry_shader; break;
   }

   state->scanner = NULL;
   state->translation_unit.make_empty();
   state->symbols = new(mem_ctx) glsl_symbol_table;
   state->info_log = talloc_strdup(shader, "");
   state->error = false;
   state->temp_index = 0;
   state->loop_or_switch_nesting = NULL;
   state->ARB_texture_rectangle_enable = true;

   _mesa_glsl_lexer_ctor(state, shader->Source);
   _mesa_glsl_parse(state);
   _mesa_glsl_lexer_dtor(state);

   shader->ir.make_empty();
   if (!state->error && !state->translation_unit.is_empty())
      _mesa_ast_to_hir(&shader->ir, state);

   /* Optimization passes */
   if (!state->error && !shader->ir.is_empty()) {
      bool progress;
      do {
	 progress = false;

	 progress = do_function_inlining(&shader->ir) || progress;
	 progress = do_if_simplification(&shader->ir) || progress;
	 progress = do_copy_propagation(&shader->ir) || progress;
	 progress = do_dead_code_local(&shader->ir) || progress;
	 progress = do_dead_code_unlinked(state, &shader->ir) || progress;
	 progress = do_constant_variable_unlinked(&shader->ir) || progress;
	 progress = do_constant_folding(&shader->ir) || progress;
	 progress = do_vec_index_to_swizzle(&shader->ir) || progress;
	 progress = do_swizzle_swizzle(&shader->ir) || progress;
      } while (progress);
   }

   shader->symbols = state->symbols;

   shader->CompileStatus = !state->error;
   shader->InfoLog = state->info_log;

   talloc_free(state);

   return shader;
}

extern "C" {

void
_mesa_glsl_compile_shader(GLcontext *ctx, struct gl_shader *sh)
{
   struct glsl_shader *shader;
   TALLOC_CTX *mem_ctx = talloc_new(NULL);

   shader = _mesa_get_glsl_shader(ctx, mem_ctx, sh);

   sh->CompileStatus = shader->CompileStatus;
   sh->InfoLog = strdup(shader->InfoLog);
   talloc_free(mem_ctx);
 }

void
_mesa_glsl_link_shader(GLcontext *ctx, struct gl_shader_program *prog)
{
   struct glsl_program *whole_program;
   unsigned int i;

   _mesa_clear_shader_program_data(ctx, prog);

   whole_program = talloc_zero(NULL, struct glsl_program);
   whole_program->LinkStatus = GL_TRUE;
   whole_program->NumShaders = prog->NumShaders;
   whole_program->Shaders = talloc_array(whole_program, struct glsl_shader *,
					 prog->NumShaders);

   for (i = 0; i < prog->NumShaders; i++) {
      whole_program->Shaders[i] = _mesa_get_glsl_shader(ctx, whole_program,
							prog->Shaders[i]);
      if (!whole_program->Shaders[i]->CompileStatus) {
	 whole_program->InfoLog =
	    talloc_asprintf_append(whole_program->InfoLog,
				   "linking with uncompiled shader");
	 whole_program->LinkStatus = GL_FALSE;
      }
   }

   prog->Uniforms = _mesa_new_uniform_list();
   prog->Varying = _mesa_new_parameter_list();
   _mesa_reference_vertprog(ctx, &prog->VertexProgram, NULL);
   _mesa_reference_fragprog(ctx, &prog->FragmentProgram, NULL);

   if (whole_program->LinkStatus)
      link_shaders(whole_program);

   prog->LinkStatus = whole_program->LinkStatus;

   /* FINISHME: This should use the linker-generated code */
   if (prog->LinkStatus) {
      for (i = 0; i < prog->NumShaders; i++) {
	 struct gl_program *linked_prog;

	 linked_prog = get_mesa_program(ctx, whole_program,
					whole_program->Shaders[i]);
	 count_resources(linked_prog);

	 link_uniforms_to_shared_uniform_list(prog->Uniforms, linked_prog);

	 switch (whole_program->Shaders[i]->Type) {
	 case GL_VERTEX_SHADER:
	    _mesa_reference_vertprog(ctx, &prog->VertexProgram,
				     (struct gl_vertex_program *)linked_prog);
	    break;
	 case GL_FRAGMENT_SHADER:
	    _mesa_reference_fragprog(ctx, &prog->FragmentProgram,
				     (struct gl_fragment_program *)linked_prog);
	    break;
	 }
      }
   }

   talloc_free(whole_program);
}

} /* extern "C" */
