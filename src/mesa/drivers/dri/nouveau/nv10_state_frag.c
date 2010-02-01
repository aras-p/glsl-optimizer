/*
 * Copyright (C) 2009 Francisco Jerez.
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

#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_gldefs.h"
#include "nouveau_class.h"
#include "nouveau_util.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

#define RC_IN_SHIFT_A	24
#define RC_IN_SHIFT_B	16
#define RC_IN_SHIFT_C	8
#define RC_IN_SHIFT_D	0
#define RC_IN_SHIFT_E	56
#define RC_IN_SHIFT_F	48
#define RC_IN_SHIFT_G	40

#define RC_IN_SOURCE(source)				\
	((uint64_t)NV10TCL_RC_IN_RGB_D_INPUT_##source)
#define RC_IN_USAGE(usage)					\
	((uint64_t)NV10TCL_RC_IN_RGB_D_COMPONENT_USAGE_##usage)
#define RC_IN_MAPPING(mapping)					\
	((uint64_t)NV10TCL_RC_IN_RGB_D_MAPPING_##mapping)

#define RC_OUT_BIAS	NV10TCL_RC_OUT_RGB_BIAS_BIAS_BY_NEGATIVE_ONE_HALF
#define RC_OUT_SCALE_1	NV10TCL_RC_OUT_RGB_SCALE_NONE
#define RC_OUT_SCALE_2	NV10TCL_RC_OUT_RGB_SCALE_SCALE_BY_TWO
#define RC_OUT_SCALE_4	NV10TCL_RC_OUT_RGB_SCALE_SCALE_BY_FOUR

/* Make the combiner do: spare0_i = A_i * B_i */
#define RC_OUT_AB	NV10TCL_RC_OUT_RGB_AB_OUTPUT_SPARE0
/* spare0_i = dot3(A, B) */
#define RC_OUT_DOT_AB	(NV10TCL_RC_OUT_RGB_AB_OUTPUT_SPARE0 |	\
			 NV10TCL_RC_OUT_RGB_AB_DOT_PRODUCT)
/* spare0_i = A_i * B_i + C_i * D_i */
#define RC_OUT_SUM	NV10TCL_RC_OUT_RGB_SUM_OUTPUT_SPARE0

struct combiner_state {
	GLcontext *ctx;
	int unit;

	/* GL state */
	GLenum mode;
	GLenum *source;
	GLenum *operand;
	GLuint logscale;

	/* Derived HW state */
	uint64_t in;
	uint32_t out;
};

/* Initialize a combiner_state struct from the texture unit
 * context. */
#define INIT_COMBINER(chan, ctx, rc, i) do {			\
		struct gl_tex_env_combine_state *c =		\
			ctx->Texture.Unit[i]._CurrentCombine;	\
		(rc)->ctx = ctx;				\
		(rc)->unit = i;					\
		(rc)->mode = c->Mode##chan;			\
		(rc)->source = c->Source##chan;			\
		(rc)->operand = c->Operand##chan;		\
		(rc)->logscale = c->ScaleShift##chan;		\
		(rc)->in = (rc)->out = 0;			\
	} while (0)

/* Get the RC input source for the specified EXT_texture_env_combine
 * argument. */
static uint32_t
get_input_source(struct combiner_state *rc, int arg)
{
	switch (rc->source[arg]) {
	case GL_TEXTURE:
		return RC_IN_SOURCE(TEXTURE0) + rc->unit;

	case GL_TEXTURE0:
		return RC_IN_SOURCE(TEXTURE0);

	case GL_TEXTURE1:
		return RC_IN_SOURCE(TEXTURE1);

	case GL_TEXTURE2:
		return RC_IN_SOURCE(TEXTURE2);

	case GL_TEXTURE3:
		return RC_IN_SOURCE(TEXTURE3);

	case GL_CONSTANT:
		return context_chipset(rc->ctx) >= 0x20 ?
			RC_IN_SOURCE(CONSTANT_COLOR0) :
			RC_IN_SOURCE(CONSTANT_COLOR0) + rc->unit;

	case GL_PRIMARY_COLOR:
		return RC_IN_SOURCE(PRIMARY_COLOR);

	case GL_PREVIOUS:
		return rc->unit ? RC_IN_SOURCE(SPARE0)
			: RC_IN_SOURCE(PRIMARY_COLOR);

	default:
		assert(0);
	}
}

/* Get the RC input mapping for the specified argument, possibly
 * inverted or biased. */
#define INVERT 0x1
#define HALF_BIAS 0x2

static uint32_t
get_input_mapping(struct combiner_state *rc, int arg, int flags)
{
	int map = 0;

	switch (rc->operand[arg]) {
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
		map |= RC_IN_USAGE(RGB);
		break;

	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
		map |= RC_IN_USAGE(ALPHA);
		break;
	}

	switch (rc->operand[arg]) {
	case GL_SRC_COLOR:
	case GL_SRC_ALPHA:
		map |= (flags & INVERT ? RC_IN_MAPPING(UNSIGNED_INVERT) :
			flags & HALF_BIAS ? RC_IN_MAPPING(HALF_BIAS_NORMAL) :
			RC_IN_MAPPING(UNSIGNED_IDENTITY));
		break;

	case GL_ONE_MINUS_SRC_COLOR:
	case GL_ONE_MINUS_SRC_ALPHA:
		map |= (flags & INVERT ? RC_IN_MAPPING(UNSIGNED_IDENTITY) :
			flags & HALF_BIAS ? RC_IN_MAPPING(HALF_BIAS_NEGATE) :
			RC_IN_MAPPING(UNSIGNED_INVERT));
		break;
	}

	return map;
}

/* Bind the RC input variable <var> to the EXT_texture_env_combine
 * argument <arg>, possibly inverted or biased. */
#define INPUT_ARG(rc, var, arg, flags)					\
	(rc)->in |= (get_input_mapping(rc, arg, flags) |		\
		     get_input_source(rc, arg)) << RC_IN_SHIFT_##var

/* Bind the RC input variable <var> to the RC source <src>. */
#define INPUT_SRC(rc, var, src, chan)					\
	(rc)->in |= (RC_IN_SOURCE(src) |				\
		     RC_IN_USAGE(chan)) << RC_IN_SHIFT_##var

/* Bind the RC input variable <var> to a constant +/-1 */
#define INPUT_ONE(rc, var, flags)					\
	(rc)->in |= (RC_IN_SOURCE(ZERO) |				\
		     (flags & INVERT ? RC_IN_MAPPING(EXPAND_NORMAL) :	\
		      RC_IN_MAPPING(UNSIGNED_INVERT))) << RC_IN_SHIFT_##var

static void
setup_combiner(struct combiner_state *rc)
{
	switch (rc->mode) {
	case GL_REPLACE:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ONE(rc, B, 0);

		rc->out = RC_OUT_AB;
		break;

	case GL_MODULATE:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ARG(rc, B, 1, 0);

		rc->out = RC_OUT_AB;
		break;

	case GL_ADD:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ONE(rc, B, 0);
		INPUT_ARG(rc, C, 1, 0);
		INPUT_ONE(rc, D, 0);

		rc->out = RC_OUT_SUM;
		break;

	case GL_ADD_SIGNED:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ONE(rc, B, 0);
		INPUT_ARG(rc, C, 1, 0);
		INPUT_ONE(rc, D, 0);

		rc->out = RC_OUT_SUM | RC_OUT_BIAS;
		break;

	case GL_INTERPOLATE:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ARG(rc, B, 2, 0);
		INPUT_ARG(rc, C, 1, 0);
		INPUT_ARG(rc, D, 2, INVERT);

		rc->out = RC_OUT_SUM;
		break;

	case GL_SUBTRACT:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ONE(rc, B, 0);
		INPUT_ARG(rc, C, 1, 0);
		INPUT_ONE(rc, D, INVERT);

		rc->out = RC_OUT_SUM;
		break;

	case GL_DOT3_RGB:
	case GL_DOT3_RGBA:
		INPUT_ARG(rc, A, 0, HALF_BIAS);
		INPUT_ARG(rc, B, 1, HALF_BIAS);

		rc->out = RC_OUT_DOT_AB | RC_OUT_SCALE_4;

		assert(!rc->logscale);
		break;

	default:
		assert(0);
	}

	switch (rc->logscale) {
	case 0:
		rc->out |= RC_OUT_SCALE_1;
		break;
	case 1:
		rc->out |= RC_OUT_SCALE_2;
		break;
	case 2:
		rc->out |= RC_OUT_SCALE_4;
		break;
	default:
		assert(0);
	}
}

/* Write the register combiner state out to the hardware. */
static void
nv10_load_combiner(GLcontext *ctx, int i, struct combiner_state *rc_a,
		   struct combiner_state *rc_c, uint32_t rc_const)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *celsius = context_eng3d(ctx);

	/* Enable the combiners we're going to need. */
	if (i == 1) {
		if (rc_c->out || rc_a->out)
			rc_c->out |= 0x5 << 27;
		else
			rc_c->out |= 0x3 << 27;
	}

	BEGIN_RING(chan, celsius, NV10TCL_RC_IN_ALPHA(i), 1);
	OUT_RING(chan, rc_a->in);
	BEGIN_RING(chan, celsius, NV10TCL_RC_IN_RGB(i), 1);
	OUT_RING(chan, rc_c->in);
	BEGIN_RING(chan, celsius, NV10TCL_RC_COLOR(i), 1);
	OUT_RING(chan, rc_const);
	BEGIN_RING(chan, celsius, NV10TCL_RC_OUT_ALPHA(i), 1);
	OUT_RING(chan, rc_a->out);
	BEGIN_RING(chan, celsius, NV10TCL_RC_OUT_RGB(i), 1);
	OUT_RING(chan, rc_c->out);
}

static void
nv10_load_final(GLcontext *ctx, struct combiner_state *rc, int n)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *celsius = context_eng3d(ctx);

	BEGIN_RING(chan, celsius, NV10TCL_RC_FINAL0, 2);
	OUT_RING(chan, rc->in);
	OUT_RING(chan, rc->in >> 32);
}

static void
nv20_load_combiner(GLcontext *ctx, int i, struct combiner_state *rc_a,
		   struct combiner_state *rc_c, uint32_t rc_const)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);

	BEGIN_RING(chan, kelvin, NV20TCL_RC_IN_ALPHA(i), 1);
	OUT_RING(chan, rc_a->in);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_OUT_ALPHA(i), 1);
	OUT_RING(chan, rc_a->out);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_IN_RGB(i), 1);
	OUT_RING(chan, rc_c->in);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_OUT_RGB(i), 1);
	OUT_RING(chan, rc_c->out);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_CONSTANT_COLOR0(i), 1);
	OUT_RING(chan, rc_const);
}

static void
nv20_load_final(GLcontext *ctx, struct combiner_state *rc, int n)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);

	BEGIN_RING(chan, kelvin, NV20TCL_RC_FINAL0, 2);
	OUT_RING(chan, rc->in);
	OUT_RING(chan, rc->in >> 32);

	BEGIN_RING(chan, kelvin, NV20TCL_RC_ENABLE, 1);
	OUT_RING(chan, n);
}

void
nv10_emit_tex_env(GLcontext *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_ENV0;
	struct combiner_state rc_a, rc_c;
	uint32_t rc_const;

	/* Compute the new combiner state. */
	if (ctx->Texture.Unit[i]._ReallyEnabled) {
		INIT_COMBINER(RGB, ctx, &rc_c, i);

		if (rc_c.mode == GL_DOT3_RGBA)
			rc_a = rc_c;
		else
			INIT_COMBINER(A, ctx, &rc_a, i);

		setup_combiner(&rc_c);
		setup_combiner(&rc_a);

		rc_const = pack_rgba_f(MESA_FORMAT_ARGB8888,
				       ctx->Texture.Unit[i].EnvColor);

	} else {
		rc_a.in = rc_a.out = rc_c.in = rc_c.out = rc_const = 0;
	}

	if (context_chipset(ctx) >= 0x20)
		nv20_load_combiner(ctx, i, &rc_a, &rc_c, rc_const);
	else
		nv10_load_combiner(ctx, i, &rc_a, &rc_c, rc_const);

	context_dirty(ctx, FRAG);
}

void
nv10_emit_frag(GLcontext *ctx, int emit)
{
	struct combiner_state rc = {};
	int n = log2i(ctx->Texture._EnabledUnits) + 1;

	/*
	 * The final fragment value equation is something like:
	 *	x_i = A_i * B_i + (1 - A_i) * C_i + D_i
	 *	x_alpha = G_alpha
	 * where D_i = E_i * F_i, i one of {red, green, blue}.
	 */
	if (ctx->Fog.ColorSumEnabled || ctx->Light.Enabled) {
		INPUT_SRC(&rc, D, E_TIMES_F, RGB);
		INPUT_SRC(&rc, F, SECONDARY_COLOR, RGB);
	}

	if (ctx->Fog.Enabled) {
		INPUT_SRC(&rc, A, FOG, ALPHA);
		INPUT_SRC(&rc, C, FOG, RGB);
		INPUT_SRC(&rc, E, FOG, ALPHA);
	} else {
		INPUT_ONE(&rc, A, 0);
		INPUT_ONE(&rc, C, 0);
		INPUT_ONE(&rc, E, 0);
	}

	if (ctx->Texture._EnabledUnits) {
		INPUT_SRC(&rc, B, SPARE0, RGB);
		INPUT_SRC(&rc, G, SPARE0, ALPHA);
	} else {
		INPUT_SRC(&rc, B, PRIMARY_COLOR, RGB);
		INPUT_SRC(&rc, G, PRIMARY_COLOR, ALPHA);
	}

	if (context_chipset(ctx) >= 0x20)
		nv20_load_final(ctx, &rc, n);
	else
		nv10_load_final(ctx, &rc, n);
}
