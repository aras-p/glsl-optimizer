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
#include "radeon_swizzle.h"


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

/**
 * This is a callback function that is meant to be passed to
 * rc_for_all_reads_mask.  This function will be called once for each source
 * register in inst.
 * @param inst The instruction that the source register belongs to.
 * @param file The register file of the source register.
 * @param index The index of the source register.
 * @param mask The components of the source register that are being read from.
 */
static void peephole_scan_read(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct peephole_state * s = data;

	/* XXX This could probably be handled better. */
	if (file == RC_FILE_ADDRESS) {
		s->Conflict = 1;
		return;
	}

	if (file != RC_FILE_TEMPORARY || index != s->Mov->U.I.DstReg.Index)
		return;

	/* These instructions cannot read from the constants file.
	 * see radeonTransformTEX()
	 */
	if(s->Mov->U.I.SrcReg[0].File != RC_FILE_TEMPORARY &&
			s->Mov->U.I.SrcReg[0].File != RC_FILE_INPUT &&
				(inst->U.I.Opcode == RC_OPCODE_TEX ||
				inst->U.I.Opcode == RC_OPCODE_TXB ||
				inst->U.I.Opcode == RC_OPCODE_TXP ||
				inst->U.I.Opcode == RC_OPCODE_KIL)){
		s->Conflict = 1;
		return;
	}
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
	}
	if (file == s->Mov->U.I.SrcReg[0].File && index == s->Mov->U.I.SrcReg[0].Index) {
		if (mask & s->SourcedMask)
			s->SourceClobbered = 1;
	} else if (s->Mov->U.I.SrcReg[0].RelAddr && file == RC_FILE_ADDRESS) {
		s->SourceClobbered = 1;
	}
}

static void peephole(struct radeon_compiler * c, struct rc_instruction * inst_mov)
{
	struct peephole_state s;

	if (inst_mov->U.I.DstReg.File != RC_FILE_TEMPORARY ||
	    inst_mov->U.I.DstReg.RelAddr ||
	    inst_mov->U.I.WriteALUResult)
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
		/* XXX In the future we might be able to make the optimizer
		 * smart enough to handle loops. */
		if(inst->U.I.Opcode == RC_OPCODE_BGNLOOP
				|| inst->U.I.Opcode == RC_OPCODE_ENDLOOP){
			return;
		}
		rc_for_all_reads_mask(inst, peephole_scan_read, &s);
		rc_for_all_writes_mask(inst, peephole_scan_write, &s);
		if (s.Conflict)
			return;

		if (s.BranchDepth >= 0) {
			if (inst->U.I.Opcode == RC_OPCODE_IF) {
				s.BranchDepth++;
			} else if (inst->U.I.Opcode == RC_OPCODE_ENDIF
				|| inst->U.I.Opcode == RC_OPCODE_ELSE) {
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
			} else if (inst->U.I.Opcode == RC_OPCODE_ENDIF
				|| inst->U.I.Opcode == RC_OPCODE_ELSE) {
				s.BranchDepth--;
				if (s.BranchDepth < 0)
					break; /* no more readers after this point */
			}
		}
	}

	/* Finally, remove the original MOV instruction */
	rc_remove_instruction(inst_mov);
}

/**
 * Check if a source register is actually always the same
 * swizzle constant.
 */
static int is_src_uniform_constant(struct rc_src_register src,
		rc_swizzle * pswz, unsigned int * pnegate)
{
	int have_used = 0;

	if (src.File != RC_FILE_NONE) {
		*pswz = 0;
		return 0;
	}

	for(unsigned int chan = 0; chan < 4; ++chan) {
		unsigned int swz = GET_SWZ(src.Swizzle, chan);
		if (swz < 4) {
			*pswz = 0;
			return 0;
		}
		if (swz == RC_SWIZZLE_UNUSED)
			continue;

		if (!have_used) {
			*pswz = swz;
			*pnegate = GET_BIT(src.Negate, chan);
			have_used = 1;
		} else {
			if (swz != *pswz || *pnegate != GET_BIT(src.Negate, chan)) {
				*pswz = 0;
				return 0;
			}
		}
	}

	return 1;
}


static void constant_folding_mad(struct rc_instruction * inst)
{
	rc_swizzle swz;
	unsigned int negate;

	if (is_src_uniform_constant(inst->U.I.SrcReg[2], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MUL;
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[1], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_ADD;
			if (negate)
				inst->U.I.SrcReg[0].Negate ^= RC_MASK_XYZW;
			inst->U.I.SrcReg[1] = inst->U.I.SrcReg[2];
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[2];
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[0], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_ADD;
			if (negate)
				inst->U.I.SrcReg[1].Negate ^= RC_MASK_XYZW;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[2];
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[2];
			return;
		}
	}
}

static void constant_folding_mul(struct rc_instruction * inst)
{
	rc_swizzle swz;
	unsigned int negate;

	if (is_src_uniform_constant(inst->U.I.SrcReg[0], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[1];
			if (negate)
				inst->U.I.SrcReg[0].Negate ^= RC_MASK_XYZW;
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_0000;
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[1], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			if (negate)
				inst->U.I.SrcReg[0].Negate ^= RC_MASK_XYZW;
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_0000;
			return;
		}
	}
}

static void constant_folding_add(struct rc_instruction * inst)
{
	rc_swizzle swz;
	unsigned int negate;

	if (is_src_uniform_constant(inst->U.I.SrcReg[0], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[1];
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[1], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			return;
		}
	}
}


/**
 * Replace 0.0, 1.0 and 0.5 immediate constants by their
 * respective swizzles. Simplify instructions like ADD dst, src, 0;
 */
static void constant_folding(struct radeon_compiler * c, struct rc_instruction * inst)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

	/* Replace 0.0, 1.0 and 0.5 immediates by their explicit swizzles */
	for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
		if (inst->U.I.SrcReg[src].File != RC_FILE_CONSTANT ||
		    inst->U.I.SrcReg[src].RelAddr ||
		    inst->U.I.SrcReg[src].Index >= c->Program.Constants.Count)
			continue;

		struct rc_constant * constant =
			&c->Program.Constants.Constants[inst->U.I.SrcReg[src].Index];

		if (constant->Type != RC_CONSTANT_IMMEDIATE)
			continue;

		struct rc_src_register newsrc = inst->U.I.SrcReg[src];
		int have_real_reference = 0;
		for(unsigned int chan = 0; chan < 4; ++chan) {
			unsigned int swz = GET_SWZ(newsrc.Swizzle, chan);
			if (swz >= 4)
				continue;

			unsigned int newswz;
			float imm = constant->u.Immediate[swz];
			float baseimm = imm;
			if (imm < 0.0)
				baseimm = -baseimm;

			if (baseimm == 0.0) {
				newswz = RC_SWIZZLE_ZERO;
			} else if (baseimm == 1.0) {
				newswz = RC_SWIZZLE_ONE;
			} else if (baseimm == 0.5 && c->has_half_swizzles) {
				newswz = RC_SWIZZLE_HALF;
			} else {
				have_real_reference = 1;
				continue;
			}

			SET_SWZ(newsrc.Swizzle, chan, newswz);
			if (imm < 0.0 && !newsrc.Abs)
				newsrc.Negate ^= 1 << chan;
		}

		if (!have_real_reference) {
			newsrc.File = RC_FILE_NONE;
			newsrc.Index = 0;
		}

		/* don't make the swizzle worse */
		if (!c->SwizzleCaps->IsNative(inst->U.I.Opcode, newsrc) &&
		    c->SwizzleCaps->IsNative(inst->U.I.Opcode, inst->U.I.SrcReg[src]))
			continue;

		inst->U.I.SrcReg[src] = newsrc;
	}

	/* Simplify instructions based on constants */
	if (inst->U.I.Opcode == RC_OPCODE_MAD)
		constant_folding_mad(inst);

	/* note: MAD can simplify to MUL or ADD */
	if (inst->U.I.Opcode == RC_OPCODE_MUL)
		constant_folding_mul(inst);
	else if (inst->U.I.Opcode == RC_OPCODE_ADD)
		constant_folding_add(inst);
}

void rc_optimize(struct radeon_compiler * c, void *user)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;
	while(inst != &c->Program.Instructions) {
		struct rc_instruction * cur = inst;
		inst = inst->Next;

		constant_folding(c, cur);

		if (cur->U.I.Opcode == RC_OPCODE_MOV) {
			peephole(c, cur);
			/* cur may no longer be part of the program */
		}
	}
}
