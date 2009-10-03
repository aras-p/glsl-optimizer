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


struct updatemask_state {
	unsigned char Output[RC_REGISTER_MAX_INDEX];
	unsigned char Temporary[RC_REGISTER_MAX_INDEX];
	unsigned char Address;
};

struct instruction_state {
	unsigned char WriteMask;
	unsigned char SrcReg[3];
};

struct branchinfo {
	unsigned int HaveElse:1;

	struct updatemask_state StoreEndif;
	struct updatemask_state StoreElse;
};

struct deadcode_state {
	struct radeon_compiler * C;
	struct instruction_state * Instructions;

	struct updatemask_state R;

	struct branchinfo * BranchStack;
	unsigned int BranchStackSize;
	unsigned int BranchStackReserved;
};


static void or_updatemasks(
	struct updatemask_state * dst,
	struct updatemask_state * a,
	struct updatemask_state * b)
{
	for(unsigned int i = 0; i < RC_REGISTER_MAX_INDEX; ++i) {
		dst->Output[i] = a->Output[i] | b->Output[i];
		dst->Temporary[i] = a->Temporary[i] | b->Temporary[i];
	}

	dst->Address = a->Address | b->Address;
}

static void push_branch(struct deadcode_state * s)
{
	if (s->BranchStackSize >= s->BranchStackReserved) {
		unsigned int new_reserve = 2 * s->BranchStackReserved;
		struct branchinfo * new_stack;

		if (!new_reserve)
			new_reserve = 4;

		new_stack = memory_pool_malloc(&s->C->Pool, new_reserve * sizeof(struct branchinfo));
		memcpy(new_stack, s->BranchStack, s->BranchStackSize * sizeof(struct branchinfo));

		s->BranchStack = new_stack;
		s->BranchStackReserved = new_reserve;
	}

	struct branchinfo * branch = &s->BranchStack[s->BranchStackSize++];
	branch->HaveElse = 0;
	memcpy(&branch->StoreEndif, &s->R, sizeof(s->R));
}

static unsigned char * get_used_ptr(struct deadcode_state *s, rc_register_file file, unsigned int index)
{
	if (file == RC_FILE_OUTPUT || file == RC_FILE_TEMPORARY) {
		if (index >= RC_REGISTER_MAX_INDEX) {
			rc_error(s->C, "%s: index %i is out of bounds for file %i\n", __FUNCTION__, index, file);
			return 0;
		}

		if (file == RC_FILE_OUTPUT)
			return &s->R.Output[index];
		else
			return &s->R.Temporary[index];
	} else if (file == RC_FILE_ADDRESS) {
		return &s->R.Address;
	}

	return 0;
}

static void mark_used(struct deadcode_state * s, rc_register_file file, unsigned int index, unsigned int mask)
{
	unsigned char * pused = get_used_ptr(s, file, index);
	if (pused)
		*pused |= mask;
}

static void update_instruction(struct deadcode_state * s, struct rc_instruction * inst)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);
	struct instruction_state * insts = &s->Instructions[inst->IP];
	unsigned int usedmask = 0;

	if (opcode->HasDstReg) {
		unsigned char * pused = get_used_ptr(s, inst->I.DstReg.File, inst->I.DstReg.Index);
		if (pused) {
			usedmask = *pused & inst->I.DstReg.WriteMask;
			*pused &= ~usedmask;
		}
	}

	insts->WriteMask |= usedmask;

	unsigned int srcmasks[3];
	rc_compute_sources_for_writemask(opcode, usedmask, srcmasks);

	for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
		unsigned int refmask = 0;
		unsigned int newsrcmask = srcmasks[src] & ~insts->SrcReg[src];
		insts->SrcReg[src] |= newsrcmask;

		for(unsigned int chan = 0; chan < 4; ++chan) {
			if (GET_BIT(newsrcmask, chan))
				refmask |= 1 << GET_SWZ(inst->I.SrcReg[src].Swizzle, chan);
		}

		/* get rid of spurious bits from ZERO, ONE, etc. swizzles */
		refmask &= RC_MASK_XYZW;

		if (!refmask)
			continue;

		mark_used(s, inst->I.SrcReg[src].File, inst->I.SrcReg[src].Index, refmask);

		if (inst->I.SrcReg[src].RelAddr)
			mark_used(s, RC_FILE_ADDRESS, 0, RC_MASK_X);
	}
}

static void mark_output_use(void * data, unsigned int index, unsigned int mask)
{
	struct deadcode_state * s = data;

	mark_used(s, RC_FILE_OUTPUT, index, mask);
}

void rc_dataflow_deadcode(struct radeon_compiler * c, rc_dataflow_mark_outputs_fn dce, void * userdata)
{
	struct deadcode_state s;
	unsigned int nr_instructions;

	memset(&s, 0, sizeof(s));
	s.C = c;

	nr_instructions = rc_recompute_ips(c);
	s.Instructions = memory_pool_malloc(&c->Pool, sizeof(struct instruction_state)*nr_instructions);
	memset(s.Instructions, 0, sizeof(struct instruction_state)*nr_instructions);

	dce(userdata, &s, &mark_output_use);

	for(struct rc_instruction * inst = c->Program.Instructions.Prev;
	    inst != &c->Program.Instructions;
	    inst = inst->Prev) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);

		if (opcode->IsControlFlow) {
			if (opcode->Opcode == RC_OPCODE_ENDIF) {
				push_branch(&s);
			} else {
				if (s.BranchStackSize) {
					struct branchinfo * branch = &s.BranchStack[s.BranchStackSize-1];

					if (opcode->Opcode == RC_OPCODE_IF) {
						or_updatemasks(&s.R,
								&s.R,
								branch->HaveElse ? &branch->StoreElse : &branch->StoreEndif);

						s.BranchStackSize--;
					} else if (opcode->Opcode == RC_OPCODE_ELSE) {
						if (branch->HaveElse) {
							rc_error(c, "%s: Multiple ELSE for one IF/ENDIF\n", __FUNCTION__);
						} else {
							memcpy(&branch->StoreElse, &s.R, sizeof(s.R));
							memcpy(&s.R, &branch->StoreEndif, sizeof(s.R));
							branch->HaveElse = 1;
						}
					} else {
						rc_error(c, "%s: Unhandled control flow instruction %s\n", __FUNCTION__, opcode->Name);
					}
				} else {
					rc_error(c, "%s: Unexpected control flow instruction\n", __FUNCTION__);
				}
			}
		}

		update_instruction(&s, inst);
	}

	unsigned int ip = 0;
	for(struct rc_instruction * inst = c->Program.Instructions.Next;
	    inst != &c->Program.Instructions;
	    inst = inst->Next, ++ip) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);

		if (opcode->HasDstReg) {
			if (s.Instructions[ip].WriteMask) {
				inst->I.DstReg.WriteMask = s.Instructions[ip].WriteMask;
			} else {
				struct rc_instruction * todelete = inst;
				inst = inst->Prev;
				rc_remove_instruction(todelete);
				continue;
			}
		}

		unsigned int srcmasks[3];
		rc_compute_sources_for_writemask(opcode, s.Instructions[ip].WriteMask, srcmasks);

		for(unsigned int src = 0; src < 3; ++src) {
			for(unsigned int chan = 0; chan < 4; ++chan) {
				if (!GET_BIT(srcmasks[src], chan))
					SET_SWZ(inst->I.SrcReg[src].Swizzle, chan, RC_SWIZZLE_UNUSED);
			}
		}
	}

	rc_calculate_inputs_outputs(c);
}
