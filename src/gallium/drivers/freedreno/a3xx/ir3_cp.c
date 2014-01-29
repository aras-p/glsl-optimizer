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

#include "ir3.h"

/*
 * Copy Propagate:
 *
 * TODO probably want some sort of visitor sort of interface to
 * avoid duplicating the same graph traversal logic everywhere..
 *
 */

static void block_cp(struct ir3_block *block);
static struct ir3_instruction * instr_cp(struct ir3_instruction *instr, bool keep);

static bool is_eligible_mov(struct ir3_instruction *instr)
{
	if ((instr->category == 1) &&
			(instr->cat1.src_type == instr->cat1.dst_type)) {
		struct ir3_register *src = instr->regs[1];
		if ((src->flags & IR3_REG_SSA) &&
				/* TODO: propagate abs/neg modifiers if possible */
				!(src->flags & (IR3_REG_ABS | IR3_REG_NEGATE)))
			return true;
	}
	return false;
}

static void walk_children(struct ir3_instruction *instr, bool keep)
{
	unsigned i;

	/* walk down the graph from each src: */
	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *src = instr->regs[i];
		if (src->flags & IR3_REG_SSA)
			src->instr = instr_cp(src->instr, keep);
	}
}

static struct ir3_instruction *
instr_cp_fanin(struct ir3_instruction *instr)
{
	unsigned i;

	/* we need to handle fanin specially, to detect cases
	 * when we need to keep a mov
	 */

	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *src = instr->regs[i];
		if (src->flags & IR3_REG_SSA) {
			struct ir3_instruction *cand =
					instr_cp(src->instr, false);

			/* if the candidate is a fanout, then keep
			 * the move.
			 *
			 * This is a bit, um, fragile, but it should
			 * catch the extra mov's that the front-end
			 * puts in for us already in these cases.
			 */
			if (is_meta(cand) && (cand->opc == OPC_META_FO))
				cand = instr_cp(src->instr, true);

			src->instr = cand;
		}
	}

	walk_children(instr, false);

	return instr;

}

static struct ir3_instruction *
instr_cp(struct ir3_instruction *instr, bool keep)
{
	/* if we've already visited this instruction, bail now: */
	if (ir3_instr_check_mark(instr))
		return instr;

	if (is_meta(instr) && (instr->opc == OPC_META_FI))
		return instr_cp_fanin(instr);

	if (is_eligible_mov(instr) && !keep) {
		struct ir3_register *src = instr->regs[1];
		return instr_cp(src->instr, false);
	}

	walk_children(instr, false);

	return instr;
}

static void block_cp(struct ir3_block *block)
{
	unsigned i, j;

	for (i = 0; i < block->noutputs; i++) {
		if (block->outputs[i]) {
			struct ir3_instruction *out =
					instr_cp(block->outputs[i], false);

			/* To deal with things like this:
			 *
			 *   43: MOV OUT[2], TEMP[5]
			 *   44: MOV OUT[0], TEMP[5]
			 *
			 * we need to ensure that no two outputs point to
			 * the same instruction
			 */
			for (j = 0; j < i; j++) {
				if (block->outputs[j] == out) {
					out = instr_cp(block->outputs[i], true);
					break;
				}
			}

			block->outputs[i] = out;
		}
	}
}

void ir3_block_cp(struct ir3_block *block)
{
	ir3_shader_clear_mark(block->shader);
	block_cp(block);
}
