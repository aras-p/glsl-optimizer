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

/* NOTE on half/full precision:
 * Currently, the front end (ie. basically this file) does everything in
 * full precision (with the exception of trans_arl() which doesn't work
 * currently.. we reject anything with relative addressing and fallback
 * to old compiler).
 *
 * In the RA step, if half_precision, it will assign the output to hr0.x
 * but use full precision everywhere else.
 *
 * Eventually we'll need a better way to communicate type information
 * to RA so that it can more properly assign both half and full precision
 * registers.  (And presumably double precision pairs for a4xx?)  This
 * would let us make more use of half precision registers, while still
 * keeping things like tex coords in full precision registers.
 *
 * Since the RA is dealing with patching instruction types for half
 * precision output, we can ignore that in the front end and just always
 * create full precision instructions.
 */

struct fd3_compile_context {
	const struct tgsi_token *tokens;
	bool free_tokens;
	struct ir3_shader *ir;
	struct fd3_shader_variant *so;

	struct ir3_block *block;
	struct ir3_instruction *current_instr;

	/* we need to defer updates to block->outputs[] until the end
	 * of an instruction (so we don't see new value until *after*
	 * the src registers are processed)
	 */
	struct {
		struct ir3_instruction *instr, **instrp;
	} output_updates[16];
	unsigned num_output_updates;

	/* are we in a sequence of "atomic" instructions?
	 */
	bool atomic;

	/* For fragment shaders, from the hw perspective the only
	 * actual input is r0.xy position register passed to bary.f.
	 * But TGSI doesn't know that, it still declares things as
	 * IN[] registers.  So we do all the input tracking normally
	 * and fix things up after compile_instructions()
	 *
	 * NOTE that frag_pos is the hardware position (possibly it
	 * is actually an index or tag or some such.. it is *not*
	 * values that can be directly used for gl_FragCoord..)
	 */
	struct ir3_instruction *frag_pos, *frag_face, *frag_coord[4];

	struct tgsi_parse_context parser;
	unsigned type;

	struct tgsi_shader_info info;

	/* for calculating input/output positions/linkages: */
	unsigned next_inloc;

	unsigned num_internal_temps;
	struct tgsi_src_register internal_temps[6];

	/* idx/slot for last compiler generated immediate */
	unsigned immediate_idx;

	/* stack of branch instructions that mark (potentially nested)
	 * branch if/else/loop/etc
	 */
	struct {
		struct ir3_instruction *instr, *cond;
		bool inv;   /* true iff in else leg of branch */
	} branch[16];
	unsigned int branch_count;

	/* list of kill instructions: */
	struct ir3_instruction *kill[16];
	unsigned int kill_count;

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
static type_t get_ftype(struct fd3_compile_context *ctx);

static unsigned
compile_init(struct fd3_compile_context *ctx, struct fd3_shader_variant *so,
		const struct tgsi_token *tokens)
{
	unsigned ret;
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
	ctx->so = so;
	ctx->next_inloc = 8;
	ctx->num_internal_temps = 0;
	ctx->branch_count = 0;
	ctx->kill_count = 0;
	ctx->block = NULL;
	ctx->current_instr = NULL;
	ctx->num_output_updates = 0;
	ctx->atomic = false;
	ctx->frag_pos = NULL;
	ctx->frag_face = NULL;

	memset(ctx->frag_coord, 0, sizeof(ctx->frag_coord));

#define FM(x) (1 << TGSI_FILE_##x)
	/* optimize can't deal with relative addressing: */
	if (info->indirect_files & (FM(TEMPORARY) | FM(INPUT) |
			FM(OUTPUT) | FM(IMMEDIATE) | FM(CONSTANT)))
		return TGSI_PARSE_ERROR;

	/* Immediates go after constants: */
	so->first_immediate = info->file_max[TGSI_FILE_CONSTANT] + 1;
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
instr_finish(struct fd3_compile_context *ctx)
{
	unsigned i;

	if (ctx->atomic)
		return;

	for (i = 0; i < ctx->num_output_updates; i++)
		*(ctx->output_updates[i].instrp) = ctx->output_updates[i].instr;

	ctx->num_output_updates = 0;
}

/* For "atomic" groups of instructions, for example the four scalar
 * instructions to perform a vec4 operation.  Basically this just
 * blocks out handling of output_updates so the next scalar instruction
 * still sees the result from before the start of the atomic group.
 *
 * NOTE: when used properly, this could probably replace get/put_dst()
 * stuff.
 */
static void
instr_atomic_start(struct fd3_compile_context *ctx)
{
	ctx->atomic = true;
}

static void
instr_atomic_end(struct fd3_compile_context *ctx)
{
	ctx->atomic = false;
	instr_finish(ctx);
}

static struct ir3_instruction *
instr_create(struct fd3_compile_context *ctx, int category, opc_t opc)
{
	instr_finish(ctx);
	return (ctx->current_instr = ir3_instr_create(ctx->block, category, opc));
}

static struct ir3_instruction *
instr_clone(struct fd3_compile_context *ctx, struct ir3_instruction *instr)
{
	instr_finish(ctx);
	return (ctx->current_instr = ir3_instr_clone(instr));
}

static struct ir3_block *
push_block(struct fd3_compile_context *ctx)
{
	struct ir3_block *block;
	unsigned ntmp, nin, nout;

#define SCALAR_REGS(file) (4 * (ctx->info.file_max[TGSI_FILE_ ## file] + 1))

	/* hmm, give ourselves room to create 4 extra temporaries (vec4):
	 */
	ntmp = SCALAR_REGS(TEMPORARY);
	ntmp += 4 * 4;

	nout = SCALAR_REGS(OUTPUT);
	nin  = SCALAR_REGS(INPUT);

	/* for outermost block, 'inputs' are the actual shader INPUT
	 * register file.  Reads from INPUT registers always go back to
	 * top block.  For nested blocks, 'inputs' is used to track any
	 * TEMPORARY file register from one of the enclosing blocks that
	 * is ready in this block.
	 */
	if (!ctx->block) {
		/* NOTE: fragment shaders actually have two inputs (r0.xy, the
		 * position)
		 */
		if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
			int n = 2;
			if (ctx->info.reads_position)
				n += 4;
			if (ctx->info.uses_frontface)
				n += 4;
			nin = MAX2(n, nin);
			nout += ARRAY_SIZE(ctx->kill);
		}
	} else {
		nin = ntmp;
	}

	block = ir3_block_create(ctx->ir, ntmp, nin, nout);

	if ((ctx->type == TGSI_PROCESSOR_FRAGMENT) && !ctx->block)
		block->noutputs -= ARRAY_SIZE(ctx->kill);

	block->parent = ctx->block;
	ctx->block = block;

	return block;
}

static void
pop_block(struct fd3_compile_context *ctx)
{
	ctx->block = ctx->block->parent;
	compile_assert(ctx, ctx->block);
}

static void
ssa_dst(struct fd3_compile_context *ctx, struct ir3_instruction *instr,
		const struct tgsi_dst_register *dst, unsigned chan)
{
	unsigned n = regid(dst->Index, chan);
	unsigned idx = ctx->num_output_updates;

	compile_assert(ctx, idx < ARRAY_SIZE(ctx->output_updates));

	/* NOTE: defer update of temporaries[idx] or output[idx]
	 * until instr_finish(), so that if the current instruction
	 * reads the same TEMP/OUT[] it gets the old value:
	 *
	 * bleh.. this might be a bit easier to just figure out
	 * in instr_finish().  But at that point we've already
	 * lost information about OUTPUT vs TEMPORARY register
	 * file..
	 */

	switch (dst->File) {
	case TGSI_FILE_OUTPUT:
		compile_assert(ctx, n < ctx->block->noutputs);
		ctx->output_updates[idx].instrp = &ctx->block->outputs[n];
		ctx->output_updates[idx].instr = instr;
		ctx->num_output_updates++;
		break;
	case TGSI_FILE_TEMPORARY:
		compile_assert(ctx, n < ctx->block->ntemporaries);
		ctx->output_updates[idx].instrp = &ctx->block->temporaries[n];
		ctx->output_updates[idx].instr = instr;
		ctx->num_output_updates++;
		break;
	}
}

static struct ir3_instruction *
create_output(struct ir3_block *block, struct ir3_instruction *instr,
		unsigned n)
{
	struct ir3_instruction *out;

	out = ir3_instr_create(block, -1, OPC_META_OUTPUT);
	out->inout.block = block;
	ir3_reg_create(out, n, 0);
	if (instr)
		ir3_reg_create(out, 0, IR3_REG_SSA)->instr = instr;

	return out;
}

static struct ir3_instruction *
create_input(struct ir3_block *block, struct ir3_instruction *instr,
		unsigned n)
{
	struct ir3_instruction *in;

	in = ir3_instr_create(block, -1, OPC_META_INPUT);
	in->inout.block = block;
	ir3_reg_create(in, n, 0);
	if (instr)
		ir3_reg_create(in, 0, IR3_REG_SSA)->instr = instr;

	return in;
}

static struct ir3_instruction *
block_input(struct ir3_block *block, unsigned n)
{
	/* references to INPUT register file always go back up to
	 * top level:
	 */
	if (block->parent)
		return block_input(block->parent, n);
	return block->inputs[n];
}

/* return temporary in scope, creating if needed meta-input node
 * to track block inputs
 */
static struct ir3_instruction *
block_temporary(struct ir3_block *block, unsigned n)
{
	/* references to TEMPORARY register file, find the nearest
	 * enclosing block which has already assigned this temporary,
	 * creating meta-input instructions along the way to keep
	 * track of block inputs
	 */
	if (block->parent && !block->temporaries[n]) {
		/* if already have input for this block, reuse: */
		if (!block->inputs[n])
			block->inputs[n] = block_temporary(block->parent, n);

		/* and create new input to return: */
		return create_input(block, block->inputs[n], n);
	}
	return block->temporaries[n];
}

static struct ir3_instruction *
create_immed(struct fd3_compile_context *ctx, float val)
{
	/* this can happen when registers (or components of a TGSI
	 * register) are used as src before they have been assigned
	 * (undefined contents).  To avoid confusing the rest of the
	 * compiler, and to generally keep things peachy, substitute
	 * an instruction that sets the src to 0.0.  Or to keep
	 * things undefined, I could plug in a random number? :-P
	 *
	 * NOTE: *don't* use instr_create() here!
	 */
	struct ir3_instruction *instr;
	instr = ir3_instr_create(ctx->block, 1, 0);
	instr->cat1.src_type = get_ftype(ctx);
	instr->cat1.dst_type = get_ftype(ctx);
	ir3_reg_create(instr, 0, 0);
	ir3_reg_create(instr, 0, IR3_REG_IMMED)->fim_val = val;
	return instr;
}

static void
ssa_src(struct fd3_compile_context *ctx, struct ir3_register *reg,
		const struct tgsi_src_register *src, unsigned chan)
{
	struct ir3_block *block = ctx->block;
	unsigned n = regid(src->Index, chan);

	switch (src->File) {
	case TGSI_FILE_INPUT:
		reg->flags |= IR3_REG_SSA;
		reg->instr = block_input(ctx->block, n);
		break;
	case TGSI_FILE_OUTPUT:
		/* really this should just happen in case of 'MOV_SAT OUT[n], ..',
		 * for the following clamp instructions:
		 */
		reg->flags |= IR3_REG_SSA;
		reg->instr = block->outputs[n];
		/* we don't have to worry about read from an OUTPUT that was
		 * assigned outside of the current block, because the _SAT
		 * clamp instructions will always be in the same block as
		 * the original instruction which wrote the OUTPUT
		 */
		compile_assert(ctx, reg->instr);
		break;
	case TGSI_FILE_TEMPORARY:
		reg->flags |= IR3_REG_SSA;
		reg->instr = block_temporary(ctx->block, n);
		break;
	}

	if ((reg->flags & IR3_REG_SSA) && !reg->instr) {
		/* this can happen when registers (or components of a TGSI
		 * register) are used as src before they have been assigned
		 * (undefined contents).  To avoid confusing the rest of the
		 * compiler, and to generally keep things peachy, substitute
		 * an instruction that sets the src to 0.0.  Or to keep
		 * things undefined, I could plug in a random number? :-P
		 *
		 * NOTE: *don't* use instr_create() here!
		 */
		reg->instr = create_immed(ctx, 0.0);
	}
}

static struct ir3_register *
add_dst_reg_wrmask(struct fd3_compile_context *ctx,
		struct ir3_instruction *instr, const struct tgsi_dst_register *dst,
		unsigned chan, unsigned wrmask)
{
	unsigned flags = 0, num = 0;
	struct ir3_register *reg;

	switch (dst->File) {
	case TGSI_FILE_OUTPUT:
	case TGSI_FILE_TEMPORARY:
		/* uses SSA */
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

	reg = ir3_reg_create(instr, regid(num, chan), flags);

	/* NOTE: do not call ssa_dst() if atomic.. vectorize()
	 * itself will call ssa_dst().  This is to filter out
	 * the (initially bogus) .x component dst which is
	 * created (but not necessarily used, ie. if the net
	 * result of the vector operation does not write to
	 * the .x component)
	 */

	reg->wrmask = wrmask;
	if (wrmask == 0x1) {
		/* normal case */
		if (!ctx->atomic)
			ssa_dst(ctx, instr, dst, chan);
	} else if ((dst->File == TGSI_FILE_TEMPORARY) ||
			(dst->File == TGSI_FILE_OUTPUT)) {
		unsigned i;

		/* if instruction writes multiple, we need to create
		 * some place-holder collect the registers:
		 */
		for (i = 0; i < 4; i++) {
			if (wrmask & (1 << i)) {
				struct ir3_instruction *collect =
						ir3_instr_create(ctx->block, -1, OPC_META_FO);
				collect->fo.off = i;
				/* unused dst reg: */
				ir3_reg_create(collect, 0, 0);
				/* and src reg used to hold original instr */
				ir3_reg_create(collect, 0, IR3_REG_SSA)->instr = instr;
				if (!ctx->atomic)
					ssa_dst(ctx, collect, dst, chan+i);
			}
		}
	}

	return reg;
}

static struct ir3_register *
add_dst_reg(struct fd3_compile_context *ctx, struct ir3_instruction *instr,
		const struct tgsi_dst_register *dst, unsigned chan)
{
	return add_dst_reg_wrmask(ctx, instr, dst, chan, 0x1);
}

static struct ir3_register *
add_src_reg_wrmask(struct fd3_compile_context *ctx,
		struct ir3_instruction *instr, const struct tgsi_src_register *src,
		unsigned chan, unsigned wrmask)
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
		flags |= IR3_REG_CONST;
		num = src->Index + ctx->so->first_immediate;
		break;
	case TGSI_FILE_CONSTANT:
		flags |= IR3_REG_CONST;
		num = src->Index;
		break;
	case TGSI_FILE_OUTPUT:
		/* NOTE: we should only end up w/ OUTPUT file for things like
		 * clamp()'ing saturated dst instructions
		 */
	case TGSI_FILE_INPUT:
	case TGSI_FILE_TEMPORARY:
		/* uses SSA */
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

	reg = ir3_reg_create(instr, regid(num, chan), flags);

	reg->wrmask = wrmask;
	if (wrmask == 0x1) {
		/* normal case */
		ssa_src(ctx, reg, src, chan);
	} else if ((src->File == TGSI_FILE_TEMPORARY) ||
			(src->File == TGSI_FILE_OUTPUT) ||
			(src->File == TGSI_FILE_INPUT)) {
		struct ir3_instruction *collect;
		unsigned i;

		/* if instruction reads multiple, we need to create
		 * some place-holder collect the registers:
		 */
		collect = ir3_instr_create(ctx->block, -1, OPC_META_FI);
		ir3_reg_create(collect, 0, 0);   /* unused dst reg */

		for (i = 0; i < 4; i++) {
			if (wrmask & (1 << i)) {
				/* and src reg used point to the original instr */
				ssa_src(ctx, ir3_reg_create(collect, 0, IR3_REG_SSA),
						src, chan + i);
			} else if (wrmask & ~((i << i) - 1)) {
				/* if any remaining components, then dummy
				 * placeholder src reg to fill in the blanks:
				 */
				ir3_reg_create(collect, 0, 0);
			}
		}

		reg->flags |= IR3_REG_SSA;
		reg->instr = collect;
	}

	return reg;
}

static struct ir3_register *
add_src_reg(struct fd3_compile_context *ctx, struct ir3_instruction *instr,
		const struct tgsi_src_register *src, unsigned chan)
{
	return add_src_reg_wrmask(ctx, instr, src, chan, 0x1);
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
	return TYPE_F32;
}

static type_t
get_utype(struct fd3_compile_context *ctx)
{
	return TYPE_U32;
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

	instr_atomic_start(ctx);

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
				cur = instr_clone(ctx, instr);
			}

			ssa_dst(ctx, cur, dst, i);

			/* fix-up dst register component: */
			cur->regs[0]->num = regid(cur->regs[0]->num >> 2, i);

			/* fix-up src register component: */
			va_start(ap, nsrcs);
			for (j = 0; j < nsrcs; j++) {
				struct ir3_register *reg = cur->regs[j+1];
				struct tgsi_src_register *src =
						va_arg(ap, struct tgsi_src_register *);
				unsigned flags = va_arg(ap, unsigned);
				if (reg->flags & IR3_REG_SSA) {
					ssa_src(ctx, reg, src, src_swiz(src, i));
				} else if (!(flags & IR3_REG_IMMED)) {
					reg->num = regid(reg->num >> 2, src_swiz(src, i));
				}
			}
			va_end(ap);
		}
	}

	instr_atomic_end(ctx);
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

	tmp_src = get_internal_temp_hr(ctx, &tmp_dst);

	/* cov.{f32,f16}s16 Rtmp, Rsrc */
	instr = instr_create(ctx, 1, 0);
	instr->cat1.src_type = get_ftype(ctx);
	instr->cat1.dst_type = TYPE_S16;
	add_dst_reg(ctx, instr, &tmp_dst, chan)->flags |= IR3_REG_HALF;
	add_src_reg(ctx, instr, src, chan);

	/* shl.b Rtmp, Rtmp, 2 */
	instr = instr_create(ctx, 2, OPC_SHL_B);
	add_dst_reg(ctx, instr, &tmp_dst, chan)->flags |= IR3_REG_HALF;
	add_src_reg(ctx, instr, tmp_src, chan)->flags |= IR3_REG_HALF;
	ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = 2;

	/* mova a0, Rtmp */
	instr = instr_create(ctx, 1, 0);
	instr->cat1.src_type = TYPE_S16;
	instr->cat1.dst_type = TYPE_S16;
	add_dst_reg(ctx, instr, dst, 0)->flags |= IR3_REG_HALF;
	add_src_reg(ctx, instr, tmp_src, chan)->flags |= IR3_REG_HALF;
}

/*
 * texture fetch/sample instructions:
 */

struct tex_info {
	int8_t order[4];
	unsigned src_wrmask, flags;
};

static const struct tex_info *
get_tex_info(struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	static const struct tex_info tex1d = {
		.order = { 0, -1, -1, -1 },  /* coord.x */
		.src_wrmask = TGSI_WRITEMASK_XY,
		.flags = 0,
	};
	static const struct tex_info tex1ds = {
		.order = { 0, -1,  2, -1 },  /* coord.xz */
		.src_wrmask = TGSI_WRITEMASK_XYZ,
		.flags = IR3_INSTR_S,
	};
	static const struct tex_info tex2d = {
		.order = { 0,  1, -1, -1 },  /* coord.xy */
		.src_wrmask = TGSI_WRITEMASK_XY,
		.flags = 0,
	};
	static const struct tex_info tex2ds = {
		.order = { 0,  1,  2, -1 },  /* coord.xyz */
		.src_wrmask = TGSI_WRITEMASK_XYZ,
		.flags = IR3_INSTR_S,
	};
	static const struct tex_info tex3d = {
		.order = { 0,  1,  2, -1 },  /* coord.xyz */
		.src_wrmask = TGSI_WRITEMASK_XYZ,
		.flags = IR3_INSTR_3D,
	};
	static const struct tex_info tex3ds = {
		.order = { 0,  1,  2,  3 },  /* coord.xyzw */
		.src_wrmask = TGSI_WRITEMASK_XYZW,
		.flags = IR3_INSTR_S | IR3_INSTR_3D,
	};
	static const struct tex_info txp1d = {
		.order = { 0, -1,  3, -1 },  /* coord.xw */
		.src_wrmask = TGSI_WRITEMASK_XYZ,
		.flags = IR3_INSTR_P,
	};
	static const struct tex_info txp1ds = {
		.order = { 0, -1,  2,  3 },  /* coord.xzw */
		.src_wrmask = TGSI_WRITEMASK_XYZW,
		.flags = IR3_INSTR_P | IR3_INSTR_S,
	};
	static const struct tex_info txp2d = {
		.order = { 0,  1,  3, -1 },  /* coord.xyw */
		.src_wrmask = TGSI_WRITEMASK_XYZ,
		.flags = IR3_INSTR_P,
	};
	static const struct tex_info txp2ds = {
		.order = { 0,  1,  2,  3 },  /* coord.xyzw */
		.src_wrmask = TGSI_WRITEMASK_XYZW,
		.flags = IR3_INSTR_P | IR3_INSTR_S,
	};
	static const struct tex_info txp3d = {
		.order = { 0,  1,  2,  3 },  /* coord.xyzw */
		.src_wrmask = TGSI_WRITEMASK_XYZW,
		.flags = IR3_INSTR_P | IR3_INSTR_3D,
	};

	unsigned tex = inst->Texture.Texture;

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_TEX:
		switch (tex) {
		case TGSI_TEXTURE_1D:
			return &tex1d;
		case TGSI_TEXTURE_SHADOW1D:
			return &tex1ds;
		case TGSI_TEXTURE_2D:
		case TGSI_TEXTURE_RECT:
			return &tex2d;
		case TGSI_TEXTURE_SHADOW2D:
		case TGSI_TEXTURE_SHADOWRECT:
			return &tex2ds;
		case TGSI_TEXTURE_3D:
		case TGSI_TEXTURE_CUBE:
			return &tex3d;
		case TGSI_TEXTURE_SHADOWCUBE:
			return &tex3ds;
		default:
			compile_error(ctx, "unknown texture type: %s\n",
					tgsi_texture_names[tex]);
			return NULL;
		}
		break;
	case TGSI_OPCODE_TXP:
		switch (tex) {
		case TGSI_TEXTURE_1D:
			return &txp1d;
		case TGSI_TEXTURE_SHADOW1D:
			return &txp1ds;
		case TGSI_TEXTURE_2D:
		case TGSI_TEXTURE_RECT:
			return &txp2d;
		case TGSI_TEXTURE_SHADOW2D:
		case TGSI_TEXTURE_SHADOWRECT:
			return &txp2ds;
		case TGSI_TEXTURE_3D:
		case TGSI_TEXTURE_CUBE:
			return &txp3d;
		default:
			compile_error(ctx, "unknown texture type: %s\n",
					tgsi_texture_names[tex]);
			break;
		}
		break;
	}
	compile_assert(ctx, 0);
	return NULL;
}

static struct tgsi_src_register *
get_tex_coord(struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst,
		const struct tex_info *tinf)
{
	struct tgsi_src_register *coord = &inst->Src[0].Register;
	struct ir3_instruction *instr;
	unsigned tex = inst->Texture.Texture;
	bool needs_mov = false;
	unsigned i;

	/* cat5 instruction cannot seem to handle const or relative: */
	if (is_rel_or_const(coord))
		needs_mov = true;

	/* 1D textures we fix up w/ 0.0 as 2nd coord: */
	if ((tex == TGSI_TEXTURE_1D) || (tex == TGSI_TEXTURE_SHADOW1D))
		needs_mov = true;

	/* The texture sample instructions need to coord in successive
	 * registers/components (ie. src.xy but not src.yx).  And TXP
	 * needs the .w component in .z for 2D..  so in some cases we
	 * might need to emit some mov instructions to shuffle things
	 * around:
	 */
	for (i = 1; (i < 4) && (tinf->order[i] >= 0) && !needs_mov; i++)
		if (src_swiz(coord, i) != (src_swiz(coord, 0) + tinf->order[i]))
			needs_mov = true;

	if (needs_mov) {
		struct tgsi_dst_register tmp_dst;
		struct tgsi_src_register *tmp_src;
		unsigned j;

		type_t type_mov = get_ftype(ctx);

		/* need to move things around: */
		tmp_src = get_internal_temp(ctx, &tmp_dst);

		for (j = 0; j < 4; j++) {
			if (tinf->order[j] < 0)
				continue;
			instr = instr_create(ctx, 1, 0);  /* mov */
			instr->cat1.src_type = type_mov;
			instr->cat1.dst_type = type_mov;
			add_dst_reg(ctx, instr, &tmp_dst, j);
			add_src_reg(ctx, instr, coord,
					src_swiz(coord, tinf->order[j]));
		}

		/* fix up .y coord: */
		if ((tex == TGSI_TEXTURE_1D) ||
				(tex == TGSI_TEXTURE_SHADOW1D)) {
			instr = instr_create(ctx, 1, 0);  /* mov */
			instr->cat1.src_type = type_mov;
			instr->cat1.dst_type = type_mov;
			add_dst_reg(ctx, instr, &tmp_dst, 1);  /* .y */
			ir3_reg_create(instr, 0, IR3_REG_IMMED)->fim_val = 0.5;
		}

		coord = tmp_src;
	}

	return coord;
}

static void
trans_samp(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_dst_register *dst = &inst->Dst[0].Register;
	struct tgsi_src_register *coord;
	struct tgsi_src_register *samp  = &inst->Src[1].Register;
	const struct tex_info *tinf;

	tinf = get_tex_info(ctx, inst);
	coord = get_tex_coord(ctx, inst, tinf);

	instr = instr_create(ctx, 5, t->opc);
	instr->cat5.type = get_ftype(ctx);
	instr->cat5.samp = samp->Index;
	instr->cat5.tex  = samp->Index;
	instr->flags |= tinf->flags;

	add_dst_reg_wrmask(ctx, instr, dst, 0, dst->WriteMask);
	add_src_reg_wrmask(ctx, instr, coord, coord->SwizzleX, tinf->src_wrmask);
}

/*
 * SEQ(a,b) = (a == b) ? 1.0 : 0.0
 *   cmps.f.eq tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * SNE(a,b) = (a != b) ? 1.0 : 0.0
 *   cmps.f.ne tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * SGE(a,b) = (a >= b) ? 1.0 : 0.0
 *   cmps.f.ge tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * SLE(a,b) = (a <= b) ? 1.0 : 0.0
 *   cmps.f.le tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * SGT(a,b) = (a > b)  ? 1.0 : 0.0
 *   cmps.f.gt tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * SLT(a,b) = (a < b)  ? 1.0 : 0.0
 *   cmps.f.lt tmp0, a, b
 *   cov.u16f16 dst, tmp0
 *
 * CMP(a,b,c) = (a < 0.0) ? b : c
 *   cmps.f.lt tmp0, a, {0.0}
 *   sel.b16 dst, b, tmp0, c
 */
static void
trans_cmp(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register *tmp_src;
	struct tgsi_src_register constval0;
	/* final instruction for CMP() uses orig src1 and src2: */
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *a0, *a1, *a2;
	unsigned condition;

	tmp_src = get_internal_temp(ctx, &tmp_dst);

	a0 = &inst->Src[0].Register;  /* a */
	a1 = &inst->Src[1].Register;  /* b */

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_SEQ:
	case TGSI_OPCODE_FSEQ:
		condition = IR3_COND_EQ;
		break;
	case TGSI_OPCODE_SNE:
	case TGSI_OPCODE_FSNE:
		condition = IR3_COND_NE;
		break;
	case TGSI_OPCODE_SGE:
	case TGSI_OPCODE_FSGE:
		condition = IR3_COND_GE;
		break;
	case TGSI_OPCODE_SLT:
	case TGSI_OPCODE_FSLT:
		condition = IR3_COND_LT;
		break;
	case TGSI_OPCODE_SLE:
		condition = IR3_COND_LE;
		break;
	case TGSI_OPCODE_SGT:
		condition = IR3_COND_GT;
		break;
	case TGSI_OPCODE_CMP:
		get_immediate(ctx, &constval0, fui(0.0));
		a0 = &inst->Src[0].Register;  /* a */
		a1 = &constval0;              /* {0.0} */
		condition = IR3_COND_LT;
		break;
	default:
		compile_assert(ctx, 0);
		return;
	}

	if (is_const(a0) && is_const(a1))
		a0 = get_unconst(ctx, a0);

	/* cmps.f.<cond> tmp, a0, a1 */
	instr = instr_create(ctx, 2, OPC_CMPS_F);
	instr->cat2.condition = condition;
	vectorize(ctx, instr, &tmp_dst, 2, a0, 0, a1, 0);

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_SEQ:
	case TGSI_OPCODE_FSEQ:
	case TGSI_OPCODE_SGE:
	case TGSI_OPCODE_FSGE:
	case TGSI_OPCODE_SLE:
	case TGSI_OPCODE_SNE:
	case TGSI_OPCODE_FSNE:
	case TGSI_OPCODE_SGT:
	case TGSI_OPCODE_SLT:
	case TGSI_OPCODE_FSLT:
		/* cov.u16f16 dst, tmp0 */
		instr = instr_create(ctx, 1, 0);
		instr->cat1.src_type = get_utype(ctx);
		instr->cat1.dst_type = get_ftype(ctx);
		vectorize(ctx, instr, dst, 1, tmp_src, 0);
		break;
	case TGSI_OPCODE_CMP:
		a1 = &inst->Src[1].Register;
		a2 = &inst->Src[2].Register;
		/* sel.{b32,b16} dst, src2, tmp, src1 */
		instr = instr_create(ctx, 3, OPC_SEL_B32);
		vectorize(ctx, instr, dst, 3, a1, 0, tmp_src, 0, a2, 0);

		break;
	}

	put_dst(ctx, inst, dst);
}

/*
 * USNE(a,b) = (a != b) ? 1 : 0
 *   cmps.u32.ne dst, a, b
 *
 * USEQ(a,b) = (a == b) ? 1 : 0
 *   cmps.u32.eq dst, a, b
 *
 * ISGE(a,b) = (a > b) ? 1 : 0
 *   cmps.s32.ge dst, a, b
 *
 * USGE(a,b) = (a > b) ? 1 : 0
 *   cmps.u32.ge dst, a, b
 *
 * ISLT(a,b) = (a < b) ? 1 : 0
 *   cmps.s32.lt dst, a, b
 *
 * USLT(a,b) = (a < b) ? 1 : 0
 *   cmps.u32.lt dst, a, b
 *
 * UCMP(a,b,c) = (a < 0) ? b : c
 *   cmps.u32.lt tmp0, a, {0}
 *   sel.b16 dst, b, tmp0, c
 */
static void
trans_icmp(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register constval0;
	struct tgsi_src_register *a0, *a1, *a2;
	unsigned condition;

	a0 = &inst->Src[0].Register;  /* a */
	a1 = &inst->Src[1].Register;  /* b */

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_USNE:
		condition = IR3_COND_NE;
		break;
	case TGSI_OPCODE_USEQ:
		condition = IR3_COND_EQ;
		break;
	case TGSI_OPCODE_ISGE:
	case TGSI_OPCODE_USGE:
		condition = IR3_COND_GE;
		break;
	case TGSI_OPCODE_ISLT:
	case TGSI_OPCODE_USLT:
		condition = IR3_COND_LT;
		break;
	case TGSI_OPCODE_UCMP:
		get_immediate(ctx, &constval0, 0);
		a0 = &inst->Src[0].Register;  /* a */
		a1 = &constval0;              /* {0} */
		condition = IR3_COND_LT;
		break;

	default:
		compile_assert(ctx, 0);
		return;
	}

	if (is_const(a0) && is_const(a1))
		a0 = get_unconst(ctx, a0);

	if (t->tgsi_opc == TGSI_OPCODE_UCMP) {
		struct tgsi_dst_register tmp_dst;
		struct tgsi_src_register *tmp_src;
		tmp_src = get_internal_temp(ctx, &tmp_dst);
		/* cmps.u32.lt tmp, a0, a1 */
		instr = instr_create(ctx, 2, t->opc);
		instr->cat2.condition = condition;
		vectorize(ctx, instr, &tmp_dst, 2, a0, 0, a1, 0);

		a1 = &inst->Src[1].Register;
		a2 = &inst->Src[2].Register;
		/* sel.{b32,b16} dst, src2, tmp, src1 */
		instr = instr_create(ctx, 3, OPC_SEL_B32);
		vectorize(ctx, instr, dst, 3, a1, 0, tmp_src, 0, a2, 0);
	} else {
		/* cmps.{u32,s32}.<cond> dst, a0, a1 */
		instr = instr_create(ctx, 2, t->opc);
		instr->cat2.condition = condition;
		vectorize(ctx, instr, dst, 2, a0, 0, a1, 0);
	}
	put_dst(ctx, inst, dst);
}

/*
 * Conditional / Flow control
 */

static void
push_branch(struct fd3_compile_context *ctx, bool inv,
		struct ir3_instruction *instr, struct ir3_instruction *cond)
{
	unsigned int idx = ctx->branch_count++;
	compile_assert(ctx, idx < ARRAY_SIZE(ctx->branch));
	ctx->branch[idx].instr = instr;
	ctx->branch[idx].inv = inv;
	/* else side of branch has same condition: */
	if (!inv)
		ctx->branch[idx].cond = cond;
}

static struct ir3_instruction *
pop_branch(struct fd3_compile_context *ctx)
{
	unsigned int idx = --ctx->branch_count;
	return ctx->branch[idx].instr;
}

static void
trans_if(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr, *cond;
	struct tgsi_src_register *src = &inst->Src[0].Register;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register *tmp_src;
	struct tgsi_src_register constval;

	get_immediate(ctx, &constval, fui(0.0));
	tmp_src = get_internal_temp(ctx, &tmp_dst);

	if (is_const(src))
		src = get_unconst(ctx, src);

	/* cmps.f.ne tmp0, b, {0.0} */
	instr = instr_create(ctx, 2, OPC_CMPS_F);
	add_dst_reg(ctx, instr, &tmp_dst, 0);
	add_src_reg(ctx, instr, src, src->SwizzleX);
	add_src_reg(ctx, instr, &constval, constval.SwizzleX);
	instr->cat2.condition = IR3_COND_NE;

	compile_assert(ctx, instr->regs[1]->flags & IR3_REG_SSA); /* because get_unconst() */
	cond = instr->regs[1]->instr;

	/* meta:flow tmp0 */
	instr = instr_create(ctx, -1, OPC_META_FLOW);
	ir3_reg_create(instr, 0, 0);  /* dummy dst */
	add_src_reg(ctx, instr, tmp_src, TGSI_SWIZZLE_X);

	push_branch(ctx, false, instr, cond);
	instr->flow.if_block = push_block(ctx);
}

static void
trans_else(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;

	pop_block(ctx);

	instr = pop_branch(ctx);

	compile_assert(ctx, (instr->category == -1) &&
			(instr->opc == OPC_META_FLOW));

	push_branch(ctx, true, instr, NULL);
	instr->flow.else_block = push_block(ctx);
}

static struct ir3_instruction *
find_temporary(struct ir3_block *block, unsigned n)
{
	if (block->parent && !block->temporaries[n])
		return find_temporary(block->parent, n);
	return block->temporaries[n];
}

static struct ir3_instruction *
find_output(struct ir3_block *block, unsigned n)
{
	if (block->parent && !block->outputs[n])
		return find_output(block->parent, n);
	return block->outputs[n];
}

static struct ir3_instruction *
create_phi(struct fd3_compile_context *ctx, struct ir3_instruction *cond,
		struct ir3_instruction *a, struct ir3_instruction *b)
{
	struct ir3_instruction *phi;

	compile_assert(ctx, cond);

	/* Either side of the condition could be null..  which
	 * indicates a variable written on only one side of the
	 * branch.  Normally this should only be variables not
	 * used outside of that side of the branch.  So we could
	 * just 'return a ? a : b;' in that case.  But for better
	 * defined undefined behavior we just stick in imm{0.0}.
	 * In the common case of a value only used within the
	 * one side of the branch, the PHI instruction will not
	 * get scheduled
	 */
	if (!a)
		a = create_immed(ctx, 0.0);
	if (!b)
		b = create_immed(ctx, 0.0);

	phi = instr_create(ctx, -1, OPC_META_PHI);
	ir3_reg_create(phi, 0, 0);  /* dummy dst */
	ir3_reg_create(phi, 0, IR3_REG_SSA)->instr = cond;
	ir3_reg_create(phi, 0, IR3_REG_SSA)->instr = a;
	ir3_reg_create(phi, 0, IR3_REG_SSA)->instr = b;

	return phi;
}

static void
trans_endif(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct ir3_block *ifb, *elseb;
	struct ir3_instruction **ifout, **elseout;
	unsigned i, ifnout = 0, elsenout = 0;

	pop_block(ctx);

	instr = pop_branch(ctx);

	compile_assert(ctx, (instr->category == -1) &&
			(instr->opc == OPC_META_FLOW));

	ifb = instr->flow.if_block;
	elseb = instr->flow.else_block;
	/* if there is no else block, the parent block is used for the
	 * branch-not-taken src of the PHI instructions:
	 */
	if (!elseb)
		elseb = ifb->parent;

	/* worst case sizes: */
	ifnout = ifb->ntemporaries + ifb->noutputs;
	elsenout = elseb->ntemporaries + elseb->noutputs;

	ifout = ir3_alloc(ctx->ir, sizeof(ifb->outputs[0]) * ifnout);
	if (elseb != ifb->parent)
		elseout = ir3_alloc(ctx->ir, sizeof(ifb->outputs[0]) * elsenout);

	ifnout = 0;
	elsenout = 0;

	/* generate PHI instructions for any temporaries written: */
	for (i = 0; i < ifb->ntemporaries; i++) {
		struct ir3_instruction *a = ifb->temporaries[i];
		struct ir3_instruction *b = elseb->temporaries[i];

		/* if temporary written in if-block, or if else block
		 * is present and temporary written in else-block:
		 */
		if (a || ((elseb != ifb->parent) && b)) {
			struct ir3_instruction *phi;

			/* if only written on one side, find the closest
			 * enclosing update on other side:
			 */
			if (!a)
				a = find_temporary(ifb, i);
			if (!b)
				b = find_temporary(elseb, i);

			ifout[ifnout] = a;
			a = create_output(ifb, a, ifnout++);

			if (elseb != ifb->parent) {
				elseout[elsenout] = b;
				b = create_output(elseb, b, elsenout++);
			}

			phi = create_phi(ctx, instr, a, b);
			ctx->block->temporaries[i] = phi;
		}
	}

	compile_assert(ctx, ifb->noutputs == elseb->noutputs);

	/* .. and any outputs written: */
	for (i = 0; i < ifb->noutputs; i++) {
		struct ir3_instruction *a = ifb->outputs[i];
		struct ir3_instruction *b = elseb->outputs[i];

		/* if output written in if-block, or if else block
		 * is present and output written in else-block:
		 */
		if (a || ((elseb != ifb->parent) && b)) {
			struct ir3_instruction *phi;

			/* if only written on one side, find the closest
			 * enclosing update on other side:
			 */
			if (!a)
				a = find_output(ifb, i);
			if (!b)
				b = find_output(elseb, i);

			ifout[ifnout] = a;
			a = create_output(ifb, a, ifnout++);

			if (elseb != ifb->parent) {
				elseout[elsenout] = b;
				b = create_output(elseb, b, elsenout++);
			}

			phi = create_phi(ctx, instr, a, b);
			ctx->block->outputs[i] = phi;
		}
	}

	ifb->noutputs = ifnout;
	ifb->outputs = ifout;

	if (elseb != ifb->parent) {
		elseb->noutputs = elsenout;
		elseb->outputs = elseout;
	}

	// TODO maybe we want to compact block->inputs?
}

/*
 * Kill
 */

static void
trans_kill(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr, *immed, *cond = NULL;
	bool inv = false;

	switch (t->tgsi_opc) {
	case TGSI_OPCODE_KILL:
		/* unconditional kill, use enclosing if condition: */
		if (ctx->branch_count > 0) {
			unsigned int idx = ctx->branch_count - 1;
			cond = ctx->branch[idx].cond;
			inv = ctx->branch[idx].inv;
		} else {
			cond = create_immed(ctx, 1.0);
		}

		break;
	}

	compile_assert(ctx, cond);

	immed = create_immed(ctx, 0.0);

	/* cmps.f.ne p0.x, cond, {0.0} */
	instr = instr_create(ctx, 2, OPC_CMPS_F);
	instr->cat2.condition = IR3_COND_NE;
	ir3_reg_create(instr, regid(REG_P0, 0), 0);
	ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = cond;
	ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = immed;
	cond = instr;

	/* kill p0.x */
	instr = instr_create(ctx, 0, OPC_KILL);
	instr->cat0.inv = inv;
	ir3_reg_create(instr, 0, 0);  /* dummy dst */
	ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = cond;

	ctx->kill[ctx->kill_count++] = instr;
}

/*
 * Kill-If
 */

static void
trans_killif(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_src_register *src = &inst->Src[0].Register;
	struct ir3_instruction *instr, *immed, *cond = NULL;
	bool inv = false;

	immed = create_immed(ctx, 0.0);

	/* cmps.f.ne p0.x, cond, {0.0} */
	instr = instr_create(ctx, 2, OPC_CMPS_F);
	instr->cat2.condition = IR3_COND_NE;
	ir3_reg_create(instr, regid(REG_P0, 0), 0);
	ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = immed;
	add_src_reg(ctx, instr, src, src->SwizzleX);

	cond = instr;

	/* kill p0.x */
	instr = instr_create(ctx, 0, OPC_KILL);
	instr->cat0.inv = inv;
	ir3_reg_create(instr, 0, 0);  /* dummy dst */
	ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = cond;

	ctx->kill[ctx->kill_count++] = instr;

}
/*
 * I2F / U2F / F2I / F2U
 */

static void
trans_cov(const struct instr_translater *t,
		struct fd3_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct ir3_instruction *instr;
	struct tgsi_dst_register *dst = get_dst(ctx, inst);
	struct tgsi_src_register *src = &inst->Src[0].Register;

	// cov.f32s32 dst, tmp0 /
	instr = instr_create(ctx, 1, 0);
	switch (t->tgsi_opc) {
	case TGSI_OPCODE_U2F:
		instr->cat1.src_type = TYPE_U32;
		instr->cat1.dst_type = TYPE_F32;
		break;
	case TGSI_OPCODE_I2F:
		instr->cat1.src_type = TYPE_S32;
		instr->cat1.dst_type = TYPE_F32;
		break;
	case TGSI_OPCODE_F2U:
		instr->cat1.src_type = TYPE_F32;
		instr->cat1.dst_type = TYPE_U32;
		break;
	case TGSI_OPCODE_F2I:
		instr->cat1.src_type = TYPE_F32;
		instr->cat1.dst_type = TYPE_S32;
		break;

	}
	vectorize(ctx, instr, dst, 1, src, 0);
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
	create_mov(ctx, dst, src);
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
	case TGSI_OPCODE_IABS:
		src0_flags = IR3_REG_ABS;
		break;
	case TGSI_OPCODE_SUB:
	case TGSI_OPCODE_INEG:
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

	instr = instr_create(ctx, 3, t->opc);
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
	unsigned i;

	/* seems like blob compiler avoids const as src.. */
	if (is_const(src))
		src = get_unconst(ctx, src);

	/* we need to replicate into each component: */
	for (i = 0; i < 4; i++) {
		if (dst->WriteMask & (1 << i)) {
			instr = instr_create(ctx, 4, t->opc);
			add_dst_reg(ctx, instr, dst, i);
			add_src_reg(ctx, instr, src, src->SwizzleX);
		}
	}

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
	INSTR(UADD,         instr_cat2, .opc = OPC_ADD_U),
	INSTR(IMIN,         instr_cat2, .opc = OPC_MIN_S),
	INSTR(UMIN,         instr_cat2, .opc = OPC_MIN_U),
	INSTR(IMAX,         instr_cat2, .opc = OPC_MAX_S),
	INSTR(UMAX,         instr_cat2, .opc = OPC_MAX_U),
	INSTR(AND,          instr_cat2, .opc = OPC_AND_B),
	INSTR(OR,           instr_cat2, .opc = OPC_OR_B),
	INSTR(NOT,          instr_cat2, .opc = OPC_NOT_B),
	INSTR(XOR,          instr_cat2, .opc = OPC_XOR_B),
	INSTR(UMUL,         instr_cat2, .opc = OPC_MUL_U),
	INSTR(SHL,          instr_cat2, .opc = OPC_SHL_B),
	INSTR(USHR,         instr_cat2, .opc = OPC_SHR_B),
	INSTR(ISHR,         instr_cat2, .opc = OPC_ASHR_B),
	INSTR(IABS,         instr_cat2, .opc = OPC_ABSNEG_S),
	INSTR(INEG,         instr_cat2, .opc = OPC_ABSNEG_S),
	INSTR(AND,          instr_cat2, .opc = OPC_AND_B),
	INSTR(MAD,          instr_cat3, .opc = OPC_MAD_F32, .hopc = OPC_MAD_F16),
	INSTR(TRUNC,        instr_cat2, .opc = OPC_TRUNC_F),
	INSTR(CLAMP,        trans_clamp),
	INSTR(FLR,          instr_cat2, .opc = OPC_FLOOR_F),
	INSTR(ROUND,        instr_cat2, .opc = OPC_RNDNE_F),
	INSTR(SSG,          instr_cat2, .opc = OPC_SIGN_F),
	INSTR(CEIL,         instr_cat2, .opc = OPC_CEIL_F),
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
	INSTR(FSLT,         trans_cmp),
	INSTR(SGE,          trans_cmp),
	INSTR(FSGE,         trans_cmp),
	INSTR(SLE,          trans_cmp),
	INSTR(SNE,          trans_cmp),
	INSTR(FSNE,         trans_cmp),
	INSTR(SEQ,          trans_cmp),
	INSTR(FSEQ,         trans_cmp),
	INSTR(CMP,          trans_cmp),
	INSTR(USNE,         trans_icmp, .opc = OPC_CMPS_U),
	INSTR(USEQ,         trans_icmp, .opc = OPC_CMPS_U),
	INSTR(ISGE,         trans_icmp, .opc = OPC_CMPS_S),
	INSTR(USGE,         trans_icmp, .opc = OPC_CMPS_U),
	INSTR(ISLT,         trans_icmp, .opc = OPC_CMPS_S),
	INSTR(USLT,         trans_icmp, .opc = OPC_CMPS_U),
	INSTR(UCMP,         trans_icmp, .opc = OPC_CMPS_U),
	INSTR(IF,           trans_if),
	INSTR(UIF,          trans_if),
	INSTR(ELSE,         trans_else),
	INSTR(ENDIF,        trans_endif),
	INSTR(END,          instr_cat0, .opc = OPC_END),
	INSTR(KILL,         trans_kill, .opc = OPC_KILL),
	INSTR(KILL_IF,      trans_killif, .opc = OPC_KILL),
	INSTR(I2F,          trans_cov),
	INSTR(U2F,          trans_cov),
	INSTR(F2I,          trans_cov),
	INSTR(F2U,          trans_cov),
};

static fd3_semantic
decl_semantic(const struct tgsi_declaration_semantic *sem)
{
	return fd3_semantic_name(sem->Name, sem->Index);
}

static struct ir3_instruction *
decl_in_frag_bary(struct fd3_compile_context *ctx, unsigned regid,
		unsigned j, unsigned inloc)
{
	struct ir3_instruction *instr;
	struct ir3_register *src;

	/* bary.f dst, #inloc, r0.x */
	instr = instr_create(ctx, 2, OPC_BARY_F);
	ir3_reg_create(instr, regid, 0);   /* dummy dst */
	ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = inloc;
	src = ir3_reg_create(instr, 0, IR3_REG_SSA);
	src->wrmask = 0x3;
	src->instr = ctx->frag_pos;

	return instr;
}

/* TGSI_SEMANTIC_POSITION
 * """"""""""""""""""""""
 *
 * For fragment shaders, TGSI_SEMANTIC_POSITION is used to indicate that
 * fragment shader input contains the fragment's window position.  The X
 * component starts at zero and always increases from left to right.
 * The Y component starts at zero and always increases but Y=0 may either
 * indicate the top of the window or the bottom depending on the fragment
 * coordinate origin convention (see TGSI_PROPERTY_FS_COORD_ORIGIN).
 * The Z coordinate ranges from 0 to 1 to represent depth from the front
 * to the back of the Z buffer.  The W component contains the reciprocol
 * of the interpolated vertex position W component.
 */
static struct ir3_instruction *
decl_in_frag_coord(struct fd3_compile_context *ctx, unsigned regid,
		unsigned j)
{
	struct ir3_instruction *instr, *src;

	compile_assert(ctx, !ctx->frag_coord[j]);

	ctx->frag_coord[j] = create_input(ctx->block, NULL, 0);


	switch (j) {
	case 0: /* .x */
	case 1: /* .y */
		/* for frag_coord, we get unsigned values.. we need
		 * to subtract (integer) 8 and divide by 16 (right-
		 * shift by 4) then convert to float:
		 */

		/* add.s tmp, src, -8 */
		instr = instr_create(ctx, 2, OPC_ADD_S);
		ir3_reg_create(instr, regid, 0);    /* dummy dst */
		ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = ctx->frag_coord[j];
		ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = -8;
		src = instr;

		/* shr.b tmp, tmp, 4 */
		instr = instr_create(ctx, 2, OPC_SHR_B);
		ir3_reg_create(instr, regid, 0);    /* dummy dst */
		ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = src;
		ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = 4;
		src = instr;

		/* mov.u32f32 dst, tmp */
		instr = instr_create(ctx, 1, 0);
		instr->cat1.src_type = TYPE_U32;
		instr->cat1.dst_type = TYPE_F32;
		ir3_reg_create(instr, regid, 0);    /* dummy dst */
		ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = src;

		break;
	case 2: /* .z */
	case 3: /* .w */
		/* seems that we can use these as-is: */
		instr = ctx->frag_coord[j];
		break;
	default:
		compile_error(ctx, "invalid channel\n");
		instr = create_immed(ctx, 0.0);
		break;
	}

	return instr;
}

/* TGSI_SEMANTIC_FACE
 * """"""""""""""""""
 *
 * This label applies to fragment shader inputs only and indicates that
 * the register contains front/back-face information of the form (F, 0,
 * 0, 1).  The first component will be positive when the fragment belongs
 * to a front-facing polygon, and negative when the fragment belongs to a
 * back-facing polygon.
 */
static struct ir3_instruction *
decl_in_frag_face(struct fd3_compile_context *ctx, unsigned regid,
		unsigned j)
{
	struct ir3_instruction *instr, *src;

	switch (j) {
	case 0: /* .x */
		compile_assert(ctx, !ctx->frag_face);

		ctx->frag_face = create_input(ctx->block, NULL, 0);

		/* for faceness, we always get -1 or 0 (int).. but TGSI expects
		 * positive vs negative float.. and piglit further seems to
		 * expect -1.0 or 1.0:
		 *
		 *    mul.s tmp, hr0.x, 2
		 *    add.s tmp, tmp, 1
		 *    mov.s16f32, dst, tmp
		 *
		 */

		instr = instr_create(ctx, 2, OPC_MUL_S);
		ir3_reg_create(instr, regid, 0);    /* dummy dst */
		ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = ctx->frag_face;
		ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = 2;
		src = instr;

		instr = instr_create(ctx, 2, OPC_ADD_S);
		ir3_reg_create(instr, regid, 0);    /* dummy dst */
		ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = src;
		ir3_reg_create(instr, 0, IR3_REG_IMMED)->iim_val = 1;
		src = instr;

		instr = instr_create(ctx, 1, 0); /* mov */
		instr->cat1.src_type = TYPE_S32;
		instr->cat1.dst_type = TYPE_F32;
		ir3_reg_create(instr, regid, 0);    /* dummy dst */
		ir3_reg_create(instr, 0, IR3_REG_SSA)->instr = src;

		break;
	case 1: /* .y */
	case 2: /* .z */
		instr = create_immed(ctx, 0.0);
		break;
	case 3: /* .w */
		instr = create_immed(ctx, 1.0);
		break;
	default:
		compile_error(ctx, "invalid channel\n");
		instr = create_immed(ctx, 0.0);
		break;
	}

	return instr;
}

static void
decl_in(struct fd3_compile_context *ctx, struct tgsi_full_declaration *decl)
{
	struct fd3_shader_variant *so = ctx->so;
	unsigned name = decl->Semantic.Name;
	unsigned i;

	/* I don't think we should get frag shader input without
	 * semantic info?  Otherwise how do inputs get linked to
	 * vert outputs?
	 */
	compile_assert(ctx, (ctx->type == TGSI_PROCESSOR_VERTEX) ||
			decl->Declaration.Semantic);

	for (i = decl->Range.First; i <= decl->Range.Last; i++) {
		unsigned n = so->inputs_count++;
		unsigned r = regid(i, 0);
		unsigned ncomp, j;

		/* we'll figure out the actual components used after scheduling */
		ncomp = 4;

		DBG("decl in -> r%d", i);

		compile_assert(ctx, n < ARRAY_SIZE(so->inputs));

		so->inputs[n].semantic = decl_semantic(&decl->Semantic);
		so->inputs[n].compmask = (1 << ncomp) - 1;
		so->inputs[n].regid = r;
		so->inputs[n].inloc = ctx->next_inloc;

		for (j = 0; j < ncomp; j++) {
			struct ir3_instruction *instr = NULL;

			if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
				/* for fragment shaders, POSITION and FACE are handled
				 * specially, not using normal varying / bary.f
				 */
				if (name == TGSI_SEMANTIC_POSITION) {
					so->inputs[n].bary = false;
					so->frag_coord = true;
					instr = decl_in_frag_coord(ctx, r + j, j);
				} else if (name == TGSI_SEMANTIC_FACE) {
					so->inputs[n].bary = false;
					so->frag_face = true;
					instr = decl_in_frag_face(ctx, r + j, j);
				} else {
					so->inputs[n].bary = true;
					instr = decl_in_frag_bary(ctx, r + j, j,
							so->inputs[n].inloc + j - 8);
				}
			} else {
				instr = create_input(ctx->block, NULL, (i * 4) + j);
			}

			ctx->block->inputs[(i * 4) + j] = instr;
		}

		if (so->inputs[n].bary || (ctx->type == TGSI_PROCESSOR_VERTEX)) {
			ctx->next_inloc += ncomp;
			so->total_in += ncomp;
		}
	}
}

static void
decl_out(struct fd3_compile_context *ctx, struct tgsi_full_declaration *decl)
{
	struct fd3_shader_variant *so = ctx->so;
	unsigned comp = 0;
	unsigned name = decl->Semantic.Name;
	unsigned i;

	compile_assert(ctx, decl->Declaration.Semantic);

	DBG("decl out[%d] -> r%d", name, decl->Range.First);

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
		unsigned ncomp, j;

		ncomp = 4;

		compile_assert(ctx, n < ARRAY_SIZE(so->outputs));

		so->outputs[n].semantic = decl_semantic(&decl->Semantic);
		so->outputs[n].regid = regid(i, comp);

		/* avoid undefined outputs, stick a dummy mov from imm{0.0},
		 * which if the output is actually assigned will be over-
		 * written
		 */
		for (j = 0; j < ncomp; j++)
			ctx->block->outputs[(i * 4) + j] = create_immed(ctx, 0.0);
	}
}

/* from TGSI perspective, we actually have inputs.  But most of the "inputs"
 * for a fragment shader are just bary.f instructions.  The *actual* inputs
 * from the hw perspective are the frag_pos and optionally frag_coord and
 * frag_face.
 */
static void
fixup_frag_inputs(struct fd3_compile_context *ctx)
{
	struct fd3_shader_variant *so = ctx->so;
	struct ir3_block *block = ctx->block;
	struct ir3_instruction **inputs;
	struct ir3_instruction *instr;
	int n, regid = 0;

	block->ninputs = 0;

	n  = 4;  /* always have frag_pos */
	n += COND(so->frag_face, 4);
	n += COND(so->frag_coord, 4);

	inputs = ir3_alloc(ctx->ir, n * (sizeof(struct ir3_instruction *)));

	if (so->frag_face) {
		/* this ultimately gets assigned to hr0.x so doesn't conflict
		 * with frag_coord/frag_pos..
		 */
		inputs[block->ninputs++] = ctx->frag_face;
		ctx->frag_face->regs[0]->num = 0;

		/* remaining channels not used, but let's avoid confusing
		 * other parts that expect inputs to come in groups of vec4
		 */
		inputs[block->ninputs++] = NULL;
		inputs[block->ninputs++] = NULL;
		inputs[block->ninputs++] = NULL;
	}

	/* since we don't know where to set the regid for frag_coord,
	 * we have to use r0.x for it.  But we don't want to *always*
	 * use r1.x for frag_pos as that could increase the register
	 * footprint on simple shaders:
	 */
	if (so->frag_coord) {
		ctx->frag_coord[0]->regs[0]->num = regid++;
		ctx->frag_coord[1]->regs[0]->num = regid++;
		ctx->frag_coord[2]->regs[0]->num = regid++;
		ctx->frag_coord[3]->regs[0]->num = regid++;

		inputs[block->ninputs++] = ctx->frag_coord[0];
		inputs[block->ninputs++] = ctx->frag_coord[1];
		inputs[block->ninputs++] = ctx->frag_coord[2];
		inputs[block->ninputs++] = ctx->frag_coord[3];
	}

	/* we always have frag_pos: */
	so->pos_regid = regid;

	/* r0.x */
	instr = create_input(block, NULL, block->ninputs);
	instr->regs[0]->num = regid++;
	inputs[block->ninputs++] = instr;
	ctx->frag_pos->regs[1]->instr = instr;

	/* r0.y */
	instr = create_input(block, NULL, block->ninputs);
	instr->regs[0]->num = regid++;
	inputs[block->ninputs++] = instr;
	ctx->frag_pos->regs[2]->instr = instr;

	block->inputs = inputs;
}

static void
compile_instructions(struct fd3_compile_context *ctx)
{
	push_block(ctx);

	/* for fragment shader, we have a single input register (usually
	 * r0.xy) which is used as the base for bary.f varying fetch instrs:
	 */
	if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
		struct ir3_instruction *instr;
		instr = ir3_instr_create(ctx->block, -1, OPC_META_FI);
		ir3_reg_create(instr, 0, 0);
		ir3_reg_create(instr, 0, IR3_REG_SSA);    /* r0.x */
		ir3_reg_create(instr, 0, IR3_REG_SSA);    /* r0.y */
		ctx->frag_pos = instr;
	}

	while (!tgsi_parse_end_of_tokens(&ctx->parser)) {
		tgsi_parse_token(&ctx->parser);

		switch (ctx->parser.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION: {
			struct tgsi_full_declaration *decl =
					&ctx->parser.FullToken.FullDeclaration;
			if (decl->Declaration.File == TGSI_FILE_OUTPUT) {
				decl_out(ctx, decl);
			} else if (decl->Declaration.File == TGSI_FILE_INPUT) {
				decl_in(ctx, decl);
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
			compile_assert(ctx, n < ARRAY_SIZE(ctx->so->immediates));
			memcpy(ctx->so->immediates[n].val, imm->u, 16);
			break;
		}
		case TGSI_TOKEN_TYPE_INSTRUCTION: {
			struct tgsi_full_instruction *inst =
					&ctx->parser.FullToken.FullInstruction;
			unsigned opc = inst->Instruction.Opcode;
			const struct instr_translater *t = &translaters[opc];

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

			instr_finish(ctx);

			break;
		}
		default:
			break;
		}
	}
}

static void
compile_dump(struct fd3_compile_context *ctx)
{
	const char *name = (ctx->so->type == SHADER_VERTEX) ? "vert" : "frag";
	static unsigned n = 0;
	char fname[16];
	FILE *f;
	snprintf(fname, sizeof(fname), "%s-%04u.dot", name, n++);
	f = fopen(fname, "w");
	if (!f)
		return;
	ir3_block_depth(ctx->block);
	ir3_shader_dump(ctx->ir, name, ctx->block, f);
	fclose(f);
}

int
fd3_compile_shader(struct fd3_shader_variant *so,
		const struct tgsi_token *tokens, struct fd3_shader_key key)
{
	struct fd3_compile_context ctx;
	struct ir3_block *block;
	struct ir3_instruction **inputs;
	unsigned i, j, actual_in;
	int ret = 0;

	assert(!so->ir);

	so->ir = ir3_shader_create();

	assert(so->ir);

	if (compile_init(&ctx, so, tokens) != TGSI_PARSE_OK) {
		ret = -1;
		goto out;
	}

	compile_instructions(&ctx);

	block = ctx.block;

	/* keep track of the inputs from TGSI perspective.. */
	inputs = block->inputs;

	/* but fixup actual inputs for frag shader: */
	if (ctx.type == TGSI_PROCESSOR_FRAGMENT)
		fixup_frag_inputs(&ctx);

	/* at this point, for binning pass, throw away unneeded outputs: */
	if (key.binning_pass) {
		for (i = 0, j = 0; i < so->outputs_count; i++) {
			unsigned name = sem2name(so->outputs[i].semantic);
			unsigned idx = sem2name(so->outputs[i].semantic);

			/* throw away everything but first position/psize */
			if ((idx == 0) && ((name == TGSI_SEMANTIC_POSITION) ||
					(name == TGSI_SEMANTIC_PSIZE))) {
				if (i != j) {
					so->outputs[j] = so->outputs[i];
					block->outputs[(j*4)+0] = block->outputs[(i*4)+0];
					block->outputs[(j*4)+1] = block->outputs[(i*4)+1];
					block->outputs[(j*4)+2] = block->outputs[(i*4)+2];
					block->outputs[(j*4)+3] = block->outputs[(i*4)+3];
				}
				j++;
			}
		}
		so->outputs_count = j;
		block->noutputs = j * 4;
	}

	/* at this point, we want the kill's in the outputs array too,
	 * so that they get scheduled (since they have no dst).. we've
	 * already ensured that the array is big enough in push_block():
	 */
	if (ctx.type == TGSI_PROCESSOR_FRAGMENT) {
		for (i = 0; i < ctx.kill_count; i++)
			block->outputs[block->noutputs++] = ctx.kill[i];
	}

	if (fd_mesa_debug & FD_DBG_OPTDUMP)
		compile_dump(&ctx);

	ret = ir3_block_flatten(block);
	if (ret < 0)
		goto out;
	if ((ret > 0) && (fd_mesa_debug & FD_DBG_OPTDUMP))
		compile_dump(&ctx);

	ir3_block_cp(block);

	if (fd_mesa_debug & FD_DBG_OPTDUMP)
		compile_dump(&ctx);

	ir3_block_depth(block);

	if (fd_mesa_debug & FD_DBG_OPTMSGS) {
		printf("AFTER DEPTH:\n");
		ir3_dump_instr_list(block->head);
	}

	ir3_block_sched(block);

	if (fd_mesa_debug & FD_DBG_OPTMSGS) {
		printf("AFTER SCHED:\n");
		ir3_dump_instr_list(block->head);
	}

	ret = ir3_block_ra(block, so->type, key.half_precision,
			so->frag_coord, so->frag_face, &so->has_samp);
	if (ret)
		goto out;

	if (fd_mesa_debug & FD_DBG_OPTMSGS) {
		printf("AFTER RA:\n");
		ir3_dump_instr_list(block->head);
	}

	/* fixup input/outputs: */
	for (i = 0; i < so->outputs_count; i++) {
		so->outputs[i].regid = block->outputs[i*4]->regs[0]->num;
		/* preserve hack for depth output.. tgsi writes depth to .z,
		 * but what we give the hw is the scalar register:
		 */
		if ((ctx.type == TGSI_PROCESSOR_FRAGMENT) &&
			(sem2name(so->outputs[i].semantic) == TGSI_SEMANTIC_POSITION))
			so->outputs[i].regid += 2;
	}
	/* Note that some or all channels of an input may be unused: */
	actual_in = 0;
	for (i = 0; i < so->inputs_count; i++) {
		unsigned j, regid = ~0, compmask = 0;
		so->inputs[i].ncomp = 0;
		for (j = 0; j < 4; j++) {
			struct ir3_instruction *in = inputs[(i*4) + j];
			if (in) {
				compmask |= (1 << j);
				regid = in->regs[0]->num - j;
				actual_in++;
				so->inputs[i].ncomp++;
			}
		}
		so->inputs[i].regid = regid;
		so->inputs[i].compmask = compmask;
	}

	/* fragment shader always gets full vec4's even if it doesn't
	 * fetch all components, but vertex shader we need to update
	 * with the actual number of components fetch, otherwise thing
	 * will hang due to mismaptch between VFD_DECODE's and
	 * TOTALATTRTOVS
	 */
	if (so->type == SHADER_VERTEX)
		so->total_in = actual_in;

out:
	if (ret) {
		ir3_shader_destroy(so->ir);
		so->ir = NULL;
	}
	compile_free(&ctx);

	return ret;
}
