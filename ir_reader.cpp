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
#include <cstdio>
#include <cstdarg>
#include "ir_reader.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "s_expression.h"

static void ir_read_error(_mesa_glsl_parse_state *, s_expression *,
			  const char *fmt, ...);
static const glsl_type *read_type(_mesa_glsl_parse_state *, s_expression *);

static void scan_for_prototypes(_mesa_glsl_parse_state *, exec_list *,
			        s_expression *);
static ir_function *read_function(_mesa_glsl_parse_state *, s_list *,
				  bool skip_body);
static ir_function_signature *read_function_sig(_mesa_glsl_parse_state *,
						s_list *, bool skip_body);

static void read_instructions(_mesa_glsl_parse_state *, exec_list *,
			      s_expression *, ir_loop *);
static ir_instruction *read_instruction(_mesa_glsl_parse_state *,
				        s_expression *, ir_loop *);
static ir_variable *read_declaration(_mesa_glsl_parse_state *, s_list *);
static ir_if *read_if(_mesa_glsl_parse_state *, s_list *, ir_loop *);
static ir_loop *read_loop(_mesa_glsl_parse_state *st, s_list *list);
static ir_return *read_return(_mesa_glsl_parse_state *, s_list *);

static ir_rvalue *read_rvalue(_mesa_glsl_parse_state *, s_expression *);
static ir_assignment *read_assignment(_mesa_glsl_parse_state *, s_list *);
static ir_expression *read_expression(_mesa_glsl_parse_state *, s_list *);
static ir_swizzle *read_swizzle(_mesa_glsl_parse_state *, s_list *);
static ir_constant *read_constant(_mesa_glsl_parse_state *, s_list *);
static ir_dereference *read_var_ref(_mesa_glsl_parse_state *, s_list *);
static ir_dereference *read_array_ref(_mesa_glsl_parse_state *, s_list *);
static ir_dereference *read_record_ref(_mesa_glsl_parse_state *, s_list *);

void
_mesa_glsl_read_ir(_mesa_glsl_parse_state *state, exec_list *instructions,
		   const char *src)
{
   s_expression *expr = s_expression::read_expression(src);
   if (expr == NULL) {
      ir_read_error(state, NULL, "couldn't parse S-Expression.");
      return;
   }
   printf("S-Expression:\n");
   expr->print();
   printf("\n-------------\n");
   
   _mesa_glsl_initialize_types(state);

   /* FINISHME: Constructors probably shouldn't be emitted as part of the IR.
    * FINISHME: Once they're not, remake them by calling:
    * FINISHME: _mesa_glsl_initialize_constructors(instructions, state);
    */

   scan_for_prototypes(state, instructions, expr);
   if (state->error)
      return;

   read_instructions(state, instructions, expr, NULL);
}

static void
ir_read_error(_mesa_glsl_parse_state *state, s_expression *expr,
	      const char *fmt, ...)
{
   char buf[1024];
   int len;
   va_list ap;

   state->error = true;

   len = snprintf(buf, sizeof(buf), "error: ");

   va_start(ap, fmt);
   vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
   va_end(ap);

   printf("%s\n", buf);
}

static const glsl_type *
read_type(_mesa_glsl_parse_state *st, s_expression *expr)
{
   s_list *list = SX_AS_LIST(expr);
   if (list != NULL) {
      s_symbol *type_sym = SX_AS_SYMBOL(list->subexpressions.get_head());
      if (type_sym == NULL) {
	 ir_read_error(st, expr, "expected type (array ...) or (struct ...)");
	 return NULL;
      }
      if (strcmp(type_sym->value(), "array") == 0) {
	 if (list->length() != 3) {
	    ir_read_error(st, expr, "expected type (array <type> <int>)");
	    return NULL;
	 }

	 // Read base type
	 s_expression *base_expr = (s_expression*) type_sym->next;
	 const glsl_type *base_type = read_type(st, base_expr);
	 if (base_type == NULL) {
	    ir_read_error(st, expr, "when reading base type of array");
	    return NULL;
	 }

	 // Read array size
	 s_int *size = SX_AS_INT(base_expr->next);
	 if (size == NULL) {
	    ir_read_error(st, expr, "found non-integer array size");
	    return NULL;
	 }

	 return glsl_type::get_array_instance(base_type, size->value());
      } else if (strcmp(type_sym->value(), "struct") == 0) {
	 assert(false); // FINISHME
      } else {
	 ir_read_error(st, expr, "expected (array ...) or (struct ...); "
				 "found (%s ...)", type_sym->value());
	 return NULL;
      }
   }
   
   s_symbol *type_sym = SX_AS_SYMBOL(expr);
   if (type_sym == NULL) {
      ir_read_error(st, expr, "expected <type> (symbol or list)");
      return NULL;
   }

   const glsl_type *type = st->symbols->get_type(type_sym->value());
   if (type == NULL)
      ir_read_error(st, expr, "invalid type: %s", type_sym->value());

   return type;
}


static void
scan_for_prototypes(_mesa_glsl_parse_state *st, exec_list *instructions,
		    s_expression *expr)
{
   s_list *list = SX_AS_LIST(expr);
   if (list == NULL) {
      ir_read_error(st, expr, "Expected (<instruction> ...); found an atom.");
      return;
   }

   foreach_iter(exec_list_iterator, it, list->subexpressions) {
      s_list *sub = SX_AS_LIST(it.get());
      if (sub == NULL)
	 continue; // not a (function ...); ignore it.

      s_symbol *tag = SX_AS_SYMBOL(sub->subexpressions.get_head());
      if (tag == NULL || strcmp(tag->value(), "function") != 0)
	 continue; // not a (function ...); ignore it.

      ir_function *f = read_function(st, sub, true);
      if (f == NULL)
	 return;
      instructions->push_tail(f);
   }
}

static ir_function *
read_function(_mesa_glsl_parse_state *st, s_list *list, bool skip_body)
{
   if (list->length() < 3) {
      ir_read_error(st, list, "Expected (function <name> (signature ...) ...)");
      return NULL;
   }

   s_symbol *name = SX_AS_SYMBOL(list->subexpressions.head->next);
   if (name == NULL) {
      ir_read_error(st, list, "Expected (function <name> ...)");
      return NULL;
   }

   ir_function *f = st->symbols->get_function(name->value());
   if (f == NULL) {
      f = new ir_function(name->value());
      bool added = st->symbols->add_function(name->value(), f);
      assert(added);
   }

   exec_list_iterator it = list->subexpressions.iterator();
   it.next(); // skip "function" tag
   it.next(); // skip function name
   for (/* nothing */; it.has_next(); it.next()) {
      s_list *siglist = SX_AS_LIST(it.get());
      if (siglist == NULL) {
	 ir_read_error(st, list, "Expected (function (signature ...) ...)");
	 return NULL;
      }

      s_symbol *tag = SX_AS_SYMBOL(siglist->subexpressions.get_head());
      if (tag == NULL || strcmp(tag->value(), "signature") != 0) {
	 ir_read_error(st, siglist, "Expected (signature ...)");
	 return NULL;
      }

      ir_function_signature *sig = read_function_sig(st, siglist, skip_body);
      if (sig == NULL)
	 return NULL;

      f->add_signature(sig);
   }
   return f;
}

static ir_function_signature *
read_function_sig(_mesa_glsl_parse_state *st, s_list *list, bool skip_body)
{
   if (list->length() != 4) {
      ir_read_error(st, list, "Expected (signature <type> (parameters ...) "
			      "(<instruction> ...))");
      return NULL;
   }

   s_expression *type_expr = (s_expression*) list->subexpressions.head->next;
   const glsl_type *return_type = read_type(st, type_expr);
   if (return_type == NULL)
      return NULL;

   s_list *paramlist = SX_AS_LIST(type_expr->next);
   s_list *body_list = SX_AS_LIST(paramlist->next);
   if (paramlist == NULL || body_list == NULL) {
      ir_read_error(st, list, "Expected (signature <type> (parameters ...) "
			      "(<instruction> ...))");
      return NULL;
   }
   s_symbol *paramtag = SX_AS_SYMBOL(paramlist->subexpressions.get_head());
   if (paramtag == NULL || strcmp(paramtag->value(), "parameters") != 0) {
      ir_read_error(st, paramlist, "Expected (parameters ...)");
      return NULL;
   }

   // FINISHME: Don't create a new one!  Look for the existing prototype first
   ir_function_signature *sig = new ir_function_signature(return_type);

   st->symbols->push_scope();

   exec_list_iterator it = paramlist->subexpressions.iterator();
   for (it.next() /* skip "parameters" */; it.has_next(); it.next()) {
      s_list *decl = SX_AS_LIST(it.get());
      ir_variable *var = read_declaration(st, decl);
      if (var == NULL) {
	 delete sig;
	 return NULL;
      }

      sig->parameters.push_tail(var);
   }

   if (!skip_body)
      read_instructions(st, &sig->body, body_list, NULL);

   st->symbols->pop_scope();

   return sig;
}

static void
read_instructions(_mesa_glsl_parse_state *st, exec_list *instructions,
		  s_expression *expr, ir_loop *loop_ctx)
{
   // Read in a list of instructions
   s_list *list = SX_AS_LIST(expr);
   if (list == NULL) {
      ir_read_error(st, expr, "Expected (<instruction> ...); found an atom.");
      return;
   }

   foreach_iter(exec_list_iterator, it, list->subexpressions) {
      s_expression *sub = (s_expression*) it.get();
      ir_instruction *ir = read_instruction(st, sub, loop_ctx);
      if (ir == NULL) {
	 ir_read_error(st, sub, "Invalid instruction.\n");
	 return;
      }
      instructions->push_tail(ir);
   }
}


static ir_instruction *
read_instruction(_mesa_glsl_parse_state *st, s_expression *expr,
	         ir_loop *loop_ctx)
{
   s_symbol *symbol = SX_AS_SYMBOL(expr);
   if (symbol != NULL) {
      if (strcmp(symbol->value(), "break") == 0 && loop_ctx != NULL)
	 return new ir_loop_jump(loop_ctx, ir_loop_jump::jump_break);
      if (strcmp(symbol->value(), "continue") == 0 && loop_ctx != NULL)
	 return new ir_loop_jump(loop_ctx, ir_loop_jump::jump_continue);
   }

   s_list *list = SX_AS_LIST(expr);
   if (list == NULL || list->subexpressions.is_empty())
      return NULL;

   s_symbol *tag = SX_AS_SYMBOL(list->subexpressions.get_head());
   if (tag == NULL) {
      ir_read_error(st, expr, "expected instruction tag");
      return NULL;
   }

   ir_instruction *inst = NULL;
   if (strcmp(tag->value(), "declare") == 0) {
      inst = read_declaration(st, list);
   } else if (strcmp(tag->value(), "if") == 0) {
      inst = read_if(st, list, loop_ctx);
   } else if (strcmp(tag->value(), "loop") == 0) {
      inst = read_loop(st, list);
   } else if (strcmp(tag->value(), "return") == 0) {
      inst = read_return(st, list);
   } else if (strcmp(tag->value(), "function") == 0) {
      inst = read_function(st, list, false);
   } else {
      inst = read_rvalue(st, list);
      if (inst == NULL)
	 ir_read_error(st, list, "when reading instruction");
   }
   return inst;
}


static ir_variable *
read_declaration(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 4) {
      ir_read_error(st, list, "expected (declare (<qualifiers>) <type> "
			      "<name>)");
      return NULL;
   }

   s_list *quals = SX_AS_LIST(list->subexpressions.head->next);
   if (quals == NULL) {
      ir_read_error(st, list, "expected a list of variable qualifiers");
      return NULL;
   }

   s_expression *type_expr = (s_expression*) quals->next;
   const glsl_type *type = read_type(st, type_expr);
   if (type == NULL)
      return NULL;

   s_symbol *var_name = SX_AS_SYMBOL(type_expr->next);
   if (var_name == NULL) {
      ir_read_error(st, list, "expected variable name, found non-symbol");
      return NULL;
   }

   ir_variable *var = new ir_variable(type, var_name->value());

   foreach_iter(exec_list_iterator, it, quals->subexpressions) {
      s_symbol *qualifier = SX_AS_SYMBOL(it.get());
      if (qualifier == NULL) {
	 ir_read_error(st, list, "qualifier list must contain only symbols");
	 delete var;
	 return NULL;
      }

      // FINISHME: Check for duplicate/conflicting qualifiers.
      if (strcmp(qualifier->value(), "centroid") == 0) {
	 var->centroid = 1;
      } else if (strcmp(qualifier->value(), "invariant") == 0) {
	 var->invariant = 1;
      } else if (strcmp(qualifier->value(), "uniform") == 0) {
	 var->mode = ir_var_uniform;
      } else if (strcmp(qualifier->value(), "auto") == 0) {
	 var->mode = ir_var_auto;
      } else if (strcmp(qualifier->value(), "in") == 0) {
	 var->mode = ir_var_in;
      } else if (strcmp(qualifier->value(), "out") == 0) {
	 var->mode = ir_var_out;
      } else if (strcmp(qualifier->value(), "inout") == 0) {
	 var->mode = ir_var_inout;
      } else if (strcmp(qualifier->value(), "smooth") == 0) {
	 var->interpolation = ir_var_smooth;
      } else if (strcmp(qualifier->value(), "flat") == 0) {
	 var->interpolation = ir_var_flat;
      } else if (strcmp(qualifier->value(), "noperspective") == 0) {
	 var->interpolation = ir_var_noperspective;
      } else {
	 ir_read_error(st, list, "unknown qualifier: %s", qualifier->value());
	 delete var;
	 return NULL;
      }
   }

   // Add the variable to the symbol table
   st->symbols->add_variable(var_name->value(), var);

   return var;
}


static ir_if *
read_if(_mesa_glsl_parse_state *st, s_list *list, ir_loop *loop_ctx)
{
   if (list->length() != 4) {
      ir_read_error(st, list, "expected (if <condition> (<then> ...) "
                          "(<else> ...))");
      return NULL;
   }

   s_expression *cond_expr = (s_expression*) list->subexpressions.head->next;
   ir_rvalue *condition = read_rvalue(st, cond_expr);
   if (condition == NULL) {
      ir_read_error(st, list, "when reading condition of (if ...)");
      return NULL;
   }

   s_expression *then_expr = (s_expression*) cond_expr->next;
   s_expression *else_expr = (s_expression*) then_expr->next;

   ir_if *iff = new ir_if(condition);

   read_instructions(st, &iff->then_instructions, then_expr, loop_ctx);
   read_instructions(st, &iff->else_instructions, else_expr, loop_ctx);
   if (st->error) {
      delete iff;
      iff = NULL;
   }
   return iff;
}


static ir_loop *
read_loop(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 6) {
      ir_read_error(st, list, "expected (loop <counter> <from> <to> "
			      "<increment> <body>)");
      return NULL;
   }

   s_expression *count_expr = (s_expression*) list->subexpressions.head->next;
   s_expression *from_expr  = (s_expression*) count_expr->next;
   s_expression *to_expr    = (s_expression*) from_expr->next;
   s_expression *inc_expr   = (s_expression*) to_expr->next;
   s_expression *body_expr  = (s_expression*) inc_expr->next;

   // FINISHME: actually read the count/from/to fields.

   ir_loop *loop = new ir_loop;
   read_instructions(st, &loop->body_instructions, body_expr, loop);
   if (st->error) {
      delete loop;
      loop = NULL;
   }
   return loop;
}


static ir_return *
read_return(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 2) {
      ir_read_error(st, list, "expected (return <rvalue>)");
      return NULL;
   }

   s_expression *expr = (s_expression*) list->subexpressions.head->next;

   ir_rvalue *retval = read_rvalue(st, expr);
   if (retval == NULL) {
      ir_read_error(st, list, "when reading return value");
      return NULL;
   }

   return new ir_return(retval);
}


static ir_rvalue *
read_rvalue(_mesa_glsl_parse_state *st, s_expression *expr)
{
   s_list *list = SX_AS_LIST(expr);
   if (list == NULL || list->subexpressions.is_empty())
      return NULL;

   s_symbol *tag = SX_AS_SYMBOL(list->subexpressions.get_head());
   if (tag == NULL) {
      ir_read_error(st, expr, "expected rvalue tag");
      return NULL;
   }

   ir_rvalue *rvalue = NULL;
   if (strcmp(tag->value(), "swiz") == 0) {
      rvalue = read_swizzle(st, list);
   } else if (strcmp(tag->value(), "assign") == 0) {
      rvalue = read_assignment(st, list);
   } else if (strcmp(tag->value(), "expression") == 0) {
      rvalue = read_expression(st, list);
   // FINISHME: ir_call
   } else if (strcmp(tag->value(), "constant") == 0) {
      rvalue = read_constant(st, list);
   } else if (strcmp(tag->value(), "var_ref") == 0) {
      rvalue = read_var_ref(st, list);
   } else if (strcmp(tag->value(), "array_ref") == 0) {
      rvalue = read_array_ref(st, list);
   } else if (strcmp(tag->value(), "record_ref") == 0) {
      rvalue = read_record_ref(st, list);
   } else {
      ir_read_error(st, expr, "unrecognized rvalue tag: %s", tag->value());
   }

   return rvalue;
}

static ir_assignment *
read_assignment(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 4) {
      ir_read_error(st, list, "expected (assign <condition> <lhs> <rhs>)");
      return NULL;
   }

   s_expression *cond_expr = (s_expression*) list->subexpressions.head->next;
   s_expression *lhs_expr  = (s_expression*) cond_expr->next;
   s_expression *rhs_expr  = (s_expression*) lhs_expr->next;

   // FINISHME: Deal with "true" condition
   ir_rvalue *condition = read_rvalue(st, cond_expr);
   if (condition == NULL) {
      ir_read_error(st, list, "when reading condition of assignment");
      return NULL;
   }

   ir_rvalue *lhs = read_rvalue(st, lhs_expr);
   if (lhs == NULL) {
      ir_read_error(st, list, "when reading left-hand side of assignment");
      return NULL;
   }

   ir_rvalue *rhs = read_rvalue(st, rhs_expr);
   if (rhs == NULL) {
      ir_read_error(st, list, "when reading right-hand side of assignment");
      return NULL;
   }

   return new ir_assignment(lhs, rhs, condition);
}


static ir_expression *
read_expression(_mesa_glsl_parse_state *st, s_list *list)
{
   const unsigned list_length = list->length();
   if (list_length < 4) {
      ir_read_error(st, list, "expected (expression <type> <operator> "
			      "<operand> [<operand>])");
      return NULL;
   }

   s_expression *type_expr = (s_expression*) list->subexpressions.head->next;
   const glsl_type *type = read_type(st, type_expr);
   if (type == NULL)
      return NULL;

   /* Read the operator */
   s_symbol *op_sym = SX_AS_SYMBOL(type_expr->next);
   if (op_sym == NULL) {
      ir_read_error(st, list, "expected operator, found non-symbol");
      return NULL;
   }

   ir_expression_operation op = ir_expression::get_operator(op_sym->value());
   if (op == (ir_expression_operation) -1) {
      ir_read_error(st, list, "invalid operator: %s", op_sym->value());
      return NULL;
   }
    
   /* Now that we know the operator, check for the right number of operands */ 
   if (ir_expression::get_num_operands(op) == 2) {
      if (list_length != 5) {
	 ir_read_error(st, list, "expected (expression %s <operand> <operand>)",
		       op_sym->value());
	 return NULL;
      }
   } else {
      if (list_length != 4) {
	 ir_read_error(st, list, "expected (expression %s <operand>)",
		       op_sym->value());
	 return NULL;
      }
   }

   s_expression *exp1 = (s_expression*) (op_sym->next);
   ir_rvalue *arg1 = read_rvalue(st, exp1);
   if (arg1 == NULL) {
      ir_read_error(st, list, "when reading first operand of %s",
		    op_sym->value());
      return NULL;
   }

   ir_rvalue *arg2 = NULL;
   if (ir_expression::get_num_operands(op) == 2) {
      s_expression *exp2 = (s_expression*) (exp1->next);
      arg2 = read_rvalue(st, exp2);
      if (arg2 == NULL) {
	 ir_read_error(st, list, "when reading second operand of %s",
		       op_sym->value());
	 return NULL;
      }
   }

   return new ir_expression(op, type, arg1, arg2);
}

static ir_swizzle *
read_swizzle(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (swiz <swizzle> <rvalue>)");
      return NULL;
   }

   s_symbol *swiz = SX_AS_SYMBOL(list->subexpressions.head->next);
   if (swiz == NULL) {
      ir_read_error(st, list, "expected a valid swizzle; found non-symbol");
      return NULL;
   }

   if (strlen(swiz->value()) > 4) {
      ir_read_error(st, list, "expected a valid swizzle; found %s",
		    swiz->value());
      return NULL;
   }

   s_expression *sub = (s_expression*) swiz->next;
   if (sub == NULL) {
      ir_read_error(st, list, "expected rvalue: (swizzle %s <rvalue>)",
		    swiz->value());
      return NULL;
   }

   ir_rvalue *rvalue = read_rvalue(st, sub);
   if (rvalue == NULL)
      return NULL;

   return ir_swizzle::create(rvalue, swiz->value(),
			     rvalue->type->vector_elements);
}

static ir_constant *
read_constant(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (constant <type> (<num> ... <num>))");
      return NULL;
   }

   s_expression *type_expr = (s_expression*) list->subexpressions.head->next;
   const glsl_type *type = read_type(st, type_expr);
   if (type == NULL)
      return NULL;

   s_list *values = SX_AS_LIST(type_expr->next);
   if (values == NULL) {
      ir_read_error(st, list, "expected (constant <type> (<num> ... <num>))");
      return NULL;
   }

   const glsl_type *const base_type = type->get_base_type();

   unsigned u[16];
   int i[16];
   float f[16];
   bool b[16];

   // Read in list of values (at most 16).
   int k = 0;
   foreach_iter(exec_list_iterator, it, values->subexpressions) {
      if (k >= 16) {
	 ir_read_error(st, values, "expected at most 16 numbers");
	 return NULL;
      }

      s_expression *expr = (s_expression*) it.get();

      if (base_type->base_type == GLSL_TYPE_FLOAT) {
	 s_number *value = SX_AS_NUMBER(expr);
	 if (value == NULL) {
	    ir_read_error(st, values, "expected numbers");
	    return NULL;
	 }
	 f[k] = value->fvalue();
      } else {
	 s_int *value = SX_AS_INT(expr);
	 if (value == NULL) {
	    ir_read_error(st, values, "expected integers");
	    return NULL;
	 }

	 switch (base_type->base_type) {
	 case GLSL_TYPE_UINT: {
	    u[k] = value->value();
	    break;
	 }
	 case GLSL_TYPE_INT: {
	    i[k] = value->value();
	    break;
	 }
	 case GLSL_TYPE_BOOL: {
	    b[k] = value->value();
	    break;
	 }
	 default:
	    ir_read_error(st, values, "unsupported constant type");
	    return NULL;
	 }
      }
      ++k;
   }
   switch (base_type->base_type) {
   case GLSL_TYPE_UINT:
      return new ir_constant(type, u);
   case GLSL_TYPE_INT:
      return new ir_constant(type, i);
   case GLSL_TYPE_BOOL:
      return new ir_constant(type, b);
   case GLSL_TYPE_FLOAT:
      return new ir_constant(type, f);
   }
   return NULL; // should not be reached
}

static ir_instruction *
read_dereferencable(_mesa_glsl_parse_state *st, s_expression *expr)
{
   // Read the subject of a dereference - either a variable name or a swizzle
   s_symbol *var_name = SX_AS_SYMBOL(expr);
   if (var_name != NULL) {
      ir_variable *var = st->symbols->get_variable(var_name->value());
      if (var == NULL) {
	 ir_read_error(st, expr, "undeclared variable: %s", var_name->value());
      }
      return var;
   } else {
      // Hopefully a (swiz ...)
      s_list *list = SX_AS_LIST(expr);
      if (list != NULL && !list->subexpressions.is_empty()) {
	 s_symbol *tag = SX_AS_SYMBOL(list->subexpressions.head);
	 if (tag != NULL && strcmp(tag->value(), "swiz") == 0)
	    return read_swizzle(st, list);
      }
   }
   ir_read_error(st, expr, "expected variable name or (swiz ...)");
   return NULL;
}

static ir_dereference *
read_var_ref(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 2) {
      ir_read_error(st, list, "expected (var_ref <variable name or (swiz)>)");
      return NULL;
   }
   s_expression *subj_expr = (s_expression*) list->subexpressions.head->next;
   ir_instruction *subject = read_dereferencable(st, subj_expr);
   if (subject == NULL)
      return NULL;
   return new ir_dereference(subject);
}

static ir_dereference *
read_array_ref(_mesa_glsl_parse_state *st, s_list *list)
{
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (array_ref <variable name or (swiz)> "
			  "<rvalue>)");
      return NULL;
   }

   s_expression *subj_expr = (s_expression*) list->subexpressions.head->next;
   ir_instruction *subject = read_dereferencable(st, subj_expr);
   if (subject == NULL)
      return NULL;

   s_expression *idx_expr = (s_expression*) subj_expr->next;
   ir_rvalue *idx = read_rvalue(st, idx_expr);
   return new ir_dereference(subject, idx);
}

static ir_dereference *
read_record_ref(_mesa_glsl_parse_state *st, s_list *list)
{
   ir_read_error(st, list, "FINISHME: record refs not yet supported.");
   return NULL;
}
