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
#ifndef _VIA_DRM_H_
#define _VIA_DRM_H_

/* WARNING: These defines must be the same as what the Xserver uses.
 * if you change them, you must change the defines in the Xserver.
 */

#ifndef _VIA_DEFINES_
#define _VIA_DEFINES_

#define VIA_DMA_BUF_ORDER		12
#define VIA_DMA_BUF_SZ 		        (1 << VIA_DMA_BUF_ORDER)
#define VIA_DMA_BUF_NR 			256
#define VIA_NR_SAREA_CLIPRECTS 		8

#define VIA_NR_TEX_REGIONS 64
#define VIA_LOG_MIN_TEX_REGION_SIZE 16
#endif


/* VIA specific ioctls */
#define DRM_IOCTL_VIA_ALLOCMEM	DRM_IOWR(0x40, drm_via_mem_t)
#define DRM_IOCTL_VIA_FREEMEM	DRM_IOW(0x41, drm_via_mem_t)
#define DRM_IOCTL_VIA_AGP_INIT	DRM_IOWR(0x42, drm_via_agp_t)
#define DRM_IOCTL_VIA_FB_INIT	DRM_IOWR(0x43, drm_via_fb_t)
#define DRM_IOCTL_VIA_MAP_INIT	DRM_IOWR(0x44, drm_via_init_t)
#define DRM_IOCTL_VIA_FLUSH_AGP	DRM_IOW(0x45, drm_via_flush_agp_t)
#define DRM_IOCTL_VIA_FLUSH_SYS	DRM_IOW(0x46, drm_via_flush_sys_t)

#define VIA_TEX_SETUP_SIZE 8

#define VIA_FRONT   0x1
#define VIA_BACK    0x2
#define VIA_DEPTH   0x4
#define VIA_STENCIL 0x8
#define VIDEO 0
#define AGP 1
typedef struct {
    unsigned int offset;
    unsigned int size;
} drm_via_agp_t;    

typedef struct {
    unsigned int offset;
    unsigned int size;
} drm_via_fb_t;    

typedef struct {
    unsigned int context;
    unsigned int type;
    unsigned int size;
    unsigned long index;
    unsigned long offset;
} drm_via_mem_t;    

typedef struct _drm_via_init {
    enum {
	VIA_INIT_MAP = 0x01,
	VIA_CLEANUP_MAP = 0x02
    } func;
    unsigned long sarea_priv_offset;
    unsigned long fb_offset;
    unsigned long mmio_offset;
    unsigned long agpAddr;
} drm_via_init_t;


typedef struct _drm_via_tex_region {
    unsigned char next, prev;
    unsigned char inUse;
    int age;
} drm_via_tex_region_t;

typedef struct _drm_via_sarea {
    unsigned int dirty;
    unsigned int nbox;
    drm_clip_rect_t boxes[VIA_NR_SAREA_CLIPRECTS];   
    drm_via_tex_region_t texList[VIA_NR_TEX_REGIONS + 1]; 
    int texAge;
    int ctxOwner;
    int vertexPrim;
} drm_via_sarea_t;

typedef struct _drm_via_flush_agp {
    unsigned int offset;
    unsigned int size;
    unsigned int index;		
    int discard;
} drm_via_flush_agp_t;

typedef struct _drm_via_flush_sys {
    unsigned int offset;
    unsigned int size;
    unsigned long index;		
    int discard;
} drm_via_flush_sys_t;

#ifdef __KERNEL__
int via_fb_init(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);		
int via_mem_alloc(struct inode *inode, struct file *filp, unsigned int cmd,
  		unsigned long arg);				
int via_mem_free(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);		
int via_agp_init(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);				
int via_dma_alloc(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);				
int via_dma_free(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);				
int via_map_init(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);				
int via_flush_agp(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);				
int via_flush_sys(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg);				
#endif
#endif /* _VIA_DRM_H_ */
