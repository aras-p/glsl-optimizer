/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#define CMD_MI				(0x0 << 29)
#define CMD_2D				(0x2 << 29)
#define CMD_3D				(0x3 << 29)

#define MI_NOOP				(CMD_MI | 0)

#define MI_BATCH_BUFFER_END		(CMD_MI | 0xA << 23)

#define MI_FLUSH			(CMD_MI | (4 << 23))
#define FLUSH_MAP_CACHE				(1 << 0)
#define INHIBIT_FLUSH_RENDER_CACHE		(1 << 2)

#define MI_LOAD_REGISTER_IMM		(CMD_MI | (0x22 << 23))

#define MI_FLUSH_DW			(CMD_MI | (0x26 << 23) | 2)

#define MI_STORE_REGISTER_MEM		(CMD_MI | (0x24 << 23))
# define MI_STORE_REGISTER_MEM_USE_GGTT		(1 << 22)

/* Load a value from memory into a register.  Only available on Gen7+. */
#define GEN7_MI_LOAD_REGISTER_MEM	(CMD_MI | (0x29 << 23))
# define MI_LOAD_REGISTER_MEM_USE_GGTT		(1 << 22)

/** @{
 *
 * PIPE_CONTROL operation, a combination MI_FLUSH and register write with
 * additional flushing control.
 */
#define _3DSTATE_PIPE_CONTROL		(CMD_3D | (3 << 27) | (2 << 24))
#define PIPE_CONTROL_CS_STALL		(1 << 20)
#define PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET	(1 << 19)
#define PIPE_CONTROL_TLB_INVALIDATE	(1 << 18)
#define PIPE_CONTROL_SYNC_GFDT		(1 << 17)
#define PIPE_CONTROL_MEDIA_STATE_CLEAR	(1 << 16)
#define PIPE_CONTROL_NO_WRITE		(0 << 14)
#define PIPE_CONTROL_WRITE_IMMEDIATE	(1 << 14)
#define PIPE_CONTROL_WRITE_DEPTH_COUNT	(2 << 14)
#define PIPE_CONTROL_WRITE_TIMESTAMP	(3 << 14)
#define PIPE_CONTROL_DEPTH_STALL	(1 << 13)
#define PIPE_CONTROL_WRITE_FLUSH	(1 << 12)
#define PIPE_CONTROL_INSTRUCTION_FLUSH	(1 << 11)
#define PIPE_CONTROL_TC_FLUSH		(1 << 10) /* GM45+ only */
#define PIPE_CONTROL_ISP_DIS		(1 << 9)
#define PIPE_CONTROL_INTERRUPT_ENABLE	(1 << 8)
/* GT */
#define PIPE_CONTROL_VF_CACHE_INVALIDATE	(1 << 4)
#define PIPE_CONTROL_CONST_CACHE_INVALIDATE	(1 << 3)
#define PIPE_CONTROL_STATE_CACHE_INVALIDATE	(1 << 2)
#define PIPE_CONTROL_STALL_AT_SCOREBOARD	(1 << 1)
#define PIPE_CONTROL_DEPTH_CACHE_FLUSH		(1 << 0)
#define PIPE_CONTROL_PPGTT_WRITE	(0 << 2)
#define PIPE_CONTROL_GLOBAL_GTT_WRITE	(1 << 2)

/** @} */

#define XY_SETUP_BLT_CMD		(CMD_2D | (0x01 << 22))

#define XY_COLOR_BLT_CMD		(CMD_2D | (0x50 << 22))

#define XY_SRC_COPY_BLT_CMD             (CMD_2D | (0x53 << 22))

#define XY_TEXT_IMMEDIATE_BLIT_CMD	(CMD_2D | (0x31 << 22))
# define XY_TEXT_BYTE_PACKED		(1 << 16)

/* BR00 */
#define XY_BLT_WRITE_ALPHA	(1 << 21)
#define XY_BLT_WRITE_RGB	(1 << 20)
#define XY_SRC_TILED		(1 << 15)
#define XY_DST_TILED		(1 << 11)

/* BR13 */
#define BR13_8			(0x0 << 24)
#define BR13_565		(0x1 << 24)
#define BR13_8888		(0x3 << 24)

/* Pipeline Statistics Counter Registers */
#define IA_VERTICES_COUNT               0x2310
#define IA_PRIMITIVES_COUNT             0x2318
#define VS_INVOCATION_COUNT             0x2320
#define HS_INVOCATION_COUNT             0x2300
#define DS_INVOCATION_COUNT             0x2308
#define GS_INVOCATION_COUNT             0x2328
#define GS_PRIMITIVES_COUNT             0x2330
#define CL_INVOCATION_COUNT             0x2338
#define CL_PRIMITIVES_COUNT             0x2340
#define PS_INVOCATION_COUNT             0x2348
#define PS_DEPTH_COUNT                  0x2350

#define GEN6_SO_PRIM_STORAGE_NEEDED     0x2280
#define GEN7_SO_PRIM_STORAGE_NEEDED(n)  (0x5240 + (n) * 8)

#define GEN6_SO_NUM_PRIMS_WRITTEN       0x2288
#define GEN7_SO_NUM_PRIMS_WRITTEN(n)    (0x5200 + (n) * 8)

#define GEN7_SO_WRITE_OFFSET(n)         (0x5280 + (n) * 4)

#define TIMESTAMP                       0x2358

#define BCS_SWCTRL                      0x22200
# define BCS_SWCTRL_SRC_Y               (1 << 0)
# define BCS_SWCTRL_DST_Y               (1 << 1)

#define OACONTROL                       0x2360
# define OACONTROL_COUNTER_SELECT_SHIFT  2
# define OACONTROL_ENABLE_COUNTERS       (1 << 0)

/* Auto-Draw / Indirect Registers */
#define GEN7_3DPRIM_END_OFFSET          0x2420
#define GEN7_3DPRIM_START_VERTEX        0x2430
#define GEN7_3DPRIM_VERTEX_COUNT        0x2434
#define GEN7_3DPRIM_INSTANCE_COUNT      0x2438
#define GEN7_3DPRIM_START_INSTANCE      0x243C
#define GEN7_3DPRIM_BASE_VERTEX         0x2440
