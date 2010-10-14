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

struct peephole_state {
	struct rc_instruction * Inst;
	/** Stores a bitmask of the components that are still "alive" (i.e.
	 * they have not been written to since Inst was executed.)
	 */
	unsigned int WriteMask;
};

typedef void (*rc_presub_replace_fn)(struct peephole_state *,
						struct rc_instruction *,
						unsigned int);

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

struct copy_propagate_state {
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
static void copy_propagate_scan_read(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct copy_propagate_state * s = data;

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

static void copy_propagate_scan_write(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct copy_propagate_state * s = data;

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

static void copy_propagate(struct radeon_compiler * c, struct rc_instruction * inst_mov)
{
	struct copy_propagate_state s;

	if (inst_mov->U.I.DstReg.File != RC_FILE_TEMPORARY ||
	    inst_mov->U.I.DstReg.RelAddr ||
	    inst_mov->U.I.WriteALUResult ||
	    inst_mov->U.I.SaturateMode)
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
		const struct rc_opcode_info * info = rc_get_opcode_info(inst->U.I.Opcode);
		/* XXX In the future we might be able to make the optimizer
		 * smart enough to handle loops. */
		if(inst->U.I.Opcode == RC_OPCODE_BGNLOOP
				|| inst->U.I.Opcode == RC_OPCODE_ENDLOOP){
			return;
		}

		/* It is possible to do copy propigation in this situation,
		 * just not right now, see peephole_add_presub_inv() */
		if (inst_mov->U.I.PreSub.Opcode != RC_PRESUB_NONE &&
				(info->NumSrcRegs > 2 || info->HasTexture)) {
			return;
		}

		rc_for_all_reads_mask(inst, copy_propagate_scan_read, &s);
		rc_for_all_writes_mask(inst, copy_propagate_scan_write, &s);
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

				if ((refmask & s.MovMask) == refmask) {
					inst->U.I.SrcReg[src] = chain_srcregs(inst->U.I.SrcReg[src], s.Mov->U.I.SrcReg[0]);
					if (s.Mov->U.I.SrcReg[0].File == RC_FILE_PRESUB)
						inst->U.I.PreSub = s.Mov->U.I.PreSub;
				}
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
	unsigned int i;

	/* Replace 0.0, 1.0 and 0.5 immediates by their explicit swizzles */
	for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
		struct rc_constant * constant;
		struct rc_src_register newsrc;
		int have_real_reference;

		if (inst->U.I.SrcReg[src].File != RC_FILE_CONSTANT ||
		    inst->U.I.SrcReg[src].RelAddr ||
		    inst->U.I.SrcReg[src].Index >= c->Program.Constants.Count)
			continue;

		constant =
			&c->Program.Constants.Constants[inst->U.I.SrcReg[src].Index];

		if (constant->Type != RC_CONSTANT_IMMEDIATE)
			continue;

		newsrc = inst->U.I.SrcReg[src];
		have_real_reference = 0;
		for(unsigned int chan = 0; chan < 4; ++chan) {
			unsigned int swz = GET_SWZ(newsrc.Swizzle, chan);
			unsigned int newswz;
			float imm;
			float baseimm;

			if (swz >= 4)
				continue;

			imm = constant->u.Immediate[swz];
			baseimm = imm;
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

	/* In case this instruction has been converted, make sure all of the
	 * registers that are no longer used are empty. */
	opcode = rc_get_opcode_info(inst->U.I.Opcode);
	for(i = opcode->NumSrcRegs; i < 3; i++) {
		memset(&inst->U.I.SrcReg[i], 0, sizeof(struct rc_src_register));
	}
}

/**
 * If src and dst use the same register, this function returns a writemask that
 * indicates wich components are read by src.  Otherwise zero is returned.
 */
static unsigned int src_reads_dst_mask(struct rc_src_register src,
						struct rc_dst_register dst)
{
	unsigned int mask = 0;
	unsigned int i;
	if (dst.File != src.File || dst.Index != src.Index) {
		return 0;
	}

	for(i = 0; i < 4; i++) {
		mask |= 1 << GET_SWZ(src.Swizzle, i);
	}
	mask &= RC_MASK_XYZW;

	return mask;
}

/* Return 1 if the source registers has a constant swizzle (e.g. 0, 0.5, 1.0)
 * in any of its channels.  Return 0 otherwise. */
static int src_has_const_swz(struct rc_src_register src) {
	int chan;
	for(chan = 0; chan < 4; chan++) {
		unsigned int swz = GET_SWZ(src.Swizzle, chan);
		if (swz == RC_SWIZZLE_ZERO || swz == RC_SWIZZLE_HALF
						|| swz == RC_SWIZZLE_ONE) {
			return 1;
		}
	}
	return 0;
}

static void peephole_scan_write(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct peephole_state * s = data;
	if(s->Inst->U.I.DstReg.File == file
	   && s->Inst->U.I.DstReg.Index == index) {
		unsigned int common_mask = s->WriteMask & mask;
		s->WriteMask &= ~common_mask;
	}
}

static int presub_helper(
	struct radeon_compiler * c,
	struct peephole_state * s,
	rc_presubtract_op presub_opcode,
	rc_presub_replace_fn presub_replace)
{
	struct rc_instruction * inst;
	unsigned int can_remove = 0;
	unsigned int cant_sub = 0;

	for(inst = s->Inst->Next; inst != &c->Program.Instructions;
							inst = inst->Next) {
		unsigned int i;
		unsigned char can_use_presub = 1;
		const struct rc_opcode_info * info =
					rc_get_opcode_info(inst->U.I.Opcode);
		/* XXX: There are some situations where instructions
		 * with more than 2 src registers can use the
		 * presubtract select, but to keep things simple we
		 * will disable presubtract on these instructions for
		 * now. */
		if (info->NumSrcRegs > 2 || info->HasTexture) {
			can_use_presub = 0;
		}

		/* We can't use more than one presubtract value in an
		 * instruction, unless the two prsubtract operations
		 * are the same and read from the same registers. */
		if (inst->U.I.PreSub.Opcode != RC_PRESUB_NONE) {
			if (inst->U.I.PreSub.Opcode != presub_opcode
				|| inst->U.I.PreSub.SrcReg[0].File !=
					s->Inst->U.I.SrcReg[1].File
				|| inst->U.I.PreSub.SrcReg[0].Index !=
					s->Inst->U.I.SrcReg[1].Index) {
				can_use_presub = 0;
			}
		}

		/* Even if the instruction can't use a presubtract operation
		 * we still need to check if the instruction reads from
		 * s->Inst->U.I.DstReg, because if it does we must not
		 * remove s->Inst. */
		for(i = 0; i < info->NumSrcRegs; i++) {
			unsigned int mask = src_reads_dst_mask(
				inst->U.I.SrcReg[i], s->Inst->U.I.DstReg);
			/* XXX We could be more aggressive here using
			 * presubtract.  It is okay if SrcReg[i] only reads
			 * from some of the mask components. */
			if(s->Inst->U.I.DstReg.WriteMask != mask) {
				if (s->Inst->U.I.DstReg.WriteMask & mask) {
					can_remove = 0;
					break;
				} else {
					continue;
				}
			}
			if (cant_sub || !can_use_presub) {
				can_remove = 0;
				break;
			}
			presub_replace(s, inst, i);
			can_remove = 1;
		}
		if(!can_remove)
			break;
		rc_for_all_writes_mask(inst, peephole_scan_write, s);
		/* If all components of inst_add's destination register have
		 * been written to by subsequent instructions, the original
		 * value of the destination register is no longer valid and
		 * we can't keep doing substitutions. */
		if (!s->WriteMask){
			break;
		}
		/* Make this instruction doesn't write to the presubtract source. */
		if (inst->U.I.DstReg.WriteMask &
				src_reads_dst_mask(s->Inst->U.I.SrcReg[1],
							inst->U.I.DstReg)
				|| src_reads_dst_mask(s->Inst->U.I.SrcReg[0],
							inst->U.I.DstReg)
				|| info->IsFlowControl) {
			cant_sub = 1;
		}
	}
	return can_remove;
}

/* This function assumes that s->Inst->U.I.SrcReg[0] and
 * s->Inst->U.I.SrcReg[1] aren't both negative. */
static void presub_replace_add(struct peephole_state *s,
						struct rc_instruction * inst,
						unsigned int src_index)
{
	rc_presubtract_op presub_opcode;
	if (s->Inst->U.I.SrcReg[1].Negate || s->Inst->U.I.SrcReg[0].Negate)
		presub_opcode = RC_PRESUB_SUB;
	else
		presub_opcode = RC_PRESUB_ADD;

	if (s->Inst->U.I.SrcReg[1].Negate) {
		inst->U.I.PreSub.SrcReg[0] = s->Inst->U.I.SrcReg[1];
		inst->U.I.PreSub.SrcReg[1] = s->Inst->U.I.SrcReg[0];
	} else {
		inst->U.I.PreSub.SrcReg[0] = s->Inst->U.I.SrcReg[0];
		inst->U.I.PreSub.SrcReg[1] = s->Inst->U.I.SrcReg[1];
	}
	inst->U.I.PreSub.SrcReg[0].Negate = 0;
	inst->U.I.PreSub.SrcReg[1].Negate = 0;
	inst->U.I.PreSub.Opcode = presub_opcode;
	inst->U.I.SrcReg[src_index] = chain_srcregs(inst->U.I.SrcReg[src_index],
						inst->U.I.PreSub.SrcReg[0]);
	inst->U.I.SrcReg[src_index].File = RC_FILE_PRESUB;
	inst->U.I.SrcReg[src_index].Index = presub_opcode;
}

static int is_presub_candidate(struct rc_instruction * inst)
{
	const struct rc_opcode_info * info = rc_get_opcode_info(inst->U.I.Opcode);
	unsigned int i;

	if (inst->U.I.PreSub.Opcode != RC_PRESUB_NONE || inst->U.I.SaturateMode)
		return 0;

	for(i = 0; i < info->NumSrcRegs; i++) {
		if (src_reads_dst_mask(inst->U.I.SrcReg[i], inst->U.I.DstReg))
			return 0;
	}
	return 1;
}

static int peephole_add_presub_add(
	struct radeon_compiler * c,
	struct rc_instruction * inst_add)
{
	struct rc_src_register * src0 = NULL;
	struct rc_src_register * src1 = NULL;
	unsigned int i;
	struct peephole_state s;

	if (!is_presub_candidate(inst_add))
		return 0;

	if (inst_add->U.I.SrcReg[0].Swizzle != inst_add->U.I.SrcReg[1].Swizzle)
		return 0;

	/* src0 and src1 can't have absolute values only one can be negative and they must be all negative or all positive. */
	for (i = 0; i < 2; i++) {
		if (inst_add->U.I.SrcReg[i].Abs)
			return 0;
		if ((inst_add->U.I.SrcReg[i].Negate
					& inst_add->U.I.DstReg.WriteMask) ==
						inst_add->U.I.DstReg.WriteMask) {
			src0 = &inst_add->U.I.SrcReg[i];
		} else if (!src1) {
			src1 = &inst_add->U.I.SrcReg[i];
		} else {
			src0 = &inst_add->U.I.SrcReg[i];
		}
	}

	if (!src1)
		return 0;

	s.Inst = inst_add;
	s.WriteMask = inst_add->U.I.DstReg.WriteMask;
	if (presub_helper(c, &s, RC_PRESUB_ADD, presub_replace_add)) {
		rc_remove_instruction(inst_add);
		return 1;
	}
	return 0;
}

static void presub_replace_inv(struct peephole_state * s,
						struct rc_instruction * inst,
						unsigned int src_index)
{
	/* We must be careful not to modify s->Inst, since it
	 * is possible it will remain part of the program. 
	 * XXX Maybe pass a struct instead of a pointer for s->Inst.*/
	inst->U.I.PreSub.SrcReg[0] = s->Inst->U.I.SrcReg[1];
	inst->U.I.PreSub.SrcReg[0].Negate = 0;
	inst->U.I.PreSub.Opcode = RC_PRESUB_INV;
	inst->U.I.SrcReg[src_index] = chain_srcregs(inst->U.I.SrcReg[src_index],
						inst->U.I.PreSub.SrcReg[0]);

	inst->U.I.SrcReg[src_index].File = RC_FILE_PRESUB;
	inst->U.I.SrcReg[src_index].Index = RC_PRESUB_INV;
}

/**
 * PRESUB_INV: ADD TEMP[0], none.1, -TEMP[1]
 * Use the presubtract 1 - src0 for all readers of TEMP[0].  The first source
 * of the add instruction must have the constatnt 1 swizzle.  This function
 * does not check const registers to see if their value is 1.0, so it should
 * be called after the constant_folding optimization.
 * @return 
 * 	0 if the ADD instruction is still part of the program.
 * 	1 if the ADD instruction is no longer part of the program.
 */
static int peephole_add_presub_inv(
	struct radeon_compiler * c,
	struct rc_instruction * inst_add)
{
	unsigned int i, swz, mask;
	struct peephole_state s;

	if (!is_presub_candidate(inst_add))
		return 0;

	mask = inst_add->U.I.DstReg.WriteMask;

	/* Check if src0 is 1. */
	/* XXX It would be nice to use is_src_uniform_constant here, but that
	 * function only works if the register's file is RC_FILE_NONE */
	for(i = 0; i < 4; i++ ) {
		swz = GET_SWZ(inst_add->U.I.SrcReg[0].Swizzle, i);
		if(((1 << i) & inst_add->U.I.DstReg.WriteMask)
						&& swz != RC_SWIZZLE_ONE) {
			return 0;
		}
	}

	/* Check src1. */
	if ((inst_add->U.I.SrcReg[1].Negate & inst_add->U.I.DstReg.WriteMask) !=
						inst_add->U.I.DstReg.WriteMask
		|| inst_add->U.I.SrcReg[1].Abs
		|| (inst_add->U.I.SrcReg[1].File != RC_FILE_TEMPORARY
			&& inst_add->U.I.SrcReg[1].File != RC_FILE_CONSTANT)
		|| src_has_const_swz(inst_add->U.I.SrcReg[1])) {

		return 0;
	}

	/* Setup the peephole_state information. */
	s.Inst = inst_add;
	s.WriteMask = inst_add->U.I.DstReg.WriteMask;

	if (presub_helper(c, &s, RC_PRESUB_INV, presub_replace_inv)) {
		rc_remove_instruction(inst_add);
		return 1;
	}
	return 0;
}

/**
 * @return
 * 	0 if inst is still part of the program.
 * 	1 if inst is no longer part of the program.
 */
static int peephole(struct radeon_compiler * c, struct rc_instruction * inst)
{
	switch(inst->U.I.Opcode){
	case RC_OPCODE_ADD:
		if (c->has_presub) {
			if(peephole_add_presub_inv(c, inst))
				return 1;
			if(peephole_add_presub_add(c, inst))
				return 1;
		}
		break;
	default:
		break;
	}
	return 0;
}

void rc_optimize(struct radeon_compiler * c, void *user)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;
	while(inst != &c->Program.Instructions) {
		struct rc_instruction * cur = inst;
		inst = inst->Next;

		constant_folding(c, cur);

		if(peephole(c, cur))
			continue;

		if (cur->U.I.Opcode == RC_OPCODE_MOV) {
			copy_propagate(c, cur);
			/* cur may no longer be part of the program */
		}
	}
}
