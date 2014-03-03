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

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "util/u_blitter.h"
#include "util/u_double_list.h"
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_framebuffer.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "r600_formats.h"
#include "compute_memory_pool.h"
#include "evergreen_compute.h"
#include "evergreen_compute_internal.h"
#include <inttypes.h>

/**
 * Creates a new pool
 */
struct compute_memory_pool* compute_memory_pool_new(
	struct r600_screen * rscreen)
{
	struct compute_memory_pool* pool = (struct compute_memory_pool*)
				CALLOC(sizeof(struct compute_memory_pool), 1);

	COMPUTE_DBG(rscreen, "* compute_memory_pool_new()\n");

	pool->screen = rscreen;
	return pool;
}

static void compute_memory_pool_init(struct compute_memory_pool * pool,
	unsigned initial_size_in_dw)
{

	COMPUTE_DBG(pool->screen, "* compute_memory_pool_init() initial_size_in_dw = %ld\n",
		initial_size_in_dw);

	pool->shadow = (uint32_t*)CALLOC(initial_size_in_dw, 4);
	pool->next_id = 1;
	pool->size_in_dw = initial_size_in_dw;
	pool->bo = (struct r600_resource*)r600_compute_buffer_alloc_vram(pool->screen,
							pool->size_in_dw * 4);
}

/**
 * Frees all stuff in the pool and the pool struct itself too
 */
void compute_memory_pool_delete(struct compute_memory_pool* pool)
{
	COMPUTE_DBG(pool->screen, "* compute_memory_pool_delete()\n");
	free(pool->shadow);
	if (pool->bo) {
		pool->screen->b.b.resource_destroy((struct pipe_screen *)
			pool->screen, (struct pipe_resource *)pool->bo);
	}
	free(pool);
}

/**
 * Searches for an empty space in the pool, return with the pointer to the
 * allocatable space in the pool, returns -1 on failure.
 */
int64_t compute_memory_prealloc_chunk(
	struct compute_memory_pool* pool,
	int64_t size_in_dw)
{
	struct compute_memory_item *item;

	int last_end = 0;

	assert(size_in_dw <= pool->size_in_dw);

	COMPUTE_DBG(pool->screen, "* compute_memory_prealloc_chunk() size_in_dw = %ld\n",
		size_in_dw);

	for (item = pool->item_list; item; item = item->next) {
		if (item->start_in_dw > -1) {
			if (item->start_in_dw-last_end > size_in_dw) {
				return last_end;
			}

			last_end = item->start_in_dw + item->size_in_dw;
			last_end += (1024 - last_end % 1024);
		}
	}

	if (pool->size_in_dw - last_end < size_in_dw) {
		return -1;
	}

	return last_end;
}

/**
 *  Search for the chunk where we can link our new chunk after it.
 */
struct compute_memory_item* compute_memory_postalloc_chunk(
	struct compute_memory_pool* pool,
	int64_t start_in_dw)
{
	struct compute_memory_item* item;

	COMPUTE_DBG(pool->screen, "* compute_memory_postalloc_chunck() start_in_dw = %ld\n",
		start_in_dw);

	/* Check if we can insert it in the front of the list */
	if (pool->item_list && pool->item_list->start_in_dw > start_in_dw) {
		return NULL;
	}

	for (item = pool->item_list; item; item = item->next) {
		if (item->next) {
			if (item->start_in_dw < start_in_dw
				&& item->next->start_in_dw > start_in_dw) {
				return item;
			}
		}
		else {
			/* end of chain */
			assert(item->start_in_dw < start_in_dw);
			return item;
		}
	}

	assert(0 && "unreachable");
	return NULL;
}

/**
 * Reallocates pool, conserves data
 */
void compute_memory_grow_pool(struct compute_memory_pool* pool,
	struct pipe_context * pipe, int new_size_in_dw)
{
	COMPUTE_DBG(pool->screen, "* compute_memory_grow_pool() "
		"new_size_in_dw = %d (%d bytes)\n",
		new_size_in_dw, new_size_in_dw * 4);

	assert(new_size_in_dw >= pool->size_in_dw);

	if (!pool->bo) {
		compute_memory_pool_init(pool, MAX2(new_size_in_dw, 1024 * 16));
	} else {
		new_size_in_dw += 1024 - (new_size_in_dw % 1024);

		COMPUTE_DBG(pool->screen, "  Aligned size = %d (%d bytes)\n",
			new_size_in_dw, new_size_in_dw * 4);

		compute_memory_shadow(pool, pipe, 1);
		pool->shadow = realloc(pool->shadow, new_size_in_dw*4);
		pool->size_in_dw = new_size_in_dw;
		pool->screen->b.b.resource_destroy(
			(struct pipe_screen *)pool->screen,
			(struct pipe_resource *)pool->bo);
		pool->bo = (struct r600_resource*)r600_compute_buffer_alloc_vram(
							pool->screen,
							pool->size_in_dw * 4);
		compute_memory_shadow(pool, pipe, 0);
	}
}

/**
 * Copy pool from device to host, or host to device.
 */
void compute_memory_shadow(struct compute_memory_pool* pool,
	struct pipe_context * pipe, int device_to_host)
{
	struct compute_memory_item chunk;

	COMPUTE_DBG(pool->screen, "* compute_memory_shadow() device_to_host = %d\n",
		device_to_host);

	chunk.id = 0;
	chunk.start_in_dw = 0;
	chunk.size_in_dw = pool->size_in_dw;
	chunk.prev = chunk.next = NULL;
	compute_memory_transfer(pool, pipe, device_to_host, &chunk,
				pool->shadow, 0, pool->size_in_dw*4);
}

/**
 * Allocates pending allocations in the pool
 */
void compute_memory_finalize_pending(struct compute_memory_pool* pool,
	struct pipe_context * pipe)
{
	struct compute_memory_item *pending_list = NULL, *end_p = NULL;
	struct compute_memory_item *item, *next;

	int64_t allocated = 0;
	int64_t unallocated = 0;

	int64_t start_in_dw = 0;

	COMPUTE_DBG(pool->screen, "* compute_memory_finalize_pending()\n");

	for (item = pool->item_list; item; item = item->next) {
		COMPUTE_DBG(pool->screen, "  + list: offset = %i id = %i size = %i "
			"(%i bytes)\n",item->start_in_dw, item->id,
			item->size_in_dw, item->size_in_dw * 4);
	}

	/* Search through the list of memory items in the pool */
	for (item = pool->item_list; item; item = next) {
		next = item->next;

		/* Check if the item is pending. */
		if (item->start_in_dw == -1) {
			/* It is pending, so add it to the pending_list... */
			if (end_p) {
				end_p->next = item;
			}
			else {
				pending_list = item;
			}

			/* ... and then remove it from the item list. */
			if (item->prev) {
				item->prev->next = next;
			}
			else {
				pool->item_list = next;
			}

			if (next) {
				next->prev = item->prev;
			}

			/* This sequence makes the item be at the end of the list */
			item->prev = end_p;
			item->next = NULL;
			end_p = item;

			/* Update the amount of space we will need to allocate. */
			unallocated += item->size_in_dw+1024;
		}
		else {
			/* The item is not pendng, so update the amount of space
			 * that has already been allocated. */
			allocated += item->size_in_dw;
		}
	}

	/* If we require more space than the size of the pool, then grow the
	 * pool.
	 *
	 * XXX: I'm pretty sure this won't work.  Imagine this scenario:
	 *
	 * Offset Item Size
	 *   0    A    50
	 * 200    B    50
	 * 400    C    50
	 *
	 * Total size = 450
	 * Allocated size = 150
	 * Pending Item D Size = 200
	 *
	 * In this case, there are 300 units of free space in the pool, but
	 * they aren't contiguous, so it will be impossible to allocate Item D.
	 */
	if (pool->size_in_dw < allocated+unallocated) {
		compute_memory_grow_pool(pool, pipe, allocated+unallocated);
	}

	/* Loop through all the pending items, allocate space for them and
	 * add them back to the item_list. */
	for (item = pending_list; item; item = next) {
		next = item->next;

		/* Search for free space in the pool for this item. */
		while ((start_in_dw=compute_memory_prealloc_chunk(pool,
						item->size_in_dw)) == -1) {
			int64_t need = item->size_in_dw+2048 -
						(pool->size_in_dw - allocated);

			need += 1024 - (need % 1024);

			if (need > 0) {
				compute_memory_grow_pool(pool,
						pipe,
						pool->size_in_dw + need);
			}
			else {
				need = pool->size_in_dw / 10;
				need += 1024 - (need % 1024);
				compute_memory_grow_pool(pool,
						pipe,
						pool->size_in_dw + need);
			}
		}
		COMPUTE_DBG(pool->screen, "  + Found space for Item %p id = %u "
			"start_in_dw = %u (%u bytes) size_in_dw = %u (%u bytes)\n",
			item, item->id, start_in_dw, start_in_dw * 4,
			item->size_in_dw, item->size_in_dw * 4);

		item->start_in_dw = start_in_dw;
		item->next = NULL;
		item->prev = NULL;

		if (pool->item_list) {
			struct compute_memory_item *pos;

			pos = compute_memory_postalloc_chunk(pool, start_in_dw);
			if (pos) {
				item->prev = pos;
				item->next = pos->next;
				pos->next = item;
				if (item->next) {
					item->next->prev = item;
				}
			} else {
				/* Add item to the front of the list */
				item->next = pool->item_list;
				item->prev = pool->item_list->prev;
				pool->item_list->prev = item;
				pool->item_list = item;
			}
		}
		else {
			pool->item_list = item;
		}

		allocated += item->size_in_dw;
	}
}


void compute_memory_free(struct compute_memory_pool* pool, int64_t id)
{
	struct compute_memory_item *item, *next;

	COMPUTE_DBG(pool->screen, "* compute_memory_free() id + %ld \n", id);

	for (item = pool->item_list; item; item = next) {
		next = item->next;

		if (item->id == id) {
			if (item->prev) {
				item->prev->next = item->next;
			}
			else {
				pool->item_list = item->next;
			}

			if (item->next) {
				item->next->prev = item->prev;
			}

			free(item);

			return;
		}
	}

	fprintf(stderr, "Internal error, invalid id %"PRIi64" "
		"for compute_memory_free\n", id);

	assert(0 && "error");
}

/**
 * Creates pending allocations
 */
struct compute_memory_item* compute_memory_alloc(
	struct compute_memory_pool* pool,
	int64_t size_in_dw)
{
	struct compute_memory_item *new_item = NULL, *last_item = NULL;

	COMPUTE_DBG(pool->screen, "* compute_memory_alloc() size_in_dw = %ld (%ld bytes)\n",
			size_in_dw, 4 * size_in_dw);

	new_item = (struct compute_memory_item *)
				CALLOC(sizeof(struct compute_memory_item), 1);
	new_item->size_in_dw = size_in_dw;
	new_item->start_in_dw = -1; /* mark pending */
	new_item->id = pool->next_id++;
	new_item->pool = pool;

	if (pool->item_list) {
		for (last_item = pool->item_list; last_item->next;
						last_item = last_item->next);

		last_item->next = new_item;
		new_item->prev = last_item;
	}
	else {
		pool->item_list = new_item;
	}

	COMPUTE_DBG(pool->screen, "  + Adding item %p id = %u size = %u (%u bytes)\n",
			new_item, new_item->id, new_item->size_in_dw,
			new_item->size_in_dw * 4);
	return new_item;
}

/**
 * Transfer data host<->device, offset and size is in bytes
 */
void compute_memory_transfer(
	struct compute_memory_pool* pool,
	struct pipe_context * pipe,
	int device_to_host,
	struct compute_memory_item* chunk,
	void* data,
	int offset_in_chunk,
	int size)
{
	int64_t aligned_size = pool->size_in_dw;
	struct pipe_resource* gart = (struct pipe_resource*)pool->bo;
	int64_t internal_offset = chunk->start_in_dw*4 + offset_in_chunk;

	struct pipe_transfer *xfer;
	uint32_t *map;

	assert(gart);

	COMPUTE_DBG(pool->screen, "* compute_memory_transfer() device_to_host = %d, "
		"offset_in_chunk = %d, size = %d\n", device_to_host,
		offset_in_chunk, size);

	if (device_to_host) {
		map = pipe->transfer_map(pipe, gart, 0, PIPE_TRANSFER_READ,
			&(struct pipe_box) { .width = aligned_size * 4,
			.height = 1, .depth = 1 }, &xfer);
                assert(xfer);
		assert(map);
		memcpy(data, map + internal_offset, size);
		pipe->transfer_unmap(pipe, xfer);
	} else {
		map = pipe->transfer_map(pipe, gart, 0, PIPE_TRANSFER_WRITE,
			&(struct pipe_box) { .width = aligned_size * 4,
			.height = 1, .depth = 1 }, &xfer);
		assert(xfer);
		assert(map);
		memcpy(map + internal_offset, data, size);
		pipe->transfer_unmap(pipe, xfer);
	}
}

/**
 * Transfer data between chunk<->data, it is for VRAM<->GART transfers
 */
void compute_memory_transfer_direct(
	struct compute_memory_pool* pool,
	int chunk_to_data,
	struct compute_memory_item* chunk,
	struct r600_resource* data,
	int offset_in_chunk,
	int offset_in_data,
	int size)
{
	///TODO: DMA
}
