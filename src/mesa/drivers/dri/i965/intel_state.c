/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/dd.h"

#include "intel_screen.h"
#include "brw_context.h"
#include "brw_defines.h"

int
intel_translate_shadow_compare_func(GLenum func)
{
   /* GL specifies the result of shadow comparisons as:
    *     1     if   ref <op> texel,
    *     0     otherwise.
    *
    * The hardware does:
    *     0     if texel <op> ref,
    *     1     otherwise.
    *
    * So, these look a bit strange because there's both a negation
    * and swapping of the arguments involved.
    */
   switch (func) {
   case GL_NEVER:
      return BRW_COMPAREFUNCTION_ALWAYS;
   case GL_LESS:
      return BRW_COMPAREFUNCTION_LEQUAL;
   case GL_LEQUAL:
      return BRW_COMPAREFUNCTION_LESS;
   case GL_GREATER:
      return BRW_COMPAREFUNCTION_GEQUAL;
   case GL_GEQUAL:
      return BRW_COMPAREFUNCTION_GREATER;
   case GL_NOTEQUAL:
      return BRW_COMPAREFUNCTION_EQUAL;
   case GL_EQUAL:
      return BRW_COMPAREFUNCTION_NOTEQUAL;
   case GL_ALWAYS:
      return BRW_COMPAREFUNCTION_NEVER;
   }

   assert(!"Invalid shadow comparison function.");
   return BRW_COMPAREFUNCTION_NEVER;
}

int
intel_translate_compare_func(GLenum func)
{
   switch (func) {
   case GL_NEVER:
      return BRW_COMPAREFUNCTION_NEVER;
   case GL_LESS:
      return BRW_COMPAREFUNCTION_LESS;
   case GL_LEQUAL:
      return BRW_COMPAREFUNCTION_LEQUAL;
   case GL_GREATER:
      return BRW_COMPAREFUNCTION_GREATER;
   case GL_GEQUAL:
      return BRW_COMPAREFUNCTION_GEQUAL;
   case GL_NOTEQUAL:
      return BRW_COMPAREFUNCTION_NOTEQUAL;
   case GL_EQUAL:
      return BRW_COMPAREFUNCTION_EQUAL;
   case GL_ALWAYS:
      return BRW_COMPAREFUNCTION_ALWAYS;
   }

   assert(!"Invalid comparison function.");
   return BRW_COMPAREFUNCTION_ALWAYS;
}

int
intel_translate_stencil_op(GLenum op)
{
   switch (op) {
   case GL_KEEP:
      return BRW_STENCILOP_KEEP;
   case GL_ZERO:
      return BRW_STENCILOP_ZERO;
   case GL_REPLACE:
      return BRW_STENCILOP_REPLACE;
   case GL_INCR:
      return BRW_STENCILOP_INCRSAT;
   case GL_DECR:
      return BRW_STENCILOP_DECRSAT;
   case GL_INCR_WRAP:
      return BRW_STENCILOP_INCR;
   case GL_DECR_WRAP:
      return BRW_STENCILOP_DECR;
   case GL_INVERT:
      return BRW_STENCILOP_INVERT;
   default:
      return BRW_STENCILOP_ZERO;
   }
}

int
intel_translate_logic_op(GLenum opcode)
{
   switch (opcode) {
   case GL_CLEAR:
      return BRW_LOGICOPFUNCTION_CLEAR;
   case GL_AND:
      return BRW_LOGICOPFUNCTION_AND;
   case GL_AND_REVERSE:
      return BRW_LOGICOPFUNCTION_AND_REVERSE;
   case GL_COPY:
      return BRW_LOGICOPFUNCTION_COPY;
   case GL_COPY_INVERTED:
      return BRW_LOGICOPFUNCTION_COPY_INVERTED;
   case GL_AND_INVERTED:
      return BRW_LOGICOPFUNCTION_AND_INVERTED;
   case GL_NOOP:
      return BRW_LOGICOPFUNCTION_NOOP;
   case GL_XOR:
      return BRW_LOGICOPFUNCTION_XOR;
   case GL_OR:
      return BRW_LOGICOPFUNCTION_OR;
   case GL_OR_INVERTED:
      return BRW_LOGICOPFUNCTION_OR_INVERTED;
   case GL_NOR:
      return BRW_LOGICOPFUNCTION_NOR;
   case GL_EQUIV:
      return BRW_LOGICOPFUNCTION_EQUIV;
   case GL_INVERT:
      return BRW_LOGICOPFUNCTION_INVERT;
   case GL_OR_REVERSE:
      return BRW_LOGICOPFUNCTION_OR_REVERSE;
   case GL_NAND:
      return BRW_LOGICOPFUNCTION_NAND;
   case GL_SET:
      return BRW_LOGICOPFUNCTION_SET;
   default:
      return BRW_LOGICOPFUNCTION_SET;
   }
}
