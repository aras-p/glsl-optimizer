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
#include <string.h>

#include "glcpp.h"
#include "glcpp-parse.h"
%}

%option reentrant noyywrap
%option extra-type="glcpp_parser_t *"

	/* This lexer has two states:
	 *
	 * The CONTROL state is for control lines (directives)
	 * It lexes exactly as specified in the C99 specification.
	 *
	 * The INITIAL state is for input lines. In this state, we
	 * make the OTHER token much more broad in that it now
	 * includes tokens consisting entirely of whitespace. This
	 * allows us to pass text through verbatim. It avoids the
	 * "inadvertent token pasting" problem that would occur if we
	 * just printed tokens, while also avoiding excess whitespace
	 * insertion in the output.*/

%x CONTROL

SPACE		[[:space:]]
NONSPACE	[^[:space:]]
NEWLINE		[\n]
HSPACE		[ \t]
HASH		^{HSPACE}*#{HSPACE}*
IDENTIFIER	[_a-zA-Z][_a-zA-Z0-9]*
PUNCTUATION	[][(){}.&*~!/%<>^|;,+-]
OTHER		[^][(){}.&*~!/%<>^|;,=#[:space:]+-]+

DECIMAL_INTEGER		[1-9][0-9]*[uU]?
OCTAL_INTEGER		0[0-7]*[uU]?
HEXADECIMAL_INTEGER	0[xX][0-9a-fA-F]+[uU]?

%%

{HASH}define{HSPACE}+/{IDENTIFIER}"(" {
	BEGIN CONTROL;
	return HASH_DEFINE_FUNC;
}

{HASH}define {
	BEGIN CONTROL;
	return HASH_DEFINE_OBJ;
}

{HASH}undef {
	BEGIN CONTROL;
	return HASH_UNDEF;
}

{HASH} {
	BEGIN CONTROL;
	return HASH;
}

<CONTROL>{IDENTIFIER} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

<CONTROL>"<<"  {
	return LEFT_SHIFT;
}

<CONTROL>">>" {
	return RIGHT_SHIFT;
}

<CONTROL>"<=" {
	return LESS_OR_EQUAL;
}

<CONTROL>">=" {
	return GREATER_OR_EQUAL;
}

<CONTROL>"==" {
	return EQUAL;
}

<CONTROL>"!=" {
	return NOT_EQUAL;
}

<CONTROL>"&&" {
	return AND;
}

<CONTROL>"||" {
	return OR;
}

<CONTROL>"##" {
	return PASTE;
}

<CONTROL>{PUNCTUATION} {
	return yytext[0];
}

<CONTROL>{OTHER} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return OTHER;
}

<CONTROL>{HSPACE}+

<CONTROL>\n {
	BEGIN INITIAL;
	return NEWLINE;
}

{IDENTIFIER} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

{OTHER}+ {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return OTHER;
}

{HSPACE}+ {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return OTHER;
}

\n {
	return NEWLINE;
}

. {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return OTHER;
}

%%
