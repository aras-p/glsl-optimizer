/**
 * \file server/radeon_common.h 
 * \brief Common header definitions for Radeon 2D/3D/DRM driver suite.
 *
 * \note Some of these structures are meant for backward compatibility and
 * aren't used by the subset driver.
 *
 * \author Gareth Hughes <gareth@valinux.com>
 * \author Kevin E. Martin <martin@valinux.com>
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * 
 * \author Converted to common header format by
 * Jens Owen <jens@tungstengraphics.com>
 */

/*
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
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86drmRadeon.h,v 1.6 2001/04/16 15:02:13 tsi Exp $ */

#ifndef _RADEON_COMMON_H_
#define _RADEON_COMMON_H_

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


/**
 * \brief DRM_RADEON_CP_INIT ioctl argument type.
 */
typedef struct {
   enum {
      DRM_RADEON_INIT_CP    = 0x01,   /**< \brief initialize CP */
      DRM_RADEON_CLEANUP_CP = 0x02,   /**< \brief clean up CP */
      DRM_RADEON_INIT_R200_CP = 0x03  /**< \brief initialize R200 CP */
   } func;                            /**< \brief request */
   unsigned long sarea_priv_offset;   /**< \brief SAREA private offset */
   int is_pci;                        /**< \brief is current card a PCI card? */
   int cp_mode;                       /**< \brief CP mode */
   int agp_size;                      /**< \brief AGP space size */
   int ring_size;                     /**< \brief CP ring buffer size */
   int usec_timeout;                  /**< \brief timeout for DRM operations in usecs */

   unsigned int fb_bpp;               
   unsigned int front_offset;         /**< \brief front color buffer offset */
   unsigned int front_pitch;          /**< \brief front color buffer pitch */
   unsigned int back_offset;          /**< \brief back color buffer offset */
   unsigned int back_pitch;           /**< \brief back color buffer pitch*/
   unsigned int depth_bpp;            /**< \brief depth buffer bits-per-pixel */
   unsigned int depth_offset;         /**< \brief depth buffer offset */
   unsigned int depth_pitch;          /**< \brief depth buffer pitch */

   unsigned long fb_offset;           /**< \brief framebuffer offset */
   unsigned long mmio_offset;         /**< \brief MMIO register offset */
   unsigned long ring_offset;         /**< \brief CP ring buffer offset */
   unsigned long ring_rptr_offset;    /**< \brief CP ring buffer read pointer offset */
   unsigned long buffers_offset;      /**< \brief vertex buffers offset */
   unsigned long agp_textures_offset; /**< \brief AGP textures offset */
} drmRadeonInit;

/**
 * \brief DRM_RADEON_CP_STOP ioctl argument type.
 */
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

/**
 * \brief DRM_RADEON_CLEAR ioctl argument type.
 */
typedef struct drmRadeonClearT {
        unsigned int flags;              /**< \brief bitmask of the planes to clear */
        unsigned int clear_color;        /**< \brief color buffer clear value */
        unsigned int clear_depth;        /**< \brief depth buffer clear value */
        unsigned int color_mask;         /**< \brief color buffer clear mask */
        unsigned int depth_mask;         /**< \brief stencil buffer clear value
					   *  \todo Misnamed field. */
        drmRadeonClearRect *depth_boxes; /**< \brief depth buffer cliprects */
} drmRadeonClearType;

typedef struct drmRadeonFullscreenT {
        enum {
                RADEON_INIT_FULLSCREEN    = 0x01,
                RADEON_CLEANUP_FULLSCREEN = 0x02
        } func;
} drmRadeonFullscreenType;

/**
 * \brief DRM_RADEON_STIPPLE ioctl argument type.
 */
typedef struct {
        unsigned int *mask;
} drmRadeonStipple;

/**
 * \brief Texture image for drmRadeonTexture.
 */
typedef struct {
        unsigned int x;
        unsigned int y;
        unsigned int width;
        unsigned int height;
        const void *data;
} drmRadeonTexImage;

/**
 * \brief DRM_RADEON_TEXTURE ioctl argument type.
 */
typedef struct {
        int offset;               /**< \brief texture offset */
        int pitch;                /**< \brief texture pitch */
        int format;               /**< \brief pixel format */
        int width;                /**< \brief texture width */
        int height;               /**< \brief texture height */
	drmRadeonTexImage *image; /**< \brief image */
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

/**
 * \brief DRM 1.1 vertex ioctl.
 *
 * Used in compatibility modes.
 */
typedef struct {
	int prim;			/**< \brief Primitive number */
	int idx;			/**< \brief Index of vertex buffer */
	int count;			/**< \brief Number of vertices in buffer */
	int discard;			/**< \brief Client finished with buffer? */
} drmRadeonVertex;

typedef struct {
	unsigned int start;
	unsigned int finish;
	unsigned int prim:8;
	unsigned int stateidx:8;
	unsigned int numverts:16;	/**< overloaded as offset/64 for elt prims */
        unsigned int vc_format;
} drmRadeonPrim;

typedef struct {
        int idx;                        /**< \brief Index of vertex buffer */
        int discard;                    /**< \brief Client finished with buffer? */
        int nr_states;
        drmRadeonState *state;
        int nr_prims;
        drmRadeonPrim *prim;
} drmRadeonVertex2;

#define RADEON_MAX_STATES 16
#define RADEON_MAX_PRIMS  64


/**
 * \brief Command buffer.  
 *
 * \todo Replace with true DMA stream?
 */
typedef struct {
	int bufsz;          /**< \brief buffer size */
	char *buf;          /**< \brief buffer */
	int nbox;           /**< \brief number of cliprects */
        drmClipRect *boxes; /**< \brief cliprects */
} drmRadeonCmdBuffer;


/**
 * \brief Per-packet identifiers for use with the ::RADEON_CMD_PACKET command
 * in the DRM_RADEON_CMDBUF ioctl.  
 *
 * \note Comments relate new packets to old state bits and the packet size.
 */
enum drmRadeonCmdPkt {
   RADEON_EMIT_PP_MISC                       = 0, /* context/7 */
   RADEON_EMIT_PP_CNTL                       = 1, /* context/3 */
   RADEON_EMIT_RB3D_COLORPITCH               = 2, /* context/1 */
   RADEON_EMIT_RE_LINE_PATTERN               = 3, /* line/2 */
   RADEON_EMIT_SE_LINE_WIDTH                 = 4, /* line/1 */
   RADEON_EMIT_PP_LUM_MATRIX                 = 5, /* bumpmap/1 */
   RADEON_EMIT_PP_ROT_MATRIX_0               = 6, /* bumpmap/2 */
   RADEON_EMIT_RB3D_STENCILREFMASK           = 7, /* masks/3 */
   RADEON_EMIT_SE_VPORT_XSCALE               = 8, /* viewport/6 */
   RADEON_EMIT_SE_CNTL                       = 9, /* setup/2 */
   RADEON_EMIT_SE_CNTL_STATUS                = 10, /* setup/1 */
   RADEON_EMIT_RE_MISC                       = 11, /* misc/1 */
   RADEON_EMIT_PP_TXFILTER_0                 = 12, /* tex0/6 */
   RADEON_EMIT_PP_BORDER_COLOR_0             = 13, /* tex0/1 */
   RADEON_EMIT_PP_TXFILTER_1                 = 14, /* tex1/6 */
   RADEON_EMIT_PP_BORDER_COLOR_1             = 15, /* tex1/1 */
   RADEON_EMIT_PP_TXFILTER_2                 = 16, /* tex2/6 */
   RADEON_EMIT_PP_BORDER_COLOR_2             = 17, /* tex2/1 */
   RADEON_EMIT_SE_ZBIAS_FACTOR               = 18, /* zbias/2 */
   RADEON_EMIT_SE_TCL_OUTPUT_VTX_FMT         = 19, /* tcl/11 */
   RADEON_EMIT_SE_TCL_MATERIAL_EMMISSIVE_RED = 20, /* material/17 */
   R200_EMIT_PP_TXCBLEND_0                   = 21, /* tex0/4 */
   R200_EMIT_PP_TXCBLEND_1                   = 22, /* tex1/4 */
   R200_EMIT_PP_TXCBLEND_2                   = 23, /* tex2/4 */
   R200_EMIT_PP_TXCBLEND_3                   = 24, /* tex3/4 */
   R200_EMIT_PP_TXCBLEND_4                   = 25, /* tex4/4 */
   R200_EMIT_PP_TXCBLEND_5                   = 26, /* tex5/4 */
   R200_EMIT_PP_TXCBLEND_6                   = 27, /* /4 */
   R200_EMIT_PP_TXCBLEND_7                   = 28, /* /4 */
   R200_EMIT_TCL_LIGHT_MODEL_CTL_0           = 29, /* tcl/6 */
   R200_EMIT_TFACTOR_0                       = 30, /* tf/6 */
   R200_EMIT_VTX_FMT_0                       = 31, /* vtx/4 */
   R200_EMIT_VAP_CTL                         = 32, /* vap/1 */
   R200_EMIT_MATRIX_SELECT_0                 = 33, /* msl/5 */
   R200_EMIT_TEX_PROC_CTL_2                  = 34, /* tcg/5 */
   R200_EMIT_TCL_UCP_VERT_BLEND_CTL          = 35, /* tcl/1 */
   R200_EMIT_PP_TXFILTER_0                   = 36, /* tex0/6 */
   R200_EMIT_PP_TXFILTER_1                   = 37, /* tex1/6 */
   R200_EMIT_PP_TXFILTER_2                   = 38, /* tex2/6 */
   R200_EMIT_PP_TXFILTER_3                   = 39, /* tex3/6 */
   R200_EMIT_PP_TXFILTER_4                   = 40, /* tex4/6 */
   R200_EMIT_PP_TXFILTER_5                   = 41, /* tex5/6 */
   R200_EMIT_PP_TXOFFSET_0                   = 42, /* tex0/1 */
   R200_EMIT_PP_TXOFFSET_1                   = 43, /* tex1/1 */
   R200_EMIT_PP_TXOFFSET_2                   = 44, /* tex2/1 */
   R200_EMIT_PP_TXOFFSET_3                   = 45, /* tex3/1 */
   R200_EMIT_PP_TXOFFSET_4                   = 46, /* tex4/1 */
   R200_EMIT_PP_TXOFFSET_5                   = 47, /* tex5/1 */
   R200_EMIT_VTE_CNTL                        = 48, /* vte/1 */
   R200_EMIT_OUTPUT_VTX_COMP_SEL             = 49, /* vtx/1 */
   R200_EMIT_PP_TAM_DEBUG3                   = 50, /* tam/1 */
   R200_EMIT_PP_CNTL_X                       = 51, /* cst/1 */
   R200_EMIT_RB3D_DEPTHXY_OFFSET             = 52, /* cst/1 */
   R200_EMIT_RE_AUX_SCISSOR_CNTL             = 53, /* cst/1 */
   R200_EMIT_RE_SCISSOR_TL_0                 = 54, /* cst/2 */
   R200_EMIT_RE_SCISSOR_TL_1                 = 55, /* cst/2 */
   R200_EMIT_RE_SCISSOR_TL_2                 = 56, /* cst/2 */
   R200_EMIT_SE_VAP_CNTL_STATUS              = 57, /* cst/1 */
   R200_EMIT_SE_VTX_STATE_CNTL               = 58, /* cst/1 */
   R200_EMIT_RE_POINTSIZE                    = 59, /* cst/1 */
   R200_EMIT_TCL_INPUT_VTX_VECTOR_ADDR_0     = 60, /* cst/4 */
   R200_EMIT_PP_CUBIC_FACES_0                = 61,
   R200_EMIT_PP_CUBIC_OFFSETS_0              = 62,
   R200_EMIT_PP_CUBIC_FACES_1                = 63,
   R200_EMIT_PP_CUBIC_OFFSETS_1              = 64,
   R200_EMIT_PP_CUBIC_FACES_2                = 65,
   R200_EMIT_PP_CUBIC_OFFSETS_2              = 66,
   R200_EMIT_PP_CUBIC_FACES_3                = 67,
   R200_EMIT_PP_CUBIC_OFFSETS_3              = 68,
   R200_EMIT_PP_CUBIC_FACES_4                = 69,
   R200_EMIT_PP_CUBIC_OFFSETS_4              = 70,
   R200_EMIT_PP_CUBIC_FACES_5                = 71,
   R200_EMIT_PP_CUBIC_OFFSETS_5              = 72,
   RADEON_MAX_STATE_PACKETS                  = 73
} ;


/**
 * \brief Command types understood by the DRM_RADEON_CMDBUF ioctl.  
 * 
 * More can be added but obviously these can't be removed or changed.
 *
 * \sa drmRadeonCmdHeader.
 */
enum drmRadeonCmdType {
   RADEON_CMD_PACKET       = 1, /**< \brief emit one of the ::drmRadeonCmdPkt register packets */
   RADEON_CMD_SCALARS      = 2, /**< \brief emit scalar data */
   RADEON_CMD_VECTORS      = 3, /**< \brief emit vector data */
   RADEON_CMD_DMA_DISCARD  = 4, /**< \brief discard current DMA buffer */
   RADEON_CMD_PACKET3      = 5, /**< \brief emit hardware packet */
   RADEON_CMD_PACKET3_CLIP = 6, /**< \brief emit hardware packet wrapped in cliprects */
   RADEON_CMD_SCALARS2     = 7, /**< \brief R200 stopgap */
   RADEON_CMD_WAIT         = 8  /**< \brief synchronization */
} ;

/**
 * \brief Command packet headers understood by the DRM_RADEON_CMDBUF ioctl.
 *
 * \sa drmRadeonCmdType.
 */
typedef union {
	/** \brief integer equivalent */
	int i;

	struct { 
	   unsigned char cmd_type, pad0, pad1, pad2;
	} header;

	/** \brief emit a register packet */
	struct { 
	   unsigned char cmd_type, packet_id, pad0, pad1;
	} packet;
	
	/** \brief scalar data */
	struct { 
	   unsigned char cmd_type, offset, stride, count; 
	} scalars;
	
	/** \brief vector data */
	struct { 
	   unsigned char cmd_type, offset, stride, count; 
	} vectors;
	
	/** \brief discard current DMA buffer */
	struct { 
	   unsigned char cmd_type, buf_idx, pad0, pad1; 
	} dma;
	
	/** \brief synchronization */
	struct { 
	   unsigned char cmd_type, flags, pad0, pad1; 
	} wait;
} drmRadeonCmdHeader;


#define RADEON_WAIT_2D  0x1
#define RADEON_WAIT_3D  0x2

/**
 * \brief DRM_RADEON_GETPARAM ioctl argument type.
 */
typedef struct drm_radeon_getparam {
	int param;  /**< \brief parameter number */
	int *value; /**< \brief parameter value */
} drmRadeonGetParam;

#define RADEON_PARAM_AGP_BUFFER_OFFSET 1
#define RADEON_PARAM_LAST_FRAME        2
#define RADEON_PARAM_LAST_DISPATCH     3
#define RADEON_PARAM_LAST_CLEAR        4
#define RADEON_PARAM_IRQ_NR            5
#define RADEON_PARAM_AGP_BASE          6
#define RADEON_PARAM_REGISTER_HANDLE   7 
#define RADEON_PARAM_STATUS_HANDLE     8
#define RADEON_PARAM_SAREA_HANDLE      9
#define RADEON_PARAM_AGP_TEX_HANDLE    10


#define RADEON_MEM_REGION_AGP 1
#define RADEON_MEM_REGION_FB  2

typedef struct drm_radeon_mem_alloc {
	int region;
	int alignment;
	int size;
	int *region_offset;	/* offset from start of fb or agp */
} drmRadeonMemAlloc;

typedef struct drm_radeon_mem_free {
	int region;
	int region_offset;
} drmRadeonMemFree;

/**
 * \brief DRM_RADEON_INIT_HEAP argument type.
 */
typedef struct drm_radeon_mem_init_heap {
	int region; /**< \brief region type */
	int size;   /**< \brief region size */
	int start;  /**< \brief region start offset */
} drmRadeonMemInitHeap;

/**
 * \brief DRM_RADEON_IRQ_EMIT ioctl argument type.
 *
 * New in DRM 1.6: userspace can request and wait on IRQ's.
 */
typedef struct drm_radeon_irq_emit {
	int *irq_seq;
} drmRadeonIrqEmit;

/**
 * \brief DRM_RADEON_IRQ_WAIT ioctl argument type.
 *
 * New in DRM 1.6: userspace can request and wait on IRQ's.
 */
typedef struct drm_radeon_irq_wait {
	int irq_seq;
} drmRadeonIrqWait;


#endif
