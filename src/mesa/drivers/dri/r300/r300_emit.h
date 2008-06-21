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

#include "glheader.h"
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

/**
 * Prepare to write a register value to register at address reg.
 * If num_extra > 0 then the following extra values are written
 * to registers with address +4, +8 and so on..
 */
#define reg_start(reg, num_extra)					\
	do {								\
		int _n;							\
		_n=(num_extra);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+2),				\
					__FUNCTION__);			\
		cmd_reserved=_n+2;					\
		cmd_written=1;						\
		cmd[0].i=cmdpacket0((reg), _n+1);			\
	} while (0);

/**
 * Emit GLuint freestyle
 */
#define e32(dword)							\
	do {								\
		if(cmd_written<cmd_reserved) {				\
			cmd[cmd_written].i=(dword);			\
			cmd_written++;					\
		} else {						\
			fprintf(stderr,					\
				"e32 but no previous packet "		\
				"declaration.\n"			\
				"Aborting! in %s::%s at line %d, "	\
				"cmd_written=%d cmd_reserved=%d\n",	\
				__FILE__, __FUNCTION__, __LINE__,	\
				cmd_written, cmd_reserved);		\
			_mesa_exit(-1);					\
		}							\
	} while(0)

#define	efloat(f) e32(r300PackFloat32(f))

#define vsf_start_fragment(dest, length)				\
	do {								\
		int _n;							\
		_n = (length);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+1),				\
					__FUNCTION__);			\
		cmd_reserved = _n+2;					\
		cmd_written =1;						\
		cmd[0].i = cmdvpu((dest), _n/4);			\
	} while (0);

#define r500fp_start_fragment(dest, length)				\
	do {								\
		int _n;							\
		_n = (length);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+1),				\
					__FUNCTION__);			\
		cmd_reserved = _n+1;					\
		cmd_written =1;						\
		cmd[0].i = cmdr500fp((dest), _n/6, 0, 0);		\
	} while (0);

#define start_packet3(packet, count)					\
	{								\
		int _n;							\
		GLuint _p;						\
		_n = (count);						\
		_p = (packet);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+3),				\
					__FUNCTION__);			\
		cmd_reserved = _n+3;					\
		cmd_written = 2;					\
		if(_n > 0x3fff) {					\
			fprintf(stderr,"Too big packet3 %08x: cannot "	\
				"store %d dwords\n",			\
				_p, _n);				\
			_mesa_exit(-1);					\
		}							\
		cmd[0].i = cmdpacket3(R300_CMD_PACKET3_RAW);		\
		cmd[1].i = _p | ((_n & 0x3fff)<<16);			\
	}

/**
 * Must be sent to switch to 2d commands
 */
void static INLINE end_3d(r300ContextPtr rmesa)
{
	drm_radeon_cmd_header_t *cmd = NULL;

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].header.cmd_type = R300_CMD_END3D;
}

void static INLINE cp_delay(r300ContextPtr rmesa, unsigned short count)
{
	drm_radeon_cmd_header_t *cmd = NULL;

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].i = cmdcpdelay(count);
}

void static INLINE cp_wait(r300ContextPtr rmesa, unsigned char flags)
{
	drm_radeon_cmd_header_t *cmd = NULL;

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].i = cmdwait(flags);
}

extern int r300EmitArrays(GLcontext * ctx);

#ifdef USER_BUFFERS
void r300UseArrays(GLcontext * ctx);
#endif

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
