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
#include "util/u_format.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "freedreno_program.h"
#include "freedreno_compiler.h"
#include "freedreno_vbo.h"
#include "freedreno_texture.h"
#include "freedreno_util.h"

static struct fd_shader_stateobj *
create_shader(enum shader_t type)
{
	struct fd_shader_stateobj *so = CALLOC_STRUCT(fd_shader_stateobj);
	if (!so)
		return NULL;
	so->type = type;
	return so;
}

static void
delete_shader(struct fd_shader_stateobj *so)
{
	ir_shader_destroy(so->ir);
	FREE(so->tokens);
	FREE(so);
}

static struct fd_shader_stateobj *
assemble(struct fd_shader_stateobj *so)
{
	free(so->bin);
	so->bin = ir_shader_assemble(so->ir, &so->info);
	if (!so->bin)
		goto fail;

	if (fd_mesa_debug & FD_DBG_DISASM) {
		DBG("disassemble: type=%d", so->type);
		disasm(so->bin, so->info.sizedwords, 0, so->type);
	}

	return so;

fail:
	debug_error("assemble failed!");
	delete_shader(so);
	return NULL;
}

static struct fd_shader_stateobj *
compile(struct fd_program_stateobj *prog, struct fd_shader_stateobj *so)
{
	int ret;

	if (fd_mesa_debug & FD_DBG_DISASM) {
		DBG("dump tgsi: type=%d", so->type);
		tgsi_dump(so->tokens, 0);
	}

	ret = fd_compile_shader(prog, so);
	if (ret)
		goto fail;

	/* NOTE: we don't assemble yet because for VS we don't know the
	 * type information for vertex fetch yet.. so those need to be
	 * patched up later before assembling.
	 */

	so->info.sizedwords = 0;

	return so;

fail:
	debug_error("compile failed!");
	delete_shader(so);
	return NULL;
}

static void
emit(struct fd_ringbuffer *ring, struct fd_shader_stateobj *so)
{
	unsigned i;

	if (so->info.sizedwords == 0)
		assemble(so);

	OUT_PKT3(ring, CP_IM_LOAD_IMMEDIATE, 2 + so->info.sizedwords);
	OUT_RING(ring, (so->type == SHADER_VERTEX) ? 0 : 1);
	OUT_RING(ring, so->info.sizedwords);
	for (i = 0; i < so->info.sizedwords; i++)
		OUT_RING(ring, so->bin[i]);
}

static void *
fd_fp_state_create(struct pipe_context *pctx,
		const struct pipe_shader_state *cso)
{
	struct fd_shader_stateobj *so = create_shader(SHADER_FRAGMENT);
	if (!so)
		return NULL;
	so->tokens = tgsi_dup_tokens(cso->tokens);
	return so;
}

static void
fd_fp_state_delete(struct pipe_context *pctx, void *hwcso)
{
	struct fd_shader_stateobj *so = hwcso;
	delete_shader(so);
}

static void
fd_fp_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->prog.fp = hwcso;
	ctx->prog.dirty |= FD_SHADER_DIRTY_FP;
	ctx->dirty |= FD_DIRTY_PROG;
}

static void *
fd_vp_state_create(struct pipe_context *pctx,
		const struct pipe_shader_state *cso)
{
	struct fd_shader_stateobj *so = create_shader(SHADER_VERTEX);
	if (!so)
		return NULL;
	so->tokens = tgsi_dup_tokens(cso->tokens);
	return so;
}

static void
fd_vp_state_delete(struct pipe_context *pctx, void *hwcso)
{
	struct fd_shader_stateobj *so = hwcso;
	delete_shader(so);
}

static void
fd_vp_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->prog.vp = hwcso;
	ctx->prog.dirty |= FD_SHADER_DIRTY_VP;
	ctx->dirty |= FD_DIRTY_PROG;
}

static void
patch_vtx_fetches(struct fd_context *ctx, struct fd_shader_stateobj *so,
		struct fd_vertex_stateobj *vtx)
{
	unsigned i;

	assert(so->num_vfetch_instrs == vtx->num_elements);

	/* update vtx fetch instructions: */
	for (i = 0; i < so->num_vfetch_instrs; i++) {
		struct ir_instruction *instr = so->vfetch_instrs[i];
		struct pipe_vertex_element *elem = &vtx->pipe[i];
		struct pipe_vertex_buffer *vb =
				&ctx->vertexbuf.vb[elem->vertex_buffer_index];
		enum pipe_format format = elem->src_format;
		const struct util_format_description *desc =
				util_format_description(format);
		unsigned j;

		/* Find the first non-VOID channel. */
		for (j = 0; j < 4; j++)
			if (desc->channel[j].type != UTIL_FORMAT_TYPE_VOID)
				break;

		/* CI/CIS can probably be set in compiler instead: */
		instr->fetch.const_idx = 20 + (i / 3);
		instr->fetch.const_idx_sel = i % 3;

		instr->fetch.fmt = fd_pipe2surface(format);
		instr->fetch.is_normalized = desc->channel[j].normalized;
		instr->fetch.is_signed =
				desc->channel[j].type == UTIL_FORMAT_TYPE_SIGNED;
		instr->fetch.stride = vb->stride ? : 1;
		instr->fetch.offset = elem->src_offset;

		for (j = 0; j < 4; j++)
			instr->regs[0]->swizzle[j] = "xyzw01__"[desc->swizzle[j]];

		assert(instr->fetch.fmt != FMT_INVALID);

		DBG("vtx[%d]: %s (%d), ci=%d, cis=%d, id=%d, swizzle=%s, "
				"stride=%d, offset=%d",
				i, util_format_name(format),
				instr->fetch.fmt,
				instr->fetch.const_idx,
				instr->fetch.const_idx_sel,
				elem->instance_divisor,
				instr->regs[0]->swizzle,
				instr->fetch.stride,
				instr->fetch.offset);
	}

	/* trigger re-assemble: */
	so->info.sizedwords = 0;
}

static void
patch_tex_fetches(struct fd_context *ctx, struct fd_shader_stateobj *so,
		struct fd_texture_stateobj *tex)
{
	unsigned i;

	/* update tex fetch instructions: */
	for (i = 0; i < so->num_tfetch_instrs; i++) {
		struct ir_instruction *instr = so->tfetch_instrs[i].instr;
		unsigned samp_id = so->tfetch_instrs[i].samp_id;
		unsigned const_idx = fd_get_const_idx(ctx, tex, samp_id);

		if (const_idx != instr->fetch.const_idx) {
			instr->fetch.const_idx = const_idx;
			/* trigger re-assemble: */
			so->info.sizedwords = 0;
		}
	}
}

void
fd_program_validate(struct fd_context *ctx)
{
	struct fd_program_stateobj *prog = &ctx->prog;

	/* if vertex or frag shader is dirty, we may need to recompile. Compile
	 * frag shader first, as that assigns the register slots for exports
	 * from the vertex shader.  And therefore if frag shader has changed we
	 * need to recompile both vert and frag shader.
	 */
	if (prog->dirty & FD_SHADER_DIRTY_FP)
		compile(prog, prog->fp);

	if (prog->dirty & (FD_SHADER_DIRTY_FP | FD_SHADER_DIRTY_VP))
		compile(prog, prog->vp);

	if (prog->dirty)
		ctx->dirty |= FD_DIRTY_PROG;

	prog->dirty = 0;

	/* if necessary, fix up vertex fetch instructions: */
	if (ctx->dirty & (FD_DIRTY_VTX | FD_DIRTY_VERTEXBUF | FD_DIRTY_PROG))
		patch_vtx_fetches(ctx, prog->vp, ctx->vtx);

	/* if necessary, fix up texture fetch instructions: */
	if (ctx->dirty & (FD_DIRTY_VERTTEX | FD_DIRTY_FRAGTEX | FD_DIRTY_PROG)) {
		patch_tex_fetches(ctx, prog->vp, &ctx->verttex);
		patch_tex_fetches(ctx, prog->fp, &ctx->fragtex);
	}
}

void
fd_program_emit(struct fd_ringbuffer *ring,
		struct fd_program_stateobj *prog)
{
	struct ir_shader_info *vsi = &prog->vp->info;
	struct ir_shader_info *fsi = &prog->fp->info;
	uint8_t vs_gprs, fs_gprs, vs_export;

	emit(ring, prog->vp);
	emit(ring, prog->fp);

	vs_gprs = (vsi->max_reg < 0) ? 0x80 : vsi->max_reg;
	fs_gprs = (fsi->max_reg < 0) ? 0x80 : fsi->max_reg;
	vs_export = max(1, prog->num_exports) - 1;

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_SQ_PROGRAM_CNTL));
	OUT_RING(ring, A2XX_SQ_PROGRAM_CNTL_PS_EXPORT_MODE(POSITION_2_VECTORS_SPRITE) |
			A2XX_SQ_PROGRAM_CNTL_VS_RESOURCE |
			A2XX_SQ_PROGRAM_CNTL_PS_RESOURCE |
			A2XX_SQ_PROGRAM_CNTL_VS_EXPORT_COUNT(vs_export) |
			A2XX_SQ_PROGRAM_CNTL_PS_REGS(fs_gprs) |
			A2XX_SQ_PROGRAM_CNTL_VS_REGS(vs_gprs));
}

/* Creates shader:
 *    EXEC ADDR(0x2) CNT(0x1)
 *       (S)FETCH:	SAMPLE	R0.xyzw = R0.xyx CONST(0) LOCATION(CENTER)
 *    ALLOC PARAM/PIXEL SIZE(0x0)
 *    EXEC_END ADDR(0x3) CNT(0x1)
 *          ALU:	MAXv	export0 = R0, R0	; gl_FragColor
 *    NOP
 */
static struct fd_shader_stateobj *
create_blit_fp(void)
{
	struct fd_shader_stateobj *so = create_shader(SHADER_FRAGMENT);
	struct ir_cf *cf;
	struct ir_instruction *instr;

	if (!so)
		return NULL;

	so->ir = ir_shader_create();

	cf = ir_cf_create(so->ir, EXEC);

	instr = ir_instr_create_tex_fetch(cf, 0);
	ir_reg_create(instr, 0, "xyzw", 0);
	ir_reg_create(instr, 0, "xyx", 0);
	instr->sync = true;

	cf = ir_cf_create_alloc(so->ir, SQ_PARAMETER_PIXEL, 0);
	cf = ir_cf_create(so->ir, EXEC_END);

	instr = ir_instr_create_alu(cf, MAXv, ~0);
	ir_reg_create(instr, 0, NULL, IR_REG_EXPORT);
	ir_reg_create(instr, 0, NULL, 0);
	ir_reg_create(instr, 0, NULL, 0);

	return assemble(so);
}

/* Creates shader:
*     EXEC ADDR(0x3) CNT(0x2)
*           FETCH:	VERTEX	R1.xy01 = R0.x FMT_32_32_FLOAT UNSIGNED STRIDE(8) CONST(26, 1)
*           FETCH:	VERTEX	R2.xyz1 = R0.x FMT_32_32_32_FLOAT UNSIGNED STRIDE(12) CONST(26, 0)
*     ALLOC POSITION SIZE(0x0)
*     EXEC ADDR(0x5) CNT(0x1)
*           ALU:	MAXv	export62 = R2, R2	; gl_Position
*     ALLOC PARAM/PIXEL SIZE(0x0)
*     EXEC_END ADDR(0x6) CNT(0x1)
*           ALU:	MAXv	export0 = R1, R1
*     NOP
 */
static struct fd_shader_stateobj *
create_blit_vp(void)
{
	struct fd_shader_stateobj *so = create_shader(SHADER_VERTEX);
	struct ir_cf *cf;
	struct ir_instruction *instr;

	if (!so)
		return NULL;

	so->ir = ir_shader_create();

	cf = ir_cf_create(so->ir, EXEC);

	instr = ir_instr_create_vtx_fetch(cf, 26, 1, FMT_32_32_FLOAT, false, 8);
	instr->fetch.is_normalized = true;
	ir_reg_create(instr, 1, "xy01", 0);
	ir_reg_create(instr, 0, "x", 0);

	instr = ir_instr_create_vtx_fetch(cf, 26, 0, FMT_32_32_32_FLOAT, false, 12);
	instr->fetch.is_normalized = true;
	ir_reg_create(instr, 2, "xyz1", 0);
	ir_reg_create(instr, 0, "x", 0);

	cf = ir_cf_create_alloc(so->ir, SQ_POSITION, 0);
	cf = ir_cf_create(so->ir, EXEC);

	instr = ir_instr_create_alu(cf, MAXv, ~0);
	ir_reg_create(instr, 62, NULL, IR_REG_EXPORT);
	ir_reg_create(instr, 2, NULL, 0);
	ir_reg_create(instr, 2, NULL, 0);

	cf = ir_cf_create_alloc(so->ir, SQ_PARAMETER_PIXEL, 0);
	cf = ir_cf_create(so->ir, EXEC_END);

	instr = ir_instr_create_alu(cf, MAXv, ~0);
	ir_reg_create(instr, 0, NULL, IR_REG_EXPORT);
	ir_reg_create(instr, 1, NULL, 0);
	ir_reg_create(instr, 1, NULL, 0);


	return assemble(so);

}

/* Creates shader:
 *    ALLOC PARAM/PIXEL SIZE(0x0)
 *    EXEC_END ADDR(0x1) CNT(0x1)
 *          ALU:	MAXv	export0 = C0, C0	; gl_FragColor
 */
static struct fd_shader_stateobj *
create_solid_fp(void)
{
	struct fd_shader_stateobj *so = create_shader(SHADER_FRAGMENT);
	struct ir_cf *cf;
	struct ir_instruction *instr;

	if (!so)
		return NULL;

	so->ir = ir_shader_create();

	cf = ir_cf_create_alloc(so->ir, SQ_PARAMETER_PIXEL, 0);
	cf = ir_cf_create(so->ir, EXEC_END);

	instr = ir_instr_create_alu(cf, MAXv, ~0);
	ir_reg_create(instr, 0, NULL, IR_REG_EXPORT);
	ir_reg_create(instr, 0, NULL, IR_REG_CONST);
	ir_reg_create(instr, 0, NULL, IR_REG_CONST);

	return assemble(so);
}

/* Creates shader:
 *    EXEC ADDR(0x3) CNT(0x1)
 *       (S)FETCH:	VERTEX	R1.xyz1 = R0.x FMT_32_32_32_FLOAT
 *                           UNSIGNED STRIDE(12) CONST(26, 0)
 *    ALLOC POSITION SIZE(0x0)
 *    EXEC ADDR(0x4) CNT(0x1)
 *          ALU:	MAXv	export62 = R1, R1	; gl_Position
 *    ALLOC PARAM/PIXEL SIZE(0x0)
 *    EXEC_END ADDR(0x5) CNT(0x0)
 */
static struct fd_shader_stateobj *
create_solid_vp(void)
{
	struct fd_shader_stateobj *so = create_shader(SHADER_VERTEX);
	struct ir_cf *cf;
	struct ir_instruction *instr;

	if (!so)
		return NULL;

	so->ir = ir_shader_create();

	cf = ir_cf_create(so->ir, EXEC);

	instr = ir_instr_create_vtx_fetch(cf, 26, 0, FMT_32_32_32_FLOAT, false, 12);
	ir_reg_create(instr, 1, "xyz1", 0);
	ir_reg_create(instr, 0, "x", 0);

	cf = ir_cf_create_alloc(so->ir, SQ_POSITION, 0);
	cf = ir_cf_create(so->ir, EXEC);

	instr = ir_instr_create_alu(cf, MAXv, ~0);
	ir_reg_create(instr, 62, NULL, IR_REG_EXPORT);
	ir_reg_create(instr, 1, NULL, 0);
	ir_reg_create(instr, 1, NULL, 0);

	cf = ir_cf_create_alloc(so->ir, SQ_PARAMETER_PIXEL, 0);
	cf = ir_cf_create(so->ir, EXEC_END);

	return assemble(so);
}

void
fd_prog_init(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	pctx->create_fs_state = fd_fp_state_create;
	pctx->bind_fs_state = fd_fp_state_bind;
	pctx->delete_fs_state = fd_fp_state_delete;

	pctx->create_vs_state = fd_vp_state_create;
	pctx->bind_vs_state = fd_vp_state_bind;
	pctx->delete_vs_state = fd_vp_state_delete;

	ctx->solid_prog.fp = create_solid_fp();
	ctx->solid_prog.vp = create_solid_vp();
	ctx->blit_prog.fp = create_blit_fp();
	ctx->blit_prog.vp = create_blit_vp();
}

void
fd_prog_fini(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	delete_shader(ctx->solid_prog.vp);
	delete_shader(ctx->solid_prog.fp);
	delete_shader(ctx->blit_prog.vp);
	delete_shader(ctx->blit_prog.fp);
}
