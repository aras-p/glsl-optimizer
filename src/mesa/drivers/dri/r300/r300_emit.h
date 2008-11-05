/*
 * Copyright (C) 2005 Vladimir Dergachev.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Vladimir Dergachev <volodya@mindspring.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 *   Aapo Tahkola <aet@rasterburn.org>
 *   Ben Skeggs <darktama@iinet.net.au>
 *   Jerome Glisse <j.glisse@gmail.com>
 */

/* This files defines functions for accessing R300 hardware.
 */
#ifndef __R300_EMIT_H__
#define __R300_EMIT_H__

#include "main/glheader.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "radeon_reg.h"

/* TODO: move these defines (and the ones from DRM) into r300_reg.h and sync up
 * with DRM */
#define CP_PACKET0(reg, n)	(RADEON_CP_PACKET0 | ((n)<<16) | ((reg)>>2))
#define CP_PACKET3( pkt, n )						\
	(RADEON_CP_PACKET3 | (pkt) | ((n) << 16))

static INLINE uint32_t cmdpacket0(int reg, int count)
{
	drm_r300_cmd_header_t cmd;

	cmd.packet0.cmd_type = R300_CMD_PACKET0;
	cmd.packet0.count = count;
	cmd.packet0.reghi = ((unsigned int)reg & 0xFF00) >> 8;
	cmd.packet0.reglo = ((unsigned int)reg & 0x00FF);

	return cmd.u;
}

static INLINE uint32_t cmdvpu(int addr, int count)
{
	drm_r300_cmd_header_t cmd;

	cmd.vpu.cmd_type = R300_CMD_VPU;
	cmd.vpu.count = count;
	cmd.vpu.adrhi = ((unsigned int)addr & 0xFF00) >> 8;
	cmd.vpu.adrlo = ((unsigned int)addr & 0x00FF);

	return cmd.u;
}

static INLINE uint32_t cmdr500fp(int addr, int count, int type, int clamp)
{
	drm_r300_cmd_header_t cmd;

	cmd.r500fp.cmd_type = R300_CMD_R500FP;
	cmd.r500fp.count = count;
	cmd.r500fp.adrhi_flags = ((unsigned int)addr & 0x100) >> 8;
	cmd.r500fp.adrhi_flags |= type ? R500FP_CONSTANT_TYPE : 0;
	cmd.r500fp.adrhi_flags |= clamp ? R500FP_CONSTANT_CLAMP : 0;
	cmd.r500fp.adrlo = ((unsigned int)addr & 0x00FF);

	return cmd.u;
}

static INLINE uint32_t cmdpacket3(int packet)
{
	drm_r300_cmd_header_t cmd;

	cmd.packet3.cmd_type = R300_CMD_PACKET3;
	cmd.packet3.packet = packet;

	return cmd.u;
}

static INLINE uint32_t cmdcpdelay(unsigned short count)
{
	drm_r300_cmd_header_t cmd;

	cmd.delay.cmd_type = R300_CMD_CP_DELAY;
	cmd.delay.count = count;

	return cmd.u;
}

static INLINE uint32_t cmdwait(unsigned char flags)
{
	drm_r300_cmd_header_t cmd;

	cmd.wait.cmd_type = R300_CMD_WAIT;
	cmd.wait.flags = flags;

	return cmd.u;
}

static INLINE uint32_t cmdpacify(void)
{
	drm_r300_cmd_header_t cmd;

	cmd.header.cmd_type = R300_CMD_END3D;

	return cmd.u;
}


/** Single register write to command buffer; requires 2 dwords. */
#define OUT_BATCH_REGVAL(reg, val) \
	OUT_BATCH(cmdpacket0((reg), 1)); \
	OUT_BATCH((val))

/** Continuous register range write to command buffer; requires 1 dword,
 * expects count dwords afterwards for register contents. */
#define OUT_BATCH_REGSEQ(reg, count) \
	OUT_BATCH(cmdpacket0((reg), (count)));

/** Write a 32 bit float to the ring; requires 1 dword. */
#define OUT_BATCH_FLOAT32(f) \
	OUT_BATCH(r300PackFloat32((f)));

/**
 * Write the header of a packet3 to the command buffer.
 * Outputs 2 dwords and expects (num_extra+1) additional dwords afterwards.
 */
#define OUT_BATCH_PACKET3(packet, num_extra) do {\
	OUT_BATCH(cmdpacket3(R300_CMD_PACKET3_RAW)); \
	OUT_BATCH(CP_PACKET3((packet), (num_extra))); \
	} while(0)

/**
 * Must be sent to switch to 2d commands
 */
void static INLINE end_3d(r300ContextPtr rmesa)
{
	BATCH_LOCALS(rmesa);

	BEGIN_BATCH(1);
	OUT_BATCH(cmdpacify());
	END_BATCH();
}

void static INLINE cp_delay(r300ContextPtr rmesa, unsigned short count)
{
	BATCH_LOCALS(rmesa);

	BEGIN_BATCH(1);
	OUT_BATCH(cmdcpdelay(count));
	END_BATCH();
}

void static INLINE cp_wait(r300ContextPtr rmesa, unsigned char flags)
{
	BATCH_LOCALS(rmesa);

	BEGIN_BATCH(1);
	OUT_BATCH(cmdwait(flags));
	END_BATCH();
}

extern int r300EmitArrays(GLcontext * ctx);

extern void r300ReleaseArrays(GLcontext * ctx);
extern int r300PrimitiveType(r300ContextPtr rmesa, int prim);
extern int r300NumVerts(r300ContextPtr rmesa, int num_verts, int prim);

extern void r300EmitCacheFlush(r300ContextPtr rmesa);

extern GLuint r300VAPInputRoute0(uint32_t * dst, GLvector4f ** attribptr,
				 int *inputs, GLint * tab, GLuint nr);
extern GLuint r300VAPInputRoute1(uint32_t * dst, int swizzle[][4], GLuint nr);
extern GLuint r300VAPInputCntl0(GLcontext * ctx, GLuint InputsRead);
extern GLuint r300VAPInputCntl1(GLcontext * ctx, GLuint InputsRead);
extern GLuint r300VAPOutputCntl0(GLcontext * ctx, GLuint OutputsWritten);
extern GLuint r300VAPOutputCntl1(GLcontext * ctx, GLuint OutputsWritten);

#endif
