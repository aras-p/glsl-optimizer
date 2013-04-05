/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_ureg.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_strings.h"
#include "tgsi/tgsi_dump.h"

#include "freedreno_program.h"
#include "freedreno_compiler.h"
#include "freedreno_util.h"

#include "instr-a2xx.h"
#include "ir.h"

struct fd_compile_context {
	struct fd_program_stateobj *prog;
	struct fd_shader_stateobj *so;

	struct tgsi_parse_context parser;
	unsigned type;

	/* predicate stack: */
	int pred_depth;
	enum ir_pred pred_stack[8];

	/* Internal-Temporary and Predicate register assignment:
	 *
	 * Some TGSI instructions which translate into multiple actual
	 * instructions need one or more temporary registers (which are not
	 * assigned from TGSI perspective (ie. not TGSI_FILE_TEMPORARY).
	 * Whenever possible, the dst register is used as the first temporary,
	 * but this is not possible when the dst register is in an export (ie.
	 * in TGSI_FILE_OUTPUT).
	 *
	 * The predicate register must be valid across multiple TGSI
	 * instructions, but internal temporary's do not.  For this reason,
	 * once the predicate register is requested, until it is no longer
	 * needed, it gets the first register slot after after the TGSI
	 * assigned temporaries (ie. num_regs[TGSI_FILE_TEMPORARY]), and the
	 * internal temporaries get the register slots above this.
	 */

	int pred_reg;
	int num_internal_temps;

	uint8_t num_regs[TGSI_FILE_COUNT];

	/* maps input register idx to prog->export_linkage idx: */
	uint8_t input_export_idx[64];

	/* maps output register idx to prog->export_linkage idx: */
	uint8_t output_export_idx[64];

	/* idx/slot for last compiler generated immediate */
	unsigned immediate_idx;

	// TODO we can skip emit exports in the VS that the FS doesn't need..
	// and get rid perhaps of num_param..
	unsigned num_position, num_param;
	unsigned position, psize;

	uint64_t need_sync;

	/* current exec CF instruction */
	struct ir_cf *cf;
};

static int
semantic_idx(struct tgsi_declaration_semantic *semantic)
{
	int idx = semantic->Name;
	if (idx == TGSI_SEMANTIC_GENERIC)
		idx = TGSI_SEMANTIC_COUNT + semantic->Index;
	return idx;
}

/* assign/get the input/export register # for given semantic idx as
 * returned by semantic_idx():
 */
static int
export_linkage(struct fd_compile_context *ctx, int idx)
{
	struct fd_program_stateobj *prog = ctx->prog;

	/* if first time we've seen this export, assign the next available slot: */
	if (prog->export_linkage[idx] == 0xff)
		prog->export_linkage[idx] = prog->num_exports++;

	return prog->export_linkage[idx];
}

static unsigned
compile_init(struct fd_compile_context *ctx, struct fd_program_stateobj *prog,
		struct fd_shader_stateobj *so)
{
	unsigned ret;

	ctx->prog = prog;
	ctx->so = so;
	ctx->cf = NULL;
	ctx->pred_depth = 0;

	ret = tgsi_parse_init(&ctx->parser, so->tokens);
	if (ret != TGSI_PARSE_OK)
		return ret;

	ctx->type = ctx->parser.FullHeader.Processor.Processor;
	ctx->position = ~0;
	ctx->psize = ~0;
	ctx->num_position = 0;
	ctx->num_param = 0;
	ctx->need_sync = 0;
	ctx->immediate_idx = 0;
	ctx->pred_reg = -1;
	ctx->num_internal_temps = 0;

	memset(ctx->num_regs, 0, sizeof(ctx->num_regs));
	memset(ctx->input_export_idx, 0, sizeof(ctx->input_export_idx));
	memset(ctx->output_export_idx, 0, sizeof(ctx->output_export_idx));

	/* do first pass to extract declarations: */
	while (!tgsi_parse_end_of_tokens(&ctx->parser)) {
		tgsi_parse_token(&ctx->parser);

		switch (ctx->parser.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION: {
			struct tgsi_full_declaration *decl =
					&ctx->parser.FullToken.FullDeclaration;
			if (decl->Declaration.File == TGSI_FILE_OUTPUT) {
				unsigned name = decl->Semantic.Name;

				assert(decl->Declaration.Semantic);  // TODO is this ever not true?

				ctx->output_export_idx[decl->Range.First] =
						semantic_idx(&decl->Semantic);

				if (ctx->type == TGSI_PROCESSOR_VERTEX) {
					switch (name) {
					case TGSI_SEMANTIC_POSITION:
						ctx->position = ctx->num_regs[TGSI_FILE_OUTPUT];
						ctx->num_position++;
						break;
					case TGSI_SEMANTIC_PSIZE:
						ctx->psize = ctx->num_regs[TGSI_FILE_OUTPUT];
						ctx->num_position++;
					case TGSI_SEMANTIC_COLOR:
					case TGSI_SEMANTIC_GENERIC:
						ctx->num_param++;
						break;
					default:
						DBG("unknown VS semantic name: %s",
								tgsi_semantic_names[name]);
						assert(0);
					}
				} else {
					switch (name) {
					case TGSI_SEMANTIC_COLOR:
					case TGSI_SEMANTIC_GENERIC:
						ctx->num_param++;
						break;
					default:
						DBG("unknown PS semantic name: %s",
								tgsi_semantic_names[name]);
						assert(0);
					}
				}
			} else if (decl->Declaration.File == TGSI_FILE_INPUT) {
				ctx->input_export_idx[decl->Range.First] =
						semantic_idx(&decl->Semantic);
			}
			ctx->num_regs[decl->Declaration.File] +=
					1 + decl->Range.Last - decl->Range.First;
			break;
		}
		case TGSI_TOKEN_TYPE_IMMEDIATE: {
			struct tgsi_full_immediate *imm =
					&ctx->parser.FullToken.FullImmediate;
			unsigned n = ctx->so->num_immediates++;
			memcpy(ctx->so->immediates[n].val, imm->u, 16);
			break;
		}
		default:
			break;
		}
	}

	/* TGSI generated immediates are always entire vec4's, ones we
	 * generate internally are not:
	 */
	ctx->immediate_idx = ctx->so->num_immediates * 4;

	ctx->so->first_immediate = ctx->num_regs[TGSI_FILE_CONSTANT];

	tgsi_parse_free(&ctx->parser);

	return tgsi_parse_init(&ctx->parser, so->tokens);
}

static void
compile_free(struct fd_compile_context *ctx)
{
	tgsi_parse_free(&ctx->parser);
}

static struct ir_cf *
next_exec_cf(struct fd_compile_context *ctx)
{
	struct ir_cf *cf = ctx->cf;
	if (!cf || cf->exec.instrs_count >= ARRAY_SIZE(ctx->cf->exec.instrs))
		ctx->cf = cf = ir_cf_create(ctx->so->ir, EXEC);
	return cf;
}

static void
compile_vtx_fetch(struct fd_compile_context *ctx)
{
	struct ir_instruction **vfetch_instrs = ctx->so->vfetch_instrs;
	int i;
	for (i = 0; i < ctx->num_regs[TGSI_FILE_INPUT]; i++) {
		struct ir_instruction *instr = ir_instr_create(
				next_exec_cf(ctx), IR_FETCH);
		instr->fetch.opc = VTX_FETCH;

		ctx->need_sync |= 1 << (i+1);

		ir_reg_create(instr, i+1, "xyzw", 0);
		ir_reg_create(instr, 0, "x", 0);

		if (i == 0)
			instr->sync = true;

		vfetch_instrs[i] = instr;
	}
	ctx->so->num_vfetch_instrs = i;
	ctx->cf = NULL;
}

/*
 * For vertex shaders (VS):
 * --- ------ -------------
 *
 *   Inputs:     R1-R(num_input)
 *   Constants:  C0-C(num_const-1)
 *   Immediates: C(num_const)-C(num_const+num_imm-1)
 *   Outputs:    export0-export(n) and export62, export63
 *      n is # of outputs minus gl_Position (export62) and gl_PointSize (export63)
 *   Temps:      R(num_input+1)-R(num_input+num_temps)
 *
 * R0 could be clobbered after the vertex fetch instructions.. so we
 * could use it for one of the temporaries.
 *
 * TODO: maybe the vertex fetch part could fetch first input into R0 as
 * the last vtx fetch instruction, which would let us use the same
 * register layout in either case.. although this is not what the blob
 * compiler does.
 *
 *
 * For frag shaders (PS):
 * --- ---- -------------
 *
 *   Inputs:     R0-R(num_input-1)
 *   Constants:  same as VS
 *   Immediates: same as VS
 *   Outputs:    export0-export(num_outputs)
 *   Temps:      R(num_input)-R(num_input+num_temps-1)
 *
 * In either case, immediates are are postpended to the constants
 * (uniforms).
 *
 */

static unsigned
get_temp_gpr(struct fd_compile_context *ctx, int idx)
{
	unsigned num = idx + ctx->num_regs[TGSI_FILE_INPUT];
	if (ctx->type == TGSI_PROCESSOR_VERTEX)
		num++;
	return num;
}

static struct ir_register *
add_dst_reg(struct fd_compile_context *ctx, struct ir_instruction *alu,
		const struct tgsi_dst_register *dst)
{
	unsigned flags = 0, num = 0;
	char swiz[5];

	switch (dst->File) {
	case TGSI_FILE_OUTPUT:
		flags |= IR_REG_EXPORT;
		if (ctx->type == TGSI_PROCESSOR_VERTEX) {
			if (dst->Index == ctx->position) {
				num = 62;
			} else if (dst->Index == ctx->psize) {
				num = 63;
			} else {
				num = export_linkage(ctx,
						ctx->output_export_idx[dst->Index]);
			}
		} else {
			num = dst->Index;
		}
		break;
	case TGSI_FILE_TEMPORARY:
		num = get_temp_gpr(ctx, dst->Index);
		break;
	default:
		DBG("unsupported dst register file: %s",
				tgsi_file_names[dst->File]);
		assert(0);
		break;
	}

	swiz[0] = (dst->WriteMask & TGSI_WRITEMASK_X) ? 'x' : '_';
	swiz[1] = (dst->WriteMask & TGSI_WRITEMASK_Y) ? 'y' : '_';
	swiz[2] = (dst->WriteMask & TGSI_WRITEMASK_Z) ? 'z' : '_';
	swiz[3] = (dst->WriteMask & TGSI_WRITEMASK_W) ? 'w' : '_';
	swiz[4] = '\0';

	return ir_reg_create(alu, num, swiz, flags);
}

static struct ir_register *
add_src_reg(struct fd_compile_context *ctx, struct ir_instruction *alu,
		const struct tgsi_src_register *src)
{
	static const char swiz_vals[] = {
			'x', 'y', 'z', 'w',
	};
	char swiz[5];
	unsigned flags = 0, num = 0;

	switch (src->File) {
	case TGSI_FILE_CONSTANT:
		num = src->Index;
		flags |= IR_REG_CONST;
		break;
	case TGSI_FILE_INPUT:
		if (ctx->type == TGSI_PROCESSOR_VERTEX) {
			num = src->Index + 1;
		} else {
			num = export_linkage(ctx,
					ctx->input_export_idx[src->Index]);
		}
		break;
	case TGSI_FILE_TEMPORARY:
		num = get_temp_gpr(ctx, src->Index);
		break;
	case TGSI_FILE_IMMEDIATE:
		num = src->Index + ctx->num_regs[TGSI_FILE_CONSTANT];
		flags |= IR_REG_CONST;
		break;
	default:
		DBG("unsupported src register file: %s",
				tgsi_file_names[src->File]);
		assert(0);
		break;
	}

	if (src->Absolute)
		flags |= IR_REG_ABS;
	if (src->Negate)
		flags |= IR_REG_NEGATE;

	swiz[0] = swiz_vals[src->SwizzleX];
	swiz[1] = swiz_vals[src->SwizzleY];
	swiz[2] = swiz_vals[src->SwizzleZ];
	swiz[3] = swiz_vals[src->SwizzleW];
	swiz[4] = '\0';

	if ((ctx->need_sync & (uint64_t)(1 << num)) &&
			!(flags & IR_REG_CONST)) {
		alu->sync = true;
		ctx->need_sync &= ~(uint64_t)(1 << num);
	}

	return ir_reg_create(alu, num, swiz, flags);
}

static void
add_vector_clamp(struct tgsi_full_instruction *inst, struct ir_instruction *alu)
{
	switch (inst->Instruction.Saturate) {
	case TGSI_SAT_NONE:
		break;
	case TGSI_SAT_ZERO_ONE:
		alu->alu.vector_clamp = true;
		break;
	case TGSI_SAT_MINUS_PLUS_ONE:
		DBG("unsupported saturate");
		assert(0);
		break;
	}
}

static void
add_scalar_clamp(struct tgsi_full_instruction *inst, struct ir_instruction *alu)
{
	switch (inst->Instruction.Saturate) {
	case TGSI_SAT_NONE:
		break;
	case TGSI_SAT_ZERO_ONE:
		alu->alu.scalar_clamp = true;
		break;
	case TGSI_SAT_MINUS_PLUS_ONE:
		DBG("unsupported saturate");
		assert(0);
		break;
	}
}

static void
add_regs_vector_1(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst, struct ir_instruction *alu)
{
	assert(inst->Instruction.NumSrcRegs == 1);
	assert(inst->Instruction.NumDstRegs == 1);

	add_dst_reg(ctx, alu, &inst->Dst[0].Register);
	add_src_reg(ctx, alu, &inst->Src[0].Register);
	add_src_reg(ctx, alu, &inst->Src[0].Register);
	add_vector_clamp(inst, alu);
}

static void
add_regs_vector_2(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst, struct ir_instruction *alu)
{
	assert(inst->Instruction.NumSrcRegs == 2);
	assert(inst->Instruction.NumDstRegs == 1);

	add_dst_reg(ctx, alu, &inst->Dst[0].Register);
	add_src_reg(ctx, alu, &inst->Src[0].Register);
	add_src_reg(ctx, alu, &inst->Src[1].Register);
	add_vector_clamp(inst, alu);
}

static void
add_regs_vector_3(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst, struct ir_instruction *alu)
{
	assert(inst->Instruction.NumSrcRegs == 3);
	assert(inst->Instruction.NumDstRegs == 1);

	add_dst_reg(ctx, alu, &inst->Dst[0].Register);
	/* maybe should re-arrange the syntax some day, but
	 * in assembler/disassembler and what ir.c expects
	 * is: MULADDv Rdst = Rsrc2 + Rsrc0 * Rscr1
	 */
	add_src_reg(ctx, alu, &inst->Src[2].Register);
	add_src_reg(ctx, alu, &inst->Src[0].Register);
	add_src_reg(ctx, alu, &inst->Src[1].Register);
	add_vector_clamp(inst, alu);
}

static void
add_regs_dummy_vector(struct ir_instruction *alu)
{
	/* create dummy, non-written vector dst/src regs
	 * for unused vector instr slot:
	 */
	ir_reg_create(alu, 0, "____", 0); /* vector dst */
	ir_reg_create(alu, 0, NULL, 0);   /* vector src1 */
	ir_reg_create(alu, 0, NULL, 0);   /* vector src2 */
}

static void
add_regs_scalar_1(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst, struct ir_instruction *alu)
{
	assert(inst->Instruction.NumSrcRegs == 1);
	assert(inst->Instruction.NumDstRegs == 1);

	add_regs_dummy_vector(alu);

	add_dst_reg(ctx, alu, &inst->Dst[0].Register);
	add_src_reg(ctx, alu, &inst->Src[0].Register);
	add_scalar_clamp(inst, alu);
}

/*
 * Helpers for TGSI instructions that don't map to a single shader instr:
 */

/* Get internal-temp src/dst to use for a sequence of instructions
 * generated by a single TGSI op.. if possible, use the final dst
 * register as the temporary to avoid allocating a new register, but
 * if necessary allocate one.  If a single TGSI op needs multiple
 * internal temps, pass NULL for orig_dst for all but the first one
 * so that you don't end up using the same register for all your
 * internal temps.
 */
static bool
get_internal_temp(struct fd_compile_context *ctx,
		struct tgsi_dst_register *orig_dst,
		struct tgsi_dst_register *tmp_dst,
		struct tgsi_src_register *tmp_src)
{
	bool using_temp = false;

	tmp_dst->File      = TGSI_FILE_TEMPORARY;
	tmp_dst->WriteMask = TGSI_WRITEMASK_XYZW;
	tmp_dst->Indirect  = 0;
	tmp_dst->Dimension = 0;

	if (orig_dst && (orig_dst->File != TGSI_FILE_OUTPUT)) {
		/* if possible, use orig dst register for the temporary: */
		tmp_dst->Index = orig_dst->Index;
	} else {
		/* otherwise assign one: */
		int n = ctx->num_internal_temps++;
		if (ctx->pred_reg != -1)
			n++;
		tmp_dst->Index = get_temp_gpr(ctx,
				ctx->num_regs[TGSI_FILE_TEMPORARY] + n);
		using_temp = true;
	}

	tmp_src->File      = tmp_dst->File;
	tmp_src->Indirect  = tmp_dst->Indirect;
	tmp_src->Dimension = tmp_dst->Dimension;
	tmp_src->Index     = tmp_dst->Index;
	tmp_src->Absolute  = 0;
	tmp_src->Negate    = 0;
	tmp_src->SwizzleX  = TGSI_SWIZZLE_X;
	tmp_src->SwizzleY  = TGSI_SWIZZLE_Y;
	tmp_src->SwizzleZ  = TGSI_SWIZZLE_Z;
	tmp_src->SwizzleW  = TGSI_SWIZZLE_W;

	return using_temp;
}

static void
get_predicate(struct fd_compile_context *ctx, struct tgsi_dst_register *dst,
		struct tgsi_src_register *src)
{
	assert(ctx->pred_reg != -1);

	dst->File      = TGSI_FILE_TEMPORARY;
	dst->WriteMask = TGSI_WRITEMASK_W;
	dst->Indirect  = 0;
	dst->Dimension = 0;
	dst->Index     = get_temp_gpr(ctx, ctx->pred_reg);

	if (src) {
		src->File      = dst->File;
		src->Indirect  = dst->Indirect;
		src->Dimension = dst->Dimension;
		src->Index     = dst->Index;
		src->Absolute  = 0;
		src->Negate    = 0;
		src->SwizzleX  = TGSI_SWIZZLE_W;
		src->SwizzleY  = TGSI_SWIZZLE_W;
		src->SwizzleZ  = TGSI_SWIZZLE_W;
		src->SwizzleW  = TGSI_SWIZZLE_W;
	}
}

static void
push_predicate(struct fd_compile_context *ctx, struct tgsi_src_register *src)
{
	struct ir_instruction *alu;
	struct tgsi_dst_register pred_dst;

	/* NOTE blob compiler seems to always puts PRED_* instrs in a CF by
	 * themselves:
	 */
	ctx->cf = NULL;

	if (ctx->pred_depth == 0) {
		/* assign predicate register: */
		ctx->pred_reg = ctx->num_regs[TGSI_FILE_TEMPORARY];

		get_predicate(ctx, &pred_dst, NULL);

		alu = ir_instr_create_alu(next_exec_cf(ctx), ~0, PRED_SETNEs);
		add_regs_dummy_vector(alu);
		add_dst_reg(ctx, alu, &pred_dst);
		add_src_reg(ctx, alu, src);
	} else {
		struct tgsi_src_register pred_src;

		get_predicate(ctx, &pred_dst, &pred_src);

		alu = ir_instr_create_alu(next_exec_cf(ctx), MULv, ~0);
		add_dst_reg(ctx, alu, &pred_dst);
		add_src_reg(ctx, alu, &pred_src);
		add_src_reg(ctx, alu, src);

		// XXX need to make PRED_SETE_PUSHv IR_PRED_NONE.. but need to make
		// sure src reg is valid if it was calculated with a predicate
		// condition..
		alu->pred = IR_PRED_NONE;
	}

	/* save previous pred state to restore in pop_predicate(): */
	ctx->pred_stack[ctx->pred_depth++] = ctx->so->ir->pred;

	ctx->cf = NULL;
}

static void
pop_predicate(struct fd_compile_context *ctx)
{
	/* NOTE blob compiler seems to always puts PRED_* instrs in a CF by
	 * themselves:
	 */
	ctx->cf = NULL;

	/* restore previous predicate state: */
	ctx->so->ir->pred = ctx->pred_stack[--ctx->pred_depth];

	if (ctx->pred_depth != 0) {
		struct ir_instruction *alu;
		struct tgsi_dst_register pred_dst;
		struct tgsi_src_register pred_src;

		get_predicate(ctx, &pred_dst, &pred_src);

		alu = ir_instr_create_alu(next_exec_cf(ctx), ~0, PRED_SET_POPs);
		add_regs_dummy_vector(alu);
		add_dst_reg(ctx, alu, &pred_dst);
		add_src_reg(ctx, alu, &pred_src);
		alu->pred = IR_PRED_NONE;
	} else {
		/* predicate register no longer needed: */
		ctx->pred_reg = -1;
	}

	ctx->cf = NULL;
}

static void
get_immediate(struct fd_compile_context *ctx,
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
		ctx->so->num_immediates = idx + 1;
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

/* POW(a,b) = EXP2(b * LOG2(a)) */
static void
translate_pow(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register tmp_src;
	struct ir_instruction *alu;

	get_internal_temp(ctx, &inst->Dst[0].Register, &tmp_dst, &tmp_src);

	alu = ir_instr_create_alu(next_exec_cf(ctx), ~0, LOG_CLAMP);
	add_regs_dummy_vector(alu);
	add_dst_reg(ctx, alu, &tmp_dst);
	add_src_reg(ctx, alu, &inst->Src[0].Register);

	alu = ir_instr_create_alu(next_exec_cf(ctx), MULv, ~0);
	add_dst_reg(ctx, alu, &tmp_dst);
	add_src_reg(ctx, alu, &tmp_src);
	add_src_reg(ctx, alu, &inst->Src[1].Register);

	/* NOTE: some of the instructions, like EXP_IEEE, seem hard-
	 * coded to take their input from the w component.
	 */
	switch(inst->Dst[0].Register.WriteMask) {
	case TGSI_WRITEMASK_X:
		tmp_src.SwizzleW = TGSI_SWIZZLE_X;
		break;
	case TGSI_WRITEMASK_Y:
		tmp_src.SwizzleW = TGSI_SWIZZLE_Y;
		break;
	case TGSI_WRITEMASK_Z:
		tmp_src.SwizzleW = TGSI_SWIZZLE_Z;
		break;
	case TGSI_WRITEMASK_W:
		tmp_src.SwizzleW = TGSI_SWIZZLE_W;
		break;
	default:
		DBG("invalid writemask!");
		assert(0);
		break;
	}

	alu = ir_instr_create_alu(next_exec_cf(ctx), ~0, EXP_IEEE);
	add_regs_dummy_vector(alu);
	add_dst_reg(ctx, alu, &inst->Dst[0].Register);
	add_src_reg(ctx, alu, &tmp_src);
	add_scalar_clamp(inst, alu);
}

static void
translate_tex(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst, unsigned opc)
{
	struct ir_instruction *instr;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register tmp_src;
	const struct tgsi_src_register *coord;
	bool using_temp;
	int idx;

	using_temp = get_internal_temp(ctx,
			&inst->Dst[0].Register, &tmp_dst, &tmp_src);

	if (opc == TGSI_OPCODE_TXP) {
		/* TXP - Projective Texture Lookup:
		 *
		 *  coord.x = src0.x / src.w
		 *  coord.y = src0.y / src.w
		 *  coord.z = src0.z / src.w
		 *  coord.w = src0.w
		 *  bias = 0.0
		 *
		 *  dst = texture_sample(unit, coord, bias)
		 */
		instr = ir_instr_create_alu(next_exec_cf(ctx), MAXv, RECIP_IEEE);

		/* MAXv: */
		add_dst_reg(ctx, instr, &tmp_dst)->swizzle = "___w";
		add_src_reg(ctx, instr, &inst->Src[0].Register);
		add_src_reg(ctx, instr, &inst->Src[0].Register);

		/* RECIP_IEEE: */
		add_dst_reg(ctx, instr, &tmp_dst)->swizzle = "x___";
		add_src_reg(ctx, instr, &inst->Src[0].Register)->swizzle = "wwww";

		instr = ir_instr_create_alu(next_exec_cf(ctx), MULv, ~0);
		add_dst_reg(ctx, instr, &tmp_dst)->swizzle = "xyz_";
		add_src_reg(ctx, instr, &tmp_src)->swizzle = "xxxx";
		add_src_reg(ctx, instr, &inst->Src[0].Register);

		coord = &tmp_src;
	} else {
		coord = &inst->Src[0].Register;
	}

	instr = ir_instr_create(next_exec_cf(ctx), IR_FETCH);
	instr->fetch.opc = TEX_FETCH;
	assert(inst->Texture.NumOffsets <= 1); // TODO what to do in other cases?

	/* save off the tex fetch to be patched later with correct const_idx: */
	idx = ctx->so->num_tfetch_instrs++;
	ctx->so->tfetch_instrs[idx].samp_id = inst->Src[1].Register.Index;
	ctx->so->tfetch_instrs[idx].instr = instr;

	add_dst_reg(ctx, instr, &tmp_dst);
	add_src_reg(ctx, instr, coord);

	/* dst register needs to be marked for sync: */
	ctx->need_sync |= 1 << instr->regs[0]->num;

	/* TODO we need some way to know if the tex fetch needs to sync on alu pipe.. */
	instr->sync = true;

	if (using_temp) {
		/* texture fetch can't write directly to export, so if tgsi
		 * is telling us the dst register is in output file, we load
		 * the texture to a temp and the use ALU instruction to move
		 * to output
		 */
		instr = ir_instr_create_alu(next_exec_cf(ctx), MAXv, ~0);

		add_dst_reg(ctx, instr, &inst->Dst[0].Register);
		add_src_reg(ctx, instr, &tmp_src);
		add_src_reg(ctx, instr, &tmp_src);
		add_vector_clamp(inst, instr);
	}
}

/* SGE(a,b) = GTE((b - a), 1.0, 0.0) */
/* SLT(a,b) = GTE((b - a), 0.0, 1.0) */
static void
translate_sge_slt(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst, unsigned opc)
{
	struct ir_instruction *instr;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register tmp_src;
	struct tgsi_src_register tmp_const;
	float c0, c1;

	switch (opc) {
	default:
		assert(0);
	case TGSI_OPCODE_SGE:
		c0 = 1.0;
		c1 = 0.0;
		break;
	case TGSI_OPCODE_SLT:
		c0 = 0.0;
		c1 = 1.0;
		break;
	}

	get_internal_temp(ctx, &inst->Dst[0].Register, &tmp_dst, &tmp_src);

	instr = ir_instr_create_alu(next_exec_cf(ctx), ADDv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst);
	add_src_reg(ctx, instr, &inst->Src[0].Register)->flags |= IR_REG_NEGATE;
	add_src_reg(ctx, instr, &inst->Src[1].Register);

	instr = ir_instr_create_alu(next_exec_cf(ctx), CNDGTEv, ~0);
	add_dst_reg(ctx, instr, &inst->Dst[0].Register);
	/* maybe should re-arrange the syntax some day, but
	 * in assembler/disassembler and what ir.c expects
	 * is: MULADDv Rdst = Rsrc2 + Rsrc0 * Rscr1
	 */
	get_immediate(ctx, &tmp_const, fui(c0));
	add_src_reg(ctx, instr, &tmp_const);
	add_src_reg(ctx, instr, &tmp_src);
	get_immediate(ctx, &tmp_const, fui(c1));
	add_src_reg(ctx, instr, &tmp_const);
}

/* LRP(a,b,c) = (a * b) + ((1 - a) * c) */
static void
translate_lrp(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst,
		unsigned opc)
{
	struct ir_instruction *instr;
	struct tgsi_dst_register tmp_dst1, tmp_dst2;
	struct tgsi_src_register tmp_src1, tmp_src2;
	struct tgsi_src_register tmp_const;

	get_internal_temp(ctx, &inst->Dst[0].Register, &tmp_dst1, &tmp_src1);
	get_internal_temp(ctx, NULL, &tmp_dst2, &tmp_src2);

	get_immediate(ctx, &tmp_const, fui(1.0));

	/* tmp1 = (a * b) */
	instr = ir_instr_create_alu(next_exec_cf(ctx), MULv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst1);
	add_src_reg(ctx, instr, &inst->Src[0].Register);
	add_src_reg(ctx, instr, &inst->Src[1].Register);

	/* tmp2 = (1 - a) */
	instr = ir_instr_create_alu(next_exec_cf(ctx), ADDv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst2);
	add_src_reg(ctx, instr, &tmp_const);
	add_src_reg(ctx, instr, &inst->Src[0].Register)->flags |= IR_REG_NEGATE;

	/* tmp2 = tmp2 * c */
	instr = ir_instr_create_alu(next_exec_cf(ctx), MULv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst2);
	add_src_reg(ctx, instr, &tmp_src2);
	add_src_reg(ctx, instr, &inst->Src[2].Register);

	/* dst = tmp1 + tmp2 */
	instr = ir_instr_create_alu(next_exec_cf(ctx), ADDv, ~0);
	add_dst_reg(ctx, instr, &inst->Dst[0].Register);
	add_src_reg(ctx, instr, &tmp_src1);
	add_src_reg(ctx, instr, &tmp_src2);
}

static void
translate_trig(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst,
		unsigned opc)
{
	struct ir_instruction *instr;
	struct tgsi_dst_register tmp_dst;
	struct tgsi_src_register tmp_src;
	struct tgsi_src_register tmp_const;
	instr_scalar_opc_t op;

	switch (opc) {
	default:
		assert(0);
	case TGSI_OPCODE_SIN:
		op = SIN;
		break;
	case TGSI_OPCODE_COS:
		op = COS;
		break;
	}

	get_internal_temp(ctx, &inst->Dst[0].Register, &tmp_dst, &tmp_src);

	tmp_dst.WriteMask = TGSI_WRITEMASK_X;
	tmp_src.SwizzleX = tmp_src.SwizzleY =
			tmp_src.SwizzleZ = tmp_src.SwizzleW = TGSI_SWIZZLE_X;

	/* maybe should re-arrange the syntax some day, but
	 * in assembler/disassembler and what ir.c expects
	 * is: MULADDv Rdst = Rsrc2 + Rsrc0 * Rscr1
	 */
	instr = ir_instr_create_alu(next_exec_cf(ctx), MULADDv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst);
	get_immediate(ctx, &tmp_const, fui(0.5));
	add_src_reg(ctx, instr, &tmp_const);
	add_src_reg(ctx, instr, &inst->Src[0].Register);
	get_immediate(ctx, &tmp_const, fui(0.159155));
	add_src_reg(ctx, instr, &tmp_const);

	instr = ir_instr_create_alu(next_exec_cf(ctx), FRACv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst);
	add_src_reg(ctx, instr, &tmp_src);
	add_src_reg(ctx, instr, &tmp_src);

	instr = ir_instr_create_alu(next_exec_cf(ctx), MULADDv, ~0);
	add_dst_reg(ctx, instr, &tmp_dst);
	get_immediate(ctx, &tmp_const, fui(-3.141593));
	add_src_reg(ctx, instr, &tmp_const);
	add_src_reg(ctx, instr, &tmp_src);
	get_immediate(ctx, &tmp_const, fui(6.283185));
	add_src_reg(ctx, instr, &tmp_const);

	instr = ir_instr_create_alu(next_exec_cf(ctx), ~0, op);
	add_regs_dummy_vector(instr);
	add_dst_reg(ctx, instr, &inst->Dst[0].Register);
	add_src_reg(ctx, instr, &tmp_src);
}

/*
 * Main part of compiler/translator:
 */

static void
translate_instruction(struct fd_compile_context *ctx,
		struct tgsi_full_instruction *inst)
{
	unsigned opc = inst->Instruction.Opcode;
	struct ir_instruction *instr;
	static struct ir_cf *cf;

	if (opc == TGSI_OPCODE_END)
		return;

	if (inst->Dst[0].Register.File == TGSI_FILE_OUTPUT) {
		unsigned num = inst->Dst[0].Register.Index;
		/* seems like we need to ensure that position vs param/pixel
		 * exports don't end up in the same EXEC clause..  easy way
		 * to do this is force a new EXEC clause on first appearance
		 * of an position or param/pixel export.
		 */
		if ((num == ctx->position) || (num == ctx->psize)) {
			if (ctx->num_position > 0) {
				ctx->cf = NULL;
				ir_cf_create_alloc(ctx->so->ir, SQ_POSITION,
						ctx->num_position - 1);
				ctx->num_position = 0;
			}
		} else {
			if (ctx->num_param > 0) {
				ctx->cf = NULL;
				ir_cf_create_alloc(ctx->so->ir, SQ_PARAMETER_PIXEL,
						ctx->num_param - 1);
				ctx->num_param = 0;
			}
		}
	}

	cf = next_exec_cf(ctx);

	/* TODO turn this into a table: */
	switch (opc) {
	case TGSI_OPCODE_MOV:
		instr = ir_instr_create_alu(cf, MAXv, ~0);
		add_regs_vector_1(ctx, inst, instr);
		break;
	case TGSI_OPCODE_RCP:
		instr = ir_instr_create_alu(cf, ~0, RECIP_IEEE);
		add_regs_scalar_1(ctx, inst, instr);
		break;
	case TGSI_OPCODE_RSQ:
		instr = ir_instr_create_alu(cf, ~0, RECIPSQ_IEEE);
		add_regs_scalar_1(ctx, inst, instr);
		break;
	case TGSI_OPCODE_MUL:
		instr = ir_instr_create_alu(cf, MULv, ~0);
		add_regs_vector_2(ctx, inst, instr);
		break;
	case TGSI_OPCODE_ADD:
		instr = ir_instr_create_alu(cf, ADDv, ~0);
		add_regs_vector_2(ctx, inst, instr);
		break;
	case TGSI_OPCODE_DP3:
		instr = ir_instr_create_alu(cf, DOT3v, ~0);
		add_regs_vector_2(ctx, inst, instr);
		break;
	case TGSI_OPCODE_DP4:
		instr = ir_instr_create_alu(cf, DOT4v, ~0);
		add_regs_vector_2(ctx, inst, instr);
		break;
	case TGSI_OPCODE_MIN:
		instr = ir_instr_create_alu(cf, MINv, ~0);
		add_regs_vector_2(ctx, inst, instr);
		break;
	case TGSI_OPCODE_MAX:
		instr = ir_instr_create_alu(cf, MAXv, ~0);
		add_regs_vector_2(ctx, inst, instr);
		break;
	case TGSI_OPCODE_SLT:
	case TGSI_OPCODE_SGE:
		translate_sge_slt(ctx, inst, opc);
		break;
	case TGSI_OPCODE_MAD:
		instr = ir_instr_create_alu(cf, MULADDv, ~0);
		add_regs_vector_3(ctx, inst, instr);
		break;
	case TGSI_OPCODE_LRP:
		translate_lrp(ctx, inst, opc);
		break;
	case TGSI_OPCODE_FRC:
		instr = ir_instr_create_alu(cf, FRACv, ~0);
		add_regs_vector_1(ctx, inst, instr);
		break;
	case TGSI_OPCODE_FLR:
		instr = ir_instr_create_alu(cf, FLOORv, ~0);
		add_regs_vector_1(ctx, inst, instr);
		break;
	case TGSI_OPCODE_EX2:
		instr = ir_instr_create_alu(cf, ~0, EXP_IEEE);
		add_regs_scalar_1(ctx, inst, instr);
		break;
	case TGSI_OPCODE_POW:
		translate_pow(ctx, inst);
		break;
	case TGSI_OPCODE_ABS:
		instr = ir_instr_create_alu(cf, MAXv, ~0);
		add_regs_vector_1(ctx, inst, instr);
		instr->regs[1]->flags |= IR_REG_NEGATE; /* src0 */
		break;
	case TGSI_OPCODE_COS:
	case TGSI_OPCODE_SIN:
		translate_trig(ctx, inst, opc);
		break;
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXP:
		translate_tex(ctx, inst, opc);
		break;
	case TGSI_OPCODE_CMP:
		instr = ir_instr_create_alu(cf, CNDGTEv, ~0);
		add_regs_vector_3(ctx, inst, instr);
		// TODO this should be src0 if regs where in sane order..
		instr->regs[2]->flags ^= IR_REG_NEGATE; /* src1 */
		break;
	case TGSI_OPCODE_IF:
		push_predicate(ctx, &inst->Src[0].Register);
		ctx->so->ir->pred = IR_PRED_EQ;
		break;
	case TGSI_OPCODE_ELSE:
		ctx->so->ir->pred = IR_PRED_NE;
		/* not sure if this is required in all cases, but blob compiler
		 * won't combine EQ and NE in same CF:
		 */
		ctx->cf = NULL;
		break;
	case TGSI_OPCODE_ENDIF:
		pop_predicate(ctx);
		break;
	case TGSI_OPCODE_F2I:
		instr = ir_instr_create_alu(cf, TRUNCv, ~0);
		add_regs_vector_1(ctx, inst, instr);
		break;
	default:
		DBG("unknown TGSI opc: %s", tgsi_get_opcode_name(opc));
		tgsi_dump(ctx->so->tokens, 0);
		assert(0);
		break;
	}

	/* internal temporaries are only valid for the duration of a single
	 * TGSI instruction:
	 */
	ctx->num_internal_temps = 0;
}

static void
compile_instructions(struct fd_compile_context *ctx)
{
	while (!tgsi_parse_end_of_tokens(&ctx->parser)) {
		tgsi_parse_token(&ctx->parser);

		switch (ctx->parser.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			translate_instruction(ctx,
					&ctx->parser.FullToken.FullInstruction);
			break;
		default:
			break;
		}
	}

	ctx->cf->cf_type = EXEC_END;
}

int
fd_compile_shader(struct fd_program_stateobj *prog,
		struct fd_shader_stateobj *so)
{
	struct fd_compile_context ctx;

	ir_shader_destroy(so->ir);
	so->ir = ir_shader_create();
	so->num_vfetch_instrs = so->num_tfetch_instrs = so->num_immediates = 0;

	if (compile_init(&ctx, prog, so) != TGSI_PARSE_OK)
		return -1;

	if (ctx.type == TGSI_PROCESSOR_VERTEX) {
		compile_vtx_fetch(&ctx);
	} else if (ctx.type == TGSI_PROCESSOR_FRAGMENT) {
		prog->num_exports = 0;
		memset(prog->export_linkage, 0xff,
				sizeof(prog->export_linkage));
	}

	compile_instructions(&ctx);

	compile_free(&ctx);

	return 0;
}

