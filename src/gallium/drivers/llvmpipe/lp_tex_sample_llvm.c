/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All rights reserved.
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
 * Texture sampling code generation
 *
 * This file is nothing more than ugly glue between three largely independent
 * entities:
 * - TGSI -> LLVM translation (i.e., lp_build_tgsi_soa)
 * - texture sampling code generation (i.e., lp_build_sample_soa)
 * - LLVM pipe driver
 *
 * All interesting code is in the functions mentioned above. There is really
 * nothing to see here.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_sample.h"
#include "gallivm/lp_bld_tgsi.h"
#include "lp_jit.h"
#include "lp_tex_sample.h"


/**
 * This provides the bridge between the sampler state store in lp_jit_context
 * and lp_jit_texture and the sampler code generator. It provides the
 * texture layout information required by the texture sampler code generator
 * in terms of the state stored in lp_jit_context and lp_jit_texture in runtime.
 */
struct llvmpipe_sampler_dynamic_state
{
   struct lp_sampler_dynamic_state base;

   const struct lp_sampler_static_state *static_state;

   LLVMValueRef context_ptr;
};


/**
 * This is the bridge between our sampler and the TGSI translator.
 */
struct lp_llvm_sampler_soa
{
   struct lp_build_sampler_soa base;

   struct llvmpipe_sampler_dynamic_state dynamic_state;
};


/**
 * Fetch the specified member of the lp_jit_texture structure.
 *
 * @sa http://llvm.org/docs/GetElementPtr.html
 */
static LLVMValueRef
lp_llvm_texture_member(struct lp_sampler_dynamic_state *base,
                       LLVMBuilderRef builder,
                       unsigned unit,
                       unsigned member_index,
                       const char *member_name)
{
   struct llvmpipe_sampler_dynamic_state *state = (struct llvmpipe_sampler_dynamic_state *)base;
   LLVMValueRef indices[4];
   LLVMValueRef ptr;
   LLVMValueRef res;

   assert(unit < PIPE_MAX_SAMPLERS);

   /* context[0] */
   indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   /* context[0].textures */
   indices[1] = LLVMConstInt(LLVMInt32Type(), LP_JIT_CONTEXT_TEXTURES_INDEX, 0);
   /* context[0].textures[unit] */
   indices[2] = LLVMConstInt(LLVMInt32Type(), unit, 0);
   /* context[0].textures[unit].member */
   indices[3] = LLVMConstInt(LLVMInt32Type(), member_index, 0);

   ptr = LLVMBuildGEP(builder, state->context_ptr, indices, Elements(indices), "");

   res = LLVMBuildLoad(builder, ptr, "");

   lp_build_name(res, "context.texture%u.%s", unit, member_name);

   return res;
}


/**
 * Helper macro to instantiate the functions that generate the code to fetch
 * the members of lp_jit_texture to fulfill the sampler code generator requests.
 *
 * This complexity is the price we have to pay to keep the texture sampler code
 * generator a reusable module without dependencies to llvmpipe internals.
 */
#define LP_LLVM_TEXTURE_MEMBER(_name, _index) \
   static LLVMValueRef \
   lp_llvm_texture_##_name( struct lp_sampler_dynamic_state *base, \
                            LLVMBuilderRef builder, \
                            unsigned unit) \
   { \
      return lp_llvm_texture_member(base, builder, unit, _index, #_name ); \
   }


LP_LLVM_TEXTURE_MEMBER(width,      LP_JIT_TEXTURE_WIDTH)
LP_LLVM_TEXTURE_MEMBER(height,     LP_JIT_TEXTURE_HEIGHT)
LP_LLVM_TEXTURE_MEMBER(depth,      LP_JIT_TEXTURE_DEPTH)
LP_LLVM_TEXTURE_MEMBER(last_level, LP_JIT_TEXTURE_LAST_LEVEL)
LP_LLVM_TEXTURE_MEMBER(stride,     LP_JIT_TEXTURE_STRIDE)
LP_LLVM_TEXTURE_MEMBER(data_ptr,   LP_JIT_TEXTURE_DATA)


static void
lp_llvm_sampler_soa_destroy(struct lp_build_sampler_soa *sampler)
{
   FREE(sampler);
}


/**
 * Fetch filtered values from texture.
 * The 'texel' parameter returns four vectors corresponding to R, G, B, A.
 */
static void
lp_llvm_sampler_soa_emit_fetch_texel(struct lp_build_sampler_soa *base,
                                     LLVMBuilderRef builder,
                                     struct lp_type type,
                                     unsigned unit,
                                     unsigned num_coords,
                                     const LLVMValueRef *coords,
                                     LLVMValueRef lodbias,
                                     LLVMValueRef *texel)
{
   struct lp_llvm_sampler_soa *sampler = (struct lp_llvm_sampler_soa *)base;

   assert(unit < PIPE_MAX_SAMPLERS);

   lp_build_sample_soa(builder,
                       &sampler->dynamic_state.static_state[unit],
                       &sampler->dynamic_state.base,
                       type,
                       unit,
                       num_coords,
                       coords,
                       lodbias,
                       texel);
}


struct lp_build_sampler_soa *
lp_llvm_sampler_soa_create(const struct lp_sampler_static_state *static_state,
                           LLVMValueRef context_ptr)
{
   struct lp_llvm_sampler_soa *sampler;

   sampler = CALLOC_STRUCT(lp_llvm_sampler_soa);
   if(!sampler)
      return NULL;

   sampler->base.destroy = lp_llvm_sampler_soa_destroy;
   sampler->base.emit_fetch_texel = lp_llvm_sampler_soa_emit_fetch_texel;
   sampler->dynamic_state.base.width = lp_llvm_texture_width;
   sampler->dynamic_state.base.height = lp_llvm_texture_height;
   sampler->dynamic_state.base.depth = lp_llvm_texture_depth;
   sampler->dynamic_state.base.last_level = lp_llvm_texture_last_level;
   sampler->dynamic_state.base.stride = lp_llvm_texture_stride;
   sampler->dynamic_state.base.data_ptr = lp_llvm_texture_data_ptr;
   sampler->dynamic_state.static_state = static_state;
   sampler->dynamic_state.context_ptr = context_ptr;

   return &sampler->base;
}

