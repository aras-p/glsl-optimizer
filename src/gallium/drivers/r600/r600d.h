/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 * Authors:
 *      Jerome Glisse
 */
#ifndef R600D_H
#define R600D_H

#define R600_TEXEL_PITCH_ALIGNMENT_MASK        0x7

#define PKT3_NOP                               0x10
#define PKT3_INDIRECT_BUFFER_END               0x17
#define PKT3_SET_PREDICATION                   0x20
#define PKT3_REG_RMW                           0x21
#define PKT3_COND_EXEC                         0x22
#define PKT3_PRED_EXEC                         0x23
#define PKT3_START_3D_CMDBUF                   0x24
#define PKT3_DRAW_INDEX_2                      0x27
#define PKT3_CONTEXT_CONTROL                   0x28
#define PKT3_DRAW_INDEX_IMMD_BE                0x29
#define PKT3_INDEX_TYPE                        0x2A
#define PKT3_DRAW_INDEX                        0x2B
#define PKT3_DRAW_INDEX_AUTO                   0x2D
#define PKT3_DRAW_INDEX_IMMD                   0x2E
#define PKT3_NUM_INSTANCES                     0x2F
#define PKT3_STRMOUT_BUFFER_UPDATE             0x34
#define PKT3_INDIRECT_BUFFER_MP                0x38
#define PKT3_MEM_SEMAPHORE                     0x39
#define PKT3_MPEG_INDEX                        0x3A
#define PKT3_WAIT_REG_MEM                      0x3C
#define PKT3_MEM_WRITE                         0x3D
#define PKT3_INDIRECT_BUFFER                   0x32
#define PKT3_CP_INTERRUPT                      0x40
#define PKT3_SURFACE_SYNC                      0x43
#define PKT3_ME_INITIALIZE                     0x44
#define PKT3_COND_WRITE                        0x45
#define PKT3_EVENT_WRITE                       0x46
#define PKT3_EVENT_WRITE_EOP                   0x47
#define PKT3_ONE_REG_WRITE                     0x57
#define PKT3_SET_CONFIG_REG                    0x68
#define PKT3_SET_CONTEXT_REG                   0x69
#define PKT3_SET_ALU_CONST                     0x6A
#define PKT3_SET_BOOL_CONST                    0x6B
#define PKT3_SET_LOOP_CONST                    0x6C
#define PKT3_SET_RESOURCE                      0x6D
#define PKT3_SET_SAMPLER                       0x6E
#define PKT3_SET_CTL_CONST                     0x6F
#define PKT3_SURFACE_BASE_UPDATE               0x73

#define PKT_TYPE_S(x)                   (((x) & 0x3) << 30)
#define PKT_TYPE_G(x)                   (((x) >> 30) & 0x3)
#define PKT_TYPE_C                      0x3FFFFFFF
#define PKT_COUNT_S(x)                  (((x) & 0x3FFF) << 16)
#define PKT_COUNT_G(x)                  (((x) >> 16) & 0x3FFF)
#define PKT_COUNT_C                     0xC000FFFF
#define PKT0_BASE_INDEX_S(x)            (((x) & 0xFFFF) << 0)
#define PKT0_BASE_INDEX_G(x)            (((x) >> 0) & 0xFFFF)
#define PKT0_BASE_INDEX_C               0xFFFF0000
#define PKT3_IT_OPCODE_S(x)             (((x) & 0xFF) << 8)
#define PKT3_IT_OPCODE_G(x)             (((x) >> 8) & 0xFF)
#define PKT3_IT_OPCODE_C                0xFFFF00FF
#define PKT0(index, count) (PKT_TYPE_S(0) | PKT0_BASE_INDEX_S(index) | PKT_COUNT_S(count))
#define PKT3(op, count) (PKT_TYPE_S(3) | PKT3_IT_OPCODE_S(op) | PKT_COUNT_S(count))

/* Registers */
#define R_008C00_SQ_CONFIG                           0x00008C00
#define   S_008C00_VC_ENABLE(x)                        (((x) & 0x1) << 0)
#define   G_008C00_VC_ENABLE(x)                        (((x) >> 0) & 0x1)
#define   C_008C00_VC_ENABLE(x)                        0xFFFFFFFE
#define   S_008C00_EXPORT_SRC_C(x)                     (((x) & 0x1) << 1)
#define   G_008C00_EXPORT_SRC_C(x)                     (((x) >> 1) & 0x1)
#define   C_008C00_EXPORT_SRC_C(x)                     0xFFFFFFFD
#define   S_008C00_DX9_CONSTS(x)                       (((x) & 0x1) << 2)
#define   G_008C00_DX9_CONSTS(x)                       (((x) >> 2) & 0x1)
#define   C_008C00_DX9_CONSTS(x)                       0xFFFFFFFB
#define   S_008C00_ALU_INST_PREFER_VECTOR(x)           (((x) & 0x1) << 3)
#define   G_008C00_ALU_INST_PREFER_VECTOR(x)           (((x) >> 3) & 0x1)
#define   C_008C00_ALU_INST_PREFER_VECTOR(x)           0xFFFFFFF7
#define   S_008C00_DX10_CLAMP(x)                       (((x) & 0x1) << 4)
#define   G_008C00_DX10_CLAMP(x)                       (((x) >> 4) & 0x1)
#define   C_008C00_DX10_CLAMP(x)                       0xFFFFFFEF
#define   S_008C00_CLAUSE_SEQ_PRIO(x)                  (((x) & 0x3) << 8)
#define   G_008C00_CLAUSE_SEQ_PRIO(x)                  (((x) >> 8) & 0x3)
#define   C_008C00_CLAUSE_SEQ_PRIO(x)                  0xFFFFFCFF
#define   S_008C00_PS_PRIO(x)                          (((x) & 0x3) << 24)
#define   G_008C00_PS_PRIO(x)                          (((x) >> 24) & 0x3)
#define   C_008C00_PS_PRIO(x)                          0xFCFFFFFF
#define   S_008C00_VS_PRIO(x)                          (((x) & 0x3) << 26)
#define   G_008C00_VS_PRIO(x)                          (((x) >> 26) & 0x3)
#define   C_008C00_VS_PRIO(x)                          0xF3FFFFFF
#define   S_008C00_GS_PRIO(x)                          (((x) & 0x3) << 28)
#define   G_008C00_GS_PRIO(x)                          (((x) >> 28) & 0x3)
#define   C_008C00_GS_PRIO(x)                          0xCFFFFFFF
#define   S_008C00_ES_PRIO(x)                          (((x) & 0x3) << 30)
#define   G_008C00_ES_PRIO(x)                          (((x) >> 30) & 0x3)
#define   C_008C00_ES_PRIO(x)                          0x3FFFFFFF
#define R_008C04_SQ_GPR_RESOURCE_MGMT_1              0x00008C04
#define   S_008C04_NUM_PS_GPRS(x)                      (((x) & 0xFF) << 0)
#define   G_008C04_NUM_PS_GPRS(x)                      (((x) >> 0) & 0xFF)
#define   C_008C04_NUM_PS_GPRS(x)                      0xFFFFFF00
#define   S_008C04_NUM_VS_GPRS(x)                      (((x) & 0xFF) << 16)
#define   G_008C04_NUM_VS_GPRS(x)                      (((x) >> 16) & 0xFF)
#define   C_008C04_NUM_VS_GPRS(x)                      0xFF00FFFF
#define   S_008C04_NUM_CLAUSE_TEMP_GPRS(x)             (((x) & 0xF) << 28)
#define   G_008C04_NUM_CLAUSE_TEMP_GPRS(x)             (((x) >> 28) & 0xF)
#define   C_008C04_NUM_CLAUSE_TEMP_GPRS(x)             0x0FFFFFFF
#define R_008C08_SQ_GPR_RESOURCE_MGMT_2              0x00008C08
#define   S_008C08_NUM_GS_GPRS(x)                      (((x) & 0xFF) << 0)
#define   G_008C08_NUM_GS_GPRS(x)                      (((x) >> 0) & 0xFF)
#define   C_008C08_NUM_GS_GPRS(x)                      0xFFFFFF00
#define   S_008C08_NUM_ES_GPRS(x)                      (((x) & 0xFF) << 16)
#define   G_008C08_NUM_ES_GPRS(x)                      (((x) >> 16) & 0xFF)
#define   C_008C08_NUM_ES_GPRS(x)                      0xFF00FFFF
#define R_008C0C_SQ_THREAD_RESOURCE_MGMT             0x00008C0C
#define   S_008C0C_NUM_PS_THREADS(x)                   (((x) & 0xFF) << 0)
#define   G_008C0C_NUM_PS_THREADS(x)                   (((x) >> 0) & 0xFF)
#define   C_008C0C_NUM_PS_THREADS(x)                   0xFFFFFF00
#define   S_008C0C_NUM_VS_THREADS(x)                   (((x) & 0xFF) << 8)
#define   G_008C0C_NUM_VS_THREADS(x)                   (((x) >> 8) & 0xFF)
#define   C_008C0C_NUM_VS_THREADS(x)                   0xFFFF00FF
#define   S_008C0C_NUM_GS_THREADS(x)                   (((x) & 0xFF) << 16)
#define   G_008C0C_NUM_GS_THREADS(x)                   (((x) >> 16) & 0xFF)
#define   C_008C0C_NUM_GS_THREADS(x)                   0xFF00FFFF
#define   S_008C0C_NUM_ES_THREADS(x)                   (((x) & 0xFF) << 24)
#define   G_008C0C_NUM_ES_THREADS(x)                   (((x) >> 24) & 0xFF)
#define   C_008C0C_NUM_ES_THREADS(x)                   0x00FFFFFF
#define R_008C10_SQ_STACK_RESOURCE_MGMT_1            0x00008C10
#define   S_008C10_NUM_PS_STACK_ENTRIES(x)             (((x) & 0xFFF) << 0)
#define   G_008C10_NUM_PS_STACK_ENTRIES(x)             (((x) >> 0) & 0xFFF)
#define   C_008C10_NUM_PS_STACK_ENTRIES(x)             0xFFFFF000
#define   S_008C10_NUM_VS_STACK_ENTRIES(x)             (((x) & 0xFFF) << 16)
#define   G_008C10_NUM_VS_STACK_ENTRIES(x)             (((x) >> 16) & 0xFFF)
#define   C_008C10_NUM_VS_STACK_ENTRIES(x)             0xF000FFFF
#define R_008C14_SQ_STACK_RESOURCE_MGMT_2            0x00008C14
#define   S_008C14_NUM_GS_STACK_ENTRIES(x)             (((x) & 0xFFF) << 0)
#define   G_008C14_NUM_GS_STACK_ENTRIES(x)             (((x) >> 0) & 0xFFF)
#define   C_008C14_NUM_GS_STACK_ENTRIES(x)             0xFFFFF000
#define   S_008C14_NUM_ES_STACK_ENTRIES(x)             (((x) & 0xFFF) << 16)
#define   G_008C14_NUM_ES_STACK_ENTRIES(x)             (((x) >> 16) & 0xFFF)
#define   C_008C14_NUM_ES_STACK_ENTRIES(x)             0xF000FFFF
#define R_0280A0_CB_COLOR0_INFO                      0x0280A0
#define   S_0280A0_ENDIAN(x)                           (((x) & 0x3) << 0)
#define   G_0280A0_ENDIAN(x)                           (((x) >> 0) & 0x3)
#define   C_0280A0_ENDIAN                              0xFFFFFFFC
#define   S_0280A0_FORMAT(x)                           (((x) & 0x3F) << 2)
#define   G_0280A0_FORMAT(x)                           (((x) >> 2) & 0x3F)
#define   C_0280A0_FORMAT                              0xFFFFFF03
#define     V_0280A0_COLOR_INVALID                     0x00000000
#define     V_0280A0_COLOR_8                           0x00000001
#define     V_0280A0_COLOR_4_4                         0x00000002
#define     V_0280A0_COLOR_3_3_2                       0x00000003
#define     V_0280A0_COLOR_16                          0x00000005
#define     V_0280A0_COLOR_16_FLOAT                    0x00000006
#define     V_0280A0_COLOR_8_8                         0x00000007
#define     V_0280A0_COLOR_5_6_5                       0x00000008
#define     V_0280A0_COLOR_6_5_5                       0x00000009
#define     V_0280A0_COLOR_1_5_5_5                     0x0000000A
#define     V_0280A0_COLOR_4_4_4_4                     0x0000000B
#define     V_0280A0_COLOR_5_5_5_1                     0x0000000C
#define     V_0280A0_COLOR_32                          0x0000000D
#define     V_0280A0_COLOR_32_FLOAT                    0x0000000E
#define     V_0280A0_COLOR_16_16                       0x0000000F
#define     V_0280A0_COLOR_16_16_FLOAT                 0x00000010
#define     V_0280A0_COLOR_8_24                        0x00000011
#define     V_0280A0_COLOR_8_24_FLOAT                  0x00000012
#define     V_0280A0_COLOR_24_8                        0x00000013
#define     V_0280A0_COLOR_24_8_FLOAT                  0x00000014
#define     V_0280A0_COLOR_10_11_11                    0x00000015
#define     V_0280A0_COLOR_10_11_11_FLOAT              0x00000016
#define     V_0280A0_COLOR_11_11_10                    0x00000017
#define     V_0280A0_COLOR_11_11_10_FLOAT              0x00000018
#define     V_0280A0_COLOR_2_10_10_10                  0x00000019
#define     V_0280A0_COLOR_8_8_8_8                     0x0000001A
#define     V_0280A0_COLOR_10_10_10_2                  0x0000001B
#define     V_0280A0_COLOR_X24_8_32_FLOAT              0x0000001C
#define     V_0280A0_COLOR_32_32                       0x0000001D
#define     V_0280A0_COLOR_32_32_FLOAT                 0x0000001E
#define     V_0280A0_COLOR_16_16_16_16                 0x0000001F
#define     V_0280A0_COLOR_16_16_16_16_FLOAT           0x00000020
#define     V_0280A0_COLOR_32_32_32_32                 0x00000022
#define     V_0280A0_COLOR_32_32_32_32_FLOAT           0x00000023
#define     V_0280A0_COLOR_32_32_32_FLOAT              0x00000030
#define   S_0280A0_ARRAY_MODE(x)                       (((x) & 0xF) << 8)
#define   G_0280A0_ARRAY_MODE(x)                       (((x) >> 8) & 0xF)
#define   C_0280A0_ARRAY_MODE                          0xFFFFF0FF
#define     V_0280A0_ARRAY_LINEAR_GENERAL              0x00000000
#define     V_0280A0_ARRAY_LINEAR_ALIGNED              0x00000001
#define     V_0280A0_ARRAY_1D_TILED_THIN1              0x00000002
#define     V_0280A0_ARRAY_2D_TILED_THIN1              0x00000004
#define   S_0280A0_NUMBER_TYPE(x)                      (((x) & 0x7) << 12)
#define   G_0280A0_NUMBER_TYPE(x)                      (((x) >> 12) & 0x7)
#define   C_0280A0_NUMBER_TYPE                         0xFFFF8FFF
#define     V_0280A0_NUMBER_UNORM                      0x00000000
#define     V_0280A0_NUMBER_SNORM                      0x00000001
#define     V_0280A0_NUMBER_USCALED                    0x00000002
#define     V_0280A0_NUMBER_SSCALED                    0x00000003
#define     V_0280A0_NUMBER_UINT                       0x00000004
#define     V_0280A0_NUMBER_SINT                       0x00000005
#define     V_0280A0_NUMBER_SRGB                       0x00000006
#define     V_0280A0_NUMBER_FLOAT                      0x00000007
#define   S_0280A0_READ_SIZE(x)                        (((x) & 0x1) << 15)
#define   G_0280A0_READ_SIZE(x)                        (((x) >> 15) & 0x1)
#define   C_0280A0_READ_SIZE                           0xFFFF7FFF
#define   S_0280A0_COMP_SWAP(x)                        (((x) & 0x3) << 16)
#define   G_0280A0_COMP_SWAP(x)                        (((x) >> 16) & 0x3)
#define   C_0280A0_COMP_SWAP                           0xFFFCFFFF
#define     V_0280A0_SWAP_STD                          0x00000000
#define     V_0280A0_SWAP_ALT                          0x00000001
#define     V_0280A0_SWAP_STD_REV                      0x00000002
#define     V_0280A0_SWAP_ALT_REV                      0x00000003
#define   S_0280A0_TILE_MODE(x)                        (((x) & 0x3) << 18)
#define   G_0280A0_TILE_MODE(x)                        (((x) >> 18) & 0x3)
#define   C_0280A0_TILE_MODE                           0xFFF3FFFF
#define   S_0280A0_BLEND_CLAMP(x)                      (((x) & 0x1) << 20)
#define   G_0280A0_BLEND_CLAMP(x)                      (((x) >> 20) & 0x1)
#define   C_0280A0_BLEND_CLAMP                         0xFFEFFFFF
#define   S_0280A0_CLEAR_COLOR(x)                      (((x) & 0x1) << 21)
#define   G_0280A0_CLEAR_COLOR(x)                      (((x) >> 21) & 0x1)
#define   C_0280A0_CLEAR_COLOR                         0xFFDFFFFF
#define   S_0280A0_BLEND_BYPASS(x)                     (((x) & 0x1) << 22)
#define   G_0280A0_BLEND_BYPASS(x)                     (((x) >> 22) & 0x1)
#define   C_0280A0_BLEND_BYPASS                        0xFFBFFFFF
#define   S_0280A0_BLEND_FLOAT32(x)                    (((x) & 0x1) << 23)
#define   G_0280A0_BLEND_FLOAT32(x)                    (((x) >> 23) & 0x1)
#define   C_0280A0_BLEND_FLOAT32                       0xFF7FFFFF
#define   S_0280A0_SIMPLE_FLOAT(x)                     (((x) & 0x1) << 24)
#define   G_0280A0_SIMPLE_FLOAT(x)                     (((x) >> 24) & 0x1)
#define   C_0280A0_SIMPLE_FLOAT                        0xFEFFFFFF
#define   S_0280A0_ROUND_MODE(x)                       (((x) & 0x1) << 25)
#define   G_0280A0_ROUND_MODE(x)                       (((x) >> 25) & 0x1)
#define   C_0280A0_ROUND_MODE                          0xFDFFFFFF
#define   S_0280A0_TILE_COMPACT(x)                     (((x) & 0x1) << 26)
#define   G_0280A0_TILE_COMPACT(x)                     (((x) >> 26) & 0x1)
#define   C_0280A0_TILE_COMPACT                        0xFBFFFFFF
#define   S_0280A0_SOURCE_FORMAT(x)                    (((x) & 0x1) << 27)
#define   G_0280A0_SOURCE_FORMAT(x)                    (((x) >> 27) & 0x1)
#define   C_0280A0_SOURCE_FORMAT                       0xF7FFFFFF
#define R_028060_CB_COLOR0_SIZE                      0x028060
#define   S_028060_PITCH_TILE_MAX(x)                   (((x) & 0x3FF) << 0)
#define   G_028060_PITCH_TILE_MAX(x)                   (((x) >> 0) & 0x3FF)
#define   C_028060_PITCH_TILE_MAX                      0xFFFFFC00
#define   S_028060_SLICE_TILE_MAX(x)                   (((x) & 0xFFFFF) << 10)
#define   G_028060_SLICE_TILE_MAX(x)                   (((x) >> 10) & 0xFFFFF)
#define   C_028060_SLICE_TILE_MAX                      0xC00003FF
#define R_028410_SX_ALPHA_TEST_CONTROL               0x028410
#define   S_028410_ALPHA_FUNC(x)                       (((x) & 0x7) << 0)
#define   G_028410_ALPHA_FUNC(x)                       (((x) >> 0) & 0x7)
#define   C_028410_ALPHA_FUNC                          0xFFFFFFF8
#define   S_028410_ALPHA_TEST_ENABLE(x)                (((x) & 0x1) << 3)
#define   G_028410_ALPHA_TEST_ENABLE(x)                (((x) >> 3) & 0x1)
#define   C_028410_ALPHA_TEST_ENABLE                   0xFFFFFFF7
#define   S_028410_ALPHA_TEST_BYPASS(x)                (((x) & 0x1) << 8)
#define   G_028410_ALPHA_TEST_BYPASS(x)                (((x) >> 8) & 0x1)
#define   C_028410_ALPHA_TEST_BYPASS                   0xFFFFFEFF
#define R_028800_DB_DEPTH_CONTROL                    0x028800
#define   S_028800_STENCIL_ENABLE(x)                   (((x) & 0x1) << 0)
#define   G_028800_STENCIL_ENABLE(x)                   (((x) >> 0) & 0x1)
#define   C_028800_STENCIL_ENABLE                      0xFFFFFFFE
#define   S_028800_Z_ENABLE(x)                         (((x) & 0x1) << 1)
#define   G_028800_Z_ENABLE(x)                         (((x) >> 1) & 0x1)
#define   C_028800_Z_ENABLE                            0xFFFFFFFD
#define   S_028800_Z_WRITE_ENABLE(x)                   (((x) & 0x1) << 2)
#define   G_028800_Z_WRITE_ENABLE(x)                   (((x) >> 2) & 0x1)
#define   C_028800_Z_WRITE_ENABLE                      0xFFFFFFFB
#define   S_028800_ZFUNC(x)                            (((x) & 0x7) << 4)
#define   G_028800_ZFUNC(x)                            (((x) >> 4) & 0x7)
#define   C_028800_ZFUNC                               0xFFFFFF8F
#define   S_028800_BACKFACE_ENABLE(x)                  (((x) & 0x1) << 7)
#define   G_028800_BACKFACE_ENABLE(x)                  (((x) >> 7) & 0x1)
#define   C_028800_BACKFACE_ENABLE                     0xFFFFFF7F
#define   S_028800_STENCILFUNC(x)                      (((x) & 0x7) << 8)
#define   G_028800_STENCILFUNC(x)                      (((x) >> 8) & 0x7)
#define   C_028800_STENCILFUNC                         0xFFFFF8FF
#define     V_028800_STENCILFUNC_NEVER                 0x00000000
#define     V_028800_STENCILFUNC_LESS                  0x00000001
#define     V_028800_STENCILFUNC_EQUAL                 0x00000002
#define     V_028800_STENCILFUNC_LEQUAL                0x00000003
#define     V_028800_STENCILFUNC_GREATER               0x00000004
#define     V_028800_STENCILFUNC_NOTEQUAL              0x00000005
#define     V_028800_STENCILFUNC_GEQUAL                0x00000006
#define     V_028800_STENCILFUNC_ALWAYS                0x00000007
#define   S_028800_STENCILFAIL(x)                      (((x) & 0x7) << 11)
#define   G_028800_STENCILFAIL(x)                      (((x) >> 11) & 0x7)
#define   C_028800_STENCILFAIL                         0xFFFFC7FF
#define     V_028800_STENCIL_KEEP                      0x00000000
#define     V_028800_STENCIL_ZERO                      0x00000001
#define     V_028800_STENCIL_REPLACE                   0x00000002
#define     V_028800_STENCIL_INCR                      0x00000003
#define     V_028800_STENCIL_DECR                      0x00000004
#define     V_028800_STENCIL_INVERT                    0x00000005
#define     V_028800_STENCIL_INCR_WRAP                 0x00000006
#define     V_028800_STENCIL_DECR_WRAP                 0x00000007
#define   S_028800_STENCILZPASS(x)                     (((x) & 0x7) << 14)
#define   G_028800_STENCILZPASS(x)                     (((x) >> 14) & 0x7)
#define   C_028800_STENCILZPASS                        0xFFFE3FFF
#define   S_028800_STENCILZFAIL(x)                     (((x) & 0x7) << 17)
#define   G_028800_STENCILZFAIL(x)                     (((x) >> 17) & 0x7)
#define   C_028800_STENCILZFAIL                        0xFFF1FFFF
#define   S_028800_STENCILFUNC_BF(x)                   (((x) & 0x7) << 20)
#define   G_028800_STENCILFUNC_BF(x)                   (((x) >> 20) & 0x7)
#define   C_028800_STENCILFUNC_BF                      0xFF8FFFFF
#define   S_028800_STENCILFAIL_BF(x)                   (((x) & 0x7) << 23)
#define   G_028800_STENCILFAIL_BF(x)                   (((x) >> 23) & 0x7)
#define   C_028800_STENCILFAIL_BF                      0xFC7FFFFF
#define   S_028800_STENCILZPASS_BF(x)                  (((x) & 0x7) << 26)
#define   G_028800_STENCILZPASS_BF(x)                  (((x) >> 26) & 0x7)
#define   C_028800_STENCILZPASS_BF                     0xE3FFFFFF
#define   S_028800_STENCILZFAIL_BF(x)                  (((x) & 0x7) << 29)
#define   G_028800_STENCILZFAIL_BF(x)                  (((x) >> 29) & 0x7)
#define   C_028800_STENCILZFAIL_BF                     0x1FFFFFFF
#define R_028808_CB_COLOR_CONTROL                    0x028808
#define   S_028808_FOG_ENABLE(x)                       (((x) & 0x1) << 0)
#define   G_028808_FOG_ENABLE(x)                       (((x) >> 0) & 0x1)
#define   C_028808_FOG_ENABLE                          0xFFFFFFFE
#define   S_028808_MULTIWRITE_ENABLE(x)                (((x) & 0x1) << 1)
#define   G_028808_MULTIWRITE_ENABLE(x)                (((x) >> 1) & 0x1)
#define   C_028808_MULTIWRITE_ENABLE                   0xFFFFFFFD
#define   S_028808_DITHER_ENABLE(x)                    (((x) & 0x1) << 2)
#define   G_028808_DITHER_ENABLE(x)                    (((x) >> 2) & 0x1)
#define   C_028808_DITHER_ENABLE                       0xFFFFFFFB
#define   S_028808_DEGAMMA_ENABLE(x)                   (((x) & 0x1) << 3)
#define   G_028808_DEGAMMA_ENABLE(x)                   (((x) >> 3) & 0x1)
#define   C_028808_DEGAMMA_ENABLE                      0xFFFFFFF7
#define   S_028808_SPECIAL_OP(x)                       (((x) & 0x7) << 4)
#define   G_028808_SPECIAL_OP(x)                       (((x) >> 4) & 0x7)
#define   C_028808_SPECIAL_OP                          0xFFFFFF8F
#define   S_028808_PER_MRT_BLEND(x)                    (((x) & 0x1) << 7)
#define   G_028808_PER_MRT_BLEND(x)                    (((x) >> 7) & 0x1)
#define   C_028808_PER_MRT_BLEND                       0xFFFFFF7F
#define   S_028808_TARGET_BLEND_ENABLE(x)              (((x) & 0xFF) << 8)
#define   G_028808_TARGET_BLEND_ENABLE(x)              (((x) >> 8) & 0xFF)
#define   C_028808_TARGET_BLEND_ENABLE                 0xFFFF00FF
#define   S_028808_ROP3(x)                             (((x) & 0xFF) << 16)
#define   G_028808_ROP3(x)                             (((x) >> 16) & 0xFF)
#define   C_028808_ROP3                                0xFF00FFFF
#define R_028810_PA_CL_CLIP_CNTL                     0x028810
#define   S_028810_UCP_ENA_0(x)                        (((x) & 0x1) << 0)
#define   G_028810_UCP_ENA_0(x)                        (((x) >> 0) & 0x1)
#define   C_028810_UCP_ENA_0                           0xFFFFFFFE
#define   S_028810_UCP_ENA_1(x)                        (((x) & 0x1) << 1)
#define   G_028810_UCP_ENA_1(x)                        (((x) >> 1) & 0x1)
#define   C_028810_UCP_ENA_1                           0xFFFFFFFD
#define   S_028810_UCP_ENA_2(x)                        (((x) & 0x1) << 2)
#define   G_028810_UCP_ENA_2(x)                        (((x) >> 2) & 0x1)
#define   C_028810_UCP_ENA_2                           0xFFFFFFFB
#define   S_028810_UCP_ENA_3(x)                        (((x) & 0x1) << 3)
#define   G_028810_UCP_ENA_3(x)                        (((x) >> 3) & 0x1)
#define   C_028810_UCP_ENA_3                           0xFFFFFFF7
#define   S_028810_UCP_ENA_4(x)                        (((x) & 0x1) << 4)
#define   G_028810_UCP_ENA_4(x)                        (((x) >> 4) & 0x1)
#define   C_028810_UCP_ENA_4                           0xFFFFFFEF
#define   S_028810_UCP_ENA_5(x)                        (((x) & 0x1) << 5)
#define   G_028810_UCP_ENA_5(x)                        (((x) >> 5) & 0x1)
#define   C_028810_UCP_ENA_5                           0xFFFFFFDF
#define   S_028810_PS_UCP_Y_SCALE_NEG(x)               (((x) & 0x1) << 13)
#define   G_028810_PS_UCP_Y_SCALE_NEG(x)               (((x) >> 13) & 0x1)
#define   C_028810_PS_UCP_Y_SCALE_NEG                  0xFFFFDFFF
#define   S_028810_PS_UCP_MODE(x)                      (((x) & 0x3) << 14)
#define   G_028810_PS_UCP_MODE(x)                      (((x) >> 14) & 0x3)
#define   C_028810_PS_UCP_MODE                         0xFFFF3FFF
#define   S_028810_CLIP_DISABLE(x)                     (((x) & 0x1) << 16)
#define   G_028810_CLIP_DISABLE(x)                     (((x) >> 16) & 0x1)
#define   C_028810_CLIP_DISABLE                        0xFFFEFFFF
#define   S_028810_UCP_CULL_ONLY_ENA(x)                (((x) & 0x1) << 17)
#define   G_028810_UCP_CULL_ONLY_ENA(x)                (((x) >> 17) & 0x1)
#define   C_028810_UCP_CULL_ONLY_ENA                   0xFFFDFFFF
#define   S_028810_BOUNDARY_EDGE_FLAG_ENA(x)           (((x) & 0x1) << 18)
#define   G_028810_BOUNDARY_EDGE_FLAG_ENA(x)           (((x) >> 18) & 0x1)
#define   C_028810_BOUNDARY_EDGE_FLAG_ENA              0xFFFBFFFF
#define   S_028810_DX_CLIP_SPACE_DEF(x)                (((x) & 0x1) << 19)
#define   G_028810_DX_CLIP_SPACE_DEF(x)                (((x) >> 19) & 0x1)
#define   C_028810_DX_CLIP_SPACE_DEF                   0xFFF7FFFF
#define   S_028810_DIS_CLIP_ERR_DETECT(x)              (((x) & 0x1) << 20)
#define   G_028810_DIS_CLIP_ERR_DETECT(x)              (((x) >> 20) & 0x1)
#define   C_028810_DIS_CLIP_ERR_DETECT                 0xFFEFFFFF
#define   S_028810_VTX_KILL_OR(x)                      (((x) & 0x1) << 21)
#define   G_028810_VTX_KILL_OR(x)                      (((x) >> 21) & 0x1)
#define   C_028810_VTX_KILL_OR                         0xFFDFFFFF
#define   S_028810_DX_LINEAR_ATTR_CLIP_ENA(x)          (((x) & 0x1) << 24)
#define   G_028810_DX_LINEAR_ATTR_CLIP_ENA(x)          (((x) >> 24) & 0x1)
#define   C_028810_DX_LINEAR_ATTR_CLIP_ENA             0xFEFFFFFF
#define   S_028810_VTE_VPORT_PROVOKE_DISABLE(x)        (((x) & 0x1) << 25)
#define   G_028810_VTE_VPORT_PROVOKE_DISABLE(x)        (((x) >> 25) & 0x1)
#define   C_028810_VTE_VPORT_PROVOKE_DISABLE           0xFDFFFFFF
#define   S_028810_ZCLIP_NEAR_DISABLE(x)               (((x) & 0x1) << 26)
#define   G_028810_ZCLIP_NEAR_DISABLE(x)               (((x) >> 26) & 0x1)
#define   C_028810_ZCLIP_NEAR_DISABLE                  0xFBFFFFFF
#define   S_028810_ZCLIP_FAR_DISABLE(x)                (((x) & 0x1) << 27)
#define   G_028810_ZCLIP_FAR_DISABLE(x)                (((x) >> 27) & 0x1)
#define   C_028810_ZCLIP_FAR_DISABLE                   0xF7FFFFFF
#define R_028010_DB_DEPTH_INFO                       0x028010
#define   S_028010_FORMAT(x)                           (((x) & 0x7) << 0)
#define   G_028010_FORMAT(x)                           (((x) >> 0) & 0x7)
#define   C_028010_FORMAT                              0xFFFFFFF8
#define     V_028010_DEPTH_INVALID                     0x00000000
#define     V_028010_DEPTH_16                          0x00000001
#define     V_028010_DEPTH_X8_24                       0x00000002
#define     V_028010_DEPTH_8_24                        0x00000003
#define     V_028010_DEPTH_X8_24_FLOAT                 0x00000004
#define     V_028010_DEPTH_8_24_FLOAT                  0x00000005
#define     V_028010_DEPTH_32_FLOAT                    0x00000006
#define     V_028010_DEPTH_X24_8_32_FLOAT              0x00000007
#define   S_028010_READ_SIZE(x)                        (((x) & 0x1) << 3)
#define   G_028010_READ_SIZE(x)                        (((x) >> 3) & 0x1)
#define   C_028010_READ_SIZE                           0xFFFFFFF7
#define   S_028010_ARRAY_MODE(x)                       (((x) & 0xF) << 15)
#define   G_028010_ARRAY_MODE(x)                       (((x) >> 15) & 0xF)
#define   C_028010_ARRAY_MODE                          0xFFF87FFF
#define   S_028010_TILE_SURFACE_ENABLE(x)              (((x) & 0x1) << 25)
#define   G_028010_TILE_SURFACE_ENABLE(x)              (((x) >> 25) & 0x1)
#define   C_028010_TILE_SURFACE_ENABLE                 0xFDFFFFFF
#define   S_028010_TILE_COMPACT(x)                     (((x) & 0x1) << 26)
#define   G_028010_TILE_COMPACT(x)                     (((x) >> 26) & 0x1)
#define   C_028010_TILE_COMPACT                        0xFBFFFFFF
#define   S_028010_ZRANGE_PRECISION(x)                 (((x) & 0x1) << 31)
#define   G_028010_ZRANGE_PRECISION(x)                 (((x) >> 31) & 0x1)
#define   C_028010_ZRANGE_PRECISION                    0x7FFFFFFF
#define R_028430_DB_STENCILREFMASK                   0x028430
#define   S_028430_STENCILREF(x)                       (((x) & 0xFF) << 0)
#define   G_028430_STENCILREF(x)                       (((x) >> 0) & 0xFF)
#define   C_028430_STENCILREF                          0xFFFFFF00
#define   S_028430_STENCILMASK(x)                      (((x) & 0xFF) << 8)
#define   G_028430_STENCILMASK(x)                      (((x) >> 8) & 0xFF)
#define   C_028430_STENCILMASK                         0xFFFF00FF
#define   S_028430_STENCILWRITEMASK(x)                 (((x) & 0xFF) << 16)
#define   G_028430_STENCILWRITEMASK(x)                 (((x) >> 16) & 0xFF)
#define   C_028430_STENCILWRITEMASK                    0xFF00FFFF
#define R_028434_DB_STENCILREFMASK_BF                0x028434
#define   S_028434_STENCILREF_BF(x)                    (((x) & 0xFF) << 0)
#define   G_028434_STENCILREF_BF(x)                    (((x) >> 0) & 0xFF)
#define   C_028434_STENCILREF_BF                       0xFFFFFF00
#define   S_028434_STENCILMASK_BF(x)                   (((x) & 0xFF) << 8)
#define   G_028434_STENCILMASK_BF(x)                   (((x) >> 8) & 0xFF)
#define   C_028434_STENCILMASK_BF                      0xFFFF00FF
#define   S_028434_STENCILWRITEMASK_BF(x)              (((x) & 0xFF) << 16)
#define   G_028434_STENCILWRITEMASK_BF(x)              (((x) >> 16) & 0xFF)
#define   C_028434_STENCILWRITEMASK_BF                 0xFF00FFFF
#define R_028804_CB_BLEND_CONTROL                    0x028804
#define   S_028804_COLOR_SRCBLEND(x)                   (((x) & 0x1F) << 0)
#define   G_028804_COLOR_SRCBLEND(x)                   (((x) >> 0) & 0x1F)
#define   C_028804_COLOR_SRCBLEND                      0xFFFFFFE0
#define     V_028804_BLEND_ZERO                        0x00000000
#define     V_028804_BLEND_ONE                         0x00000001
#define     V_028804_BLEND_SRC_COLOR                   0x00000002
#define     V_028804_BLEND_ONE_MINUS_SRC_COLOR         0x00000003
#define     V_028804_BLEND_SRC_ALPHA                   0x00000004
#define     V_028804_BLEND_ONE_MINUS_SRC_ALPHA         0x00000005
#define     V_028804_BLEND_DST_ALPHA                   0x00000006
#define     V_028804_BLEND_ONE_MINUS_DST_ALPHA         0x00000007
#define     V_028804_BLEND_DST_COLOR                   0x00000008
#define     V_028804_BLEND_ONE_MINUS_DST_COLOR         0x00000009
#define     V_028804_BLEND_SRC_ALPHA_SATURATE          0x0000000A
#define     V_028804_BLEND_BOTH_SRC_ALPHA              0x0000000B
#define     V_028804_BLEND_BOTH_INV_SRC_ALPHA          0x0000000C
#define     V_028804_BLEND_CONST_COLOR                 0x0000000D
#define     V_028804_BLEND_ONE_MINUS_CONST_COLOR       0x0000000E
#define     V_028804_BLEND_SRC1_COLOR                  0x0000000F
#define     V_028804_BLEND_INV_SRC1_COLOR              0x00000010
#define     V_028804_BLEND_SRC1_ALPHA                  0x00000011
#define     V_028804_BLEND_INV_SRC1_ALPHA              0x00000012
#define     V_028804_BLEND_CONST_ALPHA                 0x00000013
#define     V_028804_BLEND_ONE_MINUS_CONST_ALPHA       0x00000014
#define   S_028804_COLOR_COMB_FCN(x)                   (((x) & 0x7) << 5)
#define   G_028804_COLOR_COMB_FCN(x)                   (((x) >> 5) & 0x7)
#define   C_028804_COLOR_COMB_FCN                      0xFFFFFF1F
#define     V_028804_COMB_DST_PLUS_SRC                 0x00000000
#define     V_028804_COMB_SRC_MINUS_DST                0x00000001
#define     V_028804_COMB_MIN_DST_SRC                  0x00000002
#define     V_028804_COMB_MAX_DST_SRC                  0x00000003
#define     V_028804_COMB_DST_MINUS_SRC                0x00000004
#define   S_028804_COLOR_DESTBLEND(x)                  (((x) & 0x1F) << 8)
#define   G_028804_COLOR_DESTBLEND(x)                  (((x) >> 8) & 0x1F)
#define   C_028804_COLOR_DESTBLEND                     0xFFFFE0FF
#define   S_028804_OPACITY_WEIGHT(x)                   (((x) & 0x1) << 13)
#define   G_028804_OPACITY_WEIGHT(x)                   (((x) >> 13) & 0x1)
#define   C_028804_OPACITY_WEIGHT                      0xFFFFDFFF
#define   S_028804_ALPHA_SRCBLEND(x)                   (((x) & 0x1F) << 16)
#define   G_028804_ALPHA_SRCBLEND(x)                   (((x) >> 16) & 0x1F)
#define   C_028804_ALPHA_SRCBLEND                      0xFFE0FFFF
#define   S_028804_ALPHA_COMB_FCN(x)                   (((x) & 0x7) << 21)
#define   G_028804_ALPHA_COMB_FCN(x)                   (((x) >> 21) & 0x7)
#define   C_028804_ALPHA_COMB_FCN                      0xFF1FFFFF
#define   S_028804_ALPHA_DESTBLEND(x)                  (((x) & 0x1F) << 24)
#define   G_028804_ALPHA_DESTBLEND(x)                  (((x) >> 24) & 0x1F)
#define   C_028804_ALPHA_DESTBLEND                     0xE0FFFFFF
#define   S_028804_SEPARATE_ALPHA_BLEND(x)             (((x) & 0x1) << 29)
#define   G_028804_SEPARATE_ALPHA_BLEND(x)             (((x) >> 29) & 0x1)
#define   C_028804_SEPARATE_ALPHA_BLEND                0xDFFFFFFF
#define R_028814_PA_SU_SC_MODE_CNTL                  0x028814
#define   S_028814_CULL_FRONT(x)                       (((x) & 0x1) << 0)
#define   G_028814_CULL_FRONT(x)                       (((x) >> 0) & 0x1)
#define   C_028814_CULL_FRONT                          0xFFFFFFFE
#define   S_028814_CULL_BACK(x)                        (((x) & 0x1) << 1)
#define   G_028814_CULL_BACK(x)                        (((x) >> 1) & 0x1)
#define   C_028814_CULL_BACK                           0xFFFFFFFD
#define   S_028814_FACE(x)                             (((x) & 0x1) << 2)
#define   G_028814_FACE(x)                             (((x) >> 2) & 0x1)
#define   C_028814_FACE                                0xFFFFFFFB
#define   S_028814_POLY_MODE(x)                        (((x) & 0x3) << 3)
#define   G_028814_POLY_MODE(x)                        (((x) >> 3) & 0x3)
#define   C_028814_POLY_MODE                           0xFFFFFFE7
#define   S_028814_POLYMODE_FRONT_PTYPE(x)             (((x) & 0x7) << 5)
#define   G_028814_POLYMODE_FRONT_PTYPE(x)             (((x) >> 5) & 0x7)
#define   C_028814_POLYMODE_FRONT_PTYPE                0xFFFFFF1F
#define   S_028814_POLYMODE_BACK_PTYPE(x)              (((x) & 0x7) << 8)
#define   G_028814_POLYMODE_BACK_PTYPE(x)              (((x) >> 8) & 0x7)
#define   C_028814_POLYMODE_BACK_PTYPE                 0xFFFFF8FF
#define   S_028814_POLY_OFFSET_FRONT_ENABLE(x)         (((x) & 0x1) << 11)
#define   G_028814_POLY_OFFSET_FRONT_ENABLE(x)         (((x) >> 11) & 0x1)
#define   C_028814_POLY_OFFSET_FRONT_ENABLE            0xFFFFF7FF
#define   S_028814_POLY_OFFSET_BACK_ENABLE(x)          (((x) & 0x1) << 12)
#define   G_028814_POLY_OFFSET_BACK_ENABLE(x)          (((x) >> 12) & 0x1)
#define   C_028814_POLY_OFFSET_BACK_ENABLE             0xFFFFEFFF
#define   S_028814_POLY_OFFSET_PARA_ENABLE(x)          (((x) & 0x1) << 13)
#define   G_028814_POLY_OFFSET_PARA_ENABLE(x)          (((x) >> 13) & 0x1)
#define   C_028814_POLY_OFFSET_PARA_ENABLE             0xFFFFDFFF
#define   S_028814_VTX_WINDOW_OFFSET_ENABLE(x)         (((x) & 0x1) << 16)
#define   G_028814_VTX_WINDOW_OFFSET_ENABLE(x)         (((x) >> 16) & 0x1)
#define   C_028814_VTX_WINDOW_OFFSET_ENABLE            0xFFFEFFFF
#define   S_028814_PROVOKING_VTX_LAST(x)               (((x) & 0x1) << 19)
#define   G_028814_PROVOKING_VTX_LAST(x)               (((x) >> 19) & 0x1)
#define   C_028814_PROVOKING_VTX_LAST                  0xFFF7FFFF
#define   S_028814_PERSP_CORR_DIS(x)                   (((x) & 0x1) << 20)
#define   G_028814_PERSP_CORR_DIS(x)                   (((x) >> 20) & 0x1)
#define   C_028814_PERSP_CORR_DIS                      0xFFEFFFFF
#define   S_028814_MULTI_PRIM_IB_ENA(x)                (((x) & 0x1) << 21)
#define   G_028814_MULTI_PRIM_IB_ENA(x)                (((x) >> 21) & 0x1)
#define   C_028814_MULTI_PRIM_IB_ENA                   0xFFDFFFFF
#define R_028000_DB_DEPTH_SIZE                       0x028000
#define   S_028000_PITCH_TILE_MAX(x)                   (((x) & 0x3FF) << 0)
#define   G_028000_PITCH_TILE_MAX(x)                   (((x) >> 0) & 0x3FF)
#define   C_028000_PITCH_TILE_MAX                      0xFFFFFC00
#define   S_028000_SLICE_TILE_MAX(x)                   (((x) & 0xFFFFF) << 10)
#define   G_028000_SLICE_TILE_MAX(x)                   (((x) >> 10) & 0xFFFFF)
#define   C_028000_SLICE_TILE_MAX                      0xC00003FF
#define R_028004_DB_DEPTH_VIEW                       0x028004
#define   S_028004_SLICE_START(x)                      (((x) & 0x7FF) << 0)
#define   G_028004_SLICE_START(x)                      (((x) >> 0) & 0x7FF)
#define   C_028004_SLICE_START                         0xFFFFF800
#define   S_028004_SLICE_MAX(x)                        (((x) & 0x7FF) << 13)
#define   G_028004_SLICE_MAX(x)                        (((x) >> 13) & 0x7FF)
#define   C_028004_SLICE_MAX                           0xFF001FFF
#define R_028D24_DB_HTILE_SURFACE                    0x028D24
#define   S_028D24_HTILE_WIDTH(x)                      (((x) & 0x1) << 0)
#define   G_028D24_HTILE_WIDTH(x)                      (((x) >> 0) & 0x1)
#define   C_028D24_HTILE_WIDTH                         0xFFFFFFFE
#define   S_028D24_HTILE_HEIGHT(x)                     (((x) & 0x1) << 1)
#define   G_028D24_HTILE_HEIGHT(x)                     (((x) >> 1) & 0x1)
#define   C_028D24_HTILE_HEIGHT                        0xFFFFFFFD
#define   S_028D24_LINEAR(x)                           (((x) & 0x1) << 2)
#define   G_028D24_LINEAR(x)                           (((x) >> 2) & 0x1)
#define   C_028D24_LINEAR                              0xFFFFFFFB
#define   S_028D24_FULL_CACHE(x)                       (((x) & 0x1) << 3)
#define   G_028D24_FULL_CACHE(x)                       (((x) >> 3) & 0x1)
#define   C_028D24_FULL_CACHE                          0xFFFFFFF7
#define   S_028D24_HTILE_USES_PRELOAD_WIN(x)           (((x) & 0x1) << 4)
#define   G_028D24_HTILE_USES_PRELOAD_WIN(x)           (((x) >> 4) & 0x1)
#define   C_028D24_HTILE_USES_PRELOAD_WIN              0xFFFFFFEF
#define   S_028D24_PRELOAD(x)                          (((x) & 0x1) << 5)
#define   G_028D24_PRELOAD(x)                          (((x) >> 5) & 0x1)
#define   C_028D24_PRELOAD                             0xFFFFFFDF
#define   S_028D24_PREFETCH_WIDTH(x)                   (((x) & 0x3F) << 6)
#define   G_028D24_PREFETCH_WIDTH(x)                   (((x) >> 6) & 0x3F)
#define   C_028D24_PREFETCH_WIDTH                      0xFFFFF03F
#define   S_028D24_PREFETCH_HEIGHT(x)                  (((x) & 0x3F) << 12)
#define   G_028D24_PREFETCH_HEIGHT(x)                  (((x) >> 12) & 0x3F)
#define   C_028D24_PREFETCH_HEIGHT                     0xFFFC0FFF
#define R_028D34_DB_PREFETCH_LIMIT                   0x028D34
#define   S_028D34_DEPTH_HEIGHT_TILE_MAX(x)            (((x) & 0x3FF) << 0)
#define   G_028D34_DEPTH_HEIGHT_TILE_MAX(x)            (((x) >> 0) & 0x3FF)
#define   C_028D34_DEPTH_HEIGHT_TILE_MAX               0xFFFFFC00
#define R_028D0C_DB_RENDER_CONTROL                   0x028D0C
#define   S_028D0C_STENCIL_COMPRESS_DISABLE(x)         (((x) & 0x1) << 5)
#define   S_028D0C_DEPTH_COMPRESS_DISABLE(x)           (((x) & 0x1) << 6)
#define   S_028D0C_R700_PERFECT_ZPASS_COUNTS(x)        (((x) & 0x1) << 15)
#define R_028D10_DB_RENDER_OVERRIDE                  0x028D10
#define   V_028D10_FORCE_OFF                         0
#define   V_028D10_FORCE_ENABLE                      1
#define   V_028D10_FORCE_DISABLE                     2
#define   S_028D10_FORCE_HIZ_ENABLE(x)                 (((x) & 0x3) << 0)
#define   G_028D10_FORCE_HIZ_ENABLE(x)                 (((x) >> 0) & 0x3)
#define   C_028D10_FORCE_HIZ_ENABLE                    0xFFFFFFFC
#define   S_028D10_FORCE_HIS_ENABLE0(x)                (((x) & 0x3) << 2)
#define   G_028D10_FORCE_HIS_ENABLE0(x)                (((x) >> 2) & 0x3)
#define   C_028D10_FORCE_HIS_ENABLE0                   0xFFFFFFF3
#define   S_028D10_FORCE_HIS_ENABLE1(x)                (((x) & 0x3) << 4)
#define   G_028D10_FORCE_HIS_ENABLE1(x)                (((x) >> 4) & 0x3)
#define   C_028D10_FORCE_HIS_ENABLE1                   0xFFFFFFCF
#define   S_028D10_FORCE_SHADER_Z_ORDER(x)             (((x) & 0x1) << 6)
#define   G_028D10_FORCE_SHADER_Z_ORDER(x)             (((x) >> 6) & 0x1)
#define   C_028D10_FORCE_SHADER_Z_ORDER                0xFFFFFFBF
#define   S_028D10_FAST_Z_DISABLE(x)                   (((x) & 0x1) << 7)
#define   G_028D10_FAST_Z_DISABLE(x)                   (((x) >> 7) & 0x1)
#define   C_028D10_FAST_Z_DISABLE                      0xFFFFFF7F
#define   S_028D10_FAST_STENCIL_DISABLE(x)             (((x) & 0x1) << 8)
#define   G_028D10_FAST_STENCIL_DISABLE(x)             (((x) >> 8) & 0x1)
#define   C_028D10_FAST_STENCIL_DISABLE                0xFFFFFEFF
#define   S_028D10_NOOP_CULL_DISABLE(x)                (((x) & 0x1) << 9)
#define   G_028D10_NOOP_CULL_DISABLE(x)                (((x) >> 9) & 0x1)
#define   C_028D10_NOOP_CULL_DISABLE                   0xFFFFFDFF
#define   S_028D10_FORCE_COLOR_KILL(x)                 (((x) & 0x1) << 10)
#define   G_028D10_FORCE_COLOR_KILL(x)                 (((x) >> 10) & 0x1)
#define   C_028D10_FORCE_COLOR_KILL                    0xFFFFFBFF
#define   S_028D10_FORCE_Z_READ(x)                     (((x) & 0x1) << 11)
#define   G_028D10_FORCE_Z_READ(x)                     (((x) >> 11) & 0x1)
#define   C_028D10_FORCE_Z_READ                        0xFFFFF7FF
#define   S_028D10_FORCE_STENCIL_READ(x)               (((x) & 0x1) << 12)
#define   G_028D10_FORCE_STENCIL_READ(x)               (((x) >> 12) & 0x1)
#define   C_028D10_FORCE_STENCIL_READ                  0xFFFFEFFF
#define   S_028D10_FORCE_FULL_Z_RANGE(x)               (((x) & 0x3) << 13)
#define   G_028D10_FORCE_FULL_Z_RANGE(x)               (((x) >> 13) & 0x3)
#define   C_028D10_FORCE_FULL_Z_RANGE                  0xFFFF9FFF
#define   S_028D10_FORCE_QC_SMASK_CONFLICT(x)          (((x) & 0x1) << 15)
#define   G_028D10_FORCE_QC_SMASK_CONFLICT(x)          (((x) >> 15) & 0x1)
#define   C_028D10_FORCE_QC_SMASK_CONFLICT             0xFFFF7FFF
#define   S_028D10_DISABLE_VIEWPORT_CLAMP(x)           (((x) & 0x1) << 16)
#define   G_028D10_DISABLE_VIEWPORT_CLAMP(x)           (((x) >> 16) & 0x1)
#define   C_028D10_DISABLE_VIEWPORT_CLAMP              0xFFFEFFFF
#define   S_028D10_IGNORE_SC_ZRANGE(x)                 (((x) & 0x1) << 17)
#define   G_028D10_IGNORE_SC_ZRANGE(x)                 (((x) >> 17) & 0x1)
#define   C_028D10_IGNORE_SC_ZRANGE                    0xFFFDFFFF
#define R_028DF8_PA_SU_POLY_OFFSET_DB_FMT_CNTL       0x028DF8
#define   S_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS(x)      (((x) & 0xFF) << 0)
#define   G_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS(x)      (((x) >> 0) & 0xFF)
#define   C_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS         0xFFFFFF00
#define   S_028DF8_POLY_OFFSET_DB_IS_FLOAT_FMT(x)      (((x) & 0x1) << 8)
#define   G_028DF8_POLY_OFFSET_DB_IS_FLOAT_FMT(x)      (((x) >> 8) & 0x1)
#define   C_028DF8_POLY_OFFSET_DB_IS_FLOAT_FMT         0xFFFFFEFF
#define R_028E00_PA_SU_POLY_OFFSET_FRONT_SCALE       0x028E00
#define   S_028E00_SCALE(x)                            (((x) & 0xFFFFFFFF) << 0)
#define   G_028E00_SCALE(x)                            (((x) >> 0) & 0xFFFFFFFF)
#define   C_028E00_SCALE                               0x00000000
#define R_028E04_PA_SU_POLY_OFFSET_FRONT_OFFSET      0x028E04
#define   S_028E04_OFFSET(x)                           (((x) & 0xFFFFFFFF) << 0)
#define   G_028E04_OFFSET(x)                           (((x) >> 0) & 0xFFFFFFFF)
#define   C_028E04_OFFSET                              0x00000000
#define R_028E08_PA_SU_POLY_OFFSET_BACK_SCALE        0x028E08
#define   S_028E08_SCALE(x)                            (((x) & 0xFFFFFFFF) << 0)
#define   G_028E08_SCALE(x)                            (((x) >> 0) & 0xFFFFFFFF)
#define   C_028E08_SCALE                               0x00000000
#define R_028E0C_PA_SU_POLY_OFFSET_BACK_OFFSET       0x028E0C
#define   S_028E0C_OFFSET(x)                           (((x) & 0xFFFFFFFF) << 0)
#define   G_028E0C_OFFSET(x)                           (((x) >> 0) & 0xFFFFFFFF)
#define   C_028E0C_OFFSET                              0x00000000
#define R_028A00_PA_SU_POINT_SIZE                    0x028A00
#define   S_028A00_HEIGHT(x)                           (((x) & 0xFFFF) << 0)
#define   G_028A00_HEIGHT(x)                           (((x) >> 0) & 0xFFFF)
#define   C_028A00_HEIGHT                              0xFFFF0000
#define   S_028A00_WIDTH(x)                            (((x) & 0xFFFF) << 16)
#define   G_028A00_WIDTH(x)                            (((x) >> 16) & 0xFFFF)
#define   C_028A00_WIDTH                               0x0000FFFF
#define R_028A40_VGT_GS_MODE                         0x028A40
#define   S_028A40_MODE(x)                             (((x) & 0x3) << 0)
#define   G_028A40_MODE(x)                             (((x) >> 0) & 0x3)
#define   C_028A40_MODE                                0xFFFFFFFC
#define   S_028A40_ES_PASSTHRU(x)                      (((x) & 0x1) << 2)
#define   G_028A40_ES_PASSTHRU(x)                      (((x) >> 2) & 0x1)
#define   C_028A40_ES_PASSTHRU                         0xFFFFFFFB
#define   S_028A40_CUT_MODE(x)                         (((x) & 0x3) << 3)
#define   G_028A40_CUT_MODE(x)                         (((x) >> 3) & 0x3)
#define   C_028A40_CUT_MODE                            0xFFFFFFE7
#define R_008040_WAIT_UNTIL                          0x008040
#define   S_008040_WAIT_CP_DMA_IDLE(x)                 (((x) & 0x1) << 8)
#define   G_008040_WAIT_CP_DMA_IDLE(x)                 (((x) >> 8) & 0x1)
#define   C_008040_WAIT_CP_DMA_IDLE                    0xFFFFFEFF
#define   S_008040_WAIT_CMDFIFO(x)                     (((x) & 0x1) << 10)
#define   G_008040_WAIT_CMDFIFO(x)                     (((x) >> 10) & 0x1)
#define   C_008040_WAIT_CMDFIFO                        0xFFFFFBFF
#define   S_008040_WAIT_2D_IDLE(x)                     (((x) & 0x1) << 14)
#define   G_008040_WAIT_2D_IDLE(x)                     (((x) >> 14) & 0x1)
#define   C_008040_WAIT_2D_IDLE                        0xFFFFBFFF
#define   S_008040_WAIT_3D_IDLE(x)                     (((x) & 0x1) << 15)
#define   G_008040_WAIT_3D_IDLE(x)                     (((x) >> 15) & 0x1)
#define   C_008040_WAIT_3D_IDLE                        0xFFFF7FFF
#define   S_008040_WAIT_2D_IDLECLEAN(x)                (((x) & 0x1) << 16)
#define   G_008040_WAIT_2D_IDLECLEAN(x)                (((x) >> 16) & 0x1)
#define   C_008040_WAIT_2D_IDLECLEAN                   0xFFFEFFFF
#define   S_008040_WAIT_3D_IDLECLEAN(x)                (((x) & 0x1) << 17)
#define   G_008040_WAIT_3D_IDLECLEAN(x)                (((x) >> 17) & 0x1)
#define   C_008040_WAIT_3D_IDLECLEAN                   0xFFFDFFFF
#define   S_008040_WAIT_EXTERN_SIG(x)                  (((x) & 0x1) << 19)
#define   G_008040_WAIT_EXTERN_SIG(x)                  (((x) >> 19) & 0x1)
#define   C_008040_WAIT_EXTERN_SIG                     0xFFF7FFFF
#define   S_008040_CMDFIFO_ENTRIES(x)                  (((x) & 0x1F) << 20)
#define   G_008040_CMDFIFO_ENTRIES(x)                  (((x) >> 20) & 0x1F)
#define   C_008040_CMDFIFO_ENTRIES                     0xFE0FFFFF
#define R_0286CC_SPI_PS_IN_CONTROL_0                 0x0286CC
#define   S_0286CC_NUM_INTERP(x)                       (((x) & 0x3F) << 0)
#define   G_0286CC_NUM_INTERP(x)                       (((x) >> 0) & 0x3F)
#define   C_0286CC_NUM_INTERP                          0xFFFFFFC0
#define   S_0286CC_POSITION_ENA(x)                     (((x) & 0x1) << 8)
#define   G_0286CC_POSITION_ENA(x)                     (((x) >> 8) & 0x1)
#define   C_0286CC_POSITION_ENA                        0xFFFFFEFF
#define   S_0286CC_POSITION_CENTROID(x)                (((x) & 0x1) << 9)
#define   G_0286CC_POSITION_CENTROID(x)                (((x) >> 9) & 0x1)
#define   C_0286CC_POSITION_CENTROID                   0xFFFFFDFF
#define   S_0286CC_POSITION_ADDR(x)                    (((x) & 0x1F) << 10)
#define   G_0286CC_POSITION_ADDR(x)                    (((x) >> 10) & 0x1F)
#define   C_0286CC_POSITION_ADDR                       0xFFFF83FF
#define   S_0286CC_PARAM_GEN(x)                        (((x) & 0xF) << 15)
#define   G_0286CC_PARAM_GEN(x)                        (((x) >> 15) & 0xF)
#define   C_0286CC_PARAM_GEN                           0xFFF87FFF
#define   S_0286CC_PARAM_GEN_ADDR(x)                   (((x) & 0x7F) << 19)
#define   G_0286CC_PARAM_GEN_ADDR(x)                   (((x) >> 19) & 0x7F)
#define   C_0286CC_PARAM_GEN_ADDR                      0xFC07FFFF
#define   S_0286CC_BARYC_SAMPLE_CNTL(x)                (((x) & 0x3) << 26)
#define   G_0286CC_BARYC_SAMPLE_CNTL(x)                (((x) >> 26) & 0x3)
#define   C_0286CC_BARYC_SAMPLE_CNTL                   0xF3FFFFFF
#define   S_0286CC_PERSP_GRADIENT_ENA(x)               (((x) & 0x1) << 28)
#define   G_0286CC_PERSP_GRADIENT_ENA(x)               (((x) >> 28) & 0x1)
#define   C_0286CC_PERSP_GRADIENT_ENA                  0xEFFFFFFF
#define   S_0286CC_LINEAR_GRADIENT_ENA(x)              (((x) & 0x1) << 29)
#define   G_0286CC_LINEAR_GRADIENT_ENA(x)              (((x) >> 29) & 0x1)
#define   C_0286CC_LINEAR_GRADIENT_ENA                 0xDFFFFFFF
#define   S_0286CC_POSITION_SAMPLE(x)                  (((x) & 0x1) << 30)
#define   G_0286CC_POSITION_SAMPLE(x)                  (((x) >> 30) & 0x1)
#define   C_0286CC_POSITION_SAMPLE                     0xBFFFFFFF
#define   S_0286CC_BARYC_AT_SAMPLE_ENA(x)              (((x) & 0x1) << 31)
#define   G_0286CC_BARYC_AT_SAMPLE_ENA(x)              (((x) >> 31) & 0x1)
#define   C_0286CC_BARYC_AT_SAMPLE_ENA                 0x7FFFFFFF
#define R_0286D0_SPI_PS_IN_CONTROL_1                 0x0286D0
#define   S_0286D0_GEN_INDEX_PIX(x)                    (((x) & 0x1) << 0)
#define   G_0286D0_GEN_INDEX_PIX(x)                    (((x) >> 0) & 0x1)
#define   C_0286D0_GEN_INDEX_PIX                       0xFFFFFFFE
#define   S_0286D0_GEN_INDEX_PIX_ADDR(x)               (((x) & 0x7F) << 1)
#define   G_0286D0_GEN_INDEX_PIX_ADDR(x)               (((x) >> 1) & 0x7F)
#define   C_0286D0_GEN_INDEX_PIX_ADDR                  0xFFFFFF01
#define   S_0286D0_FRONT_FACE_ENA(x)                   (((x) & 0x1) << 8)
#define   G_0286D0_FRONT_FACE_ENA(x)                   (((x) >> 8) & 0x1)
#define   C_0286D0_FRONT_FACE_ENA                      0xFFFFFEFF
#define   S_0286D0_FRONT_FACE_CHAN(x)                  (((x) & 0x3) << 9)
#define   G_0286D0_FRONT_FACE_CHAN(x)                  (((x) >> 9) & 0x3)
#define   C_0286D0_FRONT_FACE_CHAN                     0xFFFFF9FF
#define   S_0286D0_FRONT_FACE_ALL_BITS(x)              (((x) & 0x1) << 11)
#define   G_0286D0_FRONT_FACE_ALL_BITS(x)              (((x) >> 11) & 0x1)
#define   C_0286D0_FRONT_FACE_ALL_BITS                 0xFFFFF7FF
#define   S_0286D0_FRONT_FACE_ADDR(x)                  (((x) & 0x1F) << 12)
#define   G_0286D0_FRONT_FACE_ADDR(x)                  (((x) >> 12) & 0x1F)
#define   C_0286D0_FRONT_FACE_ADDR                     0xFFFE0FFF
#define   S_0286D0_FOG_ADDR(x)                         (((x) & 0x7F) << 17)
#define   G_0286D0_FOG_ADDR(x)                         (((x) >> 17) & 0x7F)
#define   C_0286D0_FOG_ADDR                            0xFF01FFFF
#define   S_0286D0_FIXED_PT_POSITION_ENA(x)            (((x) & 0x1) << 24)
#define   G_0286D0_FIXED_PT_POSITION_ENA(x)            (((x) >> 24) & 0x1)
#define   C_0286D0_FIXED_PT_POSITION_ENA               0xFEFFFFFF
#define   S_0286D0_FIXED_PT_POSITION_ADDR(x)           (((x) & 0x1F) << 25)
#define   G_0286D0_FIXED_PT_POSITION_ADDR(x)           (((x) >> 25) & 0x1F)
#define   C_0286D0_FIXED_PT_POSITION_ADDR              0xC1FFFFFF
#define R_0286C4_SPI_VS_OUT_CONFIG                   0x0286C4
#define   S_0286C4_VS_PER_COMPONENT(x)                 (((x) & 0x1) << 0)
#define   G_0286C4_VS_PER_COMPONENT(x)                 (((x) >> 0) & 0x1)
#define   C_0286C4_VS_PER_COMPONENT                    0xFFFFFFFE
#define   S_0286C4_VS_EXPORT_COUNT(x)                  (((x) & 0x1F) << 1)
#define   G_0286C4_VS_EXPORT_COUNT(x)                  (((x) >> 1) & 0x1F)
#define   C_0286C4_VS_EXPORT_COUNT                     0xFFFFFFC1
#define   S_0286C4_VS_EXPORTS_FOG(x)                   (((x) & 0x1) << 8)
#define   G_0286C4_VS_EXPORTS_FOG(x)                   (((x) >> 8) & 0x1)
#define   C_0286C4_VS_EXPORTS_FOG                      0xFFFFFEFF
#define   S_0286C4_VS_OUT_FOG_VEC_ADDR(x)              (((x) & 0x1F) << 9)
#define   G_0286C4_VS_OUT_FOG_VEC_ADDR(x)              (((x) >> 9) & 0x1F)
#define   C_0286C4_VS_OUT_FOG_VEC_ADDR                 0xFFFFC1FF
#define R_028240_PA_SC_GENERIC_SCISSOR_TL            0x028240
#define   S_028240_TL_X(x)                             (((x) & 0x3FFF) << 0)
#define   G_028240_TL_X(x)                             (((x) >> 0) & 0x3FFF)
#define   C_028240_TL_X                                0xFFFFC000
#define   S_028240_TL_Y(x)                             (((x) & 0x3FFF) << 16)
#define   G_028240_TL_Y(x)                             (((x) >> 16) & 0x3FFF)
#define   C_028240_TL_Y                                0xC000FFFF
#define   S_028240_WINDOW_OFFSET_DISABLE(x)            (((x) & 0x1) << 31)
#define   G_028240_WINDOW_OFFSET_DISABLE(x)            (((x) >> 31) & 0x1)
#define   C_028240_WINDOW_OFFSET_DISABLE               0x7FFFFFFF
#define R_028244_PA_SC_GENERIC_SCISSOR_BR            0x028244
#define   S_028244_BR_X(x)                             (((x) & 0x3FFF) << 0)
#define   G_028244_BR_X(x)                             (((x) >> 0) & 0x3FFF)
#define   C_028244_BR_X                                0xFFFFC000
#define   S_028244_BR_Y(x)                             (((x) & 0x3FFF) << 16)
#define   G_028244_BR_Y(x)                             (((x) >> 16) & 0x3FFF)
#define   C_028244_BR_Y                                0xC000FFFF
#define R_028030_PA_SC_SCREEN_SCISSOR_TL             0x028030
#define   S_028030_TL_X(x)                             (((x) & 0x7FFF) << 0)
#define   G_028030_TL_X(x)                             (((x) >> 0) & 0x7FFF)
#define   C_028030_TL_X                                0xFFFF8000
#define   S_028030_TL_Y(x)                             (((x) & 0x7FFF) << 16)
#define   G_028030_TL_Y(x)                             (((x) >> 16) & 0x7FFF)
#define   C_028030_TL_Y                                0x8000FFFF
#define R_028034_PA_SC_SCREEN_SCISSOR_BR             0x028034
#define   S_028034_BR_X(x)                             (((x) & 0x7FFF) << 0)
#define   G_028034_BR_X(x)                             (((x) >> 0) & 0x7FFF)
#define   C_028034_BR_X                                0xFFFF8000
#define   S_028034_BR_Y(x)                             (((x) & 0x7FFF) << 16)
#define   G_028034_BR_Y(x)                             (((x) >> 16) & 0x7FFF)
#define   C_028034_BR_Y                                0x8000FFFF
#define R_028204_PA_SC_WINDOW_SCISSOR_TL             0x028204
#define   S_028204_TL_X(x)                             (((x) & 0x3FFF) << 0)
#define   G_028204_TL_X(x)                             (((x) >> 0) & 0x3FFF)
#define   C_028204_TL_X                                0xFFFFC000
#define   S_028204_TL_Y(x)                             (((x) & 0x3FFF) << 16)
#define   G_028204_TL_Y(x)                             (((x) >> 16) & 0x3FFF)
#define   C_028204_TL_Y                                0xC000FFFF
#define   S_028204_WINDOW_OFFSET_DISABLE(x)            (((x) & 0x1) << 31)
#define   G_028204_WINDOW_OFFSET_DISABLE(x)            (((x) >> 31) & 0x1)
#define   C_028204_WINDOW_OFFSET_DISABLE               0x7FFFFFFF
#define R_028208_PA_SC_WINDOW_SCISSOR_BR             0x028208
#define   S_028208_BR_X(x)                             (((x) & 0x3FFF) << 0)
#define   G_028208_BR_X(x)                             (((x) >> 0) & 0x3FFF)
#define   C_028208_BR_X                                0xFFFFC000
#define   S_028208_BR_Y(x)                             (((x) & 0x3FFF) << 16)
#define   G_028208_BR_Y(x)                             (((x) >> 16) & 0x3FFF)
#define   C_028208_BR_Y                                0xC000FFFF
#define R_0287F0_VGT_DRAW_INITIATOR                  0x0287F0
#define   S_0287F0_SOURCE_SELECT(x)                    (((x) & 0x3) << 0)
#define   G_0287F0_SOURCE_SELECT(x)                    (((x) >> 0) & 0x3)
#define   C_0287F0_SOURCE_SELECT                       0xFFFFFFFC
#define   S_0287F0_MAJOR_MODE(x)                       (((x) & 0x3) << 2)
#define   G_0287F0_MAJOR_MODE(x)                       (((x) >> 2) & 0x3)
#define   C_0287F0_MAJOR_MODE                          0xFFFFFFF3
#define   S_0287F0_SPRITE_EN(x)                        (((x) & 0x1) << 4)
#define   G_0287F0_SPRITE_EN(x)                        (((x) >> 4) & 0x1)
#define   C_0287F0_SPRITE_EN                           0xFFFFFFEF
#define   S_0287F0_NOT_EOP(x)                          (((x) & 0x1) << 5)
#define   G_0287F0_NOT_EOP(x)                          (((x) >> 5) & 0x1)
#define   C_0287F0_NOT_EOP                             0xFFFFFFDF
#define   S_0287F0_USE_OPAQUE(x)                       (((x) & 0x1) << 6)
#define   G_0287F0_USE_OPAQUE(x)                       (((x) >> 6) & 0x1)
#define   C_0287F0_USE_OPAQUE                          0xFFFFFFBF
#define R_038000_SQ_TEX_RESOURCE_WORD0_0             0x038000
#define   S_038000_DIM(x)                              (((x) & 0x7) << 0)
#define   G_038000_DIM(x)                              (((x) >> 0) & 0x7)
#define   C_038000_DIM                                 0xFFFFFFF8
#define     V_038000_SQ_TEX_DIM_1D                     0x00000000
#define     V_038000_SQ_TEX_DIM_2D                     0x00000001
#define     V_038000_SQ_TEX_DIM_3D                     0x00000002
#define     V_038000_SQ_TEX_DIM_CUBEMAP                0x00000003
#define     V_038000_SQ_TEX_DIM_1D_ARRAY               0x00000004
#define     V_038000_SQ_TEX_DIM_2D_ARRAY               0x00000005
#define     V_038000_SQ_TEX_DIM_2D_MSAA                0x00000006
#define     V_038000_SQ_TEX_DIM_2D_ARRAY_MSAA          0x00000007
#define   S_038000_TILE_MODE(x)                        (((x) & 0xF) << 3)
#define   G_038000_TILE_MODE(x)                        (((x) >> 3) & 0xF)
#define   C_038000_TILE_MODE                           0xFFFFFF87
#define   S_038000_TILE_TYPE(x)                        (((x) & 0x1) << 7)
#define   G_038000_TILE_TYPE(x)                        (((x) >> 7) & 0x1)
#define   C_038000_TILE_TYPE                           0xFFFFFF7F
#define   S_038000_PITCH(x)                            (((x) & 0x7FF) << 8)
#define   G_038000_PITCH(x)                            (((x) >> 8) & 0x7FF)
#define   C_038000_PITCH                               0xFFF800FF
#define   S_038000_TEX_WIDTH(x)                        (((x) & 0x1FFF) << 19)
#define   G_038000_TEX_WIDTH(x)                        (((x) >> 19) & 0x1FFF)
#define   C_038000_TEX_WIDTH                           0x0007FFFF
#define R_038004_SQ_TEX_RESOURCE_WORD1_0             0x038004
#define   S_038004_TEX_HEIGHT(x)                       (((x) & 0x1FFF) << 0)
#define   G_038004_TEX_HEIGHT(x)                       (((x) >> 0) & 0x1FFF)
#define   C_038004_TEX_HEIGHT                          0xFFFFE000
#define   S_038004_TEX_DEPTH(x)                        (((x) & 0x1FFF) << 13)
#define   G_038004_TEX_DEPTH(x)                        (((x) >> 13) & 0x1FFF)
#define   C_038004_TEX_DEPTH                           0xFC001FFF
#define   S_038004_DATA_FORMAT(x)                      (((x) & 0x3F) << 26)
#define   G_038004_DATA_FORMAT(x)                      (((x) >> 26) & 0x3F)
#define   C_038004_DATA_FORMAT                         0x03FFFFFF
#define R_038008_SQ_TEX_RESOURCE_WORD2_0             0x038008
#define   S_038008_BASE_ADDRESS(x)                     (((x) & 0xFFFFFFFF) << 0)
#define   G_038008_BASE_ADDRESS(x)                     (((x) >> 0) & 0xFFFFFFFF)
#define   C_038008_BASE_ADDRESS                        0x00000000
#define R_03800C_SQ_TEX_RESOURCE_WORD3_0             0x03800C
#define   S_03800C_MIP_ADDRESS(x)                      (((x) & 0xFFFFFFFF) << 0)
#define   G_03800C_MIP_ADDRESS(x)                      (((x) >> 0) & 0xFFFFFFFF)
#define   C_03800C_MIP_ADDRESS                         0x00000000
#define R_038010_SQ_TEX_RESOURCE_WORD4_0             0x038010
#define   S_038010_FORMAT_COMP_X(x)                    (((x) & 0x3) << 0)
#define   G_038010_FORMAT_COMP_X(x)                    (((x) >> 0) & 0x3)
#define   C_038010_FORMAT_COMP_X                       0xFFFFFFFC
#define     V_038010_SQ_FORMAT_COMP_UNSIGNED           0x00000000
#define     V_038010_SQ_FORMAT_COMP_SIGNED             0x00000001
#define     V_038010_SQ_FORMAT_COMP_UNSIGNED_BIASED    0x00000002
#define   S_038010_FORMAT_COMP_Y(x)                    (((x) & 0x3) << 2)
#define   G_038010_FORMAT_COMP_Y(x)                    (((x) >> 2) & 0x3)
#define   C_038010_FORMAT_COMP_Y                       0xFFFFFFF3
#define   S_038010_FORMAT_COMP_Z(x)                    (((x) & 0x3) << 4)
#define   G_038010_FORMAT_COMP_Z(x)                    (((x) >> 4) & 0x3)
#define   C_038010_FORMAT_COMP_Z                       0xFFFFFFCF
#define   S_038010_FORMAT_COMP_W(x)                    (((x) & 0x3) << 6)
#define   G_038010_FORMAT_COMP_W(x)                    (((x) >> 6) & 0x3)
#define   C_038010_FORMAT_COMP_W                       0xFFFFFF3F
#define   S_038010_NUM_FORMAT_ALL(x)                   (((x) & 0x3) << 8)
#define   G_038010_NUM_FORMAT_ALL(x)                   (((x) >> 8) & 0x3)
#define   C_038010_NUM_FORMAT_ALL                      0xFFFFFCFF
#define     V_038010_SQ_NUM_FORMAT_NORM                0x00000000
#define     V_038010_SQ_NUM_FORMAT_INT                 0x00000001
#define     V_038010_SQ_NUM_FORMAT_SCALED              0x00000002
#define   S_038010_SRF_MODE_ALL(x)                     (((x) & 0x1) << 10)
#define   G_038010_SRF_MODE_ALL(x)                     (((x) >> 10) & 0x1)
#define   C_038010_SRF_MODE_ALL                        0xFFFFFBFF
#define     V_038010_SFR_MODE_ZERO_CLAMP_MINUS_ONE     0x00000000
#define     V_038010_SFR_MODE_NO_ZERO                  0x00000001
#define   S_038010_FORCE_DEGAMMA(x)                    (((x) & 0x1) << 11)
#define   G_038010_FORCE_DEGAMMA(x)                    (((x) >> 11) & 0x1)
#define   C_038010_FORCE_DEGAMMA                       0xFFFFF7FF
#define   S_038010_ENDIAN_SWAP(x)                      (((x) & 0x3) << 12)
#define   G_038010_ENDIAN_SWAP(x)                      (((x) >> 12) & 0x3)
#define   C_038010_ENDIAN_SWAP                         0xFFFFCFFF
#define   S_038010_REQUEST_SIZE(x)                     (((x) & 0x3) << 14)
#define   G_038010_REQUEST_SIZE(x)                     (((x) >> 14) & 0x3)
#define   C_038010_REQUEST_SIZE                        0xFFFF3FFF
#define   S_038010_DST_SEL_X(x)                        (((x) & 0x7) << 16)
#define   G_038010_DST_SEL_X(x)                        (((x) >> 16) & 0x7)
#define   C_038010_DST_SEL_X                           0xFFF8FFFF
#define     V_038010_SQ_SEL_X                          0x00000000
#define     V_038010_SQ_SEL_Y                          0x00000001
#define     V_038010_SQ_SEL_Z                          0x00000002
#define     V_038010_SQ_SEL_W                          0x00000003
#define     V_038010_SQ_SEL_0                          0x00000004
#define     V_038010_SQ_SEL_1                          0x00000005
#define   S_038010_DST_SEL_Y(x)                        (((x) & 0x7) << 19)
#define   G_038010_DST_SEL_Y(x)                        (((x) >> 19) & 0x7)
#define   C_038010_DST_SEL_Y                           0xFFC7FFFF
#define   S_038010_DST_SEL_Z(x)                        (((x) & 0x7) << 22)
#define   G_038010_DST_SEL_Z(x)                        (((x) >> 22) & 0x7)
#define   C_038010_DST_SEL_Z                           0xFE3FFFFF
#define   S_038010_DST_SEL_W(x)                        (((x) & 0x7) << 25)
#define   G_038010_DST_SEL_W(x)                        (((x) >> 25) & 0x7)
#define   C_038010_DST_SEL_W                           0xF1FFFFFF
#define   S_038010_BASE_LEVEL(x)                       (((x) & 0xF) << 28)
#define   G_038010_BASE_LEVEL(x)                       (((x) >> 28) & 0xF)
#define   C_038010_BASE_LEVEL                          0x0FFFFFFF
#define R_038014_SQ_TEX_RESOURCE_WORD5_0             0x038014
#define   S_038014_LAST_LEVEL(x)                       (((x) & 0xF) << 0)
#define   G_038014_LAST_LEVEL(x)                       (((x) >> 0) & 0xF)
#define   C_038014_LAST_LEVEL                          0xFFFFFFF0
#define   S_038014_BASE_ARRAY(x)                       (((x) & 0x1FFF) << 4)
#define   G_038014_BASE_ARRAY(x)                       (((x) >> 4) & 0x1FFF)
#define   C_038014_BASE_ARRAY                          0xFFFE000F
#define   S_038014_LAST_ARRAY(x)                       (((x) & 0x1FFF) << 17)
#define   G_038014_LAST_ARRAY(x)                       (((x) >> 17) & 0x1FFF)
#define   C_038014_LAST_ARRAY                          0xC001FFFF
#define R_038018_SQ_TEX_RESOURCE_WORD6_0             0x038018
#define   S_038018_MPEG_CLAMP(x)                       (((x) & 0x3) << 0)
#define   G_038018_MPEG_CLAMP(x)                       (((x) >> 0) & 0x3)
#define   C_038018_MPEG_CLAMP                          0xFFFFFFFC
#define   S_038018_PERF_MODULATION(x)                  (((x) & 0x7) << 5)
#define   G_038018_PERF_MODULATION(x)                  (((x) >> 5) & 0x7)
#define   C_038018_PERF_MODULATION                     0xFFFFFF1F
#define   S_038018_INTERLACED(x)                       (((x) & 0x1) << 8)
#define   G_038018_INTERLACED(x)                       (((x) >> 8) & 0x1)
#define   C_038018_INTERLACED                          0xFFFFFEFF
#define   S_038018_TYPE(x)                             (((x) & 0x3) << 30)
#define   G_038018_TYPE(x)                             (((x) >> 30) & 0x3)
#define   C_038018_TYPE                                0x3FFFFFFF
#define     V_038010_SQ_TEX_VTX_INVALID_TEXTURE        0x00000000
#define     V_038010_SQ_TEX_VTX_INVALID_BUFFER         0x00000001
#define     V_038010_SQ_TEX_VTX_VALID_TEXTURE          0x00000002
#define     V_038010_SQ_TEX_VTX_VALID_BUFFER           0x00000003
#define R_038008_SQ_VTX_CONSTANT_WORD2_0             0x038008
#define   S_038008_BASE_ADDRESS_HI(x)                  (((x) & 0xFF) << 0)
#define   G_038008_BASE_ADDRESS_HI(x)                  (((x) >> 0) & 0xFF)
#define   C_038008_BASE_ADDRESS_HI                     0xFFFFFF00
#define   S_038008_STRIDE(x)                           (((x) & 0x7FF) << 8)
#define   G_038008_STRIDE(x)                           (((x) >> 8) & 0x7FF)
#define   C_038008_STRIDE                              0xFFF800FF
#define   S_038008_CLAMP_X(x)                          (((x) & 0x1) << 19)
#define   G_038008_CLAMP_X(x)                          (((x) >> 19) & 0x1)
#define   C_038008_CLAMP_X                             0xFFF7FFFF
#define   S_038008_DATA_FORMAT(x)                      (((x) & 0x3F) << 20)
#define   G_038008_DATA_FORMAT(x)                      (((x) >> 20) & 0x3F)
#define   C_038008_DATA_FORMAT                         0xFC0FFFFF
#define     V_038008_COLOR_INVALID                     0x00000000
#define     V_038008_COLOR_8                           0x00000001
#define     V_038008_COLOR_4_4                         0x00000002
#define     V_038008_COLOR_3_3_2                       0x00000003
#define     V_038008_COLOR_16                          0x00000005
#define     V_038008_COLOR_16_FLOAT                    0x00000006
#define     V_038008_COLOR_8_8                         0x00000007
#define     V_038008_COLOR_5_6_5                       0x00000008
#define     V_038008_COLOR_6_5_5                       0x00000009
#define     V_038008_COLOR_1_5_5_5                     0x0000000A
#define     V_038008_COLOR_4_4_4_4                     0x0000000B
#define     V_038008_COLOR_5_5_5_1                     0x0000000C
#define     V_038008_COLOR_32                          0x0000000D
#define     V_038008_COLOR_32_FLOAT                    0x0000000E
#define     V_038008_COLOR_16_16                       0x0000000F
#define     V_038008_COLOR_16_16_FLOAT                 0x00000010
#define     V_038008_COLOR_8_24                        0x00000011
#define     V_038008_COLOR_8_24_FLOAT                  0x00000012
#define     V_038008_COLOR_24_8                        0x00000013
#define     V_038008_COLOR_24_8_FLOAT                  0x00000014
#define     V_038008_COLOR_10_11_11                    0x00000015
#define     V_038008_COLOR_10_11_11_FLOAT              0x00000016
#define     V_038008_COLOR_11_11_10                    0x00000017
#define     V_038008_COLOR_11_11_10_FLOAT              0x00000018
#define     V_038008_COLOR_2_10_10_10                  0x00000019
#define     V_038008_COLOR_8_8_8_8                     0x0000001A
#define     V_038008_COLOR_10_10_10_2                  0x0000001B
#define     V_038008_COLOR_X24_8_32_FLOAT              0x0000001C
#define     V_038008_COLOR_32_32                       0x0000001D
#define     V_038008_COLOR_32_32_FLOAT                 0x0000001E
#define     V_038008_COLOR_16_16_16_16                 0x0000001F
#define     V_038008_COLOR_16_16_16_16_FLOAT           0x00000020
#define     V_038008_COLOR_32_32_32_32                 0x00000022
#define     V_038008_COLOR_32_32_32_32_FLOAT           0x00000023
#define   S_038008_NUM_FORMAT_ALL(x)                   (((x) & 0x3) << 26)
#define   G_038008_NUM_FORMAT_ALL(x)                   (((x) >> 26) & 0x3)
#define   C_038008_NUM_FORMAT_ALL                      0xF3FFFFFF
#define   S_038008_FORMAT_COMP_ALL(x)                  (((x) & 0x1) << 28)
#define   G_038008_FORMAT_COMP_ALL(x)                  (((x) >> 28) & 0x1)
#define   C_038008_FORMAT_COMP_ALL                     0xEFFFFFFF
#define   S_038008_SRF_MODE_ALL(x)                     (((x) & 0x1) << 29)
#define   G_038008_SRF_MODE_ALL(x)                     (((x) >> 29) & 0x1)
#define   C_038008_SRF_MODE_ALL                        0xDFFFFFFF
#define   S_038008_ENDIAN_SWAP(x)                      (((x) & 0x3) << 30)
#define   G_038008_ENDIAN_SWAP(x)                      (((x) >> 30) & 0x3)
#define   C_038008_ENDIAN_SWAP                         0x3FFFFFFF
#define R_03C000_SQ_TEX_SAMPLER_WORD0_0              0x03C000
#define   S_03C000_CLAMP_X(x)                          (((x) & 0x7) << 0)
#define   G_03C000_CLAMP_X(x)                          (((x) >> 0) & 0x7)
#define   C_03C000_CLAMP_X                             0xFFFFFFF8
#define     V_03C000_SQ_TEX_WRAP                       0x00000000
#define     V_03C000_SQ_TEX_MIRROR                     0x00000001
#define     V_03C000_SQ_TEX_CLAMP_LAST_TEXEL           0x00000002
#define     V_03C000_SQ_TEX_MIRROR_ONCE_LAST_TEXEL     0x00000003
#define     V_03C000_SQ_TEX_CLAMP_HALF_BORDER          0x00000004
#define     V_03C000_SQ_TEX_MIRROR_ONCE_HALF_BORDER    0x00000005
#define     V_03C000_SQ_TEX_CLAMP_BORDER               0x00000006
#define     V_03C000_SQ_TEX_MIRROR_ONCE_BORDER         0x00000007
#define   S_03C000_CLAMP_Y(x)                          (((x) & 0x7) << 3)
#define   G_03C000_CLAMP_Y(x)                          (((x) >> 3) & 0x7)
#define   C_03C000_CLAMP_Y                             0xFFFFFFC7
#define   S_03C000_CLAMP_Z(x)                          (((x) & 0x7) << 6)
#define   G_03C000_CLAMP_Z(x)                          (((x) >> 6) & 0x7)
#define   C_03C000_CLAMP_Z                             0xFFFFFE3F
#define   S_03C000_XY_MAG_FILTER(x)                    (((x) & 0x7) << 9)
#define   G_03C000_XY_MAG_FILTER(x)                    (((x) >> 9) & 0x7)
#define   C_03C000_XY_MAG_FILTER                       0xFFFFF1FF
#define     V_03C000_SQ_TEX_XY_FILTER_POINT            0x00000000
#define     V_03C000_SQ_TEX_XY_FILTER_BILINEAR         0x00000001
#define     V_03C000_SQ_TEX_XY_FILTER_BICUBIC          0x00000002
#define   S_03C000_XY_MIN_FILTER(x)                    (((x) & 0x7) << 12)
#define   G_03C000_XY_MIN_FILTER(x)                    (((x) >> 12) & 0x7)
#define   C_03C000_XY_MIN_FILTER                       0xFFFF8FFF
#define   S_03C000_Z_FILTER(x)                         (((x) & 0x3) << 15)
#define   G_03C000_Z_FILTER(x)                         (((x) >> 15) & 0x3)
#define   C_03C000_Z_FILTER                            0xFFFE7FFF
#define     V_03C000_SQ_TEX_Z_FILTER_NONE              0x00000000
#define     V_03C000_SQ_TEX_Z_FILTER_POINT             0x00000001
#define     V_03C000_SQ_TEX_Z_FILTER_LINEAR            0x00000002
#define   S_03C000_MIP_FILTER(x)                       (((x) & 0x3) << 17)
#define   G_03C000_MIP_FILTER(x)                       (((x) >> 17) & 0x3)
#define   C_03C000_MIP_FILTER                          0xFFF9FFFF
#define   S_03C000_BORDER_COLOR_TYPE(x)                (((x) & 0x3) << 22)
#define   G_03C000_BORDER_COLOR_TYPE(x)                (((x) >> 22) & 0x3)
#define   C_03C000_BORDER_COLOR_TYPE                   0xFF3FFFFF
#define     V_03C000_SQ_TEX_BORDER_COLOR_TRANS_BLACK   0x00000000
#define     V_03C000_SQ_TEX_BORDER_COLOR_OPAQUE_BLACK  0x00000001
#define     V_03C000_SQ_TEX_BORDER_COLOR_OPAQUE_WHITE  0x00000002
#define     V_03C000_SQ_TEX_BORDER_COLOR_REGISTER      0x00000003
#define   S_03C000_POINT_SAMPLING_CLAMP(x)             (((x) & 0x1) << 24)
#define   G_03C000_POINT_SAMPLING_CLAMP(x)             (((x) >> 24) & 0x1)
#define   C_03C000_POINT_SAMPLING_CLAMP                0xFEFFFFFF
#define   S_03C000_TEX_ARRAY_OVERRIDE(x)               (((x) & 0x1) << 25)
#define   G_03C000_TEX_ARRAY_OVERRIDE(x)               (((x) >> 25) & 0x1)
#define   C_03C000_TEX_ARRAY_OVERRIDE                  0xFDFFFFFF
#define   S_03C000_DEPTH_COMPARE_FUNCTION(x)           (((x) & 0x7) << 26)
#define   G_03C000_DEPTH_COMPARE_FUNCTION(x)           (((x) >> 26) & 0x7)
#define   C_03C000_DEPTH_COMPARE_FUNCTION              0xE3FFFFFF
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_NEVER        0x00000000
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_LESS         0x00000001
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_EQUAL        0x00000002
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_LESSEQUAL    0x00000003
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_GREATER      0x00000004
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_NOTEQUAL     0x00000005
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_GREATEREQUAL 0x00000006
#define     V_03C000_SQ_TEX_DEPTH_COMPARE_ALWAYS       0x00000007
#define   S_03C000_CHROMA_KEY(x)                       (((x) & 0x3) << 29)
#define   G_03C000_CHROMA_KEY(x)                       (((x) >> 29) & 0x3)
#define   C_03C000_CHROMA_KEY                          0x9FFFFFFF
#define     V_03C000_SQ_TEX_CHROMA_KEY_DISABLE         0x00000000
#define     V_03C000_SQ_TEX_CHROMA_KEY_KILL            0x00000001
#define     V_03C000_SQ_TEX_CHROMA_KEY_BLEND           0x00000002
#define   S_03C000_LOD_USES_MINOR_AXIS(x)              (((x) & 0x1) << 31)
#define   G_03C000_LOD_USES_MINOR_AXIS(x)              (((x) >> 31) & 0x1)
#define   C_03C000_LOD_USES_MINOR_AXIS                 0x7FFFFFFF
#define R_03C004_SQ_TEX_SAMPLER_WORD1_0              0x03C004
#define   S_03C004_MIN_LOD(x)                          (((x) & 0x3FF) << 0)
#define   G_03C004_MIN_LOD(x)                          (((x) >> 0) & 0x3FF)
#define   C_03C004_MIN_LOD                             0xFFFFFC00
#define   S_03C004_MAX_LOD(x)                          (((x) & 0x3FF) << 10)
#define   G_03C004_MAX_LOD(x)                          (((x) >> 10) & 0x3FF)
#define   C_03C004_MAX_LOD                             0xFFF003FF
#define   S_03C004_LOD_BIAS(x)                         (((x) & 0xFFF) << 20)
#define   G_03C004_LOD_BIAS(x)                         (((x) >> 20) & 0xFFF)
#define   C_03C004_LOD_BIAS                            0x000FFFFF
#define R_03C008_SQ_TEX_SAMPLER_WORD2_0              0x03C008
#define   S_03C008_LOD_BIAS_SEC(x)                     (((x) & 0xFFF) << 0)
#define   G_03C008_LOD_BIAS_SEC(x)                     (((x) >> 0) & 0xFFF)
#define   C_03C008_LOD_BIAS_SEC                        0xFFFFF000
#define   S_03C008_MC_COORD_TRUNCATE(x)                (((x) & 0x1) << 12)
#define   G_03C008_MC_COORD_TRUNCATE(x)                (((x) >> 12) & 0x1)
#define   C_03C008_MC_COORD_TRUNCATE                   0xFFFFEFFF
#define   S_03C008_FORCE_DEGAMMA(x)                    (((x) & 0x1) << 13)
#define   G_03C008_FORCE_DEGAMMA(x)                    (((x) >> 13) & 0x1)
#define   C_03C008_FORCE_DEGAMMA                       0xFFFFDFFF
#define   S_03C008_HIGH_PRECISION_FILTER(x)            (((x) & 0x1) << 14)
#define   G_03C008_HIGH_PRECISION_FILTER(x)            (((x) >> 14) & 0x1)
#define   C_03C008_HIGH_PRECISION_FILTER               0xFFFFBFFF
#define   S_03C008_PERF_MIP(x)                         (((x) & 0x7) << 15)
#define   G_03C008_PERF_MIP(x)                         (((x) >> 15) & 0x7)
#define   C_03C008_PERF_MIP                            0xFFFC7FFF
#define   S_03C008_PERF_Z(x)                           (((x) & 0x3) << 18)
#define   G_03C008_PERF_Z(x)                           (((x) >> 18) & 0x3)
#define   C_03C008_PERF_Z                              0xFFF3FFFF
#define   S_03C008_FETCH_4(x)                          (((x) & 0x1) << 26)
#define   G_03C008_FETCH_4(x)                          (((x) >> 26) & 0x1)
#define   C_03C008_FETCH_4                             0xFBFFFFFF
#define   S_03C008_SAMPLE_IS_PCF(x)                    (((x) & 0x1) << 27)
#define   G_03C008_SAMPLE_IS_PCF(x)                    (((x) >> 27) & 0x1)
#define   C_03C008_SAMPLE_IS_PCF                       0xF7FFFFFF
#define   S_03C008_TYPE(x)                             (((x) & 0x1) << 31)
#define   G_03C008_TYPE(x)                             (((x) >> 31) & 0x1)
#define   C_03C008_TYPE                                0x7FFFFFFF
#define R_008958_VGT_PRIMITIVE_TYPE                  0x008958
#define   S_008958_PRIM_TYPE(x)                        (((x) & 0x3F) << 0)
#define   G_008958_PRIM_TYPE(x)                        (((x) >> 0) & 0x3F)
#define   C_008958_PRIM_TYPE                           0xFFFFFFC0
#define     V_008958_DI_PT_NONE                        0x00000000
#define     V_008958_DI_PT_POINTLIST                   0x00000001
#define     V_008958_DI_PT_LINELIST                    0x00000002
#define     V_008958_DI_PT_LINESTRIP                   0x00000003
#define     V_008958_DI_PT_TRILIST                     0x00000004
#define     V_008958_DI_PT_TRIFAN                      0x00000005
#define     V_008958_DI_PT_TRISTRIP                    0x00000006
#define     V_008958_DI_PT_UNUSED_0                    0x00000007
#define     V_008958_DI_PT_UNUSED_1                    0x00000008
#define     V_008958_DI_PT_UNUSED_2                    0x00000009
#define     V_008958_DI_PT_LINELIST_ADJ                0x0000000A
#define     V_008958_DI_PT_LINESTRIP_ADJ               0x0000000B
#define     V_008958_DI_PT_TRILIST_ADJ                 0x0000000C
#define     V_008958_DI_PT_TRISTRIP_ADJ                0x0000000D
#define     V_008958_DI_PT_UNUSED_3                    0x0000000E
#define     V_008958_DI_PT_UNUSED_4                    0x0000000F
#define     V_008958_DI_PT_TRI_WITH_WFLAGS             0x00000010
#define     V_008958_DI_PT_RECTLIST                    0x00000011
#define     V_008958_DI_PT_LINELOOP                    0x00000012
#define     V_008958_DI_PT_QUADLIST                    0x00000013
#define     V_008958_DI_PT_QUADSTRIP                   0x00000014
#define     V_008958_DI_PT_POLYGON                     0x00000015
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V0        0x00000016
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V1        0x00000017
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V2        0x00000018
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V3        0x00000019
#define     V_008958_DI_PT_2D_FILL_RECT_LIST           0x0000001A
#define     V_008958_DI_PT_2D_LINE_STRIP               0x0000001B
#define     V_008958_DI_PT_2D_TRI_STRIP                0x0000001C
#define R_02881C_PA_CL_VS_OUT_CNTL                   0x02881C
#define   S_02881C_CLIP_DIST_ENA_0(x)                  (((x) & 0x1) << 0)
#define   G_02881C_CLIP_DIST_ENA_0(x)                  (((x) >> 0) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_0                     0xFFFFFFFE
#define   S_02881C_CLIP_DIST_ENA_1(x)                  (((x) & 0x1) << 1)
#define   G_02881C_CLIP_DIST_ENA_1(x)                  (((x) >> 1) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_1                     0xFFFFFFFD
#define   S_02881C_CLIP_DIST_ENA_2(x)                  (((x) & 0x1) << 2)
#define   G_02881C_CLIP_DIST_ENA_2(x)                  (((x) >> 2) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_2                     0xFFFFFFFB
#define   S_02881C_CLIP_DIST_ENA_3(x)                  (((x) & 0x1) << 3)
#define   G_02881C_CLIP_DIST_ENA_3(x)                  (((x) >> 3) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_3                     0xFFFFFFF7
#define   S_02881C_CLIP_DIST_ENA_4(x)                  (((x) & 0x1) << 4)
#define   G_02881C_CLIP_DIST_ENA_4(x)                  (((x) >> 4) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_4                     0xFFFFFFEF
#define   S_02881C_CLIP_DIST_ENA_5(x)                  (((x) & 0x1) << 5)
#define   G_02881C_CLIP_DIST_ENA_5(x)                  (((x) >> 5) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_5                     0xFFFFFFDF
#define   S_02881C_CLIP_DIST_ENA_6(x)                  (((x) & 0x1) << 6)
#define   G_02881C_CLIP_DIST_ENA_6(x)                  (((x) >> 6) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_6                     0xFFFFFFBF
#define   S_02881C_CLIP_DIST_ENA_7(x)                  (((x) & 0x1) << 7)
#define   G_02881C_CLIP_DIST_ENA_7(x)                  (((x) >> 7) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_7                     0xFFFFFF7F
#define   S_02881C_CULL_DIST_ENA_0(x)                  (((x) & 0x1) << 8)
#define   G_02881C_CULL_DIST_ENA_0(x)                  (((x) >> 8) & 0x1)
#define   C_02881C_CULL_DIST_ENA_0                     0xFFFFFEFF
#define   S_02881C_CULL_DIST_ENA_1(x)                  (((x) & 0x1) << 9)
#define   G_02881C_CULL_DIST_ENA_1(x)                  (((x) >> 9) & 0x1)
#define   C_02881C_CULL_DIST_ENA_1                     0xFFFFFDFF
#define   S_02881C_CULL_DIST_ENA_2(x)                  (((x) & 0x1) << 10)
#define   G_02881C_CULL_DIST_ENA_2(x)                  (((x) >> 10) & 0x1)
#define   C_02881C_CULL_DIST_ENA_2                     0xFFFFFBFF
#define   S_02881C_CULL_DIST_ENA_3(x)                  (((x) & 0x1) << 11)
#define   G_02881C_CULL_DIST_ENA_3(x)                  (((x) >> 11) & 0x1)
#define   C_02881C_CULL_DIST_ENA_3                     0xFFFFF7FF
#define   S_02881C_CULL_DIST_ENA_4(x)                  (((x) & 0x1) << 12)
#define   G_02881C_CULL_DIST_ENA_4(x)                  (((x) >> 12) & 0x1)
#define   C_02881C_CULL_DIST_ENA_4                     0xFFFFEFFF
#define   S_02881C_CULL_DIST_ENA_5(x)                  (((x) & 0x1) << 13)
#define   G_02881C_CULL_DIST_ENA_5(x)                  (((x) >> 13) & 0x1)
#define   C_02881C_CULL_DIST_ENA_5                     0xFFFFDFFF
#define   S_02881C_CULL_DIST_ENA_6(x)                  (((x) & 0x1) << 14)
#define   G_02881C_CULL_DIST_ENA_6(x)                  (((x) >> 14) & 0x1)
#define   C_02881C_CULL_DIST_ENA_6                     0xFFFFBFFF
#define   S_02881C_CULL_DIST_ENA_7(x)                  (((x) & 0x1) << 15)
#define   G_02881C_CULL_DIST_ENA_7(x)                  (((x) >> 15) & 0x1)
#define   C_02881C_CULL_DIST_ENA_7                     0xFFFF7FFF
#define   S_02881C_USE_VTX_POINT_SIZE(x)               (((x) & 0x1) << 16)
#define   G_02881C_USE_VTX_POINT_SIZE(x)               (((x) >> 16) & 0x1)
#define   C_02881C_USE_VTX_POINT_SIZE                  0xFFFEFFFF
#define   S_02881C_USE_VTX_EDGE_FLAG(x)                (((x) & 0x1) << 17)
#define   G_02881C_USE_VTX_EDGE_FLAG(x)                (((x) >> 17) & 0x1)
#define   C_02881C_USE_VTX_EDGE_FLAG                   0xFFFDFFFF
#define   S_02881C_USE_VTX_RENDER_TARGET_INDX(x)       (((x) & 0x1) << 18)
#define   G_02881C_USE_VTX_RENDER_TARGET_INDX(x)       (((x) >> 18) & 0x1)
#define   C_02881C_USE_VTX_RENDER_TARGET_INDX          0xFFFBFFFF
#define   S_02881C_USE_VTX_VIEWPORT_INDX(x)            (((x) & 0x1) << 19)
#define   G_02881C_USE_VTX_VIEWPORT_INDX(x)            (((x) >> 19) & 0x1)
#define   C_02881C_USE_VTX_VIEWPORT_INDX               0xFFF7FFFF
#define   S_02881C_USE_VTX_KILL_FLAG(x)                (((x) & 0x1) << 20)
#define   G_02881C_USE_VTX_KILL_FLAG(x)                (((x) >> 20) & 0x1)
#define   C_02881C_USE_VTX_KILL_FLAG                   0xFFEFFFFF
#define   S_02881C_VS_OUT_MISC_VEC_ENA(x)              (((x) & 0x1) << 21)
#define   G_02881C_VS_OUT_MISC_VEC_ENA(x)              (((x) >> 21) & 0x1)
#define   C_02881C_VS_OUT_MISC_VEC_ENA                 0xFFDFFFFF
#define   S_02881C_VS_OUT_CCDIST0_VEC_ENA(x)           (((x) & 0x1) << 22)
#define   G_02881C_VS_OUT_CCDIST0_VEC_ENA(x)           (((x) >> 22) & 0x1)
#define   C_02881C_VS_OUT_CCDIST0_VEC_ENA              0xFFBFFFFF
#define   S_02881C_VS_OUT_CCDIST1_VEC_ENA(x)           (((x) & 0x1) << 23)
#define   G_02881C_VS_OUT_CCDIST1_VEC_ENA(x)           (((x) >> 23) & 0x1)
#define   C_02881C_VS_OUT_CCDIST1_VEC_ENA              0xFF7FFFFF
#define R_028868_SQ_PGM_RESOURCES_VS                 0x028868
#define   S_028868_NUM_GPRS(x)                         (((x) & 0xFF) << 0)
#define   G_028868_NUM_GPRS(x)                         (((x) >> 0) & 0xFF)
#define   C_028868_NUM_GPRS                            0xFFFFFF00
#define   S_028868_STACK_SIZE(x)                       (((x) & 0xFF) << 8)
#define   G_028868_STACK_SIZE(x)                       (((x) >> 8) & 0xFF)
#define   C_028868_STACK_SIZE                          0xFFFF00FF
#define   S_028868_DX10_CLAMP(x)                       (((x) & 0x1) << 21)
#define   G_028868_DX10_CLAMP(x)                       (((x) >> 21) & 0x1)
#define   C_028868_DX10_CLAMP                          0xFFDFFFFF
#define   S_028868_FETCH_CACHE_LINES(x)                (((x) & 0x7) << 24)
#define   G_028868_FETCH_CACHE_LINES(x)                (((x) >> 24) & 0x7)
#define   C_028868_FETCH_CACHE_LINES                   0xF8FFFFFF
#define   S_028868_UNCACHED_FIRST_INST(x)              (((x) & 0x1) << 28)
#define   G_028868_UNCACHED_FIRST_INST(x)              (((x) >> 28) & 0x1)
#define   C_028868_UNCACHED_FIRST_INST                 0xEFFFFFFF
#define R_028850_SQ_PGM_RESOURCES_PS                 0x028850
#define   S_028850_NUM_GPRS(x)                         (((x) & 0xFF) << 0)
#define   G_028850_NUM_GPRS(x)                         (((x) >> 0) & 0xFF)
#define   C_028850_NUM_GPRS                            0xFFFFFF00
#define   S_028850_STACK_SIZE(x)                       (((x) & 0xFF) << 8)
#define   G_028850_STACK_SIZE(x)                       (((x) >> 8) & 0xFF)
#define   C_028850_STACK_SIZE                          0xFFFF00FF
#define   S_028850_DX10_CLAMP(x)                       (((x) & 0x1) << 21)
#define   G_028850_DX10_CLAMP(x)                       (((x) >> 21) & 0x1)
#define   C_028850_DX10_CLAMP                          0xFFDFFFFF
#define   S_028850_FETCH_CACHE_LINES(x)                (((x) & 0x7) << 24)
#define   G_028850_FETCH_CACHE_LINES(x)                (((x) >> 24) & 0x7)
#define   C_028850_FETCH_CACHE_LINES                   0xF8FFFFFF
#define   S_028850_UNCACHED_FIRST_INST(x)              (((x) & 0x1) << 28)
#define   G_028850_UNCACHED_FIRST_INST(x)              (((x) >> 28) & 0x1)
#define   C_028850_UNCACHED_FIRST_INST                 0xEFFFFFFF
#define   S_028850_CLAMP_CONSTS(x)                     (((x) & 0x1) << 31)
#define   G_028850_CLAMP_CONSTS(x)                     (((x) >> 31) & 0x1)
#define   C_028850_CLAMP_CONSTS                        0x7FFFFFFF
#define R_028644_SPI_PS_INPUT_CNTL_0                 0x028644
#define   S_028644_SEMANTIC(x)                         (((x) & 0xFF) << 0)
#define   G_028644_SEMANTIC(x)                         (((x) >> 0) & 0xFF)
#define   C_028644_SEMANTIC                            0xFFFFFF00
#define   S_028644_DEFAULT_VAL(x)                      (((x) & 0x3) << 8)
#define   G_028644_DEFAULT_VAL(x)                      (((x) >> 8) & 0x3)
#define   C_028644_DEFAULT_VAL                         0xFFFFFCFF
#define   S_028644_FLAT_SHADE(x)                       (((x) & 0x1) << 10)
#define   G_028644_FLAT_SHADE(x)                       (((x) >> 10) & 0x1)
#define   C_028644_FLAT_SHADE                          0xFFFFFBFF
#define   S_028644_SEL_CENTROID(x)                     (((x) & 0x1) << 11)
#define   G_028644_SEL_CENTROID(x)                     (((x) >> 11) & 0x1)
#define   C_028644_SEL_CENTROID                        0xFFFFF7FF
#define   S_028644_SEL_LINEAR(x)                       (((x) & 0x1) << 12)
#define   G_028644_SEL_LINEAR(x)                       (((x) >> 12) & 0x1)
#define   C_028644_SEL_LINEAR                          0xFFFFEFFF
#define   S_028644_CYL_WRAP(x)                         (((x) & 0xF) << 13)
#define   G_028644_CYL_WRAP(x)                         (((x) >> 13) & 0xF)
#define   C_028644_CYL_WRAP                            0xFFFE1FFF
#define   S_028644_PT_SPRITE_TEX(x)                    (((x) & 0x1) << 17)
#define   G_028644_PT_SPRITE_TEX(x)                    (((x) >> 17) & 0x1)
#define   C_028644_PT_SPRITE_TEX                       0xFFFDFFFF
#define   S_028644_SEL_SAMPLE(x)                       (((x) & 0x1) << 18)
#define   G_028644_SEL_SAMPLE(x)                       (((x) >> 18) & 0x1)
#define   C_028644_SEL_SAMPLE                          0xFFFBFFFF
#define R_0286D4_SPI_INTERP_CONTROL_0                0x0286D4
#define   S_0286D4_FLAT_SHADE_ENA(x)                   (((x) & 0x1) << 0)
#define   G_0286D4_FLAT_SHADE_ENA(x)                   (((x) >> 0) & 0x1)
#define   C_0286D4_FLAT_SHADE_ENA                      0xFFFFFFFE
#define   S_0286D4_PNT_SPRITE_ENA(x)                   (((x) & 0x1) << 1)
#define   G_0286D4_PNT_SPRITE_ENA(x)                   (((x) >> 1) & 0x1)
#define   C_0286D4_PNT_SPRITE_ENA                      0xFFFFFFFD
#define   S_0286D4_PNT_SPRITE_OVRD_X(x)                (((x) & 0x7) << 2)
#define   G_0286D4_PNT_SPRITE_OVRD_X(x)                (((x) >> 2) & 0x7)
#define   C_0286D4_PNT_SPRITE_OVRD_X                   0xFFFFFFE3
#define   S_0286D4_PNT_SPRITE_OVRD_Y(x)                (((x) & 0x7) << 5)
#define   G_0286D4_PNT_SPRITE_OVRD_Y(x)                (((x) >> 5) & 0x7)
#define   C_0286D4_PNT_SPRITE_OVRD_Y                   0xFFFFFF1F
#define   S_0286D4_PNT_SPRITE_OVRD_Z(x)                (((x) & 0x7) << 8)
#define   G_0286D4_PNT_SPRITE_OVRD_Z(x)                (((x) >> 8) & 0x7)
#define   C_0286D4_PNT_SPRITE_OVRD_Z                   0xFFFFF8FF
#define   S_0286D4_PNT_SPRITE_OVRD_W(x)                (((x) & 0x7) << 11)
#define   G_0286D4_PNT_SPRITE_OVRD_W(x)                (((x) >> 11) & 0x7)
#define   C_0286D4_PNT_SPRITE_OVRD_W                   0xFFFFC7FF
#define   S_0286D4_PNT_SPRITE_TOP_1(x)                 (((x) & 0x1) << 14)
#define   G_0286D4_PNT_SPRITE_TOP_1(x)                 (((x) >> 14) & 0x1)
#define   C_0286D4_PNT_SPRITE_TOP_1                    0xFFFFBFFF

#define SQ_TEX_INST_LD 0x03
#define SQ_TEX_INST_GET_GRADIENTS_H 0x7
#define SQ_TEX_INST_GET_GRADIENTS_V 0x8

#define SQ_TEX_INST_SAMPLE 0x10
#define SQ_TEX_INST_SAMPLE_L 0x11
#define SQ_TEX_INST_SAMPLE_C 0x18
#endif
