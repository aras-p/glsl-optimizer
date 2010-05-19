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
%x ST_DEFINE_OBJ_OR_FUNC
%x ST_DEFINE_PARAMETER
%x ST_DEFINE_VALUE
%x ST_UNDEF
%x ST_UNDEF_END

SPACE		[[:space:]]
NONSPACE	[^[:space:]]
NEWLINE		[\n]
HSPACE		[ \t]
HASH		^{HSPACE}*#{HSPACE}*
IDENTIFIER	[_a-zA-Z][_a-zA-Z0-9]*
TOKEN		[^[:space:](),]+

%%

{HASH}undef{HSPACE}* {
	BEGIN ST_UNDEF;
	return UNDEF;
}

<ST_UNDEF>{IDENTIFIER} {
	BEGIN ST_UNDEF_END;
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

<ST_UNDEF_END>{HSPACE}*

<ST_UNDEF_END>\n {
	BEGIN INITIAL;
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
	BEGIN ST_DEFINE_OBJ_OR_FUNC;
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

<ST_DEFINE_OBJ_OR_FUNC>\n {
	BEGIN INITIAL;
	yylval.str = xtalloc_strdup (yyextra, "");
	return REPLACEMENT;
}

<ST_DEFINE_OBJ_OR_FUNC>{HSPACE}+ {
	BEGIN ST_DEFINE_VALUE;
}

<ST_DEFINE_OBJ_OR_FUNC>"(" {
	BEGIN ST_DEFINE_PARAMETER;
	return '(';
}

<ST_DEFINE_PARAMETER>{IDENTIFIER} {
	yylval.str = xtalloc_strdup (yyextra, yytext);
	return IDENTIFIER;
}

<ST_DEFINE_PARAMETER>"," {
	return ',';
}

<ST_DEFINE_PARAMETER>")" {
	BEGIN ST_DEFINE_VALUE;
	return ')';
}

<ST_DEFINE_PARAMETER>{HSPACE}+

<ST_DEFINE_VALUE>.*\n {
	BEGIN INITIAL;
	yylval.str = xtalloc_strndup (yyextra, yytext, strlen (yytext) - 1);
	return REPLACEMENT;
}

{IDENTIFIER} {
	int parameter_index;
	yylval.str = xtalloc_strdup (yyextra, yytext);
	switch (glcpp_parser_classify_token (yyextra, yylval.str,
					     &parameter_index))
	{
		case TOKEN_CLASS_ARGUMENT:
			talloc_free (yylval.str);
			/* We don't return a value here since the
			 * current token will be replaced by new
			 * tokens. */
			glcpp_parser_push_expansion_argument (yyextra,
							      parameter_index);
		break;
		case TOKEN_CLASS_IDENTIFIER:
			return IDENTIFIER;
		break;
		case TOKEN_CLASS_FUNC_MACRO:
			return FUNC_MACRO;
		break;
		case TOKEN_CLASS_OBJ_MACRO:
			return OBJ_MACRO;
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
	printf ("\n");
}

{HSPACE}+

<<EOF>> {
	int done;

	done = glcpp_lex_stack_pop (yyextra->lex_stack);

	if (done)
		yyterminate ();

	glcpp_parser_pop_expansion (yyextra);
}

%%

void
glcpp_lex_stack_push (glcpp_lex_stack_t *stack, const char *string)
{
	struct yyguts_t *yyg = (struct yyguts_t*) stack->parser->scanner;
	glcpp_lex_node_t *node;

	/* Save the current buffer on the top of the stack. */
	node = xtalloc (stack, glcpp_lex_node_t);
	node->buffer = YY_CURRENT_BUFFER;

	node->next = stack->head;
	stack->head = node;

	/* Then switch to a new scan buffer for string. */
	yy_scan_string (string, stack->parser->scanner);
}

int
glcpp_lex_stack_pop (glcpp_lex_stack_t *stack)
{
	struct yyguts_t *yyg = (struct yyguts_t*) stack->parser->scanner;
	glcpp_lex_node_t *node;

	node = stack->head;

	if (node == NULL)
		return 1;

	stack->head = node->next;

	yy_delete_buffer (YY_CURRENT_BUFFER, stack->parser->scanner);
	yy_switch_to_buffer ((YY_BUFFER_STATE) node->buffer,
			     stack->parser->scanner);

	talloc_free (node);

	return 0;
}
