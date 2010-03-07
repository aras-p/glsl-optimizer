/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R600_CMDBUF_H__
#define __R600_CMDBUF_H__

#include "r600_context.h"
#include "r600_emit.h"

#define RADEON_CP_PACKET3_NOP                       0xC0001000
#define RADEON_CP_PACKET3_NEXT_CHAR                 0xC0001900
#define RADEON_CP_PACKET3_PLY_NEXTSCAN              0xC0001D00
#define RADEON_CP_PACKET3_SET_SCISSORS              0xC0001E00
#define RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM     0xC0002300
#define RADEON_CP_PACKET3_LOAD_MICROCODE            0xC0002400
#define RADEON_CP_PACKET3_WAIT_FOR_IDLE             0xC0002600
#define RADEON_CP_PACKET3_3D_DRAW_VBUF              0xC0002800
#define RADEON_CP_PACKET3_3D_DRAW_IMMD              0xC0002900
#define RADEON_CP_PACKET3_3D_DRAW_INDX              0xC0002A00
#define RADEON_CP_PACKET3_LOAD_PALETTE              0xC0002C00
#define RADEON_CP_PACKET3_3D_LOAD_VBPNTR            0xC0002F00
#define RADEON_CP_PACKET3_CNTL_PAINT                0xC0009100
#define RADEON_CP_PACKET3_CNTL_BITBLT               0xC0009200
#define RADEON_CP_PACKET3_CNTL_SMALLTEXT            0xC0009300
#define RADEON_CP_PACKET3_CNTL_HOSTDATA_BLT         0xC0009400
#define RADEON_CP_PACKET3_CNTL_POLYLINE             0xC0009500
#define RADEON_CP_PACKET3_CNTL_POLYSCANLINES        0xC0009800
#define RADEON_CP_PACKET3_CNTL_PAINT_MULTI          0xC0009A00
#define RADEON_CP_PACKET3_CNTL_BITBLT_MULTI         0xC0009B00
#define RADEON_CP_PACKET3_CNTL_TRANS_BITBLT         0xC0009C00

/* r6xx/r7xx packet 3 type offsets */
#define R600_SET_CONFIG_REG_OFFSET                  0x00008000
#define R600_SET_CONFIG_REG_END                     0x0000ac00
#define R600_SET_CONTEXT_REG_OFFSET                 0x00028000
#define R600_SET_CONTEXT_REG_END                    0x00029000
#define R600_SET_ALU_CONST_OFFSET                   0x00030000
#define R600_SET_ALU_CONST_END                      0x00032000
#define R600_SET_RESOURCE_OFFSET                    0x00038000
#define R600_SET_RESOURCE_END                       0x0003c000
#define R600_SET_SAMPLER_OFFSET                     0x0003c000
#define R600_SET_SAMPLER_END                        0x0003cff0
#define R600_SET_CTL_CONST_OFFSET                   0x0003cff0
#define R600_SET_CTL_CONST_END                      0x0003e200
#define R600_SET_LOOP_CONST_OFFSET                  0x0003e200
#define R600_SET_LOOP_CONST_END                     0x0003e380
#define R600_SET_BOOL_CONST_OFFSET                  0x0003e380
#define R600_SET_BOOL_CONST_END                     0x00040000

/* r6xx/r7xx packet 3 types */
#define R600_IT_INDIRECT_BUFFER_END               0x00001700
#define R600_IT_SET_PREDICATION                   0x00002000
#define R600_IT_REG_RMW                           0x00002100
#define R600_IT_COND_EXEC                         0x00002200
#define R600_IT_PRED_EXEC                         0x00002300
#define R600_IT_START_3D_CMDBUF                   0x00002400
#define R600_IT_DRAW_INDEX_2                      0x00002700
#define R600_IT_CONTEXT_CONTROL                   0x00002800
#define R600_IT_DRAW_INDEX_IMMD_BE                0x00002900
#define R600_IT_INDEX_TYPE                        0x00002A00
#define R600_IT_DRAW_INDEX                        0x00002B00
#define R600_IT_DRAW_INDEX_AUTO                   0x00002D00
#define R600_IT_DRAW_INDEX_IMMD                   0x00002E00
#define R600_IT_NUM_INSTANCES                     0x00002F00
#define R600_IT_STRMOUT_BUFFER_UPDATE             0x00003400
#define R600_IT_INDIRECT_BUFFER_MP                0x00003800
#define R600_IT_MEM_SEMAPHORE                     0x00003900
#define R600_IT_MPEG_INDEX                        0x00003A00
#define R600_IT_WAIT_REG_MEM                      0x00003C00
#define R600_IT_MEM_WRITE                         0x00003D00
#define R600_IT_INDIRECT_BUFFER                   0x00003200
#define R600_IT_CP_INTERRUPT                      0x00004000
#define R600_IT_SURFACE_SYNC                      0x00004300
#define R600_IT_ME_INITIALIZE                     0x00004400
#define R600_IT_COND_WRITE                        0x00004500
#define R600_IT_EVENT_WRITE                       0x00004600
#define R600_IT_EVENT_WRITE_EOP                   0x00004700
#define R600_IT_ONE_REG_WRITE                     0x00005700
#define R600_IT_SET_CONFIG_REG                    0x00006800
#define R600_IT_SET_CONTEXT_REG                   0x00006900
#define R600_IT_SET_ALU_CONST                     0x00006A00
#define R600_IT_SET_BOOL_CONST                    0x00006B00
#define R600_IT_SET_LOOP_CONST                    0x00006C00
#define R600_IT_SET_RESOURCE                      0x00006D00
#define R600_IT_SET_SAMPLER                       0x00006E00
#define R600_IT_SET_CTL_CONST                     0x00006F00
#define R600_IT_SURFACE_BASE_UPDATE               0x00007300

struct radeon_cs_manager * r600_radeon_cs_manager_legacy_ctor(struct radeon_context *ctx);

/**
 * Write one dword to the command buffer.
 */
#define R600_OUT_BATCH(data)				\
do {							\
        radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, data);	\
} while(0)

/**
 * Write n dwords from ptr to the command buffer.
 */
#define R600_OUT_BATCH_TABLE(ptr,n)		\
do {						     \
	radeon_cs_write_table(b_l_rmesa->cmdbuf.cs, ptr, n);	\
} while(0)

/**
 * Write a relocated dword to the command buffer.
 */
#define R600_OUT_BATCH_RELOC(data, bo, offset, rd, wd, flags) 	\
	do { 							\
        if (0 && offset) {					\
            fprintf(stderr, "(%s:%s:%d) offset : %d\n",		\
            __FILE__, __FUNCTION__, __LINE__, offset);		\
        }							\
        radeon_cs_write_reloc(b_l_rmesa->cmdbuf.cs, 		\
                              bo, rd, wd, flags);		\
	} while(0)

/* R600/R700 */
#define R600_OUT_BATCH_REGS(reg, num)					\
do {								\
	if ((reg) >= R600_SET_CONFIG_REG_OFFSET && (reg) < R600_SET_CONFIG_REG_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_CONFIG_REG_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_CONTEXT_REG_OFFSET && (reg) < R600_SET_CONTEXT_REG_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONTEXT_REG, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_CONTEXT_REG_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_ALU_CONST_OFFSET && (reg) < R600_SET_ALU_CONST_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_ALU_CONST, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_ALU_CONST_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_RESOURCE_OFFSET && (reg) < R600_SET_RESOURCE_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_RESOURCE_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_SAMPLER_OFFSET && (reg) < R600_SET_SAMPLER_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_SAMPLER, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_SAMPLER_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_CTL_CONST_OFFSET && (reg) < R600_SET_CTL_CONST_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_CTL_CONST_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_LOOP_CONST_OFFSET && (reg) < R600_SET_LOOP_CONST_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_LOOP_CONST, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_LOOP_CONST_OFFSET) >> 2);	\
	} else if ((reg) >= R600_SET_BOOL_CONST_OFFSET && (reg) < R600_SET_BOOL_CONST_END) { \
		R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_BOOL_CONST, (num)));	\
		R600_OUT_BATCH(((reg) - R600_SET_BOOL_CONST_OFFSET) >> 2);	\
	} else {							\
		R600_OUT_BATCH(CP_PACKET0((reg), (num))); \
	}								\
} while (0)

/** Single register write to command buffer; requires 3 dwords for most things. */
#define R600_OUT_BATCH_REGVAL(reg, val)		\
	R600_OUT_BATCH_REGS((reg), 1);		\
	R600_OUT_BATCH((val))

/** Continuous register range write to command buffer; requires 1 dword,
 * expects count dwords afterwards for register contents. */
#define R600_OUT_BATCH_REGSEQ(reg, count)	\
	R600_OUT_BATCH_REGS((reg), (count))

extern void r600InitCmdBuf(context_t *r600);

#endif				/* __R600_CMDBUF_H__ */
