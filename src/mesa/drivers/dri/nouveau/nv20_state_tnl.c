/*
 * Copyright (C) 2009-2010 Francisco Jerez.
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
#include "nouveau_util.h"
#include "nouveau_class.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

void
nv20_emit_clip_plane(GLcontext *ctx, int emit)
{
}

static inline unsigned
get_material_bitmask(unsigned m)
{
	unsigned ret = 0;

	if (m & MAT_BIT_FRONT_EMISSION)
		ret |= NV20TCL_COLOR_MATERIAL_FRONT_EMISSION_COL1;
	if (m & MAT_BIT_FRONT_AMBIENT)
		ret |= NV20TCL_COLOR_MATERIAL_FRONT_AMBIENT_COL1;
	if (m & MAT_BIT_FRONT_DIFFUSE)
		ret |= NV20TCL_COLOR_MATERIAL_FRONT_DIFFUSE_COL1;
	if (m & MAT_BIT_FRONT_SPECULAR)
		ret |= NV20TCL_COLOR_MATERIAL_FRONT_SPECULAR_COL1;

	if (m & MAT_BIT_BACK_EMISSION)
		ret |= NV20TCL_COLOR_MATERIAL_BACK_EMISSION_COL1;
	if (m & MAT_BIT_BACK_AMBIENT)
		ret |= NV20TCL_COLOR_MATERIAL_BACK_AMBIENT_COL1;
	if (m & MAT_BIT_BACK_DIFFUSE)
		ret |= NV20TCL_COLOR_MATERIAL_BACK_DIFFUSE_COL1;
	if (m & MAT_BIT_BACK_SPECULAR)
		ret |= NV20TCL_COLOR_MATERIAL_BACK_SPECULAR_COL1;

	return ret;
}

void
nv20_emit_color_material(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	unsigned mask = get_material_bitmask(ctx->Light.ColorMaterialBitmask);

	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_MATERIAL, 1);
	OUT_RING(chan, ctx->Light.ColorMaterialEnabled ? mask : 0);
}

static unsigned
get_fog_mode_signed(unsigned mode)
{
	switch (mode) {
	case GL_LINEAR:
		return NV20TCL_FOG_MODE_LINEAR_SIGNED;
	case GL_EXP:
		return NV20TCL_FOG_MODE_EXP_SIGNED;
	case GL_EXP2:
		return NV20TCL_FOG_MODE_EXP2_SIGNED;
	default:
		assert(0);
	}
}

static unsigned
get_fog_mode_unsigned(unsigned mode)
{
	switch (mode) {
	case GL_LINEAR:
		return NV20TCL_FOG_MODE_LINEAR_UNSIGNED;
	case GL_EXP:
		return NV20TCL_FOG_MODE_EXP_UNSIGNED;
	case GL_EXP2:
		return NV20TCL_FOG_MODE_EXP2_UNSIGNED;
	default:
		assert(0);
	}
}

static unsigned
get_fog_source(unsigned source)
{
	switch (source) {
	case GL_FOG_COORDINATE_EXT:
		return NV20TCL_FOG_COORD_FOG;
	case GL_FRAGMENT_DEPTH_EXT:
		return NV20TCL_FOG_COORD_DIST_ORTHOGONAL_ABS;
	default:
		assert(0);
	}
}

void
nv20_emit_fog(GLcontext *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct gl_fog_attrib *f = &ctx->Fog;
	unsigned source = nctx->fallback == HWTNL ?
		f->FogCoordinateSource : GL_FOG_COORDINATE_EXT;
	float k[3];

	nv10_get_fog_coeff(ctx, k);

	BEGIN_RING(chan, kelvin, NV20TCL_FOG_MODE, 4);
	OUT_RING(chan, (source == GL_FOG_COORDINATE_EXT ?
			get_fog_mode_signed(f->Mode) :
			get_fog_mode_unsigned(f->Mode)));
	OUT_RING(chan, get_fog_source(source));
	OUT_RING(chan, f->Enabled ? 1 : 0);
	OUT_RING(chan, pack_rgba_f(MESA_FORMAT_RGBA8888_REV, f->Color));

	BEGIN_RING(chan, kelvin, NV20TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RINGf(chan, k[0]);
	OUT_RINGf(chan, k[1]);
	OUT_RINGf(chan, k[2]);
}

void
nv20_emit_light_model(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct gl_lightmodel *m = &ctx->Light.Model;

	BEGIN_RING(chan, kelvin, NV20TCL_SEPARATE_SPECULAR_ENABLE, 1);
	OUT_RING(chan, m->ColorControl == GL_SEPARATE_SPECULAR_COLOR ? 1 : 0);

	BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_MODEL, 1);
	OUT_RING(chan, ((m->LocalViewer ?
			 NV20TCL_LIGHT_MODEL_VIEWER_LOCAL :
			 NV20TCL_LIGHT_MODEL_VIEWER_NONLOCAL) |
			(m->ColorControl == GL_SEPARATE_SPECULAR_COLOR ?
			 NV20TCL_LIGHT_MODEL_SEPARATE_SPECULAR :
			 0)));

	BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_MODEL_TWO_SIDE_ENABLE, 1);
	OUT_RING(chan, ctx->Light.Model.TwoSide ? 1 : 0);
}

void
nv20_emit_light_source(GLcontext *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_LIGHT_SOURCE0;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct gl_light *l = &ctx->Light.Light[i];

	if (l->_Flags & LIGHT_POSITIONAL) {
		BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_POSITION_X(i), 3);
		OUT_RINGf(chan, l->_Position[0]);
		OUT_RINGf(chan, l->_Position[1]);
		OUT_RINGf(chan, l->_Position[2]);

		BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_ATTENUATION_CONSTANT(i), 3);
		OUT_RINGf(chan, l->ConstantAttenuation);
		OUT_RINGf(chan, l->LinearAttenuation);
		OUT_RINGf(chan, l->QuadraticAttenuation);

	} else {
		BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_DIRECTION_X(i), 3);
		OUT_RINGf(chan, l->_VP_inf_norm[0]);
		OUT_RINGf(chan, l->_VP_inf_norm[1]);
		OUT_RINGf(chan, l->_VP_inf_norm[2]);

		BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_HALF_VECTOR_X(i), 3);
		OUT_RINGf(chan, l->_h_inf_norm[0]);
		OUT_RINGf(chan, l->_h_inf_norm[1]);
		OUT_RINGf(chan, l->_h_inf_norm[2]);
	}

	if (l->_Flags & LIGHT_SPOT) {
		float k[7];

		nv10_get_spot_coeff(l, k);

		BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_SPOT_CUTOFF_A(i), 7);
		OUT_RINGf(chan, k[0]);
		OUT_RINGf(chan, k[1]);
		OUT_RINGf(chan, k[2]);
		OUT_RINGf(chan, k[3]);
		OUT_RINGf(chan, k[4]);
		OUT_RINGf(chan, k[5]);
		OUT_RINGf(chan, k[6]);
	}
}

#define USE_COLOR_MATERIAL(attr, side)					\
	(ctx->Light.ColorMaterialEnabled &&				\
	 ctx->Light.ColorMaterialBitmask & (1 << MAT_ATTRIB_##attr(side)))

void
nv20_emit_material_ambient(GLcontext *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	float (*mat)[4] = ctx->Light.Material.Attrib;
	uint32_t m_scene[] = { NV20TCL_LIGHT_MODEL_FRONT_AMBIENT_R,
			       NV20TCL_LIGHT_MODEL_BACK_AMBIENT_R };
	uint32_t m_factor[] = { NV20TCL_MATERIAL_FACTOR_FRONT_R,
			      NV20TCL_MATERIAL_FACTOR_BACK_R };
	float c_scene[3], c_factor[3];
	struct gl_light *l;

	if (USE_COLOR_MATERIAL(AMBIENT, side)) {
		COPY_3V(c_scene, mat[MAT_ATTRIB_EMISSION(side)]);
		COPY_3V(c_factor, ctx->Light.Model.Ambient);

	} else if (USE_COLOR_MATERIAL(EMISSION, side)) {
		SCALE_3V(c_scene, mat[MAT_ATTRIB_AMBIENT(side)],
			 ctx->Light.Model.Ambient);
		ASSIGN_3V(c_factor, 1, 1, 1);

	} else {
		COPY_3V(c_scene, ctx->Light._BaseColor[side]);
		ZERO_3V(c_factor);
	}

	BEGIN_RING(chan, kelvin, m_scene[side], 3);
	OUT_RINGf(chan, c_scene[0]);
	OUT_RINGf(chan, c_scene[1]);
	OUT_RINGf(chan, c_scene[2]);

	if (ctx->Light.ColorMaterialEnabled) {
		BEGIN_RING(chan, kelvin, m_factor[side], 3);
		OUT_RINGf(chan, c_factor[0]);
		OUT_RINGf(chan, c_factor[1]);
		OUT_RINGf(chan, c_factor[2]);
	}

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		uint32_t m_light[] = { NV20TCL_LIGHT_FRONT_AMBIENT_R(i),
				      NV20TCL_LIGHT_BACK_AMBIENT_R(i) };
		float *c_light = (USE_COLOR_MATERIAL(AMBIENT, side) ?
				  l->Ambient :
				  l->_MatAmbient[side]);

		BEGIN_RING(chan, kelvin, m_light[side], 3);
		OUT_RINGf(chan, c_light[0]);
		OUT_RINGf(chan, c_light[1]);
		OUT_RINGf(chan, c_light[2]);
	}
}

void
nv20_emit_material_diffuse(GLcontext *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_DIFFUSE;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
	uint32_t m_factor[] = { NV20TCL_MATERIAL_FACTOR_FRONT_A,
				NV20TCL_MATERIAL_FACTOR_BACK_A };
	struct gl_light *l;

	BEGIN_RING(chan, kelvin, m_factor[side], 1);
	OUT_RINGf(chan, mat[MAT_ATTRIB_DIFFUSE(side)][3]);

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		uint32_t m_light[] = { NV20TCL_LIGHT_FRONT_DIFFUSE_R(i),
				       NV20TCL_LIGHT_BACK_DIFFUSE_R(i) };
		float *c_light = (USE_COLOR_MATERIAL(DIFFUSE, side) ?
				  l->Diffuse :
				  l->_MatDiffuse[side]);

		BEGIN_RING(chan, kelvin, m_light[side], 3);
		OUT_RINGf(chan, c_light[0]);
		OUT_RINGf(chan, c_light[1]);
		OUT_RINGf(chan, c_light[2]);
	}
}

void
nv20_emit_material_specular(GLcontext *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_SPECULAR;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct gl_light *l;

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		uint32_t m_light[] = { NV20TCL_LIGHT_FRONT_SPECULAR_R(i),
				       NV20TCL_LIGHT_BACK_SPECULAR_R(i) };
		float *c_light = (USE_COLOR_MATERIAL(SPECULAR, side) ?
				  l->Specular :
				  l->_MatSpecular[side]);

		BEGIN_RING(chan, kelvin, m_light[side], 3);
		OUT_RINGf(chan, c_light[0]);
		OUT_RINGf(chan, c_light[1]);
		OUT_RINGf(chan, c_light[2]);
	}
}

void
nv20_emit_material_shininess(GLcontext *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_SHININESS;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	float (*mat)[4] = ctx->Light.Material.Attrib;
	uint32_t mthd[] = { NV20TCL_FRONT_MATERIAL_SHININESS(0),
			    NV20TCL_BACK_MATERIAL_SHININESS(0) };
	float k[6];

	nv10_get_shininess_coeff(
		CLAMP(mat[MAT_ATTRIB_SHININESS(side)][0], 0, 1024),
		k);

	BEGIN_RING(chan, kelvin, mthd[side], 6);
	OUT_RINGf(chan, k[0]);
	OUT_RINGf(chan, k[1]);
	OUT_RINGf(chan, k[2]);
	OUT_RINGf(chan, k[3]);
	OUT_RINGf(chan, k[4]);
	OUT_RINGf(chan, k[5]);
}

void
nv20_emit_modelview(GLcontext *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	GLmatrix *m = ctx->ModelviewMatrixStack.Top;

	if (nctx->fallback != HWTNL)
		return;

	if (ctx->Light._NeedEyeCoords || ctx->Fog.Enabled) {
		BEGIN_RING(chan, kelvin, NV20TCL_MODELVIEW0_MATRIX(0), 16);
		OUT_RINGm(chan, m->m);
	}

	if (ctx->Light.Enabled) {
		int i, j;

		BEGIN_RING(chan, kelvin,
			   NV20TCL_INVERSE_MODELVIEW0_MATRIX(0), 12);
		for (i = 0; i < 3; i++)
			for (j = 0; j < 4; j++)
				OUT_RINGf(chan, m->inv[4*i + j]);
	}
}

void
nv20_emit_projection(GLcontext *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	GLmatrix m;

	_math_matrix_ctr(&m);
	get_viewport_scale(ctx, m.m);

	if (nctx->fallback == HWTNL)
		_math_matrix_mul_matrix(&m, &m, &ctx->_ModelProjectMatrix);

	BEGIN_RING(chan, kelvin, NV20TCL_PROJECTION_MATRIX(0), 16);
	OUT_RINGm(chan, m.m);

	_math_matrix_dtr(&m);
}
