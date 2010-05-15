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

%x ST_DEFINE
%x ST_DEFVAL

SPACE		[[:space:]]
NONSPACE	[^[:space:]]
NEWLINE		[\n]
HSPACE		[ \t]
HASH		^{HSPACE}*#{HSPACE}*
IDENTIFIER	[_a-zA-Z][_a-zA-Z0-9]*
TOKEN		[^[:space:](),]+

%%

{HASH}undef{HSPACE}* {
	return UNDEF;
}

	/* We use the ST_DEFINE and ST_DEFVAL states so that we can
	 * pass a space token, (yes, a token for whitespace!), since
	 * the preprocessor specification requires distinguishing
	 * "#define foo()" from "#define foo ()".
	 */
{HASH}define{HSPACE}* {
	BEGIN ST_DEFINE;
	return DEFINE;
}

<ST_DEFINE>{IDENTIFIER}	{
	BEGIN ST_DEFVAL;
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

<ST_DEFVAL>\n {
	BEGIN INITIAL;
	return NEWLINE;
}

<ST_DEFVAL>{HSPACE}+ {
	BEGIN INITIAL;
	return SPACE;
}

<ST_DEFVAL>"("	{
	BEGIN INITIAL;
	return '(';
}

{IDENTIFIER} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	switch (glcpp_parser_macro_type (yyextra, yylval.str))
	{
		case MACRO_TYPE_UNDEFINED:
			return IDENTIFIER;
		break;
		case MACRO_TYPE_OBJECT:
			return OBJ_MACRO;
		break;
		case MACRO_TYPE_FUNCTION:
			return FUNC_MACRO;
		break;
	}
}

[(),]	{
	return yytext[0];
}

{TOKEN} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return TOKEN;
}

\n {
	return NEWLINE;
}

{HSPACE}+

%%
