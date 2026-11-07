/*
 * Copyright © 2016 Red Hat
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
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef _NIR_SEARCH_HELPERS_
#define _NIR_SEARCH_HELPERS_

#include <math.h>
#include "util/bitscan.h"
#include "util/u_math.h"
#include "util/half_float.h"
#include "nir.h"
#include "nir_range_analysis.h"
#include "nir_search.h"

static inline bool
is_pos_power_of_two(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                    unsigned src, unsigned num_components,
                    const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      nir_alu_type type = nir_op_infos[instr->op].input_types[src];
      switch (nir_alu_type_get_base_type(type)) {
      case nir_type_int: {
         int64_t val = nir_src_comp_as_int(instr->src[src].src, swizzle[i]);
         if (val <= 0 || !util_is_power_of_two_or_zero64(val))
            return false;
         break;
      }
      case nir_type_uint: {
         uint64_t val = nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);
         if (val == 0 || !util_is_power_of_two_or_zero64(val))
            return false;
         break;
      }
      default:
         return false;
      }
   }

   return true;
}

static inline bool
is_neg_power_of_two(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                    unsigned src, unsigned num_components,
                    const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   int64_t int_min = u_intN_min(instr->src[src].src.ssa->bit_size);

   for (unsigned i = 0; i < num_components; i++) {
      nir_alu_type type = nir_op_infos[instr->op].input_types[src];
      switch (nir_alu_type_get_base_type(type)) {
      case nir_type_int: {
         int64_t val = nir_src_comp_as_int(instr->src[src].src, swizzle[i]);
         /* "int_min" is a power-of-two, but negation can cause overflow. */
         if (val == int_min || val >= 0 || !util_is_power_of_two_or_zero64(-val))
            return false;
         break;
      }
      default:
         return false;
      }
   }

   return true;
}

static inline bool
is_bitcount2(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
             unsigned src, unsigned num_components,
             const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      uint64_t val = nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);
      if (util_bitcount64(val) != 2)
         return false;
   }

   return true;
}

static inline bool
is_nan(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
       unsigned src, unsigned num_components, const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      if (!isnan(nir_src_comp_as_float(instr->src[src].src, swizzle[i])))
         return false;
   }

   return true;
}

static inline bool
is_any_comp_nan(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                unsigned src, unsigned num_components, const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      if (isnan(nir_src_comp_as_float(instr->src[src].src, swizzle[i])))
         return true;
   }

   return false;
}

#define MULTIPLE(test)                                                       \
   static inline bool                                                        \
      is_unsigned_multiple_of_##test(const nir_search_state *state,          \
                                     const nir_alu_instr *instr,             \
                                     unsigned src, unsigned num_components,  \
                                     const uint8_t *swizzle)                 \
   {                                                                         \
      unsigned num = ffs(test) - 1;                                          \
      for (unsigned i = 0; i < num_components; i++) {                        \
         nir_scalar s = nir_get_scalar(instr->src[src].src.ssa, swizzle[i]); \
         if (nir_def_num_lsb_zero(state->numlsb_ht, s) < num)                \
            return false;                                                    \
      }                                                                      \
                                                                             \
      return true;                                                           \
   }

MULTIPLE(2)
MULTIPLE(4)
MULTIPLE(8)
MULTIPLE(16)
MULTIPLE(32)
MULTIPLE(64)

/**
 * x & 1 != 0
 */
static inline bool
is_odd(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
       unsigned src, unsigned num_components,
       const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      nir_alu_type type = nir_op_infos[instr->op].input_types[src];
      switch (nir_alu_type_get_base_type(type)) {
      case nir_type_int:
      case nir_type_uint: {
         if ((nir_src_comp_as_uint(instr->src[src].src, swizzle[i]) & 1) == 0)
            return false;
         break;
      }
      default:
         return false;
      }
   }

   return true;
}

static inline bool
is_not_const_zero(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                  unsigned src, unsigned num_components,
                  const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return true;

   for (unsigned i = 0; i < num_components; i++) {
      nir_alu_type type = nir_op_infos[instr->op].input_types[src];
      switch (nir_alu_type_get_base_type(type)) {
      case nir_type_float:
         if (nir_src_comp_as_float(instr->src[src].src, swizzle[i]) == 0.0)
            return false;
         break;
      case nir_type_bool:
      case nir_type_int:
      case nir_type_uint:
         if (nir_src_comp_as_uint(instr->src[src].src, swizzle[i]) == 0)
            return false;
         break;
      default:
         return false;
      }
   }

   return true;
}

/** Is value unsigned less than the limit? */
static inline bool
is_ult(const nir_alu_instr *instr, unsigned src, unsigned num_components, const uint8_t *swizzle,
       uint64_t limit)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const uint64_t val =
         nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);

      if (val >= limit)
         return false;
   }

   return true;
}

/** Is value unsigned less than 32? */
static inline bool
is_ult_32(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
          unsigned src, unsigned num_components,
          const uint8_t *swizzle)
{
   return is_ult(instr, src, num_components, swizzle, 32);
}

/** Is value unsigned less than 0xfffc07fc? */
static inline bool
is_ult_0xfffc07fc(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                  unsigned src, unsigned num_components,
                  const uint8_t *swizzle)
{
   return is_ult(instr, src, num_components, swizzle, 0xfffc07fcU);
}

/** Is the first 5 bits of value unsigned greater than or equal 2? */
static inline bool
is_first_5_bits_uge_2(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                      unsigned src, unsigned num_components,
                      const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const unsigned val =
         nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);

      if ((val & 0x1f) < 2)
         return false;
   }

   return true;
}

/**
 * Is this a constant that could be either int16_t or uint16_t after applying
 * a scale factor?
 */
static inline bool
is_16_bits_with_scale(const nir_alu_instr *instr,
                      unsigned src, unsigned num_components,
                      const uint8_t *swizzle, int scale)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   /* All elements must be representable as int16_t or uint16_t. */
   bool must_be_signed = false;
   bool must_be_unsigned = false;

   for (unsigned i = 0; i < num_components; i++) {
      const int64_t val =
         (uint64_t)(int64_t)scale * nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);

      if (val > 0xffff || val < -0x8000)
         return false;

      if (val < 0) {
         if (must_be_unsigned)
            return false;

         must_be_signed = true;
      }

      if (val > 0x7fff) {
         if (must_be_signed)
            return false;

         must_be_unsigned = true;
      }
   }

   return true;
}

/** Is this a constant that could be either int16_t or uint16_t? */
static inline bool
is_16_bits(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
           unsigned src, unsigned num_components,
           const uint8_t *swizzle)
{
   return is_16_bits_with_scale(instr, src, num_components, swizzle, 1);
}

/** Like is_16_bits, but could 2 times the constant fit in 16 bits? */
static inline bool
is_2x_16_bits(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
              unsigned src, unsigned num_components,
              const uint8_t *swizzle)
{
   return is_16_bits_with_scale(instr, src, num_components, swizzle, 2);
}

/** Like is_16_bits, but could -2 times the constant fit in 16 bits? */
static inline bool
is_neg2x_16_bits(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                 unsigned src, unsigned num_components,
                 const uint8_t *swizzle)
{
   return is_16_bits_with_scale(instr, src, num_components, swizzle, -2);
}

/** Is this a float constant that could fit in a half? */
static inline bool
is_representable_as_f16(UNUSED const nir_search_state *state,
                        const nir_alu_instr *instr,
                        unsigned src, unsigned num_components,
                        const uint8_t *swizzle)
{
   /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      double value = nir_src_comp_as_float(instr->src[src].src, swizzle[i]);
      if (!_mesa_float_is_half(value))
         return false;
   }
   return true;
}

static inline bool
is_not_const(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
             unsigned src, UNUSED unsigned num_components,
             UNUSED const uint8_t *swizzle)
{
   return !nir_src_is_const(instr->src[src].src);
}

static inline bool
is_not_fmul(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
            UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   nir_alu_instr *src_alu =
      nir_src_as_alu(instr->src[src].src);

   if (src_alu == NULL)
      return true;

   if (src_alu->op == nir_op_fneg)
      return is_not_fmul(state, src_alu, 0, 0, NULL);

   return src_alu->op != nir_op_fmul && src_alu->op != nir_op_fmulz && src_alu->op != nir_op_fmul_rtz;
}

static inline bool
is_fmul(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
        UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   nir_alu_instr *src_alu =
      nir_src_as_alu(instr->src[src].src);

   if (src_alu == NULL)
      return false;

   if (src_alu->op == nir_op_fneg)
      return is_fmul(state, src_alu, 0, 0, NULL);

   return src_alu->op == nir_op_fmul || src_alu->op == nir_op_fmulz || src_alu->op == nir_op_fmul_rtz;
}

static inline bool
is_fsign(const nir_alu_instr *instr, unsigned src,
         UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   nir_alu_instr *src_alu =
      nir_src_as_alu(instr->src[src].src);

   if (src_alu == NULL)
      return false;

   if (src_alu->op == nir_op_fneg)
      src_alu = nir_src_as_alu(src_alu->src[0].src);

   return src_alu != NULL && src_alu->op == nir_op_fsign;
}

static inline bool
is_not_const_and_not_fsign(const nir_search_state *state, const nir_alu_instr *instr,
                           unsigned src, unsigned num_components,
                           const uint8_t *swizzle)
{
   return is_not_const(state, instr, src, num_components, swizzle) &&
          !is_fsign(instr, src, num_components, swizzle);
}

static inline bool
has_multiple_uses(const nir_search_state *state, const nir_alu_instr *instr,
                  unsigned src, unsigned num_components,
                  const uint8_t *swizzle)
{
   return !list_is_empty(&instr->def.uses) &&
          !list_is_singular(&instr->def.uses);
}

static inline bool
is_used_once(const nir_alu_instr *instr)
{
   return list_is_singular(&instr->def.uses);
}

static inline bool
is_used_by_if(const nir_alu_instr *instr)
{
   return nir_def_used_by_if(&instr->def);
}

static inline bool
is_not_used_by_if(const nir_alu_instr *instr)
{
   return !is_used_by_if(instr);
}

static inline bool
is_only_used_by_if(const nir_alu_instr *instr)
{
   return nir_def_only_used_by_if(&instr->def);
}

static inline bool
is_used_by_non_fsat(const nir_alu_instr *instr)
{
   nir_foreach_use(src, &instr->def) {
      const nir_instr *const user_instr = nir_src_use_instr(src);

      if (user_instr->type != nir_instr_type_alu)
         return true;

      const nir_alu_instr *const user_alu = nir_instr_as_alu(user_instr);

      assert(instr != user_alu);
      if (user_alu->op != nir_op_fsat)
         return true;
   }

   return false;
}

static inline bool
is_used_by_non_ldc_nv(const nir_alu_instr *instr)
{
   nir_foreach_use(src, &instr->def) {
      const nir_instr *const user_instr = nir_src_use_instr(src);

      if (user_instr->type != nir_instr_type_intrinsic)
         return true;

      const nir_intrinsic_instr *const user_intrin = nir_instr_as_intrinsic(user_instr);

      if (user_intrin->intrinsic != nir_intrinsic_ldc_nv)
         return true;
   }

   return false;
}

static inline bool
is_only_used_as_float_impl(const nir_alu_instr *instr, bool nsz, unsigned depth)
{
   nir_foreach_use(src, &instr->def) {
      const nir_instr *const user_instr = nir_src_use_instr(src);

      if (user_instr->type != nir_instr_type_alu) {
         if (user_instr->type == nir_instr_type_intrinsic) {
            switch (nir_instr_as_intrinsic(user_instr)->intrinsic) {
            case nir_intrinsic_ddx:
            case nir_intrinsic_ddy:
            case nir_intrinsic_ddx_fine:
            case nir_intrinsic_ddy_fine:
            case nir_intrinsic_ddx_coarse:
            case nir_intrinsic_ddy_coarse:
               if (nsz)
                  return false;
               continue;
            default:
               break;
            }
         } else if (user_instr->type == nir_instr_type_tex) {
            const nir_tex_instr *tex = nir_instr_as_tex(user_instr);
            const nir_tex_src *tex_src = container_of(src, nir_tex_src, src);

            /* These have unknown type. */
            if (tex_src->src_type == nir_tex_src_backend1 ||
                tex_src->src_type == nir_tex_src_backend2)
               return false;

            unsigned idx = tex_src - tex->src;
            /* Float tex sources don't care about signed zeros. */
            if (nir_tex_instr_src_type(tex, idx) == nir_type_float)
               continue;
         }
         return false;
      }

      const nir_alu_instr *const user_alu = nir_instr_as_alu(user_instr);
      assert(instr != user_alu);

      unsigned index = (nir_alu_src *)container_of(src, nir_alu_src, src) - user_alu->src;

      /* bcsel acts like a move: if the bcsel is only used by float
       * instructions, then the original value is (transitively) only used by
       * float too.
       *
       * The unbounded recursion would terminate because use chains are acyclic
       * in SSA. However, we limit the search depth regardless to avoid stack
       * overflows in patholgical shaders and to reduce the worst-case time.
       */
      bool is_mov = (user_alu->op == nir_op_bcsel && index != 0) ||
                    nir_op_is_vec_or_mov(user_alu->op);
      if (is_mov && depth < 8) {
         if (is_only_used_as_float_impl(user_alu, nsz, depth + 1))
            continue;
      }

      nir_alu_type type = nir_op_infos[user_alu->op].input_types[index];
      if (nir_alu_type_get_base_type(type) != nir_type_float)
         return false;
      if (nir_alu_instr_is_signed_zero_preserve(user_alu) && nsz)
         return false;
   }

   return true;
}

static inline bool
is_only_used_as_float(const nir_alu_instr *instr)
{
   return is_only_used_as_float_impl(instr, false, 0);
}

static inline bool
is_only_used_as_float_nsz(const nir_alu_instr *instr)
{
   return is_only_used_as_float_impl(instr, true, 0);
}

static inline bool
is_only_used_by_fadd(const nir_alu_instr *instr)
{
   nir_foreach_use(src, &instr->def) {
      const nir_instr *const user_instr = nir_src_use_instr(src);
      if (user_instr->type != nir_instr_type_alu)
         return false;

      const nir_alu_instr *const user_alu = nir_instr_as_alu(user_instr);
      assert(instr != user_alu);

      if (user_alu->op == nir_op_fneg || user_alu->op == nir_op_fabs) {
         if (!is_only_used_by_fadd(user_alu))
            return false;
      } else if (user_alu->op != nir_op_fadd) {
         return false;
      }
   }

   return true;
}

static inline bool
is_not_only_used_by_fadd(const nir_alu_instr *instr)
{
   return !is_only_used_by_fadd(instr);
}

static inline bool
is_only_used_by_alu_op(const nir_alu_instr *instr, nir_op op)
{
   nir_foreach_use(src, &instr->def) {
      const nir_instr *const user_instr = nir_src_use_instr(src);
      if (user_instr->type != nir_instr_type_alu)
         return false;

      const nir_alu_instr *const user_alu = nir_instr_as_alu(user_instr);
      assert(instr != user_alu);

      if (user_alu->op != op)
         return false;
   }

   return true;
}

static inline bool
is_only_used_by_iadd(const nir_alu_instr *instr)
{
   return is_only_used_by_alu_op(instr, nir_op_iadd);
}

static inline bool
is_only_used_by_iand(const nir_alu_instr *instr)
{
   return is_only_used_by_alu_op(instr, nir_op_iand);
}

static inline bool
is_only_used_by_ior(const nir_alu_instr *instr)
{
   return is_only_used_by_alu_op(instr, nir_op_ior);
}

static inline bool
only_lower_8_bits_used(const nir_alu_instr *instr)
{
   if (instr->def.num_components > 1)
      return false;

   return (nir_def_bits_used(nir_get_scalar((nir_def*)&instr->def, 0)) &
           ~0xffull) == 0;
}

static inline bool
only_lower_16_bits_used(const nir_alu_instr *instr)
{
   if (instr->def.num_components > 1)
      return false;

   return (nir_def_bits_used(nir_get_scalar((nir_def*)&instr->def, 0)) &
           ~0xffffull) == 0;
}

/**
 * Returns true if a NIR ALU src represents a constant integer
 * of either 32 or 64 bits, and the higher word (bit-size / 2)
 * of all its components is zero.
 */
static inline bool
is_upper_half_zero(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                   unsigned src, unsigned num_components,
                   const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      unsigned half_bit_size = nir_src_bit_size(instr->src[src].src) / 2;
      uint64_t high_bits = u_bit_consecutive64(half_bit_size, half_bit_size);
      if ((nir_src_comp_as_uint(instr->src[src].src,
                                swizzle[i]) &
           high_bits) != 0) {
         return false;
      }
   }

   return true;
}

/**
 * Returns true if a NIR ALU src represents a constant integer
 * of either 32 or 64 bits, and the lower word (bit-size / 2)
 * of all its components is zero.
 */
static inline bool
is_lower_half_zero(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                   unsigned src, unsigned num_components,
                   const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      uint64_t low_bits = BITFIELD64_MASK(nir_src_bit_size(instr->src[src].src) / 2);
      if ((nir_src_comp_as_uint(instr->src[src].src, swizzle[i]) & low_bits) != 0)
         return false;
   }

   return true;
}

static inline bool
is_upper_half_negative_one(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                           unsigned src, unsigned num_components,
                           const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      unsigned half_bit_size = nir_src_bit_size(instr->src[src].src) / 2;
      uint64_t high_bits = u_bit_consecutive64(half_bit_size, half_bit_size);
      if ((nir_src_comp_as_uint(instr->src[src].src,
                                swizzle[i]) &
           high_bits) != high_bits) {
         return false;
      }
   }

   return true;
}

static inline bool
is_lower_half_negative_one(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                           unsigned src, unsigned num_components,
                           const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      uint64_t low_bits = BITFIELD64_MASK(nir_src_bit_size(instr->src[src].src) / 2);
      if ((nir_src_comp_as_uint(instr->src[src].src, swizzle[i]) & low_bits) != low_bits)
         return false;
   }

   return true;
}

/**
 * Returns whether an operand is a constant bit-mask, meaning that it
 * only has consecutive 1 bits starting from the LSB.
 * Numbers whose MSB is 1 are excluded because they are not useful
 * for the optimizations where this function is used.
 */
static inline bool
is_const_bitmask(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                 unsigned src, unsigned num_components,
                 const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const unsigned bit_size = instr->src[src].src.ssa->bit_size;
      const uint64_t c = nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);
      const unsigned num_bits = util_bitcount64(c);
      if (c != BITFIELD64_MASK(num_bits) || num_bits == bit_size)
         return false;
   }

   return true;
}

/**
 * Returns whether an operand is a non zero constant
 * that can be created by nir_op_bfm.
 */
static inline bool
is_const_bfm(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                 unsigned src, unsigned num_components,
                 const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const unsigned bit_size = instr->src[src].src.ssa->bit_size;
      const uint64_t c = nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);
      const unsigned num_bits = util_bitcount64(c);
      const unsigned offset = ffsll(c) - 1;
      if (c == 0 || c != (BITFIELD64_MASK(num_bits) << offset)  || num_bits == bit_size)
         return false;
   }

   return true;
}

/**
 * Returns whether the 5 LSBs of an operand are non-zero.
 */
static inline bool
is_5lsb_not_zero(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                 unsigned src, unsigned num_components,
                 const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const uint64_t c = nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);
      if ((c & 0x1f) == 0)
         return false;
   }

   return true;
}

/**
 * Returns whether at least one bit is 0.
 */
static inline bool
is_not_uint_max(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                unsigned src, unsigned num_components,
                const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const int64_t c = nir_src_comp_as_int(instr->src[src].src, swizzle[i]);
      if (c == -1)
         return false;
   }

   return true;
}

/**
 * Returns whether at least one bit is 1.
 */
static inline bool
is_not_uint_zero(UNUSED const nir_search_state *state, const nir_alu_instr *instr,
                unsigned src, unsigned num_components,
                const uint8_t *swizzle)
{
   if (nir_src_as_const_value(instr->src[src].src) == NULL)
      return false;

   for (unsigned i = 0; i < num_components; i++) {
      const uint64_t c = nir_src_comp_as_uint(instr->src[src].src, swizzle[i]);
      if (c == 0)
         return false;
   }

   return true;
}

static inline bool
no_signed_wrap(const nir_alu_instr *instr)
{
   return instr->no_signed_wrap;
}

static inline bool
no_unsigned_wrap(const nir_alu_instr *instr)
{
   return instr->no_unsigned_wrap;
}

static inline bool
xz_components_unused(const nir_alu_instr *instr)
{
   return (nir_def_components_read(&instr->def) & 0x5) == 0;
}

#define RELATION(r, _exclude)                                                                        \
   static inline bool                                                                                \
   is_##r(const nir_search_state *state, const nir_alu_instr *instr,                                 \
          unsigned src, UNUSED unsigned num_components,                                              \
          UNUSED const uint8_t *swizzle)                                                             \
   {                                                                                                 \
      fp_class_mask exclude = _exclude;                                                              \
      if (exclude & (FP_CLASS_ANY_INF | FP_CLASS_NAN)) {                                             \
         /* fp_math_ctrl lets us assume inputs are not NaN/Inf for float sources. */                 \
         const nir_op_info *op_info = &nir_op_infos[(int)instr->op];                                 \
         nir_alu_type base_type = nir_alu_type_get_base_type(op_info->input_types[src]);             \
         if (base_type == nir_type_float) {                                                          \
            if (!nir_alu_instr_is_inf_preserve(instr))                                               \
               exclude &= ~FP_CLASS_ANY_INF;                                                         \
            if (!nir_alu_instr_is_nan_preserve(instr))                                               \
               exclude &= ~FP_CLASS_NAN;                                                             \
            if (!exclude)                                                                            \
               return true;                                                                          \
         }                                                                                           \
      }                                                                                              \
      const fp_class_mask fp_class = nir_analyze_fp_class(state->range_ht, instr->src[src].src.ssa); \
      return (fp_class & exclude) == 0;                                                              \
   }

#define RELATION_AND_NUM(r, exclude) \
   RELATION(r, exclude)              \
   RELATION(a_number_##r, (exclude) | FP_CLASS_NAN)

RELATION_AND_NUM(lt_zero, FP_CLASS_ANY_POS | FP_CLASS_ANY_ZERO)
RELATION_AND_NUM(not_positive, FP_CLASS_ANY_POS)
RELATION_AND_NUM(gt_zero, FP_CLASS_ANY_NEG | FP_CLASS_ANY_ZERO)
RELATION_AND_NUM(not_negative, FP_CLASS_ANY_NEG)
RELATION_AND_NUM(not_negative_or_negative_zero, FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO)
RELATION_AND_NUM(not_zero, FP_CLASS_ANY_ZERO)
RELATION_AND_NUM(zero_to_one, FP_CLASS_ANY_NEG | FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF)
RELATION_AND_NUM(le_pos_one, FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF)
RELATION_AND_NUM(gt_0_and_lt_1, FP_CLASS_ANY_NEG | FP_CLASS_ANY_ZERO | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF)
RELATION_AND_NUM(ge_pos_one, FP_CLASS_ANY_NEG | FP_CLASS_ANY_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE)
RELATION(a_number, FP_CLASS_NAN)
RELATION(finite, FP_CLASS_ANY_INF | FP_CLASS_NAN)
RELATION(finite_not_zero, FP_CLASS_ANY_INF | FP_CLASS_NAN | FP_CLASS_ANY_ZERO)
RELATION(integral, FP_CLASS_NON_INTEGRAL)
RELATION(integral_not_negative, FP_CLASS_ANY_NEG | FP_CLASS_NON_INTEGRAL)

static inline bool
compare_component(const nir_alu_instr *instr, unsigned src, unsigned component,
                  float f)
{
   nir_scalar comp = nir_scalar_resolved(instr->src[src].src.ssa, component);
   return nir_scalar_is_const(comp) && nir_scalar_as_float(comp) == f;
}

static inline bool
x_is_zero(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
          UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   return compare_component(instr, src, swizzle[0], 0.0);
}

static inline bool
y_is_zero(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
          UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   return compare_component(instr, src, swizzle[1], 0.0);
}

static inline bool
z_is_zero(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
          UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   return compare_component(instr, src, swizzle[2], 0.0);
}

static inline bool
w_is_zero(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
          UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   return compare_component(instr, src, swizzle[3], 0.0);
}

static inline bool
can_add_output_modifier(const nir_search_state *state,
                        const nir_alu_instr *instr,
                        unsigned src,
                        UNUSED unsigned num_components,
                        UNUSED const uint8_t *swizzle)
{
   nir_alu_instr *src_alu = nir_src_as_alu(instr->src[src].src);

   if (src_alu == NULL)
      return false;

   if (!list_is_singular(&src_alu->def.uses))
      return false;

   nir_alu_type output_type = nir_op_infos[src_alu->op].output_type;
   return nir_alu_type_get_base_type(output_type) == nir_type_float;
}

static inline bool
cannot_add_output_modifier(const nir_search_state *state,
                           const nir_alu_instr *instr,
                           unsigned src,
                           unsigned num_components,
                           const uint8_t *swizzle)
{
   return !can_add_output_modifier(state, instr, src, num_components, swizzle);
}


static inline bool
is_created_as_float(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
                    UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   nir_alu_instr *src_alu = nir_src_as_alu(instr->src[src].src);

   if (src_alu == NULL)
      return false;

   nir_alu_type output_type = nir_op_infos[src_alu->op].output_type;
   return nir_alu_type_get_base_type(output_type) == nir_type_float;
}

static inline bool
is_undef(const nir_search_state *state, const nir_alu_instr *instr, unsigned src,
         UNUSED unsigned num_components, UNUSED const uint8_t *swizzle)
{
   return nir_src_is_undef(instr->src[src].src);
}

#endif /* _NIR_SEARCH_ */
