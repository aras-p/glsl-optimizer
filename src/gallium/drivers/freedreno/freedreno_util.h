/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FREEDRENO_UTIL_H_
#define FREEDRENO_UTIL_H_

#include <freedreno_drmif.h>
#include <freedreno_ringbuffer.h>

#include "pipe/p_format.h"
#include "util/u_debug.h"

#include "freedreno_pm4.h"
#include "freedreno_a2xx_reg.h"

enum sq_surfaceformat fd_pipe2surface(enum pipe_format format);
enum rb_colorformatx fd_pipe2color(enum pipe_format format);
enum rb_depth_format fd_pipe2depth(enum pipe_format format);
enum pc_di_index_size fd_pipe2index(enum pipe_format format);
uint32_t fd_tex_swiz(enum pipe_format format, unsigned swizzle_r,
		unsigned swizzle_g, unsigned swizzle_b, unsigned swizzle_a);


#define FD_DBG_MSGS   0x1
#define FD_DBG_DISASM 0x2
extern int fd_mesa_debug;

#define DBG(fmt, ...) \
		do { if (fd_mesa_debug & FD_DBG_MSGS) \
			debug_printf("%s:%d: "fmt "\n", \
				__FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)

#define ALIGN(v,a) (((v) + (a) - 1) & ~((a) - 1))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))


#define LOG_DWORDS 0


static inline void
OUT_RING(struct fd_ringbuffer *ring, uint32_t data)
{
	if (LOG_DWORDS) {
		DBG("ring[%p]: OUT_RING   %04x:  %08x", ring,
				(uint32_t)(ring->cur - ring->last_start), data);
	}
	*(ring->cur++) = data;
}

static inline void
OUT_RELOC(struct fd_ringbuffer *ring, struct fd_bo *bo,
		uint32_t offset, uint32_t or)
{
	if (LOG_DWORDS) {
		DBG("ring[%p]: OUT_RELOC  %04x:  %p+%u", ring,
				(uint32_t)(ring->cur - ring->last_start), bo, offset);
	}
	fd_ringbuffer_emit_reloc(ring, bo, offset, or);
}

static inline void BEGIN_RING(struct fd_ringbuffer *ring, uint32_t ndwords)
{
	if ((ring->cur + ndwords) >= ring->end) {
		/* this probably won't really work if we have multiple tiles..
		 * but it is ok for 2d..  we might need different behavior
		 * depending on 2d or 3d pipe.
		 */
		DBG("uh oh..");
	}
}

static inline void
OUT_PKT0(struct fd_ringbuffer *ring, uint16_t regindx, uint16_t cnt)
{
	BEGIN_RING(ring, cnt+1);
	OUT_RING(ring, CP_TYPE0_PKT | ((cnt-1) << 16) | (regindx & 0x7FFF));
}

static inline void
OUT_PKT3(struct fd_ringbuffer *ring, uint8_t opcode, uint16_t cnt)
{
	BEGIN_RING(ring, cnt+1);
	OUT_RING(ring, CP_TYPE3_PKT | ((cnt-1) << 16) | ((opcode & 0xFF) << 8));
}

static inline void
OUT_IB(struct fd_ringbuffer *ring, struct fd_ringmarker *start,
		struct fd_ringmarker *end)
{
	OUT_PKT3(ring, CP_INDIRECT_BUFFER_PFD, 2);
	fd_ringbuffer_emit_reloc_ring(ring, start);
	OUT_RING(ring, fd_ringmarker_dwords(start, end));
}

#endif /* FREEDRENO_UTIL_H_ */
