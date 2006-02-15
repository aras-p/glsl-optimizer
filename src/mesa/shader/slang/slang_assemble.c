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
 * \file slang_assemble.c
 * slang intermediate code assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble.h"
/*#include "slang_compile.h"*/
#include "slang_storage.h"
/*#include "slang_assemble_constructor.h"*/
#include "slang_assemble_typeinfo.h"
#include "slang_assemble_conditional.h"
#include "slang_assemble_assignment.h"
#include "slang_execute.h"

/* slang_assembly */

static int slang_assembly_construct (slang_assembly *assem)
{
	assem->type = slang_asm_none;
	return 1;
}

static void slang_assembly_destruct (slang_assembly *assem)
{
}

/* slang_assembly_file */

int slang_assembly_file_construct (slang_assembly_file *file)
{
	file->code = NULL;
	file->count = 0;
	file->capacity = 0;
	return 1;
}

void slang_assembly_file_destruct (slang_assembly_file *file)
{
	unsigned int i;

	for (i = 0; i < file->count; i++)
		slang_assembly_destruct (&file->code[i]);
	slang_alloc_free (file->code);
}

static int push_new (slang_assembly_file *file)
{
	if (file->count == file->capacity)
	{
		unsigned int n;

		if (file->capacity == 0)
			n = 256;
		else
			n = file->capacity * 2;
		file->code = (slang_assembly *) slang_alloc_realloc (file->code,
			file->capacity * sizeof (slang_assembly), n * sizeof (slang_assembly));
		if (file->code == NULL)
			return 0;
		file->capacity = n;
	}
	if (!slang_assembly_construct (&file->code[file->count]))
		return 0;
	file->count++;
	return 1;
}

static int push_gen (slang_assembly_file *file, slang_assembly_type type, GLfloat literal,
	GLuint label, GLuint size)
{
	slang_assembly *assem;

	if (!push_new (file))
		return 0;
	assem = &file->code[file->count - 1];
	assem->type = type;
	assem->literal = literal;
	assem->param[0] = label;
	assem->param[1] = size;
	return 1;
}

int slang_assembly_file_push (slang_assembly_file *file, slang_assembly_type type)
{
	return push_gen (file, type, (GLfloat) 0, 0, 0);
}

int slang_assembly_file_push_label (slang_assembly_file *file, slang_assembly_type type,
	GLuint label)
{
	return push_gen (file, type, (GLfloat) 0, label, 0);
}

int slang_assembly_file_push_label2 (slang_assembly_file *file, slang_assembly_type type,
	GLuint label1, GLuint label2)
{
	return push_gen (file, type, (GLfloat) 0, label1, label2);
}

int slang_assembly_file_push_literal (slang_assembly_file *file, slang_assembly_type type,
	GLfloat literal)
{
	return push_gen (file, type, literal, 0, 0);
}

#define PUSH slang_assembly_file_push
#define PLAB slang_assembly_file_push_label
#define PLAB2 slang_assembly_file_push_label2
#define PLIT slang_assembly_file_push_literal

/* slang_assembly_file_restore_point */

int slang_assembly_file_restore_point_save (slang_assembly_file *file,
	slang_assembly_file_restore_point *point)
{
	point->count = file->count;
	return 1;
}

int slang_assembly_file_restore_point_load (slang_assembly_file *file,
	slang_assembly_file_restore_point *point)
{
	unsigned int i;

	for (i = point->count; i < file->count; i++)
		slang_assembly_destruct (&file->code[i]);
	file->count = point->count;
	return 1;
}

/* utility functions */

static int sizeof_variable (slang_type_specifier *spec, slang_type_qualifier qual,
	slang_operation *array_size, slang_assembly_name_space *space, unsigned int *size,
	slang_machine *mach, slang_assembly_file *pfile, slang_atom_pool *atoms)
{
	slang_storage_aggregate agg;

	/* calculate the size of the variable's aggregate */
	if (!slang_storage_aggregate_construct (&agg))
		return 0;
	if (!_slang_aggregate_variable (&agg, spec, array_size, space->funcs, space->structs,
			space->vars, mach, pfile, atoms))
	{
		slang_storage_aggregate_destruct (&agg);
		return 0;
	}
	*size += _slang_sizeof_aggregate (&agg);
	slang_storage_aggregate_destruct (&agg);

	/* for reference variables consider the additional address overhead */
	if (qual == slang_qual_out || qual == slang_qual_inout)
		*size += 4;
	return 1;
}

static int sizeof_variable2 (slang_variable *var, slang_assembly_name_space *space,
	unsigned int *size, slang_machine *mach, slang_assembly_file *pfile, slang_atom_pool *atoms)
{
	var->address = *size;
	if (var->type.qualifier == slang_qual_out || var->type.qualifier == slang_qual_inout)
		var->address += 4;
	return sizeof_variable (&var->type.specifier, var->type.qualifier, var->array_size, space,
		size, mach, pfile, atoms);
}

static int sizeof_variables (slang_variable_scope *vars, unsigned int start, unsigned int stop,
	slang_assembly_name_space *space, unsigned int *size, slang_machine *mach,
	slang_assembly_file *pfile, slang_atom_pool *atoms)
{
	unsigned int i;

	for (i = start; i < stop; i++)
		if (!sizeof_variable2 (&vars->variables[i], space, size, mach, pfile, atoms))
			return 0;
	return 1;
}

static int collect_locals (slang_operation *op, slang_assembly_name_space *space,
	unsigned int *size, slang_machine *mach, slang_assembly_file *pfile, slang_atom_pool *atoms)
{
	unsigned int i;

	if (!sizeof_variables (op->locals, 0, op->locals->num_variables, space, size, mach, pfile,
			atoms))
		return 0;
	for (i = 0; i < op->num_children; i++)
		if (!collect_locals (&op->children[i], space, size, mach, pfile, atoms))
			return 0;
	return 1;
}

/* _slang_locate_function() */

slang_function *_slang_locate_function (slang_function_scope *funcs, slang_atom a_name,
	slang_operation *params, unsigned int num_params, slang_assembly_name_space *space,
	slang_atom_pool *atoms)
{
	unsigned int i;

	for (i = 0; i < funcs->num_functions; i++)
	{
		unsigned int j;
		slang_function *f = &funcs->functions[i];

		if (a_name != f->header.a_name)
			continue;
		if (f->param_count != num_params)
			continue;
		for (j = 0; j < num_params; j++)
		{
			slang_assembly_typeinfo ti;

			if (!slang_assembly_typeinfo_construct (&ti))
				return 0;
			if (!_slang_typeof_operation (&params[j], space, &ti, atoms))
			{
				slang_assembly_typeinfo_destruct (&ti);
				return 0;
			}
			if (!slang_type_specifier_equal (&ti.spec, &f->parameters->variables[j].type.specifier))
			{
				slang_assembly_typeinfo_destruct (&ti);
				break;
			}
			slang_assembly_typeinfo_destruct (&ti);

			/* "out" and "inout" formal parameter requires the actual parameter to be l-value */
			if (!ti.can_be_referenced &&
					(f->parameters->variables[j].type.qualifier == slang_qual_out ||
					f->parameters->variables[j].type.qualifier == slang_qual_inout))
				break;
		}
		if (j == num_params)
			return f;
	}
	if (funcs->outer_scope != NULL)
		return _slang_locate_function (funcs->outer_scope, a_name, params, num_params, space, atoms);
	return NULL;
}

/* _slang_assemble_function() */

int _slang_assemble_function (slang_assemble_ctx *A, slang_function *fun)
{
	unsigned int param_size, local_size;
	unsigned int skip, cleanup;

	fun->address = A->file->count;

	if (fun->body == NULL)
	{
		/* jump to the actual function body - we do not know it, so add the instruction
		 * to fixup table */
		fun->fixups.table = (GLuint *) slang_alloc_realloc (fun->fixups.table,
			fun->fixups.count * sizeof (GLuint), (fun->fixups.count + 1) * sizeof (GLuint));
		if (fun->fixups.table == NULL)
			return 0;
		fun->fixups.table[fun->fixups.count] = fun->address;
		fun->fixups.count++;
		if (!PUSH (A->file, slang_asm_jump))
			return 0;
		return 1;
	}
	else
	{
		GLuint i;

		/* resolve all fixup table entries and delete it */
		for (i = 0; i < fun->fixups.count; i++)
			A->file->code[fun->fixups.table[i]].param[0] = fun->address;
		slang_fixup_table_free (&fun->fixups);
	}

	/* At this point traverse function formal parameters and code to calculate
	 * total memory size to be allocated on the stack.
	 * During this process the variables will be assigned local addresses to
	 * reference them in the code.
	 * No storage optimizations are performed so exclusive scopes are not detected and shared. */

	/* calculate return value size */
	param_size = 0;
	if (fun->header.type.specifier.type != slang_spec_void)
		if (!sizeof_variable (&fun->header.type.specifier, slang_qual_none, NULL, &A->space,
				&param_size, A->mach, A->file, A->atoms))
			return 0;
	A->local.ret_size = param_size;

	/* calculate formal parameter list size */
	if (!sizeof_variables (fun->parameters, 0, fun->param_count, &A->space, &param_size, A->mach, A->file,
			A->atoms))
		return 0;

	/* calculate local variables size - take into account the four-byte return address and
	 * temporaries for various tasks (4 for addr and 16 for swizzle temporaries).
	 * these include variables from the formal parameter scope and from the code */
	A->local.addr_tmp = param_size + 4;
	A->local.swizzle_tmp = param_size + 4 + 4;
	local_size = param_size + 4 + 4 + 16;
	if (!sizeof_variables (fun->parameters, fun->param_count, fun->parameters->num_variables, &A->space,
			&local_size, A->mach, A->file, A->atoms))
		return 0;
	if (!collect_locals (fun->body, &A->space, &local_size, A->mach, A->file, A->atoms))
		return 0;

	/* allocate local variable storage */
	if (!PLAB (A->file, slang_asm_local_alloc, local_size - param_size - 4))
		return 0;

	/* mark a new frame for function variable storage */
	if (!PLAB (A->file, slang_asm_enter, local_size))
		return 0;

	/* jump directly to the actual code */
	skip = A->file->count;
	if (!push_new (A->file))
		return 0;
	A->file->code[skip].type = slang_asm_jump;

	/* all "return" statements will be directed here */
	A->flow.function_end = A->file->count;
	cleanup = A->file->count;
	if (!push_new (A->file))
		return 0;
	A->file->code[cleanup].type = slang_asm_jump;

	/* execute the function body */
	A->file->code[skip].param[0] = A->file->count;
	if (!_slang_assemble_operation_ (A, fun->body, slang_ref_freelance))
		return 0;

	/* this is the end of the function - restore the old function frame */
	A->file->code[cleanup].param[0] = A->file->count;
	if (!PUSH (A->file, slang_asm_leave))
		return 0;

	/* free local variable storage */
	if (!PLAB (A->file, slang_asm_local_free, local_size - param_size - 4))
		return 0;

	/* return from the function */
	if (!PUSH (A->file, slang_asm_return))
		return 0;
	return 1;
}

int _slang_cleanup_stack (slang_assembly_file *file, slang_operation *op, int ref,
	slang_assembly_name_space *space, struct slang_machine_ *mach, slang_atom_pool *atoms)
{
	slang_assemble_ctx A;

	A.file = file;
	A.mach = mach;
	A.atoms = atoms;
	A.space = *space;
	return _slang_cleanup_stack_ (&A, op);
}

int _slang_cleanup_stack_ (slang_assemble_ctx *A, slang_operation *op)
{
	slang_assembly_typeinfo ti;
	unsigned int size = 0;

	/* get type info of the operation and calculate its size */
	if (!slang_assembly_typeinfo_construct (&ti))
		return 0;
	if (!_slang_typeof_operation (op, &A->space, &ti, A->atoms))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}
	if (A->ref == slang_ref_force)
		size = 4;
	else if (ti.spec.type != slang_spec_void)
		if (!sizeof_variable (&ti.spec, slang_qual_none, NULL, &A->space, &size, A->mach, A->file, A->atoms))
		{
			slang_assembly_typeinfo_destruct (&ti);
			return 0;
		}
	slang_assembly_typeinfo_destruct (&ti);

	/* if nonzero, free it from the stack */
	if (size != 0)
	{
		if (!PLAB (A->file, slang_asm_local_free, size))
			return 0;
	}
	return 1;
}

/* _slang_assemble_operation() */

static int dereference_aggregate (slang_assembly_file *file, const slang_storage_aggregate *agg,
	unsigned int *size, slang_assembly_local_info *info, slang_swizzle *swz, int is_swizzled)
{
	unsigned int i;

	for (i = agg->count; i > 0; i--)
	{
		const slang_storage_array *arr = &agg->arrays[i - 1];
		unsigned int j;

		for (j = arr->length; j > 0; j--)
		{
			if (arr->type == slang_stor_aggregate)
			{
				if (!dereference_aggregate (file, arr->aggregate, size, info, swz, is_swizzled))
					return 0;
			}
			else
			{
				unsigned int src_offset;
				slang_assembly_type ty;

				*size -= 4;

				/* calculate the offset within source variable to read */
				if (is_swizzled)
				{
					/* swizzle the index to get the actual offset */
					src_offset = swz->swizzle[*size / 4] * 4;
				}
				else
				{
					/* no swizzling - read sequentially */
					src_offset = *size;
				}

				/* dereference data slot of a basic type */
				if (!PLAB2 (file, slang_asm_local_addr, info->addr_tmp, 4))
					return 0;
				if (!PUSH (file, slang_asm_addr_deref))
					return 0;
				if (!PLAB (file, slang_asm_addr_push, src_offset))
					return 0;
				if (!PUSH (file, slang_asm_addr_add))
					return 0;

				switch (arr->type)
				{
				case slang_stor_bool:
					ty = slang_asm_bool_deref;
					break;
				case slang_stor_int:
					ty = slang_asm_int_deref;
					break;
				case slang_stor_float:
					ty = slang_asm_float_deref;
					break;
				default:
					_mesa_problem(NULL, "Unexpected arr->type in dereference_aggregate");
					ty = slang_asm_none;
				}
				if (!PUSH (file, ty))
					return 0;
			}
		}
	}
	return 1;
}

int _slang_dereference (slang_assembly_file *file, slang_operation *op,
	slang_assembly_name_space *space, slang_assembly_local_info *info, slang_machine *mach,
	slang_atom_pool *atoms)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg;
	unsigned int size;

	/* get type information of the given operation */
	if (!slang_assembly_typeinfo_construct (&ti))
		return 0;
	if (!_slang_typeof_operation (op, space, &ti, atoms))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	/* construct aggregate from the type info */
	if (!slang_storage_aggregate_construct (&agg))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}
	if (!_slang_aggregate_variable (&agg, &ti.spec, ti.array_size, space->funcs, space->structs,
			space->vars, mach, file, atoms))
	{
		slang_storage_aggregate_destruct (&agg);
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	/* dereference the resulting aggregate */
	size = _slang_sizeof_aggregate (&agg);
	result = dereference_aggregate (file, &agg, &size, info, &ti.swz, ti.is_swizzled);

	slang_storage_aggregate_destruct (&agg);
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

int _slang_assemble_function_call (slang_assemble_ctx *A, slang_function *fun,
	slang_operation *params, GLuint param_count, GLboolean assignment)
{
	unsigned int i;
	slang_assembly_stack_info p_stk[64];

	/* TODO: fix this, allocate dynamically */
	if (param_count > 64)
		return 0;

	/* make room for the return value, if any */
	if (fun->header.type.specifier.type != slang_spec_void)
	{
		unsigned int ret_size = 0;

		if (!sizeof_variable (&fun->header.type.specifier, slang_qual_none, NULL, &A->space,
				&ret_size, A->mach, A->file, A->atoms))
			return 0;
		if (!PLAB (A->file, slang_asm_local_alloc, ret_size))
			return 0;
	}

	/* push the actual parameters on the stack */
	for (i = 0; i < param_count; i++)
	{
		if (fun->parameters->variables[i].type.qualifier == slang_qual_inout ||
			fun->parameters->variables[i].type.qualifier == slang_qual_out)
		{
			if (!PLAB2 (A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
				return 0;
			/* TODO: optimize the "out" parameter case */
			if (!_slang_assemble_operation_ (A, &params[i], slang_ref_force))
				return 0;
			p_stk[i] = A->swz;
			if (!PUSH (A->file, slang_asm_addr_copy))
				return 0;
			if (!PUSH (A->file, slang_asm_addr_deref))
				return 0;
			if (i == 0 && assignment)
			{
				/* duplicate the resulting address */
				if (!PLAB2 (A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
					return 0;
				if (!PUSH (A->file, slang_asm_addr_deref))
					return 0;
			}
			if (!_slang_dereference (A->file, &params[i], &A->space, &A->local, A->mach, A->atoms))
				return 0;
		}
		else
		{
			if (!_slang_assemble_operation_ (A, &params[i], slang_ref_forbid))
				return 0;
			p_stk[i] = A->swz;
		}
	}

	/* call the function */
	if (!PLAB (A->file, slang_asm_call, fun->address))
		return 0;

	/* pop the parameters from the stack */
	for (i = param_count; i > 0; i--)
	{
		unsigned int j = i - 1;

		if (fun->parameters->variables[j].type.qualifier == slang_qual_inout ||
			fun->parameters->variables[j].type.qualifier == slang_qual_out)
		{
			/* for output parameter copy the contents of the formal parameter
			 * back to the original actual parameter */
			A->swz = p_stk[j];
			if (!_slang_assemble_assignment (A, &params[j]))
				return 0;
			/* pop the actual parameter's address */
			if (!PLAB (A->file, slang_asm_local_free, 4))
				return 0;
		}
		else
		{
			/* pop the value of the parameter */
			if (!_slang_cleanup_stack_ (A, &params[j]))
				return 0;
		}
	}

	return 1;
}

int _slang_assemble_function_call_name (slang_assemble_ctx *A, const char *name,
	slang_operation *params, GLuint param_count, GLboolean assignment)
{
	slang_atom atom;
	slang_function *fun;

	atom = slang_atom_pool_atom (A->atoms, name);
	if (atom == SLANG_ATOM_NULL)
		return 0;
	fun = _slang_locate_function (A->space.funcs, atom, params, param_count, &A->space, A->atoms);
	if (fun == NULL)
		return 0;
	return _slang_assemble_function_call (A, fun, params, param_count, assignment);
}

static int assemble_function_call_name_dummyint (slang_assemble_ctx *A, const char *name,
	slang_operation *params)
{
	slang_operation p[2];
	int result;

	p[0] = params[0];
	if (!slang_operation_construct (&p[1]))
		return 0;
	p[1].type = slang_oper_literal_int;
	result = _slang_assemble_function_call_name (A, name, p, 2, GL_FALSE);
	slang_operation_destruct (&p[1]);
	return result;
}

static const struct
{
	const char *name;
	slang_assembly_type code1, code2;
} inst[] = {
	/* core */
	{ "float_add",      slang_asm_float_add,      slang_asm_float_copy },
	{ "float_multiply", slang_asm_float_multiply, slang_asm_float_copy },
	{ "float_divide",   slang_asm_float_divide,   slang_asm_float_copy },
	{ "float_negate",   slang_asm_float_negate,   slang_asm_float_copy },
	{ "float_less",     slang_asm_float_less,     slang_asm_bool_copy },
	{ "float_equal",    slang_asm_float_equal_exp,slang_asm_bool_copy },
	{ "float_to_int",   slang_asm_float_to_int,   slang_asm_int_copy },
	{ "float_sine",     slang_asm_float_sine,     slang_asm_float_copy },
	{ "float_arcsine",  slang_asm_float_arcsine,  slang_asm_float_copy },
	{ "float_arctan",   slang_asm_float_arctan,   slang_asm_float_copy },
	{ "float_power",    slang_asm_float_power,    slang_asm_float_copy },
	{ "float_log2",     slang_asm_float_log2,     slang_asm_float_copy },
	{ "float_floor",    slang_asm_float_floor,    slang_asm_float_copy },
	{ "float_ceil",     slang_asm_float_ceil,     slang_asm_float_copy },
	{ "int_to_float",   slang_asm_int_to_float,   slang_asm_float_copy },
	/* mesa-specific extensions */
	{ "float_print",    slang_asm_float_deref,    slang_asm_float_print },
	{ "int_print",      slang_asm_int_deref,      slang_asm_int_print },
	{ "bool_print",     slang_asm_bool_deref,     slang_asm_bool_print },
	{ NULL,             slang_asm_none,           slang_asm_none }
};

static int call_asm_instruction (slang_assembly_file *file, slang_atom a_name, slang_atom_pool *atoms)
{
	const char *id;
	unsigned int i;

	id = slang_atom_pool_id (atoms, a_name);

	for (i = 0; inst[i].name != NULL; i++)
		if (slang_string_compare (id, inst[i].name) == 0)
			break;
	if (inst[i].name == NULL)
		return 0;

	if (!PLAB2 (file, inst[i].code1, 4, 0))
		return 0;
	if (inst[i].code2 != slang_asm_none)
		if (!PLAB2 (file, inst[i].code2, 4, 0))
			return 0;

	/* clean-up the stack from the remaining dst address */
	if (!PLAB (file, slang_asm_local_free, 4))
		return 0;

	return 1;
}

static int equality_aggregate (slang_assembly_file *file, const slang_storage_aggregate *agg,
	unsigned int *index, unsigned int size, slang_assembly_local_info *info, unsigned int z_label)
{
	unsigned int i;

	for (i = 0; i < agg->count; i++)
	{
		const slang_storage_array *arr = &agg->arrays[i];
		unsigned int j;

		for (j = 0; j < arr->length; j++)
		{
			if (arr->type == slang_stor_aggregate)
			{
				if (!equality_aggregate (file, arr->aggregate, index, size, info, z_label))
					return 0;
			}
			else
			{
				if (!PLAB2 (file, slang_asm_float_equal_int, size + *index, *index))
					return 0;
				*index += 4;
				if (!PLAB (file, slang_asm_jump_if_zero, z_label))
					return 0;
			}
		}
	}
	return 1;
}

static int equality (slang_assembly_file *file, slang_operation *op,
	slang_assembly_name_space *space, slang_assembly_local_info *info, int equal,
	slang_machine *mach, slang_atom_pool *atoms)
{
	slang_assembly_typeinfo ti;
	int result = 0;
	slang_storage_aggregate agg;
	unsigned int index, size;
	unsigned int skip_jump, true_label, true_jump, false_label, false_jump;

	/* get type of operation */
	if (!slang_assembly_typeinfo_construct (&ti))
		return 0;
	if (!_slang_typeof_operation (op, space, &ti, atoms))
		goto end1;

	/* convert it to an aggregate */
	if (!slang_storage_aggregate_construct (&agg))
		goto end1;
	if (!_slang_aggregate_variable (&agg, &ti.spec, NULL, space->funcs, space->structs,
			space->vars, mach, file, atoms))
		goto end;

	/* compute the size of the agregate - there are two such aggregates on the stack */
	size = _slang_sizeof_aggregate (&agg);

	/* jump to the actual data-comparison code */
	skip_jump = file->count;
	if (!PUSH (file, slang_asm_jump))
		goto end;

	/* pop off the stack the compared data and push 1 */
	true_label = file->count;
	if (!PLAB (file, slang_asm_local_free, size * 2))
		goto end;
	if (!PLIT (file, slang_asm_bool_push, (GLfloat) 1))
		goto end;
	true_jump = file->count;
	if (!PUSH (file, slang_asm_jump))
		goto end;

	false_label = file->count;
	if (!PLAB (file, slang_asm_local_free, size * 2))
		goto end;
	if (!PLIT (file, slang_asm_bool_push, (GLfloat) 0))
		goto end;
	false_jump = file->count;
	if (!PUSH (file, slang_asm_jump))
		goto end;

	file->code[skip_jump].param[0] = file->count;

	/* compare the data on stack, it will eventually jump either to true or false label */
	index = 0;
	if (!equality_aggregate (file, &agg, &index, size, info, equal ? false_label : true_label))
		goto end;
	if (!PLAB (file, slang_asm_jump, equal ? true_label : false_label))
		goto end;

	file->code[true_jump].param[0] = file->count;
	file->code[false_jump].param[0] = file->count;

	result = 1;
end:
	slang_storage_aggregate_destruct (&agg);
end1:
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

static int handle_subscript (slang_assembly_typeinfo *tie, slang_assembly_typeinfo *tia,
	slang_assembly_file *file, slang_operation *op, int reference, slang_assembly_flow_control *flow,
	slang_assembly_name_space *space, slang_assembly_local_info *info, slang_machine *mach,
	slang_atom_pool *atoms)
{
	unsigned int asize = 0, esize = 0;
	slang_assembly_stack_info _stk;

	/* get type info of the master expression (matrix, vector or an array */
	if (!_slang_typeof_operation (&op->children[0], space, tia, atoms))
		return 0;
	if (!sizeof_variable (&tia->spec, slang_qual_none, tia->array_size, space, &asize, mach, file,
			atoms))
		return 0;

	/* get type info of the result (matrix column, vector row or array element) */
	if (!_slang_typeof_operation (op, space, tie, atoms))
		return 0;
	if (!sizeof_variable (&tie->spec, slang_qual_none, NULL, space, &esize, mach, file, atoms))
		return 0;

	/* assemble the master expression */
	if (!_slang_assemble_operation (file, &op->children[0], reference, flow, space, info, &_stk,
			mach, atoms))
		return 0;
	/* ignre the _stk */

	/* when indexing an l-value swizzle, push the swizzle_tmp */
	if (reference && tia->is_swizzled)
	{
		if (!PLAB2 (file, slang_asm_local_addr, info->swizzle_tmp, 16))
			return 0;
	}

	/* assemble the subscript expression */
	if (!_slang_assemble_operation (file, &op->children[1], 0, flow, space, info, &_stk, mach, atoms))
		return 0;
	/* ignore the _stk */

	if (reference && tia->is_swizzled)
	{
		unsigned int i;

		/* copy the swizzle indexes to the swizzle_tmp */
		for (i = 0; i < tia->swz.num_components; i++)
		{
			if (!PLAB2 (file, slang_asm_local_addr, info->swizzle_tmp, 16))
				return 0;
			if (!PLAB (file, slang_asm_addr_push, i * 4))
				return 0;
			if (!PUSH (file, slang_asm_addr_add))
				return 0;
			if (!PLAB (file, slang_asm_addr_push, tia->swz.swizzle[i]))
				return 0;
			if (!PUSH (file, slang_asm_addr_copy))
				return 0;
			if (!PLAB (file, slang_asm_local_free, 4))
				return 0;
		}

		/* offset the pushed swizzle_tmp address and dereference it */
		if (!PUSH (file, slang_asm_int_to_addr))
			return 0;
		if (!PLAB (file, slang_asm_addr_push, 4))
			return 0;
		if (!PUSH (file, slang_asm_addr_multiply))
			return 0;
		if (!PUSH (file, slang_asm_addr_add))
			return 0;
		if (!PUSH (file, slang_asm_addr_deref))
			return 0;
	}
	else
	{
		/* convert the integer subscript to a relative address */
		if (!PUSH (file, slang_asm_int_to_addr))
			return 0;
	}

	if (!PLAB (file, slang_asm_addr_push, esize))
		return 0;
	if (!PUSH (file, slang_asm_addr_multiply))
		return 0;

	if (reference)
	{
		/* offset the base address with the relative address */
		if (!PUSH (file, slang_asm_addr_add))
			return 0;
	}
	else
	{
		unsigned int i;

		/* move the selected element to the beginning of the master expression */
		for (i = 0; i < esize; i += 4)
			if (!PLAB2 (file, slang_asm_float_move, asize - esize + i + 4, i + 4))
				return 0;
		if (!PLAB (file, slang_asm_local_free, 4))
			return 0;

		/* free the rest of the master expression */
		if (!PLAB (file, slang_asm_local_free, asize - esize))
			return 0;
	}
	return 1;
}

static int handle_field (slang_assembly_typeinfo *tia, slang_assembly_typeinfo *tib,
	slang_assembly_file *file, slang_operation *op, int reference, slang_assembly_flow_control *flow,
	slang_assembly_name_space *space, slang_assembly_local_info *info, slang_assembly_stack_info *stk,
	slang_machine *mach, slang_atom_pool *atoms)
{
	slang_assembly_stack_info _stk;

	/* get type info of the result (field or swizzle) */
	if (!_slang_typeof_operation (op, space, tia, atoms))
		return 0;

	/* get type info of the master expression being accessed (struct or vector) */
	if (!_slang_typeof_operation (&op->children[0], space, tib, atoms))
		return 0;

	/* if swizzling a vector in-place, the swizzle temporary is needed */
	if (!reference && tia->is_swizzled)
		if (!PLAB2 (file, slang_asm_local_addr, info->swizzle_tmp, 16))
			return 0;

	/* assemble the master expression */
	if (!_slang_assemble_operation (file, &op->children[0], reference, flow, space, info, &_stk,
			mach, atoms))
		return 0;
	/* ignore _stk.swizzle - we'll have it in tia->swz */

	/* assemble the field expression */
	if (tia->is_swizzled)
	{
		if (reference)
		{
#if 0
			if (tia->swz.num_components == 1)
			{
				/* simple case - adjust the vector's address to point to the selected component */
				if (!PLAB (file, slang_asm_addr_push, tia->swz.swizzle[0] * 4))
					return 0;
				if (!PUSH (file, slang_asm_addr_add))
					return 0;
			}
			else
#endif
			{
				/* two or more vector components are being referenced - the so-called write mask
				 * must be passed to the upper operations and applied when assigning value
				 * to this swizzle */
				stk->swizzle = tia->swz;
			}
		}
		else
		{
			/* swizzle the vector in-place using the swizzle temporary */
			if (!_slang_assemble_constructor_from_swizzle (file, &tia->swz, &tia->spec, &tib->spec,
					info))
				return 0;
		}
	}
	else
	{
		GLuint i, struct_size = 0, field_offset = 0, field_size = 0;

		for (i = 0; i < tib->spec._struct->fields->num_variables; i++)
		{
			slang_variable *field;
			slang_storage_aggregate agg;
			GLuint size;

			field = &tib->spec._struct->fields->variables[i];
			if (!slang_storage_aggregate_construct (&agg))
				return 0;
			if (!_slang_aggregate_variable (&agg, &field->type.specifier, field->array_size,
				space->funcs, space->structs, space->vars, mach, file, atoms))
			{
				slang_storage_aggregate_destruct (&agg);
				return 0;
			}
			size = _slang_sizeof_aggregate (&agg);
			slang_storage_aggregate_destruct (&agg);

			if (op->a_id == field->a_name)
			{
				field_size = size;
				struct_size = field_offset + size;
			}
			else if (struct_size != 0)
				struct_size += size;
			else
				field_offset += size;
		}

		if (!PLAB (file, slang_asm_addr_push, field_offset))
			return 0;

		if (reference)
		{
			if (!PUSH (file, slang_asm_addr_add))
				return 0;
		}
		else
		{
			unsigned int i;

			/* move the selected element to the beginning of the master expression */
			for (i = 0; i < field_size; i += 4)
				if (!PLAB2 (file, slang_asm_float_move, struct_size - field_size + i + 4, i + 4))
					return 0;
			if (!PLAB (file, slang_asm_local_free, 4))
				return 0;

			/* free the rest of the master expression */
			if (!PLAB (file, slang_asm_local_free, struct_size - field_size))
				return 0;
		}
	}
	return 1;
}

int _slang_assemble_operation (slang_assembly_file *file, struct slang_operation_ *op, int reference,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space, slang_assembly_local_info *info,
	slang_assembly_stack_info *stk, struct slang_machine_ *mach, slang_atom_pool *atoms)
{
	slang_assemble_ctx A;

	A.file = file;
	A.mach = mach;
	A.atoms = atoms;
	A.space = *space;
	A.flow = *flow;
	A.local = *info;
	if (!_slang_assemble_operation_ (&A, op, reference ? slang_ref_force : slang_ref_forbid))
		return 0;
	*stk = A.swz;
	return 1;
}

int _slang_assemble_operation_ (slang_assemble_ctx *A, slang_operation *op, slang_ref_type ref)
{
	unsigned int assem;
	slang_assembly_stack_info swz;

	assem = A->file->count;
	if (!push_new (A->file))
		return 0;

if (ref == slang_ref_freelance)
ref = slang_ref_forbid;

	/* set default results */
	A->ref = (ref == slang_ref_freelance) ? slang_ref_force : ref;
	swz.swizzle.num_components = 0;

	switch (op->type)
	{
	case slang_oper_block_no_new_scope:
	case slang_oper_block_new_scope:
		{
			unsigned int i;

			for (i = 0; i < op->num_children; i++)
			{
				if (!_slang_assemble_operation_ (A, &op->children[i], slang_ref_freelance))
					return 0;
				if (!_slang_cleanup_stack_ (A, &op->children[i]))
					return 0;
			}
		}
		break;
	case slang_oper_variable_decl:
		{
			unsigned int i;

			for (i = 0; i < op->num_children; i++)
			{
				/* TODO: perform initialization of op->children[i] */
				/* TODO: clean-up stack */
			}
		}
		break;
	case slang_oper_asm:
		{
			unsigned int i;

			if (!_slang_assemble_operation_ (A, &op->children[0], slang_ref_force))
				return 0;
			for (i = 1; i < op->num_children; i++)
				if (!_slang_assemble_operation_ (A, &op->children[i], slang_ref_forbid))
					return 0;
			if (!call_asm_instruction (A->file, op->a_id, A->atoms))
				return 0;
		}
		break;
	case slang_oper_break:
		A->file->code[assem].type = slang_asm_jump;
		A->file->code[assem].param[0] = A->flow.loop_end;
		break;
	case slang_oper_continue:
		A->file->code[assem].type = slang_asm_jump;
		A->file->code[assem].param[0] = A->flow.loop_start;
		break;
	case slang_oper_discard:
		A->file->code[assem].type = slang_asm_discard;
		if (!PUSH (A->file, slang_asm_exit))
			return 0;
		break;
	case slang_oper_return:
		if (A->local.ret_size != 0)
		{
			/* push the result's address */
			if (!PLAB2 (A->file, slang_asm_local_addr, 0, A->local.ret_size))
				return 0;
			if (!_slang_assemble_operation_ (A, &op->children[0], slang_ref_forbid))
				return 0;

			A->swz.swizzle.num_components = 0;
			/* assign the operation to the function result (it was reserved on the stack) */
			if (!_slang_assemble_assignment (A, op->children))
				return 0;

			if (!PLAB (A->file, slang_asm_local_free, 4))
				return 0;
		}
		if (!PLAB (A->file, slang_asm_jump, A->flow.function_end))
			return 0;
		break;
	case slang_oper_expression:
		if (ref == slang_ref_force)
			return 0;
		if (!_slang_assemble_operation_ (A, &op->children[0], ref))
			return 0;
		break;
	case slang_oper_if:
		if (!_slang_assemble_if (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		break;
	case slang_oper_while:
		if (!_slang_assemble_while (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		break;
	case slang_oper_do:
		if (!_slang_assemble_do (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		break;
	case slang_oper_for:
		if (!_slang_assemble_for (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		break;
	case slang_oper_void:
		break;
	case slang_oper_literal_bool:
		if (ref == slang_ref_force)
			return 0;
		A->file->code[assem].type = slang_asm_bool_push;
		A->file->code[assem].literal = op->literal;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_literal_int:
		if (ref == slang_ref_force)
			return 0;
		A->file->code[assem].type = slang_asm_int_push;
		A->file->code[assem].literal = op->literal;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_literal_float:
		if (ref == slang_ref_force)
			return 0;
		A->file->code[assem].type = slang_asm_float_push;
		A->file->code[assem].literal = op->literal;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_identifier:
		{
			slang_variable *var;
			unsigned int size;

			/* find the variable and calculate its size */
			var = _slang_locate_variable (op->locals, op->a_id, 1);
			if (var == NULL)
				return 0;
			size = 0;
			if (!sizeof_variable (&var->type.specifier, slang_qual_none, var->array_size, &A->space,
					&size, A->mach, A->file, A->atoms))
				return 0;

			/* prepare stack for dereferencing */
			if (ref == slang_ref_forbid)
				if (!PLAB2 (A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
					return 0;

			/* push the variable's address */
			if (var->global)
			{
				if (!PLAB (A->file, slang_asm_addr_push, var->address))
					return 0;
			}
			else
			{
				if (!PLAB2 (A->file, slang_asm_local_addr, var->address, size))
					return 0;
			}

			/* perform the dereference */
			if (ref == slang_ref_forbid)
			{
				if (!PUSH (A->file, slang_asm_addr_copy))
					return 0;
				if (!PLAB (A->file, slang_asm_local_free, 4))
					return 0;
				if (!_slang_dereference (A->file, op, &A->space, &A->local, A->mach, A->atoms))
					return 0;
			}
		}
		break;
	case slang_oper_sequence:
		if (ref == slang_ref_force)
			return 0;
		if (!_slang_assemble_operation_ (A, &op->children[0], slang_ref_freelance))
			return 0;
		if (!_slang_cleanup_stack_ (A, &op->children[0]))
			return 0;
		if (!_slang_assemble_operation_ (A, &op->children[1], slang_ref_forbid))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_assign:
		if (!_slang_assemble_assign (A, op, "=", ref))
			return 0;
		break;
	case slang_oper_addassign:
		if (!_slang_assemble_assign (A, op, "+=", ref))
			return 0;
		break;
	case slang_oper_subassign:
		if (!_slang_assemble_assign (A, op, "-=", ref))
			return 0;
		break;
	case slang_oper_mulassign:
		if (!_slang_assemble_assign (A, op, "*=", ref))
			return 0;
		break;
	/*case slang_oper_modassign:*/
	/*case slang_oper_lshassign:*/
	/*case slang_oper_rshassign:*/
	/*case slang_oper_orassign:*/
	/*case slang_oper_xorassign:*/
	/*case slang_oper_andassign:*/
	case slang_oper_divassign:
		if (!_slang_assemble_assign (A, op, "/=", ref))
			return 0;
		break;
	case slang_oper_select:
		if (!_slang_assemble_select (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_logicalor:
		if (!_slang_assemble_logicalor (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_logicaland:
		if (!_slang_assemble_logicaland (A->file, op, &A->flow, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_logicalxor:
		if (!_slang_assemble_function_call_name (A, "^^", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	/*case slang_oper_bitor:*/
	/*case slang_oper_bitxor:*/
	/*case slang_oper_bitand:*/
	case slang_oper_less:
		if (!_slang_assemble_function_call_name (A, "<", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_greater:
		if (!_slang_assemble_function_call_name (A, ">", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_lessequal:
		if (!_slang_assemble_function_call_name (A, "<=", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_greaterequal:
		if (!_slang_assemble_function_call_name (A, ">=", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	/*case slang_oper_lshift:*/
	/*case slang_oper_rshift:*/
	case slang_oper_add:
		if (!_slang_assemble_function_call_name (A, "+", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_subtract:
		if (!_slang_assemble_function_call_name (A, "-", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_multiply:
		if (!_slang_assemble_function_call_name (A, "*", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	/*case slang_oper_modulus:*/
	case slang_oper_divide:
		if (!_slang_assemble_function_call_name (A, "/", op->children, 2, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_equal:
		if (!_slang_assemble_operation_ (A, &op->children[0], slang_ref_forbid))
			return 0;
		if (!_slang_assemble_operation_ (A, &op->children[1], slang_ref_forbid))
			return 0;
		if (!equality (A->file, op->children, &A->space, &A->local, 1, A->mach, A->atoms))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_notequal:
		if (!_slang_assemble_operation_ (A, &op->children[0], slang_ref_forbid))
			return 0;
		if (!_slang_assemble_operation_ (A, &op->children[1], slang_ref_forbid))
			return 0;
		if (!equality (A->file, op->children, &A->space, &A->local, 0, A->mach, A->atoms))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_preincrement:
		if (!_slang_assemble_assign (A, op, "++", ref))
			return 0;
		break;
	case slang_oper_predecrement:
		if (!_slang_assemble_assign (A, op, "--", ref))
			return 0;
		break;
	case slang_oper_plus:
		if (!_slang_dereference (A->file, op, &A->space, &A->local, A->mach, A->atoms))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_minus:
		if (!_slang_assemble_function_call_name (A, "-", op->children, 1, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	/*case slang_oper_complement:*/
	case slang_oper_not:
		if (!_slang_assemble_function_call_name (A, "!", op->children, 1, GL_FALSE))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_subscript:
		{
			slang_assembly_typeinfo ti_arr, ti_elem;

			if (!slang_assembly_typeinfo_construct (&ti_arr))
				return 0;
			if (!slang_assembly_typeinfo_construct (&ti_elem))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				return 0;
			}
			if (!handle_subscript (&ti_elem, &ti_arr, A->file, op, ref != slang_ref_forbid, &A->flow, &A->space, &A->local,
					A->mach, A->atoms))
			{
				slang_assembly_typeinfo_destruct (&ti_arr);
				slang_assembly_typeinfo_destruct (&ti_elem);
				return 0;
			}
			slang_assembly_typeinfo_destruct (&ti_arr);
			slang_assembly_typeinfo_destruct (&ti_elem);
		}
		break;
	case slang_oper_call:
		{
			slang_function *fun;

			fun = _slang_locate_function (A->space.funcs, op->a_id, op->children, op->num_children,
				&A->space, A->atoms);
			if (fun == NULL)
			{
/*				if (!_slang_assemble_constructor (file, op, flow, space, info, mach))
*/					return 0;
			}
			else
			{
				if (!_slang_assemble_function_call (A, fun, op->children, op->num_children, GL_FALSE))
					return 0;
			}
			A->ref = slang_ref_forbid;
		}
		break;
	case slang_oper_field:
		{
			slang_assembly_typeinfo ti_after, ti_before;

			if (!slang_assembly_typeinfo_construct (&ti_after))
				return 0;
			if (!slang_assembly_typeinfo_construct (&ti_before))
			{
				slang_assembly_typeinfo_destruct (&ti_after);
				return 0;
			}
			if (!handle_field (&ti_after, &ti_before, A->file, op, ref != slang_ref_forbid, &A->flow, &A->space, &A->local, &swz,
					A->mach, A->atoms))
			{
				slang_assembly_typeinfo_destruct (&ti_after);
				slang_assembly_typeinfo_destruct (&ti_before);
				return 0;
			}
			slang_assembly_typeinfo_destruct (&ti_after);
			slang_assembly_typeinfo_destruct (&ti_before);
		}
		break;
	case slang_oper_postincrement:
		if (!assemble_function_call_name_dummyint (A, "++", op->children))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	case slang_oper_postdecrement:
		if (!assemble_function_call_name_dummyint (A, "--", op->children))
			return 0;
		A->ref = slang_ref_forbid;
		break;
	default:
		return 0;
	}

	A->swz = swz;

	return 1;
}

