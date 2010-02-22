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
#include "glsl_parser.tab.h"
#include "symbol_table.h"

void
_mesa_glsl_error(YYLTYPE *locp, void *state, const char *fmt, ...)
{
   char buf[1024];
   int len;
   va_list ap;

   (void) state;
   len = snprintf(buf, sizeof(buf), "%u:%u(%u): error: ",
		  locp->source, locp->first_line, locp->first_column);

   va_start(ap, fmt);
   vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
   va_end(ap);

   printf("%s\n", buf);
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
   printf("node_%d ", type);
}


ast_node::ast_node(void)
{
   make_empty_list(this);
}

void
ast_type_specifier::print(void) const
{
   switch (type_specifier) {
   case ast_void: printf("void "); break;
   case ast_float: printf("float "); break;
   case ast_int: printf("int "); break;
   case ast_uint: printf("uint "); break;
   case ast_bool: printf("bool "); break;
   case ast_vec2: printf("vec2 "); break;
   case ast_vec3: printf("vec3 "); break;
   case ast_vec4: printf("vec4 "); break;
   case ast_bvec2: printf("bvec2 "); break;
   case ast_bvec3: printf("bvec3 "); break;
   case ast_bvec4: printf("bvec4 "); break;
   case ast_ivec2: printf("ivec2 "); break;
   case ast_ivec3: printf("ivec3 "); break;
   case ast_ivec4: printf("ivec4 "); break;
   case ast_uvec2: printf("uvec2 "); break;
   case ast_uvec3: printf("uvec3 "); break;
   case ast_uvec4: printf("uvec4 "); break;
   case ast_mat2: printf("mat2 "); break;
   case ast_mat2x3: printf("mat2x3 "); break;
   case ast_mat2x4: printf("mat2x4 "); break;
   case ast_mat3x2: printf("mat3x2 "); break;
   case ast_mat3: printf("mat3 "); break;
   case ast_mat3x4: printf("mat3x4 "); break;
   case ast_mat4x2: printf("mat4x2 "); break;
   case ast_mat4x3: printf("mat4x3 "); break;
   case ast_mat4: printf("mat4 "); break;
   case ast_sampler1d: printf("sampler1d "); break;
   case ast_sampler2d: printf("sampler2d "); break;
   case ast_sampler3d: printf("sampler3d "); break;
   case ast_samplercube: printf("samplercube "); break;
   case ast_sampler1dshadow: printf("sampler1dshadow "); break;
   case ast_sampler2dshadow: printf("sampler2dshadow "); break;
   case ast_samplercubeshadow: printf("samplercubeshadow "); break;
   case ast_sampler1darray: printf("sampler1darray "); break;
   case ast_sampler2darray: printf("sampler2darray "); break;
   case ast_sampler1darrayshadow: printf("sampler1darrayshadow "); break;
   case ast_sampler2darrayshadow: printf("sampler2darrayshadow "); break;
   case ast_isampler1d: printf("isampler1d "); break;
   case ast_isampler2d: printf("isampler2d "); break;
   case ast_isampler3d: printf("isampler3d "); break;
   case ast_isamplercube: printf("isamplercube "); break;
   case ast_isampler1darray: printf("isampler1darray "); break;
   case ast_isampler2darray: printf("isampler2darray "); break;
   case ast_usampler1d: printf("usampler1d "); break;
   case ast_usampler2d: printf("usampler2d "); break;
   case ast_usampler3d: printf("usampler3d "); break;
   case ast_usamplercube: printf("usamplercube "); break;
   case ast_usampler1darray: printf("usampler1darray "); break;
   case ast_usampler2darray: printf("usampler2darray "); break;

   case ast_struct:
      structure->print();
      break;

   case ast_type_name: printf("%s ", type_name); break;
   }

   if (is_array) {
      printf("[ ");

      if (array_size) {
	 array_size->print();
      }

      printf("] ");
   }
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


ast_type_specifier::ast_type_specifier(int specifier)
{
   type_specifier = ast_types(specifier);
}


void
ast_compound_statement::print(void) const
{
   const struct simple_node *ptr;

   printf("{\n");
   
   foreach(ptr, & statements) {
      _mesa_ast_print(ptr);
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
   static const char *const operators[] = {
      "=",
      "+",
      "-",
      "+",
      "-",
      "*",
      "/",
      "%",
      "<<",
      ">>",
      "<",
      ">",
      "<=",
      ">=",
      "==",
      "!=",
      "&",
      "^",
      "|",
      "~",
      "&&",
      "^^",
      "!",

      "*=",
      "/=",
      "%=",
      "+=",
      "-=",
      "<<=",
      ">>=",
      "&=",
      "^=",
      "|=",

      "?:",
      "++",
      "--",
      "++",
      "--",
      ".",
   };


   switch (oper) {
   case ast_assign:
   case ast_add:
   case ast_sub:
   case ast_mul:
   case ast_div:
   case ast_mod:
   case ast_lshift:
   case ast_rshift:
   case ast_less:
   case ast_greater:
   case ast_lequal:
   case ast_gequal:
   case ast_equal:
   case ast_nequal:
   case ast_bit_and:
   case ast_bit_xor:
   case ast_bit_or:
   case ast_logic_and:
   case ast_logic_xor:
   case ast_logic_or:
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
      printf("%s ", operators[oper]);
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
      printf("%s ", operators[oper]);
      subexpressions[0]->print();
      break;

   case ast_post_inc:
   case ast_post_dec:
      subexpressions[0]->print();
      printf("%s ", operators[oper]);
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
	    _mesa_ast_print(ptr);
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

	 _mesa_ast_print(ptr);
      }
      printf(") ");
      break;
   }
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
      _mesa_ast_print(ptr);
   }

   printf(")");
}


ast_function::ast_function(void)
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

      _mesa_ast_print(ptr);
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
      _mesa_ast_print(ptr);
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
   struct simple_node instructions;

   (void) argc;
   shader = load_text_file(argv[1], & shader_len);

   state.scanner = NULL;
   make_empty_list(& state.translation_unit);
   state.symbols = _mesa_symbol_table_ctor();

   _mesa_glsl_lexer_ctor(& state, shader, shader_len);
   _mesa_glsl_parse(& state);
   _mesa_glsl_lexer_dtor(& state);

   foreach (ptr, & state.translation_unit) {
      _mesa_ast_print(ptr);
   }

#if 0
   make_empty_list(& instructions);
   foreach (ptr, & state.translation_unit) {
      _mesa_ast_to_hir(ptr, &instructions, &state);
   }
#endif

   _mesa_symbol_table_dtor(state.symbols);

   return 0;
}
