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
 * Emit the r300_fragment_program_code that can be understood by the hardware.
 * Input is a pre-transformed radeon_program.
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 *
 * \author Jerome Glisse <j.glisse@gmail.com>
 *
 * \todo FogOption
 */

#include "r300_fragprog.h"

#include "radeon_program_pair.h"
#include "r300_fragprog_swizzle.h"
#include "r300_reg.h"


#define PROG_CODE \
	struct r300_fragment_program_compiler *c = (struct r300_fragment_program_compiler*)data; \
	struct r300_fragment_program_code *code = c->code

#define error(fmt, args...) do {			\
		fprintf(stderr, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
	} while(0)


static GLboolean emit_const(void* data, GLuint file, GLuint index, GLuint *hwindex)
{
	PROG_CODE;

	for (*hwindex = 0; *hwindex < code->const_nr; ++*hwindex) {
		if (code->constant[*hwindex].File == file &&
		    code->constant[*hwindex].Index == index)
			break;
	}

	if (*hwindex >= code->const_nr) {
		if (*hwindex >= PFS_NUM_CONST_REGS) {
			error("Out of hw constants!\n");
			return GL_FALSE;
		}

		code->const_nr++;
		code->constant[*hwindex].File = file;
		code->constant[*hwindex].Index = index;
	}

	return GL_TRUE;
}


/**
 * Mark a temporary register as used.
 */
static void use_temporary(struct r300_fragment_program_code *code, GLuint index)
{
	if (index > code->max_temp_idx)
		code->max_temp_idx = index;
}


static GLuint translate_rgb_opcode(GLuint opcode)
{
	switch(opcode) {
	case OPCODE_CMP: return R300_ALU_OUTC_CMP;
	case OPCODE_DP3: return R300_ALU_OUTC_DP3;
	case OPCODE_DP4: return R300_ALU_OUTC_DP4;
	case OPCODE_FRC: return R300_ALU_OUTC_FRC;
	default:
		error("translate_rgb_opcode(%i): Unknown opcode", opcode);
		/* fall through */
	case OPCODE_NOP:
		/* fall through */
	case OPCODE_MAD: return R300_ALU_OUTC_MAD;
	case OPCODE_MAX: return R300_ALU_OUTC_MAX;
	case OPCODE_MIN: return R300_ALU_OUTC_MIN;
	case OPCODE_REPL_ALPHA: return R300_ALU_OUTC_REPL_ALPHA;
	}
}

static GLuint translate_alpha_opcode(GLuint opcode)
{
	switch(opcode) {
	case OPCODE_CMP: return R300_ALU_OUTA_CMP;
	case OPCODE_DP3: return R300_ALU_OUTA_DP4;
	case OPCODE_DP4: return R300_ALU_OUTA_DP4;
	case OPCODE_EX2: return R300_ALU_OUTA_EX2;
	case OPCODE_FRC: return R300_ALU_OUTA_FRC;
	case OPCODE_LG2: return R300_ALU_OUTA_LG2;
	default:
		error("translate_rgb_opcode(%i): Unknown opcode", opcode);
		/* fall through */
	case OPCODE_NOP:
		/* fall through */
	case OPCODE_MAD: return R300_ALU_OUTA_MAD;
	case OPCODE_MAX: return R300_ALU_OUTA_MAX;
	case OPCODE_MIN: return R300_ALU_OUTA_MIN;
	case OPCODE_RCP: return R300_ALU_OUTA_RCP;
	case OPCODE_RSQ: return R300_ALU_OUTA_RSQ;
	}
}

/**
 * Emit one paired ALU instruction.
 */
static GLboolean emit_alu(void* data, struct radeon_pair_instruction* inst)
{
	PROG_CODE;

	if (code->alu.length >= PFS_MAX_ALU_INST) {
		error("Too many ALU instructions");
		return GL_FALSE;
	}

	int ip = code->alu.length++;
	int j;
	code->node[code->cur_node].alu_end++;

	code->alu.inst[ip].inst0 = translate_rgb_opcode(inst->RGB.Opcode);
	code->alu.inst[ip].inst2 = translate_alpha_opcode(inst->Alpha.Opcode);

	for(j = 0; j < 3; ++j) {
		GLuint src = inst->RGB.Src[j].Index | (inst->RGB.Src[j].Constant << 5);
		if (!inst->RGB.Src[j].Constant)
			use_temporary(code, inst->RGB.Src[j].Index);
		code->alu.inst[ip].inst1 |= src << (6*j);

		src = inst->Alpha.Src[j].Index | (inst->Alpha.Src[j].Constant << 5);
		if (!inst->Alpha.Src[j].Constant)
			use_temporary(code, inst->Alpha.Src[j].Index);
		code->alu.inst[ip].inst3 |= src << (6*j);

		GLuint arg = r300FPTranslateRGBSwizzle(inst->RGB.Arg[j].Source, inst->RGB.Arg[j].Swizzle);
		arg |= inst->RGB.Arg[j].Abs << 6;
		arg |= inst->RGB.Arg[j].Negate << 5;
		code->alu.inst[ip].inst0 |= arg << (7*j);

		arg = r300FPTranslateAlphaSwizzle(inst->Alpha.Arg[j].Source, inst->Alpha.Arg[j].Swizzle);
		arg |= inst->Alpha.Arg[j].Abs << 6;
		arg |= inst->Alpha.Arg[j].Negate << 5;
		code->alu.inst[ip].inst2 |= arg << (7*j);
	}

	if (inst->RGB.Saturate)
		code->alu.inst[ip].inst0 |= R300_ALU_OUTC_CLAMP;
	if (inst->Alpha.Saturate)
		code->alu.inst[ip].inst2 |= R300_ALU_OUTA_CLAMP;

	if (inst->RGB.WriteMask) {
		use_temporary(code, inst->RGB.DestIndex);
		code->alu.inst[ip].inst1 |=
			(inst->RGB.DestIndex << R300_ALU_DSTC_SHIFT) |
			(inst->RGB.WriteMask << R300_ALU_DSTC_REG_MASK_SHIFT);
	}
	if (inst->RGB.OutputWriteMask) {
		code->alu.inst[ip].inst1 |= (inst->RGB.OutputWriteMask << R300_ALU_DSTC_OUTPUT_MASK_SHIFT);
		code->node[code->cur_node].flags |= R300_RGBA_OUT;
	}

	if (inst->Alpha.WriteMask) {
		use_temporary(code, inst->Alpha.DestIndex);
		code->alu.inst[ip].inst3 |=
			(inst->Alpha.DestIndex << R300_ALU_DSTA_SHIFT) |
			R300_ALU_DSTA_REG;
	}
	if (inst->Alpha.OutputWriteMask) {
		code->alu.inst[ip].inst3 |= R300_ALU_DSTA_OUTPUT;
		code->node[code->cur_node].flags |= R300_RGBA_OUT;
	}
	if (inst->Alpha.DepthWriteMask) {
		code->alu.inst[ip].inst3 |= R300_ALU_DSTA_DEPTH;
		code->node[code->cur_node].flags |= R300_W_OUT;
		c->fp->WritesDepth = GL_TRUE;
	}

	return GL_TRUE;
}


/**
 * Finish the current node without advancing to the next one.
 */
static GLboolean finish_node(struct r300_fragment_program_compiler *c)
{
	struct r300_fragment_program_code *code = c->code;
	struct r300_fragment_program_node *node = &code->node[code->cur_node];

	if (node->alu_end < 0) {
		/* Generate a single NOP for this node */
		struct radeon_pair_instruction inst;
		_mesa_bzero(&inst, sizeof(inst));
		if (!emit_alu(c, &inst))
			return GL_FALSE;
	}

	if (node->tex_end < 0) {
		if (code->cur_node == 0) {
			node->tex_end = 0;
		} else {
			error("Node %i has no TEX instructions", code->cur_node);
			return GL_FALSE;
		}
	} else {
		if (code->cur_node == 0)
			code->first_node_has_tex = 1;
	}

	return GL_TRUE;
}


/**
 * Begin a block of texture instructions.
 * Create the necessary indirection.
 */
static GLboolean begin_tex(void* data)
{
	PROG_CODE;

	if (code->cur_node == 0) {
		if (code->node[0].alu_end < 0 &&
		    code->node[0].tex_end < 0)
			return GL_TRUE;
	}

	if (code->cur_node == 3) {
		error("Too many texture indirections");
		return GL_FALSE;
	}

	if (!finish_node(c))
		return GL_FALSE;

	struct r300_fragment_program_node *node = &code->node[++code->cur_node];
	node->alu_offset = code->alu.length;
	node->alu_end = -1;
	node->tex_offset = code->tex.length;
	node->tex_end = -1;
	return GL_TRUE;
}


static GLboolean emit_tex(void* data, struct prog_instruction* inst)
{
	PROG_CODE;

	if (code->tex.length >= PFS_MAX_TEX_INST) {
		error("Too many TEX instructions");
		return GL_FALSE;
	}

	GLuint unit = inst->TexSrcUnit;
	GLuint dest = inst->DstReg.Index;
	GLuint opcode;

	switch(inst->Opcode) {
	case OPCODE_KIL: opcode = R300_TEX_OP_KIL; break;
	case OPCODE_TEX: opcode = R300_TEX_OP_LD; break;
	case OPCODE_TXB: opcode = R300_TEX_OP_TXB; break;
	case OPCODE_TXP: opcode = R300_TEX_OP_TXP; break;
	default:
		error("Unknown texture opcode %i", inst->Opcode);
		return GL_FALSE;
	}

	if (inst->Opcode == OPCODE_KIL) {
		unit = 0;
		dest = 0;
	} else {
		use_temporary(code, dest);
	}

	use_temporary(code, inst->SrcReg[0].Index);

	code->node[code->cur_node].tex_end++;
	code->tex.inst[code->tex.length++] =
		(inst->SrcReg[0].Index << R300_SRC_ADDR_SHIFT) |
		(dest << R300_DST_ADDR_SHIFT) |
		(unit << R300_TEX_ID_SHIFT) |
		(opcode << R300_TEX_INST_SHIFT);
	return GL_TRUE;
}


static const struct radeon_pair_handler pair_handler = {
	.EmitConst = &emit_const,
	.EmitPaired = &emit_alu,
	.EmitTex = &emit_tex,
	.BeginTexBlock = &begin_tex,
	.MaxHwTemps = PFS_NUM_TEMP_REGS
};

/**
 * Final compilation step: Turn the intermediate radeon_program into
 * machine-readable instructions.
 */
GLboolean r300FragmentProgramEmit(struct r300_fragment_program_compiler *compiler)
{
	struct r300_fragment_program_code *code = compiler->code;

	_mesa_bzero(code, sizeof(struct r300_fragment_program_code));
	code->node[0].alu_end = -1;
	code->node[0].tex_end = -1;

	if (!radeonPairProgram(compiler->r300->radeon.glCtx, compiler->program, &pair_handler, compiler))
		return GL_FALSE;

	if (!finish_node(compiler))
		return GL_FALSE;

	return GL_TRUE;
}

