/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 */
#ifndef R600_H
#define R600_H

#include "../../winsys/radeon/drm/radeon_winsys.h"
#include "util/u_double_list.h"
#include "util/u_range.h"
#include "util/u_transfer.h"

#define R600_ERR(fmt, args...) \
	fprintf(stderr, "EE %s:%d %s - "fmt, __FILE__, __LINE__, __func__, ##args)

struct winsys_handle;

struct r600_tiling_info {
	unsigned num_channels;
	unsigned num_banks;
	unsigned group_bytes;
};

struct r600_resource {
	struct u_resource		b;

	/* Winsys objects. */
	struct pb_buffer		*buf;
	struct radeon_winsys_cs_handle	*cs_buf;

	/* Resource state. */
	unsigned			domains;

	/* The buffer range which is initialized (with a write transfer,
	 * streamout, DMA, or as a random access target). The rest of
	 * the buffer is considered invalid and can be mapped unsynchronized.
	 *
	 * This allows unsychronized mapping of a buffer range which hasn't
	 * been used yet. It's for applications which forget to use
	 * the unsynchronized map flag and expect the driver to figure it out.
         */
	struct util_range		valid_buffer_range;
};

struct r600_query_buffer {
	/* The buffer where query results are stored. */
	struct r600_resource			*buf;
	/* Offset of the next free result after current query data */
	unsigned				results_end;
	/* If a query buffer is full, a new buffer is created and the old one
	 * is put in here. When we calculate the result, we sum up the samples
	 * from all buffers. */
	struct r600_query_buffer		*previous;
};

struct r600_query {
	/* The query buffer and how many results are in it. */
	struct r600_query_buffer		buffer;
	/* The type of query */
	unsigned				type;
	/* Size of the result in memory for both begin_query and end_query,
	 * this can be one or two numbers, or it could even be a size of a structure. */
	unsigned				result_size;
	/* The number of dwords for begin_query or end_query. */
	unsigned				num_cs_dw;
	/* linked list of queries */
	struct list_head			list;
};

struct r600_so_target {
	struct pipe_stream_output_target b;

	/* The buffer where BUFFER_FILLED_SIZE is stored. */
	struct r600_resource	*buf_filled_size;
	unsigned		buf_filled_size_offset;

	unsigned		stride_in_dw;
	unsigned		so_index;
};

#define R600_CONTEXT_INVAL_READ_CACHES		(1 << 0)
#define R600_CONTEXT_STREAMOUT_FLUSH		(1 << 1)
#define R600_CONTEXT_WAIT_3D_IDLE		(1 << 2)
#define R600_CONTEXT_WAIT_CP_DMA_IDLE		(1 << 3)
#define R600_CONTEXT_FLUSH_AND_INV		(1 << 4)
#define R600_CONTEXT_FLUSH_AND_INV_CB_META	(1 << 5)
#define R600_CONTEXT_PS_PARTIAL_FLUSH		(1 << 6)
#define R600_CONTEXT_FLUSH_AND_INV_DB_META      (1 << 7)

struct r600_context;
struct r600_screen;

void r600_get_backend_mask(struct r600_context *ctx);
void r600_context_flush(struct r600_context *ctx, unsigned flags);
void r600_begin_new_cs(struct r600_context *ctx);

void r600_context_emit_fence(struct r600_context *ctx, struct r600_resource *fence,
                             unsigned offset, unsigned value);
void r600_flush_emit(struct r600_context *ctx);

void r600_need_cs_space(struct r600_context *ctx, unsigned num_dw, boolean count_draw_in);
void r600_need_dma_space(struct r600_context *ctx, unsigned num_dw);
void r600_dma_copy(struct r600_context *rctx,
		struct pipe_resource *dst,
		struct pipe_resource *src,
		uint64_t dst_offset,
		uint64_t src_offset,
		uint64_t size);
boolean r600_dma_blit(struct pipe_context *ctx,
			struct pipe_resource *dst,
			unsigned dst_level,
			unsigned dst_x, unsigned dst_y, unsigned dst_z,
			struct pipe_resource *src,
			unsigned src_level,
			const struct pipe_box *src_box);
void evergreen_dma_copy(struct r600_context *rctx,
		struct pipe_resource *dst,
		struct pipe_resource *src,
		uint64_t dst_offset,
		uint64_t src_offset,
		uint64_t size);
boolean evergreen_dma_blit(struct pipe_context *ctx,
			struct pipe_resource *dst,
			unsigned dst_level,
			unsigned dst_x, unsigned dst_y, unsigned dst_z,
			struct pipe_resource *src,
			unsigned src_level,
			const struct pipe_box *src_box);
void r600_cp_dma_copy_buffer(struct r600_context *rctx,
			     struct pipe_resource *dst, uint64_t dst_offset,
			     struct pipe_resource *src, uint64_t src_offset,
			     unsigned size);

#endif
