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
 * Store information during compilation of fragment programs.
 */
struct r500_pfs_compile_state {
	struct r500_fragment_program_compiler *compiler;

	/* number of ALU slots used so far */
	int nrslots;

	/* Used to map Mesa's inputs/temps onto hardware temps */
	int temp_in_use;
	struct reg_acc inputs[32];	/* don't actually need 32... */
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

static const struct prog_dst_register dstreg_template = {
	.File = PROGRAM_TEMPORARY,
	.Index = 0,
	.WriteMask = WRITEMASK_XYZW
};

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
	if (src.Abs) {
		swiz |= R500_SWIZ_MOD_ABS << 9;
	} else if (src.NegateBase & 7) {
		ASSERT((src.NegateBase & 7) == 7);
		swiz |= R500_SWIZ_MOD_NEG << 9;
	}
	if (src.NegateAbs)
		swiz ^= R500_SWIZ_MOD_NEG << 9;
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

	if (src.Abs) {
		swiz |= R500_SWIZ_MOD_ABS << 3;
	} else if (src.NegateBase & 8) {
		swiz |= R500_SWIZ_MOD_NEG << 3;
	}
	if (src.NegateAbs)
		swiz ^= R500_SWIZ_MOD_NEG << 3;

	return swiz;
}

static INLINE GLuint make_sop_swizzle(struct prog_src_register src) {
	GLuint swiz = GET_SWZ(src.Swizzle, 0);

	if (swiz == 5) swiz++;

	if (src.Abs) {
		swiz |= R500_SWIZ_MOD_ABS << 3;
	} else if (src.NegateBase & 1) {
		swiz |= R500_SWIZ_MOD_NEG << 3;
	}
	if (src.NegateAbs)
		swiz ^= R500_SWIZ_MOD_NEG << 3;

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
			    struct prog_src_register srcreg)
{
	PROG_CODE;

	GLuint reg = 0x0;
	int index;

	for (index = 0; index < code->const_nr; ++index) {
		if (code->constant[index].File == srcreg.File &&
		    code->constant[index].Index == srcreg.Index)
			break;
	}

	if (index >= code->const_nr) {
		if (index >= R500_US_NUM_CONST_REGS) {
			ERROR("Out of hw constants!\n");
			return reg;
		}

		code->const_nr++;
		code->constant[index] = srcreg;
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
	case PROGRAM_ENV_PARAM:
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
	case PROGRAM_CONSTANT:
		reg = emit_const4fv(cs, src);
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

static int emit_slot(struct r500_pfs_compile_state *cs)
{
	if (cs->nrslots >= 512) {
		ERROR("Too many instructions");
		cs->nrslots = 1;
		return 0;
	}
	return cs->nrslots++;
}

static int emit_tex(struct r500_pfs_compile_state *cs,
		     struct prog_instruction *fpi, int dest)
{
	PROG_CODE;
	int hwsrc, hwdest;
	GLuint mask;
	int counter = emit_slot(cs);

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

	return counter;
}

/* Do not call directly */
static int _helper_emit_alu(struct r500_pfs_compile_state *cs, GLuint rgbop, GLuint alphaop,
	int File, int Index, int WriteMask)
{
	PROG_CODE;
	int counter = emit_slot(cs);

	code->inst[counter].inst4 = alphaop;
	code->inst[counter].inst5 = rgbop;

	if (File == PROGRAM_OUTPUT) {
		code->inst[counter].inst0 = R500_INST_TYPE_OUT;

		if (Index == FRAG_RESULT_COLR) {
			code->inst[counter].inst0 |= WriteMask << 15;
		} else if (Index == FRAG_RESULT_DEPR) {
			code->inst[counter].inst4 |= R500_ALPHA_W_OMASK;
			cs->compiler->fp->writes_depth = GL_TRUE;
		}
	} else {
		int dest = Index + code->temp_reg_offset;

		code->inst[counter].inst0 = R500_INST_TYPE_ALU
			| (WriteMask << 11);
		code->inst[counter].inst4 |= R500_ALPHA_ADDRD(dest);
		code->inst[counter].inst5 |= R500_ALU_RGBA_ADDRD(dest);
	}

	code->inst[counter].inst0 |= R500_INST_TEX_SEM_WAIT;

	return counter;
}

/**
 * Prepare an ALU slot with the given RGB operation, ALPHA operation, and
 * destination register.
 */
static int emit_alu(struct r500_pfs_compile_state *cs, GLuint rgbop, GLuint alphaop, struct prog_dst_register dst)
{
	return _helper_emit_alu(cs, rgbop, alphaop, dst.File, dst.Index, dst.WriteMask);
}

static int emit_alu_temp(struct r500_pfs_compile_state *cs, GLuint rgbop, GLuint alphaop, int dst, int writemask)
{
	return _helper_emit_alu(cs, rgbop, alphaop,
		PROGRAM_TEMPORARY, dst - cs->compiler->code->temp_reg_offset, writemask);
}

/**
 * Set an instruction's source 0 (both RGB and ALPHA) to the given hardware index.
 */
static void set_src0_direct(struct r500_pfs_compile_state *cs, int ip, GLuint src)
{
	PROG_CODE;
	code->inst[ip].inst1 |= R500_RGB_ADDR0(src);
	code->inst[ip].inst2 |= R500_ALPHA_ADDR0(src);
}

/**
 * Set an instruction's source 1 (both RGB and ALPHA) to the given hardware index.
 */
static void set_src1_direct(struct r500_pfs_compile_state *cs, int ip, GLuint src)
{
	PROG_CODE;
	code->inst[ip].inst1 |= R500_RGB_ADDR1(src);
	code->inst[ip].inst2 |= R500_ALPHA_ADDR1(src);
}

/**
 * Set an instruction's source 2 (both RGB and ALPHA) to the given hardware index.
 */
static void set_src2_direct(struct r500_pfs_compile_state *cs, int ip, GLuint src)
{
	PROG_CODE;
	code->inst[ip].inst1 |= R500_RGB_ADDR2(src);
	code->inst[ip].inst2 |= R500_ALPHA_ADDR2(src);
}

/**
 * Set an instruction's source 0 (both RGB and ALPHA) according to the given source register.
 */
static void set_src0(struct r500_pfs_compile_state *cs, int ip, struct prog_src_register srcreg)
{
	set_src0_direct(cs, ip, make_src(cs, srcreg));
}

/**
 * Set an instruction's source 1 (both RGB and ALPHA) according to the given source register.
 */
static void set_src1(struct r500_pfs_compile_state *cs, int ip, struct prog_src_register srcreg)
{
	set_src1_direct(cs, ip, make_src(cs, srcreg));
}

/**
 * Set an instruction's source 2 (both RGB and ALPHA) according to the given source register.
 */
static void set_src2(struct r500_pfs_compile_state *cs, int ip, struct prog_src_register srcreg)
{
	set_src2_direct(cs, ip, make_src(cs, srcreg));
}

/**
 * Set an instruction's argument A (both RGB and ALPHA) from the given source,
 * taking swizzles+neg+abs as specified (see also _reg version below).
 */
static void set_argA(struct r500_pfs_compile_state *cs, int ip, int source, GLuint swizRGB, GLuint swizA)
{
	PROG_CODE;
	code->inst[ip].inst3 |= (source << R500_ALU_RGB_SEL_A_SHIFT) | MAKE_SWIZ_RGB_A(swizRGB);
	code->inst[ip].inst4 |= (source << R500_ALPHA_SEL_A_SHIFT) | MAKE_SWIZ_ALPHA_A(swizA);
}

/**
 * Set an instruction's argument B (both RGB and ALPHA) from the given source,
 * taking swizzles+neg+abs as specified (see also _reg version below).
 */
static void set_argB(struct r500_pfs_compile_state *cs, int ip, int source, GLuint swizRGB, GLuint swizA)
{
	PROG_CODE;
	code->inst[ip].inst3 |= (source << R500_ALU_RGB_SEL_B_SHIFT) | MAKE_SWIZ_RGB_B(swizRGB);
	code->inst[ip].inst4 |= (source << R500_ALPHA_SEL_B_SHIFT) | MAKE_SWIZ_ALPHA_B(swizA);
}

/**
 * Set an instruction's argument C (both RGB and ALPHA) from the given source,
 * taking swizzles+neg+abs as specified (see also _reg version below).
 */
static void set_argC(struct r500_pfs_compile_state *cs, int ip, int source, GLuint swizRGB, GLuint swizA)
{
	PROG_CODE;
	code->inst[ip].inst5 |=
		(source << R500_ALU_RGBA_SEL_C_SHIFT) |
		MAKE_SWIZ_RGBA_C(swizRGB) |
		(source << R500_ALU_RGBA_ALPHA_SEL_C_SHIFT) |
		MAKE_SWIZ_ALPHA_C(swizA);
}

/**
 * Set an instruction's argument A (both RGB and ALPHA) from the given source,
 * taking swizzles, negation and absolute value from the given source register.
 */
static void set_argA_reg(struct r500_pfs_compile_state *cs, int ip, int source, struct prog_src_register srcreg)
{
	set_argA(cs, ip, source, make_rgb_swizzle(srcreg), make_alpha_swizzle(srcreg));
}

/**
 * Set an instruction's argument B (both RGB and ALPHA) from the given source,
 * taking swizzles, negation and absolute value from the given source register.
 */
static void set_argB_reg(struct r500_pfs_compile_state *cs, int ip, int source, struct prog_src_register srcreg)
{
	set_argB(cs, ip, source, make_rgb_swizzle(srcreg), make_alpha_swizzle(srcreg));
}

/**
 * Set an instruction's argument C (both RGB and ALPHA) from the given source,
 * taking swizzles, negation and absolute value from the given source register.
 */
static void set_argC_reg(struct r500_pfs_compile_state *cs, int ip, int source, struct prog_src_register srcreg)
{
	set_argC(cs, ip, source, make_rgb_swizzle(srcreg), make_alpha_swizzle(srcreg));
}

/**
 * Emit a special scalar operation.
 */
static int emit_sop(struct r500_pfs_compile_state *cs,
	int opcode, struct prog_dst_register dstreg, GLuint src, GLuint swiz)
{
	int ip = emit_alu(cs, R500_ALU_RGBA_OP_SOP, opcode, dstreg);
	set_src0_direct(cs, ip, src);
	set_argA(cs, ip, 0, R500_SWIZ_RGB_ZERO, swiz);
	return ip;
}


/**
 * Emit trigonometric function COS, SIN, SCS
 */
static void emit_trig(struct r500_pfs_compile_state *cs, struct prog_instruction *fpi)
{
	int ip;
	struct prog_dst_register temp = dstreg_template;
	temp.Index = get_temp(cs, 0);
	temp.WriteMask = WRITEMASK_W;

	struct prog_src_register srcreg;
	GLuint constant_swizzle;

	srcreg.File = PROGRAM_CONSTANT;
	srcreg.Index = _mesa_add_unnamed_constant(cs->compiler->program->Parameters,
		RCP_2PI, 4, &constant_swizzle);
	srcreg.Swizzle = constant_swizzle;

	/* temp = Input*(1/2pi) */
	ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, temp);
	set_src0(cs, ip, fpi->SrcReg[0]);
	set_src1(cs, ip, srcreg);
	set_argA(cs, ip, 0, R500_SWIZ_RGB_ZERO, make_sop_swizzle(fpi->SrcReg[0]));
	set_argB(cs, ip, 1, R500_SWIZ_RGB_ZERO, make_alpha_swizzle(srcreg));
	set_argC(cs, ip, 0, R500_SWIZ_RGB_ZERO, R500_SWIZZLE_ZERO);

	/* temp = frac(dst) */
	ip = emit_alu(cs, R500_ALU_RGBA_OP_FRC, R500_ALPHA_OP_FRC, temp);
	set_src0_direct(cs, ip, temp.Index);
	set_argA(cs, ip, 0, R500_SWIZ_RGB_RGB, SWIZZLE_W);

	/* Dest = trig(temp) */
	if (fpi->Opcode == OPCODE_COS) {
		emit_sop(cs, R500_ALPHA_OP_COS, fpi->DstReg, temp.Index, SWIZZLE_W);
	} else if (fpi->Opcode == OPCODE_SIN) {
		emit_sop(cs, R500_ALPHA_OP_SIN, fpi->DstReg, temp.Index, SWIZZLE_W);
	} else if (fpi->Opcode == OPCODE_SCS) {
		struct prog_dst_register moddst = fpi->DstReg;

		if (fpi->DstReg.WriteMask & WRITEMASK_X) {
			moddst.WriteMask = WRITEMASK_X;
			emit_sop(cs, R500_ALPHA_OP_COS, fpi->DstReg, temp.Index, SWIZZLE_W);
		}
		if (fpi->DstReg.WriteMask & WRITEMASK_Y) {
			moddst.WriteMask = WRITEMASK_Y;
			emit_sop(cs, R500_ALPHA_OP_SIN, fpi->DstReg, temp.Index, SWIZZLE_W);
		}
	}
}

static void do_inst(struct r500_pfs_compile_state *cs, struct prog_instruction *fpi) {
	PROG_CODE;
	GLuint src[3], dest = 0;
	int ip;

	if (fpi->Opcode != OPCODE_KIL) {
		dest = make_dest(cs, fpi->DstReg);
	}

	switch (fpi->Opcode) {
		case OPCODE_ADD:
			/* Variation on MAD: 1*src0+src1 */
			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA(cs, ip, 0, R500_SWIZ_RGB_ONE, R500_SWIZZLE_ONE);
			set_argB_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argC_reg(cs, ip, 1, fpi->SrcReg[1]);
			break;
		case OPCODE_CMP:
			/* This inst's selects need to be swapped as follows:
				* 0 -> C ; 1 -> B ; 2 -> A */
			ip = emit_alu(cs, R500_ALU_RGBA_OP_CMP, R500_ALPHA_OP_CMP, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_src2(cs, ip, fpi->SrcReg[2]);
			set_argA_reg(cs, ip, 2, fpi->SrcReg[2]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			set_argC_reg(cs, ip, 0, fpi->SrcReg[0]);
			break;
		case OPCODE_COS:
			emit_trig(cs, fpi);
			break;
		case OPCODE_DP3:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_DP3, R500_ALPHA_OP_DP, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			break;
		case OPCODE_DP4:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_DP4, R500_ALPHA_OP_DP, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			break;
		case OPCODE_DST:
			/* [1, src0.y*src1.y, src0.z, src1.w]
			 * So basically MUL with lotsa swizzling. */
			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA(cs, ip, 0,
				(make_rgb_swizzle(fpi->SrcReg[0]) & ~0x7) | R500_SWIZZLE_ONE,
				R500_SWIZZLE_ONE);
			set_argB(cs, ip, 1,
				(make_rgb_swizzle(fpi->SrcReg[0]) & ~0x1c7) | R500_SWIZZLE_ONE | (R500_SWIZZLE_ONE << 6),
				make_alpha_swizzle(fpi->SrcReg[1]));
			set_argC(cs, ip, 0, R500_SWIZ_RGB_ZERO, R500_SWIZZLE_ZERO);
			break;
		case OPCODE_EX2:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, R500_ALPHA_OP_EX2, fpi->DstReg, src[0], make_sop_swizzle(fpi->SrcReg[0]));
			break;
		case OPCODE_FLR:
			dest = get_temp(cs, 0);
			ip = emit_alu_temp(cs, R500_ALU_RGBA_OP_FRC, R500_ALPHA_OP_FRC, dest, WRITEMASK_XYZW);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);

			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1_direct(cs, ip, dest);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[1]);
			set_argB(cs, ip, 0, R500_SWIZ_RGB_ONE, R500_SWIZZLE_ONE);
			set_argC(cs, ip, 1,
				R500_SWIZ_RGB_RGB|(R500_SWIZ_MOD_NEG<<9),
				SWIZZLE_W|(R500_SWIZ_MOD_NEG<<3));
			break;
		case OPCODE_FRC:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_FRC, R500_ALPHA_OP_FRC, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			break;
		case OPCODE_LG2:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, R500_ALPHA_OP_LN2, fpi->DstReg, src[0], make_sop_swizzle(fpi->SrcReg[0]));
			break;
		case OPCODE_LRP:
			/* result = src0*src1 + (1-src0)*src2
			 *        = src0*src1 + src2 + (-src0)*src2
			 *
			 * Note: LRP without swizzling (or with only limited
			 * swizzling) could be done more efficiently using the
			 * presubtract hardware.
			 */
			dest = get_temp(cs, 0);
			ip = emit_alu_temp(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, dest, WRITEMASK_XYZW);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_src2(cs, ip, fpi->SrcReg[2]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			set_argC_reg(cs, ip, 2, fpi->SrcReg[2]);

			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[2]);
			set_src2_direct(cs, ip, dest);
			set_argA(cs, ip, 0,
				make_rgb_swizzle(fpi->SrcReg[0]) ^ (R500_SWIZ_MOD_NEG<<9),
				make_alpha_swizzle(fpi->SrcReg[0]) ^ (R500_SWIZ_MOD_NEG<<3));
			set_argB_reg(cs, ip, 1, fpi->SrcReg[2]);
			set_argC(cs, ip, 2, R500_SWIZ_RGB_RGB, SWIZZLE_W);
			break;
		case OPCODE_MAD:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_src2(cs, ip, fpi->SrcReg[2]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			set_argC_reg(cs, ip, 2, fpi->SrcReg[2]);
			break;
		case OPCODE_MAX:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAX, R500_ALPHA_OP_MAX, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			break;
		case OPCODE_MIN:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_MIN, R500_ALPHA_OP_MIN, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			break;
		case OPCODE_MOV:
			ip = emit_alu(cs, R500_ALU_RGBA_OP_CMP, R500_ALPHA_OP_CMP, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argC(cs, ip, 0, R500_SWIZ_RGB_ZERO, R500_SWIZZLE_ZERO);
			code->inst[ip].inst3 |= R500_ALU_RGB_OMOD_DISABLE;
			code->inst[ip].inst4 |= R500_ALPHA_OMOD_DISABLE;
			break;
		case OPCODE_MUL:
			/* Variation on MAD: src0*src1+0 */
			ip = emit_alu(cs, R500_ALU_RGBA_OP_MAD, R500_ALPHA_OP_MAD, fpi->DstReg);
			set_src0(cs, ip, fpi->SrcReg[0]);
			set_src1(cs, ip, fpi->SrcReg[1]);
			set_argA_reg(cs, ip, 0, fpi->SrcReg[0]);
			set_argB_reg(cs, ip, 1, fpi->SrcReg[1]);
			set_argC(cs, ip, 0, R500_SWIZ_RGB_ZERO, R500_SWIZZLE_ZERO);
			break;
		case OPCODE_RCP:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, R500_ALPHA_OP_RCP, fpi->DstReg, src[0], make_sop_swizzle(fpi->SrcReg[0]));
			break;
		case OPCODE_RSQ:
			src[0] = make_src(cs, fpi->SrcReg[0]);
			emit_sop(cs, R500_ALPHA_OP_RSQ, fpi->DstReg, src[0], make_sop_swizzle(fpi->SrcReg[0]));
			break;
		case OPCODE_SCS:
			emit_trig(cs, fpi);
			break;
		case OPCODE_SIN:
			emit_trig(cs, fpi);
			break;
		case OPCODE_KIL:
		case OPCODE_TEX:
		case OPCODE_TXB:
		case OPCODE_TXP:
			emit_tex(cs, fpi, dest);
			break;
		default:
			ERROR("unknown fpi->Opcode %s\n", _mesa_opcode_string(fpi->Opcode));
			break;
	}

	/* Finishing touches */
	if (fpi->SaturateMode == SATURATE_ZERO_ONE) {
		code->inst[cs->nrslots-1].inst0 |= R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP;
	}
}

static GLboolean parse_program(struct r500_pfs_compile_state *cs)
{
	PROG_CODE;
	struct prog_instruction* fpi;

	for(fpi = cs->compiler->program->Instructions; fpi->Opcode != OPCODE_END; ++fpi) {
		do_inst(cs, fpi);

		if (cs->compiler->fp->error)
			return GL_FALSE;
	}

	/* Finish him! (If it's an ALU/OUT instruction...) */
	if ((code->inst[cs->nrslots-1].inst0 & 0x3) == 1) {
		code->inst[cs->nrslots-1].inst0 |= R500_INST_LAST;
	} else {
		/* We still need to put an output inst, right? */
		WARN_ONCE("Final FP instruction is not an OUT.\n");
	}

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
	int i;

	/* New compile, reset tracking data */
	cs->compiler->fp->optimization =
	    driQueryOptioni(&cs->compiler->r300->radeon.optionCache, "fp_optimization");
	cs->compiler->fp->translated = GL_FALSE;
	cs->compiler->fp->error = GL_FALSE;

	_mesa_bzero(code, sizeof(*code));
	code->max_temp_idx = 1; /* Size of pixel stack, plus 1. */
	cs->nrslots = 0;
	cs->compiler->fp->writes_depth = GL_FALSE;

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

	int ip;

	for (ip = 0; ip < cs->compiler->program->NumInstructions; ip++) {
		fpi = cs->compiler->program->Instructions + ip;
		for (i = 0; i < 3; i++) {
			if (fpi->SrcReg[i].File == PROGRAM_TEMPORARY) {
				if (fpi->SrcReg[i].Index >= temps_used)
					temps_used = fpi->SrcReg[i].Index + 1;
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
