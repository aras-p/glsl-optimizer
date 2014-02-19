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
 * Instruction Scheduling:
 *
 * Using the depth sorted list from depth pass, attempt to recursively
 * schedule deepest unscheduled path.  The first instruction that cannot
 * be scheduled, returns the required delay slots it needs, at which
 * point we return back up to the top and attempt to schedule by next
 * highest depth.  After a sufficient number of instructions have been
 * scheduled, return back to beginning of list and start again.  If you
 * reach the end of depth sorted list without being able to insert any
 * instruction, insert nop's.  Repeat until no more unscheduled
 * instructions.
 */

struct ir3_sched_ctx {
	struct ir3_instruction *scheduled;
	unsigned cnt;
};

static struct ir3_instruction *
deepest(struct ir3_instruction **srcs, unsigned nsrcs)
{
	struct ir3_instruction *d = NULL;
	unsigned i = 0, id = 0;

	while ((i < nsrcs) && !(d = srcs[id = i]))
		i++;

	if (!d)
		return NULL;

	for (; i < nsrcs; i++)
		if (srcs[i] && (srcs[i]->depth > d->depth))
			d = srcs[id = i];

	srcs[id] = NULL;

	return d;
}

static unsigned distance(struct ir3_sched_ctx *ctx,
		struct ir3_instruction *instr, unsigned maxd)
{
	struct ir3_instruction *n = ctx->scheduled;
	unsigned d = 0;
	while (n && (n != instr) && (d < maxd)) {
		if (is_alu(n) || is_flow(n))
			d++;
		n = n->next;
	}
	return d;
}

/* TODO maybe we want double linked list? */
static struct ir3_instruction * prev(struct ir3_instruction *instr)
{
	struct ir3_instruction *p = instr->block->head;
	while (p && (p->next != instr))
		p = p->next;
	return p;
}

static void schedule(struct ir3_sched_ctx *ctx,
		struct ir3_instruction *instr, bool remove)
{
	struct ir3_block *block = instr->block;

	/* maybe there is a better way to handle this than just stuffing
	 * a nop.. ideally we'd know about this constraint in the
	 * scheduling and depth calculation..
	 */
	if (ctx->scheduled && is_sfu(ctx->scheduled) && is_sfu(instr))
		schedule(ctx, ir3_instr_create(block, 0, OPC_NOP), false);

	/* remove from depth list:
	 */
	if (remove) {
		struct ir3_instruction *p = prev(instr);

		/* NOTE: this can happen for inputs which are not
		 * read.. in that case there is no need to schedule
		 * the input, so just bail:
		 */
		if (instr != (p ? p->next : block->head))
			return;

		if (p)
			p->next = instr->next;
		else
			block->head = instr->next;
	}

	instr->flags |= IR3_INSTR_MARK;

	instr->next = ctx->scheduled;
	ctx->scheduled = instr;

	ctx->cnt++;
}

/*
 * Delay-slot calculation.  Follows fanin/fanout.
 */

static unsigned delay_calc2(struct ir3_sched_ctx *ctx,
		struct ir3_instruction *assigner,
		struct ir3_instruction *consumer, unsigned srcn)
{
	unsigned delay = 0;

	if (is_meta(assigner)) {
		unsigned i;
		for (i = 1; i < assigner->regs_count; i++) {
			struct ir3_register *reg = assigner->regs[i];
			if (reg->flags & IR3_REG_SSA) {
				unsigned d = delay_calc2(ctx, reg->instr,
						consumer, srcn);
				delay = MAX2(delay, d);
			}
		}
	} else {
		delay = ir3_delayslots(assigner, consumer, srcn);
		delay -= distance(ctx, assigner, delay);
	}

	return delay;
}

static unsigned delay_calc(struct ir3_sched_ctx *ctx,
		struct ir3_instruction *instr)
{
	unsigned i, delay = 0;

	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		if (reg->flags & IR3_REG_SSA) {
			unsigned d = delay_calc2(ctx, reg->instr,
					instr, i - 1);
			delay = MAX2(delay, d);
		}
	}

	return delay;
}

/* A negative return value signals that an instruction has been newly
 * scheduled, return back up to the top of the stack (to block_sched())
 */
static int trysched(struct ir3_sched_ctx *ctx,
		struct ir3_instruction *instr)
{
	struct ir3_instruction *srcs[ARRAY_SIZE(instr->regs) - 1];
	struct ir3_instruction *src;
	unsigned i, delay, nsrcs = 0;

	/* if already scheduled: */
	if (instr->flags & IR3_INSTR_MARK)
		return 0;

	/* figure out our src's: */
	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		if (reg->flags & IR3_REG_SSA)
			srcs[nsrcs++] = reg->instr;
	}

	/* for each src register in sorted order:
	 */
	delay = 0;
	while ((src = deepest(srcs, nsrcs))) {
		delay = trysched(ctx, src);
		if (delay)
			return delay;
	}

	/* all our dependents are scheduled, figure out if
	 * we have enough delay slots to schedule ourself:
	 */
	delay = delay_calc(ctx, instr);

	if (!delay) {
		schedule(ctx, instr, true);
		return -1;
	}

	return delay;
}

static struct ir3_instruction * reverse(struct ir3_instruction *instr)
{
	struct ir3_instruction *reversed = NULL;
	while (instr) {
		struct ir3_instruction *next = instr->next;
		instr->next = reversed;
		reversed = instr;
		instr = next;
	}
	return reversed;
}

static void block_sched(struct ir3_sched_ctx *ctx, struct ir3_block *block)
{
	struct ir3_instruction *instr;

	/* schedule all the shader input's (meta-instr) first so that
	 * the RA step sees that the input registers contain a value
	 * from the start of the shader:
	 */
	if (!block->parent) {
		unsigned i;
		for (i = 0; i < block->ninputs; i++) {
			struct ir3_instruction *in = block->inputs[i];
			if (in)
				schedule(ctx, in, true);
		}
	}

	while ((instr = block->head)) {
		/* NOTE: always grab next *before* trysched(), in case the
		 * instruction is actually scheduled (and therefore moved
		 * from depth list into scheduled list)
		 */
		struct ir3_instruction *next = instr->next;
		int cnt = trysched(ctx, instr);
		/* -1 is signal to return up stack, but to us means same as 0: */
		cnt = MAX2(0, cnt);
		cnt += ctx->cnt;
		instr = next;

		/* if deepest remaining instruction cannot be scheduled, try
		 * the increasingly more shallow instructions until needed
		 * number of delay slots is filled:
		 */
		while (instr && (cnt > ctx->cnt)) {
			next = instr->next;
			trysched(ctx, instr);
			instr = next;
		}

		/* and if we run out of instructions that can be scheduled,
		 * then it is time for nop's:
		 */
		while (cnt > ctx->cnt)
			schedule(ctx, ir3_instr_create(block, 0, OPC_NOP), false);
	}

	/* at this point, scheduled list is in reverse order, so fix that: */
	block->head = reverse(ctx->scheduled);
}

void ir3_block_sched(struct ir3_block *block)
{
	struct ir3_sched_ctx ctx = {0};
	ir3_shader_clear_mark(block->shader);
	block_sched(&ctx, block);
}
