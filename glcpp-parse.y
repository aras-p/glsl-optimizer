%{
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "glcpp.h"

static void
yyerror (void *scanner, const char *error);

static void
_define_object_macro (glcpp_parser_t *parser,
		      const char *macro,
		      token_list_t *replacements);

static void
_define_function_macro (glcpp_parser_t *parser,
			const char *macro,
			string_list_t *parameters,
			token_list_t *replacements);

static string_list_t *
_string_list_create (void *ctx);

static void
_string_list_append_item (string_list_t *list, const char *str);

static void
_string_list_append_list (string_list_t *list, string_list_t *tail);

static void
_string_list_push (string_list_t *list, const char *str);

static void
_string_list_pop (string_list_t *list);

static int
_string_list_contains (string_list_t *list, const char *member, int *index);

static int
_string_list_length (string_list_t *list);

static argument_list_t *
_argument_list_create (void *ctx);

static void
_argument_list_append (argument_list_t *list, token_list_t *argument);

static int
_argument_list_length (argument_list_t *list);

static token_list_t *
_argument_list_member_at (argument_list_t *list, int index);

/* Note: This function talloc_steal()s the str pointer. */
static token_t *
_token_create_str (void *ctx, int type, char *str);

static token_t *
_token_create_ival (void *ctx, int type, int ival);

static token_list_t *
_token_list_create (void *ctx);

/* Note: This function adds a talloc_reference() to token.
 *
 * You may want to talloc_unlink any current reference if you no
 * longer need it. */
static void
_token_list_append (token_list_t *list, token_t *token);

static void
_token_list_append_list (token_list_t *list, token_list_t *tail);

static void
_glcpp_parser_evaluate_defined (glcpp_parser_t *parser,
				token_list_t *list);

static void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list);

static void
_glcpp_parser_expand_token_list_onto (glcpp_parser_t *parser,
				      token_list_t *list,
				      token_list_t *result);

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, int condition);

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, const char *type,
				    int condition);

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser);

#define yylex glcpp_parser_lex

static int
glcpp_parser_lex (glcpp_parser_t *parser);

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list);

%}

%parse-param {glcpp_parser_t *parser}
%lex-param {glcpp_parser_t *parser}

%token COMMA_FINAL DEFINED ELIF_EXPANDED HASH HASH_DEFINE_FUNC HASH_DEFINE_OBJ HASH_ELIF HASH_ELSE HASH_ENDIF HASH_IF HASH_IFDEF HASH_IFNDEF HASH_UNDEF IDENTIFIER IF_EXPANDED INTEGER NEWLINE OTHER PLACEHOLDER SPACE
%token PASTE
%type <ival> expression INTEGER operator SPACE
%type <str> IDENTIFIER OTHER
%type <string_list> identifier_list
%type <token> preprocessing_token
%type <token_list> pp_tokens replacement_list text_line
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQUAL NOT_EQUAL
%left '<' '>' LESS_OR_EQUAL GREATER_OR_EQUAL
%left LEFT_SHIFT RIGHT_SHIFT
%left '+' '-'
%left '*' '/' '%'
%right UNARY

%%

input:
	/* empty */
|	input line
;

line:
	control_line {
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			printf ("\n");
		}
	}
|	text_line {
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			_glcpp_parser_print_expanded_token_list (parser, $1);
			printf ("\n");
		}
		talloc_free ($1);
	}
|	expanded_line
|	HASH non_directive
;

expanded_line:
	IF_EXPANDED expression NEWLINE {
		_glcpp_parser_skip_stack_push_if (parser, $2);
	}
|	ELIF_EXPANDED expression NEWLINE {
		_glcpp_parser_skip_stack_change_if (parser, "elif", $2);
	}
;

control_line:
	HASH_DEFINE_OBJ	IDENTIFIER replacement_list NEWLINE {
		_define_object_macro (parser, $2, $3);
	}
|	HASH_DEFINE_FUNC IDENTIFIER '(' ')' replacement_list NEWLINE {
		_define_function_macro (parser, $2, NULL, $5);
	}
|	HASH_DEFINE_FUNC IDENTIFIER '(' identifier_list ')' replacement_list NEWLINE {
		_define_function_macro (parser, $2, $4, $6);
	}
|	HASH_UNDEF IDENTIFIER NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, $2);
		if (macro) {
			/* XXX: Need hash table to support a real way
			 * to remove an element rather than prefixing
			 * a new node with data of NULL like this. */
			hash_table_insert (parser->defines, NULL, $2);
			talloc_free (macro);
		}
		talloc_free ($2);
	}
|	HASH_IF pp_tokens NEWLINE {
		token_list_t *expanded;
		token_t *token;

		expanded = _token_list_create (parser);
		token = _token_create_ival (parser, IF_EXPANDED, IF_EXPANDED);
		_token_list_append (expanded, token);
		talloc_unlink (parser, token);
		_glcpp_parser_evaluate_defined (parser, $2);
		_glcpp_parser_expand_token_list_onto (parser, $2, expanded);
		glcpp_parser_lex_from (parser, expanded);
	}
|	HASH_IFDEF IDENTIFIER NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, $2);
		talloc_free ($2);
		_glcpp_parser_skip_stack_push_if (parser, macro != NULL);
	}
|	HASH_IFNDEF IDENTIFIER NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, $2);
		talloc_free ($2);
		_glcpp_parser_skip_stack_push_if (parser, macro == NULL);
	}
|	HASH_ELIF pp_tokens NEWLINE {
		token_list_t *expanded;
		token_t *token;

		expanded = _token_list_create (parser);
		token = _token_create_ival (parser, ELIF_EXPANDED, ELIF_EXPANDED);
		_token_list_append (expanded, token);
		talloc_unlink (parser, token);
		_glcpp_parser_evaluate_defined (parser, $2);
		_glcpp_parser_expand_token_list_onto (parser, $2, expanded);
		glcpp_parser_lex_from (parser, expanded);
	}
|	HASH_ELSE NEWLINE {
		_glcpp_parser_skip_stack_change_if (parser, "else", 1);
	}
|	HASH_ENDIF NEWLINE {
		_glcpp_parser_skip_stack_pop (parser);
	}
|	HASH NEWLINE
;

expression:
	INTEGER {
		$$ = $1;
	}
|	expression OR expression {
		$$ = $1 || $3;
	}
|	expression AND expression {
		$$ = $1 && $3;
	}
|	expression '|' expression {
		$$ = $1 | $3;
	}
|	expression '^' expression {
		$$ = $1 ^ $3;
	}
|	expression '&' expression {
		$$ = $1 & $3;
	}
|	expression NOT_EQUAL expression {
		$$ = $1 != $3;
	}
|	expression EQUAL expression {
		$$ = $1 == $3;
	}
|	expression GREATER_OR_EQUAL expression {
		$$ = $1 >= $3;
	}
|	expression LESS_OR_EQUAL expression {
		$$ = $1 <= $3;
	}
|	expression '>' expression {
		$$ = $1 > $3;
	}
|	expression '<' expression {
		$$ = $1 < $3;
	}
|	expression RIGHT_SHIFT expression {
		$$ = $1 >> $3;
	}
|	expression LEFT_SHIFT expression {
		$$ = $1 << $3;
	}
|	expression '-' expression {
		$$ = $1 - $3;
	}
|	expression '+' expression {
		$$ = $1 + $3;
	}
|	expression '%' expression {
		$$ = $1 % $3;
	}
|	expression '/' expression {
		$$ = $1 / $3;
	}
|	expression '*' expression {
		$$ = $1 * $3;
	}
|	'!' expression %prec UNARY {
		$$ = ! $2;
	}
|	'~' expression %prec UNARY {
		$$ = ~ $2;
	}
|	'-' expression %prec UNARY {
		$$ = - $2;
	}
|	'+' expression %prec UNARY {
		$$ = + $2;
	}
|	'(' expression ')' {
		$$ = $2;
	}
;

identifier_list:
	IDENTIFIER {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
		talloc_steal ($$, $1);
	}
|	identifier_list ',' IDENTIFIER {
		$$ = $1;	
		_string_list_append_item ($$, $3);
		talloc_steal ($$, $3);
	}
;

text_line:
	NEWLINE { $$ = NULL; }
|	pp_tokens NEWLINE
;

non_directive:
	pp_tokens NEWLINE
;

replacement_list:
	/* empty */ { $$ = NULL; }
|	pp_tokens
;

pp_tokens:
	preprocessing_token {
		parser->space_tokens = 1;
		$$ = _token_list_create (parser);
		_token_list_append ($$, $1);
		talloc_unlink (parser, $1);
	}
|	pp_tokens preprocessing_token {
		$$ = $1;
		_token_list_append ($$, $2);
		talloc_unlink (parser, $2);
	}
;

preprocessing_token:
	IDENTIFIER {
		$$ = _token_create_str (parser, IDENTIFIER, $1);
	}
|	INTEGER {
		$$ = _token_create_ival (parser, INTEGER, $1);
	}
|	operator {
		$$ = _token_create_ival (parser, $1, $1);
	}
|	OTHER {
		$$ = _token_create_str (parser, OTHER, $1);
	}
|	SPACE {
		$$ = _token_create_ival (parser, SPACE, SPACE);
	}
;

operator:
	'['			{ $$ = '['; }
|	']'			{ $$ = ']'; }
|	'('			{ $$ = '('; }
|	')'			{ $$ = ')'; }
|	'{'			{ $$ = '{'; }
|	'}'			{ $$ = '}'; }
|	'.'			{ $$ = '.'; }
|	'&'			{ $$ = '&'; }
|	'*'			{ $$ = '*'; }
|	'+'			{ $$ = '+'; }
|	'-'			{ $$ = '-'; }
|	'~'			{ $$ = '~'; }
|	'!'			{ $$ = '!'; }
|	'/'			{ $$ = '/'; }
|	'%'			{ $$ = '%'; }
|	LEFT_SHIFT		{ $$ = LEFT_SHIFT; }
|	RIGHT_SHIFT		{ $$ = RIGHT_SHIFT; }
|	'<'			{ $$ = '<'; }
|	'>'			{ $$ = '>'; }
|	LESS_OR_EQUAL		{ $$ = LESS_OR_EQUAL; }
|	GREATER_OR_EQUAL	{ $$ = GREATER_OR_EQUAL; }
|	EQUAL			{ $$ = EQUAL; }
|	NOT_EQUAL		{ $$ = NOT_EQUAL; }
|	'^'			{ $$ = '^'; }
|	'|'			{ $$ = '|'; }
|	AND			{ $$ = AND; }
|	OR			{ $$ = OR; }
|	';'			{ $$ = ';'; }
|	','			{ $$ = ','; }
|	PASTE			{ $$ = PASTE; }
|	DEFINED			{ $$ = DEFINED; }
;

%%

string_list_t *
_string_list_create (void *ctx)
{
	string_list_t *list;

	list = xtalloc (ctx, string_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_string_list_append_list (string_list_t *list, string_list_t *tail)
{
	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
}

void
_string_list_append_item (string_list_t *list, const char *str)
{
	string_node_t *node;

	node = xtalloc (list, string_node_t);
	node->str = xtalloc_strdup (node, str);

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

void
_string_list_push (string_list_t *list, const char *str)
{
	string_node_t *node;

	node = xtalloc (list, string_node_t);
	node->str = xtalloc_strdup (node, str);
	node->next = list->head;

	if (list->tail == NULL) {
		list->tail = node;
	}
	list->head = node;
}

void
_string_list_pop (string_list_t *list)
{
	string_node_t *node;

	node = list->head;

	if (node == NULL) {
		fprintf (stderr, "Internal error: _string_list_pop called on an empty list.\n");
		exit (1);
	}

	list->head = node->next;
	if (list->tail == node) {
		assert (node->next == NULL);
		list->tail = NULL;
	}

	talloc_free (node);
}

int
_string_list_contains (string_list_t *list, const char *member, int *index)
{
	string_node_t *node;
	int i;

	if (list == NULL)
		return 0;

	for (i = 0, node = list->head; node; i++, node = node->next) {
		if (strcmp (node->str, member) == 0) {
			if (index)
				*index = i;
			return 1;
		}
	}

	return 0;
}

int
_string_list_length (string_list_t *list)
{
	int length = 0;
	string_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

argument_list_t *
_argument_list_create (void *ctx)
{
	argument_list_t *list;

	list = xtalloc (ctx, argument_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_argument_list_append (argument_list_t *list, token_list_t *argument)
{
	argument_node_t *node;

	node = xtalloc (list, argument_node_t);
	node->argument = argument;

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_argument_list_length (argument_list_t *list)
{
	int length = 0;
	argument_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

token_list_t *
_argument_list_member_at (argument_list_t *list, int index)
{
	argument_node_t *node;
	int i;

	if (list == NULL)
		return NULL;

	node = list->head;
	for (i = 0; i < index; i++) {
		node = node->next;
		if (node == NULL)
			break;
	}

	if (node)
		return node->argument;

	return NULL;
}

/* Note: This function talloc_steal()s the str pointer. */
token_t *
_token_create_str (void *ctx, int type, char *str)
{
	token_t *token;

	token = xtalloc (ctx, token_t);
	token->type = type;
	token->value.str = talloc_steal (token, str);

	return token;
}

token_t *
_token_create_ival (void *ctx, int type, int ival)
{
	token_t *token;

	token = xtalloc (ctx, token_t);
	token->type = type;
	token->value.ival = ival;

	return token;
}

token_list_t *
_token_list_create (void *ctx)
{
	token_list_t *list;

	list = xtalloc (ctx, token_list_t);
	list->head = NULL;
	list->tail = NULL;
	list->non_space_tail = NULL;

	return list;
}

void
_token_list_append (token_list_t *list, token_t *token)
{
	token_node_t *node;

	node = xtalloc (list, token_node_t);
	node->token = xtalloc_reference (list, token);

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
	if (token->type != SPACE)
		list->non_space_tail = node;
}

void
_token_list_append_list (token_list_t *list, token_list_t *tail)
{
	if (tail == NULL || tail->head == NULL)
		return;

	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
	list->non_space_tail = tail->non_space_tail;
}

void
_token_list_trim_trailing_space (token_list_t *list)
{
	token_node_t *tail, *next;

	if (list->non_space_tail) {
		tail = list->non_space_tail->next;
		list->non_space_tail->next = NULL;
		list->tail = list->non_space_tail;

		while (tail) {
			next = tail->next;
			talloc_free (tail);
			tail = next;
		}
	}
}

static void
_token_print (token_t *token)
{
	if (token->type < 256) {
		printf ("%c", token->type);
		return;
	}

	switch (token->type) {
	case INTEGER:
		printf ("%" PRIxMAX, token->value.ival);
		break;
	case IDENTIFIER:
	case OTHER:
		printf ("%s", token->value.str);
		break;
	case SPACE:
		printf (" ");
		break;
	case LEFT_SHIFT:
		printf ("<<");
		break;
	case RIGHT_SHIFT:
		printf (">>");
		break;
	case LESS_OR_EQUAL:
		printf ("<=");
		break;
	case GREATER_OR_EQUAL:
		printf (">=");
		break;
	case EQUAL:
		printf ("==");
		break;
	case NOT_EQUAL:
		printf ("!=");
		break;
	case AND:
		printf ("&&");
		break;
	case OR:
		printf ("||");
		break;
	case PASTE:
		printf ("##");
		break;
	case COMMA_FINAL:
		printf (",");
		break;
	case PLACEHOLDER:
		/* Nothing to print. */
		break;
	default:
		fprintf (stderr, "Error: Don't know how to print token type %d\n", token->type);
		break;
	}
}

/* Change 'token' into a new token formed by pasting 'other'. */
static void
_token_paste (token_t *token, token_t *other)
{
	/* Pasting a placeholder onto anything makes no change. */
	if (other->type == PLACEHOLDER)
		return;

	/* When 'token' is a placeholder, just return contents of 'other'. */
	if (token->type == PLACEHOLDER) {
		token->type = other->type;
		token->value = other->value;
		return;
	}

	/* A very few single-character punctuators can be combined
	 * with another to form a multi-character punctuator. */
	switch (token->type) {
	case '<':
		if (other->type == '<') {
			token->type = LEFT_SHIFT;
			token->value.ival = LEFT_SHIFT;
			return;
		} else if (other->type == '=') {
			token->type = LESS_OR_EQUAL;
			token->value.ival = LESS_OR_EQUAL;
			return;
		}
		break;
	case '>':
		if (other->type == '>') {
			token->type = RIGHT_SHIFT;
			token->value.ival = RIGHT_SHIFT;
			return;
		} else if (other->type == '=') {
			token->type = GREATER_OR_EQUAL;
			token->value.ival = GREATER_OR_EQUAL;
			return;
		}
		break;
	case '=':
		if (other->type == '=') {
			token->type = EQUAL;
			token->value.ival = EQUAL;
			return;
		}
		break;
	case '!':
		if (other->type == '=') {
			token->type = NOT_EQUAL;
			token->value.ival = NOT_EQUAL;
			return;
		}
		break;
	case '&':
		if (other->type == '&') {
			token->type = AND;
			token->value.ival = AND;
			return;
		}
		break;
	case '|':
		if (other->type == '|') {
			token->type = OR;
			token->value.ival = OR;
			return;
		}
		break;
	}

	/* Two string-valued tokens can usually just be mashed
	 * together.
	 *
	 * XXX: Since our 'OTHER' case is currently so loose, this may
	 * allow some things thruogh that should be treated as
	 * errors. */
	if ((token->type == IDENTIFIER || token->type == OTHER) &&
	    (other->type == IDENTIFIER || other->type == OTHER))
	{
		token->value.str = talloc_strdup_append (token->value.str,
							 other->value.str);
		return;
	}

	printf ("Error: Pasting \"");
	_token_print (token);
	printf ("\" and \"");
	_token_print (other);
	printf ("\" does not give a valid preprocessing token.\n");
}

static void
_token_list_print (token_list_t *list)
{
	token_node_t *node;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next)
		_token_print (node->token);
}

void
yyerror (void *scanner, const char *error)
{
	fprintf (stderr, "Parse error: %s\n", error);
}

glcpp_parser_t *
glcpp_parser_create (void)
{
	glcpp_parser_t *parser;

	parser = xtalloc (NULL, glcpp_parser_t);

	glcpp_lex_init_extra (parser, &parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
	parser->active = _string_list_create (parser);
	parser->space_tokens = 1;
	parser->newline_as_space = 0;
	parser->in_control_line = 0;
	parser->paren_count = 0;

	parser->skip_stack = NULL;

	parser->lex_from_list = NULL;
	parser->lex_from_node = NULL;

	return parser;
}

int
glcpp_parser_parse (glcpp_parser_t *parser)
{
	return yyparse (parser);
}

void
glcpp_parser_destroy (glcpp_parser_t *parser)
{
	if (parser->skip_stack)
		fprintf (stderr, "Error: Unterminated #if\n");
	glcpp_lex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	talloc_free (parser);
}

/* Replace any occurences of DEFINED tokens in 'list' with either a
 * '0' or '1' INTEGER token depending on whether the next token in the
 * list is defined or not. */
static void
_glcpp_parser_evaluate_defined (glcpp_parser_t *parser,
				token_list_t *list)
{
	token_node_t *node, *next;
	macro_t *macro;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next) {
		if (node->token->type != DEFINED)
			continue;
		next = node->next;
		while (next && next->token->type == SPACE)
			next = next->next;
		if (next == NULL || next->token->type != IDENTIFIER) {
			fprintf (stderr, "Error: operator \"defined\" requires an identifier\n");
			exit (1);
		}
		macro = hash_table_find (parser->defines,
					 next->token->value.str);

		node->token->type = INTEGER;
		node->token->value.ival = (macro != NULL);
		node->next = next->next;
	}
}
	

/* Appends onto 'expansion' a non-macro token or the expansion of an
 * object-like macro.
 *
 * Returns 0 if this token is completely processed.
 *
 * Returns 1 in the case that 'token' is a function-like macro that
 * needs further expansion.
 */
static int
_expand_token_onto (glcpp_parser_t *parser,
		    token_t *token,
		    token_list_t *result)
{
	const char *identifier;
	macro_t *macro;
	token_list_t *expansion;

	/* We only expand identifiers */
	if (token->type != IDENTIFIER) {
		/* We change any COMMA into a COMMA_FINAL to prevent
		 * it being mistaken for an argument separator
		 * later. */
		if (token->type == ',') {
			token_t *new_token;

			new_token = _token_create_ival (result, COMMA_FINAL,
							COMMA_FINAL);
			_token_list_append (result, new_token);
		} else {
			_token_list_append (result, token);
		}
		return 0;
	}

	/* Look up this identifier in the hash table. */
	identifier = token->value.str;
	macro = hash_table_find (parser->defines, identifier);

	/* Not a macro, so just append. */
	if (macro == NULL) {
		_token_list_append (result, token);
		return 0;
	}

	/* Finally, don't expand this macro if we're already actively
	 * expanding it, (to avoid infinite recursion). */
	if (_string_list_contains (parser->active, identifier, NULL))
	{
		/* We change the token type here from IDENTIFIER to
		 * OTHER to prevent any future expansion of this
		 * unexpanded token. */
		char *str;
		token_t *new_token;

		str = xtalloc_strdup (result, token->value.str);
		new_token = _token_create_str (result, OTHER, str);
		_token_list_append (result, new_token);
		return 0;
	}

	/* For function-like macros return 1 for further processing. */
	if (macro->is_function) {
		return 1;
	}

	_string_list_push (parser->active, identifier);
	_glcpp_parser_expand_token_list_onto (parser,
					      macro->replacements,
					      result);
	_string_list_pop (parser->active);

	return 0;
}

typedef enum function_status
{
	FUNCTION_STATUS_SUCCESS,
	FUNCTION_NOT_A_FUNCTION,
	FUNCTION_UNBALANCED_PARENTHESES
} function_status_t;

/* Find a set of function-like macro arguments by looking for a
 * balanced set of parentheses. Upon return *node will be the last
 * consumed node, such that further processing can continue with
 * node->next.
 *
 * Return values:
 *
 *   FUNCTION_STATUS_SUCCESS:
 *
 *	Successfully parsed a set of function arguments.	
 *
 *   FUNCTION_NOT_A_FUNCTION:
 *
 *	Macro name not followed by a '('. This is not an error, but
 *	simply that the macro name should be treated as a non-macro.
 *
 *   FUNCTION_UNBLANCED_PARENTHESES
 *
 *	Macro name is not followed by a balanced set of parentheses.
 */
static function_status_t
_arguments_parse (argument_list_t *arguments, token_node_t **node_ret)
{
	token_list_t *argument;
	token_node_t *node = *node_ret, *last;
	int paren_count;

	last = node;
	node = node->next;

	/* Ignore whitespace before first parenthesis. */
	while (node && node->token->type == SPACE)
		node = node->next;

	if (node == NULL || node->token->type != '(')
		return FUNCTION_NOT_A_FUNCTION;

	last = node;
	node = node->next;

	argument = _token_list_create (arguments);
	_argument_list_append (arguments, argument);

	for (paren_count = 1; node; last = node, node = node->next) {
		if (node->token->type == '(')
		{
			paren_count++;
		}
		else if (node->token->type == ')')
		{
			paren_count--;
			if (paren_count == 0) {
				last = node;
				node = node->next;
				break;
			}
		}

		if (node->token->type == ',' &&
			 paren_count == 1)
		{
			_token_list_trim_trailing_space (argument);
			argument = _token_list_create (arguments);
			_argument_list_append (arguments, argument);
		}
		else {
			if (argument->head == NULL) {
				/* Don't treat initial whitespace as
				 * part of the arguement. */
				if (node->token->type == SPACE)
					continue;
			}
			_token_list_append (argument, node->token);
		}
	}

	if (node && paren_count)
		return FUNCTION_UNBALANCED_PARENTHESES;

	*node_ret = last;

	return FUNCTION_STATUS_SUCCESS;
}

/* Prints the expansion of *node (consuming further tokens from the
 * list as necessary). Upon return *node will be the last consumed
 * node, such that further processing can continue with node->next. */
static function_status_t
_expand_function_onto (glcpp_parser_t *parser,
		       token_node_t **node_ret,
		       token_list_t *result)
{
	macro_t *macro;
	token_node_t *node;
	const char *identifier;
	argument_list_t *arguments;
	function_status_t status;
	token_list_t *substituted;
	int parameter_index;

	node = *node_ret;
	identifier = node->token->value.str;

	macro = hash_table_find (parser->defines, identifier);

	assert (macro->is_function);

	arguments = _argument_list_create (parser);
	status = _arguments_parse (arguments, node_ret);

	switch (status) {
	case FUNCTION_STATUS_SUCCESS:
		break;
	case FUNCTION_NOT_A_FUNCTION:
		_token_list_append (result, node->token);
		return FUNCTION_NOT_A_FUNCTION;
	case FUNCTION_UNBALANCED_PARENTHESES:
		fprintf (stderr, "Error: Macro %s call has unbalanced parentheses\n",
			 identifier);
		exit (1);
	}

	if (macro->replacements == NULL) {
		talloc_free (arguments);
		return FUNCTION_STATUS_SUCCESS;
	}

	if (! ((_argument_list_length (arguments) == 
		_string_list_length (macro->parameters)) ||
	       (_string_list_length (macro->parameters) == 0 &&
		_argument_list_length (arguments) == 1 &&
		arguments->head->argument->head == NULL)))
	{
		fprintf (stderr,
			 "Error: macro %s invoked with %d arguments (expected %d)\n",
			 identifier,
			 _argument_list_length (arguments),
			 _string_list_length (macro->parameters));
		exit (1);
	}

	/* Perform argument substitution on the replacement list. */
	substituted = _token_list_create (arguments);

	for (node = macro->replacements->head; node; node = node->next)
	{
		if (node->token->type == IDENTIFIER &&
		    _string_list_contains (macro->parameters,
					   node->token->value.str,
					   &parameter_index))
		{
			token_list_t *argument;
			argument = _argument_list_member_at (arguments,
							     parameter_index);
			/* Before substituting, we expand the argument
			 * tokens, or append a placeholder token for
			 * an empty argument. */
			if (argument->head) {
				_glcpp_parser_expand_token_list_onto (parser,
								      argument,
								      substituted);
			} else {
				token_t *new_token;

				new_token = _token_create_ival (substituted,
								PLACEHOLDER,
								PLACEHOLDER);
				_token_list_append (substituted, new_token);
			}
		} else {
			_token_list_append (substituted, node->token);
		}
	}

	/* After argument substitution, and before further expansion
	 * below, implement token pasting. */

	node = substituted->head;
	while (node)
	{
		token_node_t *next_non_space;

		/* Look ahead for a PASTE token, skipping space. */
		next_non_space = node->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL)
			break;

		if (next_non_space->token->type != PASTE) {
			node = next_non_space;
			continue;
		}

		/* Now find the next non-space token after the PASTE. */
		next_non_space = next_non_space->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL) {
			fprintf (stderr, "Error: '##' cannot appear at either end of a macro expansion\n");
			return FUNCTION_STATUS_SUCCESS;
		}

		_token_paste (node->token, next_non_space->token);
		node->next = next_non_space->next;

		node = node->next;
	}

	_string_list_push (parser->active, identifier);
	_glcpp_parser_expand_token_list_onto (parser, substituted, result);
	_string_list_pop (parser->active);

	talloc_free (arguments);

	return FUNCTION_STATUS_SUCCESS;
}

static void
_glcpp_parser_expand_token_list_onto (glcpp_parser_t *parser,
				      token_list_t *list,
				      token_list_t *result)
{
	token_node_t *node;
	token_list_t *intermediate, *list_orig = list;
	int i, need_rescan = 0;

	if (list == NULL || list->head == NULL)
		return;

	intermediate = _token_list_create (parser);

	/* XXX: The two-pass expansion here is really ugly. The
	 * problem this is solving is that we can expand a macro into
	 * a function-like macro name, and then we need to recognize
	 * that as a function-like macro, but perhaps the parentheses
	 * and arguments aren't on the token list yet, (since they are
	 * in the actual content so they are part of what we are
	 * expanding.
	 *
	 * This ugly hack works, but is messy, fragile, and hard to
	 * maintain. I think a cleaner solution would separate the
	 * notions of expanding and appending and avoid this problem
	 * altogether.
	 */

	for (i = 0; i < 2; i++) {
		if (i == 1) {
			list = intermediate;
			intermediate = _token_list_create (parser);
		}
		for (node = list->head; node; node = node->next)
		{
			if (_expand_token_onto (parser, node->token,
						intermediate))
			{	
				if (_expand_function_onto (parser, &node,
							   intermediate))
				{
					need_rescan = 1;
				}
			}
		}
		if (list != list_orig)
			talloc_free (list);
	}

	_token_list_append_list (result, intermediate);
}

void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list)
{
	token_list_t *expanded;
	token_node_t *node;
	function_status_t function_status;

	if (list == NULL)
		return;

	expanded = _token_list_create (parser);

	_glcpp_parser_expand_token_list_onto (parser, list, expanded);

	_token_list_trim_trailing_space (expanded);

	_token_list_print (expanded);

	talloc_free (expanded);
}

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *identifier,
		      token_list_t *replacements)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->identifier = talloc_strdup (macro, identifier);
	macro->replacements = talloc_steal (macro, replacements);

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			string_list_t *parameters,
			token_list_t *replacements)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 1;
	macro->parameters = talloc_steal (macro, parameters);
	macro->identifier = talloc_strdup (macro, identifier);
	macro->replacements = talloc_steal (macro, replacements);

	hash_table_insert (parser->defines, macro, identifier);
}

static int
glcpp_parser_lex (glcpp_parser_t *parser)
{
	token_node_t *node;
	int ret;

	if (parser->lex_from_list == NULL) {
		ret = glcpp_lex (parser->scanner);

		/* XXX: This ugly block of code exists for the sole
		 * purpose of converting a NEWLINE token into a SPACE
		 * token, but only in the case where we have seen a
		 * function-like macro name, but have not yet seen its
		 * closing parenthesis.
		 *
		 * There's perhaps a more compact way to do this with
		 * mid-rule actions in the grammar.
		 *
		 * I'm definitely not pleased with the complexity of
		 * this code here.
		 */
		if (parser->newline_as_space)
		{
			if (ret == '(') {
				parser->paren_count++;
			} else if (ret == ')') {
				parser->paren_count--;
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			} else if (ret == NEWLINE) {
				ret = SPACE;
			} else if (ret != SPACE) {
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			}
		}
		else if (parser->in_control_line)
		{
			if (ret == NEWLINE)
				parser->in_control_line = 0;
		}
		else if (ret == HASH_DEFINE_OBJ || ret == HASH_DEFINE_FUNC ||
			   ret == HASH_UNDEF || ret == HASH_IF ||
			   ret == HASH_IFDEF || ret == HASH_IFNDEF ||
			   ret == HASH_ELIF || ret == HASH_ELSE ||
			   ret == HASH_ENDIF || ret == HASH)
		{
			parser->in_control_line = 1;
		}
		else if (ret == IDENTIFIER)
		{
			macro_t *macro;
			macro = hash_table_find (parser->defines,
						 yylval.str);
			if (macro && macro->is_function) {
				parser->newline_as_space = 1;
				parser->paren_count = 0;
			}
		}

		return ret;
	}

	node = parser->lex_from_node;

	if (node == NULL) {
		talloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
		return NEWLINE;
	}

	yylval = node->token->value;
	ret = node->token->type;

	parser->lex_from_node = node->next;

	return ret;
}

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	assert (parser->lex_from_list == NULL);

	/* Copy list, eliminating any space tokens. */
	parser->lex_from_list = _token_list_create (parser);

	for (node = list->head; node; node = node->next) {
		if (node->token->type == SPACE)
			continue;
		_token_list_append (parser->lex_from_list, node->token);
	}

	talloc_free (list);

	parser->lex_from_node = parser->lex_from_list->head;

	/* It's possible the list consisted of nothing but whitespace. */
	if (parser->lex_from_node == NULL) {
		talloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
	}
}

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, int condition)
{
	skip_type_t current = SKIP_NO_SKIP;
	skip_node_t *node;

	if (parser->skip_stack)
		current = parser->skip_stack->type;

	node = xtalloc (parser, skip_node_t);

	if (current == SKIP_NO_SKIP) {
		if (condition)
			node->type = SKIP_NO_SKIP;
		else
			node->type = SKIP_TO_ELSE;
	} else {
		node->type = SKIP_TO_ENDIF;
	}

	node->next = parser->skip_stack;
	parser->skip_stack = node;
}

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, const char *type,
				    int condition)
{
	if (parser->skip_stack == NULL) {
		fprintf (stderr, "Error: %s without #if\n", type);
		exit (1);
	}

	if (parser->skip_stack->type == SKIP_TO_ELSE) {
		if (condition)
			parser->skip_stack->type = SKIP_NO_SKIP;
	} else {
		parser->skip_stack->type = SKIP_TO_ENDIF;
	}
}

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser)
{
	skip_node_t *node;

	if (parser->skip_stack == NULL) {
		fprintf (stderr, "Error: #endif without #if\n");
		exit (1);
	}

	node = parser->skip_stack;
	parser->skip_stack = node->next;
	talloc_free (node);
}
