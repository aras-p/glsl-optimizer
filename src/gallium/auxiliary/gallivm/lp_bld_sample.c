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
 * Texture sampling -- common code.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "lp_bld_arit.h"
#include "lp_bld_const.h"
#include "lp_bld_debug.h"
#include "lp_bld_flow.h"
#include "lp_bld_sample.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_type.h"


/**
 * Does the given texture wrap mode allow sampling the texture border color?
 * XXX maybe move this into gallium util code.
 */
boolean
lp_sampler_wrap_mode_uses_border_color(unsigned mode,
                                       unsigned min_img_filter,
                                       unsigned mag_img_filter)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      return FALSE;
   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      if (min_img_filter == PIPE_TEX_FILTER_NEAREST &&
          mag_img_filter == PIPE_TEX_FILTER_NEAREST) {
         return FALSE;
      } else {
         return TRUE;
      }
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      return TRUE;
   default:
      assert(0 && "unexpected wrap mode");
      return FALSE;
   }
}


/**
 * Initialize lp_sampler_static_state object with the gallium sampler
 * and texture state.
 * The former is considered to be static and the later dynamic.
 */
void
lp_sampler_static_state(struct lp_sampler_static_state *state,
                        const struct pipe_sampler_view *view,
                        const struct pipe_sampler_state *sampler)
{
   const struct pipe_resource *texture = view->texture;

   memset(state, 0, sizeof *state);

   if(!texture)
      return;

   if(!sampler)
      return;

   /*
    * We don't copy sampler state over unless it is actually enabled, to avoid
    * spurious recompiles, as the sampler static state is part of the shader
    * key.
    *
    * Ideally the state tracker or cso_cache module would make all state
    * canonical, but until that happens it's better to be safe than sorry here.
    *
    * XXX: Actually there's much more than can be done here, especially
    * regarding 1D/2D/3D/CUBE textures, wrap modes, etc.
    */

   state->format            = view->format;
   state->swizzle_r         = view->swizzle_r;
   state->swizzle_g         = view->swizzle_g;
   state->swizzle_b         = view->swizzle_b;
   state->swizzle_a         = view->swizzle_a;

   state->target            = texture->target;
   state->pot_width         = util_is_power_of_two(texture->width0);
   state->pot_height        = util_is_power_of_two(texture->height0);
   state->pot_depth         = util_is_power_of_two(texture->depth0);

   state->wrap_s            = sampler->wrap_s;
   state->wrap_t            = sampler->wrap_t;
   state->wrap_r            = sampler->wrap_r;
   state->min_img_filter    = sampler->min_img_filter;
   state->mag_img_filter    = sampler->mag_img_filter;
   if (view->last_level) {
      state->min_mip_filter = sampler->min_mip_filter;
   } else {
      state->min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   }

   /* If min_lod == max_lod we can greatly simplify mipmap selection.
    * This is a case that occurs during automatic mipmap generation.
    */
   if (sampler->min_lod == sampler->max_lod) {
      state->min_max_lod_equal = 1;
   }

   state->compare_mode      = sampler->compare_mode;
   if (sampler->compare_mode != PIPE_TEX_COMPARE_NONE) {
      state->compare_func   = sampler->compare_func;
   }

   state->normalized_coords = sampler->normalized_coords;

   /*
    * FIXME: Handle the remainder of pipe_sampler_view.
    */
}


/**
 * Generate code to compute texture level of detail (lambda).
 * \param ddx  partial derivatives of (s, t, r, q) with respect to X
 * \param ddy  partial derivatives of (s, t, r, q) with respect to Y
 * \param lod_bias  optional float vector with the shader lod bias
 * \param explicit_lod  optional float vector with the explicit lod
 * \param width  scalar int texture width
 * \param height  scalar int texture height
 * \param depth  scalar int texture depth
 *
 * XXX: The resulting lod is scalar, so ignore all but the first element of
 * derivatives, lod_bias, etc that are passed by the shader.
 */
LLVMValueRef
lp_build_lod_selector(struct lp_build_sample_context *bld,
                      unsigned unit,
                      const LLVMValueRef ddx[4],
                      const LLVMValueRef ddy[4],
                      LLVMValueRef lod_bias, /* optional */
                      LLVMValueRef explicit_lod, /* optional */
                      LLVMValueRef width,
                      LLVMValueRef height,
                      LLVMValueRef depth)

{
   LLVMValueRef min_lod =
      bld->dynamic_state->min_lod(bld->dynamic_state, bld->builder, unit);

   if (bld->static_state->min_max_lod_equal) {
      /* User is forcing sampling from a particular mipmap level.
       * This is hit during mipmap generation.
       */
      return min_lod;
   }
   else {
      struct lp_build_context *float_bld = &bld->float_bld;
      LLVMValueRef sampler_lod_bias =
         bld->dynamic_state->lod_bias(bld->dynamic_state, bld->builder, unit);
      LLVMValueRef max_lod =
         bld->dynamic_state->max_lod(bld->dynamic_state, bld->builder, unit);
      LLVMValueRef index0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
      LLVMValueRef lod;

      if (explicit_lod) {
         lod = LLVMBuildExtractElement(bld->builder, explicit_lod,
                                       index0, "");
      }
      else {
         const int dims = texture_dims(bld->static_state->target);
         LLVMValueRef dsdx, dsdy;
         LLVMValueRef dtdx = NULL, dtdy = NULL, drdx = NULL, drdy = NULL;
         LLVMValueRef rho;

         dsdx = LLVMBuildExtractElement(bld->builder, ddx[0], index0, "dsdx");
         dsdx = lp_build_abs(float_bld, dsdx);
         dsdy = LLVMBuildExtractElement(bld->builder, ddy[0], index0, "dsdy");
         dsdy = lp_build_abs(float_bld, dsdy);
         if (dims > 1) {
            dtdx = LLVMBuildExtractElement(bld->builder, ddx[1], index0, "dtdx");
            dtdx = lp_build_abs(float_bld, dtdx);
            dtdy = LLVMBuildExtractElement(bld->builder, ddy[1], index0, "dtdy");
            dtdy = lp_build_abs(float_bld, dtdy);
            if (dims > 2) {
               drdx = LLVMBuildExtractElement(bld->builder, ddx[2], index0, "drdx");
               drdx = lp_build_abs(float_bld, drdx);
               drdy = LLVMBuildExtractElement(bld->builder, ddy[2], index0, "drdy");
               drdy = lp_build_abs(float_bld, drdy);
            }
         }

         /* Compute rho = max of all partial derivatives scaled by texture size.
          * XXX this could be vectorized somewhat
          */
         rho = LLVMBuildFMul(bld->builder,
                            lp_build_max(float_bld, dsdx, dsdy),
                            lp_build_int_to_float(float_bld, width), "");
         if (dims > 1) {
            LLVMValueRef max;
            max = LLVMBuildFMul(bld->builder,
                               lp_build_max(float_bld, dtdx, dtdy),
                               lp_build_int_to_float(float_bld, height), "");
            rho = lp_build_max(float_bld, rho, max);
            if (dims > 2) {
               max = LLVMBuildFMul(bld->builder,
                                  lp_build_max(float_bld, drdx, drdy),
                                  lp_build_int_to_float(float_bld, depth), "");
               rho = lp_build_max(float_bld, rho, max);
            }
         }

         /* compute lod = log2(rho) */
#if 0
         lod = lp_build_log2(float_bld, rho);
#else
         lod = lp_build_fast_log2(float_bld, rho);
#endif

         /* add shader lod bias */
         if (lod_bias) {
            lod_bias = LLVMBuildExtractElement(bld->builder, lod_bias,
                                               index0, "");
            lod = LLVMBuildFAdd(bld->builder, lod, lod_bias, "shader_lod_bias");
         }
      }

      /* add sampler lod bias */
      lod = LLVMBuildFAdd(bld->builder, lod, sampler_lod_bias, "sampler_lod_bias");

      /* clamp lod */
      lod = lp_build_clamp(float_bld, lod, min_lod, max_lod);

      return lod;
   }
}


/**
 * For PIPE_TEX_MIPFILTER_NEAREST, convert float LOD to integer
 * mipmap level index.
 * Note: this is all scalar code.
 * \param lod  scalar float texture level of detail
 * \param level_out  returns integer 
 */
void
lp_build_nearest_mip_level(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level_out)
{
   struct lp_build_context *float_bld = &bld->float_bld;
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMValueRef last_level, level;

   LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);

   last_level = bld->dynamic_state->last_level(bld->dynamic_state,
                                               bld->builder, unit);

   /* convert float lod to integer */
   level = lp_build_iround(float_bld, lod);

   /* clamp level to legal range of levels */
   *level_out = lp_build_clamp(int_bld, level, zero, last_level);
}


/**
 * For PIPE_TEX_MIPFILTER_LINEAR, convert float LOD to integer to
 * two (adjacent) mipmap level indexes.  Later, we'll sample from those
 * two mipmap levels and interpolate between them.
 */
void
lp_build_linear_mip_levels(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level0_out,
                           LLVMValueRef *level1_out,
                           LLVMValueRef *weight_out)
{
   struct lp_build_context *float_bld = &bld->float_bld;
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMValueRef last_level, level;

   last_level = bld->dynamic_state->last_level(bld->dynamic_state,
                                               bld->builder, unit);

   /* convert float lod to integer */
   lp_build_ifloor_fract(float_bld, lod, &level, weight_out);

   /* compute level 0 and clamp to legal range of levels */
   *level0_out = lp_build_clamp(int_bld, level,
                                int_bld->zero,
                                last_level);
   /* compute level 1 and clamp to legal range of levels */
   level = lp_build_add(int_bld, level, int_bld->one);
   *level1_out = lp_build_clamp(int_bld, level,
                                int_bld->zero,
                                last_level);
}


/**
 * Return pointer to a single mipmap level.
 * \param data_array  array of pointers to mipmap levels
 * \param level  integer mipmap level
 */
LLVMValueRef
lp_build_get_mipmap_level(struct lp_build_sample_context *bld,
                          LLVMValueRef data_array, LLVMValueRef level)
{
   LLVMValueRef indexes[2], data_ptr;
   indexes[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indexes[1] = level;
   data_ptr = LLVMBuildGEP(bld->builder, data_array, indexes, 2, "");
   data_ptr = LLVMBuildLoad(bld->builder, data_ptr, "");
   return data_ptr;
}


LLVMValueRef
lp_build_get_const_mipmap_level(struct lp_build_sample_context *bld,
                                LLVMValueRef data_array, int level)
{
   LLVMValueRef lvl = LLVMConstInt(LLVMInt32Type(), level, 0);
   return lp_build_get_mipmap_level(bld, data_array, lvl);
}


/**
 * Codegen equivalent for u_minify().
 * Return max(1, base_size >> level);
 */
static LLVMValueRef
lp_build_minify(struct lp_build_sample_context *bld,
                LLVMValueRef base_size,
                LLVMValueRef level)
{
   if (level == bld->int_coord_bld.zero) {
      /* if we're using mipmap level zero, no minification is needed */
      return base_size;
   }
   else {
      LLVMValueRef size =
         LLVMBuildLShr(bld->builder, base_size, level, "minify");
      size = lp_build_max(&bld->int_coord_bld, size, bld->int_coord_bld.one);
      return size;
   }
}


/**
 * Dereference stride_array[mipmap_level] array to get a stride.
 * Return stride as a vector.
 */
static LLVMValueRef
lp_build_get_level_stride_vec(struct lp_build_sample_context *bld,
                              LLVMValueRef stride_array, LLVMValueRef level)
{
   LLVMValueRef indexes[2], stride;
   indexes[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indexes[1] = level;
   stride = LLVMBuildGEP(bld->builder, stride_array, indexes, 2, "");
   stride = LLVMBuildLoad(bld->builder, stride, "");
   stride = lp_build_broadcast_scalar(&bld->int_coord_bld, stride);
   return stride;
}


/**
 * When sampling a mipmap, we need to compute the width, height, depth
 * of the source levels from the level indexes.  This helper function
 * does that.
 */
void
lp_build_mipmap_level_sizes(struct lp_build_sample_context *bld,
                            unsigned dims,
                            LLVMValueRef width_vec,
                            LLVMValueRef height_vec,
                            LLVMValueRef depth_vec,
                            LLVMValueRef ilevel0,
                            LLVMValueRef ilevel1,
                            LLVMValueRef row_stride_array,
                            LLVMValueRef img_stride_array,
                            LLVMValueRef *width0_vec,
                            LLVMValueRef *width1_vec,
                            LLVMValueRef *height0_vec,
                            LLVMValueRef *height1_vec,
                            LLVMValueRef *depth0_vec,
                            LLVMValueRef *depth1_vec,
                            LLVMValueRef *row_stride0_vec,
                            LLVMValueRef *row_stride1_vec,
                            LLVMValueRef *img_stride0_vec,
                            LLVMValueRef *img_stride1_vec)
{
   const unsigned mip_filter = bld->static_state->min_mip_filter;
   LLVMValueRef ilevel0_vec, ilevel1_vec;

   ilevel0_vec = lp_build_broadcast_scalar(&bld->int_coord_bld, ilevel0);
   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
      ilevel1_vec = lp_build_broadcast_scalar(&bld->int_coord_bld, ilevel1);

   /*
    * Compute width, height, depth at mipmap level 'ilevel0'
    */
   *width0_vec = lp_build_minify(bld, width_vec, ilevel0_vec);
   if (dims >= 2) {
      *height0_vec = lp_build_minify(bld, height_vec, ilevel0_vec);
      *row_stride0_vec = lp_build_get_level_stride_vec(bld,
                                                       row_stride_array,
                                                       ilevel0);
      if (dims == 3 || bld->static_state->target == PIPE_TEXTURE_CUBE) {
         *img_stride0_vec = lp_build_get_level_stride_vec(bld,
                                                          img_stride_array,
                                                          ilevel0);
         if (dims == 3) {
            *depth0_vec = lp_build_minify(bld, depth_vec, ilevel0_vec);
         }
      }
   }
   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      /* compute width, height, depth for second mipmap level at 'ilevel1' */
      *width1_vec = lp_build_minify(bld, width_vec, ilevel1_vec);
      if (dims >= 2) {
         *height1_vec = lp_build_minify(bld, height_vec, ilevel1_vec);
         *row_stride1_vec = lp_build_get_level_stride_vec(bld,
                                                          row_stride_array,
                                                          ilevel1);
         if (dims == 3 || bld->static_state->target == PIPE_TEXTURE_CUBE) {
            *img_stride1_vec = lp_build_get_level_stride_vec(bld,
                                                             img_stride_array,
                                                             ilevel1);
            if (dims == 3) {
               *depth1_vec = lp_build_minify(bld, depth_vec, ilevel1_vec);
            }
         }
      }
   }
}



/** Helper used by lp_build_cube_lookup() */
static LLVMValueRef
lp_build_cube_ima(struct lp_build_context *coord_bld, LLVMValueRef coord)
{
   /* ima = -0.5 / abs(coord); */
   LLVMValueRef negHalf = lp_build_const_vec(coord_bld->type, -0.5);
   LLVMValueRef absCoord = lp_build_abs(coord_bld, coord);
   LLVMValueRef ima = lp_build_div(coord_bld, negHalf, absCoord);
   return ima;
}


/**
 * Helper used by lp_build_cube_lookup()
 * \param sign  scalar +1 or -1
 * \param coord  float vector
 * \param ima  float vector
 */
static LLVMValueRef
lp_build_cube_coord(struct lp_build_context *coord_bld,
                    LLVMValueRef sign, int negate_coord,
                    LLVMValueRef coord, LLVMValueRef ima)
{
   /* return negate(coord) * ima * sign + 0.5; */
   LLVMValueRef half = lp_build_const_vec(coord_bld->type, 0.5);
   LLVMValueRef res;

   assert(negate_coord == +1 || negate_coord == -1);

   if (negate_coord == -1) {
      coord = lp_build_negate(coord_bld, coord);
   }

   res = lp_build_mul(coord_bld, coord, ima);
   if (sign) {
      sign = lp_build_broadcast_scalar(coord_bld, sign);
      res = lp_build_mul(coord_bld, res, sign);
   }
   res = lp_build_add(coord_bld, res, half);

   return res;
}


/** Helper used by lp_build_cube_lookup()
 * Return (major_coord >= 0) ? pos_face : neg_face;
 */
static LLVMValueRef
lp_build_cube_face(struct lp_build_sample_context *bld,
                   LLVMValueRef major_coord,
                   unsigned pos_face, unsigned neg_face)
{
   LLVMValueRef cmp = LLVMBuildFCmp(bld->builder, LLVMRealUGE,
                                    major_coord,
                                    bld->float_bld.zero, "");
   LLVMValueRef pos = LLVMConstInt(LLVMInt32Type(), pos_face, 0);
   LLVMValueRef neg = LLVMConstInt(LLVMInt32Type(), neg_face, 0);
   LLVMValueRef res = LLVMBuildSelect(bld->builder, cmp, pos, neg, "");
   return res;
}



/**
 * Generate code to do cube face selection and compute per-face texcoords.
 */
void
lp_build_cube_lookup(struct lp_build_sample_context *bld,
                     LLVMValueRef s,
                     LLVMValueRef t,
                     LLVMValueRef r,
                     LLVMValueRef *face,
                     LLVMValueRef *face_s,
                     LLVMValueRef *face_t)
{
   struct lp_build_context *float_bld = &bld->float_bld;
   struct lp_build_context *coord_bld = &bld->coord_bld;
   LLVMValueRef rx, ry, rz;
   LLVMValueRef arx, ary, arz;
   LLVMValueRef c25 = LLVMConstReal(LLVMFloatType(), 0.25);
   LLVMValueRef arx_ge_ary, arx_ge_arz;
   LLVMValueRef ary_ge_arx, ary_ge_arz;
   LLVMValueRef arx_ge_ary_arz, ary_ge_arx_arz;
   LLVMValueRef rx_pos, ry_pos, rz_pos;

   assert(bld->coord_bld.type.length == 4);

   /*
    * Use the average of the four pixel's texcoords to choose the face.
    */
   rx = lp_build_mul(float_bld, c25,
                     lp_build_sum_vector(&bld->coord_bld, s));
   ry = lp_build_mul(float_bld, c25,
                     lp_build_sum_vector(&bld->coord_bld, t));
   rz = lp_build_mul(float_bld, c25,
                     lp_build_sum_vector(&bld->coord_bld, r));

   arx = lp_build_abs(float_bld, rx);
   ary = lp_build_abs(float_bld, ry);
   arz = lp_build_abs(float_bld, rz);

   /*
    * Compare sign/magnitude of rx,ry,rz to determine face
    */
   arx_ge_ary = LLVMBuildFCmp(bld->builder, LLVMRealUGE, arx, ary, "");
   arx_ge_arz = LLVMBuildFCmp(bld->builder, LLVMRealUGE, arx, arz, "");
   ary_ge_arx = LLVMBuildFCmp(bld->builder, LLVMRealUGE, ary, arx, "");
   ary_ge_arz = LLVMBuildFCmp(bld->builder, LLVMRealUGE, ary, arz, "");

   arx_ge_ary_arz = LLVMBuildAnd(bld->builder, arx_ge_ary, arx_ge_arz, "");
   ary_ge_arx_arz = LLVMBuildAnd(bld->builder, ary_ge_arx, ary_ge_arz, "");

   rx_pos = LLVMBuildFCmp(bld->builder, LLVMRealUGE, rx, float_bld->zero, "");
   ry_pos = LLVMBuildFCmp(bld->builder, LLVMRealUGE, ry, float_bld->zero, "");
   rz_pos = LLVMBuildFCmp(bld->builder, LLVMRealUGE, rz, float_bld->zero, "");

   {
      struct lp_build_flow_context *flow_ctx;
      struct lp_build_if_state if_ctx;

      flow_ctx = lp_build_flow_create(bld->builder);
      lp_build_flow_scope_begin(flow_ctx);

      *face_s = bld->coord_bld.undef;
      *face_t = bld->coord_bld.undef;
      *face = bld->int_bld.undef;

      lp_build_name(*face_s, "face_s");
      lp_build_name(*face_t, "face_t");
      lp_build_name(*face, "face");

      lp_build_flow_scope_declare(flow_ctx, face_s);
      lp_build_flow_scope_declare(flow_ctx, face_t);
      lp_build_flow_scope_declare(flow_ctx, face);

      lp_build_if(&if_ctx, flow_ctx, bld->builder, arx_ge_ary_arz);
      {
         /* +/- X face */
         LLVMValueRef sign = lp_build_sgn(float_bld, rx);
         LLVMValueRef ima = lp_build_cube_ima(coord_bld, s);
         *face_s = lp_build_cube_coord(coord_bld, sign, +1, r, ima);
         *face_t = lp_build_cube_coord(coord_bld, NULL, +1, t, ima);
         *face = lp_build_cube_face(bld, rx,
                                    PIPE_TEX_FACE_POS_X,
                                    PIPE_TEX_FACE_NEG_X);
      }
      lp_build_else(&if_ctx);
      {
         struct lp_build_flow_context *flow_ctx2;
         struct lp_build_if_state if_ctx2;

         LLVMValueRef face_s2 = bld->coord_bld.undef;
         LLVMValueRef face_t2 = bld->coord_bld.undef;
         LLVMValueRef face2 = bld->int_bld.undef;

         flow_ctx2 = lp_build_flow_create(bld->builder);
         lp_build_flow_scope_begin(flow_ctx2);
         lp_build_flow_scope_declare(flow_ctx2, &face_s2);
         lp_build_flow_scope_declare(flow_ctx2, &face_t2);
         lp_build_flow_scope_declare(flow_ctx2, &face2);

         ary_ge_arx_arz = LLVMBuildAnd(bld->builder, ary_ge_arx, ary_ge_arz, "");

         lp_build_if(&if_ctx2, flow_ctx2, bld->builder, ary_ge_arx_arz);
         {
            /* +/- Y face */
            LLVMValueRef sign = lp_build_sgn(float_bld, ry);
            LLVMValueRef ima = lp_build_cube_ima(coord_bld, t);
            face_s2 = lp_build_cube_coord(coord_bld, NULL, -1, s, ima);
            face_t2 = lp_build_cube_coord(coord_bld, sign, -1, r, ima);
            face2 = lp_build_cube_face(bld, ry,
                                       PIPE_TEX_FACE_POS_Y,
                                       PIPE_TEX_FACE_NEG_Y);
         }
         lp_build_else(&if_ctx2);
         {
            /* +/- Z face */
            LLVMValueRef sign = lp_build_sgn(float_bld, rz);
            LLVMValueRef ima = lp_build_cube_ima(coord_bld, r);
            face_s2 = lp_build_cube_coord(coord_bld, sign, -1, s, ima);
            face_t2 = lp_build_cube_coord(coord_bld, NULL, +1, t, ima);
            face2 = lp_build_cube_face(bld, rz,
                                       PIPE_TEX_FACE_POS_Z,
                                       PIPE_TEX_FACE_NEG_Z);
         }
         lp_build_endif(&if_ctx2);
         lp_build_flow_scope_end(flow_ctx2);
         lp_build_flow_destroy(flow_ctx2);
         *face_s = face_s2;
         *face_t = face_t2;
         *face = face2;
      }

      lp_build_endif(&if_ctx);
      lp_build_flow_scope_end(flow_ctx);
      lp_build_flow_destroy(flow_ctx);
   }
}


/**
 * Compute the partial offset of a pixel block along an arbitrary axis.
 *
 * @param coord   coordinate in pixels
 * @param stride  number of bytes between rows of successive pixel blocks
 * @param block_length  number of pixels in a pixels block along the coordinate
 *                      axis
 * @param out_offset    resulting relative offset of the pixel block in bytes
 * @param out_subcoord  resulting sub-block pixel coordinate
 */
void
lp_build_sample_partial_offset(struct lp_build_context *bld,
                               unsigned block_length,
                               LLVMValueRef coord,
                               LLVMValueRef stride,
                               LLVMValueRef *out_offset,
                               LLVMValueRef *out_subcoord)
{
   LLVMValueRef offset;
   LLVMValueRef subcoord;

   if (block_length == 1) {
      subcoord = bld->zero;
   }
   else {
      /*
       * Pixel blocks have power of two dimensions. LLVM should convert the
       * rem/div to bit arithmetic.
       * TODO: Verify this.
       * It does indeed BUT it does transform it to scalar (and back) when doing so
       * (using roughly extract, shift/and, mov, unpack) (llvm 2.7).
       * The generated code looks seriously unfunny and is quite expensive.
       */
#if 0
      LLVMValueRef block_width = lp_build_const_int_vec(bld->type, block_length);
      subcoord = LLVMBuildURem(bld->builder, coord, block_width, "");
      coord    = LLVMBuildUDiv(bld->builder, coord, block_width, "");
#else
      unsigned logbase2 = util_unsigned_logbase2(block_length);
      LLVMValueRef block_shift = lp_build_const_int_vec(bld->type, logbase2);
      LLVMValueRef block_mask = lp_build_const_int_vec(bld->type, block_length - 1);
      subcoord = LLVMBuildAnd(bld->builder, coord, block_mask, "");
      coord = LLVMBuildLShr(bld->builder, coord, block_shift, "");
#endif
   }

   offset = lp_build_mul(bld, coord, stride);

   assert(out_offset);
   assert(out_subcoord);

   *out_offset = offset;
   *out_subcoord = subcoord;
}


/**
 * Compute the offset of a pixel block.
 *
 * x, y, z, y_stride, z_stride are vectors, and they refer to pixels.
 *
 * Returns the relative offset and i,j sub-block coordinates
 */
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
                       LLVMValueRef *out_j)
{
   LLVMValueRef x_stride;
   LLVMValueRef offset;

   x_stride = lp_build_const_vec(bld->type, format_desc->block.bits/8);

   lp_build_sample_partial_offset(bld,
                                  format_desc->block.width,
                                  x, x_stride,
                                  &offset, out_i);

   if (y && y_stride) {
      LLVMValueRef y_offset;
      lp_build_sample_partial_offset(bld,
                                     format_desc->block.height,
                                     y, y_stride,
                                     &y_offset, out_j);
      offset = lp_build_add(bld, offset, y_offset);
   }
   else {
      *out_j = bld->zero;
   }

   if (z && z_stride) {
      LLVMValueRef z_offset;
      LLVMValueRef k;
      lp_build_sample_partial_offset(bld,
                                     1, /* pixel blocks are always 2D */
                                     z, z_stride,
                                     &z_offset, &k);
      offset = lp_build_add(bld, offset, z_offset);
   }

   *out_offset = offset;
}
