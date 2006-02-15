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
 * \file slang_assemble_assignment.c
 * slang assignment expressions assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble_assignment.h"
#include "slang_assemble_typeinfo.h"
#include "slang_storage.h"
#include "slang_execute.h"

/*
	_slang_assemble_assignment()

	copies values on the stack (<component 0> to <component N-1>) to a memory
	location pointed by <addr of variable>;

	in:
		+------------------+
		| addr of variable |
		+------------------+
		| component N-1    |
		| ...              |
		| component 0      |
		+------------------+

	out:
		+------------------+
		| addr of variable |
		+------------------+
*/

static int assign_aggregate (slang_assembly_file *file, const slang_storage_aggregate *agg,
	unsigned int *index, unsigned int size, slang_assembly_local_info *info,
	slang_assembly_stack_info *stk)
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
				if (!assign_aggregate (file, arr->aggregate, index, size, info, stk))
					return 0;
			}
			else
			{
				unsigned int dst_addr_loc, dst_offset;
				slang_assembly_type ty;

				/* calculate the distance from top of the stack to the destination address */
				dst_addr_loc = size - *index;

				/* calculate the offset within destination variable to write */
				if (stk->swizzle.num_components != 0)
				{
					/* swizzle the index to get the actual offset */
					dst_offset = stk->swizzle.swizzle[*index / 4] * 4;
				}
				else
				{
					/* no swizzling - write sequentially */
					dst_offset = *index;
				}

				switch (arr->type)
				{
				case slang_stor_bool:
					ty = slang_asm_bool_copy;
					break;
				case slang_stor_int:
					ty = slang_asm_int_copy;
					break;
				case slang_stor_float:
					ty = slang_asm_float_copy;
					break;
				default:
					break;
				}
				if (!slang_assembly_file_push_label2 (file, ty, dst_addr_loc, dst_offset))
					return 0;
				*index += 4;
			}
		}
	}
	return 1;
}

int _slang_assemble_assignment (slang_assemble_ctx *A, slang_operation *op)
{
	slang_assembly_typeinfo ti;
	int result;
	slang_storage_aggregate agg;
	unsigned int index, size;

	if (!slang_assembly_typeinfo_construct (&ti))
		return 0;
	if (!_slang_typeof_operation (op, &A->space, &ti, A->atoms))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	if (!slang_storage_aggregate_construct (&agg))
	{
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}
	if (!_slang_aggregate_variable (&agg, &ti.spec, NULL, A->space.funcs, A->space.structs,
			A->space.vars, A->mach, A->file, A->atoms))
	{
		slang_storage_aggregate_destruct (&agg);
		slang_assembly_typeinfo_destruct (&ti);
		return 0;
	}

	index = 0;
	size = _slang_sizeof_aggregate (&agg);
	result = assign_aggregate (A->file, &agg, &index, size, &A->local, &A->swz);

	slang_storage_aggregate_destruct (&agg);
	slang_assembly_typeinfo_destruct (&ti);
	return result;
}

/*
	_slang_assemble_assign()

	performs unary (pre ++ and --) or binary (=, +=, -=, *=, /=) assignment on the operation's
	children
*/

int _slang_assemble_assign (slang_assemble_ctx *A, slang_operation *op, const char *oper,
	slang_ref_type ref)
{
	slang_assembly_stack_info stk;

	if (ref == slang_ref_forbid)
	{
		if (!slang_assembly_file_push_label2 (A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
			return 0;
	}

	if (slang_string_compare ("=", oper) == 0)
	{
		if (!_slang_assemble_operation_ (A, &op->children[0], slang_ref_force))
			return 0;
		stk = A->swz;
		if (!_slang_assemble_operation_ (A, &op->children[1], slang_ref_forbid))
			return 0;
		A->swz = stk;
		if (!_slang_assemble_assignment (A, op->children))
			return 0;
	}
	else
	{
		if (!_slang_assemble_function_call_name (A, oper, op->children, op->num_children, GL_TRUE))
			return 0;
	}

	if (ref == slang_ref_forbid)
	{
		if (!slang_assembly_file_push (A->file, slang_asm_addr_copy))
			return 0;
		if (!slang_assembly_file_push_label (A->file, slang_asm_local_free, 4))
			return 0;
		if (!_slang_dereference (A->file, op->children, &A->space, &A->local, A->mach, A->atoms))
			return 0;
	}

	return 1;
}

