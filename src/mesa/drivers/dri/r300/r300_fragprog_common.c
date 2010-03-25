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

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "compiler/radeon_compiler.h"

#include "radeon_mesa_to_rc.h"


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

	memset(state, 0, sizeof(*state));

	for(unit = 0; unit < 16; ++unit) {
		if (fp->Base.ShadowSamplers & (1 << unit)) {
			struct gl_texture_object* tex = r300->radeon.glCtx->Texture.Unit[unit]._Current;

			state->unit[unit].depth_texture_mode = build_dtm(tex->DepthMode);
			state->unit[unit].texture_compare_func = build_func(tex->CompareFunc);
		}
	}
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
static void insert_WPOS_trailer(struct r300_fragment_program_compiler *compiler, struct r300_fragment_program * fp)
{
	int i;

	fp->wpos_attr = FRAG_ATTRIB_MAX;
	if (!(compiler->Base.Program.InputsRead & FRAG_BIT_WPOS)) {
		return;
	}

	for (i = FRAG_ATTRIB_TEX0; i <= FRAG_ATTRIB_TEX7; ++i)
	{
		if (!(compiler->Base.Program.InputsRead & (1 << i))) {
			fp->wpos_attr = i;
			break;
		}
	}

	/* No free texcoord found, fall-back to software rendering */
	if (fp->wpos_attr == FRAG_ATTRIB_MAX)
	{
		compiler->Base.Error = 1;
		return;
	}

	rc_transform_fragment_wpos(&compiler->Base, FRAG_ATTRIB_WPOS, fp->wpos_attr, GL_FALSE);
}

/**
 * Rewrite fragment.fogcoord to use a texture coordinate slot.
 * Note that fogcoord is forced into an X001 pattern, and this enforcement
 * is done here.
 *
 * See also the counterpart rewriting for vertex programs.
 */
static void rewriteFog(struct r300_fragment_program_compiler *compiler, struct r300_fragment_program * fp)
{
	struct rc_src_register src;
	int i;

	fp->fog_attr = FRAG_ATTRIB_MAX;
	if (!(compiler->Base.Program.InputsRead & FRAG_BIT_FOGC)) {
		return;
	}

	for (i = FRAG_ATTRIB_TEX0; i <= FRAG_ATTRIB_TEX7; ++i)
	{
		if (!(compiler->Base.Program.InputsRead & (1 << i))) {
			fp->fog_attr = i;
			break;
		}
	}

	/* No free texcoord found, fall-back to software rendering */
	if (fp->fog_attr == FRAG_ATTRIB_MAX)
	{
		compiler->Base.Error = 1;
		return;
	}

	memset(&src, 0, sizeof(src));
	src.File = RC_FILE_INPUT;
	src.Index = fp->fog_attr;
	src.Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ONE);
	rc_move_input(&compiler->Base, FRAG_ATTRIB_FOGC, src);
}


/**
 * Reserve hardware temporary registers for the program inputs.
 *
 * @note This allocation is performed explicitly, because the order of inputs
 * is determined by the RS hardware.
 */
static void allocate_hw_inputs(
	struct r300_fragment_program_compiler * c,
	void (*allocate)(void * data, unsigned input, unsigned hwreg),
	void * mydata)
{
	GLuint InputsRead = c->Base.Program.InputsRead;
	int i;
	GLuint hwindex = 0;

	/* Primary colour */
	if (InputsRead & FRAG_BIT_COL0)
		allocate(mydata, FRAG_ATTRIB_COL0, hwindex++);
	InputsRead &= ~FRAG_BIT_COL0;

	/* Secondary color */
	if (InputsRead & FRAG_BIT_COL1)
		allocate(mydata, FRAG_ATTRIB_COL1, hwindex++);
	InputsRead &= ~FRAG_BIT_COL1;

	/* Texcoords */
	for (i = 0; i < 8; i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i))
			allocate(mydata, FRAG_ATTRIB_TEX0+i, hwindex++);
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* Fogcoords treated as a texcoord */
	if (InputsRead & FRAG_BIT_FOGC)
		allocate(mydata, FRAG_ATTRIB_FOGC, hwindex++);
	InputsRead &= ~FRAG_BIT_FOGC;

	/* fragment position treated as a texcoord */
	if (InputsRead & FRAG_BIT_WPOS)
		allocate(mydata, FRAG_ATTRIB_WPOS, hwindex++);
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Anything else */
	if (InputsRead)
		rc_error(&c->Base, "Don't know how to handle inputs 0x%x\n", InputsRead);
}


static void translate_fragment_program(GLcontext *ctx, struct r300_fragment_program_cont *cont, struct r300_fragment_program *fp)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_fragment_program_compiler compiler;

	rc_init(&compiler.Base);
	compiler.Base.Debug = (RADEON_DEBUG & RADEON_PIXEL) ? GL_TRUE : GL_FALSE;

	compiler.code = &fp->code;
	compiler.state = fp->state;
	compiler.is_r500 = (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) ? GL_TRUE : GL_FALSE;
	compiler.OutputDepth = FRAG_RESULT_DEPTH;
	memset(compiler.OutputColor, 0, 4 * sizeof(unsigned));
	compiler.OutputColor[0] = FRAG_RESULT_COLOR;
	compiler.AllocateHwInputs = &allocate_hw_inputs;

	if (compiler.Base.Debug) {
		fflush(stderr);
		printf("Fragment Program: Initial program:\n");
		_mesa_print_program(&cont->Base.Base);
		fflush(stderr);
	}

	radeon_mesa_to_rc_program(&compiler.Base, &cont->Base.Base);

	insert_WPOS_trailer(&compiler, fp);

	rewriteFog(&compiler, fp);

	r3xx_compile_fragment_program(&compiler);

	if (compiler.is_r500) {
		/* We need to support the non-KMS DRM interface, which
		 * artificially limits the number of instructions and
		 * constants which are available to us.
		 *
		 * See also the comment in r300_context.c where we
		 * set the MAX_NATIVE_xxx values.
		 */
		if (fp->code.code.r500.inst_end >= 255 || fp->code.constants.Count > 255)
			rc_error(&compiler.Base, "Program is too big (upgrade to r300g to avoid this limitation).\n");
	}

	fp->error = compiler.Base.Error;

	fp->InputsRead = compiler.Base.Program.InputsRead;

	/* Clear the fog/wpos_attr if code accessing these
	 * attributes has been removed during compilation
	 */
	if (fp->fog_attr != FRAG_ATTRIB_MAX) {
		if (!(fp->InputsRead & (1 << fp->fog_attr)))
			fp->fog_attr = FRAG_ATTRIB_MAX;
	}

	if (fp->wpos_attr != FRAG_ATTRIB_MAX) {
		if (!(fp->InputsRead & (1 << fp->wpos_attr)))
			fp->wpos_attr = FRAG_ATTRIB_MAX;
	}

	rc_destroy(&compiler.Base);
}

struct r300_fragment_program *r300SelectAndTranslateFragmentShader(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_fragment_program_cont *fp_list;
	struct r300_fragment_program *fp;
	struct r300_fragment_program_external_state state;

	fp_list = (struct r300_fragment_program_cont *)ctx->FragmentProgram._Current;
	build_state(r300, ctx->FragmentProgram._Current, &state);

	fp = fp_list->progs;
	while (fp) {
		if (memcmp(&fp->state, &state, sizeof(state)) == 0) {
			return r300->selected_fp = fp;
		}
		fp = fp->next;
	}

	fp = calloc(1, sizeof(struct r300_fragment_program));

	fp->state = state;

	fp->next = fp_list->progs;
	fp_list->progs = fp;

	translate_fragment_program(ctx, fp_list, fp);

	return r300->selected_fp = fp;
}
