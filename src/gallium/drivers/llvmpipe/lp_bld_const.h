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
 * Helper functions for constant building.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_CONST_H
#define LP_BLD_CONST_H


#include <llvm-c/Core.h>  

#include <pipe/p_compiler.h>


union lp_type type;


unsigned
lp_mantissa(union lp_type type);


unsigned
lp_const_shift(union lp_type type);


unsigned
lp_const_offset(union lp_type type);


double
lp_const_scale(union lp_type type);

double
lp_const_min(union lp_type type);


double
lp_const_max(union lp_type type);


double
lp_const_eps(union lp_type type);


LLVMValueRef
lp_build_undef(union lp_type type);


LLVMValueRef
lp_build_zero(union lp_type type);


LLVMValueRef
lp_build_one(union lp_type type);


LLVMValueRef
lp_build_const_scalar(union lp_type type,
                      double val);


LLVMValueRef
lp_build_int_const_scalar(union lp_type type,
                          long long val);


LLVMValueRef
lp_build_const_aos(union lp_type type, 
                   double r, double g, double b, double a, 
                   const unsigned char *swizzle);


LLVMValueRef
lp_build_const_mask_aos(union lp_type type,
                        boolean cond[4]);


#endif /* !LP_BLD_CONST_H */
