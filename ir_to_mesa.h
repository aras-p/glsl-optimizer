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

#include "ir.h"
extern "C" {
#include "shader/prog_instruction.h"
};

/**
 * \file ir_to_mesa.h
 *
 * Translates the IR to Mesa IR if possible.
 */

/**
 * This struct is a corresponding struct to Mesa prog_src_register, with
 * wider fields.
 */
typedef struct ir_to_mesa_src_reg {
   int file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   int swizzle; /**< SWIZZLE_XYZWONEZERO swizzles from Mesa. */
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

struct mbtree {
   struct mbtree *left;
   struct mbtree *right;
   void *state;
   uint16_t op;
   class ir_to_mesa_visitor *v;

   /** Pointer to the ir source this tree came from for debugging */
   ir_instruction *ir;

   ir_to_mesa_dst_reg dst_reg;

   /**
    * This is the representation of this tree node's results as a
    * source register for its consumer.
    */
   ir_to_mesa_src_reg src_reg;
};

void do_ir_to_mesa(exec_list *instructions);

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

   int next_temp;
   int next_constant;

   void get_temp(struct mbtree *tree, int size);

   void get_temp_for_var(ir_variable *var, struct mbtree *tree);

   struct mbtree *create_tree(int op,
			      ir_instruction *ir,
			      struct mbtree *left,
			      struct mbtree *right);
   struct mbtree *create_tree_for_float(ir_instruction *ir, float val);

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
   virtual void visit(ir_dereference_variable *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_if *);
   /*@}*/

   struct mbtree *result;

   /** List of temp_entry */
   exec_list variable_storage;

   /** List of ir_to_mesa_instruction */
   exec_list instructions;
};

extern ir_to_mesa_src_reg ir_to_mesa_undef;
extern ir_to_mesa_dst_reg ir_to_mesa_undef_dst;

ir_to_mesa_instruction *
ir_to_mesa_emit_op1(struct mbtree *tree, enum prog_opcode op);

ir_to_mesa_instruction *
ir_to_mesa_emit_op1_full(ir_to_mesa_visitor *v, ir_instruction *ir,
			 enum prog_opcode op,
			 ir_to_mesa_dst_reg dst,
			 ir_to_mesa_src_reg src0);

ir_to_mesa_instruction *
ir_to_mesa_emit_op2(struct mbtree *tree, enum prog_opcode op);

ir_to_mesa_instruction *
ir_to_mesa_emit_op2_full(ir_to_mesa_visitor *v, ir_instruction *ir,
			 enum prog_opcode op,
			 ir_to_mesa_dst_reg dst,
			 ir_to_mesa_src_reg src0,
			 ir_to_mesa_src_reg src1);

ir_to_mesa_instruction *
ir_to_mesa_emit_simple_op2(struct mbtree *tree, enum prog_opcode op);

ir_to_mesa_instruction *
ir_to_mesa_emit_op3(ir_to_mesa_visitor *v, ir_instruction *ir,
		    enum prog_opcode op,
		    ir_to_mesa_dst_reg dst,
		    ir_to_mesa_src_reg src0,
		    ir_to_mesa_src_reg src1,
		    ir_to_mesa_src_reg src2);

void
ir_to_mesa_emit_scalar_op1(struct mbtree *tree, enum prog_opcode op,
			   ir_to_mesa_dst_reg dst,
			   ir_to_mesa_src_reg src0);

inline ir_to_mesa_dst_reg
ir_to_mesa_dst_reg_from_src(ir_to_mesa_src_reg reg)
{
   ir_to_mesa_dst_reg dst_reg;

   dst_reg.file = reg.file;
   dst_reg.index = reg.index;
   dst_reg.writemask = WRITEMASK_XYZW;

   return dst_reg;
}
