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
#include "r300_fragprog.h"
#include "r300_fragprog_swizzle.h"
#include "r500_fragprog.h"

#include "radeon_program.h"
#include "radeon_program_alu.h"

static void update_params(GLcontext *ctx, struct gl_fragment_program *fp)
{
	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (fp->Base.Parameters)
		_mesa_load_state_parameters(ctx, fp->Base.Parameters);
}

static void nqssadce_init(struct nqssadce_state* s)
{
	s->Outputs[FRAG_RESULT_COLOR].Sourced = WRITEMASK_XYZW;
	s->Outputs[FRAG_RESULT_DEPTH].Sourced = WRITEMASK_W;
}

/**
 * Transform the program to support fragment.position.
 *
 * Introduce a small fragment at the start of the program that will be
 * the only code that directly reads the FRAG_ATTRIB_WPOS input.
 * All other code pieces that reference that input will be rewritten
 * to read from a newly allocated temporary.
 *
 */
static void insert_WPOS_trailer(struct r300_fragment_program_compiler *compiler)
{
	GLuint InputsRead = compiler->fp->Base.Base.InputsRead;

	if (!(InputsRead & FRAG_BIT_WPOS))
		return;

	static gl_state_index tokens[STATE_LENGTH] = {
		STATE_INTERNAL, STATE_R300_WINDOW_DIMENSION, 0, 0, 0
	};
	struct prog_instruction *fpi;
	GLuint window_index;
	int i = 0;
	GLuint tempregi = _mesa_find_free_register(compiler->program, PROGRAM_TEMPORARY);

	_mesa_insert_instructions(compiler->program, 0, 3);
	fpi = compiler->program->Instructions;

	/* perspective divide */
	fpi[i].Opcode = OPCODE_RCP;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_W;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_INPUT;
	fpi[i].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
	fpi[i].SrcReg[0].Swizzle = SWIZZLE_WWWW;
	i++;

	fpi[i].Opcode = OPCODE_MUL;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_XYZ;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_INPUT;
	fpi[i].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
	fpi[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;

	fpi[i].SrcReg[1].File = PROGRAM_TEMPORARY;
	fpi[i].SrcReg[1].Index = tempregi;
	fpi[i].SrcReg[1].Swizzle = SWIZZLE_WWWW;
	i++;

	/* viewport transformation */
	window_index = _mesa_add_state_reference(compiler->program->Parameters, tokens);

	fpi[i].Opcode = OPCODE_MAD;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_XYZ;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_TEMPORARY;
	fpi[i].SrcReg[0].Index = tempregi;
	fpi[i].SrcReg[0].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	fpi[i].SrcReg[1].File = PROGRAM_STATE_VAR;
	fpi[i].SrcReg[1].Index = window_index;
	fpi[i].SrcReg[1].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	fpi[i].SrcReg[2].File = PROGRAM_STATE_VAR;
	fpi[i].SrcReg[2].Index = window_index;
	fpi[i].SrcReg[2].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);
	i++;

	for (; i < compiler->program->NumInstructions; ++i) {
		int reg;
		for (reg = 0; reg < 3; reg++) {
			if (fpi[i].SrcReg[reg].File == PROGRAM_INPUT &&
			    fpi[i].SrcReg[reg].Index == FRAG_ATTRIB_WPOS) {
				fpi[i].SrcReg[reg].File = PROGRAM_TEMPORARY;
				fpi[i].SrcReg[reg].Index = tempregi;
			}
		}
	}
}

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
	struct r300_fragment_program *fp,
	struct r300_fragment_program_external_state *state)
{
	int unit;

	_mesa_bzero(state, sizeof(*state));

	for(unit = 0; unit < 16; ++unit) {
		if (fp->Base.Base.ShadowSamplers & (1 << unit)) {
			struct gl_texture_object* tex = r300->radeon.glCtx->Texture.Unit[unit]._Current;

			state->unit[unit].depth_texture_mode = build_dtm(tex->DepthMode);
			state->unit[unit].texture_compare_func = build_func(tex->CompareFunc);
		}
	}
}

void r300TranslateFragmentShader(GLcontext *ctx, struct gl_fragment_program *fp)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_fragment_program *r300_fp = (struct r300_fragment_program *)fp;
	struct r300_fragment_program_external_state state;

	build_state(r300, r300_fp, &state);
	if (_mesa_memcmp(&r300_fp->state, &state, sizeof(state))) {
		/* TODO: cache compiled programs */
		r300_fp->translated = GL_FALSE;
		_mesa_memcpy(&r300_fp->state, &state, sizeof(state));
	}

	if (!r300_fp->translated) {
		struct r300_fragment_program_compiler compiler;

		compiler.r300 = r300;
		compiler.fp = r300_fp;
		compiler.code = &r300_fp->code;
		compiler.program = _mesa_clone_program(ctx, &fp->Base);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Fragment Program: Initial program:\n");
			_mesa_print_program(compiler.program);
		}

		insert_WPOS_trailer(&compiler);

		if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
			struct radeon_program_transformation transformations[] = {
				{ &r500_transform_TEX, &compiler },
				{ &radeonTransformALU, 0 },
				{ &radeonTransformDeriv, 0 },
				{ &radeonTransformTrigScale, 0 }
			};
			radeonLocalTransform(ctx, compiler.program, 4, transformations);
		} else {
			struct radeon_program_transformation transformations[] = {
				{ &r300_transform_TEX, &compiler },
				{ &radeonTransformALU, 0 },
				{ &radeonTransformTrigSimple, 0 }
			};
			radeonLocalTransform(ctx, compiler.program, 3, transformations);
		}

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Fragment Program: After native rewrite:\n");
			_mesa_print_program(compiler.program);
		}

		if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
			struct radeon_nqssadce_descr nqssadce = {
				.Init = &nqssadce_init,
				.IsNativeSwizzle = &r500FPIsNativeSwizzle,
				.BuildSwizzle = &r500FPBuildSwizzle,
				.RewriteDepthOut = GL_TRUE
			};
			radeonNqssaDce(ctx, compiler.program, &nqssadce);
		} else {
			struct radeon_nqssadce_descr nqssadce = {
				.Init = &nqssadce_init,
				.IsNativeSwizzle = &r300FPIsNativeSwizzle,
				.BuildSwizzle = &r300FPBuildSwizzle,
				.RewriteDepthOut = GL_TRUE
			};
			radeonNqssaDce(ctx, compiler.program, &nqssadce);
		}

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Compiler: after NqSSA-DCE:\n");
			_mesa_print_program(compiler.program);
		}

		if (!r300->vtbl.FragmentProgramEmit(&compiler))
			r300_fp->error = GL_TRUE;

		/* Subtle: Rescue any parameters that have been added during transformations */
		_mesa_free_parameter_list(fp->Base.Parameters);
		fp->Base.Parameters = compiler.program->Parameters;
		compiler.program->Parameters = 0;

		_mesa_reference_program(ctx, &compiler.program, NULL);

		r300_fp->translated = GL_TRUE;

		r300UpdateStateParameters(ctx, _NEW_PROGRAM);

		if (r300_fp->error || (RADEON_DEBUG & DEBUG_PIXEL))
			r300->vtbl.FragmentProgramDump(&r300_fp->code);
	}

	update_params(ctx, fp);
}
