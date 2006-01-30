/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if !defined SLANG_COMPILE_FUNCTION_H
#define SLANG_COMPILE_FUNCTION_H

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_function_kind_
{
	slang_func_ordinary,
	slang_func_constructor,
	slang_func_operator
} slang_function_kind;

typedef struct slang_function_
{
	slang_function_kind kind;
	slang_variable header;
	slang_variable_scope *parameters;
	unsigned int param_count;
	slang_operation *body;
	unsigned int address;
} slang_function;

int slang_function_construct (slang_function *);
void slang_function_destruct (slang_function *);

typedef struct slang_function_scope_
{
	slang_function *functions;
	unsigned int num_functions;
	struct slang_function_scope_ *outer_scope;
} slang_function_scope;

int slang_function_scope_construct (slang_function_scope *);
void slang_function_scope_destruct (slang_function_scope *);
int slang_function_scope_find_by_name (slang_function_scope *, const char *, int);
slang_function *slang_function_scope_find (slang_function_scope *, slang_function *, int);

#ifdef __cplusplus
}
#endif

#endif

