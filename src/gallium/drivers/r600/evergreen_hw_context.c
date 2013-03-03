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
#include "r600_pipe.h"
#include "evergreend.h"
#include "util/u_memory.h"
#include "util/u_math.h"

void evergreen_flush_vgt_streamout(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	r600_write_config_reg(cs, R_0084FC_CP_STRMOUT_CNTL, 0);

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0);

	cs->buf[cs->cdw++] = PKT3(PKT3_WAIT_REG_MEM, 5, 0);
	cs->buf[cs->cdw++] = WAIT_REG_MEM_EQUAL; /* wait until the register is equal to the reference value */
	cs->buf[cs->cdw++] = R_0084FC_CP_STRMOUT_CNTL >> 2;  /* register */
	cs->buf[cs->cdw++] = 0;
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* reference value */
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* mask */
	cs->buf[cs->cdw++] = 4; /* poll interval */
}

void evergreen_set_streamout_enable(struct r600_context *ctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	if (buffer_enable_bit) {
		r600_write_context_reg_seq(cs, R_028B94_VGT_STRMOUT_CONFIG, 2);
		r600_write_value(cs, S_028B94_STREAMOUT_0_EN(1)); /* R_028B94_VGT_STRMOUT_CONFIG */
		r600_write_value(cs, S_028B98_STREAM_0_BUFFER_EN(buffer_enable_bit)); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */
	} else {
		r600_write_context_reg(cs, R_028B94_VGT_STRMOUT_CONFIG, S_028B94_STREAMOUT_0_EN(0));
	}
}

void evergreen_dma_copy(struct r600_context *rctx,
		struct pipe_resource *dst,
		struct pipe_resource *src,
		uint64_t dst_offset,
		uint64_t src_offset,
		uint64_t size)
{
	struct radeon_winsys_cs *cs = rctx->rings.dma.cs;
	unsigned i, ncopy, csize, sub_cmd, shift;
	struct r600_resource *rdst = (struct r600_resource*)dst;
	struct r600_resource *rsrc = (struct r600_resource*)src;

	/* make sure that the dma ring is only one active */
	rctx->rings.gfx.flush(rctx, RADEON_FLUSH_ASYNC);
	dst_offset += r600_resource_va(&rctx->screen->screen, dst);
	src_offset += r600_resource_va(&rctx->screen->screen, src);

	/* see if we use dword or byte copy */
	if (!(dst_offset & 0x3) && !(src_offset & 0x3) && !(size & 0x3)) {
		size >>= 2;
		sub_cmd = 0x00;
		shift = 2;
	} else {
		sub_cmd = 0x40;
		shift = 0;
	}
	ncopy = (size / 0x000fffff) + !!(size % 0x000fffff);

	r600_need_dma_space(rctx, ncopy * 5);
	for (i = 0; i < ncopy; i++) {
		csize = size < 0x000fffff ? size : 0x000fffff;
		/* emit reloc before writting cs so that cs is always in consistent state */
		r600_context_bo_reloc(rctx, &rctx->rings.dma, rsrc, RADEON_USAGE_READ);
		r600_context_bo_reloc(rctx, &rctx->rings.dma, rdst, RADEON_USAGE_WRITE);
		cs->buf[cs->cdw++] = DMA_PACKET(DMA_PACKET_COPY, sub_cmd, csize);
		cs->buf[cs->cdw++] = dst_offset & 0xffffffff;
		cs->buf[cs->cdw++] = src_offset & 0xffffffff;
		cs->buf[cs->cdw++] = (dst_offset >> 32UL) & 0xff;
		cs->buf[cs->cdw++] = (src_offset >> 32UL) & 0xff;
		dst_offset += csize << shift;
		src_offset += csize << shift;
		size -= csize;
	}

	util_range_add(&rdst->valid_buffer_range, dst_offset,
		       dst_offset + size);
}
