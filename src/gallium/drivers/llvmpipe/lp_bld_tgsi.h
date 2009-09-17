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
 * TGSI to LLVM IR translation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_BLD_TGSI_H
#define LP_BLD_TGSI_H

#include <llvm-c/Core.h>


struct tgsi_token;
struct lp_type;
struct lp_build_context;
struct lp_build_mask_context;


/**
 * Sampler code generation interface.
 *
 * Although texture sampling is a requirement for TGSI translation, it is
 * a very different problem with several different approaches to it. This
 * structure establishes an interface for texture sampling code generation, so
 * that we can easily use different texture sampling strategies.
 */
struct lp_build_sampler_soa
{
   void
   (*destroy)( struct lp_build_sampler_soa *sampler );

   void
   (*emit_fetch_texel)( struct lp_build_sampler_soa *sampler,
                        LLVMBuilderRef builder,
                        struct lp_type type,
                        unsigned unit,
                        unsigned num_coords,
                        const LLVMValueRef *coords,
                        LLVMValueRef lodbias,
                        LLVMValueRef *texel);
};


void
lp_build_tgsi_soa(LLVMBuilderRef builder,
                  const struct tgsi_token *tokens,
                  struct lp_type type,
                  struct lp_build_mask_context *mask,
                  LLVMValueRef consts_ptr,
                  const LLVMValueRef *pos,
                  const LLVMValueRef (*inputs)[4],
                  LLVMValueRef (*outputs)[4],
                  struct lp_build_sampler_soa *sampler);


#endif /* LP_BLD_TGSI_H */
