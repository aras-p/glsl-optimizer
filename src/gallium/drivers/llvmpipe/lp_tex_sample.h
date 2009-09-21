/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef LP_TEX_SAMPLE_H
#define LP_TEX_SAMPLE_H


#include <llvm-c/Core.h>

#include "tgsi/tgsi_exec.h"


struct llvmpipe_tex_tile_cache;
struct lp_sampler_static_state;


/**
 * Subclass of tgsi_sampler
 */
struct lp_shader_sampler
{
   struct tgsi_sampler base;  /**< base class */

   unsigned processor;

   /* For lp_get_samples_2d_linear_POT:
    */
   unsigned xpot;
   unsigned ypot;
   unsigned level;

   const struct pipe_texture *texture;
   const struct pipe_sampler_state *sampler;

   struct llvmpipe_tex_tile_cache *cache;
};



static INLINE struct lp_shader_sampler *
lp_shader_sampler(const struct tgsi_sampler *sampler)
{
   return (struct lp_shader_sampler *) sampler;
}



extern void
lp_get_samples(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias,
               float rgba[NUM_CHANNELS][QUAD_SIZE]);


/**
 * Texture sampling code generator that just calls lp_get_samples C function
 * for the actual sampling computation.
 *
 * @param context_ptr LLVM value with the pointer to the struct lp_jit_context.
 */
struct lp_build_sampler_soa *
lp_c_sampler_soa_create(LLVMValueRef context_ptr);


/**
 * Pure-LLVM texture sampling code generator.
 *
 * @param context_ptr LLVM value with the pointer to the struct lp_jit_context.
 */
struct lp_build_sampler_soa *
lp_llvm_sampler_soa_create(const struct lp_sampler_static_state *key,
                           LLVMValueRef context_ptr);


#endif /* LP_TEX_SAMPLE_H */
