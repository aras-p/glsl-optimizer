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

#include "../r300_reg.h"

#include "radeon_program_pair.h"
#include "r300_fragprog_swizzle.h"


struct r300_emit_state {
	struct r300_fragment_program_compiler * compiler;

	unsigned current_node : 2;
	unsigned node_first_tex : 8;
	unsigned node_first_alu : 8;
	uint32_t node_flags;
};

#define PROG_CODE \
	struct r300_emit_state * emit = (struct r300_emit_state*)data; \
	struct r300_fragment_program_compiler *c = emit->compiler; \
	struct r300_fragment_program_code *code = &c->code->code.r300

#define error(fmt, args...) do {			\
		rc_error(&c->Base, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
	} while(0)


/**
 * Mark a temporary register as used.
 */
static void use_temporary(struct r300_fragment_program_code *code, GLuint index)
{
	if (index > code->pixsize)
		code->pixsize = index;
}


static GLuint translate_rgb_opcode(struct r300_fragment_program_compiler * c, GLuint opcode)
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

static GLuint translate_alpha_opcode(struct r300_fragment_program_compiler * c, GLuint opcode)
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

	if (code->alu.length >= R300_PFS_MAX_ALU_INST) {
		error("Too many ALU instructions");
		return GL_FALSE;
	}

	int ip = code->alu.length++;
	int j;

	code->alu.inst[ip].rgb_inst = translate_rgb_opcode(c, inst->RGB.Opcode);
	code->alu.inst[ip].alpha_inst = translate_alpha_opcode(c, inst->Alpha.Opcode);

	for(j = 0; j < 3; ++j) {
		GLuint src = inst->RGB.Src[j].Index | (inst->RGB.Src[j].Constant << 5);
		if (!inst->RGB.Src[j].Constant)
			use_temporary(code, inst->RGB.Src[j].Index);
		code->alu.inst[ip].rgb_addr |= src << (6*j);

		src = inst->Alpha.Src[j].Index | (inst->Alpha.Src[j].Constant << 5);
		if (!inst->Alpha.Src[j].Constant)
			use_temporary(code, inst->Alpha.Src[j].Index);
		code->alu.inst[ip].alpha_addr |= src << (6*j);

		GLuint arg = r300FPTranslateRGBSwizzle(inst->RGB.Arg[j].Source, inst->RGB.Arg[j].Swizzle);
		arg |= inst->RGB.Arg[j].Abs << 6;
		arg |= inst->RGB.Arg[j].Negate << 5;
		code->alu.inst[ip].rgb_inst |= arg << (7*j);

		arg = r300FPTranslateAlphaSwizzle(inst->Alpha.Arg[j].Source, inst->Alpha.Arg[j].Swizzle);
		arg |= inst->Alpha.Arg[j].Abs << 6;
		arg |= inst->Alpha.Arg[j].Negate << 5;
		code->alu.inst[ip].alpha_inst |= arg << (7*j);
	}

	if (inst->RGB.Saturate)
		code->alu.inst[ip].rgb_inst |= R300_ALU_OUTC_CLAMP;
	if (inst->Alpha.Saturate)
		code->alu.inst[ip].alpha_inst |= R300_ALU_OUTA_CLAMP;

	if (inst->RGB.WriteMask) {
		use_temporary(code, inst->RGB.DestIndex);
		code->alu.inst[ip].rgb_addr |=
			(inst->RGB.DestIndex << R300_ALU_DSTC_SHIFT) |
			(inst->RGB.WriteMask << R300_ALU_DSTC_REG_MASK_SHIFT);
	}
	if (inst->RGB.OutputWriteMask) {
		code->alu.inst[ip].rgb_addr |= (inst->RGB.OutputWriteMask << R300_ALU_DSTC_OUTPUT_MASK_SHIFT);
		emit->node_flags |= R300_RGBA_OUT;
	}

	if (inst->Alpha.WriteMask) {
		use_temporary(code, inst->Alpha.DestIndex);
		code->alu.inst[ip].alpha_addr |=
			(inst->Alpha.DestIndex << R300_ALU_DSTA_SHIFT) |
			R300_ALU_DSTA_REG;
	}
	if (inst->Alpha.OutputWriteMask) {
		code->alu.inst[ip].alpha_addr |= R300_ALU_DSTA_OUTPUT;
		emit->node_flags |= R300_RGBA_OUT;
	}
	if (inst->Alpha.DepthWriteMask) {
		code->alu.inst[ip].alpha_addr |= R300_ALU_DSTA_DEPTH;
		emit->node_flags |= R300_W_OUT;
		c->code->writes_depth = GL_TRUE;
	}

	return GL_TRUE;
}


/**
 * Finish the current node without advancing to the next one.
 */
static GLboolean finish_node(struct r300_emit_state * emit)
{
	struct r300_fragment_program_compiler * c = emit->compiler;
	struct r300_fragment_program_code *code = &emit->compiler->code->code.r300;

	if (code->alu.length == emit->node_first_alu) {
		/* Generate a single NOP for this node */
		struct radeon_pair_instruction inst;
		_mesa_bzero(&inst, sizeof(inst));
		if (!emit_alu(emit, &inst))
			return GL_FALSE;
	}

	unsigned alu_offset = emit->node_first_alu;
	unsigned alu_end = code->alu.length - alu_offset - 1;
	unsigned tex_offset = emit->node_first_tex;
	unsigned tex_end = code->tex.length - tex_offset - 1;

	if (code->tex.length == emit->node_first_tex) {
		if (emit->current_node > 0) {
			error("Node %i has no TEX instructions", emit->current_node);
			return GL_FALSE;
		}

		tex_end = 0;
	} else {
		if (emit->current_node == 0)
			code->config |= R300_PFS_CNTL_FIRST_NODE_HAS_TEX;
	}

	/* Write the config register.
	 * Note: The order in which the words for each node are written
	 * is not correct here and needs to be fixed up once we're entirely
	 * done
	 *
	 * Also note that the register specification from AMD is slightly
	 * incorrect in its description of this register. */
	code->code_addr[emit->current_node] =
			(alu_offset << R300_ALU_START_SHIFT) |
			(alu_end << R300_ALU_SIZE_SHIFT) |
			(tex_offset << R300_TEX_START_SHIFT) |
			(tex_end << R300_TEX_SIZE_SHIFT) |
			emit->node_flags;

	return GL_TRUE;
}


/**
 * Begin a block of texture instructions.
 * Create the necessary indirection.
 */
static GLboolean begin_tex(void* data)
{
	PROG_CODE;

	if (code->alu.length == emit->node_first_alu &&
	    code->tex.length == emit->node_first_tex) {
		return GL_TRUE;
	}

	if (emit->current_node == 3) {
		error("Too many texture indirections");
		return GL_FALSE;
	}

	if (!finish_node(emit))
		return GL_FALSE;

	emit->current_node++;
	emit->node_first_tex = code->tex.length;
	emit->node_first_alu = code->alu.length;
	emit->node_flags = 0;
	return GL_TRUE;
}


static GLboolean emit_tex(void* data, struct radeon_pair_texture_instruction* inst)
{
	PROG_CODE;

	if (code->tex.length >= R300_PFS_MAX_TEX_INST) {
		error("Too many TEX instructions");
		return GL_FALSE;
	}

	GLuint unit = inst->TexSrcUnit;
	GLuint dest = inst->DestIndex;
	GLuint opcode;

	switch(inst->Opcode) {
	case RADEON_OPCODE_KIL: opcode = R300_TEX_OP_KIL; break;
	case RADEON_OPCODE_TEX: opcode = R300_TEX_OP_LD; break;
	case RADEON_OPCODE_TXB: opcode = R300_TEX_OP_TXB; break;
	case RADEON_OPCODE_TXP: opcode = R300_TEX_OP_TXP; break;
	default:
		error("Unknown texture opcode %i", inst->Opcode);
		return GL_FALSE;
	}

	if (inst->Opcode == RADEON_OPCODE_KIL) {
		unit = 0;
		dest = 0;
	} else {
		use_temporary(code, dest);
	}

	use_temporary(code, inst->SrcIndex);

	code->tex.inst[code->tex.length++] =
		(inst->SrcIndex << R300_SRC_ADDR_SHIFT) |
		(dest << R300_DST_ADDR_SHIFT) |
		(unit << R300_TEX_ID_SHIFT) |
		(opcode << R300_TEX_INST_SHIFT);
	return GL_TRUE;
}


static const struct radeon_pair_handler pair_handler = {
	.EmitPaired = &emit_alu,
	.EmitTex = &emit_tex,
	.BeginTexBlock = &begin_tex,
	.MaxHwTemps = R300_PFS_NUM_TEMP_REGS
};

/**
 * Final compilation step: Turn the intermediate radeon_program into
 * machine-readable instructions.
 */
void r300BuildFragmentProgramHwCode(struct r300_fragment_program_compiler *compiler)
{
	struct r300_emit_state emit;
	struct r300_fragment_program_code *code = &compiler->code->code.r300;

	memset(&emit, 0, sizeof(emit));
	emit.compiler = compiler;

	_mesa_bzero(code, sizeof(struct r300_fragment_program_code));

	radeonPairProgram(compiler, &pair_handler, &emit);
	if (compiler->Base.Error)
		return;

	/* Finish the program */
	finish_node(&emit);

	code->config |= emit.current_node; /* FIRST_NODE_HAS_TEX set by finish_node */
	code->code_offset =
		(0 << R300_PFS_CNTL_ALU_OFFSET_SHIFT) |
		((code->alu.length-1) << R300_PFS_CNTL_ALU_END_SHIFT) |
		(0 << R300_PFS_CNTL_TEX_OFFSET_SHIFT) |
		((code->tex.length ? code->tex.length-1 : 0) << R300_PFS_CNTL_TEX_END_SHIFT);

	if (emit.current_node < 3) {
		int shift = 3 - emit.current_node;
		int i;
		for(i = emit.current_node; i >= 0; --i)
			code->code_addr[shift + i] = code->code_addr[i];
		for(i = 0; i < shift; ++i)
			code->code_addr[i] = 0;
	}
}
