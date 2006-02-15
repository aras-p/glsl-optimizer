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
 * \file slang_assemble_typeinfo.c
 * slang type info
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble_typeinfo.h"

/* slang_assembly_typeinfo */

int slang_assembly_typeinfo_construct (slang_assembly_typeinfo *ti)
{
	if (!slang_type_specifier_construct (&ti->spec))
		return 0;
	ti->array_size = NULL;
	return 1;
}

void slang_assembly_typeinfo_destruct (slang_assembly_typeinfo *ti)
{
	slang_type_specifier_destruct (&ti->spec);
	/* do not free ti->array_size */
}

/* _slang_typeof_operation() */

static int typeof_existing_function (const char *name, slang_operation *params,
	unsigned int num_params, slang_assembly_name_space *space, slang_type_specifier *spec,
	slang_atom_pool *atoms)
{
	slang_atom atom;
	int exists;

	atom = slang_atom_pool_atom (atoms, name);
	if (!_slang_typeof_function (atom, params, num_params, space, spec, &exists, atoms))
		return 0;
	return exists;
}

int _slang_typeof_operation (slang_operation *op, slang_assembly_name_space *space,
	slang_assembly_typeinfo *ti, slang_atom_pool *atoms)
{
	ti->can_be_referenced = 0;
	ti->is_swizzled = 0;

	switch (op->type)
	{
	case slang_oper_block_no_new_scope:
	case slang_oper_block_new_scope:
	case slang_oper_variable_decl:
	case slang_oper_asm:
	case slang_oper_break:
	case slang_oper_continue:
	case slang_oper_discard:
	case slang_oper_return:
	case slang_oper_if:
	case slang_oper_while:
	case slang_oper_do:
	case slang_oper_for:
	case slang_oper_void:
		ti->spec.type = slang_spec_void;
		break;
	case slang_oper_expression:
	case slang_oper_assign:
	case slang_oper_addassign:
	case slang_oper_subassign:
	case slang_oper_mulassign:
	case slang_oper_divassign:
	case slang_oper_preincrement:
	case slang_oper_predecrement:
		if (!_slang_typeof_operation (op->children, space, ti, atoms))
			return 0;
		break;
	case slang_oper_literal_bool:
	case slang_oper_logicalor:
	case slang_oper_logicalxor:
	case slang_oper_logicaland:
	case slang_oper_equal:
	case slang_oper_notequal:
	case slang_oper_less:
	case slang_oper_greater:
	case slang_oper_lessequal:
	case slang_oper_greaterequal:
	case slang_oper_not:
		ti->spec.type = slang_spec_bool;
		break;
	case slang_oper_literal_int:
		ti->spec.type = slang_spec_int;
		break;
	case slang_oper_literal_float:
		ti->spec.type = slang_spec_float;
		break;
	case slang_oper_identifier:
		{
			slang_variable *var;

			var = _slang_locate_variable (op->locals, op->a_id, 1);
			if (var == NULL)
				return 0;
			if (!slang_type_specifier_copy (&ti->spec, &var->type.specifier))
				return 0;
			ti->can_be_referenced = 1;
			ti->array_size = var->array_size;
		}
		break;
	case slang_oper_sequence:
		/* TODO: check [0] and [1] if they match */
		if (!_slang_typeof_operation (&op->children[1], space, ti, atoms))
			return 0;
		ti->can_be_referenced = 0;
		ti->is_swizzled = 0;
		break;
	/*case slang_oper_modassign:*/
	/*case slang_oper_lshassign:*/
	/*case slang_oper_rshassign:*/
	/*case slang_oper_orassign:*/
	/*case slang_oper_xorassign:*/
	/*case slang_oper_andassign:*/
	case slang_oper_select:
		/* TODO: check [1] and [2] if they match */
		if (!_slang_typeof_operation (&op->children[1], space, ti, atoms))
			return 0;
		ti->can_be_referenced = 0;
		ti->is_swizzled = 0;
		break;
	/*case slang_oper_bitor:*/
	/*case slang_oper_bitxor:*/
	/*case slang_oper_bitand:*/
	/*case slang_oper_lshift:*/
	/*case slang_oper_rshift:*/
	case slang_oper_add:
		if (!typeof_existing_function ("+", op->children, 2, space, &ti->spec, atoms))
			return 0;
		break;
	case slang_oper_subtract:
		if (!typeof_existing_function ("-", op->children, 2, space, &ti->spec, atoms))
			return 0;
		break;
	case slang_oper_multiply:
		if (!typeof_existing_function ("*", op->children, 2, space, &ti->spec, atoms))
			return 0;
		break;
	case slang_oper_divide:
		if (!typeof_existing_function ("/", op->children, 2, space, &ti->spec, atoms))
			return 0;
		break;
	/*case slang_oper_modulus:*/
	case slang_oper_plus:
		if (!_slang_typeof_operation (op->children, space, ti, atoms))
			return 0;
		ti->can_be_referenced = 0;
		ti->is_swizzled = 0;
		break;
	case slang_oper_minus:
		if (!typeof_existing_function ("-", op->children, 1, space, &ti->spec, atoms))
			return 0;
		break;
	/*case slang_oper_complement:*/
	case slang_oper_subscript:
		{
			slang_assembly_typeinfo _ti;

			if (!slang_assembly_typeinfo_construct (&_ti))
				return 0;
			if (!_slang_typeof_operation (op->children, space, &_ti, atoms))
			{
				slang_assembly_typeinfo_destruct (&_ti);
				return 0;
			}
			ti->can_be_referenced = _ti.can_be_referenced;
			if (_ti.spec.type == slang_spec_array)
			{
				if (!slang_type_specifier_copy (&ti->spec, _ti.spec._array))
				{
					slang_assembly_typeinfo_destruct (&_ti);
					return 0;
				}
			}
			else
			{
				if (!_slang_type_is_vector (_ti.spec.type) && !_slang_type_is_matrix (_ti.spec.type))
				{
					slang_assembly_typeinfo_destruct (&_ti);
					return 0;
				}
				ti->spec.type = _slang_type_base (_ti.spec.type);
			}
			slang_assembly_typeinfo_destruct (&_ti);
		}
		break;
	case slang_oper_call:
		{
			int exists;

			if (!_slang_typeof_function (op->a_id, op->children, op->num_children, space, &ti->spec,
					&exists, atoms))
				return 0;
			if (!exists)
			{
/*				slang_struct *s = slang_struct_scope_find (space->structs, op->identifier, 1);
				if (s != NULL)
				{
					ti->spec.type = slang_spec_struct;
					ti->spec._struct = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
					if (ti->spec._struct == NULL)
						return 0;
					if (!slang_struct_construct (ti->spec._struct))
					{
						slang_alloc_free (ti->spec._struct);
						ti->spec._struct = NULL;
						return 0;
					}
					if (!slang_struct_copy (ti->spec._struct, s))
						return 0;
				}
				else
*/				{
					const char *name;
					slang_type_specifier_type type;

					name = slang_atom_pool_id (atoms, op->a_id);
					type = slang_type_specifier_type_from_string (name);
					if (type == slang_spec_void)
						return 0;
					ti->spec.type = type;
				}
			}
		}
		break;
	case slang_oper_field:
		{
			slang_assembly_typeinfo _ti;

			if (!slang_assembly_typeinfo_construct (&_ti))
				return 0;
			if (!_slang_typeof_operation (op->children, space, &_ti, atoms))
			{
				slang_assembly_typeinfo_destruct (&_ti);
				return 0;
			}
			if (_ti.spec.type == slang_spec_struct)
			{
				slang_variable *field;
				
				field = _slang_locate_variable (_ti.spec._struct->fields, op->a_id, 0);
				if (field == NULL)
				{
					slang_assembly_typeinfo_destruct (&_ti);
					return 0;
				}
				if (!slang_type_specifier_copy (&ti->spec, &field->type.specifier))
				{
					slang_assembly_typeinfo_destruct (&_ti);
					return 0;
				}
				ti->can_be_referenced = _ti.can_be_referenced;
			}
			else
			{
				unsigned int rows;
				const char *swizzle;
				slang_type_specifier_type base;

				/* determine the swizzle of the field expression */
				if (!_slang_type_is_vector (_ti.spec.type))
				{
					slang_assembly_typeinfo_destruct (&_ti);
					return 0;
				}
				rows = _slang_type_dim (_ti.spec.type);
				swizzle = slang_atom_pool_id (atoms, op->a_id);
				if (!_slang_is_swizzle (swizzle, rows, &ti->swz))
				{
					slang_assembly_typeinfo_destruct (&_ti);
					return 0;
				}
				ti->is_swizzled = 1;
				ti->can_be_referenced = _ti.can_be_referenced && _slang_is_swizzle_mask (&ti->swz,
					rows);
				if (_ti.is_swizzled)
				{
					slang_swizzle swz;

					/* swizzle the swizzle */
					_slang_multiply_swizzles (&swz, &_ti.swz, &ti->swz);
					ti->swz = swz;
				}
				base = _slang_type_base (_ti.spec.type);
				switch (ti->swz.num_components)
				{
				case 1:
					ti->spec.type = base;
					break;
				case 2:
					switch (base)
					{
					case slang_spec_float:
						ti->spec.type = slang_spec_vec2;
						break;
					case slang_spec_int:
						ti->spec.type = slang_spec_ivec2;
						break;
					case slang_spec_bool:
						ti->spec.type = slang_spec_bvec2;
						break;
					default:
						_mesa_problem(NULL, "unexepected base in _slang_typeof_operation");
						ti->spec.type = slang_spec_void;
					}
					break;
				case 3:
					switch (base)
					{
					case slang_spec_float:
						ti->spec.type = slang_spec_vec3;
						break;
					case slang_spec_int:
						ti->spec.type = slang_spec_ivec3;
						break;
					case slang_spec_bool:
						ti->spec.type = slang_spec_bvec3;
						break;
					default:
						_mesa_problem(NULL, "unexepected base in _slang_typeof_operation");
						ti->spec.type = slang_spec_void;
					}
					break;
				case 4:
					switch (base)
					{
					case slang_spec_float:
						ti->spec.type = slang_spec_vec4;
						break;
					case slang_spec_int:
						ti->spec.type = slang_spec_ivec4;
						break;
					case slang_spec_bool:
						ti->spec.type = slang_spec_bvec4;
						break;
					default:
						_mesa_problem(NULL, "unexepected base in _slang_typeof_operation");
						ti->spec.type = slang_spec_void;
					}
					break;
				default:
					break;
				}
			}
			slang_assembly_typeinfo_destruct (&_ti);
		}
		break;
	case slang_oper_postincrement:
	case slang_oper_postdecrement:
		if (!_slang_typeof_operation (op->children, space, ti, atoms))
			return 0;
		ti->can_be_referenced = 0;
		ti->is_swizzled = 0;
		break;
	default:
		return 0;
	}
	return 1;
}

/* _slang_typeof_function() */

int _slang_typeof_function (slang_atom a_name, slang_operation *params, unsigned int num_params,
	slang_assembly_name_space *space, slang_type_specifier *spec, int *exists, slang_atom_pool *atoms)
{
	slang_function *fun;
	
	fun = _slang_locate_function (space->funcs, a_name, params, num_params, space, atoms);
	*exists = fun != NULL;
	if (fun == NULL)
		return 1;
	return slang_type_specifier_copy (spec, &fun->header.type.specifier);
}

/* _slang_type_is_matrix() */

int _slang_type_is_matrix (slang_type_specifier_type ty)
{
	switch (ty)
	{
	case slang_spec_mat2:
	case slang_spec_mat3:
	case slang_spec_mat4:
		return 1;
	default:
		return 0;
	}
}

/* _slang_type_is_vector() */

int _slang_type_is_vector (slang_type_specifier_type ty)
{
	switch (ty)
	{
	case slang_spec_vec2:
	case slang_spec_vec3:
	case slang_spec_vec4:
	case slang_spec_ivec2:
	case slang_spec_ivec3:
	case slang_spec_ivec4:
	case slang_spec_bvec2:
	case slang_spec_bvec3:
	case slang_spec_bvec4:
		return 1;
	default:
		return 0;
	}
}

/* _slang_type_base_of_vector() */

slang_type_specifier_type _slang_type_base (slang_type_specifier_type ty)
{
	switch (ty)
	{
	case slang_spec_float:
	case slang_spec_vec2:
	case slang_spec_vec3:
	case slang_spec_vec4:
		return slang_spec_float;
	case slang_spec_int:
	case slang_spec_ivec2:
	case slang_spec_ivec3:
	case slang_spec_ivec4:
		return slang_spec_int;
	case slang_spec_bool:
	case slang_spec_bvec2:
	case slang_spec_bvec3:
	case slang_spec_bvec4:
		return slang_spec_bool;
	case slang_spec_mat2:
		return slang_spec_vec2;
	case slang_spec_mat3:
		return slang_spec_vec3;
	case slang_spec_mat4:
		return slang_spec_vec4;
	default:
		return slang_spec_void;
	}
}

/* _slang_type_dim */

unsigned int _slang_type_dim (slang_type_specifier_type ty)
{
	switch (ty)
	{
	case slang_spec_float:
	case slang_spec_int:
	case slang_spec_bool:
		return 1;
	case slang_spec_vec2:
	case slang_spec_ivec2:
	case slang_spec_bvec2:
	case slang_spec_mat2:
		return 2;
	case slang_spec_vec3:
	case slang_spec_ivec3:
	case slang_spec_bvec3:
	case slang_spec_mat3:
		return 3;
	case slang_spec_vec4:
	case slang_spec_ivec4:
	case slang_spec_bvec4:
	case slang_spec_mat4:
		return 4;
	default:
		return 0;
	}
}

