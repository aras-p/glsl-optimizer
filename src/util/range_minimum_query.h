/*
 * Copyright 2025 Valve Corporation
 * SPDX-License-Identifier: MIT
 */

#ifndef RANGE_MINIMUM_QUERY_H
#define RANGE_MINIMUM_QUERY_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Find the smallest integer in a portion of an array.
 *
 * We use the simple RMQ algorithm that uses O(n log n) preprcessing time and
 * O(1) query time (see eg. Bender and Colton section 3).
 *
 * Bender, M.A., Farach-Colton, M. (2000). The LCA Problem Revisited. In:
 *     Gonnet, G.H., Viola, A. (eds) LATIN 2000: Theoretical Informatics. LATIN
 *     2000. Lecture Notes in Computer Science, vol 1776. Springer, Berlin,
 *     Heidelberg. https://doi.org/10.1007/10719839_9
 */

struct range_minimum_query_table {
   uint32_t *table;
   uint32_t width, height;
};

static inline void
range_minimum_query_table_init(struct range_minimum_query_table *table)
{
   memset((void*) table, 0, sizeof(*table));
}

void
range_minimum_query_table_resize(struct range_minimum_query_table *table,
                                 void *mem_ctx, uint32_t width);

/**
 * Perform preprocessing on the table to ready it for queries.
 *
 * Takes O(n log n) time.
 */
void
range_minimum_query_table_preprocess(struct range_minimum_query_table *table);

/**
 * Find the smallest value in the array among indices in the half-open interval
 * [left_idx, right_idx).
 *
 * Takes O(1) time.
 */
uint32_t
range_minimum_query(struct range_minimum_query_table *const table,
                    uint32_t left_idx, uint32_t right_idx);

#ifdef __cplusplus
}
#endif

#endif
