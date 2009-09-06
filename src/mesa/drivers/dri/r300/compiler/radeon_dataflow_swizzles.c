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


static void rewrite_source(struct radeon_compiler * c,
		struct rc_instruction * inst, unsigned src)
{
	struct rc_swizzle_split split;
	unsigned int tempreg = rc_find_free_temporary(c);
	unsigned int usemask;
	struct rc_dataflow_ref * oldref = inst->Dataflow.SrcReg[src];
	struct rc_dataflow_vector * vector = 0;

	usemask = 0;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		if (GET_SWZ(inst->I.SrcReg[src].Swizzle, chan) != RC_SWIZZLE_UNUSED)
			usemask |= 1 << chan;
	}

	c->SwizzleCaps->Split(inst->I.SrcReg[src], usemask, &split);

	for(unsigned int phase = 0; phase < split.NumPhases; ++phase) {
		struct rc_instruction * mov = rc_insert_new_instruction(c, inst->Prev);
		unsigned int phase_refmask;
		unsigned int masked_negate;

		mov->I.Opcode = RC_OPCODE_MOV;
		mov->I.DstReg.File = RC_FILE_TEMPORARY;
		mov->I.DstReg.Index = tempreg;
		mov->I.DstReg.WriteMask = split.Phase[phase];
		mov->I.SrcReg[0] = inst->I.SrcReg[src];

		phase_refmask = 0;
		for(unsigned int chan = 0; chan < 4; ++chan) {
			if (!GET_BIT(split.Phase[phase], chan))
				SET_SWZ(mov->I.SrcReg[0].Swizzle, chan, RC_SWIZZLE_UNUSED);
			else
				phase_refmask |= 1 << GET_SWZ(mov->I.SrcReg[0].Swizzle, chan);
		}

		phase_refmask &= RC_MASK_XYZW;

		masked_negate = split.Phase[phase] & mov->I.SrcReg[0].Negate;
		if (masked_negate == 0)
			mov->I.SrcReg[0].Negate = 0;
		else if (masked_negate == split.Phase[phase])
			mov->I.SrcReg[0].Negate = RC_MASK_XYZW;

		if (oldref) {
			mov->Dataflow.SrcReg[0] = rc_dataflow_create_ref(c, oldref->Vector, mov);
			mov->Dataflow.SrcReg[0]->UseMask = phase_refmask;
		}

		mov->Dataflow.DstReg = rc_dataflow_create_vector(c, RC_FILE_TEMPORARY, tempreg, mov);
		mov->Dataflow.DstReg->ValidMask = split.Phase[phase];

		if (vector) {
			mov->Dataflow.DstRegPrev = rc_dataflow_create_ref(c, vector, mov);
			mov->Dataflow.DstRegPrev->UseMask = vector->ValidMask;
			mov->Dataflow.DstReg->ValidMask |= vector->ValidMask;
			mov->Dataflow.DstRegAliased = 1;
		}

		mov->Dataflow.DstReg->UseMask = mov->Dataflow.DstReg->ValidMask;
		vector = mov->Dataflow.DstReg;
	}

	if (oldref)
		rc_dataflow_remove_ref(oldref);
	inst->Dataflow.SrcReg[src] = rc_dataflow_create_ref(c, vector, inst);
	inst->Dataflow.SrcReg[src]->UseMask = usemask;

	inst->I.SrcReg[src].File = RC_FILE_TEMPORARY;
	inst->I.SrcReg[src].Index = tempreg;
	inst->I.SrcReg[src].Swizzle = 0;
	inst->I.SrcReg[src].Negate = RC_MASK_NONE;
	inst->I.SrcReg[src].Abs = 0;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		SET_SWZ(inst->I.SrcReg[src].Swizzle, chan,
				GET_BIT(usemask, chan) ? chan : RC_SWIZZLE_UNUSED);
	}
}

void rc_dataflow_swizzles(struct radeon_compiler * c)
{
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);
		unsigned int src;

		for(src = 0; src < opcode->NumSrcRegs; ++src) {
			if (!c->SwizzleCaps->IsNative(inst->I.Opcode, inst->I.SrcReg[src]))
				rewrite_source(c, inst, src);
		}
	}
}
