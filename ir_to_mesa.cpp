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
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP, false,
};

ir_to_mesa_dst_reg ir_to_mesa_undef_dst = {
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP
};

ir_to_mesa_instruction *
ir_to_mesa_emit_op3(ir_to_mesa_visitor *v, ir_instruction *ir,
		    enum prog_opcode op,
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
   inst->ir = ir;

   v->instructions.push_tail(inst);

   return inst;
}


ir_to_mesa_instruction *
ir_to_mesa_emit_op2_full(ir_to_mesa_visitor *v, ir_instruction *ir,
			 enum prog_opcode op,
			 ir_to_mesa_dst_reg dst,
			 ir_to_mesa_src_reg src0,
			 ir_to_mesa_src_reg src1)
{
   return ir_to_mesa_emit_op3(v, ir,
			      op, dst, src0, src1, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_emit_op2(struct mbtree *tree, enum prog_opcode op)
{
   return ir_to_mesa_emit_op2_full(tree->v, tree->ir, op,
				   tree->dst_reg,
				   tree->left->src_reg,
				   tree->right->src_reg);
}

ir_to_mesa_instruction *
ir_to_mesa_emit_op1_full(ir_to_mesa_visitor *v, ir_instruction *ir,
			 enum prog_opcode op,
			 ir_to_mesa_dst_reg dst,
			 ir_to_mesa_src_reg src0)
{
   return ir_to_mesa_emit_op3(v, ir, op,
			      dst, src0, ir_to_mesa_undef, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_emit_op1(struct mbtree *tree, enum prog_opcode op)
{
   return ir_to_mesa_emit_op1_full(tree->v, tree->ir, op,
				   tree->dst_reg,
				   tree->left->src_reg);
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

      inst = ir_to_mesa_emit_op1_full(tree->v, tree->ir, op,
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

struct mbtree *
ir_to_mesa_visitor::create_tree_for_float(ir_instruction *ir, float val)
{
   struct mbtree *tree = (struct mbtree *)calloc(sizeof(struct mbtree), 1);

   tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);

   /* FINISHME: This will end up being _mesa_add_unnamed_constant,
    * which handles sharing values and sharing channels of vec4
    * constants for small values.
    */
   /* FINISHME: Do something with the constant values for now.
    */
   (void)val;
   ir_to_mesa_set_tree_reg(tree, PROGRAM_CONSTANT, this->next_constant++);
   tree->src_reg.swizzle = SWIZZLE_NOOP;

   this->result = tree;
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
      assert(!type->is_matrix());
      /* Regardless of size of vector, it gets a vec4. This is bad
       * packing for things like floats, but otherwise arrays become a
       * mess.  Hopefully a later pass over the code can pack scalars
       * down if appropriate.
       */
      return 1;
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

   entry = new temp_entry(var, PROGRAM_TEMPORARY, this->next_temp);
   this->variable_storage.push_tail(entry);

   next_temp += type_size(var->type);

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
   assert(!ir->from);
   assert(!ir->to);
   assert(!ir->increment);
   assert(!ir->counter);

   ir_to_mesa_emit_op1_full(this, NULL,
			    OPCODE_BGNLOOP, ir_to_mesa_undef_dst,
			    ir_to_mesa_undef);

   visit_exec_list(&ir->body_instructions, this);

   ir_to_mesa_emit_op1_full(this, NULL,
			    OPCODE_ENDLOOP, ir_to_mesa_undef_dst,
			    ir_to_mesa_undef);
}

void
ir_to_mesa_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      ir_to_mesa_emit_op1_full(this, NULL,
			       OPCODE_BRK, ir_to_mesa_undef_dst,
			       ir_to_mesa_undef);
      break;
   case ir_loop_jump::jump_continue:
      ir_to_mesa_emit_op1_full(this, NULL,
			       OPCODE_CONT, ir_to_mesa_undef_dst,
			       ir_to_mesa_undef);
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
   case ir_unop_logic_not:
      this->result = this->create_tree_for_float(ir, 0.0);
      this->result = this->create_tree(MB_TERM_seq_vec4_vec4, ir,
				       op[0], this->result);
      break;
   case ir_unop_exp:
      this->result = this->create_tree(MB_TERM_exp_vec4, ir, op[0], NULL);
      break;
   case ir_unop_exp2:
      this->result = this->create_tree(MB_TERM_exp2_vec4, ir, op[0], NULL);
      break;
   case ir_unop_log:
      this->result = this->create_tree(MB_TERM_log_vec4, ir, op[0], NULL);
      break;
   case ir_unop_log2:
      this->result = this->create_tree(MB_TERM_log2_vec4, ir, op[0], NULL);
      break;
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

   case ir_binop_less:
      this->result = this->create_tree(MB_TERM_slt_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_greater:
      this->result = this->create_tree(MB_TERM_sgt_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_lequal:
      this->result = this->create_tree(MB_TERM_sle_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_gequal:
      this->result = this->create_tree(MB_TERM_sge_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_equal:
      this->result = this->create_tree(MB_TERM_seq_vec4_vec4, ir, op[0], op[1]);
      break;
   case ir_binop_logic_xor:
   case ir_binop_nequal:
      this->result = this->create_tree(MB_TERM_sne_vec4_vec4, ir, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      /* This could be a saturated add. */
      this->result = this->create_tree(MB_TERM_add_vec4_vec4, ir, op[0], op[1]);
      this->result = this->create_tree(MB_TERM_sne_vec4_vec4, ir,
				       this->create_tree_for_float(ir, 0.0),
				       this->result);
      break;

   case ir_binop_logic_and:
      /* the bool args are stored as float 0.0 or 1.0, so "mul" gives us "and". */
      this->result = this->create_tree(MB_TERM_mul_vec4_vec4, ir, op[0], op[1]);
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
   case ir_unop_rsq:
      this->result = this->create_tree(MB_TERM_rsq_vec4, ir, op[0], op[1]);
      break;
   case ir_unop_i2f:
      /* Mesa IR lacks types, ints are stored as truncated floats. */
      this->result = op[0];
      break;
   case ir_unop_f2i:
      this->result = this->create_tree(MB_TERM_trunc_vec4, ir, op[0], NULL);
      break;
   case ir_unop_f2b:
      this->result = this->create_tree_for_float(ir, 0.0);
      this->result = this->create_tree(MB_TERM_sne_vec4_vec4, ir,
				       op[0], this->result);
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
   {"gl_FragColor", PROGRAM_INPUT, FRAG_ATTRIB_COL0},
   {"gl_FragDepth", PROGRAM_UNDEFINED, FRAG_ATTRIB_WPOS}, /* FINISHME: WPOS.z */

   /* 110_deprecated_fs */
   {"gl_Color", PROGRAM_INPUT, FRAG_ATTRIB_COL0},
   {"gl_SecondaryColor", PROGRAM_INPUT, FRAG_ATTRIB_COL1},
   {"gl_FogFragCoord", PROGRAM_INPUT, FRAG_ATTRIB_FOGC},

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
   ir_variable *var = ir->var;

   /* By the time we make it to this stage, matrices should be broken down
    * to vectors.
    */
   assert(!var->type->is_matrix());

   tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);

   if (strncmp(var->name, "gl_", 3) == 0) {
      unsigned int i;

      for (i = 0; i < ARRAY_SIZE(builtin_var_to_mesa_reg); i++) {
	 if (strcmp(var->name, builtin_var_to_mesa_reg[i].name) == 0)
	    break;
      }
      assert(i != ARRAY_SIZE(builtin_var_to_mesa_reg));
      ir_to_mesa_set_tree_reg(tree, builtin_var_to_mesa_reg[i].file,
			      builtin_var_to_mesa_reg[i].index);
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
   ir_variable *var = ir->variable_referenced();
   ir_constant *index = ir->array_index->constant_expression_value();

   /* By the time we make it to this stage, matrices should be broken down
    * to vectors.
    */
   assert(!var->type->is_matrix());

   if (strncmp(var->name, "gl_", 3) == 0) {
      unsigned int i;
      unsigned int offset = 0;

      tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);

      offset = index->value.i[0];

      for (i = 0; i < ARRAY_SIZE(builtin_var_to_mesa_reg); i++) {
	 if (strcmp(var->name, builtin_var_to_mesa_reg[i].name) == 0)
	    break;
      }
      assert(i != ARRAY_SIZE(builtin_var_to_mesa_reg));
      ir_to_mesa_set_tree_reg(tree, builtin_var_to_mesa_reg[i].file,
			      builtin_var_to_mesa_reg[i].index + offset);
   } else {
      tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);
      this->get_temp_for_var(var, tree);

      if (index) {
	 tree->src_reg.index += index->value.i[0];
	 tree->dst_reg.index += index->value.i[0];
      } else {
	 /* Variable index array dereference.  It eats the "vec4" of the
	  * base of the array and an index that offsets the Mesa register
	  * index.
	  */
	 ir->array_index->accept(this);

	 tree->src_reg.reladdr = true;
	 tree = this->create_tree(MB_TERM_array_reference_vec4_vec4,
				  ir, tree, this->result);
	 this->get_temp(tree, ir->type->vector_elements);
      }
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   tree->src_reg.swizzle = size_swizzles[ir->type->vector_elements - 1];

   this->result = tree;
}

void
ir_to_mesa_visitor::visit(ir_dereference_record *ir)
{
   ir_variable *var = ir->variable_referenced();
   const char *field = ir->field;
   struct mbtree *tree;
   unsigned int i;

   const glsl_type *struct_type = var->type;
   int offset = 0;

   tree = this->create_tree(MB_TERM_reference_vec4, ir, NULL, NULL);
   this->get_temp_for_var(var, tree);

   for (i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }
   tree->src_reg.index += offset;
   tree->dst_reg.index += offset;
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
static struct mbtree *
get_assignment_lhs(ir_instruction *ir, ir_to_mesa_visitor *v)
{
   struct mbtree *tree = NULL;
   ir_dereference *deref;
   ir_swizzle *swiz;

   ir->accept(v);
   tree = v->result;

   if ((deref = ir->as_dereference())) {
      ir_dereference_array *deref_array = ir->as_dereference_array();
      assert(!deref_array || deref_array->array->type->is_array());

      ir->accept(v);
      tree = v->result;
   } else if ((swiz = ir->as_swizzle())) {
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
ir_to_mesa_visitor::visit(ir_assignment *ir)
{
   struct mbtree *l, *r, *t;

   assert(!ir->lhs->type->is_matrix());
   assert(!ir->lhs->type->is_array());
   assert(ir->lhs->type->base_type != GLSL_TYPE_STRUCT);

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
   ir_to_mesa_instruction *if_inst, *else_inst = NULL;

   ir->condition->accept(this);
   assert(this->result);

   if_inst = ir_to_mesa_emit_op1_full(this, ir->condition,
				      OPCODE_IF, ir_to_mesa_undef_dst,
				      this->result->src_reg);

   this->instructions.push_tail(if_inst);

   visit_exec_list(&ir->then_instructions, this);

   if (!ir->else_instructions.is_empty()) {
      else_inst = ir_to_mesa_emit_op1_full(this, ir->condition,
					   OPCODE_ELSE, ir_to_mesa_undef_dst,
					   ir_to_mesa_undef);
      visit_exec_list(&ir->then_instructions, this);
   }

   if_inst = ir_to_mesa_emit_op1_full(this, ir->condition,
				      OPCODE_ENDIF, ir_to_mesa_undef_dst,
				      ir_to_mesa_undef);
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

void
do_ir_to_mesa(exec_list *instructions)
{
   ir_to_mesa_visitor v;
   struct prog_instruction *mesa_instructions, *mesa_inst;
   ir_instruction **mesa_instruction_annotation;
   int i;

   visit_exec_list(instructions, &v);

   int num_instructions = 0;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      num_instructions++;
   }

   mesa_instructions =
      (struct prog_instruction *)calloc(num_instructions,
					sizeof(*mesa_instructions));
   mesa_instruction_annotation =
      (ir_instruction **)calloc(num_instructions,
				sizeof(*mesa_instruction_annotation));

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
   print_program(mesa_instructions, mesa_instruction_annotation, num_instructions);

   free(mesa_instruction_annotation);
}
