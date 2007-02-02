/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
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

#ifndef SLANG_ASSEMBLE_H
#define SLANG_ASSEMBLE_H

#include "imports.h"
#include "mtypes.h"
#include "slang_utility.h"
#include "slang_vartable.h"


struct slang_operation_;


/**
 * Holds complete information about vector swizzle - the <swizzle>
 * array contains vector component source indices, where 0 is "x", 1
 * is "y", 2 is "z" and 3 is "w".
 * Example: "xwz" --> { 3, { 0, 3, 2, not used } }.
 */
typedef struct slang_swizzle_
{
   GLuint num_components;
   GLuint swizzle[4];
} slang_swizzle;

typedef struct slang_assembly_name_space_
{
   struct slang_function_scope_ *funcs;
   struct slang_struct_scope_ *structs;
   struct slang_variable_scope_ *vars;
} slang_assembly_name_space;


typedef struct slang_assemble_ctx_
{
   slang_atom_pool *atoms;
   slang_assembly_name_space space;
   slang_swizzle swz;
   struct gl_program *program;
   slang_var_table *vartable;

   struct slang_function_ *CurFunction;
   slang_atom CurLoopBreak;
   slang_atom CurLoopCont;
} slang_assemble_ctx;

extern struct slang_function_ *
_slang_locate_function(const struct slang_function_scope_ *funcs,
                       slang_atom name, const struct slang_operation_ *params,
                       GLuint num_params,
                       const slang_assembly_name_space *space,
                       slang_atom_pool *);


extern GLboolean
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle *swz);

extern GLboolean
_slang_is_swizzle_mask(const slang_swizzle *swz, GLuint rows);

extern GLvoid
_slang_multiply_swizzles(slang_swizzle *, const slang_swizzle *,
                         const slang_swizzle *);

#include "slang_assemble_typeinfo.h"

#endif
