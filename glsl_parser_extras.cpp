/*
 * Copyright © 2008, 2009 Intel Corporation
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
    
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_constant_folding.h"
#include "ir_print_visitor.h"

const char *
_mesa_glsl_shader_target_name(enum _mesa_glsl_parser_targets target)
{
   switch (target) {
   case vertex_shader:   return "vertex";
   case fragment_shader: return "fragment";
   case geometry_shader: return "geometry";
   }

   assert(!"Should not get here.");
}


void
_mesa_glsl_error(YYLTYPE *locp, _mesa_glsl_parse_state *state,
		 const char *fmt, ...)
{
   char buf[1024];
   int len;
   va_list ap;

   state->error = true;

   len = snprintf(buf, sizeof(buf), "%u:%u(%u): error: ",
		  locp->source, locp->first_line, locp->first_column);

   va_start(ap, fmt);
   vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
   va_end(ap);

   printf("%s\n", buf);
}


void
_mesa_glsl_warning(const YYLTYPE *locp, const _mesa_glsl_parse_state *state,
		   const char *fmt, ...)
{
   char buf[1024];
   int len;
   va_list ap;

   len = snprintf(buf, sizeof(buf), "%u:%u(%u): warning: ",
		  locp->source, locp->first_line, locp->first_column);

   va_start(ap, fmt);
   vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
   va_end(ap);

   printf("%s\n", buf);
}


bool
_mesa_glsl_process_extension(const char *name, YYLTYPE *name_locp,
			     const char *behavior, YYLTYPE *behavior_locp,
			     _mesa_glsl_parse_state *state)
{
   enum {
      extension_disable,
      extension_enable,
      extension_require,
      extension_warn
   } ext_mode;
   bool error = false;

   if (strcmp(behavior, "warn") == 0) {
      ext_mode = extension_warn;
   } else if (strcmp(behavior, "require") == 0) {
      ext_mode = extension_require;
   } else if (strcmp(behavior, "enable") == 0) {
      ext_mode = extension_enable;
   } else if (strcmp(behavior, "disable") == 0) {
      ext_mode = extension_disable;
   } else {
      _mesa_glsl_error(behavior_locp, state,
		       "Unknown extension behavior `%s'",
		       behavior);
      return false;
   }

   if (strcmp(name, "all") == 0) {
      if ((ext_mode == extension_enable) || (ext_mode == extension_require)) {
	 _mesa_glsl_error(name_locp, state, "Cannot %s all extensions",
			  (ext_mode == extension_enable)
			  ? "enable" : "require");
	 return false;
      }
   } else {
      if (ext_mode == extension_require) {
	 _mesa_glsl_error(name_locp, state, "Unknown extension `%s'",
			  name);
	 return false;
      } else {
	 _mesa_glsl_warning(name_locp, state, "Unknown extension `%s'",
			    name);
      }
   }

   return true;
}


ast_node::~ast_node()
{
   /* empty */
}


void
_mesa_ast_type_qualifier_print(const struct ast_type_qualifier *q)
{
   if (q->constant)
      printf("const ");

   if (q->invariant)
      printf("invariant ");

   if (q->attribute)
      printf("attribute ");

   if (q->varying)
      printf("varying ");

   if (q->in && q->out) 
      printf("inout ");
   else {
      if (q->in)
	 printf("in ");

      if (q->out)
	 printf("out ");
   }

   if (q->centroid)
      printf("centroid ");
   if (q->uniform)
      printf("uniform ");
   if (q->smooth)
      printf("smooth ");
   if (q->flat)
      printf("flat ");
   if (q->noperspective)
      printf("noperspective ");
}


void
ast_node::print(void) const
{
   printf("unhandled node ");
}


ast_node::ast_node(void)
{
   make_empty_list(this);
}


static void
ast_opt_array_size_print(bool is_array, const ast_expression *array_size)
{
   if (is_array) {
      printf("[ ");

      if (array_size)
	 array_size->print();

      printf("] ");
   }
}


void
ast_compound_statement::print(void) const
{
   const struct simple_node *ptr;

   printf("{\n");
   
   foreach(ptr, & statements) {
      ((ast_node *)ptr)->print();
   }

   printf("}\n");
}


ast_compound_statement::ast_compound_statement(int new_scope,
					       ast_node *statements)
{
   this->new_scope = new_scope;
   make_empty_list(& this->statements);

   if (statements != NULL) {
      /* This seems odd, but it works.  The simple_list is,
       * basically, a circular list.  insert_at_tail adds
       * the specified node to the list before the current
       * head.
       */
      insert_at_tail((struct simple_node *) statements,
		     & this->statements);
   }
}


void
ast_expression::print(void) const
{
   switch (oper) {
   case ast_assign:
   case ast_mul_assign:
   case ast_div_assign:
   case ast_mod_assign:
   case ast_add_assign:
   case ast_sub_assign:
   case ast_ls_assign:
   case ast_rs_assign:
   case ast_and_assign:
   case ast_xor_assign:
   case ast_or_assign:
      subexpressions[0]->print();
      printf("%s ", operator_string(oper));
      subexpressions[1]->print();
      break;

   case ast_field_selection:
      subexpressions[0]->print();
      printf(". %s ", primary_expression.identifier);
      break;

   case ast_plus:
   case ast_neg:
   case ast_bit_not:
   case ast_logic_not:
   case ast_pre_inc:
   case ast_pre_dec:
      printf("%s ", operator_string(oper));
      subexpressions[0]->print();
      break;

   case ast_post_inc:
   case ast_post_dec:
      subexpressions[0]->print();
      printf("%s ", operator_string(oper));
      break;

   case ast_conditional:
      subexpressions[0]->print();
      printf("? ");
      subexpressions[1]->print();
      printf(": ");
      subexpressions[1]->print();
      break;

   case ast_array_index:
      subexpressions[0]->print();
      printf("[ ");
      subexpressions[1]->print();
      printf("] ");
      break;

   case ast_function_call: {
      ast_expression *parameters = subexpressions[1];

      subexpressions[0]->print();
      printf("( ");

      if (parameters != NULL) {
	 struct simple_node *ptr;

	 parameters->print();
	 foreach (ptr, (struct simple_node *) parameters) {
	    printf(", ");
	    ((ast_node *)ptr)->print();
	 }
      }

      printf(") ");
      break;
   }

   case ast_identifier:
      printf("%s ", primary_expression.identifier);
      break;

   case ast_int_constant:
      printf("%d ", primary_expression.int_constant);
      break;

   case ast_uint_constant:
      printf("%u ", primary_expression.uint_constant);
      break;

   case ast_float_constant:
      printf("%f ", primary_expression.float_constant);
      break;

   case ast_bool_constant:
      printf("%s ",
	     primary_expression.bool_constant
	     ? "true" : "false");
      break;

   case ast_sequence: {
      struct simple_node *ptr;
      struct simple_node *const head = first_elem(& expressions);
      
      printf("( ");
      foreach (ptr, & expressions) {
	 if (ptr != head)
	    printf(", ");

	 ((ast_node *)ptr)->print();
      }
      printf(") ");
      break;
   }

   default:
      assert(0);
      break;
   }
}

ast_expression::ast_expression(int oper,
			       ast_expression *ex0,
			       ast_expression *ex1,
			       ast_expression *ex2)
{
   this->oper = ast_operators(oper);
   this->subexpressions[0] = ex0;
   this->subexpressions[1] = ex1;
   this->subexpressions[2] = ex2;
   make_empty_list(& expressions);
}


void
ast_expression_statement::print(void) const
{
   if (expression)
      expression->print();

   printf("; ");
}


ast_expression_statement::ast_expression_statement(ast_expression *ex) :
   expression(ex)
{
   /* empty */
}


void
ast_function::print(void) const
{
   struct simple_node *ptr;

   return_type->print();
   printf(" %s (", identifier);

   foreach(ptr, & parameters) {
      ((ast_node *)ptr)->print();
   }

   printf(")");
}


ast_function::ast_function(void)
   : is_definition(false), signature(NULL)
{
   make_empty_list(& parameters);
}


void
ast_fully_specified_type::print(void) const
{
   _mesa_ast_type_qualifier_print(& qualifier);
   specifier->print();
}


void
ast_parameter_declarator::print(void) const
{
   type->print();
   if (identifier)
      printf("%s ", identifier);
   ast_opt_array_size_print(is_array, array_size);
}


void
ast_function_definition::print(void) const
{
   prototype->print();
   body->print();
}


void
ast_declaration::print(void) const
{
   printf("%s ", identifier);
   ast_opt_array_size_print(is_array, array_size);

   if (initializer) {
      printf("= ");
      initializer->print();
   }
}


ast_declaration::ast_declaration(char *identifier, int is_array,
				 ast_expression *array_size,
				 ast_expression *initializer)
{
   this->identifier = identifier;
   this->is_array = is_array;
   this->array_size = array_size;
   this->initializer = initializer;
}


void
ast_declarator_list::print(void) const
{
   struct simple_node *head;
   struct simple_node *ptr;

   assert(type || invariant);

   if (type)
      type->print();
   else
      printf("invariant ");

   head = first_elem(& declarations);
   foreach (ptr, & declarations) {
      if (ptr != head)
	 printf(", ");

      ((ast_node *)ptr)->print();
   }

   printf("; ");
}


ast_declarator_list::ast_declarator_list(ast_fully_specified_type *type)
{
   this->type = type;
   make_empty_list(& this->declarations);
}

void
ast_jump_statement::print(void) const
{
   switch (mode) {
   case ast_continue:
      printf("continue; ");
      break;
   case ast_break:
      printf("break; ");
      break;
   case ast_return:
      printf("return ");
      if (opt_return_value)
	 opt_return_value->print();

      printf("; ");
      break;
   case ast_discard:
      printf("discard; ");
      break;
   }
}


ast_jump_statement::ast_jump_statement(int mode, ast_expression *return_value)
{
   this->mode = ast_jump_modes(mode);

   if (mode == ast_return)
      opt_return_value = return_value;
}


void
ast_selection_statement::print(void) const
{
   printf("if ( ");
   condition->print();
   printf(") ");

   then_statement->print();

   if (else_statement) {
      printf("else ");
      else_statement->print();
   }
   
}


ast_selection_statement::ast_selection_statement(ast_expression *condition,
						 ast_node *then_statement,
						 ast_node *else_statement)
{
   this->condition = condition;
   this->then_statement = then_statement;
   this->else_statement = else_statement;
}


void
ast_iteration_statement::print(void) const
{
   switch (mode) {
   case ast_for:
      printf("for( ");
      if (init_statement)
	 init_statement->print();
      printf("; ");

      if (condition)
	 condition->print();
      printf("; ");

      if (rest_expression)
	 rest_expression->print();
      printf(") ");

      body->print();
      break;

   case ast_while:
      printf("while ( ");
      if (condition)
	 condition->print();
      printf(") ");
      body->print();
      break;

   case ast_do_while:
      printf("do ");
      body->print();
      printf("while ( ");
      if (condition)
	 condition->print();
      printf("); ");
      break;
   }
}


ast_iteration_statement::ast_iteration_statement(int mode,
						 ast_node *init,
						 ast_node *condition,
						 ast_expression *rest_expression,
						 ast_node *body)
{
   this->mode = ast_iteration_modes(mode);
   this->init_statement = init;
   this->condition = condition;
   this->rest_expression = rest_expression;
   this->body = body;
}


void
ast_struct_specifier::print(void) const
{
   struct simple_node *ptr;

   printf("struct %s { ", name);
   foreach (ptr, & declarations) {
      ((ast_node *)ptr)->print();
   }
   printf("} ");
}


ast_struct_specifier::ast_struct_specifier(char *identifier,
					   ast_node *declarator_list)
{
   name = identifier;

   /* This seems odd, but it works.  The simple_list is,
    * basically, a circular list.  insert_at_tail adds
    * the specified node to the list before the current
    * head.
    */
   insert_at_tail((struct simple_node *) declarator_list,
		  & declarations);
}


static char *
load_text_file(const char *file_name, size_t *size)
{
	char *text = NULL;
	struct stat st;
	ssize_t total_read = 0;
	int fd = open(file_name, O_RDONLY);

	*size = 0;
	if (fd < 0) {
		return NULL;
	}

	if (fstat(fd, & st) == 0) {
	   text = (char *) malloc(st.st_size + 1);
		if (text != NULL) {
			do {
				ssize_t bytes = read(fd, text + total_read,
						     st.st_size - total_read);
				if (bytes < 0) {
					free(text);
					text = NULL;
					break;
				}

				if (bytes == 0) {
					break;
				}

				total_read += bytes;
			} while (total_read < st.st_size);

			text[total_read] = '\0';
			*size = total_read;
		}
	}

	close(fd);

	return text;
}


int
main(int argc, char **argv)
{
   struct _mesa_glsl_parse_state state;
   char *shader;
   size_t shader_len;
   struct simple_node *ptr;
   exec_list instructions;

   if (argc < 3) {
      printf("Usage: %s [v|g|f] <shader_file>\n", argv[0]);
      return EXIT_FAILURE;
   }

   switch (argv[1][0]) {
   case 'v':
      state.target = vertex_shader;
      break;
   case 'g':
      state.target = geometry_shader;
      break;
   case 'f':
      state.target = fragment_shader;
      break;
   default:
      printf("Usage: %s [v|g|f] <shader_file>\n", argv[0]);
      return EXIT_FAILURE;
   }

   shader = load_text_file(argv[2], & shader_len);

   state.scanner = NULL;
   make_empty_list(& state.translation_unit);
   state.symbols = new glsl_symbol_table;
   state.error = false;
   state.temp_index = 0;
   state.loop_or_switch_nesting = NULL;

   _mesa_glsl_lexer_ctor(& state, shader, shader_len);
   _mesa_glsl_parse(& state);
   _mesa_glsl_lexer_dtor(& state);

   foreach (ptr, & state.translation_unit) {
      ((ast_node *)ptr)->print();
   }

   _mesa_ast_to_hir(&instructions, &state);

   /* Optimization passes */
   if (!state.error) {
      /* Constant folding */
      ir_constant_folding_visitor constant_folding;
      visit_exec_list(&instructions, &constant_folding);
   }

   /* Print out the resulting IR */
   printf("\n\n");

   if (!state.error) {
      foreach_iter(exec_list_iterator, iter, instructions) {
	 ir_print_visitor v;

	 ((ir_instruction *)iter.get())->accept(& v);
	 printf("\n");
      }
   }

   delete state.symbols;

   return state.error != 0;
}
