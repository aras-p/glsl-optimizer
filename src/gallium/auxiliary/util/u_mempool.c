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

#include "util/u_mempool.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include <stdio.h>

#define UTIL_MEMPOOL_MAGIC 0xcafe4321

/* The block is either allocated memory or free space. */
struct util_mempool_block {
   /* The header. */
   /* The first next free block. */
   struct util_mempool_block *next_free;

   intptr_t magic;

   /* Memory after the last member is dedicated to the block itself.
    * The allocated size is always larger than this structure. */
};

static struct util_mempool_block *
util_mempool_get_block(struct util_mempool *pool,
                       struct util_mempool_page *page, unsigned index)
{
   return (struct util_mempool_block*)
          ((uint8_t*)page + sizeof(struct util_mempool_page) +
           (pool->block_size * index));
}

static void util_mempool_add_new_page(struct util_mempool *pool)
{
   struct util_mempool_page *page;
   struct util_mempool_block *block;
   int i;

   page = MALLOC(pool->page_size);
   insert_at_tail(&pool->list, page);

   /* Mark all blocks as free. */
   for (i = 0; i < pool->num_blocks-1; i++) {
      block = util_mempool_get_block(pool, page, i);
      block->next_free = util_mempool_get_block(pool, page, i+1);
      block->magic = UTIL_MEMPOOL_MAGIC;
   }

   block = util_mempool_get_block(pool, page, pool->num_blocks-1);
   block->next_free = pool->first_free;
   block->magic = UTIL_MEMPOOL_MAGIC;
   pool->first_free = util_mempool_get_block(pool, page, 0);
   pool->num_pages++;

#if 0
   fprintf(stderr, "New page! Num of pages: %i\n", pool->num_pages);
#endif
}

static void *util_mempool_malloc_st(struct util_mempool *pool)
{
   struct util_mempool_block *block;

   if (!pool->first_free)
      util_mempool_add_new_page(pool);

   block = pool->first_free;
   assert(block->magic == UTIL_MEMPOOL_MAGIC);
   pool->first_free = block->next_free;

   return (uint8_t*)block + sizeof(struct util_mempool_block);
}

static void util_mempool_free_st(struct util_mempool *pool, void *ptr)
{
   struct util_mempool_block *block =
         (struct util_mempool_block*)
         ((uint8_t*)ptr - sizeof(struct util_mempool_block));

   assert(block->magic == UTIL_MEMPOOL_MAGIC);
   block->next_free = pool->first_free;
   pool->first_free = block;
}

static void *util_mempool_malloc_mt(struct util_mempool *pool)
{
   void *mem;

   pipe_mutex_lock(pool->mutex);
   mem = util_mempool_malloc_st(pool);
   pipe_mutex_unlock(pool->mutex);
   return mem;
}

static void util_mempool_free_mt(struct util_mempool *pool, void *ptr)
{
   pipe_mutex_lock(pool->mutex);
   util_mempool_free_st(pool, ptr);
   pipe_mutex_unlock(pool->mutex);
}

void util_mempool_set_thread_safety(struct util_mempool *pool,
                                    enum util_mempool_threading threading)
{
   pool->threading = threading;

   if (threading) {
      pool->malloc = util_mempool_malloc_mt;
      pool->free = util_mempool_free_mt;
   } else {
      pool->malloc = util_mempool_malloc_st;
      pool->free = util_mempool_free_st;
   }
}

void util_mempool_create(struct util_mempool *pool,
                         unsigned item_size,
                         unsigned num_blocks,
                         enum util_mempool_threading threading)
{
   item_size = align(item_size, sizeof(intptr_t));

   pool->num_pages = 0;
   pool->num_blocks = num_blocks;
   pool->block_size = sizeof(struct util_mempool_block) + item_size;
   pool->block_size = align(pool->block_size, sizeof(intptr_t));
   pool->page_size = sizeof(struct util_mempool_page) +
                     num_blocks * pool->block_size;
   pool->first_free = NULL;

   make_empty_list(&pool->list);

   pipe_mutex_init(pool->mutex);

   util_mempool_set_thread_safety(pool, threading);
}

void util_mempool_destroy(struct util_mempool *pool)
{
   struct util_mempool_page *page, *temp;

   foreach_s(page, temp, &pool->list) {
      remove_from_list(page);
      FREE(page);
   }

   pipe_mutex_destroy(pool->mutex);
}
