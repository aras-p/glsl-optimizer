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

#include "nir_range_analysis.h"
#include <float.h>
#include <math.h>
#include "util/hash_table.h"
#include "util/u_dynarray.h"
#include "util/u_math.h"
#include "c99_alloca.h"

/**
 * Analyzes a sequence of operations to determine some aspects of the range of
 * the result.
 */

struct analysis_query {
   uint32_t pushed_queries;
   uint32_t result_index;
};

struct analysis_state {
   nir_shader *shader;
   void *range_ht;

   struct util_dynarray query_stack;
   struct util_dynarray result_stack;

   size_t query_size;
   uint32_t (*get_key)(struct analysis_query *q);
   bool (*lookup)(void *table, uint32_t key, uint32_t *value);
   void (*insert)(void *table, uint32_t key, uint32_t value);
   void (*process_query)(struct analysis_state *state, struct analysis_query *q,
                         uint32_t *result, const uint32_t *src);
};

static void *
push_analysis_query(struct analysis_state *state, size_t size)
{
   struct analysis_query *q = util_dynarray_grow_bytes(&state->query_stack, 1, size);
   q->pushed_queries = 0;
   q->result_index = util_dynarray_num_elements(&state->result_stack, uint32_t);

   util_dynarray_append_typed(&state->result_stack, uint32_t, 0);

   return q;
}

/* Helper for performing range analysis without recursion. */
static uint32_t
perform_analysis(struct analysis_state *state)
{
   while (state->query_stack.size) {
      struct analysis_query *cur =
         (struct analysis_query *)((char *)util_dynarray_end(&state->query_stack) - state->query_size);
      uint32_t *result = util_dynarray_element(&state->result_stack, uint32_t, cur->result_index);

      uint32_t key = state->get_key(cur);
      /* There might be a cycle-resolving entry for loop header phis. Ignore this when finishing
       * them by testing pushed_queries.
       */
      if (cur->pushed_queries == 0 && key != UINT32_MAX &&
          state->lookup(state->range_ht, key, result)) {
         state->query_stack.size -= state->query_size;
         continue;
      }

      uint32_t *src = (uint32_t *)util_dynarray_end(&state->result_stack) - cur->pushed_queries;
      state->result_stack.size -= sizeof(uint32_t) * cur->pushed_queries;

      uint32_t prev_num_queries = state->query_stack.size;
      state->process_query(state, cur, result, src);

      uint32_t num_queries = state->query_stack.size;
      if (num_queries > prev_num_queries) {
         cur = (struct analysis_query *)util_dynarray_element(&state->query_stack, char,
                                                              prev_num_queries - state->query_size);
         cur->pushed_queries = (num_queries - prev_num_queries) / state->query_size;
         continue;
      }

      if (key != UINT32_MAX)
         state->insert(state->range_ht, key, *result);

      state->query_stack.size -= state->query_size;
   }

   assert(state->result_stack.size == sizeof(uint32_t));

   uint32_t res = util_dynarray_top(&state->result_stack, uint32_t);
   util_dynarray_fini(&state->query_stack);
   util_dynarray_fini(&state->result_stack);

   return res;
}

static fp_class_mask
analyze_fp_constant(const nir_load_const_instr *const load)
{
   fp_class_mask result = 0;

   for (unsigned i = 0; i < load->def.num_components; ++i) {
      const double v = nir_const_value_as_float(load->value[i],
                                                load->def.bit_size);

      if (!isnan(v) && floor(v) != v)
         result |= FP_CLASS_NON_INTEGRAL;

      if (isnan(v))
         result |= FP_CLASS_NAN;
      else if (v == -INFINITY)
         result |= FP_CLASS_NEG_INF;
      else if (v < -1.0)
         result |= FP_CLASS_LT_NEG_ONE;
      else if (v == -1.0)
         result |= FP_CLASS_NEG_ONE;
      else if (v < 0.0)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE;
      else if (dui(v) == 0)
         result |= FP_CLASS_POS_ZERO;
      else if (v == 0.0)
         result |= FP_CLASS_NEG_ZERO;
      else if (v < 1.0)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE;
      else if (v == 1.0)
         result |= FP_CLASS_POS_ONE;
      else if (v < INFINITY)
         result |= FP_CLASS_GT_POS_ONE;
      else
         result |= FP_CLASS_POS_INF;

      if (v != 0) {
         /* handle potential denorm flushing. */
         bool is_denorm = false;
         switch (load->def.bit_size) {
         case 64:
            is_denorm = fabs(v) < DBL_MIN;
            break;
         case 32:
            is_denorm = fabs(v) < FLT_MIN;
            break;
         case 16:
            is_denorm = fabs(v) < ldexp(1.0, -14);
            break;
         default:
            UNREACHABLE("unsupported float size");
         }
         if (is_denorm)
            result |= v < 0.0 ? FP_CLASS_NEG_ZERO : FP_CLASS_POS_ZERO;
      }
   }

   return result;
}

struct fp_query {
   struct analysis_query head;
   const nir_def *def;
};

static void
push_fp_query(struct analysis_state *state, const nir_def *def)
{
   struct fp_query *pushed_q = push_analysis_query(state, sizeof(struct fp_query));
   pushed_q->def = def;
}

static uint32_t
get_fp_key(struct analysis_query *q)
{
   return ((struct fp_query *)q)->def->index;
}

static bool
fp_lookup(void *table, uint32_t key, uint32_t *value)
{
   nir_fp_analysis_state *state = table;
   if (BITSET_TEST(state->bitset, key)) {
      *value = state->arr[key];
      return true;
   } else {
      return false;
   }
}

static void
fp_insert(void *table, uint32_t key, uint32_t value)
{
   nir_fp_analysis_state *state = table;
   BITSET_SET(state->bitset, key);
   state->max = MAX2(state->max, (int)key);
   state->arr[key] = value;
}

static fp_class_mask
fneg_fp_class(fp_class_mask src)
{
   fp_class_mask result = src & (FP_CLASS_NAN | FP_CLASS_NON_INTEGRAL);

#define NEG_BIT(neg, pos)       \
   if (src & FP_CLASS_##pos)    \
      result |= FP_CLASS_##neg; \
   if (src & FP_CLASS_##neg)    \
      result |= FP_CLASS_##pos;

   NEG_BIT(NEG_INF, POS_INF);
   NEG_BIT(LT_NEG_ONE, GT_POS_ONE);
   NEG_BIT(NEG_ONE, POS_ONE);
   NEG_BIT(LT_ZERO_GT_NEG_ONE, GT_ZERO_LT_POS_ONE);
   NEG_BIT(NEG_ZERO, POS_ZERO);

#undef NEG_BIT

   return result;
}

static fp_class_mask
fmul_fp_class(fp_class_mask left, fp_class_mask right, bool mulz, bool src_eq, bool src_neg_eq)
{
   /* For runtime performance, shortcut the common completely unknown case. */
   if (left == FP_CLASS_UNKNOWN && right == FP_CLASS_UNKNOWN && !src_eq && !src_neg_eq)
      return FP_CLASS_UNKNOWN;

   fp_class_mask result = 0;

   if (left & FP_CLASS_NAN) {
      if (right & FP_CLASS_ANY_ZERO)
         result |= mulz ? FP_CLASS_POS_ZERO : FP_CLASS_NAN;

      if (right & (FP_CLASS_ANY_NEG | FP_CLASS_ANY_POS | FP_CLASS_NAN))
         result |= FP_CLASS_NAN;
   }

   if (left & FP_CLASS_NEG_INF) {
      if (right & FP_CLASS_ANY_ZERO)
         result |= mulz ? FP_CLASS_POS_ZERO : FP_CLASS_NAN;

      if (right & FP_CLASS_ANY_NEG)
         result |= FP_CLASS_POS_INF;

      if (right & FP_CLASS_ANY_POS)
         result |= FP_CLASS_NEG_INF;

      if (right & FP_CLASS_NAN)
         result |= FP_CLASS_NAN;
   }

   if (left & FP_CLASS_POS_INF) {
      if (right & FP_CLASS_ANY_ZERO)
         result |= mulz ? FP_CLASS_POS_ZERO : FP_CLASS_NAN;

      if (right & FP_CLASS_ANY_POS)
         result |= FP_CLASS_POS_INF;

      if (right & FP_CLASS_ANY_NEG)
         result |= FP_CLASS_NEG_INF;

      if (right & FP_CLASS_NAN)
         result |= FP_CLASS_NAN;
   }

   if (left & FP_CLASS_ANY_NEG_FINITE) {
      if (right & FP_CLASS_POS_ZERO)
         result |= mulz ? FP_CLASS_POS_ZERO : FP_CLASS_NEG_ZERO;

      result |= fneg_fp_class(right & (FP_CLASS_NEG_ZERO | FP_CLASS_POS_INF | FP_CLASS_NEG_INF | FP_CLASS_NAN));
   }

   if (left & FP_CLASS_ANY_POS_FINITE) {
      if (right & FP_CLASS_NEG_ZERO)
         result |= mulz ? FP_CLASS_POS_ZERO : FP_CLASS_NEG_ZERO;

      result |= right & (FP_CLASS_POS_ZERO | FP_CLASS_POS_INF | FP_CLASS_NEG_INF | FP_CLASS_NAN);
   }

   if (left & FP_CLASS_LT_NEG_ONE) {
      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_POS_INF | FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_NEG_INF | FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE;
   }

   if (left & FP_CLASS_GT_POS_ONE) {
      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_POS_INF | FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_NEG_INF | FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE;
   }

   if (left & FP_CLASS_NEG_ONE)
      result |= fneg_fp_class(right & ~FP_CLASS_ANY_ZERO);

   if (left & FP_CLASS_POS_ONE)
      result |= right & ~FP_CLASS_ANY_ZERO;

   if (left & FP_CLASS_LT_ZERO_GT_NEG_ONE) {
      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_POS_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_NEG_ZERO | FP_CLASS_LT_ZERO_GT_NEG_ONE;
   }

   if (left & FP_CLASS_GT_ZERO_LT_POS_ONE) {
      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_POS_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_NEG_ZERO | FP_CLASS_LT_ZERO_GT_NEG_ONE;
   }

   if (left & FP_CLASS_NEG_ZERO) {
      if (mulz) {
         result |= FP_CLASS_POS_ZERO;
      } else {
         if (right & (FP_CLASS_ANY_INF | FP_CLASS_NAN))
            result |= FP_CLASS_NAN;

         if (right & (FP_CLASS_ANY_NEG_FINITE | FP_CLASS_NEG_ZERO))
            result |= FP_CLASS_POS_ZERO;

         if (right & (FP_CLASS_ANY_POS_FINITE | FP_CLASS_POS_ZERO))
            result |= FP_CLASS_NEG_ZERO;
      }
   }

   if (left & FP_CLASS_POS_ZERO) {
      if (mulz) {
         result |= FP_CLASS_POS_ZERO;
      } else {
         if (right & (FP_CLASS_ANY_INF | FP_CLASS_NAN))
            result |= FP_CLASS_NAN;

         if (right & (FP_CLASS_ANY_POS_FINITE | FP_CLASS_POS_ZERO))
            result |= FP_CLASS_POS_ZERO;

         if (right & (FP_CLASS_ANY_NEG_FINITE | FP_CLASS_NEG_ZERO))
            result |= FP_CLASS_NEG_ZERO;
      }
   }

   if (src_eq || src_neg_eq) {
      /* This case can't create new ones. */
      if (!(left & (FP_CLASS_POS_ONE | FP_CLASS_NEG_ONE)))
         result &= ~(FP_CLASS_POS_ONE | FP_CLASS_NEG_ONE);

      if (src_eq)
         result &= ~(FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO);
      else if (src_neg_eq && mulz)
         result &= ~FP_CLASS_ANY_POS;
      else if (src_neg_eq)
         result &= ~(FP_CLASS_ANY_POS | FP_CLASS_POS_ZERO);
   }

   if ((left | right) & FP_CLASS_NON_INTEGRAL) {
      if (result & (FP_CLASS_LT_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE |
                    FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_GT_POS_ONE))
         result |= FP_CLASS_NON_INTEGRAL;
   } else {
      result &= ~(FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE);
   }

   return result;
}

static fp_class_mask
fadd_fp_class(fp_class_mask left, fp_class_mask right)
{
   /* For runtime performance, shortcut the common completely unknown case. */
   if (left == FP_CLASS_UNKNOWN && right == FP_CLASS_UNKNOWN)
      return FP_CLASS_UNKNOWN;

   fp_class_mask result = (left | right) & FP_CLASS_NAN;
   /* X + Y is NaN if either operand is NaN or if one operand is +Inf and
    * the other is -Inf.
    */
   if (left & FP_CLASS_NEG_INF) {
      if (right & FP_CLASS_POS_INF)
         result |= FP_CLASS_NAN;
      if (right & (FP_CLASS_ANY_FINITE | FP_CLASS_NEG_INF))
         result |= FP_CLASS_NEG_INF;
   }

   if (left & FP_CLASS_POS_INF) {
      if (right & FP_CLASS_NEG_INF)
         result |= FP_CLASS_NAN;
      if (right & (FP_CLASS_ANY_FINITE | FP_CLASS_POS_INF))
         result |= FP_CLASS_POS_INF;
   }

   if (left & FP_CLASS_LT_NEG_ONE) {
      result |= (right & FP_CLASS_ANY_INF);

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_NEG_INF | FP_CLASS_LT_NEG_ONE;

      if (right & (FP_CLASS_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_ANY_ZERO))
         result |= FP_CLASS_LT_NEG_ONE;

      if (right & (FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE))
         result |= FP_CLASS_LT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_ANY_FINITE;
   }

   if (left & FP_CLASS_GT_POS_ONE) {
      result |= (right & FP_CLASS_ANY_INF);

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_POS_INF | FP_CLASS_GT_POS_ONE;

      if (right & (FP_CLASS_POS_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_ANY_ZERO))
         result |= FP_CLASS_GT_POS_ONE;

      if (right & (FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE))
         result |= FP_CLASS_GT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_ANY_FINITE;
   }

   if (left & FP_CLASS_NEG_ONE) {
      result |= (right & FP_CLASS_ANY_INF);

      if (right & (FP_CLASS_LT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE))
         result |= FP_CLASS_LT_NEG_ONE;

      if (right & (FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE))
         result |= FP_CLASS_NEG_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_POS_ZERO;

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE;
   }

   if (left & FP_CLASS_POS_ONE) {
      result |= (right & FP_CLASS_ANY_INF);

      if (right & (FP_CLASS_GT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE))
         result |= FP_CLASS_GT_POS_ONE;

      if (right & (FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_LT_ZERO_GT_NEG_ONE))
         result |= FP_CLASS_POS_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_POS_ZERO;

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE;
   }

   if (left & FP_CLASS_LT_ZERO_GT_NEG_ONE) {
      result |= (right & FP_CLASS_ANY_INF);

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_LT_NEG_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_LT_NEG_ONE | FP_CLASS_NEG_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_LT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_ANY_ZERO)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE;

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE;
   }

   if (left & FP_CLASS_GT_ZERO_LT_POS_ONE) {
      result |= (right & FP_CLASS_ANY_INF);

      if (right & FP_CLASS_GT_POS_ONE)
         result |= FP_CLASS_GT_POS_ONE;

      if (right & FP_CLASS_POS_ONE)
         result |= FP_CLASS_GT_POS_ONE | FP_CLASS_POS_ONE;

      if (right & FP_CLASS_GT_ZERO_LT_POS_ONE)
         result |= FP_CLASS_GT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_ANY_ZERO)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE;

      if (right & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_LT_ZERO_GT_NEG_ONE;

      if (right & FP_CLASS_NEG_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE;

      if (right & FP_CLASS_LT_NEG_ONE)
         result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_LT_NEG_ONE;
   }

   if (left & FP_CLASS_NEG_ZERO) {
      result |= right;
   }

   if (left & FP_CLASS_POS_ZERO) {
      result |= right & ~FP_CLASS_NEG_ZERO;
      if (right & FP_CLASS_NEG_ZERO)
         result |= FP_CLASS_POS_ZERO;
   }

   if ((left | right) & FP_CLASS_NON_INTEGRAL) {
      if (result & (FP_CLASS_LT_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE |
                    FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_GT_POS_ONE))
         result |= FP_CLASS_NON_INTEGRAL;
   } else {
      result &= ~(FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE);
   }

   return result;
}

static fp_class_mask
frcp_fp_class(fp_class_mask src)
{
   fp_class_mask result = src & FP_CLASS_NAN;

   /* Inf/Zero result in Zero/Inf.*/
   if (src & FP_CLASS_NEG_INF)
      result |= FP_CLASS_NEG_ZERO;
   if (src & FP_CLASS_POS_INF)
      result |= FP_CLASS_POS_ZERO;
   if (src & FP_CLASS_NEG_ZERO)
      result |= FP_CLASS_NEG_INF;
   if (src & FP_CLASS_POS_ZERO)
      result |= FP_CLASS_POS_INF;

   /* One results in one. */
   if (src & FP_CLASS_NEG_ONE)
      result |= FP_CLASS_NEG_ONE;
   if (src & FP_CLASS_POS_ONE)
      result |= FP_CLASS_POS_ONE;

   if (src & FP_CLASS_LT_NEG_ONE)
      result |= FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_NEG_ZERO | FP_CLASS_NON_INTEGRAL;
   if (src & FP_CLASS_GT_POS_ONE)
      result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ZERO | FP_CLASS_NON_INTEGRAL;

   if (src & FP_CLASS_LT_ZERO_GT_NEG_ONE)
      result |= FP_CLASS_LT_NEG_ONE | FP_CLASS_NEG_INF | FP_CLASS_NON_INTEGRAL;
   if (src & FP_CLASS_GT_ZERO_LT_POS_ONE)
      result |= FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF | FP_CLASS_NON_INTEGRAL;

   return result;
}

static fp_class_mask
fsqrt_fp_class(fp_class_mask src)
{
   fp_class_mask result = src & (FP_CLASS_NAN | FP_CLASS_ANY_ZERO);

   if (src & FP_CLASS_ANY_NEG)
      result |= FP_CLASS_NAN;

   if (src & FP_CLASS_GT_ZERO_LT_POS_ONE)
      result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_NON_INTEGRAL;
   if (src & FP_CLASS_POS_ONE)
      result |= FP_CLASS_POS_ONE;
   if (src & FP_CLASS_GT_POS_ONE)
      result |= FP_CLASS_GT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_NON_INTEGRAL;
   if (src & FP_CLASS_POS_INF)
      result |= FP_CLASS_POS_INF;

   return result;
}

static fp_class_mask
fmin_part(fp_class_mask upper_bound, fp_class_mask value)
{
   /* Find the highest value in upper_bound, and return all
    * smaller or equal values from value.
    */
   upper_bound &= FP_CLASS_ANY_NEG | FP_CLASS_ANY_ZERO | FP_CLASS_ANY_POS;
   value &= FP_CLASS_ANY_NEG | FP_CLASS_ANY_ZERO | FP_CLASS_ANY_POS;

   /* This works even in the case where upper_bound is 0 */
   return value & BITFIELD_MASK(util_last_bit(upper_bound));
}

static fp_class_mask
fmin_fp_class(fp_class_mask left, fp_class_mask right)
{
   fp_class_mask result = 0;

   /* If one source is NaN, we have to include the whole range of the other source. */
   if (left & FP_CLASS_NAN)
      result |= right;
   if (right & FP_CLASS_NAN)
      result |= left;

   result |= fmin_part(left, right);
   result |= fmin_part(right, left);

   /* Could probably do better, but meh. */
   if ((left | right) & FP_CLASS_NON_INTEGRAL) {
      if (result & (FP_CLASS_LT_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE |
                    FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_GT_POS_ONE))
         result |= FP_CLASS_NON_INTEGRAL;
   }

   return result;
}

static fp_class_mask
handle_sz(const nir_alu_instr *alu, fp_class_mask src)
{
   if (nir_alu_instr_is_signed_zero_preserve(alu) || !(src & FP_CLASS_ANY_ZERO))
      return src;

   return src | FP_CLASS_ANY_ZERO;
}

static fp_class_mask
intrinsic_fp_class(const nir_intrinsic_instr *intrin)
{
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_typed_buffer_amd: {
      const enum pipe_format format = nir_intrinsic_format(intrin);
      if (format == PIPE_FORMAT_NONE)
         return FP_CLASS_UNKNOWN;
      const struct util_format_description *desc = util_format_description(format);

      int i = util_format_get_first_non_void_channel(format);
      if (i == -1)
         return FP_CLASS_UNKNOWN;

      bool is_signed = desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED;
      bool is_unsigned = desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED;
      bool normalized = desc->channel[i].normalized;
      if ((!is_signed && !is_unsigned) || desc->channel[i].pure_integer)
         return FP_CLASS_UNKNOWN;

      fp_class_mask result = FP_CLASS_POS_ZERO | FP_CLASS_POS_ONE;
      result |= is_signed ? FP_CLASS_NEG_ONE : 0;

      if (normalized) {
         result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_NON_INTEGRAL;
         result |= is_signed ? FP_CLASS_LT_ZERO_GT_NEG_ONE : 0;
      } else {
         result |= FP_CLASS_GT_POS_ONE;
         result |= is_signed ? FP_CLASS_LT_NEG_ONE : 0;
      }
      return result;
   }
   case nir_intrinsic_load_front_face_fsign:
      return FP_CLASS_POS_ONE | FP_CLASS_NEG_ONE;
   default:
      return FP_CLASS_UNKNOWN;
   }
}

static fp_class_mask
tex_fp_class(nir_tex_instr *tex)
{
   /* Not much to analyze, except shadow compare. */
   if (!tex->is_shadow)
      return FP_CLASS_UNKNOWN;

   fp_class_mask result = FP_CLASS_POS_ZERO | FP_CLASS_POS_ONE;

   /* Gather returns 0 or 1, other ops  can interpolate.
    * Cube corners are special even for gathers.
    */
   if (tex->op != nir_texop_tg4 || tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE)
      result |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_NON_INTEGRAL;

   return result;
}

/**
 * Analyze an expression to determine the possible fp classes of its result
 */
static void
process_fp_query(struct analysis_state *state, struct analysis_query *aq, uint32_t *result,
                 const uint32_t *src_res)
{
   struct fp_query q = *(struct fp_query *)aq;
   const nir_def *def = q.def;

   if (nir_def_is_const(def)) {
      *result = analyze_fp_constant(nir_def_as_load_const(def));
      return;
   } else if (nir_def_is_intrinsic(def)) {
      *result = intrinsic_fp_class(nir_def_as_intrinsic(def));
      return;
   } else if (nir_def_is_tex(def)) {
      *result = tex_fp_class(nir_def_as_tex(def));
      return;
   } else if (!nir_def_is_alu(def)) {
      *result = FP_CLASS_UNKNOWN;
      return;
   }

   const nir_alu_instr *const alu = nir_def_as_alu(def);

   if (!aq->pushed_queries) {
      switch (alu->op) {
      case nir_op_bcsel:
         push_fp_query(state, alu->src[1].src.ssa);
         push_fp_query(state, alu->src[2].src.ssa);
         return;
      case nir_op_mov:
      case nir_op_fabs:
      case nir_op_fexp2:
      case nir_op_flog2:
      case nir_op_frcp:
      case nir_op_fsqrt:
      case nir_op_frsq:
      case nir_op_fneg:
      case nir_op_fsat:
      case nir_op_fsign:
      case nir_op_ffloor:
      case nir_op_fceil:
      case nir_op_ftrunc:
      case nir_op_fround_even:
      case nir_op_ffract:
      case nir_op_fsin:
      case nir_op_fcos:
      case nir_op_fsin_normalized_2_pi:
      case nir_op_fcos_normalized_2_pi:
      case nir_op_f2f16:
      case nir_op_f2f16_rtz:
      case nir_op_f2f16_rtne:
      case nir_op_f2f32:
      case nir_op_f2f64:
      case nir_op_fdot2:
      case nir_op_fdot3:
      case nir_op_fdot4:
      case nir_op_fdot8:
      case nir_op_fdot16:
      case nir_op_fdot2_replicated:
      case nir_op_fdot3_replicated:
      case nir_op_fdot4_replicated:
      case nir_op_fdot8_replicated:
      case nir_op_fdot16_replicated:
         push_fp_query(state, alu->src[0].src.ssa);
         return;
      case nir_op_fadd:
      case nir_op_fsub:
      case nir_op_fmax:
      case nir_op_fmin:
      case nir_op_fmul:
      case nir_op_fmul_rtz:
      case nir_op_fmulz:
      case nir_op_fpow:
      case nir_op_vec2:
         push_fp_query(state, alu->src[0].src.ssa);
         push_fp_query(state, alu->src[1].src.ssa);
         return;
      case nir_op_ffma:
      case nir_op_ffma_weak:
      case nir_op_ffmaz:
      case nir_op_fmad:
      case nir_op_fmadz:
      case nir_op_flrp:
         push_fp_query(state, alu->src[0].src.ssa);
         push_fp_query(state, alu->src[1].src.ssa);
         push_fp_query(state, alu->src[2].src.ssa);
         return;
      default:
         break;
      }
   }

   fp_class_mask r = FP_CLASS_UNKNOWN;

   switch (alu->op) {
   case nir_op_b2i16:
   case nir_op_b2i32:
   case nir_op_b2i64:
      /* b2i32 will generate either 0x00000000 or 0x00000001.  When those bit
       * patterns are interpreted as floating point, they are 0.0 and
       * 1.401298464324817e-45.  The latter is subnormal.
       */
      r = FP_CLASS_POS_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE;
      break;

   case nir_op_b2f16:
   case nir_op_b2f32:
   case nir_op_b2f64:
      r = FP_CLASS_POS_ZERO | FP_CLASS_POS_ONE;
      break;

   case nir_op_vec2:
   case nir_op_bcsel:
      r = src_res[0] | src_res[1];
      break;

   case nir_op_i2f16:
   case nir_op_i2f32:
   case nir_op_i2f64:
      r &= ~FP_CLASS_NAN;
      r &= ~FP_CLASS_NON_INTEGRAL;
      r &= ~FP_CLASS_GT_ZERO_LT_POS_ONE;
      r &= ~FP_CLASS_LT_ZERO_GT_NEG_ONE;
      r &= ~FP_CLASS_NEG_ZERO;
      if (alu->def.bit_size > 16 || alu->src[0].src.ssa->bit_size <= 16)
         r &= ~FP_CLASS_ANY_INF;
      break;

   case nir_op_u2f16:
   case nir_op_u2f32:
   case nir_op_u2f64:
      r &= ~FP_CLASS_NAN;
      r &= ~FP_CLASS_NON_INTEGRAL;
      r &= ~FP_CLASS_GT_ZERO_LT_POS_ONE;
      r &= ~FP_CLASS_NEG_ZERO;
      r &= ~FP_CLASS_ANY_NEG;
      if (alu->def.bit_size > 16 || alu->src[0].src.ssa->bit_size < 16)
         r &= ~FP_CLASS_ANY_INF;
      break;

   case nir_op_f2f16:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16_rtne:
   case nir_op_f2f32:
   case nir_op_f2f64: {
      r = handle_sz(alu, src_res[0]);

      if (alu->src[0].src.ssa->bit_size > alu->def.bit_size) {
         bool rtz = alu->op == nir_op_f2f16_rtz;
         if (alu->op != nir_op_f2f16_rtne && alu->op != nir_op_f2f16_rtz) {
            nir_shader *shader = alu->instr.block->impl->function->shader;
            unsigned execution_mode = shader->info.float_controls_execution_mode;
            rtz = nir_is_rounding_mode_rtz(execution_mode, alu->def.bit_size);
         }

         /* Unless we are rounding towards zero, large values can create Inf. */
         if (r & FP_CLASS_LT_NEG_ONE) {
            if (!rtz)
               r |= FP_CLASS_NEG_INF;
            r |= FP_CLASS_NEG_ONE;
         }
         if (r & FP_CLASS_GT_POS_ONE) {
            if (!rtz)
               r |= FP_CLASS_POS_INF;
            r |= FP_CLASS_POS_ONE;
         }

         /* Underflow can create new zeros. */
         if (r & FP_CLASS_LT_ZERO_GT_NEG_ONE) {
            if (!rtz)
               r |= FP_CLASS_NEG_ONE;
            r |= FP_CLASS_NEG_ZERO;
         }
         if (r & FP_CLASS_GT_ZERO_LT_POS_ONE) {
            if (!rtz)
               r |= FP_CLASS_POS_ONE;
            r |= FP_CLASS_POS_ZERO;
         }
      }
      break;
   }

   case nir_op_fneg:
      r = fneg_fp_class(src_res[0]);
      break;

   case nir_op_fabs:
      r = src_res[0];

      r |= fneg_fp_class(r & (FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO));
      r &= ~(FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO);
      break;

   case nir_op_fadd: {
      r = fadd_fp_class(src_res[0], src_res[1]);
      break;
   }

   case nir_op_fsub: {
      r = fadd_fp_class(src_res[0], fneg_fp_class(src_res[1]));
      break;
   }

   case nir_op_fexp2: {
      fp_class_mask src = src_res[0];
      r = 0;

      /* If the parameter might be less than zero, the mathematically result
       * will be on (0, 1).  For sufficiently large magnitude negative
       * parameters, the result will flush to zero.
       */
      if (src & FP_CLASS_NEG_INF)
         r |= FP_CLASS_POS_ZERO;

      if (src & FP_CLASS_LT_NEG_ONE)
         r |= FP_CLASS_POS_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_NON_INTEGRAL;

      if (src & (FP_CLASS_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE))
         r |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_NON_INTEGRAL;

      if (src & (FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_GT_ZERO_LT_POS_ONE))
         r |= FP_CLASS_POS_ONE;

      if (src & (FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE))
         r |= FP_CLASS_GT_POS_ONE;

      if (src & (FP_CLASS_GT_POS_ONE))
         r |= FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF;

      if (src & FP_CLASS_POS_INF)
         r |= FP_CLASS_POS_INF;

      if (src & FP_CLASS_NON_INTEGRAL)
         r |= FP_CLASS_NON_INTEGRAL;

      if (src & FP_CLASS_NAN)
         r |= FP_CLASS_NAN;
      break;
   }

   case nir_op_flog2: {
      r = 0;

      if (src_res[0] & (FP_CLASS_ANY_NEG | FP_CLASS_NAN))
         r |= FP_CLASS_NAN;

      if (src_res[0] & FP_CLASS_ANY_ZERO)
         r |= FP_CLASS_NEG_INF;

      if (src_res[0] & FP_CLASS_GT_ZERO_LT_POS_ONE)
         r |= FP_CLASS_ANY_NEG | FP_CLASS_NON_INTEGRAL;

      if (src_res[0] & FP_CLASS_POS_ONE)
         r |= FP_CLASS_POS_ZERO;

      if (src_res[0] & FP_CLASS_GT_POS_ONE)
         r |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE | FP_CLASS_NON_INTEGRAL;

      if (src_res[0] & FP_CLASS_POS_INF)
         r |= FP_CLASS_POS_INF;
      break;
   }

   case nir_op_fmax: {
      fp_class_mask left = fneg_fp_class(src_res[0]);
      fp_class_mask right = fneg_fp_class(src_res[1]);
      r = fneg_fp_class(fmin_fp_class(left, right));
      break;
   }

   case nir_op_fmin:
      r = fmin_fp_class(src_res[0], src_res[1]);
      break;

   case nir_op_fmul:
   case nir_op_fmul_rtz:
   case nir_op_fmulz: {
      bool mulz = alu->op == nir_op_fmulz;
      bool src_eq = nir_alu_srcs_equal(alu, alu, 0, 1);
      bool src_neg_eq = !nir_src_is_const(alu->src[0].src) && nir_alu_srcs_negative_equal(alu, alu, 0, 1);
      r = fmul_fp_class(src_res[0], src_res[1], mulz, src_eq, src_neg_eq);
      break;
   }

   case nir_op_frcp:
      /* Some backends have terrible precision for fp64,
       * so don't try to do anything clever.
       */
      if (alu->def.bit_size < 64)
         r = frcp_fp_class(handle_sz(alu, src_res[0]));
      break;

   case nir_op_mov:
      r = src_res[0];
      break;

   case nir_op_fsat: {
      r = src_res[0];

      /* max(+0.0, x) */
      if (r & (FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO | FP_CLASS_NAN)) {
         r &= ~(FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO | FP_CLASS_NAN);
         r |= FP_CLASS_POS_ZERO;
      }

      /* min(+1.0, x) */
      if (r & (FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF)) {
         r &= ~(FP_CLASS_GT_POS_ONE | FP_CLASS_POS_INF);
         r |= FP_CLASS_POS_ONE;
      }

      if (!(r & FP_CLASS_GT_ZERO_LT_POS_ONE))
         r &= ~FP_CLASS_NON_INTEGRAL;

      break;
   }

   case nir_op_fsign:
      r = 0;

      if (src_res[0] & FP_CLASS_ANY_NEG)
         r |= FP_CLASS_NEG_ONE;

      if (src_res[0] & FP_CLASS_ANY_ZERO)
         r |= FP_CLASS_ANY_ZERO;

      if (src_res[0] & FP_CLASS_ANY_POS)
         r |= FP_CLASS_POS_ONE;

      /* fsign is -1, 0, or 1, even for NaN */
      if (src_res[0] & FP_CLASS_NAN)
         r |= FP_CLASS_NEG_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_POS_ONE;
      break;

   case nir_op_fsqrt:
      if (alu->def.bit_size < 64)
         r = fsqrt_fp_class(src_res[0]);
      break;

   case nir_op_frsq:
      if (alu->def.bit_size < 64)
         r = frcp_fp_class(fsqrt_fp_class(handle_sz(alu, src_res[0])));
      break;

   case nir_op_ffloor: {
      /* In IEEE 754, floor(NaN) is NaN, and floor(±Inf) is ±Inf. See
       * https://pubs.opengroup.org/onlinepubs/9699919799.2016edition/functions/floor.html
       */
      r = src_res[0];

      if (r & FP_CLASS_NON_INTEGRAL) {
         if (r & FP_CLASS_LT_ZERO_GT_NEG_ONE)
            r |= FP_CLASS_NEG_ONE;

         if (r & FP_CLASS_GT_ZERO_LT_POS_ONE)
            r |= FP_CLASS_POS_ZERO;

         if (r & FP_CLASS_GT_POS_ONE)
            r |= FP_CLASS_POS_ONE;

         r &= ~(FP_CLASS_NON_INTEGRAL | FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE);
      }
      break;
   }

   case nir_op_fceil: {
      /* In IEEE 754, ceil(NaN) is NaN, and ceil(±Inf) is ±Inf. See
       * https://pubs.opengroup.org/onlinepubs/9699919799.2016edition/functions/ceil.html
       */
      r = src_res[0];

      if (r & FP_CLASS_NON_INTEGRAL) {
         if (r & FP_CLASS_LT_NEG_ONE)
            r |= FP_CLASS_NEG_ONE;

         if (r & FP_CLASS_LT_ZERO_GT_NEG_ONE)
            r |= FP_CLASS_NEG_ZERO;

         if (r & FP_CLASS_GT_ZERO_LT_POS_ONE)
            r |= FP_CLASS_POS_ONE;

         r &= ~(FP_CLASS_NON_INTEGRAL | FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE);
      }
      break;
   }

   case nir_op_ftrunc: {
      /* In IEEE 754, trunc(NaN) is NaN, and trunc(±Inf) is ±Inf.  See
       * https://pubs.opengroup.org/onlinepubs/9699919799.2016edition/functions/trunc.html
       */
      r = src_res[0];

      if (r & FP_CLASS_NON_INTEGRAL) {
         if (r & FP_CLASS_LT_NEG_ONE)
            r |= FP_CLASS_NEG_ONE;

         if (r & FP_CLASS_LT_ZERO_GT_NEG_ONE)
            r |= FP_CLASS_NEG_ZERO;

         if (r & FP_CLASS_GT_ZERO_LT_POS_ONE)
            r |= FP_CLASS_POS_ZERO;

         if (r & FP_CLASS_GT_POS_ONE)
            r |= FP_CLASS_POS_ONE;

         r &= ~(FP_CLASS_NON_INTEGRAL | FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE);
      }
      break;
   }

   case nir_op_fround_even: {
      r = src_res[0];

      if (r & FP_CLASS_NON_INTEGRAL) {
         if (r & FP_CLASS_LT_NEG_ONE)
            r |= FP_CLASS_NEG_ONE;

         if (r & FP_CLASS_LT_ZERO_GT_NEG_ONE)
            r |= FP_CLASS_NEG_ZERO | FP_CLASS_NEG_ONE;

         if (r & FP_CLASS_GT_ZERO_LT_POS_ONE)
            r |= FP_CLASS_POS_ZERO | FP_CLASS_POS_ONE;

         if (r & FP_CLASS_GT_POS_ONE)
            r |= FP_CLASS_POS_ONE;

         r &= ~(FP_CLASS_NON_INTEGRAL | FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE);
      }
      break;
   }

   case nir_op_ffract: {
      r = 0;

      /* fract(±Inf) is NaN. */
      if (src_res[0] & (FP_CLASS_ANY_INF | FP_CLASS_NAN))
         r |= FP_CLASS_NAN;

      /* fract(non_integral) is in (0, 1). */
      if (src_res[0] & FP_CLASS_NON_INTEGRAL)
         r |= FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_NON_INTEGRAL;

      /* fract(small, negative) can be 1.0. */
      if (src_res[0] & FP_CLASS_LT_ZERO_GT_NEG_ONE)
         r |= FP_CLASS_POS_ONE;

      /* fract(integral) is +0.0. */
      if (src_res[0] & (FP_CLASS_LT_NEG_ONE | FP_CLASS_NEG_ONE | FP_CLASS_ANY_ZERO | FP_CLASS_POS_ONE | FP_CLASS_GT_POS_ONE))
         r |= FP_CLASS_POS_ZERO;

      break;
   }

   case nir_op_fsin:
   case nir_op_fcos:
   case nir_op_fsin_normalized_2_pi:
   case nir_op_fcos_normalized_2_pi: {
      /* [-1, +1], and sin/cos(Inf) is NaN */
      r = FP_CLASS_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE | FP_CLASS_ANY_ZERO |
          FP_CLASS_GT_ZERO_LT_POS_ONE | FP_CLASS_POS_ONE | FP_CLASS_NON_INTEGRAL;

      if (src_res[0] & (FP_CLASS_NAN | FP_CLASS_ANY_INF))
         r |= FP_CLASS_NAN;

      break;
   }

   case nir_op_fdot2:
   case nir_op_fdot3:
   case nir_op_fdot4:
   case nir_op_fdot8:
   case nir_op_fdot16:
   case nir_op_fdot2_replicated:
   case nir_op_fdot3_replicated:
   case nir_op_fdot4_replicated:
   case nir_op_fdot8_replicated:
   case nir_op_fdot16_replicated: {
      /* If the two sources are the same SSA value, then the result is either
       * NaN or some number >= 0.  If one source is the negation of the other,
       * the result is either NaN or some number <= 0.
       *
       * In either of these two cases, if one source is a number, then the
       * other must also be a number.  Since it should not be possible to get
       * Inf-Inf in the dot-product, the result must also be a number.
       */
      if (nir_alu_srcs_equal(alu, alu, 0, 1)) {
         r = FP_CLASS_ANY_POS | FP_CLASS_POS_ZERO;
      } else if (nir_alu_srcs_negative_equal(alu, alu, 0, 1)) {
         r = FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO;
      } else {
         r = FP_CLASS_UNKNOWN;
      }

      if (src_res[0] & FP_CLASS_NAN)
         r |= FP_CLASS_NAN;

      if (src_res[0] & FP_CLASS_NON_INTEGRAL)
         r |= FP_CLASS_NON_INTEGRAL;

      break;
   }

   case nir_op_fpow: {
      /* This is a basic port of the old range analysis, the opcode is very
       * underdefined. But improvements are likely possible.
       * Due to flush-to-zero semanatics of floating-point numbers with very
       * small mangnitudes, we can never really be sure a result will be
       * non-zero.
       *
       * NIR uses pow() and powf() to constant evaluate nir_op_fpow.  The man
       * page for that function says:
       *
       *    If y is 0, the result is 1.0 (even if x is a NaN).
       *
       * gt_zero: pow(*, eq_zero)
       *        | pow(eq_zero, lt_zero)   # 0^-y = +inf
       *        | pow(eq_zero, le_zero)   # 0^-y = +inf or 0^0 = 1.0
       *        ;
       *
       * eq_zero: pow(eq_zero, gt_zero)
       *        ;
       *
       * ge_zero: pow(gt_zero, gt_zero)
       *        | pow(gt_zero, ge_zero)
       *        | pow(gt_zero, lt_zero)
       *        | pow(gt_zero, le_zero)
       *        | pow(gt_zero, ne_zero)
       *        | pow(gt_zero, unknown)
       *        | pow(ge_zero, gt_zero)
       *        | pow(ge_zero, ge_zero)
       *        | pow(ge_zero, lt_zero)
       *        | pow(ge_zero, le_zero)
       *        | pow(ge_zero, ne_zero)
       *        | pow(ge_zero, unknown)
       *        | pow(eq_zero, ge_zero)  # 0^0 = 1.0 or 0^+y = 0.0
       *        | pow(eq_zero, ne_zero)  # 0^-y = +inf or 0^+y = 0.0
       *        | pow(eq_zero, unknown)  # union of all other y cases
       *        ;
       *
       * All other cases are unknown.
       *
       * We could do better if the right operand is a constant, integral
       * value.
       */

      fp_class_mask left = src_res[0];
      fp_class_mask right = src_res[1];

      if (!(right & (FP_CLASS_ANY_NEG | FP_CLASS_ANY_POS))) {
         r = FP_CLASS_ANY_POS;
      } else if (left & (FP_CLASS_ANY_NEG | FP_CLASS_NEG_ZERO)) {
         r = FP_CLASS_UNKNOWN;
      } else {
         r = FP_CLASS_ANY_POS | FP_CLASS_ANY_ZERO;
         if ((right & (FP_CLASS_ANY_NEG | FP_CLASS_NON_INTEGRAL)) || (left & FP_CLASS_NON_INTEGRAL))
            r |= FP_CLASS_NON_INTEGRAL;
      }

      /* Various cases can result in NaN, so assume the worst. */
      r |= FP_CLASS_NAN;

      break;
   }

   case nir_op_fmad:
   case nir_op_fmadz:
   case nir_op_ffma:
   case nir_op_ffmaz: {
      bool mulz = nir_alu_instr_is_mul_add_z(alu);
      bool src_eq = nir_alu_srcs_equal(alu, alu, 0, 1);
      bool src_neg_eq = !nir_src_is_const(alu->src[0].src) && nir_alu_srcs_negative_equal(alu, alu, 0, 1);
      fp_class_mask r_mul = fmul_fp_class(src_res[0], src_res[1], mulz, src_eq, src_neg_eq);
      r = fadd_fp_class(r_mul, src_res[2]);

      /* fma(a, b, +0.0) can be -0.0 if a * b underflows.
       * When fused, the underflow is not flushed before the addition.
       */
      bool mul_underflow = (((src_res[0] & FP_CLASS_LT_ZERO_GT_NEG_ONE) && (src_res[1] & FP_CLASS_GT_ZERO_LT_POS_ONE)) ||
                            ((src_res[1] & FP_CLASS_LT_ZERO_GT_NEG_ONE) && (src_res[0] & FP_CLASS_GT_ZERO_LT_POS_ONE)));
      if (!src_eq && mul_underflow && (src_res[2] & FP_CLASS_POS_ZERO))
         r |= FP_CLASS_NEG_ZERO;

      break;
   }

   case nir_op_flrp: {
      /* Decompose the flrp to first + third * (second + -first) */
      fp_class_mask inner_fadd_class =
         fadd_fp_class(src_res[1], fneg_fp_class(src_res[0]));

      fp_class_mask fmul_class =
         fmul_fp_class(src_res[2], inner_fadd_class, false, false, false);

      r = fadd_fp_class(src_res[0], fmul_class);

      /* Various cases can result in NaN, so assume the worst. */
      r |= FP_CLASS_NAN;
      break;
   }

   default:
      r = FP_CLASS_UNKNOWN;
      break;
   }

   if (nir_alu_type_get_base_type(nir_op_infos[alu->op].output_type) == nir_type_float)
      r = handle_sz(alu, r);

   assert((r & FP_CLASS_UNKNOWN) == r);
   assert((r & ~FP_CLASS_NON_INTEGRAL) != 0);
   assert(!(r & FP_CLASS_NON_INTEGRAL) || (r & (FP_CLASS_LT_NEG_ONE | FP_CLASS_LT_ZERO_GT_NEG_ONE |
                                                FP_CLASS_GT_POS_ONE | FP_CLASS_GT_ZERO_LT_POS_ONE)));

   *result = r;
}

fp_class_mask
nir_analyze_fp_class(nir_fp_analysis_state *fp_state, const nir_def *def)
{
   struct fp_query query_alloc[64];
   uint32_t result_alloc[64];

   struct analysis_state state;
   state.range_ht = fp_state;
   util_dynarray_init_from_stack(&state.query_stack, query_alloc, sizeof(query_alloc));
   util_dynarray_init_from_stack(&state.result_stack, result_alloc, sizeof(result_alloc));
   state.query_size = sizeof(struct fp_query);
   state.get_key = &get_fp_key;
   state.lookup = &fp_lookup;
   state.insert = &fp_insert;
   state.process_query = &process_fp_query;

   push_fp_query(&state, def);

   return perform_analysis(&state);
}

nir_fp_analysis_state
nir_create_fp_analysis_state(nir_function_impl *impl)
{
   nir_fp_analysis_state state;
   state.impl = impl;
   /* Over-allocate, so that we can keep using the allocated table memory
    * even when new SSA values are added. */
   state.size = (impl->ssa_alloc + impl->ssa_alloc / 4u);
   state.max = -1;
   state.bitset = calloc(BITSET_BYTES(state.size), 1);
   state.arr = malloc(state.size * sizeof(uint16_t));
   return state;
}

void
nir_invalidate_fp_analysis_state(nir_fp_analysis_state *state)
{
   if (state->impl->ssa_alloc > state->size) {
      state->size = state->impl->ssa_alloc + state->impl->ssa_alloc / 4u;

      free(state->arr);
      free(state->bitset);
      state->arr = malloc(state->size * sizeof(uint16_t));
      state->bitset = calloc(BITSET_BYTES(state->size), 1);
   } else if (state->max >= 0) {
      memset(state->bitset, 0, BITSET_BYTES(state->max + 1));
   }
   state->max = -1;
}

void
nir_free_fp_analysis_state(nir_fp_analysis_state *state)
{
   free(state->arr);
   free(state->bitset);
}

static uint32_t
bitmask(uint32_t size)
{
   return size >= 32 ? 0xffffffffu : ((uint32_t)1 << size) - 1u;
}

static uint64_t
mul_clamp(uint32_t a, uint32_t b)
{
   if (a != 0 && (a * b) / a != b)
      return (uint64_t)UINT32_MAX + 1;
   else
      return a * b;
}

/* recursively gather at most "buf_size" phi/bcsel sources */
static unsigned
search_phi_bcsel(nir_scalar scalar, nir_scalar *buf, unsigned buf_size, struct set *visited)
{
   if (_mesa_set_search(visited, scalar.def))
      return 0;
   _mesa_set_add(visited, scalar.def);

   if (nir_def_instr_type(scalar.def) == nir_instr_type_phi) {
      nir_phi_instr *phi = nir_def_as_phi(scalar.def);
      unsigned num_sources_left = exec_list_length(&phi->srcs);
      if (buf_size >= num_sources_left) {
         unsigned total_added = 0;
         nir_foreach_phi_src(src, phi) {
            num_sources_left--;
            unsigned added = search_phi_bcsel(nir_get_scalar(src->src.ssa, scalar.comp),
                                              buf + total_added, buf_size - num_sources_left, visited);
            assert(added <= buf_size);
            buf_size -= added;
            total_added += added;
         }
         return total_added;
      }
   }

   if (nir_scalar_is_alu(scalar)) {
      nir_op op = nir_scalar_alu_op(scalar);

      if ((op == nir_op_bcsel || op == nir_op_b32csel) && buf_size >= 2) {
         nir_scalar src1 = nir_scalar_chase_alu_src(scalar, 1);
         nir_scalar src2 = nir_scalar_chase_alu_src(scalar, 2);

         unsigned added = search_phi_bcsel(src1, buf, buf_size - 1, visited);
         buf_size -= added;
         added += search_phi_bcsel(src2, buf + added, buf_size, visited);
         return added;
      }
   }

   buf[0] = scalar;
   return 1;
}

static uint32_t
get_max_workgroup_invocations(nir_shader *nir)
{
   if (!nir->options || !nir->options->max_workgroup_invocations)
      return UINT16_MAX;

   return nir->options->max_workgroup_invocations;
}

static uint32_t
get_max_workgroup_count(nir_shader *nir, unsigned dim)
{
   /* max_workgroup_count represents the maximum compute shader / kernel
    * dispatchable work size. On most hardware, this is essentially
    * unbounded. On some hardware max_workgroup_count[1] and
    * max_workgroup_count[2] may be smaller.
    */
   if (!nir->options || !nir->options->max_workgroup_count[dim])
      return UINT32_MAX;

   return nir->options->max_workgroup_count[dim];
}

struct scalar_query {
   struct analysis_query head;
   nir_scalar scalar;
};

static void
push_scalar_query(struct analysis_state *state, nir_scalar scalar)
{
   struct scalar_query *pushed_q = push_analysis_query(state, sizeof(struct scalar_query));
   pushed_q->scalar = scalar;
}

static uint32_t
get_scalar_key(struct analysis_query *q)
{
   nir_scalar scalar = ((struct scalar_query *)q)->scalar;
   /* keys can't be 0, so we have to add 1 to the index */
   unsigned shift_amount = ffs(NIR_MAX_VEC_COMPONENTS) - 1;
   return nir_scalar_is_const(scalar)
             ? UINT32_MAX
             : ((scalar.def->index + 1) << shift_amount) | scalar.comp;
}

static bool
scalar_lookup(void *table, uint32_t key, uint32_t *value)
{
   struct hash_table *ht = table;
   struct hash_entry *he = _mesa_hash_table_search(ht, (void *)(uintptr_t)key);
   if (he)
      *value = (uintptr_t)he->data;
   return he != NULL;
}

static void
scalar_insert(void *table, uint32_t key, uint32_t value)
{
   struct hash_table *ht = table;
   _mesa_hash_table_insert(ht, (void *)(uintptr_t)key, (void *)(uintptr_t)value);
}

static void
get_phi_query(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src,
              uint32_t (*combine)(uint32_t, uint32_t), uint32_t safe_value)
{
   nir_phi_instr *phi = nir_def_as_phi(q.scalar.def);

   if (exec_list_is_empty(&phi->srcs))
      return;

   if (q.head.pushed_queries) {
      *result = src[0];
      for (unsigned i = 1; i < q.head.pushed_queries; i++)
         *result = combine(*result, src[i]);
      return;
   }

   nir_cf_node *prev = nir_cf_node_prev(&phi->instr.block->cf_node);
   if (!prev || prev->type == nir_cf_node_block) {
      /* Resolve cycles by inserting safe_value into range_ht. */
      scalar_insert(state->range_ht, get_scalar_key(&q.head), safe_value);

      struct set *visited = _mesa_pointer_set_create(NULL);
      nir_scalar *defs = alloca(sizeof(nir_scalar) * 64);
      unsigned def_count = search_phi_bcsel(q.scalar, defs, 64, visited);
      _mesa_set_destroy(visited, NULL);

      for (unsigned i = 0; i < def_count; i++)
         push_scalar_query(state, defs[i]);
   } else {
      nir_foreach_phi_src(src, phi)
         push_scalar_query(state, nir_get_scalar(src->src.ssa, q.scalar.comp));
   }
}

static void
get_intrinsic_uub(struct analysis_state *state, struct scalar_query q, uint32_t *result,
                  const uint32_t *src)
{
   nir_shader *shader = state->shader;

   nir_intrinsic_instr *intrin = nir_def_as_intrinsic(q.scalar.def);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_local_invocation_index:
      /* The local invocation index is used under the hood by RADV for
       * some non-compute-like shaders (eg. LS and NGG). These technically
       * run in workgroups on the HW, even though this fact is not exposed
       * by the API.
       * They can safely use the same code path here as variable sized
       * compute-like shader stages.
       */
      if (!mesa_shader_stage_uses_workgroup(shader->info.stage) ||
          shader->info.workgroup_size_variable) {
         *result = get_max_workgroup_invocations(shader) - 1;
      } else {
         *result = (shader->info.workgroup_size[0] *
                    shader->info.workgroup_size[1] *
                    shader->info.workgroup_size[2]) -
                   1u;
      }
      break;
   case nir_intrinsic_load_local_invocation_id:
      if (shader->info.workgroup_size_variable)
         *result = get_max_workgroup_invocations(shader) - 1u;
      else
         *result = shader->info.workgroup_size[q.scalar.comp] - 1u;
      break;
   case nir_intrinsic_load_workgroup_id:
      *result = get_max_workgroup_count(shader, q.scalar.comp) - 1u;
      break;
   case nir_intrinsic_load_num_workgroups:
      *result = get_max_workgroup_count(shader, q.scalar.comp);
      break;
   case nir_intrinsic_load_global_invocation_id:
      if (shader->info.workgroup_size_variable) {
         *result = mul_clamp(get_max_workgroup_invocations(shader),
                             get_max_workgroup_count(shader, q.scalar.comp)) -
                   1u;
      } else {
         *result = (shader->info.workgroup_size[q.scalar.comp] *
                    get_max_workgroup_count(shader, q.scalar.comp)) -
                   1u;
      }
      break;
   case nir_intrinsic_load_invocation_id:
      if (shader->info.stage == MESA_SHADER_TESS_CTRL)
         *result = shader->info.tess.tcs_vertices_out
                      ? (shader->info.tess.tcs_vertices_out - 1)
                      : 511; /* Generous maximum output patch size of 512 */
      break;
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_first_invocation:
      *result = shader->info.max_subgroup_size - 1;
      break;
   case nir_intrinsic_mbcnt_amd: {
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_get_scalar(intrin->src[1].ssa, 0));
         return;
      } else {
         uint32_t src0 = shader->info.max_subgroup_size - 1;
         uint32_t src1 = src[0];
         if (src0 + src1 >= src0) /* check overflow */
            *result = src0 + src1;
      }
      break;
   }
   case nir_intrinsic_load_subgroup_size:
      if (shader->info.api_subgroup_size)
         *result = shader->info.api_subgroup_size;
      else
         *result = shader->info.max_subgroup_size;
      break;
   case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_num_subgroups: {
      uint32_t workgroup_size = get_max_workgroup_invocations(shader);
      if (mesa_shader_stage_uses_workgroup(shader->info.stage) &&
          !shader->info.workgroup_size_variable) {
         workgroup_size = shader->info.workgroup_size[0] *
                          shader->info.workgroup_size[1] *
                          shader->info.workgroup_size[2];
      }
      *result = DIV_ROUND_UP(workgroup_size, shader->info.min_subgroup_size);
      if (intrin->intrinsic == nir_intrinsic_load_subgroup_id)
         (*result)--;
      break;
   }
   case nir_intrinsic_reduce:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan: {
      nir_op op = nir_intrinsic_reduction_op(intrin);

      switch (op) {
      case nir_op_umin:
      case nir_op_umax:
      case nir_op_imax:
      case nir_op_imin:
      case nir_op_iand:
      case nir_op_ior:
      case nir_op_ixor:
      case nir_op_iadd:
         if (!q.head.pushed_queries) {
            push_scalar_query(state, nir_get_scalar(intrin->src[0].ssa, q.scalar.comp));
            return;
         }
         break;
      default:
         return;
      }

      unsigned bit_size = q.scalar.def->bit_size;
      bool exclusive = intrin->intrinsic == nir_intrinsic_exclusive_scan;
      switch (op) {
      case nir_op_umin:
      case nir_op_umax:
      case nir_op_imax:
      case nir_op_imin:
      case nir_op_iand:
         *result = src[0];
         break;
      case nir_op_ior:
      case nir_op_ixor:
         *result = bitmask(util_last_bit64(src[0]));
         break;
      case nir_op_iadd:
         *result = MIN2(*result, (uint64_t)src[0] * (shader->info.max_subgroup_size - exclusive));
         break;
      default:
         UNREACHABLE("unhandled op");
      }

      if (exclusive) {
         uint32_t identity = nir_const_value_as_uint(nir_alu_binop_identity(op, bit_size), bit_size);
         *result = MAX2(*result, identity);
      }

      break;
   }
   case nir_intrinsic_shuffle_up_intel:
   case nir_intrinsic_shuffle_down_intel:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_get_scalar(intrin->src[0].ssa, q.scalar.comp));
         push_scalar_query(state, nir_get_scalar(intrin->src[1].ssa, q.scalar.comp));
         return;
      } else {
         *result = MAX2(src[0], src[1]);
      }
      break;
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_shuffle:
   case nir_intrinsic_shuffle_xor:
   case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down:
   case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_quad_swizzle_amd:
   case nir_intrinsic_masked_swizzle_amd:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_get_scalar(intrin->src[0].ssa, q.scalar.comp));
         return;
      } else {
         *result = src[0];
      }
      break;
   case nir_intrinsic_write_invocation_amd:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_get_scalar(intrin->src[0].ssa, q.scalar.comp));
         push_scalar_query(state, nir_get_scalar(intrin->src[1].ssa, q.scalar.comp));
         return;
      } else {
         *result = MAX2(src[0], src[1]);
      }
      break;
   case nir_intrinsic_load_tess_rel_patch_id_amd:
   case nir_intrinsic_load_tcs_num_patches_amd:
      /* Very generous maximum: TCS/TES executed by largest possible workgroup */
      *result = get_max_workgroup_invocations(shader) / MAX2(shader->info.tess.tcs_vertices_out, 1u);
      break;
   case nir_intrinsic_load_typed_buffer_amd: {
      const enum pipe_format format = nir_intrinsic_format(intrin);
      if (format == PIPE_FORMAT_NONE)
         break;

      const struct util_format_description *desc = util_format_description(format);
      if (desc->channel[q.scalar.comp].type != UTIL_FORMAT_TYPE_UNSIGNED)
         break;

      if (desc->channel[q.scalar.comp].normalized) {
         *result = fui(1.0);
         break;
      }

      const uint32_t chan_max = u_uintN_max(desc->channel[q.scalar.comp].size);
      *result = desc->channel[q.scalar.comp].pure_integer ? chan_max : fui(chan_max);
      break;
   }
   case nir_intrinsic_load_ttmp_register_amd:
   case nir_intrinsic_load_ttmp_register_wg_div_amd:
   case nir_intrinsic_load_scalar_arg_amd:
   case nir_intrinsic_load_scalar_arg_wg_div_amd:
   case nir_intrinsic_load_vector_arg_amd: {
      uint32_t upper_bound = nir_intrinsic_arg_upper_bound_u32_amd(intrin);
      if (upper_bound)
         *result = upper_bound;
      break;
   }

   case nir_intrinsic_image_samples:
      if (state->shader->options->max_samples > 0)
         *result = state->shader->options->max_samples;
      break;

   default:
      break;
   }
}

static void
get_alu_uub(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src)
{
   nir_op op = nir_scalar_alu_op(q.scalar);

   /* Early exit for unsupported ALU opcodes. */
   switch (op) {
   case nir_op_umin:
   case nir_op_imin:
   case nir_op_imax:
   case nir_op_umax:
   case nir_op_iand:
   case nir_op_ior:
   case nir_op_ixor:
   case nir_op_ishl:
   case nir_op_imul:
   case nir_op_ushr:
   case nir_op_ishr:
   case nir_op_iadd:
   case nir_op_umod:
   case nir_op_udiv:
   case nir_op_bcsel:
   case nir_op_b32csel:
   case nir_op_ubfe:
   case nir_op_bfi:
   case nir_op_bfm:
   case nir_op_bitfield_select:
   case nir_op_msad_4x8:
   case nir_op_extract_u8:
   case nir_op_extract_i8:
   case nir_op_extract_u16:
   case nir_op_extract_i16:
   case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
      break;
   case nir_op_u2u1:
   case nir_op_u2u8:
   case nir_op_u2u16:
   case nir_op_u2u32:
      if (nir_scalar_chase_alu_src(q.scalar, 0).def->bit_size > 32) {
         /* If src is >32 bits, return max */
         return;
      }
      break;
   case nir_op_fsat:
   case nir_op_fmul:
   case nir_op_fmul_rtz:
   case nir_op_fmulz:
   case nir_op_f2u32:
   case nir_op_f2i32:
      if (nir_scalar_chase_alu_src(q.scalar, 0).def->bit_size != 32) {
         /* Only 32bit floats support for now, return max */
         return;
      }
      break;
   case nir_op_bit_count:
      if (nir_scalar_chase_alu_src(q.scalar, 0).def->bit_size > 32) {
         *result = nir_scalar_chase_alu_src(q.scalar, 0).def->bit_size;
         return;
      }
      break;
   default:
      return;
   }

   if (!q.head.pushed_queries) {
      for (unsigned i = 0; i < nir_op_infos[op].num_inputs; i++)
         push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, i));
      return;
   }

   uint32_t max = bitmask(q.scalar.def->bit_size);
   switch (op) {
   case nir_op_umin:
      *result = src[0] < src[1] ? src[0] : src[1];
      break;
   case nir_op_imin:
   case nir_op_imax:
   case nir_op_umax:
      *result = src[0] > src[1] ? src[0] : src[1];
      break;
   case nir_op_iand: {
      nir_scalar src0_scalar = nir_scalar_chase_alu_src(q.scalar, 0);
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      if (nir_scalar_is_const(src0_scalar))
         *result = bitmask(util_last_bit64(src[1])) & nir_scalar_as_uint(src0_scalar);
      else if (nir_scalar_is_const(src1_scalar))
         *result = bitmask(util_last_bit64(src[0])) & nir_scalar_as_uint(src1_scalar);
      else
         *result = bitmask(util_last_bit64(src[0])) & bitmask(util_last_bit64(src[1]));
      break;
   }
   case nir_op_ior:
   case nir_op_ixor: {
      nir_scalar src0_scalar = nir_scalar_chase_alu_src(q.scalar, 0);
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      if (nir_scalar_is_const(src0_scalar))
         *result = bitmask(util_last_bit64(src[1])) | nir_scalar_as_uint(src0_scalar);
      else if (nir_scalar_is_const(src1_scalar))
         *result = bitmask(util_last_bit64(src[0])) | nir_scalar_as_uint(src1_scalar);
      else
         *result = bitmask(util_last_bit64(src[0])) | bitmask(util_last_bit64(src[1]));
      break;
   }
   case nir_op_ishl: {
      uint32_t src1 = MIN2(src[1], q.scalar.def->bit_size - 1u);
      if (util_last_bit64(src[0]) + src1 <= q.scalar.def->bit_size) /* check overflow */
         *result = src[0] << src1;
      *result = MIN2(*result, max);

      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      if (nir_scalar_is_const(src1_scalar)) {
         uint32_t const_val = 1u << (nir_scalar_as_uint(src1_scalar) & (q.scalar.def->bit_size - 1u));
         *result = MIN2(*result, max / const_val * const_val);
      }
      break;
   }
   case nir_op_imul: {
      if (src[0] == 0 || (src[0] * src[1]) / src[0] == src[1]) /* check overflow */
         *result = src[0] * src[1];
      *result = MIN2(*result, max);

      nir_scalar src0_scalar = nir_scalar_chase_alu_src(q.scalar, 0);
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      if (nir_scalar_is_const(src0_scalar)) {
         uint32_t const_val = nir_scalar_as_uint(src0_scalar);
         *result = const_val ? MIN2(*result, max / const_val * const_val) : 0;
      } else if (nir_scalar_is_const(src1_scalar)) {
         uint32_t const_val = nir_scalar_as_uint(src1_scalar);
         *result = const_val ? MIN2(*result, max / const_val * const_val) : 0;
      }
      break;
   }
   case nir_op_ushr: {
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      uint32_t mask = q.scalar.def->bit_size - 1u;
      if (nir_scalar_is_const(src1_scalar))
         *result = src[0] >> (nir_scalar_as_uint(src1_scalar) & mask);
      else
         *result = src[0];
      break;
   }
   case nir_op_ishr: {
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      uint32_t mask = q.scalar.def->bit_size - 1u;
      if (src[0] <= 2147483647 && nir_scalar_is_const(src1_scalar))
         *result = src[0] >> (nir_scalar_as_uint(src1_scalar) & mask);
      else
         *result = src[0];
      break;
   }
   case nir_op_iadd:
      if (src[0] + src[1] >= src[0]) /* check overflow */
         *result = src[0] + src[1];
      *result = MIN2(*result, max);
      break;
   case nir_op_umod:
      *result = src[1] ? src[1] - 1 : 0;
      break;
   case nir_op_udiv: {
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      if (nir_scalar_is_const(src1_scalar))
         *result = nir_scalar_as_uint(src1_scalar)
                      ? src[0] / nir_scalar_as_uint(src1_scalar)
                      : 0;
      else
         *result = src[0];
      break;
   }
   case nir_op_bcsel:
   case nir_op_b32csel:
      *result = src[1] > src[2] ? src[1] : src[2];
      break;
   case nir_op_ubfe:
      *result = bitmask(MIN2(src[2], q.scalar.def->bit_size));
      break;
   case nir_op_bfm: {
      nir_scalar src1_scalar = nir_scalar_chase_alu_src(q.scalar, 1);
      if (nir_scalar_is_const(src1_scalar)) {
         uint32_t src0 = MIN2(src[0], 31);
         uint32_t src1 = nir_scalar_as_uint(src1_scalar) & 0x1fu;
         *result = bitmask(src0) << src1;
      } else {
         uint32_t src0 = MIN2(src[0], 31);
         uint32_t src1 = MIN2(src[1], 31);
         *result = bitmask(MIN2(src0 + src1, 32));
      }
      break;
   }

   case nir_op_bfi: {
      nir_scalar src0_scalar = nir_scalar_chase_alu_src(q.scalar, 0);
      const uint64_t s1 = bitmask(util_last_bit64(src[1]));
      const uint64_t s2 = bitmask(util_last_bit64(src[2]));

      if (nir_scalar_is_const(src0_scalar)) {
         const uint64_t s0 = nir_scalar_as_uint(src0_scalar);

         /* This case should be eliminated by opt_algebraic. */
         if (s0 == 0) {
            *result = s2;
         } else {
            const int x = ffsll(s0) - 1;
            *result = (s0 & (s1 << x)) | (~s0 & s2);
         }
      } else {
         const uint64_t s0 = bitmask(util_last_bit64(src[0]));

         /* Due to the unpredictable shift, the true maximum value of (s0 &
          * (s1 << x)) cannot be known. However, it cannot be larger than
          * s0.
          *
          * inot doesn't work in get_alu_uub. It is known that (~s0 & s2)
          * cannot be larger than s2, so just use s2 as a loose upper bound.
          */
         *result = s0 | s2;
      }
      break;
   }

   case nir_op_bitfield_select: {
      nir_scalar src0_scalar = nir_scalar_chase_alu_src(q.scalar, 0);
      const uint64_t s1 = bitmask(util_last_bit64(src[1]));
      const uint64_t s2 = bitmask(util_last_bit64(src[2]));

      if (nir_scalar_is_const(src0_scalar)) {
         const uint64_t s0 = nir_scalar_as_uint(src0_scalar);

         *result = (s0 & s1) | (~s0 & s2);
      } else {
         const uint64_t s0 = bitmask(util_last_bit64(src[0]));

         /* inot doesn't work in get_alu_uub. It is known that (~s0 & s2)
          * cannot be larger than s2, so just use s2 as a loose upper bound.
          */
         *result = (s0 & s1) | s2;
      }
      break;
   }

   /* limited floating-point support for f2u32(fmul(load_input(), <constant>)) */
   case nir_op_f2i32:
   case nir_op_f2u32:
      /* infinity/NaN starts at 0x7f800000u, negative numbers at 0x80000000
       * but conversions to uint32_t are undefined for values larger than UINT32_MAX,
       * so clamp.
       */
      if (src[0] < 0x4f800000u) {
         float val;
         memcpy(&val, &src[0], 4);
         *result = (uint32_t)val;
      }
      break;
   case nir_op_fmul:
   case nir_op_fmul_rtz:
   case nir_op_fmulz:
      /* infinity/NaN starts at 0x7f800000u, negative numbers at 0x80000000 */
      if (src[0] < 0x7f800000u && src[1] < 0x7f800000u) {
         float src0_f, src1_f;
         memcpy(&src0_f, &src[0], 4);
         memcpy(&src1_f, &src[1], 4);
         /* not a proper rounding-up multiplication, but should be good enough */
         float max_f = ceilf(src0_f) * ceilf(src1_f);
         memcpy(result, &max_f, 4);
      }
      break;
   case nir_op_fsat:
      *result = 0x3f800000u;
      break;
   case nir_op_u2u1:
   case nir_op_u2u8:
   case nir_op_u2u16:
   case nir_op_u2u32:
      *result = MIN2(src[0], max);
      break;
   case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
      *result = 1;
      break;
   case nir_op_msad_4x8:
      *result = MIN2((uint64_t)src[2] + 4 * 255, UINT32_MAX);
      break;
   case nir_op_extract_u8:
      *result = MIN2(src[0], UINT8_MAX);
      break;
   case nir_op_extract_i8:
      *result = (src[0] >= 0x80) ? max : MIN2(src[0], INT8_MAX);
      break;
   case nir_op_extract_u16:
      *result = MIN2(src[0], UINT16_MAX);
      break;
   case nir_op_extract_i16:
      *result = (src[0] >= 0x8000) ? max : MIN2(src[0], INT16_MAX);
      break;
   case nir_op_bit_count:
      *result = util_last_bit64(src[0]);
      break;
   default:
      break;
   }
}

static void
get_tex_uub(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src)
{
   nir_tex_instr *tex = nir_scalar_as_tex(q.scalar);

   if (tex->op == nir_texop_texture_samples && state->shader->options->max_samples > 0)
      *result = state->shader->options->max_samples;
}

static uint32_t
max2(uint32_t a, uint32_t b)
{
   return MAX2(a, b);
}

static void
get_phi_uub(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src)
{
   get_phi_query(state, q, result, src, max2, bitmask(q.scalar.def->bit_size));
}

static void
process_uub_query(struct analysis_state *state, struct analysis_query *aq, uint32_t *result,
                  const uint32_t *src)
{
   struct scalar_query q = *(struct scalar_query *)aq;

   *result = bitmask(q.scalar.def->bit_size);
   if (nir_scalar_is_const(q.scalar))
      *result = nir_scalar_as_uint(q.scalar);
   else if (nir_scalar_is_intrinsic(q.scalar))
      get_intrinsic_uub(state, q, result, src);
   else if (nir_scalar_is_alu(q.scalar))
      get_alu_uub(state, q, result, src);
   else if (nir_def_instr_type(q.scalar.def) == nir_instr_type_tex)
      get_tex_uub(state, q, result, src);
   else if (nir_def_instr_type(q.scalar.def) == nir_instr_type_phi)
      get_phi_uub(state, q, result, src);
}

uint32_t
nir_unsigned_upper_bound(nir_shader *shader, struct hash_table *range_ht,
                         nir_scalar scalar)
{
   struct scalar_query query_alloc[16];
   uint32_t result_alloc[16];

   struct analysis_state state;
   state.shader = shader;
   state.range_ht = range_ht;
   util_dynarray_init_from_stack(&state.query_stack, query_alloc, sizeof(query_alloc));
   util_dynarray_init_from_stack(&state.result_stack, result_alloc, sizeof(result_alloc));
   state.query_size = sizeof(struct scalar_query);
   state.get_key = &get_scalar_key;
   state.lookup = &scalar_lookup,
   state.insert = &scalar_insert,
   state.process_query = &process_uub_query;

   push_scalar_query(&state, scalar);

   return perform_analysis(&state);
}

bool
nir_addition_might_overflow(nir_shader *shader, struct hash_table *range_ht,
                            nir_scalar ssa, unsigned const_val)
{
   uint32_t ub = nir_unsigned_upper_bound(shader, range_ht, ssa);
   return const_val + ub < const_val;
}

static uint64_t
ssa_def_bits_used(const nir_def *def, unsigned comp, int recur)
{
   uint64_t bits_used = 0;
   uint64_t all_bits = BITFIELD64_MASK(def->bit_size);

   assert(comp < def->num_components);

   /* Limit recursion */
   if (recur-- <= 0)
      return all_bits;

   nir_foreach_use(src, def) {
      switch (nir_src_use_instr(src)->type) {
      case nir_instr_type_alu: {
         nir_alu_instr *use_alu = nir_instr_as_alu(nir_src_use_instr(src));
         unsigned src_idx = container_of(src, nir_alu_src, src) - use_alu->src;

         /* Only look at scalar or trivial ALU. */
         if (use_alu->def.num_components == 1 && use_alu->src[src_idx].swizzle[0] == comp) {
            comp = 0;
         } else if (!nir_alu_has_trivial_src(use_alu, src_idx)) {
            /* If a user of the value produces a vector result and swizzles the source,
             * return the conservative answer that all bits are used. It is possible to
             * answer this query by looping over the components used. For example,
             *
             * vec4 32 ssa_5 = load_const(0x0000f000, 0x00000f00, 0x000000f0, 0x0000000f)
             * ...
             * vec4 32 ssa_8 = iand ssa_7.xxxx, ssa_5
             *
             * could conceivably return 0x0000ffff when queyring the bits used of
             * ssa_7.  This is unlikely to be worth the effort because the
             * question can eventually answered after the shader has been
             * scalarized.
             */
            return all_bits;
        }

         switch (use_alu->op) {
         case nir_op_u2u8:
         case nir_op_i2i8:
         case nir_op_u2u16:
         case nir_op_i2i16:
         case nir_op_u2u32:
         case nir_op_i2i32:
         case nir_op_u2u64:
         case nir_op_i2i64: {
            uint64_t def_bits_used = ssa_def_bits_used(&use_alu->def, comp, recur);

            /* If one of the sign-extended bits is used, set the last src bit
             * as used.
             */
            if ((use_alu->op == nir_op_i2i8 || use_alu->op == nir_op_i2i16 ||
                 use_alu->op == nir_op_i2i32 || use_alu->op == nir_op_i2i64) &&
                def_bits_used & ~all_bits)
               def_bits_used |= BITFIELD64_BIT(def->bit_size - 1);

            bits_used |= def_bits_used & all_bits;
            break;
         }

         case nir_op_extract_u8:
         case nir_op_extract_i8:
         case nir_op_extract_u16:
         case nir_op_extract_i16: {
            uint64_t chunk;

            if (src_idx == 0 && nir_alu_src_comp_get_uint(use_alu->src[1],
                                                          comp, &chunk)) {
               uint64_t defs_bits_used = ssa_def_bits_used(&use_alu->def, comp, recur);
               unsigned field_bits = use_alu->op == nir_op_extract_u8 ||
                                     use_alu->op == nir_op_extract_i8 ? 8 : 16;
               uint64_t field_bitmask = BITFIELD64_MASK(field_bits);

               /* If one of the sign-extended bits is used, set the last src bit
                * as used.
                */
               if ((use_alu->op == nir_op_extract_i8 ||
                    use_alu->op == nir_op_extract_i16) &&
                   defs_bits_used & ~field_bitmask)
                  defs_bits_used |= BITFIELD64_BIT(field_bits - 1);

               bits_used |= (field_bitmask & defs_bits_used) <<
                            (chunk * field_bits);
               break;
            } else {
               return all_bits;
            }
         }

         case nir_op_ishl:
         case nir_op_ishr:
         case nir_op_ushr: {
            uint64_t shift;

            if (src_idx == 0 && nir_alu_src_comp_get_uint(use_alu->src[1],
                                                          comp, &shift)) {
               unsigned bit_size = def->bit_size;
               shift &= bit_size - 1;
               uint64_t def_bits_used = ssa_def_bits_used(&use_alu->def, comp, recur);

               /* If one of the sign-extended bits is used, set the "last src
                * bit before shifting" as used.
                */
               if (use_alu->op == nir_op_ishr &&
                   def_bits_used & ~(all_bits >> shift))
                  def_bits_used |= BITFIELD64_BIT(bit_size - 1 - shift);

               /* Reverse the shift to get the bits before shifting. */
               if (use_alu->op == nir_op_ushr || use_alu->op == nir_op_ishr)
                  bits_used |= (def_bits_used << shift) & all_bits;
               else
                  bits_used |= def_bits_used >> shift;
               break;
            } else if (src_idx == 1) {
               bits_used |= use_alu->def.bit_size - 1;
               break;
            } else {
               return all_bits;
            }
         }

         case nir_op_iand:
         case nir_op_ior: {
            uint64_t other_src;

            assert(src_idx < 2);
            if (nir_alu_src_comp_get_uint(use_alu->src[1 - src_idx], comp,
                                          &other_src)) {
               if (use_alu->op == nir_op_iand)
                  bits_used |= ssa_def_bits_used(&use_alu->def, comp, recur) & other_src;
               else
                  bits_used |= ssa_def_bits_used(&use_alu->def, comp, recur) & ~other_src;
               break;
            } else {
               return all_bits;
            }
         }

         case nir_op_ibfe:
         case nir_op_ubfe: {
            uint64_t offset;

            if (src_idx == 0 && nir_alu_src_comp_get_uint(use_alu->src[1], comp, &offset)) {
               uint64_t def_bits_used = ssa_def_bits_used(&use_alu->def, comp, recur);
               unsigned bit_size = use_alu->def.bit_size;
               offset &= bit_size - 1;

               nir_scalar bits_src = nir_scalar_resolved(use_alu->src[2].src.ssa,
                                                         use_alu->src[2].swizzle[comp]);
               bool bits_is_const = nir_scalar_is_const(bits_src);
               uint64_t bits;

               if (bits_is_const)
                  bits = nir_scalar_as_uint(bits_src) & (bit_size - 1);
               else
                  bits = bit_size - offset; /* Worst case if bits is not constant. */

               uint64_t field_bitmask = BITFIELD64_MASK(bits);

               /* If one of the sign-extended bits is used, set the last src
                * bit as used.
                * If bits is not constant, all bits can be the last one.
                */
               if (use_alu->op == nir_op_ibfe &&
                   (def_bits_used & ~(bits_is_const ? field_bitmask : 1u))) {
                  if (bits_is_const)
                     def_bits_used |= BITFIELD64_BIT(bits - 1);
                  else
                     def_bits_used |= field_bitmask;
               }

               bits_used |= (field_bitmask & def_bits_used) << offset;
               break;
            } else if (src_idx == 1 || src_idx == 2) {
               bits_used |= use_alu->src[0].src.ssa->bit_size - 1;
               break;
            } else {
               return all_bits;
            }
         }

         case nir_op_imul24:
         case nir_op_umul24:
            bits_used |= all_bits & 0xffffff;
            break;

         case nir_op_mov:
            bits_used |= ssa_def_bits_used(&use_alu->def, comp, recur);
            break;

         case nir_op_bcsel:
            if (src_idx == 0)
               bits_used |= 0x1;
            else
               bits_used |= ssa_def_bits_used(&use_alu->def, comp, recur);
            break;

         default:
            /* We don't know what this op does */
            return all_bits;
         }
         break;
      }

      case nir_instr_type_intrinsic: {
         nir_intrinsic_instr *use_intrin =
            nir_instr_as_intrinsic(nir_src_use_instr(src));
         unsigned src_idx = src - use_intrin->src;

         switch (use_intrin->intrinsic) {
         case nir_intrinsic_shuffle_up_intel:
         case nir_intrinsic_shuffle_down_intel:
            if (src_idx == 0 || src_idx == 1) {
               bits_used |= ssa_def_bits_used(&use_intrin->def, comp, recur);
            } else {
               /* Subgroups larger than 128 are not a thing */
               bits_used |= 127;
            }
            break;

         case nir_intrinsic_read_invocation:
         case nir_intrinsic_shuffle:
         case nir_intrinsic_shuffle_up:
         case nir_intrinsic_shuffle_down:
         case nir_intrinsic_shuffle_xor:
         case nir_intrinsic_quad_broadcast:
         case nir_intrinsic_quad_swap_horizontal:
         case nir_intrinsic_quad_swap_vertical:
         case nir_intrinsic_quad_swap_diagonal:
            if (src_idx == 0) {
               bits_used |= ssa_def_bits_used(&use_intrin->def, comp, recur);
            } else {
               if (use_intrin->intrinsic == nir_intrinsic_quad_broadcast) {
                  bits_used |= 3;
               } else {
                  /* Subgroups larger than 128 are not a thing */
                  bits_used |= 127;
               }
            }
            break;

         case nir_intrinsic_reduce:
         case nir_intrinsic_inclusive_scan:
         case nir_intrinsic_exclusive_scan:
            assert(src_idx == 0);
            switch (nir_intrinsic_reduction_op(use_intrin)) {
            case nir_op_iadd:
            case nir_op_imul:
            case nir_op_ior:
            case nir_op_iand:
            case nir_op_ixor:
               bits_used |= ssa_def_bits_used(&use_intrin->def, comp, recur);
               break;

            default:
               return all_bits;
            }
            break;

         default:
            /* We don't know what this op does */
            return all_bits;
         }
         break;
      }

      case nir_instr_type_phi: {
         nir_phi_instr *use_phi = nir_instr_as_phi(nir_src_use_instr(src));
         bits_used |= ssa_def_bits_used(&use_phi->def, comp, recur);
         break;
      }

      default:
         return all_bits;
      }

      /* If we've somehow shown that all our bits are used, we're done */
      assert((bits_used & ~all_bits) == 0);
      if (bits_used == all_bits)
         return all_bits;
   }

   return bits_used;
}

uint64_t
nir_def_bits_used(nir_scalar def)
{
   return ssa_def_bits_used(def.def, def.comp, 2);
}

static void
get_intrinsic_num_lsb(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src)
{
   nir_intrinsic_instr *intrin = nir_def_as_intrinsic(q.scalar.def);

   switch (intrin->intrinsic) {
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_as_uniform:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_get_scalar(intrin->src[0].ssa, q.scalar.comp));
      } else {
         *result = src[0];
      }
      break;
   case nir_intrinsic_load_param:
   case nir_intrinsic_load_ttmp_register_amd:
   case nir_intrinsic_load_ttmp_register_wg_div_amd:
   case nir_intrinsic_load_scalar_arg_amd:
   case nir_intrinsic_load_scalar_arg_wg_div_amd:
   case nir_intrinsic_load_vector_arg_amd: {
      *result = nir_intrinsic_arg_num_lsb_zero(intrin);
      break;
   }
   default:
      break;
   }
}

static uint32_t
min2(uint32_t a, uint32_t b)
{
   return MIN2(a, b);
}

static void
get_phi_num_lsb(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src)
{
   get_phi_query(state, q, result, src, min2, 0);
}

static void
get_alu_num_lsb(struct analysis_state *state, struct scalar_query q, uint32_t *result, const uint32_t *src)
{
   nir_op op = nir_scalar_alu_op(q.scalar);

   switch (op) {
   case nir_op_ior:
   case nir_op_ixor:
   case nir_op_iadd:
   case nir_op_iand:
   case nir_op_imul:
   case nir_op_pack_64_2x32_split:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, 0));
         push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, 1));
         return;
      }
      break;
   case nir_op_ishl:
   case nir_op_u2u64:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, 0));
         return;
      }
      break;
   case nir_op_ishr:
   case nir_op_ushr:
      if (!q.head.pushed_queries) {
         if (nir_scalar_is_const(nir_scalar_chase_alu_src(q.scalar, 1)))
            push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, 0));
         return;
      }
      break;
   case nir_op_bcsel:
      if (!q.head.pushed_queries) {
         push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, 1));
         push_scalar_query(state, nir_scalar_chase_alu_src(q.scalar, 2));
         return;
      }
      break;
   default:
      return;
   }

   switch (op) {
   case nir_op_ior:
   case nir_op_ixor:
   case nir_op_iadd: {
      *result = MIN2(src[0], src[1]);
      break;
   }
   case nir_op_iand: {
      *result = MAX2(src[0], src[1]);
      break;
   }
   case nir_op_imul: {
      *result = MIN2(src[0] + src[1], q.scalar.def->bit_size);
      break;
   }
   case nir_op_ishl: {
      nir_scalar src1 = nir_scalar_chase_alu_src(q.scalar, 1);
      uint32_t mask = q.scalar.def->bit_size - 1;
      unsigned amount = nir_scalar_is_const(src1) ? nir_scalar_as_uint(src1) & mask : 0;
      *result = MIN2(src[0] + amount, q.scalar.def->bit_size);
      break;
   }
   case nir_op_ishr:
   case nir_op_ushr: {
      nir_scalar src1 = nir_scalar_chase_alu_src(q.scalar, 1);
      unsigned amount = nir_scalar_as_uint(src1) & (q.scalar.def->bit_size - 1);
      *result = amount > src[0] ? 0 : src[0] - amount;
      break;
   }
   case nir_op_bcsel: {
      *result = MIN2(src[0], src[1]);
      break;
   }
   case nir_op_u2u64: {
      nir_scalar src0 = nir_scalar_chase_alu_src(q.scalar, 0);
      *result = src[0] == src0.def->bit_size ? 64 : src[0];
      break;
   }
   case nir_op_pack_64_2x32_split: {
      *result = src[0] < 32 ? src[0] : 32 + src[1];
      break;
   }
   default:
      UNREACHABLE("Unknown opcode");
   }
}

static void
process_num_lsb_query(struct analysis_state *state, struct analysis_query *aq, uint32_t *result,
                      const uint32_t *src)
{
   struct scalar_query q = *(struct scalar_query *)aq;

   *result = 0;
   if (nir_scalar_is_const(q.scalar)) {
      uint64_t val = nir_scalar_as_uint(q.scalar);
      *result = val ? ffsll(val) - 1 : q.scalar.def->bit_size;
   } else if (nir_scalar_is_alu(q.scalar)) {
      get_alu_num_lsb(state, q, result, src);
   } else if (nir_scalar_is_intrinsic(q.scalar)) {
      get_intrinsic_num_lsb(state, q, result, src);
   } else if (nir_scalar_is_phi(q.scalar)) {
      get_phi_num_lsb(state, q, result, src);
   }
}

unsigned
nir_def_num_lsb_zero(struct hash_table *numlsb_ht, nir_scalar def)
{
   struct scalar_query query_alloc[16];
   uint32_t result_alloc[16];

   struct analysis_state state;
   state.shader = NULL;
   state.range_ht = numlsb_ht;
   util_dynarray_init_from_stack(&state.query_stack, query_alloc, sizeof(query_alloc));
   util_dynarray_init_from_stack(&state.result_stack, result_alloc, sizeof(result_alloc));
   state.query_size = sizeof(struct scalar_query);
   state.get_key = &get_scalar_key;
   state.lookup = &scalar_lookup,
   state.insert = &scalar_insert,
   state.process_query = &process_num_lsb_query;

   push_scalar_query(&state, def);

   return perform_analysis(&state);
}

/* Return the following:
 * - float_uses: the component mask of def used as float
 * - integer_uses: the component mask of def used as integer
 * - integer_bits_used: if non-NULL, it will have those bits set to 1
 *   that are used by integer uses of def, e.g. (f2u32(def.y) & mask) sets
 *   integer_bits_used[1] |= mask;
 *
 * The function only accumulates 1 bits in the parameters, preserving all
 * bits set by the caller.
 *
 * If stop_on_float_use is true and *float_uses != 0, the function terminates
 * immediately and all masks may be incomplete.
 */
void
nir_gather_type_uses_of_float_def(nir_def *def,
                                  nir_component_mask_t *float_uses,
                                  nir_component_mask_t *integer_uses,
                                  uint64_t integer_bits_used[NIR_MAX_VEC_COMPONENTS],
                                  bool stop_on_float_use)
{
   nir_foreach_use(use, def) {
      if (stop_on_float_use && *float_uses)
         return;

      nir_component_mask_t src_mask = nir_src_components_read(use);
      nir_instr *use_instr = nir_src_use_instr(use);

      if (use_instr->type != nir_instr_type_alu) {
         *float_uses |= src_mask;
         continue;
      }

      nir_alu_instr *alu = nir_instr_as_alu(use_instr);

      /* Trivially handle non-component-wise ALU. */
      if (nir_op_infos[alu->op].output_size) {
         *float_uses |= src_mask;
         continue;
      }

      /* Only component-wise ALU. */
      for (unsigned i = 0; i < alu->def.num_components; i++) {
         uint8_t src0_comp = alu->src[0].swizzle[i];

         switch (alu->op) {
         case nir_op_f2i8:
         case nir_op_f2i16:
         case nir_op_f2i32:
         case nir_op_f2i64:
         case nir_op_f2u8:
         case nir_op_f2u16:
         case nir_op_f2u32:
         case nir_op_f2u64:
            *integer_uses |= src_mask;
            if (integer_bits_used) {
               integer_bits_used[src0_comp] |=
                  nir_def_bits_used(nir_get_scalar(&alu->def, i));
            }
            break;

         case nir_op_ftrunc:
         case nir_op_ffloor:
            *integer_uses |= src_mask;
            if (integer_bits_used)
               integer_bits_used[src0_comp] = ~0ull;
            break;

         default:
            *float_uses |= src_mask;
            break;
         }
      }
   }
}
