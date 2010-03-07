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

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_CONTEXT_H__
#define __R300_CONTEXT_H__

#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "radeon_common.h"

#include "main/mtypes.h"
#include "shader/prog_instruction.h"
#include "compiler/radeon_code.h"

struct r300_context;
typedef struct r300_context r300ContextRec;
typedef struct r300_context *r300ContextPtr;


#include "r300_vertprog.h"


/* The blit width for texture uploads
 */
#define R300_BLIT_WIDTH_BYTES 1024
#define R300_MAX_TEXTURE_UNITS 8



#define R300_VPT_CMD_0		0
#define R300_VPT_XSCALE		1
#define R300_VPT_XOFFSET	2
#define R300_VPT_YSCALE		3
#define R300_VPT_YOFFSET	4
#define R300_VPT_ZSCALE		5
#define R300_VPT_ZOFFSET	6
#define R300_VPT_CMDSIZE	7

#define R300_VIR_CMD_0		0	/* vir is variable size (at least 1) */
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

#define R300_VOF_CMD_0		0
#define R300_VOF_CNTL_0		1
#define R300_VOF_CNTL_1		2
#define R300_VOF_CMDSIZE	3

#define R300_PVS_CMD_0		0
#define R300_PVS_CNTL_1		1
#define R300_PVS_CNTL_2		2
#define R300_PVS_CNTL_3		3
#define R300_PVS_CMDSIZE	4

#define R300_GB_MISC_CMD_0		0
#define R300_GB_MISC_MSPOS_0		1
#define R300_GB_MISC_MSPOS_1		2
#define R300_GB_MISC_TILE_CONFIG	3
#define R300_GB_MISC_CMDSIZE		4
#define R300_GB_MISC2_CMD_0		    0
#define R300_GB_MISC2_SELECT		1
#define R300_GB_MISC2_AA_CONFIG		2
#define R300_GB_MISC2_CMDSIZE		3

#define R300_TXE_CMD_0		0
#define R300_TXE_ENABLE		1
#define R300_TXE_CMDSIZE	2

#define R300_PS_CMD_0		0
#define R300_PS_POINTSIZE	1
#define R300_PS_CMDSIZE		2

#define R300_ZBS_CMD_0		0
#define R300_ZBS_T_FACTOR	1
#define R300_ZBS_T_CONSTANT	2
#define R300_ZBS_W_FACTOR	3
#define R300_ZBS_W_CONSTANT	4
#define R300_ZBS_CMDSIZE	5

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

#define R500_RI_CMDSIZE	       17

#define R300_RR_CMD_0		0	/* rr is variable size (at least 1) */
#define R300_RR_INST_0		1
#define R300_RR_INST_1		2
#define R300_RR_INST_2		3
#define R300_RR_INST_3		4
#define R300_RR_INST_4		5
#define R300_RR_INST_5		6
#define R300_RR_INST_6		7
#define R300_RR_INST_7		8
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

#define R500_FP_CMD_0           0
#define R500_FP_CNTL            1
#define R500_FP_PIXSIZE         2
#define R500_FP_CMD_1           3
#define R500_FP_CODE_ADDR       4
#define R500_FP_CODE_RANGE      5
#define R500_FP_CODE_OFFSET     6
#define R500_FP_CMD_2           7
#define R500_FP_FC_CNTL         8
#define R500_FP_CMDSIZE         9

#define R300_FPT_CMD_0		0
#define R300_FPT_INSTR_0	1
#define R300_FPT_CMDSIZE	65

#define R300_FPI_CMD_0		0
#define R300_FPI_INSTR_0	1
#define R300_FPI_CMDSIZE	65
/* R500 has space for 512 instructions - 6 dwords per instruction */
#define R500_FPI_CMDSIZE	(512*6+1)

#define R300_FPP_CMD_0		0
#define R300_FPP_PARAM_0	1
#define R300_FPP_CMDSIZE	(32*4+1)
/* R500 has spcae for 256 constants - 4 dwords per constant */
#define R500_FPP_CMDSIZE	(256*4+1)

#define R300_FOGS_CMD_0		0
#define R300_FOGS_STATE		1
#define R300_FOGS_CMDSIZE	2

#define R300_FOGC_CMD_0		0
#define R300_FOGC_R		1
#define R300_FOGC_G		2
#define R300_FOGC_B		3
#define R300_FOGC_CMDSIZE	4

#define R300_FOGP_CMD_0		0
#define R300_FOGP_SCALE		1
#define R300_FOGP_START		2
#define R300_FOGP_CMDSIZE	3

#define R300_AT_CMD_0		0
#define R300_AT_ALPHA_TEST	1
#define R300_AT_UNKNOWN		2
#define R300_AT_CMDSIZE		3

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

#define R300_ZS_CMD_0		0
#define R300_ZS_CNTL_0		1
#define R300_ZS_CNTL_1		2
#define R300_ZS_CNTL_2		3
#define R300_ZS_CMDSIZE		4

#define R300_ZSB_CMD_0		0
#define R300_ZSB_CNTL_0		1
#define R300_ZSB_CMDSIZE	2

#define R300_ZB_CMD_0		0
#define R300_ZB_OFFSET		1
#define R300_ZB_PITCH		2
#define R300_ZB_CMDSIZE		3

#define R300_VAP_CNTL_FLUSH     0
#define R300_VAP_CNTL_FLUSH_1   1
#define R300_VAP_CNTL_CMD       2
#define R300_VAP_CNTL_INSTR     3
#define R300_VAP_CNTL_SIZE      4

#define R300_VPI_CMD_0		0
#define R300_VPI_INSTR_0	1
#define R300_VPI_CMDSIZE	1025	/* 256 16 byte instructions */

#define R300_VPP_CMD_0		0
#define R300_VPP_PARAM_0	1
#define R300_VPP_CMDSIZE	1025	/* 256 4-component parameters */

#define R300_VPUCP_CMD_0		0
#define R300_VPUCP_X            1
#define R300_VPUCP_Y            2
#define R300_VPUCP_Z            3
#define R300_VPUCP_W            4
#define R300_VPUCP_CMDSIZE	5	/* 256 4-component parameters */

#define R300_VPS_CMD_0		0
#define R300_VPS_ZERO_0		1
#define R300_VPS_ZERO_1		2
#define R300_VPS_POINTSIZE	3
#define R300_VPS_ZERO_3		4
#define R300_VPS_CMDSIZE	5

	/* the layout is common for all fields inside tex */
#define R300_TEX_CMD_0		0
#define R300_TEX_VALUE_0	1
/* We don't really use this, instead specify mtu+1 dynamically
#define R300_TEX_CMDSIZE	(MAX_TEXTURE_UNITS+1)
*/

#define R300_QUERYOBJ_CMD_0  0
#define R300_QUERYOBJ_DATA_0 1
#define R300_QUERYOBJ_CMD_1  2
#define R300_QUERYOBJ_DATA_1  3
#define R300_QUERYOBJ_CMDSIZE  4

/**
 * Cache for hardware register state.
 */
struct r300_hw_state {
	struct radeon_state_atom vpt;	/* viewport (1D98) */
	struct radeon_state_atom vap_cntl;
	struct radeon_state_atom vap_index_offset; /* 0x208c r5xx only */
	struct radeon_state_atom vof;	/* VAP output format register 0x2090 */
	struct radeon_state_atom vte;	/* (20B0) */
	struct radeon_state_atom vap_vf_max_vtx_indx;	/* Maximum Vertex Indx Clamp (2134) */
	struct radeon_state_atom vap_cntl_status;
	struct radeon_state_atom vir[2];	/* vap input route (2150/21E0) */
	struct radeon_state_atom vic;	/* vap input control (2180) */
	struct radeon_state_atom vap_psc_sgn_norm_cntl; /* Programmable Stream Control Signed Normalize Control (21DC) */
	struct radeon_state_atom vap_clip_cntl;
	struct radeon_state_atom vap_clip;
	struct radeon_state_atom vap_pvs_vtx_timeout_reg;	/* Vertex timeout register (2288) */
	struct radeon_state_atom pvs;	/* pvs_cntl (22D0) */
	struct radeon_state_atom gb_enable;	/* (4008) */
	struct radeon_state_atom gb_misc;	/* Multisampling position shifts ? (4010) */
	struct radeon_state_atom gb_misc2;	/* Multisampling position shifts ? (4010) */
	struct radeon_state_atom ga_point_s0;	/* S Texture Coordinate of Vertex 0 for Point texture stuffing (LLC) (4200) */
	struct radeon_state_atom ga_triangle_stipple;	/* (4214) */
	struct radeon_state_atom ps;	/* pointsize (421C) */
	struct radeon_state_atom ga_point_minmax;	/* (4230) */
	struct radeon_state_atom lcntl;	/* line control */
	struct radeon_state_atom ga_line_stipple;	/* (4260) */
	struct radeon_state_atom shade;
	struct radeon_state_atom shade2;
	struct radeon_state_atom polygon_mode;
	struct radeon_state_atom fogp;	/* fog parameters (4294) */
	struct radeon_state_atom ga_soft_reset;	/* (429C) */
	struct radeon_state_atom zbias_cntl;
	struct radeon_state_atom zbs;	/* zbias (42A4) */
	struct radeon_state_atom occlusion_cntl;
	struct radeon_state_atom cul;	/* cull cntl (42B8) */
	struct radeon_state_atom su_depth_scale;	/* (42C0) */
	struct radeon_state_atom rc;	/* rs control (4300) */
	struct radeon_state_atom ri;	/* rs interpolators (4310) */
	struct radeon_state_atom rr;	/* rs route (4330) */
	struct radeon_state_atom sc_hyperz;	/* (43A4) */
	struct radeon_state_atom sc_screendoor;	/* (43E8) */
	struct radeon_state_atom fp;	/* fragment program cntl + nodes (4600) */
	struct radeon_state_atom fpt;	/* texi - (4620) */
	struct radeon_state_atom us_out_fmt;	/* (46A4) */
	struct radeon_state_atom r500fp;	/* r500 fp instructions */
	struct radeon_state_atom r500fp_const;	/* r500 fp constants */
	struct radeon_state_atom fpi[4];	/* fp instructions (46C0/47C0/48C0/49C0) */
	struct radeon_state_atom fogs;	/* fog state (4BC0) */
	struct radeon_state_atom fogc;	/* fog color (4BC8) */
	struct radeon_state_atom at;	/* alpha test (4BD4) */
	struct radeon_state_atom fg_depth_src;	/* (4BD8) */
	struct radeon_state_atom fpp;	/* 0x4C00 and following */
	struct radeon_state_atom rb3d_cctl;	/* (4E00) */
	struct radeon_state_atom bld;	/* blending (4E04) */
	struct radeon_state_atom cmk;	/* colormask (4E0C) */
	struct radeon_state_atom blend_color;	/* constant blend color */
	struct radeon_state_atom rop;	/* ropcntl */
	struct radeon_state_atom cb;	/* colorbuffer (4E28) */
	struct radeon_state_atom rb3d_dither_ctl;	/* (4E50) */
	struct radeon_state_atom rb3d_aaresolve_ctl;	/* (4E88) */
	struct radeon_state_atom rb3d_discard_src_pixel_lte_threshold;	/* (4E88) I saw it only written on RV350 hardware..  */
	struct radeon_state_atom zs;	/* zstencil control (4F00) */
	struct radeon_state_atom zsb;	/* zstencil bf */
	struct radeon_state_atom zstencil_format;
	struct radeon_state_atom zb;	/* z buffer (4F20) */
	struct radeon_state_atom zb_depthclearvalue;	/* (4F28) */
	struct radeon_state_atom zb_zmask;	/* (4F30) */
	struct radeon_state_atom zb_hiz_offset;	/* (4F44) */
	struct radeon_state_atom zb_hiz_pitch;	/* (4F54) */

	struct radeon_state_atom vpi;	/* vp instructions */
	struct radeon_state_atom vpp;	/* vp parameters */
	struct radeon_state_atom vps;	/* vertex point size (?) */
	struct radeon_state_atom vpucp[6];	/* vp user clip plane - 6 */
	/* 8 texture units */
	/* the state is grouped by function and not by
	   texture unit. This makes single unit updates
	   really awkward - we are much better off
	   updating the whole thing at once */
	struct {
		struct radeon_state_atom filter;
		struct radeon_state_atom filter_1;
		struct radeon_state_atom size;
		struct radeon_state_atom format;
		struct radeon_state_atom pitch;
		struct radeon_state_atom offset;
		struct radeon_state_atom chroma_key;
		struct radeon_state_atom border_color;
	} tex;
	struct radeon_state_atom txe;	/* tex enable (4104) */
	radeonTexObj *textures[R300_MAX_TEXTURE_UNITS];
};

/**
 * State cache
 */

/* Vertex shader state */

#define COLOR_IS_RGBA
#define TAG(x) r300##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

struct r300_vertex_program_key {
	GLbitfield FpReads;
	GLuint FogAttr;
	GLuint WPosAttr;
};

struct r300_vertex_program {
	struct gl_vertex_program *Base;
	struct r300_vertex_program *next;

	struct r300_vertex_program_key key;
	struct r300_vertex_program_code code;

	GLboolean error;
};

struct r300_vertex_program_cont {
	/* This is the unmodified vertex program mesa provided us with.
	 * We need to keep it unchanged because we may need to create another
	 * hw specific vertex program based on this.
	 */
	struct gl_vertex_program mesa_program;
	/* This is the list of hw specific vertex programs derived from mesa_program */
	struct r300_vertex_program *progs;
};


/**
* Store everything about a fragment program that is needed
* to render with that program.
*/
struct r300_fragment_program {
	GLboolean error;
	struct r300_fragment_program *next;
	struct r300_fragment_program_external_state state;

	struct rX00_fragment_program_code code;
	GLbitfield InputsRead;

	/* attribute that we are sending the WPOS in */
	gl_frag_attrib wpos_attr;
	/* attribute that we are sending the fog coordinate in */
	gl_frag_attrib fog_attr;
};

struct r300_fragment_program_cont {
	/* This is the unmodified fragment program mesa provided us with.
	 * We need to keep it unchanged because we may need to create another
	 * hw specific fragment program based on this.
	 */
	struct gl_fragment_program Base;
	/* This is the list of hw specific fragment programs derived from Base */
	struct r300_fragment_program *progs;
};


#define R300_MAX_AOS_ARRAYS		16


/* r300_swtcl.c
 */
struct r300_swtcl_info {
  /*
    * Offset of the 4UB color data within a hardware (swtcl) vertex.
    */
   GLuint coloroffset;

   /**
    * Offset of the 3UB specular color data within a hardware (swtcl) vertex.
    */
   GLuint specoffset;
};

struct r300_vtable {
	void (* SetupRSUnit)(GLcontext *ctx);
	void (* SetupFragmentShaderTextures)(GLcontext *ctx, int *tmu_mappings);
	void (* SetupPixelShader)(GLcontext *ctx);
};

struct r300_vertex_buffer {
	struct vertex_attribute {
		/* generic */
		GLubyte element;
		GLuint stride;
		GLuint dwords;
		GLubyte size; /* number of components */
		GLboolean is_named_bo;
		struct radeon_bo *bo;
		GLint bo_offset;

		/* hw specific */
		uint32_t data_type:4;
		uint32_t dst_loc:5;
		uint32_t _signed:1;
		uint32_t normalize:1;
		uint32_t swizzle:12;
		uint32_t write_mask:4;
	} attribs[VERT_ATTRIB_MAX];

	GLubyte num_attribs;
};

struct r300_index_buffer {
	struct radeon_bo *bo;
	int bo_offset;

	GLboolean is_32bit;
	GLuint count;
};


/**
 * \brief R300 context structure.
 */
struct r300_context {
	struct radeon_context radeon;	/* parent class, must be first */

	struct r300_vtable vtbl;

	struct r300_hw_state hw;

	struct r300_vertex_program *selected_vp;
	struct r300_fragment_program *selected_fp;

	/* Vertex buffers
	 */
	GLvector4f dummy_attrib[_TNL_ATTRIB_MAX];
	GLvector4f *temp_attrib[_TNL_ATTRIB_MAX];

	struct r300_options {
		uint32_t conformance_mode:1;
		uint32_t hw_tcl_enabled:1;
		uint32_t s3tc_force_enabled:1;
		uint32_t s3tc_force_disabled:1;
		uint32_t stencil_two_side_disabled:1;
	} options;

	struct r300_swtcl_info swtcl;
	struct r300_vertex_buffer vbuf;
	struct r300_index_buffer ind_buf;

	uint32_t fallback;

	struct {
		struct r300_vertex_program_code vp_code;
		struct rX00_fragment_program_code fp_code;
	} blit;

	DECLARE_RENDERINPUTS(render_inputs_bitset);
};

#define R300_CONTEXT(ctx)		((r300ContextPtr)(ctx->DriverCtx))

extern void r300DestroyContext(__DRIcontext * driContextPriv);
extern GLboolean r300CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontext * driContextPriv,
				   void *sharedContextPrivate);

extern void r300InitShaderFuncs(struct dd_function_table *functions);

extern void r300InitShaderFunctions(r300ContextPtr r300);

extern void r300InitDraw(GLcontext *ctx);

#define r300PackFloat32 radeonPackFloat32
#define r300PackFloat24 radeonPackFloat24

#endif				/* __R300_CONTEXT_H__ */
