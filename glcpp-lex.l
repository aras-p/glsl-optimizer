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
	yyextra->space_tokens = 0;
	return HASH_DEFINE_FUNC;
}

{HASH}define {
	yyextra->space_tokens = 0;
	return HASH_DEFINE_OBJ;
}

{HASH}undef {
	yyextra->space_tokens = 0;
	return HASH_UNDEF;
}

{HASH}if {
	yyextra->space_tokens = 0;
	return HASH_IF;
}

{HASH}elif {
	yyextra->space_tokens = 0;
	return HASH_ELIF;
}

{HASH}else {
	yyextra->space_tokens = 0;
	return HASH_ELSE;
}

{HASH}endif {
	yyextra->space_tokens = 0;
	return HASH_ENDIF;
}

{HASH} {
	yyextra->space_tokens = 0;
	return HASH;
}

{DECIMAL_INTEGER} {
	yylval.ival = strtoll (yytext, NULL, 10);
	return INTEGER;
}

{OCTAL_INTEGER} {
	yylval.ival = strtoll (yytext + 1, NULL, 8);
	return INTEGER;
}

{HEXADECIMAL_INTEGER} {
	yylval.ival = strtoll (yytext + 2, NULL, 16);
	return INTEGER;
}

"<<"  {
	return LEFT_SHIFT;
}

">>" {
	return RIGHT_SHIFT;
}

"<=" {
	return LESS_OR_EQUAL;
}

">=" {
	return GREATER_OR_EQUAL;
}

"==" {
	return EQUAL;
}

"!=" {
	return NOT_EQUAL;
}

"&&" {
	return AND;
}

"||" {
	return OR;
}

"##" {
	return PASTE;
}

"defined" {
	return DEFINED;
}

{IDENTIFIER} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

{PUNCTUATION} {
	return yytext[0];
}

{OTHER}+ {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return OTHER;
}

{HSPACE}+ {
	if (yyextra->space_tokens) {
		return SPACE;
	}
}

\n {
	return NEWLINE;
}

%%
