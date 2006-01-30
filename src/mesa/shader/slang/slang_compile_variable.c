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
 * \file slang_compile_variable.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_compile_variable.h"
#include "slang_compile_struct.h"
#include "slang_compile_operation.h"
#include "slang_compile_function.h"

/* slang_type_specifier_type */

typedef struct
{
	const char *name;
	slang_type_specifier_type type;
} type_specifier_type_name;

static type_specifier_type_name type_specifier_type_names[] = {
	{ "void", slang_spec_void },
	{ "bool", slang_spec_bool },
	{ "bvec2", slang_spec_bvec2 },
	{ "bvec3", slang_spec_bvec3 },
	{ "bvec4", slang_spec_bvec4 },
	{ "int", slang_spec_int },
	{ "ivec2", slang_spec_ivec2 },
	{ "ivec3", slang_spec_ivec3 },
	{ "ivec4", slang_spec_ivec4 },
	{ "float", slang_spec_float },
	{ "vec2", slang_spec_vec2 },
	{ "vec3", slang_spec_vec3 },
	{ "vec4", slang_spec_vec4 },
	{ "mat2", slang_spec_mat2 },
	{ "mat3", slang_spec_mat3 },
	{ "mat4", slang_spec_mat4 },
	{ "sampler1D", slang_spec_sampler1D },
	{ "sampler2D", slang_spec_sampler2D },
	{ "sampler3D", slang_spec_sampler3D },
	{ "samplerCube", slang_spec_samplerCube },
	{ "sampler1DShadow", slang_spec_sampler1DShadow },
	{ "sampler2DShadow", slang_spec_sampler2DShadow },
	{ NULL, slang_spec_void }
};

slang_type_specifier_type slang_type_specifier_type_from_string (const char *name)
{
	type_specifier_type_name *p = type_specifier_type_names;
	while (p->name != NULL)
	{
		if (slang_string_compare (p->name, name) == 0)
			break;
		p++;
	}
	return p->type;
}

const char *slang_type_specifier_type_to_string (slang_type_specifier_type type)
{
	type_specifier_type_name *p = type_specifier_type_names;
	while (p->name != NULL)
	{
		if (p->type == type)
			break;
		p++;
	}
	return p->name;
}

/* slang_type_specifier */

int slang_type_specifier_construct (slang_type_specifier *spec)
{
	spec->type = slang_spec_void;
	spec->_struct = NULL;
	spec->_array = NULL;
	return 1;
}

void slang_type_specifier_destruct (slang_type_specifier *spec)
{
	if (spec->_struct != NULL)
	{
		slang_struct_destruct (spec->_struct);
		slang_alloc_free (spec->_struct);
	}
	if (spec->_array != NULL)
	{
		slang_type_specifier_destruct (spec->_array);
		slang_alloc_free (spec->_array);
	}
}

int slang_type_specifier_copy (slang_type_specifier *x, const slang_type_specifier *y)
{
	slang_type_specifier z;

	if (!slang_type_specifier_construct (&z))
		return 0;
	z.type = y->type;
	if (z.type == slang_spec_struct)
	{
		z._struct = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
		if (z._struct == NULL)
		{
			slang_type_specifier_destruct (&z);
			return 0;
		}
		if (!slang_struct_construct (z._struct))
		{
			slang_alloc_free (z._struct);
			slang_type_specifier_destruct (&z);
			return 0;
		}
		if (!slang_struct_copy (z._struct, y->_struct))
		{
			slang_type_specifier_destruct (&z);
			return 0;
		}
	}
	else if (z.type == slang_spec_array)
	{
		z._array = (slang_type_specifier *) slang_alloc_malloc (sizeof (slang_type_specifier));
		if (z._array == NULL)
		{
			slang_type_specifier_destruct (&z);
			return 0;
		}
		if (!slang_type_specifier_construct (z._array))
		{
			slang_alloc_free (z._array);
			slang_type_specifier_destruct (&z);
			return 0;
		}
		if (!slang_type_specifier_copy (z._array, y->_array))
		{
			slang_type_specifier_destruct (&z);
			return 0;
		}
	}
	slang_type_specifier_destruct (x);
	*x = z;
	return 1;
}

int slang_type_specifier_equal (const slang_type_specifier *x, const slang_type_specifier *y)
{
	if (x->type != y->type)
		return 0;
	if (x->type == slang_spec_struct)
		return slang_struct_equal (x->_struct, y->_struct);
	if (x->type == slang_spec_array)
		return slang_type_specifier_equal (x->_array, y->_array);
	return 1;
}

/* slang_fully_specified_type */

int slang_fully_specified_type_construct (slang_fully_specified_type *type)
{
	type->qualifier = slang_qual_none;
	if (!slang_type_specifier_construct (&type->specifier))
		return 0;
	return 1;
}

void slang_fully_specified_type_destruct (slang_fully_specified_type *type)
{
	slang_type_specifier_destruct (&type->specifier);
}

int slang_fully_specified_type_copy (slang_fully_specified_type *x, const slang_fully_specified_type *y)
{
	slang_fully_specified_type z;

	if (!slang_fully_specified_type_construct (&z))
		return 0;
	z.qualifier = y->qualifier;
	if (!slang_type_specifier_copy (&z.specifier, &y->specifier))
	{
		slang_fully_specified_type_destruct (&z);
		return 0;
	}
	slang_fully_specified_type_destruct (x);
	*x = z;
	return 1;
}

/* slang_variable_scope */

int slang_variable_scope_construct (slang_variable_scope *scope)
{
	scope->variables = NULL;
	scope->num_variables = 0;
	scope->outer_scope = NULL;
	return 1;
}

void slang_variable_scope_destruct (slang_variable_scope *scope)
{
	unsigned int i;

	for (i = 0; i < scope->num_variables; i++)
		slang_variable_destruct (scope->variables + i);
	slang_alloc_free (scope->variables);
	/* do not free scope->outer_scope */
}

int slang_variable_scope_copy (slang_variable_scope *x, const slang_variable_scope *y)
{
	slang_variable_scope z;
	unsigned int i;

	if (!slang_variable_scope_construct (&z))
		return 0;
	z.variables = (slang_variable *) slang_alloc_malloc (y->num_variables * sizeof (slang_variable));
	if (z.variables == NULL)
	{
		slang_variable_scope_destruct (&z);
		return 0;
	}
	for (z.num_variables = 0; z.num_variables < y->num_variables; z.num_variables++)
		if (!slang_variable_construct (&z.variables[z.num_variables]))
		{
			slang_variable_scope_destruct (&z);
			return 0;
		}
	for (i = 0; i < z.num_variables; i++)
		if (!slang_variable_copy (&z.variables[i], &y->variables[i]))
		{
			slang_variable_scope_destruct (&z);
			return 0;
		}
	z.outer_scope = y->outer_scope;
	slang_variable_scope_destruct (x);
	*x = z;
	return 1;
}

/* slang_variable */

int slang_variable_construct (slang_variable *var)
{
	if (!slang_fully_specified_type_construct (&var->type))
		return 0;
	var->name = NULL;
	var->array_size = NULL;
	var->initializer = NULL;
	var->address = ~0;
	return 1;
}

void slang_variable_destruct (slang_variable *var)
{
	slang_fully_specified_type_destruct (&var->type);
	slang_alloc_free (var->name);
	if (var->array_size != NULL)
	{
		slang_operation_destruct (var->array_size);
		slang_alloc_free (var->array_size);
	}
	if (var->initializer != NULL)
	{
		slang_operation_destruct (var->initializer);
		slang_alloc_free (var->initializer);
	}
}

int slang_variable_copy (slang_variable *x, const slang_variable *y)
{
	slang_variable z;

	if (!slang_variable_construct (&z))
		return 0;
	if (!slang_fully_specified_type_copy (&z.type, &y->type))
	{
		slang_variable_destruct (&z);
		return 0;
	}
	if (y->name != NULL)
	{
		z.name = slang_string_duplicate (y->name);
		if (z.name == NULL)
		{
			slang_variable_destruct (&z);
			return 0;
		}
	}
	if (y->array_size != NULL)
	{
		z.array_size = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (z.array_size == NULL)
		{
			slang_variable_destruct (&z);
			return 0;
		}
		if (!slang_operation_construct (z.array_size))
		{
			slang_alloc_free (z.array_size);
			slang_variable_destruct (&z);
			return 0;
		}
		if (!slang_operation_copy (z.array_size, y->array_size))
		{
			slang_variable_destruct (&z);
			return 0;
		}
	}
	if (y->initializer != NULL)
	{
		z.initializer = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (z.initializer == NULL)
		{
			slang_variable_destruct (&z);
			return 0;
		}
		if (!slang_operation_construct (z.initializer))
		{
			slang_alloc_free (z.initializer);
			slang_variable_destruct (&z);
			return 0;
		}
		if (!slang_operation_copy (z.initializer, y->initializer))
		{
			slang_variable_destruct (&z);
			return 0;
		}
	}
	slang_variable_destruct (x);
	*x = z;
	return 1;
}

slang_variable *_slang_locate_variable (slang_variable_scope *scope, const char *name, int all)
{
	unsigned int i;

	for (i = 0; i < scope->num_variables; i++)
		if (slang_string_compare (name, scope->variables[i].name) == 0)
			return scope->variables + i;
	if (all && scope->outer_scope != NULL)
		return _slang_locate_variable (scope->outer_scope, name, 1);
	return NULL;
}

