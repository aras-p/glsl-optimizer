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

#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"

#include "ir3.h"
#include "ir3_visitor.h"

/*
 * Register Assignment:
 *
 * NOTE: currently only works on a single basic block.. need to think
 * about how multiple basic blocks are going to get scheduled.  But
 * I think I want to re-arrange how blocks work, ie. get rid of the
 * block nesting thing..
 *
 * NOTE: we could do register coalescing (eliminate moves) as part of
 * the RA step.. OTOH I think we need to do scheduling before register
 * assignment.  And if we remove a mov that effects scheduling (unless
 * we leave a placeholder nop, which seems lame), so I'm not really
 * sure how practical this is to do both in a single stage.  But OTOH
 * I'm not really sure a sane way for the CP stage to realize when it
 * cannot remove a mov due to multi-register constraints..
 *
 */

struct ir3_ra_ctx {
	struct ir3_block *block;
	enum shader_t type;
	bool half_precision;
	bool frag_coord;
	bool frag_face;
	bool has_samp;
	int cnt;
	bool error;
};

/* sorta ugly way to retrofit half-precision support.. rather than
 * passing extra param around, just OR in a high bit.  All the low
 * value arithmetic (ie. +/- offset within a contiguous vec4, etc)
 * will continue to work as long as you don't underflow (and that
 * would go badly anyways).
 */
#define REG_HALF  0x8000

struct ir3_ra_assignment {
	int8_t  off;        /* offset of instruction dst within range */
	uint8_t num;        /* number of components for the range */
};

static void ra_assign(struct ir3_ra_ctx *ctx,
		struct ir3_instruction *assigner, int num);
static struct ir3_ra_assignment ra_calc(struct ir3_instruction *instr);

/*
 * Register Allocation:
 */

#define REG(n, wm) (struct ir3_register){ \
		/*.flags  = ((so)->half_precision) ? IR3_REG_HALF : 0,*/ \
		.num    = (n), \
		.wrmask = TGSI_WRITEMASK_ ## wm, \
	}

/* check that the register exists, is a GPR and is not special (a0/p0) */
static struct ir3_register * reg_check(struct ir3_instruction *instr, unsigned n)
{
	if ((n < instr->regs_count) && reg_gpr(instr->regs[n]))
		return instr->regs[n];
	return NULL;
}

static int output_base(struct ir3_ra_ctx *ctx)
{
	/* ugg, for fragment shader we need to have input at r0.x
	 * (or at least if there is a way to configure it, I can't
	 * see how because the blob driver always uses r0.x (ie.
	 * all zeros)
	 */
	if (ctx->type == SHADER_FRAGMENT) {
		if (ctx->half_precision)
			return ctx->frag_face ? 4 : 3;
		return ctx->frag_coord ? 8 : 4;
	}
	return 0;
}

/* live means read before written */
static void compute_liveregs(struct ir3_ra_ctx *ctx,
		struct ir3_instruction *instr, regmask_t *liveregs)
{
	struct ir3_block *block = instr->block;
	regmask_t written;
	unsigned i, j;

	regmask_init(liveregs);
	regmask_init(&written);

	for (instr = instr->next; instr; instr = instr->next) {
		struct ir3_register *r;

		if (is_meta(instr))
			continue;

		/* check first src's read: */
		for (j = 1; j < instr->regs_count; j++) {
			r = reg_check(instr, j);
			if (r)
				regmask_set_if_not(liveregs, r, &written);
		}

		/* then dst written (if assigned already): */
		if (instr->flags & IR3_INSTR_MARK) {
			r = reg_check(instr, 0);
			if (r)
				regmask_set(&written, r);
		}
	}

	/* be sure to account for output registers too: */
	for (i = 0; i < block->noutputs; i++) {
		struct ir3_register reg = REG(output_base(ctx) + i, X);
		regmask_set_if_not(liveregs, &reg, &written);
	}
}

/* calculate registers that are clobbered before last use of 'assigner'.
 * This needs to be done backwards, although it could possibly be
 * combined into compute_liveregs().  (Ie. compute_liveregs() could
 * reverse the list, then do this part backwards reversing the list
 * again back to original order.)  Otoh, probably I should try to
 * construct a proper interference graph instead.
 *
 * XXX this need to follow the same recursion path that is used for
 * to rename/assign registers (ie. ra_assign_src()).. this is a bit
 * ugly right now, maybe refactor into node iterator sort of things
 * that iterates nodes in the correct order?
 */
static bool compute_clobbers(struct ir3_ra_ctx *ctx,
		struct ir3_instruction *instr, struct ir3_instruction *assigner,
		regmask_t *liveregs)
{
	unsigned i;
	bool live = false, was_live = false;

	if (instr == NULL) {
		struct ir3_block *block = ctx->block;

		/* if at the end, check outputs: */
		for (i = 0; i < block->noutputs; i++)
			if (block->outputs[i] == assigner)
				return true;
		return false;
	}

	for (i = 1; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		if ((reg->flags & IR3_REG_SSA) && (reg->instr == assigner)) {
			if (is_meta(instr)) {
				switch (instr->opc) {
				case OPC_META_INPUT:
					// TODO
					assert(0);
					break;
				case OPC_META_FO:
				case OPC_META_FI:
					was_live |= compute_clobbers(ctx, instr->next,
							instr, liveregs);
					break;
				default:
					break;
				}
			}
			live = true;
			break;
		}
	}

	was_live |= compute_clobbers(ctx, instr->next, assigner, liveregs);

	if (was_live && (instr->regs_count > 0) &&
			(instr->flags & IR3_INSTR_MARK) &&
			!is_meta(instr))
		regmask_set(liveregs, instr->regs[0]);

	return live || was_live;
}

static int find_available(regmask_t *liveregs, int size)
{
	unsigned i;
	for (i = 0; i < MAX_REG - size; i++) {
		if (!regmask_get(liveregs, &REG(i, X))) {
			unsigned start = i++;
			for (; (i < MAX_REG) && ((i - start) < size); i++)
				if (regmask_get(liveregs, &REG(i, X)))
					break;
			if ((i - start) >= size)
				return start;
		}
	}
	assert(0);
	return -1;
}

static int alloc_block(struct ir3_ra_ctx *ctx,
		struct ir3_instruction *instr, int size)
{
	if (!instr) {
		/* special case, allocating shader outputs.  At this
		 * point, nothing is allocated, just start the shader
		 * outputs at r0.x and let compute_liveregs() take
		 * care of the rest from here:
		 */
		return 0;
	} else {
		regmask_t liveregs;
		compute_liveregs(ctx, instr, &liveregs);

		// XXX XXX XXX XXX XXX XXX XXX XXX XXX
		// XXX hack.. maybe ra_calc should give us a list of
		// instrs to compute_clobbers() on?
		if (is_meta(instr) && (instr->opc == OPC_META_INPUT) &&
				(instr->regs_count == 1)) {
			unsigned i, base = instr->regs[0]->num & ~0x3;
			for (i = 0; i < 4; i++) {
				struct ir3_instruction *in = ctx->block->inputs[base + i];
				if (in)
					compute_clobbers(ctx, in->next, in, &liveregs);
			}
		} else
		// XXX XXX XXX XXX XXX XXX XXX XXX XXX
		compute_clobbers(ctx, instr->next, instr, &liveregs);
		return find_available(&liveregs, size);
	}
}

/*
 * Constraint Calculation:
 */

struct ra_calc_visitor {
	struct ir3_visitor base;
	struct ir3_ra_assignment a;
};

static inline struct ra_calc_visitor *ra_calc_visitor(struct ir3_visitor *v)
{
	return (struct ra_calc_visitor *)v;
}

/* calculate register assignment for the instruction.  If the register
 * written by this instruction is required to be part of a range, to
 * handle other (input/output/sam/bary.f/etc) contiguous register range
 * constraints, that is calculated handled here.
 */
static void ra_calc_dst(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_calc_visitor *c = ra_calc_visitor(v);
	if (is_tex(instr)) {
		c->a.off = 0;
		c->a.num = 4;
	} else {
		c->a.off = 0;
		c->a.num = 1;
	}
}

static void
ra_calc_dst_shader_input(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_calc_visitor *c = ra_calc_visitor(v);
	struct ir3_block *block = instr->block;
	struct ir3_register *dst = instr->regs[0];
	unsigned base = dst->num & ~0x3;
	unsigned i, num = 0;

	assert(!(dst->flags & IR3_REG_IA));

	/* check what input components we need: */
	for (i = 0; i < 4; i++) {
		unsigned idx = base + i;
		if ((idx < block->ninputs) && block->inputs[idx])
			num = i + 1;
	}

	c->a.off = dst->num - base;
	c->a.num = num;
}

static void ra_calc_src_fanin(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_calc_visitor *c = ra_calc_visitor(v);
	unsigned srcn = ir3_instr_regno(instr, reg) - 1;
	c->a.off += srcn;
	c->a.num += srcn;
	c->a.num = MAX2(c->a.num, instr->regs_count - 1);
}

static const struct ir3_visitor_funcs calc_visitor_funcs = {
		.instr = ir3_visit_instr,
		.dst_shader_input = ra_calc_dst_shader_input,
		.dst_fanout = ra_calc_dst,
		.dst_fanin = ra_calc_dst,
		.dst = ra_calc_dst,
		.src_fanout = ir3_visit_reg,
		.src_fanin = ra_calc_src_fanin,
		.src = ir3_visit_reg,
};

static struct ir3_ra_assignment ra_calc(struct ir3_instruction *assigner)
{
	struct ra_calc_visitor v = {
			.base.funcs = &calc_visitor_funcs,
	};

	ir3_visit_instr(&v.base, assigner);

	return v.a;
}

/*
 * Register Assignment:
 */

struct ra_assign_visitor {
	struct ir3_visitor base;
	struct ir3_ra_ctx *ctx;
	int num;
};

static inline struct ra_assign_visitor *ra_assign_visitor(struct ir3_visitor *v)
{
	return (struct ra_assign_visitor *)v;
}

static type_t half_type(type_t type)
{
	switch (type) {
	case TYPE_F32: return TYPE_F16;
	case TYPE_U32: return TYPE_U16;
	case TYPE_S32: return TYPE_S16;
	/* instructions may already be fixed up: */
	case TYPE_F16:
	case TYPE_U16:
	case TYPE_S16:
		return type;
	default:
		assert(0);
		return ~0;
	}
}

/* some instructions need fix-up if dst register is half precision: */
static void fixup_half_instr_dst(struct ir3_instruction *instr)
{
	switch (instr->category) {
	case 1: /* move instructions */
		instr->cat1.dst_type = half_type(instr->cat1.dst_type);
		break;
	case 3:
		switch (instr->opc) {
		case OPC_MAD_F32:
			instr->opc = OPC_MAD_F16;
			break;
		case OPC_SEL_B32:
			instr->opc = OPC_SEL_B16;
			break;
		case OPC_SEL_S32:
			instr->opc = OPC_SEL_S16;
			break;
		case OPC_SEL_F32:
			instr->opc = OPC_SEL_F16;
			break;
		case OPC_SAD_S32:
			instr->opc = OPC_SAD_S16;
			break;
		/* instructions may already be fixed up: */
		case OPC_MAD_F16:
		case OPC_SEL_B16:
		case OPC_SEL_S16:
		case OPC_SEL_F16:
		case OPC_SAD_S16:
			break;
		default:
			assert(0);
			break;
		}
		break;
	case 5:
		instr->cat5.type = half_type(instr->cat5.type);
		break;
	}
}
/* some instructions need fix-up if src register is half precision: */
static void fixup_half_instr_src(struct ir3_instruction *instr)
{
	switch (instr->category) {
	case 1: /* move instructions */
		instr->cat1.src_type = half_type(instr->cat1.src_type);
		break;
	}
}

static void ra_assign_reg(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_assign_visitor *a = ra_assign_visitor(v);

	if (is_flow(instr) && (instr->opc == OPC_KILL))
		return;

	reg->flags &= ~IR3_REG_SSA;
	reg->num = a->num & ~REG_HALF;

	assert(reg->num >= 0);

	if (a->num & REG_HALF) {
		reg->flags |= IR3_REG_HALF;
		/* if dst reg being assigned, patch up the instr: */
		if (reg == instr->regs[0])
			fixup_half_instr_dst(instr);
		else
			fixup_half_instr_src(instr);
	}
}

static void ra_assign_dst_shader_input(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_assign_visitor *a = ra_assign_visitor(v);
	unsigned i, base = reg->num & ~0x3;
	int off = base - reg->num;

	ra_assign_reg(v, instr, reg);
	reg->flags |= IR3_REG_IA;

	/* trigger assignment of all our companion input components: */
	for (i = 0; i < 4; i++) {
		struct ir3_instruction *in = instr->block->inputs[i+base];
		if (in && is_meta(in) && (in->opc == OPC_META_INPUT))
			ra_assign(a->ctx, in, a->num + off + i);
	}
}

static void ra_assign_dst_fanout(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_assign_visitor *a = ra_assign_visitor(v);
	struct ir3_register *src = instr->regs[1];
	ra_assign_reg(v, instr, reg);
	if (src->flags & IR3_REG_SSA)
		ra_assign(a->ctx, src->instr, a->num - instr->fo.off);
}

static void ra_assign_src_fanout(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_assign_visitor *a = ra_assign_visitor(v);
	ra_assign_reg(v, instr, reg);
	ra_assign(a->ctx, instr, a->num + instr->fo.off);
}


static void ra_assign_src_fanin(struct ir3_visitor *v,
		struct ir3_instruction *instr, struct ir3_register *reg)
{
	struct ra_assign_visitor *a = ra_assign_visitor(v);
	unsigned j, srcn = ir3_instr_regno(instr, reg) - 1;
	ra_assign_reg(v, instr, reg);
	ra_assign(a->ctx, instr, a->num - srcn);
	for (j = 1; j < instr->regs_count; j++) {
		struct ir3_register *reg = instr->regs[j];
		if (reg->flags & IR3_REG_SSA)  /* could be renamed already */
			ra_assign(a->ctx, reg->instr, a->num - srcn + j - 1);
	}
}

static const struct ir3_visitor_funcs assign_visitor_funcs = {
		.instr = ir3_visit_instr,
		.dst_shader_input = ra_assign_dst_shader_input,
		.dst_fanout = ra_assign_dst_fanout,
		.dst_fanin = ra_assign_reg,
		.dst = ra_assign_reg,
		.src_fanout = ra_assign_src_fanout,
		.src_fanin = ra_assign_src_fanin,
		.src = ra_assign_reg,
};

static void ra_assign(struct ir3_ra_ctx *ctx,
		struct ir3_instruction *assigner, int num)
{
	struct ra_assign_visitor v = {
			.base.funcs = &assign_visitor_funcs,
			.ctx = ctx,
			.num = num,
	};

	/* if we've already visited this instruction, bail now: */
	if (ir3_instr_check_mark(assigner)) {
		debug_assert(assigner->regs[0]->num == (num & ~REG_HALF));
		if (assigner->regs[0]->num != (num & ~REG_HALF)) {
			/* impossible situation, should have been resolved
			 * at an earlier stage by inserting extra mov's:
			 */
			ctx->error = true;
		}
		return;
	}

	ir3_visit_instr(&v.base, assigner);
}

/*
 *
 */

static void ir3_instr_ra(struct ir3_ra_ctx *ctx,
		struct ir3_instruction *instr)
{
	struct ir3_ra_assignment a;
	unsigned num;

	/* skip over nop's */
	if (instr->regs_count == 0)
		return;

	/* skip writes to a0, p0, etc */
	if (!reg_gpr(instr->regs[0]))
		return;

	/* if we've already visited this instruction, bail now: */
	if (instr->flags & IR3_INSTR_MARK)
		return;

	/* allocate register(s): */
	a = ra_calc(instr);
	num = alloc_block(ctx, instr, a.num) + a.off;

	ra_assign(ctx, instr, num);
}

/* flatten into shader: */
// XXX this should probably be somewhere else:
static void legalize(struct ir3_ra_ctx *ctx, struct ir3_block *block)
{
	struct ir3_instruction *n;
	struct ir3_shader *shader = block->shader;
	struct ir3_instruction *end =
			ir3_instr_create(block, 0, OPC_END);
	struct ir3_instruction *last_input = NULL;
	regmask_t needs_ss_war;       /* write after read */
	regmask_t needs_ss;
	regmask_t needs_sy;

	regmask_init(&needs_ss_war);
	regmask_init(&needs_ss);
	regmask_init(&needs_sy);

	shader->instrs_count = 0;

	for (n = block->head; n; n = n->next) {
		struct ir3_register *reg;
		unsigned i;

		if (is_meta(n))
			continue;

		for (i = 1; i < n->regs_count; i++) {
			reg = n->regs[i];

			if (reg_gpr(reg)) {

				/* TODO: we probably only need (ss) for alu
				 * instr consuming sfu result.. need to make
				 * some tests for both this and (sy)..
				 */
				if (regmask_get(&needs_ss, reg)) {
					n->flags |= IR3_INSTR_SS;
					regmask_init(&needs_ss);
				}

				if (regmask_get(&needs_sy, reg)) {
					n->flags |= IR3_INSTR_SY;
					regmask_init(&needs_sy);
				}
			}
		}

		if (n->regs_count > 0) {
			reg = n->regs[0];
			if (regmask_get(&needs_ss_war, reg)) {
				n->flags |= IR3_INSTR_SS;
				regmask_init(&needs_ss_war); // ??? I assume?
			}
		}

		/* cat5+ does not have an (ss) bit, if needed we need to
		 * insert a nop to carry the sync flag.  Would be kinda
		 * clever if we were aware of this during scheduling, but
		 * this should be a pretty rare case:
		 */
		if ((n->flags & IR3_INSTR_SS) && (n->category >= 5)) {
			struct ir3_instruction *nop;
			nop = ir3_instr_create(block, 0, OPC_NOP);
			nop->flags |= IR3_INSTR_SS;
			n->flags &= ~IR3_INSTR_SS;
		}

		/* need to be able to set (ss) on first instruction: */
		if ((shader->instrs_count == 0) && (n->category >= 5))
			ir3_instr_create(block, 0, OPC_NOP);

		if (is_nop(n) && shader->instrs_count) {
			struct ir3_instruction *last =
					shader->instrs[shader->instrs_count-1];
			if (is_nop(last) && (last->repeat < 5)) {
				last->repeat++;
				last->flags |= n->flags;
				continue;
			}
		}

		shader->instrs[shader->instrs_count++] = n;

		if (is_sfu(n))
			regmask_set(&needs_ss, n->regs[0]);

		if (is_tex(n)) {
			/* this ends up being the # of samp instructions.. but that
			 * is ok, everything else only cares whether it is zero or
			 * not.  We do this here, rather than when we encounter a
			 * SAMP decl, because (especially in binning pass shader)
			 * the samp instruction(s) could get eliminated if the
			 * result is not used.
			 */
			ctx->has_samp = true;
			regmask_set(&needs_sy, n->regs[0]);
		}

		/* both tex/sfu appear to not always immediately consume
		 * their src register(s):
		 */
		if (is_tex(n) || is_sfu(n)) {
			for (i = 1; i < n->regs_count; i++) {
				reg = n->regs[i];
				if (reg_gpr(reg))
					regmask_set(&needs_ss_war, reg);
			}
		}

		if (is_input(n))
			last_input = n;
	}

	if (last_input)
		last_input->regs[0]->flags |= IR3_REG_EI;

	shader->instrs[shader->instrs_count++] = end;

	shader->instrs[0]->flags |= IR3_INSTR_SS | IR3_INSTR_SY;
}

static int block_ra(struct ir3_ra_ctx *ctx, struct ir3_block *block)
{
	struct ir3_instruction *n;

	if (!block->parent) {
		unsigned i, j;
		int base, off = output_base(ctx);

		base = alloc_block(ctx, NULL, block->noutputs + off);

		if (ctx->half_precision)
			base |= REG_HALF;

		for (i = 0; i < block->noutputs; i++)
			if (block->outputs[i] && !is_kill(block->outputs[i]))
				ra_assign(ctx, block->outputs[i], base + i + off);

		if (ctx->type == SHADER_FRAGMENT) {
			i = 0;
			if (ctx->frag_face) {
				/* if we have frag_face, it gets hr0.x */
				ra_assign(ctx, block->inputs[i], REG_HALF | 0);
				i += 4;
			}
			for (j = 0; i < block->ninputs; i++, j++)
				if (block->inputs[i])
					ra_assign(ctx, block->inputs[i], (base & ~REG_HALF) + j);
		} else {
			for (i = 0; i < block->ninputs; i++)
				if (block->inputs[i])
					ir3_instr_ra(ctx, block->inputs[i]);
		}
	}

	/* then loop over instruction list and assign registers:
	 */
	n = block->head;
	while (n) {
		ir3_instr_ra(ctx, n);
		if (ctx->error)
			return -1;
		n = n->next;
	}

	legalize(ctx, block);

	return 0;
}

int ir3_block_ra(struct ir3_block *block, enum shader_t type,
		bool half_precision, bool frag_coord, bool frag_face,
		bool *has_samp)
{
	struct ir3_ra_ctx ctx = {
			.block = block,
			.type = type,
			.half_precision = half_precision,
			.frag_coord = frag_coord,
			.frag_face = frag_face,
	};
	int ret;

	ir3_shader_clear_mark(block->shader);
	ret = block_ra(&ctx, block);
	*has_samp = ctx.has_samp;

	return ret;
}
