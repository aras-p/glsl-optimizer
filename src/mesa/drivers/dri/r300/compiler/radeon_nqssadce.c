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
 */

#include "radeon_nqssadce.h"

#include "radeon_compiler.h"


/**
 * Return the @ref register_state for the given register (or 0 for untracked
 * registers, i.e. constants).
 */
static struct register_state *get_reg_state(struct nqssadce_state* s, rc_register_file file, unsigned int index)
{
	if (index >= RC_REGISTER_MAX_INDEX)
		return 0;

	switch(file) {
	case RC_FILE_TEMPORARY: return &s->Temps[index];
	case RC_FILE_OUTPUT: return &s->Outputs[index];
	case RC_FILE_ADDRESS: return &s->Address;
	default: return 0;
	}
}


static void track_used_srcreg(struct nqssadce_state* s,
	int src, unsigned int sourced)
{
	struct rc_sub_instruction * inst = &s->IP->I;
	int i;
	unsigned int deswz_source = 0;

	for(i = 0; i < 4; ++i) {
		if (GET_BIT(sourced, i)) {
			unsigned int swz = GET_SWZ(inst->SrcReg[src].Swizzle, i);
			deswz_source |= 1 << swz;
		} else {
			inst->SrcReg[src].Swizzle &= ~(7 << (3*i));
			inst->SrcReg[src].Swizzle |= RC_SWIZZLE_UNUSED << (3*i);
		}
	}

	if (!s->Descr->IsNativeSwizzle(inst->Opcode, inst->SrcReg[src])) {
		struct rc_dst_register dstreg = inst->DstReg;
		dstreg.File = RC_FILE_TEMPORARY;
		dstreg.Index = rc_find_free_temporary(s->Compiler);
		dstreg.WriteMask = sourced;

		s->Descr->BuildSwizzle(s, dstreg, inst->SrcReg[src]);

		inst->SrcReg[src].File = RC_FILE_TEMPORARY;
		inst->SrcReg[src].Index = dstreg.Index;
		inst->SrcReg[src].Swizzle = 0;
		inst->SrcReg[src].Negate = RC_MASK_NONE;
		inst->SrcReg[src].Abs = 0;
		for(i = 0; i < 4; ++i) {
			if (GET_BIT(sourced, i))
				inst->SrcReg[src].Swizzle |= i << (3*i);
			else
				inst->SrcReg[src].Swizzle |= RC_SWIZZLE_UNUSED << (3*i);
		}
		deswz_source = sourced;
	}

	struct register_state *regstate;

	if (inst->SrcReg[src].RelAddr) {
		regstate = get_reg_state(s, RC_FILE_ADDRESS, 0);
		if (regstate)
			regstate->Sourced |= RC_MASK_X;
	} else {
		regstate = get_reg_state(s, inst->SrcReg[src].File, inst->SrcReg[src].Index);
		if (regstate)
			regstate->Sourced |= deswz_source & 0xf;
	}
}

static void unalias_srcregs(struct rc_instruction *inst, unsigned int oldindex, unsigned int newindex)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);
	int i;
	for(i = 0; i < opcode->NumSrcRegs; ++i)
		if (inst->I.SrcReg[i].File == RC_FILE_TEMPORARY && inst->I.SrcReg[i].Index == oldindex)
			inst->I.SrcReg[i].Index = newindex;
}

static void unalias_temporary(struct nqssadce_state* s, unsigned int oldindex)
{
	unsigned int newindex = rc_find_free_temporary(s->Compiler);
	struct rc_instruction * inst;
	for(inst = s->Compiler->Program.Instructions.Next; inst != s->IP; inst = inst->Next) {
		if (inst->I.DstReg.File == RC_FILE_TEMPORARY && inst->I.DstReg.Index == oldindex)
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
	struct rc_sub_instruction *inst = &s->IP->I;
	unsigned int WriteMask;

	if (inst->Opcode != RC_OPCODE_KIL) {
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

		if (inst->DstReg.File == RC_FILE_TEMPORARY && !regstate->Sourced)
			unalias_temporary(s, inst->DstReg.Index);
	}

	WriteMask = inst->DstReg.WriteMask;

	switch (inst->Opcode) {
	case RC_OPCODE_ARL:
	case RC_OPCODE_DDX:
	case RC_OPCODE_DDY:
	case RC_OPCODE_FRC:
	case RC_OPCODE_MOV:
		track_used_srcreg(s, 0, WriteMask);
		break;
	case RC_OPCODE_ADD:
	case RC_OPCODE_MAX:
	case RC_OPCODE_MIN:
	case RC_OPCODE_MUL:
	case RC_OPCODE_SGE:
	case RC_OPCODE_SLT:
		track_used_srcreg(s, 0, WriteMask);
		track_used_srcreg(s, 1, WriteMask);
		break;
	case RC_OPCODE_CMP:
	case RC_OPCODE_MAD:
		track_used_srcreg(s, 0, WriteMask);
		track_used_srcreg(s, 1, WriteMask);
		track_used_srcreg(s, 2, WriteMask);
		break;
	case RC_OPCODE_COS:
	case RC_OPCODE_EX2:
	case RC_OPCODE_LG2:
	case RC_OPCODE_RCP:
	case RC_OPCODE_RSQ:
	case RC_OPCODE_SIN:
		track_used_srcreg(s, 0, 0x1);
		break;
	case RC_OPCODE_DP3:
		track_used_srcreg(s, 0, 0x7);
		track_used_srcreg(s, 1, 0x7);
		break;
	case RC_OPCODE_DP4:
		track_used_srcreg(s, 0, 0xf);
		track_used_srcreg(s, 1, 0xf);
		break;
	case RC_OPCODE_KIL:
	case RC_OPCODE_TEX:
	case RC_OPCODE_TXB:
	case RC_OPCODE_TXP:
		track_used_srcreg(s, 0, 0xf);
		break;
	case RC_OPCODE_DST:
		track_used_srcreg(s, 0, 0x6);
		track_used_srcreg(s, 1, 0xa);
		break;
	case RC_OPCODE_EXP:
	case RC_OPCODE_LOG:
	case RC_OPCODE_POW:
		track_used_srcreg(s, 0, 0x3);
		break;
	case RC_OPCODE_LIT:
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
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);
		int i;

		for (i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->I.SrcReg[i].File == RC_FILE_INPUT)
				c->Program.InputsRead |= 1 << inst->I.SrcReg[i].Index;
		}

		if (opcode->HasDstReg) {
			if (inst->I.DstReg.File == RC_FILE_OUTPUT)
				c->Program.OutputsWritten |= 1 << inst->I.DstReg.Index;
		}
	}
}

void radeonNqssaDce(struct radeon_compiler * c, struct radeon_nqssadce_descr* descr, void * data)
{
	struct nqssadce_state s;

	memset(&s, 0, sizeof(s));
	s.Compiler = c;
	s.Descr = descr;
	s.UserData = data;
	s.Descr->Init(&s);
	s.IP = c->Program.Instructions.Prev;

	while(s.IP != &c->Program.Instructions && !c->Error)
		process_instruction(&s);

	rc_calculate_inputs_outputs(c);
}
