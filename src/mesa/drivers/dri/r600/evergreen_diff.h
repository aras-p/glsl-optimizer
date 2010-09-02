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

#ifndef _EVERGREEN_DIFF_H_
#define _EVERGREEN_DIFF_H_

enum {
    /* CB_BLEND_CONTROL */
        EG_CB_BLENDX_CONTROL_ENABLE_bit = 1 << 30,
    /* PA_SC_SCREEN_SCISSOR_TL */
        EG_PA_SC_SCREEN_SCISSOR_TL__TL_X_mask                = 0xffff << 0,	
	    EG_PA_SC_SCREEN_SCISSOR_TL__TL_Y_mask                = 0xffff << 16,
    /* PA_SC_SCREEN_SCISSOR_BR */
	    EG_PA_SC_SCREEN_SCISSOR_BR__BR_X_mask                = 0xffff << 0,	
	    EG_PA_SC_SCREEN_SCISSOR_BR__BR_Y_mask                = 0xffff << 16,
    /* PA_SC_WINDOW_SCISSOR_TL */ 
	    EG_PA_SC_WINDOW_SCISSOR_TL__TL_X_mask                = 0x7fff << 0,	
	    EG_PA_SC_WINDOW_SCISSOR_TL__TL_Y_mask                = 0x7fff << 16,	
    /* PA_SC_WINDOW_SCISSOR_BR */
	    EG_PA_SC_WINDOW_SCISSOR_BR__BR_X_mask                = 0x7fff << 0,	
	    EG_PA_SC_WINDOW_SCISSOR_BR__BR_Y_mask                = 0x7fff << 16,
    /* PA_SC_CLIPRECT_0_TL */
        EG_PA_SC_CLIPRECT_0_TL__TL_X_mask                    = 0x7fff << 0,	
	    EG_PA_SC_CLIPRECT_0_TL__TL_Y_mask                    = 0x7fff << 16,	
    /* PA_SC_CLIPRECT_0_BR */	
	    EG_PA_SC_CLIPRECT_0_BR__BR_X_mask                    = 0x7fff << 0,	
	    EG_PA_SC_CLIPRECT_0_BR__BR_Y_mask                    = 0x7fff << 16,
    /* PA_SC_GENERIC_SCISSOR_TL */
	    EG_PA_SC_GENERIC_SCISSOR_TL__TL_X_mask               = 0x7fff << 0,	
	    EG_PA_SC_GENERIC_SCISSOR_TL__TL_Y_mask               = 0x7fff << 16,	
    /* PA_SC_GENERIC_SCISSOR_BR */
	    EG_PA_SC_GENERIC_SCISSOR_BR__BR_X_mask               = 0x7fff << 0,	
	    EG_PA_SC_GENERIC_SCISSOR_BR__BR_Y_mask               = 0x7fff << 16,
    /* PA_SC_VPORT_SCISSOR_0_TL */	
	    EG_PA_SC_VPORT_SCISSOR_0_TL__TL_X_mask               = 0x7fff << 0,	
	    EG_PA_SC_VPORT_SCISSOR_0_TL__TL_Y_mask               = 0x7fff << 16,	
    /* PA_SC_VPORT_SCISSOR_0_BR */		
	    EG_PA_SC_VPORT_SCISSOR_0_BR__BR_X_mask               = 0x7fff << 0,	
	    EG_PA_SC_VPORT_SCISSOR_0_BR__BR_Y_mask               = 0x7fff << 16,
    /* PA_SC_WINDOW_OFFSET */
        EG_PA_SC_WINDOW_OFFSET__WINDOW_X_OFFSET_shift        = 0,
        EG_PA_SC_WINDOW_OFFSET__WINDOW_X_OFFSET_mask         = 0xffff << 0,
        EG_PA_SC_WINDOW_OFFSET__WINDOW_Y_OFFSET_shift        = 16,
        EG_PA_SC_WINDOW_OFFSET__WINDOW_Y_OFFSET_mask         = 0xffff << 16,
    /* SPI_BARYC_CNTL */
        EG_SPI_BARYC_CNTL__PERSP_CENTROID_ENA_shift          = 4,
        EG_SPI_BARYC_CNTL__PERSP_CENTROID_ENA_mask           = 0x3    << 4,
        EG_SPI_BARYC_CNTL__LINEAR_CENTROID_ENA_shift         = 20,
        EG_SPI_BARYC_CNTL__LINEAR_CENTROID_ENA_mask          = 0x3    << 20,
    /* DB_SHADER_CONTROL */
        EG_DB_SHADER_CONTROL__DUAL_EXPORT_ENABLE_bit         = 1 << 9,

    /* DB_Z_INFO */
        EG_DB_Z_INFO__FORMAT_shift                           = 0,        //2;
        EG_DB_Z_INFO__FORMAT_mask                            = 0x3,
		                                                                 //2;
		EG_DB_Z_INFO__ARRAY_MODE_shift                       = 4,        //4;
        EG_DB_Z_INFO__ARRAY_MODE_mask                        = 0xf << 4,
		EG_DB_Z_INFO__TILE_SPLIT_shift                       = 8,        //3;
        EG_DB_Z_INFO__TILE_SPLIT_mask                        = 0x7 << 8,
		                                                                 //1;
		EG_DB_Z_INFO__NUM_BANKS_shift                        = 12,       //2;
        EG_DB_Z_INFO__NUM_BANKS_mask                         = 0x3 << 12,
		                                                                 //2;
		EG_DB_Z_INFO__BANK_WIDTH_shift                       = 16,       //2;
        EG_DB_Z_INFO__BANK_WIDTH_mask                        = 0x3 << 16,
		                                                                 //2;
		EG_DB_Z_INFO__BANK_HEIGHT_shift                      = 20,       //2;
        EG_DB_Z_INFO__BANK_HEIGHT_mask                       = 0x3 << 20,
        
        EG_Z_INVALID                                         = 0x00000000,
        EG_Z_16                                              = 0x00000001,
        EG_Z_24                                              = 0x00000002,
        EG_Z_32_FLOAT                                        = 0x00000003,
        EG_ADDR_SURF_TILE_SPLIT_256B                         = 0x00000002,
        EG_ADDR_SURF_8_BANK                                  = 0x00000002,
        EG_ADDR_SURF_BANK_WIDTH_1                            = 0x00000000,
        EG_ADDR_SURF_BANK_HEIGHT_1                           = 0x00000000,
    /* DB_STENCIL_INFO */
        EG_DB_STENCIL_INFO__FORMAT_bit                       = 1,   //1;
		                                                            //7;
		EG_DB_STENCIL_INFO__TILE_SPLIT_shift                 = 8,   //3;
        EG_DB_STENCIL_INFO__TILE_SPLIT_mask                  = 0x7 << 8,
    
    /* DB_DEPTH_SIZE */
        EG_DB_DEPTH_SIZE__PITCH_TILE_MAX_shift               = 0,  // 11;
        EG_DB_DEPTH_SIZE__PITCH_TILE_MAX_mask                = 0x7ff,
		EG_DB_DEPTH_SIZE__HEIGHT_TILE_MAX_shift              = 11, // 11;
        EG_DB_DEPTH_SIZE__HEIGHT_TILE_MAX_mask               = 0x7ff << 11,

    /* DB_COUNT_CONTROL */
        EG_DB_COUNT_CONTROL__ZPASS_INCREMENT_DISABLE_shift   = 0,  //1
        EG_DB_COUNT_CONTROL__ZPASS_INCREMENT_DISABLE_bit     = 1,
        EG_DB_COUNT_CONTROL__PERFECT_ZPASS_COUNTS_shift      = 1,  //1
        EG_DB_COUNT_CONTROL__PERFECT_ZPASS_COUNTS_bit        = 1 << 1,    

    /* CB_COLOR_CONTROL */
                                                                      //3;
		EG_CB_COLOR_CONTROL__DEGAMMA_ENABLE_bit              = 1 << 3,//1;
		EG_CB_COLOR_CONTROL__MODE_shift                      = 4,     //3;
        EG_CB_COLOR_CONTROL__MODE_mask                       = 0x7 << 4,
		                                                              //9;
		EG_CB_COLOR_CONTROL__ROP3_shift                      = 16,    //8;
        EG_CB_COLOR_CONTROL__ROP3_mask                       = 0xff << 16,
        EG_CB_NORMAL                                         = 0x00000001,

    /* CB_COLOR0_INFO */
        EG_CB_COLOR0_INFO__ENDIAN_shift                      = 0,      //2;
        EG_CB_COLOR0_INFO__ENDIAN_mask                       = 0x3,
		EG_CB_COLOR0_INFO__FORMAT_shift                      = 2,      //6;
        EG_CB_COLOR0_INFO__FORMAT_mask                       = 0x3f << 2,
		EG_CB_COLOR0_INFO__ARRAY_MODE_shift                  = 8,      //4;
        EG_CB_COLOR0_INFO__ARRAY_MODE_mask                   = 0xf << 8,
		EG_CB_COLOR0_INFO__NUMBER_TYPE_shift                 = 12,     //3;
        EG_CB_COLOR0_INFO__NUMBER_TYPE_mask                  = 0x7 << 12,
		EG_CB_COLOR0_INFO__COMP_SWAP_shift                   = 15,     //2;
        EG_CB_COLOR0_INFO__COMP_SWAP_mask                    = 0x3 << 15,
		EG_CB_COLOR0_INFO__FAST_CLEAR_bit                    = 1 << 17,//1;
		EG_CB_COLOR0_INFO__COMPRESSION_bit                   = 1 << 18,//1;
		EG_CB_COLOR0_INFO__BLEND_CLAMP_bit                   = 1 << 19,//1;
		EG_CB_COLOR0_INFO__BLEND_BYPASS_bit                  = 1 << 20,//1;
		EG_CB_COLOR0_INFO__SIMPLE_FLOAT_bit                  = 1 << 21,//1;
		EG_CB_COLOR0_INFO__ROUND_MODE_bit                    = 1 << 22,//1;
		EG_CB_COLOR0_INFO__TILE_COMPACT_bit                  = 1 << 23,//1;
		EG_CB_COLOR0_INFO__SOURCE_FORMAT_shift               = 24,     //2;
        EG_CB_COLOR0_INFO__SOURCE_FORMAT_mask                = 0x3 << 24,
		EG_CB_COLOR0_INFO__RAT_bit                           = 1 << 26,//1;
		EG_CB_COLOR0_INFO__RESOURCE_TYPE_shift               = 27,     //3;
        EG_CB_COLOR0_INFO__RESOURCE_TYPE_mask                = 0x7 << 27,

    /* CB_COLOR0_ATTRIB */
        EG_CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_shift     = 4,
        EG_CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_bit       = 1 << 4,

    /* SPI_CONFIG_CNTL_1 */
        EG_SPI_CONFIG_CNTL_1__VTX_DONE_DELAY_shift           = 0,
        EG_SPI_CONFIG_CNTL_1__VTX_DONE_DELAY_mask            = 0xf,
    /* SQ_MS_FIFO_SIZES */
        EG_SQ_MS_FIFO_SIZES__CACHE_FIFO_SIZE_shift           = 0,
        EG_SQ_MS_FIFO_SIZES__CACHE_FIFO_SIZE_mask            = 0xff,
        EG_SQ_MS_FIFO_SIZES__FETCH_FIFO_HIWATER_shift        = 8,
        EG_SQ_MS_FIFO_SIZES__FETCH_FIFO_HIWATER_mask         = 0x1f << 8,
        EG_SQ_MS_FIFO_SIZES__DONE_FIFO_HIWATER_shift         = 16,
        EG_SQ_MS_FIFO_SIZES__DONE_FIFO_HIWATER_mask          = 0xff << 16,
        EG_SQ_MS_FIFO_SIZES__ALU_UPDATE_FIFO_HIWATER_shift   = 24,
        EG_SQ_MS_FIFO_SIZES__ALU_UPDATE_FIFO_HIWATER_mask    = 0x1f << 24,
    /* SQ_CONFIG */
        EG_SQ_CONFIG__VC_ENABLE_bit                          = 1,
        EG_SQ_CONFIG__EXPORT_SRC_C_bit                       = 1 << 1,
        EG_SQ_CONFIG__PS_PRIO_shift                          = 24,
        EG_SQ_CONFIG__PS_PRIO_mask                           = 0x3 << 24,
        EG_SQ_CONFIG__VS_PRIO_shift                          = 26,
        EG_SQ_CONFIG__VS_PRIO_mask                           = 0x3 << 26,
        EG_SQ_CONFIG__GS_PRIO_shift                          = 28,
        EG_SQ_CONFIG__GS_PRIO_mask                           = 0x3 << 28,
        EG_SQ_CONFIG__ES_PRIO_shift                          = 30,
        EG_SQ_CONFIG__ES_PRIO_mask                           = 0x3 << 30,
    /* PA_SC_FORCE_EOV_MAX_CNTS */
        EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_CLK_CNT_shift = 0,
        EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_CLK_CNT_mask  = 0x3fff,
        EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_REZ_CNT_shift = 16,
        EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_REZ_CNT_mask  = 0x3fff << 16,
    /* VGT_CACHE_INVALIDATION */
        EG_VGT_CACHE_INVALIDATION__CACHE_INVALIDATION_shift      = 0,
        EG_VGT_CACHE_INVALIDATION__CACHE_INVALIDATION_mask       = 0x3, 
    /* CB_COLOR0_PITCH */
        EG_CB_COLOR0_PITCH__TILE_MAX_shift                       = 0,
        EG_CB_COLOR0_PITCH__TILE_MAX_mask                        = 0x7ff,
    /* CB_COLOR0_SLICE */
        EG_CB_COLOR0_SLICE__TILE_MAX_shift                       = 0,
        EG_CB_COLOR0_SLICE__TILE_MAX_mask                        = 0x3fffff,    
    /* SQ_VTX_CONSTANT_WORD3_0 */
        EG_SQ_VTX_CONSTANT_WORD3_0__UNCACHED_shift  = 2,
        EG_SQ_VTX_CONSTANT_WORD3_0__UNCACHED_bit    = 1 << 2,

        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_shift = 3, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_mask  = 0x7 << 3,
     
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_shift = 6, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_mask  = 0x7 << 6,
     
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_shift = 9, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_mask  = 0x7 << 9,
     
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_shift = 12, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_mask  = 0x7 << 12,
    /* SQ_VTX_CONSTANT_WORD4_0 */
        EG_SQ_VTX_CONSTANT_WORD4_0__NUM_ELEMENTS_shift = 0,
        EG_SQ_VTX_CONSTANT_WORD4_0__NUM_ELEMENTS_mask  = 0xFFFFFFFF,
    /* SQ_VTX_CONSTANT_WORD7_0 */
        EG_SQ_VTX_CONSTANT_WORD7_0__TYPE_shift         = 30,
        EG_SQ_VTX_CONSTANT_WORD7_0__TYPE_mask          = 0x3 << 30,
    /* SQ_TEX_SAMPLER_WORD0_0 */
        EG_SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_shift         = 0,  // 3;
        EG_SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_mask          = 0x7,
		EG_SQ_TEX_SAMPLER_WORD0_0__CLAMP_Y_shift         = 3,  // 3;
        EG_SQ_TEX_SAMPLER_WORD0_0__CLAMP_Y_mask          = 0x7 << 3,
		EG_SQ_TEX_SAMPLER_WORD0_0__CLAMP_Z_shift         = 6,  // 3;
        EG_SQ_TEX_SAMPLER_WORD0_0__CLAMP_Z_mask          = 0x7 << 6,
		EG_SQ_TEX_SAMPLER_WORD0_0__XY_MAG_FILTER_shift   = 9,  // 2;
        EG_SQ_TEX_SAMPLER_WORD0_0__XY_MAG_FILTER_mask    = 0x3 << 9,
		EG_SQ_TEX_SAMPLER_WORD0_0__XY_MIN_FILTER_shift   = 11, // 2;
        EG_SQ_TEX_SAMPLER_WORD0_0__XY_MIN_FILTER_mask    = 0x3 << 11,
		EG_SQ_TEX_SAMPLER_WORD0_0__Z_FILTER_shift        = 13, // 2;
        EG_SQ_TEX_SAMPLER_WORD0_0__Z_FILTER_mask         = 0x3 << 13,
		EG_SQ_TEX_SAMPLER_WORD0_0__MIP_FILTER_shift      = 15, // 2;
        EG_SQ_TEX_SAMPLER_WORD0_0__MIP_FILTER_mask       = 0x3 << 15,
		EG_SQ_TEX_SAMPLER_WORD0_0__MAX_ANISO_RATIO_shift = 17, // 3;
        EG_SQ_TEX_SAMPLER_WORD0_0__MAX_ANISO_RATIO_mask  = 0x7 << 17,
		EG_SQ_TEX_SAMPLER_WORD0_0__BORDER_COLOR_TYPE_shift = 20,//2;
        EG_SQ_TEX_SAMPLER_WORD0_0__BORDER_COLOR_TYPE_mask  = 0x3 << 20,
		EG_SQ_TEX_SAMPLER_WORD0_0__DCF_shift             = 22, // 3;
        EG_SQ_TEX_SAMPLER_WORD0_0__DCF_mask              = 0x7 << 22,
		EG_SQ_TEX_SAMPLER_WORD0_0__CHROMA_KEY_shift      = 25, // 2;
        EG_SQ_TEX_SAMPLER_WORD0_0__CHROMA_KEY_mask       = 0x3 << 25,
		EG_SQ_TEX_SAMPLER_WORD0_0__ANISO_THRESHOLD_shift = 27, // 3;
        EG_SQ_TEX_SAMPLER_WORD0_0__ANISO_THRESHOLD_mask  = 0x7 << 27,
		EG_SQ_TEX_SAMPLER_WORD0_0__Reserved_shift        = 30, // 2         
        EG_SQ_TEX_SAMPLER_WORD0_0__Reserved_mask         = 0x3 << 30,
    /* SQ_TEX_SAMPLER_WORD1_0 */
        EG_SQ_TEX_SAMPLER_WORD1_0__MIN_LOD_shift         = 0, // 12;
        EG_SQ_TEX_SAMPLER_WORD1_0__MIN_LOD_mask          = 0xfff,
		EG_SQ_TEX_SAMPLER_WORD1_0__MAX_LOD_shift         = 12,// 12;
        EG_SQ_TEX_SAMPLER_WORD1_0__MAX_LOD_mask          = 0xfff << 12,
    /* SQ_TEX_SAMPLER_WORD2_0 */
        EG_SQ_TEX_SAMPLER_WORD2_0__LOD_BIAS_shift          = 0, //14;
        EG_SQ_TEX_SAMPLER_WORD2_0__LOD_BIAS_mask           = 0x3fff,
		EG_SQ_TEX_SAMPLER_WORD2_0__LOD_BIAS_SEC_shift      = 14,//6;
        EG_SQ_TEX_SAMPLER_WORD2_0__LOD_BIAS_SEC_mask       = 0x3f << 14,
		EG_SQ_TEX_SAMPLER_WORD2_0__MC_COORD_TRUNCATE_shift = 20,//1;
        EG_SQ_TEX_SAMPLER_WORD2_0__MC_COORD_TRUNCATE_bit   = 1 << 20,
		EG_SQ_TEX_SAMPLER_WORD2_0__FORCE_DEGAMMA_shift     = 21,//1;
        EG_SQ_TEX_SAMPLER_WORD2_0__FORCE_DEGAMMA_bit       = 1 << 21,
		EG_SQ_TEX_SAMPLER_WORD2_0__ANISO_BIAS_shift        = 22,//6;
        EG_SQ_TEX_SAMPLER_WORD2_0__ANISO_BIAS_mask         = 0x3f << 22,
		EG_SQ_TEX_SAMPLER_WORD2_0__TRUNCATE_COORD_shift    = 28,//1;
        EG_SQ_TEX_SAMPLER_WORD2_0__TRUNCATE_COORD_bit      = 1 << 28,
		EG_SQ_TEX_SAMPLER_WORD2_0__DISABLE_CUBE_WRAP_shift = 29,//1;
        EG_SQ_TEX_SAMPLER_WORD2_0__DISABLE_CUBE_WRAP_bit   = 1 << 29,
		EG_SQ_TEX_SAMPLER_WORD2_0__Reserved_shift          = 30,//1;
        EG_SQ_TEX_SAMPLER_WORD2_0__Reserved_bit            = 1 << 30,
		EG_SQ_TEX_SAMPLER_WORD2_0__TYPE_shift              = 31,//1;
        EG_SQ_TEX_SAMPLER_WORD2_0__TYPE_bit                = 1 << 31,
    /* SQ_TEX_RESOURCE_WORD0_0 */
        EG_SQ_TEX_RESOURCE_WORD0_0__DIM_shift              = 0, // 3;
        EG_SQ_TEX_RESOURCE_WORD0_0__DIM_mask               = 0x7,
		EG_SQ_TEX_RESOURCE_WORD0_0__ISET_shift             = 3, // 1;
        EG_SQ_TEX_RESOURCE_WORD0_0__ISET_bit               = 1 << 3,
		EG_SQ_TEX_RESOURCE_WORD0_0__Reserve_shift          = 4, // 1;
        EG_SQ_TEX_RESOURCE_WORD0_0__Reserve_bit            = 1 << 4,
		EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_shift             = 5, // 1;
        EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit               = 1 << 5,
		EG_SQ_TEX_RESOURCE_WORD0_0__PITCH_shift            = 6, // 12;
        EG_SQ_TEX_RESOURCE_WORD0_0__PITCH_mask             = 0xfff << 6,
		EG_SQ_TEX_RESOURCE_WORD0_0__TEX_WIDTH_shift        = 18,// 14;
        EG_SQ_TEX_RESOURCE_WORD0_0__TEX_WIDTH_mask         = 0x3fff << 18,
    /* SQ_TEX_RESOURCE_WORD1_0 */
        EG_SQ_TEX_RESOURCE_WORD1_0__TEX_HEIGHT_shift       = 0, // 14;
        EG_SQ_TEX_RESOURCE_WORD1_0__TEX_HEIGHT_mask        = 0x3fff,
		EG_SQ_TEX_RESOURCE_WORD1_0__TEX_DEPTH_shift        = 14,// 13;
        EG_SQ_TEX_RESOURCE_WORD1_0__TEX_DEPTH_mask         = 0x1fff << 14,
		EG_SQ_TEX_RESOURCE_WORD1_0__Reserved_shift         = 27,// 1;
        EG_SQ_TEX_RESOURCE_WORD1_0__Reserved_bit           = 1 << 27,
		EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift       = 28,// 4;
        EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask        = 0xf << 28,
    /* SQ_TEX_RESOURCE_WORD6_0 */
        EG_SQ_TEX_RESOURCE_WORD6_0__MAX_ANISO_RATIO_shift  = 0, //: 3;
        EG_SQ_TEX_RESOURCE_WORD6_0__MAX_ANISO_RATIO_mask   = 0x7,		
		EG_SQ_TEX_RESOURCE_WORD6_0__INTERLACED_shift       = 6, //1;
        EG_SQ_TEX_RESOURCE_WORD6_0__INTERLACED_bit         = 1 << 6,		
		EG_SQ_TEX_RESOURCE_WORD6_0__MIN_LOD_shift          = 8, //12;
        EG_SQ_TEX_RESOURCE_WORD6_0__MIN_LOD_mask           = 0xfff << 8,		
		EG_SQ_TEX_RESOURCE_WORD6_0__TILE_SPLIT_shift       = 29,// 3;
        EG_SQ_TEX_RESOURCE_WORD6_0__TILE_SPLIT_mask        = 0x7 << 29,
    /* SQ_TEX_RESOURCE_WORD7_0 */
        EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift            = 0, // 6;
        EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask             = 0x3f,
		EG_SQ_TEX_RESOURCE_WORD7_0__MACRO_TILE_ASPECT_shift      = 6, // 2;
        EG_SQ_TEX_RESOURCE_WORD7_0__MACRO_TILE_ASPECT_mask       = 0x3 << 6,
		EG_SQ_TEX_RESOURCE_WORD7_0__BANK_WIDTH_shift             = 8, // 2;
        EG_SQ_TEX_RESOURCE_WORD7_0__BANK_WIDTH_mask              = 0x3 << 8,
		EG_SQ_TEX_RESOURCE_WORD7_0__BANK_HEIGHT_shift            = 10,// 2;
        EG_SQ_TEX_RESOURCE_WORD7_0__BANK_HEIGHT_mask             = 0x3 << 10,		
		EG_SQ_TEX_RESOURCE_WORD7_0__DEPTH_SAMPLE_ORDER_shift     = 15,// 1;
        EG_SQ_TEX_RESOURCE_WORD7_0__DEPTH_SAMPLE_ORDER_bit       = 1 << 15,
		EG_SQ_TEX_RESOURCE_WORD7_0__NUM_BANKS_shift              = 16,// 2;
        EG_SQ_TEX_RESOURCE_WORD7_0__NUM_BANKS_mask               = 0x3 << 16,		
		EG_SQ_TEX_RESOURCE_WORD7_0__TYPE_shift                   = 30,// 2;
        EG_SQ_TEX_RESOURCE_WORD7_0__TYPE_mask                    = 0x3 << 30,
};

/*  */

#define EG_SQ_FETCH_RESOURCE_COUNT        0x00000400
#define EG_SQ_TEX_SAMPLER_COUNT           0x0000006c
#define EG_SQ_LOOP_CONST_COUNT            0x000000c0

#define EG_SET_RESOURCE_OFFSET  0x30000
#define EG_SET_RESOURCE_END     0x30400 //r600 := offset + 0x4000

#define EG_SET_LOOP_CONST_OFFSET 0x3A200
#define EG_SET_LOOP_CONST_END    0x3A26C //r600 := offset + 0x180


#define EG_SQ_FETCH_RESOURCE_VS_OFFSET 0x000000b0
#define EG_FETCH_RESOURCE_STRIDE       8

#define EG_SET_BOOL_CONST_OFFSET       0x3A500
#define EG_SET_BOOL_CONST_END          0x3A506


#endif //_EVERGREEN_DIFF_H_
