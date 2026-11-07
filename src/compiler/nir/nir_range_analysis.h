/*
 * Copyright © 2018 Intel Corporation
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
#ifndef _NIR_RANGE_ANALYSIS_H_
#define _NIR_RANGE_ANALYSIS_H_

#include "nir.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
   nir_function_impl *impl;
   uint16_t *arr;
   BITSET_WORD *bitset;
   uint32_t size;
   int32_t max;
} nir_fp_analysis_state;

nir_fp_analysis_state nir_create_fp_analysis_state(nir_function_impl *impl);
void nir_invalidate_fp_analysis_state(nir_fp_analysis_state *state);
void nir_free_fp_analysis_state(nir_fp_analysis_state *state);

enum fp_class_bit {
   FP_CLASS_NEG_INF = BITFIELD_BIT(0),
   FP_CLASS_LT_NEG_ONE = BITFIELD_BIT(1),
   FP_CLASS_NEG_ONE = BITFIELD_BIT(2),
   FP_CLASS_LT_ZERO_GT_NEG_ONE = BITFIELD_BIT(3),
   FP_CLASS_NEG_ZERO = BITFIELD_BIT(4),
   FP_CLASS_POS_ZERO = BITFIELD_BIT(5),
   FP_CLASS_GT_ZERO_LT_POS_ONE = BITFIELD_BIT(6),
   FP_CLASS_POS_ONE = BITFIELD_BIT(7),
   FP_CLASS_GT_POS_ONE = BITFIELD_BIT(8),
   FP_CLASS_POS_INF = BITFIELD_BIT(9),
   FP_CLASS_NAN = BITFIELD_BIT(10),

   /**
    * A floating-point value that can have non integer (fractional) values.
    * Does not replace any of the values above.
    */
   FP_CLASS_NON_INTEGRAL = BITFIELD_BIT(11),
};

#define FP_CLASS_UNKNOWN        BITFIELD_MASK(12)
#define FP_CLASS_ANY_ZERO       (FP_CLASS_NEG_ZERO | FP_CLASS_POS_ZERO)
#define FP_CLASS_ANY_INF        (FP_CLASS_NEG_INF | FP_CLASS_POS_INF)
#define FP_CLASS_ANY_NEG_FINITE (FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE)
#define FP_CLASS_ANY_NEG        (FP_CLASS_ANY_NEG_FINITE | FP_CLASS_NEG_INF)
#define FP_CLASS_ANY_POS_FINITE (FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE)
#define FP_CLASS_ANY_POS        (FP_CLASS_ANY_POS_FINITE | FP_CLASS_POS_INF)
#define FP_CLASS_ANY_FINITE     (FP_CLASS_ANY_NEG_FINITE | FP_CLASS_ANY_ZERO | FP_CLASS_ANY_POS_FINITE)
#define FP_CLASS_ANY_NUMBER     (FP_CLASS_ANY_FINITE | FP_CLASS_ANY_INF)

typedef uint16_t fp_class_mask;

fp_class_mask nir_analyze_fp_class(nir_fp_analysis_state *state, const nir_def *def);

uint64_t nir_def_bits_used(nir_scalar def);

unsigned nir_def_num_lsb_zero(struct hash_table *numlsb_ht, nir_scalar def);

void
nir_gather_type_uses_of_float_def(nir_def *def,
                                  nir_component_mask_t *float_uses,
                                  nir_component_mask_t *integer_uses,
                                  uint64_t integer_bits_used[NIR_MAX_VEC_COMPONENTS],
                                  bool stop_on_float_use);

#ifdef __cplusplus
}
#endif
#endif /* _NIR_RANGE_ANALYSIS_H_ */
