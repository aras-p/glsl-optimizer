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


#include "util/u_format.h"

#include "lp_bld.h"


LLVMValueRef
lp_build_load_rgba(LLVMBuilderRef builder,
                   enum pipe_format format,
                   LLVMValueRef ptr)
{
   const struct util_format_description *desc;
   LLVMTypeRef type;
   LLVMValueRef packed;

   desc = util_format_description(format);

   /* FIXME: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_ARITH);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
   assert(desc->block.bits <= 32);

   type = LLVMIntType(desc->block.bits);

   ptr = LLVMBuildBitCast(builder, ptr, LLVMPointerType(type, 0), "");

   packed = LLVMBuildLoad(builder, ptr, "");

   return lp_build_unpack_rgba(builder, format, packed);
}

