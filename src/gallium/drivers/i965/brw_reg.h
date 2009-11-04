/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef BRW_REG_H
#define BRW_REG_H

#define CMD_MI				(0x0 << 29)
#define CMD_2D				(0x2 << 29)
#define CMD_3D				(0x3 << 29)

#define MI_NOOP				(CMD_MI | 0)
#define MI_BATCH_BUFFER_END		(CMD_MI | 0xA << 23)
#define MI_FLUSH			(CMD_MI | (4 << 23))

#define _3DSTATE_DRAWRECT_INFO_I965	(CMD_3D | (3 << 27) | (1 << 24) | 0x2)

/** @{
 *
 * PIPE_CONTROL operation, a combination MI_FLUSH and register write with
 * additional flushing control.
 */
#define _3DSTATE_PIPE_CONTROL		(CMD_3D | (3 << 27) | (2 << 24) | 2)
#define PIPE_CONTROL_NO_WRITE		(0 << 14)
#define PIPE_CONTROL_WRITE_IMMEDIATE	(1 << 14)
#define PIPE_CONTROL_WRITE_DEPTH_COUNT	(2 << 14)
#define PIPE_CONTROL_WRITE_TIMESTAMP	(3 << 14)
#define PIPE_CONTROL_DEPTH_STALL	(1 << 13)
#define PIPE_CONTROL_WRITE_FLUSH	(1 << 12)
#define PIPE_CONTROL_INSTRUCTION_FLUSH	(1 << 11)
#define PIPE_CONTROL_INTERRUPT_ENABLE	(1 << 8)
#define PIPE_CONTROL_PPGTT_WRITE	(0 << 2)
#define PIPE_CONTROL_GLOBAL_GTT_WRITE	(1 << 2)

/** @} */

#define XY_SETUP_BLT_CMD		(CMD_2D | (0x01 << 22) | 6)
#define XY_COLOR_BLT_CMD		(CMD_2D | (0x50 << 22) | 4)
#define XY_SRC_COPY_BLT_CMD             (CMD_2D | (0x53 << 22) | 6)

/* BR00 */
#define XY_BLT_WRITE_ALPHA	(1 << 21)
#define XY_BLT_WRITE_RGB	(1 << 20)
#define XY_SRC_TILED		(1 << 15)
#define XY_DST_TILED		(1 << 11)

/* BR13 */
#define BR13_565		(0x1 << 24)
#define BR13_8888		(0x3 << 24)

#define FENCE_LINEAR 0
#define FENCE_XMAJOR 1
#define FENCE_YMAJOR 2



/* PCI IDs
 */
#define PCI_CHIP_I965_G			0x29A2
#define PCI_CHIP_I965_Q			0x2992
#define PCI_CHIP_I965_G_1		0x2982
#define PCI_CHIP_I946_GZ		0x2972
#define PCI_CHIP_I965_GM                0x2A02
#define PCI_CHIP_I965_GME               0x2A12

#define PCI_CHIP_GM45_GM                0x2A42

#define PCI_CHIP_IGD_E_G                0x2E02
#define PCI_CHIP_Q45_G                  0x2E12
#define PCI_CHIP_G45_G                  0x2E22
#define PCI_CHIP_G41_G                  0x2E32
#define PCI_CHIP_B43_G                  0x2E42

#define PCI_CHIP_ILD_G                  0x0042
#define PCI_CHIP_ILM_G                  0x0046

struct brw_chipset {
   unsigned pci_id:16;
   unsigned is_965:1;
   unsigned is_igdng:1;
   unsigned is_g4x:1;
   unsigned pad:13;
};


/* XXX: hacks
 */
#define VERT_RESULT_HPOS 0	/* not always true */
#define VERT_RESULT_PSIZ 10000	/* disabled */


#endif
