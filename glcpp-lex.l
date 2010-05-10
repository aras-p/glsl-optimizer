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

%x ST_DEFINE
%x ST_DEFVAL

SPACE		[[:space:]]
NONSPACE	[^[:space:]]
NOTNEWLINE	[^\n]
HSPACE		[ \t]
HASH		^{HSPACE}*#
IDENTIFIER	[_a-zA-Z][_a-zA-Z0-9]*
DEFVAL		{NONSPACE}{NOTNEWLINE}*
%%

{HASH}define		{ BEGIN ST_DEFINE; return DEFINE; }

<ST_DEFINE>{HSPACE}+
<ST_DEFINE>{IDENTIFIER}	{ BEGIN ST_DEFVAL; yylval = strdup (yytext); return IDENTIFIER; }

<ST_DEFVAL>{SPACE}+
<ST_DEFVAL>{DEFVAL}	{ BEGIN INITIAL; yylval = strdup (yytext); return DEFVAL; }

	/* Anything we don't specifically recognize is a stream of tokens */
{NONSPACE}+		{ yylval = strdup (yytext); return TOKEN; }

%%
