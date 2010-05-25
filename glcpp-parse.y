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

#include "glcpp.h"

void
yyerror (void *scanner, const char *error);

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *macro,
		      token_list_t *replacements);

void
_define_function_macro (glcpp_parser_t *parser,
			const char *macro,
			string_list_t *parameters,
			token_list_t *replacements);

void
_expand_object_macro (glcpp_parser_t *parser, const char *identifier);

void
_expand_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			argument_list_t *arguments);

string_list_t *
_string_list_create (void *ctx);

void
_string_list_append_item (string_list_t *list, const char *str);

void
_string_list_append_list (string_list_t *list, string_list_t *tail);

int
_string_list_contains (string_list_t *list, const char *member, int *index);

int
_string_list_length (string_list_t *list);

argument_list_t *
_argument_list_create (void *ctx);

void
_argument_list_append (argument_list_t *list, token_list_t *argument);

int
_argument_list_length (argument_list_t *list);

token_list_t *
_argument_list_member_at (argument_list_t *list, int index);

/* Note: This function talloc_steal()s the str pointer. */
token_t *
_token_create_str (void *ctx, int type, char *str);

token_t *
_token_create_ival (void *ctx, int type, int ival);

token_list_t *
_token_list_create (void *ctx);

/* Note: This function add a talloc_reference() to token.
 *
 * You may want to talloc_unlink any current reference if you no
 * longer need it. */
void
_token_list_append (token_list_t *list, token_t *token);

void
_token_list_append_list (token_list_t *list, token_list_t *tail);

void
_token_list_print (token_list_t *list);

static void
glcpp_parser_pop_expansion (glcpp_parser_t *parser);

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

%}

%union {
	int ival;
	char *str;
	token_t *token;
	token_list_t *token_list;
}

%parse-param {glcpp_parser_t *parser}
%lex-param {glcpp_parser_t *parser}

%token HASH_DEFINE_FUNC HASH_DEFINE_OBJ HASH IDENTIFIER NEWLINE OTHER HASH_UNDEF
%token LEFT_SHIFT RIGHT_SHIFT LESS_OR_EQUAL GREATER_OR_EQUAL EQUAL NOT_EQUAL AND OR PASTE
%type <ival> punctuator
%type <str> IDENTIFIER OTHER
%type <token> preprocessing_token
%type <token_list> pp_tokens replacement_list text_line

	/* Stale stuff just to allow code to compile. */
%token IDENTIFIER_FINALIZED FUNC_MACRO OBJ_MACRO

%%

input:
	/* empty */
|	input line
;

line:
	control_line
|	text_line {
		_token_list_print ($1);
		printf ("\n");
		talloc_free ($1);
	}
|	HASH non_directive
;

control_line:
	HASH_DEFINE_OBJ IDENTIFIER replacement_list NEWLINE
|	HASH_DEFINE_FUNC IDENTIFIER '(' ')' replacement_list NEWLINE
|	HASH_DEFINE_FUNC IDENTIFIER '(' identifier_list ')' replacement_list NEWLINE
|	HASH_UNDEF IDENTIFIER NEWLINE
|	HASH NEWLINE
;

identifier_list:
	IDENTIFIER
|	identifier_list ',' IDENTIFIER
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
|	punctuator {
		$$ = _token_create_ival (parser, $1, $1);
	}
|	OTHER {
		$$ = _token_create_str (parser, OTHER, $1);
	}
;

punctuator:
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

	if (argument == NULL || argument->head == NULL)
		return;

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

void
_token_print (token_t *token)
{
	if (token->type < 256) {
		printf ("%c", token->type);
		return;
	}

	switch (token->type) {
	case IDENTIFIER:
	case OTHER:
		printf ("%s", token->value.str);
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
	default:
		fprintf (stderr, "Error: Don't know how to print token type %d\n", token->type);
		break;
	}
}

token_list_t *
_token_list_create (void *ctx)
{
	token_list_t *list;

	list = xtalloc (ctx, token_list_t);
	list->head = NULL;
	list->tail = NULL;

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
}

void
_token_list_append_list (token_list_t *list, token_list_t *tail)
{
	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
}

void
_token_list_print (token_list_t *list)
{
	token_node_t *node;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next) {
		_token_print (node->token);
	}
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
	parser->expansions = NULL;

	parser->just_printed_separator = 1;
	parser->need_newline = 0;

	parser->skip_stack = NULL;

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
	if (parser->need_newline)
		printf ("\n");
	if (parser->skip_stack)
		fprintf (stderr, "Error: Unterminated #if\n");
	glcpp_lex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	talloc_free (parser);
}

static int
glcpp_parser_is_expanding (glcpp_parser_t *parser, const char *member)
{
	expansion_node_t *node;

	for (node = parser->expansions; node; node = node->next) {
		if (node->macro &&
		    strcmp (node->macro->identifier, member) == 0)
		{
			return 1;
		}
	}

	return 0;
}

token_class_t
glcpp_parser_classify_token (glcpp_parser_t *parser,
			     const char *identifier,
			     int *parameter_index)
{
	macro_t *macro;

	/* Is this token a defined macro? */
	macro = hash_table_find (parser->defines, identifier);

	if (macro == NULL)
		return TOKEN_CLASS_IDENTIFIER;

	/* Don't consider this a macro if we are already actively
	 * expanding this macro. */
	if (glcpp_parser_is_expanding (parser, identifier))
		return TOKEN_CLASS_IDENTIFIER_FINALIZED;

	/* Definitely a macro. Just need to check if it's function-like. */
	if (macro->is_function)
		return TOKEN_CLASS_FUNC_MACRO;
	else
		return TOKEN_CLASS_OBJ_MACRO;
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

static void
_glcpp_parser_push_expansion (glcpp_parser_t *parser,
			      macro_t *macro,
			      token_node_t *replacements)
{
	expansion_node_t *node;

	node = xtalloc (parser, expansion_node_t);

	node->macro = macro;
	node->replacements = replacements;

	node->next = parser->expansions;
	parser->expansions = node;
}

static void
glcpp_parser_pop_expansion (glcpp_parser_t *parser)
{
	expansion_node_t *node;

	node = parser->expansions;

	if (node == NULL) {
		fprintf (stderr, "Internal error: _expansion_list_pop called on an empty list.\n");
		exit (1);
	}

	parser->expansions = node->next;

	talloc_free (node);
}

void
_expand_object_macro (glcpp_parser_t *parser, const char *identifier)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (! macro->is_function);
	assert (! glcpp_parser_is_expanding (parser, identifier));

	_glcpp_parser_push_expansion (parser, macro, macro->replacements->head);
}

void
_expand_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			argument_list_t *arguments)
{
	macro_t *macro;
	token_list_t *expanded;
	token_node_t *i, *j;
	int parameter_index;

	macro = hash_table_find (parser->defines, identifier);
	assert (macro->is_function);
	assert (! glcpp_parser_is_expanding (parser, identifier));

	if (_argument_list_length (arguments) !=
	    _string_list_length (macro->parameters))
	{
		fprintf (stderr,
			 "Error: macro %s invoked with %d arguments (expected %d)\n",
			 identifier,
			 _argument_list_length (arguments),
			 _string_list_length (macro->parameters));
		return;
	}

	expanded = _token_list_create (macro);

	for (i = macro->replacements->head; i; i = i->next) {
		if (_string_list_contains (macro->parameters,
					   i->token->value.str,
					   &parameter_index))
		{
			token_list_t *argument;
			argument = _argument_list_member_at (arguments,
							     parameter_index);
			for (j = argument->head; j; j = j->next)
			{
				_token_list_append (expanded, j->token);
			}
		} else {
			_token_list_append (expanded, i->token);
		}
	}

	_glcpp_parser_push_expansion (parser, macro, expanded->head);
}

static int
glcpp_parser_lex (glcpp_parser_t *parser)
{
	expansion_node_t *expansion;
	token_node_t *replacements;
	int parameter_index;
	const char *token;
	token_class_t class;

    /* Who says C can't do efficient tail recursion? */
    RECURSE:

	expansion = parser->expansions;

	if (expansion == NULL)
		return glcpp_lex (parser->scanner);

	replacements = expansion->replacements;

	/* Pop expansion when replacements is exhausted. */
	if (replacements == NULL) {
		glcpp_parser_pop_expansion (parser);
		goto RECURSE;
	}

	expansion->replacements = replacements->next;

	token = replacements->token->value.str;

	/* Implement token pasting. */
	if (replacements->next && strcmp (replacements->next->token->value.str, "##") == 0) {
		token_node_t *next_node;

		next_node = replacements->next->next;

		if (next_node == NULL) {
			fprintf (stderr, "Error: '##' cannot appear at the end of a macro expansion.\n");
			exit (1);
		}

		token = xtalloc_asprintf (parser, "%s%s",
					  token, next_node->token->value.str);
		expansion->replacements = next_node->next;
	}


	if (strcmp (token, "(") == 0)
		return '(';
	else if (strcmp (token, ")") == 0)
		return ')';

	yylval.str = xtalloc_strdup (parser, token);

	/* Carefully refuse to expand any finalized identifier. */
	if (replacements->token->type == IDENTIFIER_FINALIZED)
		return IDENTIFIER_FINALIZED;

	switch (glcpp_parser_classify_token (parser, yylval.str,
					     &parameter_index))
	{
	case TOKEN_CLASS_IDENTIFIER:
		return IDENTIFIER;
		break;
	case TOKEN_CLASS_IDENTIFIER_FINALIZED:
		return IDENTIFIER_FINALIZED;
		break;
	case TOKEN_CLASS_FUNC_MACRO:
		return FUNC_MACRO;
		break;
	default:
	case TOKEN_CLASS_OBJ_MACRO:
		return OBJ_MACRO;
		break;
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
