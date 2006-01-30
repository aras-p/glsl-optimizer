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

#if !defined SLANG_COMPILE_H
#define SLANG_COMPILE_H

#include "slang_compile_variable.h"
#include "slang_compile_struct.h"
#include "slang_compile_operation.h"
#include "slang_compile_function.h"

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_unit_type_
{
	slang_unit_fragment_shader,
	slang_unit_vertex_shader,
	slang_unit_fragment_builtin,
	slang_unit_vertex_builtin
} slang_unit_type;

typedef struct slang_var_pool_
{
	GLuint next_addr;
} slang_var_pool;

typedef struct slang_translation_unit_
{
	slang_variable_scope globals;
	slang_function_scope functions;
	slang_struct_scope structs;
	slang_unit_type type;
	struct slang_assembly_file_ *assembly;
	slang_var_pool global_pool;
} slang_translation_unit;

int slang_translation_unit_construct (slang_translation_unit *);
void slang_translation_unit_destruct (slang_translation_unit *);

typedef struct slang_info_log_
{
	char *text;
	int dont_free_text;
} slang_info_log;

void slang_info_log_construct (slang_info_log *);
void slang_info_log_destruct (slang_info_log *);
int slang_info_log_error (slang_info_log *, const char *, ...);
int slang_info_log_warning (slang_info_log *, const char *, ...);
void slang_info_log_memory (slang_info_log *);

int _slang_compile (const char *, slang_translation_unit *, slang_unit_type type, slang_info_log *);

#ifdef __cplusplus
}
#endif

#endif

