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
 * \file slang_compile.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "grammar_mesa.h"
#include "slang_utility.h"
#include "slang_compile.h"
#include "slang_preprocess.h"
#include "slang_storage.h"
#include "slang_assemble.h"
#include "slang_execute.h"

/*
 * This is a straightforward implementation of the slang front-end compiler.
 * Lots of error-checking functionality is missing but every well-formed shader source should
 * compile successfully and execute as expected. However, some semantically ill-formed shaders
 * may be accepted resulting in undefined behaviour.
 */

/* slang_var_pool */

static GLuint slang_var_pool_alloc (slang_var_pool *pool, unsigned int size)
{
	GLuint addr;

	addr = pool->next_addr;
	pool->next_addr += size;
	return addr;
}

/* slang_translation_unit */

int slang_translation_unit_construct (slang_translation_unit *unit)
{
	unit->assembly = (slang_assembly_file *) slang_alloc_malloc (sizeof (slang_assembly_file));
	if (unit->assembly == NULL)
		return 0;
	if (!slang_assembly_file_construct (unit->assembly))
	{
		slang_alloc_free (unit->assembly);
		return 0;
	}
	unit->global_pool = (slang_var_pool *) slang_alloc_malloc (sizeof (slang_var_pool));
	if (unit->global_pool == NULL)
	{
		slang_assembly_file_destruct (unit->assembly);
		slang_alloc_free (unit->assembly);
		return 0;
	}
	unit->global_pool->next_addr = 0;
	unit->machine = (slang_machine *) slang_alloc_malloc (sizeof (slang_machine));
	if (unit->machine == NULL)
	{
		slang_alloc_free (unit->global_pool);
		slang_assembly_file_destruct (unit->assembly);
		slang_alloc_free (unit->assembly);
		return 0;
	}
	slang_machine_init (unit->machine);
	unit->atom_pool = (slang_atom_pool *) slang_alloc_malloc (sizeof (slang_atom_pool));
	if (unit->atom_pool == NULL)
	{
		slang_alloc_free (unit->machine);
		slang_alloc_free (unit->global_pool);
		slang_assembly_file_destruct (unit->assembly);
		slang_alloc_free (unit->assembly);
		return 0;
	}
	slang_atom_pool_construct (unit->atom_pool);
	if (!slang_translation_unit_construct2 (unit, unit->assembly, unit->global_pool, unit->machine,
			unit->atom_pool))
	{
		slang_alloc_free (unit->atom_pool);
		slang_alloc_free (unit->machine);
		slang_alloc_free (unit->global_pool);
		slang_assembly_file_destruct (unit->assembly);
		slang_alloc_free (unit->assembly);
		return 0;
	}
	unit->free_assembly = 1;
	unit->free_global_pool = 1;
	unit->free_machine = 1;
	unit->free_atom_pool = 1;
	return 1;
}

int slang_translation_unit_construct2 (slang_translation_unit *unit, slang_assembly_file *file,
	slang_var_pool *pool, struct slang_machine_ *mach, slang_atom_pool *atoms)
{
	if (!slang_variable_scope_construct (&unit->globals))
		return 0;
	if (!slang_function_scope_construct (&unit->functions))
	{
		slang_variable_scope_destruct (&unit->globals);
		return 0;
	}
	if (!slang_struct_scope_construct (&unit->structs))
	{
		slang_variable_scope_destruct (&unit->globals);
		slang_function_scope_destruct (&unit->functions);
		return 0;
	}
	unit->assembly = file;
	unit->free_assembly = 0;
	unit->global_pool = pool;
	unit->free_global_pool = 0;
	unit->machine = mach;
	unit->free_machine = 0;
	unit->atom_pool = atoms;
	unit->free_atom_pool = 0;
	slang_export_data_table_ctr (&unit->exp_data);
	slang_active_uniforms_ctr (&unit->uniforms);
	return 1;
}

void slang_translation_unit_destruct (slang_translation_unit *unit)
{
	slang_variable_scope_destruct (&unit->globals);
	slang_function_scope_destruct (&unit->functions);
	slang_struct_scope_destruct (&unit->structs);
	if (unit->free_assembly)
	{
		slang_assembly_file_destruct (unit->assembly);
		slang_alloc_free (unit->assembly);
	}
	if (unit->free_global_pool)
		slang_alloc_free (unit->global_pool);
	if (unit->free_machine)
		slang_alloc_free (unit->machine);
	if (unit->free_atom_pool)
	{
		slang_atom_pool_destruct (unit->atom_pool);
		slang_alloc_free (unit->atom_pool);
	}
	slang_active_uniforms_dtr (&unit->uniforms);
	slang_export_data_table_dtr (&unit->exp_data);
}

/* slang_info_log */

static char *out_of_memory = "error: out of memory\n";

void slang_info_log_construct (slang_info_log *log)
{
	log->text = NULL;
	log->dont_free_text = 0;
}

void slang_info_log_destruct (slang_info_log *log)
{
	if (!log->dont_free_text)
		slang_alloc_free (log->text);
}

static int slang_info_log_message (slang_info_log *log, const char *prefix, const char *msg)
{
	unsigned int new_size;

	if (log->dont_free_text)
		return 0;
	new_size = slang_string_length (prefix) + 3 + slang_string_length (msg);
	if (log->text != NULL)
	{
		unsigned int text_len = slang_string_length (log->text);

		log->text = (char *) slang_alloc_realloc (log->text, text_len + 1, new_size + text_len + 1);
	}
	else
	{
		log->text = (char *) slang_alloc_malloc (new_size + 1);
		if (log->text != NULL)
			log->text[0] = '\0';
	}
	if (log->text == NULL)
		return 0;
	slang_string_concat (log->text, prefix);
	slang_string_concat (log->text, ": ");
	slang_string_concat (log->text, msg);
	slang_string_concat (log->text, "\n");
	return 1;
}

int slang_info_log_error (slang_info_log *log, const char *msg, ...)
{
	va_list va;
	char buf[1024];

	va_start (va, msg);
	_mesa_vsprintf (buf, msg, va);
	if (slang_info_log_message (log, "error", buf))
		return 1;
	slang_info_log_memory (log);
	va_end (va);
	return 0;
}

int slang_info_log_warning (slang_info_log *log, const char *msg, ...)
{
	va_list va;
	char buf[1024];

	va_start (va, msg);
	_mesa_vsprintf (buf, msg, va);
	if (slang_info_log_message (log, "warning", buf))
		return 1;
	slang_info_log_memory (log);
	va_end (va);
	return 0;
}

void slang_info_log_memory (slang_info_log *log)
{
	if (!slang_info_log_message (log, "error", "out of memory"))
	{
		log->dont_free_text = 1;
		log->text = out_of_memory;
	}
}

/* slang_parse_ctx */

typedef struct slang_parse_ctx_
{
	const byte *I;
	slang_info_log *L;
	int parsing_builtin;
	int global_scope;
	slang_atom_pool *atoms;
} slang_parse_ctx;

/* slang_output_ctx */

typedef struct slang_output_ctx_
{
	slang_variable_scope *vars;
	slang_function_scope *funs;
	slang_struct_scope *structs;
	slang_assembly_file *assembly;
	slang_var_pool *global_pool;
	slang_machine *machine;
} slang_output_ctx;

/* _slang_compile() */

static void parse_identifier_str (slang_parse_ctx *C, char **id)
{
	*id = (char *) C->I;
	C->I += _mesa_strlen (*id) + 1;
}

static slang_atom parse_identifier (slang_parse_ctx *C)
{
	const char *id;
	
	id = (const char *) C->I;
	C->I += _mesa_strlen (id) + 1;
	return slang_atom_pool_atom (C->atoms, id);
}

static int parse_number (slang_parse_ctx *C, int *number)
{
	const int radix = (int) (*C->I++);
	*number = 0;
	while (*C->I != '\0')
	{
		int digit;
		if (*C->I >= '0' && *C->I <= '9')
			digit = (int) (*C->I - '0');
		else if (*C->I >= 'A' && *C->I <= 'Z')
			digit = (int) (*C->I - 'A') + 10;
		else
			digit = (int) (*C->I - 'a') + 10;
		*number = *number * radix + digit;
		C->I++;
	}
	C->I++;
	if (*number > 65535)
		slang_info_log_warning (C->L, "%d: literal integer overflow", *number);
	return 1;
}

static int parse_float (slang_parse_ctx *C, float *number)
{
	char *integral = NULL;
	char *fractional = NULL;
	char *exponent = NULL;
	char *whole = NULL;

	parse_identifier_str (C, &integral);
	parse_identifier_str (C, &fractional);
	parse_identifier_str (C, &exponent);

	whole = (char *) (slang_alloc_malloc ((_mesa_strlen (integral) + _mesa_strlen (fractional) +
		_mesa_strlen (exponent) + 3) * sizeof (char)));
	if (whole == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}

	slang_string_copy (whole, integral);
	slang_string_concat (whole, ".");
	slang_string_concat (whole, fractional);
	slang_string_concat (whole, "E");
	slang_string_concat (whole, exponent);

	*number = (float) (_mesa_strtod(whole, (char **)NULL));

	slang_alloc_free (whole);
	return 1;
}

/* revision number - increment after each change affecting emitted output */
#define REVISION 3

static int check_revision (slang_parse_ctx *C)
{
	if (*C->I != REVISION)
	{
		slang_info_log_error (C->L, "internal compiler error");
		return 0;
	}
	C->I++;
	return 1;
}

static int parse_statement (slang_parse_ctx *, slang_output_ctx *, slang_operation *);
static int parse_expression (slang_parse_ctx *, slang_output_ctx *, slang_operation *);
static int parse_type_specifier (slang_parse_ctx *, slang_output_ctx *, slang_type_specifier *);

static GLboolean parse_array_len (slang_parse_ctx *C, slang_output_ctx *O, GLuint *len)
{
	slang_operation array_size;
	slang_assembly_name_space space;
	GLboolean result;

	if (!slang_operation_construct (&array_size))
		return GL_FALSE;
	if (!parse_expression (C, O, &array_size))
	{
		slang_operation_destruct (&array_size);
		return GL_FALSE;
	}

	space.funcs = O->funs;
	space.structs = O->structs;
	space.vars = O->vars;
	result = _slang_evaluate_int (O->assembly, O->machine, &space, &array_size, len, C->atoms);
	slang_operation_destruct (&array_size);
	return result;
}

static GLboolean calculate_var_size (slang_parse_ctx *C, slang_output_ctx *O, slang_variable *var)
{
	slang_storage_aggregate agg;

	if (!slang_storage_aggregate_construct (&agg))
		return GL_FALSE;
	if (!_slang_aggregate_variable (&agg, &var->type.specifier, var->array_len, O->funs, O->structs,
			O->vars, O->machine, O->assembly, C->atoms))
	{
		slang_storage_aggregate_destruct (&agg);
		return GL_FALSE;
	}
	var->size = _slang_sizeof_aggregate (&agg);
	slang_storage_aggregate_destruct (&agg);
	return GL_TRUE;
}

static GLboolean convert_to_array (slang_parse_ctx *C, slang_variable *var,
	const slang_type_specifier *sp)
{
	/* sized array - mark it as array, copy the specifier to the array element and
	 * parse the expression */
	var->type.specifier.type = slang_spec_array;
	var->type.specifier._array = (slang_type_specifier *) slang_alloc_malloc (sizeof (
		slang_type_specifier));
	if (var->type.specifier._array == NULL)
	{
		slang_info_log_memory (C->L);
		return GL_FALSE;
	}
	if (!slang_type_specifier_construct (var->type.specifier._array))
	{
		slang_alloc_free (var->type.specifier._array);
		var->type.specifier._array = NULL;
		slang_info_log_memory (C->L);
		return GL_FALSE;
	}
	return slang_type_specifier_copy (var->type.specifier._array, sp);
}

/* structure field */
#define FIELD_NONE 0
#define FIELD_NEXT 1
#define FIELD_ARRAY 2

static GLboolean parse_struct_field_var (slang_parse_ctx *C, slang_output_ctx *O, slang_variable *var,
	const slang_type_specifier *sp)
{
	var->a_name = parse_identifier (C);
	if (var->a_name == SLANG_ATOM_NULL)
		return GL_FALSE;

	switch (*C->I++)
	{
	case FIELD_NONE:
		if (!slang_type_specifier_copy (&var->type.specifier, sp))
			return GL_FALSE;
		break;
	case FIELD_ARRAY:
		if (!convert_to_array (C, var, sp))
			return GL_FALSE;
		if (!parse_array_len (C, O, &var->array_len))
			return GL_FALSE;
		break;
	default:
		return GL_FALSE;
	}

	return calculate_var_size (C, O, var);
}

static int parse_struct_field (slang_parse_ctx *C, slang_output_ctx *O, slang_struct *st,
	slang_type_specifier *sp)
{
	slang_output_ctx o = *O;

	o.structs = st->structs;
	if (!parse_type_specifier (C, &o, sp))
		return 0;
	do
	{
		slang_variable *var;

		st->fields->variables = (slang_variable *) slang_alloc_realloc (st->fields->variables,
			st->fields->num_variables * sizeof (slang_variable),
			(st->fields->num_variables + 1) * sizeof (slang_variable));
		if (st->fields->variables == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		var = &st->fields->variables[st->fields->num_variables];
		if (!slang_variable_construct (var))
			return 0;
		st->fields->num_variables++;
		if (!parse_struct_field_var (C, &o, var, sp))
			return 0;
	}
	while (*C->I++ != FIELD_NONE);

	return 1;
}

static int parse_struct (slang_parse_ctx *C, slang_output_ctx *O, slang_struct **st)
{
	slang_atom a_name;
	const char *name;

	/* parse struct name (if any) and make sure it is unique in current scope */
	a_name = parse_identifier (C);
	if (a_name == SLANG_ATOM_NULL)
		return 0;
	name = slang_atom_pool_id (C->atoms, a_name);
	if (name[0] != '\0' && slang_struct_scope_find (O->structs, a_name, 0) != NULL)
	{
		slang_info_log_error (C->L, "%s: duplicate type name", name);
		return 0;
	}

	/* set-up a new struct */
	*st = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
	if (*st == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	if (!slang_struct_construct (*st))
	{
		slang_alloc_free (*st);
		*st = NULL;
		slang_info_log_memory (C->L);
		return 0;
	}
	(**st).a_name = a_name;
	(**st).structs->outer_scope = O->structs;

	/* parse individual struct fields */
	do
	{
		slang_type_specifier sp;

		if (!slang_type_specifier_construct (&sp))
			return 0;
		if (!parse_struct_field (C, O, *st, &sp))
		{
			slang_type_specifier_destruct (&sp);
			return 0;
		}
		slang_type_specifier_destruct (&sp);
	}
	while (*C->I++ != FIELD_NONE);

	/* if named struct, copy it to current scope */
	if (name[0] != '\0')
	{
		slang_struct *s;

		O->structs->structs = (slang_struct *) slang_alloc_realloc (O->structs->structs,
			O->structs->num_structs * sizeof (slang_struct),
			(O->structs->num_structs + 1) * sizeof (slang_struct));
		if (O->structs->structs == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		s = &O->structs->structs[O->structs->num_structs];
		if (!slang_struct_construct (s))
			return 0;
		O->structs->num_structs++;
		if (!slang_struct_copy (s, *st))
			return 0;
	}

	return 1;
}

/* type qualifier */
#define TYPE_QUALIFIER_NONE 0
#define TYPE_QUALIFIER_CONST 1
#define TYPE_QUALIFIER_ATTRIBUTE 2
#define TYPE_QUALIFIER_VARYING 3
#define TYPE_QUALIFIER_UNIFORM 4
#define TYPE_QUALIFIER_FIXEDOUTPUT 5
#define TYPE_QUALIFIER_FIXEDINPUT 6

static int parse_type_qualifier (slang_parse_ctx *C, slang_type_qualifier *qual)
{
	switch (*C->I++)
	{
	case TYPE_QUALIFIER_NONE:
		*qual = slang_qual_none;
		break;
	case TYPE_QUALIFIER_CONST:
		*qual = slang_qual_const;
		break;
	case TYPE_QUALIFIER_ATTRIBUTE:
		*qual = slang_qual_attribute;
		break;
	case TYPE_QUALIFIER_VARYING:
		*qual = slang_qual_varying;
		break;
	case TYPE_QUALIFIER_UNIFORM:
		*qual = slang_qual_uniform;
		break;
	case TYPE_QUALIFIER_FIXEDOUTPUT:
		*qual = slang_qual_fixedoutput;
		break;
	case TYPE_QUALIFIER_FIXEDINPUT:
		*qual = slang_qual_fixedinput;
		break;
	default:
		return 0;
	}
	return 1;
}

/* type specifier */
#define TYPE_SPECIFIER_VOID 0
#define TYPE_SPECIFIER_BOOL 1
#define TYPE_SPECIFIER_BVEC2 2
#define TYPE_SPECIFIER_BVEC3 3
#define TYPE_SPECIFIER_BVEC4 4
#define TYPE_SPECIFIER_INT 5
#define TYPE_SPECIFIER_IVEC2 6
#define TYPE_SPECIFIER_IVEC3 7
#define TYPE_SPECIFIER_IVEC4 8
#define TYPE_SPECIFIER_FLOAT 9
#define TYPE_SPECIFIER_VEC2 10
#define TYPE_SPECIFIER_VEC3 11
#define TYPE_SPECIFIER_VEC4 12
#define TYPE_SPECIFIER_MAT2 13
#define TYPE_SPECIFIER_MAT3 14
#define TYPE_SPECIFIER_MAT4 15
#define TYPE_SPECIFIER_SAMPLER1D 16
#define TYPE_SPECIFIER_SAMPLER2D 17
#define TYPE_SPECIFIER_SAMPLER3D 18
#define TYPE_SPECIFIER_SAMPLERCUBE 19
#define TYPE_SPECIFIER_SAMPLER1DSHADOW 20
#define TYPE_SPECIFIER_SAMPLER2DSHADOW 21
#define TYPE_SPECIFIER_STRUCT 22
#define TYPE_SPECIFIER_TYPENAME 23

static int parse_type_specifier (slang_parse_ctx *C, slang_output_ctx *O, slang_type_specifier *spec)
{
	switch (*C->I++)
	{
	case TYPE_SPECIFIER_VOID:
		spec->type = slang_spec_void;
		break;
	case TYPE_SPECIFIER_BOOL:
		spec->type = slang_spec_bool;
		break;
	case TYPE_SPECIFIER_BVEC2:
		spec->type = slang_spec_bvec2;
		break;
	case TYPE_SPECIFIER_BVEC3:
		spec->type = slang_spec_bvec3;
		break;
	case TYPE_SPECIFIER_BVEC4:
		spec->type = slang_spec_bvec4;
		break;
	case TYPE_SPECIFIER_INT:
		spec->type = slang_spec_int;
		break;
	case TYPE_SPECIFIER_IVEC2:
		spec->type = slang_spec_ivec2;
		break;
	case TYPE_SPECIFIER_IVEC3:
		spec->type = slang_spec_ivec3;
		break;
	case TYPE_SPECIFIER_IVEC4:
		spec->type = slang_spec_ivec4;
		break;
	case TYPE_SPECIFIER_FLOAT:
		spec->type = slang_spec_float;
		break;
	case TYPE_SPECIFIER_VEC2:
		spec->type = slang_spec_vec2;
		break;
	case TYPE_SPECIFIER_VEC3:
		spec->type = slang_spec_vec3;
		break;
	case TYPE_SPECIFIER_VEC4:
		spec->type = slang_spec_vec4;
		break;
	case TYPE_SPECIFIER_MAT2:
		spec->type = slang_spec_mat2;
		break;
	case TYPE_SPECIFIER_MAT3:
		spec->type = slang_spec_mat3;
		break;
	case TYPE_SPECIFIER_MAT4:
		spec->type = slang_spec_mat4;
		break;
	case TYPE_SPECIFIER_SAMPLER1D:
		spec->type = slang_spec_sampler1D;
		break;
	case TYPE_SPECIFIER_SAMPLER2D:
		spec->type = slang_spec_sampler2D;
		break;
	case TYPE_SPECIFIER_SAMPLER3D:
		spec->type = slang_spec_sampler3D;
		break;
	case TYPE_SPECIFIER_SAMPLERCUBE:
		spec->type = slang_spec_samplerCube;
		break;
	case TYPE_SPECIFIER_SAMPLER1DSHADOW:
		spec->type = slang_spec_sampler1DShadow;
		break;
	case TYPE_SPECIFIER_SAMPLER2DSHADOW:
		spec->type = slang_spec_sampler2DShadow;
		break;
	case TYPE_SPECIFIER_STRUCT:
		spec->type = slang_spec_struct;
		if (!parse_struct (C, O, &spec->_struct))
			return 0;
		break;
	case TYPE_SPECIFIER_TYPENAME:
		spec->type = slang_spec_struct;
		{
			slang_atom a_name;
			slang_struct *stru;

			a_name = parse_identifier (C);
			if (a_name == NULL)
				return 0;

			stru = slang_struct_scope_find (O->structs, a_name, 1);
			if (stru == NULL)
			{
				slang_info_log_error (C->L, "%s: undeclared type name",
					slang_atom_pool_id (C->atoms, a_name));
				return 0;
			}

			spec->_struct = (slang_struct *) slang_alloc_malloc (sizeof (slang_struct));
			if (spec->_struct == NULL)
			{
				slang_info_log_memory (C->L);
				return 0;
			}
			if (!slang_struct_construct (spec->_struct))
			{
				slang_alloc_free (spec->_struct);
				spec->_struct = NULL;
				return 0;
			}
			if (!slang_struct_copy (spec->_struct, stru))
				return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

static int parse_fully_specified_type (slang_parse_ctx *C, slang_output_ctx *O,
	slang_fully_specified_type *type)
{
	if (!parse_type_qualifier (C, &type->qualifier))
		return 0;
	return parse_type_specifier (C, O, &type->specifier);
}

/* operation */
#define OP_END 0
#define OP_BLOCK_BEGIN_NO_NEW_SCOPE 1
#define OP_BLOCK_BEGIN_NEW_SCOPE 2
#define OP_DECLARE 3
#define OP_ASM 4
#define OP_BREAK 5
#define OP_CONTINUE 6
#define OP_DISCARD 7
#define OP_RETURN 8
#define OP_EXPRESSION 9
#define OP_IF 10
#define OP_WHILE 11
#define OP_DO 12
#define OP_FOR 13
#define OP_PUSH_VOID 14
#define OP_PUSH_BOOL 15
#define OP_PUSH_INT 16
#define OP_PUSH_FLOAT 17
#define OP_PUSH_IDENTIFIER 18
#define OP_SEQUENCE 19
#define OP_ASSIGN 20
#define OP_ADDASSIGN 21
#define OP_SUBASSIGN 22
#define OP_MULASSIGN 23
#define OP_DIVASSIGN 24
/*#define OP_MODASSIGN 25*/
/*#define OP_LSHASSIGN 26*/
/*#define OP_RSHASSIGN 27*/
/*#define OP_ORASSIGN 28*/
/*#define OP_XORASSIGN 29*/
/*#define OP_ANDASSIGN 30*/
#define OP_SELECT 31
#define OP_LOGICALOR 32
#define OP_LOGICALXOR 33
#define OP_LOGICALAND 34
/*#define OP_BITOR 35*/
/*#define OP_BITXOR 36*/
/*#define OP_BITAND 37*/
#define OP_EQUAL 38
#define OP_NOTEQUAL 39
#define OP_LESS 40
#define OP_GREATER 41
#define OP_LESSEQUAL 42
#define OP_GREATEREQUAL 43
/*#define OP_LSHIFT 44*/
/*#define OP_RSHIFT 45*/
#define OP_ADD 46
#define OP_SUBTRACT 47
#define OP_MULTIPLY 48
#define OP_DIVIDE 49
/*#define OP_MODULUS 50*/
#define OP_PREINCREMENT 51
#define OP_PREDECREMENT 52
#define OP_PLUS 53
#define OP_MINUS 54
/*#define OP_COMPLEMENT 55*/
#define OP_NOT 56
#define OP_SUBSCRIPT 57
#define OP_CALL 58
#define OP_FIELD 59
#define OP_POSTINCREMENT 60
#define OP_POSTDECREMENT 61

static int parse_child_operation (slang_parse_ctx *C, slang_output_ctx *O, slang_operation *oper,
	int statement)
{
	slang_operation *ch;

	oper->children = (slang_operation *) slang_alloc_realloc (oper->children,
		oper->num_children * sizeof (slang_operation),
		(oper->num_children + 1) * sizeof (slang_operation));
	if (oper->children == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	ch = &oper->children[oper->num_children];
	if (!slang_operation_construct (ch))
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	oper->num_children++;
	if (statement)
		return parse_statement (C, O, ch);
	return parse_expression (C, O, ch);
}

static int parse_declaration (slang_parse_ctx *C, slang_output_ctx *O);

static int parse_statement (slang_parse_ctx *C, slang_output_ctx *O, slang_operation *oper)
{
	oper->locals->outer_scope = O->vars;
	switch (*C->I++)
	{
	case OP_BLOCK_BEGIN_NO_NEW_SCOPE:
		/* parse child statements, do not create new variable scope */
		oper->type = slang_oper_block_no_new_scope;
		while (*C->I != OP_END)
			if (!parse_child_operation (C, O, oper, 1))
				return 0;
		C->I++;
		break;
	case OP_BLOCK_BEGIN_NEW_SCOPE:
		/* parse child statements, create new variable scope */
		{
			slang_output_ctx o = *O;

			oper->type = slang_oper_block_new_scope;
			o.vars = oper->locals;
			while (*C->I != OP_END)
				if (!parse_child_operation (C, &o, oper, 1))
					return 0;
			C->I++;
		}
		break;
	case OP_DECLARE:
		/* local variable declaration, individual declarators are stored as children identifiers */
		oper->type = slang_oper_variable_decl;
		{
			const unsigned int first_var = O->vars->num_variables;

			/* parse the declaration, note that there can be zero or more than one declarators */
			if (!parse_declaration (C, O))
				return 0;
			if (first_var < O->vars->num_variables)
			{
				const unsigned int num_vars = O->vars->num_variables - first_var;
				unsigned int i;

				oper->children = (slang_operation *) slang_alloc_malloc (num_vars * sizeof (
					slang_operation));
				if (oper->children == NULL)
				{
					slang_info_log_memory (C->L);
					return 0;
				}
				for (oper->num_children = 0; oper->num_children < num_vars; oper->num_children++)
					if (!slang_operation_construct (&oper->children[oper->num_children]))
					{
						slang_info_log_memory (C->L);
						return 0;
					}
				for (i = first_var; i < O->vars->num_variables; i++)
				{
					slang_operation *o = &oper->children[i - first_var];

					o->type = slang_oper_identifier;
					o->locals->outer_scope = O->vars;
					o->a_id = O->vars->variables[i].a_name;
				}
			}
		}
		break;
	case OP_ASM:
		/* the __asm statement, parse the mnemonic and all its arguments as expressions */
		oper->type = slang_oper_asm;
		oper->a_id = parse_identifier (C);
		if (oper->a_id == SLANG_ATOM_NULL)
			return 0;
		while (*C->I != OP_END)
			if (!parse_child_operation (C, O, oper, 0))
				return 0;
		C->I++;
		break;
	case OP_BREAK:
		oper->type = slang_oper_break;
		break;
	case OP_CONTINUE:
		oper->type = slang_oper_continue;
		break;
	case OP_DISCARD:
		oper->type = slang_oper_discard;
		break;
	case OP_RETURN:
		oper->type = slang_oper_return;
		if (!parse_child_operation (C, O, oper, 0))
			return 0;
		break;
	case OP_EXPRESSION:
		oper->type = slang_oper_expression;
		if (!parse_child_operation (C, O, oper, 0))
			return 0;
		break;
	case OP_IF:
		oper->type = slang_oper_if;
		if (!parse_child_operation (C, O, oper, 0))
			return 0;
		if (!parse_child_operation (C, O, oper, 1))
			return 0;
		if (!parse_child_operation (C, O, oper, 1))
			return 0;
		break;
	case OP_WHILE:
		{
			slang_output_ctx o = *O;

			oper->type = slang_oper_while;
			o.vars = oper->locals;
			if (!parse_child_operation (C, &o, oper, 1))
				return 0;
			if (!parse_child_operation (C, &o, oper, 1))
				return 0;
		}
		break;
	case OP_DO:
		oper->type = slang_oper_do;
		if (!parse_child_operation (C, O, oper, 1))
			return 0;
		if (!parse_child_operation (C, O, oper, 0))
			return 0;
		break;
	case OP_FOR:
		{
			slang_output_ctx o = *O;

			oper->type = slang_oper_for;
			o.vars = oper->locals;
			if (!parse_child_operation (C, &o, oper, 1))
				return 0;
			if (!parse_child_operation (C, &o, oper, 1))
				return 0;
			if (!parse_child_operation (C, &o, oper, 0))
				return 0;
			if (!parse_child_operation (C, &o, oper, 1))
				return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

static int handle_nary_expression (slang_parse_ctx *C, slang_operation *op, slang_operation **ops,
	unsigned int *total_ops, unsigned int n)
{
	unsigned int i;

	op->children = (slang_operation *) slang_alloc_malloc (n * sizeof (slang_operation));
	if (op->children == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	op->num_children = n;

	for (i = 0; i < n; i++)
		op->children[i] = (*ops)[*total_ops - (n + 1 - i)];
	(*ops)[*total_ops - (n + 1)] = (*ops)[*total_ops - 1];
	*total_ops -= n;

	*ops = (slang_operation *) slang_alloc_realloc (*ops, (*total_ops + n) * sizeof (slang_operation),
		*total_ops * sizeof (slang_operation));
	if (*ops == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	return 1;
}

static int is_constructor_name (const char *name, slang_atom a_name, slang_struct_scope *structs)
{
	if (slang_type_specifier_type_from_string (name) != slang_spec_void)
		return 1;
	return slang_struct_scope_find (structs, a_name, 1) != NULL;
}

static int parse_expression (slang_parse_ctx *C, slang_output_ctx *O, slang_operation *oper)
{
	slang_operation *ops = NULL;
	unsigned int num_ops = 0;
	int number;

	while (*C->I != OP_END)
	{
		slang_operation *op;
		const unsigned int op_code = *C->I++;

		/* allocate default operation, becomes a no-op if not used  */
		ops = (slang_operation *) slang_alloc_realloc (ops,
			num_ops * sizeof (slang_operation), (num_ops + 1) * sizeof (slang_operation));
		if (ops == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		op = &ops[num_ops];
		if (!slang_operation_construct (op))
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		num_ops++;
		op->locals->outer_scope = O->vars;

		switch (op_code)
		{
		case OP_PUSH_VOID:
			op->type = slang_oper_void;
			break;
		case OP_PUSH_BOOL:
			op->type = slang_oper_literal_bool;
			if (!parse_number (C, &number))
				return 0;
			op->literal = (GLfloat) number;
			break;
		case OP_PUSH_INT:
			op->type = slang_oper_literal_int;
			if (!parse_number (C, &number))
				return 0;
			op->literal = (GLfloat) number;
			break;
		case OP_PUSH_FLOAT:
			op->type = slang_oper_literal_float;
			if (!parse_float (C, &op->literal))
				return 0;
			break;
		case OP_PUSH_IDENTIFIER:
			op->type = slang_oper_identifier;
			op->a_id = parse_identifier (C);
			if (op->a_id == SLANG_ATOM_NULL)
				return 0;
			break;
		case OP_SEQUENCE:
			op->type = slang_oper_sequence;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_ASSIGN:
			op->type = slang_oper_assign;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_ADDASSIGN:
			op->type = slang_oper_addassign;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_SUBASSIGN:
			op->type = slang_oper_subassign;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_MULASSIGN:
			op->type = slang_oper_mulassign;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_DIVASSIGN:
			op->type = slang_oper_divassign;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		/*case OP_MODASSIGN:*/
		/*case OP_LSHASSIGN:*/
		/*case OP_RSHASSIGN:*/
		/*case OP_ORASSIGN:*/
		/*case OP_XORASSIGN:*/
		/*case OP_ANDASSIGN:*/
		case OP_SELECT:
			op->type = slang_oper_select;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 3))
				return 0;
			break;
		case OP_LOGICALOR:
			op->type = slang_oper_logicalor;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_LOGICALXOR:
			op->type = slang_oper_logicalxor;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_LOGICALAND:
			op->type = slang_oper_logicaland;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		/*case OP_BITOR:*/
		/*case OP_BITXOR:*/
		/*case OP_BITAND:*/
		case OP_EQUAL:
			op->type = slang_oper_equal;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_NOTEQUAL:
			op->type = slang_oper_notequal;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_LESS:
			op->type = slang_oper_less;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_GREATER:
			op->type = slang_oper_greater;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_LESSEQUAL:
			op->type = slang_oper_lessequal;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_GREATEREQUAL:
			op->type = slang_oper_greaterequal;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		/*case OP_LSHIFT:*/
		/*case OP_RSHIFT:*/
		case OP_ADD:
			op->type = slang_oper_add;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_SUBTRACT:
			op->type = slang_oper_subtract;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_MULTIPLY:
			op->type = slang_oper_multiply;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_DIVIDE:
			op->type = slang_oper_divide;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		/*case OP_MODULUS:*/
		case OP_PREINCREMENT:
			op->type = slang_oper_preincrement;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		case OP_PREDECREMENT:
			op->type = slang_oper_predecrement;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		case OP_PLUS:
			op->type = slang_oper_plus;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		case OP_MINUS:
			op->type = slang_oper_minus;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		case OP_NOT:
			op->type = slang_oper_not;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		/*case OP_COMPLEMENT:*/
		case OP_SUBSCRIPT:
			op->type = slang_oper_subscript;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 2))
				return 0;
			break;
		case OP_CALL:
			op->type = slang_oper_call;
			op->a_id = parse_identifier (C);
			if (op->a_id == SLANG_ATOM_NULL)
				return 0;
			while (*C->I != OP_END)
				if (!parse_child_operation (C, O, op, 0))
					return 0;
			C->I++;
			if (!C->parsing_builtin && !slang_function_scope_find_by_name (O->funs, op->a_id, 1))
			{
				const char *id;

				id = slang_atom_pool_id (C->atoms, op->a_id);
				if (!is_constructor_name (id, op->a_id, O->structs))
				{
					slang_info_log_error (C->L, "%s: undeclared function name", id);
					return 0;
				}
			}
			break;
		case OP_FIELD:
			op->type = slang_oper_field;
			op->a_id = parse_identifier (C);
			if (op->a_id == SLANG_ATOM_NULL)
				return 0;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		case OP_POSTINCREMENT:
			op->type = slang_oper_postincrement;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		case OP_POSTDECREMENT:
			op->type = slang_oper_postdecrement;
			if (!handle_nary_expression (C, op, &ops, &num_ops, 1))
				return 0;
			break;
		default:
			return 0;
		}
	}
	C->I++;

	*oper = *ops;
	slang_alloc_free (ops);
	return 1;
}

/* parameter qualifier */
#define PARAM_QUALIFIER_IN 0
#define PARAM_QUALIFIER_OUT 1
#define PARAM_QUALIFIER_INOUT 2

/* function parameter array presence */
#define PARAMETER_ARRAY_NOT_PRESENT 0
#define PARAMETER_ARRAY_PRESENT 1

static int parse_parameter_declaration (slang_parse_ctx *C, slang_output_ctx *O,
	slang_variable *param)
{
	/* parse and validate the parameter's type qualifiers (there can be two at most) because
	 * not all combinations are valid */
	if (!parse_type_qualifier (C, &param->type.qualifier))
		return 0;
	switch (*C->I++)
	{
	case PARAM_QUALIFIER_IN:
		if (param->type.qualifier != slang_qual_const && param->type.qualifier != slang_qual_none)
		{
			slang_info_log_error (C->L, "invalid type qualifier");
			return 0;
		}
		break;
	case PARAM_QUALIFIER_OUT:
		if (param->type.qualifier == slang_qual_none)
			param->type.qualifier = slang_qual_out;
		else
		{
			slang_info_log_error (C->L, "invalid type qualifier");
			return 0;
		}
		break;
	case PARAM_QUALIFIER_INOUT:
		if (param->type.qualifier == slang_qual_none)
			param->type.qualifier = slang_qual_inout;
		else
		{
			slang_info_log_error (C->L, "invalid type qualifier");
			return 0;
		}
		break;
	default:
		return 0;
	}

	/* parse parameter's type specifier and name */
	if (!parse_type_specifier (C, O, &param->type.specifier))
		return 0;
	param->a_name = parse_identifier (C);
	if (param->a_name == SLANG_ATOM_NULL)
		return 0;

	/* if the parameter is an array, parse its size (the size must be explicitly defined */
	if (*C->I++ == PARAMETER_ARRAY_PRESENT)
	{
		slang_type_specifier p;

		if (!slang_type_specifier_construct (&p))
			return GL_FALSE;
		if (!slang_type_specifier_copy (&p, &param->type.specifier))
		{
			slang_type_specifier_destruct (&p);
			return GL_FALSE;
		}
		if (!convert_to_array (C, param, &p))
		{
			slang_type_specifier_destruct (&p);
			return GL_FALSE;
		}
		slang_type_specifier_destruct (&p);
		if (!parse_array_len (C, O, &param->array_len))
			return GL_FALSE;
	}

	/* calculate the parameter size */
	if (!calculate_var_size (C, O, param))
		return GL_FALSE;

	/* TODO: allocate the local address here? */
	return 1;
}

/* function type */
#define FUNCTION_ORDINARY 0
#define FUNCTION_CONSTRUCTOR 1
#define FUNCTION_OPERATOR 2

/* function parameter */
#define PARAMETER_NONE 0
#define PARAMETER_NEXT 1

/* operator type */
#define OPERATOR_ADDASSIGN 1
#define OPERATOR_SUBASSIGN 2
#define OPERATOR_MULASSIGN 3
#define OPERATOR_DIVASSIGN 4
/*#define OPERATOR_MODASSIGN 5*/
/*#define OPERATOR_LSHASSIGN 6*/
/*#define OPERATOR_RSHASSIGN 7*/
/*#define OPERATOR_ANDASSIGN 8*/
/*#define OPERATOR_XORASSIGN 9*/
/*#define OPERATOR_ORASSIGN 10*/
#define OPERATOR_LOGICALXOR 11
/*#define OPERATOR_BITOR 12*/
/*#define OPERATOR_BITXOR 13*/
/*#define OPERATOR_BITAND 14*/
#define OPERATOR_LESS 15
#define OPERATOR_GREATER 16
#define OPERATOR_LESSEQUAL 17
#define OPERATOR_GREATEREQUAL 18
/*#define OPERATOR_LSHIFT 19*/
/*#define OPERATOR_RSHIFT 20*/
#define OPERATOR_MULTIPLY 21
#define OPERATOR_DIVIDE 22
/*#define OPERATOR_MODULUS 23*/
#define OPERATOR_INCREMENT 24
#define OPERATOR_DECREMENT 25
#define OPERATOR_PLUS 26
#define OPERATOR_MINUS 27
/*#define OPERATOR_COMPLEMENT 28*/
#define OPERATOR_NOT 29

static const struct {
	unsigned int o_code;
	const char *o_name;
} operator_names[] = {
	{ OPERATOR_INCREMENT, "++" },
	{ OPERATOR_ADDASSIGN, "+=" },
	{ OPERATOR_PLUS, "+" },
	{ OPERATOR_DECREMENT, "--" },
	{ OPERATOR_SUBASSIGN, "-=" },
	{ OPERATOR_MINUS, "-" },
	{ OPERATOR_NOT, "!" },
	{ OPERATOR_MULASSIGN, "*=" },
	{ OPERATOR_MULTIPLY, "*" },
	{ OPERATOR_DIVASSIGN, "/=" },
	{ OPERATOR_DIVIDE, "/" },
	{ OPERATOR_LESSEQUAL, "<=" },
	/*{ OPERATOR_LSHASSIGN, "<<=" },*/
	/*{ OPERATOR_LSHIFT, "<<" },*/
	{ OPERATOR_LESS, "<" },
	{ OPERATOR_GREATEREQUAL, ">=" },
	/*{ OPERATOR_RSHASSIGN, ">>=" },*/
	/*{ OPERATOR_RSHIFT, ">>" },*/
	{ OPERATOR_GREATER, ">" },
	/*{ OPERATOR_MODASSIGN, "%=" },*/
	/*{ OPERATOR_MODULUS, "%" },*/
	/*{ OPERATOR_ANDASSIGN, "&=" },*/
	/*{ OPERATOR_BITAND, "&" },*/
	/*{ OPERATOR_ORASSIGN, "|=" },*/
	/*{ OPERATOR_BITOR, "|" },*/
	/*{ OPERATOR_COMPLEMENT, "~" },*/
	/*{ OPERATOR_XORASSIGN, "^=" },*/
	{ OPERATOR_LOGICALXOR, "^^" },
	/*{ OPERATOR_BITXOR, "^" }*/
};

static slang_atom parse_operator_name (slang_parse_ctx *C)
{
	unsigned int i;

	for (i = 0; i < sizeof (operator_names) / sizeof (*operator_names); i++)
	{
		if (operator_names[i].o_code == (unsigned int) (*C->I))
		{
			slang_atom atom = slang_atom_pool_atom (C->atoms, operator_names[i].o_name);
			if (atom == SLANG_ATOM_NULL)
			{
				slang_info_log_memory (C->L);
				return 0;
			}
			C->I++;
			return atom;
		}
	}
	return 0;
}

static int parse_function_prototype (slang_parse_ctx *C, slang_output_ctx *O, slang_function *func)
{
	/* parse function type and name */
	if (!parse_fully_specified_type (C, O, &func->header.type))
		return 0;
	switch (*C->I++)
	{
	case FUNCTION_ORDINARY:
		func->kind = slang_func_ordinary;
		func->header.a_name = parse_identifier (C);
		if (func->header.a_name == SLANG_ATOM_NULL)
			return 0;
		break;
	case FUNCTION_CONSTRUCTOR:
		func->kind = slang_func_constructor;
		if (func->header.type.specifier.type == slang_spec_struct)
			return 0;
		func->header.a_name = slang_atom_pool_atom (C->atoms,
			slang_type_specifier_type_to_string (func->header.type.specifier.type));
		if (func->header.a_name == SLANG_ATOM_NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		break;
	case FUNCTION_OPERATOR:
		func->kind = slang_func_operator;
		func->header.a_name = parse_operator_name (C);
		if (func->header.a_name == SLANG_ATOM_NULL)
			return 0;
		break;
	default:
		return 0;
	}

	/* parse function parameters */
	while (*C->I++ == PARAMETER_NEXT)
	{
		slang_variable *p;

		func->parameters->variables = (slang_variable *) slang_alloc_realloc (
			func->parameters->variables,
			func->parameters->num_variables * sizeof (slang_variable),
			(func->parameters->num_variables + 1) * sizeof (slang_variable));
		if (func->parameters->variables == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		p = &func->parameters->variables[func->parameters->num_variables];
		if (!slang_variable_construct (p))
			return 0;
		func->parameters->num_variables++;
		if (!parse_parameter_declaration (C, O, p))
			return 0;
	}

	/* function formal parameters and local variables share the same scope, so save
	 * the information about param count in a seperate place
	 * also link the scope to the global variable scope so when a given identifier is not
	 * found here, the search process continues in the global space */
	func->param_count = func->parameters->num_variables;
	func->parameters->outer_scope = O->vars;
	return 1;
}

static int parse_function_definition (slang_parse_ctx *C, slang_output_ctx *O, slang_function *func)
{
	slang_output_ctx o = *O;

	if (!parse_function_prototype (C, O, func))
		return 0;

	/* create function's body operation */
	func->body = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
	if (func->body == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	if (!slang_operation_construct (func->body))
	{
		slang_alloc_free (func->body);
		func->body = NULL;
		slang_info_log_memory (C->L);
		return 0;
	}

	/* to parse the body the parse context is modified in order to capture parsed variables
	 * into function's local variable scope */
	C->global_scope = 0;
	o.vars = func->parameters;
	if (!parse_statement (C, &o, func->body))
		return 0;
	C->global_scope = 1;
	return 1;
}

static GLboolean initialize_global (slang_assemble_ctx *A, slang_variable *var)
{
	slang_assembly_file_restore_point point;
	slang_machine mach;
	slang_assembly_local_info save_local = A->local;
	slang_operation op_id, op_assign;
	GLboolean result;

	/* save the current assembly */
	if (!slang_assembly_file_restore_point_save (A->file, &point))
		return GL_FALSE;

	/* setup the machine */
	mach = *A->mach;
	mach.ip = A->file->count;

	/* allocate local storage for expression */
	A->local.ret_size = 0;
	A->local.addr_tmp = 0;
	A->local.swizzle_tmp = 4;
	if (!slang_assembly_file_push_label (A->file, slang_asm_local_alloc, 20))
		return GL_FALSE;
	if (!slang_assembly_file_push_label (A->file, slang_asm_enter, 20))
		return GL_FALSE;

	/* construct the left side of assignment */
	if (!slang_operation_construct (&op_id))
		return GL_FALSE;
	op_id.type = slang_oper_identifier;
	op_id.a_id = var->a_name;

	/* put the variable into operation's scope */
	op_id.locals->variables = (slang_variable *) slang_alloc_malloc (sizeof (slang_variable));
	if (op_id.locals->variables == NULL)
	{
		slang_operation_destruct (&op_id);
		return GL_FALSE;
	}
	op_id.locals->num_variables = 1;
	op_id.locals->variables[0] = *var;

	/* construct the assignment expression */
	if (!slang_operation_construct (&op_assign))
	{
		op_id.locals->num_variables = 0;
		slang_operation_destruct (&op_id);
		return GL_FALSE;
	}
	op_assign.type = slang_oper_assign;
	op_assign.children = (slang_operation *) slang_alloc_malloc (2 * sizeof (slang_operation));
	if (op_assign.children == NULL)
	{
		slang_operation_destruct (&op_assign);
		op_id.locals->num_variables = 0;
		slang_operation_destruct (&op_id);
		return GL_FALSE;
	}
	op_assign.num_children = 2;
	op_assign.children[0] = op_id;
	op_assign.children[1] = *var->initializer;

	/* insert the actual expression */
	result = _slang_assemble_operation (A, &op_assign, slang_ref_forbid);

	/* carefully destroy the operations */
	op_assign.num_children = 0;
	slang_alloc_free (op_assign.children);
	op_assign.children = NULL;
	slang_operation_destruct (&op_assign);
	op_id.locals->num_variables = 0;
	slang_operation_destruct (&op_id);

	if (!result)
		return GL_FALSE;
	if (!slang_assembly_file_push (A->file, slang_asm_exit))
		return GL_FALSE;

	/* execute the expression */
	if (!_slang_execute2 (A->file, &mach))
		return GL_FALSE;

	/* restore the old assembly */
	if (!slang_assembly_file_restore_point_load (A->file, &point))
		return GL_FALSE;
	A->local = save_local;

	/* now we copy the contents of the initialized variable back to the original machine */
	_mesa_memcpy ((GLubyte *) A->mach->mem + var->address, (GLubyte *) mach.mem + var->address,
		var->size);

	return GL_TRUE;
}

/* init declarator list */
#define DECLARATOR_NONE 0
#define DECLARATOR_NEXT 1

/* variable declaration */
#define VARIABLE_NONE 0
#define VARIABLE_IDENTIFIER 1
#define VARIABLE_INITIALIZER 2
#define VARIABLE_ARRAY_EXPLICIT 3
#define VARIABLE_ARRAY_UNKNOWN 4

static int parse_init_declarator (slang_parse_ctx *C, slang_output_ctx *O,
	const slang_fully_specified_type *type)
{
	slang_variable *var;

	/* empty init declatator (without name, e.g. "float ;") */
	if (*C->I++ == VARIABLE_NONE)
		return 1;

	/* make room for the new variable and initialize it */
	O->vars->variables = (slang_variable *) slang_alloc_realloc (O->vars->variables,
		O->vars->num_variables * sizeof (slang_variable),
		(O->vars->num_variables + 1) * sizeof (slang_variable));
	if (O->vars->variables == NULL)
	{
		slang_info_log_memory (C->L);
		return 0;
	}
	var = &O->vars->variables[O->vars->num_variables];
	if (!slang_variable_construct (var))
		return 0;
	O->vars->num_variables++;

	/* copy the declarator qualifier type, parse the identifier */
	var->global = C->global_scope;
	var->type.qualifier = type->qualifier;
	var->a_name = parse_identifier (C);
	if (var->a_name == SLANG_ATOM_NULL)
		return 0;

	switch (*C->I++)
	{
	case VARIABLE_NONE:
		/* simple variable declarator - just copy the specifier */
		if (!slang_type_specifier_copy (&var->type.specifier, &type->specifier))
			return 0;
		break;
	case VARIABLE_INITIALIZER:
		/* initialized variable - copy the specifier and parse the expression */
		if (!slang_type_specifier_copy (&var->type.specifier, &type->specifier))
			return 0;
		var->initializer = (slang_operation *) slang_alloc_malloc (sizeof (slang_operation));
		if (var->initializer == NULL)
		{
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!slang_operation_construct (var->initializer))
		{
			slang_alloc_free (var->initializer);
			var->initializer = NULL;
			slang_info_log_memory (C->L);
			return 0;
		}
		if (!parse_expression (C, O, var->initializer))
			return 0;
		break;
#if 0
	case VARIABLE_ARRAY_UNKNOWN:
		/* unsized array - mark it as array and copy the specifier to the array element */
		if (!convert_to_array (C, var, &type->specifier))
			return GL_FALSE;
		break;
#endif
	case VARIABLE_ARRAY_EXPLICIT:
		if (!convert_to_array (C, var, &type->specifier))
			return GL_FALSE;
		if (!parse_array_len (C, O, &var->array_len))
			return GL_FALSE;
		break;
	default:
		return 0;
	}

	/* allocate global address space for a variable with a known size */
	if (C->global_scope && !(var->type.specifier.type == slang_spec_array && var->array_len == 0))
	{
		if (!calculate_var_size (C, O, var))
			return GL_FALSE;
		var->address = slang_var_pool_alloc (O->global_pool, var->size);
	}

	/* initialize global variable */
	if (C->global_scope && var->initializer != NULL)
	{
		slang_assemble_ctx A;

		A.file = O->assembly;
		A.mach = O->machine;
		A.atoms = C->atoms;
		A.space.funcs = O->funs;
		A.space.structs = O->structs;
		A.space.vars = O->vars;
		if (!initialize_global (&A, var))
			return 0;
	}
	return 1;
}

static int parse_init_declarator_list (slang_parse_ctx *C, slang_output_ctx *O)
{
	slang_fully_specified_type type;

	/* parse the fully specified type, common to all declarators */
	if (!slang_fully_specified_type_construct (&type))
		return 0;
	if (!parse_fully_specified_type (C, O, &type))
	{
		slang_fully_specified_type_destruct (&type);
		return 0;
	}

	/* parse declarators, pass-in the parsed type */
	do
	{
		if (!parse_init_declarator (C, O, &type))
		{
			slang_fully_specified_type_destruct (&type);
			return 0;
		}
	}
	while (*C->I++ == DECLARATOR_NEXT);

	slang_fully_specified_type_destruct (&type);
	return 1;
}

static int parse_function (slang_parse_ctx *C, slang_output_ctx *O, int definition,
	slang_function **parsed_func_ret)
{
	slang_function parsed_func, *found_func;

	/* parse function definition/declaration */
	if (!slang_function_construct (&parsed_func))
		return 0;
	if (definition)
	{
		if (!parse_function_definition (C, O, &parsed_func))
		{
			slang_function_destruct (&parsed_func);
			return 0;
		}
	}
	else
	{
		if (!parse_function_prototype (C, O, &parsed_func))
		{
			slang_function_destruct (&parsed_func);
			return 0;
		}
	}

	/* find a function with a prototype matching the parsed one - only the current scope
	 * is being searched to allow built-in function overriding */
	found_func = slang_function_scope_find (O->funs, &parsed_func, 0);
	if (found_func == NULL)
	{
		/* add the parsed function to the function list */
		O->funs->functions = (slang_function *) slang_alloc_realloc (O->funs->functions,
			O->funs->num_functions * sizeof (slang_function),
			(O->funs->num_functions + 1) * sizeof (slang_function));
		if (O->funs->functions == NULL)
		{
			slang_info_log_memory (C->L);
			slang_function_destruct (&parsed_func);
			return 0;
		}
		O->funs->functions[O->funs->num_functions] = parsed_func;
		O->funs->num_functions++;

		/* return the newly parsed function */
		*parsed_func_ret = &O->funs->functions[O->funs->num_functions - 1];
	}
	else
	{
		/* TODO: check function return type qualifiers and specifiers */
		if (definition)
		{
			if (found_func->body != NULL)
			{
				slang_info_log_error (C->L, "%s: function already has a body",
					slang_atom_pool_id (C->atoms, parsed_func.header.a_name));
				slang_function_destruct (&parsed_func);
				return 0;
			}

			/* destroy the existing function declaration and replace it with the new one,
			 * remember to save the fixup table */
			parsed_func.fixups = found_func->fixups;
			slang_fixup_table_init (&found_func->fixups);
			slang_function_destruct (found_func);
			*found_func = parsed_func;
		}
		else
		{
			/* another declaration of the same function prototype - ignore it */
			slang_function_destruct (&parsed_func);
		}

		/* return the found function */
		*parsed_func_ret = found_func;
	}

	/* assemble the parsed function */
	{
		slang_assemble_ctx A;

		A.file = O->assembly;
		A.mach = O->machine;
		A.atoms = C->atoms;
		A.space.funcs = O->funs;
		A.space.structs = O->structs;
		A.space.vars = O->vars;
		if (!_slang_assemble_function (&A, *parsed_func_ret))
			return 0;
	}
	return 1;
}

/* declaration */
#define DECLARATION_FUNCTION_PROTOTYPE 1
#define DECLARATION_INIT_DECLARATOR_LIST 2

static int parse_declaration (slang_parse_ctx *C, slang_output_ctx *O)
{
	switch (*C->I++)
	{
	case DECLARATION_INIT_DECLARATOR_LIST:
		if (!parse_init_declarator_list (C, O))
			return 0;
		break;
	case DECLARATION_FUNCTION_PROTOTYPE:
		{
			slang_function *dummy_func;

			if (!parse_function (C, O, 0, &dummy_func))
				return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

/* external declaration */
#define EXTERNAL_NULL 0
#define EXTERNAL_FUNCTION_DEFINITION 1
#define EXTERNAL_DECLARATION 2

static int parse_translation_unit (slang_parse_ctx *C, slang_translation_unit *unit)
{
	slang_output_ctx o;

	/* setup output context */
	o.funs = &unit->functions;
	o.structs = &unit->structs;
	o.vars = &unit->globals;
	o.assembly = unit->assembly;
	o.global_pool = unit->global_pool;
	o.machine = unit->machine;

	/* parse individual functions and declarations */
	while (*C->I != EXTERNAL_NULL)
	{
		switch (*C->I++)
		{
		case EXTERNAL_FUNCTION_DEFINITION:
			{
				slang_function *func;

				if (!parse_function (C, &o, 1, &func))
					return 0;
			}
			break;
		case EXTERNAL_DECLARATION:
			if (!parse_declaration (C, &o))
				return 0;
			break;
		default:
			return 0;
		}
	}
	C->I++;
	return 1;
}

#define BUILTIN_CORE 0
#define BUILTIN_COMMON 1
#define BUILTIN_TARGET 2
#define BUILTIN_TOTAL 3

static int compile_binary (const byte *prod, slang_translation_unit *unit, slang_unit_type type,
	slang_info_log *log, slang_translation_unit *builtins, slang_assembly_file *file,
	slang_var_pool *pool, slang_machine *mach, slang_translation_unit *downlink,
	slang_atom_pool *atoms)
{
	slang_parse_ctx C;

	/* create translation unit object */
	if (file != NULL)
	{
		if (!slang_translation_unit_construct2 (unit, file, pool, mach, atoms))
			return 0;
		unit->type = type;
	}

	/* set-up parse context */
	C.I = prod;
	C.L = log;
	C.parsing_builtin = builtins == NULL;
	C.global_scope = 1;
	C.atoms = unit->atom_pool;

	if (!check_revision (&C))
	{
		slang_translation_unit_destruct (unit);
		return 0;
	}

	if (downlink != NULL)
	{
		unit->functions.outer_scope = &downlink->functions;
		unit->globals.outer_scope = &downlink->globals;
		unit->structs.outer_scope = &downlink->structs;
	}

	/* parse translation unit */
	if (!parse_translation_unit (&C, unit))
	{
		slang_translation_unit_destruct (unit);
		return 0;
	}

	return 1;
}

static int compile_with_grammar (grammar id, const char *source, slang_translation_unit *unit,
	slang_unit_type type, slang_info_log *log, slang_translation_unit *builtins)
{
	byte *prod;
	unsigned int size, start, version;

	/* retrieve version */
	if (!_slang_preprocess_version (source, &version, &start, log))
		return 0;

	/* check the syntax and generate its binary representation */
	if (!grammar_fast_check (id, (const byte *) source + start, &prod, &size, 65536))
	{
		char buf[1024];
		unsigned int pos;
		grammar_get_last_error ( (unsigned char*) buf, 1024, (int*) &pos);
		slang_info_log_error (log, buf);
		return 0;
	}

	/* syntax is okay - translate it to internal representation */
	if (!compile_binary (prod, unit, type, log, builtins, NULL, NULL, NULL,
			&builtins[BUILTIN_TARGET], NULL))
	{
		grammar_alloc_free (prod);
		return 0;
	}

	grammar_alloc_free (prod);
	return 1;
}

static const char *slang_shader_syn =
#include "library/slang_shader_syn.h"
;

static const byte slang_core_gc[] = {
#include "library/slang_core_gc.h"
};

static const byte slang_common_builtin_gc[] = {
#include "library/slang_common_builtin_gc.h"
};

static const byte slang_fragment_builtin_gc[] = {
#include "library/slang_fragment_builtin_gc.h"
};

static const byte slang_vertex_builtin_gc[] = {
#include "library/slang_vertex_builtin_gc.h"
};

static int compile (grammar *id, slang_translation_unit *builtin_units, int *compiled,
	const char *source, slang_translation_unit *unit, slang_unit_type type, slang_info_log *log)
{
	slang_translation_unit *builtins = NULL;

	/* load slang grammar */
	*id = grammar_load_from_text ((const byte *) (slang_shader_syn));
	if (*id == 0)
	{
		byte buf[1024];
		int pos;

		grammar_get_last_error (buf, 1024, &pos);
		slang_info_log_error (log, (const char *) (buf));
		return 0;
	}

	/* set shader type - the syntax is slightly different for different shaders */
	if (type == slang_unit_fragment_shader || type == slang_unit_fragment_builtin)
		grammar_set_reg8 (*id, (const byte *) "shader_type", 1);
	else
		grammar_set_reg8 (*id, (const byte *) "shader_type", 2);

	/* enable language extensions */
	grammar_set_reg8 (*id, (const byte *) "parsing_builtin", 1);

	/* if parsing user-specified shader, load built-in library */
	if (type == slang_unit_fragment_shader || type == slang_unit_vertex_shader)
	{
		/* compile core functionality first */
		if (!compile_binary (slang_core_gc, &builtin_units[BUILTIN_CORE],
				slang_unit_fragment_builtin, log, NULL, unit->assembly, unit->global_pool,
				unit->machine, NULL, unit->atom_pool))
			return 0;
		compiled[BUILTIN_CORE] = 1;

		/* compile common functions and variables, link to core */
		if (!compile_binary (slang_common_builtin_gc, &builtin_units[BUILTIN_COMMON],
				slang_unit_fragment_builtin, log, NULL, unit->assembly, unit->global_pool,
				unit->machine, &builtin_units[BUILTIN_CORE], unit->atom_pool))
			return 0;
		compiled[BUILTIN_COMMON] = 1;

		/* compile target-specific functions and variables, link to common */
		if (type == slang_unit_fragment_shader)
		{
			if (!compile_binary (slang_fragment_builtin_gc, &builtin_units[BUILTIN_TARGET],
					slang_unit_fragment_builtin, log, NULL, unit->assembly, unit->global_pool,
					unit->machine, &builtin_units[BUILTIN_COMMON], unit->atom_pool))
				return 0;
		}
		else if (type == slang_unit_vertex_shader)
		{
			if (!compile_binary (slang_vertex_builtin_gc, &builtin_units[BUILTIN_TARGET],
					slang_unit_vertex_builtin, log, NULL, unit->assembly, unit->global_pool,
					unit->machine, &builtin_units[BUILTIN_COMMON], unit->atom_pool))
				return 0;
		}
		compiled[BUILTIN_TARGET] = 1;

		/* disable language extensions */
		grammar_set_reg8 (*id, (const byte *) "parsing_builtin", 0);
		builtins = builtin_units;
	}

	/* compile the actual shader - pass-in built-in library for external shader */
	if (!compile_with_grammar (*id, source, unit, type, log, builtins))
		return 0;

	return 1;
}

int _slang_compile (const char *source, slang_translation_unit *unit, slang_unit_type type,
	slang_info_log *log)
{
	int success;
	grammar id = 0;
/*	slang_translation_unit builtin_units[BUILTIN_TOTAL];*/
	slang_translation_unit *builtin_units;
	int compiled[BUILTIN_TOTAL] = { 0 };

	/* create the main unit first */
	if (!slang_translation_unit_construct (unit))
		return 0;
	unit->type = type;

	builtin_units = (slang_translation_unit *) slang_alloc_malloc (BUILTIN_TOTAL * sizeof (slang_translation_unit));
	success = compile (&id, builtin_units, compiled, source, unit, type, log);

	/* destroy built-in library */
	/* XXX: free with the unit */
	/*if (type == slang_unit_fragment_shader || type == slang_unit_vertex_shader)
	{
		int i;

		for (i = 0; i < BUILTIN_TOTAL; i++)
			if (compiled[i] != 0)
				slang_translation_unit_destruct (&builtin_units[i]);
	}*/
	if (id != 0)
		grammar_destroy (id);

	if (!success)
		return 0;
	unit->exp_data.atoms = unit->atom_pool;
	if (!_slang_build_export_data_table (&unit->exp_data, &unit->globals))
		return 0;
	if (!_slang_gather_active_uniforms (&unit->uniforms, &unit->exp_data))
		return 0;

	return 1;
}

