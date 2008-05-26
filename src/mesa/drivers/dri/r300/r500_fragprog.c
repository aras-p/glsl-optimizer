/*
 * Copyright (C) 2005 Ben Skeggs.
 *
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Adaptation and modification for ATI/AMD Radeon R500 GPU chipsets.
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

#define R500_US_NUM_TEMP_REGS 128
#define R500_US_NUM_CONST_REGS 256

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
#define R500_SWIZ_MOD_NEG 1
#define R500_SWIZ_MOD_ABS 2
#define R500_SWIZ_MOD_NEG_ABS 3
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

/* Writemasks */
#define R500_WRITEMASK_G 0x2
#define R500_WRITEMASK_A 0x8
#define R500_WRITEMASK_AR 0x9
#define R500_WRITEMASK_AG 0xA
#define R500_WRITEMASK_ARG 0xB
#define R500_WRITEMASK_AB 0xC
#define R500_WRITEMASK_ARGB 0xF

/* 1/(2pi), needed for quick modulus in trig insts
 * Thanks to glisse for pointing out how to do it! */
static const GLfloat RCP_2PI[] = {0.15915494309189535,
	0.15915494309189535,
	0.15915494309189535,
	0.15915494309189535};

static const GLfloat LIT[] = {127.999999,
	127.999999,
	127.999999,
	-127.999999};

static void dump_program(struct r500_fragment_program *fp);

static inline GLuint make_rgb_swizzle(struct prog_src_register src) {
	GLuint swiz = 0x0;
	GLuint temp;
	/* This could be optimized, but it should be plenty fast already. */
	int i;
	for (i = 0; i < 3; i++) {
	        temp = GET_SWZ(src.Swizzle, i);
		/* Fix SWIZZLE_ONE */
		if (temp == 5) temp++;
		swiz |= temp << i*3;
	}
	if (src.NegateBase)
		swiz |= (R500_SWIZ_MOD_NEG << 9);
	return swiz;
}

static inline GLuint make_alpha_swizzle(struct prog_src_register src) {
	GLuint swiz = GET_SWZ(src.Swizzle, 3);

	if (swiz == 5) swiz++;

	if (src.NegateBase)
		swiz |= (R500_SWIZ_MOD_NEG << 3);

	return swiz;
}

static inline GLuint make_sop_swizzle(struct prog_src_register src) {
	GLuint swiz = GET_SWZ(src.Swizzle, 0);

	if (swiz == 5) swiz++;
	return swiz;
}

static inline GLuint make_strq_swizzle(struct prog_src_register src) {
	GLuint swiz = 0x0, temp = 0x0;
	int i;
	for (i = 0; i < 4; i++) {
		temp = GET_SWZ(src.Swizzle, i) & 0x3;
		swiz |= temp << i*2;
	}
	return swiz;
}

static int get_temp(struct r500_fragment_program *fp, int slot) {

	COMPILE_STATE;

	int r = cs->temp_in_use + 1 + slot;

	if (r > R500_US_NUM_TEMP_REGS) {
		ERROR("Too many temporary registers requested, can't compile!\n");
	}

	return r;
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
		if (index >= R500_US_NUM_CONST_REGS) {
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
	COMPILE_STATE;
	GLuint reg;
	switch (src.File) {
	case PROGRAM_TEMPORARY:
		reg = src.Index + fp->temp_reg_offset;
		break;
	case PROGRAM_INPUT:
		reg = cs->inputs[src.Index].reg;
		break;
	case PROGRAM_LOCAL_PARAM:
		reg = emit_const4fv(fp,
				    fp->mesa_program.Base.LocalParams[src.
								      Index]);
		break;
	case PROGRAM_ENV_PARAM:
		reg = emit_const4fv(fp,
				    fp->ctx->FragmentProgram.Parameters[src.
									Index]);
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
			reg = dest.Index + fp->temp_reg_offset;
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
		     struct prog_instruction *fpi, int dest, int counter)
{
	int hwsrc, hwdest;
	GLuint mask;

	mask = fpi->DstReg.WriteMask << 11;
	hwsrc = make_src(fp, fpi->SrcReg[0]);

	if (fpi->DstReg.File == PROGRAM_OUTPUT) {
		hwdest = get_temp(fp, 0);
	} else {
		hwdest = dest;
	}

	fp->inst[counter].inst0 = R500_INST_TYPE_TEX | mask
		| R500_INST_TEX_SEM_WAIT;

	fp->inst[counter].inst1 = R500_TEX_ID(fpi->TexSrcUnit)
		| R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED;
	
	if (fpi->TexSrcTarget == TEXTURE_RECT_INDEX)
	        fp->inst[counter].inst1 |= R500_TEX_UNSCALED;

	switch (fpi->Opcode) {
	case OPCODE_KIL:
		fp->inst[counter].inst1 |= R500_TEX_INST_TEXKILL;
		break;
	case OPCODE_TEX:
		fp->inst[counter].inst1 |= R500_TEX_INST_LD;
		break;
	case OPCODE_TXB:
		fp->inst[counter].inst1 |= R500_TEX_INST_LODBIAS;
		break;
	case OPCODE_TXP:
		fp->inst[counter].inst1 |= R500_TEX_INST_PROJ;
		break;
	default:
		ERROR("emit_tex can't handle opcode %x\n", fpi->Opcode);
	}

	fp->inst[counter].inst2 = R500_TEX_SRC_ADDR(hwsrc)
		| MAKE_SWIZ_TEX_STRQ(make_strq_swizzle(fpi->SrcReg[0]))
		/* | R500_TEX_SRC_S_SWIZ_R | R500_TEX_SRC_T_SWIZ_G
		| R500_TEX_SRC_R_SWIZ_B | R500_TEX_SRC_Q_SWIZ_A */
		| R500_TEX_DST_ADDR(hwdest)
		| R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;

	fp->inst[counter].inst3 = 0x0;
	fp->inst[counter].inst4 = 0x0;
	fp->inst[counter].inst5 = 0x0;

	if (fpi->DstReg.File == PROGRAM_OUTPUT) {
		counter++;
		fp->inst[counter].inst0 = R500_INST_TYPE_OUT
			| R500_INST_TEX_SEM_WAIT | (mask << 4);
		fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
		fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
		fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
			| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
			| R500_ALU_RGB_SEL_B_SRC0
			| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB)
			| R500_ALU_RGB_OMOD_DISABLE;
		fp->inst[counter].inst4 = R500_ALPHA_OP_CMP
			| R500_ALPHA_ADDRD(dest)
			| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_ALPHA_SWIZ_A_A)
			| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(R500_ALPHA_SWIZ_A_A)
			| R500_ALPHA_OMOD_DISABLE;
		fp->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
			| R500_ALU_RGBA_ADDRD(dest)
			| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
			| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
	}
}

static void emit_alu(struct r500_fragment_program *fp, int counter, struct prog_instruction *fpi) {
	if (fpi->DstReg.File == PROGRAM_OUTPUT) {
		fp->inst[counter].inst0 = R500_INST_TYPE_OUT;

		if (fpi->DstReg.Index == FRAG_RESULT_COLR)
			fp->inst[counter].inst0 |= (fpi->DstReg.WriteMask << 15);

		if (fpi->DstReg.Index == FRAG_RESULT_DEPR)
			fp->inst[counter].inst4 = R500_ALPHA_W_OMASK;
	} else {
		fp->inst[counter].inst0 = R500_INST_TYPE_ALU
			/* pixel_mask */
			| (fpi->DstReg.WriteMask << 11);
	}

	fp->inst[counter].inst0 |= R500_INST_TEX_SEM_WAIT;

	/* Ideally, we shouldn't have to explicitly clear memory here! */
	fp->inst[counter].inst1 = 0x0;
	fp->inst[counter].inst2 = 0x0;
	fp->inst[counter].inst3 = 0x0;
	fp->inst[counter].inst5 = 0x0;
}

static void emit_mov(struct r500_fragment_program *fp, int counter, struct prog_instruction *fpi, GLuint src_reg, GLuint swizzle, GLuint dest) {
	/* The r3xx shader uses MAD to implement MOV. We are using CMP, since
	 * it is technically more accurate and recommended by ATI/AMD. */
	emit_alu(fp, counter, fpi);
	fp->inst[counter].inst1 = R500_RGB_ADDR0(src_reg);
	fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src_reg);
	/* 0x1FF is 9 bits, size of an RGB swizzle. */
	fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
		| MAKE_SWIZ_RGB_A((swizzle & 0x1ff))
		| R500_ALU_RGB_SEL_B_SRC0
		| MAKE_SWIZ_RGB_B((swizzle & 0x1ff))
		| R500_ALU_RGB_OMOD_DISABLE;
	fp->inst[counter].inst4 |= R500_ALPHA_OP_CMP
		| R500_ALPHA_ADDRD(dest)
		| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(GET_SWZ(swizzle, 3))
		| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(GET_SWZ(swizzle, 3))
		| R500_ALPHA_OMOD_DISABLE;
	fp->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
		| R500_ALU_RGBA_ADDRD(dest)
		| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
		| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
}

static void emit_mad(struct r500_fragment_program *fp, int counter, struct prog_instruction *fpi, int one, int two, int three) {
	/* Note: This code was all Corbin's. Corbin is a rather hackish coder.
	 * If you can make it pretty or fast, please do so! */
	emit_alu(fp, counter, fpi);
	/* Common MAD stuff */
	fp->inst[counter].inst4 |= R500_ALPHA_OP_MAD
		| R500_ALPHA_ADDRD(make_dest(fp, fpi->DstReg));
	fp->inst[counter].inst5 |= R500_ALU_RGBA_OP_MAD
		| R500_ALU_RGBA_ADDRD(make_dest(fp, fpi->DstReg));
	switch (one) {
		case 0:
		case 1:
		case 2:
			fp->inst[counter].inst1 |= R500_RGB_ADDR0(make_src(fp, fpi->SrcReg[one]));
			fp->inst[counter].inst2 |= R500_ALPHA_ADDR0(make_src(fp, fpi->SrcReg[one]));
			fp->inst[counter].inst3 |= R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[one]));
			fp->inst[counter].inst4 |= R500_ALPHA_SEL_A_SRC0
				| MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[one]));
			break;
		case R500_SWIZZLE_ZERO:
			fp->inst[counter].inst3 |= MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ZERO);
			fp->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ZERO);
			break;
		case R500_SWIZZLE_ONE:
			fp->inst[counter].inst3 |= MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE);
			fp->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE);
			break;
		default:
			ERROR("Bad src index in emit_mad: %d\n", one);
			break;
	}
	switch (two) {
		case 0:
		case 1:
		case 2:
			fp->inst[counter].inst1 |= R500_RGB_ADDR1(make_src(fp, fpi->SrcReg[two]));
			fp->inst[counter].inst2 |= R500_ALPHA_ADDR1(make_src(fp, fpi->SrcReg[two]));
			fp->inst[counter].inst3 |= R500_ALU_RGB_SEL_B_SRC1
				| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[two]));
			fp->inst[counter].inst4 |= R500_ALPHA_SEL_B_SRC1
				| MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[two]));
			break;
		case R500_SWIZZLE_ZERO:
			fp->inst[counter].inst3 |= MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ZERO);
			fp->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ZERO);
			break;
		case R500_SWIZZLE_ONE:
			fp->inst[counter].inst3 |= MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
			fp->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ONE);
			break;
		default:
			ERROR("Bad src index in emit_mad: %d\n", two);
			break;
	}
	switch (three) {
		case 0:
		case 1:
		case 2:
			fp->inst[counter].inst1 |= R500_RGB_ADDR2(make_src(fp, fpi->SrcReg[three]));
			fp->inst[counter].inst2 |= R500_ALPHA_ADDR2(make_src(fp, fpi->SrcReg[three]));
			fp->inst[counter].inst5 |= R500_ALU_RGBA_SEL_C_SRC2
				| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[three]))
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[three]));
			break;
		case R500_SWIZZLE_ZERO:
			fp->inst[counter].inst5 |= MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			break;
		case R500_SWIZZLE_ONE:
			fp->inst[counter].inst5 |= MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ONE)
			| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ONE);
			break;
		default:
			ERROR("Bad src index in emit_mad: %d\n", three);
			break;
	}
}

static GLboolean parse_program(struct r500_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;
	const struct prog_instruction *inst = mp->Base.Instructions;
	struct prog_instruction *fpi;
	GLuint src[3], dest = 0;
	int temp_swiz, counter = 0;

	if (!inst || inst[0].Opcode == OPCODE_END) {
		ERROR("The program is empty!\n");
		return GL_FALSE;
	}

	for (fpi = mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {

		if (fpi->Opcode != OPCODE_KIL) {
			dest = make_dest(fp, fpi->DstReg);
		}

		switch (fpi->Opcode) {
			case OPCODE_ABS:
				emit_mov(fp, counter, fpi, make_src(fp, fpi->SrcReg[0]), fpi->SrcReg[0].Swizzle, dest);
				fp->inst[counter].inst3 |= R500_ALU_RGB_MOD_A_ABS
					| R500_ALU_RGB_MOD_B_ABS;
				fp->inst[counter].inst4 |= R500_ALPHA_MOD_A_ABS
					| R500_ALPHA_MOD_B_ABS;
				break;
			case OPCODE_ADD:
				/* Variation on MAD: 1*src0+src1 */
				emit_mad(fp, counter, fpi, R500_SWIZZLE_ONE, 0, 1);
				break;
		        case OPCODE_CMP:
				/* This inst's selects need to be swapped as follows:
				 * 0 -> C ; 1 -> B ; 2 -> A */
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				src[2] = make_src(fp, fpi->SrcReg[2]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[2])
					| R500_RGB_ADDR1(src[1]) | R500_RGB_ADDR2(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[2])
					| R500_ALPHA_ADDR1(src[1]) | R500_ALPHA_ADDR2(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[2]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_CMP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[2]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[0]));
				break;
			case OPCODE_COS:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = emit_const4fv(fp, RCP_2PI);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
					| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_FRC
					| R500_ALPHA_ADDRD(get_temp(fp, 1))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 1));
				counter++;
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0;
				fp->inst[counter].inst4 |= R500_ALPHA_OP_COS
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_DP3:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_DP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_DP3
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_DP4:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				/* Based on DP3 */
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_DP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_DP4
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_DPH:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				/* Based on DP3 */
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_DP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_DP4
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_DST:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				/* [1, src0.y*src1.y, src0.z, src1.w]
				 * So basically MUL with lotsa swizzling. */
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| R500_ALU_RGB_SEL_B_SRC1;
				/* Select [1, y, z, 1] */
				temp_swiz = (make_rgb_swizzle(fpi->SrcReg[0]) & ~0x7) | R500_SWIZZLE_ONE;
				fp->inst[counter].inst3 |= MAKE_SWIZ_RGB_A(temp_swiz);
				/* Select [1, y, 1, w] */
				temp_swiz = (make_rgb_swizzle(fpi->SrcReg[0]) & ~0x1c7) | R500_SWIZZLE_ONE | (R500_SWIZZLE_ONE << 6);
				fp->inst[counter].inst3 |= MAKE_SWIZ_RGB_B(temp_swiz);
				fp->inst[counter].inst4 |= R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				break;
			case OPCODE_EX2:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_EX2
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_FRC:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_FRC
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_LG2:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_LN2
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_LIT:
				/* To be honest, I have no idea how I came up with the following.
				 * All I know is that it's based on the r3xx stuff, and was
				 * concieved with the help of NyQuil. Mmm, MyQuil. */

				/* First instruction */
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = emit_const4fv(fp, LIT);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARG << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ZERO);
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAX
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0));
				counter++;
				/* Second instruction */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_AB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0)) | R500_ALPHA_ADDR1(src[1]);
				/* Select [z, z, z, y] */
				temp_swiz = 2 | (2 << 3) | (2 << 6);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(temp_swiz)
					| R500_ALU_RGB_SEL_B_SRC0
					| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_LN2
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_G;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MIN
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0));
				counter++;
				/* Third instruction */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_AG << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				/* Select [x, x, x, z] */
				temp_swiz = 0;
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(temp_swiz)
					| R500_ALU_RGB_SEL_B_SRC0
					| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 1))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_B
					| R500_ALPHA_SEL_B_SRC0 | R500_ALPHA_SWIZ_B_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 1))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| R500_ALU_RGBA_A_SWIZ_0;
				counter++;
				/* Fourth instruction */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_AR << 11);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst3 = MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
					| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
				fp->inst[counter].inst4 = R500_ALPHA_OP_EX2
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				/* Fifth instruction */
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_AB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				/* Select [w, w, w] */
				temp_swiz = 3 | (3 << 3) | (3 << 6);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ZERO)
					| R500_ALU_RGB_SEL_B_SRC0
					| MAKE_SWIZ_RGB_B(temp_swiz);
				fp->inst[counter].inst4 |= R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SWIZ_A_1
					| R500_ALPHA_SWIZ_B_1;
				/* Select [-y, -y, -y] */
				temp_swiz = 1 | (1 << 3) | (1 << 6);
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| MAKE_SWIZ_RGBA_C(temp_swiz)
					| R500_ALU_RGBA_MOD_C_NEG
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				/* Final instruction */
				emit_mov(fp, counter, fpi, get_temp(fp, 0), 1672, dest);
				break;
			case OPCODE_LRP:
				/* src0 * src1 + INV(src0) * src2
				 * 1) MUL src0, src1, temp
				 * 2) PRE 1-src0; MAD srcp, src2, temp */
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				src[2] = make_src(fp, fpi->SrcReg[2]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| R500_INST_NOP | (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[2])
					| R500_RGB_ADDR2(get_temp(fp, 0))
					| R500_RGB_SRCP_OP_1_MINUS_RGB0;
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[2])
					| R500_ALPHA_ADDR2(get_temp(fp, 0))
					| R500_ALPHA_SRCP_OP_1_MINUS_A0;
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRCP
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 |= R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRCP | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC2 | MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[2]))
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[2]));
				break;
			case OPCODE_MAD:
				emit_mad(fp, counter, fpi, 0, 1, 2);
				break;
			case OPCODE_MAX:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1
					| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_MAX
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_MIN:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1
					| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_MIN
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MIN
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_MOV:
				emit_mov(fp, counter, fpi, make_src(fp, fpi->SrcReg[0]), fpi->SrcReg[0].Swizzle, dest);
				break;
			case OPCODE_MUL:
				/* Variation on MAD: src0*src1+0 */
				emit_mad(fp, counter, fpi, 0, 1, R500_SWIZZLE_ZERO);
				break;
			case OPCODE_POW:
				/* POW(a,b) = EX2(LN2(a)*b) */
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_LN2
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0));
				counter++;
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0))
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0))
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 1))
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 1))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_EX2
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_RCP:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_RCP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_RSQ:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_RSQ
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_SCS:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = emit_const4fv(fp, RCP_2PI);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
					| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_FRC
					| R500_ALPHA_ADDRD(get_temp(fp, 1))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 1));
				counter++;
				/* Do a cosine, then a sine, masking out the channels we want to protect. */
				/* Cosine only goes in R (x) channel. */
				fpi->DstReg.WriteMask = 0x1;
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_COS
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				counter++;
				/* Sine only goes in G (y) channel. */
				fpi->DstReg.WriteMask = 0x2;
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 |= R500_ALPHA_OP_SIN
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_SGE:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR1(src[0])
					| R500_RGB_ADDR2(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR1(src[0])
					| R500_ALPHA_ADDR2(src[1]);
				fp->inst[counter].inst3 = /* 1 */
					MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_MOD_C_NEG
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_ALPHA_MOD_C_NEG;
				counter++;
				/* This inst's selects need to be swapped as follows:
				 * 0 -> C ; 1 -> B ; 2 -> A */
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
					| R500_ALU_RGB_SEL_B_SRC0
					| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ZERO);
				fp->inst[counter].inst4 |= R500_ALPHA_OP_CMP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ZERO);
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC0
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_RGB)
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC0
					| R500_ALU_RGBA_A_SWIZ_A;
				break;
			case OPCODE_SIN:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = emit_const4fv(fp, RCP_2PI);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
					| R500_RGB_ADDR1(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
					| R500_ALPHA_ADDR1(src[1]);
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
					| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
					| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
				counter++;
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB);
				fp->inst[counter].inst4 = R500_ALPHA_OP_FRC
					| R500_ALPHA_ADDRD(get_temp(fp, 1))
					| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 1));
				counter++;
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 1));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0;
				fp->inst[counter].inst4 |= R500_ALPHA_OP_SIN
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_sop_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
					| R500_ALU_RGBA_ADDRD(dest);
				break;
			case OPCODE_SLT:
				src[0] = make_src(fp, fpi->SrcReg[0]);
				src[1] = make_src(fp, fpi->SrcReg[1]);
				fp->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
					| (R500_WRITEMASK_ARGB << 11);
				fp->inst[counter].inst1 = R500_RGB_ADDR1(src[0])
					| R500_RGB_ADDR2(src[1]);
				fp->inst[counter].inst2 = R500_ALPHA_ADDR1(src[0])
					| R500_ALPHA_ADDR2(src[1]);
				fp->inst[counter].inst3 = /* 1 */
					MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
					| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst4 = R500_ALPHA_OP_MAD
					| R500_ALPHA_ADDRD(get_temp(fp, 0))
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
					| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
					| R500_ALU_RGBA_ADDRD(get_temp(fp, 0))
					| R500_ALU_RGBA_SEL_C_SRC2
					| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_MOD_C_NEG
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
					| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[1]))
					| R500_ALU_RGBA_ALPHA_MOD_C_NEG;
				counter++;
				/* This inst's selects need to be swapped as follows:
				 * 0 -> C ; 1 -> B ; 2 -> A */
				emit_alu(fp, counter, fpi);
				fp->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(fp, 0));
				fp->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
					| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ZERO)
					| R500_ALU_RGB_SEL_B_SRC0
					| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
				fp->inst[counter].inst4 |= R500_ALPHA_OP_CMP
					| R500_ALPHA_ADDRD(dest)
					| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ZERO)
					| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ONE);
				fp->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
					| R500_ALU_RGBA_ADDRD(dest)
					| R500_ALU_RGBA_SEL_C_SRC0
					| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_RGB)
					| R500_ALU_RGBA_ALPHA_SEL_C_SRC0
					| R500_ALU_RGBA_A_SWIZ_A;
				break;
			case OPCODE_SUB:
				/* Variation on MAD: 1*src0-src1 */
				fpi->SrcReg[1].NegateBase = 0xF; /* NEG_XYZW */
				emit_mad(fp, counter, fpi, R500_SWIZZLE_ONE, 0, 1);
				break;
			case OPCODE_SWZ:
				/* TODO: The rarer negation masks! */
				emit_mov(fp, counter, fpi, make_src(fp, fpi->SrcReg[0]), fpi->SrcReg[0].Swizzle, dest);
				break;
			case OPCODE_KIL:
			case OPCODE_TEX:
			case OPCODE_TXB:
			case OPCODE_TXP:
				emit_tex(fp, fpi, dest, counter);
					if (fpi->DstReg.File == PROGRAM_OUTPUT)
						counter++;
				break;
			default:
			        ERROR("unknown fpi->Opcode %s\n", _mesa_opcode_string(fpi->Opcode));
				break;
		}

		/* Finishing touches */
		if (fpi->SaturateMode == SATURATE_ZERO_ONE) {
			fp->inst[counter].inst0 |= R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP;
		}

		counter++;

		if (fp->error)
			return GL_FALSE;

	}

	/* Finish him! (If it's an ALU/OUT instruction...) */
	if ((fp->inst[counter-1].inst0 & 0x3) == 1) {
		fp->inst[counter-1].inst0 |= R500_INST_LAST;
	} else {
		/* We still need to put an output inst, right? */
		WARN_ONCE("Final FP instruction is not an OUT.\n");
	}

	fp->cs->nrslots = counter;

	fp->max_temp_idx++;

	return GL_TRUE;
}

static void init_program(r300ContextPtr r300, struct r500_fragment_program *fp)
{
	struct r300_pfs_compile_state *cs = NULL;
	struct gl_fragment_program *mp = &fp->mesa_program;
	struct prog_instruction *fpi;
	GLuint InputsRead = mp->Base.InputsRead;
	GLuint temps_used = 0;
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
	/* Size of pixel stack, plus 1. */
	fp->max_temp_idx = 1;
	/* Temp register offset. */
	fp->temp_reg_offset = 0;
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
	 * starting from register 0, so we're just going to do that instead.
	 */

	/* Texcoords come first */
	for (i = 0; i < fp->ctx->Const.MaxTextureUnits; i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			cs->inputs[FRAG_ATTRIB_TEX0 + i].refcount = 0;
			cs->inputs[FRAG_ATTRIB_TEX0 + i].reg =
				fp->temp_reg_offset;
			fp->temp_reg_offset++;
		}
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* fragment position treated as a texcoord */
	if (InputsRead & FRAG_BIT_WPOS) {
		cs->inputs[FRAG_ATTRIB_WPOS].refcount = 0;
		cs->inputs[FRAG_ATTRIB_WPOS].reg =
			fp->temp_reg_offset;
		fp->temp_reg_offset++;
	}
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Then primary colour */
	if (InputsRead & FRAG_BIT_COL0) {
		cs->inputs[FRAG_ATTRIB_COL0].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL0].reg =
			fp->temp_reg_offset;
		fp->temp_reg_offset++;
	}
	InputsRead &= ~FRAG_BIT_COL0;

	/* Secondary color */
	if (InputsRead & FRAG_BIT_COL1) {
		cs->inputs[FRAG_ATTRIB_COL1].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL1].reg =
			fp->temp_reg_offset;
		fp->temp_reg_offset++;
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

	if (!mp->Base.Instructions) {
		ERROR("No instructions found in program, going to go die now.\n");
		return;
	}

	for (fpi = mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {
		for (i = 0; i < 3; i++) {
			if (fpi->SrcReg[i].File == PROGRAM_TEMPORARY) {
				if (fpi->SrcReg[i].Index > temps_used)
					temps_used = fpi->SrcReg[i].Index;
			}
		}
	}

	cs->temp_in_use = temps_used;

	fp->max_temp_idx = fp->temp_reg_offset + cs->temp_in_use + 1;
}

static void update_params(struct r500_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;

	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (mp->Base.Parameters)
		_mesa_load_state_parameters(fp->ctx, mp->Base.Parameters);
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

void r500TranslateFragmentShader(r300ContextPtr r300,
				 struct r500_fragment_program *fp)
{

	struct r300_pfs_compile_state *cs = NULL;

	if (!fp->translated) {

		init_program(r300, fp);
		cs = fp->cs;

		if (parse_program(fp) == GL_FALSE) {
			ERROR("Huh. Couldn't parse program. There should be additional errors explaining why.\nUsing dumb shader...\n");
			dumb_shader(fp);
			fp->inst_offset = 0;
			fp->inst_end = cs->nrslots - 1;
			return;
		}
		fp->inst_offset = 0;
		fp->inst_end = cs->nrslots - 1;

		fp->translated = GL_TRUE;
		if (RADEON_DEBUG & DEBUG_PIXEL) {
			fprintf(stderr, "Mesa program:\n");
			fprintf(stderr, "-------------\n");
			_mesa_print_program(&fp->mesa_program.Base);
			fflush(stdout);
			dump_program(fp);
		}


		r300UpdateStateParameters(fp->ctx, _NEW_PROGRAM);
	}

	update_params(fp);

}

static char *toswiz(int swiz_val) {
  switch(swiz_val) {
  case 0: return "R";
  case 1: return "G";
  case 2: return "B";
  case 3: return "A";
  case 4: return "0";
  case 5: return "1/2";
  case 6: return "1";
  case 7: return "U";
  }
  return NULL;
}

static char *toop(int op_val)
{
  char *str;
  switch (op_val) {
  case 0: str = "MAD"; break;
  case 1: str = "DP3"; break;
  case 2: str = "DP4"; break;
  case 3: str = "D2A"; break;
  case 4: str = "MIN"; break;
  case 5: str = "MAX"; break;
  case 6: str = "Reserved"; break;
  case 7: str = "CND"; break;
  case 8: str = "CMP"; break;
  case 9: str = "FRC"; break;
  case 10: str = "SOP"; break;
  case 11: str = "MDH"; break;
  case 12: str = "MDV"; break;
  }
  return str;
}

static char *to_alpha_op(int op_val)
{
  char *str = NULL;
  switch (op_val) {
  case 0: str = "MAD"; break;
  case 1: str = "DP"; break;
  case 2: str = "MIN"; break;
  case 3: str = "MAX"; break;
  case 4: str = "Reserved"; break;
  case 5: str = "CND"; break;
  case 6: str = "CMP"; break;
  case 7: str = "FRC"; break;
  case 8: str = "EX2"; break;
  case 9: str = "LN2"; break;
  case 10: str = "RCP"; break;
  case 11: str = "RSQ"; break;
  case 12: str = "SIN"; break;
  case 13: str = "COS"; break;
  case 14: str = "MDH"; break;
  case 15: str = "MDV"; break;
  }
  return str;
}

static char *to_mask(int val)
{
  char *str = NULL;
  switch(val) {
  case 0: str = "NONE"; break;
  case 1: str = "R"; break;
  case 2: str = "G"; break;
  case 3: str = "RG"; break;
  case 4: str = "B"; break;
  case 5: str = "RB"; break;
  case 6: str = "GB"; break;
  case 7: str = "RGB"; break;
  case 8: str = "A"; break;
  case 9: str = "AR"; break;
  case 10: str = "AG"; break;
  case 11: str = "ARG"; break;
  case 12: str = "AB"; break;
  case 13: str = "ARB"; break;
  case 14: str = "AGB"; break;
  case 15: str = "ARGB"; break;
  }
  return str;
}

static char *to_texop(int val)
{
  switch(val) {
  case 0: return "NOP";
  case 1: return "LD";
  case 2: return "TEXKILL";
  case 3: return "PROJ";
  case 4: return "LODBIAS";
  case 5: return "LOD";
  case 6: return "DXDY";
  }
  return NULL;
}

static void dump_program(struct r500_fragment_program *fp)
{
  int pc = 0;
  int n;
  uint32_t inst;
  uint32_t inst0;
  char *str = NULL;

  for (n = 0; n < fp->inst_end+1; n++) {
    inst0 = inst = fp->inst[n].inst0;
    fprintf(stderr,"%d\t0:CMN_INST   0x%08x:", n, inst);
    switch(inst & 0x3) {
    case R500_INST_TYPE_ALU: str = "ALU"; break;
    case R500_INST_TYPE_OUT: str = "OUT"; break;
    case R500_INST_TYPE_FC: str = "FC"; break;
    case R500_INST_TYPE_TEX: str = "TEX"; break;
    };
    fprintf(stderr,"%s %s %s %s %s ", str,
	    inst & R500_INST_TEX_SEM_WAIT ? "TEX_WAIT" : "",
	    inst & R500_INST_LAST ? "LAST" : "",
	    inst & R500_INST_NOP ? "NOP" : "",
	    inst & R500_INST_ALU_WAIT ? "ALU WAIT" : "");
    fprintf(stderr,"wmask: %s omask: %s\n", to_mask((inst >> 11) & 0xf),
	    to_mask((inst >> 15) & 0xf));

    switch(inst0 & 0x3) {
    case 0:
    case 1:
      fprintf(stderr,"\t1:RGB_ADDR   0x%08x:", fp->inst[n].inst1);
      inst = fp->inst[n].inst1;

      fprintf(stderr,"Addr0: %d%c, Addr1: %d%c, Addr2: %d%c, srcp:%d\n",
	      inst & 0xff, (inst & (1<<8)) ? 'c' : 't',
	      (inst >> 10) & 0xff, (inst & (1<<18)) ? 'c' : 't',
	      (inst >> 20) & 0xff, (inst & (1<<28)) ? 'c' : 't',
	      (inst >> 30));

      fprintf(stderr,"\t2:ALPHA_ADDR 0x%08x:", fp->inst[n].inst2);
      inst = fp->inst[n].inst2;
      fprintf(stderr,"Addr0: %d%c, Addr1: %d%c, Addr2: %d%c, srcp:%d\n",
	      inst & 0xff, (inst & (1<<8)) ? 'c' : 't',
	      (inst >> 10) & 0xff, (inst & (1<<18)) ? 'c' : 't',
	      (inst >> 20) & 0xff, (inst & (1<<28)) ? 'c' : 't',
	      (inst >> 30));
      fprintf(stderr,"\t3 RGB_INST:  0x%08x:", fp->inst[n].inst3);
      inst = fp->inst[n].inst3;
      fprintf(stderr,"rgb_A_src:%d %s/%s/%s %d rgb_B_src:%d %s/%s/%s %d\n",
	      (inst) & 0x3, toswiz((inst >> 2) & 0x7), toswiz((inst >> 5) & 0x7), toswiz((inst >> 8) & 0x7),
	      (inst >> 11) & 0x3,
	      (inst >> 13) & 0x3, toswiz((inst >> 15) & 0x7), toswiz((inst >> 18) & 0x7), toswiz((inst >> 21) & 0x7),
	      (inst >> 24) & 0x3);


      fprintf(stderr,"\t4 ALPHA_INST:0x%08x:", fp->inst[n].inst4);
      inst = fp->inst[n].inst4;
      fprintf(stderr,"%s dest:%d%s alp_A_src:%d %s %d alp_B_src:%d %s %d w:%d\n", to_alpha_op(inst & 0xf),
	      (inst >> 4) & 0x7f, inst & (1<<11) ? "(rel)":"",
	      (inst >> 12) & 0x3, toswiz((inst >> 14) & 0x7), (inst >> 17) & 0x3,
	      (inst >> 19) & 0x3, toswiz((inst >> 21) & 0x7), (inst >> 24) & 0x3,
	      (inst >> 31) & 0x1);

      fprintf(stderr,"\t5 RGBA_INST: 0x%08x:", fp->inst[n].inst5);
      inst = fp->inst[n].inst5;
      fprintf(stderr,"%s dest:%d%s rgb_C_src:%d %s/%s/%s %d alp_C_src:%d %s %d\n", toop(inst & 0xf),
	      (inst >> 4) & 0x7f, inst & (1<<11) ? "(rel)":"",
	      (inst >> 12) & 0x3, toswiz((inst >> 14) & 0x7), toswiz((inst >> 17) & 0x7), toswiz((inst >> 20) & 0x7),
	      (inst >> 23) & 0x3,
	      (inst >> 25) & 0x3, toswiz((inst >> 27) & 0x7), (inst >> 30) & 0x3);
      break;
    case 2:
      break;
    case 3:
      inst = fp->inst[n].inst1;
      fprintf(stderr,"\t1:TEX_INST:  0x%08x: id: %d op:%s, %s, %s %s\n", inst, (inst >> 16) & 0xf,
	      to_texop((inst >> 22) & 0x7), (inst & (1<<25)) ? "ACQ" : "",
	      (inst & (1<<26)) ? "IGNUNC" : "", (inst & (1<<27)) ? "UNSCALED" : "SCALED");
      inst = fp->inst[n].inst2;
      fprintf(stderr,"\t2:TEX_ADDR:  0x%08x: src: %d%s %s/%s/%s/%s dst: %d%s %s/%s/%s/%s\n", inst,
	      inst & 127, inst & (1<<7) ? "(rel)" : "",
	      toswiz((inst >> 8) & 0x3), toswiz((inst >> 10) & 0x3),
	      toswiz((inst >> 12) & 0x3), toswiz((inst >> 14) & 0x3),
	      (inst >> 16) & 127, inst & (1<<23) ? "(rel)" : "",
	      toswiz((inst >> 24) & 0x3), toswiz((inst >> 26) & 0x3),
	      toswiz((inst >> 28) & 0x3), toswiz((inst >> 30) & 0x3));

      fprintf(stderr,"\t3:TEX_DXDY:  0x%08x\n", fp->inst[n].inst3);
      break;
    }
    fprintf(stderr,"\n");
  }

}
