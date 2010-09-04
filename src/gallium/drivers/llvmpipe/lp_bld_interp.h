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
 * Position and shader input interpolation.
 *
 * Special attention is given to the interpolation of side by side quads.
 * Multiplications are made only for the first quad. Interpolation of
 * inputs for posterior quads are done exclusively with additions, and
 * perspective divide if necessary.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_BLD_INTERP_H
#define LP_BLD_INTERP_H


#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_type.h"

#include "tgsi/tgsi_exec.h"

#include "lp_setup.h"


struct lp_build_interp_soa_context
{
   /* QUAD_SIZE x float */
   struct lp_build_context coeff_bld;

   unsigned num_attribs;
   unsigned mask[1 + PIPE_MAX_SHADER_INPUTS]; /**< TGSI_WRITE_MASK_x */
   enum lp_interp interp[1 + PIPE_MAX_SHADER_INPUTS];

   LLVMValueRef x;
   LLVMValueRef y;

   LLVMValueRef a   [1 + PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
   LLVMValueRef dadq[1 + PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];

   LLVMValueRef oow;

   LLVMValueRef attribs[1 + PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];

   /*
    * Convenience pointers. Callers may access this one.
    */
   const LLVMValueRef *pos;
   const LLVMValueRef (*inputs)[NUM_CHANNELS];
};


void
lp_build_interp_soa_init(struct lp_build_interp_soa_context *bld,
                         unsigned num_inputs,
                         const struct lp_shader_input *inputs,
                         LLVMBuilderRef builder,
                         struct lp_type type,
                         LLVMValueRef a0_ptr,
                         LLVMValueRef dadx_ptr,
                         LLVMValueRef dady_ptr,
                         LLVMValueRef x,
                         LLVMValueRef y);

void
lp_build_interp_soa_update(struct lp_build_interp_soa_context *bld,
                           int quad_index);


#endif /* LP_BLD_INTERP_H */
