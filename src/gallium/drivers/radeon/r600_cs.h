/*
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 * Authors: Marek Olšák <maraeo@gmail.com>
 */

/**
 * This file contains helpers for writing commands to commands streams.
 */

#ifndef R600_CS_H
#define R600_CS_H

#include "r600_pipe_common.h"
#include "r600d_common.h"

static INLINE uint64_t r600_resource_va(struct pipe_screen *screen,
					struct pipe_resource *resource)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;
	struct r600_resource *rresource = (struct r600_resource*)resource;

	return rscreen->ws->buffer_get_virtual_address(rresource->cs_buf);
}

static INLINE unsigned r600_context_bo_reloc(struct r600_common_context *rctx,
					     struct r600_ring *ring,
					     struct r600_resource *rbo,
					     enum radeon_bo_usage usage,
					     enum radeon_bo_priority priority)
{
	assert(usage);

	/* Make sure that all previous rings are flushed so that everything
	 * looks serialized from the driver point of view.
	 */
	if (!ring->flushing) {
		if (ring == &rctx->rings.gfx) {
			if (rctx->rings.dma.cs) {
				/* flush dma ring */
				rctx->rings.dma.flush(rctx, RADEON_FLUSH_ASYNC, NULL);
			}
		} else {
			/* flush gfx ring */
			rctx->rings.gfx.flush(rctx, RADEON_FLUSH_ASYNC, NULL);
		}
	}
	return rctx->ws->cs_add_reloc(ring->cs, rbo->cs_buf, usage,
				      rbo->domains, priority) * 4;
}

static INLINE void r600_emit_reloc(struct r600_common_context *rctx,
				   struct r600_ring *ring, struct r600_resource *rbo,
				   enum radeon_bo_usage usage,
				   enum radeon_bo_priority priority)
{
	struct radeon_winsys_cs *cs = ring->cs;
	bool has_vm = ((struct r600_common_screen*)rctx->b.screen)->info.r600_virtual_address;
	unsigned reloc = r600_context_bo_reloc(rctx, ring, rbo, usage, priority);

	if (!has_vm) {
		radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
		radeon_emit(cs, reloc);
	}
}

static INLINE void r600_write_config_reg_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	assert(reg < R600_CONTEXT_REG_OFFSET);
	assert(cs->cdw+2+num <= RADEON_MAX_CMDBUF_DWORDS);
	radeon_emit(cs, PKT3(PKT3_SET_CONFIG_REG, num, 0));
	radeon_emit(cs, (reg - R600_CONFIG_REG_OFFSET) >> 2);
}

static INLINE void r600_write_config_reg(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	r600_write_config_reg_seq(cs, reg, 1);
	radeon_emit(cs, value);
}

static INLINE void r600_write_context_reg_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	assert(reg >= R600_CONTEXT_REG_OFFSET);
	assert(cs->cdw+2+num <= RADEON_MAX_CMDBUF_DWORDS);
	radeon_emit(cs, PKT3(PKT3_SET_CONTEXT_REG, num, 0));
	radeon_emit(cs, (reg - R600_CONTEXT_REG_OFFSET) >> 2);
}

static INLINE void r600_write_context_reg(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	r600_write_context_reg_seq(cs, reg, 1);
	radeon_emit(cs, value);
}

static INLINE void cik_write_uconfig_reg_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	assert(reg >= CIK_UCONFIG_REG_OFFSET && reg < CIK_UCONFIG_REG_END);
	assert(cs->cdw+2+num <= RADEON_MAX_CMDBUF_DWORDS);
	radeon_emit(cs, PKT3(PKT3_SET_UCONFIG_REG, num, 0));
	radeon_emit(cs, (reg - CIK_UCONFIG_REG_OFFSET) >> 2);
}

static INLINE void cik_write_uconfig_reg(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	cik_write_uconfig_reg_seq(cs, reg, 1);
	radeon_emit(cs, value);
}

#endif
