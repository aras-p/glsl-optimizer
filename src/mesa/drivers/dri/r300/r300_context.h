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

#include "tnl/t_vertex.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "texmem.h"

#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#define USER_BUFFERS

struct r300_context;
typedef struct r300_context r300ContextRec;
typedef struct r300_context *r300ContextPtr;

#include "radeon_lock.h"
#include "mm.h"

/* From http://gcc.gnu.org/onlinedocs/gcc-3.2.3/gcc/Variadic-Macros.html .
   I suppose we could inline this and use macro to fetch out __LINE__ and stuff in case we run into trouble
   with other compilers ... GLUE!
*/
#define WARN_ONCE(a, ...)	{ \
	static int warn##__LINE__=1; \
	if(warn##__LINE__){ \
		fprintf(stderr, "*********************************WARN_ONCE*********************************\n"); \
		fprintf(stderr, "File %s function %s line %d\n", \
			__FILE__, __FUNCTION__, __LINE__); \
		fprintf(stderr,  a, ## __VA_ARGS__);\
		fprintf(stderr, "***************************************************************************\n"); \
		warn##__LINE__=0;\
		} \
	}

#include "r300_vertprog.h"
#include "r500_fragprog.h"

/**
 * This function takes a float and packs it into a uint32_t
 */
static INLINE uint32_t r300PackFloat32(float fl)
{
	union {
		float fl;
		uint32_t u;
	} u;

	u.fl = fl;
	return u.u;
}

/* This is probably wrong for some values, I need to test this
 * some more.  Range checking would be a good idea also..
 *
 * But it works for most things.  I'll fix it later if someone
 * else with a better clue doesn't
 */
static INLINE uint32_t r300PackFloat24(float f)
{
	float mantissa;
	int exponent;
	uint32_t float24 = 0;

	if (f == 0.0)
		return 0;

	mantissa = frexpf(f, &exponent);

	/* Handle -ve */
	if (mantissa < 0) {
		float24 |= (1 << 23);
		mantissa = mantissa * -1.0;
	}
	/* Handle exponent, bias of 63 */
	exponent += 62;
	float24 |= (exponent << 16);
	/* Kill 7 LSB of mantissa */
	float24 |= (r300PackFloat32(mantissa) & 0x7FFFFF) >> 7;

	return float24;
}

/************ DMA BUFFERS **************/

/* Need refcounting on dma buffers:
 */
struct r300_dma_buffer {
	int refcount;		/**< the number of retained regions in buf */
	drmBufPtr buf;
	int id;
};
#undef GET_START
#ifdef USER_BUFFERS
#define GET_START(rvb) (r300GartOffsetFromVirtual(rmesa, (rvb)->address+(rvb)->start))
#else
#define GET_START(rvb) (rmesa->radeon.radeonScreen->gart_buffer_offset +		\
			(rvb)->address - rmesa->dma.buf0_address +	\
			(rvb)->start)
#endif
/* A retained region, eg vertices for indexed vertices.
 */
struct r300_dma_region {
	struct r300_dma_buffer *buf;
	char *address;		/* == buf->address */
	int start, end, ptr;	/* offsets from start of buf */

	int aos_offset;		/* address in GART memory */
	int aos_stride;		/* distance between elements, in dwords */
	int aos_size;		/* number of components (1-4) */
};

struct r300_dma {
	/* Active dma region.  Allocations for vertices and retained
	 * regions come from here.  Also used for emitting random vertices,
	 * these may be flushed by calling flush_current();
	 */
	struct r300_dma_region current;

	void (*flush) (r300ContextPtr);

	char *buf0_address;	/* start of buf[0], for index calcs */

	/* Number of "in-flight" DMA buffers, i.e. the number of buffers
	 * for which a DISCARD command is currently queued in the command buffer.
	 */
	GLuint nr_released_bufs;
};

       /* Texture related */

typedef struct r300_tex_obj r300TexObj, *r300TexObjPtr;

/* Texture object in locally shared texture space.
 */
struct r300_tex_obj {
	driTextureObject base;

	GLuint bufAddr;		/* Offset to start of locally
				   shared texture block */

	drm_radeon_tex_image_t image[6][RADEON_MAX_TEXTURE_LEVELS];
	/* Six, for the cube faces */

	GLboolean image_override;	/* Image overridden by GLX_EXT_tfp */

	GLuint pitch;		/* this isn't sent to hardware just used in calculations */
	/* hardware register values */
	/* Note that R200 has 8 registers per texture and R300 only 7 */
	GLuint filter;
	GLuint filter_1;
	GLuint pitch_reg;
	GLuint size;		/* npot only */
	GLuint format;
	GLuint offset;		/* Image location in the card's address space.
				   All cube faces follow. */
	GLuint unknown4;
	GLuint unknown5;
	/* end hardware registers */

	/* registers computed by r200 code - keep them here to
	   compare against what is actually written.

	   to be removed later.. */
	GLuint pp_border_color;
	GLuint pp_cubic_faces;	/* cube face 1,2,3,4 log2 sizes */
	GLuint format_x;

	GLboolean border_fallback;

	GLuint tile_bits;	/* hw texture tile bits used on this texture */
};

struct r300_texture_env_state {
	r300TexObjPtr texobj;
	GLenum format;
	GLenum envMode;
};

/* The blit width for texture uploads
 */
#define R300_BLIT_WIDTH_BYTES 1024
#define R300_MAX_TEXTURE_UNITS 8

struct r300_texture_state {
	struct r300_texture_env_state unit[R300_MAX_TEXTURE_UNITS];
	int tc_count;		/* number of incoming texture coordinates from VAP */
};

/**
 * A block of hardware state.
 *
 * When check returns non-zero, the returned number of dwords must be
 * copied verbatim into the command buffer in order to update a state atom
 * when it is dirty.
 */
struct r300_state_atom {
	struct r300_state_atom *next, *prev;
	const char *name;	/* for debug */
	int cmd_size;		/* maximum size in dwords */
	GLuint idx;		/* index in an array (e.g. textures) */
	uint32_t *cmd;
	GLboolean dirty;

	int (*check) (r300ContextPtr, struct r300_state_atom * atom);
};

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
#define R300_GB_MISC_SELECT		4
#define R300_GB_MISC_AA_CONFIG		5
#define R300_GB_MISC_CMDSIZE		6

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

/**
 * Cache for hardware register state.
 */
struct r300_hw_state {
	struct r300_state_atom atomlist;

	GLboolean is_dirty;
	GLboolean all_dirty;
	int max_state_size;	/* in dwords */

	struct r300_state_atom vpt;	/* viewport (1D98) */
	struct r300_state_atom vap_cntl;
        struct r300_state_atom vap_index_offset; /* 0x208c r5xx only */
	struct r300_state_atom vof;	/* VAP output format register 0x2090 */
	struct r300_state_atom vte;	/* (20B0) */
	struct r300_state_atom vap_vf_max_vtx_indx;	/* Maximum Vertex Indx Clamp (2134) */
	struct r300_state_atom vap_cntl_status;
	struct r300_state_atom vir[2];	/* vap input route (2150/21E0) */
	struct r300_state_atom vic;	/* vap input control (2180) */
	struct r300_state_atom vap_psc_sgn_norm_cntl; /* Programmable Stream Control Signed Normalize Control (21DC) */
	struct r300_state_atom vap_clip_cntl;
	struct r300_state_atom vap_clip;
	struct r300_state_atom vap_pvs_vtx_timeout_reg;	/* Vertex timeout register (2288) */
	struct r300_state_atom pvs;	/* pvs_cntl (22D0) */
	struct r300_state_atom gb_enable;	/* (4008) */
	struct r300_state_atom gb_misc;	/* Multisampling position shifts ? (4010) */
	struct r300_state_atom ga_point_s0;	/* S Texture Coordinate of Vertex 0 for Point texture stuffing (LLC) (4200) */
	struct r300_state_atom ga_triangle_stipple;	/* (4214) */
	struct r300_state_atom ps;	/* pointsize (421C) */
	struct r300_state_atom ga_point_minmax;	/* (4230) */
	struct r300_state_atom lcntl;	/* line control */
	struct r300_state_atom ga_line_stipple;	/* (4260) */
	struct r300_state_atom shade;
	struct r300_state_atom polygon_mode;
	struct r300_state_atom fogp;	/* fog parameters (4294) */
	struct r300_state_atom ga_soft_reset;	/* (429C) */
	struct r300_state_atom zbias_cntl;
	struct r300_state_atom zbs;	/* zbias (42A4) */
	struct r300_state_atom occlusion_cntl;
	struct r300_state_atom cul;	/* cull cntl (42B8) */
	struct r300_state_atom su_depth_scale;	/* (42C0) */
	struct r300_state_atom rc;	/* rs control (4300) */
	struct r300_state_atom ri;	/* rs interpolators (4310) */
	struct r300_state_atom rr;	/* rs route (4330) */
	struct r300_state_atom sc_hyperz;	/* (43A4) */
	struct r300_state_atom sc_screendoor;	/* (43E8) */
	struct r300_state_atom fp;	/* fragment program cntl + nodes (4600) */
	struct r300_state_atom fpt;	/* texi - (4620) */
	struct r300_state_atom us_out_fmt;	/* (46A4) */
	struct r300_state_atom r500fp;	/* r500 fp instructions */
	struct r300_state_atom r500fp_const;	/* r500 fp constants */
	struct r300_state_atom fpi[4];	/* fp instructions (46C0/47C0/48C0/49C0) */
	struct r300_state_atom fogs;	/* fog state (4BC0) */
	struct r300_state_atom fogc;	/* fog color (4BC8) */
	struct r300_state_atom at;	/* alpha test (4BD4) */
	struct r300_state_atom fg_depth_src;	/* (4BD8) */
	struct r300_state_atom fpp;	/* 0x4C00 and following */
	struct r300_state_atom rb3d_cctl;	/* (4E00) */
	struct r300_state_atom bld;	/* blending (4E04) */
	struct r300_state_atom cmk;	/* colormask (4E0C) */
	struct r300_state_atom blend_color;	/* constant blend color */
	struct r300_state_atom rop;	/* ropcntl */
	struct r300_state_atom cb;	/* colorbuffer (4E28) */
	struct r300_state_atom rb3d_dither_ctl;	/* (4E50) */
	struct r300_state_atom rb3d_aaresolve_ctl;	/* (4E88) */
	struct r300_state_atom rb3d_discard_src_pixel_lte_threshold;	/* (4E88) I saw it only written on RV350 hardware..  */
	struct r300_state_atom zs;	/* zstencil control (4F00) */
	struct r300_state_atom zstencil_format;
	struct r300_state_atom zb;	/* z buffer (4F20) */
	struct r300_state_atom zb_depthclearvalue;	/* (4F28) */
	struct r300_state_atom unk4F30;	/* (4F30) */
	struct r300_state_atom zb_hiz_offset;	/* (4F44) */
	struct r300_state_atom zb_hiz_pitch;	/* (4F54) */

	struct r300_state_atom vpi;	/* vp instructions */
	struct r300_state_atom vpp;	/* vp parameters */
	struct r300_state_atom vps;	/* vertex point size (?) */
	struct r300_state_atom vpucp[6];	/* vp user clip plane - 6 */
	/* 8 texture units */
	/* the state is grouped by function and not by
	   texture unit. This makes single unit updates
	   really awkward - we are much better off
	   updating the whole thing at once */
	struct {
		struct r300_state_atom filter;
		struct r300_state_atom filter_1;
		struct r300_state_atom size;
		struct r300_state_atom format;
		struct r300_state_atom pitch;
		struct r300_state_atom offset;
		struct r300_state_atom chroma_key;
		struct r300_state_atom border_color;
	} tex;
	struct r300_state_atom txe;	/* tex enable (4104) */
};

/**
 * This structure holds the command buffer while it is being constructed.
 *
 * The first batch of commands in the buffer is always the state that needs
 * to be re-emitted when the context is lost. This batch can be skipped
 * otherwise.
 */
struct r300_cmdbuf {
	int size;		/* DWORDs allocated for buffer */
	uint32_t *cmd_buf;
	int count_used;		/* DWORDs filled so far */
	int count_reemit;	/* size of re-emission batch */
};

/**
 * State cache
 */

struct r300_depthbuffer_state {
	GLfloat scale;
};

struct r300_stencilbuffer_state {
	GLboolean hw_stencil;
};

/* Vertex shader state */

/* Perhaps more if we store programs in vmem? */
/* drm_r300_cmd_header_t->vpu->count is unsigned char */
#define VSF_MAX_FRAGMENT_LENGTH (255*4)

/* Can be tested with colormat currently. */
#define VSF_MAX_FRAGMENT_TEMPS (14)

#define STATE_R300_WINDOW_DIMENSION (STATE_INTERNAL_DRIVER+0)
#define STATE_R300_TEXRECT_FACTOR (STATE_INTERNAL_DRIVER+1)

struct r300_vertex_shader_fragment {
	int length;
	union {
		GLuint d[VSF_MAX_FRAGMENT_LENGTH];
		float f[VSF_MAX_FRAGMENT_LENGTH];
		GLuint i[VSF_MAX_FRAGMENT_LENGTH];
	} body;
};

struct r300_vertex_shader_state {
	struct r300_vertex_shader_fragment program;
};

extern int hw_tcl_on;

#define COLOR_IS_RGBA
#define TAG(x) r300##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

//#define CURRENT_VERTEX_SHADER(ctx) (ctx->VertexProgram._Current)
#define CURRENT_VERTEX_SHADER(ctx) (R300_CONTEXT(ctx)->selected_vp)

/* Should but doesnt work */
//#define CURRENT_VERTEX_SHADER(ctx) (R300_CONTEXT(ctx)->curr_vp)

/* r300_vertex_shader_state and r300_vertex_program should probably be merged together someday.
 * Keeping them them seperate for now should ensure fixed pipeline keeps functioning properly.
 */

struct r300_vertex_program_key {
	GLuint InputsRead;
	GLuint OutputsWritten;
	GLuint OutputsAdded;
};

struct r300_vertex_program {
	struct r300_vertex_program *next;
	struct r300_vertex_program_key key;
	int translated;

	struct r300_vertex_shader_fragment program;

	int pos_end;
	int num_temporaries;	/* Number of temp vars used by program */
	int wpos_idx;
	int inputs[VERT_ATTRIB_MAX];
	int outputs[VERT_RESULT_MAX];
	int native;
	int ref_count;
	int use_ref_count;
};

struct r300_vertex_program_cont {
	struct gl_vertex_program mesa_program;	/* Must be first */
	struct r300_vertex_shader_fragment params;
	struct r300_vertex_program *progs;
};

#define PFS_MAX_ALU_INST	64
#define PFS_MAX_TEX_INST	64
#define PFS_MAX_TEX_INDIRECT 4
#define PFS_NUM_TEMP_REGS	32
#define PFS_NUM_CONST_REGS	16

struct r300_pfs_compile_state;


/**
 * Stores state that influences the compilation of a fragment program.
 */
struct r300_fragment_program_external_state {
	struct {
		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is:
		 *  0 - GL_LUMINANCE
		 *  1 - GL_INTENSITY
		 *  2 - GL_ALPHA
		 * depending on the depth texture mode.
		 */
		GLuint depth_texture_mode : 2;

		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is (texture_compare_func - GL_NEVER).
		 * [e.g. if compare function is GL_LEQUAL, this field is 3]
		 *
		 * Otherwise, this field is 0.
		 */
		GLuint texture_compare_func : 3;
	} unit[16];
};


struct r300_fragment_program_node {
	int tex_offset; /**< first tex instruction */
	int tex_end; /**< last tex instruction, relative to tex_offset */
	int alu_offset; /**< first ALU instruction */
	int alu_end; /**< last ALU instruction, relative to alu_offset */
	int flags;
};

/**
 * Stores an R300 fragment program in its compiled-to-hardware form.
 */
struct r300_fragment_program_code {
	struct {
		int length; /**< total # of texture instructions used */
		GLuint inst[PFS_MAX_TEX_INST];
	} tex;

	struct {
		int length; /**< total # of ALU instructions used */
		struct {
			GLuint inst0;
			GLuint inst1;
			GLuint inst2;
			GLuint inst3;
		} inst[PFS_MAX_ALU_INST];
	} alu;

	struct r300_fragment_program_node node[4];
	int cur_node;
	int first_node_has_tex;

	/**
	 * Remember which program register a given hardware constant
	 * belongs to.
	 */
	struct prog_src_register constant[PFS_NUM_CONST_REGS];
	int const_nr;

	int max_temp_idx;
};

/**
 * Store everything about a fragment program that is needed
 * to render with that program.
 */
struct r300_fragment_program {
	struct gl_fragment_program mesa_program;

	GLboolean translated;
	GLboolean error;

	struct r300_fragment_program_external_state state;
	struct r300_fragment_program_code code;

	GLboolean WritesDepth;
	GLuint optimization;
};

struct r500_pfs_compile_state;

struct r500_fragment_program_external_state {
	struct {
		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is:
		 *  0 - GL_LUMINANCE
		 *  1 - GL_INTENSITY
		 *  2 - GL_ALPHA
		 * depending on the depth texture mode.
		 */
		GLuint depth_texture_mode : 2;

		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is (texture_compare_func - GL_NEVER).
		 * [e.g. if compare function is GL_LEQUAL, this field is 3]
		 *
		 * Otherwise, this field is 0.
		 */
		GLuint texture_compare_func : 3;
	} unit[16];
};

struct r500_fragment_program_code {
	struct {
		GLuint inst0;
		GLuint inst1;
		GLuint inst2;
		GLuint inst3;
		GLuint inst4;
		GLuint inst5;
	} inst[512];

	int inst_offset;
	int inst_end;

	/**
	 * Remember which program register a given hardware constant
	 * belongs to.
	 */
	struct prog_src_register constant[PFS_NUM_CONST_REGS];
	int const_nr;

	int max_temp_idx;
};

struct r500_fragment_program {
	struct gl_fragment_program mesa_program;

	GLcontext *ctx;
	GLboolean translated;
	GLboolean error;

	struct r500_fragment_program_external_state state;
	struct r500_fragment_program_code code;

	GLboolean writes_depth;

	GLuint optimization;
};

#define R300_MAX_AOS_ARRAYS		16

#define REG_COORDS	0
#define REG_COLOR0	1
#define REG_TEX0	2

struct r300_state {
	struct r300_depthbuffer_state depth;
	struct r300_texture_state texture;
	int sw_tcl_inputs[VERT_ATTRIB_MAX];
	struct r300_vertex_shader_state vertex_shader;
	struct r300_dma_region aos[R300_MAX_AOS_ARRAYS];
	int aos_count;

	GLuint *Elts;
	struct r300_dma_region elt_dma;

	struct r300_dma_region swtcl_dma;
	DECLARE_RENDERINPUTS(render_inputs_bitset);	/* actual render inputs that R300 was configured for.
							   They are the same as tnl->render_inputs for fixed pipeline */

	struct r300_stencilbuffer_state stencil;

};

#define R300_FALLBACK_NONE 0
#define R300_FALLBACK_TCL 1
#define R300_FALLBACK_RAST 2

/* r300_swtcl.c
 */
struct r300_swtcl_info {
   GLuint RenderIndex;

   /**
    * Size of a hardware vertex.  This is calculated when \c ::vertex_attrs is
    * installed in the Mesa state vector.
    */
   GLuint vertex_size;

   /**
    * Attributes instructing the Mesa TCL pipeline where / how to put vertex
    * data in the hardware buffer.
    */
   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];

   /**
    * Number of elements of \c ::vertex_attrs that are actually used.
    */
   GLuint vertex_attr_count;

   /**
    * Cached pointer to the buffer where Mesa will store vertex data.
    */
   GLubyte *verts;

   /* Fallback rasterization functions
    */
  //   r200_point_func draw_point;
  //   r200_line_func draw_line;
  //   r200_tri_func draw_tri;

   GLuint hw_primitive;
   GLenum render_primitive;
   GLuint numverts;

   /**
    * Offset of the 4UB color data within a hardware (swtcl) vertex.
    */
   GLuint coloroffset;

   /**
    * Offset of the 3UB specular color data within a hardware (swtcl) vertex.
    */
   GLuint specoffset;

   /**
    * Should Mesa project vertex data or will the hardware do it?
    */
   GLboolean needproj;

   struct r300_dma_region indexed_verts;
};


/**
 * \brief R300 context structure.
 */
struct r300_context {
	struct radeon_context radeon;	/* parent class, must be first */

	struct r300_hw_state hw;
	struct r300_cmdbuf cmdbuf;
	struct r300_state state;
	struct gl_vertex_program *curr_vp;
	struct r300_vertex_program *selected_vp;

	/* Vertex buffers
	 */
	struct r300_dma dma;
	GLboolean save_on_next_unlock;
	GLuint NewGLState;

	/* Texture object bookkeeping
	 */
	unsigned nr_heaps;
	driTexHeap *texture_heaps[RADEON_NR_TEX_HEAPS];
	driTextureObject swapped;
	int texture_depth;
	float initialMaxAnisotropy;

	/* Clientdata textures;
	 */
	GLuint prefer_gart_client_texturing;

#ifdef USER_BUFFERS
	struct r300_memory_manager *rmm;
#endif

	GLvector4f dummy_attrib[_TNL_ATTRIB_MAX];
	GLvector4f *temp_attrib[_TNL_ATTRIB_MAX];

	GLboolean disable_lowimpact_fallback;

	DECLARE_RENDERINPUTS(tnl_index_bitset);	/* index of bits for last tnl_install_attrs */
	struct r300_swtcl_info swtcl;
};

struct r300_buffer_object {
	struct gl_buffer_object mesa_obj;
	int id;
};

#define R300_CONTEXT(ctx)		((r300ContextPtr)(ctx->DriverCtx))

extern void r300DestroyContext(__DRIcontextPrivate * driContextPriv);
extern GLboolean r300CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);

extern void r300SelectVertexShader(r300ContextPtr r300);
extern void r300InitShaderFuncs(struct dd_function_table *functions);
extern int r300VertexProgUpdateParams(GLcontext * ctx,
				      struct r300_vertex_program_cont *vp,
				      float *dst);

#define RADEON_D_CAPTURE 0
#define RADEON_D_PLAYBACK 1
#define RADEON_D_PLAYBACK_RAW 2
#define RADEON_D_T 3

#endif				/* __R300_CONTEXT_H__ */
