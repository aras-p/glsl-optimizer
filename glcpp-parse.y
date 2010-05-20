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

#include "glcpp.h"

void
yyerror (void *scanner, const char *error);

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *macro,
		      token_list_t *replacements);

void
_define_function_macro (glcpp_parser_t *parser,
			const char *macro,
			string_list_t *parameters,
			token_list_t *replacements);

void
_expand_object_macro (glcpp_parser_t *parser, const char *identifier);

void
_expand_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			argument_list_t *arguments);

string_list_t *
_string_list_create (void *ctx);

void
_string_list_append_item (string_list_t *list, const char *str);

void
_string_list_append_list (string_list_t *list, string_list_t *tail);

int
_string_list_contains (string_list_t *list, const char *member, int *index);

int
_string_list_length (string_list_t *list);

argument_list_t *
_argument_list_create (void *ctx);

void
_argument_list_append (argument_list_t *list, token_list_t *argument);

int
_argument_list_length (argument_list_t *list);

token_list_t *
_argument_list_member_at (argument_list_t *list, int index);

token_list_t *
_token_list_create (void *ctx);

void
_token_list_append (token_list_t *list, int type, const char *value);

void
_token_list_append_list (token_list_t *list, token_list_t *tail);

static void
glcpp_parser_pop_expansion (glcpp_parser_t *parser);

#define yylex glcpp_parser_lex

static int
glcpp_parser_lex (glcpp_parser_t *parser);

%}

%union {
	int ival;
	char *str;
	argument_list_t *argument_list;
	string_list_t *string_list;
	token_t token;
	token_list_t *token_list;
}

%parse-param {glcpp_parser_t *parser}
%lex-param {glcpp_parser_t *parser}

%token DEFINE FUNC_MACRO IDENTIFIER IDENTIFIER_FINALIZED OBJ_MACRO NEWLINE SEPARATOR SPACE TOKEN UNDEF
%type <ival> punctuator
%type <str> content FUNC_MACRO IDENTIFIER IDENTIFIER_FINALIZED OBJ_MACRO
%type <argument_list> argument_list
%type <string_list> macro parameter_list
%type <token> TOKEN argument_word argument_word_or_comma
%type <token_list> argument argument_or_comma replacement_list pp_tokens

/* Hard to remove shift/reduce conflicts documented as follows:
 *
 * 1. '(' after FUNC_MACRO name which is correctly resolved to shift
 *    to form macro invocation rather than reducing directly to
 *    content.
 *
 * 2. Similarly, '(' after FUNC_MACRO which is correctly resolved to
 *    shift to form macro invocation rather than reducing directly to
 *    argument.
 *
 * 3. Similarly again now that we added argument_or_comma as well.
 */
%expect 3

%%

	/* We do all printing at the input level.
	 *
	 * The value for "input" is simply TOKEN or SEPARATOR so we
	 * can decide whether it's necessary to print a space
	 * character between any two. */
input:
	/* empty */ {
		parser->just_printed_separator = 1;
	}
|	input content {
		int is_token;

		if ($2 && strlen ($2)) {
			int c = $2[0];
			int is_not_separator = ((c >= 'a' && c <= 'z') ||
						(c >= 'A' && c <= 'Z') ||
						(c >= 'A' && c <= 'Z') ||
						(c >= '0' && c <= '9') ||
						(c == '_'));

			if (! parser->just_printed_separator && is_not_separator)
			{
				printf (" ");
			}
			printf ("%s", $2);

			if (is_not_separator)
				parser->just_printed_separator = 0;
			else
				parser->just_printed_separator = 1;
		}

		if ($2)
			talloc_free ($2);

		if (parser->need_newline) {
			printf ("\n");
			parser->just_printed_separator = 1;
			parser->need_newline = 0;
		}
	}
;

content:
	IDENTIFIER {
		$$ = $1;
	}
|	IDENTIFIER_FINALIZED {
		$$ = $1;
	}
|	TOKEN {
		$$ = $1.value;
	}
|	FUNC_MACRO {
		$$ = $1;
	}
|	directive {
		$$ = talloc_strdup (parser, "\n");
	}
|	punctuator {
		$$ = talloc_asprintf (parser, "%c", $1);
	}
|	macro {
		$$ = NULL;
	}
;

punctuator:
	'('	{ $$ = '('; }
|	')'	{ $$ = ')'; }
|	','	{ $$ = ','; }
	;

macro:
	FUNC_MACRO '(' argument_list ')' {
		_expand_function_macro (parser, $1, $3);
	}
|	OBJ_MACRO {
		_expand_object_macro (parser, $1);
		talloc_free ($1);
	}
;

argument_list:
	/* empty */ {
		$$ = _argument_list_create (parser);
	}
|	argument {
		$$ = _argument_list_create (parser);
		_argument_list_append ($$, $1);
	}
|	argument_list ',' argument {
		_argument_list_append ($1, $3);
		$$ = $1;
	}
;

argument:
	argument_word {
		$$ = _token_list_create (parser);
		_token_list_append ($$, $1.type, $1.value);
	}
|	argument argument_word {
		_token_list_append ($1, $2.type, $2.value);
		talloc_free ($2.value);
		$$ = $1;
	}
|	argument '(' argument_or_comma ')' {
		_token_list_append ($1, '(', "(");
		_token_list_append_list ($1, $3);
		_token_list_append ($1, ')', ")");
		$$ = $1;
	}
;

argument_word:
	IDENTIFIER { $$.type = IDENTIFIER; $$.value = $1; }
|	IDENTIFIER_FINALIZED { $$.type = IDENTIFIER_FINALIZED; $$.value = $1; }
|	TOKEN { $$ = $1; }
|	FUNC_MACRO { $$.type = FUNC_MACRO; $$.value = $1; }
|	macro {	$$.type = TOKEN; $$.value = xtalloc_strdup (parser, ""); }
;

	/* XXX: The body of argument_or_comma is the same as the body
	 * of argument, but with "argument" and "argument_word"
	 * changed to "argument_or_comma" and
	 * "argument_word_or_comma". It would be nice to have less
	 * redundancy here, but I'm not sure how.
	 *
	 * It would also be nice to have a less ugly grammar to have
	 * to implement, but such is the C preprocessor.
	 */
argument_or_comma:
	argument_word_or_comma {
		$$ = _token_list_create (parser);
		_token_list_append ($$, $1.type, $1.value);
	}
|	argument_or_comma argument_word_or_comma {
		_token_list_append ($1, $2.type, $2.value);
		$$ = $1;
	}
|	argument_or_comma '(' argument_or_comma ')' {
		_token_list_append ($1, '(', "(");
		_token_list_append_list ($1, $3);
		_token_list_append ($1, ')', ")");
		$$ = $1;
	}
;

argument_word_or_comma:
	IDENTIFIER { $$.type = IDENTIFIER; $$.value = $1; }
|	IDENTIFIER_FINALIZED { $$.type = IDENTIFIER_FINALIZED; $$.value = $1; }
|	TOKEN { $$ = $1; }
|	FUNC_MACRO { $$.type = FUNC_MACRO; $$.value = $1; }
|	macro {	$$.type = TOKEN; $$.value = xtalloc_strdup (parser, ""); }
|	',' { $$.type = ','; $$.value = xtalloc_strdup (parser, ","); }
;

directive:
	DEFINE IDENTIFIER NEWLINE {
		token_list_t *list = _token_list_create (parser);
		_define_object_macro (parser, $2, list);
	}
|	DEFINE IDENTIFIER SPACE replacement_list NEWLINE {
		_define_object_macro (parser, $2, $4);
	}
|	DEFINE IDENTIFIER '(' parameter_list ')' replacement_list NEWLINE {
		_define_function_macro (parser, $2, $4, $6);
	}
|	UNDEF IDENTIFIER {
		string_list_t *macro = hash_table_find (parser->defines, $2);
		if (macro) {
			/* XXX: Need hash table to support a real way
			 * to remove an element rather than prefixing
			 * a new node with data of NULL like this. */
			hash_table_insert (parser->defines, NULL, $2);
			talloc_free (macro);
		}
		talloc_free ($2);
	}
;

parameter_list:
	/* empty */ {
		$$ = _string_list_create (parser);
	}
|	IDENTIFIER {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
		talloc_free ($1);
	}
|	parameter_list ',' IDENTIFIER {
		_string_list_append_item ($1, $3);
		talloc_free ($3);
		$$ = $1;
	}
;

replacement_list:
	/* empty */ {
		$$ = _token_list_create (parser);
	}
|	pp_tokens {
		$$ = $1;
	}
;


pp_tokens:
	TOKEN {
		$$ = _token_list_create (parser);
		_token_list_append ($$, $1.type, $1.value);
	}
|	pp_tokens TOKEN {
	_token_list_append ($1, $2.type, $2.value);
		$$ = $1;
	}
;

%%

string_list_t *
_string_list_create (void *ctx)
{
	string_list_t *list;

	list = xtalloc (ctx, string_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_string_list_append_list (string_list_t *list, string_list_t *tail)
{
	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
}

void
_string_list_append_item (string_list_t *list, const char *str)
{
	string_node_t *node;

	node = xtalloc (list, string_node_t);
	node->str = xtalloc_strdup (node, str);
		
	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_string_list_contains (string_list_t *list, const char *member, int *index)
{
	string_node_t *node;
	int i;

	if (list == NULL)
		return 0;

	for (i = 0, node = list->head; node; i++, node = node->next) {
		if (strcmp (node->str, member) == 0) {
			if (index)
				*index = i;
			return 1;
		}
	}

	return 0;
}

int
_string_list_length (string_list_t *list)
{
	int length = 0;
	string_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

argument_list_t *
_argument_list_create (void *ctx)
{
	argument_list_t *list;

	list = xtalloc (ctx, argument_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_argument_list_append (argument_list_t *list, token_list_t *argument)
{
	argument_node_t *node;

	if (argument == NULL || argument->head == NULL)
		return;

	node = xtalloc (list, argument_node_t);
	node->argument = argument;

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_argument_list_length (argument_list_t *list)
{
	int length = 0;
	argument_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

token_list_t *
_argument_list_member_at (argument_list_t *list, int index)
{
	argument_node_t *node;
	int i;

	if (list == NULL)
		return NULL;

	node = list->head;
	for (i = 0; i < index; i++) {
		node = node->next;
		if (node == NULL)
			break;
	}

	if (node)
		return node->argument;

	return NULL;
}

token_list_t *
_token_list_create (void *ctx)
{
	token_list_t *list;

	list = xtalloc (ctx, token_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_token_list_append (token_list_t *list, int type, const char *value)
{
	token_node_t *node;

	node = xtalloc (list, token_node_t);
	node->type = type;
	node->value = xtalloc_strdup (list, value);

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

void
_token_list_append_list (token_list_t *list, token_list_t *tail)
{
	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
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

	glcpp_lex_init_extra (parser, &parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
	parser->expansions = NULL;

	parser->just_printed_separator = 1;
	parser->need_newline = 0;

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
	if (parser->need_newline)
		printf ("\n");
	glcpp_lex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	talloc_free (parser);
}

static int
glcpp_parser_is_expanding (glcpp_parser_t *parser, const char *member)
{
	expansion_node_t *node;

	for (node = parser->expansions; node; node = node->next) {
		if (node->macro &&
		    strcmp (node->macro->identifier, member) == 0)
		{
			return 1;
		}
	}

	return 0;
}

token_class_t
glcpp_parser_classify_token (glcpp_parser_t *parser,
			     const char *identifier,
			     int *parameter_index)
{
	macro_t *macro;

	/* Is this token a defined macro? */
	macro = hash_table_find (parser->defines, identifier);

	if (macro == NULL)
		return TOKEN_CLASS_IDENTIFIER;

	/* Don't consider this a macro if we are already actively
	 * expanding this macro. */
	if (glcpp_parser_is_expanding (parser, identifier))
		return TOKEN_CLASS_IDENTIFIER_FINALIZED;

	/* Definitely a macro. Just need to check if it's function-like. */
	if (macro->is_function)
		return TOKEN_CLASS_FUNC_MACRO;
	else
		return TOKEN_CLASS_OBJ_MACRO;
}

void
_define_object_macro (glcpp_parser_t *parser,
		      const char *identifier,
		      token_list_t *replacements)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->identifier = talloc_strdup (macro, identifier);
	macro->replacements = talloc_steal (macro, replacements);

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			string_list_t *parameters,
			token_list_t *replacements)
{
	macro_t *macro;

	macro = xtalloc (parser, macro_t);

	macro->is_function = 1;
	macro->parameters = talloc_steal (macro, parameters);
	macro->identifier = talloc_strdup (macro, identifier);
	macro->replacements = talloc_steal (macro, replacements);

	hash_table_insert (parser->defines, macro, identifier);
}

static void
_glcpp_parser_push_expansion (glcpp_parser_t *parser,
			      macro_t *macro,
			      token_node_t *replacements)
{
	expansion_node_t *node;

	node = xtalloc (parser, expansion_node_t);

	node->macro = macro;
	node->replacements = replacements;

	node->next = parser->expansions;
	parser->expansions = node;
}

static void
glcpp_parser_pop_expansion (glcpp_parser_t *parser)
{
	expansion_node_t *node;

	node = parser->expansions;

	if (node == NULL) {
		fprintf (stderr, "Internal error: _expansion_list_pop called on an empty list.\n");
		exit (1);
	}

	parser->expansions = node->next;

	talloc_free (node);
}

void
_expand_object_macro (glcpp_parser_t *parser, const char *identifier)
{
	macro_t *macro;

	macro = hash_table_find (parser->defines, identifier);
	assert (! macro->is_function);
	assert (! glcpp_parser_is_expanding (parser, identifier));

	_glcpp_parser_push_expansion (parser, macro, macro->replacements->head);
}

void
_expand_function_macro (glcpp_parser_t *parser,
			const char *identifier,
			argument_list_t *arguments)
{
	macro_t *macro;
	token_list_t *expanded;
	token_node_t *i, *j;
	int parameter_index;

	macro = hash_table_find (parser->defines, identifier);
	assert (macro->is_function);
	assert (! glcpp_parser_is_expanding (parser, identifier));

	if (_argument_list_length (arguments) !=
	    _string_list_length (macro->parameters))
	{
		fprintf (stderr,
			 "Error: macro %s invoked with %d arguments (expected %d)\n",
			 identifier,
			 _argument_list_length (arguments),
			 _string_list_length (macro->parameters));
		return;
	}

	expanded = _token_list_create (macro);

	for (i = macro->replacements->head; i; i = i->next) {
		if (_string_list_contains (macro->parameters, i->value,
					   &parameter_index))
		{
			token_list_t *argument;
			argument = _argument_list_member_at (arguments,
							     parameter_index);
			for (j = argument->head; j; j = j->next)
			{
				_token_list_append (expanded, j->type,
						    j->value);
			}
		} else {
			_token_list_append (expanded, i->type, i->value);
		}
	}

	_glcpp_parser_push_expansion (parser, macro, expanded->head);
}

static int
glcpp_parser_lex (glcpp_parser_t *parser)
{
	expansion_node_t *expansion;
	token_node_t *replacements;
	int parameter_index;

    /* Who says C can't do efficient tail recursion? */
    RECURSE:

	expansion = parser->expansions;

	if (expansion == NULL)
		return glcpp_lex (parser->scanner);

	replacements = expansion->replacements;

	/* Pop expansion when replacements is exhausted. */
	if (replacements == NULL) {
		glcpp_parser_pop_expansion (parser);
		goto RECURSE;
	}

	expansion->replacements = replacements->next;

	if (strcmp (replacements->value, "(") == 0)
		return '(';
	else if (strcmp (replacements->value, ")") == 0)
		return ')';

	yylval.str = xtalloc_strdup (parser, replacements->value);

	/* Carefully refuse to expand any finalized identifier. */
	if (replacements->type == IDENTIFIER_FINALIZED)
		return IDENTIFIER_FINALIZED;

	switch (glcpp_parser_classify_token (parser, yylval.str,
					     &parameter_index))
	{
	case TOKEN_CLASS_IDENTIFIER:
		return IDENTIFIER;
		break;
	case TOKEN_CLASS_IDENTIFIER_FINALIZED:
		return IDENTIFIER_FINALIZED;
		break;
	case TOKEN_CLASS_FUNC_MACRO:
		return FUNC_MACRO;
		break;
	default:
	case TOKEN_CLASS_OBJ_MACRO:
		return OBJ_MACRO;
		break;
	}
}
