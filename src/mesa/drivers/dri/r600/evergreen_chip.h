/*
 * Copyright (C) 2008-2010  Advanced Micro Devices, Inc.
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

#ifndef _EVERGREEN_CHIP_H_
#define _EVERGREEN_CHIP_H_

#include "r700_chip.h"

#define EVERGREEN_MAX_DX9_CONSTS      256
#define EVERGREEN_MAX_SHADER_EXPORTS  32
#define EVERGREEN_MAX_VIEWPORTS       16

typedef struct _EVERGREEN_VIEWPORT_STATE
{
	union UINT_FLOAT PA_SC_VPORT_SCISSOR_0_TL;   ////0,1         // = 0x28250, // DIFF 
    union UINT_FLOAT PA_SC_VPORT_SCISSOR_0_BR;   ////0,1         // = 0x28254, // DIFF 
	union UINT_FLOAT PA_SC_VPORT_ZMIN_0; ////0                        // = 0x282D0, // SAME 
    union UINT_FLOAT PA_SC_VPORT_ZMAX_0; ////0                        // = 0x282D4, // SAME 
	union UINT_FLOAT PA_CL_VPORT_XSCALE;    ////                 // = 0x2843C, // SAME 
    union UINT_FLOAT PA_CL_VPORT_XOFFSET;   ////                 // = 0x28440, // SAME 
    union UINT_FLOAT PA_CL_VPORT_YSCALE;    ////                 // = 0x28444, // SAME 
    union UINT_FLOAT PA_CL_VPORT_YOFFSET;   ////                 // = 0x28448, // SAME 
    union UINT_FLOAT PA_CL_VPORT_ZSCALE;    ////                 // = 0x2844C, // SAME 
    union UINT_FLOAT PA_CL_VPORT_ZOFFSET;   ////                 // = 0x28450, // SAME 
	GLboolean                         enabled;
	GLboolean                         dirty;
} EVERGREEN_VIEWPORT_STATE;

#define EVERGREEN_MAX_UCP             6

typedef struct _EVERGREEN_UCP_STATE
{
	union UINT_FLOAT PA_CL_UCP_0_X;                              // = 0x285BC, // SAME 0x28E20 
    union UINT_FLOAT PA_CL_UCP_0_Y;                              // = 0x285C0, // SAME 0x28E24 
    union UINT_FLOAT PA_CL_UCP_0_Z;                              // = 0x285C4, // SAME 0x28E28 
    union UINT_FLOAT PA_CL_UCP_0_W;                              // = 0x285C8, // SAME 0x28E2C 
	GLboolean                         enabled;
	GLboolean                         dirty;
} EVERGREEN_UCP_STATE;

#define EVERGREEN_MAX_RENDER_TARGETS  12

typedef struct _EVERGREEN_RENDER_TARGET_STATE
{
	union UINT_FLOAT CB_COLOR0_BASE;   ////0                          // = 0x28C60, // SAME 0x28040 
    union UINT_FLOAT CB_COLOR0_PITCH;  ////0                          // = 0x28C64, // 
    union UINT_FLOAT CB_COLOR0_SLICE;  ////0                          // = 0x28C68, // 
    union UINT_FLOAT CB_COLOR0_VIEW;   ////0                          // = 0x28C6C, // SAME 0x28080 
    union UINT_FLOAT CB_COLOR0_INFO;   ////0,1,2,3,4,5,6,78,9,10,11                          // = 0x28C70, // DIFF 0x280A0 
    union UINT_FLOAT CB_COLOR0_ATTRIB; ////0                          // = 0x28C74, // 
    union UINT_FLOAT CB_COLOR0_DIM;                              // = 0x28C78, // 
    union UINT_FLOAT CB_COLOR0_CMASK;  ////0                          // = 0x28C7C, // 
    union UINT_FLOAT CB_COLOR0_CMASK_SLICE; ////0                     // = 0x28C80, // 
    union UINT_FLOAT CB_COLOR0_FMASK; ////0                           // = 0x28C84, // 
    union UINT_FLOAT CB_COLOR0_FMASK_SLICE;  ////0                    // = 0x28C88, // 
    union UINT_FLOAT CB_COLOR0_CLEAR_WORD0;                      // = 0x28C8C, // 
    union UINT_FLOAT CB_COLOR0_CLEAR_WORD1;                      // = 0x28C90, // 
    union UINT_FLOAT CB_COLOR0_CLEAR_WORD2;                      // = 0x28C94, // 
    union UINT_FLOAT CB_COLOR0_CLEAR_WORD3;                      // = 0x28C98, //
	GLboolean                         enabled;
	GLboolean                         dirty;
} EVERGREEN_RENDER_TARGET_STATE;

typedef struct _EVERGREEN_CONFIG
{
    union UINT_FLOAT                SPI_CONFIG_CNTL;              // = 0x9100,  // DIFF
    union UINT_FLOAT                SPI_CONFIG_CNTL_1;            // = 0x913C,  // DIFF
    union UINT_FLOAT                CP_PERFMON_CNTL;              // = 0x87FC,  // SAME
    union UINT_FLOAT                SQ_MS_FIFO_SIZES;             // = 0x8CF0,  // SAME

	union UINT_FLOAT     	        SQ_CONFIG;                    // = 0x8C00,  // DIFF 
	union UINT_FLOAT     	        SQ_GPR_RESOURCE_MGMT_1;       // = 0x8C04,  // SAME 
	union UINT_FLOAT     	        SQ_GPR_RESOURCE_MGMT_2;       // = 0x8C08,  // SAME 
    union UINT_FLOAT     	        SQ_GPR_RESOURCE_MGMT_3;       // = 0x8C0C,  //

	union UINT_FLOAT     	        SQ_THREAD_RESOURCE_MGMT;      // = 0x8C18,  // SAME 0x8C0C 
    union UINT_FLOAT     	        SQ_THREAD_RESOURCE_MGMT_2;    // = 0x8C1C,  //
	union UINT_FLOAT     	        SQ_STACK_RESOURCE_MGMT_1;     // = 0x8C20,  // SAME 0x8C10 
	union UINT_FLOAT     	        SQ_STACK_RESOURCE_MGMT_2;     // = 0x8C24,  // SAME 0x8C14 
    union UINT_FLOAT     	        SQ_STACK_RESOURCE_MGMT_3;     // = 0x8C28,  //

    union UINT_FLOAT     	        SQ_DYN_GPR_CNTL_PS_FLUSH_REQ; // = 0x8D8C,  // DIFF
    union UINT_FLOAT     	        SQ_LDS_RESOURCE_MGMT;         // = 0x8E2C,  //            
    union UINT_FLOAT     	        VGT_CACHE_INVALIDATION;       // = 0x88C4,  // DIFF
    union UINT_FLOAT     	        VGT_GS_VERTEX_REUSE;          // = 0x88D4,  // SAME
    union UINT_FLOAT     	        PA_SC_FORCE_EOV_MAX_CNTS;     // = 0x8B24,  // SAME
    union UINT_FLOAT     	        PA_SC_LINE_STIPPLE_STATE;     // = 0x8B10,  // SAME
    union UINT_FLOAT     	        PA_CL_ENHANCE;                // = 0x8A14,  // SAME
} EVERGREEN_CONFIG;

typedef struct _EVERGREEN_PS_RES
{
	union            UINT_FLOAT SQ_PGM_START_PS; ////                           // = 0x28840, // SAME 	
	GLboolean        dirty;

    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_0;                    // = 0x28940, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_1;                    // = 0x28944, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_2;                    // = 0x28948, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_3;                    // = 0x2894C, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_4;                    // = 0x28950, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_5;                    // = 0x28954, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_6;                    // = 0x28958, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_7;                    // = 0x2895C, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_8;                    // = 0x28960, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_9;                    // = 0x28964, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_10;                   // = 0x28968, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_11;                   // = 0x2896C, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_12;                   // = 0x28970, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_13;                   // = 0x28974, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_14;                   // = 0x28978, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_PS_15;                   // = 0x2897C, // SAME 

	int              num_consts;
	union UINT_FLOAT consts[EVERGREEN_MAX_DX9_CONSTS][4];
} EVERGREEN_PS_RES;

typedef struct _EVERGREEN_VS_RES
{
 	union UINT_FLOAT SQ_PGM_START_VS;               ////             // = 0x2885C, // SAME 0x28858 	
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_VS_0; ////             // = 0x28180, //?
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_0;       ////             // = 0x28980, // SAME 

    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_1;                    // = 0x28984, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_2;                    // = 0x28988, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_3;                    // = 0x2898C, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_4;                    // = 0x28990, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_5;                    // = 0x28994, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_6;                    // = 0x28998, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_7;                    // = 0x2899C, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_8;                    // = 0x289A0, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_9;                    // = 0x289A4, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_10;                   // = 0x289A8, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_11;                   // = 0x289AC, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_12;                   // = 0x289B0, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_13;                   // = 0x289B4, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_14;                   // = 0x289B8, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_VS_15;                   // = 0x289BC, // SAME

	GLboolean        dirty;
	int              num_consts;
	union UINT_FLOAT consts[EVERGREEN_MAX_DX9_CONSTS][4];
} EVERGREEN_VS_RES;

typedef struct _EVERGREEN_CHIP_CONTEXT 
{
/* Registers from PA block: */
    union UINT_FLOAT PA_SC_SCREEN_SCISSOR_TL;  ////                  // = 0x28030, // DIFF 
    union UINT_FLOAT PA_SC_SCREEN_SCISSOR_BR;  ////                  // = 0x28034, // DIFF 
    union UINT_FLOAT PA_SC_WINDOW_OFFSET;      ////                  // = 0x28200, // DIFF 
    union UINT_FLOAT PA_SC_WINDOW_SCISSOR_TL;  ////                  // = 0x28204, // DIFF 
    union UINT_FLOAT PA_SC_WINDOW_SCISSOR_BR;  ////                  // = 0x28208, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_RULE;      ////                  // = 0x2820C, // SAME 
    union UINT_FLOAT PA_SC_CLIPRECT_0_TL;      ////                  // = 0x28210, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_0_BR;      ////                  // = 0x28214, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_1_TL; ////                   // = 0x28218, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_1_BR; ////                   // = 0x2821C, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_2_TL; ////                   // = 0x28220, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_2_BR; ////                   // = 0x28224, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_3_TL; ////                   // = 0x28228, // DIFF 
    union UINT_FLOAT PA_SC_CLIPRECT_3_BR; ////                   // = 0x2822C, // DIFF 
    union UINT_FLOAT PA_SC_EDGERULE;                             // = 0x28230, // SAME 
    union UINT_FLOAT PA_SU_HARDWARE_SCREEN_OFFSET;               // = 0x28234, // 
    union UINT_FLOAT PA_SC_GENERIC_SCISSOR_TL; ////              // = 0x28240, // DIFF 
    union UINT_FLOAT PA_SC_GENERIC_SCISSOR_BR; ////              // = 0x28244, // DIFF
    
    EVERGREEN_VIEWPORT_STATE viewport[EVERGREEN_MAX_VIEWPORTS];    
    EVERGREEN_UCP_STATE      ucp[EVERGREEN_MAX_UCP];

    union UINT_FLOAT PA_CL_POINT_X_RAD;                          // = 0x287D4, // SAME 0x28E10 
    union UINT_FLOAT PA_CL_POINT_Y_RAD;                          // = 0x287D8, // SAME 0x28E14 
    union UINT_FLOAT PA_CL_POINT_SIZE;                           // = 0x287DC, // SAME 0x28E18 
    union UINT_FLOAT PA_CL_POINT_CULL_RAD;                       // = 0x287E0, // SAME 0x28E1C 
    union UINT_FLOAT PA_CL_CLIP_CNTL;   ////                         // = 0x28810, // SAME 
    union UINT_FLOAT PA_SU_SC_MODE_CNTL; ////                        // = 0x28814, // SAME 
    union UINT_FLOAT PA_CL_VTE_CNTL;   ////                          // = 0x28818, // SAME 
    union UINT_FLOAT PA_CL_VS_OUT_CNTL; ////                         // = 0x2881C, // SAME 
    union UINT_FLOAT PA_CL_NANINF_CNTL;  ////                        // = 0x28820, // SAME 
    union UINT_FLOAT PA_SU_LINE_STIPPLE_CNTL;                    // = 0x28824, // 
    union UINT_FLOAT PA_SU_LINE_STIPPLE_SCALE;                   // = 0x28828, // 
    union UINT_FLOAT PA_SU_PRIM_FILTER_CNTL;                     // = 0x2882C, // 
    union UINT_FLOAT PA_SU_POINT_SIZE; ////                          // = 0x28A00, // SAME 
    union UINT_FLOAT PA_SU_POINT_MINMAX;  ////                       // = 0x28A04, // SAME 
    union UINT_FLOAT PA_SU_LINE_CNTL;    ////                        // = 0x28A08, // SAME 
    union UINT_FLOAT PA_SC_LINE_STIPPLE;                         // = 0x28A0C, // SAME 
    union UINT_FLOAT PA_SC_MODE_CNTL_0; ////                         // = 0x28A48, // 
    union UINT_FLOAT PA_SC_MODE_CNTL_1; ////                         // = 0x28A4C, // 
    union UINT_FLOAT PA_SU_POLY_OFFSET_DB_FMT_CNTL; ////             // = 0x28B78, // SAME 0x28DF8 
    union UINT_FLOAT PA_SU_POLY_OFFSET_CLAMP; ////                   // = 0x28B7C, // SAME 0x28DFC 
    union UINT_FLOAT PA_SU_POLY_OFFSET_FRONT_SCALE;////              // = 0x28B80, // SAME 0x28E00 
    union UINT_FLOAT PA_SU_POLY_OFFSET_FRONT_OFFSET; ////            // = 0x28B84, // SAME 0x28E04 
    union UINT_FLOAT PA_SU_POLY_OFFSET_BACK_SCALE;  ////             // = 0x28B88, // SAME 0x28E08 
    union UINT_FLOAT PA_SU_POLY_OFFSET_BACK_OFFSET; ////             // = 0x28B8C, // SAME 0x28E0C 
    union UINT_FLOAT PA_SC_LINE_CNTL; ////                           // = 0x28C00, // DIFF 
    union UINT_FLOAT PA_SC_AA_CONFIG; ////                           // = 0x28C04, // SAME 
    union UINT_FLOAT PA_SU_VTX_CNTL;  ////                           // = 0x28C08, // SAME 
    union UINT_FLOAT PA_CL_GB_VERT_CLIP_ADJ; ////                    // = 0x28C0C, // SAME 
    union UINT_FLOAT PA_CL_GB_VERT_DISC_ADJ; ////                    // = 0x28C10, // SAME 
    union UINT_FLOAT PA_CL_GB_HORZ_CLIP_ADJ; ////                    // = 0x28C14, // SAME 
    union UINT_FLOAT PA_CL_GB_HORZ_DISC_ADJ; ////                    // = 0x28C18, // SAME 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_0; ////                    // = 0x28C1C, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_1; ////                    // = 0x28C20, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_2; ////                    // = 0x28C24, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_3; ////                    // = 0x28C28, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_4; ////                    // = 0x28C2C, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_5; ////                    // = 0x28C30, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_6; ////                    // = 0x28C34, // 
    union UINT_FLOAT PA_SC_AA_SAMPLE_LOCS_7; ////                    // = 0x28C38, // 
    union UINT_FLOAT PA_SC_AA_MASK;   ////                           // = 0x28C3C, // SAME 0x28C48 

/* Registers from VGT block: */
    union UINT_FLOAT VGT_INDEX_TYPE;                             // =  0x895C, // SAME
    union UINT_FLOAT VGT_PRIMITIVE_TYPE;                         // =  0x8958, // SAME
    union UINT_FLOAT VGT_MAX_VTX_INDX;   ////                    // = 0x28400, // SAME 
    union UINT_FLOAT VGT_MIN_VTX_INDX;   ////                    // = 0x28404, // SAME 
    union UINT_FLOAT VGT_INDX_OFFSET;    ////                    // = 0x28408, // SAME 
    union UINT_FLOAT VGT_MULTI_PRIM_IB_RESET_INDX;               // = 0x2840C, // SAME         
         
    union UINT_FLOAT VGT_DRAW_INITIATOR;                         // = 0x287F0, // SAME 
    union UINT_FLOAT VGT_IMMED_DATA;                             // = 0x287F4, // SAME 
     
    union UINT_FLOAT VGT_OUTPUT_PATH_CNTL; ////                      // = 0x28A10, // DIFF 
    union UINT_FLOAT VGT_HOS_CNTL;                               // = 0x28A14, // SAME 
    union UINT_FLOAT VGT_HOS_MAX_TESS_LEVEL;                     // = 0x28A18, // SAME 
    union UINT_FLOAT VGT_HOS_MIN_TESS_LEVEL;                     // = 0x28A1C, // SAME 
    union UINT_FLOAT VGT_HOS_REUSE_DEPTH;                        // = 0x28A20, // SAME 
    union UINT_FLOAT VGT_GROUP_PRIM_TYPE;                        // = 0x28A24, // SAME 
    union UINT_FLOAT VGT_GROUP_FIRST_DECR;                       // = 0x28A28, // SAME 
    union UINT_FLOAT VGT_GROUP_DECR;                             // = 0x28A2C, // SAME 
    union UINT_FLOAT VGT_GROUP_VECT_0_CNTL;                      // = 0x28A30, // SAME 
    union UINT_FLOAT VGT_GROUP_VECT_1_CNTL;                      // = 0x28A34, // SAME 
    union UINT_FLOAT VGT_GROUP_VECT_0_FMT_CNTL;                  // = 0x28A38, // SAME 
    union UINT_FLOAT VGT_GROUP_VECT_1_FMT_CNTL;                  // = 0x28A3C, // SAME 
    union UINT_FLOAT VGT_GS_MODE; ////                               // = 0x28A40, // DIFF 
       
    union UINT_FLOAT VGT_PRIMITIVEID_EN;   ////                  // = 0x28A84, // SAME 
    union UINT_FLOAT VGT_DMA_NUM_INSTANCES;  ////                // = 0x28A88, // SAME 
    union UINT_FLOAT VGT_EVENT_INITIATOR;                        // = 0x28A90, // SAME 
    union UINT_FLOAT VGT_MULTI_PRIM_IB_RESET_EN;                 // = 0x28A94, // SAME 
    union UINT_FLOAT VGT_INSTANCE_STEP_RATE_0; ////                  // = 0x28AA0, // SAME 
    union UINT_FLOAT VGT_INSTANCE_STEP_RATE_1; ////                  // = 0x28AA4, // SAME 
    union UINT_FLOAT VGT_REUSE_OFF; ////                             // = 0x28AB4, // SAME 
    union UINT_FLOAT VGT_VTX_CNT_EN; ////                            // = 0x28AB8, // SAME 
    
    union UINT_FLOAT VGT_SHADER_STAGES_EN;  ////                     // = 0x28B54, //
 
    union UINT_FLOAT VGT_STRMOUT_CONFIG; ////                        // = 0x28B94, // 
    union UINT_FLOAT VGT_STRMOUT_BUFFER_CONFIG;  ////                // = 0x28B98, // 
    union UINT_FLOAT VGT_VERTEX_REUSE_BLOCK_CNTL;////                // = 0x28C58, // SAME 
    union UINT_FLOAT VGT_OUT_DEALLOC_CNTL;  ////                     // = 0x28C5C, // SAME 

/* Registers from SQ block: */     
    union UINT_FLOAT SQ_VTX_SEMANTIC_0;    ////                      // = 0x28380, // SAME
    union UINT_FLOAT SQ_VTX_SEMANTIC_1;    ////                      // = 0x28384, // SAME
    union UINT_FLOAT SQ_VTX_SEMANTIC_2;    ////                      // = 0x28388, // SAME
    union UINT_FLOAT SQ_VTX_SEMANTIC_3;    ////                      // = 0x2838C, // SAME
    union UINT_FLOAT SQ_VTX_SEMANTIC_4;    ////                      // = 0x28390, // SAME
    union UINT_FLOAT SQ_VTX_SEMANTIC_5;    ////                      // = 0x28394, // SAME
    union UINT_FLOAT SQ_VTX_SEMANTIC_6;    ////                      // = 0x28398, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_7;    ////                      // = 0x2839C, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_8;    ////                      // = 0x283A0, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_9;    ////                      // = 0x283A4, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_10;   ////                      // = 0x283A8, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_11;   ////                      // = 0x283AC, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_12;   ////                      // = 0x283B0, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_13;   ////                      // = 0x283B4, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_14;   ////                      // = 0x283B8, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_15;   ////                      // = 0x283BC, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_16;   ////                      // = 0x283C0, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_17;   ////                      // = 0x283C4, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_18;   ////                      // = 0x283C8, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_19;   ////                      // = 0x283CC, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_20;   ////                      // = 0x283D0, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_21;   ////                      // = 0x283D4, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_22;   ////                      // = 0x283D8, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_23;   ////                      // = 0x283DC, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_24;   ////                      // = 0x283E0, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_25;   ////                      // = 0x283E4, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_26;   ////                      // = 0x283E8, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_27;   ////                      // = 0x283EC, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_28;   ////                      // = 0x283F0, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_29;   ////                      // = 0x283F4, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_30;   ////                      // = 0x283F8, // SAME 
    union UINT_FLOAT SQ_VTX_SEMANTIC_31;   ////                      // = 0x283FC, // SAME     
    union UINT_FLOAT SQ_DYN_GPR_RESOURCE_LIMIT_1;////                // = 0x28838, // 
    
    union UINT_FLOAT SQ_PGM_RESOURCES_PS;  ////                      // = 0x28844, // DIFF 0x28850 
    union UINT_FLOAT SQ_PGM_RESOURCES_2_PS; ////                     // = 0x28848, // 
    union UINT_FLOAT SQ_PGM_EXPORTS_PS; ////                         // = 0x2884C, // SAME 0x28854 
    
    union UINT_FLOAT SQ_PGM_RESOURCES_VS;////                        // = 0x28860, // DIFF 0x28868 
    union UINT_FLOAT SQ_PGM_RESOURCES_2_VS;  ////                    // = 0x28864, // 
    union UINT_FLOAT SQ_PGM_START_GS; ////                           // = 0x28874, // SAME 0x2886C 
    union UINT_FLOAT SQ_PGM_RESOURCES_GS; ////                       // = 0x28878, // DIFF 0x2887C 
    union UINT_FLOAT SQ_PGM_RESOURCES_2_GS; ////                     // = 0x2887C, // 
    union UINT_FLOAT SQ_PGM_START_ES;  ////                          // = 0x2888C, // SAME 0x28880 
    union UINT_FLOAT SQ_PGM_RESOURCES_ES; ////                       // = 0x28890, // DIFF 
    union UINT_FLOAT SQ_PGM_RESOURCES_2_ES; ////                     // = 0x28894, // 
    union UINT_FLOAT SQ_PGM_START_FS; ////                           // = 0x288A4, // SAME 0x28894 
    union UINT_FLOAT SQ_PGM_RESOURCES_FS;  ////                      // = 0x288A8, // DIFF 0x288A4 
    union UINT_FLOAT SQ_PGM_START_HS;                            // = 0x288B8, // 
    union UINT_FLOAT SQ_PGM_RESOURCES_HS;                        // = 0x288BC, // 
    union UINT_FLOAT SQ_PGM_RESOURCES_2_HS;////                      // = 0x288C0, // 
    union UINT_FLOAT SQ_PGM_START_LS;                            // = 0x288D0, // 
    union UINT_FLOAT SQ_PGM_RESOURCES_LS;                        // = 0x288D4, // 
    union UINT_FLOAT SQ_PGM_RESOURCES_2_LS;  ////                    // = 0x288D8, //         
    union UINT_FLOAT SQ_LDS_ALLOC_PS; ////                           // = 0x288EC, //         
    union UINT_FLOAT SQ_ESGS_RING_ITEMSIZE; ////                     // = 0x28900, // SAME 0x288A8 
    union UINT_FLOAT SQ_GSVS_RING_ITEMSIZE; ////                     // = 0x28904, // SAME 0x288AC 
    union UINT_FLOAT SQ_ESTMP_RING_ITEMSIZE;  ////                   // = 0x28908, // SAME 0x288B0 
    union UINT_FLOAT SQ_GSTMP_RING_ITEMSIZE;  ////                   // = 0x2890C, // SAME 0x288B4 
    union UINT_FLOAT SQ_VSTMP_RING_ITEMSIZE;  ////                   // = 0x28910, // SAME 0x288B8 
    union UINT_FLOAT SQ_PSTMP_RING_ITEMSIZE;  ////                   // = 0x28914, // SAME 0x288BC 
    union UINT_FLOAT SQ_GS_VERT_ITEMSIZE;     ////                   // = 0x2891C, // SAME 0x288C8 
    union UINT_FLOAT SQ_GS_VERT_ITEMSIZE_1;                      // = 0x28920, // 
    union UINT_FLOAT SQ_GS_VERT_ITEMSIZE_2;                      // = 0x28924, // 
    union UINT_FLOAT SQ_GS_VERT_ITEMSIZE_3;                      // = 0x28928, //             
     
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_0;                    // = 0x289C0, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_1;                    // = 0x289C4, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_2;                    // = 0x289C8, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_3;                    // = 0x289CC, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_4;                    // = 0x289D0, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_5;                    // = 0x289D4, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_6;                    // = 0x289D8, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_7;                    // = 0x289DC, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_8;                    // = 0x289E0, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_9;                    // = 0x289E4, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_10;                   // = 0x289E8, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_11;                   // = 0x289EC, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_12;                   // = 0x289F0, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_13;                   // = 0x289F4, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_14;                   // = 0x289F8, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_GS_15;                   // = 0x289FC, // SAME 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_0;                    // = 0x28F00, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_1;                    // = 0x28F04, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_2;                    // = 0x28F08, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_3;                    // = 0x28F0C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_4;                    // = 0x28F10, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_5;                    // = 0x28F14, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_6;                    // = 0x28F18, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_7;                    // = 0x28F1C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_8;                    // = 0x28F20, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_9;                    // = 0x28F24, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_10;                   // = 0x28F28, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_11;                   // = 0x28F2C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_12;                   // = 0x28F30, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_13;                   // = 0x28F34, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_14;                   // = 0x28F38, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_HS_15;                   // = 0x28F3C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_0;                    // = 0x28F40, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_1;                    // = 0x28F44, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_2;                    // = 0x28F48, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_3;                    // = 0x28F4C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_4;                    // = 0x28F50, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_5;                    // = 0x28F54, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_6;                    // = 0x28F58, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_7;                    // = 0x28F5C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_8;                    // = 0x28F60, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_9;                    // = 0x28F64, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_10;                   // = 0x28F68, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_11;                   // = 0x28F6C, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_12;                   // = 0x28F70, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_13;                   // = 0x28F74, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_14;                   // = 0x28F78, // 
    union UINT_FLOAT SQ_ALU_CONST_CACHE_LS_15;                   // = 0x28F7C, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_0;              // = 0x28F80, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_1;              // = 0x28F84, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_2;              // = 0x28F88, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_3;              // = 0x28F8C, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_4;              // = 0x28F90, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_5;              // = 0x28F94, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_6;              // = 0x28F98, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_7;              // = 0x28F9C, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_8;              // = 0x28FA0, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_9;              // = 0x28FA4, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_10;             // = 0x28FA8, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_11;             // = 0x28FAC, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_12;             // = 0x28FB0, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_13;             // = 0x28FB4, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_14;             // = 0x28FB8, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_HS_15;             // = 0x28FBC, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_0;              // = 0x28FC0, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_1;              // = 0x28FC4, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_2;              // = 0x28FC8, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_3;              // = 0x28FCC, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_4;              // = 0x28FD0, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_5;              // = 0x28FD4, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_6;              // = 0x28FD8, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_7;              // = 0x28FDC, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_8;              // = 0x28FE0, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_9;              // = 0x28FE4, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_10;             // = 0x28FE8, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_11;             // = 0x28FEC, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_12;             // = 0x28FF0, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_13;             // = 0x28FF4, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_14;             // = 0x28FF8, // 
    union UINT_FLOAT SQ_ALU_CONST_BUFFER_SIZE_LS_15;             // = 0x28FFC, // 

    EVERGREEN_PS_RES ps;
    EVERGREEN_VS_RES vs;

/* Registers from SPI block: */
    union UINT_FLOAT SPI_VS_OUT_ID_0;     ////                   // = 0x2861C, // SAME 0x28614 
    union UINT_FLOAT SPI_VS_OUT_ID_1;     ////                   // = 0x28620, // SAME 0x28618 
    union UINT_FLOAT SPI_VS_OUT_ID_2;     ////                   // = 0x28624, // SAME 0x2861C 
    union UINT_FLOAT SPI_VS_OUT_ID_3;     ////                   // = 0x28628, // SAME 0x28620 
    union UINT_FLOAT SPI_VS_OUT_ID_4;     ////                   // = 0x2862C, // SAME 0x28624 
    union UINT_FLOAT SPI_VS_OUT_ID_5;     ////                   // = 0x28630, // SAME 0x28628 
    union UINT_FLOAT SPI_VS_OUT_ID_6;     ////                   // = 0x28634, // SAME 0x2862C 
    union UINT_FLOAT SPI_VS_OUT_ID_7;     ////                   // = 0x28638, // SAME 0x28630 
    union UINT_FLOAT SPI_VS_OUT_ID_8;     ////                   // = 0x2863C, // SAME 0x28634 
    union UINT_FLOAT SPI_VS_OUT_ID_9;     ////                   // = 0x28640, // SAME 0x28638 
    union UINT_FLOAT SPI_PS_INPUT_CNTL[32];     ////                   // = 0x28644, // SAME 
    
    union UINT_FLOAT SPI_VS_OUT_CONFIG;  ////                        // = 0x286C4, // SAME 
    union UINT_FLOAT SPI_THREAD_GROUPING; ////                       // = 0x286C8, // DIFF 
    union UINT_FLOAT SPI_PS_IN_CONTROL_0; ////                       // = 0x286CC, // SAME 
    union UINT_FLOAT SPI_PS_IN_CONTROL_1; ////                       // = 0x286D0, // SAME 
    union UINT_FLOAT SPI_INTERP_CONTROL_0;  ////                     // = 0x286D4, // SAME 
    union UINT_FLOAT SPI_INPUT_Z;           ////                     // = 0x286D8, // SAME 
    union UINT_FLOAT SPI_FOG_CNTL;          ////                     // = 0x286DC, // SAME 
    union UINT_FLOAT SPI_BARYC_CNTL;  ////                           // = 0x286E0, // 
    union UINT_FLOAT SPI_PS_IN_CONTROL_2; ////                       // = 0x286E4, // 
    union UINT_FLOAT SPI_COMPUTE_INPUT_CNTL;                     // = 0x286E8, // 
    union UINT_FLOAT SPI_COMPUTE_NUM_THREAD_X;                   // = 0x286EC, // 
    union UINT_FLOAT SPI_COMPUTE_NUM_THREAD_Y;                   // = 0x286F0, // 
    union UINT_FLOAT SPI_COMPUTE_NUM_THREAD_Z;                   // = 0x286F4, // 

/* Registers from SX block: */
    union UINT_FLOAT SX_MISC;                                    // = 0x28350, // SAME 
    union UINT_FLOAT SX_SURFACE_SYNC;                            // = 0x28354, // DIFF 
    union UINT_FLOAT SX_ALPHA_TEST_CONTROL;  ////                // = 0x28410, // SAME 
    union UINT_FLOAT SX_ALPHA_REF;                               // = 0x28438, // SAME 

/* Registers from DB block: */
    union UINT_FLOAT DB_RENDER_CONTROL; ////                         // = 0x28000, // DIFF 0x28D0C 
    union UINT_FLOAT DB_COUNT_CONTROL; ////                          // = 0x28004, // 
    union UINT_FLOAT DB_DEPTH_VIEW;  ////                            // = 0x28008, // DIFF 0x28004 
    union UINT_FLOAT DB_RENDER_OVERRIDE;   ////                      // = 0x2800C, // DIFF 0x28D10 
    union UINT_FLOAT DB_RENDER_OVERRIDE2;  ////                      // = 0x28010, // 
    union UINT_FLOAT DB_HTILE_DATA_BASE;   ////                      // = 0x28014, // SAME 
    union UINT_FLOAT DB_STENCIL_CLEAR; ////                          // = 0x28028, // SAME 
    union UINT_FLOAT DB_DEPTH_CLEAR; ////                            // = 0x2802C, // SAME 
    union UINT_FLOAT DB_Z_INFO;   ////                               // = 0x28040, // 
    union UINT_FLOAT DB_STENCIL_INFO;  ////                          // = 0x28044, // 
    union UINT_FLOAT DB_Z_READ_BASE; ////                            // = 0x28048, // 
    union UINT_FLOAT DB_STENCIL_READ_BASE;////                       // = 0x2804C, // 
    union UINT_FLOAT DB_Z_WRITE_BASE;  ////                          // = 0x28050, // 
    union UINT_FLOAT DB_STENCIL_WRITE_BASE; ////                     // = 0x28054, // 
    union UINT_FLOAT DB_DEPTH_SIZE;   ////                           // = 0x28058, // DIFF 0x28000 
    union UINT_FLOAT DB_DEPTH_SLICE; ////                            // = 0x2805C, // 
    union UINT_FLOAT DB_STENCILREFMASK;                          // = 0x28430, // SAME 
    union UINT_FLOAT DB_STENCILREFMASK_BF;                       // = 0x28434, // SAME 
    union UINT_FLOAT DB_DEPTH_CONTROL;      ////                     // = 0x28800, // SAME 
    union UINT_FLOAT DB_SHADER_CONTROL;////                          // = 0x2880C, // DIFF 
    union UINT_FLOAT DB_HTILE_SURFACE;  ////                         // = 0x28ABC, // SAME 0x28D24 
    union UINT_FLOAT DB_SRESULTS_COMPARE_STATE0; ////                // = 0x28AC0, // SAME 0x28D28 
    union UINT_FLOAT DB_SRESULTS_COMPARE_STATE1; ////                // = 0x28AC4, // SAME 0x28D2C 
    union UINT_FLOAT DB_PRELOAD_CONTROL; ////                        // = 0x28AC8, // SAME 0x28D30 
    union UINT_FLOAT DB_ALPHA_TO_MASK; ////                          // = 0x28B70, // SAME 0x28D44 

/* Registers from CB block: */
    union UINT_FLOAT CB_TARGET_MASK;  ////                       // = 0x28238, // SAME 
    union UINT_FLOAT CB_SHADER_MASK;  ////                       // = 0x2823C, // SAME 
    union UINT_FLOAT CB_BLEND_RED;   ////                            // = 0x28414, // SAME 
    union UINT_FLOAT CB_BLEND_GREEN; ////                            // = 0x28418, // SAME 
    union UINT_FLOAT CB_BLEND_BLUE;  ////                            // = 0x2841C, // SAME 
    union UINT_FLOAT CB_BLEND_ALPHA;  ////                           // = 0x28420, // SAME 
    union UINT_FLOAT CB_BLEND0_CONTROL; ////                         // = 0x28780, // DIFF 
    union UINT_FLOAT CB_BLEND1_CONTROL;                          // = 0x28784, // DIFF 
    union UINT_FLOAT CB_BLEND2_CONTROL;                          // = 0x28788, // DIFF 
    union UINT_FLOAT CB_BLEND3_CONTROL;                          // = 0x2878C, // DIFF 
    union UINT_FLOAT CB_BLEND4_CONTROL;                          // = 0x28790, // DIFF 
    union UINT_FLOAT CB_BLEND5_CONTROL;                          // = 0x28794, // DIFF 
    union UINT_FLOAT CB_BLEND6_CONTROL;                          // = 0x28798, // DIFF 
    union UINT_FLOAT CB_BLEND7_CONTROL;                          // = 0x2879C, // DIFF 
    union UINT_FLOAT CB_COLOR_CONTROL; ////                          // = 0x28808, // DIFF      
    union UINT_FLOAT CB_CLRCMP_CONTROL; ////                         // = 0x28C40, // SAME 0x28C30 
    union UINT_FLOAT CB_CLRCMP_SRC;  ////                            // = 0x28C44, // SAME 0x28C34 
    union UINT_FLOAT CB_CLRCMP_DST;  ////                            // = 0x28C48, // SAME 0x28C38 
    union UINT_FLOAT CB_CLRCMP_MSK;  ////                            // = 0x28C4C, // SAME 0x28C3C 
    
    EVERGREEN_RENDER_TARGET_STATE      render_target[EVERGREEN_MAX_RENDER_TARGETS];  
    
    radeonTexObj*    textures[R700_TEXTURE_NUMBERUNITS];

    EVERGREEN_CONFIG evergreen_config;

    GLboolean           bEnablePerspective;

} EVERGREEN_CHIP_CONTEXT;

#endif /* _EVERGREEN_CHIP_H_ */