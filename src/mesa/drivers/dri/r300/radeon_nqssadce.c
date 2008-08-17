/*
 * Copyright (C) 2008 Nicolai Haehnle.
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
 * @file
 *
 * "Not-quite SSA" and Dead-Code Elimination.
 *
 * @note This code uses SWIZZLE_NIL in a source register to indicate that
 * the corresponding component is ignored by the corresponding instruction.
 */

#include "radeon_nqssadce.h"


/**
 * Return the @ref register_state for the given register (or 0 for untracked
 * registers, i.e. constants).
 */
static struct register_state *get_reg_state(struct nqssadce_state* s, GLuint file, GLuint index)
{
	switch(file) {
	case PROGRAM_TEMPORARY: return &s->Temps[index];
	case PROGRAM_OUTPUT: return &s->Outputs[index];
	default: return 0;
	}
}


/**
 * Left multiplication of a register with a swizzle
 *
 * @note Works correctly only for X, Y, Z, W swizzles, not for constant swizzles.
 */
static struct prog_src_register lmul_swizzle(GLuint swizzle, struct prog_src_register srcreg)
{
	struct prog_src_register tmp = srcreg;
	int i;
	tmp.Swizzle = 0;
	tmp.NegateBase = 0;
	for(i = 0; i < 4; ++i) {
		GLuint swz = GET_SWZ(swizzle, i);
		if (swz < 4) {
			tmp.Swizzle |= GET_SWZ(srcreg.Swizzle, swz) << (i*3);
			tmp.NegateBase |= GET_BIT(srcreg.NegateBase, swz) << i;
		} else {
			tmp.Swizzle |= swz << (i*3);
		}
	}
	return tmp;
}


static struct prog_instruction* track_used_srcreg(struct nqssadce_state* s,
	struct prog_instruction *inst, GLint src, GLuint sourced)
{
	int i;
	GLuint deswz_source = 0;

	for(i = 0; i < 4; ++i) {
		if (GET_BIT(sourced, i)) {
			GLuint swz = GET_SWZ(inst->SrcReg[src].Swizzle, i);
			deswz_source |= 1 << swz;
		} else {
			inst->SrcReg[src].Swizzle &= ~(7 << (3*i));
			inst->SrcReg[src].Swizzle |= SWIZZLE_NIL << (3*i);
		}
	}

	if (!s->Descr->IsNativeSwizzle(inst->Opcode, inst->SrcReg[src])) {
		struct prog_dst_register dstreg = inst->DstReg;
		dstreg.File = PROGRAM_TEMPORARY;
		dstreg.Index = _mesa_find_free_register(s->Program, PROGRAM_TEMPORARY);
		dstreg.WriteMask = sourced;

		s->Descr->BuildSwizzle(s, dstreg, inst->SrcReg[src]);

		inst = s->Program->Instructions + s->IP;
		inst->SrcReg[src].File = PROGRAM_TEMPORARY;
		inst->SrcReg[src].Index = dstreg.Index;
		inst->SrcReg[src].Swizzle = 0;
		inst->SrcReg[src].NegateBase = 0;
		inst->SrcReg[src].Abs = 0;
		inst->SrcReg[src].NegateAbs = 0;
		for(i = 0; i < 4; ++i) {
			if (GET_BIT(sourced, i))
				inst->SrcReg[src].Swizzle |= i << (3*i);
			else
				inst->SrcReg[src].Swizzle |= SWIZZLE_NIL << (3*i);
		}
		deswz_source = sourced;
	}

	struct register_state *regstate = get_reg_state(s, inst->SrcReg[src].File, inst->SrcReg[src].Index);
	if (regstate)
		regstate->Sourced |= deswz_source & 0xf;

	return inst;
}


static void rewrite_depth_out(struct prog_instruction *inst)
{
	if (inst->DstReg.WriteMask & WRITEMASK_Z) {
		inst->DstReg.WriteMask = WRITEMASK_W;
	} else {
		inst->DstReg.WriteMask = 0;
		return;
	}

	switch (inst->Opcode) {
	case OPCODE_FRC:
	case OPCODE_MOV:
		inst->SrcReg[0] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[0]);
		break;
	case OPCODE_ADD:
	case OPCODE_MAX:
	case OPCODE_MIN:
	case OPCODE_MUL:
		inst->SrcReg[0] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[0]);
		inst->SrcReg[1] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[1]);
		break;
	case OPCODE_CMP:
	case OPCODE_MAD:
		inst->SrcReg[0] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[0]);
		inst->SrcReg[1] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[1]);
		inst->SrcReg[2] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[2]);
		break;
	default:
		// Scalar instructions needn't be reswizzled
		break;
	}
}

static void unalias_srcregs(struct prog_instruction *inst, GLuint oldindex, GLuint newindex)
{
	int nsrc = _mesa_num_inst_src_regs(inst->Opcode);
	int i;
	for(i = 0; i < nsrc; ++i)
		if (inst->SrcReg[i].File == PROGRAM_TEMPORARY && inst->SrcReg[i].Index == oldindex)
			inst->SrcReg[i].Index = newindex;
}

static void unalias_temporary(struct nqssadce_state* s, GLuint oldindex)
{
	GLuint newindex = _mesa_find_free_register(s->Program, PROGRAM_TEMPORARY);
	int ip;
	for(ip = 0; ip < s->IP; ++ip) {
		struct prog_instruction* inst = s->Program->Instructions + ip;
		if (inst->DstReg.File == PROGRAM_TEMPORARY && inst->DstReg.Index == oldindex)
			inst->DstReg.Index = newindex;
		unalias_srcregs(inst, oldindex, newindex);
	}
	unalias_srcregs(s->Program->Instructions + s->IP, oldindex, newindex);
}


/**
 * Handle one instruction.
 */
static void process_instruction(struct nqssadce_state* s)
{
	struct prog_instruction *inst = s->Program->Instructions + s->IP;

	if (inst->Opcode == OPCODE_END)
		return;

	if (inst->Opcode != OPCODE_KIL) {
		if (s->Descr->RewriteDepthOut) {
			if (inst->DstReg.File == PROGRAM_OUTPUT && inst->DstReg.Index == FRAG_RESULT_DEPR)
				rewrite_depth_out(inst);
		}

		struct register_state *regstate = get_reg_state(s, inst->DstReg.File, inst->DstReg.Index);
		if (!regstate) {
			_mesa_problem(s->Ctx, "NqssaDce: bad destination register (%i[%i])\n",
				inst->DstReg.File, inst->DstReg.Index);
			return;
		}

		inst->DstReg.WriteMask &= regstate->Sourced;
		regstate->Sourced &= ~inst->DstReg.WriteMask;

		if (inst->DstReg.WriteMask == 0) {
			_mesa_delete_instructions(s->Program, s->IP, 1);
			return;
		}

		if (inst->DstReg.File == PROGRAM_TEMPORARY && !regstate->Sourced)
			unalias_temporary(s, inst->DstReg.Index);
	}

	/* Attention: Due to swizzle emulation code, the following
	 * might change the instruction stream under us, so we have
	 * to be careful with the inst pointer. */
	switch (inst->Opcode) {
	case OPCODE_DDX:
	case OPCODE_DDY:
	case OPCODE_FRC:
	case OPCODE_MOV:
		inst = track_used_srcreg(s, inst, 0, inst->DstReg.WriteMask);
		break;
	case OPCODE_ADD:
	case OPCODE_MAX:
	case OPCODE_MIN:
	case OPCODE_MUL:
		inst = track_used_srcreg(s, inst, 0, inst->DstReg.WriteMask);
		inst = track_used_srcreg(s, inst, 1, inst->DstReg.WriteMask);
		break;
	case OPCODE_CMP:
	case OPCODE_MAD:
		inst = track_used_srcreg(s, inst, 0, inst->DstReg.WriteMask);
		inst = track_used_srcreg(s, inst, 1, inst->DstReg.WriteMask);
		inst = track_used_srcreg(s, inst, 2, inst->DstReg.WriteMask);
		break;
	case OPCODE_COS:
	case OPCODE_EX2:
	case OPCODE_LG2:
	case OPCODE_RCP:
	case OPCODE_RSQ:
	case OPCODE_SIN:
		inst = track_used_srcreg(s, inst, 0, 0x1);
		break;
	case OPCODE_DP3:
		inst = track_used_srcreg(s, inst, 0, 0x7);
		inst = track_used_srcreg(s, inst, 1, 0x7);
		break;
	case OPCODE_DP4:
		inst = track_used_srcreg(s, inst, 0, 0xf);
		inst = track_used_srcreg(s, inst, 1, 0xf);
		break;
	case OPCODE_KIL:
	case OPCODE_TEX:
	case OPCODE_TXB:
	case OPCODE_TXP:
		inst = track_used_srcreg(s, inst, 0, 0xf);
		break;
	default:
		_mesa_problem(s->Ctx, "NqssaDce: Unknown opcode %d\n", inst->Opcode);
		return;
	}
}


void radeonNqssaDce(GLcontext *ctx, struct gl_program *p, struct radeon_nqssadce_descr* descr)
{
	struct nqssadce_state s;

	_mesa_bzero(&s, sizeof(s));
	s.Ctx = ctx;
	s.Program = p;
	s.Descr = descr;
	s.Descr->Init(&s);
	s.IP = p->NumInstructions;

	while(s.IP > 0) {
		s.IP--;
		process_instruction(&s);
	}
}
