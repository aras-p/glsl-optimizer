/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_ureg.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_strings.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"

#include "freedreno_lowering.h"

#include "fd3_compiler.h"
#include "fd3_program.h"
#include "fd3_util.h"

#include "instr-a3xx.h"
#include "ir3.h"


struct fd3_compile_context {
	const struct tgsi_token *tokens;
	bool free_tokens;
	struct ir3_shader *ir;
	struct ir3_block *block;
	struct fd3_shader_variant *so;

	struct tgsi_parse_context parser;
	unsigned type;

	struct tgsi_shader_info info;

	/* last input dst (for setting (ei) flag): */
	struct ir3_register *last_input;

	/* last instruction with relative addressing: */
	struct ir3_instruction *last_rel;

	/* for calculating input/output positions/linkages: */
	unsigned next_inloc;

	unsigned num_internal_temps;
	struct tgsi_src_register internal_temps[6];

	/* track registers which need to synchronize w/ "complex alu" cat3
	 * instruction pipeline:
	 */
	regmask_t needs_ss;

	/* track registers which need to synchronize with texture fetch
	 * pipeline:
	 */
	regmask_t needs_sy;

	/* inputs start at r0, temporaries start after last input, and
	 * outputs start after last temporary.
	 *
	 * We could be more clever, because this is not a hw restriction,
	 * but probably best just to implement an optimizing pass to
	 * reduce the # of registers used and get rid of redundant mov's
	 * (to output register).
	 */
	unsigned base_reg[TGSI_FILE_COUNT];

	/* idx/slot for last compiler generated immediate */
	unsigned immediate_idx;

	/* stack of branch instructions that start (potentially nested)
	 * branch instructions, so that we can fix up the branch targets
	 * so that we can fix up the branch target on the corresponding
	 * END instruction
	 */
	struct ir3_instruction *branch[16];
	unsigned int branch_count;

	/* used when dst is same as one of the src, to avoid overwriting a
	 * src element before the remaining scalar instructions that make
	 * up the vector operation
	 */
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register *tmp_src;
};


static void vectorize(struct fd3_compile_context *ctx,
		struct ir3_instruction *instr, struct tgsi_dst_register *dst,
		int nsrcs, ...);
static void create_mov(struct fd3_compile_context *ctx,
		struct tgsi_dst_register *dst, struct tgsi_src_register *src);

static unsigned
compile_init(struct fd3_compile_context *ctx, struct fd3_shader_variant *so,
		const struct tgsi_token *tokens)
{
	unsigned ret, base = 0;
	struct tgsi_shader_info *info = &ctx->info;
	const struct fd_lowering_config lconfig = {
			.color_two_side = so->key.color_two_side,
			.lower_DST  = true,
			.lower_XPD  = true,
			.lower_SCS  = true,
			.lower_LRP  = true,
			.lower_FRC  = true,
			.lower_POW  = true,
			.lower_LIT  = true,
			.lower_EXP  = true,
			.lower_LOG  = true,
			.lower_DP4  = true,
			.lower_DP3  = true,
			.lower_DPH  = true,
			.lower_DP2  = true,
			.lower_DP2A = true,
	};

	ctx->tokens = fd_transform_lowering(&lconfig, tokens, &ctx->info);
	ctx->free_tokens = !!ctx->tokens;
	if (!ctx->tokens) {
		/* no lowering */
		ctx->tokens = tokens;
	}
	ctx->ir = so->ir;
	ctx->block = ir3_block_create(ctx->ir, 0, 0, 0);
	ctx->so = so;
	ctx->last_input = NULL;
	ctx->last_rel = NULL;
	ctx->next_inloc = 8;
	ctx->num_internal_temps = 0;
	ctx->branch_count = 0;

	regmask_init(&ctx->needs_ss);
	regmask_init(&ctx->needs_sy);
	memset(ctx->base_reg, 0, sizeof(ctx->base_reg));

	/* Immediates go after constants: */
	ctx->base_reg[TGSI_FILE_CONSTANT]  = 0;
	ctx->base_reg[TGSI_FILE_IMMEDIATE] =
			info->file_max[TGSI_FILE_CONSTANT] + 1;

	/* if full precision and fragment shader, don't clobber
	 * r0.x w/ bary fetch:
	 */
	if ((so->type == SHADER_FRAGMENT) && !so->key.half_precision)
		base = 1;

	/* Temporaries after outputs after inputs: */
	ctx->base_reg[TGSI_FILE_INPUT]     = base;
	ctx->base_reg[TGSI_FILE_OUTPUT]    = base +
			info->file_max[TGSI_FILE_INPUT] + 1;
	ctx->base_reg[TGSI_FILE_TEMPORARY] = base +
			info->file_max[TGSI_FILE_INPUT] + 1 +
			info->file_max[TGSI_FILE_OUTPUT] + 1;

	so->first_immediate = ctx->base_reg[TGSI_FILE_IMMEDIATE];
	ctx->immediate_idx = 4 * (ctx->info.file_max[TGSI_FILE_IMMEDIATE] + 1);

	ret = tgsi_parse_init(&ctx->parser, ctx->tokens);
	if (ret != TGSI_PARSE_OK)
		return ret;

	ctx->type = ctx->parser.FullHeader.Processor.Processor;

	return ret;
}

static void
compile_error(struct fd3_compile_context *ctx, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	_debug_vprintf(format, ap);
	va_end(ap);
	tgsi_dump(ctx->tokens, 0);
	debug_assert(0);
}

#define compile_assert(ctx, cond) do { \
		if (!(cond)) compile_error((ctx), "failed assert: "#cond"\n"); \
	} while (0)

static void
compile_free(struct fd3_compile_context *ctx)
{
	if (ctx->free_tokens)
		free((void *)ctx->tokens);
	tgsi_parse_free(&ctx->parser);
}

struct instr_translater {
	void (*fxn)(const struct instr_translater *t,
			struct fd3_compile_context *ctx,
			struct tgsi_full_instruction *inst);
	unsigned tgsi_opc;
	opc_t opc;
	opc_t hopc;    /* opc to use for half_precision mode, if different */
	unsigned arg;
};

static void
handle_last_rel(struct fd3_compile_context *ctx)
{
	if (ctx->last_rel) {
		ctx->last_rel->flags |= IR3_INSTR_UL;
		ctx->last_rel = NULL;
	}
}

static struct ir3_instruction *
instr_create(struct fd3_compile_context *ctx, int category, opc_t opc)
{
	return ir3_instr_create(ctx->block, category, opc);
}

static void
add_nop(struct fd3_compile_context *ctx, unsigned count)
{
	while (count-- > 0)
		instr_create(ctx, 0, OPC_NOP);
}

static unsigned
src_flags(struct fd3_compile_context *ctx, struct ir3_register *reg)
{
	unsigned flags = 0;

	if (reg->flags & (IR3_REG_CONST | IR3_REG_IMMED))
		return flags;

	if (regmask_get(&ctx->needs_ss, reg)) {
		flags |= IR3_INSTR_SS;
		regmask_init(&ctx->needs_ss);
	}

	if (regmask_get(&ctx->needs_sy, reg)) {
		flags |= IR3_INSTR_SY;
		regmask_init(&ctx->needs_sy);
	}

	return flags;
}

static struct ir3_register *
add_dst_reg(struct fd3_compile_context *ctx, struct ir3_instruction *instr,
		const struct tgsi_dst_register *dst, unsigned chan)
{
	unsigned flags = 0, num = 0;
	struct ir3_register *reg;

	switch (dst->File) {
	case TGSI_FILE_OUTPUT:
	case TGSI_FILE_TEMPORARY:
		num = dst->Index + ctx->base_reg[dst->File];
		break;
	case TGSI_FILE_ADDRESS:
		num = REG_A0;
		break;
	default:
		compile_error(ctx, "unsupported dst register file: %s\n",
			tgsi_file_name(dst->File));
		break;
	}

	if (dst->Indirect)
		flags |= IR3_REG_RELATIV;
	if (ctx->so->key.half_precision)
		flags |= IR3_REG_HALF;

	reg = ir3_reg_create(instr, regid(num, chan), flags);

	if (dst->Indirect)
		ctx->last_rel = instr;

	return reg;
}

static struct ir3_register *
add_src_reg(struct fd3_compile_context *ctx, struct ir3_instruction *instr,
		const struct tgsi_src_register *src, unsigned chan)
{
	unsigned flags = 0, num = 0;
	struct ir3_register *reg;

	/* TODO we need to use a mov to temp for const >= 64.. or maybe
	 * we could use relative addressing..
	 */
	compile_assert(ctx, src->Index < 64);

	switch (src->File) {
	case TGSI_FILE_IMMEDIATE:
		/* TODO if possible, use actual immediate instead of const.. but
		 * TGSI has vec4 immediates, we can only embed scalar (of limited
		 * size, depending on instruction..)
		 */
	case TGSI_FILE_CONSTANT:
		flags |= IR3_REG_CONST;
		num = src->Index + ctx->base_reg[src->File];
		break;
	case TGSI_FILE_OUTPUT:
		/* NOTE: we should only end up w/ OUTPUT file for things like
		 * clamp()'ing saturated dst instructions
		 */
	case TGSI_FILE_INPUT:
	case TGSI_FILE_TEMPORARY:
		num = src->Index + ctx->base_reg[src->File];
		break;
	default:
		compile_error(ctx, "unsupported src register file: %s\n",
			tgsi_file_name(src->File));
		break;
	}

	if (src->Absolute)
		flags |= IR3_REG_ABS;
	if (src->Negate)
		flags |= IR3_REG_NEGATE;
	if (src->Indirect)
		flags |= IR3_REG_RELATIV;
	if (ctx->so->key.half_precision)
		flags |= IR3_REG_HALF;

	reg = ir3_reg_create(instr, regid(num, chan), flags);

	if (src->Indirect)
		ctx->last_rel = instr;

	instr->flags |= src_flags(ctx, reg);

	return reg;
}

static void
src_from_dst(struct tgsi_src_register *src, struct tgsi_dst_register *dst)
{
	src->File      = dst->File;
	src->Indirect  = dst->Indirect;
	src->Dimension = dst->Dimension;
	src->Index     = dst->Index;
	src->Absolute  = 0;
	src->Negate    = 0;
	src->SwizzleX  = TGSI_SWIZZLE_X;
	src->SwizzleY  = TGSI_SWIZZLE_Y;
	src->SwizzleZ  = TGSI_SWIZZLE_Z;
	src->SwizzleW  = TGSI_SWIZZLE_W;
}

/* Get internal-temp src/dst to use for a sequence of instructions
 * generated by a single TGSI op.
 */
static struct tgsi_src_register *
get_internal_temp(struct fd3_compile_context *ctx,
		struct tgsi_dst_register *tmp_dst)
{
	struct tgsi_src_register *tmp_src;
	int n;

	tmp_dst->File      = TGSI_FILE_TEMPORARY;
	tmp_dst->WriteMask = TGSI_WRITEMASK_XYZW;
	tmp_dst->Indirect  = 0;
	tmp_dst->Dimension = 0;

	/* assign next temporary: */
	n = ctx->num_internal_temps++;
	compile_assert(ctx, n < ARRAY_SIZE(ctx->internal_temps));
	tmp_src = &ctx->internal_temps[n];

	tmp_dst->Index = ctx->info.file_max[TGSI_FILE_TEMPORARY] + n + 1;

	src_from_dst(tmp_src, tmp_dst);

	return tmp_src;
}

/* Get internal half-precision temp src/dst to use for a sequence of
 * instructions generated by a single TGSI op.
 */
static struct tgsi_src_register *
get_internal_temp_hr(struct fd3_compile_context *ctx,
		struct tgsi_dst_register *tmp_dst)
{
	struct tgsi_src_register *tmp_src;
	int n;

	if (ctx->so->key.half_precision)
		return get_internal_temp(ctx, tmp_dst);

	tmp_dst->File      = TGSI_FILE_TEMPORARY;
	tmp_dst->WriteMask = TGSI_WRITEMASK_XYZW;
	tmp_dst->Indirect  = 0;
	tmp_dst->Dimension = 0;

	/* assign next temporary: */
	n = ctx->num_internal_temps++;
	compile_assert(ctx, n < ARRAY_SIZE(ctx->internal_temps));
	tmp_src = &ctx->internal_temps[n];

	/* just use hr0 because no one else should be using half-
	 * precision regs:
	 */
	tmp_dst->Index = 0;

	src_from_dst(tmp_src, tmp_dst);

	return tmp_src;
}

static inline bool
is_const(struct tgsi_src_register *src)
{
	return (src->File == TGSI_FILE_CONSTANT) ||
			(src->File == TGSI_FILE_IMMEDIATE);
}

static inline bool
is_relative(struct tgsi_src_register *src)
{
	return src->Indirect;
}

static inline bool
is_rel_or_const(struct tgsi_src_register *src)
{
	return is_relative(src) || is_const(src);
}

static type_t
get_ftype(struct fd3_compile_context *ctx)
{
	return ctx->so->key.half_precision ? TYPE_F16 : TYPE_F32;
}

static type_t
get_utype(struct fd3_compile_context *ctx)
{
	return ctx->so->key.half_precision ? TYPE_U16 : TYPE_U32;
}

static unsigned
src_swiz(struct tgsi_src_register *src, int chan)
{
	switch (chan) {
	case 0: return src->SwizzleX;
	case 1: return src->SwizzleY;
	case 2: return src->SwizzleZ;
	case 3: return src->SwizzleW;
	}
	assert(0);
	return 0;
}

/* for instructions that cannot take a const register as src, if needed
 * generate a move to temporary gpr:
 */
static struct tgsi_src_register *
get_unconst(struct fd3_compile_context *ctx, struct tgsi_src_register *src)
{
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register *tmp_src;

	compile_assert(ctx, is_rel_or_const(src));

	tmp_src = get_internal_temp(ctx, &tmp_dst);

	create_mov(ctx, &tmp_dst, src);

	return tmp_src;
}

static void
get_immediate(struct fd3_compile_context *ctx,
		struct tgsi_src_register *reg, uint32_t val)
{
	unsigned neg, swiz, idx, i;
	/* actually maps 1:1 currently.. not sure if that is safe to rely on: */
	static const unsigned swiz2tgsi[] = {
			TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W,
	};

	for (i = 0; i < ctx->immediate_idx; i++) {
		swiz = i % 4;
		idx  = i / 4;

		if (ctx->so->immediates[idx].val[swiz] == val) {
			neg = 0;
			break;
		}

		if (ctx->so->immediates[idx].val[swiz] == -val) {
			neg = 1;
			break;
		}
	}

	if (i == ctx->immediate_idx) {
		/* need to generate a new immediate: */
		swiz = i % 4;
		idx  = i / 4;
		neg  = 0;
		ctx->so->immediates[idx].val[swiz] = val;
		ctx->so->immediates_count = idx + 1;
		ctx->immediate_idx++;
	}

	reg->File      = TGSI_FILE_IMMEDIATE;
	reg->Indirect  = 0;
	reg->Dimension = 0;
	reg->Index     = idx;
	reg->Absolute  = 0;
	reg->Negate    = neg;
	reg->SwizzleX  = swiz2tgsi[swiz];
	reg->SwizzleY  = swiz2tgsi[swiz];
	reg->SwizzleZ  = swiz2tgsi[swiz];
	reg->SwizzleW  = swiz2tgsi[swiz];
}

static void
create_mov(struct fd3_compile_context *ctx, struct tgsi_dst_register *dst,
		struct tgsi_src_register *src)
{
	type_t type_mov = get_ftype(ctx);
	unsigned i;

	for (i = 0; i < 4; i++) {
		/* move to destination: */
		if (dst->WriteMask & (1 << i)) {
			struct ir3_instruction *instr;

			if (src->Absolute || src->Negate) {
				/* can't have abs or neg on a mov instr, so use
				 * absneg.f instead to handle these cases:
				 */
				instr = instr_create(ctx, 2, OPC_ABSNEG_F);
			} else {
				instr = instr_create(ctx, 1, 0);
				instr->cat1.src_type = type_mov;
				instr->cat1.dst_type = type_mov;
			}

			add_dst_reg(ctx, instr, dst, i);
			add_src_reg(ctx, instr, src, src_swiz(src, i));
		} else {
			add_nop(ctx, 1);
		}
	}
}

static void
create_clamp(struct fd3_compile_context *ctx,
		struct tgsi_dst_register *dst, struct tgsi_src_register *val,
		struct tgsi_src_register *minval, struct tgsi_src_register *maxval)
{
	struct ir3_instruction *instr;

	instr = instr_create(ctx, 2, OPC_MAX_F);
	vectorize(ctx, instr, dst, 2, val, 0, minval, 0);

	instr = instr_create(ctx, 2, OPC_MIN_F);
	vectorize(ctx, instr, dst, 2, val, 0, maxval, 0);
}

static void
create_clamp_imm(struct fd3_compile_context *ctx,
		struct tgsi_dst_register *dst,
		uint32_t minval, uint32_t maxval)
{
	struct tgsi_src_register minconst, maxconst;
	struct tgsi_src_register src;

	src_from_dst(&src, dst);

	get_immediate(ctx, &minconst, minval);
	get_immediate(ctx, &maxconst, maxval);

	create_clamp(ctx, dst, &src, &minconst, &maxconst);
}

static struct tgsi_dst_register *
get_dst(struct fd3_compile_context *ctx, struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register *dst = &inst->Dst[0].Register;
	unsigned i;
	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		struct tgsi_src_register *src = &inst->Src[i].Register;
		if ((src->File == dst->File) && (src->Index == dst->Index)) {
			if ((dst->WriteMask == TGSI_WRITEMASK_XYZW) &&
					(src->SwizzleX == TGSI_SWIZZLE_X) &&
					(src->SwizzleY == TGSI_SWIZZLE_Y) &&
					(src->SwizzleZ == TGSI_SWIZZLE_Z) &&
					(src->SwizzleW == TGSI_SWIZZLE_W))
				continue;
			ctx->tmp_src = get_internal_temp(ctx, &ctx->tmp_dst);
			ctx->tmp_dst.WriteMask = dst->WriteMask;
			dst = &ctx->tmp_dst;
			break;
		}
	}
	return dst;
}

static void
put_dst(struct fd3_compile_context *ctx, struct tgsi_full_instruction *inst,
		struct tgsi_dst_register *dst)
{
	/* if necessary, add mov back into original dst: */
	if (dst != &inst->Dst[0].Register) {
		create_mov(ctx, &inst->Dst[0].Register, ctx->tmp_src);
	}
}

/* helper to generate the necessary repeat and/or additional instructions
 * to turn a scalar instruction into a vector operation:
 */
static void
vectorize(struct fd3_compile_context *ctx, struct ir3_instruction *instr,
		struct tgsi_dst_register *dst, int nsrcs, ...)
{
	va_list ap;
	int i, j, n = 0;
	bool indirect = dst->Indirect;

	add_dst_reg(ctx, instr, dst, TGSI_SWIZZLE_X);

	va_start(ap, nsrcs);
	for (j = 0; j < nsrcs; j++) {
		struct tgsi_src_register *src =
				va_arg(ap, struct tgsi_src_register *);
		unsigned flags = va_arg(ap, unsigned);
		struct ir3_register *reg;
		if (flags & IR3_REG_IMMED) {
			reg = ir3_reg_create(instr, 0, IR3_REG_IMMED);
			/* this is an ugly cast.. should have put flags first! */
			reg->iim_val = *(int *)&src;
		} else {
			reg = add_src_reg(ctx, instr, src, TGSI_SWIZZLE_X);
			indirect |= src->Indirect;
		}
		reg->flags |= flags & ~IR3_REG_NEGATE;
		if (flags & IR3_REG_NEGATE)
			reg->flags ^= IR3_REG_NEGATE;
	}
	va_end(ap);

	for (i = 0; i < 4; i++) {
		if (dst->WriteMask & (1 << i)) {
			struct ir3_instruction *cur;

			if (n++ == 0) {
				cur = instr;
			} else {
				cur = ir3_instr_clone(instr);
				cur->flags &= ~(IR3_INSTR_SY | IR3_INSTR_SS | IR3_INSTR_JP);
			}

			/* fix-up dst register component: */
			cur->regs[0]->num = regid(cur->regs[0]->num >> 2, i);

			/* fix-up src register component: */
			va_start(ap, nsrcs);
			for (j = 0; j < nsrcs; j++) {
				struct tgsi_src_register *src =
						va_arg(ap, struct tgsi_src_register *);
				unsigned flags = va_arg(ap, unsigned);
				if (!(flags & IR3_REG_IMMED)) {
					cur->regs[j+1]->num =
							regid(cur->regs[j+1]->num >> 2,
									src_swiz(src, i));
					cur->flags |= src_flags(ctx, cur->regs[j+1]);
				}
			}
			va_end(ap);

			if (indirect)
				ctx->last_rel = cur;
		}
	}

	/* pad w/ nop's.. at least until we are clever enough to
	 * figure out if we really need to..
	 */
	add_nop(ctx, 4 - n);
}

/*
 * Handlers for TGSI instructions which do not have a 1:1 mapping to
 * native instructions:
 */

static void
trans_clamp(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *src0 = &inst->Src[0].Register;
	struct tgsi_src_register *src1 = &inst->Src[1].Register;
	struct tgsi_src_register *src2 = &inst->Src[2].Register;

	create_clamp(ctx, dst, src0, src1, src2);

	put_dst(ctx, inst, dst);
}

/* ARL(x) = x, but mova from hrN.x to a0.. */
static void
trans_arl(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register *tmp_src;
	struct tgsi_dst_register *dst = &inst->Dst[0].Register;
	struct tgsi_src_register *src = &inst->Src[0].Register;
	unsigned chan = src->SwizzleX;
	compile_assert(ctx, dst->File == TGSI_FILE_ADDRESS);

	handle_last_rel(ctx);

	tmp_src = get_internal_temp_hr(ctx, &tmp_dst);

	/* cov.{f32,f16}s16 Rtmp, Rsrc */
	instr = instr_create(ctx, 1, 0);
	instr->cat1.src_type = get_ftype(ctx);
	instr->cat1.dst_type = TYPE_S16;
	add_dst_reg(ctx, instr, &tmp_dst, chan)->flags |= IR3_REG_HALF;
	add_src_reg(ctx, instr, src, chan);

	add_nop(ctx, 3);

	/* shl.b Rtmp, Rtmp, 2 */
	instr = instr_create(ctx, 2, OPC_SHL_B);
	add_dst_reg(ctx, instr, &tmp_dst, chan)->flags |= IR3_REG_HALF;
	add_src_reg(ctx, instr, tmp_src, chan)->flags |= IR3_REG_HALF;
	ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = 2;

	add_nop(ctx, 3);

	/* mova a0, Rtmp */
	instr = instr_create(ctx, 1, 0);
	instr->cat1.src_type = TYPE_S16;
	instr->cat1.dst_type = TYPE_S16;
	add_dst_reg(ctx, instr, dst, 0)->flags |= IR3_REG_HALF;
	add_src_reg(ctx, instr, tmp_src, chan)->flags |= IR3_REG_HALF;

	/* need to ensure 5 instr slots before a0 is used: */
	add_nop(ctx, 6);
}

/* texture fetch/sample instructions: */
static void
trans_samp(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_register *r;
	struct ir3_instruction *instr;
	struct tgsi_src_register *coord = &inst->Src[0].Register;
	struct tgsi_src_register *samp  = &inst->Src[1].Register;
	unsigned tex = inst->Texture.Texture;
	int8_t *order;
	unsigned i, flags = 0, src_wrmask;
	bool needs_mov = false;

	switch (t->arg) {
	case TGSI_OPCODE_TEX:
		if (tex == TGSI_TEXTURE_2D) {
			order = (int8_t[4]){ 0,  1, -1, -1 };
			src_wrmask = TGSI_WRITEMASK_XY;
		} else {
			order = (int8_t[4]){ 0,  1,  2, -1 };
			src_wrmask = TGSI_WRITEMASK_XYZ;
		}
		break;
	case TGSI_OPCODE_TXP:
		if (tex == TGSI_TEXTURE_2D) {
			order = (int8_t[4]){ 0,  1,  3, -1 };
			src_wrmask = TGSI_WRITEMASK_XYZ;
		} else {
			order = (int8_t[4]){ 0,  1,  2,  3 };
			src_wrmask = TGSI_WRITEMASK_XYZW;
		}
		flags |= IR3_INSTR_P;
		break;
	default:
		compile_assert(ctx, 0);
		break;
	}

	if ((tex == TGSI_TEXTURE_3D) || (tex == TGSI_TEXTURE_CUBE)) {
		add_nop(ctx, 3);
		flags |= IR3_INSTR_3D;
	}

	/* cat5 instruction cannot seem to handle const or relative: */
	if (is_rel_or_const(coord))
		needs_mov = true;

	/* The texture sample instructions need to coord in successive
	 * registers/components (ie. src.xy but not src.yx).  And TXP
	 * needs the .w component in .z for 2D..  so in some cases we
	 * might need to emit some mov instructions to shuffle things
	 * around:
	 */
	for (i = 1; (i < 4) && (order[i] >= 0) && !needs_mov; i++)
		if (src_swiz(coord, i) != (src_swiz(coord, 0) + order[i]))
			needs_mov = true;

	if (needs_mov) {
		struct tgsi_dst_register tmp_dst;
		struct tgsi_src_register *tmp_src;
		unsigned j;

		type_t type_mov = get_ftype(ctx);

		/* need to move things around: */
		tmp_src = get_internal_temp(ctx, &tmp_dst);

		for (j = 0; (j < 4) && (order[j] >= 0); j++) {
			instr = instr_create(ctx, 1, 0);
			instr->cat1.src_type = type_mov;
			instr->cat1.dst_type = type_mov;
			add_dst_reg(ctx, instr, &tmp_dst, j);
			add_src_reg(ctx, instr, coord,
					src_swiz(coord, order[j]));
		}

		coord = tmp_src;

		add_nop(ctx, 4 - j);
	}

	instr = instr_create(ctx, 5, t->opc);
	instr->cat5.type = get_ftype(ctx);
	instr->cat5.samp = samp->Index;
	instr->cat5.tex  = samp->Index;
	instr->flags |= flags;

	r = add_dst_reg(ctx, instr, &inst->Dst[0].Register, 0);
	r->wrmask = inst->Dst[0].Register.WriteMask;

	add_src_reg(ctx, instr, coord, coord->SwizzleX)->wrmask = src_wrmask;

	/* after add_src_reg() so we don't set (sy) on sam instr itself! */
	regmask_set(&ctx->needs_sy, r);
}

/*
 * SEQ(a,b) = (a == b) ? 1.0 : 0.0
 *   cmps.f.eq tmp0, b, a
 *   cov.u16f16 dst, tmp0
 *
 * SNE(a,b) = (a != b) ? 1.0 : 0.0
 *   cmps.f.eq tmp0, b, a
 *   add.s tmp0, tmp0, -1
 *   sel.f16 dst, {0.0}, tmp0, {1.0}
 *
 * SGE(a,b) = (a >= b) ? 1.0 : 0.0
 *   cmps.f.ge tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * SLE(a,b) = (a <= b) ? 1.0 : 0.0
 *   cmps.f.ge tmp0, b, a
 *   cov.u16f16 dst, tmp0
 *
 * SGT(a,b) = (a > b)  ? 1.0 : 0.0
 *   cmps.f.ge tmp0, b, a
 *   add.s tmp0, tmp0, -1
 *   sel.f16 dst, {0.0}, tmp0, {1.0}
 *
 * SLT(a,b) = (a < b)  ? 1.0 : 0.0
 *   cmps.f.ge tmp0, a, b
 *   add.s tmp0, tmp0, -1
 *   sel.f16 dst, {0.0}, tmp0, {1.0}
 *
 * CMP(a,b,c) = (a < 0.0) ? b : c
 *   cmps.f.ge tmp0, a, {0.0}
 *   add.s tmp0, tmp0, -1
 *   sel.f16 dst, c, tmp0, b
 */
static void
trans_cmp(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register *tmp_src;
	struct tgsi_src_register constval0, constval1;
	/* final instruction for CMP() uses orig src1 and src2: */
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *a0, *a1;
	unsigned condition;

	tmp_src = get_internal_temp(ctx, &tmp_dst);

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_SEQ:
	case TGSI_OPCODE_SNE:
		a0 = &inst->Src[1].Register;  /* b */
		a1 = &inst->Src[0].Register;  /* a */
		condition = IR3_COND_EQ;
		break;
	case TGSI_OPCODE_SGE:
	case TGSI_OPCODE_SLT:
		a0 = &inst->Src[0].Register;  /* a */
		a1 = &inst->Src[1].Register;  /* b */
		condition = IR3_COND_GE;
		break;
	case TGSI_OPCODE_SLE:
	case TGSI_OPCODE_SGT:
		a0 = &inst->Src[1].Register;  /* b */
		a1 = &inst->Src[0].Register;  /* a */
		condition = IR3_COND_GE;
		break;
	case TGSI_OPCODE_CMP:
		get_immediate(ctx, &constval0, fui(0.0));
		a0 = &inst->Src[0].Register;  /* a */
		a1 = &constval0;              /* {0.0} */
		condition = IR3_COND_GE;
		break;
	default:
		compile_assert(ctx, 0);
		return;
	}

	if (is_const(a0) && is_const(a1))
		a0 = get_unconst(ctx, a0);

	/* cmps.f.ge tmp, a0, a1 */
	instr = instr_create(ctx, 2, OPC_CMPS_F);
	instr->cat2.condition = condition;
	vectorize(ctx, instr, &tmp_dst, 2, a0, 0, a1, 0);

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_SEQ:
	case TGSI_OPCODE_SGE:
	case TGSI_OPCODE_SLE:
		/* cov.u16f16 dst, tmp0 */
		instr = instr_create(ctx, 1, 0);
		instr->cat1.src_type = get_utype(ctx);
		instr->cat1.dst_type = get_ftype(ctx);
		vectorize(ctx, instr, dst, 1, tmp_src, 0);
		break;
	case TGSI_OPCODE_SNE:
	case TGSI_OPCODE_SGT:
	case TGSI_OPCODE_SLT:
	case TGSI_OPCODE_CMP:
		/* add.s tmp, tmp, -1 */
		instr = instr_create(ctx, 2, OPC_ADD_S);
		vectorize(ctx, instr, &tmp_dst, 2, tmp_src, 0, -1, IR3_REG_IMMED);

		if (t->tgsi_opc == TGSI_OPCODE_CMP) {
			/* sel.{f32,f16} dst, src2, tmp, src1 */
			instr = instr_create(ctx, 3,
					ctx->so->key.half_precision ? OPC_SEL_F16 : OPC_SEL_F32);
			vectorize(ctx, instr, dst, 3,
					&inst->Src[2].Register, 0,
					tmp_src, 0,
					&inst->Src[1].Register, 0);
		} else {
			get_immediate(ctx, &constval0, fui(0.0));
			get_immediate(ctx, &constval1, fui(1.0));
			/* sel.{f32,f16} dst, {0.0}, tmp0, {1.0} */
			instr = instr_create(ctx, 3,
					ctx->so->key.half_precision ? OPC_SEL_F16 : OPC_SEL_F32);
			vectorize(ctx, instr, dst, 3,
					&constval0, 0, tmp_src, 0, &constval1, 0);
		}

		break;
	}

	put_dst(ctx, inst, dst);
}

/*
 * Conditional / Flow control
 */

static unsigned
find_instruction(struct fd3_compile_context *ctx, struct ir3_instruction *instr)
{
	unsigned i;
	for (i = 0; i < ctx->ir->instrs_count; i++)
		if (ctx->ir->instrs[i] == instr)
			return i;
	return ~0;
}

static void
push_branch(struct fd3_compile_context *ctx, struct ir3_instruction *instr)
{
	ctx->branch[ctx->branch_count++] = instr;
}

static void
pop_branch(struct fd3_compile_context *ctx)
{
	struct ir3_instruction *instr;

	/* if we were clever enough, we'd patch this up after the fact,
	 * and set (jp) flag on whatever the next instruction was, rather
	 * than inserting an extra nop..
	 */
	instr = instr_create(ctx, 0, OPC_NOP);
	instr->flags |= IR3_INSTR_JP;

	/* pop the branch instruction from the stack and fix up branch target: */
	instr = ctx->branch[--ctx->branch_count];
	instr->cat0.immed = ctx->ir->instrs_count - find_instruction(ctx, instr) - 1;
}

/* We probably don't really want to translate if/else/endif into branches..
 * the blob driver evaluates both legs of the if and then uses the sel
 * instruction to pick which sides of the branch to "keep".. but figuring
 * that out will take somewhat more compiler smarts.  So hopefully branches
 * don't kill performance too badly.
 */
static void
trans_if(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_src_register *src = &inst->Src[0].Register;
	struct tgsi_src_register constval;

	get_immediate(ctx, &constval, fui(0.0));

	if (is_const(src))
		src = get_unconst(ctx, src);

	instr = instr_create(ctx, 2, OPC_CMPS_F);
	ir3_reg_create(instr, regid(REG_P0, 0), 0);
	add_src_reg(ctx, instr, src, src->SwizzleX);
	add_src_reg(ctx, instr, &constval, constval.SwizzleX);
	instr->cat2.condition = IR3_COND_EQ;

	instr = instr_create(ctx, 0, OPC_BR);
	push_branch(ctx, instr);
}

static void
trans_else(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;

	/* for first half of if/else/endif, generate a jump past the else: */
	instr = instr_create(ctx, 0, OPC_JUMP);

	pop_branch(ctx);
	push_branch(ctx, instr);
}

static void
trans_endif(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	pop_branch(ctx);
}

/*
 * Handlers for TGSI instructions which do have 1:1 mapping to native
 * instructions:
 */

static void
instr_cat0(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	instr_create(ctx, 0, t->opc);
}

static void
instr_cat1(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *src = &inst->Src[0].Register;

	/* mov instructions can't handle a negate on src: */
	if (src->Negate) {
		struct tgsi_src_register constval;
		struct ir3_instruction *instr;

		/* since right now, we are using uniformly either TYPE_F16 or
		 * TYPE_F32, and we don't utilize the conversion possibilities
		 * of mov instructions, we can get away with substituting an
		 * add.f which can handle negate.  Might need to revisit this
		 * in the future if we start supporting widening/narrowing or
		 * conversion to/from integer..
		 */
		instr = instr_create(ctx, 2, OPC_ADD_F);
		get_immediate(ctx, &constval, fui(0.0));
		vectorize(ctx, instr, dst, 2, src, 0, &constval, 0);
	} else {
		create_mov(ctx, dst, src);
		/* create_mov() generates vector sequence, so no vectorize() */
	}
	put_dst(ctx, inst, dst);
}

static void
instr_cat2(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *src0 = &inst->Src[0].Register;
	struct tgsi_src_register *src1 = &inst->Src[1].Register;
	struct ir3_instruction *instr;
	unsigned src0_flags = 0, src1_flags = 0;

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_ABS:
		src0_flags = IR3_REG_ABS;
		break;
	case TGSI_OPCODE_SUB:
		src1_flags = IR3_REG_NEGATE;
		break;
	}

	switch (t->opc) {
	case OPC_ABSNEG_F:
	case OPC_ABSNEG_S:
	case OPC_CLZ_B:
	case OPC_CLZ_S:
	case OPC_SIGN_F:
	case OPC_FLOOR_F:
	case OPC_CEIL_F:
	case OPC_RNDNE_F:
	case OPC_RNDAZ_F:
	case OPC_TRUNC_F:
	case OPC_NOT_B:
	case OPC_BFREV_B:
	case OPC_SETRM:
	case OPC_CBITS_B:
		/* these only have one src reg */
		instr = instr_create(ctx, 2, t->opc);
		vectorize(ctx, instr, dst, 1, src0, src0_flags);
		break;
	default:
		if (is_const(src0) && is_const(src1))
			src0 = get_unconst(ctx, src0);

		instr = instr_create(ctx, 2, t->opc);
		vectorize(ctx, instr, dst, 2, src0, src0_flags,
				src1, src1_flags);
		break;
	}

	put_dst(ctx, inst, dst);
}

static void
instr_cat3(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *src0 = &inst->Src[0].Register;
	struct tgsi_src_register *src1 = &inst->Src[1].Register;
	struct ir3_instruction *instr;

	/* in particular, can't handle const for src1 for cat3..
	 * for mad, we can swap first two src's if needed:
	 */
	if (is_rel_or_const(src1)) {
		if (is_mad(t->opc) && !is_rel_or_const(src0)) {
			struct tgsi_src_register *tmp;
			tmp = src0;
			src0 = src1;
			src1 = tmp;
		} else {
			src1 = get_unconst(ctx, src1);
		}
	}

	instr = instr_create(ctx, 3,
			ctx->so->key.half_precision ? t->hopc : t->opc);
	vectorize(ctx, instr, dst, 3, src0, 0, src1, 0,
			&inst->Src[2].Register, 0);
	put_dst(ctx, inst, dst);
}

static void
instr_cat4(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *src = &inst->Src[0].Register;
	struct ir3_instruction *instr;
	unsigned i, n;

	/* seems like blob compiler avoids const as src.. */
	if (is_const(src))
		src = get_unconst(ctx, src);

	/* worst case: */
	add_nop(ctx, 6);

	/* we need to replicate into each component: */
	for (i = 0, n = 0; i < 4; i++) {
		if (dst->WriteMask & (1 << i)) {
			if (n++)
				add_nop(ctx, 1);
			instr = instr_create(ctx, 4, t->opc);
			add_dst_reg(ctx, instr, dst, i);
			add_src_reg(ctx, instr, src, src->SwizzleX);
		}
	}

	regmask_set(&ctx->needs_ss, instr->regs[0]);
	put_dst(ctx, inst, dst);
}

static const struct instr_translater translaters[TGSI_OPCODE_LAST] = {
#define INSTR(n, f, ...) \
	[TGSI_OPCODE_ ## n] = { .fxn = (f), .tgsi_opc = TGSI_OPCODE_ ## n, ##__VA_ARGS__ }

	INSTR(MOV,          instr_cat1),
	INSTR(RCP,          instr_cat4, .opc = OPC_RCP),
	INSTR(RSQ,          instr_cat4, .opc = OPC_RSQ),
	INSTR(SQRT,         instr_cat4, .opc = OPC_SQRT),
	INSTR(MUL,          instr_cat2, .opc = OPC_MUL_F),
	INSTR(ADD,          instr_cat2, .opc = OPC_ADD_F),
	INSTR(SUB,          instr_cat2, .opc = OPC_ADD_F),
	INSTR(MIN,          instr_cat2, .opc = OPC_MIN_F),
	INSTR(MAX,          instr_cat2, .opc = OPC_MAX_F),
	INSTR(MAD,          instr_cat3, .opc = OPC_MAD_F32, .hopc = OPC_MAD_F16),
	INSTR(TRUNC,        instr_cat2, .opc = OPC_TRUNC_F),
	INSTR(CLAMP,        trans_clamp),
	INSTR(FLR,          instr_cat2, .opc = OPC_FLOOR_F),
	INSTR(ROUND,        instr_cat2, .opc = OPC_RNDNE_F),
	INSTR(SSG,          instr_cat2, .opc = OPC_SIGN_F),
	INSTR(ARL,          trans_arl),
	INSTR(EX2,          instr_cat4, .opc = OPC_EXP2),
	INSTR(LG2,          instr_cat4, .opc = OPC_LOG2),
	INSTR(ABS,          instr_cat2, .opc = OPC_ABSNEG_F),
	INSTR(COS,          instr_cat4, .opc = OPC_COS),
	INSTR(SIN,          instr_cat4, .opc = OPC_SIN),
	INSTR(TEX,          trans_samp, .opc = OPC_SAM, .arg = TGSI_OPCODE_TEX),
	INSTR(TXP,          trans_samp, .opc = OPC_SAM, .arg = TGSI_OPCODE_TXP),
	INSTR(SGT,          trans_cmp),
	INSTR(SLT,          trans_cmp),
	INSTR(SGE,          trans_cmp),
	INSTR(SLE,          trans_cmp),
	INSTR(SNE,          trans_cmp),
	INSTR(SEQ,          trans_cmp),
	INSTR(CMP,          trans_cmp),
	INSTR(IF,           trans_if),
	INSTR(ELSE,         trans_else),
	INSTR(ENDIF,        trans_endif),
	INSTR(END,          instr_cat0, .opc = OPC_END),
	INSTR(KILL,         instr_cat0, .opc = OPC_KILL),
};

static fd3_semantic
decl_semantic(const struct tgsi_declaration_semantic *sem)
{
	return fd3_semantic_name(sem->Name, sem->Index);
}

static int
decl_in(struct fd3_compile_context *ctx, struct tgsi_full_declaration *decl)
{
	struct fd3_shader_variant *so = ctx->so;
	unsigned base = ctx->base_reg[TGSI_FILE_INPUT];
	unsigned i, flags = 0;
	int nop = 0;

	/* I don't think we should get frag shader input without
	 * semantic info?  Otherwise how do inputs get linked to
	 * vert outputs?
	 */
	compile_assert(ctx, (ctx->type == TGSI_PROCESSOR_VERTEX) ||
			decl->Declaration.Semantic);

	if (ctx->so->key.half_precision)
		flags |= IR3_REG_HALF;

	for (i = decl->Range.First; i <= decl->Range.Last; i++) {
		unsigned n = so->inputs_count++;
		unsigned r = regid(i + base, 0);
		unsigned ncomp;

		/* TODO use ctx->info.input_usage_mask[decl->Range.n] to figure out ncomp: */
		ncomp = 4;

		DBG("decl in -> r%d", i + base);   // XXX

		compile_assert(ctx, n < ARRAY_SIZE(so->inputs));

		so->inputs[n].semantic = decl_semantic(&decl->Semantic);
		so->inputs[n].compmask = (1 << ncomp) - 1;
		so->inputs[n].ncomp = ncomp;
		so->inputs[n].regid = r;
		so->inputs[n].inloc = ctx->next_inloc;
		so->inputs[n].bary = true;   /* all that is supported */
		ctx->next_inloc += ncomp;

		so->total_in += ncomp;

		/* for frag shaders, we need to generate the corresponding bary instr: */
		if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
			unsigned j;

			for (j = 0; j < ncomp; j++) {
				struct ir3_instruction *instr;
				struct ir3_register *dst;

				instr = instr_create(ctx, 2, OPC_BARY_F);

				/* dst register: */
				dst = ir3_reg_create(instr, r + j, flags);
				ctx->last_input = dst;

				/* input position: */
				ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val =
						so->inputs[n].inloc + j - 8;

				/* input base (always r0.xy): */
				ir3_reg_create(instr, regid(0,0), 0)->wrmask = 0x3;
			}

			nop = 6;
		}
	}

	return nop;
}

static void
decl_out(struct fd3_compile_context *ctx, struct tgsi_full_declaration *decl)
{
	struct fd3_shader_variant *so = ctx->so;
	unsigned base = ctx->base_reg[TGSI_FILE_OUTPUT];
	unsigned comp = 0;
	unsigned name = decl->Semantic.Name;
	unsigned i;

	compile_assert(ctx, decl->Declaration.Semantic);  // TODO is this ever not true?

	DBG("decl out[%d] -> r%d", name, decl->Range.First + base);   // XXX

	if (ctx->type == TGSI_PROCESSOR_VERTEX) {
		switch (name) {
		case TGSI_SEMANTIC_POSITION:
			so->writes_pos = true;
			break;
		case TGSI_SEMANTIC_PSIZE:
			so->writes_psize = true;
			break;
		case TGSI_SEMANTIC_COLOR:
		case TGSI_SEMANTIC_BCOLOR:
		case TGSI_SEMANTIC_GENERIC:
		case TGSI_SEMANTIC_FOG:
		case TGSI_SEMANTIC_TEXCOORD:
			break;
		default:
			compile_error(ctx, "unknown VS semantic name: %s\n",
					tgsi_semantic_names[name]);
		}
	} else {
		switch (name) {
		case TGSI_SEMANTIC_POSITION:
			comp = 2;  /* tgsi will write to .z component */
			so->writes_pos = true;
			break;
		case TGSI_SEMANTIC_COLOR:
			break;
		default:
			compile_error(ctx, "unknown FS semantic name: %s\n",
					tgsi_semantic_names[name]);
		}
	}

	for (i = decl->Range.First; i <= decl->Range.Last; i++) {
		unsigned n = so->outputs_count++;
		compile_assert(ctx, n < ARRAY_SIZE(so->outputs));
		so->outputs[n].semantic = decl_semantic(&decl->Semantic);
		so->outputs[n].regid = regid(i + base, comp);
	}
}

static void
decl_samp(struct fd3_compile_context *ctx, struct tgsi_full_declaration *decl)
{
	ctx->so->has_samp = true;
}

static void
compile_instructions(struct fd3_compile_context *ctx)
{
	struct ir3_shader *ir = ctx->ir;
	int nop = 0;

	while (!tgsi_parse_end_of_tokens(&ctx->parser)) {
		tgsi_parse_token(&ctx->parser);

		switch (ctx->parser.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION: {
			struct tgsi_full_declaration *decl =
					&ctx->parser.FullToken.FullDeclaration;
			if (decl->Declaration.File == TGSI_FILE_OUTPUT) {
				decl_out(ctx, decl);
			} else if (decl->Declaration.File == TGSI_FILE_INPUT) {
				nop = decl_in(ctx, decl);
			} else if (decl->Declaration.File == TGSI_FILE_SAMPLER) {
				decl_samp(ctx, decl);
			}
			break;
		}
		case TGSI_TOKEN_TYPE_IMMEDIATE: {
			/* TODO: if we know the immediate is small enough, and only
			 * used with instructions that can embed an immediate, we
			 * can skip this:
			 */
			struct tgsi_full_immediate *imm =
					&ctx->parser.FullToken.FullImmediate;
			unsigned n = ctx->so->immediates_count++;
			memcpy(ctx->so->immediates[n].val, imm->u, 16);
			break;
		}
		case TGSI_TOKEN_TYPE_INSTRUCTION: {
			struct tgsi_full_instruction *inst =
					&ctx->parser.FullToken.FullInstruction;
			unsigned opc = inst->Instruction.Opcode;
			const struct instr_translater *t = &translaters[opc];

			add_nop(ctx, nop);
			nop = 0;

			if (t->fxn) {
				t->fxn(t, ctx, inst);
				ctx->num_internal_temps = 0;
			} else {
				compile_error(ctx, "unknown TGSI opc: %s\n",
						tgsi_get_opcode_name(opc));
			}

			switch (inst->Instruction.Saturate) {
			case TGSI_SAT_ZERO_ONE:
				create_clamp_imm(ctx, &inst->Dst[0].Register,
						fui(0.0), fui(1.0));
				break;
			case TGSI_SAT_MINUS_PLUS_ONE:
				create_clamp_imm(ctx, &inst->Dst[0].Register,
						fui(-1.0), fui(1.0));
				break;
			}

			break;
		}
		default:
			break;
		}
	}

	if (ir->instrs_count > 0)
		ir->instrs[0]->flags |= IR3_INSTR_SS | IR3_INSTR_SY;

	if (ctx->last_input)
		ctx->last_input->flags |= IR3_REG_EI;

	handle_last_rel(ctx);
}

int
fd3_compile_shader_old(struct fd3_shader_variant *so,
		const struct tgsi_token *tokens, struct fd3_shader_key key)
{
	struct fd3_compile_context ctx;

	assert(!so->ir);

	so->ir = ir3_shader_create();

	assert(so->ir);

	if (compile_init(&ctx, so, tokens) != TGSI_PARSE_OK)
		return -1;

	compile_instructions(&ctx);

	compile_free(&ctx);

	return 0;
}
