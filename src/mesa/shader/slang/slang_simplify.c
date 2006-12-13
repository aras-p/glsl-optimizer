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

/**
 * \file slang_assemble_typeinfo.c
 * slang type info
 * \author Michal Krol
 */

#include "imports.h"
#include "macros.h"
#include "slang_compile.h"
#include "slang_simplify.h"


/**
 * Recursively traverse an AST tree, applying simplifications wherever
 * possible.
 * At the least, we do constant folding.  We need to do that much so that
 * compile-time expressions can be evaluated for things like array
 * declarations.  I.e.:  float foo[3 + 5];
 */
void
slang_simplify(slang_operation *oper,
               const slang_assembly_name_space * space,
               slang_atom_pool * atoms)
{
   GLboolean isFloat[4];
   GLboolean isBool[4];
   GLuint i, n;

   /* first, simplify children */
   for (i = 0; i < oper->num_children; i++) {
      slang_simplify(&oper->children[i], space, atoms);
   }

   /* examine children */
   n = MIN2(oper->num_children, 4);
   for (i = 0; i < n; i++) {
      isFloat[i] = (oper->children[i].type == slang_oper_literal_float ||
                   oper->children[i].type == slang_oper_literal_int);
      isBool[i] = (oper->children[i].type == slang_oper_literal_bool);
   }
                              
   if (n == 2 && isFloat[0] && isFloat[1]) {
      /* probably simple arithmetic */
      switch (oper->type) {
      case slang_oper_add:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] + oper->children[1].literal[i];
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_float;
         break;
      case slang_oper_subtract:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] - oper->children[1].literal[i];
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_float;
         break;
      case slang_oper_multiply:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] * oper->children[1].literal[i];
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_float;
         break;
      case slang_oper_divide:
         for (i = 0; i < 4; i++) {
            oper->literal[i]
               = oper->children[0].literal[i] / oper->children[1].literal[i];
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_float;
         break;
      default:
         ; /* nothing */
      }
   }
   else if (n == 1 && isFloat[0]) {
      switch (oper->type) {
      case slang_oper_minus:
         for (i = 0; i < 4; i++) {
            oper->literal[i] = -oper->children[0].literal[i];
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_float;
         break;
      case slang_oper_plus:
         COPY_4V(oper->literal, oper->children[0].literal);
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_float;
         break;
      default:
         ; /* nothing */
      }
   }
   else if (n == 2 && isBool[0] && isBool[1]) {
      /* simple boolean expression */
      switch (oper->type) {
      case slang_oper_logicaland:
         for (i = 0; i < 4; i++) {
            const GLint a = oper->children[0].literal[i] ? 1 : 0;
            const GLint b = oper->children[1].literal[i] ? 1 : 0;
            oper->literal[i] = (GLfloat) (a && b);
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_bool;
         break;
      case slang_oper_logicalor:
         for (i = 0; i < 4; i++) {
            const GLint a = oper->children[0].literal[i] ? 1 : 0;
            const GLint b = oper->children[1].literal[i] ? 1 : 0;
            oper->literal[i] = (GLfloat) (a || b);
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_bool;
         break;
      case slang_oper_logicalxor:
         for (i = 0; i < 4; i++) {
            const GLint a = oper->children[0].literal[i] ? 1 : 0;
            const GLint b = oper->children[1].literal[i] ? 1 : 0;
            oper->literal[i] = (GLfloat) (a ^ b);
         }
         slang_operation_destruct(oper);
         oper->type = slang_oper_literal_bool;
         break;
      default:
         ; /* nothing */
      }
   }
   else if (n == 4 && isFloat[0] && isFloat[1] && isFloat[2] && isFloat[3]) {
      if (oper->type == slang_oper_call) {
         if (strcmp((char *) oper->a_id, "vec4") == 0) {
            oper->literal[0] = oper->children[0].literal[0];
            oper->literal[1] = oper->children[1].literal[0];
            oper->literal[2] = oper->children[2].literal[0];
            oper->literal[3] = oper->children[3].literal[0];
            slang_operation_destruct(oper);
            oper->type = slang_oper_literal_float;
         }
      }
   }
}

