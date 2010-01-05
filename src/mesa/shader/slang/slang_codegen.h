/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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


#ifndef SLANG_CODEGEN_H
#define SLANG_CODEGEN_H


#include "main/imports.h"
#include "slang_compile.h"


#define MAX_LOOP_DEPTH 30


typedef struct slang_assemble_ctx_
{
   slang_atom_pool *atoms;
   slang_name_space space;
   struct gl_program *program;
   struct gl_sl_pragmas *pragmas;
   slang_var_table *vartable;
   slang_info_log *log;
   GLboolean allow_uniform_initializers;

   /* current loop stack */
   const slang_operation *LoopOperStack[MAX_LOOP_DEPTH];
   struct slang_ir_node_ *LoopIRStack[MAX_LOOP_DEPTH];
   GLuint LoopDepth;

   /* current function */
   struct slang_function_ *CurFunction;
   struct slang_label_ *curFuncEndLabel;
   GLboolean UseReturnFlag;

   GLboolean UnresolvedRefs;
   GLboolean EmitContReturn;
} slang_assemble_ctx;


extern GLuint
_slang_sizeof_type_specifier(const slang_type_specifier *spec);

extern GLboolean
_slang_codegen_function(slang_assemble_ctx *A , struct slang_function_ *fun);

extern GLboolean
_slang_codegen_global_variable(slang_assemble_ctx *A, slang_variable *var,
                               slang_unit_type type);


#endif /* SLANG_CODEGEN_H */
