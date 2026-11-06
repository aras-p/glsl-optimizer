/*
 * Copyright 2025 Valve Corporation
 * SPDX-License-Identifier: MIT
 */

#include "nir.h"

/**
 * Find the lowest common ancestor in the dominance tree.
 *
 * We reduce the LCA problem to range minimum query using the standard euler
 * tour method (see eg. Bender and Colton section 2). From there, we use the
 * simple RMQ algorithm that uses O(n log n) preprcessing time and O(1) query
 * time (Bender and Colton section 3).
 *
 * As a slight modification, we store the block index instead of the block
 * depth. We can do this because the lower tree depth is always at a lower block
 * index and we use an RMQ algorithm that doesn't rely on the -1/+1 property.
 *
 * Bender, M.A., Farach-Colton, M. (2000). The LCA Problem Revisited. In:
 *     Gonnet, G.H., Viola, A. (eds) LATIN 2000: Theoretical Informatics. LATIN
 *     2000. Lecture Notes in Computer Science, vol 1776. Springer, Berlin,
 *     Heidelberg. https://doi.org/10.1007/10719839_9
 */

static void
realloc_info(nir_function_impl *impl)
{
   struct nir_dom_lca_info *info = &impl->dom_lca_info;
   const uint32_t euler_tour_size = impl->num_blocks * 2 - 1;

   void *mem_ctx = ralloc_parent(impl);
   range_minimum_query_table_resize(&info->table, mem_ctx, euler_tour_size);
   info->block_from_idx = reralloc_array_size(mem_ctx, info->block_from_idx,
                                              sizeof(nir_block *),
                                              impl->num_blocks);
}

static uint32_t
dom_lca_representative(nir_block *block)
{
   /* The dom_pre_index is 1-indexed so we need to subtract one to match our
    * indices
    */
   return block->dom_pre_index - 1;
}

static void
generate_euler_tour(nir_function_impl *impl)
{
   uint32_t *table = impl->dom_lca_info.table.table;
   nir_block **block_from_idx = impl->dom_lca_info.block_from_idx;
   if (impl->num_blocks == 1) {
      nir_block *block = nir_start_block(impl);
      table[0] = 0;
      block_from_idx[0] = block;
      return;
   }

   /* By definition, the first row of the table contains range minimum query
    * lookups for each single-element block, meaning it is just the array that
    * we will perform RMQs on. Therefore, when generating the Euler tour, we
    * store results in the first row and are free to use the rest of the table
    * as scratch memory for the depth-first search.
    *
    * The stack contains the index of the node's next child to visit.
    */
   assert(impl->dom_lca_info.table.height >= 2);
   STATIC_ASSERT(sizeof(uint32_t) <= sizeof(nir_block *));
   uint32_t *dfs_stack = (uint32_t *)&table[impl->dom_lca_info.table.width];

   nir_block *cur_block = nir_start_block(impl);
   uint32_t *cur_stack = dfs_stack;

   bool first_visit = true;
   uint32_t i;
   for (i = 0; i < impl->dom_lca_info.table.width; i++) {
      if (cur_block == NULL) {
         /* This can happen earlier than expected if some blocks are
          * unreachable
          */
         break;
      }

      assert(cur_stack >= dfs_stack);
      table[i] = cur_block->index;

      if (first_visit) {
         /* First visit. Place it on the stack. */
         *cur_stack = 0;
         assert(i == dom_lca_representative(cur_block));
         block_from_idx[cur_block->index] = cur_block;
      }

      if (*cur_stack < cur_block->num_dom_children) {
         cur_block = cur_block->dom_children[*cur_stack];
         *cur_stack += 1;
         cur_stack += 1;
         first_visit = true;
      } else {
         assert(*cur_stack == cur_block->num_dom_children);
         cur_block = cur_block->imm_dom;
         cur_stack -= 1;
         first_visit = false;
      }
   }

   assert(cur_block == NULL);

   if (i != impl->dom_lca_info.table.width) {
      void *mem_ctx = ralloc_parent(impl);
      range_minimum_query_table_resize(&impl->dom_lca_info.table, mem_ctx, i);
   }
}

void
nir_calc_dominance_lca_impl(nir_function_impl *impl)
{
   if (impl->valid_metadata & nir_metadata_dominance_lca)
      return;

   nir_metadata_require(impl, nir_metadata_block_index |
                                 nir_metadata_dominance);

   realloc_info(impl);
   generate_euler_tour(impl);
   range_minimum_query_table_preprocess(&impl->dom_lca_info.table);
}

static nir_block *
block_return_if_reachable(nir_block *b)
{
   return (b && nir_block_is_reachable(b)) ? b : NULL;
}

static bool
is_lca(nir_block *result, nir_block *b1, nir_block *b2)
{
   if (!nir_block_dominates(result, b1) || !nir_block_dominates(result, b2))
      return false;

   for (int i = 0; i < result->num_dom_children; i++) {
      nir_block *child = result->dom_children[i];
      if (nir_block_dominates(child, b1) &&
          nir_block_dominates(child, b2))
         return false;
   }

   return true;
}

nir_block *
nir_dominance_lca(nir_block *b1, nir_block *b2)
{
   if (b1 == NULL || !nir_block_is_reachable(b1))
      return block_return_if_reachable(b2);

   if (b2 == NULL || !nir_block_is_reachable(b2))
      return block_return_if_reachable(b1);

   assert(b1->impl == b2->impl);

   nir_function_impl *impl = b1->impl;
   assert(impl->valid_metadata & nir_metadata_dominance_lca);

   uint32_t i1 = dom_lca_representative(b1);
   uint32_t i2 = dom_lca_representative(b2);
   if (i1 > i2)
      SWAP(i1, i2);
   uint32_t index = range_minimum_query(&impl->dom_lca_info.table, i1, i2 + 1);
   nir_block *result = impl->dom_lca_info.block_from_idx[index];

   assert(is_lca(result, b1, b2));

   return result;
}
