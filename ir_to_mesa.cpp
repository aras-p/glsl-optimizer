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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_to_mesa.cpp
 *
 * Translates the IR to ARB_fragment_program text if possible,
 * printing the result
 *
 * The code generation is performed using monoburg.  Because monoburg
 * produces a single C file with the definitions of the node types in
 * it, this file is included from the monoburg output.
 */

/* Quiet compiler warnings due to monoburg not marking functions defined
 * in the header as inline.
 */
#define g_new
#define g_error
#include "mesa_codegen.h"

#include "ir.h"
#include "ir_visitor.h"
#include "ir_print_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

extern "C" {
#include "shader/prog_instruction.h"
#include "shader/prog_print.h"
}

ir_to_mesa_src_reg ir_to_mesa_undef = {
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP
};

ir_to_mesa_instruction *
ir_to_mesa_emit_op3(struct mbtree *tree, enum prog_opcode op,
		    ir_to_mesa_dst_reg dst,
		    ir_to_mesa_src_reg src0,
		    ir_to_mesa_src_reg src1,
		    ir_to_mesa_src_reg src2)
{
   ir_to_mesa_instruction *inst = new ir_to_mesa_instruction();

   inst->op = op;
   inst->dst_reg = dst;
   inst->src_reg[0] = src0;
   inst->src_reg[1] = src1;
   inst->src_reg[2] = src2;
   inst->ir = tree->ir;

   tree->v->instructions.push_tail(inst);

   return inst;
}


ir_to_mesa_instruction *
ir_to_mesa_emit_op2(struct mbtree *tree, enum prog_opcode op,
		    ir_to_mesa_dst_reg dst,
		    ir_to_mesa_src_reg src0,
		    ir_to_mesa_src_reg src1)
{
   return ir_to_mesa_emit_op3(tree, op, dst, src0, src1, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_emit_op1(struct mbtree *tree, enum prog_opcode op,
		    ir_to_mesa_dst_reg dst,
		    ir_to_mesa_src_reg src0)
{
   return ir_to_mesa_emit_op3(tree, op,
			      dst, src0, ir_to_mesa_undef, ir_to_mesa_undef);
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
ir_to_mesa_emit_scalar_op1(struct mbtree *tree, enum prog_opcode op,
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
      int this_mask = (1 << i);
      ir_to_mesa_instruction *inst;
      ir_to_mesa_src_reg src = src0;

      if (done_mask & this_mask)
	 continue;

      int src_swiz = GET_SWZ(src.swizzle, i);
      for (j = i + 1; j < 4; j++) {
	 if (!(done_mask & (1 << j)) && GET_SWZ(src.swizzle, j) == src_swiz) {
	    this_mask |= (1 << j);
	 }
      }
      src.swizzle = MAKE_SWIZZLE4(src_swiz, src_swiz,
				  src_swiz, src_swiz);

      inst = ir_to_mesa_emit_op1(tree, op,
				 dst,
				 src);
      inst->dst_reg.writemask = this_mask;
      done_mask |= this_mask;
   }
}

static void
ir_to_mesa_set_tree_reg(struct mbtree *tree, int file, int index)
{
   tree->dst_reg.file = file;
   tree->dst_reg.index = index;

   tree->src_reg.file = file;
   tree->src_reg.index = index;
}

struct mbtree *
ir_to_mesa_visitor::create_tree(int op,
				ir_instruction *ir,
				struct mbtree *left, struct mbtree *right)
{
   struct mbtree *tree = (struct mbtree *)calloc(sizeof(struct mbtree), 1);

   assert(ir);

   tree->op = op;
   tree->left = left;
   tree->right = right;
   tree->v = this;
   tree->src_reg.swizzle = SWIZZLE_XYZW;
   tree->dst_reg.writemask = WRITEMASK_XYZW;
   ir_to_mesa_set_tree_reg(tree, PROGRAM_UNDEFINED, 0);
   tree->ir = ir;

   return tree;
}

/**
 * In the initial pass of codegen, we assign temporary numbers to
 * intermediate results.  (not SSA -- variable assignments will reuse
 * storage).  Actual register allocation for the Mesa VM occurs in a
 * pass over the Mesa IR later.
 */
void
ir_to_mesa_visitor::get_temp(struct mbtree *tree, int size)
{
   int swizzle[4];
   int i;

   ir_to_mesa_set_tree_reg(tree, PROGRAM_TEMPORARY, this->next_temp++);

   for (i = 0; i < size; i++)
      swizzle[i] = i;
   for (; i < 4; i++)
      swizzle[i] = size - 1;
   tree->src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
					 swizzle[2], swizzle[3]);
   tree->dst_reg.writemask = (1 << size) - 1;
}

void
ir_to_mesa_visitor::get_temp_for_var(ir_variable *var, struct mbtree *tree)
{
   temp_entry *entry;

   foreach_iter(exec_list_iterator, iter, this->variable_storage) {
      entry = (temp_entry *)iter.get();

      if (entry->var == var) {
	 ir_to_mesa_set_tree_reg(tree, entry->file, entry->index);
	 return;
      }
   }

   entry = new temp_entry(var, PROGRAM_TEMPORARY, this->next_temp++);
   this->variable_storage.push_tail(entry);

   ir_to_mesa_set_tree_reg(tree, entry->file, entry->index);
}

static void
reduce(struct mbtree *t, int goal)
{
   struct mbtree *kids[10];
   int rule = mono_burg_rule((MBState *)t->state, goal);
   const uint16_t *nts = mono_burg_nts[rule];
   int i;

   mono_burg_kids (t, rule, kids);

   for (i = 0; nts[i]; i++) {
      reduce(kids[i], nts[i]);
   }

   if (t->left) {
      if (mono_burg_func[rule]) {
	 mono_burg_func[rule](t, NULL);
      } else {
	 printf("no code for rules %s\n", mono_burg_rule_string[rule]);
	 exit(1);
      }
   } else {
      if (mono_burg_func[rule]) {
	 printf("unused code for rule %s\n", mono_burg_rule_string[rule]);
	 exit(1);
      }
   }
}

void
ir_to_mesa_visitor::visit(ir_variable *ir)
{
   (void)ir;
}

void
ir_to_mesa_visitor::visit(ir_loop *ir)
{
   (void)ir;

   printf("Can't support loops, should be flattened before here\n");
   exit(1);
}

void
ir_to_mesa_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
   printf("Can't support loops, should be flattened before here\n");
   exit(1);
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
   struct mbtree *op[2];
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result = NULL;
      ir->operands[operand]->accept(this);
      if (!this->result) {
	 ir_print_visitor v;
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
	 exit(1);
      }
      op[operand] = this->result;
   }

   this->result = NULL;

   switch (ir->operation) {
   case ir_binop_add:
      this->result = this->create_tree(MB_TERM_add_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_sub:
      this->result = this->create_tree(MB_TERM_sub_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_mul:
      this->result = this->create_tree(MB_TERM_mul_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_div:
      this->result = this->create_tree(MB_TERM_div_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_dot:
      if (ir->operands[0]->type == vec4_type) {
	 assert(ir->operands[1]->type == vec4_type);
	 this->result = this->create_tree(MB_TERM_dp4_vec4_vec4,
					  ir, op[0], op[1]);
      } else if (ir->operands[0]->type == vec3_type) {
	 assert(ir->operands[1]->type == vec3_type);
	 this->result = this->create_tree(MB_TERM_dp3_vec4_vec4,
					  ir, op[0], op[1]);
      } else if (ir->operands[0]->type == vec2_type) {
	 assert(ir->operands[1]->type == vec2_type);
	 this->result = this->create_tree(MB_TERM_dp2_vec4_vec4,
					  ir, op[0], op[1]);
      }
      break;
   case ir_unop_sqrt:
      this->result = this->create_tree(MB_TERM_sqrt_vec4, ir, op[0], op[1]);
      break;
   case ir_unop_i2f:
      /* Mesa IR lacks types, ints are stored as floats. */
      this->result = op[0];
      break;
   default:
      break;
   }
   if (!this->result) {
      ir_print_visitor v;
      printf("Failed to get tree for expression:\n");
      ir->accept(&v);
      exit(1);
   }

   /* Allocate a temporary for the result. */
   this->get_temp(this->result, ir->type->vector_elements);
}


void
ir_to_mesa_visitor::visit(ir_swizzle *ir)
{
   struct mbtree *tree;
   int i;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   assert(this->result);

   tree = this->create_tree(MB_TERM_swizzle_vec4, ir, this->result, NULL);
   this->get_temp(tree, 4);

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

   tree->src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0],
					 swizzle[1],
					 swizzle[2],
					 swizzle[3]);

   this->result = tree;
}


void
ir_to_mesa_visitor::visit(ir_dereference_variable *ir)
{
   struct mbtree *tree;
   int size_swizzles[4] = {
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
   };

   ir_variable *var = ir->var->as_variable();

   /* By the time we make it to this stage, matric`es should be broken down
    * to vectors.
    */
   assert(!var->type->is_matrix());

   tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);

   if (strncmp(var->name, "gl_", 3) == 0) {
      if (strcmp(var->name, "gl_FragColor") == 0) {
	 ir_to_mesa_set_tree_reg(tree, PROGRAM_INPUT, FRAG_ATTRIB_COL0);
      } else {
	 assert(0);
      }
   } else {
      this->get_temp_for_var(var, tree);
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   tree->src_reg.swizzle = size_swizzles[ir->type->vector_elements - 1];

   this->result = tree;
}

void
ir_to_mesa_visitor::visit(ir_dereference_array *ir)
{
   struct mbtree *tree;
   int size_swizzles[4] = {
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
   };
   ir_variable *var = ir->var->as_variable();
   ir_constant *index = ir->selector.array_index->constant_expression_value();
   int file = PROGRAM_UNDEFINED;
   int base_index = 0;

   assert(var);
   assert(index);
   if (strcmp(var->name, "gl_TexCoord") == 0) {
      file = PROGRAM_INPUT;
      base_index = FRAG_ATTRIB_TEX0;
   } else if (strcmp(var->name, "gl_FragData") == 0) {
      file = PROGRAM_OUTPUT;
      base_index = FRAG_RESULT_DATA0;
   }

   tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);
   ir_to_mesa_set_tree_reg(tree, file, base_index + index->value.i[0]);

   /* If the type is smaller than a vec4, replicate the last channel out. */
   tree->src_reg.swizzle = size_swizzles[ir->type->vector_elements - 1];

   this->result = tree;
}

static struct mbtree *
get_assignment_lhs(ir_instruction *ir, ir_to_mesa_visitor *v)
{
   struct mbtree *tree = NULL;
   ir_dereference *deref;
   ir_swizzle *swiz;

   if ((deref = ir->as_dereference())) {
      ir->accept(v);
      tree = v->result;
   } else if ((swiz = ir->as_swizzle())) {
      tree = get_assignment_lhs(swiz->val, v);
      tree->dst_reg.writemask = 0;
      if (swiz->mask.num_components >= 1)
	 tree->dst_reg.writemask |= (1 << swiz->mask.x);
      if (swiz->mask.num_components >= 2)
	 tree->dst_reg.writemask |= (1 << swiz->mask.y);
      if (swiz->mask.num_components >= 3)
	 tree->dst_reg.writemask |= (1 << swiz->mask.z);
      if (swiz->mask.num_components >= 4)
	 tree->dst_reg.writemask |= (1 << swiz->mask.w);
   }

   assert(tree);

   return tree;
}

void
ir_to_mesa_visitor::visit(ir_dereference_record *ir)
{
   (void)ir;
   assert(0);
}

void
ir_to_mesa_visitor::visit(ir_assignment *ir)
{
   struct mbtree *l, *r, *t;

   l = get_assignment_lhs(ir->lhs, this);

   ir->rhs->accept(this);
   r = this->result;
   assert(l);
   assert(r);

   assert(!ir->condition);

   t = this->create_tree(MB_TERM_assign, ir, l, r);
   mono_burg_label(t, NULL);
   reduce(t, MB_NTERM_stmt);
}


void
ir_to_mesa_visitor::visit(ir_constant *ir)
{
   struct mbtree *tree;

   assert(!ir->type->is_matrix());

   tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);

   assert(ir->type->base_type == GLSL_TYPE_FLOAT ||
	  ir->type->base_type == GLSL_TYPE_UINT ||
	  ir->type->base_type == GLSL_TYPE_INT ||
	  ir->type->base_type == GLSL_TYPE_BOOL);

   /* FINISHME: This will end up being _mesa_add_unnamed_constant,
    * which handles sharing values and sharing channels of vec4
    * constants for small values.
    */
   /* FINISHME: Do something with the constant values for now.
    */
   ir_to_mesa_set_tree_reg(tree, PROGRAM_CONSTANT, this->next_constant++);
   tree->src_reg.swizzle = SWIZZLE_NOOP;

   this->result = tree;
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
   (void)ir;
   printf("Can't support conditionals, should be flattened before here.\n");
   exit(1);
}

ir_to_mesa_visitor::ir_to_mesa_visitor()
{
   result = NULL;
   next_temp = 1;
   next_constant = 0;
}

static struct prog_src_register
mesa_src_reg_from_ir_src_reg(ir_to_mesa_src_reg reg)
{
   struct prog_src_register mesa_reg;

   mesa_reg.File = reg.file;
   assert(reg.index < (1 << INST_INDEX_BITS) - 1);
   mesa_reg.Index = reg.index;
   mesa_reg.Swizzle = reg.swizzle;

   return mesa_reg;
}

void
do_ir_to_mesa(exec_list *instructions)
{
   ir_to_mesa_visitor v;
   struct prog_instruction *mesa_instructions, *mesa_inst;
   ir_instruction *last_ir = NULL;

   visit_exec_list(instructions, &v);

   int num_instructions = 0;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      num_instructions++;
   }

   mesa_instructions =
      (struct prog_instruction *)calloc(num_instructions,
					sizeof(*mesa_instructions));

   mesa_inst = mesa_instructions;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      ir_to_mesa_instruction *inst = (ir_to_mesa_instruction *)iter.get();

      if (last_ir != inst->ir) {
	 ir_print_visitor print;
	 inst->ir->accept(&print);
	 printf("\n");
	 last_ir = inst->ir;
      }

      mesa_inst->Opcode = inst->op;
      mesa_inst->DstReg.File = inst->dst_reg.file;
      mesa_inst->DstReg.Index = inst->dst_reg.index;
      mesa_inst->DstReg.CondMask = COND_TR;
      mesa_inst->DstReg.WriteMask = inst->dst_reg.writemask;
      mesa_inst->SrcReg[0] = mesa_src_reg_from_ir_src_reg(inst->src_reg[0]);
      mesa_inst->SrcReg[1] = mesa_src_reg_from_ir_src_reg(inst->src_reg[1]);
      mesa_inst->SrcReg[2] = mesa_src_reg_from_ir_src_reg(inst->src_reg[2]);

      _mesa_print_instruction(mesa_inst);

      mesa_inst++;
   }
}
