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

/**
 * \file
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 *
 * \author Jerome Glisse <j.glisse@gmail.com>
 *
 * \author Corbin Simpson <MostAwesomeDude@gmail.com>
 *
 * \todo Depth write, WPOS/FOGC inputs
 *
 * \todo FogOption
 *
 * \todo Verify results of opcodes for accuracy, I've only checked them in
 * specific cases.
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "r300_context.h"
#include "r500_fragprog.h"
#include "r300_reg.h"
#include "r300_state.h"

/*
 * Useful macros and values
 */
#define ERROR(fmt, args...) do {			\
		fprintf(stderr, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
		fp->error = GL_TRUE;			\
	} while(0)

#define COMPILE_STATE struct r300_pfs_compile_state *cs = fp->cs

/* "Register" flags */
#define REG_CONSTANT (1 << 8)
#define REG_SRC_REL (1 << 9)
#define REG_DEST_REL (1 << 7)

/* Swizzle tools */
#define R500_SWIZZLE_ZERO 4
#define R500_SWIZZLE_HALF 5
#define R500_SWIZZLE_ONE 6
#define R500_SWIZ_RGB_ZERO ((4 << 0) | (4 << 3) | (4 << 6))
#define R500_SWIZ_RGB_ONE ((6 << 0) | (6 << 3) | (6 << 6))
#define R500_SWIZ_RGB_RGB ((0 << 0) | (1 << 3) | (2 << 6))
/* Swizzles for inst2 */
#define MAKE_SWIZ_TEX_STRQ(x) (x << 8)
#define MAKE_SWIZ_TEX_RGBA(x) (x << 24)
/* Swizzles for inst3 */
#define MAKE_SWIZ_RGB_A(x) (x << 2)
#define MAKE_SWIZ_RGB_B(x) (x << 15)
/* Swizzles for inst4 */
#define MAKE_SWIZ_ALPHA_A(x) (x << 14)
#define MAKE_SWIZ_ALPHA_B(x) (x << 21)
/* Swizzle for inst5 */
#define MAKE_SWIZ_RGBA_C(x) (x << 14)
#define MAKE_SWIZ_ALPHA_C(x) (x << 27)

static inline GLuint make_rgb_swizzle(struct prog_src_register src) {
	GLuint swiz = 0x0;
	GLuint temp;
	/* This could be optimized, but it should be plenty fast already. */
	int i;
	for (i = 0; i < 3; i++) {
		temp = (src.Swizzle >> i*3) & 0x7;
		/* Fix SWIZZLE_ONE */
		if (temp == 5) temp++;
		swiz += temp << i*3;
	}
	return swiz;
}

static inline GLuint make_alpha_swizzle(struct prog_src_register src) {
	GLuint swiz = (src.Swizzle >> 12) & 0x7;
	if (swiz == 5) swiz++;
	return swiz;
}

static inline GLuint make_strq_swizzle(struct prog_src_register src) {
	GLuint swiz = 0x0;
	GLuint temp = src.Swizzle;
	int i;
	for (i = 0; i < 4; i++) {
		swiz += (temp & 0x3) << i*2;
		temp >>= 3;
	}
	return swiz;
}

/* Borrowed verbatim from r300_fragprog since it hasn't changed. */
static GLuint emit_const4fv(struct r500_fragment_program *fp,
			    const GLfloat * cp)
{
	GLuint reg = 0x0;
	int index;

	for (index = 0; index < fp->const_nr; ++index) {
		if (fp->constant[index] == cp)
			break;
	}

	if (index >= fp->const_nr) {
		/* TODO: This should be r5xx nums, not r300 */
		if (index >= PFS_NUM_CONST_REGS) {
			ERROR("Out of hw constants!\n");
			return reg;
		}

		fp->const_nr++;
		fp->constant[index] = cp;
	}

	reg = index | REG_CONSTANT;
	return reg;
}

static GLuint make_src(struct r500_fragment_program *fp, struct prog_src_register src) {
	GLuint reg;
	switch (src.File) {
		case PROGRAM_TEMPORARY:
			reg = (src.Index << 0x1) | 0x1;
			break;
		case PROGRAM_INPUT:
			/* Ugly hack needed to work around Mesa;
			 * fragments don't get loaded right otherwise! */
			reg = 0x0;
			break;
	        case PROGRAM_STATE_VAR:
	        case PROGRAM_NAMED_PARAM:
		case PROGRAM_CONSTANT:
			reg = emit_const4fv(fp, fp->mesa_program.Base.Parameters->
				  ParameterValues[src.Index]);
			break;
		default:
			ERROR("Can't handle src.File %x\n", src.File);
			reg = 0x0;
			break;
	}
	return reg;
}

static GLuint make_dest(struct r500_fragment_program *fp, struct prog_dst_register dest) {
	GLuint reg;
	switch (dest.File) {
		case PROGRAM_TEMPORARY:
			reg = (dest.Index << 0x1) | 0x1;
			break;
		case PROGRAM_OUTPUT:
			/* Eventually we may need to handle multiple
			 * rendering targets... */
			reg = dest.Index;
			break;
		default:
			ERROR("Can't handle dest.File %x\n", dest.File);
			reg = 0x0;
			break;
	}
	return reg;
}

static void emit_tex(struct r500_fragment_program *fp,
		     struct prog_instruction *fpi, int opcode, int dest, int counter)
{
	int hwsrc, hwdest;
	GLuint mask;

	mask = fpi->DstReg.WriteMask << 11;
	hwsrc = make_src(fp, fpi->SrcReg[0]);

	fp->inst[counter].inst0 = R500_INST_TYPE_TEX | mask
		| R500_INST_TEX_SEM_WAIT;

	fp->inst[counter].inst1 = fpi->TexSrcUnit
		| R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED;
	switch (opcode) {
	case OPCODE_TEX:
		fp->inst[counter].inst1 |= R500_TEX_INST_LD;
		break;
	case OPCODE_TXP:
		fp->inst[counter].inst1 |= R500_TEX_INST_PROJ;
	}

	fp->inst[counter].inst2 = R500_TEX_SRC_ADDR(hwsrc)
		/* | MAKE_SWIZ_TEX_STRQ(make_strq_swizzle(fpi->SrcReg[0])) */
		| R500_TEX_SRC_S_SWIZ_R | R500_TEX_SRC_T_SWIZ_G
		| R500_TEX_SRC_R_SWIZ_B | R500_TEX_SRC_Q_SWIZ_A
		| R500_TEX_DST_ADDR(dest)
		| R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;



	fp->inst[counter].inst3 = 0x0;
	fp->inst[counter].inst4 = 0x0;
	fp->inst[counter].inst5 = 0x0;	
}

static void dumb_shader(struct r500_fragment_program *fp)
{
	fp->inst[0].inst0 = R500_INST_TYPE_TEX
		| R500_INST_TEX_SEM_WAIT
		| R500_INST_RGB_WMASK_R
		| R500_INST_RGB_WMASK_G
		| R500_INST_RGB_WMASK_B
		| R500_INST_ALPHA_WMASK
		| R500_INST_RGB_CLAMP
		| R500_INST_ALPHA_CLAMP;
	fp->inst[0].inst1 = R500_TEX_ID(0)
		| R500_TEX_INST_LD
		| R500_TEX_SEM_ACQUIRE
		| R500_TEX_IGNORE_UNCOVERED;
	fp->inst[0].inst2 = R500_TEX_SRC_ADDR(0)
		| R500_TEX_SRC_S_SWIZ_R
		| R500_TEX_SRC_T_SWIZ_G
		| R500_TEX_DST_ADDR(0)
		| R500_TEX_DST_R_SWIZ_R
		| R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B
		| R500_TEX_DST_A_SWIZ_A;
	fp->inst[0].inst3 = R500_DX_ADDR(0)
		| R500_DX_S_SWIZ_R
		| R500_DX_T_SWIZ_R
		| R500_DX_R_SWIZ_R
		| R500_DX_Q_SWIZ_R
		| R500_DY_ADDR(0)
		| R500_DY_S_SWIZ_R
		| R500_DY_T_SWIZ_R
		| R500_DY_R_SWIZ_R
		| R500_DY_Q_SWIZ_R;
	fp->inst[0].inst4 = 0x0;
	fp->inst[0].inst5 = 0x0;

	fp->inst[1].inst0 = R500_INST_TYPE_OUT |
		R500_INST_TEX_SEM_WAIT |
		R500_INST_LAST |
		R500_INST_RGB_OMASK_R |
		R500_INST_RGB_OMASK_G |
		R500_INST_RGB_OMASK_B |
		R500_INST_ALPHA_OMASK;
	fp->inst[1].inst1 = R500_RGB_ADDR0(0) |
		R500_RGB_ADDR1(0) |
		R500_RGB_ADDR1_CONST |
		R500_RGB_ADDR2(0) |
		R500_RGB_ADDR2_CONST |
		R500_RGB_SRCP_OP_1_MINUS_2RGB0;
	fp->inst[1].inst2 = R500_ALPHA_ADDR0(0) |
		R500_ALPHA_ADDR1(0) |
		R500_ALPHA_ADDR1_CONST |
		R500_ALPHA_ADDR2(0) |
		R500_ALPHA_ADDR2_CONST |
		R500_ALPHA_SRCP_OP_1_MINUS_2A0;
	fp->inst[1].inst3 = R500_ALU_RGB_SEL_A_SRC0 |
		R500_ALU_RGB_R_SWIZ_A_R |
		R500_ALU_RGB_G_SWIZ_A_G |
		R500_ALU_RGB_B_SWIZ_A_B |
		R500_ALU_RGB_SEL_B_SRC0 |
		R500_ALU_RGB_R_SWIZ_B_1 |
		R500_ALU_RGB_B_SWIZ_B_1 |
		R500_ALU_RGB_G_SWIZ_B_1;
	fp->inst[1].inst4 = R500_ALPHA_OP_MAD |
		R500_ALPHA_SWIZ_A_A |
		R500_ALPHA_SWIZ_B_1;
	fp->inst[1].inst5 = R500_ALU_RGBA_OP_MAD |
		R500_ALU_RGBA_R_SWIZ_0 |
		R500_ALU_RGBA_G_SWIZ_0 |
		R500_ALU_RGBA_B_SWIZ_0 |
		R500_ALU_RGBA_A_SWIZ_0;

	fp->cs->nrslots = 2;
	fp->translated = GL_TRUE;
}

static void emit_alu(struct r500_fragment_program *fp) {
}

static GLboolean parse_program(struct r500_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;
	const struct prog_instruction *inst = mp->Base.Instructions;
	struct prog_instruction *fpi;
	GLuint src[3], dest, temp[2];
	int flags, mask, counter = 0;

	if (!inst || inst[0].Opcode == OPCODE_END) {
		ERROR("The program is empty!\n");
		return GL_FALSE;
	}

	for (fpi = mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {

		if (fpi->Opcode != OPCODE_KIL) {
			dest = make_dest(fp, fpi->DstReg);
			mask = fpi->DstReg.WriteMask << 11;
		}

		switch (fpi->Opcode) {
			case OPCODE_ABS:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				/* Variation on MOV */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_MOD_A_ABS | R500_ALU_RGB_SEL_B_SRC0
					| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAX
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0
					| MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0])) | R500_ALPHA_MOD_A_ABS
					| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_ADD:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				/* Variation on MAD: 1*src0+src1 */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = /* 1 */
					MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
					| R500_ALU_RGB_SEL_B_SRC0 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC1
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC1
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[1]));
				break;
			case OPCODE_DP3:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				src[2] = make_src(fp, fpi->SrcReg[2]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]) | R500_RGB_ADDR2(src[2]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]) | R500_ALPHA_ADDR2(src[2]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_DP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_DP3
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[2]))
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[2]));
				break;
			case OPCODE_DP4:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				src[2] = make_src(fp, fpi->SrcReg[2]);
				/* Based on DP3 */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]) | R500_RGB_ADDR2(src[2]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]) | R500_ALPHA_ADDR2(src[2]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_DP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_DP4
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[2]))
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[2]));
				break;
			case OPCODE_MAD:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				src[2] = make_src(fp, fpi->SrcReg[2]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]) | R500_RGB_ADDR2(src[2]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]) | R500_ALPHA_ADDR2(src[2]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[2]))
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[2]));
				break;
			case OPCODE_MAX:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[0]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1
					| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAX
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_MIN:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[0]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1
					| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MIN
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MIN
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_MOV:
				src[0] = make_src(fp, fpi->SrcReg[0]);

				/* changed to use MAD - not sure if we
				   ever have negative things which max will fail on */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				  | MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB) 
				  | R500_ALU_RGB_SEL_B_SRC0
				  | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SEL_B_SRC0 
				  | R500_ALPHA_SWIZ_A_A | R500_ALPHA_SWIZ_B_1;

				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
				  | MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				  | MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				break;
			case OPCODE_MUL:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				/* Variation on MAD: src0*src1+0 */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
					// | R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					// | R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				break;
			case OPCODE_SUB:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				/* Variation on MAD: 1*src0-src1 */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU
					| mask;
				fp->inst[counter].inst1 = R500_RGB_ADDR1(src[0])
					| R500_RGB_ADDR2(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR1(src[0])
					| R500_ALPHA_ADDR2(src[1]);
				fp->inst[counter].inst3 = /* 1 */
					MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_MOD_C_NEG
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_ALPHA_MOD_C_NEG;
				break;
			case OPCODE_TEX:
				emit_tex(fp, fpi, OPCODE_TEX, dest, counter);
				break;
			case OPCODE_TXP:
				emit_tex(fp, fpi, OPCODE_TXP, dest, counter);
				break;
			default:
				ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
				break;
		}

		/* Finishing touches */
		if (fpi->SaturateMode == SATURATE_ZERO_ONE) {
			fp->inst[counter].inst0 |= R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP;
		}
		if (fpi->DstReg.File == PROGRAM_OUTPUT) {
			fp->inst[counter].inst0 |= R500_INST_TYPE_OUT
			| R500_INST_RGB_OMASK_R | R500_INST_RGB_OMASK_G
			| R500_INST_RGB_OMASK_B | R500_INST_ALPHA_OMASK;
		}

		counter++;

		if (fp->error)
			return GL_FALSE;

	}

	fp->cs->nrslots = counter;

	/* Finish him! (If it's an output instruction...)
	 * Yes, I know it's ugly... */
	if ((fp->inst[counter].inst0 & 0x3) ^ 0x2) {
		fp->inst[counter].inst0 |= R500_INST_TYPE_OUT
		| R500_INST_TEX_SEM_WAIT | R500_INST_LAST;
	}

	return GL_TRUE;
}

static void init_program(r300ContextPtr r300, struct r500_fragment_program *fp)
{
	struct r300_pfs_compile_state *cs = NULL;
	struct gl_fragment_program *mp = &fp->mesa_program;
	struct prog_instruction *fpi;
	GLuint InputsRead = mp->Base.InputsRead;
	GLuint temps_used = 0;	/* for fp->temps[] */
	int i, j;

	/* New compile, reset tracking data */
	fp->optimization =
	    driQueryOptioni(&r300->radeon.optionCache, "fp_optimization");
	fp->translated = GL_FALSE;
	fp->error = GL_FALSE;
	fp->cs = cs = &(R300_CONTEXT(fp->ctx)->state.pfs_compile);
	fp->cur_node = 0;
	fp->first_node_has_tex = 0;
	fp->const_nr = 0;
	fp->max_temp_idx = 64;
	fp->node[0].alu_end = -1;
	fp->node[0].tex_end = -1;

	_mesa_memset(cs, 0, sizeof(*fp->cs));
	for (i = 0; i < PFS_MAX_ALU_INST; i++) {
		for (j = 0; j < 3; j++) {
			cs->slot[i].vsrc[j] = SRC_CONST;
			cs->slot[i].ssrc[j] = SRC_CONST;
		}
	}

	/* Work out what temps the Mesa inputs correspond to, this must match
	 * what setup_rs_unit does, which shouldn't be a problem as rs_unit
	 * configures itself based on the fragprog's InputsRead
	 *
	 * NOTE: this depends on get_hw_temp() allocating registers in order,
	 * starting from register 0.
	 */

#if 0
	/* Texcoords come first */
	for (i = 0; i < fp->ctx->Const.MaxTextureUnits; i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			cs->inputs[FRAG_ATTRIB_TEX0 + i].refcount = 0;
			cs->inputs[FRAG_ATTRIB_TEX0 + i].reg =
			    get_hw_temp(fp, 0);
		}
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* fragment position treated as a texcoord */
	if (InputsRead & FRAG_BIT_WPOS) {
		cs->inputs[FRAG_ATTRIB_WPOS].refcount = 0;
		cs->inputs[FRAG_ATTRIB_WPOS].reg = get_hw_temp(fp, 0);
		insert_wpos(&mp->Base);
	}
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Then primary colour */
	if (InputsRead & FRAG_BIT_COL0) {
		cs->inputs[FRAG_ATTRIB_COL0].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL0].reg = get_hw_temp(fp, 0);
	}
	InputsRead &= ~FRAG_BIT_COL0;

	/* Secondary color */
	if (InputsRead & FRAG_BIT_COL1) {
		cs->inputs[FRAG_ATTRIB_COL1].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL1].reg = get_hw_temp(fp, 0);
	}
	InputsRead &= ~FRAG_BIT_COL1;

	/* Anything else */
	if (InputsRead) {
		WARN_ONCE("Don't know how to handle inputs 0x%x\n", InputsRead);
		/* force read from hwreg 0 for now */
		for (i = 0; i < 32; i++)
			if (InputsRead & (1 << i))
				cs->inputs[i].reg = 0;
	}
#endif

	/* Pre-parse the mesa program, grabbing refcounts on input/temp regs.
	 * That way, we can free up the reg when it's no longer needed
	 */
	if (!mp->Base.Instructions) {
		ERROR("No instructions found in program\n");
		return;
	}

	for (fpi = mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {
		int idx;

		for (i = 0; i < 3; i++) {
			idx = fpi->SrcReg[i].Index;
			switch (fpi->SrcReg[i].File) {
			case PROGRAM_TEMPORARY:
				if (!(temps_used & (1 << idx))) {
					cs->temps[idx].reg = -1;
					cs->temps[idx].refcount = 1;
					temps_used |= (1 << idx);
				} else
					cs->temps[idx].refcount++;
				break;
			case PROGRAM_INPUT:
				cs->inputs[idx].refcount++;
				break;
			default:
				break;
			}
		}

		idx = fpi->DstReg.Index;
		if (fpi->DstReg.File == PROGRAM_TEMPORARY) {
			if (!(temps_used & (1 << idx))) {
				cs->temps[idx].reg = -1;
				cs->temps[idx].refcount = 1;
				temps_used |= (1 << idx);
			} else
				cs->temps[idx].refcount++;
		}
	}
	cs->temp_in_use = temps_used;
}

static void update_params(struct r500_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;

	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (mp->Base.Parameters)
		_mesa_load_state_parameters(fp->ctx, mp->Base.Parameters);
}

void r500TranslateFragmentShader(r300ContextPtr r300,
				 struct r500_fragment_program *fp)
{

	struct r300_pfs_compile_state *cs = NULL;

	if (!fp->translated) {

		/* I need to see what I'm working with! */
		fprintf(stderr, "Mesa program:\n");
		fprintf(stderr, "-------------\n");
		_mesa_print_program(&fp->mesa_program.Base);
		fflush(stdout);

		init_program(r300, fp);
		cs = fp->cs;

		if (parse_program(fp) == GL_FALSE) {
			ERROR("Huh. Couldn't parse program. There should be additional errors explaining why.\nUsing dumb shader...\n");
			dumb_shader(fp);
			return;
		}

		/* Finish off */
		fp->node[fp->cur_node].alu_end =
		    cs->nrslots - fp->node[fp->cur_node].alu_offset - 1;
		if (fp->node[fp->cur_node].tex_end < 0)
			fp->node[fp->cur_node].tex_end = 0;
		fp->alu_offset = 0;
		fp->alu_end = cs->nrslots - 1;
		//assert(fp->node[fp->cur_node].alu_end >= 0);
		//assert(fp->alu_end >= 0);

		fp->translated = GL_TRUE;
		r300UpdateStateParameters(fp->ctx, _NEW_PROGRAM);
	}

	update_params(fp);
}
