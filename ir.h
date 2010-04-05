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

#pragma once
#ifndef IR_H
#define IR_H

#include "list.h"
#include "ir_visitor.h"

struct ir_program {
   void *bong_hits;
};

/**
 * Base class of all IR instructions
 */
class ir_instruction : public exec_node {
public:
   const struct glsl_type *type;

   class ir_constant *constant_expression_value();
   virtual void accept(ir_visitor *) = 0;

   /**
    * \name IR instruction downcast functions
    *
    * These functions either cast the object to a derived class or return
    * \c NULL if the object's type does not match the specified derived class.
    * Additional downcast functions will be added as needed.
    */
   /*@{*/
   virtual class ir_variable *          as_variable()         { return NULL; }
   virtual class ir_dereference *       as_dereference()      { return NULL; }
   virtual class ir_rvalue *            as_rvalue()           { return NULL; }
   /*@}*/

protected:
   ir_instruction()
   {
      /* empty */
   }
};


class ir_rvalue : public ir_instruction {
public:
   virtual ir_rvalue * as_rvalue()
   {
      return this;
   }

   virtual bool is_lvalue()
   {
      return false;
   }

protected:
   ir_rvalue() : ir_instruction() { }
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

   virtual ir_variable *as_variable()
   {
      return this;
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Duplicate an IR variable
    *
    * \note
    * This will probably be made \c virtual and moved to the base class
    * eventually.
    */
   ir_variable *clone() const
   {
      ir_variable *var = new ir_variable(type, name);

      var->max_array_access = this->max_array_access;
      var->read_only = this->read_only;
      var->centroid = this->centroid;
      var->invariant = this->invariant;
      var->mode = this->mode;
      var->interpolation = this->interpolation;

      return var;
   }

   const char *name;

   /**
    * Highest element accessed with a constant expression array index
    *
    * Not used for non-array variables.
    */
   unsigned max_array_access;

   unsigned read_only:1;
   unsigned centroid:1;
   unsigned invariant:1;

   unsigned mode:3;
   unsigned interpolation:2;

   /**
    * Flag that the whole array is assignable
    *
    * In GLSL 1.20 and later whole arrays are assignable (and comparable for
    * equality).  This flag enables this behavior.
    */
   unsigned array_lvalue:1;

   /**
    * Value assigned in the initializer of a variable declared "const"
    */
   ir_constant *constant_value;
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
   ir_function_signature(const glsl_type *return_type);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Get the name of the function for which this is a signature
    */
   const char *function_name() const;

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

private:
   /** Function of which this signature is one overload. */
   class ir_function *function;

   friend class ir_function;
};


/**
 * Header for tracking functions in the symbol table
 */
class ir_function : public ir_instruction {
public:
   ir_function(const char *name);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   void add_signature(ir_function_signature *sig)
   {
      sig->function = this;
      signatures.push_tail(sig);
   }

   /**
    * Get an iterator for the set of function signatures
    */
   exec_list_iterator iterator()
   {
      return signatures.iterator();
   }

   /**
    * Find a signature that matches a set of actual parameters.
    */
   const ir_function_signature *matching_signature(exec_list *actual_param);

   /**
    * Name of the function.
    */
   const char *name;

private:
   /**
    * Set of overloaded functions with this name.
    */
   struct exec_list signatures;
};

inline const char *ir_function_signature::function_name() const
{
   return function->name;
}
/*@}*/


/**
 * IR instruction representing high-level if-statements
 */
class ir_if : public ir_instruction {
public:
   ir_if(ir_rvalue *condition)
      : condition(condition)
   {
      /* empty */
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   ir_rvalue *condition;
   exec_list  then_instructions;
   exec_list  else_instructions;
};


/**
 * IR instruction representing a high-level loop structure.
 */
class ir_loop : public ir_instruction {
public:
   ir_loop() : from(NULL), to(NULL), increment(NULL), counter(NULL)
   {
      /* empty */
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Get an iterator for the instructions of the loop body
    */
   exec_list_iterator iterator()
   {
      return body_instructions.iterator();
   }

   /** List of instructions that make up the body of the loop. */
   exec_list body_instructions;

   /**
    * \name Loop counter and controls
    */
   /*@{*/
   ir_rvalue *from;
   ir_rvalue *to;
   ir_rvalue *increment;
   ir_variable *counter;
   /*@}*/
};


class ir_assignment : public ir_rvalue {
public:
   ir_assignment(ir_rvalue *lhs, ir_rvalue *rhs, ir_rvalue *condition);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Left-hand side of the assignment.
    */
   ir_rvalue *lhs;

   /**
    * Value being assigned
    */
   ir_rvalue *rhs;

   /**
    * Optional condition for the assignment.
    */
   ir_rvalue *condition;
};

/* Update ir_expression::num_operands() and ir_print_visitor.cpp when
 * updating this list.
*/
enum ir_expression_operation {
   ir_unop_bit_not,
   ir_unop_logic_not,
   ir_unop_neg,
   ir_unop_abs,
   ir_unop_rcp,
   ir_unop_rsq,
   ir_unop_sqrt,
   ir_unop_exp,
   ir_unop_log,
   ir_unop_exp2,
   ir_unop_log2,
   ir_unop_f2i,      /**< Float-to-integer conversion. */
   ir_unop_i2f,      /**< Integer-to-float conversion. */
   ir_unop_f2b,      /**< Float-to-boolean conversion */
   ir_unop_b2f,      /**< Boolean-to-float conversion */
   ir_unop_i2b,      /**< int-to-boolean conversion */
   ir_unop_b2i,      /**< Boolean-to-int conversion */
   ir_unop_u2f,      /**< Unsigned-to-float conversion. */

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

   ir_binop_dot,
   ir_binop_min,
   ir_binop_max,

   ir_binop_pow
};

class ir_expression : public ir_rvalue {
public:
   ir_expression(int op, const struct glsl_type *type,
		 ir_rvalue *, ir_rvalue *);

   unsigned int get_num_operands(void);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   ir_expression_operation operation;
   ir_rvalue *operands[2];
};


/**
 * IR instruction representing a function call
 */
class ir_call : public ir_rvalue {
public:
   ir_call(const ir_function_signature *callee, exec_list *actual_parameters)
      : ir_rvalue(), callee(callee)
   {
      assert(callee->return_type != NULL);
      type = callee->return_type;
      actual_parameters->move_nodes_to(& this->actual_parameters);
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   /**
    * Get a generic ir_call object when an error occurs
    */
   static ir_call *get_error_instruction();

   /**
    * Get an iterator for the set of acutal parameters
    */
   exec_list_iterator iterator()
   {
      return actual_parameters.iterator();
   }

   /**
    * Get the name of the function being called.
    */
   const char *callee_name() const
   {
      return callee->function_name();
   }

private:
   ir_call()
      : ir_rvalue(), callee(NULL)
   {
      /* empty */
   }

   const ir_function_signature *callee;
   exec_list actual_parameters;
};


/**
 * \name Jump-like IR instructions.
 *
 * These include \c break, \c continue, \c return, and \c discard.
 */
/*@{*/
class ir_jump : public ir_instruction {
protected:
   ir_jump()
      : ir_instruction()
   {
      /* empty */
   }
};

class ir_return : public ir_jump {
public:
   ir_return()
      : value(NULL)
   {
      /* empty */
   }

   ir_return(ir_rvalue *value)
      : value(value)
   {
      /* empty */
   }

   ir_rvalue *get_value() const
   {
      return value;
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

private:
   ir_rvalue *value;
};


/**
 * Jump instructions used inside loops
 *
 * These include \c break and \c continue.  The \c break within a loop is
 * different from the \c break within a switch-statement.
 *
 * \sa ir_switch_jump
 */
class ir_loop_jump : public ir_jump {
public:
   enum jump_mode {
      jump_break,
      jump_continue
   };

   ir_loop_jump(ir_loop *loop, jump_mode mode)
      : loop(loop), mode(mode)
   {
      /* empty */
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   bool is_break() const
   {
      return mode == jump_break;
   }

   bool is_continue() const
   {
      return mode == jump_continue;
   }

private:
   /** Loop containing this break instruction. */
   ir_loop *loop;

   /** Mode selector for the jump instruction. */
   enum jump_mode mode;
};
/*@}*/


struct ir_swizzle_mask {
   unsigned x:2;
   unsigned y:2;
   unsigned z:2;
   unsigned w:2;

   /**
    * Number of components in the swizzle.
    */
   unsigned num_components:3;

   /**
    * Does the swizzle contain duplicate components?
    *
    * L-value swizzles cannot contain duplicate components.
    */
   unsigned has_duplicates:1;
};


class ir_swizzle : public ir_rvalue {
public:
   ir_swizzle(ir_rvalue *, unsigned x, unsigned y, unsigned z, unsigned w,
              unsigned count);
   /**
    * Construct an ir_swizzle from the textual representation.  Can fail.
    */
   static ir_swizzle *create(ir_rvalue *, const char *, unsigned vector_length);

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   bool is_lvalue()
   {
      return val->is_lvalue() && !mask.has_duplicates;
   }

   ir_rvalue *val;
   ir_swizzle_mask mask;
};


class ir_dereference : public ir_rvalue {
public:
   ir_dereference(struct ir_instruction *);

   ir_dereference(ir_instruction *variable, ir_rvalue *array_index);

   virtual ir_dereference *as_dereference()
   {
      return this;
   }

   virtual void accept(ir_visitor *v)
   {
      v->visit(this);
   }

   bool is_lvalue();

   enum {
      ir_reference_variable,
      ir_reference_array,
      ir_reference_record
   } mode;

   /**
    * Object being dereferenced.
    *
    * Must be either an \c ir_variable or an \c ir_rvalue.
    */
   ir_instruction *var;

   union {
      ir_rvalue *array_index;
      const char *field;
   } selector;
};


class ir_constant : public ir_rvalue {
public:
   ir_constant(const struct glsl_type *type, const void *data);
   ir_constant(bool b);
   ir_constant(unsigned int u);
   ir_constant(int i);
   ir_constant(float f);

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

void
visit_exec_list(exec_list *list, ir_visitor *visitor);

extern void
_mesa_glsl_initialize_variables(exec_list *instructions,
				struct _mesa_glsl_parse_state *state);

extern void
_mesa_glsl_initialize_functions(exec_list *instructions,
				struct _mesa_glsl_parse_state *state);

#endif /* IR_H */
