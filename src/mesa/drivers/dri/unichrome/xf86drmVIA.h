/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __XF86DRI_VIA_H__
#define __XF86DRI_VIA_H__

#define DRM_VIA_ALLOCMEM	0x00
#define DRM_VIA_FREEMEM	        0x01
#define DRM_VIA_AGP_INIT	0x02
#define DRM_VIA_FB_INIT	        0x03
#define DRM_VIA_MAP_INIT	0x04
#define DRM_VIA_DEC_FUTEX       0x05
#define NOT_USED
#define DRM_VIA_DMA_INIT	0x07
#define DRM_VIA_CMDBUFFER	0x08
#define DRM_VIA_FLUSH	        0x09
#define DRM_VIA_PCICMD	        0x0a
#define DRM_VIA_CMDBUF_SIZE	0x0b

typedef struct {
    unsigned long sarea_priv_offset;
    unsigned long fb_offset;
    unsigned long mmio_offset;
    unsigned long agpAddr;
} drmVIAInit;

typedef struct {
    unsigned int offset;
    unsigned int size;
	unsigned int index;
} drmVIAAGPBuf;

typedef struct {
    unsigned int offset;
    unsigned int size;
    unsigned long index;
    unsigned long *address;
} drmVIADMABuf;

typedef struct {
  char *buf;
  unsigned long size;
} drmVIACommandBuffer;

typedef struct {
	enum {
		VIA_CMDBUF_SPACE = 0x01,
		VIA_CMDBUF_LAG = 0x02
	} func;
	int wait;
	unsigned size;
} drmVIACmdBufSize;

typedef struct {
	unsigned int offset;
	unsigned int size;
	unsigned long index;
	int discard;		/* client is finished with the buffer? */
} drmVIAFlush;

typedef struct{
	enum {
		VIA_INIT_DMA = 0x01,
		VIA_CLEANUP_DMA = 0x02,
                VIA_DMA_INITIALIZED = 0x03
	} func;

	unsigned long offset;
	unsigned long size;
	unsigned long reg_pause_addr;
} drmVIADMAInit;

extern int drmVIAAgpInit(int fd, int offset, int size);
extern int drmVIAFBInit(int fd, int offset, int size);
extern int drmVIAInitMAP(int fd, drmVIAInit *info);
extern int drmVIAAllocateDMA(int fd, drmVIADMABuf *buf);
extern int drmVIAReleaseDMA(int fd, drmVIADMABuf *buf);


#endif
