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

#include "r500_fragprog.h"

#include "radeon_program_pair.h"


#define PROG_CODE \
	struct r500_fragment_program_compiler *c = (struct r500_fragment_program_compiler*)data; \
	struct r500_fragment_program_code *code = c->code

#define error(fmt, args...) do {			\
		fprintf(stderr, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
	} while(0)


/**
 * Callback to register hardware constants.
 */
static GLboolean emit_const(void *data, GLuint file, GLuint idx, GLuint *hwindex)
{
	PROG_CODE;

	for (*hwindex = 0; *hwindex < code->const_nr; ++*hwindex) {
		if (code->constant[*hwindex].File == file &&
		    code->constant[*hwindex].Index == idx)
			break;
	}

	if (*hwindex >= code->const_nr) {
		if (*hwindex >= PFS_NUM_CONST_REGS) {
			error("Out of hw constants!\n");
			return GL_FALSE;
		}

		code->const_nr++;
		code->constant[*hwindex].File = file;
		code->constant[*hwindex].Index = idx;
	}

	return GL_TRUE;
}

static GLuint translate_rgb_op(GLuint opcode)
{
	switch(opcode) {
	case OPCODE_CMP: return R500_ALU_RGBA_OP_CMP;
	case OPCODE_DDX: return R500_ALU_RGBA_OP_MDH;
	case OPCODE_DDY: return R500_ALU_RGBA_OP_MDV;
	case OPCODE_DP3: return R500_ALU_RGBA_OP_DP3;
	case OPCODE_DP4: return R500_ALU_RGBA_OP_DP4;
	case OPCODE_FRC: return R500_ALU_RGBA_OP_FRC;
	default:
		error("translate_rgb_op(%d): unknown opcode\n", opcode);
		/* fall through */
	case OPCODE_NOP:
		/* fall through */
	case OPCODE_MAD: return R500_ALU_RGBA_OP_MAD;
	case OPCODE_MAX: return R500_ALU_RGBA_OP_MAX;
	case OPCODE_MIN: return R500_ALU_RGBA_OP_MIN;
	case OPCODE_REPL_ALPHA: return R500_ALU_RGBA_OP_SOP;
	}
}

static GLuint translate_alpha_op(GLuint opcode)
{
	switch(opcode) {
	case OPCODE_CMP: return R500_ALPHA_OP_CMP;
	case OPCODE_COS: return R500_ALPHA_OP_COS;
	case OPCODE_DDX: return R500_ALPHA_OP_MDH;
	case OPCODE_DDY: return R500_ALPHA_OP_MDV;
	case OPCODE_DP3: return R500_ALPHA_OP_DP;
	case OPCODE_DP4: return R500_ALPHA_OP_DP;
	case OPCODE_EX2: return R500_ALPHA_OP_EX2;
	case OPCODE_FRC: return R500_ALPHA_OP_FRC;
	case OPCODE_LG2: return R500_ALPHA_OP_LN2;
	default:
		error("translate_alpha_op(%d): unknown opcode\n", opcode);
		/* fall through */
	case OPCODE_NOP:
		/* fall through */
	case OPCODE_MAD: return R500_ALPHA_OP_MAD;
	case OPCODE_MAX: return R500_ALPHA_OP_MAX;
	case OPCODE_MIN: return R500_ALPHA_OP_MIN;
	case OPCODE_RCP: return R500_ALPHA_OP_RCP;
	case OPCODE_RSQ: return R500_ALPHA_OP_RSQ;
	case OPCODE_SIN: return R500_ALPHA_OP_SIN;
	}
}

static GLuint fix_hw_swizzle(GLuint swz)
{
	if (swz == 5) swz = 6;
	if (swz == SWIZZLE_NIL) swz = 4;
	return swz;
}

static GLuint translate_arg_rgb(struct radeon_pair_instruction *inst, int arg)
{
	GLuint t = inst->RGB.Arg[arg].Source;
	int comp;
	t |= inst->RGB.Arg[arg].Negate << 11;
	t |= inst->RGB.Arg[arg].Abs << 12;

	for(comp = 0; comp < 3; ++comp)
		t |= fix_hw_swizzle(GET_SWZ(inst->RGB.Arg[arg].Swizzle, comp)) << (3*comp + 2);

	return t;
}

static GLuint translate_arg_alpha(struct radeon_pair_instruction *inst, int i)
{
	GLuint t = inst->Alpha.Arg[i].Source;
	t |= fix_hw_swizzle(inst->Alpha.Arg[i].Swizzle) << 2;
	t |= inst->Alpha.Arg[i].Negate << 5;
	t |= inst->Alpha.Arg[i].Abs << 6;
	return t;
}

static void use_temporary(struct r500_fragment_program_code* code, GLuint index)
{
	if (index > code->max_temp_idx)
		code->max_temp_idx = index;
}

static GLuint use_source(struct r500_fragment_program_code* code, struct radeon_pair_instruction_source src)
{
	if (!src.Constant)
		use_temporary(code, src.Index);
	return src.Index | src.Constant << 8;
}


/**
 * Emit a paired ALU instruction.
 */
static GLboolean emit_paired(void *data, struct radeon_pair_instruction *inst)
{
	PROG_CODE;

	if (code->inst_end >= 511) {
		error("emit_alu: Too many instructions");
		return GL_FALSE;
	}

	int ip = ++code->inst_end;

	code->inst[ip].inst5 = translate_rgb_op(inst->RGB.Opcode);
	code->inst[ip].inst4 = translate_alpha_op(inst->Alpha.Opcode);

	if (inst->RGB.OutputWriteMask || inst->Alpha.OutputWriteMask || inst->Alpha.DepthWriteMask)
		code->inst[ip].inst0 = R500_INST_TYPE_OUT;
	else
		code->inst[ip].inst0 = R500_INST_TYPE_ALU;
	code->inst[ip].inst0 |= R500_INST_TEX_SEM_WAIT;

	code->inst[ip].inst0 |= (inst->RGB.WriteMask << 11) | (inst->Alpha.WriteMask << 14);
	code->inst[ip].inst0 |= (inst->RGB.OutputWriteMask << 15) | (inst->Alpha.OutputWriteMask << 18);
	if (inst->Alpha.DepthWriteMask) {
		code->inst[ip].inst4 |= R500_ALPHA_W_OMASK;
		c->fp->writes_depth = GL_TRUE;
	}

	code->inst[ip].inst4 |= R500_ALPHA_ADDRD(inst->Alpha.DestIndex);
	code->inst[ip].inst5 |= R500_ALU_RGBA_ADDRD(inst->RGB.DestIndex);
	use_temporary(code, inst->Alpha.DestIndex);
	use_temporary(code, inst->RGB.DestIndex);

	if (inst->RGB.Saturate)
		code->inst[ip].inst0 |= R500_INST_RGB_CLAMP;
	if (inst->Alpha.Saturate)
		code->inst[ip].inst0 |= R500_INST_ALPHA_CLAMP;

	code->inst[ip].inst1 |= R500_RGB_ADDR0(use_source(code, inst->RGB.Src[0]));
	code->inst[ip].inst1 |= R500_RGB_ADDR1(use_source(code, inst->RGB.Src[1]));
	code->inst[ip].inst1 |= R500_RGB_ADDR2(use_source(code, inst->RGB.Src[2]));

	code->inst[ip].inst2 |= R500_ALPHA_ADDR0(use_source(code, inst->Alpha.Src[0]));
	code->inst[ip].inst2 |= R500_ALPHA_ADDR1(use_source(code, inst->Alpha.Src[1]));
	code->inst[ip].inst2 |= R500_ALPHA_ADDR2(use_source(code, inst->Alpha.Src[2]));

	code->inst[ip].inst3 |= translate_arg_rgb(inst, 0) << R500_ALU_RGB_SEL_A_SHIFT;
	code->inst[ip].inst3 |= translate_arg_rgb(inst, 1) << R500_ALU_RGB_SEL_B_SHIFT;
	code->inst[ip].inst5 |= translate_arg_rgb(inst, 2) << R500_ALU_RGBA_SEL_C_SHIFT;

	code->inst[ip].inst4 |= translate_arg_alpha(inst, 0) << R500_ALPHA_SEL_A_SHIFT;
	code->inst[ip].inst4 |= translate_arg_alpha(inst, 1) << R500_ALPHA_SEL_B_SHIFT;
	code->inst[ip].inst5 |= translate_arg_alpha(inst, 2) << R500_ALU_RGBA_ALPHA_SEL_C_SHIFT;

	return GL_TRUE;
}

static GLuint translate_strq_swizzle(struct prog_src_register src)
{
	GLuint swiz = 0;
	int i;
	for (i = 0; i < 4; i++)
		swiz |= (GET_SWZ(src.Swizzle, i) & 0x3) << i*2;
	return swiz;
}

/**
 * Emit a single TEX instruction
 */
static GLboolean emit_tex(void *data, struct prog_instruction *inst)
{
	PROG_CODE;

	if (code->inst_end >= 511) {
		error("emit_tex: Too many instructions");
		return GL_FALSE;
	}

	int ip = ++code->inst_end;

	code->inst[ip].inst0 = R500_INST_TYPE_TEX
		| (inst->DstReg.WriteMask << 11)
		| R500_INST_TEX_SEM_WAIT;
	code->inst[ip].inst1 = R500_TEX_ID(inst->TexSrcUnit)
		| R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED;

	if (inst->TexSrcTarget == TEXTURE_RECT_INDEX)
	        code->inst[ip].inst1 |= R500_TEX_UNSCALED;

	switch (inst->Opcode) {
	case OPCODE_KIL:
		code->inst[ip].inst1 |= R500_TEX_INST_TEXKILL;
		break;
	case OPCODE_TEX:
		code->inst[ip].inst1 |= R500_TEX_INST_LD;
		break;
	case OPCODE_TXB:
		code->inst[ip].inst1 |= R500_TEX_INST_LODBIAS;
		break;
	case OPCODE_TXP:
		code->inst[ip].inst1 |= R500_TEX_INST_PROJ;
		break;
	default:
		error("emit_tex can't handle opcode %x\n", inst->Opcode);
	}

	code->inst[ip].inst2 = R500_TEX_SRC_ADDR(inst->SrcReg[0].Index)
		| (translate_strq_swizzle(inst->SrcReg[0]) << 8)
		| R500_TEX_DST_ADDR(inst->DstReg.Index)
		| R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;

	return GL_TRUE;
}

static const struct radeon_pair_handler pair_handler = {
	.EmitConst = emit_const,
	.EmitPaired = emit_paired,
	.EmitTex = emit_tex,
	.MaxHwTemps = 128
};

GLboolean r500FragmentProgramEmit(struct r500_fragment_program_compiler *compiler)
{
	struct r500_fragment_program_code *code = compiler->code;

	_mesa_bzero(code, sizeof(*code));
	code->max_temp_idx = 1;
	code->inst_offset = 0;
	code->inst_end = -1;

	if (!radeonPairProgram(compiler->r300->radeon.glCtx, compiler->program, &pair_handler, compiler))
		return GL_FALSE;

	if ((code->inst[code->inst_end].inst0 & R500_INST_TYPE_MASK) != R500_INST_TYPE_OUT) {
		/* This may happen when dead-code elimination is disabled or
		 * when most of the fragment program logic is leading to a KIL */
		if (code->inst_end >= 511) {
			error("Introducing fake OUT: Too many instructions");
			return GL_FALSE;
		}

		int ip = ++code->inst_end;
		code->inst[ip].inst0 = R500_INST_TYPE_OUT | R500_INST_TEX_SEM_WAIT;
	}

	return GL_TRUE;
}
