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

#include "glcpp.h"

#define YYLEX_PARAM parser->scanner

void
yyerror (void *scanner, const char *error);

%}

%parse-param {glcpp_parser_t *parser}
%lex-param {void *scanner}

%token DEFINE
%token DEFVAL
%token IDENTIFIER
%token TOKEN

%%

input:		/* empty */
	|	content
;

content:	token
	|	directive
	|	content token
	|	content directive
;

directive:	DEFINE IDENTIFIER DEFVAL {
	hash_table_insert (parser->defines, $3, $2);
}
;

token:		TOKEN {
	char *value = hash_table_find (parser->defines, $1);
	if (value)
		printf ("%s", value);
	else
		printf ("%s", $1);
	free ($1);
}
;

%%

void
yyerror (void *scanner, const char *error)
{
	fprintf (stderr, "Parse error: %s\n", error);
}

void
glcpp_parser_init (glcpp_parser_t *parser)
{
	yylex_init (&parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
}

int
glcpp_parser_parse (glcpp_parser_t *parser)
{
	return yyparse (parser);
}

void
glcpp_parser_fini (glcpp_parser_t *parser)
{
	yylex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
}
