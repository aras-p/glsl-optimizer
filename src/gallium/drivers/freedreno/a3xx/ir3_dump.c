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

#define PTRID(x) ((unsigned long)(x))

struct ir3_dump_ctx {
	FILE *f;
	bool verbose;
};

static void dump_instr_name(struct ir3_dump_ctx *ctx,
		struct ir3_instruction *instr)
{
	/* for debugging: */
	if (ctx->verbose) {
#ifdef DEBUG
		fprintf(ctx->f, "%04u:", instr->serialno);
#endif
		fprintf(ctx->f, "%03u: ", instr->depth);
	}

	if (instr->flags & IR3_INSTR_SY)
		fprintf(ctx->f, "(sy)");
	if (instr->flags & IR3_INSTR_SS)
		fprintf(ctx->f, "(ss)");

	if (is_meta(instr)) {
		switch(instr->opc) {
		case OPC_META_PHI:
			fprintf(ctx->f, "&#934;");
			break;
		default:
			/* shouldn't hit here.. just for debugging: */
			switch (instr->opc) {
			case OPC_META_INPUT:  fprintf(ctx->f, "_meta:in");   break;
			case OPC_META_OUTPUT: fprintf(ctx->f, "_meta:out");  break;
			case OPC_META_FO:     fprintf(ctx->f, "_meta:fo");   break;
			case OPC_META_FI:     fprintf(ctx->f, "_meta:fi");   break;
			case OPC_META_FLOW:   fprintf(ctx->f, "_meta:flow"); break;
			case OPC_META_PHI:    fprintf(ctx->f, "_meta:phi");  break;

			default: fprintf(ctx->f, "_meta:%d", instr->opc); break;
			}
			break;
		}
	} else if (instr->category == 1) {
		static const char *type[] = {
				[TYPE_F16] = "f16",
				[TYPE_F32] = "f32",
				[TYPE_U16] = "u16",
				[TYPE_U32] = "u32",
				[TYPE_S16] = "s16",
				[TYPE_S32] = "s32",
				[TYPE_U8]  = "u8",
				[TYPE_S8]  = "s8",
		};
		if (instr->cat1.src_type == instr->cat1.dst_type)
			fprintf(ctx->f, "mov");
		else
			fprintf(ctx->f, "cov");
		fprintf(ctx->f, ".%s%s", type[instr->cat1.src_type], type[instr->cat1.dst_type]);
	} else {
		fprintf(ctx->f, "%s", ir3_instr_name(instr));
		if (instr->flags & IR3_INSTR_3D)
			fprintf(ctx->f, ".3d");
		if (instr->flags & IR3_INSTR_A)
			fprintf(ctx->f, ".a");
		if (instr->flags & IR3_INSTR_O)
			fprintf(ctx->f, ".o");
		if (instr->flags & IR3_INSTR_P)
			fprintf(ctx->f, ".p");
		if (instr->flags & IR3_INSTR_S)
			fprintf(ctx->f, ".s");
		if (instr->flags & IR3_INSTR_S2EN)
			fprintf(ctx->f, ".s2en");
	}
}

static void dump_reg_name(struct ir3_dump_ctx *ctx,
		struct ir3_register *reg)
{
	if ((reg->flags & IR3_REG_ABS) && (reg->flags & IR3_REG_NEGATE))
		fprintf(ctx->f, "(absneg)");
	else if (reg->flags & IR3_REG_NEGATE)
		fprintf(ctx->f, "(neg)");
	else if (reg->flags & IR3_REG_ABS)
		fprintf(ctx->f, "(abs)");

	if (reg->flags & IR3_REG_IMMED) {
		fprintf(ctx->f, "imm[%f,%d,0x%x]", reg->fim_val, reg->iim_val, reg->iim_val);
	} else if (reg->flags & IR3_REG_SSA) {
		if (ctx->verbose) {
			fprintf(ctx->f, "_[");
			dump_instr_name(ctx, reg->instr);
			fprintf(ctx->f, "]");
		}
	} else {
		if (reg->flags & IR3_REG_HALF)
			fprintf(ctx->f, "h");
		if (reg->flags & IR3_REG_CONST)
			fprintf(ctx->f, "c%u.%c", reg_num(reg), "xyzw"[reg_comp(reg)]);
		else
			fprintf(ctx->f, "r%u.%c", reg_num(reg), "xyzw"[reg_comp(reg)]);
	}
}

static void ir3_instr_dump(struct ir3_dump_ctx *ctx,
		struct ir3_instruction *instr);
static void ir3_block_dump(struct ir3_dump_ctx *ctx,
		struct ir3_block *block, const char *name);

static void dump_instr(struct ir3_dump_ctx *ctx,
		struct ir3_instruction *instr)
{
	/* if we've already visited this instruction, bail now: */
	if (ir3_instr_check_mark(instr))
		return;

	/* some meta-instructions need to be handled specially: */
	if (is_meta(instr)) {
		if ((instr->opc == OPC_META_FO) ||
				(instr->opc == OPC_META_FI)) {
			unsigned i;
			for (i = 1; i < instr->regs_count; i++) {
				struct ir3_register *reg = instr->regs[i];
				if (reg->flags & IR3_REG_SSA)
					dump_instr(ctx, reg->instr);
			}
		} else if (instr->opc == OPC_META_FLOW) {
			struct ir3_register *reg = instr->regs[1];
			ir3_block_dump(ctx, instr->flow.if_block, "if");
			if (instr->flow.else_block)
				ir3_block_dump(ctx, instr->flow.else_block, "else");
			if (reg->flags & IR3_REG_SSA)
				dump_instr(ctx, reg->instr);
		} else if (instr->opc == OPC_META_PHI) {
			/* treat like a normal instruction: */
			ir3_instr_dump(ctx, instr);
		}
	} else {
		ir3_instr_dump(ctx, instr);
	}
}

/* arrarraggh!  if link is to something outside of the current block, we
 * need to defer emitting the link until the end of the block, since the
 * edge triggers pre-creation of the node it links to inside the cluster,
 * even though it is meant to be outside..
 */
static struct {
	char buf[40960];
	unsigned n;
} edge_buf;

/* helper to print or defer: */
static void printdef(struct ir3_dump_ctx *ctx,
		bool defer, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (defer) {
		unsigned n = edge_buf.n;
		n += vsnprintf(&edge_buf.buf[n], sizeof(edge_buf.buf) - n,
				fmt, ap);
		edge_buf.n = n;
	} else {
		vfprintf(ctx->f, fmt, ap);
	}
	va_end(ap);
}

static void dump_link2(struct ir3_dump_ctx *ctx,
		struct ir3_instruction *instr, const char *target, bool defer)
{
	/* some meta-instructions need to be handled specially: */
	if (is_meta(instr)) {
		if (instr->opc == OPC_META_INPUT) {
			printdef(ctx, defer, "input%lx:<in%u>:w -> %s",
					PTRID(instr->inout.block),
					instr->regs[0]->num, target);
		} else if (instr->opc == OPC_META_FO) {
			struct ir3_register *reg = instr->regs[1];
			dump_link2(ctx, reg->instr, target, defer);
			printdef(ctx, defer, "[label=\".%c\"]",
					"xyzw"[instr->fo.off & 0x3]);
		} else if (instr->opc == OPC_META_FI) {
			unsigned i;

			/* recursively dump all parents and links */
			for (i = 1; i < instr->regs_count; i++) {
				struct ir3_register *reg = instr->regs[i];
				if (reg->flags & IR3_REG_SSA) {
					dump_link2(ctx, reg->instr, target, defer);
					printdef(ctx, defer, "[label=\".%c\"]",
							"xyzw"[(i - 1) & 0x3]);
				}
			}
		} else if (instr->opc == OPC_META_OUTPUT) {
			printdef(ctx, defer, "output%lx:<out%u>:w -> %s",
					PTRID(instr->inout.block),
					instr->regs[0]->num, target);
		} else if (instr->opc == OPC_META_PHI) {
			/* treat like a normal instruction: */
			printdef(ctx, defer, "instr%lx:<dst0> -> %s", PTRID(instr), target);
		}
	} else {
		printdef(ctx, defer, "instr%lx:<dst0> -> %s", PTRID(instr), target);
	}
}

static void dump_link(struct ir3_dump_ctx *ctx,
		struct ir3_instruction *instr,
		struct ir3_block *block, const char *target)
{
	bool defer = instr->block != block;
	dump_link2(ctx, instr, target, defer);
	printdef(ctx, defer, "\n");
}

static struct ir3_register *follow_flow(struct ir3_register *reg)
{
	if (reg->flags & IR3_REG_SSA) {
		struct ir3_instruction *instr = reg->instr;
		/* go with the flow.. */
		if (is_meta(instr) && (instr->opc == OPC_META_FLOW))
			return instr->regs[1];
	}
	return reg;
}

static void ir3_instr_dump(struct ir3_dump_ctx *ctx,
		struct ir3_instruction *instr)
{
	unsigned i;

	fprintf(ctx->f, "instr%lx [shape=record,style=filled,fillcolor=lightgrey,label=\"{",
			PTRID(instr));
	dump_instr_name(ctx, instr);

	/* destination register: */
	fprintf(ctx->f, "|<dst0>");

	/* source register(s): */
	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *reg = follow_flow(instr->regs[i]);

		fprintf(ctx->f, "|");

		if (reg->flags & IR3_REG_SSA)
			fprintf(ctx->f, "<src%u> ", (i - 1));

		dump_reg_name(ctx, reg);
	}

	fprintf(ctx->f, "}\"];\n");

	/* and recursively dump dependent instructions: */
	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		char target[32];  /* link target */

		if (!(reg->flags & IR3_REG_SSA))
			continue;

		snprintf(target, sizeof(target), "instr%lx:<src%u>",
				PTRID(instr), (i - 1));

		dump_instr(ctx, reg->instr);
		dump_link(ctx, follow_flow(reg)->instr, instr->block, target);
	}
}

static void ir3_block_dump(struct ir3_dump_ctx *ctx,
		struct ir3_block *block, const char *name)
{
	unsigned i, n;

	n = edge_buf.n;

	fprintf(ctx->f, "subgraph cluster%lx {\n", PTRID(block));
	fprintf(ctx->f, "label=\"%s\";\n", name);

	/* draw inputs: */
	fprintf(ctx->f, "input%lx [shape=record,label=\"inputs", PTRID(block));
	for (i = 0; i < block->ninputs; i++)
		if (block->inputs[i])
			fprintf(ctx->f, "|<in%u> i%u.%c", i, (i >> 2), "xyzw"[i & 0x3]);
	fprintf(ctx->f, "\"];\n");

	/* draw instruction graph: */
	for (i = 0; i < block->noutputs; i++)
		dump_instr(ctx, block->outputs[i]);

	/* draw outputs: */
	fprintf(ctx->f, "output%lx [shape=record,label=\"outputs", PTRID(block));
	for (i = 0; i < block->noutputs; i++)
		fprintf(ctx->f, "|<out%u> o%u.%c", i, (i >> 2), "xyzw"[i & 0x3]);
	fprintf(ctx->f, "\"];\n");

	/* and links to outputs: */
	for (i = 0; i < block->noutputs; i++) {
		char target[32];  /* link target */

		/* NOTE: there could be outputs that are never assigned,
		 * so skip them
		 */
		if (!block->outputs[i])
			continue;

		snprintf(target, sizeof(target), "output%lx:<out%u>:e",
				PTRID(block), i);

		dump_link(ctx, block->outputs[i], block, target);
	}

	fprintf(ctx->f, "}\n");

	/* and links to inputs: */
	if (block->parent) {
		for (i = 0; i < block->ninputs; i++) {
			char target[32];  /* link target */

			if (!block->inputs[i])
				continue;

			dump_instr(ctx, block->inputs[i]);

			snprintf(target, sizeof(target), "input%lx:<in%u>:e",
					PTRID(block), i);

			dump_link(ctx, block->inputs[i], block, target);
		}
	}

	/* dump deferred edges: */
	if (edge_buf.n > n) {
		fprintf(ctx->f, "%*s", edge_buf.n - n, &edge_buf.buf[n]);
		edge_buf.n = n;
	}
}

void ir3_shader_dump(struct ir3_shader *shader, const char *name,
		struct ir3_block *block /* XXX maybe 'block' ptr should move to ir3_shader? */,
		FILE *f)
{
	struct ir3_dump_ctx ctx = {
			.f = f,
	};
	ir3_shader_clear_mark(shader);
	fprintf(ctx.f, "digraph G {\n");
	fprintf(ctx.f, "rankdir=RL;\n");
	fprintf(ctx.f, "nodesep=0.25;\n");
	fprintf(ctx.f, "ranksep=1.5;\n");
	ir3_block_dump(&ctx, block, name);
	fprintf(ctx.f, "}\n");
}

/*
 * For Debugging:
 */

void
ir3_dump_instr_single(struct ir3_instruction *instr)
{
	struct ir3_dump_ctx ctx = {
			.f = stdout,
			.verbose = true,
	};
	unsigned i;

	dump_instr_name(&ctx, instr);
	for (i = 0; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		printf(i ? ", " : " ");
		dump_reg_name(&ctx, reg);
	}
	printf("\n");
}

void
ir3_dump_instr_list(struct ir3_instruction *instr)
{
	unsigned n = 0;

	while (instr) {
		ir3_dump_instr_single(instr);
		if (!is_meta(instr))
			n++;
		instr = instr->next;
	}
	printf("%u instructions\n", n);
}
