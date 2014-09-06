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

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "freedreno_context.h"
#include "freedreno_lowering.h"
#include "freedreno_util.h"

#include "ir3_shader.h"
#include "ir3_compiler.h"


static void
delete_variant(struct ir3_shader_variant *v)
{
	if (v->ir)
		ir3_destroy(v->ir);
	fd_bo_del(v->bo);
	free(v);
}

static void
assemble_variant(struct ir3_shader_variant *v)
{
	struct fd_context *ctx = fd_context(v->shader->pctx);
	uint32_t sz, *bin;

	bin = ir3_assemble(v->ir, &v->info);
	sz = v->info.sizedwords * 4;

	v->bo = fd_bo_new(ctx->dev, sz,
			DRM_FREEDRENO_GEM_CACHE_WCOMBINE |
			DRM_FREEDRENO_GEM_TYPE_KMEM);

	memcpy(fd_bo_map(v->bo), bin, sz);

	free(bin);

	v->instrlen = v->info.sizedwords / 8;

	/* NOTE: if relative addressing is used, we set constlen in
	 * the compiler (to worst-case value) since we don't know in
	 * the assembler what the max addr reg value can be:
	 */
	v->constlen = MAX2(v->constlen, v->info.max_const + 1);

	/* no need to keep the ir around beyond this point: */
	ir3_destroy(v->ir);
	v->ir = NULL;
}

/* for vertex shader, the inputs are loaded into registers before the shader
 * is executed, so max_regs from the shader instructions might not properly
 * reflect the # of registers actually used:
 */
static void
fixup_vp_regfootprint(struct ir3_shader_variant *v)
{
	unsigned i;
	for (i = 0; i < v->inputs_count; i++) {
		if (v->inputs[i].compmask) {
			uint32_t regid = (v->inputs[i].regid + 3) >> 2;
			v->info.max_reg = MAX2(v->info.max_reg, regid);
		}
	}
	for (i = 0; i < v->outputs_count; i++) {
		uint32_t regid = (v->outputs[i].regid + 3) >> 2;
		v->info.max_reg = MAX2(v->info.max_reg, regid);
	}
}

/* reset before attempting to compile again.. */
static void reset_variant(struct ir3_shader_variant *v, const char *msg)
{
	debug_error(msg);
	v->inputs_count = 0;
	v->outputs_count = 0;
	v->total_in = 0;
	v->has_samp = false;
	v->immediates_count = 0;
}

static struct ir3_shader_variant *
create_variant(struct ir3_shader *shader, struct ir3_shader_key key)
{
	struct ir3_shader_variant *v = CALLOC_STRUCT(ir3_shader_variant);
	const struct tgsi_token *tokens = shader->tokens;
	int ret;

	if (!v)
		return NULL;

	v->shader = shader;
	v->key = key;
	v->type = shader->type;

	if (fd_mesa_debug & FD_DBG_DISASM) {
		DBG("dump tgsi: type=%d, k={bp=%u,cts=%u,hp=%u}", shader->type,
			key.binning_pass, key.color_two_side, key.half_precision);
		tgsi_dump(tokens, 0);
	}

	if (!(fd_mesa_debug & FD_DBG_NOOPT)) {
		ret = ir3_compile_shader(v, tokens, key, true);
		if (ret) {
			reset_variant(v, "new compiler failed, trying without copy propagation!");
			ret = ir3_compile_shader(v, tokens, key, false);
			if (ret)
				reset_variant(v, "new compiler failed, trying fallback!");
		}
	} else {
		ret = -1;  /* force fallback to old compiler */
	}

	if (ret)
		ret = ir3_compile_shader_old(v, tokens, key);

	if (ret) {
		debug_error("compile failed!");
		goto fail;
	}

	assemble_variant(v);
	if (!v->bo) {
		debug_error("assemble failed!");
		goto fail;
	}

	if (shader->type == SHADER_VERTEX)
		fixup_vp_regfootprint(v);

	if (fd_mesa_debug & FD_DBG_DISASM) {
		DBG("disassemble: type=%d, k={bp=%u,cts=%u,hp=%u}", v->type,
			key.binning_pass, key.color_two_side, key.half_precision);
		disasm_a3xx(fd_bo_map(v->bo), v->info.sizedwords, 0, v->type);
	}

	return v;

fail:
	delete_variant(v);
	return NULL;
}

struct ir3_shader_variant *
ir3_shader_variant(struct ir3_shader *shader, struct ir3_shader_key key)
{
	struct ir3_shader_variant *v;

	/* some shader key values only apply to vertex or frag shader,
	 * so normalize the key to avoid constructing multiple identical
	 * variants:
	 */
	if (shader->type == SHADER_FRAGMENT) {
		key.binning_pass = false;
	}
	if (shader->type == SHADER_VERTEX) {
		key.color_two_side = false;
		key.half_precision = false;
	}

	for (v = shader->variants; v; v = v->next)
		if (!memcmp(&key, &v->key, sizeof(key)))
			return v;

	/* compile new variant if it doesn't exist already: */
	v = create_variant(shader, key);
	v->next = shader->variants;
	shader->variants = v;

	return v;
}


void
ir3_shader_destroy(struct ir3_shader *shader)
{
	struct ir3_shader_variant *v, *t;
	for (v = shader->variants; v; ) {
		t = v;
		v = v->next;
		delete_variant(t);
	}
	free((void *)shader->tokens);
	free(shader);
}

struct ir3_shader *
ir3_shader_create(struct pipe_context *pctx, const struct tgsi_token *tokens,
		enum shader_t type)
{
	struct ir3_shader *shader = CALLOC_STRUCT(ir3_shader);
	shader->pctx = pctx;
	shader->type = type;
	shader->tokens = tgsi_dup_tokens(tokens);
	return shader;
}
