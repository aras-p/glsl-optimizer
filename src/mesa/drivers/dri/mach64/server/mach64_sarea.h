/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
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
 */

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Leif Delgass <ldelgass@retinalburn.net>
 */

#ifndef __MACH64_SAREA_H__
#define __MACH64_SAREA_H__ 1

/* WARNING: If you change any of these defines, make sure to change the
 * defines in the kernel file (mach64_drm.h)
 */
#ifndef __MACH64_SAREA_DEFINES__
#define __MACH64_SAREA_DEFINES__ 1

/* What needs to be changed for the current vertex buffer?
 * GH: We're going to be pedantic about this.  We want the card to do as
 * little as possible, so let's avoid having it fetch a whole bunch of
 * register values that don't change all that often, if at all.
 */
#define MACH64_UPLOAD_DST_OFF_PITCH	0x0001
#define MACH64_UPLOAD_Z_OFF_PITCH	0x0002
#define MACH64_UPLOAD_Z_ALPHA_CNTL	0x0004
#define MACH64_UPLOAD_SCALE_3D_CNTL	0x0008
#define MACH64_UPLOAD_DP_FOG_CLR	0x0010
#define MACH64_UPLOAD_DP_WRITE_MASK	0x0020
#define MACH64_UPLOAD_DP_PIX_WIDTH	0x0040
#define MACH64_UPLOAD_SETUP_CNTL	0x0080
#define MACH64_UPLOAD_MISC		0x0100
#define MACH64_UPLOAD_TEXTURE		0x0200
#define MACH64_UPLOAD_TEX0IMAGE		0x0400
#define MACH64_UPLOAD_TEX1IMAGE		0x0800
#define MACH64_UPLOAD_CLIPRECTS		0x1000 /* handled client-side */
#define MACH64_UPLOAD_CONTEXT		0x00ff
#define MACH64_UPLOAD_ALL		0x1fff

/* DMA buffer size
 */
#define MACH64_BUFFER_SIZE		16384

/* Max number of swaps allowed on the ring
 * before the client must wait
 */
#define MACH64_MAX_QUEUED_FRAMES        3

/* Byte offsets for host blit buffer data
 */
#define MACH64_HOSTDATA_BLIT_OFFSET	104

/* Keep these small for testing.
 */
#define MACH64_NR_SAREA_CLIPRECTS	8


#define MACH64_CARD_HEAP		0
#define MACH64_AGP_HEAP			1
#define MACH64_NR_TEX_HEAPS		2
#define MACH64_NR_TEX_REGIONS		64
#define MACH64_LOG_TEX_GRANULARITY	16

#define MACH64_TEX_MAXLEVELS		1

#define MACH64_NR_CONTEXT_REGS		15
#define MACH64_NR_TEXTURE_REGS		4

#endif /* __MACH64_SAREA_DEFINES__ */

typedef struct {
   /* Context state */
   unsigned int dst_off_pitch;		/* 0x500 */

   unsigned int z_off_pitch;		/* 0x548 */ /* ****** */
   unsigned int z_cntl;			/* 0x54c */
   unsigned int alpha_tst_cntl;		/* 0x550 */

   unsigned int scale_3d_cntl;		/* 0x5fc */

   unsigned int sc_left_right;		/* 0x6a8 */
   unsigned int sc_top_bottom;		/* 0x6b4 */

   unsigned int dp_fog_clr;		/* 0x6c4 */
   unsigned int dp_write_mask;		/* 0x6c8 */
   unsigned int dp_pix_width;		/* 0x6d0 */
   unsigned int dp_mix;			/* 0x6d4 */ /* ****** */
   unsigned int dp_src;			/* 0x6d8 */ /* ****** */

   unsigned int clr_cmp_cntl;		/* 0x708 */ /* ****** */
   unsigned int gui_traj_cntl;		/* 0x730 */ /* ****** */

   unsigned int setup_cntl;		/* 0x304 */

   /* Texture state */
   unsigned int tex_size_pitch;		/* 0x770 */
   unsigned int tex_cntl;		/* 0x774 */
   unsigned int secondary_tex_off;	/* 0x778 */
   unsigned int tex_offset;		/* 0x5c0 */
} mach64_context_regs_t;

typedef struct {
   /* The channel for communication of state information to the kernel
    * on firing a vertex buffer.
    */
   mach64_context_regs_t ContextState;
   unsigned int dirty;
   unsigned int vertsize;

   /* The current cliprects, or a subset thereof.
    */
   XF86DRIClipRectRec boxes[MACH64_NR_SAREA_CLIPRECTS];
   unsigned int nbox;

   /* Counter for throttling of rendering clients.
    */
   unsigned int frames_queued;

   /* Maintain an LRU of contiguous regions of texture space.  If you
    * think you own a region of texture memory, and it has an age
    * different to the one you set, then you are mistaken and it has
    * been stolen by another client.  If global texAge hasn't changed,
    * there is no need to walk the list.
    *
    * These regions can be used as a proxy for the fine-grained texture
    * information of other clients - by maintaining them in the same
    * lru which is used to age their own textures, clients have an
    * approximate lru for the whole of global texture space, and can
    * make informed decisions as to which areas to kick out.  There is
    * no need to choose whether to kick out your own texture or someone
    * else's - simply eject them all in LRU order.
    */
   drmTextureRegion texList[MACH64_NR_TEX_HEAPS][MACH64_NR_TEX_REGIONS+1];
   unsigned int texAge[MACH64_NR_TEX_HEAPS];

   int ctxOwner;     /* last context to upload state */
} ATISAREAPrivRec, *ATISAREAPrivPtr;

#endif /* __MACH64_SAREA_H__ */
