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

/**
 * \file slang_compile_function.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_compile_variable.h"
#include "slang_compile_struct.h"
#include "slang_compile_operation.h"
#include "slang_compile_function.h"

/* slang_function */

int slang_function_construct (slang_function *func)
{
	func->kind = slang_func_ordinary;
	if (!slang_variable_construct (&func->header))
		return 0;
	func->parameters = (slang_variable_scope *) slang_alloc_malloc (sizeof (slang_variable_scope));
	if (func->parameters == NULL)
	{
		slang_variable_destruct (&func->header);
		return 0;
	}
	if (!slang_variable_scope_construct (func->parameters))
	{
		slang_alloc_free (func->parameters);
		slang_variable_destruct (&func->header);
		return 0;
	}
	func->param_count = 0;
	func->body = NULL;
	func->address = ~0;
	return 1;
}

void slang_function_destruct (slang_function *func)
{
	slang_variable_destruct (&func->header);
	slang_variable_scope_destruct (func->parameters);
	slang_alloc_free (func->parameters);
	if (func->body != NULL)
	{
		slang_operation_destruct (func->body);
		slang_alloc_free (func->body);
	}
}

/* slang_function_scope */

int slang_function_scope_construct (slang_function_scope *scope)
{
	scope->functions = NULL;
	scope->num_functions = 0;
	scope->outer_scope = NULL;
	return 1;
}

void slang_function_scope_destruct (slang_function_scope *scope)
{
	unsigned int i;

	for (i = 0; i < scope->num_functions; i++)
		slang_function_destruct (scope->functions + i);
	slang_alloc_free (scope->functions);
}

int slang_function_scope_find_by_name (slang_function_scope *funcs, const char *name, int all_scopes)
{
	unsigned int i;

	for (i = 0; i < funcs->num_functions; i++)
		if (slang_string_compare (name, funcs->functions[i].header.name) == 0)
			return 1;
	if (all_scopes && funcs->outer_scope != NULL)
		return slang_function_scope_find_by_name (funcs->outer_scope, name, 1);
	return 0;
}

slang_function *slang_function_scope_find (slang_function_scope *funcs, slang_function *fun,
	int all_scopes)
{
	unsigned int i;

	for (i = 0; i < funcs->num_functions; i++)
	{
		slang_function *f = funcs->functions + i;
		unsigned int j;

		if (slang_string_compare (fun->header.name, f->header.name) != 0)
			continue;
		if (fun->param_count != f->param_count)
			continue;
		for (j = 0; j < fun->param_count; j++)
		{
			if (!slang_type_specifier_equal (&fun->parameters->variables[j].type.specifier,
					&f->parameters->variables[j].type.specifier))
				break;
		}
		if (j == fun->param_count)
			return f;
	}
	if (all_scopes && funcs->outer_scope != NULL)
		return slang_function_scope_find (funcs->outer_scope, fun, 1);
	return NULL;
}

