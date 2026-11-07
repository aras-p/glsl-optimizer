/*
 * Copyright © 2025 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _UTIL_SPARSE_BITSET_H
#define _UTIL_SPARSE_BITSET_H

#include "rb_tree.h"
#include "bitset.h"
#include "ralloc.h"

#define U_SPARSE_BITSET_LOG2_BITS_PER_NODE 10
#define U_SPARSE_BITSET_BITS_PER_NODE      (1u << U_SPARSE_BITSET_LOG2_BITS_PER_NODE)
#define U_SPARSE_BITSET_BIT_INDEX_MASK     (U_SPARSE_BITSET_BITS_PER_NODE - 1)
#define U_SPARSE_BITSET_OFFSET_MASK        (~U_SPARSE_BITSET_BIT_INDEX_MASK)

/* Sets under this # of bits use small-set optimization */
#define U_SPARSE_BITSET_SMALL_SET_THRESHOLD 0

struct u_sparse_bitset_node {
   struct rb_node node;

   /* The first bit covered by this node */
   unsigned offset;
   BITSET_DECLARE(vals, U_SPARSE_BITSET_BITS_PER_NODE);
};

#define to_u_sparse_bitset_node(rb_node)                                       \
   rb_node_data(struct u_sparse_bitset_node, rb_node, node)

/*
 * sparse_bitset wraps around a rb_tree of bitset nodes.
 *
 * Using sparse_bitset over a regular bitset is advantageous when you have a
 * large number of potentially-set bits, but expect most of them to be zero
 * (with the set bits mostly being within small, scattered regions).
 *
 * By default, bits are assumed to be unset. Areas that have set bits are
 * represented by nodes in the rb_tree. One node represents a fixed-size bit
 * range (internally with a non-sparse bitset of U_SPARSE_BITSET_BITS_PER_NODE).
 */
struct u_sparse_bitset {
   union {
      /* Large set - rb_tree of bitset nodes */
      struct {
         void *mem_ctx;
         struct rb_tree tree;
      };

      /* Small set optimization - bitset on heap */
      BITSET_WORD *vals;
   };

   /* Capacity of a small set, or 0 to indicate a large set */
   unsigned capacity;
};

static inline bool
_u_sparse_bitset_is_small(const struct u_sparse_bitset *s)
{
   return s->capacity != 0;
}

static inline void
u_sparse_bitset_init(struct u_sparse_bitset *s, unsigned capacity,
                     void *mem_ctx)
{
   if (capacity && capacity < U_SPARSE_BITSET_SMALL_SET_THRESHOLD) {
      s->vals = BITSET_RZALLOC(mem_ctx, capacity);
      s->capacity = capacity;
   } else {
      rb_tree_init(&s->tree);
      s->mem_ctx = mem_ctx;
      s->capacity = 0;
   }
}

static inline void
u_sparse_bitset_clear_all(struct u_sparse_bitset *s)
{
   if (_u_sparse_bitset_is_small(s)) {
      memset(s->vals, 0, BITSET_BYTES(s->capacity));
   } else {
      rb_tree_foreach_safe(struct u_sparse_bitset_node, node, &s->tree, node) {
         rb_tree_remove(&s->tree, &node->node);
         ralloc_free(node);
      }
   }
}

static inline int
_u_sparse_bitset_node_comparator(const struct rb_node *a, unsigned offset_b)
{
   unsigned offset_a = to_u_sparse_bitset_node(a)->offset;
   return (offset_a < offset_b) - (offset_a > offset_b);
}

static inline int
_u_sparse_bitset_node_compare(const struct rb_node *a, const struct rb_node *b)
{
   unsigned offs_b = to_u_sparse_bitset_node(b)->offset;
   return _u_sparse_bitset_node_comparator(a, offs_b);
}

static inline int
_u_sparse_bitset_node_search(const struct rb_node *node, const void *key)
{
   return _u_sparse_bitset_node_comparator(node, (unsigned)(uintptr_t)key);
}

static inline struct u_sparse_bitset_node *
_u_sparse_bitset_get_node(struct u_sparse_bitset *s, unsigned offset)
{
   assert(!_u_sparse_bitset_is_small(s));

   struct rb_node *node = rb_tree_search(&s->tree, (void *)(uintptr_t)offset,
                                         _u_sparse_bitset_node_search);
   if (!node)
      return NULL;

   return rb_node_data(struct u_sparse_bitset_node, node, node);
}

static inline struct u_sparse_bitset_node *
_u_sparse_bitset_get_or_add_node(struct u_sparse_bitset *s, unsigned offset)
{
   assert(!_u_sparse_bitset_is_small(s));
   assert((offset & U_SPARSE_BITSET_BIT_INDEX_MASK) == 0);

   struct u_sparse_bitset_node *node = _u_sparse_bitset_get_node(s, offset);
   if (!node) {
      node = rzalloc(s->mem_ctx, struct u_sparse_bitset_node);
      node->offset = offset;
      rb_tree_insert(&s->tree, &node->node, _u_sparse_bitset_node_compare);
   }

   return node;
}

static inline void
u_sparse_bitset_set(struct u_sparse_bitset *s, unsigned bit)
{
   if (_u_sparse_bitset_is_small(s)) {
      assert(bit < s->capacity);
      BITSET_SET(s->vals, bit);
   } else {
      struct u_sparse_bitset_node *node =
         _u_sparse_bitset_get_or_add_node(s, bit & U_SPARSE_BITSET_OFFSET_MASK);

      BITSET_SET(node->vals, bit & U_SPARSE_BITSET_BIT_INDEX_MASK);
   }
}

static inline void
u_sparse_bitset_clear(struct u_sparse_bitset *s, unsigned bit)
{
   if (_u_sparse_bitset_is_small(s)) {
      assert(bit < s->capacity);
      BITSET_CLEAR(s->vals, bit);
   } else {
      struct u_sparse_bitset_node *node =
         _u_sparse_bitset_get_node(s, bit & U_SPARSE_BITSET_OFFSET_MASK);

      if (node)
         BITSET_CLEAR(node->vals, bit & U_SPARSE_BITSET_BIT_INDEX_MASK);
   }
}

static inline bool
u_sparse_bitset_test(struct u_sparse_bitset *s, unsigned bit)
{
   if (_u_sparse_bitset_is_small(s)) {
      assert(bit < s->capacity);
      return BITSET_TEST(s->vals, bit);
   } else {
      struct u_sparse_bitset_node *node =
         _u_sparse_bitset_get_node(s, bit & U_SPARSE_BITSET_OFFSET_MASK);

      return node != NULL &&
             BITSET_TEST(node->vals, bit & U_SPARSE_BITSET_BIT_INDEX_MASK);
   }
}

static inline int
u_sparse_bitset_cmp(struct u_sparse_bitset *a, struct u_sparse_bitset *b)
{
   assert(a->capacity == b->capacity);

   if (_u_sparse_bitset_is_small(a)) {
      return memcmp(a->vals, b->vals, BITSET_BYTES(a->capacity));
   }

   struct rb_node *a_iter = rb_tree_first(&a->tree);
   struct rb_node *b_iter = rb_tree_first(&b->tree);

   while (a_iter && b_iter) {
      struct u_sparse_bitset_node *node_a = to_u_sparse_bitset_node(a_iter);
      struct u_sparse_bitset_node *node_b = to_u_sparse_bitset_node(b_iter);

      int node_cmp = _u_sparse_bitset_node_compare(a_iter, b_iter);
      if (node_cmp)
         return node_cmp;

      int cmp_res = memcmp(node_a->vals, node_b->vals, sizeof(node_a->vals));
      if (cmp_res)
         return cmp_res;

      a_iter = rb_node_next(a_iter);
      b_iter = rb_node_next(b_iter);
   }

   return (a_iter != NULL) - (b_iter != NULL);
}

static inline void
u_sparse_bitset_dup_with_ctx(struct u_sparse_bitset *dst,
                             struct u_sparse_bitset *src, void *mem_ctx)
{
   u_sparse_bitset_init(dst, src->capacity, mem_ctx);

   if (_u_sparse_bitset_is_small(src)) {
      memcpy(dst->vals, src->vals, BITSET_BYTES(src->capacity));
      return;
   }

   rb_tree_foreach(struct u_sparse_bitset_node, node, &src->tree, node) {
      if (BITSET_IS_EMPTY(node->vals))
         continue;

      struct u_sparse_bitset_node *dst_node =
         (struct u_sparse_bitset_node *)ralloc_memdup(
            mem_ctx, node, sizeof(struct u_sparse_bitset_node));

      rb_tree_insert(&dst->tree, &dst_node->node,
                     _u_sparse_bitset_node_compare);
   }
}

static inline void
u_sparse_bitset_dup(struct u_sparse_bitset *dst, struct u_sparse_bitset *src)
{
   u_sparse_bitset_dup_with_ctx(dst, src, src->mem_ctx);
}

static inline bool
_u_bitset_merge(BITSET_WORD *dst, const BITSET_WORD *src, unsigned words)
{
   bool changed = false;

   for (unsigned i = 0; i < words; i++) {
      changed |= (bool)(src[i] & ~dst[i]);
      dst[i] |= src[i];
   }

   return changed;
}

static inline bool
u_sparse_bitset_merge(struct u_sparse_bitset *dst, struct u_sparse_bitset *src)
{
   bool changed = false;
   assert(dst->capacity == src->capacity);

   if (_u_sparse_bitset_is_small(src)) {
      return _u_bitset_merge(dst->vals, src->vals, BITSET_WORDS(src->capacity));
   }

   rb_tree_foreach(struct u_sparse_bitset_node, node, &src->tree, node) {
      if (!BITSET_IS_EMPTY(node->vals)) {
         struct u_sparse_bitset_node *dst_node =
            _u_sparse_bitset_get_or_add_node(dst, node->offset);

         changed |=
            _u_bitset_merge(dst_node->vals, node->vals, ARRAY_SIZE(node->vals));
      }
   }

   return changed;
}

static inline unsigned
u_sparse_bitset_count(struct u_sparse_bitset *s)
{
   if (_u_sparse_bitset_is_small(s)) {
      return __bitset_count(s->vals, s->capacity);
   } else {
      unsigned sum = 0;

      rb_tree_foreach_safe(struct u_sparse_bitset_node, node, &s->tree, node) {
         sum += BITSET_COUNT(node->vals);
      }

      return sum;
   }
}

static inline void
u_sparse_bitset_free(struct u_sparse_bitset *s)
{
   if (_u_sparse_bitset_is_small(s)) {
      ralloc_free(s->vals);
   } else {
      rb_tree_foreach_safe(struct u_sparse_bitset_node, node, &s->tree, node) {
         rb_tree_remove(&s->tree, &node->node);
         ralloc_free(node);
      }
   }
}

static inline unsigned
_u_sparse_bitset_next_set_dense(BITSET_WORD *set, unsigned size, unsigned from)
{
   /* Check if there even is a first node */
   if (from >= size) {
      return UINT_MAX;
   }

   unsigned i = BITSET_BITWORD(from);
   unsigned offset = from % BITSET_WORDBITS;

   /* Check for a next bit in the first node */
   if (set[i] >> offset) {
      return from + ffs(set[i] >> offset) - 1;
   }

   /* Else look for the next node */
   for (i++; i < BITSET_WORDS(size); ++i) {
      if (set[i]) {
         return (i * BITSET_WORDBITS) + ffs(set[i]) - 1;
      }
   }

   return UINT_MAX;
}

static inline unsigned
_u_sparse_bitset_next_set(const struct u_sparse_bitset *s, uintptr_t *node_,
                          const unsigned from)
{
   unsigned ret = UINT_MAX;

   if (_u_sparse_bitset_is_small(s)) {
      unsigned i = _u_sparse_bitset_next_set_dense(s->vals, s->capacity, from);
      if (i < s->capacity) {
         ret = i;
      }
   } else {
      unsigned node_from = from;

      /* Look for the next set bit in the current node */
      for (struct rb_node *rb = (struct rb_node *)*node_; rb != NULL;
           rb = rb_node_next(rb), node_from = 0) {

         struct u_sparse_bitset_node *it = to_u_sparse_bitset_node(rb);
         unsigned num = U_SPARSE_BITSET_BITS_PER_NODE;

         /* Deal with from at the end of the node */
         if (node_from != 0 &&
             (node_from - it->offset) >= U_SPARSE_BITSET_BITS_PER_NODE) {
            continue;
         }

         node_from &= U_SPARSE_BITSET_BIT_INDEX_MASK;

         unsigned i = _u_sparse_bitset_next_set_dense(it->vals, num, node_from);

         if (i < num) {
            *node_ = (uintptr_t)rb;
            ret = it->offset + i;
            break;
         }
      }
   }

   assert(from <= ret);
   return ret;
}

static inline unsigned
_u_sparse_bitset_first_set(struct u_sparse_bitset *s, uintptr_t *node_)
{
   uintptr_t node = 0;
   if (!_u_sparse_bitset_is_small(s)) {
      node = (uintptr_t)rb_tree_first(&s->tree);
   }

   unsigned ret = _u_sparse_bitset_next_set(s, &node, 0);
   *node_ = node;
   return ret;
}

#define U_SPARSE_BITSET_FOREACH_SET(s, it)                                     \
   for (uintptr_t _node, it = _u_sparse_bitset_first_set((s), &_node);         \
        it != UINT_MAX; it = _u_sparse_bitset_next_set((s), &_node, it + 1))

#endif //_UTIL_SPARSE_BITSET_H
