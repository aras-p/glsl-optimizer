/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_sarea.h,v 1.7 2002/02/16 21:26:35 herrb Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef _R128_SAREA_H_
#define _R128_SAREA_H_

/* WARNING: If you change any of these defines, make sure to change the
 * defines in the kernel file (r128_drm.h)
 */
#ifndef __R128_SAREA_DEFINES__
#define __R128_SAREA_DEFINES__

/* What needs to be changed for the current vertex buffer?
 */
#define R128_UPLOAD_CONTEXT		0x001
#define R128_UPLOAD_SETUP		0x002
#define R128_UPLOAD_TEX0		0x004
#define R128_UPLOAD_TEX1		0x008
#define R128_UPLOAD_TEX0IMAGES		0x010
#define R128_UPLOAD_TEX1IMAGES		0x020
#define R128_UPLOAD_CORE		0x040
#define R128_UPLOAD_MASKS		0x080
#define R128_UPLOAD_WINDOW		0x100
#define R128_UPLOAD_CLIPRECTS		0x200	/* handled client-side */
#define R128_REQUIRE_QUIESCENCE		0x400
#define R128_UPLOAD_ALL			0x7ff

#define R128_FRONT			0x1
#define R128_BACK			0x2
#define R128_DEPTH			0x4

/* Primitive types
 */
#define R128_POINTS			0x1
#define R128_LINES			0x2
#define R128_LINE_STRIP			0x3
#define R128_TRIANGLES			0x4
#define R128_TRIANGLE_FAN		0x5
#define R128_TRIANGLE_STRIP		0x6

/* Vertex/indirect buffer size
 */
#define R128_BUFFER_SIZE		16384

/* Byte offsets for indirect buffer data
 */
#define R128_INDEX_PRIM_OFFSET		20
#define R128_HOSTDATA_BLIT_OFFSET	32

/* Keep these small for testing
 */
#define R128_NR_SAREA_CLIPRECTS		12

/* There are 2 heaps (local/AGP).  Each region within a heap is a
 * minimum of 64k, and there are at most 64 of them per heap.
 */
#define R128_CARD_HEAP			0
#define R128_AGP_HEAP			1
#define R128_NR_TEX_HEAPS		2
#define R128_NR_TEX_REGIONS		64
#define R128_LOG_TEX_GRANULARITY	16

#define R128_NR_CONTEXT_REGS		12

#define R128_MAX_TEXTURE_LEVELS		11
#define R128_MAX_TEXTURE_UNITS		2

#endif /* __R128_SAREA_DEFINES__ */

typedef struct {
    /* Context state - can be written in one large chunk */
    unsigned int dst_pitch_offset_c;
    unsigned int dp_gui_master_cntl_c;
    unsigned int sc_top_left_c;
    unsigned int sc_bottom_right_c;
    unsigned int z_offset_c;
    unsigned int z_pitch_c;
    unsigned int z_sten_cntl_c;
    unsigned int tex_cntl_c;
    unsigned int misc_3d_state_cntl_reg;
    unsigned int texture_clr_cmp_clr_c;
    unsigned int texture_clr_cmp_msk_c;
    unsigned int fog_color_c;

    /* Texture state */
    unsigned int tex_size_pitch_c;
    unsigned int constant_color_c;

    /* Setup state */
    unsigned int pm4_vc_fpu_setup;
    unsigned int setup_cntl;

    /* Mask state */
    unsigned int dp_write_mask;
    unsigned int sten_ref_mask_c;
    unsigned int plane_3d_mask_c;

    /* Window state */
    unsigned int window_xy_offset;

    /* Core state */
    unsigned int scale_3d_cntl;
} r128_context_regs_t;

/* Setup registers for each texture unit
 */
typedef struct {
    unsigned int tex_cntl;
    unsigned int tex_combine_cntl;
    unsigned int tex_size_pitch;
    unsigned int tex_offset[R128_MAX_TEXTURE_LEVELS];
    unsigned int tex_border_color;
} r128_texture_regs_t;

typedef struct {
    /* The channel for communication of state information to the kernel
     * on firing a vertex buffer.
     */
    r128_context_regs_t	ContextState;
    r128_texture_regs_t	TexState[R128_MAX_TEXTURE_UNITS];
    unsigned int dirty;
    unsigned int vertsize;
    unsigned int vc_format;

#if defined(XF86DRI) | defined(_SOLO)
    /* The current cliprects, or a subset thereof.
     */
    drm_clip_rect_t boxes[R128_NR_SAREA_CLIPRECTS];
    unsigned int nbox;
#endif

    /* Counters for throttling of rendering clients.
     */
    unsigned int last_frame;
    unsigned int last_dispatch;

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
				/* Last elt is sentinal */
    drmTextureRegion texList[R128_NR_TEX_HEAPS][R128_NR_TEX_REGIONS+1];
				/* last time texture was uploaded */
    unsigned int texAge[R128_NR_TEX_HEAPS];

    int ctxOwner;		/* last context to upload state */
    int pfAllowPageFlip;	/* set by the 2d driver, read by the client */
    int pfCurrentPage;		/* set by kernel, read by others */
} R128SAREAPriv, *R128SAREAPrivPtr;

#endif
