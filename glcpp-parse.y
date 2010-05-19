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

#define YYLEX_PARAM parser->scanner

void
yyerror (void *scanner, const char *error);

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *macro,
		      const char *replacement);

void
_define_function_macro (glcpp_parser_t *parser,
			const char *macro,
			string_list_t *parameters,
			const char *replacement);

void
_expand_object_macro (glcpp_parser_t *parser, const char *identifier);

void
_expand_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			argument_list_t *arguments);

void
_print_string_list (string_list_t *list);

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
_argument_list_append (argument_list_t *list, string_list_t *argument);

int
_argument_list_length (argument_list_t *list);

string_list_t *
_argument_list_member_at (argument_list_t *list, int index);

%}

%union {
	char *str;
	string_list_t *string_list;
	argument_list_t *argument_list;
}

%parse-param {glcpp_parser_t *parser}
%lex-param {void *scanner}

%token DEFINE FUNC_MACRO IDENTIFIER NEWLINE OBJ_MACRO REPLACEMENT TOKEN UNDEF
%type <str> FUNC_MACRO IDENTIFIER OBJ_MACRO REPLACEMENT TOKEN word
%type <string_list> argument macro parameter_list
%type <argument_list> argument_list

/* Hard to remove shift/reduce conflicts documented as follows:
 *
 * 1. '(' after FUNC_MACRO name which is correctly resolved to shift
 *    to form macro invocation rather than reducing directly to
 *    content.
 */
%expect 1

%%

input:
	/* empty */
|	input content
;

	/* We do all printing at the content level */
content:
	IDENTIFIER {
		printf ("%s", $1);
		talloc_free ($1);
	}
|	TOKEN {
		printf ("%s", $1);
		talloc_free ($1);
	}
|	FUNC_MACRO {
		printf ("%s", $1);
		talloc_free ($1);
	}
|	directive {
		printf ("\n");
	}
|	'('	{ printf ("("); }
|	')'	{ printf (")"); }
|	','	{ printf (","); }
|	macro
;

macro:
	FUNC_MACRO '(' argument_list ')' {
		_expand_function_macro (parser, $1, $3);
	}
|	OBJ_MACRO {
		_expand_object_macro (parser, $1);
		talloc_free ($1);
	}
;

argument_list:
	/* empty */ {
		$$ = _argument_list_create (parser);
	}
|	argument {
		$$ = _argument_list_create (parser);
		_argument_list_append ($$, $1);
	}
|	argument_list ',' argument {
		_argument_list_append ($1, $3);
		$$ = $1;
	}
;

argument:
	word {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
	}
|	macro {
		$$ = _string_list_create (parser);
	}
|	argument word {
		_string_list_append_item ($1, $2);
		talloc_free ($2);
		$$ = $1;
	}
|	argument '(' argument ')' {
		_string_list_append_item ($1, "(");
		_string_list_append_list ($1, $3);
		_string_list_append_item ($1, ")");
		$$ = $1;
	}
;

directive:
	DEFINE IDENTIFIER REPLACEMENT {
		_define_object_macro (parser, $2, $3);
	}
|	DEFINE IDENTIFIER '(' parameter_list ')' REPLACEMENT {
		_define_function_macro (parser, $2, $4, $6);
	}
|	UNDEF IDENTIFIER {
		string_list_t *macro = hash_table_find (parser->defines, $2);
		if (macro) {
			/* XXX: Need hash table to support a real way
			 * to remove an element rather than prefixing
			 * a new node with data of NULL like this. */
			hash_table_insert (parser->defines, NULL, $2);
			talloc_free (macro);
		}
		talloc_free ($2);
	}
;

parameter_list:
	/* empty */ {
		$$ = _string_list_create (parser);
	}
|	IDENTIFIER {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
		talloc_free ($1);
	}
|	parameter_list ',' IDENTIFIER {
		_string_list_append_item ($1, $3);
		talloc_free ($3);
		$$ = $1;
	}
;

word:
	IDENTIFIER { $$ = $1; }
|	TOKEN { $$ = $1; }
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

void
_print_string_list (string_list_t *list)
{
	string_node_t *node;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next) {
		printf ("%s", node->str);
		if (node->next)
			printf (" ");
	}
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
_argument_list_append (argument_list_t *list, string_list_t *argument)
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

string_list_t *
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

	yylex_init_extra (parser, &parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
	parser->expansions = NULL;

	parser->lex_stack = xtalloc (parser, glcpp_lex_stack_t);
	parser->lex_stack->parser = parser;
	parser->lex_stack->head = NULL;

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
	yylex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	talloc_free (parser);
}

token_class_t
glcpp_parser_classify_token (glcpp_parser_t *parser,
			     const char *identifier,
			     int *parameter_index)
{
	macro_t *macro;

	/* First we check if we are currently expanding a
	 * function-like macro, and if so, whether the parameter list
	 * contains a parameter matching this token name. */
	if (parser->expansions &&
	    parser->expansions->macro &&
	    parser->expansions->macro->parameters)
	{
		string_list_t *list;

		list = parser->expansions->macro->parameters;

		if (_string_list_contains (list, identifier, parameter_index))
		    return TOKEN_CLASS_ARGUMENT;
	}

	/* If not a function-like macro parameter, we next check if
	 * this token is a macro itself. */

	macro = hash_table_find (parser->defines, identifier);

	if (macro == NULL)
		return TOKEN_CLASS_IDENTIFIER;

	if (macro->is_function)
		return TOKEN_CLASS_FUNC_MACRO;
	else
		return TOKEN_CLASS_OBJ_MACRO;
}

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *identifier,
		      const char *replacement)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->identifier = talloc_strdup (macro, identifier);
	macro->replacement = talloc_steal (macro, replacement);

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			string_list_t *parameters,
			const char *replacement)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 1;
	macro->parameters = talloc_steal (macro, parameters);
	macro->identifier = talloc_strdup (macro, identifier);
	macro->replacement = talloc_steal (macro, replacement);

	hash_table_insert (parser->defines, macro, identifier);
}

static void
_glcpp_parser_push_expansion_internal (glcpp_parser_t *parser,
				       macro_t *macro,
				       argument_list_t *arguments,
				       const char * replacement)
{
	expansion_node_t *node;

	node = xtalloc (parser, expansion_node_t);

	node->macro = macro;
	node->arguments = arguments;

	node->next = parser->expansions;
	parser->expansions = node;
		
	glcpp_lex_stack_push (parser->lex_stack, replacement);
}

void
glcpp_parser_push_expansion_macro (glcpp_parser_t *parser,
				   macro_t *macro,
				   argument_list_t *arguments)
{
	_glcpp_parser_push_expansion_internal (parser, macro, arguments,
					       macro->replacement);
}

void
glcpp_parser_push_expansion_argument (glcpp_parser_t *parser,
				      int argument_index)
{
	argument_list_t *arguments;
	string_list_t *argument;
	string_node_t *node;
	char *argument_str, *s;
	int length;

	arguments = parser->expansions->arguments;

	argument = _argument_list_member_at (arguments, argument_index);

	length = 0;
	for (node = argument->head; node; node = node->next)
		length += strlen (node->str) + 1;

	argument_str = xtalloc_size (parser, length);

	*argument_str = '\0';
	s = argument_str;
	for (node = argument->head; node; node = node->next) {
		strcpy (s, node->str);
		s += strlen (node->str);
		if (node->next) {
			*s = ' ';
			s++;
			*s = '\0';
		}
	}

	_glcpp_parser_push_expansion_internal (parser, NULL, NULL,
					       argument_str);
}

/* The lexer calls this when it exhausts a string. */
void
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

int
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

static void
_expand_macro (glcpp_parser_t *parser,
	       const char *token,
	       macro_t *macro,
	       argument_list_t *arguments)
{
	/* Don't recurse if we're already actively expanding this token. */
	if (glcpp_parser_is_expanding (parser, token)) {
		printf ("%s", token);
		return;
	}

	glcpp_parser_push_expansion_macro (parser, macro, arguments);
}

void
_expand_object_macro (glcpp_parser_t *parser, const char *identifier)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (! macro->is_function);

	_expand_macro (parser, identifier, macro, NULL);
}

void
_expand_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			argument_list_t *arguments)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (macro->is_function);

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

	_expand_macro (parser, identifier, macro, arguments);
}
