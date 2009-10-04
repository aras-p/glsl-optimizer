/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#include "radeon_mesa_to_rc.h"

#include "main/mtypes.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"

#include "compiler/radeon_compiler.h"
#include "compiler/radeon_program.h"


static rc_opcode translate_opcode(gl_inst_opcode opcode)
{
	switch(opcode) {
	case OPCODE_NOP: return RC_OPCODE_NOP;
	case OPCODE_ABS: return RC_OPCODE_ABS;
	case OPCODE_ADD: return RC_OPCODE_ADD;
	case OPCODE_ARL: return RC_OPCODE_ARL;
	case OPCODE_CMP: return RC_OPCODE_CMP;
	case OPCODE_COS: return RC_OPCODE_COS;
	case OPCODE_DDX: return RC_OPCODE_DDX;
	case OPCODE_DDY: return RC_OPCODE_DDY;
	case OPCODE_DP3: return RC_OPCODE_DP3;
	case OPCODE_DP4: return RC_OPCODE_DP4;
	case OPCODE_DPH: return RC_OPCODE_DPH;
	case OPCODE_DST: return RC_OPCODE_DST;
	case OPCODE_EX2: return RC_OPCODE_EX2;
	case OPCODE_EXP: return RC_OPCODE_EXP;
	case OPCODE_FLR: return RC_OPCODE_FLR;
	case OPCODE_FRC: return RC_OPCODE_FRC;
	case OPCODE_KIL: return RC_OPCODE_KIL;
	case OPCODE_LG2: return RC_OPCODE_LG2;
	case OPCODE_LIT: return RC_OPCODE_LIT;
	case OPCODE_LOG: return RC_OPCODE_LOG;
	case OPCODE_LRP: return RC_OPCODE_LRP;
	case OPCODE_MAD: return RC_OPCODE_MAD;
	case OPCODE_MAX: return RC_OPCODE_MAX;
	case OPCODE_MIN: return RC_OPCODE_MIN;
	case OPCODE_MOV: return RC_OPCODE_MOV;
	case OPCODE_MUL: return RC_OPCODE_MUL;
	case OPCODE_POW: return RC_OPCODE_POW;
	case OPCODE_RCP: return RC_OPCODE_RCP;
	case OPCODE_RSQ: return RC_OPCODE_RSQ;
	case OPCODE_SCS: return RC_OPCODE_SCS;
	case OPCODE_SEQ: return RC_OPCODE_SEQ;
	case OPCODE_SFL: return RC_OPCODE_SFL;
	case OPCODE_SGE: return RC_OPCODE_SGE;
	case OPCODE_SGT: return RC_OPCODE_SGT;
	case OPCODE_SIN: return RC_OPCODE_SIN;
	case OPCODE_SLE: return RC_OPCODE_SLE;
	case OPCODE_SLT: return RC_OPCODE_SLT;
	case OPCODE_SNE: return RC_OPCODE_SNE;
	case OPCODE_SUB: return RC_OPCODE_SUB;
	case OPCODE_SWZ: return RC_OPCODE_SWZ;
	case OPCODE_TEX: return RC_OPCODE_TEX;
	case OPCODE_TXB: return RC_OPCODE_TXB;
	case OPCODE_TXD: return RC_OPCODE_TXD;
	case OPCODE_TXL: return RC_OPCODE_TXL;
	case OPCODE_TXP: return RC_OPCODE_TXP;
	case OPCODE_XPD: return RC_OPCODE_XPD;
	default: return RC_OPCODE_ILLEGAL_OPCODE;
	}
}

static rc_saturate_mode translate_saturate(unsigned int saturate)
{
	switch(saturate) {
	default:
	case SATURATE_OFF: return RC_SATURATE_NONE;
	case SATURATE_ZERO_ONE: return RC_SATURATE_ZERO_ONE;
	}
}

static rc_register_file translate_register_file(unsigned int file)
{
	switch(file) {
	case PROGRAM_TEMPORARY: return RC_FILE_TEMPORARY;
	case PROGRAM_INPUT: return RC_FILE_INPUT;
	case PROGRAM_OUTPUT: return RC_FILE_OUTPUT;
	case PROGRAM_LOCAL_PARAM:
	case PROGRAM_ENV_PARAM:
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
	case PROGRAM_CONSTANT:
	case PROGRAM_UNIFORM: return RC_FILE_CONSTANT;
	case PROGRAM_ADDRESS: return RC_FILE_ADDRESS;
	default: return RC_FILE_NONE;
	}
}

static void translate_srcreg(struct rc_src_register * dest, struct prog_src_register * src)
{
	dest->File = translate_register_file(src->File);
	dest->Index = src->Index;
	dest->RelAddr = src->RelAddr;
	dest->Swizzle = src->Swizzle;
	dest->Abs = src->Abs;
	dest->Negate = src->Negate;
}

static void translate_dstreg(struct rc_dst_register * dest, struct prog_dst_register * src)
{
	dest->File = translate_register_file(src->File);
	dest->Index = src->Index;
	dest->RelAddr = src->RelAddr;
	dest->WriteMask = src->WriteMask;
}

static rc_texture_target translate_tex_target(gl_texture_index target)
{
	switch(target) {
	case TEXTURE_2D_ARRAY_INDEX: return RC_TEXTURE_2D_ARRAY;
	case TEXTURE_1D_ARRAY_INDEX: return RC_TEXTURE_1D_ARRAY;
	case TEXTURE_CUBE_INDEX: return RC_TEXTURE_CUBE;
	case TEXTURE_3D_INDEX: return RC_TEXTURE_3D;
	case TEXTURE_RECT_INDEX: return RC_TEXTURE_RECT;
	default:
	case TEXTURE_2D_INDEX: return RC_TEXTURE_2D;
	case TEXTURE_1D_INDEX: return RC_TEXTURE_1D;
	}
}

static void translate_instruction(struct radeon_compiler * c,
		struct rc_instruction * dest, struct prog_instruction * src)
{
	const struct rc_opcode_info * opcode;
	unsigned int i;

	dest->U.I.Opcode = translate_opcode(src->Opcode);
	if (dest->U.I.Opcode == RC_OPCODE_ILLEGAL_OPCODE) {
		rc_error(c, "Unsupported opcode %i\n", src->Opcode);
		return;
	}
	dest->U.I.SaturateMode = translate_saturate(src->SaturateMode);

	opcode = rc_get_opcode_info(dest->U.I.Opcode);

	for(i = 0; i < opcode->NumSrcRegs; ++i)
		translate_srcreg(&dest->U.I.SrcReg[i], &src->SrcReg[i]);

	if (opcode->HasDstReg)
		translate_dstreg(&dest->U.I.DstReg, &src->DstReg);

	if (opcode->HasTexture) {
		dest->U.I.TexSrcUnit = src->TexSrcUnit;
		dest->U.I.TexSrcTarget = translate_tex_target(src->TexSrcTarget);
		dest->U.I.TexShadow = src->TexShadow;
	}
}

void radeon_mesa_to_rc_program(struct radeon_compiler * c, struct gl_program * program)
{
	struct prog_instruction *source;
	unsigned int i;

	for(source = program->Instructions; source->Opcode != OPCODE_END; ++source) {
		struct rc_instruction * dest = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
		translate_instruction(c, dest, source);
	}

	c->Program.ShadowSamplers = program->ShadowSamplers;
	c->Program.InputsRead = program->InputsRead;
	c->Program.OutputsWritten = program->OutputsWritten;

	int isNVProgram = 0;

	if (program->Target == GL_VERTEX_PROGRAM_ARB) {
		struct gl_vertex_program * vp = (struct gl_vertex_program *) program;
		isNVProgram = vp->IsNVProgram;
	}

	if (isNVProgram) {
		/* NV_vertex_program has a fixed-sized constant environment.
		 * This could be handled more efficiently for programs that
		 * do not use relative addressing.
		 */
		for(i = 0; i < 96; ++i) {
			struct rc_constant constant;

			constant.Type = RC_CONSTANT_EXTERNAL;
			constant.Size = 4;
			constant.u.External = i;

			rc_constants_add(&c->Program.Constants, &constant);
		}
	} else {
		for(i = 0; i < program->Parameters->NumParameters; ++i) {
			struct rc_constant constant;

			constant.Type = RC_CONSTANT_EXTERNAL;
			constant.Size = 4;
			constant.u.External = i;

			rc_constants_add(&c->Program.Constants, &constant);
		}
	}
}
