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
	list_t *parameter_list;
	list_t *replacement_list;
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
		      list_t *replacement_list);

void
_define_function_macro (glcpp_parser_t *parser,
			const char *macro,
			list_t *parameter_list,
			list_t *replacement_list);

void
_print_expanded_object_macro (glcpp_parser_t *parser, const char *macro);

void
_print_expanded_function_macro (glcpp_parser_t *parser,
				const char *macro,
				list_t *arguments);

list_t *
_list_create (void *ctx);

void
_list_append_item (list_t *list, const char *str);

void
_list_append_list (list_t *list, list_t *tail);

%}

%union {
	char *str;
	list_t *list;
}

%parse-param {glcpp_parser_t *parser}
%lex-param {void *scanner}

%token DEFINE FUNC_MACRO IDENTIFIER NEWLINE OBJ_MACRO TOKEN UNDEF
%type <str> FUNC_MACRO IDENTIFIER OBJ_MACRO TOKEN string
%type <list> argument argument_list parameter_list replacement_list

%%

input:
	/* empty */
|	input content
;

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
|	directive_with_newline
|	NEWLINE {
		printf ("\n");
	}
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
	/* empty */ {
		$$ = _list_create (parser);
	}
|	argument {
		$$ = _list_create (parser);
		_list_append_list ($$, $1);
	}
|	argument_list ',' argument {
		_list_append_list ($1, $3);
		$$ = $1;
	}
;

argument:
	/* empty */ {
		$$ = _list_create (parser);
	}
|	argument string {
		_list_append_item ($1, $2);
		talloc_free ($2);
	}
|	argument '(' argument ')'
;

directive_with_newline:
	directive NEWLINE {
		printf ("\n");
	}
;

directive:
	DEFINE IDENTIFIER replacement_list {
		_define_object_macro (parser, $2, $3);
	}
|	DEFINE IDENTIFIER '(' parameter_list ')' replacement_list {
		_define_function_macro (parser, $2, $4, $6);
	}
|	UNDEF FUNC_MACRO {
		list_t *replacement = hash_table_find (parser->defines, $2);
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
		list_t *replacement = hash_table_find (parser->defines, $2);
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
	/* empty */ {
		$$ = _list_create (parser);
	}
|	replacement_list string {
		_list_append_item ($1, $2);
		talloc_free ($2);
		$$ = $1;
	}
;

parameter_list:
	/* empty */ {
		$$ = _list_create (parser);
	}
|	IDENTIFIER {
		$$ = _list_create (parser);
		_list_append_item ($$, $1);
		talloc_free ($1);
	}
|	parameter_list ',' IDENTIFIER {
		_list_append_item ($1, $3);
		talloc_free ($3);
		$$ = $1;
	}
;

string:
	IDENTIFIER { $$ = $1; }
|	FUNC_MACRO { $$ = $1; }
|	OBJ_MACRO { $$ = $1; }
|	TOKEN { $$ = $1; }
;

%%

list_t *
_list_create (void *ctx)
{
	list_t *list;

	list = xtalloc (ctx, list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_list_append_list (list_t *list, list_t *tail)
{
	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
}

void
_list_append_item (list_t *list, const char *str)
{
	node_t *node;

	node = xtalloc (list, node_t);
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

static void
_print_expanded_macro_recursive (glcpp_parser_t *parser,
				 const char *token,
				 const char *orig,
				 int *first)
{
	macro_t *macro;
	node_t *node;

	macro = hash_table_find (parser->defines, token);
	if (macro == NULL) {
		printf ("%s%s", *first ? "" : " ", token);
		*first = 0;
	} else {
		list_t *replacement_list = macro->replacement_list;

		for (node = replacement_list->head ; node ; node = node->next) {
			token = node->str;
			if (strcmp (token, orig) == 0) {
				printf ("%s%s", *first ? "" : " ", token);
				*first = 0;
			} else {
				_print_expanded_macro_recursive (parser,
								 token, orig,
								 first);
			}
		}
	}
}

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *identifier,
		      list_t *replacement_list)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameter_list = NULL;
	macro->replacement_list = talloc_steal (macro, replacement_list);

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			list_t *parameter_list,
			list_t *replacement_list)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 1;
	macro->parameter_list = talloc_steal (macro, parameter_list);
	macro->replacement_list = talloc_steal (macro, replacement_list);

	hash_table_insert (parser->defines, macro, identifier);
}

void
_print_expanded_object_macro (glcpp_parser_t *parser, const char *identifier)
{
	int first = 1;
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (! macro->is_function);

	_print_expanded_macro_recursive (parser, identifier, identifier, &first);
}

void
_print_expanded_function_macro (glcpp_parser_t *parser,
				const char *identifier,
				list_t *arguments)
{
	int first = 1;
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (macro->is_function);

	/* XXX: Need to use argument list here in the expansion. */

	_print_expanded_macro_recursive (parser, identifier, identifier, &first);
}
