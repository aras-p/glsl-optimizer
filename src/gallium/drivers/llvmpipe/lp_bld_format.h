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

#ifndef LP_BLD_H
#define LP_BLD_H


/**
 * @file
 * LLVM IR building helpers interfaces.
 *
 * We use LLVM-C bindings for now. They are not documented, but follow the C++
 * interfaces very closely, and appear to be complete enough for code
 * genration. See
 * http://npcontemplation.blogspot.com/2008/06/secret-of-llvm-c-bindings.html
 * for a standalone example.
 */

#include <llvm-c/Core.h>  
 
#include "pipe/p_format.h"


union lp_type;


/**
 * Unpack a pixel into its RGBA components.
 *
 * @param packed integer.
 *
 * @return RGBA in a 4 floats vector.
 */
LLVMValueRef
lp_build_unpack_rgba(LLVMBuilderRef builder,
                     enum pipe_format format, 
                     LLVMValueRef packed);


/**
 * Pack a pixel.
 *
 * @param rgba 4 float vector with the unpacked components.
 */
LLVMValueRef
lp_build_pack_rgba(LLVMBuilderRef builder,
                   enum pipe_format format,
                   LLVMValueRef rgba);


/**
 * Load a pixel into its RGBA components.
 *
 * @param ptr value with the pointer to the packed pixel. Pointer type is
 * irrelevant.
 *
 * @return RGBA in a 4 floats vector.
 */
LLVMValueRef
lp_build_load_rgba(LLVMBuilderRef builder,
                   enum pipe_format format, 
                   LLVMValueRef ptr);


/**
 * Store a pixel.
 *
 * @param rgba 4 float vector with the unpacked components.
 */
void 
lp_build_store_rgba(LLVMBuilderRef builder,
                    enum pipe_format format,
                    LLVMValueRef ptr,
                    LLVMValueRef rgba);


#endif /* !LP_BLD_H */
