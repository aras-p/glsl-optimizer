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
#include "util/u_debug.h"
#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_swizzle.h"


struct pipe_resource;
struct pipe_sampler_view;
struct pipe_sampler_state;
struct util_format_description;
struct lp_type;
struct lp_build_context;


/**
 * Helper struct holding all derivatives needed for sampling
 */
struct lp_derivatives
{
   LLVMValueRef ddx[3];
   LLVMValueRef ddy[3];
};


enum lp_sampler_lod_property {
   LP_SAMPLER_LOD_SCALAR,
   LP_SAMPLER_LOD_PER_ELEMENT,
   LP_SAMPLER_LOD_PER_QUAD
};


/**
 * Texture static state.
 *
 * These are the bits of state from pipe_resource/pipe_sampler_view that
 * are embedded in the generated code.
 */
struct lp_static_texture_state
{
   /* pipe_sampler_view's state */
   enum pipe_format format;
   unsigned swizzle_r:3;     /**< PIPE_SWIZZLE_* */
   unsigned swizzle_g:3;
   unsigned swizzle_b:3;
   unsigned swizzle_a:3;

   /* pipe_texture's state */
   unsigned target:4;        /**< PIPE_TEXTURE_* */
   unsigned pot_width:1;     /**< is the width a power of two? */
   unsigned pot_height:1;
   unsigned pot_depth:1;
   unsigned level_zero_only:1;
};


/**
 * Sampler static state.
 *
 * These are the bits of state from pipe_sampler_state that
 * are embedded in the generated code.
 */
struct lp_static_sampler_state
{
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
   unsigned min_max_lod_equal:1;  /**< min_lod == max_lod ? */
   unsigned lod_bias_non_zero:1;
   unsigned apply_min_lod:1;  /**< min_lod > 0 ? */
   unsigned apply_max_lod:1;  /**< max_lod < last_level ? */
   unsigned seamless_cube_map:1;

   /* Hacks */
   unsigned force_nearest_s:1;
   unsigned force_nearest_t:1;
};


/**
 * Sampler dynamic state.
 *
 * These are the bits of state from pipe_resource/pipe_sampler_view
 * as well as from sampler state that are computed at runtime.
 *
 * There are obtained through callbacks, as we don't want to tie the texture
 * sampling code generation logic to any particular texture layout or pipe
 * driver.
 */
struct lp_sampler_dynamic_state
{
   /* First callbacks for sampler view state */

   /** Obtain the base texture width (or number of elements) (returns int32) */
   LLVMValueRef
   (*width)( const struct lp_sampler_dynamic_state *state,
             struct gallivm_state *gallivm,
             unsigned texture_unit);

   /** Obtain the base texture height (returns int32) */
   LLVMValueRef
   (*height)( const struct lp_sampler_dynamic_state *state,
              struct gallivm_state *gallivm,
              unsigned texture_unit);

   /** Obtain the base texture depth (or array size) (returns int32) */
   LLVMValueRef
   (*depth)( const struct lp_sampler_dynamic_state *state,
             struct gallivm_state *gallivm,
             unsigned texture_unit);

   /** Obtain the first mipmap level (base level) (returns int32) */
   LLVMValueRef
   (*first_level)( const struct lp_sampler_dynamic_state *state,
                   struct gallivm_state *gallivm,
                   unsigned texture_unit);

   /** Obtain the number of mipmap levels minus one (returns int32) */
   LLVMValueRef
   (*last_level)( const struct lp_sampler_dynamic_state *state,
                  struct gallivm_state *gallivm,
                  unsigned texture_unit);

   /** Obtain stride in bytes between image rows/blocks (returns int32) */
   LLVMValueRef
   (*row_stride)( const struct lp_sampler_dynamic_state *state,
                  struct gallivm_state *gallivm,
                  unsigned texture_unit);

   /** Obtain stride in bytes between image slices (returns int32) */
   LLVMValueRef
   (*img_stride)( const struct lp_sampler_dynamic_state *state,
                  struct gallivm_state *gallivm,
                  unsigned texture_unit);

   /** Obtain pointer to base of texture */
   LLVMValueRef
   (*base_ptr)( const struct lp_sampler_dynamic_state *state,
                struct gallivm_state *gallivm,
                unsigned texture_unit);

   /** Obtain pointer to array of mipmap offsets */
   LLVMValueRef
   (*mip_offsets)( const struct lp_sampler_dynamic_state *state,
                   struct gallivm_state *gallivm,
                   unsigned texture_unit);

   /* These are callbacks for sampler state */

   /** Obtain texture min lod (returns float) */
   LLVMValueRef
   (*min_lod)(const struct lp_sampler_dynamic_state *state,
              struct gallivm_state *gallivm, unsigned sampler_unit);

   /** Obtain texture max lod (returns float) */
   LLVMValueRef
   (*max_lod)(const struct lp_sampler_dynamic_state *state,
              struct gallivm_state *gallivm, unsigned sampler_unit);

   /** Obtain texture lod bias (returns float) */
   LLVMValueRef
   (*lod_bias)(const struct lp_sampler_dynamic_state *state,
               struct gallivm_state *gallivm, unsigned sampler_unit);

   /** Obtain texture border color (returns ptr to float[4]) */
   LLVMValueRef
   (*border_color)(const struct lp_sampler_dynamic_state *state,
                   struct gallivm_state *gallivm, unsigned sampler_unit);
};


/**
 * Keep all information for sampling code generation in a single place.
 */
struct lp_build_sample_context
{
   struct gallivm_state *gallivm;

   const struct lp_static_texture_state *static_texture_state;
   const struct lp_static_sampler_state *static_sampler_state;

   struct lp_sampler_dynamic_state *dynamic_state;

   const struct util_format_description *format_desc;

   /* See texture_dims() */
   unsigned dims;

   /** SIMD vector width */
   unsigned vector_width;

   /** number of mipmaps (valid are 1, length/4, length) */
   unsigned num_mips;

   /** number of lod values (valid are 1, length/4, length) */
   unsigned num_lods;

   /** regular scalar float type */
   struct lp_type float_type;
   struct lp_build_context float_bld;

   /** float vector type */
   struct lp_build_context float_vec_bld;

   /** regular scalar int type */
   struct lp_type int_type;
   struct lp_build_context int_bld;

   /** Incoming coordinates type and build context */
   struct lp_type coord_type;
   struct lp_build_context coord_bld;

   /** Signed integer coordinates */
   struct lp_type int_coord_type;
   struct lp_build_context int_coord_bld;

   /** Unsigned integer texture size */
   struct lp_type int_size_in_type;
   struct lp_build_context int_size_in_bld;

   /** Float incoming texture size */
   struct lp_type float_size_in_type;
   struct lp_build_context float_size_in_bld;

   /** Unsigned integer texture size (might be per quad) */
   struct lp_type int_size_type;
   struct lp_build_context int_size_bld;

   /** Float texture size (might be per quad) */
   struct lp_type float_size_type;
   struct lp_build_context float_size_bld;

   /** Output texels type and build context */
   struct lp_type texel_type;
   struct lp_build_context texel_bld;

   /** Float level type */
   struct lp_type levelf_type;
   struct lp_build_context levelf_bld;

   /** Int level type */
   struct lp_type leveli_type;
   struct lp_build_context leveli_bld;

   /** Float lod type */
   struct lp_type lodf_type;
   struct lp_build_context lodf_bld;

   /** Int lod type */
   struct lp_type lodi_type;
   struct lp_build_context lodi_bld;

   /* Common dynamic state values */
   LLVMValueRef row_stride_array;
   LLVMValueRef img_stride_array;
   LLVMValueRef base_ptr;
   LLVMValueRef mip_offsets;

   /** Integer vector with texture width, height, depth */
   LLVMValueRef int_size;

   LLVMValueRef border_color_clamped;
};



/**
 * We only support a few wrap modes in lp_build_sample_wrap_linear_int() at
 * this time.  Return whether the given mode is supported by that function.
 */
static INLINE boolean
lp_is_simple_wrap_mode(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return TRUE;
   default:
      return FALSE;
   }
}


static INLINE void
apply_sampler_swizzle(struct lp_build_sample_context *bld,
                      LLVMValueRef *texel)
{
   unsigned char swizzles[4];

   swizzles[0] = bld->static_texture_state->swizzle_r;
   swizzles[1] = bld->static_texture_state->swizzle_g;
   swizzles[2] = bld->static_texture_state->swizzle_b;
   swizzles[3] = bld->static_texture_state->swizzle_a;

   lp_build_swizzle_soa_inplace(&bld->texel_bld, texel, swizzles);
}

/*
 * not really dimension as such, this indicates the amount of
 * "normal" texture coords subject to minification, wrapping etc.
 */
static INLINE unsigned
texture_dims(enum pipe_texture_target tex)
{
   switch (tex) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_BUFFER:
      return 1;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_CUBE:
      return 2;
   case PIPE_TEXTURE_CUBE_ARRAY:
      assert(0);
      return 2;
   case PIPE_TEXTURE_3D:
      return 3;
   default:
      assert(0 && "bad texture target in texture_dims()");
      return 2;
   }
}


boolean
lp_sampler_wrap_mode_uses_border_color(unsigned mode,
                                       unsigned min_img_filter,
                                       unsigned mag_img_filter);

/**
 * Derive the sampler static state.
 */
void
lp_sampler_static_sampler_state(struct lp_static_sampler_state *state,
                                const struct pipe_sampler_state *sampler);


void
lp_sampler_static_texture_state(struct lp_static_texture_state *state,
                                const struct pipe_sampler_view *view);


void
lp_build_lod_selector(struct lp_build_sample_context *bld,
                      unsigned texture_index,
                      unsigned sampler_index,
                      LLVMValueRef s,
                      LLVMValueRef t,
                      LLVMValueRef r,
                      LLVMValueRef cube_rho,
                      const struct lp_derivatives *derivs,
                      LLVMValueRef lod_bias, /* optional */
                      LLVMValueRef explicit_lod, /* optional */
                      unsigned mip_filter,
                      LLVMValueRef *out_lod_ipart,
                      LLVMValueRef *out_lod_fpart,
                      LLVMValueRef *out_lod_positive);

void
lp_build_nearest_mip_level(struct lp_build_sample_context *bld,
                           unsigned texture_unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level_out,
                           LLVMValueRef *out_of_bounds);

void
lp_build_linear_mip_levels(struct lp_build_sample_context *bld,
                           unsigned texture_unit,
                           LLVMValueRef lod_ipart,
                           LLVMValueRef *lod_fpart_inout,
                           LLVMValueRef *level0_out,
                           LLVMValueRef *level1_out);

LLVMValueRef
lp_build_get_mipmap_level(struct lp_build_sample_context *bld,
                          LLVMValueRef level);


LLVMValueRef
lp_build_get_mip_offsets(struct lp_build_sample_context *bld,
                         LLVMValueRef level);


void
lp_build_mipmap_level_sizes(struct lp_build_sample_context *bld,
                            LLVMValueRef ilevel,
                            LLVMValueRef *out_size_vec,
                            LLVMValueRef *row_stride_vec,
                            LLVMValueRef *img_stride_vec);


void
lp_build_extract_image_sizes(struct lp_build_sample_context *bld,
                             struct lp_build_context *size_bld,
                             struct lp_type coord_type,
                             LLVMValueRef size,
                             LLVMValueRef *out_width,
                             LLVMValueRef *out_height,
                             LLVMValueRef *out_depth);


void
lp_build_unnormalized_coords(struct lp_build_sample_context *bld,
                             LLVMValueRef flt_size,
                             LLVMValueRef *s,
                             LLVMValueRef *t,
                             LLVMValueRef *r);


void
lp_build_cube_lookup(struct lp_build_sample_context *bld,
                     LLVMValueRef *coords,
                     const struct lp_derivatives *derivs_in, /* optional */
                     LLVMValueRef *rho,
                     struct lp_derivatives *derivs_out, /* optional */
                     boolean need_derivs);


void
lp_build_cube_new_coords(struct lp_build_context *ivec_bld,
                         LLVMValueRef face,
                         LLVMValueRef x0,
                         LLVMValueRef x1,
                         LLVMValueRef y0,
                         LLVMValueRef y1,
                         LLVMValueRef max_coord,
                         LLVMValueRef new_faces[4],
                         LLVMValueRef new_xcoords[4][2],
                         LLVMValueRef new_ycoords[4][2]);


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
lp_build_sample_soa(struct gallivm_state *gallivm,
                    const struct lp_static_texture_state *static_texture_state,
                    const struct lp_static_sampler_state *static_sampler_state,
                    struct lp_sampler_dynamic_state *dynamic_texture_state,
                    struct lp_type fp_type,
                    boolean is_fetch,
                    unsigned texture_index,
                    unsigned sampler_index,
                    const LLVMValueRef *coords,
                    const LLVMValueRef *offsets,
                    const struct lp_derivatives *derivs,
                    LLVMValueRef lod_bias,
                    LLVMValueRef explicit_lod,
                    enum lp_sampler_lod_property lod_property,
                    LLVMValueRef texel_out[4]);


void
lp_build_coord_repeat_npot_linear(struct lp_build_sample_context *bld,
                                  LLVMValueRef coord_f,
                                  LLVMValueRef length_i,
                                  LLVMValueRef length_f,
                                  LLVMValueRef *coord0_i,
                                  LLVMValueRef *weight_f);


void
lp_build_size_query_soa(struct gallivm_state *gallivm,
                        const struct lp_static_texture_state *static_state,
                        struct lp_sampler_dynamic_state *dynamic_state,
                        struct lp_type int_type,
                        unsigned texture_unit,
                        unsigned target,
                        boolean is_sviewinfo,
                        enum lp_sampler_lod_property lod_property,
                        LLVMValueRef explicit_lod,
                        LLVMValueRef *sizes_out);

void
lp_build_sample_nop(struct gallivm_state *gallivm, 
                    struct lp_type type,
                    const LLVMValueRef *coords,
                    LLVMValueRef texel_out[4]);


LLVMValueRef
lp_build_minify(struct lp_build_context *bld,
                LLVMValueRef base_size,
                LLVMValueRef level,
                boolean lod_scalar);


#endif /* LP_BLD_SAMPLE_H */
