/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_CONTEXT_H__
#define __R300_CONTEXT_H__

#include "tnl/t_vertex.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "texmem.h"

#include "macros.h"
#include "mtypes.h"
#include "colormac.h"
#include "radeon_context.h"

struct r300_context;
typedef struct r300_context r300ContextRec;
typedef struct r300_context *r300ContextPtr;

#include "radeon_lock.h"
#include "mm.h"

typedef GLuint uint32_t;
typedef GLubyte uint8_t;


static __inline__ uint32_t r300PackFloat32(float fl)
{
	union { float fl; uint32_t u; } u;

	u.fl = fl;
	return u.u;
}

/**
 * A block of hardware state.
 *
 * When check returns non-zero, the returned number of dwords must be
 * copied verbatim into the command buffer in order to update a state atom
 * when it is dirty.
 */
struct r300_state_atom {
	struct r300_state_atom *next, *prev;
	const char* name;	/* for debug */
	int cmd_size;		/* maximum size in dwords */
	GLuint idx;		/* index in an array (e.g. textures) */
	uint32_t* cmd;
	GLboolean dirty;

	int (*check)(r300ContextPtr, struct r300_state_atom* atom);
};


#define R300_VPT_CMD_0		0
#define R300_VPT_XSCALE		1
#define R300_VPT_XOFFSET	2
#define R300_VPT_YSCALE		3
#define R300_VPT_YOFFSET	4
#define R300_VPT_ZSCALE		5
#define R300_VPT_ZOFFSET	6
#define R300_VPT_CMDSIZE	7

#define R300_OVF_CMD_0		0
#define R300_OVF_FMT_0		1
#define R300_OVF_FMT_1		2
#define R300_OVF_CMDSIZE	3

#define R300_VIR_CMD_0		0 /* vir is variable size (at least 1) */
#define R300_VIR_CNTL_0		1
#define R300_VIR_CNTL_1		2
#define R300_VIR_CNTL_2		3
#define R300_VIR_CNTL_3		4
#define R300_VIR_CNTL_4		5
#define R300_VIR_CNTL_5		6
#define R300_VIR_CNTL_6		7
#define R300_VIR_CNTL_7		8
#define R300_VIR_CMDSIZE	9

#define R300_VIC_CMD_0		0
#define R300_VIC_CNTL_0		1
#define R300_VIC_CNTL_1		2
#define R300_VIC_CMDSIZE	3

#define R300_PVS_CMD_0		0
#define R300_PVS_CNTL_1		1
#define R300_PVS_CNTL_2		2
#define R300_PVS_CNTL_3		3
#define R300_PVS_CMDSIZE	4

#define R300_TXE_CMD_0		0
#define R300_TXE_ENABLE		1
#define R300_TXE_CMDSIZE	2

#define R300_PS_CMD_0		0
#define R300_PS_POINTSIZE	1
#define R300_PS_CMDSIZE		2

#define R300_CUL_CMD_0		0
#define R300_CUL_CULL		1
#define R300_CUL_CMDSIZE	2

#define R300_RC_CMD_0		0
#define R300_RC_CNTL_0		1
#define R300_RC_CNTL_1		2
#define R300_RC_CMDSIZE		3

#define R300_RI_CMD_0		0
#define R300_RI_INTERP_0	1
#define R300_RI_INTERP_1	2
#define R300_RI_INTERP_2	3
#define R300_RI_INTERP_3	4
#define R300_RI_INTERP_4	5
#define R300_RI_INTERP_5	6
#define R300_RI_INTERP_6	7
#define R300_RI_INTERP_7	8
#define R300_RI_CMDSIZE		9

#define R300_RR_CMD_0		0 /* rr is variable size (at least 1) */
#define R300_RR_ROUTE_0		1
#define R300_RR_ROUTE_1		2
#define R300_RR_ROUTE_2		3
#define R300_RR_ROUTE_3		4
#define R300_RR_ROUTE_4		5
#define R300_RR_ROUTE_5		6
#define R300_RR_ROUTE_6		7
#define R300_RR_ROUTE_7		8
#define R300_RR_CMDSIZE		9

#define R300_FP_CMD_0		0
#define R300_FP_CNTL0		1
#define R300_FP_CNTL1		2
#define R300_FP_CNTL2		3
#define R300_FP_CMD_1		4
#define R300_FP_NODE0		5
#define R300_FP_NODE1		6
#define R300_FP_NODE2		7
#define R300_FP_NODE3		8
#define R300_FP_CMDSIZE		9

#define R300_FPI_CMD_0		0
#define R300_FPI_INSTR_0	1
#define R300_FPI_CMDSIZE	65

#define R300_AT_CMD_0		0
#define R300_AT_ALPHA_TEST	1
#define R300_AT_CMDSIZE		2

#define R300_BLD_CMD_0		0
#define R300_BLD_CBLEND		1
#define R300_BLD_ABLEND		2
#define R300_BLD_CMDSIZE	3

#define R300_CMK_CMD_0		0
#define R300_CMK_COLORMASK	1
#define R300_CMK_CMDSIZE	2

#define R300_CB_CMD_0		0
#define R300_CB_OFFSET		1
#define R300_CB_CMD_1		2
#define R300_CB_PITCH		3
#define R300_CB_CMDSIZE		4

#define R300_ZC_CMD_0		0
#define R300_ZC_CNTL_0		1
#define R300_ZC_CNTL_1		2
#define R300_ZC_CMDSIZE		3

#define R300_ZB_CMD_0		0
#define R300_ZB_OFFSET		1
#define R300_ZB_PITCH		2
#define R300_ZB_CMDSIZE		3

#define R300_VPI_CMD_0		0
#define R300_VPI_INSTR_0	1
#define R300_VPI_CMDSIZE	1025 /* 256 16 byte instructions */

#define R300_VPP_CMD_0		0
#define R300_VPP_PARAM_0	1
#define R300_VPP_CMDSIZE	1025 /* 256 4-component parameters */

#define R300_VPS_CMD_0		0
#define R300_VPS_ZERO_0		1
#define R300_VPS_ZERO_1		2
#define R300_VPS_POINTSIZE	3
#define R300_VPS_ZERO_3		4
#define R300_VPS_CMDSIZE	5

/**
 * Cache for hardware register state.
 */
struct r300_hw_state {
	struct r300_state_atom atomlist;

	GLboolean	is_dirty;
	GLboolean	all_dirty;
	int		max_state_size;	/* in dwords */

	struct r300_state_atom vpt;	/* viewport (1D98) */
	struct r300_state_atom unk2080;	/* (2080) */
	struct r300_state_atom ovf;	/* output vertex format (2090) */
	struct r300_state_atom unk20B0;	/* (20B0) */
	struct r300_state_atom unk2134;	/* (2134) */
	struct r300_state_atom unk2140;	/* (2140) */
	struct r300_state_atom vir[2];	/* vap input route (2150/21E0) */
	struct r300_state_atom vic;	/* vap input control (2180) */
	struct r300_state_atom unk21DC; /* (21DC) */
	struct r300_state_atom unk221C; /* (221C) */
	struct r300_state_atom unk2220; /* (2220) */
	struct r300_state_atom unk2288; /* (2288) */
	struct r300_state_atom pvs;	/* pvs_cntl (22D0) */
	struct r300_state_atom unk4008; /* (4008) */
	struct r300_state_atom unk4010; /* (4010) */
	struct r300_state_atom txe;	/* tex enable (4104) */
	struct r300_state_atom unk4200; /* (4200) */
	struct r300_state_atom unk4214; /* (4214) */
	struct r300_state_atom ps;	/* pointsize (421C) */
	struct r300_state_atom unk4230; /* (4230) */
	struct r300_state_atom unk4260; /* (4260) */
	struct r300_state_atom unk4274; /* (4274) */
	struct r300_state_atom unk4288; /* (4288) */
	struct r300_state_atom unk42A0; /* (42A0) */
	struct r300_state_atom unk42B4; /* (42B4) */
	struct r300_state_atom cul;	/* cull cntl (42B8) */
	struct r300_state_atom unk42C0; /* (42C0) */
	struct r300_state_atom rc;	/* rs control (4300) */
	struct r300_state_atom ri;	/* rs interpolators (4310) */
	struct r300_state_atom rr;	/* rs route (4330) */
	struct r300_state_atom unk43A4;	/* (43A4) */
	struct r300_state_atom unk43E8;	/* (43E8) */
	struct r300_state_atom fp;	/* fragment program cntl + nodes (4600) */
	struct r300_state_atom unk46A4;	/* (46A4) */
	struct r300_state_atom fpi[4];	/* fp instructions (46C0/47C0/48C0/49C0) */
	struct r300_state_atom unk4BC0;	/* (4BC0) */
	struct r300_state_atom unk4BC8;	/* (4BC8) */
	struct r300_state_atom at;	/* alpha test (4BD4) */
	struct r300_state_atom unk4BD8;	/* (4BD8) */
	struct r300_state_atom unk4E00;	/* (4E00) */
	struct r300_state_atom bld;	/* blending (4E04) */
	struct r300_state_atom cmk;	/* colormask (4E0C) */
	struct r300_state_atom unk4E10;	/* (4E10) */
	struct r300_state_atom cb;	/* colorbuffer (4E28) */
	struct r300_state_atom unk4E50;	/* (4E50) */
	struct r300_state_atom unk4E88;	/* (4E88) */
	struct r300_state_atom zc;	/* z control (4F00) */
	struct r300_state_atom unk4F08;	/* (4F08) */
	struct r300_state_atom unk4F10;	/* (4F10) */
	struct r300_state_atom zb;	/* z buffer (4F20) */
	struct r300_state_atom unk4F28;	/* (4F28) */
	struct r300_state_atom unk4F30;	/* (4F30) */
	struct r300_state_atom unk4F44;	/* (4F44) */
	struct r300_state_atom unk4F54;	/* (4F54) */

	struct r300_state_atom vpi;	/* vp instructions */
	struct r300_state_atom vpp;	/* vp parameters */
	struct r300_state_atom vps;	/* vertex point size (?) */
};


/**
 * This structure holds the command buffer while it is being constructed.
 *
 * The first batch of commands in the buffer is always the state that needs
 * to be re-emitted when the context is lost. This batch can be skipped
 * otherwise.
 */
struct r300_cmdbuf {
	int		size;		/* DWORDs allocated for buffer */
	uint32_t*	cmd_buf;
	int		count_used;	/* DWORDs filled so far */
	int		count_reemit;	/* size of re-emission batch */
};


/**
 * State cache
 */
 
struct r300_depthbuffer_state {
	GLfloat scale;
};

struct r300_state {
	struct r300_depthbuffer_state depth;
};


/**
 * R300 context structure.
 */
struct r300_context {
	struct radeon_context radeon; /* parent class, must be first */

	struct r300_hw_state hw;
	struct r300_cmdbuf cmdbuf;
	struct r300_state state;
	
	/* Vertex buffers */
	int elt_count;  /* size of the buffer for vertices */
	int attrib_count; /* size of the buffer for vertex attributes.. Somehow it can be different ? */
	
};

#define R300_CONTEXT(ctx)		((r300ContextPtr)(ctx->DriverCtx))

extern void r300DestroyContext(__DRIcontextPrivate * driContextPriv);
extern GLboolean r300CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);

#endif				/* __R300_CONTEXT_H__ */
