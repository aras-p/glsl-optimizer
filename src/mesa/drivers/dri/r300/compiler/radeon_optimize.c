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

#include "radeon_dataflow.h"

#include "radeon_compiler.h"


static struct rc_src_register chain_srcregs(struct rc_src_register outer, struct rc_src_register inner)
{
	struct rc_src_register combine;
	combine.File = inner.File;
	combine.Index = inner.Index;
	combine.RelAddr = inner.RelAddr;
	if (outer.Abs) {
		combine.Abs = 1;
		combine.Negate = outer.Negate;
	} else {
		combine.Abs = inner.Abs;
		combine.Negate = 0;
		for(unsigned int chan = 0; chan < 4; ++chan) {
			unsigned int swz = GET_SWZ(outer.Swizzle, chan);
			if (swz < 4)
				combine.Negate |= GET_BIT(inner.Negate, swz) << chan;
		}
		combine.Negate ^= outer.Negate;
	}
	combine.Swizzle = combine_swizzles(inner.Swizzle, outer.Swizzle);
	return combine;
}

struct peephole_state {
	struct radeon_compiler * C;
	struct rc_instruction * Mov;
	unsigned int Conflict:1;

	/** Whether Mov's source has been clobbered */
	unsigned int SourceClobbered:1;

	/** Which components of Mov's destination register are still from that Mov? */
	unsigned int MovMask:4;

	/** Which components of Mov's destination register are clearly *not* from that Mov */
	unsigned int DefinedMask:4;

	/** Which components of Mov's source register are sourced */
	unsigned int SourcedMask:4;

	/** Branch depth beyond Mov; negative value indicates we left the Mov's block */
	int BranchDepth;
};

static void peephole_scan_read(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct peephole_state * s = data;

	if (file != RC_FILE_TEMPORARY || index != s->Mov->U.I.DstReg.Index)
		return;

	if ((mask & s->MovMask) == mask) {
		if (s->SourceClobbered) {
			s->Conflict = 1;
		}
	} else if ((mask & s->DefinedMask) == mask) {
		/* read from something entirely written by other instruction: this is okay */
	} else {
		/* read from component combination that is not well-defined without
		 * the MOV: cannot remove it */
		s->Conflict = 1;
	}
}

static void peephole_scan_write(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct peephole_state * s = data;

	if (s->BranchDepth < 0)
		return;

	if (file == s->Mov->U.I.DstReg.File && index == s->Mov->U.I.DstReg.Index) {
		s->MovMask &= ~mask;
		if (s->BranchDepth == 0)
			s->DefinedMask |= mask;
		else
			s->DefinedMask &= ~mask;
	} else if (file == s->Mov->U.I.SrcReg[0].File && index == s->Mov->U.I.SrcReg[0].Index) {
		if (mask & s->SourcedMask)
			s->SourceClobbered = 1;
	} else if (s->Mov->U.I.SrcReg[0].RelAddr && file == RC_FILE_ADDRESS) {
		s->SourceClobbered = 1;
	}
}

static void peephole(struct radeon_compiler * c, struct rc_instruction * inst_mov)
{
	struct peephole_state s;

	if (inst_mov->U.I.DstReg.File != RC_FILE_TEMPORARY || inst_mov->U.I.WriteALUResult)
		return;

	memset(&s, 0, sizeof(s));
	s.C = c;
	s.Mov = inst_mov;
	s.MovMask = inst_mov->U.I.DstReg.WriteMask;
	s.DefinedMask = RC_MASK_XYZW & ~s.MovMask;

	for(unsigned int chan = 0; chan < 4; ++chan) {
		unsigned int swz = GET_SWZ(inst_mov->U.I.SrcReg[0].Swizzle, chan);
		s.SourcedMask |= (1 << swz) & RC_MASK_XYZW;
	}

	/* 1st pass: Check whether all subsequent readers can be changed */
	for(struct rc_instruction * inst = inst_mov->Next;
	    inst != &c->Program.Instructions;
	    inst = inst->Next) {
		rc_for_all_reads_mask(inst, peephole_scan_read, &s);
		rc_for_all_writes_mask(inst, peephole_scan_write, &s);
		if (s.Conflict)
			return;

		if (s.BranchDepth >= 0) {
			if (inst->U.I.Opcode == RC_OPCODE_IF) {
				s.BranchDepth++;
			} else if (inst->U.I.Opcode == RC_OPCODE_ENDIF) {
				s.BranchDepth--;
				if (s.BranchDepth < 0) {
					s.DefinedMask &= ~s.MovMask;
					s.MovMask = 0;
				}
			}
		}
	}

	if (s.Conflict)
		return;

	/* 2nd pass: We can satisfy all readers, so switch them over all at once */
	s.MovMask = inst_mov->U.I.DstReg.WriteMask;
	s.BranchDepth = 0;

	for(struct rc_instruction * inst = inst_mov->Next;
	    inst != &c->Program.Instructions;
	    inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
			if (inst->U.I.SrcReg[src].File == RC_FILE_TEMPORARY &&
			    inst->U.I.SrcReg[src].Index == s.Mov->U.I.DstReg.Index) {
				unsigned int refmask = 0;

				for(unsigned int chan = 0; chan < 4; ++chan) {
					unsigned int swz = GET_SWZ(inst->U.I.SrcReg[src].Swizzle, chan);
					refmask |= (1 << swz) & RC_MASK_XYZW;
				}

				if ((refmask & s.MovMask) == refmask)
					inst->U.I.SrcReg[src] = chain_srcregs(inst->U.I.SrcReg[src], s.Mov->U.I.SrcReg[0]);
			}
		}

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_TEMPORARY &&
			    inst->U.I.DstReg.Index == s.Mov->U.I.DstReg.Index) {
				s.MovMask &= ~inst->U.I.DstReg.WriteMask;
			}
		}

		if (s.BranchDepth >= 0) {
			if (inst->U.I.Opcode == RC_OPCODE_IF) {
				s.BranchDepth++;
			} else if (inst->U.I.Opcode == RC_OPCODE_ENDIF) {
				s.BranchDepth--;
				if (s.BranchDepth < 0)
					break; /* no more readers after this point */
			}
		}
	}

	/* Finally, remove the original MOV instruction */
	rc_remove_instruction(inst_mov);
}

void rc_optimize(struct radeon_compiler * c)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;
	while(inst != &c->Program.Instructions) {
		struct rc_instruction * cur = inst;
		inst = inst->Next;

		if (cur->U.I.Opcode == RC_OPCODE_MOV)
			peephole(c, cur);
	}
}
