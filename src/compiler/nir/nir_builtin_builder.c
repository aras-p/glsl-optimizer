/*
 * Copyright © 2018 Red Hat Inc.
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <math.h>

#include "nir.h"
#include "nir_builder.h"
#include "nir_builtin_builder.h"

#ifndef M_PIf
#define M_PIf   ((float) M_PI)
#endif
#ifndef M_PI_2f
#define M_PI_2f ((float) M_PI_2)
#endif

nir_def *
nir_cross3(nir_builder *b, nir_def *x, nir_def *y)
{
   unsigned yzx[3] = { 1, 2, 0 };
   unsigned zxy[3] = { 2, 0, 1 };

   return nir_ffma_weak(b, nir_swizzle(b, x, yzx, 3),
                           nir_swizzle(b, y, zxy, 3),
                           nir_fneg(b, nir_fmul(b, nir_swizzle(b, x, zxy, 3),
                                                   nir_swizzle(b, y, yzx, 3))));
}

nir_def *
nir_cross4(nir_builder *b, nir_def *x, nir_def *y)
{
   nir_def *cross = nir_cross3(b, x, y);

   return nir_vec4(b,
                   nir_channel(b, cross, 0),
                   nir_channel(b, cross, 1),
                   nir_channel(b, cross, 2),
                   nir_imm_intN_t(b, 0, cross->bit_size));
}

nir_def *
nir_fast_length(nir_builder *b, nir_def *vec)
{
   return nir_fsqrt(b, nir_fdot(b, vec, vec));
}

nir_def *
nir_nextafter(nir_builder *b, nir_def *x, nir_def *y)
{
   nir_def *zero = nir_imm_intN_t(b, 0, x->bit_size);
   nir_def *one = nir_imm_intN_t(b, 1, x->bit_size);

   nir_def *condeq = nir_feq(b, x, y);
   nir_def *conddir = nir_flt(b, x, y);
   nir_def *condzero = nir_feq(b, x, zero);

   uint64_t sign_mask = 1ull << (x->bit_size - 1);
   uint64_t min_abs = 1;

   if (nir_is_denorm_flush_to_zero(b->shader->info.float_controls_execution_mode, x->bit_size)) {
      switch (x->bit_size) {
      case 16:
         min_abs = 1 << 10;
         break;
      case 32:
         min_abs = 1 << 23;
         break;
      case 64:
         min_abs = 1ULL << 52;
         break;
      }

      /* Flush denorm to zero to avoid returning a denorm when condeq is true. */
      x = nir_fmul_imm(b, x, 1.0);
   }

   /* beware of: +/-0.0 - 1 == NaN */
   nir_def *xn =
      nir_bcsel(b,
                condzero,
                nir_imm_intN_t(b, sign_mask | min_abs, x->bit_size),
                nir_isub(b, x, one));

   /* beware of -0.0 + 1 == -0x1p-149 */
   nir_def *xp = nir_bcsel(b, condzero,
                           nir_imm_intN_t(b, min_abs, x->bit_size),
                           nir_iadd(b, x, one));

   /* nextafter can be implemented by just +/- 1 on the int value */
   nir_def *res =
      nir_bcsel(b, nir_ixor(b, conddir, nir_flt(b, x, zero)), xp, xn);

   return nir_nan_check2(b, x, y, nir_bcsel(b, condeq, x, res));
}

nir_def *
nir_normalize(nir_builder *b, nir_def *vec)
{
   if (vec->num_components == 1)
      return nir_fsign(b, vec);

   nir_def *f0 = nir_imm_floatN_t(b, 0.0, vec->bit_size);
   nir_def *f1 = nir_imm_floatN_t(b, 1.0, vec->bit_size);
   nir_def *finf = nir_imm_floatN_t(b, INFINITY, vec->bit_size);

   /* scale the input to increase precision */
   nir_def *maxc = nir_fmax_abs_vec_comp(b, vec);
   nir_def *svec = nir_fdiv(b, vec, maxc);
   /* for inf */
   nir_def *finfvec = nir_copysign(b, nir_bcsel(b, nir_feq(b, vec, finf), f1, f0), f1);

   nir_def *temp = nir_bcsel(b, nir_feq(b, maxc, finf), finfvec, svec);
   nir_def *res = nir_fmul(b, temp, nir_frsq(b, nir_fdot(b, temp, temp)));

   return nir_bcsel(b, nir_feq(b, maxc, f0), vec, res);
}

nir_def *
nir_smoothstep(nir_builder *b, nir_def *edge0, nir_def *edge1, nir_def *x)
{
   nir_def *f2 = nir_imm_floatN_t(b, 2.0, x->bit_size);
   nir_def *f3 = nir_imm_floatN_t(b, 3.0, x->bit_size);

   /* t = clamp((x - edge0) / (edge1 - edge0), 0, 1) */
   nir_def *t =
      nir_fsat(b, nir_fdiv(b, nir_fsub(b, x, edge0),
                           nir_fsub(b, edge1, edge0)));

   /* result = t * t * (3 - 2 * t) */
   return nir_fmul(b, t, nir_fmul(b, t, nir_a_minus_bc(b, f3, f2, t)));
}

nir_def *
nir_upsample(nir_builder *b, nir_def *hi, nir_def *lo)
{
   assert(lo->num_components == hi->num_components);
   assert(lo->bit_size == hi->bit_size);

   nir_def *res[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < lo->num_components; ++i) {
      nir_def *vec = nir_vec2(b, nir_channel(b, lo, i), nir_channel(b, hi, i));
      res[i] = nir_pack_bits(b, vec, vec->bit_size * 2);
   }

   return nir_vec(b, res, lo->num_components);
}

/**
 * Approximate asin(x) by formula 4.45 from Abramowitz & Stegun, "Handbook
 * of Mathematical Functions":
 *
 * asin~(x) = (π/2 - sqrt(1 - |x|) * ( a0 + a1 * |x| + a2 * |x|^2 + a3 * |x|^3 )
 *
 * where a0 = 1.5707288 a1 = -0.2121144 a2 = 0.0742610 a3 = -0.0187203
 *
 * This has a very small absolute error, but the relative error can become
 * large when |x| is small. For small |x| the Taylor series makes a good
 * approximation, so when the relative error matters (i.e. for asin rather
 * than acos) we do a piecewise approximation with the Taylor series for
 * |x| < 0.21502245 and formula 4.45 elsewhere. The crossover point is
 * the value in [0.1, 0.7071] where the two approximations are equal.
 */
static nir_def *
build_asin(nir_builder *b, nir_def *x, bool piecewise)
{
   /* The polynomial approximation may not be precise enough to meet half-float
    * precision requirements. Alternatively, we could implement this using
    * the formula:
    *
    * asin(x) = atan2(x, sqrt(1 - x*x))
    *
    * But that is very expensive, so instead we enforce the polynomial
    * approximation in 32-bit math and then the caller can convert the result
    * back to 16-bit.
    */
   assert(x->bit_size != 16);
   nir_def *abs_x = nir_fabs(b, x);

   nir_def *p0_plus_xp1 = nir_ffma_weak_imm12(b, abs_x, -0.0187293, 0.0742610);

   nir_def *expr_tail =
      nir_ffma_weak_imm2(b, abs_x,
                            nir_ffma_weak_imm2(b, abs_x, p0_plus_xp1, -0.2121144),
                            1.5707288);

   nir_def *result0 = nir_fmul(b, nir_fsign(b, x),
                      nir_a_minus_bc(b, nir_imm_floatN_t(b, M_PI_2f, x->bit_size),
                                        nir_fsqrt(b,
                                                  nir_fsub_imm(b, 1.0, abs_x)),
                                        expr_tail));
   if (piecewise) {
      /* use taylor approximation for |x| < 0.21502245 */

      nir_def *x2 = nir_fmul(b, x, x);
      nir_def *result1 = nir_fmul(b,
                                  x,
                                  nir_ffma_weak_imm12(b, x2, (1.0/6.0), 1.0));
      return nir_bcsel(b,
                       nir_flt_imm(b, abs_x, 0.21502245),
                       result1,
                       result0);
   } else {
      return result0;
   }
}

nir_def *
nir_asin(nir_builder *b, nir_def *x)
{
   /* See build_asin for promotion explanation */
   if (x->bit_size == 16)
      return nir_f2f16(b, nir_asin(b, nir_f2f32(b, x)));

   /* use piecewise approximation to keep low relative error near 0 */
   return build_asin(b, x, true);
}

nir_def *
nir_acos(nir_builder *b, nir_def *x)
{
   /* Promote acos in a similar fashion to asin to reduce error */
   if (x->bit_size == 16)
      return nir_f2f16(b, nir_acos(b, nir_f2f32(b, x)));

   /* piecewise approximation not needed to keep low relative error */
   return nir_fsub_imm(b, M_PI_2f, build_asin(b, x, false));
}

nir_def *
nir_atan(nir_builder *b, nir_def *y_over_x)
{
   const uint32_t bit_size = y_over_x->bit_size;

   nir_def *abs_y_over_x = nir_fabs(b, y_over_x);

   /*
    * range-reduction, first step:
    *
    *      / y_over_x         if |y_over_x| <= 1.0;
    * u = <
    *      \ 1.0 / y_over_x   otherwise
    *
    * x = |u| for the corrected sign.
    */
   nir_def *le_1 = nir_fle_imm(b, abs_y_over_x, 1.0);
   nir_def *u = nir_bcsel(b, le_1, y_over_x, nir_frcp(b, y_over_x));

   /*
    * approximate atan by evaluating polynomial using Horner's method:
    *
    * x   * 0.9999793128310355 - x^3  * 0.3326756418091246 +
    * x^5 * 0.1938924977115610 - x^7  * 0.1173503194786851 +
    * x^9 * 0.0536813784310406 - x^11 * 0.0121323213173444
    */
   float coeffs[] = {
      -0.0121323213173444f, 0.0536813784310406f,
      -0.1173503194786851f, 0.1938924977115610f,
      -0.3326756418091246f, 0.9999793128310355f
   };

   nir_def *x_2 = nir_fmul(b, u, u);
   nir_def *res = nir_imm_floatN_t(b, coeffs[0], bit_size);

   for (unsigned i = 1; i < ARRAY_SIZE(coeffs); ++i) {
      res = nir_ffma_weak_imm2(b, res, x_2, coeffs[i]);
   }

   /* range-reduction fixup value */
   nir_def *bias = nir_bcsel(b, le_1, nir_imm_floatN_t(b, 0, bit_size),
                             nir_imm_floatN_t(b, -M_PI_2, bit_size));

   /* multiply through by x while fixing up the range reduction */
   nir_def *tmp = nir_ffma_weak(b, nir_fabs(b, u), res, bias);

   /* sign fixup */
   return nir_copysign(b, tmp, y_over_x);
}

nir_def *
nir_atan2(nir_builder *b, nir_def *y, nir_def *x)
{
   assert(y->bit_size == x->bit_size);
   const uint32_t bit_size = x->bit_size;

   /* For zero inputs, we end up with intermediate infinities from
    * frcp(0.0). The final output is not infinity though, so this has to
    * be well defined even when applications don't request preserving infinities
    * on their own. Also preserve signed zeros to make the sign of the infinities
    * well defined.
    */
   unsigned old_fp_math_ctrl = b->fp_math_ctrl;
   b->fp_math_ctrl |= nir_fp_preserve_signed_zero | nir_fp_preserve_inf;

   nir_def *zero = nir_imm_floatN_t(b, 0, bit_size);
   nir_def *one = nir_imm_floatN_t(b, 1, bit_size);

   /* If we're on the left half-plane rotate the coordinates π/2 clock-wise
    * for the y=0 discontinuity to end up aligned with the vertical
    * discontinuity of atan(s/t) along t=0.  This also makes sure that we
    * don't attempt to divide by zero along the vertical line, which may give
    * unspecified results on non-GLSL 4.1-capable hardware.
    */
   nir_def *flip = nir_fge(b, zero, x);
   nir_def *s = nir_bcsel(b, flip, nir_fabs(b, x), y);
   nir_def *t = nir_bcsel(b, flip, y, nir_fabs(b, x));

   /* If the magnitude of the denominator exceeds some huge value, scale down
    * the arguments in order to prevent the reciprocal operation from flushing
    * its result to zero, which would cause precision problems, and for s
    * infinite would cause us to return a NaN instead of the correct finite
    * value.
    *
    * If fmin and fmax are respectively the smallest and largest positive
    * normalized floating point values representable by the implementation,
    * the constants below should be in agreement with:
    *
    *    huge <= 1 / fmin
    *    scale <= 1 / fmin / fmax (for |t| >= huge)
    *
    * In addition scale should be a negative power of two in order to avoid
    * loss of precision.  The values chosen below should work for most usual
    * floating point representations with at least the dynamic range of ATI's
    * 24-bit representation.
    */
   const double huge_val = bit_size >= 32 ? 1e18 : 16384;
   nir_def *scale = nir_bcsel(b, nir_fge_imm(b, nir_fabs(b, t), huge_val),
                              nir_imm_floatN_t(b, 0.25, bit_size), one);
   nir_def *rcp_scaled_t = nir_frcp(b, nir_fmul(b, t, scale));
   nir_def *abs_s_over_t = nir_fmul(b, nir_fabs(b, nir_fmul(b, s, scale)),
                                    nir_fabs(b, rcp_scaled_t));

   /* For |x| = |y| assume tan = 1 even if infinite (i.e. pretend momentarily
    * that ∞/∞ = 1) in order to comply with the rather artificial rules
    * inherited from IEEE 754-2008, namely:
    *
    *  "atan2(±∞, −∞) is ±3π/4
    *   atan2(±∞, +∞) is ±π/4"
    *
    * Note that this is inconsistent with the rules for the neighborhood of
    * zero that are based on iterated limits:
    *
    *  "atan2(±0, −0) is ±π
    *   atan2(±0, +0) is ±0"
    *
    * but GLSL specifically allows implementations to deviate from IEEE rules
    * at (0,0), so we take that license (i.e. pretend that 0/0 = 1 here as
    * well).
    */
   nir_def *tan = nir_bcsel(b, nir_feq(b, nir_fabs(b, x), nir_fabs(b, y)),
                            one, abs_s_over_t);

   /* Calculate the arctangent and fix up the result if we had flipped the
    * coordinate system.
    */
   nir_def *arc =
      nir_ffma_weak_imm1(b, nir_b2fN(b, flip, bit_size), M_PI_2, nir_atan(b, tan));

   /* Rather convoluted calculation of the sign of the result.  When x < 0 we
    * cannot use fsign because we need to be able to distinguish between
    * negative and positive zero.  We don't use bitwise arithmetic tricks for
    * consistency with the GLSL front-end.  When x >= 0 rcp_scaled_t will
    * always be non-negative so this won't be able to distinguish between
    * negative and positive zero, but we don't care because atan2 is
    * continuous along the whole positive y = 0 half-line, so it won't affect
    * the result significantly.
    */
   nir_def *result = nir_bcsel(b, nir_flt(b, nir_fmin(b, y, rcp_scaled_t), zero),
                               nir_fneg(b, arc), arc);

   b->fp_math_ctrl = old_fp_math_ctrl;
   return result;
}

nir_def *
nir_build_texture_query(nir_builder *b, nir_tex_instr *tex, nir_texop texop,
                        unsigned components, nir_alu_type dest_type,
                        bool include_coord, bool include_lod)
{
   nir_tex_instr *query;

   unsigned num_srcs = include_lod ? 1 : 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if ((tex->src[i].src_type == nir_tex_src_coord && include_coord) ||
          tex->src[i].src_type == nir_tex_src_texture_deref ||
          tex->src[i].src_type == nir_tex_src_sampler_deref ||
          tex->src[i].src_type == nir_tex_src_texture_offset ||
          tex->src[i].src_type == nir_tex_src_sampler_offset ||
          tex->src[i].src_type == nir_tex_src_texture_handle ||
          tex->src[i].src_type == nir_tex_src_sampler_handle)
         num_srcs++;
   }

   query = nir_tex_instr_create(b->shader, num_srcs);
   query->op = texop;
   query->sampler_dim = tex->sampler_dim;
   query->is_array = tex->is_array;
   query->is_shadow = tex->is_shadow;
   query->is_new_style_shadow = tex->is_new_style_shadow;
   query->texture_index = tex->texture_index;
   query->sampler_index = tex->sampler_index;
   query->can_speculate = tex->can_speculate;
   query->texture_non_uniform = tex->texture_non_uniform;
   query->sampler_non_uniform = tex->sampler_non_uniform;
   query->dest_type = dest_type;

   if (include_coord) {
      query->coord_components = tex->coord_components;
   }

   unsigned idx = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if ((tex->src[i].src_type == nir_tex_src_coord && include_coord) ||
          tex->src[i].src_type == nir_tex_src_texture_deref ||
          tex->src[i].src_type == nir_tex_src_sampler_deref ||
          tex->src[i].src_type == nir_tex_src_texture_offset ||
          tex->src[i].src_type == nir_tex_src_sampler_offset ||
          tex->src[i].src_type == nir_tex_src_texture_handle ||
          tex->src[i].src_type == nir_tex_src_sampler_handle) {
         query->src[idx].src = nir_src_for_ssa(tex->src[i].src.ssa);
         query->src[idx].src_type = tex->src[i].src_type;
         idx++;
      }
   }

   /* Add in an LOD because some back-ends require it */
   if (include_lod) {
      query->src[idx] = nir_tex_src_for_ssa(nir_tex_src_lod, nir_imm_int(b, 0));
   }

   nir_def_init(&query->instr, &query->def, nir_tex_instr_dest_size(query),
                nir_alu_type_get_type_size(dest_type));

   nir_builder_instr_insert(b, &query->instr);
   return &query->def;
}

nir_def *
nir_get_texture_size(nir_builder *b, nir_tex_instr *tex)
{
   b->cursor = nir_before_instr(&tex->instr);

   return nir_build_texture_query(b, tex, nir_texop_txs,
                                  nir_tex_instr_dest_size(tex),
                                  nir_type_int32, false, true);
}

nir_def *
nir_get_texture_lod(nir_builder *b, nir_tex_instr *tex)
{
   b->cursor = nir_before_instr(&tex->instr);

   nir_def *tql = nir_build_texture_query(b, tex, nir_texop_lod, 2,
                                          nir_type_float32, true, false);

   /* The LOD is the y component of the result */
   return nir_channel(b, tql, 1);
}
