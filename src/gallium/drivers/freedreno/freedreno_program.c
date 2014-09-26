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

#include "tgsi/tgsi_text.h"

#include "freedreno_program.h"
#include "freedreno_context.h"

static void
fd_fp_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->prog.fp = hwcso;
	ctx->prog.dirty |= FD_SHADER_DIRTY_FP;
	ctx->dirty |= FD_DIRTY_PROG;
}

static void
fd_vp_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->prog.vp = hwcso;
	ctx->prog.dirty |= FD_SHADER_DIRTY_VP;
	ctx->dirty |= FD_DIRTY_PROG;
}

static const char *solid_fp =
	"FRAG                                        \n"
	"PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1       \n"
	"DCL CONST[0]                                \n"
	"DCL OUT[0], COLOR                           \n"
	"  0: MOV OUT[0], CONST[0]                   \n"
	"  1: END                                    \n";

static const char *solid_vp =
	"VERT                                        \n"
	"DCL IN[0]                                   \n"
	"DCL OUT[0], POSITION                        \n"
	"  0: MOV OUT[0], IN[0]                      \n"
	"  1: END                                    \n";

static const char *blit_fp =
	"FRAG                                        \n"
	"PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1       \n"
	"DCL IN[0], TEXCOORD[0], PERSPECTIVE         \n"
	"DCL OUT[0], COLOR                           \n"
	"DCL SAMP[0]                                 \n"
	"  0: TEX OUT[0], IN[0], SAMP[0], 2D         \n"
	"  1: END                                    \n";

static const char *blit_vp =
	"VERT                                        \n"
	"DCL IN[0]                                   \n"
	"DCL IN[1]                                   \n"
	"DCL OUT[0], TEXCOORD[0]                     \n"
	"DCL OUT[1], POSITION                        \n"
	"  0: MOV OUT[0], IN[0]                      \n"
	"  0: MOV OUT[1], IN[1]                      \n"
	"  1: END                                    \n";

static void * assemble_tgsi(struct pipe_context *pctx,
		const char *src, bool frag)
{
	struct tgsi_token toks[32];
	struct pipe_shader_state cso = {
			.tokens = toks,
	};

	tgsi_text_translate(src, toks, ARRAY_SIZE(toks));

	if (frag)
		return pctx->create_fs_state(pctx, &cso);
	else
		return pctx->create_vs_state(pctx, &cso);
}

void fd_prog_init(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	pctx->bind_fs_state = fd_fp_state_bind;
	pctx->bind_vs_state = fd_vp_state_bind;

	// XXX for now, let a2xx keep it's own hand-rolled shaders
	// for solid and blit progs:
	if (ctx->screen->gpu_id < 300)
		return;

	ctx->solid_prog.fp = assemble_tgsi(pctx, solid_fp, true);
	ctx->solid_prog.vp = assemble_tgsi(pctx, solid_vp, false);
	ctx->blit_prog.fp = assemble_tgsi(pctx, blit_fp, true);
	ctx->blit_prog.vp = assemble_tgsi(pctx, blit_vp, false);
}

void fd_prog_fini(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	pctx->delete_vs_state(pctx, ctx->solid_prog.vp);
	pctx->delete_fs_state(pctx, ctx->solid_prog.fp);
	pctx->delete_vs_state(pctx, ctx->blit_prog.vp);
	pctx->delete_fs_state(pctx, ctx->blit_prog.fp);
}
