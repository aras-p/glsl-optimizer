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
#include <ctype.h>

#include "glcpp.h"
#include "glcpp-parse.h"

/* Flex annoyingly generates some functions without making them
 * static. Let's declare them here. */
int glcpp_get_column  (yyscan_t yyscanner);
void glcpp_set_column (int  column_no , yyscan_t yyscanner);

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#endif

#define YY_NO_INPUT

#define YY_USER_ACTION							\
	do {								\
		if (parser->has_new_line_number)			\
			yylineno = parser->new_line_number;		\
		if (parser->has_new_source_number)			\
			yylloc->source = parser->new_source_number;	\
		yylloc->first_column = yycolumn + 1;			\
		yylloc->first_line = yylloc->last_line = yylineno;	\
		yycolumn += yyleng;					\
		yylloc->last_column = yycolumn + 1;			\
		parser->has_new_line_number = 0;			\
		parser->has_new_source_number = 0;			\
 } while(0);

#define YY_USER_INIT			\
	do {				\
		yylineno = 1;		\
		yycolumn = 1;		\
		yylloc->source = 0;	\
	} while(0)

#define RETURN_TOKEN(token)					\
	do {							\
		if (token == NEWLINE)				\
			parser->last_token_was_newline = 1;	\
		else						\
			parser->last_token_was_newline = 0;	\
		return (token);					\
	} while(0)

#define RETURN_STRING_TOKEN(token)				\
	do {							\
		yylval->str = ralloc_strdup (yyextra, yytext);	\
		RETURN_TOKEN (token);				\
	} while(0)

%}

%option bison-bridge bison-locations reentrant noyywrap
%option extra-type="glcpp_parser_t *"
%option prefix="glcpp_"
%option stack
%option never-interactive

%x DONE COMMENT UNREACHABLE SKIP DEFINE NEWLINE_CATCHUP

SPACE		[[:space:]]
NONSPACE	[^[:space:]]
NEWLINE		[\n]
HSPACE		[ \t]
HASH		^{HSPACE}*#{HSPACE}*
IDENTIFIER	[_a-zA-Z][_a-zA-Z0-9]*
PP_NUMBER	[.]?[0-9]([._a-zA-Z0-9]|[eEpP][-+])*
PUNCTUATION	[][(){}.&*~!/%<>^|;,=+-]

/* The OTHER class is simply a catch-all for things that the CPP
parser just doesn't care about. Since flex regular expressions that
match longer strings take priority over those matching shorter
strings, we have to be careful to avoid OTHER matching and hiding
something that CPP does care about. So we simply exclude all
characters that appear in any other expressions. */

OTHER		[^][_#[:space:]#a-zA-Z0-9(){}.&*~!/%<>^|;,=+-]

DIGITS			[0-9][0-9]*
DECIMAL_INTEGER		[1-9][0-9]*[uU]?
OCTAL_INTEGER		0[0-7]*[uU]?
HEXADECIMAL_INTEGER	0[xX][0-9a-fA-F]+[uU]?

%%

	glcpp_parser_t *parser = yyextra;

	/* When we lex a multi-line comment, we replace it (as
	 * specified) with a single space. But if the comment spanned
	 * multiple lines, then subsequent parsing stages will not
	 * count correct line numbers. To avoid this problem we keep
	 * track of all newlines that were commented out by a
	 * multi-line comment, and we emit a NEWLINE token for each at
	 * the next legal opportunity, (which is when the lexer would
	 * be emitting a NEWLINE token anyway).
	 */
	if (YY_START == NEWLINE_CATCHUP) {
		if (parser->commented_newlines)
			parser->commented_newlines--;
		if (parser->commented_newlines == 0)
			BEGIN INITIAL;
		RETURN_TOKEN (NEWLINE);
	}

	/* The handling of the SKIP vs INITIAL start states requires
	 * some special handling. Typically, a lexer would change
	 * start states with statements like "BEGIN SKIP" within the
	 * lexer rules. We can't get away with that here, since we
	 * need the parser to actually evaluate expressions for
	 * directives like "#if".
	 *
	 * So, here, in code that will be executed on every call to
	 * the lexer,and before any rules, we examine the skip_stack
	 * as set by the parser to know whether to change from INITIAL
	 * to SKIP or from SKIP back to INITIAL.
	 *
	 * Three cases cause us to switch out of the SKIP state and
	 * back to the INITIAL state:
	 *
	 *	1. The top of the skip_stack is of type SKIP_NO_SKIP
	 *	   This means we're still evaluating some #if
	 *	   hierarchy, but we're on a branch of it where
	 *	   content should not be skipped (such as "#if 1" or
	 *	   "#else" or so).
	 *
	 *	2. The skip_stack is NULL meaning that we've reached
	 *	   the last #endif.
	 *
	 *	3. The lexing_directive bit is set. This indicates that we are
	 *	   lexing a pre-processor directive, (such as #if, #elif, or
	 *	   #else). For the #if and #elif directives we always need to
	 *	   parse the conditions, (even if otherwise within an #if
	 *	   0). And for #else, we want to be able to generate an error
	 *	   if any garbage follows #else.
	 */
	if (YY_START == INITIAL || YY_START == SKIP) {
		if (parser->lexing_directive ||
		    parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			BEGIN INITIAL;
		} else {
			BEGIN SKIP;
		}
	}

	/* Single-line comments */
"//"[^\n]* {
}

	/* Multi-line comments */
<DEFINE,INITIAL>"/*"                    { yy_push_state(COMMENT, yyscanner); }
<COMMENT>[^*\n]*
<COMMENT>[^*\n]*\n      { yylineno++; yycolumn = 0; parser->commented_newlines++; }
<COMMENT>"*"+[^*/\n]*
<COMMENT>"*"+[^*/\n]*\n { yylineno++; yycolumn = 0; parser->commented_newlines++; }
<COMMENT>"*"+"/"        {
	yy_pop_state(yyscanner);
	if (yyextra->space_tokens)
		RETURN_TOKEN (SPACE);
}

{HASH}version{HSPACE}+ {
	yyextra->space_tokens = 0;
	RETURN_STRING_TOKEN (HASH_VERSION);
}

	/* glcpp doesn't handle #extension, #version, or #pragma directives.
	 * Simply pass them through to the main compiler's lexer/parser. */
{HASH}(extension|pragma)[^\n]* {
	yylineno++;
	yycolumn = 0;
	RETURN_STRING_TOKEN (OTHER);
}

{HASH}line{HSPACE}+ {
	RETURN_TOKEN (HASH_LINE);
}

<SKIP,INITIAL>{
{HASH}ifdef {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_IFDEF);
}

{HASH}ifndef {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_IFNDEF);
}

{HASH}if/[^_a-zA-Z0-9] {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_IF);
}

{HASH}elif/[^_a-zA-Z0-9] {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_ELIF);
}

{HASH}else {
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_ELSE);
}

{HASH}endif {
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_ENDIF);
}
}

<SKIP>[^\n] {
}

{HASH}error.* {
	char *p;
	for (p = yytext; !isalpha(p[0]); p++); /* skip "  #   " */
	p += 5; /* skip "error" */
	glcpp_error(yylloc, yyextra, "#error%s", p);
}

	/* After we see a "#define" we enter the <DEFINE> start state
	 * for the lexer. Within <DEFINE> we are looking for the first
	 * identifier and specifically checking whether the identifier
	 * is followed by a '(' or not, (to lex either a
	 * FUNC_IDENTIFIER or an OBJ_IDENITIFIER token).
	 *
	 * While in the <DEFINE> state we also need to explicitly
	 * handle a few other things that may appear before the
	 * identifier:
	 * 
	 * 	* Comments, (handled above with the main support for
	 * 	  comments).
	 *
	 *	* Whitespace (simply ignored)
	 *
	 *	* Anything else, (not an identifier, not a comment,
	 *	  and not whitespace). This will generate an error.
	 */
{HASH}define{HSPACE}+ {
	yyextra->space_tokens = 0;
	yy_push_state(DEFINE, yyscanner);
	RETURN_TOKEN (HASH_DEFINE);
}

	/* An identifier immediately followed by '(' */
<DEFINE>{IDENTIFIER}/"(" {
	yy_pop_state(yyscanner);
	RETURN_STRING_TOKEN (FUNC_IDENTIFIER);
}

	/* An identifier not immediately followed by '(' */
<DEFINE>{IDENTIFIER} {
	yy_pop_state(yyscanner);
	RETURN_STRING_TOKEN (OBJ_IDENTIFIER);
}

	/* Whitespace */
<DEFINE>{HSPACE}+ {
	/* Just ignore it. Nothing to do here. */
}

	/* '/' not followed by '*', so not a comment. This is an error. */
<DEFINE>[/][^*]{NONSPACE}* {
	BEGIN INITIAL;
	glcpp_error(yylloc, yyextra, "#define followed by a non-identifier: %s", yytext);
	RETURN_STRING_TOKEN (INTEGER_STRING);
}

	/* A character that can't start an identifier, comment, or
	 * space. This is an error. */
<DEFINE>[^_a-zA-Z/[:space:]]{NONSPACE}* {
	BEGIN INITIAL;
	glcpp_error(yylloc, yyextra, "#define followed by a non-identifier: %s", yytext);
	RETURN_STRING_TOKEN (INTEGER_STRING);
}

{HASH}undef {
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH_UNDEF);
}

{HASH} {
	yyextra->space_tokens = 0;
	RETURN_TOKEN (HASH);
}

{DECIMAL_INTEGER} {
	RETURN_STRING_TOKEN (INTEGER_STRING);
}

{OCTAL_INTEGER} {
	RETURN_STRING_TOKEN (INTEGER_STRING);
}

{HEXADECIMAL_INTEGER} {
	RETURN_STRING_TOKEN (INTEGER_STRING);
}

"<<"  {
	RETURN_TOKEN (LEFT_SHIFT);
}

">>" {
	RETURN_TOKEN (RIGHT_SHIFT);
}

"<=" {
	RETURN_TOKEN (LESS_OR_EQUAL);
}

">=" {
	RETURN_TOKEN (GREATER_OR_EQUAL);
}

"==" {
	RETURN_TOKEN (EQUAL);
}

"!=" {
	RETURN_TOKEN (NOT_EQUAL);
}

"&&" {
	RETURN_TOKEN (AND);
}

"||" {
	RETURN_TOKEN (OR);
}

"##" {
	if (parser->is_gles)
		glcpp_error(yylloc, yyextra, "Token pasting (##) is illegal in GLES");
	RETURN_TOKEN (PASTE);
}

"defined" {
	RETURN_TOKEN (DEFINED);
}

{IDENTIFIER} {
	RETURN_STRING_TOKEN (IDENTIFIER);
}

{PP_NUMBER} {
	RETURN_STRING_TOKEN (OTHER);
}

{PUNCTUATION} {
	RETURN_TOKEN (yytext[0]);
}

{OTHER}+ {
	RETURN_STRING_TOKEN (OTHER);
}

{HSPACE} {
	if (yyextra->space_tokens) {
		RETURN_TOKEN (SPACE);
	}
}

<SKIP,INITIAL>\n {
	if (parser->commented_newlines) {
		BEGIN NEWLINE_CATCHUP;
	}
	yyextra->space_tokens = 1;
	yyextra->lexing_directive = 0;
	yylineno++;
	yycolumn = 0;
	RETURN_TOKEN (NEWLINE);
}

<INITIAL,COMMENT,DEFINE><<EOF>> {
	if (YY_START == COMMENT)
		glcpp_error(yylloc, yyextra, "Unterminated comment");
	if (YY_START == DEFINE)
		glcpp_error(yylloc, yyextra, "#define without macro name");
	BEGIN DONE; /* Don't keep matching this rule forever. */
	yyextra->lexing_directive = 0;
	if (! parser->last_token_was_newline)
		RETURN_TOKEN (NEWLINE);
}

	/* We don't actually use the UNREACHABLE start condition. We
	only have this action here so that we can pretend to call some
	generated functions, (to avoid "defined but not used"
	warnings. */
<UNREACHABLE>. {
	unput('.');
	yy_top_state(yyextra);
}

%%

void
glcpp_lex_set_source_string(glcpp_parser_t *parser, const char *shader)
{
	yy_scan_string(shader, parser->scanner);
}
