/*
 * Copyright © 2025 Valve Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct range_remap {
   /* Linked list of range remap entries */
   struct list_head r_list;

   void *list_mem_ctx;

   /* Sorted array built from the linked list for fast binary searches */
   struct range_entry *sorted_array;
   unsigned sorted_array_length;
};

struct range_entry {
    unsigned start, end;
    void *ptr;
};

struct list_range_entry {
    struct list_head node;
    struct range_entry entry;
};

struct range_entry *
util_range_insert_remap(unsigned start, unsigned end,
                        struct range_remap *r_remap, void *ptr,
                        bool allow_range_truncation);

struct range_entry *
util_range_remap(unsigned n, const struct range_remap *r_remap);

struct range_remap *
util_create_range_remap(void);

struct range_remap *
util_reset_range_remap(struct range_remap *r_remap);

void
util_range_switch_to_sorted_array(struct range_remap *r_remap);

#ifdef __cplusplus
}
#endif
