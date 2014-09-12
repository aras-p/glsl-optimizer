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
#include "pipe/p_state.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_half.h"
#include "util/u_dynarray.h"

#include "adreno_common.xml.h"
#include "adreno_pm4.xml.h"

enum adreno_rb_depth_format fd_pipe2depth(enum pipe_format format);
enum pc_di_index_size fd_pipe2index(enum pipe_format format);
enum adreno_rb_blend_factor fd_blend_factor(unsigned factor);
enum adreno_pa_su_sc_draw fd_polygon_mode(unsigned mode);
enum adreno_stencil_op fd_stencil_op(unsigned op);

#define A3XX_MAX_MIP_LEVELS 14
/* TBD if it is same on a2xx, but for now: */
#define MAX_MIP_LEVELS A3XX_MAX_MIP_LEVELS

#define FD_DBG_MSGS     0x0001
#define FD_DBG_DISASM   0x0002
#define FD_DBG_DCLEAR   0x0004
#define FD_DBG_FLUSH    0x0008
#define FD_DBG_DSCIS    0x0010
#define FD_DBG_DIRECT   0x0020
#define FD_DBG_DBYPASS  0x0040
#define FD_DBG_FRAGHALF 0x0080
#define FD_DBG_NOBIN    0x0100
#define FD_DBG_NOOPT    0x0200
#define FD_DBG_OPTMSGS  0x0400
#define FD_DBG_OPTDUMP  0x0800
#define FD_DBG_GLSL130  0x1000

extern int fd_mesa_debug;
extern bool fd_binning_enabled;

#define DBG(fmt, ...) \
		do { if (fd_mesa_debug & FD_DBG_MSGS) \
			debug_printf("%s:%d: "fmt "\n", \
				__FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* for conditionally setting boolean flag(s): */
#define COND(bool, val) ((bool) ? (val) : 0)

#define CP_REG(reg) ((0x4 << 16) | ((unsigned int)((reg) - (0x2000))))

static inline uint32_t DRAW(enum pc_di_primtype prim_type,
		enum pc_di_src_sel source_select, enum pc_di_index_size index_size,
		enum pc_di_vis_cull_mode vis_cull_mode)
{
	return (prim_type         << 0) |
			(source_select     << 6) |
			((index_size & 1)  << 11) |
			((index_size >> 1) << 13) |
			(vis_cull_mode     << 9) |
			(1                 << 14);
}

/* for tracking cmdstream positions that need to be patched: */
struct fd_cs_patch {
	uint32_t *cs;
	uint32_t val;
};
#define fd_patch_num_elements(buf) ((buf)->size / sizeof(struct fd_cs_patch))
#define fd_patch_element(buf, i)   util_dynarray_element(buf, struct fd_cs_patch, i)

static inline enum pipe_format
pipe_surface_format(struct pipe_surface *psurf)
{
	if (!psurf)
		return PIPE_FORMAT_NONE;
	return psurf->format;
}

#define LOG_DWORDS 0

static inline void emit_marker(struct fd_ringbuffer *ring, int scratch_idx);

static inline void
OUT_RING(struct fd_ringbuffer *ring, uint32_t data)
{
	if (LOG_DWORDS) {
		DBG("ring[%p]: OUT_RING   %04x:  %08x", ring,
				(uint32_t)(ring->cur - ring->last_start), data);
	}
	*(ring->cur++) = data;
}

/* like OUT_RING() but appends a cmdstream patch point to 'buf' */
static inline void
OUT_RINGP(struct fd_ringbuffer *ring, uint32_t data,
		struct util_dynarray *buf)
{
	if (LOG_DWORDS) {
		DBG("ring[%p]: OUT_RINGP  %04x:  %08x", ring,
				(uint32_t)(ring->cur - ring->last_start), data);
	}
	util_dynarray_append(buf, struct fd_cs_patch, ((struct fd_cs_patch){
		.cs  = ring->cur++,
		.val = data,
	}));
}

static inline void
OUT_RELOC(struct fd_ringbuffer *ring, struct fd_bo *bo,
		uint32_t offset, uint32_t or, int32_t shift)
{
	if (LOG_DWORDS) {
		DBG("ring[%p]: OUT_RELOC   %04x:  %p+%u << %d", ring,
				(uint32_t)(ring->cur - ring->last_start), bo, offset, shift);
	}
	fd_ringbuffer_reloc(ring, &(struct fd_reloc){
		.bo = bo,
		.flags = FD_RELOC_READ,
		.offset = offset,
		.or = or,
		.shift = shift,
	});
}

static inline void
OUT_RELOCW(struct fd_ringbuffer *ring, struct fd_bo *bo,
		uint32_t offset, uint32_t or, int32_t shift)
{
	if (LOG_DWORDS) {
		DBG("ring[%p]: OUT_RELOCW  %04x:  %p+%u << %d", ring,
				(uint32_t)(ring->cur - ring->last_start), bo, offset, shift);
	}
	fd_ringbuffer_reloc(ring, &(struct fd_reloc){
		.bo = bo,
		.flags = FD_RELOC_READ | FD_RELOC_WRITE,
		.offset = offset,
		.or = or,
		.shift = shift,
	});
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
OUT_WFI(struct fd_ringbuffer *ring)
{
	OUT_PKT3(ring, CP_WAIT_FOR_IDLE, 1);
	OUT_RING(ring, 0x00000000);
}

static inline void
OUT_IB(struct fd_ringbuffer *ring, struct fd_ringmarker *start,
		struct fd_ringmarker *end)
{
	/* for debug after a lock up, write a unique counter value
	 * to scratch6 for each IB, to make it easier to match up
	 * register dumps to cmdstream.  The combination of IB and
	 * DRAW (scratch7) is enough to "triangulate" the particular
	 * draw that caused lockup.
	 */
	emit_marker(ring, 6);

	OUT_PKT3(ring, CP_INDIRECT_BUFFER_PFD, 2);
	fd_ringbuffer_emit_reloc_ring(ring, start, end);
	OUT_RING(ring, fd_ringmarker_dwords(start, end));

	emit_marker(ring, 6);
}

/* CP_SCRATCH_REG4 is used to hold base address for query results: */
#define HW_QUERY_BASE_REG REG_AXXX_CP_SCRATCH_REG4

static inline void
emit_marker(struct fd_ringbuffer *ring, int scratch_idx)
{
	extern unsigned marker_cnt;
	unsigned reg = REG_AXXX_CP_SCRATCH_REG0 + scratch_idx;
	assert(reg != HW_QUERY_BASE_REG);
	if (reg == HW_QUERY_BASE_REG)
		return;
	OUT_PKT0(ring, reg, 1);
	OUT_RING(ring, ++marker_cnt);
}

/* helper to get numeric value from environment variable..  mostly
 * just leaving this here because it is helpful to brute-force figure
 * out unknown formats, etc, which blob driver does not support:
 */
static inline uint32_t env2u(const char *envvar)
{
	char *str = getenv(envvar);
	if (str)
		return strtol(str, NULL, 0);
	return 0;
}

#endif /* FREEDRENO_UTIL_H_ */
