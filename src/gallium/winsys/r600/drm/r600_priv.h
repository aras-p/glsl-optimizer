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
#ifndef R600_PRIV_H
#define R600_PRIV_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "r600.h"


struct radeon {
	int				fd;
	int				refcount;
	unsigned			device;
	unsigned			family;
	enum chip_class			chip_class;
	boolean				use_mem_constant; /* true for evergreen */
	struct pb_manager *mman; /* malloc manager */
	struct pb_manager *kman; /* kernel bo manager */
	struct pb_manager *cman; /* cached bo manager */
};

struct radeon *r600_new(int fd, unsigned device);
void r600_delete(struct radeon *r600);

struct r600_reg {
	unsigned			need_bo;
	unsigned			flush_flags;
	unsigned			offset;
};

/* radeon_pciid.c */
unsigned radeon_family_from_device(unsigned device);


static void inline r600_context_reg(struct r600_context *ctx, unsigned group_id,
					unsigned offset, unsigned value,
					unsigned mask)
{
	struct r600_group *group = &ctx->groups[group_id];
	struct r600_group_block *block;
	unsigned id;

	id = group->offset_block_id[(offset - group->start_offset) >> 2];
	block = &group->blocks[id];
	id = (offset - block->start_offset) >> 2;
	block->reg[id] &= ~mask;
	block->reg[id] |= value;
	if (!(block->status & R600_BLOCK_STATUS_DIRTY)) {
		ctx->pm4_dirty_cdwords += block->pm4_ndwords;
	}
	block->status |= R600_BLOCK_STATUS_ENABLED;
	block->status |= R600_BLOCK_STATUS_DIRTY;
}

#endif
