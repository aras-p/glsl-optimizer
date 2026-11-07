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

#include "ralloc.h"
#include "u_range_remap.h"

/* Binary search for the range that contains n */
static struct range_entry *
get_range_entry(unsigned n, const struct range_remap *r_remap)
{
   if (!r_remap->sorted_array)
      return NULL;

   unsigned low = 0;
   unsigned high = r_remap->sorted_array_length - 1;
   unsigned mid = (low + high) / 2;

   struct range_entry *mid_entry = &r_remap->sorted_array[mid];

   while (low <= high) {
      if (n < mid_entry->start) {
         if (low == high || mid == low) {
            /* No entry found for n */
            return NULL;
         }

         high = mid - 1;
         mid = (low + high) / 2;

         /* Move backward to new_mid */
         mid_entry = &r_remap->sorted_array[mid];
      } else if (n > mid_entry->end) {
         if (low == high || mid == high) {
            /* No entry found for n */
            return NULL;
         }

         low = mid + 1;
         mid = (low + high) / 2;

         /* Move forward to new_mid */
         mid_entry = &r_remap->sorted_array[mid];
      } else {
         /* n is within the current range */
         return mid_entry;
      }
   }

   return NULL;
}

/* Insert a new range entry or if ptr is non-null update an existing entries
 * pointer value if start and end match exactly. If the range overlaps an
 * existing entry we return NULL or if start and end match an entry exactly
 * but ptr is null we return the existing entry.
 *
 * If allow_range_truncation is true this function will match a range if the
 * starting values match and the end fits within and existing range. If ptr
 * param is non-null the range end will be truncated to the new end value and
 * the entry ptr value will be updated.
 */
struct range_entry *
util_range_insert_remap(unsigned start, unsigned end,
                        struct range_remap *r_remap, void *ptr,
                        bool allow_range_truncation)
{
   struct list_head *r_list = &r_remap->r_list;
   struct list_range_entry *lre = NULL;
   if (list_is_empty(r_list)) {
      lre = rzalloc(r_remap->list_mem_ctx, struct list_range_entry);
      list_addtail(&lre->node, r_list);
      goto insert_end;
   }

   /* Shortcut for consecutive location inserts */
   struct list_range_entry *last_entry =
      list_last_entry(r_list, struct list_range_entry, node);
   if (last_entry->entry.end < start) {
      lre = rzalloc(r_remap->list_mem_ctx, struct list_range_entry);
      list_addtail(&lre->node, r_list);
      goto insert_end;
   }

   unsigned low = 0;
   unsigned high = list_length(r_list) - 1;
   unsigned mid = (low + high) / 2;

   struct list_range_entry *mid_entry =
      list_first_entry(r_list, struct list_range_entry, node);
   unsigned i = 0;
   while (i < mid) {
      mid_entry =
         list_entry(mid_entry->node.next, struct list_range_entry, node);
      i++;
   }

   while (low <= high) {
      if (end < mid_entry->entry.start) {
         if (low == high || mid == low) {
            lre = rzalloc(r_remap->list_mem_ctx, struct list_range_entry);
            list_addtail(&lre->node, &mid_entry->node); /* insert before mid */
            goto insert_end;
         }

         high = mid - 1;
         unsigned new_mid = (low + high) / 2;
         while (mid > new_mid) {
            mid_entry =
               list_entry(mid_entry->node.prev, struct list_range_entry, node);
            mid--;
         }
      } else if (start > mid_entry->entry.end) {
         if (low == high || mid == high) {
            lre = rzalloc(r_remap->list_mem_ctx, struct list_range_entry);
            list_add(&lre->node, &mid_entry->node); /* insert after mid */
            goto insert_end;
         }

         low = mid + 1;
         unsigned new_mid = (low + high) / 2;
         while (mid < new_mid) {
            mid_entry =
               list_entry(mid_entry->node.next, struct list_range_entry, node);
            mid++;
         }
      } else if (mid_entry->entry.start == start &&
                 (mid_entry->entry.end == end || (allow_range_truncation && mid_entry->entry.end > end))) {
         if (!ptr)
            return &mid_entry->entry;

         lre = mid_entry;
         goto insert_end;
      } else {
         /* Attempting to insert an entry that overlaps an existing range */
         return NULL;
      }
   }

insert_end:
   lre->entry.start = start;
   lre->entry.end = end;
   lre->entry.ptr = ptr;

   return &lre->entry;
}

void
util_range_switch_to_sorted_array(struct range_remap *r_remap)
{
   r_remap->sorted_array_length = list_length(&r_remap->r_list);

   if (r_remap->sorted_array) {
      ralloc_free(r_remap->sorted_array);
      r_remap->sorted_array = NULL;
   }

   if (r_remap->sorted_array_length == 0)
      return;

   r_remap->sorted_array = rzalloc_array(r_remap, struct range_entry,
                                         r_remap->sorted_array_length);

   unsigned i = 0;
   list_for_each_entry(struct list_range_entry, e, &r_remap->r_list, node) {
      r_remap->sorted_array[i].start = e->entry.start;
      r_remap->sorted_array[i].end = e->entry.end;
      r_remap->sorted_array[i].ptr = e->entry.ptr;
      i++;
   }

   /* Free linked list and reset head */
   list_inithead(&r_remap->r_list);
   ralloc_free(r_remap->list_mem_ctx);
   r_remap->list_mem_ctx = ralloc_context(r_remap);
}

/* Return the range entry that maps to n or NULL if no match found. */
struct range_entry *
util_range_remap(unsigned n, const struct range_remap *r_remap)
{
   return get_range_entry(n, r_remap);
}

struct range_remap *
util_create_range_remap()
{
   struct range_remap *r = rzalloc(NULL, struct range_remap);
   list_inithead(&r->r_list);
   r->list_mem_ctx = ralloc_context(r);
   return r;
}

/* Free previous list and create a new empty list */
struct range_remap *
util_reset_range_remap(struct range_remap *r_remap)
{
   ralloc_free(r_remap);

   return util_create_range_remap();
}
