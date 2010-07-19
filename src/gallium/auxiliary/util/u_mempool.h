/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

/**
 * @file
 * Simple memory pool for equally sized memory allocations.
 * util_mempool_malloc and util_mempool_free are in O(1).
 *
 * Good for allocations which have very low lifetime and are allocated
 * and freed very often. Use a profiler first!
 *
 * Candidates: get_transfer, user_buffer_create
 *
 * @author Marek Olšák
 */

#ifndef U_MEMPOOL_H
#define U_MEMPOOL_H

#include "os/os_thread.h"

enum util_mempool_threading {
   UTIL_MEMPOOL_SINGLETHREADED = FALSE,
   UTIL_MEMPOOL_MULTITHREADED = TRUE
};

/* The page is an array of blocks (allocations). */
struct util_mempool_page {
   /* The header (linked-list pointers). */
   struct util_mempool_page *prev, *next;

   /* Memory after the last member is dedicated to the page itself.
    * The allocated size is always larger than this structure. */
};

struct util_mempool {
   /* Public members. */
   void *(*malloc)(struct util_mempool *pool);
   void (*free)(struct util_mempool *pool, void *ptr);

   /* Private members. */
   struct util_mempool_block *first_free;

   struct util_mempool_page list;

   unsigned block_size;
   unsigned page_size;
   unsigned num_blocks;
   unsigned num_pages;
   enum util_mempool_threading threading;

   pipe_mutex mutex;
};

void util_mempool_create(struct util_mempool *pool,
                         unsigned item_size,
                         unsigned num_blocks,
                         enum util_mempool_threading threading);

void util_mempool_destroy(struct util_mempool *pool);

void util_mempool_set_thread_safety(struct util_mempool *pool,
                                    enum util_mempool_threading threading);

#define util_mempool_malloc(pool)    (pool)->malloc(pool)
#define util_mempool_free(pool, ptr) (pool)->free(pool, ptr)

#endif
