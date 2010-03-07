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

#include "radeon_program.h"

#include <stdio.h>

#include "radeon_compiler.h"


/**
 * Transform the given clause in the following way:
 *  1. Replace it with an empty clause
 *  2. For every instruction in the original clause, try the given
 *     transformations in order.
 *  3. If one of the transformations returns GL_TRUE, assume that it
 *     has emitted the appropriate instruction(s) into the new clause;
 *     otherwise, copy the instruction verbatim.
 *
 * \note The transformation is currently not recursive; in other words,
 * instructions emitted by transformations are not transformed.
 *
 * \note The transform is called 'local' because it can only look at
 * one instruction at a time.
 */
void radeonLocalTransform(
	struct radeon_compiler * c,
	int num_transformations,
	struct radeon_program_transformation* transformations)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;

	while(inst != &c->Program.Instructions) {
		struct rc_instruction * current = inst;
		int i;

		inst = inst->Next;

		for(i = 0; i < num_transformations; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(c, current, t->userData))
				break;
		}
	}
}

/**
 * Left multiplication of a register with a swizzle
 */
struct rc_src_register lmul_swizzle(unsigned int swizzle, struct rc_src_register srcreg)
{
	struct rc_src_register tmp = srcreg;
	int i;
	tmp.Swizzle = 0;
	tmp.Negate = 0;
	for(i = 0; i < 4; ++i) {
		rc_swizzle swz = GET_SWZ(swizzle, i);
		if (swz < 4) {
			tmp.Swizzle |= GET_SWZ(srcreg.Swizzle, swz) << (i*3);
			tmp.Negate |= GET_BIT(srcreg.Negate, swz) << i;
		} else {
			tmp.Swizzle |= swz << (i*3);
		}
	}
	return tmp;
}

unsigned int rc_find_free_temporary(struct radeon_compiler * c)
{
	char used[RC_REGISTER_MAX_INDEX];
	unsigned int i;
	struct rc_instruction * rcinst;

	memset(used, 0, sizeof(used));

	for (rcinst = c->Program.Instructions.Next; rcinst != &c->Program.Instructions; rcinst = rcinst->Next) {
		const struct rc_sub_instruction *inst = &rcinst->U.I;
		const struct rc_opcode_info *opcode = rc_get_opcode_info(inst->Opcode);
		unsigned int k;

		for (k = 0; k < opcode->NumSrcRegs; k++) {
			if (inst->SrcReg[k].File == RC_FILE_TEMPORARY)
				used[inst->SrcReg[k].Index] = 1;
		}

		if (opcode->HasDstReg) {
			if (inst->DstReg.File == RC_FILE_TEMPORARY)
				used[inst->DstReg.Index] = 1;
		}
	}

	for (i = 0; i < RC_REGISTER_MAX_INDEX; i++) {
		if (!used[i])
			return i;
	}

	rc_error(c, "Ran out of temporary registers\n");
	return 0;
}


struct rc_instruction *rc_alloc_instruction(struct radeon_compiler * c)
{
	struct rc_instruction * inst = memory_pool_malloc(&c->Pool, sizeof(struct rc_instruction));

	memset(inst, 0, sizeof(struct rc_instruction));

	inst->U.I.Opcode = RC_OPCODE_ILLEGAL_OPCODE;
	inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;
	inst->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_XYZW;
	inst->U.I.SrcReg[2].Swizzle = RC_SWIZZLE_XYZW;

	return inst;
}

void rc_insert_instruction(struct rc_instruction * after, struct rc_instruction * inst)
{
	inst->Prev = after;
	inst->Next = after->Next;

	inst->Prev->Next = inst;
	inst->Next->Prev = inst;
}

struct rc_instruction *rc_insert_new_instruction(struct radeon_compiler * c, struct rc_instruction * after)
{
	struct rc_instruction * inst = rc_alloc_instruction(c);

	rc_insert_instruction(after, inst);

	return inst;
}

void rc_remove_instruction(struct rc_instruction * inst)
{
	inst->Prev->Next = inst->Next;
	inst->Next->Prev = inst->Prev;
}

/**
 * Return the number of instructions in the program.
 */
unsigned int rc_recompute_ips(struct radeon_compiler * c)
{
	unsigned int ip = 0;
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next;
	    inst != &c->Program.Instructions;
	    inst = inst->Next) {
		inst->IP = ip++;
	}

	c->Program.Instructions.IP = 0xcafedead;

	return ip;
}
