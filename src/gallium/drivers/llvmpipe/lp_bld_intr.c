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


/**
 * @file
 * Helpers for emiting intrinsic calls.
 *
 * LLVM vanilla IR doesn't represent all basic arithmetic operations we care
 * about, and it is often necessary to resort target-specific intrinsics for
 * performance, convenience.
 *
 * Ideally we would like to stay away from target specific intrinsics and
 * move all the instruction selection logic into upstream LLVM where it belongs.
 *
 * These functions are also used for calling C functions provided by us from
 * generated LLVM code.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"

#include "lp_bld_intr.h"


LLVMValueRef
lp_declare_intrinsic(LLVMModuleRef module,
                     const char *name,
                     LLVMTypeRef ret_type,
                     LLVMTypeRef *arg_types,
                     unsigned num_args)
{
   LLVMTypeRef function_type;
   LLVMValueRef function;

   assert(!LLVMGetNamedFunction(module, name));

   function_type = LLVMFunctionType(ret_type, arg_types, num_args, 0);
   function = LLVMAddFunction(module, name, function_type);

   LLVMSetFunctionCallConv(function, LLVMCCallConv);
   LLVMSetLinkage(function, LLVMExternalLinkage);

   assert(LLVMIsDeclaration(function));

   if(name[0] == 'l' &&
      name[1] == 'l' &&
      name[2] == 'v' &&
      name[3] == 'm' &&
      name[4] == '.')
      assert(LLVMGetIntrinsicID(function));

   return function;
}


LLVMValueRef
lp_build_intrinsic(LLVMBuilderRef builder,
                   const char *name,
                   LLVMTypeRef ret_type,
                   LLVMValueRef *args,
                   unsigned num_args)
{
   LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
   LLVMValueRef function;

   function = LLVMGetNamedFunction(module, name);
   if(!function) {
      LLVMTypeRef arg_types[LP_MAX_FUNC_ARGS];
      unsigned i;

      assert(num_args <= LP_MAX_FUNC_ARGS);

      for(i = 0; i < num_args; ++i) {
         assert(args[i]);
         arg_types[i] = LLVMTypeOf(args[i]);
      }

      function = lp_declare_intrinsic(module, name, ret_type, arg_types, num_args);
   }

   return LLVMBuildCall(builder, function, args, num_args, "");
}


LLVMValueRef
lp_build_intrinsic_unary(LLVMBuilderRef builder,
                         const char *name,
                         LLVMTypeRef ret_type,
                         LLVMValueRef a)
{
   return lp_build_intrinsic(builder, name, ret_type, &a, 1);
}


LLVMValueRef
lp_build_intrinsic_binary(LLVMBuilderRef builder,
                          const char *name,
                          LLVMTypeRef ret_type,
                          LLVMValueRef a,
                          LLVMValueRef b)
{
   LLVMValueRef args[2];

   args[0] = a;
   args[1] = b;

   return lp_build_intrinsic(builder, name, ret_type, args, 2);
}


LLVMValueRef
lp_build_intrinsic_map(LLVMBuilderRef builder,
                       const char *name,
                       LLVMTypeRef ret_type,
                       LLVMValueRef *args,
                       unsigned num_args)
{
   LLVMTypeRef ret_elem_type = LLVMGetElementType(ret_type);
   unsigned n = LLVMGetVectorSize(ret_type);
   unsigned i, j;
   LLVMValueRef res;

   assert(num_args <= LP_MAX_FUNC_ARGS);

   res = LLVMGetUndef(ret_type);
   for(i = 0; i < n; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef arg_elems[LP_MAX_FUNC_ARGS];
      LLVMValueRef res_elem;
      for(j = 0; j < num_args; ++j)
         arg_elems[j] = LLVMBuildExtractElement(builder, args[j], index, "");
      res_elem = lp_build_intrinsic(builder, name, ret_elem_type, arg_elems, num_args);
      res = LLVMBuildInsertElement(builder, res, res_elem, index, "");
   }

   return res;
}


LLVMValueRef
lp_build_intrinsic_map_unary(LLVMBuilderRef builder,
                             const char *name,
                             LLVMTypeRef ret_type,
                             LLVMValueRef a)
{
   return lp_build_intrinsic_map(builder, name, ret_type, &a, 1);
}


LLVMValueRef
lp_build_intrinsic_map_binary(LLVMBuilderRef builder,
                              const char *name,
                              LLVMTypeRef ret_type,
                              LLVMValueRef a,
                              LLVMValueRef b)
{
   LLVMValueRef args[2];

   args[0] = a;
   args[1] = b;

   return lp_build_intrinsic_map(builder, name, ret_type, args, 2);
}


