/* r128_common.h -- common header definitions for R128 2D/3D/DRM suite
 * Created: Sun Apr  9 18:16:28 2000 by kevin@precisioninsight.com
 *
 * Copyright 1999, 2000 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2002 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 * Converted to common header format:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86drmR128.h,v 3.11 2001/04/16 15:02:13 tsi Exp $
 *
 */

#ifndef _R128_COMMON_H_
#define _R128_COMMON_H_

/*
 * WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (r128_drm.h)
 */

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_R128_INIT           0x00
#define DRM_R128_CCE_START      0x01
#define DRM_R128_CCE_STOP       0x02
#define DRM_R128_CCE_RESET      0x03
#define DRM_R128_CCE_IDLE       0x04
#define DRM_R128_UNDEFINED1     0x05
#define DRM_R128_RESET          0x06
#define DRM_R128_SWAP           0x07
#define DRM_R128_CLEAR          0x08
#define DRM_R128_VERTEX         0x09
#define DRM_R128_INDICES        0x0a
#define DRM_R128_BLIT           0x0b
#define DRM_R128_DEPTH          0x0c
#define DRM_R128_STIPPLE        0x0d
#define DRM_R128_UNDEFINED2     0x0e
#define DRM_R128_INDIRECT       0x0f
#define DRM_R128_FULLSCREEN     0x10
#define DRM_R128_CLEAR2         0x11
#define DRM_R128_GETPARAM       0x12
#define DRM_R128_FLIP           0x13

#define DRM_R128_FRONT_BUFFER	0x1
#define DRM_R128_BACK_BUFFER	0x2
#define DRM_R128_DEPTH_BUFFER	0x4

typedef struct {
   enum {
      DRM_R128_INIT_CCE    = 0x01,
      DRM_R128_CLEANUP_CCE = 0x02
   } func;
   unsigned long sarea_priv_offset;
   int is_pci;
   int cce_mode;
   int cce_secure;		/* FIXME: Deprecated, we should remove this */
   int ring_size;
   int usec_timeout;

   unsigned int fb_bpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;
   unsigned int depth_bpp;
   unsigned int depth_offset, depth_pitch;
   unsigned int span_offset;

   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long ring_offset;
   unsigned long ring_rptr_offset;
   unsigned long buffers_offset;
   unsigned long agp_textures_offset;
} drmR128Init;

typedef struct {
   int flush;
   int idle;
} drmR128CCEStop;

typedef struct {
   int idx;
   int start;
   int end;
   int discard;
} drmR128Indirect;

typedef struct {
   int idx;
   int pitch;
   int offset;
   int format;
   unsigned short x, y;
   unsigned short width, height;
} drmR128Blit;

typedef struct {
   enum {
      DRM_R128_WRITE_SPAN         = 0x01,
      DRM_R128_WRITE_PIXELS       = 0x02,
      DRM_R128_READ_SPAN          = 0x03,
      DRM_R128_READ_PIXELS        = 0x04
   } func;
   int n;
   int *x;
   int *y;
   unsigned int *buffer;
   unsigned char *mask;
} drmR128Depth;

typedef struct {
   int prim;
   int idx;                        /* Index of vertex buffer */
   int count;                      /* Number of vertices in buffer */
   int discard;                    /* Client finished with buffer? */
} drmR128Vertex;

typedef struct {
   unsigned int *mask;
} drmR128Stipple;

typedef struct {
   unsigned int flags;
   unsigned int clear_color;
   unsigned int clear_depth;
   unsigned int color_mask;
   unsigned int depth_mask;
} drmR128Clear;

typedef struct {
   enum {
      DRM_R128_INIT_FULLSCREEN    = 0x01,
      DRM_R128_CLEANUP_FULLSCREEN = 0x02
   } func;
} drmR128Fullscreen;

typedef struct drm_r128_getparam {
	int param;
	void *value;
} drmR128GetParam;

#define R128_PARAM_IRQ_NR            1

#endif
