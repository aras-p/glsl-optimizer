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

#if !defined SLANG_COMPILE_VARIABLE_H
#define SLANG_COMPILE_VARIABLE_H

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_type_qualifier_
{
	slang_qual_none,
	slang_qual_const,
	slang_qual_attribute,
	slang_qual_varying,
	slang_qual_uniform,
	slang_qual_out,
	slang_qual_inout,
	slang_qual_fixedoutput,	/* internal */
	slang_qual_fixedinput	/* internal */
} slang_type_qualifier;

slang_type_specifier_type slang_type_specifier_type_from_string (const char *);
const char *slang_type_specifier_type_to_string (slang_type_specifier_type);

typedef struct slang_fully_specified_type_
{
	slang_type_qualifier qualifier;
	slang_type_specifier specifier;
} slang_fully_specified_type;

int slang_fully_specified_type_construct (slang_fully_specified_type *);
void slang_fully_specified_type_destruct (slang_fully_specified_type *);
int slang_fully_specified_type_copy (slang_fully_specified_type *, const slang_fully_specified_type *);

typedef struct slang_variable_scope_
{
	struct slang_variable_ *variables;
	unsigned int num_variables;
	struct slang_variable_scope_ *outer_scope;
} slang_variable_scope;

int slang_variable_scope_construct (slang_variable_scope *);
void slang_variable_scope_destruct (slang_variable_scope *);
int slang_variable_scope_copy (slang_variable_scope *, const slang_variable_scope *);

typedef struct slang_variable_
{
	slang_fully_specified_type type;
	slang_atom a_name;
	GLuint array_len;					/* type: spec_array */
	struct slang_operation_ *initializer;
	unsigned int address;
	unsigned int size;
	int global;
} slang_variable;

int slang_variable_construct (slang_variable *);
void slang_variable_destruct (slang_variable *);
int slang_variable_copy (slang_variable *, const slang_variable *);

slang_variable *_slang_locate_variable (slang_variable_scope *, slang_atom a_name, GLboolean all);

GLboolean _slang_build_export_data_table (slang_export_data_table *, slang_variable_scope *);

#ifdef __cplusplus
}
#endif

#endif

