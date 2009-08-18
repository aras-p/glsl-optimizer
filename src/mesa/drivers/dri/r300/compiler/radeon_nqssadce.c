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

#include "radeon_compiler.h"


/**
 * Return the @ref register_state for the given register (or 0 for untracked
 * registers, i.e. constants).
 */
static struct register_state *get_reg_state(struct nqssadce_state* s, GLuint file, GLuint index)
{
	switch(file) {
	case PROGRAM_TEMPORARY: return &s->Temps[index];
	case PROGRAM_OUTPUT: return &s->Outputs[index];
	case PROGRAM_ADDRESS: return &s->Address;
	default: return 0;
	}
}


/**
 * Left multiplication of a register with a swizzle
 *
 * @note Works correctly only for X, Y, Z, W swizzles, not for constant swizzles.
 */
struct prog_src_register lmul_swizzle(GLuint swizzle, struct prog_src_register srcreg)
{
	struct prog_src_register tmp = srcreg;
	int i;
	tmp.Swizzle = 0;
	tmp.Negate = NEGATE_NONE;
	for(i = 0; i < 4; ++i) {
		GLuint swz = GET_SWZ(swizzle, i);
		if (swz < 4) {
			tmp.Swizzle |= GET_SWZ(srcreg.Swizzle, swz) << (i*3);
			tmp.Negate |= GET_BIT(srcreg.Negate, swz) << i;
		} else {
			tmp.Swizzle |= swz << (i*3);
		}
	}
	return tmp;
}


static void track_used_srcreg(struct nqssadce_state* s,
	GLint src, GLuint sourced)
{
	struct prog_instruction * inst = &s->IP->I;
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
		dstreg.Index = rc_find_free_temporary(s->Compiler);
		dstreg.WriteMask = sourced;

		s->Descr->BuildSwizzle(s, dstreg, inst->SrcReg[src]);

		inst->SrcReg[src].File = PROGRAM_TEMPORARY;
		inst->SrcReg[src].Index = dstreg.Index;
		inst->SrcReg[src].Swizzle = 0;
		inst->SrcReg[src].Negate = NEGATE_NONE;
		inst->SrcReg[src].Abs = 0;
		for(i = 0; i < 4; ++i) {
			if (GET_BIT(sourced, i))
				inst->SrcReg[src].Swizzle |= i << (3*i);
			else
				inst->SrcReg[src].Swizzle |= SWIZZLE_NIL << (3*i);
		}
		deswz_source = sourced;
	}

	struct register_state *regstate;

	if (inst->SrcReg[src].RelAddr) {
		regstate = get_reg_state(s, PROGRAM_ADDRESS, 0);
		if (regstate)
			regstate->Sourced |= WRITEMASK_X;
	} else {
		regstate = get_reg_state(s, inst->SrcReg[src].File, inst->SrcReg[src].Index);
		if (regstate)
			regstate->Sourced |= deswz_source & 0xf;
	}
}

static void unalias_srcregs(struct rc_instruction *inst, GLuint oldindex, GLuint newindex)
{
	int nsrc = _mesa_num_inst_src_regs(inst->I.Opcode);
	int i;
	for(i = 0; i < nsrc; ++i)
		if (inst->I.SrcReg[i].File == PROGRAM_TEMPORARY && inst->I.SrcReg[i].Index == oldindex)
			inst->I.SrcReg[i].Index = newindex;
}

static void unalias_temporary(struct nqssadce_state* s, GLuint oldindex)
{
	GLuint newindex = rc_find_free_temporary(s->Compiler);
	struct rc_instruction * inst;
	for(inst = s->Compiler->Program.Instructions.Next; inst != s->IP; inst = inst->Next) {
		if (inst->I.DstReg.File == PROGRAM_TEMPORARY && inst->I.DstReg.Index == oldindex)
			inst->I.DstReg.Index = newindex;
		unalias_srcregs(inst, oldindex, newindex);
	}
	unalias_srcregs(s->IP, oldindex, newindex);
}


/**
 * Handle one instruction.
 */
static void process_instruction(struct nqssadce_state* s)
{
	struct prog_instruction *inst = &s->IP->I;
	GLuint WriteMask;

	if (inst->Opcode == OPCODE_END)
		return;

	if (inst->Opcode != OPCODE_KIL) {
		struct register_state *regstate = get_reg_state(s, inst->DstReg.File, inst->DstReg.Index);
		if (!regstate) {
			rc_error(s->Compiler, "NqssaDce: bad destination register (%i[%i])\n",
				inst->DstReg.File, inst->DstReg.Index);
			return;
		}

		inst->DstReg.WriteMask &= regstate->Sourced;
		regstate->Sourced &= ~inst->DstReg.WriteMask;

		if (inst->DstReg.WriteMask == 0) {
			struct rc_instruction * inst_remove = s->IP;
			s->IP = s->IP->Prev;
			rc_remove_instruction(inst_remove);
			return;
		}

		if (inst->DstReg.File == PROGRAM_TEMPORARY && !regstate->Sourced)
			unalias_temporary(s, inst->DstReg.Index);
	}

	WriteMask = inst->DstReg.WriteMask;

	switch (inst->Opcode) {
	case OPCODE_ARL:
	case OPCODE_DDX:
	case OPCODE_DDY:
	case OPCODE_FRC:
	case OPCODE_MOV:
		track_used_srcreg(s, 0, WriteMask);
		break;
	case OPCODE_ADD:
	case OPCODE_MAX:
	case OPCODE_MIN:
	case OPCODE_MUL:
	case OPCODE_SGE:
	case OPCODE_SLT:
		track_used_srcreg(s, 0, WriteMask);
		track_used_srcreg(s, 1, WriteMask);
		break;
	case OPCODE_CMP:
	case OPCODE_MAD:
		track_used_srcreg(s, 0, WriteMask);
		track_used_srcreg(s, 1, WriteMask);
		track_used_srcreg(s, 2, WriteMask);
		break;
	case OPCODE_COS:
	case OPCODE_EX2:
	case OPCODE_LG2:
	case OPCODE_RCP:
	case OPCODE_RSQ:
	case OPCODE_SIN:
		track_used_srcreg(s, 0, 0x1);
		break;
	case OPCODE_DP3:
		track_used_srcreg(s, 0, 0x7);
		track_used_srcreg(s, 1, 0x7);
		break;
	case OPCODE_DP4:
		track_used_srcreg(s, 0, 0xf);
		track_used_srcreg(s, 1, 0xf);
		break;
	case OPCODE_KIL:
	case OPCODE_TEX:
	case OPCODE_TXB:
	case OPCODE_TXP:
		track_used_srcreg(s, 0, 0xf);
		break;
	case OPCODE_DST:
		track_used_srcreg(s, 0, 0x6);
		track_used_srcreg(s, 1, 0xa);
		break;
	case OPCODE_EXP:
	case OPCODE_LOG:
	case OPCODE_POW:
		track_used_srcreg(s, 0, 0x3);
		break;
	case OPCODE_LIT:
		track_used_srcreg(s, 0, 0xb);
		break;
	default:
		rc_error(s->Compiler, "NqssaDce: Unknown opcode %d\n", inst->Opcode);
		return;
	}

	s->IP = s->IP->Prev;
}

void rc_calculate_inputs_outputs(struct radeon_compiler * c)
{
	struct rc_instruction *inst;

	c->Program.InputsRead = 0;
	c->Program.OutputsWritten = 0;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next)
	{
		int i;
		int num_src_regs = _mesa_num_inst_src_regs(inst->I.Opcode);

		for (i = 0; i < num_src_regs; ++i) {
			if (inst->I.SrcReg[i].File == PROGRAM_INPUT)
				c->Program.InputsRead |= 1 << inst->I.SrcReg[i].Index;
		}

		if (_mesa_num_inst_dst_regs(inst->I.Opcode)) {
			if (inst->I.DstReg.File == PROGRAM_OUTPUT)
				c->Program.OutputsWritten |= 1 << inst->I.DstReg.Index;
		}
	}
}

void radeonNqssaDce(struct radeon_compiler * c, struct radeon_nqssadce_descr* descr, void * data)
{
	struct nqssadce_state s;

	_mesa_bzero(&s, sizeof(s));
	s.Compiler = c;
	s.Descr = descr;
	s.UserData = data;
	s.Descr->Init(&s);
	s.IP = c->Program.Instructions.Prev;

	while(s.IP != &c->Program.Instructions && !c->Error)
		process_instruction(&s);

	rc_calculate_inputs_outputs(c);
}
