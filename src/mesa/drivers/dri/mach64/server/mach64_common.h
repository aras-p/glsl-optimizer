/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/* mach64_common.h -- common header definitions for Rage Pro 2D/3D/DRM suite
 * Created: Sun Dec 03 11:34:16 2000 by gareth@valinux.com
 *
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
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
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *	Gareth Hughes <gareth@valinux.com>
 *      Leif Delgass <ldelgass@retinalburn.net>
 */

#ifndef __MACH64_COMMON_H__
#define __MACH64_COMMON_H__ 1

/* WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (mach64_drm.h)
 */

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_MACH64_INIT           0x00
#define DRM_MACH64_IDLE           0x01
#define DRM_MACH64_RESET          0x02
#define DRM_MACH64_SWAP           0x03
#define DRM_MACH64_CLEAR          0x04
#define DRM_MACH64_VERTEX         0x05
#define DRM_MACH64_BLIT           0x06
#define DRM_MACH64_FLUSH          0x07
#define DRM_MACH64_GETPARAM       0x08

/* Buffer flags for clears
 */
#define MACH64_FRONT	          0x1
#define MACH64_BACK	          0x2
#define MACH64_DEPTH	          0x4

/* Primitive types for vertex buffers
 */
#define MACH64_PRIM_POINTS		0x00000000
#define MACH64_PRIM_LINES		0x00000001
#define MACH64_PRIM_LINE_LOOP		0x00000002
#define MACH64_PRIM_LINE_STRIP		0x00000003
#define MACH64_PRIM_TRIANGLES		0x00000004
#define MACH64_PRIM_TRIANGLE_STRIP	0x00000005
#define MACH64_PRIM_TRIANGLE_FAN	0x00000006
#define MACH64_PRIM_QUADS		0x00000007
#define MACH64_PRIM_QUAD_STRIP		0x00000008
#define MACH64_PRIM_POLYGON		0x00000009


typedef enum _drmMach64DMAMode {
   MACH64_MODE_DMA_ASYNC,
   MACH64_MODE_DMA_SYNC,
   MACH64_MODE_MMIO
} drmMach64DMAMode;

typedef struct {
   enum {
      DRM_MACH64_INIT_DMA    = 0x01,
      DRM_MACH64_CLEANUP_DMA = 0x02
   } func;
   unsigned long sarea_priv_offset;
   int is_pci;
   drmMach64DMAMode dma_mode;

   unsigned int fb_bpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;

   unsigned int depth_bpp;
   unsigned int depth_offset, depth_pitch;

   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long ring_offset;
   unsigned long buffers_offset;
   unsigned long agp_textures_offset;
} drmMach64Init;

typedef struct {
   unsigned int flags;
   int x, y, w, h;
   unsigned int clear_color;
   unsigned int clear_depth;
} drmMach64Clear;

typedef struct {
   int prim;
   void *buf;			/* Address of vertex buffer */
   unsigned long used;		/* Number of bytes in buffer */
   int discard;			/* Client finished with buffer? */
} drmMach64Vertex;

typedef struct {
   int idx;
   int pitch;
   int offset;
   int format;
   unsigned short x, y;
   unsigned short width, height;
} drmMach64Blit;

typedef struct {
   int param;
   int *value;
} drmMach64GetParam;

#define MACH64_PARAM_FRAMES_QUEUED 1
#define MACH64_PARAM_IRQ_NR       2

#endif /* __MACH64_COMMON_H__ */
