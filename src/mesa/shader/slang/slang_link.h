/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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

#if !defined SLANG_LINK_H
#define SLANG_LINK_H

#include "slang_assemble.h"
#include "slang_execute.h"

#if defined __cplusplus
extern "C" {
#endif

#define SLANG_UNIFORM_BINDING_VERTEX 0
#define SLANG_UNIFORM_BINDING_FRAGMENT 1
#define SLANG_UNIFORM_BINDING_MAX 2

typedef struct
{
	slang_export_data_quant *quant;
	char *name;
	GLuint address[SLANG_UNIFORM_BINDING_MAX];
} slang_uniform_binding;

typedef struct
{
	slang_uniform_binding *table;
	GLuint count;
} slang_uniform_bindings;

typedef struct
{
	slang_uniform_bindings uniforms;
	slang_machine *machines[SLANG_UNIFORM_BINDING_MAX];
} slang_program;

GLvoid slang_program_ctr (slang_program *);
GLvoid slang_program_dtr (slang_program *);

GLboolean _slang_link (slang_program *, slang_translation_unit **, GLuint);

#ifdef __cplusplus
}
#endif

#endif

