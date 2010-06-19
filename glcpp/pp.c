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

#include "glcpp.h"

void
glcpp_error (YYLTYPE *locp, glcpp_parser_t *parser, const char *fmt, ...)
{
	parser->info_log = talloc_asprintf_append(parser->info_log,
						  "%u:%u(%u): "
						  "preprocessor error: ",
						  locp->source,
						  locp->first_line,
						  locp->first_column);
	va_list ap;
	va_start(ap, fmt);
	parser->info_log = talloc_vasprintf_append(parser->info_log, fmt, ap);
	va_end(ap);
	parser->info_log = talloc_strdup_append(parser->info_log, "\n");
}

extern int
preprocess(void *talloc_ctx, const char **shader, size_t *shader_len)
{
	int errors;
	glcpp_parser_t *parser = glcpp_parser_create ();
	glcpp_lex_set_source_string (parser, *shader);

	glcpp_parser_parse (parser);

	errors = parser->info_log[0] != '\0';
	printf("%s", parser->info_log);

	talloc_steal(talloc_ctx, parser->output);
	*shader = parser->output;
	*shader_len = strlen(parser->output);

	glcpp_parser_destroy (parser);
	return errors;
}
