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
		yycolumn = 0;		\
		yylloc->source = 0;	\
	} while(0)

/* It's ugly to have macros that have return statements inside of
 * them, but flex-based lexer generation is all built around the
 * return statement.
 *
 * To mitigate the ugliness, we defer as much of the logic as possible
 * to an actual function, not a macro (see
 * glcpplex_update_state_per_token) and we make the word RETURN
 * prominent in all of the macros which may return.
 *
 * The most-commonly-used macro is RETURN_TOKEN which will perform all
 * necessary state updates based on the provided token,, then
 * conditionally return the token. It will not return a token if the
 * parser is currently skipping tokens, (such as within #if
 * 0...#else).
 *
 * The RETURN_TOKEN_NEVER_SKIP macro is a lower-level variant that
 * makes the token returning unconditional. This is needed for things
 * like #if and the tokens of its condition, (since these must be
 * evaluated by the parser even when otherwise skipping).
 *
 * Finally, RETURN_STRING_TOKEN is a simple convenience wrapper on top
 * of RETURN_TOKEN that performs a string copy of yytext before the
 * return.
 */
#define RETURN_TOKEN_NEVER_SKIP(token)				\
	do {							\
		if (token == NEWLINE)				\
			parser->last_token_was_newline = 1;	\
		else						\
			parser->last_token_was_newline = 0;	\
		return (token);					\
	} while (0)

#define RETURN_TOKEN(token)						\
	do {								\
		if (! parser->skipping) {				\
			RETURN_TOKEN_NEVER_SKIP(token);			\
		}							\
	} while(0)

#define RETURN_STRING_TOKEN(token)					\
	do {								\
		if (! parser->skipping) {				\
			yylval->str = ralloc_strdup (yyextra, yytext);	\
			RETURN_TOKEN_NEVER_SKIP (token);		\
		}							\
	} while(0)

%}

%option bison-bridge bison-locations reentrant noyywrap
%option extra-type="glcpp_parser_t *"
%option prefix="glcpp_"
%option stack
%option never-interactive

%x DONE COMMENT UNREACHABLE DEFINE NEWLINE_CATCHUP

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

	/* Set up the parser->skipping bit here before doing any lexing.
	 *
	 * This bit controls whether tokens are skipped, (as implemented by
         * RETURN_TOKEN), such as between "#if 0" and "#endif".
	 *
	 * The parser maintains a skip_stack indicating whether we should be
         * skipping, (and nested levels of #if/#ifdef/#ifndef/#endif) will
         * push and pop items from the stack.
	 *
	 * Here are the rules for determining whether we are skipping:
	 *
	 *	1. If the skip stack is NULL, we are outside of all #if blocks
	 *         and we are not skipping.
	 *
	 *	2. If the skip stack is non-NULL, the type of the top node in
	 *	   the stack determines whether to skip. A type of
	 *	   SKIP_NO_SKIP is used for blocks wheere we are emitting
	 *	   tokens, (such as between #if 1 and #endif, or after the
	 *	   #else of an #if 0, etc.).
	 *
	 *	3. The lexing_directive bit overrides the skip stack. This bit
	 *	   is set when we are actively lexing the expression for a
	 *	   pre-processor condition, (such as #if, #elif, or #else). In
	 *	   this case, even if otherwise skipping, we need to emit the
	 *	   tokens for this condition so that the parser can evaluate
	 *	   the expression. (For, #else, there's no expression, but we
	 *	   emit tokens so the parser can generate a nice error message
	 *	   if there are any tokens here).
	 */
	if (parser->skip_stack &&
	    parser->skip_stack->type != SKIP_NO_SKIP &&
	    ! parser->lexing_directive)
	{
		parser->skipping = 1;
	} else {
		parser->skipping = 0;
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

	/* For the pre-processor directives, we return these tokens
	 * even when we are otherwise skipping. */
{HASH}ifdef {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN_NEVER_SKIP (HASH_IFDEF);
}

{HASH}ifndef {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN_NEVER_SKIP (HASH_IFNDEF);
}

{HASH}if/[^_a-zA-Z0-9] {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN_NEVER_SKIP (HASH_IF);
}

{HASH}elif/[^_a-zA-Z0-9] {
	yyextra->lexing_directive = 1;
	yyextra->space_tokens = 0;
	RETURN_TOKEN_NEVER_SKIP (HASH_ELIF);
}

{HASH}else {
	yyextra->space_tokens = 0;
	RETURN_TOKEN_NEVER_SKIP (HASH_ELSE);
}

{HASH}endif {
	yyextra->space_tokens = 0;
	RETURN_TOKEN_NEVER_SKIP (HASH_ENDIF);
}

{HASH}error.* {
	if (! parser->skipping) {
		char *p;
		for (p = yytext; !isalpha(p[0]); p++); /* skip "  #   " */
		p += 5; /* skip "error" */
		glcpp_error(yylloc, yyextra, "#error%s", p);
	}
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
	if (! parser->skipping) {
		BEGIN DEFINE;
		yyextra->space_tokens = 0;
		RETURN_TOKEN (HASH_DEFINE);
	}
}

	/* An identifier immediately followed by '(' */
<DEFINE>{IDENTIFIER}/"(" {
	BEGIN INITIAL;
	RETURN_STRING_TOKEN (FUNC_IDENTIFIER);
}

	/* An identifier not immediately followed by '(' */
<DEFINE>{IDENTIFIER} {
	BEGIN INITIAL;
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
	RETURN_TOKEN (HASH_TOKEN);
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
	if (! parser->skipping) {
		if (parser->is_gles)
			glcpp_error(yylloc, yyextra, "Token pasting (##) is illegal in GLES");
		RETURN_TOKEN (PASTE);
	}
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

	/* We preserve all newlines, even between #if 0..#endif, so no
	skipping.. */
\n {
	if (parser->commented_newlines) {
		BEGIN NEWLINE_CATCHUP;
	}
	yyextra->space_tokens = 1;
	yyextra->lexing_directive = 0;
	yylineno++;
	yycolumn = 0;
	RETURN_TOKEN_NEVER_SKIP (NEWLINE);
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
