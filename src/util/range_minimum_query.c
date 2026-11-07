/*
 * Copyright 2025 Valve Corporation
 * SPDX-License-Identifier: MIT
 */

#include "range_minimum_query.h"

#include "macros.h"
#include "ralloc.h"
#include "u_math.h"

void
range_minimum_query_table_resize(struct range_minimum_query_table *table,
                                 void *mem_ctx, uint32_t width)
{
   const uint64_t height = util_logbase2_64(width) + 1;
   const uint64_t size = width * height;
   assert(size < UINT32_MAX);

   table->table = reralloc_array_size(mem_ctx, table->table,
                                      sizeof(uint32_t), size);
   table->width = width;
   table->height = height;
}

static void
elementwise_minimum(uint32_t *restrict out,
                    uint32_t *const a,
                    uint32_t *const b,
                    uint32_t count)
{
   for (uint32_t i = 0; i < count; i++) {
      out[i] = MIN2(a[i], b[i]);
   }
}

/**
 * For a given level (row) of the table, how many input values is the
 * minimum computed over?
 *
 * Each row of the table has (table->width - rmq_distance(level) + 1)
 * valid elements.
 */
static uint32_t
rmq_distance(int32_t level)
{
   return 1 << level;
}

void
range_minimum_query_table_preprocess(struct range_minimum_query_table *table)
{
   for (uint32_t i = 1; i < table->height; i++) {
      uint32_t in_distance = rmq_distance(i - 1);
      uint32_t out_distance = rmq_distance(i);
      uint32_t *in_row = table->table + table->width * (i - 1);
      uint32_t *out_row = table->table + table->width * i;
      /*
       * This reads elements [0, x) from in_row, where x is:
       *    in_distance + table->width - out_distance + 1
       *    in_distance + table->width - (2 * in_distance) + 1
       *    table->width - in_distance + 1
       * which is the number of valid elements in in_row
       */
      elementwise_minimum(out_row, in_row, in_row + in_distance,
                          table->width - out_distance + 1);
   }
}

uint32_t
range_minimum_query(struct range_minimum_query_table *const table,
                    uint32_t left_idx, uint32_t right_idx)
{
   assert(left_idx < right_idx);
   assert(right_idx <= table->width);
   const uint32_t distance = right_idx - left_idx;

   uint32_t level = util_logbase2(distance);
   assert(rmq_distance(level) <= distance);
   assert(distance < 2 * rmq_distance(level));
   assert(level < table->height);

   /*
    * Since right_idx <= table->width by precondition, we know
    *    right_idx - rmq_distance(level) <= table->width - rmq_distance(level)
    *    right_idx - rmq_distance(level) < table->width - rmq_distance(level) + 1
    * which means that the read for `right` is in bounds.
    *
    * The read for `left` is then in bounds because
    *    left_idx == right_idx - width <= right_idx - rmq_distance(level)
    */
   uint32_t *const row = table->table + table->width * level;
   uint32_t left = row[left_idx];
   uint32_t right = row[right_idx - rmq_distance(level)];
   return MIN2(left, right);
}
