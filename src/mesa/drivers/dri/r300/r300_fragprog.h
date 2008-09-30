/*
 * Copyright (C) 2005 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 *   Jerome Glisse <j.glisse@gmail.com>
 */
#ifndef __R300_FRAGPROG_H_
#define __R300_FRAGPROG_H_

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"

#include "r300_context.h"
#include "radeon_program.h"

#define DRI_CONF_FP_OPTIMIZATION_SPEED   0
#define DRI_CONF_FP_OPTIMIZATION_QUALITY 1

#if 1

/**
 * Fragment program helper macros
 */

/* Produce unshifted source selectors */
#define FP_TMP(idx) (idx)
#define FP_CONST(idx) ((idx) | (1 << 5))

/* Produce source/dest selector dword */
#define FP_SELC_MASK_NO		0
#define FP_SELC_MASK_X		1
#define FP_SELC_MASK_Y		2
#define FP_SELC_MASK_XY		3
#define FP_SELC_MASK_Z		4
#define FP_SELC_MASK_XZ		5
#define FP_SELC_MASK_YZ		6
#define FP_SELC_MASK_XYZ	7

#define FP_SELC(destidx,regmask,outmask,src0,src1,src2) \
	(((destidx) << R300_ALU_DSTC_SHIFT) |		\
	 (FP_SELC_MASK_##regmask << 23) |		\
	 (FP_SELC_MASK_##outmask << 26) |		\
	 ((src0) << R300_ALU_SRC0C_SHIFT) |		\
	 ((src1) << R300_ALU_SRC1C_SHIFT) |		\
	 ((src2) << R300_ALU_SRC2C_SHIFT))

#define FP_SELA_MASK_NO		0
#define FP_SELA_MASK_W		1

#define FP_SELA(destidx,regmask,outmask,src0,src1,src2) \
	(((destidx) << R300_ALU_DSTA_SHIFT) |		\
	 (FP_SELA_MASK_##regmask << 23) |		\
	 (FP_SELA_MASK_##outmask << 24) |		\
	 ((src0) << R300_ALU_SRC0A_SHIFT) |		\
	 ((src1) << R300_ALU_SRC1A_SHIFT) |		\
	 ((src2) << R300_ALU_SRC2A_SHIFT))

/* Produce unshifted argument selectors */
#define FP_ARGC(source)	R300_ALU_ARGC_##source
#define FP_ARGA(source) R300_ALU_ARGA_##source
#define FP_ABS(arg) ((arg) | (1 << 6))
#define FP_NEG(arg) ((arg) ^ (1 << 5))

/* Produce instruction dword */
#define FP_INSTRC(opcode,arg0,arg1,arg2) \
	(R300_ALU_OUTC_##opcode | 		\
	((arg0) << R300_ALU_ARG0C_SHIFT) |	\
	((arg1) << R300_ALU_ARG1C_SHIFT) |	\
	((arg2) << R300_ALU_ARG2C_SHIFT))

#define FP_INSTRA(opcode,arg0,arg1,arg2) \
	(R300_ALU_OUTA_##opcode | 		\
	((arg0) << R300_ALU_ARG0A_SHIFT) |	\
	((arg1) << R300_ALU_ARG1A_SHIFT) |	\
	((arg2) << R300_ALU_ARG2A_SHIFT))

#endif

struct r300_fragment_program;

extern void r300TranslateFragmentShader(r300ContextPtr r300,
					struct r300_fragment_program *fp);


/**
 * Used internally by the r300 fragment program code to store compile-time
 * only data.
 */
struct r300_fragment_program_compiler {
	r300ContextPtr r300;
	struct r300_fragment_program *fp;
	struct r300_fragment_program_code *code;
	struct gl_program *program;
};

extern GLboolean r300FragmentProgramEmit(struct r300_fragment_program_compiler *compiler);


extern void r300FragmentProgramDump(
	struct r300_fragment_program *fp,
	struct r300_fragment_program_code *code);

#endif
