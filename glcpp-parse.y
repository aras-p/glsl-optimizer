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
#include <talloc.h>

#include "glcpp.h"

#define YYLEX_PARAM parser->scanner

struct glcpp_parser {
	yyscan_t scanner;
	struct hash_table *defines;
};

void
yyerror (void *scanner, const char *error);

void
_print_resolved_token (glcpp_parser_t *parser, const char *token);

list_t *
_list_create (void *ctx);

void
_list_append (list_t *list, const char *str);

%}

%union {
	char *str;
	list_t *list;
}

%parse-param {glcpp_parser_t *parser}
%lex-param {void *scanner}

%token DEFINE IDENTIFIER NEWLINE TOKEN
%type <str> token IDENTIFIER TOKEN
%type <list> replacement_list

%%

input:
	/* empty */
|	content
;

content:
	token {
		_print_resolved_token (parser, $1);
		free ($1);
	}
|	directive
|	content token {
		_print_resolved_token (parser, $2);
		free ($2);
	}
|	content directive
;

directive:
	DEFINE IDENTIFIER replacement_list NEWLINE {
		char *key = talloc_strdup ($3, $2);
		free ($2);
		hash_table_insert (parser->defines, $3, key);
		printf ("\n");
	}
;

replacement_list:
	/* empty */ {
		$$ = _list_create (parser);
	}

|	replacement_list token {
		_list_append ($1, $2);
		free ($2);
		$$ = $1;
	}
;

token:
	TOKEN { $$ = $1; }
|	IDENTIFIER { $$ = $1; }
;

%%

list_t *
_list_create (void *ctx)
{
	list_t *list;

	list = talloc (ctx, list_t);
	if (list == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_list_append (list_t *list, const char *str)
{
	node_t *node;

	node = talloc (list, node_t);
	if (node == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	node->str = talloc_strdup (node, str);
	if (node->str == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}
		
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

	parser = talloc (NULL, glcpp_parser_t);
	if (parser == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	yylex_init (&parser->scanner);
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

static void
_print_resolved_recursive (glcpp_parser_t *parser,
			   const char *token,
			   const char *orig,
			   int *first)
{
	list_t *replacement;
	node_t *node;

	replacement = hash_table_find (parser->defines, token);
	if (replacement == NULL) {
		printf ("%s%s", *first ? "" : " ", token);
		*first = 0;
	} else {
		for (node = replacement->head ; node ; node = node->next) {
			token = node->str;
			if (strcmp (token, orig) == 0) {
				printf ("%s%s", *first ? "" : " ", token);
				*first = 0;
			} else {
				_print_resolved_recursive (parser, token, orig, first);
			}
		}
	}
}

void
_print_resolved_token (glcpp_parser_t *parser, const char *token)
{
	int first = 1;

	_print_resolved_recursive (parser, token, token, &first);
}
