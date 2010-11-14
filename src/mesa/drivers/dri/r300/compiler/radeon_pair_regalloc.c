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

#include "radeon_program_pair.h"

#include <stdio.h>

#include "radeon_compiler.h"
#include "radeon_dataflow.h"


#define VERBOSE 0

#define DBG(...) do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while(0)


struct live_intervals {
	int Start;
	int End;
	struct live_intervals * Next;
};

struct register_info {
	struct live_intervals Live;

	unsigned int Used:1;
	unsigned int Allocated:1;
	unsigned int File:3;
	unsigned int Index:RC_REGISTER_INDEX_BITS;
};

struct hardware_register {
	struct live_intervals * Used;
};

struct regalloc_state {
	struct radeon_compiler * C;

	struct register_info Input[RC_REGISTER_MAX_INDEX];
	struct register_info Temporary[RC_REGISTER_MAX_INDEX];

	struct hardware_register * HwTemporary;
	unsigned int NumHwTemporaries;
	/**
	 * If an instruction is inside of a loop, EndLoop will be the
	 * IP of the ENDLOOP instruction, and BeginLoop will be the IP
	 * of the BGNLOOP instruction.  Otherwise, EndLoop and BeginLoop
	 * will be -1.
	 */
	int EndLoop;
	int BeginLoop;
};

static void print_live_intervals(struct live_intervals * src)
{
	if (!src) {
		DBG("(null)");
		return;
	}

	while(src) {
		DBG("(%i,%i)", src->Start, src->End);
		src = src->Next;
	}
}

static void add_live_intervals(struct regalloc_state * s,
		struct live_intervals ** dst, struct live_intervals * src)
{
	struct live_intervals ** dst_backup = dst;

	if (VERBOSE) {
		DBG("add_live_intervals: ");
		print_live_intervals(*dst);
		DBG(" to ");
		print_live_intervals(src);
		DBG("\n");
	}

	while(src) {
		if (*dst && (*dst)->End < src->Start) {
			dst = &(*dst)->Next;
		} else if (!*dst || (*dst)->Start > src->End) {
			struct live_intervals * li = memory_pool_malloc(&s->C->Pool, sizeof(*li));
			li->Start = src->Start;
			li->End = src->End;
			li->Next = *dst;
			*dst = li;
			src = src->Next;
		} else {
			if (src->End > (*dst)->End)
				(*dst)->End = src->End;
			if (src->Start < (*dst)->Start)
				(*dst)->Start = src->Start;
			src = src->Next;
		}
	}

	if (VERBOSE) {
		DBG("    result: ");
		print_live_intervals(*dst_backup);
		DBG("\n");
	}
}

static int overlap_live_intervals(struct live_intervals * dst, struct live_intervals * src)
{
	if (VERBOSE) {
		DBG("overlap_live_intervals: ");
		print_live_intervals(dst);
		DBG(" to ");
		print_live_intervals(src);
		DBG("\n");
	}

	while(src && dst) {
		if (dst->End <= src->Start) {
			dst = dst->Next;
		} else if (dst->End <= src->End) {
			DBG("    overlap\n");
			return 1;
		} else if (dst->Start < src->End) {
			DBG("    overlap\n");
			return 1;
		} else {
			src = src->Next;
		}
	}

	DBG("    no overlap\n");

	return 0;
}

static int try_add_live_intervals(struct regalloc_state * s,
		struct live_intervals ** dst, struct live_intervals * src)
{
	if (overlap_live_intervals(*dst, src))
		return 0;

	add_live_intervals(s, dst, src);
	return 1;
}

static void scan_callback(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct regalloc_state * s = data;
	struct register_info * reg;

	if (file == RC_FILE_TEMPORARY)
		reg = &s->Temporary[index];
	else if (file == RC_FILE_INPUT)
		reg = &s->Input[index];
	else
		return;

	if (!reg->Used) {
		reg->Used = 1;
		if (file == RC_FILE_INPUT)
			reg->Live.Start = -1;
		else if (s->BeginLoop >= 0)
			reg->Live.Start = s->BeginLoop;
		else
			reg->Live.Start = inst->IP;
		reg->Live.End = inst->IP;
	} else if (s->EndLoop >= 0)
		reg->Live.End = s->EndLoop;
	else if (inst->IP > reg->Live.End)
		reg->Live.End = inst->IP;
}

static void compute_live_intervals(struct radeon_compiler *c,
				   struct regalloc_state *s)
{
	memset(s, 0, sizeof(*s));
	s->C = c;
	s->NumHwTemporaries = c->max_temp_regs;
	s->BeginLoop = -1;
	s->EndLoop = -1;
	s->HwTemporary =
		memory_pool_malloc(&c->Pool,
				   s->NumHwTemporaries * sizeof(struct hardware_register));
	memset(s->HwTemporary, 0, s->NumHwTemporaries * sizeof(struct hardware_register));

	rc_recompute_ips(s->C);

	for(struct rc_instruction * inst = s->C->Program.Instructions.Next;
	    inst != &s->C->Program.Instructions;
	    inst = inst->Next) {

		/* For all instructions inside of a loop, the ENDLOOP
		 * instruction is used as the end of the live interval and
		 * the BGNLOOP instruction is used as the beginning. */
		if (inst->U.I.Opcode == RC_OPCODE_BGNLOOP && s->EndLoop < 0) {
			s->BeginLoop = inst->IP;
			int loops = 1;
			struct rc_instruction * tmp;
			for(tmp = inst->Next;
					tmp != &s->C->Program.Instructions;
					tmp = tmp->Next) {
				if (tmp->U.I.Opcode == RC_OPCODE_BGNLOOP) {
					loops++;
				} else if (tmp->U.I.Opcode
							== RC_OPCODE_ENDLOOP) {
					if(!--loops) {
						s->EndLoop = tmp->IP;
						break;
					}
				}
			}
		}

		if (inst->IP == s->EndLoop) {
			s->EndLoop = -1;
			s->BeginLoop = -1;
		}

		rc_for_all_reads_mask(inst, scan_callback, s);
		rc_for_all_writes_mask(inst, scan_callback, s);
	}
}

static void remap_register(void * data, struct rc_instruction * inst,
		rc_register_file * file, unsigned int * index)
{
	struct regalloc_state * s = data;
	const struct register_info * reg;

	if (*file == RC_FILE_TEMPORARY)
		reg = &s->Temporary[*index];
	else if (*file == RC_FILE_INPUT)
		reg = &s->Input[*index];
	else
		return;

	if (reg->Allocated) {
		*file = reg->File;
		*index = reg->Index;
	}
}

static void do_regalloc(struct regalloc_state * s)
{
	/* Simple and stupid greedy register allocation */
	for(unsigned int index = 0; index < RC_REGISTER_MAX_INDEX; ++index) {
		struct register_info * reg = &s->Temporary[index];

		if (!reg->Used)
			continue;

		for(unsigned int hwreg = 0; hwreg < s->NumHwTemporaries; ++hwreg) {
			if (try_add_live_intervals(s, &s->HwTemporary[hwreg].Used, &reg->Live)) {
				reg->Allocated = 1;
				reg->File = RC_FILE_TEMPORARY;
				reg->Index = hwreg;
				goto success;
			}
		}

		rc_error(s->C, "Ran out of hardware temporaries\n");
		return;

	success:;
	}

	/* Rewrite all instructions based on the translation table we built */
	for(struct rc_instruction * inst = s->C->Program.Instructions.Next;
	    inst != &s->C->Program.Instructions;
	    inst = inst->Next) {
		rc_remap_registers(inst, &remap_register, s);
	}
}

static void alloc_input(void * data, unsigned int input, unsigned int hwreg)
{
	struct regalloc_state * s = data;

	if (!s->Input[input].Used)
		return;

	add_live_intervals(s, &s->HwTemporary[hwreg].Used, &s->Input[input].Live);

	s->Input[input].Allocated = 1;
	s->Input[input].File = RC_FILE_TEMPORARY;
	s->Input[input].Index = hwreg;

}

void rc_pair_regalloc(struct radeon_compiler *cc, void *user)
{
	struct r300_fragment_program_compiler *c = (struct r300_fragment_program_compiler*)cc;
	struct regalloc_state s;

	compute_live_intervals(cc, &s);

	c->AllocateHwInputs(c, &alloc_input, &s);

	do_regalloc(&s);
}

/* This functions offsets the temporary register indices by the number
 * of input registers, because input registers are actually temporaries and
 * should not occupy the same space.
 *
 * This pass is supposed to be used to maintain correct allocation of inputs
 * if the standard register allocation is disabled. */
void rc_pair_regalloc_inputs_only(struct radeon_compiler *cc, void *user)
{
	struct r300_fragment_program_compiler *c = (struct r300_fragment_program_compiler*)cc;
	struct regalloc_state s;
	int temp_reg_offset;

	compute_live_intervals(cc, &s);

	c->AllocateHwInputs(c, &alloc_input, &s);

	temp_reg_offset = 0;
	for (unsigned i = 0; i < RC_REGISTER_MAX_INDEX; i++) {
		if (s.Input[i].Allocated && temp_reg_offset <= s.Input[i].Index)
			temp_reg_offset = s.Input[i].Index + 1;
	}

	if (temp_reg_offset) {
		for (unsigned i = 0; i < RC_REGISTER_MAX_INDEX; i++) {
			if (s.Temporary[i].Used) {
				s.Temporary[i].Allocated = 1;
				s.Temporary[i].File = RC_FILE_TEMPORARY;
				s.Temporary[i].Index = i + temp_reg_offset;
			}
		}

		/* Rewrite all registers. */
		for (struct rc_instruction *inst = cc->Program.Instructions.Next;
		    inst != &cc->Program.Instructions;
		    inst = inst->Next) {
			rc_remap_registers(inst, &remap_register, &s);
		}
	}
}
