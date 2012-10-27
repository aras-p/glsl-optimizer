/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FREEDRENO_PM4_H_
#define FREEDRENO_PM4_H_

#define CP_TYPE0_PKT              (0 << 30)
#define CP_TYPE1_PKT              (1 << 30)
#define CP_TYPE2_PKT              (2 << 30)
#define CP_TYPE3_PKT              (3 << 30)


#define CP_ME_INIT                0x48
#define CP_NOP                    0x10
#define CP_INDIRECT_BUFFER        0x3f
#define CP_INDIRECT_BUFFER_PFD    0x37
#define CP_WAIT_FOR_IDLE          0x26
#define CP_WAIT_REG_MEM           0x3c
#define CP_WAIT_REG_EQ            0x52
#define CP_WAT_REG_GTE            0x53
#define CP_WAIT_UNTIL_READ        0x5c
#define CP_WAIT_IB_PFD_COMPLETE   0x5d
#define CP_REG_RMW                0x21
#define CP_REG_TO_MEM             0x3e
#define CP_MEM_WRITE              0x3d
#define CP_MEM_WRITE_CNTR         0x4f
#define CP_COND_EXEC              0x44
#define CP_COND_WRITE             0x45
#define CP_EVENT_WRITE            0x46
#define CP_EVENT_WRITE_SHD        0x58
#define CP_EVENT_WRITE_CFL        0x59
#define CP_EVENT_WRITE_ZPD        0x5b
#define CP_DRAW_INDX              0x22
#define CP_DRAW_INDX_2            0x36
#define CP_DRAW_INDX_BIN          0x34
#define CP_DRAW_INDX_2_BIN        0x35
#define CP_VIZ_QUERY              0x23
#define CP_SET_STATE              0x25
#define CP_SET_CONSTANT           0x2d
#define CP_IM_LOAD                0x27
#define CP_IM_LOAD_IMMEDIATE      0x2b
#define CP_LOAD_CONSTANT_CONTEXT  0x2e
#define CP_INVALIDATE_STATE       0x3b
#define CP_SET_SHADER_BASES       0x4a
#define CP_SET_BIN_MASK           0x50
#define CP_SET_BIN_SELECT         0x51
#define CP_CONTEXT_UPDATE         0x5e
#define CP_INTERRUPT              0x40
#define CP_IM_STORE               0x2c
#define CP_SET_BIN_BASE_OFFSET    0x4b      /* for a20x */
#define CP_SET_DRAW_INIT_FLAGS    0x4b      /* for a22x */
#define CP_SET_PROTECTED_MODE     0x5f
#define CP_LOAD_STATE             0x30
#define CP_COND_INDIRECT_BUFFER_PFE 0x3a
#define CP_COND_INDIRECT_BUFFER_PFD 0x32


#define CP_REG(reg) ((0x4 << 16) | ((reg) - 0x2000))


#endif	/* FREEDRENO_PM4_H_ */
