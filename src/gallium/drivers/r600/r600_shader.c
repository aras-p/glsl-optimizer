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
 *
 * Authors:
 *      Jerome Glisse
 */
#include <stdio.h>
#include <errno.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include <tgsi/tgsi_dump.h>
#include "r600_screen.h"
#include "r600_texture.h"
#include "r600_context.h"
#include "r600d.h"

static int r600_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *rpshader)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_shader *rshader = &rpshader->shader;
	struct radeon_state *state;
	unsigned i, tmp;

	rpshader->state = radeon_state_decref(rpshader->state);
	state = radeon_state(rscreen->rw, R600_VS_SHADER_TYPE, R600_VS_SHADER);
	if (state == NULL)
		return -ENOMEM;
	for (i = 0; i < rshader->noutput; i += 4) {
		tmp = rshader->output[i].sid;
		tmp |= rshader->output[i + 1].sid << 8;
		tmp |= rshader->output[i + 2].sid << 16;
		tmp |= rshader->output[i + 3].sid << 24;
		state->states[R600_VS_SHADER__SPI_VS_OUT_ID_0 + i / 4] = tmp;
	}
	state->states[R600_VS_SHADER__SPI_VS_OUT_CONFIG] = S_0286C4_VS_EXPORT_COUNT(rshader->noutput - 1);
	state->states[R600_VS_SHADER__SQ_PGM_RESOURCES_VS] = S_028868_NUM_GPRS(rshader->ngpr);
	rpshader->state = state;
	rpshader->state->bo[0] = radeon_bo_incref(rscreen->rw, rpshader->bo);
	rpshader->state->bo[1] = radeon_bo_incref(rscreen->rw, rpshader->bo);
	rpshader->state->nbo = 2;
	rpshader->state->placement[0] = RADEON_GEM_DOMAIN_GTT;
	return radeon_state_pm4(state);
}

static int r600_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *rpshader)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_shader *rshader = &rpshader->shader;
	struct radeon_state *state;
	unsigned i, tmp;

	rpshader->state = radeon_state_decref(rpshader->state);
	state = radeon_state(rscreen->rw, R600_PS_SHADER_TYPE, R600_PS_SHADER);
	if (state == NULL)
		return -ENOMEM;
	for (i = 0; i < rshader->ninput; i++) {
		tmp = S_028644_SEMANTIC(rshader->input[i].sid);
		tmp |= S_028644_SEL_CENTROID(1);
		tmp |= S_028644_FLAT_SHADE(rshader->flat_shade);
		state->states[R600_PS_SHADER__SPI_PS_INPUT_CNTL_0 + i] = tmp;
	}
	state->states[R600_PS_SHADER__SPI_PS_IN_CONTROL_0] = S_0286CC_NUM_INTERP(rshader->ninput) |
							S_0286CC_PERSP_GRADIENT_ENA(1);
	state->states[R600_PS_SHADER__SPI_PS_IN_CONTROL_1] = 0x00000000;
	state->states[R600_PS_SHADER__SQ_PGM_RESOURCES_PS] = S_028868_NUM_GPRS(rshader->ngpr);
	state->states[R600_PS_SHADER__SQ_PGM_EXPORTS_PS] = 0x00000002;
	rpshader->state = state;
	rpshader->state->bo[0] = radeon_bo_incref(rscreen->rw, rpshader->bo);
	rpshader->state->nbo = 1;
	rpshader->state->placement[0] = RADEON_GEM_DOMAIN_GTT;
	return radeon_state_pm4(state);
}

static int r600_pipe_shader(struct pipe_context *ctx, struct r600_pipe_shader *rpshader)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_shader *rshader = &rpshader->shader;
	int r;

	/* copy new shader */
	radeon_bo_decref(rscreen->rw, rpshader->bo);
	rpshader->bo = NULL;
	rpshader->bo = radeon_bo(rscreen->rw, 0, rshader->ndw * 4,
				4096, NULL);
	if (rpshader->bo == NULL) {
		return -ENOMEM;
	}
	radeon_bo_map(rscreen->rw, rpshader->bo);
	memcpy(rpshader->bo->data, rshader->bcode, rshader->ndw * 4);
	radeon_bo_unmap(rscreen->rw, rpshader->bo);
	/* build state */
	rshader->flat_shade = rctx->flat_shade;
	switch (rpshader->type) {
	case C_PROGRAM_TYPE_VS:
		r = r600_pipe_shader_vs(ctx, rpshader);
		break;
	case C_PROGRAM_TYPE_FS:
		r = r600_pipe_shader_ps(ctx, rpshader);
		break;
	default:
		r = -EINVAL;
		break;
	}
	return r;
}

struct r600_pipe_shader *r600_pipe_shader_create(struct pipe_context *ctx, unsigned type, const struct tgsi_token *tokens)
{
	struct r600_pipe_shader *rpshader = CALLOC_STRUCT(r600_pipe_shader);
	struct r600_shader *rshader = &rpshader->shader;
	int r;

	if (rpshader == NULL)
		return NULL;
	rpshader->type = type;
	c_list_init(&rshader->nodes);
	fprintf(stderr, "<<\n");
	tgsi_dump(tokens, 0);
	fprintf(stderr, "--------------------------------------------------------------\n");
	r = c_shader_from_tgsi(&rshader->cshader, type, tokens);
	if (r) {
		r600_pipe_shader_destroy(ctx, rpshader);
		fprintf(stderr, "ERROR(%s %d)>>\n\n", __func__, __LINE__);
		return NULL;
	}
	r = r600_shader_insert_fetch(&rshader->cshader);
	if (r) {
		r600_pipe_shader_destroy(ctx, rpshader);
		fprintf(stderr, "ERROR(%s %d)>>\n\n", __func__, __LINE__);
		return NULL;
	}
	r = c_shader_build_dominator_tree(&rshader->cshader);
	if (r) {
		r600_pipe_shader_destroy(ctx, rpshader);
		fprintf(stderr, "ERROR(%s %d)>>\n\n", __func__, __LINE__);
		return NULL;
	}
	c_shader_dump(&rshader->cshader);
	r = r700_shader_translate(rshader);
	if (r) {
		r600_pipe_shader_destroy(ctx, rpshader);
		fprintf(stderr, "ERROR(%s %d)>>\n\n", __func__, __LINE__);
		return NULL;
	}
#if 1
#if 0
	fprintf(stderr, "--------------------------------------------------------------\n");
	for (int i = 0; i < rshader->ndw; i++) {
		fprintf(stderr, "0x%08X\n", rshader->bcode[i]);
	}
#endif
	fprintf(stderr, ">>\n\n");
#endif
	return rpshader;
}

void r600_pipe_shader_destroy(struct pipe_context *ctx, struct r600_pipe_shader *rpshader)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;

	if (rpshader == NULL)
		return;
	radeon_bo_decref(rscreen->rw, rpshader->bo);
	rpshader->bo = NULL;
	r600_shader_cleanup(&rpshader->shader);
	FREE(rpshader);
}

int r600_pipe_shader_update(struct pipe_context *ctx, struct r600_pipe_shader *rpshader)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_shader *rshader;
	enum pipe_format resource_format[160];
	unsigned i, nresources = 0;
	int r;

	if (rpshader == NULL)
		return -EINVAL;
	rshader = &rpshader->shader;
	switch (rpshader->type) {
	case C_PROGRAM_TYPE_VS:
		for (i = 0; i < rctx->nvertex_element; i++) {
			resource_format[nresources++] = rctx->vertex_element[i].src_format;
		}
		break;
	default:
		break;
	}
	/* there should be enough input */
	if (nresources < rshader->nresource)
		return -EINVAL;
	/* FIXME compare resources */
	r = r600_shader_update(rshader, resource_format);
	if (r)
		return r;
	return r600_pipe_shader(ctx, rpshader);
}
