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

#include <cstdarg>

extern "C" {
#include <talloc.h>
}

#include "ir_reader.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "s_expression.h"

const static bool debug = false;

static void ir_read_error(_mesa_glsl_parse_state *, s_expression *,
			  const char *fmt, ...);
static const glsl_type *read_type(_mesa_glsl_parse_state *, s_expression *);

static void scan_for_prototypes(_mesa_glsl_parse_state *, exec_list *,
			        s_expression *);
static ir_function *read_function(_mesa_glsl_parse_state *, s_list *,
				  bool skip_body);
static void read_function_sig(_mesa_glsl_parse_state *, ir_function *,
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
static ir_call *read_call(_mesa_glsl_parse_state *, s_list *);
static ir_swizzle *read_swizzle(_mesa_glsl_parse_state *, s_list *);
static ir_constant *read_constant(_mesa_glsl_parse_state *, s_list *);
static ir_texture *read_texture(_mesa_glsl_parse_state *, s_list *);

static ir_dereference *read_dereference(_mesa_glsl_parse_state *,
				        s_expression *);
static ir_dereference *read_var_ref(_mesa_glsl_parse_state *, s_list *);
static ir_dereference *read_array_ref(_mesa_glsl_parse_state *, s_list *);
static ir_dereference *read_record_ref(_mesa_glsl_parse_state *, s_list *);

void
_mesa_glsl_read_ir(_mesa_glsl_parse_state *state, exec_list *instructions,
		   const char *src, bool scan_for_protos)
{
   s_expression *expr = s_expression::read_expression(state, src);
   if (expr == NULL) {
      ir_read_error(state, NULL, "couldn't parse S-Expression.");
      return;
   }
   
   if (scan_for_protos) {
      scan_for_prototypes(state, instructions, expr);
      if (state->error)
	 return;
   }

   read_instructions(state, instructions, expr, NULL);
   talloc_free(expr);

   if (debug)
      validate_ir_tree(instructions);
}

static void
ir_read_error(_mesa_glsl_parse_state *state, s_expression *expr,
	      const char *fmt, ...)
{
   va_list ap;

   state->error = true;

   if (state->current_function != NULL)
      state->info_log = talloc_asprintf_append(state->info_log,
			   "In function %s:\n",
			   state->current_function->function_name());
   state->info_log = talloc_strdup_append(state->info_log, "error: ");

   va_start(ap, fmt);
   state->info_log = talloc_vasprintf_append(state->info_log, fmt, ap);
   va_end(ap);
   state->info_log = talloc_strdup_append(state->info_log, "\n");

   if (expr != NULL) {
      state->info_log = talloc_strdup_append(state->info_log,
					     "...in this context:\n   ");
      expr->print();
      state->info_log = talloc_strdup_append(state->info_log, "\n\n");
   }
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
	    ir_read_error(st, NULL, "when reading base type of array");
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
   void *ctx = st;
   bool added = false;
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
      f = new(ctx) ir_function(name->value());
      added = st->symbols->add_function(f->name, f);
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

      read_function_sig(st, f, siglist, skip_body);
   }
   return added ? f : NULL;
}

static void
read_function_sig(_mesa_glsl_parse_state *st, ir_function *f, s_list *list,
		  bool skip_body)
{
   void *ctx = st;
   if (list->length() != 4) {
      ir_read_error(st, list, "Expected (signature <type> (parameters ...) "
			      "(<instruction> ...))");
      return;
   }

   s_expression *type_expr = (s_expression*) list->subexpressions.head->next;
   const glsl_type *return_type = read_type(st, type_expr);
   if (return_type == NULL)
      return;

   s_list *paramlist = SX_AS_LIST(type_expr->next);
   s_list *body_list = SX_AS_LIST(paramlist->next);
   if (paramlist == NULL || body_list == NULL) {
      ir_read_error(st, list, "Expected (signature <type> (parameters ...) "
			      "(<instruction> ...))");
      return;
   }
   s_symbol *paramtag = SX_AS_SYMBOL(paramlist->subexpressions.get_head());
   if (paramtag == NULL || strcmp(paramtag->value(), "parameters") != 0) {
      ir_read_error(st, paramlist, "Expected (parameters ...)");
      return;
   }

   // Read the parameters list into a temporary place.
   exec_list hir_parameters;
   st->symbols->push_scope();

   exec_list_iterator it = paramlist->subexpressions.iterator();
   for (it.next() /* skip "parameters" */; it.has_next(); it.next()) {
      s_list *decl = SX_AS_LIST(it.get());
      ir_variable *var = read_declaration(st, decl);
      if (var == NULL)
	 return;

      hir_parameters.push_tail(var);
   }

   ir_function_signature *sig = f->exact_matching_signature(&hir_parameters);
   if (sig == NULL && skip_body) {
      /* If scanning for prototypes, generate a new signature. */
      sig = new(ctx) ir_function_signature(return_type, glsl_precision_undefined);
      sig->is_builtin = true;
      f->add_signature(sig);
   } else if (sig != NULL) {
      const char *badvar = sig->qualifiers_match(&hir_parameters);
      if (badvar != NULL) {
	 ir_read_error(st, list, "function `%s' parameter `%s' qualifiers "
		       "don't match prototype", f->name, badvar);
	 return;
      }

      if (sig->return_type != return_type) {
	 ir_read_error(st, list, "function `%s' return type doesn't "
		       "match prototype", f->name);
	 return;
      }
   } else {
      /* No prototype for this body exists - skip it. */
      st->symbols->pop_scope();
      return;
   }
   assert(sig != NULL);

   sig->replace_parameters(&hir_parameters);

   if (!skip_body && !body_list->subexpressions.is_empty()) {
      if (sig->is_defined) {
	 ir_read_error(st, list, "function %s redefined", f->name);
	 return;
      }
      st->current_function = sig;
      read_instructions(st, &sig->body, body_list, NULL);
      st->current_function = NULL;
      sig->is_defined = true;
   }

   st->symbols->pop_scope();
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
      if (ir != NULL) {
	 /* Global variable declarations should be moved to the top, before
	  * any functions that might use them.  Functions are added to the
	  * instruction stream when scanning for prototypes, so without this
	  * hack, they always appear before variable declarations.
	  */
	 if (st->current_function == NULL && ir->as_variable() != NULL)
	    instructions->push_head(ir);
	 else
	    instructions->push_tail(ir);
      }
   }
}


static ir_instruction *
read_instruction(_mesa_glsl_parse_state *st, s_expression *expr,
	         ir_loop *loop_ctx)
{
   void *ctx = st;
   s_symbol *symbol = SX_AS_SYMBOL(expr);
   if (symbol != NULL) {
      if (strcmp(symbol->value(), "break") == 0 && loop_ctx != NULL)
	 return new(ctx) ir_loop_jump(ir_loop_jump::jump_break);
      if (strcmp(symbol->value(), "continue") == 0 && loop_ctx != NULL)
	 return new(ctx) ir_loop_jump(ir_loop_jump::jump_continue);
   }

   s_list *list = SX_AS_LIST(expr);
   if (list == NULL || list->subexpressions.is_empty()) {
      ir_read_error(st, expr, "Invalid instruction.\n");
      return NULL;
   }

   s_symbol *tag = SX_AS_SYMBOL(list->subexpressions.get_head());
   if (tag == NULL) {
      ir_read_error(st, expr, "expected instruction tag");
      return NULL;
   }

   ir_instruction *inst = NULL;
   if (strcmp(tag->value(), "declare") == 0) {
      inst = read_declaration(st, list);
   } else if (strcmp(tag->value(), "assign") == 0) {
      inst = read_assignment(st, list);
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
	 ir_read_error(st, NULL, "when reading instruction");
   }
   return inst;
}


static ir_variable *
read_declaration(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
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

   ir_variable *var = new(ctx) ir_variable(type, var_name->value(),
					   ir_var_auto, glsl_precision_undefined);

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
   st->symbols->add_variable(var->name, var);

   return var;
}


static ir_if *
read_if(_mesa_glsl_parse_state *st, s_list *list, ir_loop *loop_ctx)
{
   void *ctx = st;
   if (list->length() != 4) {
      ir_read_error(st, list, "expected (if <condition> (<then> ...) "
                          "(<else> ...))");
      return NULL;
   }

   s_expression *cond_expr = (s_expression*) list->subexpressions.head->next;
   ir_rvalue *condition = read_rvalue(st, cond_expr);
   if (condition == NULL) {
      ir_read_error(st, NULL, "when reading condition of (if ...)");
      return NULL;
   }

   s_expression *then_expr = (s_expression*) cond_expr->next;
   s_expression *else_expr = (s_expression*) then_expr->next;

   ir_if *iff = new(ctx) ir_if(condition);

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
   void *ctx = st;
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

   ir_loop *loop = new(ctx) ir_loop;
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
   void *ctx = st;
   if (list->length() != 2) {
      ir_read_error(st, list, "expected (return <rvalue>)");
      return NULL;
   }

   s_expression *expr = (s_expression*) list->subexpressions.head->next;

   ir_rvalue *retval = read_rvalue(st, expr);
   if (retval == NULL) {
      ir_read_error(st, NULL, "when reading return value");
      return NULL;
   }

   return new(ctx) ir_return(retval);
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

   ir_rvalue *rvalue = read_dereference(st, list);
   if (rvalue != NULL || st->error)
      return rvalue;
   else if (strcmp(tag->value(), "swiz") == 0) {
      rvalue = read_swizzle(st, list);
   } else if (strcmp(tag->value(), "expression") == 0) {
      rvalue = read_expression(st, list);
   } else if (strcmp(tag->value(), "call") == 0) {
      rvalue = read_call(st, list);
   } else if (strcmp(tag->value(), "constant") == 0) {
      rvalue = read_constant(st, list);
   } else {
      rvalue = read_texture(st, list);
      if (rvalue == NULL && !st->error)
	 ir_read_error(st, expr, "unrecognized rvalue tag: %s", tag->value());
   }

   return rvalue;
}

static ir_assignment *
read_assignment(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   if (list->length() != 5) {
      ir_read_error(st, list, "expected (assign <condition> (<write mask>) "
			      "<lhs> <rhs>)");
      return NULL;
   }

   s_expression *cond_expr = (s_expression*) list->subexpressions.head->next;
   s_list       *mask_list = SX_AS_LIST(cond_expr->next);
   s_expression *lhs_expr  = (s_expression*) cond_expr->next->next;
   s_expression *rhs_expr  = (s_expression*) lhs_expr->next;

   ir_rvalue *condition = read_rvalue(st, cond_expr);
   if (condition == NULL) {
      ir_read_error(st, NULL, "when reading condition of assignment");
      return NULL;
   }

   if (mask_list == NULL || mask_list->length() > 1) {
      ir_read_error(st, mask_list, "expected () or (<write mask>)");
      return NULL;
   }

   unsigned mask = 0;
   if (mask_list->length() == 1) {
      s_symbol *mask_symbol = SX_AS_SYMBOL(mask_list->subexpressions.head);
      if (mask_symbol == NULL) {
	 ir_read_error(st, list, "expected a write mask; found non-symbol");
	 return NULL;
      }

      const char *mask_str = mask_symbol->value();
      unsigned mask_length = strlen(mask_str);
      if (mask_length > 4) {
	 ir_read_error(st, list, "invalid write mask: %s", mask_str);
	 return NULL;
      }

      const unsigned idx_map[] = { 3, 0, 1, 2 }; /* w=bit 3, x=0, y=1, z=2 */

      for (unsigned i = 0; i < mask_length; i++) {
	 if (mask_str[i] < 'w' || mask_str[i] > 'z') {
	    ir_read_error(st, list, "write mask contains invalid character: %c",
			  mask_str[i]);
	    return NULL;
	 }
	 mask |= 1 << idx_map[mask_str[i] - 'w'];
      }
   }

   ir_dereference *lhs = read_dereference(st, lhs_expr);
   if (lhs == NULL) {
      ir_read_error(st, NULL, "when reading left-hand side of assignment");
      return NULL;
   }

   ir_rvalue *rhs = read_rvalue(st, rhs_expr);
   if (rhs == NULL) {
      ir_read_error(st, NULL, "when reading right-hand side of assignment");
      return NULL;
   }

   if (mask == 0 && (lhs->type->is_vector() || lhs->type->is_scalar())) {
      ir_read_error(st, list, "non-zero write mask required.");
      return NULL;
   }

   return new(ctx) ir_assignment(lhs, rhs, condition, mask);
}

static ir_call *
read_call(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (call <name> (<param> ...))");
      return NULL;
   }

   s_symbol *name = SX_AS_SYMBOL(list->subexpressions.head->next);
   s_list *params = SX_AS_LIST(name->next);
   if (name == NULL || params == NULL) {
      ir_read_error(st, list, "expected (call <name> (<param> ...))");
      return NULL;
   }

   exec_list parameters;

   foreach_iter(exec_list_iterator, it, params->subexpressions) {
      s_expression *expr = (s_expression*) it.get();
      ir_rvalue *param = read_rvalue(st, expr);
      if (param == NULL) {
	 ir_read_error(st, list, "when reading parameter to function call");
	 return NULL;
      }
      parameters.push_tail(param);
   }

   ir_function *f = st->symbols->get_function(name->value());
   if (f == NULL) {
      ir_read_error(st, list, "found call to undefined function %s",
		    name->value());
      return NULL;
   }

   ir_function_signature *callee = f->matching_signature(&parameters);
   if (callee == NULL) {
      ir_read_error(st, list, "couldn't find matching signature for function "
                    "%s", name->value());
      return NULL;
   }

   return new(ctx) ir_call(callee, &parameters);
}

static ir_expression *
read_expression(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
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
	 ir_read_error(st, list, "expected (expression <type> %s <operand> "
				 " <operand>)", op_sym->value());
	 return NULL;
      }
   } else {
      if (list_length != 4) {
	 ir_read_error(st, list, "expected (expression <type> %s <operand>)",
		       op_sym->value());
	 return NULL;
      }
   }

   s_expression *exp1 = (s_expression*) (op_sym->next);
   ir_rvalue *arg1 = read_rvalue(st, exp1);
   if (arg1 == NULL) {
      ir_read_error(st, NULL, "when reading first operand of %s",
		    op_sym->value());
      return NULL;
   }

   ir_rvalue *arg2 = NULL;
   if (ir_expression::get_num_operands(op) == 2) {
      s_expression *exp2 = (s_expression*) (exp1->next);
      arg2 = read_rvalue(st, exp2);
      if (arg2 == NULL) {
	 ir_read_error(st, NULL, "when reading second operand of %s",
		       op_sym->value());
	 return NULL;
      }
   }

   return new(ctx) ir_expression(op, type, arg1, arg2);
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

   ir_swizzle *ir = ir_swizzle::create(rvalue, swiz->value(),
				       rvalue->type->vector_elements);
   if (ir == NULL)
      ir_read_error(st, list, "invalid swizzle");

   return ir;
}

static ir_constant *
read_constant(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (constant <type> (...))");
      return NULL;
   }

   s_expression *type_expr = (s_expression*) list->subexpressions.head->next;
   const glsl_type *type = read_type(st, type_expr);
   if (type == NULL)
      return NULL;

   s_list *values = SX_AS_LIST(type_expr->next);
   if (values == NULL) {
      ir_read_error(st, list, "expected (constant <type> (...))");
      return NULL;
   }

   if (type->is_array()) {
      const unsigned elements_supplied = values->length();
      if (elements_supplied != type->length) {
	 ir_read_error(st, values, "expected exactly %u array elements, "
		       "given %u", type->length, elements_supplied);
	 return NULL;
      }

      exec_list elements;
      foreach_iter(exec_list_iterator, it, values->subexpressions) {
	 s_expression *expr = (s_expression *) it.get();
	 s_list *elt = SX_AS_LIST(expr);
	 if (elt == NULL) {
	    ir_read_error(st, expr, "expected (constant ...) array element");
	    return NULL;
	 }

	 ir_constant *ir_elt = read_constant(st, elt);
	 if (ir_elt == NULL)
	    return NULL;
	 elements.push_tail(ir_elt);
      }
      return new(ctx) ir_constant(type, &elements);
   }

   const glsl_type *const base_type = type->get_base_type();

   ir_constant_data data = { { 0 } };

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
	 data.f[k] = value->fvalue();
      } else {
	 s_int *value = SX_AS_INT(expr);
	 if (value == NULL) {
	    ir_read_error(st, values, "expected integers");
	    return NULL;
	 }

	 switch (base_type->base_type) {
	 case GLSL_TYPE_UINT: {
	    data.u[k] = value->value();
	    break;
	 }
	 case GLSL_TYPE_INT: {
	    data.i[k] = value->value();
	    break;
	 }
	 case GLSL_TYPE_BOOL: {
	    data.b[k] = value->value();
	    break;
	 }
	 default:
	    ir_read_error(st, values, "unsupported constant type");
	    return NULL;
	 }
      }
      ++k;
   }

   return new(ctx) ir_constant(type, &data);
}

static ir_dereference *
read_dereference(_mesa_glsl_parse_state *st, s_expression *expr)
{
   s_list *list = SX_AS_LIST(expr);
   if (list == NULL || list->subexpressions.is_empty())
      return NULL;

   s_symbol *tag = SX_AS_SYMBOL(list->subexpressions.head);
   assert(tag != NULL);

   if (strcmp(tag->value(), "var_ref") == 0)
      return read_var_ref(st, list);
   if (strcmp(tag->value(), "array_ref") == 0)
      return read_array_ref(st, list);
   if (strcmp(tag->value(), "record_ref") == 0)
      return read_record_ref(st, list);
   return NULL;
}

static ir_dereference *
read_var_ref(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   if (list->length() != 2) {
      ir_read_error(st, list, "expected (var_ref <variable name>)");
      return NULL;
   }
   s_symbol *var_name = SX_AS_SYMBOL(list->subexpressions.head->next);
   if (var_name == NULL) {
      ir_read_error(st, list, "expected (var_ref <variable name>)");
      return NULL;
   }

   ir_variable *var = st->symbols->get_variable(var_name->value());
   if (var == NULL) {
      ir_read_error(st, list, "undeclared variable: %s", var_name->value());
      return NULL;
   }

   return new(ctx) ir_dereference_variable(var);
}

static ir_dereference *
read_array_ref(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (array_ref <rvalue> <index>)");
      return NULL;
   }

   s_expression *subj_expr = (s_expression*) list->subexpressions.head->next;
   ir_rvalue *subject = read_rvalue(st, subj_expr);
   if (subject == NULL) {
      ir_read_error(st, NULL, "when reading the subject of an array_ref");
      return NULL;
   }

   s_expression *idx_expr = (s_expression*) subj_expr->next;
   ir_rvalue *idx = read_rvalue(st, idx_expr);
   return new(ctx) ir_dereference_array(subject, idx);
}

static ir_dereference *
read_record_ref(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   if (list->length() != 3) {
      ir_read_error(st, list, "expected (record_ref <rvalue> <field>)");
      return NULL;
   }

   s_expression *subj_expr = (s_expression*) list->subexpressions.head->next;
   ir_rvalue *subject = read_rvalue(st, subj_expr);
   if (subject == NULL) {
      ir_read_error(st, NULL, "when reading the subject of a record_ref");
      return NULL;
   }

   s_symbol *field = SX_AS_SYMBOL(subj_expr->next);
   if (field == NULL) {
      ir_read_error(st, list, "expected (record_ref ... <field name>)");
      return NULL;
   }
   return new(ctx) ir_dereference_record(subject, field->value());
}

static bool
valid_texture_list_length(ir_texture_opcode op, s_list *list)
{
   unsigned required_length = 7;
   if (op == ir_txf)
      required_length = 5;
   else if (op == ir_tex)
      required_length = 6;

   return list->length() == required_length;
}

static ir_texture *
read_texture(_mesa_glsl_parse_state *st, s_list *list)
{
   void *ctx = st;
   s_symbol *tag = SX_AS_SYMBOL(list->subexpressions.head);
   assert(tag != NULL);

   ir_texture_opcode op = ir_texture::get_opcode(tag->value());
   if (op == (ir_texture_opcode) -1)
      return NULL;

   if (!valid_texture_list_length(op, list)) {
      ir_read_error(st, NULL, "invalid list size in (%s ...)", tag->value());
      return NULL;
   }

   ir_texture *tex = new(ctx) ir_texture(op);

   // Read sampler (must be a deref)
   s_expression *sampler_expr = (s_expression *) tag->next;
   ir_dereference *sampler = read_dereference(st, sampler_expr);
   if (sampler == NULL) {
      ir_read_error(st, NULL, "when reading sampler in (%s ...)", tag->value());
      return NULL;
   }
   tex->set_sampler(sampler);

   // Read coordinate (any rvalue)
   s_expression *coordinate_expr = (s_expression *) sampler_expr->next;
   tex->coordinate = read_rvalue(st, coordinate_expr);
   if (tex->coordinate == NULL) {
      ir_read_error(st, NULL, "when reading coordinate in (%s ...)",
		    tag->value());
      return NULL;
   }

   // Read texel offset, i.e. (0 0 0)
   s_list *offset_list = SX_AS_LIST(coordinate_expr->next);
   if (offset_list == NULL || offset_list->length() != 3) {
      ir_read_error(st, offset_list, "expected (<int> <int> <int>)");
      return NULL;
   }
   s_int *offset_x = SX_AS_INT(offset_list->subexpressions.head);
   s_int *offset_y = SX_AS_INT(offset_x->next);
   s_int *offset_z = SX_AS_INT(offset_y->next);
   if (offset_x == NULL || offset_y == NULL || offset_z == NULL) {
      ir_read_error(st, offset_list, "expected (<int> <int> <int>)");
      return NULL;
   }
   tex->offsets[0] = offset_x->value();
   tex->offsets[1] = offset_y->value();
   tex->offsets[2] = offset_z->value();

   if (op == ir_txf) {
      s_expression *lod_expr = (s_expression *) offset_list->next;
      tex->lod_info.lod = read_rvalue(st, lod_expr);
      if (tex->lod_info.lod == NULL) {
	 ir_read_error(st, NULL, "when reading LOD in (txf ...)");
	 return NULL;
      }
   } else {
      s_expression *proj_expr = (s_expression *) offset_list->next;
      s_int *proj_as_int = SX_AS_INT(proj_expr);
      if (proj_as_int && proj_as_int->value() == 1) {
	 tex->projector = NULL;
      } else {
	 tex->projector = read_rvalue(st, proj_expr);
	 if (tex->projector == NULL) {
	    ir_read_error(st, NULL, "when reading projective divide in (%s ..)",
	                  tag->value());
	    return NULL;
	 }
      }

      s_list *shadow_list = SX_AS_LIST(proj_expr->next);
      if (shadow_list == NULL) {
	 ir_read_error(st, NULL, "shadow comparitor must be a list");
	 return NULL;
      }
      if (shadow_list->subexpressions.is_empty()) {
	 tex->shadow_comparitor= NULL;
      } else {
	 tex->shadow_comparitor = read_rvalue(st, shadow_list);
	 if (tex->shadow_comparitor == NULL) {
	    ir_read_error(st, NULL, "when reading shadow comparitor in (%s ..)",
			  tag->value());
	    return NULL;
	 }
      }
      s_expression *lod_expr = (s_expression *) shadow_list->next;

      switch (op) {
      case ir_txb:
	 tex->lod_info.bias = read_rvalue(st, lod_expr);
	 if (tex->lod_info.bias == NULL) {
	    ir_read_error(st, NULL, "when reading LOD bias in (txb ...)");
	    return NULL;
	 }
	 break;
      case ir_txl:
	 tex->lod_info.lod = read_rvalue(st, lod_expr);
	 if (tex->lod_info.lod == NULL) {
	    ir_read_error(st, NULL, "when reading LOD in (txl ...)");
	    return NULL;
	 }
	 break;
      case ir_txd: {
	 s_list *lod_list = SX_AS_LIST(lod_expr);
	 if (lod_list->length() != 2) {
	    ir_read_error(st, lod_expr, "expected (dPdx dPdy) in (txd ...)");
	    return NULL;
	 }
	 s_expression *dx_expr = (s_expression *) lod_list->subexpressions.head;
	 s_expression *dy_expr = (s_expression *) dx_expr->next;

	 tex->lod_info.grad.dPdx = read_rvalue(st, dx_expr);
	 if (tex->lod_info.grad.dPdx == NULL) {
	    ir_read_error(st, NULL, "when reading dPdx in (txd ...)");
	    return NULL;
	 }
	 tex->lod_info.grad.dPdy = read_rvalue(st, dy_expr);
	 if (tex->lod_info.grad.dPdy == NULL) {
	    ir_read_error(st, NULL, "when reading dPdy in (txd ...)");
	    return NULL;
	 }
	 break;
      }
      default:
	 // tex doesn't have any extra parameters and txf was handled earlier.
	 break;
      };
   }
   return tex;
}
