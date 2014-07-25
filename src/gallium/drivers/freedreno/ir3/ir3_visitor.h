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

#ifndef IR3_VISITOR_H_
#define IR3_VISITOR_H_

/**
 * Visitor which follows dst to src relationships between instructions,
 * first visiting the dst (writer) instruction, followed by src (reader)
 * instruction(s).
 *
 * TODO maybe we want multiple different visitors to walk the
 * graph in different ways?
 */

struct ir3_visitor;

typedef void (*ir3_visit_instr_func)(struct ir3_visitor *v,
		struct ir3_instruction *instr);

typedef void (*ir3_visit_reg_func)(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg);

struct ir3_visitor_funcs {
	ir3_visit_instr_func instr;  // TODO do we need??

	ir3_visit_reg_func dst_shader_input;
	ir3_visit_reg_func dst_block_input;
	ir3_visit_reg_func dst_fanout;
	ir3_visit_reg_func dst_fanin;
	ir3_visit_reg_func dst;

	ir3_visit_reg_func src_block_input;
	ir3_visit_reg_func src_fanout;
	ir3_visit_reg_func src_fanin;
	ir3_visit_reg_func src;
};

struct ir3_visitor {
	const struct ir3_visitor_funcs *funcs;
	bool error;
};

#include "util/u_debug.h"

static void visit_instr_dst(struct ir3_visitor *v,
		struct ir3_instruction *instr)
{
	struct ir3_register *reg = instr->regs[0];

	if (is_meta(instr)) {
		switch (instr->opc) {
		case OPC_META_INPUT:
			if (instr->regs_count == 1)
				v->funcs->dst_shader_input(v, instr, reg);
			else
				v->funcs->dst_block_input(v, instr, reg);
			return;
		case OPC_META_FO:
			v->funcs->dst_fanout(v, instr, reg);
			return;
		case OPC_META_FI:
			v->funcs->dst_fanin(v, instr, reg);
			return;
		default:
			break;

		}
	}

	v->funcs->dst(v, instr, reg);
}

static void visit_instr_src(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	if (is_meta(instr)) {
		switch (instr->opc) {
		case OPC_META_INPUT:
			/* shader-input does not have a src, only block input: */
			debug_assert(instr->regs_count == 2);
			v->funcs->src_block_input(v, instr, reg);
			return;
		case OPC_META_FO:
			v->funcs->src_fanout(v, instr, reg);
			return;
		case OPC_META_FI:
			v->funcs->src_fanin(v, instr, reg);
			return;
		default:
			break;

		}
	}

	v->funcs->src(v, instr, reg);
}

static void ir3_visit_instr(struct ir3_visitor *v,
		struct ir3_instruction *instr)
{
	struct ir3_instruction *n;

	/* visit instruction that assigns value: */
	if (instr->regs_count > 0)
		visit_instr_dst(v, instr);

	/* and of any following instructions which read that value: */
	n = instr->next;
	while (n && !v->error) {
		unsigned i;

		for (i = 1; i < n->regs_count; i++) {
			struct ir3_register *reg = n->regs[i];
			if ((reg->flags & IR3_REG_SSA) && (reg->instr == instr))
				visit_instr_src(v, n, reg);
		}

		n = n->next;
	}
}

static void ir3_visit_reg(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	/* no-op */
}

#endif /* IR3_VISITOR_H_ */
