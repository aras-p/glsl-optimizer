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

#define CMD_MI				(0x0 << 29)
#define CMD_2D				(0x2 << 29)
#define CMD_3D				(0x3 << 29)

#define MI_BATCH_BUFFER_END		(CMD_MI | 0xA << 23)

/* Stalls command execution waiting for the given events to have occurred. */
#define MI_WAIT_FOR_EVENT               (CMD_MI | (0x3 << 23))
#define MI_WAIT_FOR_PLANE_B_FLIP        (1<<6)
#define MI_WAIT_FOR_PLANE_A_FLIP        (1<<2)

/* Primitive dispatch on 830-945 */
#define _3DPRIMITIVE			(CMD_3D | (0x1f << 24))
#define PRIM_INDIRECT            (1<<23)
#define PRIM_INLINE              (0<<23)
#define PRIM_INDIRECT_SEQUENTIAL (0<<17)
#define PRIM_INDIRECT_ELTS       (1<<17)

#define PRIM3D_TRILIST		(0x0<<18)
#define PRIM3D_TRISTRIP 	(0x1<<18)
#define PRIM3D_TRISTRIP_RVRSE	(0x2<<18)
#define PRIM3D_TRIFAN		(0x3<<18)
#define PRIM3D_POLY		(0x4<<18)
#define PRIM3D_LINELIST 	(0x5<<18)
#define PRIM3D_LINESTRIP	(0x6<<18)
#define PRIM3D_RECTLIST 	(0x7<<18)
#define PRIM3D_POINTLIST	(0x8<<18)
#define PRIM3D_DIB		(0x9<<18)
#define PRIM3D_MASK		(0x1f<<18)

#define XY_SETUP_BLT_CMD		(CMD_2D | (0x01 << 22) | 6)

#define XY_COLOR_BLT_CMD		(CMD_2D | (0x50 << 22) | 4)

#define XY_SRC_COPY_BLT_CMD             (CMD_2D | (0x53 << 22) | 6)

#define XY_TEXT_IMMEDIATE_BLIT_CMD	(CMD_2D | (0x31 << 22))
# define XY_TEXT_BYTE_PACKED		(1 << 16)

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
