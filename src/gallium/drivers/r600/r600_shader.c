/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_dump.h"
#include "util/u_format.h"
#include "r600_pipe.h"
#include "r600_asm.h"
#include "r600_sq.h"
#include "r600_opcodes.h"
#include "r600d.h"
#include <stdio.h>
#include <errno.h>

static void r600_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned spi_vs_out_id[10];
	unsigned i, tmp;

	/* clear previous register */
	rstate->nregs = 0;

	/* so far never got proper semantic id from tgsi */
	/* FIXME better to move this in config things so they get emited
	 * only one time per cs
	 */
	for (i = 0; i < 10; i++) {
		spi_vs_out_id[i] = 0;
	}
	for (i = 0; i < 32; i++) {
		tmp = i << ((i & 3) * 8);
		spi_vs_out_id[i / 4] |= tmp;
	}
	for (i = 0; i < 10; i++) {
		r600_pipe_state_add_reg(rstate,
					R_028614_SPI_VS_OUT_ID_0 + i * 4,
					spi_vs_out_id[i], 0xFFFFFFFF, NULL);
	}

	r600_pipe_state_add_reg(rstate,
			R_0286C4_SPI_VS_OUT_CONFIG,
			S_0286C4_VS_EXPORT_COUNT(rshader->noutput - 2),
			0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
			R_028868_SQ_PGM_RESOURCES_VS,
			S_028868_NUM_GPRS(rshader->bc.ngpr) |
			S_028868_STACK_SIZE(rshader->bc.nstack),
			0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
			R_0288D0_SQ_PGM_CF_OFFSET_VS,
			0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
			R_028858_SQ_PGM_START_VS,
			r600_bo_offset(shader->bo) >> 8, 0xFFFFFFFF, shader->bo);

	r600_pipe_state_add_reg(rstate,
				R_03E200_SQ_LOOP_CONST_0 + (32 * 4), 0x01000FFF,
				0xFFFFFFFF, NULL);

}

int r600_find_vs_semantic_index(struct r600_shader *vs,
				struct r600_shader *ps, int id)
{
	struct r600_shader_io *input = &ps->input[id];

	for (int i = 0; i < vs->noutput; i++) {
		if (input->name == vs->output[i].name &&
			input->sid == vs->output[i].sid) {
			return i - 1;
		}
	}
	return 0;
}

static void r600_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, exports_ps, num_cout, spi_ps_in_control_0, spi_input_z, spi_ps_in_control_1;
	int pos_index = -1, face_index = -1;

	rstate->nregs = 0;

	for (i = 0; i < rshader->ninput; i++) {
		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION)
			pos_index = i;
		if (rshader->input[i].name == TGSI_SEMANTIC_FACE)
			face_index = i;
	}

	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			r600_pipe_state_add_reg(rstate,
						R_02880C_DB_SHADER_CONTROL,
						S_02880C_Z_EXPORT_ENABLE(1),
						S_02880C_Z_EXPORT_ENABLE(1), NULL);
		if (rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			r600_pipe_state_add_reg(rstate,
						R_02880C_DB_SHADER_CONTROL,
						S_02880C_STENCIL_REF_EXPORT_ENABLE(1),
						S_02880C_STENCIL_REF_EXPORT_ENABLE(1), NULL);
	}

	exports_ps = 0;
	num_cout = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION || rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
		else if (rshader->output[i].name == TGSI_SEMANTIC_COLOR) {
			num_cout++;
		}
	}
	exports_ps |= S_028854_EXPORT_COLORS(num_cout);
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}

	spi_ps_in_control_0 = S_0286CC_NUM_INTERP(rshader->ninput) |
				S_0286CC_PERSP_GRADIENT_ENA(1);
	spi_input_z = 0;
	if (pos_index != -1) {
		spi_ps_in_control_0 |= (S_0286CC_POSITION_ENA(1) |
					S_0286CC_POSITION_CENTROID(rshader->input[pos_index].centroid) |
					S_0286CC_POSITION_ADDR(rshader->input[pos_index].gpr) |
					S_0286CC_BARYC_SAMPLE_CNTL(1));
		spi_input_z |= 1;
	}

	spi_ps_in_control_1 = 0;
	if (face_index != -1) {
		spi_ps_in_control_1 |= S_0286D0_FRONT_FACE_ENA(1) |
			S_0286D0_FRONT_FACE_ADDR(rshader->input[face_index].gpr);
	}

	r600_pipe_state_add_reg(rstate, R_0286CC_SPI_PS_IN_CONTROL_0, spi_ps_in_control_0, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286D0_SPI_PS_IN_CONTROL_1, spi_ps_in_control_1, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286D8_SPI_INPUT_Z, spi_input_z, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028840_SQ_PGM_START_PS,
				r600_bo_offset(shader->bo) >> 8, 0xFFFFFFFF, shader->bo);
	r600_pipe_state_add_reg(rstate,
				R_028850_SQ_PGM_RESOURCES_PS,
				S_028868_NUM_GPRS(rshader->bc.ngpr) |
				S_028868_STACK_SIZE(rshader->bc.nstack),
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028854_SQ_PGM_EXPORTS_PS,
				exports_ps, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_0288CC_SQ_PGM_CF_OFFSET_PS,
				0x00000000, 0xFFFFFFFF, NULL);

	if (rshader->uses_kill) {
		/* only set some bits here, the other bits are set in the dsa state */
		r600_pipe_state_add_reg(rstate,
					R_02880C_DB_SHADER_CONTROL,
					S_02880C_KILL_ENABLE(1),
					S_02880C_KILL_ENABLE(1), NULL);
	}
	r600_pipe_state_add_reg(rstate,
				R_03E200_SQ_LOOP_CONST_0, 0x01000FFF,
				0xFFFFFFFF, NULL);
}

int r600_pipe_shader(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_shader *rshader = &shader->shader;
	void *ptr;

	/* copy new shader */
	if (shader->bo == NULL) {
		shader->bo = r600_bo(rctx->radeon, rshader->bc.ndw * 4, 4096, 0, 0);
		if (shader->bo == NULL) {
			return -ENOMEM;
		}
		ptr = r600_bo_map(rctx->radeon, shader->bo, 0, NULL);
		memcpy(ptr, rshader->bc.bytecode, rshader->bc.ndw * 4);
		r600_bo_unmap(rctx->radeon, shader->bo);
	}
	/* build state */
	switch (rshader->processor_type) {
	case TGSI_PROCESSOR_VERTEX:
		if (rshader->family >= CHIP_CEDAR) {
			evergreen_pipe_shader_vs(ctx, shader);
		} else {
			r600_pipe_shader_vs(ctx, shader);
		}
		break;
	case TGSI_PROCESSOR_FRAGMENT:
		if (rshader->family >= CHIP_CEDAR) {
			evergreen_pipe_shader_ps(ctx, shader);
		} else {
			r600_pipe_shader_ps(ctx, shader);
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int r600_shader_from_tgsi(const struct tgsi_token *tokens, struct r600_shader *shader, u32 **literals);
int r600_pipe_shader_create(struct pipe_context *ctx, struct r600_pipe_shader *shader, const struct tgsi_token *tokens)
{
	static int dump_shaders = -1;
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	u32 *literals;
	int r;

        /* Would like some magic "get_bool_option_once" routine.
         */
        if (dump_shaders == -1)
                dump_shaders = debug_get_bool_option("R600_DUMP_SHADERS", FALSE);

	if (dump_shaders) {
		fprintf(stderr, "--------------------------------------------------------------\n");
		tgsi_dump(tokens, 0);
	}
	shader->shader.family = r600_get_family(rctx->radeon);
	r = r600_shader_from_tgsi(tokens, &shader->shader, &literals);
	if (r) {
		R600_ERR("translation from TGSI failed !\n");
		return r;
	}
	r = r600_bc_build(&shader->shader.bc);
	free(literals);
	if (r) {
		R600_ERR("building bytecode failed !\n");
		return r;
	}
	if (dump_shaders) {
		r600_bc_dump(&shader->shader.bc);
		fprintf(stderr, "______________________________________________________________\n");
	}
	return r600_pipe_shader(ctx, shader);
}

void r600_pipe_shader_destroy(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	r600_bo_reference(rctx->radeon, &shader->bo, NULL);
	r600_bc_clear(&shader->shader.bc);
}

/*
 * tgsi -> r600 shader
 */
struct r600_shader_tgsi_instruction;

struct r600_shader_ctx {
	struct tgsi_shader_info			info;
	struct tgsi_parse_context		parse;
	const struct tgsi_token			*tokens;
	unsigned				type;
	unsigned				file_offset[TGSI_FILE_COUNT];
	unsigned				temp_reg;
	struct r600_shader_tgsi_instruction	*inst_info;
	struct r600_bc				*bc;
	struct r600_shader			*shader;
	u32					*literals;
	u32					nliterals;
	u32					max_driver_temp_used;
	/* needed for evergreen interpolation */
	boolean                                 input_centroid;
	boolean                                 input_linear;
	boolean                                 input_perspective;
	int					num_interp_gpr;
};

struct r600_shader_tgsi_instruction {
	unsigned	tgsi_opcode;
	unsigned	is_op3;
	unsigned	r600_opcode;
	int (*process)(struct r600_shader_ctx *ctx);
};

static struct r600_shader_tgsi_instruction r600_shader_tgsi_instruction[], eg_shader_tgsi_instruction[];
static int tgsi_helper_tempx_replicate(struct r600_shader_ctx *ctx);

static int tgsi_is_supported(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *i = &ctx->parse.FullToken.FullInstruction;
	int j;

	if (i->Instruction.NumDstRegs > 1) {
		R600_ERR("too many dst (%d)\n", i->Instruction.NumDstRegs);
		return -EINVAL;
	}
	if (i->Instruction.Predicate) {
		R600_ERR("predicate unsupported\n");
		return -EINVAL;
	}
#if 0
	if (i->Instruction.Label) {
		R600_ERR("label unsupported\n");
		return -EINVAL;
	}
#endif
	for (j = 0; j < i->Instruction.NumSrcRegs; j++) {
		if (i->Src[j].Register.Dimension) {
			R600_ERR("unsupported src %d (dimension %d)\n", j,
				 i->Src[j].Register.Dimension);
			return -EINVAL;
		}
	}
	for (j = 0; j < i->Instruction.NumDstRegs; j++) {
		if (i->Dst[j].Register.Dimension) {
			R600_ERR("unsupported dst (dimension)\n");
			return -EINVAL;
		}
	}
	return 0;
}

static int evergreen_interp_alu(struct r600_shader_ctx *ctx, int input)
{
	int i, r;
	struct r600_bc_alu alu;
	int gpr = 0, base_chan = 0;
	int ij_index = 0;

	if (ctx->shader->input[input].interpolate == TGSI_INTERPOLATE_PERSPECTIVE) {
		ij_index = 0;
		if (ctx->shader->input[input].centroid)
			ij_index++;
	} else if (ctx->shader->input[input].interpolate == TGSI_INTERPOLATE_LINEAR) {
		ij_index = 0;
		/* if we have perspective add one */
		if (ctx->input_perspective)  {
			ij_index++;
			/* if we have perspective centroid */
			if (ctx->input_centroid)
				ij_index++;
		}
		if (ctx->shader->input[input].centroid)
			ij_index++;
	}

	/* work out gpr and base_chan from index */
	gpr = ij_index / 2;
	base_chan = (2 * (ij_index % 2)) + 1;

	for (i = 0; i < 8; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		if (i < 4)
			alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INTERP_ZW;
		else
			alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INTERP_XY;

		if ((i > 1) && (i < 6)) {
			alu.dst.sel = ctx->shader->input[input].gpr;
			alu.dst.write = 1;
		}

		alu.dst.chan = i % 4;

		alu.src[0].sel = gpr;
		alu.src[0].chan = (base_chan - (i % 2));

		alu.src[1].sel = V_SQ_ALU_SRC_PARAM_BASE + ctx->shader->input[input].lds_pos;

		alu.bank_swizzle_force = SQ_ALU_VEC_210;
		if ((i % 4) == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}


static int tgsi_declaration(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_declaration *d = &ctx->parse.FullToken.FullDeclaration;
	unsigned i;

	switch (d->Declaration.File) {
	case TGSI_FILE_INPUT:
		i = ctx->shader->ninput++;
		ctx->shader->input[i].name = d->Semantic.Name;
		ctx->shader->input[i].sid = d->Semantic.Index;
		ctx->shader->input[i].interpolate = d->Declaration.Interpolate;
		ctx->shader->input[i].centroid = d->Declaration.Centroid;
		ctx->shader->input[i].gpr = ctx->file_offset[TGSI_FILE_INPUT] + i;
		if (ctx->type == TGSI_PROCESSOR_FRAGMENT && ctx->bc->chiprev == CHIPREV_EVERGREEN) {
			/* turn input into interpolate on EG */
			if (ctx->shader->input[i].name != TGSI_SEMANTIC_POSITION) {
				if (ctx->shader->input[i].interpolate > 0) {
					ctx->shader->input[i].lds_pos = ctx->shader->nlds++;
					evergreen_interp_alu(ctx, i);
				}
			}
		}
		break;
	case TGSI_FILE_OUTPUT:
		i = ctx->shader->noutput++;
		ctx->shader->output[i].name = d->Semantic.Name;
		ctx->shader->output[i].sid = d->Semantic.Index;
		ctx->shader->output[i].gpr = ctx->file_offset[TGSI_FILE_OUTPUT] + i;
		ctx->shader->output[i].interpolate = d->Declaration.Interpolate;
		break;
	case TGSI_FILE_CONSTANT:
	case TGSI_FILE_TEMPORARY:
	case TGSI_FILE_SAMPLER:
	case TGSI_FILE_ADDRESS:
		break;
	default:
		R600_ERR("unsupported file %d declaration\n", d->Declaration.File);
		return -EINVAL;
	}
	return 0;
}

static int r600_get_temp(struct r600_shader_ctx *ctx)
{
	return ctx->temp_reg + ctx->max_driver_temp_used++;
}

/*
 * for evergreen we need to scan the shader to find the number of GPRs we need to
 * reserve for interpolation.
 *
 * we need to know if we are going to emit
 * any centroid inputs
 * if perspective and linear are required
*/
static int evergreen_gpr_count(struct r600_shader_ctx *ctx)
{
	int i;
	int num_baryc;

	ctx->input_linear = FALSE;
	ctx->input_perspective = FALSE;
	ctx->input_centroid = FALSE;
	ctx->num_interp_gpr = 1;

	/* any centroid inputs */
	for (i = 0; i < ctx->info.num_inputs; i++) {
		/* skip position/face */
		if (ctx->info.input_semantic_name[i] == TGSI_SEMANTIC_POSITION ||
		    ctx->info.input_semantic_name[i] == TGSI_SEMANTIC_FACE)
			continue;
		if (ctx->info.input_interpolate[i] == TGSI_INTERPOLATE_LINEAR)
			ctx->input_linear = TRUE;
		if (ctx->info.input_interpolate[i] == TGSI_INTERPOLATE_PERSPECTIVE)
			ctx->input_perspective = TRUE;
		if (ctx->info.input_centroid[i])
			ctx->input_centroid = TRUE;
	}

	num_baryc = 0;
	/* ignoring sample for now */
	if (ctx->input_perspective)
		num_baryc++;
	if (ctx->input_linear)
		num_baryc++;
	if (ctx->input_centroid)
		num_baryc *= 2;

	ctx->num_interp_gpr += (num_baryc + 1) >> 1;

	/* TODO PULL MODEL and LINE STIPPLE, FIXED PT POS */
	return ctx->num_interp_gpr;
}

int r600_shader_from_tgsi(const struct tgsi_token *tokens, struct r600_shader *shader, u32 **literals)
{
	struct tgsi_full_immediate *immediate;
	struct r600_shader_ctx ctx;
	struct r600_bc_output output[32];
	unsigned output_done, noutput;
	unsigned opcode;
	int i, r = 0, pos0;

	ctx.bc = &shader->bc;
	ctx.shader = shader;
	r = r600_bc_init(ctx.bc, shader->family);
	if (r)
		return r;
	ctx.tokens = tokens;
	tgsi_scan_shader(tokens, &ctx.info);
	tgsi_parse_init(&ctx.parse, tokens);
	ctx.type = ctx.parse.FullHeader.Processor.Processor;
	shader->processor_type = ctx.type;
	ctx.bc->type = shader->processor_type;

	/* register allocations */
	/* Values [0,127] correspond to GPR[0..127].
	 * Values [128,159] correspond to constant buffer bank 0
	 * Values [160,191] correspond to constant buffer bank 1
	 * Values [256,511] correspond to cfile constants c[0..255]. (Gone on EG)
	 * Values [256,287] correspond to constant buffer bank 2 (EG)
	 * Values [288,319] correspond to constant buffer bank 3 (EG)
	 * Other special values are shown in the list below.
	 * 244  ALU_SRC_1_DBL_L: special constant 1.0 double-float, LSW. (RV670+)
	 * 245  ALU_SRC_1_DBL_M: special constant 1.0 double-float, MSW. (RV670+)
	 * 246  ALU_SRC_0_5_DBL_L: special constant 0.5 double-float, LSW. (RV670+)
	 * 247  ALU_SRC_0_5_DBL_M: special constant 0.5 double-float, MSW. (RV670+)
	 * 248	SQ_ALU_SRC_0: special constant 0.0.
	 * 249	SQ_ALU_SRC_1: special constant 1.0 float.
	 * 250	SQ_ALU_SRC_1_INT: special constant 1 integer.
	 * 251	SQ_ALU_SRC_M_1_INT: special constant -1 integer.
	 * 252	SQ_ALU_SRC_0_5: special constant 0.5 float.
	 * 253	SQ_ALU_SRC_LITERAL: literal constant.
	 * 254	SQ_ALU_SRC_PV: previous vector result.
	 * 255	SQ_ALU_SRC_PS: previous scalar result.
	 */
	for (i = 0; i < TGSI_FILE_COUNT; i++) {
		ctx.file_offset[i] = 0;
	}
	if (ctx.type == TGSI_PROCESSOR_VERTEX) {
		ctx.file_offset[TGSI_FILE_INPUT] = 1;
		if (ctx.bc->chiprev == CHIPREV_EVERGREEN) {
			r600_bc_add_cfinst(ctx.bc, EG_V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS);
		} else {
			r600_bc_add_cfinst(ctx.bc, V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS);
		}
	}
	if (ctx.type == TGSI_PROCESSOR_FRAGMENT && ctx.bc->chiprev == CHIPREV_EVERGREEN) {
		ctx.file_offset[TGSI_FILE_INPUT] = evergreen_gpr_count(&ctx);
	}
	ctx.file_offset[TGSI_FILE_OUTPUT] = ctx.file_offset[TGSI_FILE_INPUT] +
						ctx.info.file_count[TGSI_FILE_INPUT];
	ctx.file_offset[TGSI_FILE_TEMPORARY] = ctx.file_offset[TGSI_FILE_OUTPUT] +
						ctx.info.file_count[TGSI_FILE_OUTPUT];

	/* Outside the GPR range. This will be translated to one of the
	 * kcache banks later. */
	ctx.file_offset[TGSI_FILE_CONSTANT] = 512;

	ctx.file_offset[TGSI_FILE_IMMEDIATE] = V_SQ_ALU_SRC_LITERAL;
	ctx.temp_reg = ctx.file_offset[TGSI_FILE_TEMPORARY] +
			ctx.info.file_count[TGSI_FILE_TEMPORARY];

	ctx.nliterals = 0;
	ctx.literals = NULL;

	while (!tgsi_parse_end_of_tokens(&ctx.parse)) {
		tgsi_parse_token(&ctx.parse);
		switch (ctx.parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
			immediate = &ctx.parse.FullToken.FullImmediate;
			ctx.literals = realloc(ctx.literals, (ctx.nliterals + 1) * 16);
			if(ctx.literals == NULL) {
				r = -ENOMEM;
				goto out_err;
			}
			ctx.literals[ctx.nliterals * 4 + 0] = immediate->u[0].Uint;
			ctx.literals[ctx.nliterals * 4 + 1] = immediate->u[1].Uint;
			ctx.literals[ctx.nliterals * 4 + 2] = immediate->u[2].Uint;
			ctx.literals[ctx.nliterals * 4 + 3] = immediate->u[3].Uint;
			ctx.nliterals++;
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
			r = tgsi_declaration(&ctx);
			if (r)
				goto out_err;
			break;
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			r = tgsi_is_supported(&ctx);
			if (r)
				goto out_err;
			ctx.max_driver_temp_used = 0;
			/* reserve first tmp for everyone */
			r600_get_temp(&ctx);
			opcode = ctx.parse.FullToken.FullInstruction.Instruction.Opcode;
			if (ctx.bc->chiprev == CHIPREV_EVERGREEN)
				ctx.inst_info = &eg_shader_tgsi_instruction[opcode];
			else
				ctx.inst_info = &r600_shader_tgsi_instruction[opcode];
			r = ctx.inst_info->process(&ctx);
			if (r)
				goto out_err;
			break;
		case TGSI_TOKEN_TYPE_PROPERTY:
			break;
		default:
			R600_ERR("unsupported token type %d\n", ctx.parse.FullToken.Token.Type);
			r = -EINVAL;
			goto out_err;
		}
	}
	/* export output */
	noutput = shader->noutput;
	for (i = 0, pos0 = 0; i < noutput; i++) {
		memset(&output[i], 0, sizeof(struct r600_bc_output));
		output[i].gpr = shader->output[i].gpr;
		output[i].elem_size = 3;
		output[i].swizzle_x = 0;
		output[i].swizzle_y = 1;
		output[i].swizzle_z = 2;
		output[i].swizzle_w = 3;
		output[i].barrier = 1;
		output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM;
		output[i].array_base = i - pos0;
		output[i].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
		switch (ctx.type) {
		case TGSI_PROCESSOR_VERTEX:
			if (shader->output[i].name == TGSI_SEMANTIC_POSITION) {
				output[i].array_base = 60;
				output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS;
				/* position doesn't count in array_base */
				pos0++;
			}
			if (shader->output[i].name == TGSI_SEMANTIC_PSIZE) {
				output[i].array_base = 61;
				output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS;
				/* position doesn't count in array_base */
				pos0++;
			}
			break;
		case TGSI_PROCESSOR_FRAGMENT:
			if (shader->output[i].name == TGSI_SEMANTIC_COLOR) {
				output[i].array_base = shader->output[i].sid;
				output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
			} else if (shader->output[i].name == TGSI_SEMANTIC_POSITION) {
				output[i].array_base = 61;
				output[i].swizzle_x = 2;
				output[i].swizzle_y = 7;
				output[i].swizzle_z = output[i].swizzle_w = 7;
				output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
			} else if (shader->output[i].name == TGSI_SEMANTIC_STENCIL) {
				output[i].array_base = 61;
				output[i].swizzle_x = 7;
				output[i].swizzle_y = 1;
				output[i].swizzle_z = output[i].swizzle_w = 7;
				output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
			} else {
				R600_ERR("unsupported fragment output name %d\n", shader->output[i].name);
				r = -EINVAL;
				goto out_err;
			}
			break;
		default:
			R600_ERR("unsupported processor type %d\n", ctx.type);
			r = -EINVAL;
			goto out_err;
		}
	}
	/* add fake param output for vertex shader if no param is exported */
	if (ctx.type == TGSI_PROCESSOR_VERTEX) {
		for (i = 0, pos0 = 0; i < noutput; i++) {
			if (output[i].type == V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM) {
				pos0 = 1;
				break;
			}
		}
		if (!pos0) {
			memset(&output[i], 0, sizeof(struct r600_bc_output));
			output[i].gpr = 0;
			output[i].elem_size = 3;
			output[i].swizzle_x = 0;
			output[i].swizzle_y = 1;
			output[i].swizzle_z = 2;
			output[i].swizzle_w = 3;
			output[i].barrier = 1;
			output[i].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM;
			output[i].array_base = 0;
			output[i].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
			noutput++;
		}
	}
	/* add fake pixel export */
	if (ctx.type == TGSI_PROCESSOR_FRAGMENT && !noutput) {
		memset(&output[0], 0, sizeof(struct r600_bc_output));
		output[0].gpr = 0;
		output[0].elem_size = 3;
		output[0].swizzle_x = 7;
		output[0].swizzle_y = 7;
		output[0].swizzle_z = 7;
		output[0].swizzle_w = 7;
		output[0].barrier = 1;
		output[0].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
		output[0].array_base = 0;
		output[0].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
		noutput++;
	}
	/* set export done on last export of each type */
	for (i = noutput - 1, output_done = 0; i >= 0; i--) {
		if (i == (noutput - 1)) {
			output[i].end_of_program = 1;
		}
		if (!(output_done & (1 << output[i].type))) {
			output_done |= (1 << output[i].type);
			output[i].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE);
		}
	}
	/* add output to bytecode */
	for (i = 0; i < noutput; i++) {
		r = r600_bc_add_output(ctx.bc, &output[i]);
		if (r)
			goto out_err;
	}
	*literals = ctx.literals;
	tgsi_parse_free(&ctx.parse);
	return 0;
out_err:
	free(ctx.literals);
	tgsi_parse_free(&ctx.parse);
	return r;
}

static int tgsi_unsupported(struct r600_shader_ctx *ctx)
{
	R600_ERR("%d tgsi opcode unsupported\n", ctx->inst_info->tgsi_opcode);
	return -EINVAL;
}

static int tgsi_end(struct r600_shader_ctx *ctx)
{
	return 0;
}

static int tgsi_src(struct r600_shader_ctx *ctx,
			const struct tgsi_full_src_register *tgsi_src,
			struct r600_bc_alu_src *r600_src)
{
	memset(r600_src, 0, sizeof(struct r600_bc_alu_src));
	r600_src->neg = tgsi_src->Register.Negate;
	r600_src->abs = tgsi_src->Register.Absolute;
	if (tgsi_src->Register.File == TGSI_FILE_IMMEDIATE) {		
		int index;
		if((tgsi_src->Register.SwizzleX == tgsi_src->Register.SwizzleY) &&
			(tgsi_src->Register.SwizzleX == tgsi_src->Register.SwizzleZ) &&
			(tgsi_src->Register.SwizzleX == tgsi_src->Register.SwizzleW)) {

			index = tgsi_src->Register.Index * 4 + tgsi_src->Register.SwizzleX;
			r600_bc_special_constants(ctx->literals[index], &r600_src->sel, &r600_src->neg);
			if (r600_src->sel != V_SQ_ALU_SRC_LITERAL)
				return 0;
		}
		index = tgsi_src->Register.Index;
		r600_src->sel = V_SQ_ALU_SRC_LITERAL;
		r600_src->value = ctx->literals + index * 4;
	} else {
		if (tgsi_src->Register.Indirect)
			r600_src->rel = V_SQ_REL_RELATIVE;
		r600_src->sel = tgsi_src->Register.Index;
		r600_src->sel += ctx->file_offset[tgsi_src->Register.File];
	}
	return 0;
}

static int tgsi_dst(struct r600_shader_ctx *ctx,
			const struct tgsi_full_dst_register *tgsi_dst,
			unsigned swizzle,
			struct r600_bc_alu_dst *r600_dst)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;

	r600_dst->sel = tgsi_dst->Register.Index;
	r600_dst->sel += ctx->file_offset[tgsi_dst->Register.File];
	r600_dst->chan = swizzle;
	r600_dst->write = 1;
	if (tgsi_dst->Register.Indirect)
		r600_dst->rel = V_SQ_REL_RELATIVE;
	if (inst->Instruction.Saturate) {
		r600_dst->clamp = 1;
	}
	return 0;
}

static unsigned tgsi_chan(const struct tgsi_full_src_register *tgsi_src, unsigned swizzle)
{
	switch (swizzle) {
	case 0:
		return tgsi_src->Register.SwizzleX;
	case 1:
		return tgsi_src->Register.SwizzleY;
	case 2:
		return tgsi_src->Register.SwizzleZ;
	case 3:
		return tgsi_src->Register.SwizzleW;
	default:
		return 0;
	}
}

static int tgsi_split_constant(struct r600_shader_ctx *ctx, struct r600_bc_alu_src r600_src[3])
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, j, k, nconst, r;

	for (i = 0, nconst = 0; i < inst->Instruction.NumSrcRegs; i++) {
		if (inst->Src[i].Register.File == TGSI_FILE_CONSTANT) {
			nconst++;
		}
		r = tgsi_src(ctx, &inst->Src[i], &r600_src[i]);
		if (r) {
			return r;
		}
	}
	for (i = 0, j = nconst - 1; i < inst->Instruction.NumSrcRegs; i++) {
		if (j > 0 && inst->Src[i].Register.File == TGSI_FILE_CONSTANT) {
			int treg = r600_get_temp(ctx);
			for (k = 0; k < 4; k++) {
				memset(&alu, 0, sizeof(struct r600_bc_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
				alu.src[0].sel = r600_src[i].sel;
				alu.src[0].chan = k;
				alu.src[0].rel = r600_src[i].rel;
				alu.dst.sel = treg;
				alu.dst.chan = k;
				alu.dst.write = 1;
				if (k == 3)
					alu.last = 1;
				r = r600_bc_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
			r600_src[i].sel = treg;
			r600_src[i].rel =0;
			j--;
		}
	}
	return 0;
}

/* need to move any immediate into a temp - for trig functions which use literal for PI stuff */
static int tgsi_split_literal_constant(struct r600_shader_ctx *ctx, struct r600_bc_alu_src r600_src[3])
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, j, k, nliteral, r;

	for (i = 0, nliteral = 0; i < inst->Instruction.NumSrcRegs; i++) {
		if (r600_src[i].sel == V_SQ_ALU_SRC_LITERAL) {
			nliteral++;
		}
	}
	for (i = 0, j = nliteral - 1; i < inst->Instruction.NumSrcRegs; i++) {
		if (j > 0 && r600_src[i].sel == V_SQ_ALU_SRC_LITERAL) {
			int treg = r600_get_temp(ctx);
			for (k = 0; k < 4; k++) {
				memset(&alu, 0, sizeof(struct r600_bc_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
				alu.src[0].sel = r600_src[i].sel;
				alu.src[0].chan = k;
				alu.src[0].value = r600_src[i].value;
				alu.dst.sel = treg;
				alu.dst.chan = k;
				alu.dst.write = 1;
				if (k == 3)
					alu.last = 1;
				r = r600_bc_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
			r600_src[i].sel = treg;
			j--;
		}
	}
	return 0;
}

static int tgsi_last_instruction(unsigned writemask)
{
	int i, lasti = 0;

	for (i = 0; i < 4; i++) {
		if (writemask & (1 << i)) {
			lasti = i;
		}
	}
	return lasti;
}

static int tgsi_op2_s(struct r600_shader_ctx *ctx, int swap)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int i, j, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		alu.inst = ctx->inst_info->r600_opcode;
		if (!swap) {
			for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
				alu.src[j] = r600_src[j];
				alu.src[j].chan = tgsi_chan(&inst->Src[j], i);
			}
		} else {
			alu.src[0] = r600_src[1];
			alu.src[0].chan = tgsi_chan(&inst->Src[1], i);

			alu.src[1] = r600_src[0];
			alu.src[1].chan = tgsi_chan(&inst->Src[0], i);
		}
		/* handle some special cases */
		switch (ctx->inst_info->tgsi_opcode) {
		case TGSI_OPCODE_SUB:
			alu.src[1].neg = 1;
			break;
		case TGSI_OPCODE_ABS:
			alu.src[0].abs = 1;
			break;
		default:
			break;
		}
		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_op2(struct r600_shader_ctx *ctx)
{
	return tgsi_op2_s(ctx, 0);
}

static int tgsi_op2_swap(struct r600_shader_ctx *ctx)
{
	return tgsi_op2_s(ctx, 1);
}

/*
 * r600 - trunc to -PI..PI range
 * r700 - normalize by dividing by 2PI
 * see fdo bug 27901
 */
static int tgsi_setup_trig(struct r600_shader_ctx *ctx,
			   struct r600_bc_alu_src r600_src[3])
{
	static float half_inv_pi = 1.0 /(3.1415926535 * 2);
	static float double_pi = 3.1415926535 * 2;
	static float neg_pi = -3.1415926535;

	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	int r;
	struct r600_bc_alu alu;

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
	alu.is_op3 = 1;

	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0] = r600_src[0];
	alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

	alu.src[1].sel = V_SQ_ALU_SRC_LITERAL;
	alu.src[1].chan = 0;
	alu.src[1].value = (uint32_t *)&half_inv_pi;
	alu.src[2].sel = V_SQ_ALU_SRC_0_5;
	alu.src[2].chan = 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT);

	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
	alu.is_op3 = 1;

	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;

	alu.src[1].sel = V_SQ_ALU_SRC_LITERAL;
	alu.src[1].chan = 0;
	alu.src[2].sel = V_SQ_ALU_SRC_LITERAL;
	alu.src[2].chan = 1;

	if (ctx->bc->chiprev == CHIPREV_R600) {
		alu.src[1].value = (uint32_t *)&double_pi;
		alu.src[2].value = (uint32_t *)&neg_pi;
	} else {
		alu.src[1].sel = V_SQ_ALU_SRC_1;
		alu.src[2].sel = V_SQ_ALU_SRC_0_5;
		alu.src[2].neg = 1;
	}

	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	return 0;
}

static int tgsi_trig(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int i, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	r = tgsi_setup_trig(ctx, r600_src);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = ctx->inst_info->r600_opcode;
	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	/* replicate result */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);

		alu.src[0].sel = ctx->temp_reg;
		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;
		if (i == lasti)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_scs(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int r;

	/* We'll only need the trig stuff if we are going to write to the
	 * X or Y components of the destination vector.
	 */
	if (likely(inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XY)) {
		r = tgsi_setup_trig(ctx, r600_src);
		if (r)
			return r;
	}

	/* dst.x = COS */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS);
		r = tgsi_dst(ctx, &inst->Dst[0], 0, &alu.dst);
		if (r)
			return r;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst.y = SIN */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN);
		r = tgsi_dst(ctx, &inst->Dst[0], 1, &alu.dst);
		if (r)
			return r;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst.z = 0.0; */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);

		r = tgsi_dst(ctx, &inst->Dst[0], 2, &alu.dst);
		if (r)
			return r;

		alu.src[0].sel = V_SQ_ALU_SRC_0;
		alu.src[0].chan = 0;

		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst.w = 1.0; */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);

		r = tgsi_dst(ctx, &inst->Dst[0], 3, &alu.dst);
		if (r)
			return r;

		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;

		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	return 0;
}

static int tgsi_kill(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = ctx->inst_info->r600_opcode;

		alu.dst.chan = i;

		alu.src[0].sel = V_SQ_ALU_SRC_0;

		if (ctx->inst_info->tgsi_opcode == TGSI_OPCODE_KILP) {
			alu.src[1].sel = V_SQ_ALU_SRC_1;
			alu.src[1].neg = 1;
		} else {
			r = tgsi_src(ctx, &inst->Src[0], &alu.src[1]);
			if (r)
				return r;
			alu.src[1].chan = tgsi_chan(&inst->Src[0], i);
		}
		if (i == 3) {
			alu.last = 1;
		}
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* kill must be last in ALU */
	ctx->bc->force_add_cf = 1;
	ctx->shader->uses_kill = TRUE;
	return 0;
}

static int tgsi_lit(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	struct r600_bc_alu_src r600_src[3];
	int r;

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;

	/* dst.x, <- 1.0  */
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
	alu.src[0].sel  = V_SQ_ALU_SRC_1; /*1.0*/
	alu.src[0].chan = 0;
	r = tgsi_dst(ctx, &inst->Dst[0], 0, &alu.dst);
	if (r)
		return r;
	alu.dst.write = (inst->Dst[0].Register.WriteMask >> 0) & 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	/* dst.y = max(src.x, 0.0) */
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX);
	alu.src[0] = r600_src[0];
	alu.src[1].sel  = V_SQ_ALU_SRC_0; /*0.0*/
	alu.src[1].chan = 0;
	r = tgsi_dst(ctx, &inst->Dst[0], 1, &alu.dst);
	if (r)
		return r;
	alu.dst.write = (inst->Dst[0].Register.WriteMask >> 1) & 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	/* dst.w, <- 1.0  */
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
	alu.src[0].sel  = V_SQ_ALU_SRC_1;
	alu.src[0].chan = 0;
	r = tgsi_dst(ctx, &inst->Dst[0], 3, &alu.dst);
	if (r)
		return r;
	alu.dst.write = (inst->Dst[0].Register.WriteMask >> 3) & 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	if (inst->Dst[0].Register.WriteMask & (1 << 2))
	{
		int chan;
		int sel;

		/* dst.z = log(src.y) */
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED);
		alu.src[0] = r600_src[0];
		alu.src[0].chan = tgsi_chan(&inst->Src[0], 1);
		r = tgsi_dst(ctx, &inst->Dst[0], 2, &alu.dst);
		if (r)
			return r;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		chan = alu.dst.chan;
		sel = alu.dst.sel;

		/* tmp.x = amd MUL_LIT(src.w, dst.z, src.x ) */
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT);
		alu.src[0] = r600_src[0];
		alu.src[0].chan = tgsi_chan(&inst->Src[0], 3);
		alu.src[1].sel  = sel;
		alu.src[1].chan = chan;

		alu.src[2] = r600_src[0];
		alu.src[2].chan = tgsi_chan(&inst->Src[0], 0);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		/* dst.z = exp(tmp.x) */
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;
		r = tgsi_dst(ctx, &inst->Dst[0], 2, &alu.dst);
		if (r)
			return r;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_rsq(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));

	/* FIXME:
	 * For state trackers other than OpenGL, we'll want to use
	 * _RECIPSQRT_IEEE instead.
	 */
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED);

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		r = tgsi_src(ctx, &inst->Src[i], &alu.src[i]);
		if (r)
			return r;
		alu.src[i].chan = tgsi_chan(&inst->Src[i], 0);
		alu.src[i].abs = 1;
	}
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* replicate result */
	return tgsi_helper_tempx_replicate(ctx);
}

static int tgsi_helper_tempx_replicate(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.src[0].sel = ctx->temp_reg;
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.dst.chan = i;
		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;
		if (i == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_trans_srcx_replicate(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = ctx->inst_info->r600_opcode;
	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		r = tgsi_src(ctx, &inst->Src[i], &alu.src[i]);
		if (r)
			return r;
		alu.src[i].chan = tgsi_chan(&inst->Src[i], 0);
	}
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* replicate result */
	return tgsi_helper_tempx_replicate(ctx);
}

static int tgsi_pow(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int r;

	/* LOG2(a) */
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
	r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
	if (r)
		return r;
	alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* b * LOG2(a) */
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
	r = tgsi_src(ctx, &inst->Src[1], &alu.src[0]);
	if (r)
		return r;
	alu.src[0].chan = tgsi_chan(&inst->Src[1], 0);
	alu.src[1].sel = ctx->temp_reg;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* POW(a,b) = EXP2(b * LOG2(a))*/
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
	alu.src[0].sel = ctx->temp_reg;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bc_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	return tgsi_helper_tempx_replicate(ctx);
}

static int tgsi_ssg(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	struct r600_bc_alu_src r600_src[3];
	int i, r;

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;

	/* tmp = (src > 0 ? 1 : src) */
	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT);
		alu.is_op3 = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;

		alu.src[0] = r600_src[0];
		alu.src[0].chan = tgsi_chan(&inst->Src[0], i);

		alu.src[1].sel = V_SQ_ALU_SRC_1;

		alu.src[2] = r600_src[0];
		alu.src[2].chan = tgsi_chan(&inst->Src[0], i);
		if (i == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst = (-tmp > 0 ? -1 : tmp) */
	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT);
		alu.is_op3 = 1;
		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;
		alu.src[0].neg = 1;

		alu.src[1].sel = V_SQ_ALU_SRC_1;
		alu.src[1].neg = 1;

		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].chan = i;

		if (i == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_helper_copy(struct r600_shader_ctx *ctx, struct tgsi_full_instruction *inst)
{
	struct r600_bc_alu alu;
	int i, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		if (!(inst->Dst[0].Register.WriteMask & (1 << i))) {
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP);
			alu.dst.chan = i;
		} else {
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
			r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
			if (r)
				return r;
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = i;
		}
		if (i == 3) {
			alu.last = 1;
		}
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_op3(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int i, j, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = ctx->inst_info->r600_opcode;
		for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
			alu.src[j] = r600_src[j];
			alu.src[j].chan = tgsi_chan(&inst->Src[j], i);
		}

		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		alu.dst.chan = i;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_dp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int i, j, r;

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;
	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = ctx->inst_info->r600_opcode;
		for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
			alu.src[j] = r600_src[j];
			alu.src[j].chan = tgsi_chan(&inst->Src[j], i);
		}

		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		alu.dst.chan = i;
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;
		/* handle some special cases */
		switch (ctx->inst_info->tgsi_opcode) {
		case TGSI_OPCODE_DP2:
			if (i > 1) {
				alu.src[0].sel = alu.src[1].sel = V_SQ_ALU_SRC_0;
				alu.src[0].chan = alu.src[1].chan = 0;
			}
			break;
		case TGSI_OPCODE_DP3:
			if (i > 2) {
				alu.src[0].sel = alu.src[1].sel = V_SQ_ALU_SRC_0;
				alu.src[0].chan = alu.src[1].chan = 0;
			}
			break;
		case TGSI_OPCODE_DPH:
			if (i == 3) {
				alu.src[0].sel = V_SQ_ALU_SRC_1;
				alu.src[0].chan = 0;
				alu.src[0].neg = 0;
			}
			break;
		default:
			break;
		}
		if (i == 3) {
			alu.last = 1;
		}
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_tex(struct r600_shader_ctx *ctx)
{
	static float one_point_five = 1.5f;
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_tex tex;
	struct r600_bc_alu alu;
	unsigned src_gpr;
	int r, i;
	int opcode;
	boolean src_not_temp =
		inst->Src[0].Register.File != TGSI_FILE_TEMPORARY &&
		inst->Src[0].Register.File != TGSI_FILE_INPUT;

	src_gpr = ctx->file_offset[inst->Src[0].Register.File] + inst->Src[0].Register.Index;

	if (inst->Instruction.Opcode == TGSI_OPCODE_TXP) {
		/* Add perspective divide */
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;

		alu.src[0].chan = tgsi_chan(&inst->Src[0], 3);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.last = 1;
		alu.dst.write = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		for (i = 0; i < 3; i++) {
			memset(&alu, 0, sizeof(struct r600_bc_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 3;
			r = tgsi_src(ctx, &inst->Src[0], &alu.src[1]);
			if (r)
				return r;
			alu.src[1].chan = tgsi_chan(&inst->Src[0], i);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = i;
			alu.dst.write = 1;
			r = r600_bc_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.last = 1;
		alu.dst.write = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
		src_not_temp = FALSE;
		src_gpr = ctx->temp_reg;
	}

	if (inst->Texture.Texture == TGSI_TEXTURE_CUBE) {
		int src_chan, src2_chan;

		/* tmp1.xyzw = CUBE(R0.zzxy, R0.yxzz) */
		for (i = 0; i < 4; i++) {
			memset(&alu, 0, sizeof(struct r600_bc_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE);
			switch (i) {
			case 0:
				src_chan = 2;
				src2_chan = 1;
				break;
			case 1:
				src_chan = 2;
				src2_chan = 0;
				break;
			case 2:
				src_chan = 0;
				src2_chan = 2;
				break;
			case 3:
				src_chan = 1;
				src2_chan = 2;
				break;
			default:
				assert(0);
				src_chan = 0;
				src2_chan = 0;
				break;
			}
			r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
			if (r)
				return r;
			alu.src[0].chan = tgsi_chan(&inst->Src[0], src_chan);
			r = tgsi_src(ctx, &inst->Src[0], &alu.src[1]);
			if (r)
				return r;
			alu.src[1].chan = tgsi_chan(&inst->Src[0], src2_chan);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = i;
			if (i == 3)
				alu.last = 1;
			alu.dst.write = 1;
			r = r600_bc_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		/* tmp1.z = RCP_e(|tmp1.z|) */
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 2;
		alu.src[0].abs = 1;
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 2;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		/* MULADD R0.x,  R0.x,  PS1,  (0x3FC00000, 1.5f).x
		 * MULADD R0.y,  R0.y,  PS1,  (0x3FC00000, 1.5f).x
		 * muladd has no writemask, have to use another temp
		 */
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
		alu.is_op3 = 1;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;
		alu.src[1].sel = ctx->temp_reg;
		alu.src[1].chan = 2;

		alu.src[2].sel = V_SQ_ALU_SRC_LITERAL;
		alu.src[2].chan = 0;
		alu.src[2].value = (u32*)&one_point_five;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
		alu.is_op3 = 1;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 1;
		alu.src[1].sel = ctx->temp_reg;
		alu.src[1].chan = 2;

		alu.src[2].sel = V_SQ_ALU_SRC_LITERAL;
		alu.src[2].chan = 0;
		alu.src[2].value = (u32*)&one_point_five;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;

		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		src_not_temp = FALSE;
		src_gpr = ctx->temp_reg;
	}

	if (src_not_temp) {
		for (i = 0; i < 4; i++) {
			memset(&alu, 0, sizeof(struct r600_bc_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
			alu.src[0].sel = src_gpr;
			alu.src[0].chan = tgsi_chan(&inst->Src[0], i);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = i;
			if (i == 3)
				alu.last = 1;
			alu.dst.write = 1;
			r = r600_bc_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
		src_gpr = ctx->temp_reg;
	}

	opcode = ctx->inst_info->r600_opcode;
	if (opcode == SQ_TEX_INST_SAMPLE &&
	    (inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D || inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D))
		opcode = SQ_TEX_INST_SAMPLE_C;

	memset(&tex, 0, sizeof(struct r600_bc_tex));
	tex.inst = opcode;
	tex.sampler_id = ctx->file_offset[inst->Src[1].Register.File] + inst->Src[1].Register.Index;
	tex.resource_id = tex.sampler_id;
	tex.src_gpr = src_gpr;
	tex.dst_gpr = ctx->file_offset[inst->Dst[0].Register.File] + inst->Dst[0].Register.Index;
	tex.dst_sel_x = (inst->Dst[0].Register.WriteMask & 1) ? 0 : 7;
	tex.dst_sel_y = (inst->Dst[0].Register.WriteMask & 2) ? 1 : 7;
	tex.dst_sel_z = (inst->Dst[0].Register.WriteMask & 4) ? 2 : 7;
	tex.dst_sel_w = (inst->Dst[0].Register.WriteMask & 8) ? 3 : 7;
	tex.src_sel_x = 0;
	tex.src_sel_y = 1;
	tex.src_sel_z = 2;
	tex.src_sel_w = 3;

	if (inst->Texture.Texture == TGSI_TEXTURE_CUBE) {
		tex.src_sel_x = 1;
		tex.src_sel_y = 0;
		tex.src_sel_z = 3;
		tex.src_sel_w = 1;
	}

	if (inst->Texture.Texture != TGSI_TEXTURE_RECT) {
		tex.coord_type_x = 1;
		tex.coord_type_y = 1;
		tex.coord_type_z = 1;
		tex.coord_type_w = 1;
	}

	if (inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D || inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D)
		tex.src_sel_w = 2;

	r = r600_bc_add_tex(ctx->bc, &tex);
	if (r)
		return r;

	/* add shadow ambient support  - gallium doesn't do it yet */
	return 0;
}

static int tgsi_lrp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);
	unsigned i;
	int r;

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;

	/* optimize if it's just an equal balance */
	if(r600_src[0].sel == V_SQ_ALU_SRC_0_5) {
		for (i = 0; i < lasti + 1; i++) {
			if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
				continue;

			memset(&alu, 0, sizeof(struct r600_bc_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD);
			alu.src[0] = r600_src[1];
			alu.src[0].chan = tgsi_chan(&inst->Src[1], i);
			alu.src[1] = r600_src[2];
			alu.src[1].chan = tgsi_chan(&inst->Src[2], i);
			alu.omod = 3;
			r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
			if (r)
				return r;

			alu.dst.chan = i;
			if (i == lasti) {
				alu.last = 1;
			}
			r = r600_bc_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
		return 0;
	}

	/* 1 - src0 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;
		alu.src[1] = r600_src[0];
		alu.src[1].chan = tgsi_chan(&inst->Src[0], i);
		alu.src[1].neg = 1;
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		if (i == lasti) {
			alu.last = 1;
		}
		alu.dst.write = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* (1 - src0) * src2 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;
		alu.src[1] = r600_src[2];
		alu.src[1].chan = tgsi_chan(&inst->Src[2], i);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		if (i == lasti) {
			alu.last = 1;
		}
		alu.dst.write = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* src0 * src1 + (1 - src0) * src2 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
		alu.is_op3 = 1;
		alu.src[0] = r600_src[0];
		alu.src[0].chan = tgsi_chan(&inst->Src[0], i);
		alu.src[1] = r600_src[1];
		alu.src[1].chan = tgsi_chan(&inst->Src[1], i);
		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].chan = i;

		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		alu.dst.chan = i;
		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_cmp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	int i, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;

	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE);
		alu.src[0] = r600_src[0];
		alu.src[0].chan = tgsi_chan(&inst->Src[0], i);

		alu.src[1] = r600_src[2];
		alu.src[1].chan = tgsi_chan(&inst->Src[2], i);

		alu.src[2] = r600_src[1];
		alu.src[2].chan = tgsi_chan(&inst->Src[1], i);

		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		alu.dst.chan = i;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		if (i == lasti)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_xpd(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3];
	struct r600_bc_alu alu;
	uint32_t use_temp = 0;
	int i, r;

	if (inst->Dst[0].Register.WriteMask != 0xf)
		use_temp = 1;

	r = tgsi_split_constant(ctx, r600_src);
	if (r)
		return r;
	r = tgsi_split_literal_constant(ctx, r600_src);
	if (r)
		return r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);

		alu.src[0] = r600_src[0];
		switch (i) {
		case 0:
			alu.src[0].chan = tgsi_chan(&inst->Src[0], 2);
			break;
		case 1:
			alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);
			break;
		case 2:
			alu.src[0].chan = tgsi_chan(&inst->Src[0], 1);
			break;
		case 3:
			alu.src[0].sel = V_SQ_ALU_SRC_0;
			alu.src[0].chan = i;
		}

		alu.src[1] = r600_src[1];
		switch (i) {
		case 0:
			alu.src[1].chan = tgsi_chan(&inst->Src[1], 1);
			break;
		case 1:
			alu.src[1].chan = tgsi_chan(&inst->Src[1], 2);
			break;
		case 2:
			alu.src[1].chan = tgsi_chan(&inst->Src[1], 0);
			break;
		case 3:
			alu.src[1].sel = V_SQ_ALU_SRC_0;
			alu.src[1].chan = i;
		}

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		alu.dst.write = 1;

		if (i == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);

		alu.src[0] = r600_src[0];
		switch (i) {
		case 0:
			alu.src[0].chan = tgsi_chan(&inst->Src[0], 1);
			break;
		case 1:
			alu.src[0].chan = tgsi_chan(&inst->Src[0], 2);
			break;
		case 2:
			alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);
			break;
		case 3:
			alu.src[0].sel = V_SQ_ALU_SRC_0;
			alu.src[0].chan = i;
		}

		alu.src[1] = r600_src[1];
		switch (i) {
		case 0:
			alu.src[1].chan = tgsi_chan(&inst->Src[1], 2);
			break;
		case 1:
			alu.src[1].chan = tgsi_chan(&inst->Src[1], 0);
			break;
		case 2:
			alu.src[1].chan = tgsi_chan(&inst->Src[1], 1);
			break;
		case 3:
			alu.src[1].sel = V_SQ_ALU_SRC_0;
			alu.src[1].chan = i;
		}

		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].neg = 1;
		alu.src[2].chan = i;

		if (use_temp)
			alu.dst.sel = ctx->temp_reg;
		else {
			r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
			if (r)
				return r;
		}
		alu.dst.chan = i;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		if (i == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	if (use_temp)
		return tgsi_helper_copy(ctx, inst);
	return 0;
}

static int tgsi_exp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu_src r600_src[3] = { { 0 } };
	struct r600_bc_alu alu;
	int r;

	/* result.x = 2^floor(src); */
	if (inst->Dst[0].Register.WriteMask & 1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR);
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;

		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.y = tmp - floor(tmp); */
	if ((inst->Dst[0].Register.WriteMask >> 1) & 1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT);
		alu.src[0] = r600_src[0];
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;
		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.dst.sel = ctx->temp_reg;
//		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
//		if (r)
//			return r;
		alu.dst.write = 1;
		alu.dst.chan = 1;

		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.z = RoughApprox2ToX(tmp);*/
	if ((inst->Dst[0].Register.WriteMask >> 2) & 0x1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;
		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.write = 1;
		alu.dst.chan = 2;

		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.w = 1.0;*/
	if ((inst->Dst[0].Register.WriteMask >> 3) & 0x1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return tgsi_helper_copy(ctx, inst);
}

static int tgsi_log(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int r;

	/* result.x = floor(log2(src)); */
	if (inst->Dst[0].Register.WriteMask & 1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;

		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.y = src.x / (2 ^ floor(log2(src.x))); */
	if ((inst->Dst[0].Register.WriteMask >> 1) & 1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;

		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);

		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;

		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.src[1].sel = ctx->temp_reg;
		alu.src[1].chan = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.z = log2(src);*/
	if ((inst->Dst[0].Register.WriteMask >> 2) & 1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
		r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
		if (r)
			return r;

		alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.write = 1;
		alu.dst.chan = 2;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.w = 1.0; */
	if ((inst->Dst[0].Register.WriteMask >> 3) & 1) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	return tgsi_helper_copy(ctx, inst);
}

static int tgsi_eg_arl(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int r;
	memset(&alu, 0, sizeof(struct r600_bc_alu));

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ARL:
		alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT_FLOOR;
		break;
	case TGSI_OPCODE_ARR:
		alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT;
		break;
	default:
		assert(0);
		return -1;
	}

	r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
	if (r)
		return r;
	alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);
	alu.last = 1;
	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	r = r600_bc_add_alu_type(ctx->bc, &alu, CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU));
	if (r)
		return r;
	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT;
	r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
	if (r)
		return r;
	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;
	alu.last = 1;
	r = r600_bc_add_alu_type(ctx->bc, &alu, CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU));
	if (r)
		return r;
	return 0;
}
static int tgsi_r600_arl(struct r600_shader_ctx *ctx)
{
	/* TODO from r600c, ar values don't persist between clauses */
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int r;
	memset(&alu, 0, sizeof(struct r600_bc_alu));

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ARL:
		alu.inst = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_FLOOR;
		break;
	case TGSI_OPCODE_ARR:
		alu.inst = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA;
		break;
	default:
		assert(0);
		return -1;
	}


	r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
	if (r)
		return r;
	alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);

	alu.last = 1;

	r = r600_bc_add_alu_type(ctx->bc, &alu, CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU));
	if (r)
		return r;
	ctx->bc->cf_last->r6xx_uses_waterfall = 1;
	return 0;
}

static int tgsi_opdst(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int i, r = 0;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bc_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;

		if (i == 0 || i == 3) {
			alu.src[0].sel = V_SQ_ALU_SRC_1;
		} else {
			r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
			if (r)
				return r;
			alu.src[0].chan = tgsi_chan(&inst->Src[0], i);
		}

	        if (i == 0 || i == 2) {
			alu.src[1].sel = V_SQ_ALU_SRC_1;
		} else {
			r = tgsi_src(ctx, &inst->Src[1], &alu.src[1]);
			if (r)
				return r;
			alu.src[1].chan = tgsi_chan(&inst->Src[1], i);
		}
		if (i == 3)
			alu.last = 1;
		r = r600_bc_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int emit_logic_pred(struct r600_shader_ctx *ctx, int opcode)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bc_alu alu;
	int r;

	memset(&alu, 0, sizeof(struct r600_bc_alu));
	alu.inst = opcode;
	alu.predicate = 1;

	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.dst.chan = 0;

	r = tgsi_src(ctx, &inst->Src[0], &alu.src[0]);
	if (r)
		return r;
	alu.src[0].chan = tgsi_chan(&inst->Src[0], 0);
	alu.src[1].sel = V_SQ_ALU_SRC_0;
	alu.src[1].chan = 0;

	alu.last = 1;

	r = r600_bc_add_alu_type(ctx->bc, &alu, CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE));
	if (r)
		return r;
	return 0;
}

static int pops(struct r600_shader_ctx *ctx, int pops)
{
	int alu_pop = 3;
	if (ctx->bc->cf_last) {
		if (ctx->bc->cf_last->inst == CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU) << 3)
			alu_pop = 0;
		else if (ctx->bc->cf_last->inst == CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER) << 3)
			alu_pop = 1;
	}
	alu_pop += pops;
	if (alu_pop == 1) {
		ctx->bc->cf_last->inst = CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER) << 3;
		ctx->bc->force_add_cf = 1;
	} else if (alu_pop == 2) {
		ctx->bc->cf_last->inst = CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER) << 3;
		ctx->bc->force_add_cf = 1;
	} else {
		r600_bc_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_POP));
		ctx->bc->cf_last->pop_count = pops;
		ctx->bc->cf_last->cf_addr = ctx->bc->cf_last->id + 2;
	}
	return 0;
}

static inline void callstack_decrease_current(struct r600_shader_ctx *ctx, unsigned reason)
{
	switch(reason) {
	case FC_PUSH_VPM:
		ctx->bc->callstack[ctx->bc->call_sp].current--;
		break;
	case FC_PUSH_WQM:
	case FC_LOOP:
		ctx->bc->callstack[ctx->bc->call_sp].current -= 4;
		break;
	case FC_REP:
		/* TOODO : for 16 vp asic should -= 2; */
		ctx->bc->callstack[ctx->bc->call_sp].current --;
		break;
	}
}

static inline void callstack_check_depth(struct r600_shader_ctx *ctx, unsigned reason, unsigned check_max_only)
{
	if (check_max_only) {
		int diff;
		switch (reason) {
		case FC_PUSH_VPM:
			diff = 1;
			break;
		case FC_PUSH_WQM:
			diff = 4;
			break;
		default:
			assert(0);
			diff = 0;
		}
		if ((ctx->bc->callstack[ctx->bc->call_sp].current + diff) >
		    ctx->bc->callstack[ctx->bc->call_sp].max) {
			ctx->bc->callstack[ctx->bc->call_sp].max =
				ctx->bc->callstack[ctx->bc->call_sp].current + diff;
		}
		return;
	}
	switch (reason) {
	case FC_PUSH_VPM:
		ctx->bc->callstack[ctx->bc->call_sp].current++;
		break;
	case FC_PUSH_WQM:
	case FC_LOOP:
		ctx->bc->callstack[ctx->bc->call_sp].current += 4;
		break;
	case FC_REP:
		ctx->bc->callstack[ctx->bc->call_sp].current++;
		break;
	}

	if ((ctx->bc->callstack[ctx->bc->call_sp].current) >
	    ctx->bc->callstack[ctx->bc->call_sp].max) {
		ctx->bc->callstack[ctx->bc->call_sp].max =
			ctx->bc->callstack[ctx->bc->call_sp].current;
	}
}

static void fc_set_mid(struct r600_shader_ctx *ctx, int fc_sp)
{
	struct r600_cf_stack_entry *sp = &ctx->bc->fc_stack[fc_sp];

	sp->mid = (struct r600_bc_cf **)realloc((void *)sp->mid,
						sizeof(struct r600_bc_cf *) * (sp->num_mid + 1));
	sp->mid[sp->num_mid] = ctx->bc->cf_last;
	sp->num_mid++;
}

static void fc_pushlevel(struct r600_shader_ctx *ctx, int type)
{
	ctx->bc->fc_sp++;
	ctx->bc->fc_stack[ctx->bc->fc_sp].type = type;
	ctx->bc->fc_stack[ctx->bc->fc_sp].start = ctx->bc->cf_last;
}

static void fc_poplevel(struct r600_shader_ctx *ctx)
{
	struct r600_cf_stack_entry *sp = &ctx->bc->fc_stack[ctx->bc->fc_sp];
	if (sp->mid) {
		free(sp->mid);
		sp->mid = NULL;
	}
	sp->num_mid = 0;
	sp->start = NULL;
	sp->type = 0;
	ctx->bc->fc_sp--;
}

#if 0
static int emit_return(struct r600_shader_ctx *ctx)
{
	r600_bc_add_cfinst(ctx->bc, V_SQ_CF_WORD1_SQ_CF_INST_RETURN);
	return 0;
}

static int emit_jump_to_offset(struct r600_shader_ctx *ctx, int pops, int offset)
{

	r600_bc_add_cfinst(ctx->bc, V_SQ_CF_WORD1_SQ_CF_INST_JUMP);
	ctx->bc->cf_last->pop_count = pops;
	/* TODO work out offset */
	return 0;
}

static int emit_setret_in_loop_flag(struct r600_shader_ctx *ctx, unsigned flag_value)
{
	return 0;
}

static void emit_testflag(struct r600_shader_ctx *ctx)
{

}

static void emit_return_on_flag(struct r600_shader_ctx *ctx, unsigned ifidx)
{
	emit_testflag(ctx);
	emit_jump_to_offset(ctx, 1, 4);
	emit_setret_in_loop_flag(ctx, V_SQ_ALU_SRC_0);
	pops(ctx, ifidx + 1);
	emit_return(ctx);
}

static void break_loop_on_flag(struct r600_shader_ctx *ctx, unsigned fc_sp)
{
	emit_testflag(ctx);

	r600_bc_add_cfinst(ctx->bc, ctx->inst_info->r600_opcode);
	ctx->bc->cf_last->pop_count = 1;

	fc_set_mid(ctx, fc_sp);

	pops(ctx, 1);
}
#endif

static int tgsi_if(struct r600_shader_ctx *ctx)
{
	emit_logic_pred(ctx, CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE));

	r600_bc_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_JUMP));

	fc_pushlevel(ctx, FC_IF);

	callstack_check_depth(ctx, FC_PUSH_VPM, 0);
	return 0;
}

static int tgsi_else(struct r600_shader_ctx *ctx)
{
	r600_bc_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_ELSE));
	ctx->bc->cf_last->pop_count = 1;

	fc_set_mid(ctx, ctx->bc->fc_sp);
	ctx->bc->fc_stack[ctx->bc->fc_sp].start->cf_addr = ctx->bc->cf_last->id;
	return 0;
}

static int tgsi_endif(struct r600_shader_ctx *ctx)
{
	pops(ctx, 1);
	if (ctx->bc->fc_stack[ctx->bc->fc_sp].type != FC_IF) {
		R600_ERR("if/endif unbalanced in shader\n");
		return -1;
	}

	if (ctx->bc->fc_stack[ctx->bc->fc_sp].mid == NULL) {
		ctx->bc->fc_stack[ctx->bc->fc_sp].start->cf_addr = ctx->bc->cf_last->id + 2;
		ctx->bc->fc_stack[ctx->bc->fc_sp].start->pop_count = 1;
	} else {
		ctx->bc->fc_stack[ctx->bc->fc_sp].mid[0]->cf_addr = ctx->bc->cf_last->id + 2;
	}
	fc_poplevel(ctx);

	callstack_decrease_current(ctx, FC_PUSH_VPM);
	return 0;
}

static int tgsi_bgnloop(struct r600_shader_ctx *ctx)
{
	r600_bc_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL));

	fc_pushlevel(ctx, FC_LOOP);

	/* check stack depth */
	callstack_check_depth(ctx, FC_LOOP, 0);
	return 0;
}

static int tgsi_endloop(struct r600_shader_ctx *ctx)
{
	int i;

	r600_bc_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END));

	if (ctx->bc->fc_stack[ctx->bc->fc_sp].type != FC_LOOP) {
		R600_ERR("loop/endloop in shader code are not paired.\n");
		return -EINVAL;
	}

	/* fixup loop pointers - from r600isa
	   LOOP END points to CF after LOOP START,
	   LOOP START point to CF after LOOP END
	   BRK/CONT point to LOOP END CF
	*/
	ctx->bc->cf_last->cf_addr = ctx->bc->fc_stack[ctx->bc->fc_sp].start->id + 2;

	ctx->bc->fc_stack[ctx->bc->fc_sp].start->cf_addr = ctx->bc->cf_last->id + 2;

	for (i = 0; i < ctx->bc->fc_stack[ctx->bc->fc_sp].num_mid; i++) {
		ctx->bc->fc_stack[ctx->bc->fc_sp].mid[i]->cf_addr = ctx->bc->cf_last->id;
	}
	/* TODO add LOOPRET support */
	fc_poplevel(ctx);
	callstack_decrease_current(ctx, FC_LOOP);
	return 0;
}

static int tgsi_loop_brk_cont(struct r600_shader_ctx *ctx)
{
	unsigned int fscp;

	for (fscp = ctx->bc->fc_sp; fscp > 0; fscp--)
	{
		if (FC_LOOP == ctx->bc->fc_stack[fscp].type)
			break;
	}

	if (fscp == 0) {
		R600_ERR("Break not inside loop/endloop pair\n");
		return -EINVAL;
	}

	r600_bc_add_cfinst(ctx->bc, ctx->inst_info->r600_opcode);
	ctx->bc->cf_last->pop_count = 1;

	fc_set_mid(ctx, fscp);

	pops(ctx, 1);
	callstack_check_depth(ctx, FC_PUSH_VPM, 1);
	return 0;
}

static struct r600_shader_tgsi_instruction r600_shader_tgsi_instruction[] = {
	{TGSI_OPCODE_ARL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_r600_arl},
	{TGSI_OPCODE_MOV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_LIT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lit},

	/* FIXME:
	 * For state trackers other than OpenGL, we'll want to use
	 * _RECIP_IEEE instead.
	 */
	{TGSI_OPCODE_RCP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED, tgsi_trans_srcx_replicate},

	{TGSI_OPCODE_RSQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_rsq},
	{TGSI_OPCODE_EXP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_exp},
	{TGSI_OPCODE_LOG,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_log},
	{TGSI_OPCODE_MUL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL, tgsi_op2},
	{TGSI_OPCODE_ADD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_DP3,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DP4,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DST,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_opdst},
	{TGSI_OPCODE_MIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN, tgsi_op2},
	{TGSI_OPCODE_MAX,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX, tgsi_op2},
	{TGSI_OPCODE_SLT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2_swap},
	{TGSI_OPCODE_SGE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2},
	{TGSI_OPCODE_MAD,	1, V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD, tgsi_op3},
	{TGSI_OPCODE_SUB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_LRP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lrp},
	{TGSI_OPCODE_CND,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{20,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2A,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{22,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{23,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FRC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT, tgsi_op2},
	{TGSI_OPCODE_CLAMP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FLR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR, tgsi_op2},
	{TGSI_OPCODE_ROUND,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_EX2,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_LG2,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_POW,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_pow},
	{TGSI_OPCODE_XPD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_xpd},
	/* gap */
	{32,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ABS,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_RCC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DPH,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_COS,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS, tgsi_trig},
	{TGSI_OPCODE_DDX,	0, SQ_TEX_INST_GET_GRADIENTS_H, tgsi_tex},
	{TGSI_OPCODE_DDY,	0, SQ_TEX_INST_GET_GRADIENTS_V, tgsi_tex},
	{TGSI_OPCODE_KILP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* predicated kill */
	{TGSI_OPCODE_PK2H,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK2US,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4B,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4UB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RFL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SEQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE, tgsi_op2},
	{TGSI_OPCODE_SFL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SGT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2},
	{TGSI_OPCODE_SIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN, tgsi_trig},
	{TGSI_OPCODE_SLE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2_swap},
	{TGSI_OPCODE_SNE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE, tgsi_op2},
	{TGSI_OPCODE_STR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TEX,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_TXD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXP,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_UP2H,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP2US,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4B,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4UB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_X2D,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_r600_arl},
	{TGSI_OPCODE_BRA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CAL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RET,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SSG,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_ssg},
	{TGSI_OPCODE_CMP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_cmp},
	{TGSI_OPCODE_SCS,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_scs},
	{TGSI_OPCODE_TXB,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_NRM,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DIV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_TXL,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_BRK,	0, V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK, tgsi_loop_brk_cont},
	{TGSI_OPCODE_IF,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_if},
	/* gap */
	{75,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{76,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ELSE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_else},
	{TGSI_OPCODE_ENDIF,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endif},
	/* gap */
	{79,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{80,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PUSHA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_POPA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CEIL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_I2F,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TRUNC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_SHL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{88,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_AND,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_OR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_MOD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_XOR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SAD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXF,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CONT,	0, V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE, tgsi_loop_brk_cont},
	{TGSI_OPCODE_EMIT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDPRIM,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BGNLOOP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_bgnloop},
	{TGSI_OPCODE_BGNSUB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDLOOP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endloop},
	{TGSI_OPCODE_ENDSUB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{103,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{104,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{105,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{106,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{108,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{109,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{110,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{111,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NRM4,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CALLNZ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IFC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BREAKC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_KIL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* conditional kill */
	{TGSI_OPCODE_END,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_end},  /* aka HALT */
	/* gap */
	{118,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2I,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IDIV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IMAX,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IMIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_INEG,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ISGE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ISHR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ISLT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2U,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_U2F,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UADD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UDIV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMAD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMAX,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMOD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMUL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USEQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USGE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USHR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USLT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USNE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SWITCH,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CASE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DEFAULT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDSWITCH,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_LAST,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
};

static struct r600_shader_tgsi_instruction eg_shader_tgsi_instruction[] = {
	{TGSI_OPCODE_ARL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_eg_arl},
	{TGSI_OPCODE_MOV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_LIT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lit},
	{TGSI_OPCODE_RCP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_RSQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_EXP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_exp},
	{TGSI_OPCODE_LOG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_MUL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL, tgsi_op2},
	{TGSI_OPCODE_ADD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_DP3,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DP4,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DST,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_opdst},
	{TGSI_OPCODE_MIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN, tgsi_op2},
	{TGSI_OPCODE_MAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX, tgsi_op2},
	{TGSI_OPCODE_SLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2_swap},
	{TGSI_OPCODE_SGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2},
	{TGSI_OPCODE_MAD,	1, EG_V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD, tgsi_op3},
	{TGSI_OPCODE_SUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_LRP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lrp},
	{TGSI_OPCODE_CND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{20,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2A,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{22,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{23,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FRC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT, tgsi_op2},
	{TGSI_OPCODE_CLAMP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FLR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR, tgsi_op2},
	{TGSI_OPCODE_ROUND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_EX2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_LG2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_POW,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_pow},
	{TGSI_OPCODE_XPD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_xpd},
	/* gap */
	{32,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ABS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_RCC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DPH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_COS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS, tgsi_trig},
	{TGSI_OPCODE_DDX,	0, SQ_TEX_INST_GET_GRADIENTS_H, tgsi_tex},
	{TGSI_OPCODE_DDY,	0, SQ_TEX_INST_GET_GRADIENTS_V, tgsi_tex},
	{TGSI_OPCODE_KILP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* predicated kill */
	{TGSI_OPCODE_PK2H,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK2US,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4B,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4UB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RFL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SEQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE, tgsi_op2},
	{TGSI_OPCODE_SFL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SGT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2},
	{TGSI_OPCODE_SIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN, tgsi_trig},
	{TGSI_OPCODE_SLE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2_swap},
	{TGSI_OPCODE_SNE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE, tgsi_op2},
	{TGSI_OPCODE_STR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TEX,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_TXD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXP,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_UP2H,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP2US,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4B,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4UB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_X2D,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_eg_arl},
	{TGSI_OPCODE_BRA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CAL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RET,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SSG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_ssg},
	{TGSI_OPCODE_CMP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_cmp},
	{TGSI_OPCODE_SCS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_scs},
	{TGSI_OPCODE_TXB,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_NRM,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_TXL,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_BRK,	0, EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK, tgsi_loop_brk_cont},
	{TGSI_OPCODE_IF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_if},
	/* gap */
	{75,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{76,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ELSE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_else},
	{TGSI_OPCODE_ENDIF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endif},
	/* gap */
	{79,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{80,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PUSHA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_POPA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CEIL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_I2F,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TRUNC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_SHL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{88,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_AND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_OR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_MOD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_XOR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SAD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CONT,	0, EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE, tgsi_loop_brk_cont},
	{TGSI_OPCODE_EMIT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDPRIM,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BGNLOOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_bgnloop},
	{TGSI_OPCODE_BGNSUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDLOOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endloop},
	{TGSI_OPCODE_ENDSUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{103,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{104,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{105,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{106,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{108,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{109,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{110,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{111,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NRM4,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CALLNZ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IFC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BREAKC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_KIL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* conditional kill */
	{TGSI_OPCODE_END,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_end},  /* aka HALT */
	/* gap */
	{118,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2I,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IDIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IMAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IMIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_INEG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ISGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ISHR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ISLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2U,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_U2F,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UADD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UDIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMAD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMOD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UMUL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USEQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USHR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_USNE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SWITCH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CASE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DEFAULT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDSWITCH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_LAST,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
};
