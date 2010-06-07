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
 * Texture sampling.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_BLD_SAMPLE_H
#define LP_BLD_SAMPLE_H


#include "pipe/p_format.h"

#include "gallivm/lp_bld.h"

struct pipe_resource;
struct pipe_sampler_view;
struct pipe_sampler_state;
struct util_format_description;
struct lp_type;
struct lp_build_context;


/**
 * Sampler static state.
 *
 * These are the bits of state from pipe_resource and pipe_sampler_state that
 * are embedded in the generated code.
 */
struct lp_sampler_static_state
{
   /* pipe_sampler_view's state */
   enum pipe_format format;
   unsigned swizzle_r:3;     /**< PIPE_SWIZZLE_* */
   unsigned swizzle_g:3;
   unsigned swizzle_b:3;
   unsigned swizzle_a:3;

   /* pipe_texture's state */
   unsigned target:3;        /**< PIPE_TEXTURE_* */
   unsigned pot_width:1;     /**< is the width a power of two? */
   unsigned pot_height:1;
   unsigned pot_depth:1;

   /* pipe_sampler_state's state */
   unsigned wrap_s:3;
   unsigned wrap_t:3;
   unsigned wrap_r:3;
   unsigned min_img_filter:2;
   unsigned min_mip_filter:2;
   unsigned mag_img_filter:2;
   unsigned compare_mode:1;
   unsigned compare_func:3;
   unsigned normalized_coords:1;
   float lod_bias, min_lod, max_lod;
   float border_color[4];
};


/**
 * Sampler dynamic state.
 *
 * These are the bits of state from pipe_resource and pipe_sampler_state that
 * are computed in runtime.
 *
 * There are obtained through callbacks, as we don't want to tie the texture
 * sampling code generation logic to any particular texture layout or pipe
 * driver.
 */
struct lp_sampler_dynamic_state
{

   /** Obtain the base texture width. */
   LLVMValueRef
   (*width)( const struct lp_sampler_dynamic_state *state,
             LLVMBuilderRef builder,
             unsigned unit);

   /** Obtain the base texture height. */
   LLVMValueRef
   (*height)( const struct lp_sampler_dynamic_state *state,
              LLVMBuilderRef builder,
              unsigned unit);

   /** Obtain the base texture depth. */
   LLVMValueRef
   (*depth)( const struct lp_sampler_dynamic_state *state,
             LLVMBuilderRef builder,
             unsigned unit);

   /** Obtain the number of mipmap levels (minus one). */
   LLVMValueRef
   (*last_level)( const struct lp_sampler_dynamic_state *state,
                  LLVMBuilderRef builder,
                  unsigned unit);

   LLVMValueRef
   (*row_stride)( const struct lp_sampler_dynamic_state *state,
                  LLVMBuilderRef builder,
                  unsigned unit);

   LLVMValueRef
   (*img_stride)( const struct lp_sampler_dynamic_state *state,
                  LLVMBuilderRef builder,
                  unsigned unit);

   LLVMValueRef
   (*data_ptr)( const struct lp_sampler_dynamic_state *state,
                LLVMBuilderRef builder,
                unsigned unit);

};


/**
 * Derive the sampler static state.
 */
void
lp_sampler_static_state(struct lp_sampler_static_state *state,
                        const struct pipe_sampler_view *view,
                        const struct pipe_sampler_state *sampler);


void
lp_build_sample_partial_offset(struct lp_build_context *bld,
                               unsigned block_length,
                               LLVMValueRef coord,
                               LLVMValueRef stride,
                               LLVMValueRef *out_offset,
                               LLVMValueRef *out_i);


void
lp_build_sample_offset(struct lp_build_context *bld,
                       const struct util_format_description *format_desc,
                       LLVMValueRef x,
                       LLVMValueRef y,
                       LLVMValueRef z,
                       LLVMValueRef y_stride,
                       LLVMValueRef z_stride,
                       LLVMValueRef *out_offset,
                       LLVMValueRef *out_i,
                       LLVMValueRef *out_j);


void
lp_build_sample_soa(LLVMBuilderRef builder,
                    const struct lp_sampler_static_state *static_state,
                    struct lp_sampler_dynamic_state *dynamic_state,
                    struct lp_type fp_type,
                    unsigned unit,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    const LLVMValueRef *ddx,
                    const LLVMValueRef *ddy,
                    LLVMValueRef lod_bias,
                    LLVMValueRef explicit_lod,
                    LLVMValueRef texel_out[4]);



#endif /* LP_BLD_SAMPLE_H */
