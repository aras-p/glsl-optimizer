%{
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
    
#include "ast.h"
#include "glsl_parser_extras.h"
#include "symbol_table.h"
#include "glsl_types.h"

#define YYLEX_PARAM state->scanner

%}

%pure-parser
%locations
%error-verbose

%lex-param   {void *scanner}
%parse-param {struct _mesa_glsl_parse_state *state}

%union {
   int n;
   float real;
   char *identifier;

   union {
      struct ast_type_qualifier q;
      unsigned i;
   } type_qualifier;

   struct ast_node *node;
   struct ast_type_specifier *type_specifier;
   struct ast_fully_specified_type *fully_specified_type;
   struct ast_function *function;
   struct ast_parameter_declarator *parameter_declarator;
   struct ast_function_definition *function_definition;
   struct ast_compound_statement *compound_statement;
   struct ast_expression *expression;
   struct ast_declarator_list *declarator_list;
   struct ast_struct_specifier *struct_specifier;
   struct ast_declaration *declaration;

   struct {
      struct ast_node *cond;
      struct ast_expression *rest;
   } for_rest_statement;
}

%token ATTRIBUTE CONST BOOL FLOAT INT UINT
%token BREAK CONTINUE DO ELSE FOR IF DISCARD RETURN SWITCH CASE DEFAULT
%token BVEC2 BVEC3 BVEC4 IVEC2 IVEC3 IVEC4 UVEC2 UVEC3 UVEC4 VEC2 VEC3 VEC4
%token MAT2 MAT3 MAT4 CENTROID IN OUT INOUT UNIFORM VARYING
%token NOPERSPECTIVE FLAT SMOOTH
%token MAT2X2 MAT2X3 MAT2X4
%token MAT3X2 MAT3X3 MAT3X4
%token MAT4X2 MAT4X3 MAT4X4
%token SAMPLER1D SAMPLER2D SAMPLER3D SAMPLERCUBE SAMPLER1DSHADOW SAMPLER2DSHADOW
%token SAMPLERCUBESHADOW SAMPLER1DARRAY SAMPLER2DARRAY SAMPLER1DARRAYSHADOW
%token SAMPLER2DARRAYSHADOW ISAMPLER1D ISAMPLER2D ISAMPLER3D ISAMPLERCUBE
%token ISAMPLER1DARRAY ISAMPLER2DARRAY USAMPLER1D USAMPLER2D USAMPLER3D
%token USAMPLERCUBE USAMPLER1DARRAY USAMPLER2DARRAY
%token STRUCT VOID WHILE
%token <identifier> IDENTIFIER TYPE_NAME
%token <real> FLOATCONSTANT
%token <n> INTCONSTANT UINTCONSTANT BOOLCONSTANT
%token <identifier> FIELD_SELECTION
%token LEFT_OP RIGHT_OP
%token INC_OP DEC_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP XOR_OP MUL_ASSIGN DIV_ASSIGN ADD_ASSIGN
%token MOD_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token SUB_ASSIGN
%token INVARIANT
%token HIGH_PRECISION MEDIUM_PRECISION LOW_PRECISION PRECISION

%token VERSION EXTENSION LINE PRAGMA COLON EOL INTERFACE OUTPUT

   /* Reserved words that are not actually used in the grammar.
    */
%token ASM CLASS UNION ENUM TYPEDEF TEMPLATE THIS PACKED GOTO
%token INLINE NOINLINE VOLATILE PUBLIC STATIC EXTERN EXTERNAL
%token LONG SHORT DOUBLE HALF FIXED UNSIGNED INPUT OUPTUT
%token HVEC2 HVEC3 HVEC4 DVEC2 DVEC3 DVEC4 FVEC2 FVEC3 FVEC4
%token SAMPLER2DRECT SAMPLER3DRECT SAMPLER2DRECTSHADOW
%token SIZEOF CAST NAMESPACE USING LOWP MEDIUMP HIGHP

%type <identifier> variable_identifier
%type <node> statement
%type <node> statement_list
%type <node> simple_statement
%type <node> statement_matched
%type <node> statement_unmatched
%type <n> precision_qualifier
%type <type_qualifier> type_qualifier
%type <type_qualifier> storage_qualifier
%type <type_qualifier> interpolation_qualifier
%type <type_specifier> type_specifier
%type <type_specifier> type_specifier_no_prec
%type <type_specifier> type_specifier_nonarray
%type <n> basic_type_specifier_nonarray
%type <fully_specified_type> fully_specified_type
%type <function> function_prototype
%type <function> function_header
%type <function> function_header_with_parameters
%type <function> function_declarator
%type <parameter_declarator> parameter_declarator
%type <parameter_declarator> parameter_declaration
%type <type_qualifier> parameter_qualifier
%type <type_qualifier> parameter_type_qualifier
%type <type_specifier> parameter_type_specifier
%type <function_definition> function_definition
%type <compound_statement> compound_statement_no_new_scope
%type <compound_statement> compound_statement
%type <node> statement_no_new_scope
%type <node> expression_statement
%type <expression> expression
%type <expression> primary_expression
%type <expression> assignment_expression
%type <expression> conditional_expression
%type <expression> logical_or_expression
%type <expression> logical_xor_expression
%type <expression> logical_and_expression
%type <expression> inclusive_or_expression
%type <expression> exclusive_or_expression
%type <expression> and_expression
%type <expression> equality_expression
%type <expression> relational_expression
%type <expression> shift_expression
%type <expression> additive_expression
%type <expression> multiplicative_expression
%type <expression> unary_expression
%type <expression> constant_expression
%type <expression> integer_expression
%type <expression> postfix_expression
%type <expression> function_call_header_with_parameters
%type <expression> function_call_header_no_parameters
%type <expression> function_call_header
%type <expression> function_call_generic
%type <expression> function_call_or_method
%type <expression> function_call
%type <n> assignment_operator
%type <n> unary_operator
%type <node> function_identifier
%type <node> external_declaration
%type <declarator_list> init_declarator_list
%type <declarator_list> single_declaration
%type <expression> initializer
%type <node> declaration
%type <node> declaration_statement
%type <node> jump_statement
%type <struct_specifier> struct_specifier
%type <node> struct_declaration_list
%type <declarator_list> struct_declaration
%type <declaration> struct_declarator
%type <declaration> struct_declarator_list
%type <node> selection_statement_matched
%type <node> selection_statement_unmatched
%type <node> iteration_statement
%type <node> condition
%type <node> conditionopt
%type <node> for_init_statement
%type <for_rest_statement> for_rest_statement
%%

translation_unit: 
	version_statement
	{
	   _mesa_glsl_initialize_types(state);
	}
	external_declaration_list
	|
	{
	   state->language_version = 110;
	   _mesa_glsl_initialize_types(state);
	}
	external_declaration_list
	;

version_statement:
	VERSION INTCONSTANT EOL
	{
	   switch ($2) {
	   case 110:
	   case 120:
	   case 130:
	      /* FINISHME: Check against implementation support versions. */
	      state->language_version = $2;
	      break;
	   default:
	      _mesa_glsl_error(& @2, state, "Shading language version"
			       "%u is not supported\n", $2);
	      break;
	   }
	}
	;

external_declaration_list:
	external_declaration
	{
	   insert_at_tail(& state->translation_unit,
			  (struct simple_node *) $1);
	}
	| external_declaration_list external_declaration
	{
	   insert_at_tail(& state->translation_unit,
			  (struct simple_node *) $2);
	}
	;

variable_identifier:
	IDENTIFIER
	;

primary_expression:
	variable_identifier
	{
	   $$ = new ast_expression(ast_identifier, NULL, NULL, NULL);
	   $$->primary_expression.identifier = $1;
	}
	| INTCONSTANT
	{
	   $$ = new ast_expression(ast_int_constant, NULL, NULL, NULL);
	   $$->primary_expression.int_constant = $1;
	}
	| UINTCONSTANT
	{
	   $$ = new ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   $$->primary_expression.uint_constant = $1;
	}
	| FLOATCONSTANT
	{
	   $$ = new ast_expression(ast_float_constant, NULL, NULL, NULL);
	   $$->primary_expression.float_constant = $1;
	}
	| BOOLCONSTANT
	{
	   $$ = new ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   $$->primary_expression.bool_constant = $1;
	}
	| '(' expression ')'
	{
	   $$ = $2;
	}
	;

postfix_expression:
	primary_expression
	| postfix_expression '[' integer_expression ']'
	{
	   $$ = new ast_expression(ast_array_index, $1, $3, NULL);
	}
	| function_call
	{
	   $$ = $1;
	}
	| postfix_expression '.' IDENTIFIER
	{
	   $$ = new ast_expression(ast_field_selection, $1, NULL, NULL);
	   $$->primary_expression.identifier = $3;
	}
	| postfix_expression INC_OP
	{
	   $$ = new ast_expression(ast_post_inc, $1, NULL, NULL);
	}
	| postfix_expression DEC_OP
	{
	   $$ = new ast_expression(ast_post_dec, $1, NULL, NULL);
	}
	;

integer_expression:
	expression
	;

function_call:
	function_call_or_method
	;

function_call_or_method:
	function_call_generic
	| postfix_expression '.' function_call_generic
	{
	   $$ = new ast_expression(ast_field_selection, $1, $3, NULL);
	}
	;

function_call_generic:
	function_call_header_with_parameters ')'
	| function_call_header_no_parameters ')'
	;

function_call_header_no_parameters:
	function_call_header VOID
	| function_call_header
	;

function_call_header_with_parameters:
	function_call_header assignment_expression
	{
	   $$ = $1;
	   $$->subexpressions[1] = $2;
	}
	| function_call_header_with_parameters ',' assignment_expression
	{
	   $$ = $1;
	   insert_at_tail((struct simple_node *) $$->subexpressions[1],
			  (struct simple_node *) $3);
	}
	;

	// Grammar Note: Constructors look like functions, but lexical 
	// analysis recognized most of them as keywords. They are now
	// recognized through "type_specifier".
function_call_header:
	function_identifier '('
	{
	   $$ = new ast_expression(ast_function_call,
				   (struct ast_expression *) $1,
				   NULL, NULL);
	}
	;

function_identifier:
	type_specifier
	{
	   $$ = (struct ast_node *) $1;
   	}
	| IDENTIFIER
	{
	   ast_expression *expr =
	      new ast_expression(ast_identifier, NULL, NULL, NULL);
	   expr->primary_expression.identifier = $1;

	   $$ = (struct ast_node *) expr;
   	}
	| FIELD_SELECTION
	{
	   ast_expression *expr =
	      new ast_expression(ast_identifier, NULL, NULL, NULL);
	   expr->primary_expression.identifier = $1;

	   $$ = (struct ast_node *) expr;
   	}
	;

	// Grammar Note: No traditional style type casts.
unary_expression:
	postfix_expression
	| INC_OP unary_expression
	{
	   $$ = new ast_expression(ast_pre_inc, $2, NULL, NULL);
	}
	| DEC_OP unary_expression
	{
	   $$ = new ast_expression(ast_pre_dec, $2, NULL, NULL);
	}
	| unary_operator unary_expression
	{
	   $$ = new ast_expression($1, $2, NULL, NULL);
	}
	;

	// Grammar Note: No '*' or '&' unary ops. Pointers are not supported.
unary_operator:
	'+'	{ $$ = ast_plus; }
	| '-'	{ $$ = ast_neg; }
	| '!'	{ $$ = ast_logic_not; }
	| '~'	{ $$ = ast_bit_not; }
	;

multiplicative_expression:
	unary_expression
	| multiplicative_expression '*' unary_expression
	{
	   $$ = new ast_expression_bin(ast_mul, $1, $3);
	}
	| multiplicative_expression '/' unary_expression
	{
	   $$ = new ast_expression_bin(ast_div, $1, $3);
	}
	| multiplicative_expression '%' unary_expression
	{
	   $$ = new ast_expression_bin(ast_mod, $1, $3);
	}
	;

additive_expression:
	multiplicative_expression
	| additive_expression '+' multiplicative_expression
	{
	   $$ = new ast_expression_bin(ast_add, $1, $3);
	}
	| additive_expression '-' multiplicative_expression
	{
	   $$ = new ast_expression_bin(ast_sub, $1, $3);
	}
	;

shift_expression:
	additive_expression
	| shift_expression LEFT_OP additive_expression
	{
	   $$ = new ast_expression_bin(ast_lshift, $1, $3);
	}
	| shift_expression RIGHT_OP additive_expression
	{
	   $$ = new ast_expression_bin(ast_rshift, $1, $3);
	}
	;

relational_expression:
	shift_expression
	| relational_expression '<' shift_expression
	{
	   $$ = new ast_expression_bin(ast_less, $1, $3);
	}
	| relational_expression '>' shift_expression
	{
	   $$ = new ast_expression_bin(ast_greater, $1, $3);
	}
	| relational_expression LE_OP shift_expression
	{
	   $$ = new ast_expression_bin(ast_lequal, $1, $3);
	}
	| relational_expression GE_OP shift_expression
	{
	   $$ = new ast_expression_bin(ast_gequal, $1, $3);
	}
	;

equality_expression:
	relational_expression
	| equality_expression EQ_OP relational_expression
	{
	   $$ = new ast_expression_bin(ast_equal, $1, $3);
	}
	| equality_expression NE_OP relational_expression
	{
	   $$ = new ast_expression_bin(ast_nequal, $1, $3);
	}
	;

and_expression:
	equality_expression
	| and_expression '&' equality_expression
	{
	   $$ = new ast_expression_bin(ast_bit_or, $1, $3);
	}
	;

exclusive_or_expression:
	and_expression
	| exclusive_or_expression '^' and_expression
	{
	   $$ = new ast_expression_bin(ast_bit_xor, $1, $3);
	}
	;

inclusive_or_expression:
	exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
	{
	   $$ = new ast_expression_bin(ast_bit_or, $1, $3);
	}
	;

logical_and_expression:
	inclusive_or_expression
	| logical_and_expression AND_OP inclusive_or_expression
	{
	   $$ = new ast_expression_bin(ast_logic_and, $1, $3);
	}
	;

logical_xor_expression:
	logical_and_expression
	| logical_xor_expression XOR_OP logical_and_expression
	{
	   $$ = new ast_expression_bin(ast_logic_xor, $1, $3);
	}
	;

logical_or_expression:
	logical_xor_expression
	| logical_or_expression OR_OP logical_xor_expression
	{
	   $$ = new ast_expression_bin(ast_logic_or, $1, $3);
	}
	;

conditional_expression:
	logical_or_expression
	| logical_or_expression '?' expression ':' assignment_expression
	{
	   $$ = new ast_expression(ast_conditional, $1, $3, $5);
	}
	;

assignment_expression:
	conditional_expression
	| unary_expression assignment_operator assignment_expression
	{
	   $$ = new ast_expression($2, $1, $3, NULL);
	}
	;

assignment_operator:
	'='		{ $$ = ast_assign; }
	| MUL_ASSIGN	{ $$ = ast_mul_assign; }
	| DIV_ASSIGN	{ $$ = ast_div_assign; }
	| MOD_ASSIGN	{ $$ = ast_mod_assign; }
	| ADD_ASSIGN	{ $$ = ast_add_assign; }
	| SUB_ASSIGN	{ $$ = ast_sub_assign; }
	| LEFT_ASSIGN	{ $$ = ast_ls_assign; }
	| RIGHT_ASSIGN	{ $$ = ast_rs_assign; }
	| AND_ASSIGN	{ $$ = ast_and_assign; }
	| XOR_ASSIGN	{ $$ = ast_xor_assign; }
	| OR_ASSIGN	{ $$ = ast_or_assign; }
	;

expression:
	assignment_expression
	{
	   $$ = $1;
	}
	| expression ',' assignment_expression
	{
	   if ($1->oper != ast_sequence) {
	      $$ = new ast_expression(ast_sequence, NULL, NULL, NULL);
	      insert_at_tail(& $$->expressions, $1);
	   } else {
	      $$ = $1;
	   }

	   insert_at_tail(& $$->expressions, $3);
	}
	;

constant_expression:
	conditional_expression
	;

declaration:
	function_prototype ';'
	{
	   $$ = $1;
	}
	| init_declarator_list ';'
	{
	   $$ = $1;
	}
	| PRECISION precision_qualifier type_specifier_no_prec ';'
	{
	   $$ = NULL; /* FINISHME */
	}
	;

function_prototype:
	function_declarator ')'
	;

function_declarator:
	function_header
	| function_header_with_parameters
	;

function_header_with_parameters:
	function_header parameter_declaration
	{
	   $$ = $1;
	   insert_at_head(& $$->parameters,
			  (struct simple_node *) $2);
	}
	| function_header_with_parameters ',' parameter_declaration
	{
	   $$ = $1;
	   insert_at_head(& $$->parameters,
			  (struct simple_node *) $3);
	}
	;

function_header:
	fully_specified_type IDENTIFIER '('
	{
	   $$ = new ast_function();
	   $$->return_type = $1;
	   $$->identifier = $2;
	}
	;

parameter_declarator:
	type_specifier IDENTIFIER
	{
	   $$ = new ast_parameter_declarator();
	   $$->type = new ast_fully_specified_type();
	   $$->type->specifier = $1;
	   $$->identifier = $2;
	}
	| type_specifier IDENTIFIER '[' constant_expression ']'
	{
	   $$ = new ast_parameter_declarator();
	   $$->type = new ast_fully_specified_type();
	   $$->type->specifier = $1;
	   $$->identifier = $2;
	   $$->is_array = true;
	   $$->array_size = $4;
	}
	;

parameter_declaration:
	parameter_type_qualifier parameter_qualifier parameter_declarator
	{
	   $1.i |= $2.i;

	   $$ = $3;
	   $$->type->qualifier = $1.q;
	}
	| parameter_qualifier parameter_declarator
	{
	   $$ = $2;
	   $$->type->qualifier = $1.q;
	}
	| parameter_type_qualifier parameter_qualifier parameter_type_specifier
	{
	   $1.i |= $2.i;

	   $$ = new ast_parameter_declarator();
	   $$->type = new ast_fully_specified_type();
	   $$->type->qualifier = $1.q;
	   $$->type->specifier = $3;
	}
	| parameter_qualifier parameter_type_specifier
	{
	   $$ = new ast_parameter_declarator();
	   $$->type = new ast_fully_specified_type();
	   $$->type->qualifier = $1.q;
	   $$->type->specifier = $2;
	}
	;

parameter_qualifier:
	/* empty */	{ $$.i = 0; }
	| IN		{ $$.i = 0; $$.q.in = 1; }
	| OUT		{ $$.i = 0; $$.q.out = 1; }
	| INOUT		{ $$.i = 0; $$.q.in = 1; $$.q.out = 1; }
	;

parameter_type_specifier:
	type_specifier
	;

init_declarator_list:
	single_declaration
	| init_declarator_list ',' IDENTIFIER
	{
	   ast_declaration *decl = new ast_declaration($3, false, NULL, NULL);

	   $$ = $1;
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| init_declarator_list ',' IDENTIFIER '[' ']'
	{
	   ast_declaration *decl = new ast_declaration($3, true, NULL, NULL);

	   $$ = $1;
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| init_declarator_list ',' IDENTIFIER '[' constant_expression ']'
	{
	   ast_declaration *decl = new ast_declaration($3, true, $5, NULL);

	   $$ = $1;
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| init_declarator_list ',' IDENTIFIER '[' ']' '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($3, true, NULL, $7);

	   $$ = $1;
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| init_declarator_list ',' IDENTIFIER '[' constant_expression ']' '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($3, true, $5, $8);

	   $$ = $1;
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| init_declarator_list ',' IDENTIFIER '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($3, false, NULL, $5);

	   $$ = $1;
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	;

	// Grammar Note: No 'enum', or 'typedef'.
single_declaration:
	fully_specified_type
	{
	   $$ = new ast_declarator_list($1);
	}
	| fully_specified_type IDENTIFIER
	{
	   ast_declaration *decl = new ast_declaration($2, false, NULL, NULL);

	   $$ = new ast_declarator_list($1);
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| fully_specified_type IDENTIFIER '[' ']'
	{
	   ast_declaration *decl = new ast_declaration($2, true, NULL, NULL);

	   $$ = new ast_declarator_list($1);
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| fully_specified_type IDENTIFIER '[' constant_expression ']'
	{
	   ast_declaration *decl = new ast_declaration($2, true, $4, NULL);

	   $$ = new ast_declarator_list($1);
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| fully_specified_type IDENTIFIER '[' ']' '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($2, true, NULL, $6);

	   $$ = new ast_declarator_list($1);
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| fully_specified_type IDENTIFIER '[' constant_expression ']' '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($2, true, $4, $7);

	   $$ = new ast_declarator_list($1);
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| fully_specified_type IDENTIFIER '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($2, false, NULL, $4);

	   $$ = new ast_declarator_list($1);
	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	| INVARIANT IDENTIFIER // Vertex only.
	{
	   ast_declaration *decl = new ast_declaration($2, false, NULL, NULL);

	   $$ = new ast_declarator_list(NULL);
	   $$->invariant = true;

	   insert_at_tail(& $$->declarations,
			  (struct simple_node *) decl);
	}
	;

fully_specified_type:
	type_specifier
	{
	   $$ = new ast_fully_specified_type();
	   $$->specifier = $1;
	}
	| type_qualifier type_specifier
	{
	   $$ = new ast_fully_specified_type();
	   $$->qualifier = $1.q;
	   $$->specifier = $2;
	}
	;

interpolation_qualifier:
	SMOOTH		{ $$.i = 0; $$.q.smooth = 1; }
	| FLAT		{ $$.i = 0; $$.q.flat = 1; }
	| NOPERSPECTIVE	{ $$.i = 0; $$.q.noperspective = 1; }
	;

parameter_type_qualifier:
	CONST		{ $$.i = 0; $$.q.constant = 1; }
	;

type_qualifier:
	storage_qualifier
	| interpolation_qualifier type_qualifier
	{
	   $$.i = $1.i | $2.i;
	}
	| INVARIANT type_qualifier
	{
	   $$ = $2;
	   $$.q.invariant = 1;
	}
	;

storage_qualifier:
	CONST			{ $$.i = 0; $$.q.constant = 1; }
	| ATTRIBUTE 		{ $$.i = 0; $$.q.attribute = 1; }
	| VARYING		{ $$.i = 0; $$.q.varying = 1; }
	| CENTROID VARYING	{ $$.i = 0; $$.q.centroid = 1; $$.q.varying = 1; }
	| IN			{ $$.i = 0; $$.q.in = 1; }
	| OUT			{ $$.i = 0; $$.q.out = 1; }
	| CENTROID IN		{ $$.i = 0; $$.q.centroid = 1; $$.q.in = 1; }
	| CENTROID OUT		{ $$.i = 0; $$.q.centroid = 1; $$.q.out = 1; }
	| UNIFORM		{ $$.i = 0; $$.q.uniform = 1; }
	;

type_specifier:
	type_specifier_no_prec
	| precision_qualifier type_specifier_no_prec
	{
	   $$ = $2;
	   $$->precision = $1;
	}
	;

type_specifier_no_prec:
	type_specifier_nonarray
	| type_specifier_nonarray '[' ']'
	{
	   $$ = $1;
	   $$->is_array = true;
	   $$->array_size = NULL;
	}
	| type_specifier_nonarray '[' constant_expression ']'
	{
	   $$ = $1;
	   $$->is_array = true;
	   $$->array_size = $3;
	}
	;

type_specifier_nonarray:
	basic_type_specifier_nonarray
	{
	   $$ = new ast_type_specifier($1);
	}
	| struct_specifier
	{
	   $$ = new ast_type_specifier(ast_struct);
	   $$->structure = $1;
	}
	| TYPE_NAME
	{
	   $$ = new ast_type_specifier(ast_type_name);
	   $$->type_name = $1;
	}
	;

basic_type_specifier_nonarray:
	VOID			{ $$ = ast_void; }
	| FLOAT			{ $$ = ast_float; }
	| INT			{ $$ = ast_int; }
	| UINT			{ $$ = ast_uint; }
	| BOOL			{ $$ = ast_bool; }
	| VEC2			{ $$ = ast_vec2; }
	| VEC3			{ $$ = ast_vec3; }
	| VEC4			{ $$ = ast_vec4; }
	| BVEC2			{ $$ = ast_bvec2; }
	| BVEC3			{ $$ = ast_bvec3; }
	| BVEC4			{ $$ = ast_bvec4; }
	| IVEC2			{ $$ = ast_ivec2; }
	| IVEC3			{ $$ = ast_ivec3; }
	| IVEC4			{ $$ = ast_ivec4; }
	| UVEC2			{ $$ = ast_uvec2; }
	| UVEC3			{ $$ = ast_uvec3; }
	| UVEC4			{ $$ = ast_uvec4; }
	| MAT2			{ $$ = ast_mat2; }
	| MAT3			{ $$ = ast_mat3; }
	| MAT4			{ $$ = ast_mat4; }
	| MAT2X2		{ $$ = ast_mat2; }
	| MAT2X3		{ $$ = ast_mat2x3; }
	| MAT2X4		{ $$ = ast_mat2x4; }
	| MAT3X2		{ $$ = ast_mat3x2; }
	| MAT3X3		{ $$ = ast_mat3; }
	| MAT3X4		{ $$ = ast_mat3x4; }
	| MAT4X2		{ $$ = ast_mat4x2; }
	| MAT4X3		{ $$ = ast_mat4x3; }
	| MAT4X4		{ $$ = ast_mat4; }
	| SAMPLER1D		{ $$ = ast_sampler1d; }
	| SAMPLER2D		{ $$ = ast_sampler2d; }
	| SAMPLER3D		{ $$ = ast_sampler3d; }
	| SAMPLERCUBE		{ $$ = ast_samplercube; }
	| SAMPLER1DSHADOW	{ $$ = ast_sampler1dshadow; }
	| SAMPLER2DSHADOW	{ $$ = ast_sampler2dshadow; }
	| SAMPLERCUBESHADOW	{ $$ = ast_samplercubeshadow; }
	| SAMPLER1DARRAY	{ $$ = ast_sampler1darray; }
	| SAMPLER2DARRAY	{ $$ = ast_sampler2darray; }
	| SAMPLER1DARRAYSHADOW	{ $$ = ast_sampler1darrayshadow; }
	| SAMPLER2DARRAYSHADOW	{ $$ = ast_sampler2darrayshadow; }
	| ISAMPLER1D		{ $$ = ast_isampler1d; }
	| ISAMPLER2D		{ $$ = ast_isampler2d; }
	| ISAMPLER3D		{ $$ = ast_isampler3d; }
	| ISAMPLERCUBE		{ $$ = ast_isamplercube; }
	| ISAMPLER1DARRAY	{ $$ = ast_isampler1darray; }
	| ISAMPLER2DARRAY	{ $$ = ast_isampler2darray; }
	| USAMPLER1D		{ $$ = ast_usampler1d; }
	| USAMPLER2D		{ $$ = ast_usampler2d; }
	| USAMPLER3D		{ $$ = ast_usampler3d; }
	| USAMPLERCUBE		{ $$ = ast_usamplercube; }
	| USAMPLER1DARRAY	{ $$ = ast_usampler1darray; }
	| USAMPLER2DARRAY	{ $$ = ast_usampler2darray; }
	;

precision_qualifier:
	HIGH_PRECISION		{ $$ = ast_precision_high; }
	| MEDIUM_PRECISION	{ $$ = ast_precision_medium; }
	| LOW_PRECISION		{ $$ = ast_precision_low; }
	;

struct_specifier:
	STRUCT IDENTIFIER '{' struct_declaration_list '}'
	{
	   $$ = new ast_struct_specifier($2, $4);

	   _mesa_symbol_table_add_symbol(state->symbols, 0, $2, $$);
	}
	| STRUCT '{' struct_declaration_list '}'
	{
	   $$ = new ast_struct_specifier(NULL, $3);
	}
	;

struct_declaration_list:
	struct_declaration
	{
	   $$ = (struct ast_node *) $1;
	}
	| struct_declaration_list struct_declaration
	{
	   $$ = (struct ast_node *) $1;
	   insert_at_tail((struct simple_node *) $$,
			  (struct simple_node *) $2);
	}
	;

struct_declaration:
	type_specifier struct_declarator_list ';'
	{
	   ast_fully_specified_type *type = new ast_fully_specified_type();

	   type->specifier = $1;
	   $$ = new ast_declarator_list(type);

	   insert_at_tail((struct simple_node *) $2,
			  & $$->declarations);
	}
	;

struct_declarator_list:
	struct_declarator
	| struct_declarator_list ',' struct_declarator
	{
	   $$ = $1;
	   insert_at_tail((struct simple_node *) $$,
			  (struct simple_node *) $3);
	}
	;

struct_declarator:
	IDENTIFIER
	{
	   $$ = new ast_declaration($1, false, NULL, NULL);
	}
	| IDENTIFIER '[' constant_expression ']'
	{
	   $$ = new ast_declaration($1, true, $3, NULL);
	}
	;

initializer:
	assignment_expression
	;

declaration_statement:
	declaration
	;

	// Grammar Note: labeled statements for SWITCH only; 'goto' is not
	// supported.
statement:
	statement_matched
	| statement_unmatched
	;

statement_matched:
	compound_statement	{ $$ = (struct ast_node *) $1; }
	| simple_statement
	;

statement_unmatched:
	selection_statement_unmatched
	;

simple_statement:
	declaration_statement
	| expression_statement
	| selection_statement_matched
	| switch_statement		{ $$ = NULL; }
	| case_label			{ $$ = NULL; }
	| iteration_statement
	| jump_statement
	;

compound_statement:
	'{' '}'
	{
	   $$ = new ast_compound_statement(true, NULL);
	}
	| '{' statement_list '}'
	{
	   $$ = new ast_compound_statement(true, $2);
	}
	;

statement_no_new_scope:
	compound_statement_no_new_scope { $$ = (struct ast_node *) $1; }
	| simple_statement
	;

compound_statement_no_new_scope:
	'{' '}'
	{
	   $$ = new ast_compound_statement(false, NULL);
	}
	| '{' statement_list '}'
	{
	   $$ = new ast_compound_statement(false, $2);
	}
	;

statement_list:
	statement
	{
	   if ($1 == NULL) {
	      _mesa_glsl_error(& @1, state, "<nil> statement\n");
	      assert($1 != NULL);
	   }

	   $$ = $1;
	   make_empty_list((struct simple_node *) $$);
	}
	| statement_list statement
	{
	   if ($2 == NULL) {
	      _mesa_glsl_error(& @2, state, "<nil> statement\n");
	      assert($2 != NULL);
	   }
	   $$ = $1;
	   insert_at_tail((struct simple_node *) $$,
			  (struct simple_node *) $2);
	}
	;

expression_statement:
	';'
	{
	   $$ = new ast_expression_statement(NULL);
	}
	| expression ';'
	{
	   $$ = new ast_expression_statement($1);
	}
	;

selection_statement_matched:
	IF '(' expression ')' statement_matched ELSE statement_matched
	{
	   $$ = new ast_selection_statement($3, $5, $7);
	}
	;

selection_statement_unmatched:
	IF '(' expression ')' statement_matched
	{
	   $$ = new ast_selection_statement($3, $5, NULL);
	}
	| IF '(' expression ')' statement_unmatched
	{
	   $$ = new ast_selection_statement($3, $5, NULL);
	}
	| IF '(' expression ')' statement_matched ELSE statement_unmatched
	{
	   $$ = new ast_selection_statement($3, $5, $7);
	}
	;

condition:
	expression
	{
	   $$ = (struct ast_node *) $1;
	}
	| fully_specified_type IDENTIFIER '=' initializer
	{
	   ast_declaration *decl = new ast_declaration($2, false, NULL, $4);
	   ast_declarator_list *declarator = new ast_declarator_list($1);

	   insert_at_tail(& declarator->declarations,
			  (struct simple_node *) decl);

	   $$ = declarator;
	}
	;

switch_statement:
	SWITCH '(' expression ')' compound_statement
	;

case_label:
	CASE expression ':'
	| DEFAULT ':'
	;

iteration_statement:
	WHILE '(' condition ')' statement_no_new_scope
	{
	   $$ = new ast_iteration_statement(ast_iteration_statement::ast_while,
					    NULL, $3, NULL, $5);
	}
	| DO statement WHILE '(' expression ')' ';'
	{
	   $$ = new ast_iteration_statement(ast_iteration_statement::ast_do_while,
					    NULL, $5, NULL, $2);
	}
	| FOR '(' for_init_statement for_rest_statement ')' statement_no_new_scope
	{
	   $$ = new ast_iteration_statement(ast_iteration_statement::ast_for,
					    $3, $4.cond, $4.rest, $6);
	}
	;

for_init_statement:
	expression_statement
	| declaration_statement
	;

conditionopt:
	condition
	| /* empty */
	{
	   $$ = NULL;
	}
	;

for_rest_statement:
	conditionopt ';'
	{
	   $$.cond = $1;
	   $$.rest = NULL;
	}
	| conditionopt ';' expression
	{
	   $$.cond = $1;
	   $$.rest = $3;
	}
	;

	// Grammar Note: No 'goto'. Gotos are not supported.
jump_statement:
	CONTINUE ';' 
	{
	   $$ = new ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	}
	| BREAK ';'
	{
	   $$ = new ast_jump_statement(ast_jump_statement::ast_break, NULL);
	}
	| RETURN ';'
	{
	   $$ = new ast_jump_statement(ast_jump_statement::ast_return, NULL);
	}
	| RETURN expression ';'
	{
	   $$ = new ast_jump_statement(ast_jump_statement::ast_return, $2);
	}
	| DISCARD ';' // Fragment shader only.
	{
	   $$ = new ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	}
	;

external_declaration:
	function_definition	{ $$ = $1; }
	| declaration		{ $$ = $1; }
	;

function_definition:
	function_prototype compound_statement_no_new_scope
	{
	   $$ = new ast_function_definition();
	   $$->prototype = $1;
	   $$->body = $2;
	}
	;
