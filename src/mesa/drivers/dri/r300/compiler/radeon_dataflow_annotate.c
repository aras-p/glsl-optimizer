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


struct dataflow_state {
	struct radeon_compiler * C;
	unsigned int DCE:1;
	unsigned int UpdateRunning:1;

	struct rc_dataflow_vector * Input[RC_REGISTER_MAX_INDEX];
	struct rc_dataflow_vector * Output[RC_REGISTER_MAX_INDEX];
	struct rc_dataflow_vector * Temporary[RC_REGISTER_MAX_INDEX];
	struct rc_dataflow_vector * Address;

	struct rc_dataflow_vector ** UpdateStack;
	unsigned int UpdateStackSize;
	unsigned int UpdateStackReserved;
};

static void mark_vector_use(struct dataflow_state * s, struct rc_dataflow_vector * vector, unsigned int mask);

static struct rc_dataflow_vector * get_register_contents(struct dataflow_state * s,
		rc_register_file file, unsigned int index)
{
	if (file == RC_FILE_INPUT || file == RC_FILE_OUTPUT || file == RC_FILE_TEMPORARY) {
		if (index >= RC_REGISTER_MAX_INDEX)
			return 0; /* cannot happen, but be defensive */

		if (file == RC_FILE_TEMPORARY)
			return s->Temporary[index];
		if (file == RC_FILE_INPUT)
			return s->Input[index];
		if (file == RC_FILE_OUTPUT)
			return s->Output[index];
	}

	if (file == RC_FILE_ADDRESS)
		return s->Address;

	return 0; /* can happen, constant register file */
}

static void mark_ref_use(struct dataflow_state * s, struct rc_dataflow_ref * ref, unsigned int mask)
{
	if (!(mask & ~ref->UseMask))
		return;

	ref->UseMask |= mask;
	mark_vector_use(s, ref->Vector, ref->UseMask);
}

static void mark_source_use(struct dataflow_state * s, struct rc_instruction * inst,
		unsigned int src, unsigned int srcmask)
{
	unsigned int refmask = 0;

	for(unsigned int i = 0; i < 4; ++i) {
		if (GET_BIT(srcmask, i))
			refmask |= 1 << GET_SWZ(inst->I.SrcReg[src].Swizzle, i);
	}

	/* get rid of spurious bits from ZERO, ONE, etc. swizzles */
	refmask &= RC_MASK_XYZW;

	if (!refmask)
		return; /* can happen if the swizzle contains constant components */

	if (inst->Dataflow.SrcReg[src])
		mark_ref_use(s, inst->Dataflow.SrcReg[src], refmask);

	if (inst->Dataflow.SrcRegAddress[src])
		mark_ref_use(s, inst->Dataflow.SrcRegAddress[src], RC_MASK_X);
}

static void compute_sources_for_writemask(
		struct rc_instruction * inst,
		unsigned int writemask,
		unsigned int *srcmasks)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);

	srcmasks[0] = 0;
	srcmasks[1] = 0;
	srcmasks[2] = 0;

	if (inst->I.Opcode == RC_OPCODE_KIL)
		srcmasks[0] |= RC_MASK_XYZW;

	if (!writemask)
		return;

	if (opcode->IsComponentwise) {
		for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src)
			srcmasks[src] |= writemask;
	} else if (opcode->IsStandardScalar) {
		for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src)
			srcmasks[src] |= RC_MASK_X;
	} else {
		switch(inst->I.Opcode) {
		case RC_OPCODE_ARL:
			srcmasks[0] |= RC_MASK_X;
			break;
		case RC_OPCODE_DP3:
			srcmasks[0] |= RC_MASK_XYZ;
			srcmasks[1] |= RC_MASK_XYZ;
			break;
		case RC_OPCODE_DP4:
			srcmasks[0] |= RC_MASK_XYZW;
			srcmasks[1] |= RC_MASK_XYZW;
			break;
		case RC_OPCODE_TEX:
		case RC_OPCODE_TXB:
		case RC_OPCODE_TXP:
			srcmasks[0] |= RC_MASK_XYZW;
			break;
		case RC_OPCODE_DST:
			srcmasks[0] |= 0x6;
			srcmasks[1] |= 0xa;
			break;
		case RC_OPCODE_EXP:
		case RC_OPCODE_LOG:
			srcmasks[0] |= RC_MASK_XY;
			break;
		case RC_OPCODE_LIT:
			srcmasks[0] |= 0xb;
			break;
		default:
			break;
		}
	}
}

static void mark_instruction_source_use(struct dataflow_state * s,
		struct rc_instruction * inst, unsigned int writemask)
{
	unsigned int srcmasks[3];

	compute_sources_for_writemask(inst, writemask, srcmasks);

	for(unsigned int src = 0; src < 3; ++src)
		mark_source_use(s, inst, src, srcmasks[src]);
}

static void run_update(struct dataflow_state * s)
{
	s->UpdateRunning = 1;

	while(s->UpdateStackSize) {
		struct rc_dataflow_vector * vector = s->UpdateStack[--s->UpdateStackSize];
		vector->PassBit = 0;

		if (vector->WriteInstruction) {
			struct rc_instruction * inst = vector->WriteInstruction;

			if (inst->Dataflow.DstRegPrev) {
				unsigned int carryover = vector->UseMask & ~inst->I.DstReg.WriteMask;

				if (carryover)
					mark_ref_use(s, inst->Dataflow.DstRegPrev, carryover);
			}

			mark_instruction_source_use(
					s, vector->WriteInstruction,
					vector->UseMask & inst->I.DstReg.WriteMask);
		}
	}

	s->UpdateRunning = 0;
}

static void mark_vector_use(struct dataflow_state * s, struct rc_dataflow_vector * vector, unsigned int mask)
{
	if (!(mask & ~vector->UseMask))
		return; /* no new used bits */

	vector->UseMask |= mask;
	if (vector->PassBit)
		return;

	if (s->UpdateStackSize >= s->UpdateStackReserved) {
		unsigned int new_reserve = 2 * s->UpdateStackReserved;
		struct rc_dataflow_vector ** new_stack;

		if (!new_reserve)
			new_reserve = 16;

		new_stack = memory_pool_malloc(&s->C->Pool, new_reserve * sizeof(struct rc_dataflow_vector *));
		memcpy(new_stack, s->UpdateStack, s->UpdateStackSize * sizeof(struct rc_dataflow_vector *));

		s->UpdateStack = new_stack;
		s->UpdateStackReserved = new_reserve;
	}

	s->UpdateStack[s->UpdateStackSize++] = vector;
	vector->PassBit = 1;

	if (!s->UpdateRunning)
		run_update(s);
}

static void annotate_instruction(struct dataflow_state * s, struct rc_instruction * inst)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);
	unsigned int src;

	for(src = 0; src < opcode->NumSrcRegs; ++src) {
		struct rc_dataflow_vector * vector = get_register_contents(s, inst->I.SrcReg[src].File, inst->I.SrcReg[src].Index);
		if (vector) {
			inst->Dataflow.SrcReg[src] = rc_dataflow_create_ref(s->C, vector, inst);
		}
		if (inst->I.SrcReg[src].RelAddr) {
			struct rc_dataflow_vector * addr = get_register_contents(s, RC_FILE_ADDRESS, 0);
			if (addr)
				inst->Dataflow.SrcRegAddress[src] = rc_dataflow_create_ref(s->C, addr, inst);
		}
	}

	mark_instruction_source_use(s, inst, 0); /* for KIL */

	if (opcode->HasDstReg) {
		struct rc_dataflow_vector * oldvec = get_register_contents(s, inst->I.DstReg.File, inst->I.DstReg.Index);
		struct rc_dataflow_vector * newvec = rc_dataflow_create_vector(s->C, inst->I.DstReg.File, inst->I.DstReg.Index, inst);

		newvec->ValidMask = inst->I.DstReg.WriteMask;

		if (oldvec) {
			unsigned int carryover = oldvec->ValidMask & ~inst->I.DstReg.WriteMask;

			if (oldvec->ValidMask)
				inst->Dataflow.DstRegAliased = 1;

			if (carryover) {
				inst->Dataflow.DstRegPrev = rc_dataflow_create_ref(s->C, oldvec, inst);
				newvec->ValidMask |= carryover;

				if (!s->DCE)
					mark_ref_use(s, inst->Dataflow.DstRegPrev, carryover);
			}
		}

		inst->Dataflow.DstReg = newvec;

		if (newvec->File == RC_FILE_TEMPORARY)
			s->Temporary[newvec->Index] = newvec;
		else if (newvec->File == RC_FILE_OUTPUT)
			s->Output[newvec->Index] = newvec;
		else
			s->Address = newvec;

		if (!s->DCE)
			mark_vector_use(s, newvec, inst->I.DstReg.WriteMask);
	}
}

static void init_inputs(struct dataflow_state * s)
{
	unsigned int index;

	for(index = 0; index < 32; ++index) {
		if (s->C->Program.InputsRead & (1 << index)) {
			s->Input[index] = rc_dataflow_create_vector(s->C, RC_FILE_INPUT, index, 0);
			s->Input[index]->ValidMask = RC_MASK_XYZW;
		}
	}
}

static void mark_output_use(void * data, unsigned int index, unsigned int mask)
{
	struct dataflow_state * s = data;
	struct rc_dataflow_vector * vec = s->Output[index];

	if (vec)
		mark_vector_use(s, vec, mask);
}

void rc_dataflow_annotate(struct radeon_compiler * c, rc_dataflow_mark_outputs_fn dce, void * userdata)
{
	struct dataflow_state s;
	struct rc_instruction * inst;

	memset(&s, 0, sizeof(s));
	s.C = c;
	s.DCE = dce ? 1 : 0;

	init_inputs(&s);

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		annotate_instruction(&s, inst);
	}

	if (s.DCE) {
		dce(userdata, &s, &mark_output_use);

		for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
			const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);

			if (opcode->HasDstReg) {
				unsigned int redundant_writes = inst->I.DstReg.WriteMask & ~inst->Dataflow.DstReg->UseMask;

				inst->I.DstReg.WriteMask &= ~redundant_writes;

				if (!inst->I.DstReg.WriteMask) {
					struct rc_instruction * todelete = inst;
					inst = inst->Prev;
					rc_remove_instruction(todelete);
					continue;
				}
			}

			unsigned int srcmasks[3];
			compute_sources_for_writemask(inst, inst->I.DstReg.WriteMask, srcmasks);

			for(unsigned int src = 0; src < 3; ++src) {
				for(unsigned int chan = 0; chan < 4; ++chan) {
					if (!GET_BIT(srcmasks[src], chan))
						SET_SWZ(inst->I.SrcReg[src].Swizzle, chan, RC_SWIZZLE_UNUSED);
				}

				if (inst->Dataflow.SrcReg[src]) {
					if (!inst->Dataflow.SrcReg[src]->UseMask) {
						rc_dataflow_remove_ref(inst->Dataflow.SrcReg[src]);
						inst->Dataflow.SrcReg[src] = 0;
					}
				}

				if (inst->Dataflow.SrcRegAddress[src]) {
					if (!inst->Dataflow.SrcRegAddress[src]->UseMask) {
						rc_dataflow_remove_ref(inst->Dataflow.SrcRegAddress[src]);
						inst->Dataflow.SrcRegAddress[src] = 0;
					}
				}
			}
		}

		rc_calculate_inputs_outputs(c);
	}
}
