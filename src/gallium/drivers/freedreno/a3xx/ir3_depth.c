/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include "util/u_math.h"

#include "ir3.h"

/*
 * Instruction Depth:
 *
 * Calculates weighted instruction depth, ie. the sum of # of needed
 * instructions plus delay slots back to original input (ie INPUT or
 * CONST).  That is to say, an instructions depth is:
 *
 *   depth(instr) {
 *     d = 0;
 *     // for each src register:
 *     foreach (src in instr->regs[1..n])
 *       d = max(d, delayslots(src->instr, n) + depth(src->instr));
 *     return d + 1;
 *   }
 *
 * After an instruction's depth is calculated, it is inserted into the
 * blocks depth sorted list, which is used by the scheduling pass.
 */

/* calculate required # of delay slots between the instruction that
 * assigns a value and the one that consumes
 */
int ir3_delayslots(struct ir3_instruction *assigner,
		struct ir3_instruction *consumer, unsigned n)
{
	/* worst case is cat1-3 (alu) -> cat4/5 needing 6 cycles, normal
	 * alu -> alu needs 3 cycles, cat4 -> alu and texture fetch
	 * handled with sync bits
	 */

	if (is_meta(assigner))
		return 0;

	/* handled via sync flags: */
	if (is_sfu(assigner) || is_tex(assigner))
		return 0;

	/* assigner must be alu: */
	if (is_flow(consumer) || is_sfu(consumer) || is_tex(consumer)) {
		return 6;
	} else if ((consumer->category == 3) &&
			is_mad(consumer->opc) && (n == 2)) {
		/* special case, 3rd src to cat3 not required on first cycle */
		return 1;
	} else {
		return 3;
	}
}

static void insert_by_depth(struct ir3_instruction *instr)
{
	struct ir3_block *block = instr->block;
	struct ir3_instruction *n = block->head;
	struct ir3_instruction *p = NULL;

	while (n && (n != instr) && (n->depth > instr->depth)) {
		p = n;
		n = n->next;
	}

	instr->next = n;
	if (p)
		p->next = instr;
	else
		block->head = instr;
}

static void ir3_instr_depth(struct ir3_instruction *instr)
{
	unsigned i;

	/* if we've already visited this instruction, bail now: */
	if (ir3_instr_check_mark(instr))
		return;

	instr->depth = 0;

	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *src = instr->regs[i];
		if (src->flags & IR3_REG_SSA) {
			unsigned sd;

			/* visit child to compute it's depth: */
			ir3_instr_depth(src->instr);

			sd = ir3_delayslots(src->instr, instr, i-1) +
					src->instr->depth;

			instr->depth = MAX2(instr->depth, sd);
		}
	}

	/* meta-instructions don't add cycles, other than PHI.. which
	 * might translate to a real instruction..
	 *
	 * well, not entirely true, fan-in/out, etc might need to need
	 * to generate some extra mov's in edge cases, etc.. probably
	 * we might want to do depth calculation considering the worst
	 * case for these??
	 */
	if (!is_meta(instr))
		instr->depth++;

	insert_by_depth(instr);
}

void ir3_block_depth(struct ir3_block *block)
{
	unsigned i;

	block->head = NULL;

	ir3_shader_clear_mark(block->shader);
	for (i = 0; i < block->noutputs; i++)
		if (block->outputs[i])
			ir3_instr_depth(block->outputs[i]);

	/* at this point, any unvisited input is unused: */
	for (i = 0; i < block->ninputs; i++) {
		struct ir3_instruction *in = block->inputs[i];
		if (in && !ir3_instr_check_mark(in))
			block->inputs[i] = NULL;
	}
}
