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

/* Mapping Mesa registers to R500 temporaries */
struct reg_acc {
	int reg;		/* Assigned hw temp */
	unsigned int refcount;	/* Number of uses by mesa program */
};

/**
 * Describe the current lifetime information for an R300 temporary
 */
struct reg_lifetime {
	/* Index of the first slot where this register is free in the sense
	   that it can be used as a new destination register.
	   This is -1 if the register has been assigned to a Mesa register
	   and the last access to the register has not yet been emitted */
	int free;

	/* Index of the first slot where this register is currently reserved.
	   This is used to stop e.g. a scalar operation from being moved
	   before the allocation time of a register that was first allocated
	   for a vector operation. */
	int reserved;

	/* Index of the first slot in which the register can be used as a
	   source without losing the value that is written by the last
	   emitted instruction that writes to the register */
	int vector_valid;
	int scalar_valid;

	/* Index to the slot where the register was last read.
	   This is also the first slot in which the register may be written again */
	int vector_lastread;
	int scalar_lastread;
};

/**
 * Store usage information about an ALU instruction slot during the
 * compilation of a fragment program.
 */
#define SLOT_SRC_VECTOR  (1<<0)
#define SLOT_SRC_SCALAR  (1<<3)
#define SLOT_SRC_BOTH    (SLOT_SRC_VECTOR | SLOT_SRC_SCALAR)
#define SLOT_OP_VECTOR   (1<<16)
#define SLOT_OP_SCALAR   (1<<17)
#define SLOT_OP_BOTH     (SLOT_OP_VECTOR | SLOT_OP_SCALAR)

struct r500_pfs_compile_slot {
	/* Bitmask indicating which parts of the slot are used, using SLOT_ constants
	   defined above */
	unsigned int used;

	/* Selected sources */
	int vsrc[3];
	int ssrc[3];
};

/**
 * Store information during compilation of fragment programs.
 */
struct r500_pfs_compile_state {
	struct r500_fragment_program_compiler *compiler;

	/* number of ALU slots used so far */
	int nrslots;

	/* Track which (parts of) slots are already filled with instructions */
	struct r500_pfs_compile_slot slot[PFS_MAX_ALU_INST];

	/* Track the validity of R300 temporaries */
	struct reg_lifetime hwtemps[PFS_NUM_TEMP_REGS];

	/* Used to map Mesa's inputs/temps onto hardware temps */
	int temp_in_use;
	struct reg_acc temps[PFS_NUM_TEMP_REGS];
	struct reg_acc inputs[32];	/* don't actually need 32... */

	/* Track usage of hardware temps, for register allocation,
	 * indirection detection, etc. */
	GLuint used_in_node;
	GLuint dest_in_node;
};

/*
 * Useful macros and values
 */
#define ERROR(fmt, args...) do {			\
		fprintf(stderr, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
		cs->compiler->fp->error = GL_TRUE;			\
	} while(0)

#define PROG_CODE struct r500_fragment_program_code *code = cs->compiler->code

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
#define R500_WRITEMASK_B 0x4
#define R500_WRITEMASK_RGB 0x7
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

static INLINE GLuint make_rgb_swizzle(struct prog_src_register src) {
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

static INLINE GLuint make_rgba_swizzle(GLuint src) {
	GLuint swiz = 0x0;
	GLuint temp;
	int i;
	for (i = 0; i < 4; i++) {
	        temp = GET_SWZ(src, i);
		/* Fix SWIZZLE_ONE */
		if (temp == 5) temp++;
		swiz |= temp << i*3;
	}
	return swiz;
}

static INLINE GLuint make_alpha_swizzle(struct prog_src_register src) {
	GLuint swiz = GET_SWZ(src.Swizzle, 3);

	if (swiz == 5) swiz++;

	if (src.NegateBase)
		swiz |= (R500_SWIZ_MOD_NEG << 3);

	return swiz;
}

static INLINE GLuint make_sop_swizzle(struct prog_src_register src) {
	GLuint swiz = GET_SWZ(src.Swizzle, 0);

	if (swiz == 5) swiz++;
	return swiz;
}

static INLINE GLuint make_strq_swizzle(struct prog_src_register src) {
	GLuint swiz = 0x0, temp = 0x0;
	int i;
	for (i = 0; i < 4; i++) {
		temp = GET_SWZ(src.Swizzle, i) & 0x3;
		swiz |= temp << i*2;
	}
	return swiz;
}

static int get_temp(struct r500_pfs_compile_state *cs, int slot) {

	PROG_CODE;

	int r = code->temp_reg_offset + cs->temp_in_use + slot;

	if (r > R500_US_NUM_TEMP_REGS) {
		ERROR("Too many temporary registers requested, can't compile!\n");
	}

	return r;
}

/* Borrowed verbatim from r300_fragprog since it hasn't changed. */
static GLuint emit_const4fv(struct r500_pfs_compile_state *cs,
			    const GLfloat * cp)
{
	PROG_CODE;

	GLuint reg = 0x0;
	int index;

	for (index = 0; index < code->const_nr; ++index) {
		if (code->constant[index] == cp)
			break;
	}

	if (index >= code->const_nr) {
		if (index >= R500_US_NUM_CONST_REGS) {
			ERROR("Out of hw constants!\n");
			return reg;
		}

		code->const_nr++;
		code->constant[index] = cp;
	}

	reg = index | REG_CONSTANT;
	return reg;
}

static GLuint make_src(struct r500_pfs_compile_state *cs, struct prog_src_register src) {
	PROG_CODE;
	GLuint reg;
	switch (src.File) {
	case PROGRAM_TEMPORARY:
		reg = src.Index + code->temp_reg_offset;
		break;
	case PROGRAM_INPUT:
		reg = cs->inputs[src.Index].reg;
		break;
	case PROGRAM_LOCAL_PARAM:
		reg = emit_const4fv(cs,
			cs->compiler->fp->mesa_program.Base.LocalParams[src.Index]);
		break;
	case PROGRAM_ENV_PARAM:
		reg = emit_const4fv(cs,
			cs->compiler->compiler.Ctx->FragmentProgram.Parameters[src.Index]);
		break;
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
	case PROGRAM_CONSTANT:
		reg = emit_const4fv(cs,
			cs->compiler->fp->mesa_program.Base.Parameters->ParameterValues[src.Index]);
		break;
	case PROGRAM_BUILTIN:
		reg = 0x0;
		break;
	default:
		ERROR("Can't handle src.File %x\n", src.File);
		reg = 0x0;
		break;
	}
	return reg;
}

static GLuint make_dest(struct r500_pfs_compile_state *cs, struct prog_dst_register dest) {
	PROG_CODE;
	GLuint reg;
	switch (dest.File) {
	case PROGRAM_TEMPORARY:
		reg = dest.Index + code->temp_reg_offset;
		break;
	case PROGRAM_OUTPUT:
		/* Eventually we may need to handle multiple
			* rendering targets... */
		reg = dest.Index;
		break;
	case PROGRAM_BUILTIN:
		reg = 0x0;
		break;
	default:
		ERROR("Can't handle dest.File %x\n", dest.File);
		reg = 0x0;
		break;
	}
	return reg;
}

static void emit_tex(struct r500_pfs_compile_state *cs,
		     struct prog_instruction *fpi, int dest, int counter)
{
	PROG_CODE;
	int hwsrc, hwdest;
	GLuint mask;

	mask = fpi->DstReg.WriteMask << 11;
	hwsrc = make_src(cs, fpi->SrcReg[0]);

	if (fpi->DstReg.File == PROGRAM_OUTPUT) {
		hwdest = get_temp(cs, 0);
	} else {
		hwdest = dest;
	}

	code->inst[counter].inst0 = R500_INST_TYPE_TEX | mask
		| R500_INST_TEX_SEM_WAIT;

	code->inst[counter].inst1 = R500_TEX_ID(fpi->TexSrcUnit)
		| R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED;

	if (fpi->TexSrcTarget == TEXTURE_RECT_INDEX)
	        code->inst[counter].inst1 |= R500_TEX_UNSCALED;

	switch (fpi->Opcode) {
	case OPCODE_KIL:
		code->inst[counter].inst1 |= R500_TEX_INST_TEXKILL;
		break;
	case OPCODE_TEX:
		code->inst[counter].inst1 |= R500_TEX_INST_LD;
		break;
	case OPCODE_TXB:
		code->inst[counter].inst1 |= R500_TEX_INST_LODBIAS;
		break;
	case OPCODE_TXP:
		code->inst[counter].inst1 |= R500_TEX_INST_PROJ;
		break;
	default:
		ERROR("emit_tex can't handle opcode %x\n", fpi->Opcode);
	}

	code->inst[counter].inst2 = R500_TEX_SRC_ADDR(hwsrc)
		| MAKE_SWIZ_TEX_STRQ(make_strq_swizzle(fpi->SrcReg[0]))
		/* | R500_TEX_SRC_S_SWIZ_R | R500_TEX_SRC_T_SWIZ_G
		| R500_TEX_SRC_R_SWIZ_B | R500_TEX_SRC_Q_SWIZ_A */
		| R500_TEX_DST_ADDR(hwdest)
		| R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;

	code->inst[counter].inst3 = 0x0;
	code->inst[counter].inst4 = 0x0;
	code->inst[counter].inst5 = 0x0;

	if (fpi->DstReg.File == PROGRAM_OUTPUT) {
		counter++;
		code->inst[counter].inst0 = R500_INST_TYPE_OUT
			| R500_INST_TEX_SEM_WAIT | (mask << 4);
		code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
		code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
		code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
			| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
			| R500_ALU_RGB_SEL_B_SRC0
			| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB)
			| R500_ALU_RGB_OMOD_DISABLE;
		code->inst[counter].inst4 = R500_ALPHA_OP_CMP
			| R500_ALPHA_ADDRD(dest)
			| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_ALPHA_SWIZ_A_A)
			| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(R500_ALPHA_SWIZ_A_A)
			| R500_ALPHA_OMOD_DISABLE;
		code->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
			| R500_ALU_RGBA_ADDRD(dest)
			| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
			| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
	}
}

static void emit_alu(struct r500_pfs_compile_state *cs, int counter, struct prog_instruction *fpi) {
	PROG_CODE;
	/* Ideally, we shouldn't have to explicitly clear memory here! */
	code->inst[counter].inst0 = 0x0;
	code->inst[counter].inst1 = 0x0;
	code->inst[counter].inst2 = 0x0;
	code->inst[counter].inst3 = 0x0;
	code->inst[counter].inst4 = 0x0;
	code->inst[counter].inst5 = 0x0;

	if (fpi->DstReg.File == PROGRAM_OUTPUT) {
		code->inst[counter].inst0 = R500_INST_TYPE_OUT;

		if (fpi->DstReg.Index == FRAG_RESULT_COLR)
			code->inst[counter].inst0 |= (fpi->DstReg.WriteMask << 15);

		if (fpi->DstReg.Index == FRAG_RESULT_DEPR) {
			code->inst[counter].inst4 |= R500_ALPHA_W_OMASK;
			/* Notify the state emission! */
			cs->compiler->fp->writes_depth = GL_TRUE;
		}
	} else {
		code->inst[counter].inst0 = R500_INST_TYPE_ALU
			/* pixel_mask */
			| (fpi->DstReg.WriteMask << 11);
	}

	code->inst[counter].inst0 |= R500_INST_TEX_SEM_WAIT;
}

static void emit_mov(struct r500_pfs_compile_state *cs, int counter, struct prog_instruction *fpi, GLuint src_reg, GLuint swizzle, GLuint dest) {
	PROG_CODE;
	/* The r3xx shader uses MAD to implement MOV. We are using CMP, since
	 * it is technically more accurate and recommended by ATI/AMD. */
	emit_alu(cs, counter, fpi);
	code->inst[counter].inst1 = R500_RGB_ADDR0(src_reg);
	code->inst[counter].inst2 = R500_ALPHA_ADDR0(src_reg);
	/* (De)mangle the swizzle from Mesa to R500. */
	swizzle = make_rgba_swizzle(swizzle);
	/* 0x1FF is 9 bits, size of an RGB swizzle. */
	code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
		| MAKE_SWIZ_RGB_A((swizzle & 0x1ff))
		| R500_ALU_RGB_SEL_B_SRC0
		| MAKE_SWIZ_RGB_B((swizzle & 0x1ff))
		| R500_ALU_RGB_OMOD_DISABLE;
	code->inst[counter].inst4 |= R500_ALPHA_OP_CMP
		| R500_ALPHA_ADDRD(dest)
		| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(GET_SWZ(swizzle, 3))
		| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(GET_SWZ(swizzle, 3))
		| R500_ALPHA_OMOD_DISABLE;
	code->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
		| R500_ALU_RGBA_ADDRD(dest)
		| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
		| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
}

static void emit_mad(struct r500_pfs_compile_state *cs, int counter, struct prog_instruction *fpi, int one, int two, int three) {
	PROG_CODE;
	/* Note: This code was all Corbin's. Corbin is a rather hackish coder.
	 * If you can make it pretty or fast, please do so! */
	emit_alu(cs, counter, fpi);
	/* Common MAD stuff */
	code->inst[counter].inst4 |= R500_ALPHA_OP_MAD
		| R500_ALPHA_ADDRD(make_dest(cs, fpi->DstReg));
	code->inst[counter].inst5 |= R500_ALU_RGBA_OP_MAD
		| R500_ALU_RGBA_ADDRD(make_dest(cs, fpi->DstReg));
	switch (one) {
		case 0:
		case 1:
		case 2:
			code->inst[counter].inst1 |= R500_RGB_ADDR0(make_src(cs, fpi->SrcReg[one]));
			code->inst[counter].inst2 |= R500_ALPHA_ADDR0(make_src(cs, fpi->SrcReg[one]));
			code->inst[counter].inst3 |= R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[one]));
			code->inst[counter].inst4 |= R500_ALPHA_SEL_A_SRC0
				| MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[one]));
			break;
		case R500_SWIZZLE_ZERO:
			code->inst[counter].inst3 |= MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ZERO);
			code->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ZERO);
			break;
		case R500_SWIZZLE_ONE:
			code->inst[counter].inst3 |= MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE);
			code->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE);
			break;
		default:
			ERROR("Bad src index in emit_mad: %d\n", one);
			break;
	}
	switch (two) {
		case 0:
		case 1:
		case 2:
			code->inst[counter].inst1 |= R500_RGB_ADDR1(make_src(cs, fpi->SrcReg[two]));
			code->inst[counter].inst2 |= R500_ALPHA_ADDR1(make_src(cs, fpi->SrcReg[two]));
			code->inst[counter].inst3 |= R500_ALU_RGB_SEL_B_SRC1
				| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[two]));
			code->inst[counter].inst4 |= R500_ALPHA_SEL_B_SRC1
				| MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[two]));
			break;
		case R500_SWIZZLE_ZERO:
			code->inst[counter].inst3 |= MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ZERO);
			code->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ZERO);
			break;
		case R500_SWIZZLE_ONE:
			code->inst[counter].inst3 |= MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
			code->inst[counter].inst4 |= MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ONE);
			break;
		default:
			ERROR("Bad src index in emit_mad: %d\n", two);
			break;
	}
	switch (three) {
		case 0:
		case 1:
		case 2:
			code->inst[counter].inst1 |= R500_RGB_ADDR2(make_src(cs, fpi->SrcReg[three]));
			code->inst[counter].inst2 |= R500_ALPHA_ADDR2(make_src(cs, fpi->SrcReg[three]));
			code->inst[counter].inst5 |= R500_ALU_RGBA_SEL_C_SRC2
				| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[three]))
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[three]));
			break;
		case R500_SWIZZLE_ZERO:
			code->inst[counter].inst5 |= MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			break;
		case R500_SWIZZLE_ONE:
			code->inst[counter].inst5 |= MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ONE)
			| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ONE);
			break;
		default:
			ERROR("Bad src index in emit_mad: %d\n", three);
			break;
	}
}

static void emit_sop(struct r500_pfs_compile_state *cs, int counter, struct prog_instruction *fpi, int opcode, GLuint src, GLuint swiz, GLuint dest) {
	PROG_CODE;
	emit_alu(cs, counter, fpi);
	code->inst[counter].inst1 = R500_RGB_ADDR0(src);
	code->inst[counter].inst2 = R500_ALPHA_ADDR0(src);
	code->inst[counter].inst4 |= R500_ALPHA_ADDRD(dest)
		| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(swiz);
	code->inst[counter].inst5 = R500_ALU_RGBA_OP_SOP
		| R500_ALU_RGBA_ADDRD(dest);
	switch (opcode) {
		case OPCODE_COS:
			code->inst[counter].inst4 |= R500_ALPHA_OP_COS;
			break;
		case OPCODE_EX2:
			code->inst[counter].inst4 |= R500_ALPHA_OP_EX2;
			break;
		case OPCODE_LG2:
			code->inst[counter].inst4 |= R500_ALPHA_OP_LN2;
			break;
		case OPCODE_RCP:
			code->inst[counter].inst4 |= R500_ALPHA_OP_RCP;
			break;
		case OPCODE_RSQ:
			code->inst[counter].inst4 |= R500_ALPHA_OP_RSQ;
			break;
		case OPCODE_SIN:
			code->inst[counter].inst4 |= R500_ALPHA_OP_SIN;
			break;
		default:
			ERROR("Bad opcode in emit_sop: %d\n", opcode);
			break;
	}
}

static int do_inst(struct r500_pfs_compile_state *cs, struct prog_instruction *fpi, int counter) {
	PROG_CODE;
	GLuint src[3], dest = 0;
	int temp_swiz = 0;

	if (fpi->Opcode != OPCODE_KIL) {
		dest = make_dest(cs, fpi->DstReg);
	}

	switch (fpi->Opcode) {
		case OPCODE_ABS:
			emit_mov(cs, counter, fpi, make_src(cs, fpi->SrcReg[0]), fpi->SrcReg[0].Swizzle, dest);
			code->inst[counter].inst3 |= R500_ALU_RGB_MOD_A_ABS
				| R500_ALU_RGB_MOD_B_ABS;
			code->inst[counter].inst4 |= R500_ALPHA_MOD_A_ABS
				| R500_ALPHA_MOD_B_ABS;
			break;
		case OPCODE_ADD:
			/* Variation on MAD: 1*src0+src1 */
			emit_mad(cs, counter, fpi, R500_SWIZZLE_ONE, 0, 1);
			break;
		case OPCODE_CMP:
			/* This inst's selects need to be swapped as follows:
				* 0 -> C ; 1 -> B ; 2 -> A */
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			src[2] = make_src(cs, fpi->SrcReg[2]);
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[2])
				| R500_RGB_ADDR1(src[1]) | R500_RGB_ADDR2(src[0]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[2])
				| R500_ALPHA_ADDR1(src[1]) | R500_ALPHA_ADDR2(src[0]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[2]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_CMP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[2]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC2
				| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[0]));
			break;
		case OPCODE_COS:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = emit_const4fv(cs, RCP_2PI);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_FRC
				| R500_ALPHA_ADDRD(get_temp(cs, 1))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 1));
			counter++;
			emit_sop(cs, counter, fpi, OPCODE_COS, get_temp(cs, 1), make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_DP3:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_DP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_DP3
				| R500_ALU_RGBA_ADDRD(dest);
			break;
		case OPCODE_DP4:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			/* Based on DP3 */
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_DP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_DP4
				| R500_ALU_RGBA_ADDRD(dest);
			break;
		case OPCODE_DPH:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			/* Based on DP3 */
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_DP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_DP4
				| R500_ALU_RGBA_ADDRD(dest);
			break;
		case OPCODE_DST:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			/* [1, src0.y*src1.y, src0.z, src1.w]
				* So basically MUL with lotsa swizzling. */
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| R500_ALU_RGB_SEL_B_SRC1;
			/* Select [1, y, z, 1] */
			temp_swiz = (make_rgb_swizzle(fpi->SrcReg[0]) & ~0x7) | R500_SWIZZLE_ONE;
			code->inst[counter].inst3 |= MAKE_SWIZ_RGB_A(temp_swiz);
			/* Select [1, y, 1, w] */
			temp_swiz = (make_rgb_swizzle(fpi->SrcReg[0]) & ~0x1c7) | R500_SWIZZLE_ONE | (R500_SWIZZLE_ONE << 6);
			code->inst[counter].inst3 |= MAKE_SWIZ_RGB_B(temp_swiz);
			code->inst[counter].inst4 |= R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(dest)
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			break;
		case OPCODE_EX2:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, counter, fpi, OPCODE_EX2, src[0], make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_FLR:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_FRC
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0));
			counter++;
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(get_temp(cs, 0));
			code->inst[counter].inst3 = MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
				| R500_ALU_RGB_SEL_B_SRC0 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SWIZ_A_A
				| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC1
				| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC1
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGBA_MOD_C_NEG;
			break;
		case OPCODE_FRC:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_FRC
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
				| R500_ALU_RGBA_ADDRD(dest);
			break;
		case OPCODE_LG2:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, counter, fpi, OPCODE_LG2, src[0], make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_LIT:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = emit_const4fv(cs, LIT);
			/* First inst: MAX temp, input, [0, 0, 0, -128]
				* Write: RG, A  */
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_ARG << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ZERO);
			code->inst[counter].inst4 = R500_ALPHA_OP_MAX
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0));
			counter++;
			/* Second inst: MIN temp, temp, [x, x, x, 128]
				* Write: A */
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_A << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0)) | R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0)) | R500_ALPHA_ADDR1(src[1]);
			/* code->inst[counter].inst3; */
			code->inst[counter].inst4 = R500_ALPHA_OP_MAX
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
				| R500_ALU_RGBA_ADDRD(dest);
			counter++;
			/* Third-fifth insts: POW temp, temp.y, temp.w
				* Write: B */
			emit_sop(cs, counter, fpi, OPCODE_LG2, get_temp(cs, 0), SWIZZLE_Y, get_temp(cs, 1));
			code->inst[counter].inst0 |= (R500_WRITEMASK_ARGB << 11);
			counter++;
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 1))
				| R500_RGB_ADDR1(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 1))
				| R500_ALPHA_ADDR1(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 1))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 1))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			emit_sop(cs, counter, fpi, OPCODE_EX2, get_temp(cs, 1), SWIZZLE_W, get_temp(cs, 0));
			code->inst[counter].inst0 |= (R500_WRITEMASK_B << 11);
			counter++;
			/* Sixth inst: CMP dest, temp.xxxx, temp.[1, x, z, 1], temp.[1, x, 0, 1];
				* Write: ARGB
				* This inst's selects need to be swapped as follows:
				* 0 -> C ; 1 -> B ; 2 -> A */
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| R500_ALU_RGB_R_SWIZ_A_1
				| R500_ALU_RGB_G_SWIZ_A_R
				| R500_ALU_RGB_B_SWIZ_A_B
				| R500_ALU_RGB_SEL_B_SRC0
				| R500_ALU_RGB_R_SWIZ_B_1
				| R500_ALU_RGB_G_SWIZ_B_R
				| R500_ALU_RGB_B_SWIZ_B_0;
			code->inst[counter].inst4 |= R500_ALPHA_OP_CMP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_1
				| R500_ALPHA_SEL_B_SRC0 | R500_ALPHA_SWIZ_B_1;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC0
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC0
				| R500_ALU_RGBA_R_SWIZ_R
				| R500_ALU_RGBA_G_SWIZ_R
				| R500_ALU_RGBA_B_SWIZ_R
				| R500_ALU_RGBA_A_SWIZ_R;
			break;
		case OPCODE_LRP:
			/* src0 * src1 + INV(src0) * src2
				* 1) MUL src0, src1, temp
				* 2) PRE 1-src0; MAD srcp, src2, temp */
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			src[2] = make_src(cs, fpi->SrcReg[2]);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| R500_INST_NOP | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[2])
				| R500_RGB_ADDR2(get_temp(cs, 0))
				| R500_RGB_SRCP_OP_1_MINUS_RGB0;
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[2])
				| R500_ALPHA_ADDR2(get_temp(cs, 0))
				| R500_ALPHA_SRCP_OP_1_MINUS_A0;
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRCP
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 |= R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRCP | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC2 | MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[2]))
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[2]));
			break;
		case OPCODE_MAD:
			emit_mad(cs, counter, fpi, 0, 1, 2);
			break;
		case OPCODE_MAX:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1
				| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_MAX
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAX
				| R500_ALU_RGBA_ADDRD(dest);
			break;
		case OPCODE_MIN:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0]) | R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0]) | R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1
				| MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 |= R500_ALPHA_OP_MIN
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MIN
				| R500_ALU_RGBA_ADDRD(dest);
			break;
		case OPCODE_MOV:
			emit_mov(cs, counter, fpi, make_src(cs, fpi->SrcReg[0]), fpi->SrcReg[0].Swizzle, dest);
			break;
		case OPCODE_MUL:
			/* Variation on MAD: src0*src1+0 */
			emit_mad(cs, counter, fpi, 0, 1, R500_SWIZZLE_ZERO);
			break;
		case OPCODE_POW:
			/* POW(a,b) = EX2(LN2(a)*b) */
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			emit_sop(cs, counter, fpi, OPCODE_LG2, src[0], make_sop_swizzle(fpi->SrcReg[0]), get_temp(cs, 0));
			code->inst[counter].inst0 |= (R500_WRITEMASK_ARGB << 11);
			counter++;
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0))
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0))
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(make_rgb_swizzle(fpi->SrcReg[0]))
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 1))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 1))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			emit_sop(cs, counter, fpi, OPCODE_EX2, get_temp(cs, 1), make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_RCP:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, counter, fpi, OPCODE_RCP, src[0], make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_RSQ:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, counter, fpi, OPCODE_RSQ, src[0], make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_SCS:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = emit_const4fv(cs, RCP_2PI);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_FRC
				| R500_ALPHA_ADDRD(get_temp(cs, 1))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 1));
			counter++;
			/* Do a cosine, then a sine, masking out the channels we want to protect. */
			/* Cosine only goes in R (x) channel. */
			fpi->DstReg.WriteMask = 0x1;
			emit_sop(cs, counter, fpi, OPCODE_COS, get_temp(cs, 1), make_sop_swizzle(fpi->SrcReg[0]), dest);
			counter++;
			/* Sine only goes in G (y) channel. */
			fpi->DstReg.WriteMask = 0x2;
			emit_sop(cs, counter, fpi, OPCODE_SIN, get_temp(cs, 1), make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_SGE:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR1(src[0])
				| R500_RGB_ADDR2(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR1(src[0])
				| R500_ALPHA_ADDR2(src[1]);
			code->inst[counter].inst3 = /* 1 */
				MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| R500_ALU_RGBA_SEL_C_SRC2
				| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[1]))
				| R500_ALU_RGBA_MOD_C_NEG
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[1]))
				| R500_ALU_RGBA_ALPHA_MOD_C_NEG;
			counter++;
			/* This inst's selects need to be swapped as follows:
				* 0 -> C ; 1 -> B ; 2 -> A */
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
				| R500_ALU_RGB_SEL_B_SRC0
				| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ZERO);
			code->inst[counter].inst4 |= R500_ALPHA_OP_CMP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
				| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ZERO);
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC0
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC0
				| R500_ALU_RGBA_A_SWIZ_A;
			break;
		case OPCODE_SIN:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = emit_const4fv(cs, RCP_2PI);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A
				| R500_ALPHA_SEL_B_SRC1 | R500_ALPHA_SWIZ_B_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_RGB);
			code->inst[counter].inst4 = R500_ALPHA_OP_FRC
				| R500_ALPHA_ADDRD(get_temp(cs, 1))
				| R500_ALPHA_SEL_A_SRC0 | R500_ALPHA_SWIZ_A_A;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_FRC
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 1));
			counter++;
			emit_sop(cs, counter, fpi, OPCODE_SIN, get_temp(cs, 1), make_sop_swizzle(fpi->SrcReg[0]), dest);
			break;
		case OPCODE_SLT:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_ARGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR1(src[0])
				| R500_RGB_ADDR2(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR1(src[0])
				| R500_ALPHA_ADDR2(src[1]);
			code->inst[counter].inst3 = /* 1 */
				MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ONE)
				| R500_ALU_RGB_SEL_B_SRC1 | MAKE_SWIZ_RGB_B(make_rgb_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ONE)
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[0]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| R500_ALU_RGBA_SEL_C_SRC2
				| MAKE_SWIZ_RGBA_C(make_rgb_swizzle(fpi->SrcReg[1]))
				| R500_ALU_RGBA_MOD_C_NEG
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC2
				| MAKE_SWIZ_ALPHA_C(make_alpha_swizzle(fpi->SrcReg[1]))
				| R500_ALU_RGBA_ALPHA_MOD_C_NEG;
			counter++;
			/* This inst's selects need to be swapped as follows:
				* 0 -> C ; 1 -> B ; 2 -> A */
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(get_temp(cs, 0));
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(R500_SWIZ_RGB_ZERO)
				| R500_ALU_RGB_SEL_B_SRC0
				| MAKE_SWIZ_RGB_B(R500_SWIZ_RGB_ONE);
			code->inst[counter].inst4 |= R500_ALPHA_OP_CMP
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(R500_SWIZZLE_ZERO)
				| R500_ALPHA_SEL_B_SRC0 | MAKE_SWIZ_ALPHA_B(R500_SWIZZLE_ONE);
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_CMP
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC0
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGBA_ALPHA_SEL_C_SRC0
				| R500_ALU_RGBA_A_SWIZ_A;
			break;
		case OPCODE_SUB:
			/* Variation on MAD: 1*src0-src1 */
			fpi->SrcReg[1].NegateBase = 0xF; /* NEG_XYZW */
			emit_mad(cs, counter, fpi, R500_SWIZZLE_ONE, 0, 1);
			break;
		case OPCODE_SWZ:
			/* TODO: The rarer negation masks! */
			emit_mov(cs, counter, fpi, make_src(cs, fpi->SrcReg[0]), fpi->SrcReg[0].Swizzle, dest);
			break;
		case OPCODE_XPD:
			/* src0 * src1 - src1 * src0
				* 1) MUL temp.xyz, src0.yzx, src1.zxy
				* 2) MAD src0.zxy, src1.yzx, -temp.xyz */
			src[0] = make_src(cs, fpi->SrcReg[0]);
			src[1] = make_src(cs, fpi->SrcReg[1]);
			code->inst[counter].inst0 = R500_INST_TYPE_ALU | R500_INST_TEX_SEM_WAIT
				| (R500_WRITEMASK_RGB << 11);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1]);
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1]);
			/* Select [y, z, x] */
			temp_swiz = make_rgb_swizzle(fpi->SrcReg[0]);
			temp_swiz = (GET_SWZ(temp_swiz, 1) << 0) | (GET_SWZ(temp_swiz, 2) << 3) | (GET_SWZ(temp_swiz, 0) << 6);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(temp_swiz);
			/* Select [z, x, y] */
			temp_swiz = make_rgb_swizzle(fpi->SrcReg[1]);
			temp_swiz = (GET_SWZ(temp_swiz, 2) << 0) | (GET_SWZ(temp_swiz, 0) << 3) | (GET_SWZ(temp_swiz, 1) << 6);
			code->inst[counter].inst3 |= R500_ALU_RGB_SEL_B_SRC1
				| MAKE_SWIZ_RGB_B(temp_swiz);
			code->inst[counter].inst4 = R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(get_temp(cs, 0))
				| R500_ALPHA_SEL_A_SRC0 | MAKE_SWIZ_ALPHA_A(make_alpha_swizzle(fpi->SrcReg[0]))
				| R500_ALPHA_SEL_B_SRC1 | MAKE_SWIZ_ALPHA_B(make_alpha_swizzle(fpi->SrcReg[1]));
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(get_temp(cs, 0))
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_ZERO)
				| MAKE_SWIZ_ALPHA_C(R500_SWIZZLE_ZERO);
			counter++;
			emit_alu(cs, counter, fpi);
			code->inst[counter].inst1 = R500_RGB_ADDR0(src[0])
				| R500_RGB_ADDR1(src[1])
				| R500_RGB_ADDR2(get_temp(cs, 0));
			code->inst[counter].inst2 = R500_ALPHA_ADDR0(src[0])
				| R500_ALPHA_ADDR1(src[1])
				| R500_ALPHA_ADDR2(get_temp(cs, 0));
			/* Select [z, x, y] */
			temp_swiz = make_rgb_swizzle(fpi->SrcReg[0]);
			temp_swiz = (GET_SWZ(temp_swiz, 2) << 0) | (GET_SWZ(temp_swiz, 0) << 3) | (GET_SWZ(temp_swiz, 1) << 6);
			code->inst[counter].inst3 = R500_ALU_RGB_SEL_A_SRC0
				| MAKE_SWIZ_RGB_A(temp_swiz);
			/* Select [y, z, x] */
			temp_swiz = make_rgb_swizzle(fpi->SrcReg[1]);
			temp_swiz = (GET_SWZ(temp_swiz, 1) << 0) | (GET_SWZ(temp_swiz, 2) << 3) | (GET_SWZ(temp_swiz, 0) << 6);
			code->inst[counter].inst3 |= R500_ALU_RGB_SEL_B_SRC1
				| MAKE_SWIZ_RGB_B(temp_swiz);
			code->inst[counter].inst4 |= R500_ALPHA_OP_MAD
				| R500_ALPHA_ADDRD(dest)
				| R500_ALPHA_SWIZ_A_1
				| R500_ALPHA_SWIZ_B_1;
			code->inst[counter].inst5 = R500_ALU_RGBA_OP_MAD
				| R500_ALU_RGBA_ADDRD(dest)
				| R500_ALU_RGBA_SEL_C_SRC2
				| MAKE_SWIZ_RGBA_C(R500_SWIZ_RGB_RGB)
				| R500_ALU_RGBA_MOD_C_NEG
				| R500_ALU_RGBA_A_SWIZ_0;
			break;
		case OPCODE_KIL:
		case OPCODE_TEX:
		case OPCODE_TXB:
		case OPCODE_TXP:
			emit_tex(cs, fpi, dest, counter);
				if (fpi->DstReg.File == PROGRAM_OUTPUT)
					counter++;
			break;
		default:
			ERROR("unknown fpi->Opcode %s\n", _mesa_opcode_string(fpi->Opcode));
			break;
	}

	/* Finishing touches */
	if (fpi->SaturateMode == SATURATE_ZERO_ONE) {
		code->inst[counter].inst0 |= R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP;
	}

	counter++;

	return counter;
}

static GLboolean parse_program(struct r500_pfs_compile_state *cs)
{
	PROG_CODE;
	int clauseidx, counter = 0;

	for (clauseidx = 0; clauseidx < cs->compiler->compiler.NumClauses; clauseidx++) {
		struct radeon_clause* clause = &cs->compiler->compiler.Clauses[clauseidx];
		struct prog_instruction* fpi;

		int ip;

		for (ip = 0; ip < clause->NumInstructions; ip++) {
			fpi = clause->Instructions + ip;
			counter = do_inst(cs, fpi, counter);

			if (cs->compiler->fp->error)
				return GL_FALSE;
		}
	}

	/* Finish him! (If it's an ALU/OUT instruction...) */
	if ((code->inst[counter-1].inst0 & 0x3) == 1) {
		code->inst[counter-1].inst0 |= R500_INST_LAST;
	} else {
		/* We still need to put an output inst, right? */
		WARN_ONCE("Final FP instruction is not an OUT.\n");
	}

	cs->nrslots = counter;

	code->max_temp_idx++;

	return GL_TRUE;
}

static void init_program(struct r500_pfs_compile_state *cs)
{
	PROG_CODE;
	struct gl_fragment_program *mp = &cs->compiler->fp->mesa_program;
	struct prog_instruction *fpi;
	GLuint InputsRead = mp->Base.InputsRead;
	GLuint temps_used = 0;
	int i, j;

	/* New compile, reset tracking data */
	cs->compiler->fp->optimization =
	    driQueryOptioni(&cs->compiler->r300->radeon.optionCache, "fp_optimization");
	cs->compiler->fp->translated = GL_FALSE;
	cs->compiler->fp->error = GL_FALSE;
	code->const_nr = 0;
	/* Size of pixel stack, plus 1. */
	code->max_temp_idx = 1;
	/* Temp register offset. */
	code->temp_reg_offset = 0;
	/* Whether or not we perform any depth writing. */
	cs->compiler->fp->writes_depth = GL_FALSE;

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
	for (i = 0; i < cs->compiler->fp->ctx->Const.MaxTextureUnits; i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			cs->inputs[FRAG_ATTRIB_TEX0 + i].refcount = 0;
			cs->inputs[FRAG_ATTRIB_TEX0 + i].reg =
				code->temp_reg_offset;
			code->temp_reg_offset++;
		}
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* fragment position treated as a texcoord */
	if (InputsRead & FRAG_BIT_WPOS) {
		cs->inputs[FRAG_ATTRIB_WPOS].refcount = 0;
		cs->inputs[FRAG_ATTRIB_WPOS].reg =
			code->temp_reg_offset;
		code->temp_reg_offset++;
	}
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Then primary colour */
	if (InputsRead & FRAG_BIT_COL0) {
		cs->inputs[FRAG_ATTRIB_COL0].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL0].reg =
			code->temp_reg_offset;
		code->temp_reg_offset++;
	}
	InputsRead &= ~FRAG_BIT_COL0;

	/* Secondary color */
	if (InputsRead & FRAG_BIT_COL1) {
		cs->inputs[FRAG_ATTRIB_COL1].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL1].reg =
			code->temp_reg_offset;
		code->temp_reg_offset++;
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

	int clauseidx;

	for (clauseidx = 0; clauseidx < cs->compiler->compiler.NumClauses; ++clauseidx) {
		struct radeon_clause* clause = &cs->compiler->compiler.Clauses[clauseidx];
		int ip;

		for (ip = 0; ip < clause->NumInstructions; ip++) {
			fpi = clause->Instructions + ip;
			for (i = 0; i < 3; i++) {
				if (fpi->SrcReg[i].File == PROGRAM_TEMPORARY) {
					if (fpi->SrcReg[i].Index >= temps_used)
						temps_used = fpi->SrcReg[i].Index + 1;
				}
			}
		}
	}


	cs->temp_in_use = temps_used + 1;

	code->max_temp_idx = code->temp_reg_offset + cs->temp_in_use;

	if (RADEON_DEBUG & DEBUG_PIXEL)
		fprintf(stderr, "FP temp indices: code->max_temp_idx: %d cs->temp_in_use: %d\n", code->max_temp_idx, cs->temp_in_use);
}

static void dumb_shader(struct r500_pfs_compile_state *cs)
{
	PROG_CODE;
	code->inst[0].inst0 = R500_INST_TYPE_TEX
		| R500_INST_TEX_SEM_WAIT
		| R500_INST_RGB_WMASK_R
		| R500_INST_RGB_WMASK_G
		| R500_INST_RGB_WMASK_B
		| R500_INST_ALPHA_WMASK
		| R500_INST_RGB_CLAMP
		| R500_INST_ALPHA_CLAMP;
	code->inst[0].inst1 = R500_TEX_ID(0)
		| R500_TEX_INST_LD
		| R500_TEX_SEM_ACQUIRE
		| R500_TEX_IGNORE_UNCOVERED;
	code->inst[0].inst2 = R500_TEX_SRC_ADDR(0)
		| R500_TEX_SRC_S_SWIZ_R
		| R500_TEX_SRC_T_SWIZ_G
		| R500_TEX_DST_ADDR(0)
		| R500_TEX_DST_R_SWIZ_R
		| R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B
		| R500_TEX_DST_A_SWIZ_A;
	code->inst[0].inst3 = R500_DX_ADDR(0)
		| R500_DX_S_SWIZ_R
		| R500_DX_T_SWIZ_R
		| R500_DX_R_SWIZ_R
		| R500_DX_Q_SWIZ_R
		| R500_DY_ADDR(0)
		| R500_DY_S_SWIZ_R
		| R500_DY_T_SWIZ_R
		| R500_DY_R_SWIZ_R
		| R500_DY_Q_SWIZ_R;
	code->inst[0].inst4 = 0x0;
	code->inst[0].inst5 = 0x0;

	code->inst[1].inst0 = R500_INST_TYPE_OUT |
		R500_INST_TEX_SEM_WAIT |
		R500_INST_LAST |
		R500_INST_RGB_OMASK_R |
		R500_INST_RGB_OMASK_G |
		R500_INST_RGB_OMASK_B |
		R500_INST_ALPHA_OMASK;
	code->inst[1].inst1 = R500_RGB_ADDR0(0) |
		R500_RGB_ADDR1(0) |
		R500_RGB_ADDR1_CONST |
		R500_RGB_ADDR2(0) |
		R500_RGB_ADDR2_CONST |
		R500_RGB_SRCP_OP_1_MINUS_2RGB0;
	code->inst[1].inst2 = R500_ALPHA_ADDR0(0) |
		R500_ALPHA_ADDR1(0) |
		R500_ALPHA_ADDR1_CONST |
		R500_ALPHA_ADDR2(0) |
		R500_ALPHA_ADDR2_CONST |
		R500_ALPHA_SRCP_OP_1_MINUS_2A0;
	code->inst[1].inst3 = R500_ALU_RGB_SEL_A_SRC0 |
		R500_ALU_RGB_R_SWIZ_A_R |
		R500_ALU_RGB_G_SWIZ_A_G |
		R500_ALU_RGB_B_SWIZ_A_B |
		R500_ALU_RGB_SEL_B_SRC0 |
		R500_ALU_RGB_R_SWIZ_B_1 |
		R500_ALU_RGB_B_SWIZ_B_1 |
		R500_ALU_RGB_G_SWIZ_B_1;
	code->inst[1].inst4 = R500_ALPHA_OP_MAD |
		R500_ALPHA_SWIZ_A_A |
		R500_ALPHA_SWIZ_B_1;
	code->inst[1].inst5 = R500_ALU_RGBA_OP_MAD |
		R500_ALU_RGBA_R_SWIZ_0 |
		R500_ALU_RGBA_G_SWIZ_0 |
		R500_ALU_RGBA_B_SWIZ_0 |
		R500_ALU_RGBA_A_SWIZ_0;

	cs->nrslots = 2;
}

GLboolean r500FragmentProgramEmit(struct r500_fragment_program_compiler *compiler)
{
	struct r500_pfs_compile_state cs;
	struct r500_fragment_program_code *code = compiler->code;

	_mesa_memset(&cs, 0, sizeof(cs));
	cs.compiler = compiler;
	init_program(&cs);

	if (!parse_program(&cs)) {
#if 0
		ERROR("Huh. Couldn't parse program. There should be additional errors explaining why.\nUsing dumb shader...\n");
		dumb_shader(fp);
		code->inst_offset = 0;
		code->inst_end = cs.nrslots - 1;
#endif
		return GL_FALSE;
	}

	code->inst_offset = 0;
	code->inst_end = cs.nrslots - 1;

	return GL_TRUE;
}
