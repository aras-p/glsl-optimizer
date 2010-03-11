/* -*- c++ -*- */
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

#include "list.h"
#include "ir_visitor.h"

struct ir_program {
   void *bong_hits;
};


enum ir_opcodes {
   ir_op_var_decl,
   ir_op_assign,
   ir_op_expression,
   ir_op_dereference,
   ir_op_jump,
   ir_op_label,
   ir_op_constant,
   ir_op_func_sig,
   ir_op_func,
   ir_op_call,
};

/**
 * Base class of all IR instructions
 */
class ir_instruction : public exec_node {
public:
   unsigned mode;
   const struct glsl_type *type;

   virtual void accept(ir_visitor *) = 0;

protected:
   ir_instruction(int mode)
      : mode(mode)
   {
      /* empty */
   }

private:
   /**
    * Dummy constructor to catch bad constructors in derived classes.
    *
    * Every derived must use the constructor that sets the instructions
    * mode.  Having the \c void constructor private prevents derived classes
    * from accidentally doing the wrong thing.
    */
   ir_instruction(void);
};


enum ir_variable_mode {
   ir_var_auto = 0,
   ir_var_uniform,
   ir_var_in,
   ir_var_out,
   ir_var_inout
};

enum ir_varaible_interpolation {
   ir_var_smooth = 0,
   ir_var_flat,
   ir_var_noperspective
};

class ir_variable : public ir_instruction {
public:
   ir_variable(const struct glsl_type *, const char *);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   const char *name;

   unsigned read_only:1;
   unsigned centroid:1;
   unsigned invariant:1;

   unsigned mode:3;
   unsigned interpolation:2;
};


class ir_label : public ir_instruction {
public:
   ir_label(const char *label);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   const char *label;
};


/*@{*/
class ir_function_signature : public ir_instruction {
public:
   ir_function_signature(void);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Function return type.
    *
    * \note This discards the optional precision qualifier.
    */
   const struct glsl_type *return_type;

   /**
    * List of function parameters stored as ir_variable objects.
    */
   struct exec_list parameters;

   /**
    * Pointer to the label that begins the function definition.
    */
   ir_label *definition;
};


/**
 * Header for tracking functions in the symbol table
 */
class ir_function : public ir_instruction {
public:
   ir_function(void);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Name of the function.
    */
   const char *name;

   struct exec_list signatures;
};
/*@}*/

class ir_expression;
class ir_dereference;

class ir_assignment : public ir_instruction {
public:
   ir_assignment(ir_instruction *lhs, ir_instruction *rhs,
		 ir_expression *condition);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Left-hand side of the assignment.
    */
   ir_dereference *lhs;

   /**
    * Value being assigned
    *
    * This should be either \c ir_op_expression or \c ir_op_deference.
    */
   ir_instruction *rhs;

   /**
    * Optional condition for the assignment.
    */
   ir_expression *condition;
};


enum ir_expression_operation {
   ir_unop_bit_not,
   ir_unop_logic_not,
   ir_unop_neg,
   ir_unop_abs,
   ir_unop_rcp,
   ir_unop_rsq,
   ir_unop_exp,
   ir_unop_log,
   ir_unop_f2i,      /**< Float-to-integer conversion. */
   ir_unop_i2f,      /**< Integer-to-float conversion. */

   /**
    * \name Unary floating-point rounding operations.
    */
   /*@{*/
   ir_unop_trunc,
   ir_unop_ceil,
   ir_unop_floor,
   /*@}*/

   ir_binop_add,
   ir_binop_sub,
   ir_binop_mul,
   ir_binop_div,
   ir_binop_mod,

   /**
    * \name Binary comparison operators
    */
   /*@{*/
   ir_binop_less,
   ir_binop_greater,
   ir_binop_lequal,
   ir_binop_gequal,
   ir_binop_equal,
   ir_binop_nequal,
   /*@}*/

   /**
    * \name Bit-wise binary operations.
    */
   /*@{*/
   ir_binop_lshift,
   ir_binop_rshift,
   ir_binop_bit_and,
   ir_binop_bit_xor,
   ir_binop_bit_or,
   /*@}*/

   ir_binop_logic_and,
   ir_binop_logic_xor,
   ir_binop_logic_or,
   ir_binop_logic_not,

   ir_binop_dot,
   ir_binop_min,
   ir_binop_max,

   ir_binop_pow
};

class ir_expression : public ir_instruction {
public:
   ir_expression(int op, const struct glsl_type *type,
		 ir_instruction *, ir_instruction *);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   ir_expression_operation operation;
   ir_instruction *operands[2];
};


/**
 * IR instruction representing a function call
 */
class ir_call : public ir_instruction {
public:
   ir_call()
      : ir_instruction(ir_op_call), callee(NULL)
   {
      /* empty */
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Get a generic ir_call object when an error occurs
    */
   static ir_call *get_error_instruction();

private:
   ir_function_signature *callee;
   exec_list actual_parameters;
};


struct ir_swizzle_mask {
   unsigned x:2;
   unsigned y:2;
   unsigned z:2;
   unsigned w:2;

   /**
    * Number of components in the swizzle.
    */
   unsigned num_components:2;

   /**
    * Does the swizzle contain duplicate components?
    *
    * L-value swizzles cannot contain duplicate components.
    */
   unsigned has_duplicates:1;
};

class ir_dereference : public ir_instruction {
public:
   ir_dereference(struct ir_instruction *);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   enum {
      ir_reference_variable,
      ir_reference_array,
      ir_reference_record
   } mode;

   /**
    * Object being dereferenced.
    *
    * Must be either an \c ir_variable or an \c ir_deference.
    */
   ir_instruction *var;

   union {
      ir_expression *array_index;
      const char *field;
      struct ir_swizzle_mask swizzle;
   } selector;
};


class ir_constant : public ir_instruction {
public:
   ir_constant(const struct glsl_type *type, const void *data);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Value of the constant.
    *
    * The field used to back the values supplied by the constant is determined
    * by the type associated with the \c ir_instruction.  Constants may be
    * scalars, vectors, or matrices.
    */
   union {
      unsigned u[16];
      int i[16];
      float f[16];
      bool b[16];
   } value;
};


extern void
_mesa_glsl_initialize_variables(exec_list *instructions,
				struct _mesa_glsl_parse_state *state);
