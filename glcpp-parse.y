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
#include <talloc.h>

#include "glcpp.h"

#define YYLEX_PARAM parser->scanner

typedef struct {
	int is_function;
	string_list_t *parameters;
	string_list_t *replacements;
} macro_t;

struct glcpp_parser {
	yyscan_t scanner;
	struct hash_table *defines;
};

void
yyerror (void *scanner, const char *error);

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *macro,
		      string_list_t *replacements);

void
_define_function_macro (glcpp_parser_t *parser,
			const char *macro,
			string_list_t *parameters,
			string_list_t *replacements);

void
_print_expanded_object_macro (glcpp_parser_t *parser, const char *macro);

void
_print_expanded_function_macro (glcpp_parser_t *parser,
				const char *macro,
				string_list_t *arguments);

string_list_t *
_string_list_create (void *ctx);

void
_string_list_append_item (string_list_t *list, const char *str);

void
_string_list_append_list (string_list_t *list, string_list_t *tail);

int
_string_list_contains (string_list_t *list, const char *member, int *index);

const char *
_string_list_member_at (string_list_t *list, int index);

int
_string_list_length (string_list_t *list);

%}

%union {
	char *str;
	string_list_t *list;
}

%parse-param {glcpp_parser_t *parser}
%lex-param {void *scanner}

%token DEFINE FUNC_MACRO IDENTIFIER NEWLINE OBJ_MACRO SPACE TOKEN UNDEF
%type <str> FUNC_MACRO IDENTIFIER identifier_perhaps_macro OBJ_MACRO TOKEN word word_or_symbol
%type <list> argument argument_list parameter_list replacement_list

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
|	macro
|	directive_with_newline { printf ("\n"); }
|	NEWLINE	{ printf ("\n"); }
|	'('	{ printf ("("); }
|	')'	{ printf (")"); }
|	','	{ printf (","); }
|	SPACE	{ printf (" "); }
;

macro:
	FUNC_MACRO '(' argument_list ')' {
		_print_expanded_function_macro (parser, $1, $3);
	}
|	OBJ_MACRO {
		_print_expanded_object_macro (parser, $1);
		talloc_free ($1);
	}
;

argument_list:
	argument {
		$$ = _string_list_create (parser);
		_string_list_append_list ($$, $1);
	}
|	argument_list ',' argument {
		_string_list_append_list ($1, $3);
		$$ = $1;
	}
;

argument:
	/* empty */ {
		$$ = _string_list_create (parser);
	}
|	argument word {
		_string_list_append_item ($1, $2);
		talloc_free ($2);
	}
|	argument '(' argument ')'
;

directive_with_newline:
	directive NEWLINE
;

directive:
	DEFINE IDENTIFIER {
		string_list_t *list = _string_list_create (parser);
		_define_object_macro (parser, $2, list);
	}
|	DEFINE IDENTIFIER SPACE replacement_list {
		_define_object_macro (parser, $2, $4);
	}
|	DEFINE IDENTIFIER '(' parameter_list ')' {
		string_list_t *list = _string_list_create (parser);
		_define_function_macro (parser, $2, $4, list);
	}
|	DEFINE IDENTIFIER '(' parameter_list ')' SPACE replacement_list {
		_define_function_macro (parser, $2, $4, $7);
	}
|	UNDEF FUNC_MACRO {
		string_list_t *replacement = hash_table_find (parser->defines, $2);
		if (replacement) {
			/* XXX: Need hash table to support a real way
			 * to remove an element rather than prefixing
			 * a new node with data of NULL like this. */
			hash_table_insert (parser->defines, NULL, $2);
			talloc_free (replacement);
		}
		talloc_free ($2);
	}
|	UNDEF OBJ_MACRO {
		string_list_t *replacement = hash_table_find (parser->defines, $2);
		if (replacement) {
			/* XXX: Need hash table to support a real way
			 * to remove an element rather than prefixing
			 * a new node with data of NULL like this. */
			hash_table_insert (parser->defines, NULL, $2);
			talloc_free (replacement);
		}
		talloc_free ($2);
	}
;

replacement_list:
	word_or_symbol {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
		talloc_free ($1);
	}
|	replacement_list word_or_symbol {
		_string_list_append_item ($1, $2);
		talloc_free ($2);
		$$ = $1;
	}
;

parameter_list:
	/* empty */ {
		$$ = _string_list_create (parser);
	}
|	identifier_perhaps_macro {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
		talloc_free ($1);
	}
|	parameter_list ',' identifier_perhaps_macro {
		_string_list_append_item ($1, $3);
		talloc_free ($3);
		$$ = $1;
	}
;

identifier_perhaps_macro:
	IDENTIFIER { $$ = $1; }
|	FUNC_MACRO { $$ = $1; }
|	OBJ_MACRO { $$ = $1; }
;

word_or_symbol:
	word	{ $$ = $1; }
|	'('	{ $$ = xtalloc_strdup (parser, "("); }
|	')'	{ $$ = xtalloc_strdup (parser, ")"); }
|	','	{ $$ = xtalloc_strdup (parser, ","); }
|	SPACE	{ $$ = xtalloc_strdup (parser, " "); }
;

word:
	IDENTIFIER { $$ = $1; }
|	FUNC_MACRO { $$ = $1; }
|	OBJ_MACRO { $$ = $1; }
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

const char *
_string_list_member_at (string_list_t *list, int index)
{
	string_node_t *node;
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
		return node->str;

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

macro_type_t
glcpp_parser_macro_type (glcpp_parser_t *parser, const char *identifier)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);

	if (macro == NULL)
		return MACRO_TYPE_UNDEFINED;

	if (macro->is_function)
		return MACRO_TYPE_FUNCTION;
	else
		return MACRO_TYPE_OBJECT;
}

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *identifier,
		      string_list_t *replacements)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->replacements = talloc_steal (macro, replacements);

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			string_list_t *parameters,
			string_list_t *replacements)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 1;
	macro->parameters = talloc_steal (macro, parameters);
	macro->replacements = talloc_steal (macro, replacements);

	hash_table_insert (parser->defines, macro, identifier);
}

static void
_print_expanded_macro_recursive (glcpp_parser_t *parser,
				 const char *token,
				 const char *orig,
				 string_list_t *parameters,
				 string_list_t *arguments);

static void
_print_expanded_string_list_recursive (glcpp_parser_t *parser,
				string_list_t *list,
				const char *orig,
				string_list_t *parameters,
				string_list_t *arguments)
{
	const char *token;
	string_node_t *node;
	int index;

	for (node = list->head ; node ; node = node->next) {
		token = node->str;

		if (strcmp (token, orig) == 0) {
			printf ("%s", token);
			continue;
		}

		if (_string_list_contains (parameters, token, &index)) {
			const char *argument;

			argument = _string_list_member_at (arguments, index);
			_print_expanded_macro_recursive (parser, argument,
							 orig, parameters,
							 arguments);
		} else {
			_print_expanded_macro_recursive (parser, token,
							 orig, parameters,
							 arguments);
		}
	}
}


static void
_print_expanded_macro_recursive (glcpp_parser_t *parser,
				 const char *token,
				 const char *orig,
				 string_list_t *parameters,
				 string_list_t *arguments)
{
	macro_t *macro;
	string_list_t *replacements;

	macro = hash_table_find (parser->defines, token);
	if (macro == NULL) {
		printf ("%s", token);
		return;
	}

	replacements = macro->replacements;

	_print_expanded_string_list_recursive (parser, replacements,
					       orig, parameters, arguments);
}

void
_print_expanded_object_macro (glcpp_parser_t *parser, const char *identifier)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (! macro->is_function);

	_print_expanded_macro_recursive (parser, identifier, identifier,
					 NULL, NULL);
}

void
_print_expanded_function_macro (glcpp_parser_t *parser,
				const char *identifier,
				string_list_t *arguments)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (macro->is_function);

	if (_string_list_length (arguments) != _string_list_length (macro->parameters)) {
		fprintf (stderr,
			 "Error: macro %s invoked with %d arguments (expected %d)\n",
			 identifier,
			 _string_list_length (arguments),
			 _string_list_length (macro->parameters));
		return;
	}

	_print_expanded_macro_recursive (parser, identifier, identifier,
					 macro->parameters, arguments);
}
