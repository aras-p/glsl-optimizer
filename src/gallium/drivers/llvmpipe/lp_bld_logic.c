/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


#include "pipe/p_defines.h"
#include "lp_bld_type.h"
#include "lp_bld_intr.h"
#include "lp_bld_logic.h"


LLVMValueRef
lp_build_cmp(struct lp_build_context *bld,
             unsigned func,
             LLVMValueRef a,
             LLVMValueRef b)
{
   const union lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
   LLVMValueRef zeros = LLVMConstNull(int_vec_type);
   LLVMValueRef ones = LLVMConstAllOnes(int_vec_type);
   LLVMValueRef cond;

   if(func == PIPE_FUNC_NEVER)
      return zeros;
   if(func == PIPE_FUNC_ALWAYS)
      return ones;

   /* TODO: optimize the constant case */

   /* XXX: It is not clear if we should use the ordered or unordered operators */

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width * type.length == 128) {
      if(type.floating) {
         LLVMValueRef args[3];
         unsigned cc;
         boolean swap;
         LLVMValueRef res;

         swap = FALSE;
         switch(func) {
         case PIPE_FUNC_EQUAL:
            cc = 0;
            break;
         case PIPE_FUNC_NOTEQUAL:
            cc = 4;
            break;
         case PIPE_FUNC_LESS:
            cc = 1;
            break;
         case PIPE_FUNC_LEQUAL:
            cc = 2;
            break;
         case PIPE_FUNC_GREATER:
            cc = 1;
            swap = TRUE;
            break;
         case PIPE_FUNC_GEQUAL:
            cc = 2;
            swap = TRUE;
            break;
         default:
            assert(0);
            return bld->undef;
         }

         if(swap) {
            args[0] = b;
            args[1] = a;
         }
         else {
            args[0] = a;
            args[1] = b;
         }

         args[2] = LLVMConstInt(LLVMInt8Type(), cc, 0);
         res = lp_build_intrinsic(bld->builder,
                                  "llvm.x86.sse.cmp.ps",
                                  vec_type,
                                  args, 3);
         res = LLVMBuildBitCast(bld->builder, res, int_vec_type, "");
         return res;
      }
   }
#endif

   if(type.floating) {
      LLVMRealPredicate op;
      switch(func) {
      case PIPE_FUNC_NEVER:
         op = LLVMRealPredicateFalse;
         break;
      case PIPE_FUNC_ALWAYS:
         op = LLVMRealPredicateTrue;
         break;
      case PIPE_FUNC_EQUAL:
         op = LLVMRealUEQ;
         break;
      case PIPE_FUNC_NOTEQUAL:
         op = LLVMRealUNE;
         break;
      case PIPE_FUNC_LESS:
         op = LLVMRealULT;
         break;
      case PIPE_FUNC_LEQUAL:
         op = LLVMRealULE;
         break;
      case PIPE_FUNC_GREATER:
         op = LLVMRealUGT;
         break;
      case PIPE_FUNC_GEQUAL:
         op = LLVMRealUGE;
         break;
      default:
         assert(0);
         return bld->undef;
      }
      cond = LLVMBuildFCmp(bld->builder, op, a, b, "");
   }
   else {
      LLVMIntPredicate op;
      switch(func) {
      case PIPE_FUNC_EQUAL:
         op = LLVMIntEQ;
         break;
      case PIPE_FUNC_NOTEQUAL:
         op = LLVMIntNE;
         break;
      case PIPE_FUNC_LESS:
         op = type.sign ? LLVMIntSLT : LLVMIntULT;
         break;
      case PIPE_FUNC_LEQUAL:
         op = type.sign ? LLVMIntSLE : LLVMIntULE;
         break;
      case PIPE_FUNC_GREATER:
         op = type.sign ? LLVMIntSGT : LLVMIntUGT;
         break;
      case PIPE_FUNC_GEQUAL:
         op = type.sign ? LLVMIntSGE : LLVMIntUGE;
         break;
      default:
         assert(0);
         return bld->undef;
      }
      cond = LLVMBuildICmp(bld->builder, op, a, b, "");
   }

   return LLVMBuildSelect(bld->builder, cond, ones, zeros, "");
}
