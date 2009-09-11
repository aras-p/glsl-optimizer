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
 * C - JIT interfaces
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_JIT_H
#define LP_JIT_H


#include "lp_bld_struct.h"

#include "pipe/p_state.h"


struct tgsi_sampler;
struct llvmpipe_screen;


struct lp_jit_texture
{
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   const void *data;
};


enum {
   LP_JIT_TEXTURE_WIDTH = 0,
   LP_JIT_TEXTURE_HEIGHT,
   LP_JIT_TEXTURE_STRIDE,
   LP_JIT_TEXTURE_DATA
};



/**
 * This structure is passed directly to the generated fragment shader.
 *
 * It contains the derived state.
 *
 * Changes here must be reflected in the lp_jit_context_* macros and
 * lp_jit_init_types function. Changes to the ordering should be avoided.
 *
 * Only use types with a clear size and padding here, in particular prefer the
 * stdint.h types to the basic integer types.
 */
struct lp_jit_context
{
   const float *constants;

   struct tgsi_sampler **samplers;

   float alpha_ref_value;

   /* FIXME: store (also?) in floats */
   uint8_t *blend_color;

   struct lp_jit_texture textures[PIPE_MAX_SAMPLERS];
};


#define lp_jit_context_constants(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 0, "constants")

#define lp_jit_context_samplers(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 1, "samplers")

#define lp_jit_context_alpha_ref_value(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 2, "alpha_ref_value")

#define lp_jit_context_blend_color(_builder, _ptr) \
   lp_build_struct_get(_builder, _ptr, 3, "blend_color")

#define LP_JIT_CONTEXT_TEXTURES_INDEX 4

#define lp_jit_context_textures(_builder, _ptr) \
   lp_build_struct_get_ptr(_builder, _ptr, LP_JIT_CONTEXT_TEXTURES_INDEX, "textures")


typedef void
(*lp_jit_frag_func)(struct lp_jit_context *context,
                    uint32_t x,
                    uint32_t y,
                    const void *a0,
                    const void *dadx,
                    const void *dady,
                    uint32_t *mask,
                    void *color,
                    void *depth);

void PIPE_CDECL
lp_fetch_texel_soa( struct tgsi_sampler **samplers,
                    uint32_t unit,
                    float *store );


void
lp_jit_screen_cleanup(struct llvmpipe_screen *screen);


void
lp_jit_screen_init(struct llvmpipe_screen *screen);


#endif /* LP_JIT_H */
