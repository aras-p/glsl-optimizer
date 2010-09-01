/*
 * Copyright 2010 Tom Stellard <tstellar@gmail.com>
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
 * \file
 */

#include "radeon_rename_regs.h"

#include "radeon_compiler.h"
#include "radeon_dataflow.h"

struct reg_rename {
	int old_index;
	int new_index;
	int temp_index;
};

static void rename_reg(void * data, struct rc_instruction * inst,
			rc_register_file * file, unsigned int * index)
{
	struct reg_rename *r = data;

	if(r->old_index == *index && *file == RC_FILE_TEMPORARY) {
		*index = r->new_index;
	}
	else if(r->new_index == *index && *file == RC_FILE_TEMPORARY) {
		*index = r->temp_index;
	}
}

static void rename_all(
	struct radeon_compiler *c,
	struct rc_instruction * start,
	unsigned int old,
	unsigned int new,
	unsigned int temp)
{
	struct rc_instruction * inst;
	struct reg_rename r;
	r.old_index = old;
	r.new_index = new;
	r.temp_index = temp;
	for(inst = start; inst != &c->Program.Instructions;
						inst = inst->Next) {
		rc_remap_registers(inst, rename_reg, &r);
	}
}

/**
 * This function renames registers in an attempt to get the code close to
 * SSA form.  After this function has completed, most of the register are only
 * written to one time, with a few exceptions.  For example, this block of code
 * will not be modified by this function:
 * Mov Temp[0].x Const[0].x
 * Mov Temp[0].y Const[0].y
 * Basically, destination registers will be renamed if:
 * 1. There have been no previous writes to that register
 * or
 * 2. If the instruction is writting to the exact components (no more, no less)
 * of a register that has been written to by previous instructions.
 *
 * This function assumes all the instructions are still of type
 * RC_INSTRUCTION_NORMAL.
 */
void rc_rename_regs(struct radeon_compiler *c, void *user)
{
	unsigned int cur_index = 0;
	unsigned int icount;
	struct rc_instruction * inst;
	unsigned int * masks;

	/* The number of instructions in the program is also the maximum
	 * number of temp registers that could potentially be used. */
	icount = rc_recompute_ips(c);
	masks = memory_pool_malloc(&c->Pool, icount * sizeof(unsigned int));
	memset(masks, 0, icount * sizeof(unsigned int));

	for(inst = c->Program.Instructions.Next;
					inst != &c->Program.Instructions;
					inst = inst->Next) {
		const struct rc_opcode_info * info;
		if(inst->Type != RC_INSTRUCTION_NORMAL) {
			rc_error(c, "%s only works with normal instructions.",
								__FUNCTION__);
			return;
		}
		unsigned int old_index, temp_index;
		struct rc_dst_register * dst = &inst->U.I.DstReg;
		info = rc_get_opcode_info(inst->U.I.Opcode);
		if(!info->HasDstReg || dst->File != RC_FILE_TEMPORARY) {
			continue;
		}
		if(dst->Index >= icount || !masks[dst->Index] ||
					masks[dst->Index] == dst->WriteMask) {
			old_index = dst->Index;
			/* We need to set dst->Index here so get free temporary
			 * will work. */
			dst->Index = cur_index++;
			temp_index = rc_find_free_temporary(c);
			rename_all(c, inst->Next, old_index,
						dst->Index, temp_index);
		}
		assert(dst->Index < icount);
		masks[dst->Index] |= dst->WriteMask;
	}
}
