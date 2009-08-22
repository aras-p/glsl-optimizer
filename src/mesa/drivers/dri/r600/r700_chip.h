/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#ifndef _R700_CHIP_H_
#define _R700_CHIP_H_

#include "r600_context.h"

#include "r600_reg.h"
#include "r600_reg_auto_r6xx.h"
#include "r600_reg_r6xx.h"
#include "r600_reg_r7xx.h"

#include "r700_chipoffset.h"

#define SETfield(x, val, shift, mask)  ( (x) = ((x) & ~(mask)) | ((val) << (shift)) ) /* u32All */
#define CLEARfield(x, mask)            ( (x) &= ~(mask) )
#define SETbit(x, bit)                 ( (x) |= (bit) )
#define CLEARbit(x, bit)               ( (x) &= ~(bit) )

#define R700_TEXTURE_NUMBERUNITS 16
#define R700_MAX_RENDER_TARGETS  8
#define R700_MAX_VIEWPORTS       16
#define R700_MAX_SHADER_EXPORTS  32
#define R700_MAX_UCP             6
#define R700_MAX_DX9_CONSTS      256

/* Enum not show in r600_*.h */

#define FETCH_RESOURCE_STRIDE 7

#define ASIC_CONFIG_BASE_INDEX    0x2000
#define ASIC_CONTEXT_BASE_INDEX   0xA000
#define ASIC_CTL_CONST_BASE_INDEX 0xF3FC


enum 
{
    SQ_ABSOLUTE                              = 0x00000000,
    SQ_RELATIVE                              = 0x00000001,
};

enum 
{
    SQ_ALU_SCL_210                           = 0x00000000,
    SQ_ALU_SCL_122                           = 0x00000001,
    SQ_ALU_SCL_212                           = 0x00000002,
    SQ_ALU_SCL_221                           = 0x00000003,
};

enum 
{
    SQ_TEX_UNNORMALIZED                      = 0x00000000,
    SQ_TEX_NORMALIZED                        = 0x00000001,
};

enum 
{
    SQ_CF_PIXEL_MRT0                         = 0x00000000,
    SQ_CF_PIXEL_MRT1                         = 0x00000001,
    SQ_CF_PIXEL_MRT2                         = 0x00000002,
    SQ_CF_PIXEL_MRT3                         = 0x00000003,
    SQ_CF_PIXEL_MRT4                         = 0x00000004,
    SQ_CF_PIXEL_MRT5                         = 0x00000005,
    SQ_CF_PIXEL_MRT6                         = 0x00000006,
    SQ_CF_PIXEL_MRT7                         = 0x00000007,
    SQ_CF_PIXEL_Z                            = 0x0000003d,
};

typedef enum ENUM_SQ_CF_ARRAY_BASE_POS {
SQ_CF_POS_0                              = 0x0000003c,
SQ_CF_POS_1                              = 0x0000003d,
SQ_CF_POS_2                              = 0x0000003e,
SQ_CF_POS_3                              = 0x0000003f,
} ENUM_SQ_CF_ARRAY_BASE_POS;

enum
{
    PGM_RESOURCES__PRIME_CACHE_ON_DRAW_bit = 23,
};

enum 
{
    TEX_XYFilter_Point                       = 0x00000000,
    TEX_XYFilter_Linear                      = 0x00000001,
    TEX_XYFilter_Cubic                       = 0x00000002,
    TEX_XYFilter_Cleartype                   = 0x00000003,

    TEX_MipFilter_None                       = 0x00000000,
    TEX_MipFilter_Point                      = 0x00000001,
    TEX_MipFilter_Linear                     = 0x00000002,
};

enum 
{
    SQ_EXPORT_WRITE                          = 0x00000000,
    SQ_EXPORT_WRITE_IND                      = 0x00000001,
    SQ_EXPORT_WRITE_ACK                      = 0x00000002,
    SQ_EXPORT_WRITE_IND_ACK                  = 0x00000003,
};

/* --------------------------------- */

enum
{
    R700_PM4_PACKET0_NOP = 0x00000000,
    R700_PM4_PACKET1_NOP = 0x40000000,
    R700_PM4_PACKET2_NOP = 0x80000000,
    R700_PM4_PACKET3_NOP = 0xC0000000,
};

#define  PM4_OPCODE_SET_INDEX_TYPE      (R700_PM4_PACKET3_NOP | (IT_INDEX_TYPE << 8))

#define  PM4_OPCODE_DRAW_INDEX_AUTO     (R700_PM4_PACKET3_NOP | (IT_DRAW_INDEX_AUTO << 8))
#define  PM4_OPCODE_DRAW_INDEX_IMMD     (R700_PM4_PACKET3_NOP | (IT_DRAW_INDEX_IMMD << 8))
#define  PM4_OPCODE_WAIT_REG_MEM        (R700_PM4_PACKET3_NOP | (IT_WAIT_REG_MEM << 8))
#define  PM4_OPCODE_SET_CONTEXT_REG     (R700_PM4_PACKET3_NOP | (IT_SET_CONTEXT_REG << 8))
#define  PM4_OPCODE_SET_CONFIG_REG      (R700_PM4_PACKET3_NOP | (IT_SET_CONFIG_REG << 8))
#define  PM4_OPCODE_SET_ALU_CONST       (R700_PM4_PACKET3_NOP | (IT_SET_ALU_CONST << 8))
#define  PM4_OPCODE_SET_RESOURCE        (R700_PM4_PACKET3_NOP | (IT_SET_RESOURCE << 8))
#define  PM4_OPCODE_SET_SAMPLER         (R700_PM4_PACKET3_NOP | (IT_SET_SAMPLER << 8))
#define  PM4_OPCODE_CONTEXT_CONTROL     (R700_PM4_PACKET3_NOP | (IT_CONTEXT_CONTROL << 8))

union UINT_FLOAT 
{
    unsigned int u32All;
    float	f32All;
};

#if 0
typedef struct _TEXTURE_STATE_STRUCT
{
    union UINT_FLOAT     SQ_TEX_RESOURCE0;
    union UINT_FLOAT     SQ_TEX_RESOURCE1;
    union UINT_FLOAT     SQ_TEX_RESOURCE2;
    union UINT_FLOAT     SQ_TEX_RESOURCE3;
    union UINT_FLOAT     SQ_TEX_RESOURCE4;
    union UINT_FLOAT     SQ_TEX_RESOURCE5;
    union UINT_FLOAT     SQ_TEX_RESOURCE6;
    GLboolean                         enabled;
} TEXTURE_STATE_STRUCT;

typedef struct _SAMPLER_STATE_STRUCT
{
    union UINT_FLOAT      SQ_TEX_SAMPLER0;
    union UINT_FLOAT      SQ_TEX_SAMPLER1;
    union UINT_FLOAT      SQ_TEX_SAMPLER2;
    GLboolean                         enabled;
} SAMPLER_STATE_STRUCT;

typedef struct _R700_TEXTURE_STATES
{
    TEXTURE_STATE_STRUCT *textures[R700_TEXTURE_NUMBERUNITS];
    SAMPLER_STATE_STRUCT *samplers[R700_TEXTURE_NUMBERUNITS];
} R700_TEXTURE_STATES;
#endif

typedef struct _RENDER_TARGET_STATE_STRUCT
{
	union UINT_FLOAT            	CB_COLOR0_BASE;  /* 0xA010 */
	union UINT_FLOAT            	CB_COLOR0_SIZE;  /* 0xA018 */
	union UINT_FLOAT            	CB_COLOR0_VIEW;  /* 0xA020 */
	union UINT_FLOAT            	CB_COLOR0_INFO;  /* 0xA028 */
	union UINT_FLOAT            	CB_COLOR0_TILE;  /* 0xA030 */
	union UINT_FLOAT            	CB_COLOR0_FRAG;  /* 0xA038 */
	union UINT_FLOAT            	CB_COLOR0_MASK;  /* 0xA040 */
	union UINT_FLOAT         	CB_BLEND0_CONTROL;  /* 0xA1E0 */
	GLboolean                         enabled;
	GLboolean                         dirty;
} RENDER_TARGET_STATE_STRUCT;

typedef struct _VIEWPORT_STATE_STRUCT
{
	union UINT_FLOAT  	PA_SC_VPORT_SCISSOR_0_TL;  /* 0xA094 */
	union UINT_FLOAT  	PA_SC_VPORT_SCISSOR_0_BR;  /* 0xA095 */
	union UINT_FLOAT        PA_SC_VPORT_ZMIN_0;        /* 0xA0B4 */
	union UINT_FLOAT        PA_SC_VPORT_ZMAX_0;        /* 0xA0B5 */
	union UINT_FLOAT        PA_CL_VPORT_XSCALE;        /* 0xA10F */
	union UINT_FLOAT       	PA_CL_VPORT_XOFFSET;       /* 0xA110 */
	union UINT_FLOAT        PA_CL_VPORT_YSCALE;        /* 0xA111 */
	union UINT_FLOAT       	PA_CL_VPORT_YOFFSET;       /* 0xA112 */
	union UINT_FLOAT        PA_CL_VPORT_ZSCALE;        /* 0xA113 */
	union UINT_FLOAT       	PA_CL_VPORT_ZOFFSET;       /* 0xA114 */
	GLboolean                         enabled;
	GLboolean                         dirty;
} VIEWPORT_STATE_STRUCT;

typedef struct _UCP_STATE_STRUCT
{
	union UINT_FLOAT        PA_CL_UCP_0_X;
	union UINT_FLOAT        PA_CL_UCP_0_Y;
	union UINT_FLOAT        PA_CL_UCP_0_Z;
	union UINT_FLOAT        PA_CL_UCP_0_W;
	GLboolean                         enabled;
	GLboolean                         dirty;
} UCP_STATE_STRUCT;

typedef struct _PS_STATE_STRUCT
{
	union UINT_FLOAT           	SQ_PGM_START_PS           ;  /* 0xA210 */
	union UINT_FLOAT       	        SQ_PGM_RESOURCES_PS       ;  /* 0xA214 */
	union UINT_FLOAT         	SQ_PGM_EXPORTS_PS         ;  /* 0xA215 */
	union UINT_FLOAT       	        SQ_PGM_CF_OFFSET_PS       ;  /* 0xA233 */
	GLboolean                         dirty;
	int                             num_consts;
	union UINT_FLOAT                consts[R700_MAX_DX9_CONSTS][4];
} PS_STATE_STRUCT;

typedef struct _VS_STATE_STRUCT
{
 	union UINT_FLOAT           	SQ_PGM_START_VS           ;  /* 0xA216 */
	union UINT_FLOAT  		SQ_PGM_RESOURCES_VS       ;  /* 0xA21A */
	union UINT_FLOAT       	        SQ_PGM_CF_OFFSET_VS       ;  /* 0xA234 */
	GLboolean                         dirty;
	int                             num_consts;
	union UINT_FLOAT                consts[R700_MAX_DX9_CONSTS][4];
} VS_STATE_STRUCT;

typedef struct _GS_STATE_STRUCT
{
	union UINT_FLOAT           	SQ_PGM_START_GS           ;  /* 0xA21B */
	union UINT_FLOAT       	        SQ_PGM_RESOURCES_GS       ;  /* 0xA21F */
	union UINT_FLOAT       	        SQ_PGM_CF_OFFSET_GS       ;  /* 0xA235 */
	GLboolean                         dirty;
} GS_STATE_STRUCT;

typedef struct _ES_STATE_STRUCT
{
	union UINT_FLOAT           	SQ_PGM_START_ES           ;  /* 0xA220 */
	union UINT_FLOAT       	        SQ_PGM_RESOURCES_ES       ;  /* 0xA224 */
	union UINT_FLOAT       	        SQ_PGM_CF_OFFSET_ES       ;  /* 0xA236 */
	GLboolean                         dirty;
} ES_STATE_STRUCT;

typedef struct _FS_STATE_STRUCT
{
	union UINT_FLOAT           	SQ_PGM_START_FS           ;  /* 0xA225 */
	union UINT_FLOAT       	        SQ_PGM_RESOURCES_FS       ;  /* 0xA229 */
	union UINT_FLOAT       	        SQ_PGM_CF_OFFSET_FS       ;  /* 0xA237 */
	GLboolean                         dirty;
} FS_STATE_STRUCT;

typedef struct _SQ_CONFIG_STRUCT
{
	union UINT_FLOAT     	        SQ_CONFIG                 ;  /* 0x2300 */
	union UINT_FLOAT     	        SQ_GPR_RESOURCE_MGMT_1    ;  /* 0x2301 */
	union UINT_FLOAT     	        SQ_GPR_RESOURCE_MGMT_2    ;  /* 0x2302 */
	union UINT_FLOAT     	        SQ_THREAD_RESOURCE_MGMT   ;  /* 0x2303 */
	union UINT_FLOAT     	        SQ_STACK_RESOURCE_MGMT_1  ;  /* 0x2304 */
	union UINT_FLOAT     	        SQ_STACK_RESOURCE_MGMT_2  ;  /* 0x2305 */
} SQ_CONFIG_STRUCT;

typedef struct _R700_CHIP_CONTEXT
{
	// DB
	union UINT_FLOAT             	DB_DEPTH_SIZE             ;  /* 0xA000 */
	union UINT_FLOAT             	DB_DEPTH_VIEW             ;  /* 0xA001 */
	union UINT_FLOAT             	DB_DEPTH_BASE             ;  /* 0xA003 */
	union UINT_FLOAT             	DB_DEPTH_INFO             ;  /* 0xA004 */
	GLboolean                       db_target_dirty;
	union UINT_FLOAT                DB_HTILE_DATA_BASE        ;  /* 0xA005 */
	union UINT_FLOAT          	DB_STENCIL_CLEAR          ;  /* 0xA00A */
	union UINT_FLOAT            	DB_DEPTH_CLEAR            ;  /* 0xA00B */
	union UINT_FLOAT            	DB_STENCILREFMASK         ;  /* 0xA10C */
	union UINT_FLOAT            	DB_STENCILREFMASK_BF      ;  /* 0xA10D */
	union UINT_FLOAT         	DB_RENDER_CONTROL         ;  /* 0xA343 */
	union UINT_FLOAT        	DB_RENDER_OVERRIDE        ;  /* 0xA344 */
	union UINT_FLOAT          	DB_HTILE_SURFACE          ;  /* 0xA349 */
	union UINT_FLOAT          	DB_ALPHA_TO_MASK          ;  /* 0xA351 */
	union UINT_FLOAT          	DB_DEPTH_CONTROL          ;  /* 0xA200 */
	union UINT_FLOAT         	DB_SHADER_CONTROL         ;  /* 0xA203 */
	GLboolean                       db_dirty;

	// SC
	union UINT_FLOAT   	        PA_SC_SCREEN_SCISSOR_TL   ;  /* 0xA00C */
	union UINT_FLOAT   	        PA_SC_SCREEN_SCISSOR_BR   ;  /* 0xA00D */
	union UINT_FLOAT       	        PA_SC_WINDOW_OFFSET       ;  /* 0xA080 */
	union UINT_FLOAT   	        PA_SC_WINDOW_SCISSOR_TL   ;  /* 0xA081 */
	union UINT_FLOAT   	        PA_SC_WINDOW_SCISSOR_BR   ;  /* 0xA082 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_RULE       ;  /* 0xA083 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_0_TL       ;  /* 0xA084 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_0_BR       ;  /* 0xA085 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_1_TL       ;  /* 0xA086 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_1_BR       ;  /* 0xA087 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_2_TL       ;  /* 0xA088 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_2_BR       ;  /* 0xA089 */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_3_TL       ;  /* 0xA08A */
	union UINT_FLOAT       	        PA_SC_CLIPRECT_3_BR       ;  /* 0xA08B */
	union UINT_FLOAT            	PA_SC_EDGERULE            ;  /* 0xA08C */
	union UINT_FLOAT  	        PA_SC_GENERIC_SCISSOR_TL  ;  /* 0xA090 */
	union UINT_FLOAT  	        PA_SC_GENERIC_SCISSOR_BR  ;  /* 0xA091 */
	GLboolean                       scissor_dirty;

	union UINT_FLOAT        	PA_SC_LINE_STIPPLE        ;  /* 0xA283 */
	union UINT_FLOAT           	PA_SC_LINE_CNTL           ;  /* 0xA300 */
	union UINT_FLOAT           	PA_SC_AA_CONFIG           ;  /* 0xA301 */
	union UINT_FLOAT       	        PA_SC_MPASS_PS_CNTL       ;  /* 0xA292 */
	union UINT_FLOAT           	PA_SC_MODE_CNTL           ;  /* 0xA293 */
	union UINT_FLOAT 	        PA_SC_AA_SAMPLE_LOCS_MCTX ;  /* 0xA307 */
	union UINT_FLOAT                PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX; /* 0xA308 */
	union UINT_FLOAT             	PA_SC_AA_MASK             ;  /* 0xA312 */
	GLboolean                       sc_dirty;

	// CL
	union UINT_FLOAT           	PA_CL_CLIP_CNTL           ;  /* 0xA204 */
	union UINT_FLOAT            	PA_CL_VTE_CNTL            ;  /* 0xA206 */
	union UINT_FLOAT         	PA_CL_VS_OUT_CNTL         ;  /* 0xA207 */
	union UINT_FLOAT         	PA_CL_NANINF_CNTL         ;  /* 0xA208 */
	union UINT_FLOAT    	        PA_CL_GB_VERT_CLIP_ADJ    ;  /* 0xA303 */
	union UINT_FLOAT    	        PA_CL_GB_VERT_DISC_ADJ    ;  /* 0xA304 */
	union UINT_FLOAT    	        PA_CL_GB_HORZ_CLIP_ADJ    ;  /* 0xA305 */
	union UINT_FLOAT    	        PA_CL_GB_HORZ_DISC_ADJ    ;  /* 0xA306 */
	GLboolean                       cl_dirty;

	// SU
	union UINT_FLOAT        	PA_SU_SC_MODE_CNTL        ;  /* 0xA205 */
	union UINT_FLOAT          	PA_SU_POINT_SIZE          ;  /* 0xA280 */
	union UINT_FLOAT        	PA_SU_POINT_MINMAX        ;  /* 0xA281 */
	union UINT_FLOAT           	PA_SU_LINE_CNTL           ;  /* 0xA282 */
	union UINT_FLOAT            	PA_SU_VTX_CNTL            ;  /* 0xA302 */
	union UINT_FLOAT                PA_SU_POLY_OFFSET_DB_FMT_CNTL;   /* 0xA37E */
	union UINT_FLOAT   	        PA_SU_POLY_OFFSET_CLAMP   ;      /* 0xA37F */
	union UINT_FLOAT                PA_SU_POLY_OFFSET_FRONT_SCALE;   /* 0xA380 */
	union UINT_FLOAT                PA_SU_POLY_OFFSET_FRONT_OFFSET; /* 0xA381 */
	union UINT_FLOAT                PA_SU_POLY_OFFSET_BACK_SCALE;    /* 0xA382 */
	union UINT_FLOAT                PA_SU_POLY_OFFSET_BACK_OFFSET;   /* 0xA383 */
	GLboolean                       su_dirty;

	VIEWPORT_STATE_STRUCT           viewport[R700_MAX_VIEWPORTS];
	UCP_STATE_STRUCT                ucp[R700_MAX_UCP];

	// CB
	union UINT_FLOAT              	CB_CLEAR_RED_R6XX         ;  /* 0xA048 */
	union UINT_FLOAT            	CB_CLEAR_GREEN_R6XX       ;  /* 0xA049 */
	union UINT_FLOAT             	CB_CLEAR_BLUE_R6XX        ;  /* 0xA04A */
	union UINT_FLOAT            	CB_CLEAR_ALPHA_R6XX       ;  /* 0xA04B */
	union UINT_FLOAT            	CB_TARGET_MASK            ;  /* 0xA08E */
	union UINT_FLOAT            	CB_SHADER_MASK            ;  /* 0xA08F */
	union UINT_FLOAT              	CB_BLEND_RED              ;  /* 0xA105 */
	union UINT_FLOAT            	CB_BLEND_GREEN            ;  /* 0xA106 */
	union UINT_FLOAT             	CB_BLEND_BLUE             ;  /* 0xA107 */
	union UINT_FLOAT            	CB_BLEND_ALPHA            ;  /* 0xA108 */
	union UINT_FLOAT              	CB_FOG_RED_R6XX           ;  /* 0xA109 */
	union UINT_FLOAT            	CB_FOG_GREEN_R6XX         ;  /* 0xA10A */
	union UINT_FLOAT             	CB_FOG_BLUE_R6XX          ;  /* 0xA10B */
	union UINT_FLOAT         	CB_SHADER_CONTROL         ;  /* 0xA1E8 */
	union UINT_FLOAT          	CB_COLOR_CONTROL          ;  /* 0xA202 */
	union UINT_FLOAT         	CB_CLRCMP_CONTROL         ;  /* 0xA30C */
	union UINT_FLOAT             	CB_CLRCMP_SRC             ;  /* 0xA30D */
	union UINT_FLOAT             	CB_CLRCMP_DST             ;  /* 0xA30E */
	union UINT_FLOAT             	CB_CLRCMP_MSK             ;  /* 0xA30F */
	union UINT_FLOAT             	CB_BLEND_CONTROL          ;  /* 0xABD0 */
	GLboolean                       cb_dirty;
	RENDER_TARGET_STATE_STRUCT      render_target[R700_MAX_RENDER_TARGETS];

	// SX
	union UINT_FLOAT                SX_MISC                   ;  /* 0xA0D4 */
	union UINT_FLOAT     	        SX_ALPHA_TEST_CONTROL     ;  /* 0xA104 */
	union UINT_FLOAT     	        SX_ALPHA_REF              ;  /* 0xA10E */
	GLboolean                       sx_dirty;

	// VGT
	union UINT_FLOAT          	VGT_MAX_VTX_INDX          ;  /* 0xA100 */
	union UINT_FLOAT          	VGT_MIN_VTX_INDX          ;  /* 0xA101 */
	union UINT_FLOAT           	VGT_INDX_OFFSET           ;  /* 0xA102 */
	union UINT_FLOAT                VGT_MULTI_PRIM_IB_RESET_INDX;  /* 0xA103 */
	union UINT_FLOAT      	        VGT_OUTPUT_PATH_CNTL      ;  /* 0xA284 */
	union UINT_FLOAT      	        VGT_HOS_CNTL              ;  /* 0xA285 */
	union UINT_FLOAT      	        VGT_HOS_MAX_TESS_LEVEL    ;  /* 0xA286 */
	union UINT_FLOAT      	        VGT_HOS_MIN_TESS_LEVEL    ;  /* 0xA287 */
	union UINT_FLOAT      	        VGT_HOS_REUSE_DEPTH       ;  /* 0xA288 */
	union UINT_FLOAT      	        VGT_GROUP_PRIM_TYPE       ;  /* 0xA289 */
	union UINT_FLOAT      	        VGT_GROUP_FIRST_DECR      ;  /* 0xA28A */
	union UINT_FLOAT      	        VGT_GROUP_DECR            ;  /* 0xA28B */
	union UINT_FLOAT      	        VGT_GROUP_VECT_0_CNTL     ;  /* 0xA28C */
	union UINT_FLOAT      	        VGT_GROUP_VECT_1_CNTL     ;  /* 0xA28D */
	union UINT_FLOAT      	        VGT_GROUP_VECT_0_FMT_CNTL ;  /* 0xA28E */
	union UINT_FLOAT      	        VGT_GROUP_VECT_1_FMT_CNTL ;  /* 0xA28F */
	union UINT_FLOAT               	VGT_GS_MODE               ;  /* 0xA290 */
	union UINT_FLOAT        	VGT_PRIMITIVEID_EN        ;  /* 0xA2A1 */
	union UINT_FLOAT	        VGT_MULTI_PRIM_IB_RESET_EN;  /* 0xA2A5 */
	union UINT_FLOAT  	        VGT_INSTANCE_STEP_RATE_0  ;  /* 0xA2A8 */
	union UINT_FLOAT  	        VGT_INSTANCE_STEP_RATE_1  ;  /* 0xA2A9 */
	union UINT_FLOAT            	VGT_STRMOUT_EN            ;  /* 0xA2AC */
	union UINT_FLOAT             	VGT_REUSE_OFF             ;  /* 0xA2AD */
	union UINT_FLOAT             	VGT_VTX_CNT_EN            ;  /* 0xA2AE */
	union UINT_FLOAT            	VGT_STRMOUT_BUFFER_EN     ;  /* 0xA2C8 */
	GLboolean                       vgt_dirty;

	// SPI
	union UINT_FLOAT           	SPI_VS_OUT_ID_0           ;  /* 0xA185 */
	union UINT_FLOAT           	SPI_VS_OUT_ID_1           ;  /* 0xA186 */
	union UINT_FLOAT           	SPI_VS_OUT_ID_2           ;  /* 0xA187 */
	union UINT_FLOAT           	SPI_VS_OUT_ID_3           ;  /* 0xA188 */
	union UINT_FLOAT           	SPI_VS_OUT_ID_4           ;  /* 0xA189 */
	union UINT_FLOAT           	SPI_VS_OUT_ID_5           ;  /* 0xA18A */
	union UINT_FLOAT           	SPI_VS_OUT_ID_6           ;  /* 0xA18B */
	union UINT_FLOAT           	SPI_VS_OUT_ID_7           ;  /* 0xA18C */
	union UINT_FLOAT           	SPI_VS_OUT_ID_8           ;  /* 0xA18D */
	union UINT_FLOAT           	SPI_VS_OUT_ID_9           ;  /* 0xA18E */
	union UINT_FLOAT                SPI_VS_OUT_CONFIG         ;  /* 0xA1B1 */
	union UINT_FLOAT       	        SPI_THREAD_GROUPING       ;  /* 0xA1B2 */
	union UINT_FLOAT       	        SPI_PS_IN_CONTROL_0       ;  /* 0xA1B3 */
	union UINT_FLOAT       	        SPI_PS_IN_CONTROL_1       ;  /* 0xA1B4 */
	union UINT_FLOAT       	        SPI_INTERP_CONTROL_0      ;  /* 0xA1B5 */
 	union UINT_FLOAT               	SPI_INPUT_Z               ;  /* 0xA1B6 */
	union UINT_FLOAT              	SPI_FOG_CNTL              ;  /* 0xA1B7 */
	union UINT_FLOAT              	SPI_FOG_FUNC_SCALE        ;  /* 0xA1B8 */
	union UINT_FLOAT              	SPI_FOG_FUNC_BIAS         ;  /* 0xA1B9 */

	union UINT_FLOAT         	SQ_VTX_SEMANTIC_0         ;  /* 0xA0E0 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_1         ;  /* 0xA0E1 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_2         ;  /* 0xA0E2 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_3         ;  /* 0xA0E3 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_4         ;  /* 0xA0E4 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_5         ;  /* 0xA0E5 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_6         ;  /* 0xA0E6 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_7         ;  /* 0xA0E7 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_8         ;  /* 0xA0E8 */
	union UINT_FLOAT         	SQ_VTX_SEMANTIC_9         ;  /* 0xA0E9 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_10        ;  /* 0xA0EA */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_11        ;  /* 0xA0EB */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_12        ;  /* 0xA0EC */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_13        ;  /* 0xA0ED */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_14        ;  /* 0xA0EE */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_15        ;  /* 0xA0EF */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_16        ;  /* 0xA0F0 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_17        ;  /* 0xA0F1 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_18        ;  /* 0xA0F2 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_19        ;  /* 0xA0F3 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_20        ;  /* 0xA0F4 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_21        ;  /* 0xA0F5 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_22        ;  /* 0xA0F6 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_23        ;  /* 0xA0F7 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_24        ;  /* 0xA0F8 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_25        ;  /* 0xA0F9 */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_26        ;  /* 0xA0FA */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_27        ;  /* 0xA0FB */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_28        ;  /* 0xA0FC */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_29        ;  /* 0xA0FD */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_30        ;  /* 0xA0FE */
	union UINT_FLOAT        	SQ_VTX_SEMANTIC_31        ;  /* 0xA0FF */
	union UINT_FLOAT       	        SPI_PS_INPUT_CNTL[R700_MAX_SHADER_EXPORTS];
	GLboolean                       spi_dirty;

	// shaders
	PS_STATE_STRUCT                 ps;
	VS_STATE_STRUCT                 vs;
	GS_STATE_STRUCT                 gs;
	ES_STATE_STRUCT                 es;
	FS_STATE_STRUCT                 fs;

	// SQ CONFIG
	SQ_CONFIG_STRUCT                sq_config;
	// misc
	union UINT_FLOAT             	TA_CNTL_AUX               ;  /* 0x2542 */
	union UINT_FLOAT             	VC_ENHANCE                ;  /* 0x25C5 */
	union UINT_FLOAT             	SQ_DYN_GPR_CNTL_PS_FLUSH_REQ;  /* 0x2363 */
	union UINT_FLOAT             	DB_DEBUG                  ;  /* 0x260C */
	union UINT_FLOAT             	DB_WATERMARKS             ;  /* 0x260E */
	// SQ
	union UINT_FLOAT     	        SQ_ESGS_RING_ITEMSIZE     ;  /* 0xA22A */
	union UINT_FLOAT     	        SQ_GSVS_RING_ITEMSIZE     ;  /* 0xA22B */
	union UINT_FLOAT    	        SQ_ESTMP_RING_ITEMSIZE    ;  /* 0xA22C */
	union UINT_FLOAT    	        SQ_GSTMP_RING_ITEMSIZE    ;  /* 0xA22D */
	union UINT_FLOAT    	        SQ_VSTMP_RING_ITEMSIZE    ;  /* 0xA22E */
	union UINT_FLOAT    	        SQ_PSTMP_RING_ITEMSIZE    ;  /* 0xA22F */
	union UINT_FLOAT     	        SQ_FBUF_RING_ITEMSIZE     ;  /* 0xA230 */
	union UINT_FLOAT    	        SQ_REDUC_RING_ITEMSIZE    ;  /* 0xA231 */
	union UINT_FLOAT       	        SQ_GS_VERT_ITEMSIZE       ;  /* 0xA232 */
	GLboolean                       sq_dirty;

	radeonTexObj*                   textures[R700_TEXTURE_NUMBERUNITS];

	GLboolean                       bEnablePerspective;

} R700_CHIP_CONTEXT;

#endif /* _R700_CHIP_H_ */

