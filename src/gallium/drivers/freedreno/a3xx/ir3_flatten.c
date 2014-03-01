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

#include <stdarg.h>

#include "ir3.h"

/*
 * Flatten: flatten out legs of if/else, etc
 *
 * TODO probably should use some heuristic to decide to not flatten
 * if one side of the other is too large / deeply nested / whatever?
 */

struct ir3_flatten_ctx {
	struct ir3_block *block;
	unsigned cnt;
};

static struct ir3_register *unwrap(struct ir3_register *reg)
{

	if (reg->flags & IR3_REG_SSA) {
		struct ir3_instruction *instr = reg->instr;
		if (is_meta(instr)) {
			switch (instr->opc) {
			case OPC_META_OUTPUT:
			case OPC_META_FLOW:
				if (instr->regs_count > 1)
					return instr->regs[1];
				return NULL;
			default:
				break;
			}
		}
	}
	return reg;
}

static void ir3_instr_flatten(struct ir3_flatten_ctx *ctx,
		struct ir3_instruction *instr)
{
	unsigned i;

	/* if we've already visited this instruction, bail now: */
	if (ir3_instr_check_mark(instr))
		return;

	instr->block = ctx->block;

	/* TODO: maybe some threshold to decide whether to
	 * flatten or not??
	 */
	if (is_meta(instr)) {
		if (instr->opc == OPC_META_PHI) {
			struct ir3_register *cond, *t, *f;

			cond = unwrap(instr->regs[1]);
			t    = unwrap(instr->regs[2]);  /* true val */
			f    = unwrap(instr->regs[3]);  /* false val */

			/* must have cond, but t or f may be null if only written
			 * one one side of the if/else (in which case we can just
			 * convert the PHI to a simple move).
			 */
			assert(cond);
			assert(t || f);

			if (t && f) {
				/* convert the PHI instruction to sel.{b16,b32} */
				instr->category = 3;

				/* instruction type based on dst size: */
				if (instr->regs[0]->flags & IR3_REG_HALF)
					instr->opc = OPC_SEL_B16;
				else
					instr->opc = OPC_SEL_B32;

				instr->regs[1] = t;
				instr->regs[2] = cond;
				instr->regs[3] = f;
			} else {
				/* convert to simple mov: */
				instr->category = 1;
				instr->cat1.dst_type = TYPE_F32;
				instr->cat1.src_type = TYPE_F32;
				instr->regs_count = 2;
				instr->regs[1] = t ? t : f;
			}

			ctx->cnt++;
		} else if ((instr->opc == OPC_META_INPUT) &&
				(instr->regs_count == 2)) {
			type_t ftype;

			if (instr->regs[0]->flags & IR3_REG_HALF)
				ftype = TYPE_F16;
			else
				ftype = TYPE_F32;

			/* convert meta:input to mov: */
			instr->category = 1;
			instr->cat1.src_type = ftype;
			instr->cat1.dst_type = ftype;
		}
	}

	/* recursively visit children: */
	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *src = instr->regs[i];
		if (src->flags & IR3_REG_SSA)
			ir3_instr_flatten(ctx, src->instr);
	}
}

/* return >= 0 is # of phi's flattened, < 0 is error */
int ir3_block_flatten(struct ir3_block *block)
{
	struct ir3_flatten_ctx ctx = {
			.block = block,
	};
	unsigned i;

	ir3_shader_clear_mark(block->shader);
	for(i = 0; i < block->noutputs; i++)
		if (block->outputs[i])
			ir3_instr_flatten(&ctx, block->outputs[i]);

	return ctx.cnt;
}
