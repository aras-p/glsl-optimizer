/*
 * Copyright (C) 2009 Maciej Cencora <m.cencora@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * \file
 *
 * Fragment program compiler. Perform transformations on the intermediate
 * representation until the program is in a form where we can translate
 * it more or less directly into machine-readable form.
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 * \author Jerome Glisse <j.glisse@gmail.com>
 */

#include "r300_fragprog_common.h"

#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "r300_state.h"

#include "compiler/radeon_program.h"
#include "compiler/radeon_program_alu.h"
#include "compiler/r300_fragprog_swizzle.h"
#include "compiler/r300_fragprog.h"
#include "compiler/r500_fragprog.h"


static GLuint build_dtm(GLuint depthmode)
{
	switch(depthmode) {
	default:
	case GL_LUMINANCE: return 0;
	case GL_INTENSITY: return 1;
	case GL_ALPHA: return 2;
	}
}

static GLuint build_func(GLuint comparefunc)
{
	return comparefunc - GL_NEVER;
}

/**
 * Collect all external state that is relevant for compiling the given
 * fragment program.
 */
static void build_state(
	r300ContextPtr r300,
	struct gl_fragment_program *fp,
	struct r300_fragment_program_external_state *state)
{
	int unit;

	_mesa_bzero(state, sizeof(*state));

	for(unit = 0; unit < 16; ++unit) {
		if (fp->Base.ShadowSamplers & (1 << unit)) {
			struct gl_texture_object* tex = r300->radeon.glCtx->Texture.Unit[unit]._Current;

			state->unit[unit].depth_texture_mode = build_dtm(tex->DepthMode);
			state->unit[unit].texture_compare_func = build_func(tex->CompareFunc);
		}
	}
}


void r300TranslateFragmentShader(GLcontext *ctx, struct r300_fragment_program *fp)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_fragment_program_compiler compiler;

	compiler.code = &fp->code;
	compiler.state = fp->state;
	compiler.program = fp->Base;
	compiler.is_r500 = (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) ? GL_TRUE : GL_FALSE;
	compiler.debug = (RADEON_DEBUG & DEBUG_PIXEL) ? GL_TRUE : GL_FALSE;

	if (!r3xx_compile_fragment_program(&compiler))
		fp->error = GL_TRUE;

	fp->translated = GL_TRUE;
}

struct r300_fragment_program *r300SelectFragmentShader(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_fragment_program_cont *fp_list;
	struct r300_fragment_program *fp;
	struct r300_fragment_program_external_state state;

	fp_list = (struct r300_fragment_program_cont *)ctx->FragmentProgram._Current;
	build_state(r300, ctx->FragmentProgram._Current, &state);

	fp = fp_list->progs;
	while (fp) {
		if (_mesa_memcmp(&fp->state, &state, sizeof(state)) == 0) {
			return r300->selected_fp = fp;
		}
		fp = fp->next;
	}

	fp = _mesa_calloc(sizeof(struct r300_fragment_program));

	fp->state = state;
	fp->translated = GL_FALSE;
	fp->Base = _mesa_clone_program(ctx, &ctx->FragmentProgram._Current->Base);

	fp->next = fp_list->progs;
	fp_list->progs = fp;

	return r300->selected_fp = fp;
}
