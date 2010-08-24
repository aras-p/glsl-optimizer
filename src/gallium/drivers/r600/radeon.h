/*
 * Copyright © 2009 Jerome Glisse <glisse@freedesktop.org>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef RADEON_H
#define RADEON_H

#define RADEON_CTX_MAX_PM4	(64 * 1024 / 4)

#include <stdint.h>

typedef uint64_t		u64;
typedef uint32_t		u32;
typedef uint16_t		u16;
typedef uint8_t			u8;

struct radeon;

enum radeon_family {
	CHIP_UNKNOWN,
	CHIP_R100,
	CHIP_RV100,
	CHIP_RS100,
	CHIP_RV200,
	CHIP_RS200,
	CHIP_R200,
	CHIP_RV250,
	CHIP_RS300,
	CHIP_RV280,
	CHIP_R300,
	CHIP_R350,
	CHIP_RV350,
	CHIP_RV380,
	CHIP_R420,
	CHIP_R423,
	CHIP_RV410,
	CHIP_RS400,
	CHIP_RS480,
	CHIP_RS600,
	CHIP_RS690,
	CHIP_RS740,
	CHIP_RV515,
	CHIP_R520,
	CHIP_RV530,
	CHIP_RV560,
	CHIP_RV570,
	CHIP_R580,
	CHIP_R600,
	CHIP_RV610,
	CHIP_RV630,
	CHIP_RV670,
	CHIP_RV620,
	CHIP_RV635,
	CHIP_RS780,
	CHIP_RS880,
	CHIP_RV770,
	CHIP_RV730,
	CHIP_RV710,
	CHIP_RV740,
	CHIP_CEDAR,
	CHIP_REDWOOD,
	CHIP_JUNIPER,
	CHIP_CYPRESS,
	CHIP_HEMLOCK,
	CHIP_LAST,
};

enum radeon_family radeon_get_family(struct radeon *rw);

/*
 * radeon object functions
 */
struct radeon_bo {
	unsigned			refcount;
	unsigned			handle;
	unsigned			size;
	unsigned			alignment;
	unsigned			map_count;
	void				*data;
};
struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			unsigned size, unsigned alignment, void *ptr);
int radeon_bo_map(struct radeon *radeon, struct radeon_bo *bo);
void radeon_bo_unmap(struct radeon *radeon, struct radeon_bo *bo);
struct radeon_bo *radeon_bo_incref(struct radeon *radeon, struct radeon_bo *bo);
struct radeon_bo *radeon_bo_decref(struct radeon *radeon, struct radeon_bo *bo);
int radeon_bo_wait(struct radeon *radeon, struct radeon_bo *bo);

/*
 * states functions
 */
struct radeon_state {
	struct radeon			*radeon;
	unsigned			refcount;
	unsigned			id;
	unsigned			cpm4;
	u32				states[128];
	u32				pm4_crc;
	unsigned			nbo;
	struct radeon_bo		*bo[4];
	unsigned			reloc_pm4_id[4];
	u32				placement[8];
	unsigned			bo_dirty[4];
};

struct radeon_state *radeon_state(struct radeon *radeon, u32 id);
struct radeon_state *radeon_state_incref(struct radeon_state *state);
struct radeon_state *radeon_state_decref(struct radeon_state *state);
int radeon_state_pm4(struct radeon_state *state);

/*
 * draw functions
 */
struct radeon_draw {
	unsigned			refcount;
	struct radeon			*radeon;
	unsigned			nstate;
	struct radeon_state		**state;
	unsigned			cpm4;
};

struct radeon_draw *radeon_draw(struct radeon *radeon);
struct radeon_draw *radeon_draw_duplicate(struct radeon_draw *draw);
struct radeon_draw *radeon_draw_incref(struct radeon_draw *draw);
struct radeon_draw *radeon_draw_decref(struct radeon_draw *draw);
int radeon_draw_set(struct radeon_draw *draw, struct radeon_state *state);
int radeon_draw_set_new(struct radeon_draw *draw, struct radeon_state *state);
int radeon_draw_check(struct radeon_draw *draw);

/*
 * radeon context functions
 */
#pragma pack(1)
struct radeon_cs_reloc {
	uint32_t	handle;
	uint32_t	read_domain;
	uint32_t	write_domain;
	uint32_t	flags;
};
#pragma pack()

struct radeon_ctx_bo {
	struct radeon_bo		*bo;
	u32				bo_flushed;
	unsigned			state_id;
};

struct radeon_ctx {
	int				refcount;
	struct radeon			*radeon;
	u32				*pm4;
	int				npm4;
	unsigned			id;
	unsigned			nreloc;
	unsigned			max_reloc;
	struct radeon_cs_reloc		*reloc;
	unsigned			nbo;
	struct radeon_ctx_bo		*bo;
	unsigned			max_bo;
	u32				*state_crc32;
};

struct radeon_ctx *radeon_ctx(struct radeon *radeon);
struct radeon_ctx *radeon_ctx_decref(struct radeon_ctx *ctx);
struct radeon_ctx *radeon_ctx_incref(struct radeon_ctx *ctx);
void radeon_ctx_clear(struct radeon_ctx *ctx);
int radeon_ctx_set_draw(struct radeon_ctx *ctx, struct radeon_draw *draw);
int radeon_ctx_set_query_state(struct radeon_ctx *ctx, struct radeon_state *state);
int radeon_ctx_pm4(struct radeon_ctx *ctx);
int radeon_ctx_submit(struct radeon_ctx *ctx);
void radeon_ctx_dump_bof(struct radeon_ctx *ctx, const char *file);


/*
 * R600/R700
 */
#define R600_CONFIG		0
#define R600_CB_CNTL		1
#define R600_RASTERIZER		2
#define R600_VIEWPORT		3
#define R600_SCISSOR		4
#define R600_BLEND		5
#define R600_DSA		6
#define R600_VGT		7
#define R600_QUERY_BEGIN	8
#define R600_QUERY_END		9
#define R600_VS_SHADER		10
#define R600_PS_SHADER		11
#define R600_DB			12
#define R600_CB0		13
#define R600_UCP0		21
#define R600_PS_RESOURCE0	27
#define R600_VS_RESOURCE0	187
#define R600_FS_RESOURCE0	347
#define R600_GS_RESOURCE0	363
#define R600_PS_CONSTANT0	523
#define R600_VS_CONSTANT0	779
#define R600_PS_SAMPLER0	1035
#define R600_VS_SAMPLER0	1053
#define R600_GS_SAMPLER0	1071
#define R600_PS_SAMPLER_BORDER0	1089
#define R600_VS_SAMPLER_BORDER0	1107
#define R600_GS_SAMPLER_BORDER0	1125
#define R600_DRAW_AUTO		1143
#define R600_DRAW		1144
#define R600_NSTATE		1145

/* R600_CONFIG */
#define R600_CONFIG__SQ_CONFIG					0
#define R600_CONFIG__SQ_GPR_RESOURCE_MGMT_1			1
#define R600_CONFIG__SQ_GPR_RESOURCE_MGMT_2			2
#define R600_CONFIG__SQ_THREAD_RESOURCE_MGMT			3
#define R600_CONFIG__SQ_STACK_RESOURCE_MGMT_1			4
#define R600_CONFIG__SQ_STACK_RESOURCE_MGMT_2			5
#define R600_CONFIG__SQ_DYN_GPR_CNTL_PS_FLUSH_REQ		8
#define R600_CONFIG__TA_CNTL_AUX				11
#define R600_CONFIG__VC_ENHANCE					14
#define R600_CONFIG__DB_DEBUG					17
#define R600_CONFIG__DB_WATERMARKS				20
#define R600_CONFIG__SX_MISC					23
#define R600_CONFIG__SPI_THREAD_GROUPING			26
#define R600_CONFIG__CB_SHADER_CONTROL				29
#define R600_CONFIG__SQ_ESGS_RING_ITEMSIZE			32
#define R600_CONFIG__SQ_GSVS_RING_ITEMSIZE			33
#define R600_CONFIG__SQ_ESTMP_RING_ITEMSIZE			34
#define R600_CONFIG__SQ_GSTMP_RING_ITEMSIZE			35
#define R600_CONFIG__SQ_VSTMP_RING_ITEMSIZE			36
#define R600_CONFIG__SQ_PSTMP_RING_ITEMSIZE			37
#define R600_CONFIG__SQ_FBUF_RING_ITEMSIZE			38
#define R600_CONFIG__SQ_REDUC_RING_ITEMSIZE			39
#define R600_CONFIG__SQ_GS_VERT_ITEMSIZE			40
#define R600_CONFIG__VGT_OUTPUT_PATH_CNTL			43
#define R600_CONFIG__VGT_HOS_CNTL				44
#define R600_CONFIG__VGT_HOS_MAX_TESS_LEVEL			45
#define R600_CONFIG__VGT_HOS_MIN_TESS_LEVEL			46
#define R600_CONFIG__VGT_HOS_REUSE_DEPTH			47
#define R600_CONFIG__VGT_GROUP_PRIM_TYPE			48
#define R600_CONFIG__VGT_GROUP_FIRST_DECR			49
#define R600_CONFIG__VGT_GROUP_DECR				50
#define R600_CONFIG__VGT_GROUP_VECT_0_CNTL			51
#define R600_CONFIG__VGT_GROUP_VECT_1_CNTL			52
#define R600_CONFIG__VGT_GROUP_VECT_0_FMT_CNTL			53
#define R600_CONFIG__VGT_GROUP_VECT_1_FMT_CNTL			54
#define R600_CONFIG__VGT_GS_MODE				55
#define R600_CONFIG__PA_SC_MODE_CNTL				58
#define R600_CONFIG__VGT_STRMOUT_EN				61
#define R600_CONFIG__VGT_REUSE_OFF				62
#define R600_CONFIG__VGT_VTX_CNT_EN				63
#define R600_CONFIG__VGT_STRMOUT_BUFFER_EN			66
/* R600_CB_CNTL */
#define R600_CB_CNTL__CB_CLEAR_RED				0
#define R600_CB_CNTL__CB_CLEAR_GREEN				1
#define R600_CB_CNTL__CB_CLEAR_BLUE				2
#define R600_CB_CNTL__CB_CLEAR_ALPHA				3
#define R600_CB_CNTL__CB_SHADER_MASK				6
#define R600_CB_CNTL__CB_TARGET_MASK				7
#define R600_CB_CNTL__CB_FOG_RED				10
#define R600_CB_CNTL__CB_FOG_GREEN				11
#define R600_CB_CNTL__CB_FOG_BLUE				12
#define R600_CB_CNTL__CB_COLOR_CONTROL				15
#define R600_CB_CNTL__PA_SC_AA_CONFIG				18
#define R600_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_MCTX			21
#define R600_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX		22
#define R600_CB_CNTL__CB_CLRCMP_CONTROL				25
#define R600_CB_CNTL__CB_CLRCMP_SRC				26
#define R600_CB_CNTL__CB_CLRCMP_DST				27
#define R600_CB_CNTL__CB_CLRCMP_MSK				28
#define R600_CB_CNTL__PA_SC_AA_MASK				31
/* R600_RASTERIZER */
#define R600_RASTERIZER__SPI_INTERP_CONTROL_0			0
#define R600_RASTERIZER__PA_CL_CLIP_CNTL			3
#define R600_RASTERIZER__PA_SU_SC_MODE_CNTL			4
#define R600_RASTERIZER__PA_CL_VS_OUT_CNTL			7
#define R600_RASTERIZER__PA_CL_NANINF_CNTL			8
#define R600_RASTERIZER__PA_SU_POINT_SIZE			11
#define R600_RASTERIZER__PA_SU_POINT_MINMAX			12
#define R600_RASTERIZER__PA_SU_LINE_CNTL			13
#define R600_RASTERIZER__PA_SC_LINE_STIPPLE			14
#define R600_RASTERIZER__PA_SC_MPASS_PS_CNTL			17
#define R600_RASTERIZER__PA_SC_LINE_CNTL			20
#define R600_RASTERIZER__PA_CL_GB_VERT_CLIP_ADJ			23
#define R600_RASTERIZER__PA_CL_GB_VERT_DISC_ADJ			24
#define R600_RASTERIZER__PA_CL_GB_HORZ_CLIP_ADJ			25
#define R600_RASTERIZER__PA_CL_GB_HORZ_DISC_ADJ			26
#define R600_RASTERIZER__PA_SU_POLY_OFFSET_DB_FMT_CNTL		29
#define R600_RASTERIZER__PA_SU_POLY_OFFSET_CLAMP		30
#define R600_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_SCALE		31
#define R600_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_OFFSET		32
#define R600_RASTERIZER__PA_SU_POLY_OFFSET_BACK_SCALE		33
#define R600_RASTERIZER__PA_SU_POLY_OFFSET_BACK_OFFSET		34
/* R600_VIEWPORT */
#define R600_VIEWPORT__PA_SC_VPORT_ZMIN_0			0
#define R600_VIEWPORT__PA_SC_VPORT_ZMAX_0			1
#define R600_VIEWPORT__PA_CL_VPORT_XSCALE_0			4
#define R600_VIEWPORT__PA_CL_VPORT_YSCALE_0			7
#define R600_VIEWPORT__PA_CL_VPORT_ZSCALE_0			10
#define R600_VIEWPORT__PA_CL_VPORT_XOFFSET_0			13
#define R600_VIEWPORT__PA_CL_VPORT_YOFFSET_0			16
#define R600_VIEWPORT__PA_CL_VPORT_ZOFFSET_0			19
#define R600_VIEWPORT__PA_CL_VTE_CNTL				22
/* R600_SCISSOR */
#define R600_SCISSOR__PA_SC_SCREEN_SCISSOR_TL			0
#define R600_SCISSOR__PA_SC_SCREEN_SCISSOR_BR			1
#define R600_SCISSOR__PA_SC_WINDOW_OFFSET			4
#define R600_SCISSOR__PA_SC_WINDOW_SCISSOR_TL			5
#define R600_SCISSOR__PA_SC_WINDOW_SCISSOR_BR			6
#define R600_SCISSOR__PA_SC_CLIPRECT_RULE			7
#define R600_SCISSOR__PA_SC_CLIPRECT_0_TL			8
#define R600_SCISSOR__PA_SC_CLIPRECT_0_BR			9
#define R600_SCISSOR__PA_SC_CLIPRECT_1_TL			10
#define R600_SCISSOR__PA_SC_CLIPRECT_1_BR			11
#define R600_SCISSOR__PA_SC_CLIPRECT_2_TL			12
#define R600_SCISSOR__PA_SC_CLIPRECT_2_BR			13
#define R600_SCISSOR__PA_SC_CLIPRECT_3_TL			14
#define R600_SCISSOR__PA_SC_CLIPRECT_3_BR			15
#define R600_SCISSOR__PA_SC_EDGERULE				16
#define R600_SCISSOR__PA_SC_GENERIC_SCISSOR_TL			19
#define R600_SCISSOR__PA_SC_GENERIC_SCISSOR_BR			20
#define R600_SCISSOR__PA_SC_VPORT_SCISSOR_0_TL			23
#define R600_SCISSOR__PA_SC_VPORT_SCISSOR_0_BR			24
/* R600_BLEND */
#define R600_BLEND__CB_BLEND_RED				0
#define R600_BLEND__CB_BLEND_GREEN				1
#define R600_BLEND__CB_BLEND_BLUE				2
#define R600_BLEND__CB_BLEND_ALPHA				3
#define R600_BLEND__CB_BLEND0_CONTROL				6
#define R600_BLEND__CB_BLEND1_CONTROL				7
#define R600_BLEND__CB_BLEND2_CONTROL				8
#define R600_BLEND__CB_BLEND3_CONTROL				9
#define R600_BLEND__CB_BLEND4_CONTROL				10
#define R600_BLEND__CB_BLEND5_CONTROL				11
#define R600_BLEND__CB_BLEND6_CONTROL				12
#define R600_BLEND__CB_BLEND7_CONTROL				13
#define R600_BLEND__CB_BLEND_CONTROL				16
/* R600_DSA */
#define R600_DSA__DB_STENCIL_CLEAR				0
#define R600_DSA__DB_DEPTH_CLEAR				1
#define R600_DSA__SX_ALPHA_TEST_CONTROL				4
#define R600_DSA__DB_STENCILREFMASK				7
#define R600_DSA__DB_STENCILREFMASK_BF				8
#define R600_DSA__SX_ALPHA_REF					9
#define R600_DSA__SPI_FOG_FUNC_SCALE				12
#define R600_DSA__SPI_FOG_FUNC_BIAS				13
#define R600_DSA__SPI_FOG_CNTL					16
#define R600_DSA__DB_DEPTH_CONTROL				19
#define R600_DSA__DB_SHADER_CONTROL				22
#define R600_DSA__DB_RENDER_CONTROL				25
#define R600_DSA__DB_RENDER_OVERRIDE				26
#define R600_DSA__DB_SRESULTS_COMPARE_STATE1			29
#define R600_DSA__DB_PRELOAD_CONTROL				30
#define R600_DSA__DB_ALPHA_TO_MASK				33
/* R600_VS_SHADER */
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_0			0
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_1			1
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_2			2
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_3			3
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_4			4
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_5			5
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_6			6
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_7			7
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_8			8
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_9			9
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_10			10
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_11			11
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_12			12
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_13			13
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_14			14
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_15			15
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_16			16
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_17			17
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_18			18
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_19			19
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_20			20
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_21			21
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_22			22
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_23			23
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_24			24
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_25			25
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_26			26
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_27			27
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_28			28
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_29			29
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_30			30
#define R600_VS_SHADER__SQ_VTX_SEMANTIC_31			31
#define R600_VS_SHADER__SPI_VS_OUT_ID_0				34
#define R600_VS_SHADER__SPI_VS_OUT_ID_1				35
#define R600_VS_SHADER__SPI_VS_OUT_ID_2				36
#define R600_VS_SHADER__SPI_VS_OUT_ID_3				37
#define R600_VS_SHADER__SPI_VS_OUT_ID_4				38
#define R600_VS_SHADER__SPI_VS_OUT_ID_5				39
#define R600_VS_SHADER__SPI_VS_OUT_ID_6				40
#define R600_VS_SHADER__SPI_VS_OUT_ID_7				41
#define R600_VS_SHADER__SPI_VS_OUT_ID_8				42
#define R600_VS_SHADER__SPI_VS_OUT_ID_9				43
#define R600_VS_SHADER__SPI_VS_OUT_CONFIG			46
#define R600_VS_SHADER__SQ_PGM_START_VS				49
#define R600_VS_SHADER__SQ_PGM_START_VS_BO_ID			51
#define R600_VS_SHADER__SQ_PGM_RESOURCES_VS			54
#define R600_VS_SHADER__SQ_PGM_START_FS				57
#define R600_VS_SHADER__SQ_PGM_START_FS_BO_ID			59
#define R600_VS_SHADER__SQ_PGM_RESOURCES_FS			62
#define R600_VS_SHADER__SQ_PGM_CF_OFFSET_VS			65
#define R600_VS_SHADER__SQ_PGM_CF_OFFSET_FS			68
/* R600_PS_SHADER */
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_0			0
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_1			1
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_2			2
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_3			3
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_4			4
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_5			5
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_6			6
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_7			7
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_8			8
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_9			9
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_10			10
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_11			11
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_12			12
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_13			13
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_14			14
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_15			15
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_16			16
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_17			17
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_18			18
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_19			19
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_20			20
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_21			21
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_22			22
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_23			23
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_24			24
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_25			25
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_26			26
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_27			27
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_28			28
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_29			29
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_30			30
#define R600_PS_SHADER__SPI_PS_INPUT_CNTL_31			31
#define R600_PS_SHADER__SPI_PS_IN_CONTROL_0			34
#define R600_PS_SHADER__SPI_PS_IN_CONTROL_1			35
#define R600_PS_SHADER__SPI_INPUT_Z				38
#define R600_PS_SHADER__SQ_PGM_START_PS				41
#define R600_PS_SHADER__SQ_PGM_START_PS_BO_ID			43
#define R600_PS_SHADER__SQ_PGM_RESOURCES_PS			46
#define R600_PS_SHADER__SQ_PGM_EXPORTS_PS			47
#define R600_PS_SHADER__SQ_PGM_CF_OFFSET_PS			50
/* R600_PS_CONSTANT */
#define R600_PS_CONSTANT__SQ_ALU_CONSTANT0_0			0
#define R600_PS_CONSTANT__SQ_ALU_CONSTANT1_0			1
#define R600_PS_CONSTANT__SQ_ALU_CONSTANT2_0			2
#define R600_PS_CONSTANT__SQ_ALU_CONSTANT3_0			3
/* R600_VS_CONSTANT */
#define R600_VS_CONSTANT__SQ_ALU_CONSTANT0_256			0
#define R600_VS_CONSTANT__SQ_ALU_CONSTANT1_256			1
#define R600_VS_CONSTANT__SQ_ALU_CONSTANT2_256			2
#define R600_VS_CONSTANT__SQ_ALU_CONSTANT3_256			3
/* R600_PS_RESOURCE */
#define R600_RESOURCE__RESOURCE_WORD0				0
#define R600_RESOURCE__RESOURCE_WORD1				1
#define R600_RESOURCE__RESOURCE_WORD2				2
#define R600_RESOURCE__RESOURCE_WORD3				3
#define R600_RESOURCE__RESOURCE_WORD4				4
#define R600_RESOURCE__RESOURCE_WORD5				5
#define R600_RESOURCE__RESOURCE_WORD6				6
#define R600_RESOURCE__RESOURCE_BO0_ID				8
#define R600_RESOURCE__RESOURCE_BO1_ID				10
/* R600_PS_SAMPLER */
#define R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD0_0			0
#define R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD1_0			1
#define R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD2_0			2
/* R600_VS_SAMPLER */
#define R600_VS_SAMPLER__SQ_TEX_SAMPLER_WORD0_18		0
#define R600_VS_SAMPLER__SQ_TEX_SAMPLER_WORD1_18		1
#define R600_VS_SAMPLER__SQ_TEX_SAMPLER_WORD2_18		2
/* R600_GS_SAMPLER */
#define R600_GS_SAMPLER__SQ_TEX_SAMPLER_WORD0_36		0
#define R600_GS_SAMPLER__SQ_TEX_SAMPLER_WORD1_36		1
#define R600_GS_SAMPLER__SQ_TEX_SAMPLER_WORD2_36		2
/* R600_PS_SAMPLER_BORDER */
#define R600_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_RED	0
#define R600_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_GREEN	1
#define R600_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_BLUE	2
#define R600_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_ALPHA	3
/* R600_VS_SAMPLER_BORDER */
#define R600_VS_SAMPLER_BORDER__TD_VS_SAMPLER0_BORDER_RED	0
#define R600_VS_SAMPLER_BORDER__TD_VS_SAMPLER0_BORDER_GREEN	1
#define R600_VS_SAMPLER_BORDER__TD_VS_SAMPLER0_BORDER_BLUE	2
#define R600_VS_SAMPLER_BORDER__TD_VS_SAMPLER0_BORDER_ALPHA	3
/* R600_GS_SAMPLER_BORDER */
#define R600_GS_SAMPLER_BORDER__TD_GS_SAMPLER0_BORDER_RED	0
#define R600_GS_SAMPLER_BORDER__TD_GS_SAMPLER0_BORDER_GREEN	1
#define R600_GS_SAMPLER_BORDER__TD_GS_SAMPLER0_BORDER_BLUE	2
#define R600_GS_SAMPLER_BORDER__TD_GS_SAMPLER0_BORDER_ALPHA	3
/* R600_CB */
#define R600_CB__CB_COLOR0_BASE					0
#define R600_CB__CB_COLOR0_BASE_BO_ID				2
#define R600_CB__CB_COLOR0_INFO					5
#define R600_CB__CB_COLOR0_SIZE					8
#define R600_CB__CB_COLOR0_VIEW					11
#define R600_CB__CB_COLOR0_FRAG					14
#define R600_CB__CB_COLOR0_FRAG_BO_ID				16
#define R600_CB__CB_COLOR0_TILE					19
#define R600_CB__CB_COLOR0_TILE_BO_ID				21
#define R600_CB__CB_COLOR0_MASK					24
/* R600_DB */
#define R600_DB__DB_DEPTH_BASE					0
#define R600_DB__DB_DEPTH_BASE_BO_ID				2
#define R600_DB__DB_DEPTH_SIZE					5
#define R600_DB__DB_DEPTH_VIEW					6
#define R600_DB__DB_DEPTH_INFO					9
#define R600_DB__DB_HTILE_SURFACE				12
#define R600_DB__DB_PREFETCH_LIMIT				15
/* R600_VGT */
#define R600_VGT__VGT_MAX_VTX_INDX				0
#define R600_VGT__VGT_MIN_VTX_INDX				1
#define R600_VGT__VGT_INDX_OFFSET				2
#define R600_VGT__VGT_MULTI_PRIM_IB_RESET_INDX			3
#define R600_VGT__VGT_PRIMITIVE_TYPE				6
#define R600_VGT__VGT_DMA_INDEX_TYPE				8
#define R600_VGT__VGT_DMA_NUM_INSTANCES				10
/* R600_DRAW_AUTO */
#define R600_DRAW_AUTO__VGT_NUM_INDICES				0
#define R600_DRAW_AUTO__VGT_DRAW_INITIATOR			1
/* R600_DRAW */
#define R600_DRAW__VGT_DMA_BASE					0
#define R600_DRAW__VGT_DMA_BASE_HI				1
#define R600_DRAW__VGT_NUM_INDICES				2
#define R600_DRAW__VGT_DRAW_INITIATOR				3
#define R600_DRAW__INDICES_BO_ID				5
/* R600_UCP */
#define R600_UCP__PA_CL_UCP_X_0					0
#define R600_UCP__PA_CL_UCP_Y_0					1
#define R600_UCP__PA_CL_UCP_Z_0					2
#define R600_UCP__PA_CL_UCP_W_0					3
/* R600 QUERY BEGIN/END */
#define R600_QUERY__OFFSET					0
#define R600_QUERY__BO_ID					3

#endif
