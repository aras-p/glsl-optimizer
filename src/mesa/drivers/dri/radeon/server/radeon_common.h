/* radeon_common.h -- common header definitions for Radeon 2D/3D/DRM suite
 *
 * Copyright 2000 VA Linux Systems, Inc., Fremont, California.
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
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 * Converted to common header format:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/radeon_common.h,v 1.2 2003/04/07 01:22:09 martin Exp $
 *
 */

#ifndef _RADEON_COMMON_H_
#define _RADEON_COMMON_H_

#include <inttypes.h>
#include "xf86drm.h"

/* WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (radeon_drm.h)
 */

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_RADEON_CP_INIT                0x00
#define DRM_RADEON_CP_START               0x01
#define DRM_RADEON_CP_STOP                0x02
#define DRM_RADEON_CP_RESET               0x03
#define DRM_RADEON_CP_IDLE                0x04
#define DRM_RADEON_RESET                  0x05
#define DRM_RADEON_FULLSCREEN             0x06
#define DRM_RADEON_SWAP                   0x07
#define DRM_RADEON_CLEAR                  0x08
#define DRM_RADEON_VERTEX                 0x09
#define DRM_RADEON_INDICES                0x0a
#define DRM_RADEON_STIPPLE                0x0c
#define DRM_RADEON_INDIRECT               0x0d
#define DRM_RADEON_TEXTURE                0x0e
#define DRM_RADEON_VERTEX2                0x0f
#define DRM_RADEON_CMDBUF                 0x10
#define DRM_RADEON_GETPARAM               0x11
#define DRM_RADEON_FLIP                   0x12
#define DRM_RADEON_ALLOC                  0x13
#define DRM_RADEON_FREE                   0x14
#define DRM_RADEON_INIT_HEAP              0x15
#define DRM_RADEON_IRQ_EMIT               0x16
#define DRM_RADEON_IRQ_WAIT               0x17
#define DRM_RADEON_CP_RESUME              0x18
#define DRM_RADEON_SETPARAM               0x19
#define DRM_RADEON_MAX_DRM_COMMAND_INDEX  0x39


#define RADEON_FRONT	0x1
#define RADEON_BACK	0x2
#define RADEON_DEPTH	0x4
#define RADEON_STENCIL	0x8

#define RADEON_CLEAR_X1        0
#define RADEON_CLEAR_Y1        1
#define RADEON_CLEAR_X2        2
#define RADEON_CLEAR_Y2        3
#define RADEON_CLEAR_DEPTH     4


typedef struct {
   enum {
      DRM_RADEON_INIT_CP    = 0x01,
      DRM_RADEON_CLEANUP_CP = 0x02,
      DRM_RADEON_INIT_R200_CP = 0x03
   } func;
   unsigned long sarea_priv_offset;
   int is_pci;
   int cp_mode;
   int gart_size;
   int ring_size;
   int usec_timeout;

   unsigned int fb_bpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;
   unsigned int depth_bpp;
   unsigned int depth_offset, depth_pitch;

   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long ring_offset;
   unsigned long ring_rptr_offset;
   unsigned long buffers_offset;
   unsigned long gart_textures_offset;
} drmRadeonInit;

typedef struct {
   int flush;
   int idle;
} drmRadeonCPStop;

typedef struct {
   int idx;
   int start;
   int end;
   int discard;
} drmRadeonIndirect;

typedef union drmRadeonClearR {
        float f[5];
        unsigned int ui[5];
} drmRadeonClearRect;

typedef struct drmRadeonClearT {
        unsigned int flags;
        unsigned int clear_color;
        unsigned int clear_depth;
        unsigned int color_mask;
        unsigned int depth_mask;   /* misnamed field:  should be stencil */
        drmRadeonClearRect *depth_boxes;
} drmRadeonClearType;

typedef struct drmRadeonFullscreenT {
        enum {
                RADEON_INIT_FULLSCREEN    = 0x01,
                RADEON_CLEANUP_FULLSCREEN = 0x02
        } func;
} drmRadeonFullscreenType;

typedef struct {
        unsigned int *mask;
} drmRadeonStipple;

typedef struct {
        unsigned int x;
        unsigned int y;
        unsigned int width;
        unsigned int height;
        const void *data;
} drmRadeonTexImage;

typedef struct {
        unsigned int offset;
        int pitch;
        int format;
        int width;                      /* Texture image coordinates */
        int height;
        drmRadeonTexImage *image;
} drmRadeonTexture;


#define RADEON_MAX_TEXTURE_UNITS 3

/* Layout matches drm_radeon_state_t in linux drm_radeon.h.  
 */
typedef struct {
	struct {
		unsigned int pp_misc;				/* 0x1c14 */
		unsigned int pp_fog_color;
		unsigned int re_solid_color;
		unsigned int rb3d_blendcntl;
		unsigned int rb3d_depthoffset;
		unsigned int rb3d_depthpitch;
		unsigned int rb3d_zstencilcntl;
		unsigned int pp_cntl;				/* 0x1c38 */
		unsigned int rb3d_cntl;
		unsigned int rb3d_coloroffset;
		unsigned int re_width_height;
		unsigned int rb3d_colorpitch;
	} context;
	struct {
		unsigned int se_cntl;
	} setup1;
	struct {
		unsigned int se_coord_fmt;			/* 0x1c50 */
	} vertex;
	struct {
		unsigned int re_line_pattern;			/* 0x1cd0 */
		unsigned int re_line_state;
		unsigned int se_line_width;			/* 0x1db8 */
	} line;
	struct {
		unsigned int pp_lum_matrix;			/* 0x1d00 */
		unsigned int pp_rot_matrix_0;			/* 0x1d58 */
		unsigned int pp_rot_matrix_1;
	} bumpmap;
	struct {
		unsigned int rb3d_stencilrefmask;		/* 0x1d7c */
		unsigned int rb3d_ropcntl;
		unsigned int rb3d_planemask;
	} mask;
	struct {
		unsigned int se_vport_xscale;			/* 0x1d98 */
		unsigned int se_vport_xoffset;
		unsigned int se_vport_yscale;
		unsigned int se_vport_yoffset;
		unsigned int se_vport_zscale;
		unsigned int se_vport_zoffset;
	} viewport;
	struct {
		unsigned int se_cntl_status;			/* 0x2140 */
	} setup2;
	struct {
		unsigned int re_top_left;	/*ignored*/	/* 0x26c0 */
		unsigned int re_misc;
	} misc;
	struct {
		unsigned int pp_txfilter;
		unsigned int pp_txformat;
		unsigned int pp_txoffset;
		unsigned int pp_txcblend;
		unsigned int pp_txablend;
		unsigned int pp_tfactor;
		unsigned int pp_border_color;
	} texture[RADEON_MAX_TEXTURE_UNITS];
	struct {
		unsigned int se_zbias_factor; 
		unsigned int se_zbias_constant;
	} zbias;
	unsigned int dirty;
} drmRadeonState;

/* 1.1 vertex ioctl.  Used in compatibility modes.
 */
typedef struct {
	int prim;
	int idx;			/* Index of vertex buffer */
	int count;			/* Number of vertices in buffer */
	int discard;			/* Client finished with buffer? */
} drmRadeonVertex;

typedef struct {
	unsigned int start;
	unsigned int finish;
	unsigned int prim:8;
	unsigned int stateidx:8;
	unsigned int numverts:16; /* overloaded as offset/64 for elt prims */
        unsigned int vc_format;
} drmRadeonPrim;

typedef struct {
        int idx;                        /* Index of vertex buffer */
        int discard;                    /* Client finished with buffer? */
        int nr_states;
        drmRadeonState *state;
        int nr_prims;
        drmRadeonPrim *prim;
} drmRadeonVertex2;

#define RADEON_MAX_STATES 16
#define RADEON_MAX_PRIMS  64

/* Command buffer.  Replace with true dma stream?
 */
typedef struct {
	int bufsz;
	char *buf;
	int nbox;
        drmClipRect *boxes;
} drmRadeonCmdBuffer;

/* New style per-packet identifiers for use in cmd_buffer ioctl with
 * the RADEON_EMIT_PACKET command.  Comments relate new packets to old
 * state bits and the packet size:
 */
#define RADEON_EMIT_PP_MISC                         0 /* context/7 */
#define RADEON_EMIT_PP_CNTL                         1 /* context/3 */
#define RADEON_EMIT_RB3D_COLORPITCH                 2 /* context/1 */
#define RADEON_EMIT_RE_LINE_PATTERN                 3 /* line/2 */
#define RADEON_EMIT_SE_LINE_WIDTH                   4 /* line/1 */
#define RADEON_EMIT_PP_LUM_MATRIX                   5 /* bumpmap/1 */
#define RADEON_EMIT_PP_ROT_MATRIX_0                 6 /* bumpmap/2 */
#define RADEON_EMIT_RB3D_STENCILREFMASK             7 /* masks/3 */
#define RADEON_EMIT_SE_VPORT_XSCALE                 8 /* viewport/6 */
#define RADEON_EMIT_SE_CNTL                         9 /* setup/2 */
#define RADEON_EMIT_SE_CNTL_STATUS                  10 /* setup/1 */
#define RADEON_EMIT_RE_MISC                         11 /* misc/1 */
#define RADEON_EMIT_PP_TXFILTER_0                   12 /* tex0/6 */
#define RADEON_EMIT_PP_BORDER_COLOR_0               13 /* tex0/1 */
#define RADEON_EMIT_PP_TXFILTER_1                   14 /* tex1/6 */
#define RADEON_EMIT_PP_BORDER_COLOR_1               15 /* tex1/1 */
#define RADEON_EMIT_PP_TXFILTER_2                   16 /* tex2/6 */
#define RADEON_EMIT_PP_BORDER_COLOR_2               17 /* tex2/1 */
#define RADEON_EMIT_SE_ZBIAS_FACTOR                 18 /* zbias/2 */
#define RADEON_EMIT_SE_TCL_OUTPUT_VTX_FMT           19 /* tcl/11 */
#define RADEON_EMIT_SE_TCL_MATERIAL_EMMISSIVE_RED   20 /* material/17 */
#define R200_EMIT_PP_TXCBLEND_0                     21 /* tex0/4 */
#define R200_EMIT_PP_TXCBLEND_1                     22 /* tex1/4 */
#define R200_EMIT_PP_TXCBLEND_2                     23 /* tex2/4 */
#define R200_EMIT_PP_TXCBLEND_3                     24 /* tex3/4 */
#define R200_EMIT_PP_TXCBLEND_4                     25 /* tex4/4 */
#define R200_EMIT_PP_TXCBLEND_5                     26 /* tex5/4 */
#define R200_EMIT_PP_TXCBLEND_6                     27 /* /4 */
#define R200_EMIT_PP_TXCBLEND_7                     28 /* /4 */
#define R200_EMIT_TCL_LIGHT_MODEL_CTL_0             29 /* tcl/6 */
#define R200_EMIT_TFACTOR_0                         30 /* tf/6 */
#define R200_EMIT_VTX_FMT_0                         31 /* vtx/4 */
#define R200_EMIT_VAP_CTL                           32 /* vap/1 */
#define R200_EMIT_MATRIX_SELECT_0                   33 /* msl/5 */
#define R200_EMIT_TEX_PROC_CTL_2                    34 /* tcg/5 */
#define R200_EMIT_TCL_UCP_VERT_BLEND_CTL            35 /* tcl/1 */
#define R200_EMIT_PP_TXFILTER_0                     36 /* tex0/6 */
#define R200_EMIT_PP_TXFILTER_1                     37 /* tex1/6 */
#define R200_EMIT_PP_TXFILTER_2                     38 /* tex2/6 */
#define R200_EMIT_PP_TXFILTER_3                     39 /* tex3/6 */
#define R200_EMIT_PP_TXFILTER_4                     40 /* tex4/6 */
#define R200_EMIT_PP_TXFILTER_5                     41 /* tex5/6 */
#define R200_EMIT_PP_TXOFFSET_0                     42 /* tex0/1 */
#define R200_EMIT_PP_TXOFFSET_1                     43 /* tex1/1 */
#define R200_EMIT_PP_TXOFFSET_2                     44 /* tex2/1 */
#define R200_EMIT_PP_TXOFFSET_3                     45 /* tex3/1 */
#define R200_EMIT_PP_TXOFFSET_4                     46 /* tex4/1 */
#define R200_EMIT_PP_TXOFFSET_5                     47 /* tex5/1 */
#define R200_EMIT_VTE_CNTL                          48 /* vte/1 */
#define R200_EMIT_OUTPUT_VTX_COMP_SEL               49 /* vtx/1 */
#define R200_EMIT_PP_TAM_DEBUG3                     50 /* tam/1 */
#define R200_EMIT_PP_CNTL_X                         51 /* cst/1 */
#define R200_EMIT_RB3D_DEPTHXY_OFFSET               52 /* cst/1 */
#define R200_EMIT_RE_AUX_SCISSOR_CNTL               53 /* cst/1 */
#define R200_EMIT_RE_SCISSOR_TL_0                   54 /* cst/2 */
#define R200_EMIT_RE_SCISSOR_TL_1                   55 /* cst/2 */
#define R200_EMIT_RE_SCISSOR_TL_2                   56 /* cst/2 */
#define R200_EMIT_SE_VAP_CNTL_STATUS                57 /* cst/1 */
#define R200_EMIT_SE_VTX_STATE_CNTL                 58 /* cst/1 */
#define R200_EMIT_RE_POINTSIZE                      59 /* cst/1 */
#define R200_EMIT_TCL_INPUT_VTX_VECTOR_ADDR_0       60 /* cst/4 */
#define R200_EMIT_PP_CUBIC_FACES_0                  61
#define R200_EMIT_PP_CUBIC_OFFSETS_0                62
#define R200_EMIT_PP_CUBIC_FACES_1                  63
#define R200_EMIT_PP_CUBIC_OFFSETS_1                64
#define R200_EMIT_PP_CUBIC_FACES_2                  65
#define R200_EMIT_PP_CUBIC_OFFSETS_2                66
#define R200_EMIT_PP_CUBIC_FACES_3                  67
#define R200_EMIT_PP_CUBIC_OFFSETS_3                68
#define R200_EMIT_PP_CUBIC_FACES_4                  69
#define R200_EMIT_PP_CUBIC_OFFSETS_4                70
#define R200_EMIT_PP_CUBIC_FACES_5                  71
#define R200_EMIT_PP_CUBIC_OFFSETS_5                72
#define RADEON_EMIT_PP_TEX_SIZE_0                   73
#define RADEON_EMIT_PP_TEX_SIZE_1                   74
#define RADEON_EMIT_PP_TEX_SIZE_2                   75
#define RADEON_MAX_STATE_PACKETS                    76


/* Commands understood by cmd_buffer ioctl.  More can be added but
 * obviously these can't be removed or changed:
 */
#define RADEON_CMD_PACKET      1 /* emit one of the register packets above */
#define RADEON_CMD_SCALARS     2 /* emit scalar data */
#define RADEON_CMD_VECTORS     3 /* emit vector data */
#define RADEON_CMD_DMA_DISCARD 4 /* discard current dma buf */
#define RADEON_CMD_PACKET3     5 /* emit hw packet */
#define RADEON_CMD_PACKET3_CLIP 6 /* emit hw packet wrapped in cliprects */
#define RADEON_CMD_SCALARS2     7 /* R200 stopgap */
#define RADEON_CMD_WAIT         8 /* synchronization */

typedef union {
	int i;
	struct { 
	   unsigned char cmd_type, pad0, pad1, pad2;
	} header;
	struct { 
	   unsigned char cmd_type, packet_id, pad0, pad1;
	} packet;
	struct { 
	   unsigned char cmd_type, offset, stride, count; 
	} scalars;
	struct { 
	   unsigned char cmd_type, offset, stride, count; 
	} vectors;
	struct { 
	   unsigned char cmd_type, buf_idx, pad0, pad1; 
	} dma;
	struct { 
	   unsigned char cmd_type, flags, pad0, pad1; 
	} wait;
} drmRadeonCmdHeader;


#define RADEON_WAIT_2D  0x1
#define RADEON_WAIT_3D  0x2


/* 1.3: An ioctl to get parameters that aren't available to the 3d
 * client any other way.  
 */
#define RADEON_PARAM_GART_BUFFER_OFFSET    1 /* card offset of 1st GART buffer */
#define RADEON_PARAM_LAST_FRAME            2
#define RADEON_PARAM_LAST_DISPATCH         3
#define RADEON_PARAM_LAST_CLEAR            4
/* Added with DRM version 1.6. */
#define RADEON_PARAM_IRQ_NR                5
#define RADEON_PARAM_GART_BASE             6 /* card offset of GART base */
/* Added with DRM version 1.8. */
#define RADEON_PARAM_REGISTER_HANDLE       7 /* for drmMap() */
#define RADEON_PARAM_STATUS_HANDLE         8
#define RADEON_PARAM_SAREA_HANDLE          9
#define RADEON_PARAM_GART_TEX_HANDLE       10
#define RADEON_PARAM_SCRATCH_OFFSET        11

typedef struct drm_radeon_getparam {
	int param;
	int *value;
} drmRadeonGetParam;


#define RADEON_MEM_REGION_GART 1
#define RADEON_MEM_REGION_FB   2

typedef struct drm_radeon_mem_alloc {
	int region;
	int alignment;
	int size;
	int *region_offset;	/* offset from start of fb or GART */
} drmRadeonMemAlloc;

typedef struct drm_radeon_mem_free {
	int region;
	int region_offset;
} drmRadeonMemFree;

typedef struct drm_radeon_mem_init_heap {
	int region;
	int size;
	int start;	
} drmRadeonMemInitHeap;

/* 1.6: Userspace can request & wait on irq's:
 */
typedef struct drm_radeon_irq_emit {
	int *irq_seq;
} drmRadeonIrqEmit;

typedef struct drm_radeon_irq_wait {
	int irq_seq;
} drmRadeonIrqWait;


/* 1.10: Clients tell the DRM where they think the framebuffer is located in
 * the card's address space, via a new generic ioctl to set parameters
 */

typedef struct drm_radeon_set_param {
	unsigned int param;
	int64_t      value;
} drmRadeonSetParam;

#define RADEON_SETPARAM_FB_LOCATION     1


#endif
