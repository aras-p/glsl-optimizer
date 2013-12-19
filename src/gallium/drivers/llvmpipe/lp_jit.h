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


#include "gallivm/lp_bld_struct.h"
#include "gallivm/lp_bld_limits.h"

#include "pipe/p_state.h"
#include "lp_texture.h"


struct lp_fragment_shader_variant;
struct llvmpipe_screen;


struct lp_jit_texture
{
   uint32_t width;        /* same as number of elements */
   uint32_t height;
   uint32_t depth;        /* doubles as array size */
   uint32_t first_level;
   uint32_t last_level;
   const void *base;
   uint32_t row_stride[LP_MAX_TEXTURE_LEVELS];
   uint32_t img_stride[LP_MAX_TEXTURE_LEVELS];
   uint32_t mip_offsets[LP_MAX_TEXTURE_LEVELS];
};


struct lp_jit_sampler
{
   float min_lod;
   float max_lod;
   float lod_bias;
   float border_color[4];
};


struct lp_jit_viewport
{
   float min_depth;
   float max_depth;
};


enum {
   LP_JIT_TEXTURE_WIDTH = 0,
   LP_JIT_TEXTURE_HEIGHT,
   LP_JIT_TEXTURE_DEPTH,
   LP_JIT_TEXTURE_FIRST_LEVEL,
   LP_JIT_TEXTURE_LAST_LEVEL,
   LP_JIT_TEXTURE_BASE,
   LP_JIT_TEXTURE_ROW_STRIDE,
   LP_JIT_TEXTURE_IMG_STRIDE,
   LP_JIT_TEXTURE_MIP_OFFSETS,
   LP_JIT_TEXTURE_NUM_FIELDS  /* number of fields above */
};


enum {
   LP_JIT_SAMPLER_MIN_LOD,
   LP_JIT_SAMPLER_MAX_LOD,
   LP_JIT_SAMPLER_LOD_BIAS,
   LP_JIT_SAMPLER_BORDER_COLOR,
   LP_JIT_SAMPLER_NUM_FIELDS  /* number of fields above */
};


enum {
   LP_JIT_VIEWPORT_MIN_DEPTH,
   LP_JIT_VIEWPORT_MAX_DEPTH,
   LP_JIT_VIEWPORT_NUM_FIELDS /* number of fields above */
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
   const float *constants[LP_MAX_TGSI_CONST_BUFFERS];
   int num_constants[LP_MAX_TGSI_CONST_BUFFERS];

   float alpha_ref_value;

   uint32_t stencil_ref_front, stencil_ref_back;

   uint8_t *u8_blend_color;
   float *f_blend_color;

   struct lp_jit_viewport *viewports;

   struct lp_jit_texture textures[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   struct lp_jit_sampler samplers[PIPE_MAX_SAMPLERS];
};


/**
 * These enum values must match the position of the fields in the
 * lp_jit_context struct above.
 */
enum {
   LP_JIT_CTX_CONSTANTS = 0,
   LP_JIT_CTX_NUM_CONSTANTS,
   LP_JIT_CTX_ALPHA_REF,
   LP_JIT_CTX_STENCIL_REF_FRONT,
   LP_JIT_CTX_STENCIL_REF_BACK,
   LP_JIT_CTX_U8_BLEND_COLOR,
   LP_JIT_CTX_F_BLEND_COLOR,
   LP_JIT_CTX_VIEWPORTS,
   LP_JIT_CTX_TEXTURES,
   LP_JIT_CTX_SAMPLERS,
   LP_JIT_CTX_COUNT
};


#define lp_jit_context_constants(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, LP_JIT_CTX_CONSTANTS, "constants")

#define lp_jit_context_num_constants(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, LP_JIT_CTX_NUM_CONSTANTS, "num_constants")

#define lp_jit_context_alpha_ref_value(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_ALPHA_REF, "alpha_ref_value")

#define lp_jit_context_stencil_ref_front_value(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_STENCIL_REF_FRONT, "stencil_ref_front")

#define lp_jit_context_stencil_ref_back_value(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_STENCIL_REF_BACK, "stencil_ref_back")

#define lp_jit_context_u8_blend_color(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_U8_BLEND_COLOR, "u8_blend_color")

#define lp_jit_context_f_blend_color(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_F_BLEND_COLOR, "f_blend_color")

#define lp_jit_context_viewports(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_VIEWPORTS, "viewports")

#define lp_jit_context_textures(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, LP_JIT_CTX_TEXTURES, "textures")

#define lp_jit_context_samplers(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, LP_JIT_CTX_SAMPLERS, "samplers")


struct lp_jit_thread_data
{
   uint64_t vis_counter;

   /*
    * Non-interpolated rasterizer state passed through to the fragment shader.
    */
   struct {
      uint32_t viewport_index;
   } raster_state;
};


enum {
   LP_JIT_THREAD_DATA_COUNTER = 0,
   LP_JIT_THREAD_DATA_RASTER_STATE_VIEWPORT_INDEX,
   LP_JIT_THREAD_DATA_COUNT
};


#define lp_jit_thread_data_counter(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, LP_JIT_THREAD_DATA_COUNTER, "counter")

#define lp_jit_thread_data_raster_state_viewport_index(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, \
                       LP_JIT_THREAD_DATA_RASTER_STATE_VIEWPORT_INDEX, \
                       "raster_state.viewport_index")
 
/**
 * typedef for fragment shader function
 *
 * @param context       jit context
 * @param x             block start x
 * @param y             block start y
 * @param facing        is front facing
 * @param a0            shader input a0
 * @param dadx          shader input dadx
 * @param dady          shader input dady
 * @param color         color buffer
 * @param depth         depth buffer
 * @param mask          mask of visible pixels in block
 * @param thread_data   task thread data
 * @param stride        color buffer row stride in bytes
 * @param depth_stride  depth buffer row stride in bytes
 */
typedef void
(*lp_jit_frag_func)(const struct lp_jit_context *context,
                    uint32_t x,
                    uint32_t y,
                    uint32_t facing,
                    const void *a0,
                    const void *dadx,
                    const void *dady,
                    uint8_t **color,
                    uint8_t *depth,
                    uint32_t mask,
                    struct lp_jit_thread_data *thread_data,
                    unsigned *stride,
                    unsigned depth_stride);


void
lp_jit_screen_cleanup(struct llvmpipe_screen *screen);


void
lp_jit_screen_init(struct llvmpipe_screen *screen);


void
lp_jit_init_types(struct lp_fragment_shader_variant *lp);


#endif /* LP_JIT_H */
