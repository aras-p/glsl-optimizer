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

#if !defined SLANG_ASSEMBLE_TYPEINFO_H
#define SLANG_ASSEMBLE_TYPEINFO_H

#include "slang_assemble_constructor.h"
#include "slang_compile.h"

#if defined __cplusplus
extern "C" {
#endif

typedef struct slang_assembly_typeinfo_
{
	GLboolean can_be_referenced;
	GLboolean is_swizzled;
	slang_swizzle swz;
	slang_type_specifier spec;
	slang_operation *array_size;
} slang_assembly_typeinfo;

GLboolean slang_assembly_typeinfo_construct (slang_assembly_typeinfo *);
GLvoid slang_assembly_typeinfo_destruct (slang_assembly_typeinfo *);

/*
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
GLboolean _slang_typeof_operation (slang_operation *, slang_assembly_name_space *,
	slang_assembly_typeinfo *, slang_atom_pool *);

/*
 * Retrieves type of a function prototype, if one exists.
 * Returns GL_TRUE on success, even if the function was not found.
 * Returns GL_FALSE otherwise.
 */
GLboolean _slang_typeof_function (slang_atom a_name, slang_operation *params, GLuint num_params,
	slang_assembly_name_space *, slang_type_specifier *spec, GLboolean *exists, slang_atom_pool *);

GLboolean _slang_type_is_matrix (slang_type_specifier_type);

GLboolean _slang_type_is_vector (slang_type_specifier_type);

slang_type_specifier_type _slang_type_base (slang_type_specifier_type);

GLuint _slang_type_dim (slang_type_specifier_type);

#ifdef __cplusplus
}
#endif

#endif

