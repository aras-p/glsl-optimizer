/* mga_common.h -- common header definitions for MGA 2D/3D/DRM suite
 *
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
 * Converted to common header format:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_common.h,v 1.2 2002/12/16 16:19:18 dawes Exp $
 *
 */

#ifndef _MGA_COMMON_H_
#define _MGA_COMMON_H_

/*
 * WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (mga_drm.h)
 */

#define  DRM_MGA_IDLE_RETRY          2048
#define  DRM_MGA_NR_TEX_HEAPS        2

typedef struct {
   int installed;
   unsigned long phys_addr;
   int size;
} drmMGAWarpIndex;

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_MGA_INIT              0x00
#define DRM_MGA_FLUSH             0x01
#define DRM_MGA_RESET             0x02
#define DRM_MGA_SWAP              0x03
#define DRM_MGA_CLEAR             0x04
#define DRM_MGA_VERTEX            0x05
#define DRM_MGA_INDICES           0x06
#define DRM_MGA_ILOAD             0x07
#define DRM_MGA_BLIT              0x08
#define DRM_MGA_GETPARAM          0x09

typedef struct {
   enum {
      MGA_INIT_DMA    = 0x01,
      MGA_CLEANUP_DMA = 0x02
   } func;

   unsigned long sarea_priv_offset;

   int chipset;
   int sgram;

   unsigned int maccess;

   unsigned int fb_cpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;

   unsigned int depth_cpp;
   unsigned int depth_offset, depth_pitch;

   unsigned int texture_offset[DRM_MGA_NR_TEX_HEAPS];
   unsigned int texture_size[DRM_MGA_NR_TEX_HEAPS];

   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long status_offset;
   unsigned long warp_offset;
   unsigned long primary_offset;
   unsigned long buffers_offset;
} drmMGAInit;

typedef enum {
   DRM_MGA_LOCK_READY      = 0x01, /* Wait until hardware is ready for DMA */
   DRM_MGA_LOCK_QUIESCENT  = 0x02, /* Wait until hardware quiescent        */
   DRM_MGA_LOCK_FLUSH      = 0x04, /* Flush this context's DMA queue first */
   DRM_MGA_LOCK_FLUSH_ALL  = 0x08, /* Flush all DMA queues first           */
                                   /* These *HALT* flags aren't supported yet
                                      -- they will be used to support the
                                         full-screen DGA-like mode.        */
   DRM_MGA_HALT_ALL_QUEUES = 0x10, /* Halt all current and future queues   */
   DRM_MGA_HALT_CUR_QUEUES = 0x20  /* Halt all current queues              */
} drmMGALockFlags;

typedef struct {
   int             context;
   drmMGALockFlags flags;
} drmMGALock;

typedef struct {
   int idx;
   unsigned int dstorg;
   unsigned int length;
} drmMGAIload;

typedef struct {
   unsigned int flags;
   unsigned int clear_color;
   unsigned int clear_depth;
   unsigned int color_mask;
   unsigned int depth_mask;
} drmMGAClearRec;

typedef struct {
   int idx;                        /* buffer to queue */
   int used;                       /* bytes in use */
   int discard;                    /* client finished with buffer?  */
} drmMGAVertex;

typedef struct {
        unsigned int planemask;
        unsigned int srcorg;
        unsigned int dstorg;
        int src_pitch, dst_pitch;
        int delta_sx, delta_sy;
        int delta_dx, delta_dy;
        int height, ydir;               /* flip image vertically */
        int source_pitch, dest_pitch;
} drmMGABlit;

/* 3.1: An ioctl to get parameters that aren't available to the 3d
 * client any other way.  
 */
#define MGA_PARAM_IRQ_NR            1

typedef struct {
	int param;
	int *value;
} drmMGAGetParam;

#endif
