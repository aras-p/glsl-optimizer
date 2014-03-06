/*
 * Copyright 2013 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Marek Olšák <maraeo@gmail.com>
 */

#ifndef R600D_COMMON_H
#define R600D_COMMON_H

#define R600_CONFIG_REG_OFFSET	0x08000
#define R600_CONTEXT_REG_OFFSET 0x28000
#define CIK_UCONFIG_REG_OFFSET               0x00030000
#define CIK_UCONFIG_REG_END                  0x00031000

#define PKT_TYPE_S(x)                   (((x) & 0x3) << 30)
#define PKT_COUNT_S(x)                  (((x) & 0x3FFF) << 16)
#define PKT3_IT_OPCODE_S(x)             (((x) & 0xFF) << 8)
#define PKT3_PREDICATE(x)               (((x) >> 0) & 0x1)
#define PKT3(op, count, predicate) (PKT_TYPE_S(3) | PKT_COUNT_S(count) | PKT3_IT_OPCODE_S(op) | PKT3_PREDICATE(predicate))

#define RADEON_CP_PACKET3_COMPUTE_MODE 0x00000002

#define PKT3_NOP                               0x10
#define PKT3_SET_PREDICATION                   0x20
#define PKT3_STRMOUT_BUFFER_UPDATE             0x34
#define		STRMOUT_STORE_BUFFER_FILLED_SIZE	1
#define		STRMOUT_OFFSET_SOURCE(x)	(((x) & 0x3) << 1)
#define			STRMOUT_OFFSET_FROM_PACKET		0
#define			STRMOUT_OFFSET_FROM_VGT_FILLED_SIZE	1
#define			STRMOUT_OFFSET_FROM_MEM			2
#define			STRMOUT_OFFSET_NONE			3
#define		STRMOUT_SELECT_BUFFER(x)	(((x) & 0x3) << 8)
#define PKT3_WAIT_REG_MEM                      0x3C
#define		WAIT_REG_MEM_EQUAL		3
#define PKT3_EVENT_WRITE                       0x46
#define PKT3_EVENT_WRITE_EOP                   0x47
#define PKT3_SET_CONFIG_REG		       0x68
#define PKT3_SET_CONTEXT_REG		       0x69
#define PKT3_STRMOUT_BASE_UPDATE	       0x72 /* r700 only */
#define PKT3_SURFACE_BASE_UPDATE               0x73 /* r600 only */
#define		SURFACE_BASE_UPDATE_DEPTH      (1 << 0)
#define		SURFACE_BASE_UPDATE_COLOR(x)   (2 << (x))
#define		SURFACE_BASE_UPDATE_COLOR_NUM(x) (((1 << x) - 1) << 1)
#define		SURFACE_BASE_UPDATE_STRMOUT(x) (0x200 << (x))
#define PKT3_SET_UCONFIG_REG                   0x79 /* new for CIK */

#define EVENT_TYPE_PS_PARTIAL_FLUSH            0x10
#define EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT 0x14
#define EVENT_TYPE_ZPASS_DONE                  0x15
#define EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT   0x16
#define EVENT_TYPE_PIPELINESTAT_START		25
#define EVENT_TYPE_PIPELINESTAT_STOP		26
#define EVENT_TYPE_SAMPLE_PIPELINESTAT		30
#define EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH	0x1f
#define EVENT_TYPE_SAMPLE_STREAMOUTSTATS	0x20
#define EVENT_TYPE_FLUSH_AND_INV_DB_META       0x2c /* supported on r700+ */
#define EVENT_TYPE_FLUSH_AND_INV_CB_META	46 /* supported on r700+ */
#define		EVENT_TYPE(x)                           ((x) << 0)
#define		EVENT_INDEX(x)                          ((x) << 8)
                /* 0 - any non-TS event
		 * 1 - ZPASS_DONE
		 * 2 - SAMPLE_PIPELINESTAT
		 * 3 - SAMPLE_STREAMOUTSTAT*
		 * 4 - *S_PARTIAL_FLUSH
		 * 5 - TS events
		 */

#define PREDICATION_OP_CLEAR 0x0
#define PREDICATION_OP_ZPASS 0x1
#define PREDICATION_OP_PRIMCOUNT 0x2
#define PRED_OP(x) ((x) << 16)
#define PREDICATION_CONTINUE (1 << 31)
#define PREDICATION_HINT_WAIT (0 << 12)
#define PREDICATION_HINT_NOWAIT_DRAW (1 << 12)
#define PREDICATION_DRAW_NOT_VISIBLE (0 << 8)
#define PREDICATION_DRAW_VISIBLE (1 << 8)

/* R600-R700*/
#define R_008490_CP_STRMOUT_CNTL		     0x008490
#define   S_008490_OFFSET_UPDATE_DONE(x)		(((x) & 0x1) << 0)
#define R_028AB0_VGT_STRMOUT_EN                      0x028AB0
#define   S_028AB0_STREAMOUT(x)                        (((x) & 0x1) << 0)
#define   G_028AB0_STREAMOUT(x)                        (((x) >> 0) & 0x1)
#define   C_028AB0_STREAMOUT                           0xFFFFFFFE
#define R_028B20_VGT_STRMOUT_BUFFER_EN               0x028B20
#define   S_028B20_BUFFER_0_EN(x)                      (((x) & 0x1) << 0)
#define   G_028B20_BUFFER_0_EN(x)                      (((x) >> 0) & 0x1)
#define   C_028B20_BUFFER_0_EN                         0xFFFFFFFE
#define   S_028B20_BUFFER_1_EN(x)                      (((x) & 0x1) << 1)
#define   G_028B20_BUFFER_1_EN(x)                      (((x) >> 1) & 0x1)
#define   C_028B20_BUFFER_1_EN                         0xFFFFFFFD
#define   S_028B20_BUFFER_2_EN(x)                      (((x) & 0x1) << 2)
#define   G_028B20_BUFFER_2_EN(x)                      (((x) >> 2) & 0x1)
#define   C_028B20_BUFFER_2_EN                         0xFFFFFFFB
#define   S_028B20_BUFFER_3_EN(x)                      (((x) & 0x1) << 3)
#define   G_028B20_BUFFER_3_EN(x)                      (((x) >> 3) & 0x1)
#define   C_028B20_BUFFER_3_EN                         0xFFFFFFF7
#define R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0                              0x028AD0

#define     V_0280A0_SWAP_STD                          0x00000000
#define     V_0280A0_SWAP_ALT                          0x00000001
#define     V_0280A0_SWAP_STD_REV                      0x00000002
#define     V_0280A0_SWAP_ALT_REV                      0x00000003

/* EG+ */
#define R_0084FC_CP_STRMOUT_CNTL		     0x0084FC
#define   S_0084FC_OFFSET_UPDATE_DONE(x)		(((x) & 0x1) << 0)
#define R_028B94_VGT_STRMOUT_CONFIG                                     0x028B94
#define   S_028B94_STREAMOUT_0_EN(x)                                  (((x) & 0x1) << 0)
#define   G_028B94_STREAMOUT_0_EN(x)                                  (((x) >> 0) & 0x1)
#define   C_028B94_STREAMOUT_0_EN                                     0xFFFFFFFE
#define   S_028B94_STREAMOUT_1_EN(x)                                  (((x) & 0x1) << 1)
#define   G_028B94_STREAMOUT_1_EN(x)                                  (((x) >> 1) & 0x1)
#define   C_028B94_STREAMOUT_1_EN                                     0xFFFFFFFD
#define   S_028B94_STREAMOUT_2_EN(x)                                  (((x) & 0x1) << 2)
#define   G_028B94_STREAMOUT_2_EN(x)                                  (((x) >> 2) & 0x1)
#define   C_028B94_STREAMOUT_2_EN                                     0xFFFFFFFB
#define   S_028B94_STREAMOUT_3_EN(x)                                  (((x) & 0x1) << 3)
#define   G_028B94_STREAMOUT_3_EN(x)                                  (((x) >> 3) & 0x1)
#define   C_028B94_STREAMOUT_3_EN                                     0xFFFFFFF7
#define   S_028B94_RAST_STREAM(x)                                     (((x) & 0x07) << 4)
#define   G_028B94_RAST_STREAM(x)                                     (((x) >> 4) & 0x07)
#define   C_028B94_RAST_STREAM                                        0xFFFFFF8F
#define   S_028B94_RAST_STREAM_MASK(x)                                (((x) & 0x0F) << 8) /* SI+ */
#define   G_028B94_RAST_STREAM_MASK(x)                                (((x) >> 8) & 0x0F)
#define   C_028B94_RAST_STREAM_MASK                                   0xFFFFF0FF
#define   S_028B94_USE_RAST_STREAM_MASK(x)                            (((x) & 0x1) << 31) /* SI+ */
#define   G_028B94_USE_RAST_STREAM_MASK(x)                            (((x) >> 31) & 0x1)
#define   C_028B94_USE_RAST_STREAM_MASK                               0x7FFFFFFF
#define R_028B98_VGT_STRMOUT_BUFFER_CONFIG                              0x028B98
#define   S_028B98_STREAM_0_BUFFER_EN(x)                              (((x) & 0x0F) << 0)
#define   G_028B98_STREAM_0_BUFFER_EN(x)                              (((x) >> 0) & 0x0F)
#define   C_028B98_STREAM_0_BUFFER_EN                                 0xFFFFFFF0
#define   S_028B98_STREAM_1_BUFFER_EN(x)                              (((x) & 0x0F) << 4)
#define   G_028B98_STREAM_1_BUFFER_EN(x)                              (((x) >> 4) & 0x0F)
#define   C_028B98_STREAM_1_BUFFER_EN                                 0xFFFFFF0F
#define   S_028B98_STREAM_2_BUFFER_EN(x)                              (((x) & 0x0F) << 8)
#define   G_028B98_STREAM_2_BUFFER_EN(x)                              (((x) >> 8) & 0x0F)
#define   C_028B98_STREAM_2_BUFFER_EN                                 0xFFFFF0FF
#define   S_028B98_STREAM_3_BUFFER_EN(x)                              (((x) & 0x0F) << 12)
#define   G_028B98_STREAM_3_BUFFER_EN(x)                              (((x) >> 12) & 0x0F)
#define   C_028B98_STREAM_3_BUFFER_EN                                 0xFFFF0FFF

#define CM_R_028804_DB_EQAA                          0x00028804
#define   S_028804_MAX_ANCHOR_SAMPLES(x)		(((x) & 0x7) << 0)
#define   S_028804_PS_ITER_SAMPLES(x)			(((x) & 0x7) << 4)
#define   S_028804_MASK_EXPORT_NUM_SAMPLES(x)		(((x) & 0x7) << 8)
#define   S_028804_ALPHA_TO_MASK_NUM_SAMPLES(x)		(((x) & 0x7) << 12)
#define   S_028804_HIGH_QUALITY_INTERSECTIONS(x)	(((x) & 0x1) << 16)
#define   S_028804_INCOHERENT_EQAA_READS(x)		(((x) & 0x1) << 17)
#define   S_028804_INTERPOLATE_COMP_Z(x)		(((x) & 0x1) << 18)
#define   S_028804_INTERPOLATE_SRC_Z(x)			(((x) & 0x1) << 19)
#define   S_028804_STATIC_ANCHOR_ASSOCIATIONS(x)	(((x) & 0x1) << 20)
#define   S_028804_ALPHA_TO_MASK_EQAA_DISABLE(x)	(((x) & 0x1) << 21)
#define CM_R_028BDC_PA_SC_LINE_CNTL                  0x28bdc
#define   S_028BDC_EXPAND_LINE_WIDTH(x)                (((x) & 0x1) << 9)
#define   G_028BDC_EXPAND_LINE_WIDTH(x)                (((x) >> 9) & 0x1)
#define   C_028BDC_EXPAND_LINE_WIDTH                   0xFFFFFDFF
#define   S_028BDC_LAST_PIXEL(x)                       (((x) & 0x1) << 10)
#define   G_028BDC_LAST_PIXEL(x)                       (((x) >> 10) & 0x1)
#define   C_028BDC_LAST_PIXEL                          0xFFFFFBFF
#define CM_R_028BE0_PA_SC_AA_CONFIG                  0x28be0
#define   S_028BE0_MSAA_NUM_SAMPLES(x)                  (((x) & 0x7) << 0)
#define   S_028BE0_AA_MASK_CENTROID_DTMN(x)		(((x) & 0x1) << 4)
#define   S_028BE0_MAX_SAMPLE_DIST(x)			(((x) & 0xf) << 13)
#define   S_028BE0_MSAA_EXPOSED_SAMPLES(x)		(((x) & 0x7) << 20)
#define   S_028BE0_DETAIL_TO_EXPOSED_MODE(x)		(((x) & 0x3) << 24)
#define CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0 0x28bf8
#define CM_R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0 0x28c08
#define CM_R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0 0x28c18
#define CM_R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0 0x28c28

#define   EG_S_028C70_FAST_CLEAR(x)                       (((x) & 0x1) << 17)
#define   SI_S_028C70_FAST_CLEAR(x)                       (((x) & 0x1) << 13)

/*CIK+*/
#define R_0300FC_CP_STRMOUT_CNTL		     0x0300FC

#endif
