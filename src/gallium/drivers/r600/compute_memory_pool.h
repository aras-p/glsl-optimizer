/*
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Adam Rak <adam.rak@streamnovation.com>
 */

#ifndef COMPUTE_MEMORY_POOL
#define COMPUTE_MEMORY_POOL

#include <stdlib.h>

#define ITEM_MAPPED_FOR_READING (1<<0)
#define ITEM_MAPPED_FOR_WRITING (1<<1)
#define ITEM_FOR_PROMOTING      (1<<2)
#define ITEM_FOR_DEMOTING       (1<<3)

#define POOL_FRAGMENTED (1<<0)

struct compute_memory_pool;

struct compute_memory_item
{
	int64_t id;		/**< ID of the memory chunk */

	uint32_t status;	/**< Will track the status of the item */

	/** Start pointer in dwords relative in the pool bo. If an item
	 * is unallocated, then this value must be -1 to indicate this. */
	int64_t start_in_dw;
	int64_t size_in_dw;	/**< Size of the chunk in dwords */

	/** Intermediate buffer asociated with an item. It is used mainly for mapping
	 * items against it. They are listed in the pool's unallocated list */
	struct r600_resource *real_buffer;

	struct compute_memory_pool* pool;

	struct list_head link;
};

struct compute_memory_pool
{
	int64_t next_id;	/**< For generating unique IDs for memory chunks */
	int64_t size_in_dw;	/**< Size of the pool in dwords */

	struct r600_resource *bo;	/**< The pool buffer object resource */
	struct r600_screen *screen;

	uint32_t *shadow;	/**< host copy of the pool, used for growing the pool */

	uint32_t status;	/**< Status of the pool */

	/** Allocated memory items in the pool, they must be ordered by "start_in_dw" */
	struct list_head *item_list;

	/** Unallocated memory items, this list contains all the items that aren't
	 * yet in the pool */
	struct list_head *unallocated_list;
};


static inline int is_item_in_pool(struct compute_memory_item *item)
{
	return item->start_in_dw != -1;
}

struct compute_memory_pool* compute_memory_pool_new(struct r600_screen *rscreen);

void compute_memory_pool_delete(struct compute_memory_pool* pool);

int64_t compute_memory_prealloc_chunk(struct compute_memory_pool* pool,
	int64_t size_in_dw);

struct list_head *compute_memory_postalloc_chunk(struct compute_memory_pool* pool,
	int64_t start_in_dw);

int compute_memory_grow_defrag_pool(struct compute_memory_pool* pool,
	struct pipe_context *pipe, int new_size_in_dw);

void compute_memory_shadow(struct compute_memory_pool* pool,
	struct pipe_context *pipe, int device_to_host);

int compute_memory_finalize_pending(struct compute_memory_pool* pool,
	struct pipe_context * pipe);

void compute_memory_defrag(struct compute_memory_pool *pool,
	struct pipe_resource *src, struct pipe_resource *dst,
	struct pipe_context *pipe);

int compute_memory_promote_item(struct compute_memory_pool *pool,
	struct compute_memory_item *item, struct pipe_context *pipe,
	int64_t allocated);

void compute_memory_demote_item(struct compute_memory_pool *pool,
	struct compute_memory_item *item, struct pipe_context *pipe);

void compute_memory_move_item(struct compute_memory_pool *pool,
	struct pipe_resource *src, struct pipe_resource *dst,
	struct compute_memory_item *item, uint64_t new_start_in_dw,
	struct pipe_context *pipe);

void compute_memory_free(struct compute_memory_pool* pool, int64_t id);

struct compute_memory_item* compute_memory_alloc(struct compute_memory_pool* pool,
	int64_t size_in_dw);

void compute_memory_transfer(struct compute_memory_pool* pool,
	struct pipe_context * pipe, int device_to_host,
	struct compute_memory_item* chunk, void* data,
	int offset_in_chunk, int size);

void compute_memory_transfer_direct(struct compute_memory_pool* pool,
	int chunk_to_data, struct compute_memory_item* chunk,
	struct r600_resource* data, int offset_in_chunk,
	int offset_in_data, int size);

#endif
