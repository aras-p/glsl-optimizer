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
#include "nouveau_texture.h"
#include "nouveau_util.h"

#include "swrast/swrast.h"
#include "tnl/tnl.h"

static void
nouveau_alpha_func(GLcontext *ctx, GLenum func, GLfloat ref)
{
	context_dirty(ctx, ALPHA_FUNC);
}

static void
nouveau_blend_color(GLcontext *ctx, const GLfloat color[4])
{
	context_dirty(ctx, BLEND_COLOR);
}

static void
nouveau_blend_equation_separate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	context_dirty(ctx, BLEND_EQUATION);
}

static void
nouveau_blend_func_separate(GLcontext *ctx, GLenum sfactorRGB,
			    GLenum dfactorRGB, GLenum sfactorA, GLenum dfactorA)
{
	context_dirty(ctx, BLEND_FUNC);
}

static void
nouveau_clip_plane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	context_dirty_i(ctx, CLIP_PLANE, plane - GL_CLIP_PLANE0);
}

static void
nouveau_color_mask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		   GLboolean bmask, GLboolean amask)
{
	context_dirty(ctx, COLOR_MASK);
}

static void
nouveau_color_material(GLcontext *ctx, GLenum face, GLenum mode)
{
	context_dirty(ctx, COLOR_MATERIAL);
	context_dirty(ctx, MATERIAL_FRONT_AMBIENT);
	context_dirty(ctx, MATERIAL_BACK_AMBIENT);
	context_dirty(ctx, MATERIAL_FRONT_DIFFUSE);
	context_dirty(ctx, MATERIAL_BACK_DIFFUSE);
	context_dirty(ctx, MATERIAL_FRONT_SPECULAR);
	context_dirty(ctx, MATERIAL_BACK_SPECULAR);
}

static void
nouveau_cull_face(GLcontext *ctx, GLenum mode)
{
	context_dirty(ctx, CULL_FACE);
}

static void
nouveau_front_face(GLcontext *ctx, GLenum mode)
{
	context_dirty(ctx, FRONT_FACE);
}

static void
nouveau_depth_func(GLcontext *ctx, GLenum func)
{
	context_dirty(ctx, DEPTH);
}

static void
nouveau_depth_mask(GLcontext *ctx, GLboolean flag)
{
	context_dirty(ctx, DEPTH);
}

static void
nouveau_depth_range(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	context_dirty(ctx, VIEWPORT);
}

static void
nouveau_draw_buffer(GLcontext *ctx, GLenum buffer)
{
	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_draw_buffers(GLcontext *ctx, GLsizei n, const GLenum *buffers)
{
	context_dirty(ctx, FRAMEBUFFER);
}

static void
nouveau_enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	int i;

	switch (cap) {
	case GL_ALPHA_TEST:
		context_dirty(ctx, ALPHA_FUNC);
		break;
	case GL_BLEND:
		context_dirty(ctx, BLEND_EQUATION);
		break;
	case GL_COLOR_LOGIC_OP:
		context_dirty(ctx, LOGIC_OPCODE);
		break;
	case GL_COLOR_MATERIAL:
		context_dirty(ctx, COLOR_MATERIAL);
		context_dirty(ctx, MATERIAL_FRONT_AMBIENT);
		context_dirty(ctx, MATERIAL_BACK_AMBIENT);
		context_dirty(ctx, MATERIAL_FRONT_DIFFUSE);
		context_dirty(ctx, MATERIAL_BACK_DIFFUSE);
		context_dirty(ctx, MATERIAL_FRONT_SPECULAR);
		context_dirty(ctx, MATERIAL_BACK_SPECULAR);
		break;
	case GL_COLOR_SUM_EXT:
		context_dirty(ctx, FRAG);
		break;
	case GL_CULL_FACE:
		context_dirty(ctx, CULL_FACE);
		break;
	case GL_DEPTH_TEST:
		context_dirty(ctx, DEPTH);
		break;
	case GL_DITHER:
		context_dirty(ctx, DITHER);
		break;
	case GL_FOG:
		context_dirty(ctx, FOG);
		context_dirty(ctx, FRAG);
		context_dirty(ctx, MODELVIEW);
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		context_dirty(ctx, MODELVIEW);
		context_dirty(ctx, LIGHT_ENABLE);
		context_dirty_i(ctx, LIGHT_SOURCE, cap - GL_LIGHT0);
		context_dirty(ctx, MATERIAL_FRONT_AMBIENT);
		context_dirty(ctx, MATERIAL_BACK_AMBIENT);
		context_dirty(ctx, MATERIAL_FRONT_DIFFUSE);
		context_dirty(ctx, MATERIAL_BACK_DIFFUSE);
		context_dirty(ctx, MATERIAL_FRONT_SPECULAR);
		context_dirty(ctx, MATERIAL_BACK_SPECULAR);
		context_dirty(ctx, MATERIAL_FRONT_SHININESS);
		context_dirty(ctx, MATERIAL_BACK_SHININESS);
		break;
	case GL_LIGHTING:
		context_dirty(ctx, FRAG);
		context_dirty(ctx, MODELVIEW);
		context_dirty(ctx, LIGHT_ENABLE);

		for (i = 0; i < MAX_LIGHTS; i++) {
			if (ctx->Light.Light[i].Enabled)
				context_dirty_i(ctx, LIGHT_SOURCE, i);
		}

		context_dirty(ctx, MATERIAL_FRONT_AMBIENT);
		context_dirty(ctx, MATERIAL_BACK_AMBIENT);
		context_dirty(ctx, MATERIAL_FRONT_DIFFUSE);
		context_dirty(ctx, MATERIAL_BACK_DIFFUSE);
		context_dirty(ctx, MATERIAL_FRONT_SPECULAR);
		context_dirty(ctx, MATERIAL_BACK_SPECULAR);
		context_dirty(ctx, MATERIAL_FRONT_SHININESS);
		context_dirty(ctx, MATERIAL_BACK_SHININESS);
		break;
	case GL_LINE_SMOOTH:
		context_dirty(ctx, LINE_MODE);
		break;
	case GL_NORMALIZE:
		context_dirty(ctx, LIGHT_ENABLE);
		break;
	case GL_POINT_SMOOTH:
		context_dirty(ctx, POINT_MODE);
		break;
	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
	case GL_POLYGON_OFFSET_FILL:
		context_dirty(ctx, POLYGON_OFFSET);
		break;
	case GL_POLYGON_SMOOTH:
		context_dirty(ctx, POLYGON_MODE);
		break;
	case GL_SCISSOR_TEST:
		context_dirty(ctx, SCISSOR);
		break;
	case GL_STENCIL_TEST:
		context_dirty(ctx, STENCIL_FUNC);
		break;
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D:
		context_dirty_i(ctx, TEX_ENV, ctx->Texture.CurrentUnit);
		context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
		break;
	}
}

static void
nouveau_fog(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	context_dirty(ctx, FOG);
}

static void
nouveau_light(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params)
{
	switch (pname) {
	case GL_AMBIENT:
		context_dirty(ctx, MATERIAL_FRONT_AMBIENT);
		context_dirty(ctx, MATERIAL_BACK_AMBIENT);
		break;
	case GL_DIFFUSE:
		context_dirty(ctx, MATERIAL_FRONT_DIFFUSE);
		context_dirty(ctx, MATERIAL_BACK_DIFFUSE);
		break;
	case GL_SPECULAR:
		context_dirty(ctx, MATERIAL_FRONT_SPECULAR);
		context_dirty(ctx, MATERIAL_BACK_SPECULAR);
		break;
	case GL_SPOT_CUTOFF:
	case GL_POSITION:
		context_dirty(ctx, MODELVIEW);
		context_dirty(ctx, LIGHT_ENABLE);
		context_dirty_i(ctx, LIGHT_SOURCE, light - GL_LIGHT0);
		break;
	default:
		context_dirty_i(ctx, LIGHT_SOURCE, light - GL_LIGHT0);
		break;
	}
}

static void
nouveau_light_model(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	context_dirty(ctx, LIGHT_MODEL);
	context_dirty(ctx, MODELVIEW);
}

static void
nouveau_line_stipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
	context_dirty(ctx, LINE_STIPPLE);
}

static void
nouveau_line_width(GLcontext *ctx, GLfloat width)
{
	context_dirty(ctx, LINE_MODE);
}

static void
nouveau_logic_opcode(GLcontext *ctx, GLenum opcode)
{
	context_dirty(ctx, LOGIC_OPCODE);
}

static void
nouveau_point_parameter(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	context_dirty(ctx, POINT_PARAMETER);
}

static void
nouveau_point_size(GLcontext *ctx, GLfloat size)
{
	context_dirty(ctx, POINT_MODE);
}

static void
nouveau_polygon_mode(GLcontext *ctx, GLenum face, GLenum mode)
{
	context_dirty(ctx, POLYGON_MODE);
}

static void
nouveau_polygon_offset(GLcontext *ctx, GLfloat factor, GLfloat units)
{
	context_dirty(ctx, POLYGON_OFFSET);
}

static void
nouveau_polygon_stipple(GLcontext *ctx, const GLubyte *mask)
{
	context_dirty(ctx, POLYGON_STIPPLE);
}

static void
nouveau_render_mode(GLcontext *ctx, GLenum mode)
{
	context_dirty(ctx, RENDER_MODE);
}

static void
nouveau_scissor(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	context_dirty(ctx, SCISSOR);
}

static void
nouveau_shade_model(GLcontext *ctx, GLenum mode)
{
	context_dirty(ctx, SHADE_MODEL);
}

static void
nouveau_stencil_func_separate(GLcontext *ctx, GLenum face, GLenum func,
			      GLint ref, GLuint mask)
{
	context_dirty(ctx, STENCIL_FUNC);
}

static void
nouveau_stencil_mask_separate(GLcontext *ctx, GLenum face, GLuint mask)
{
	context_dirty(ctx, STENCIL_MASK);
}

static void
nouveau_stencil_op_separate(GLcontext *ctx, GLenum face, GLenum fail,
			    GLenum zfail, GLenum zpass)
{
	context_dirty(ctx, STENCIL_OP);
}

static void
nouveau_tex_gen(GLcontext *ctx, GLenum coord, GLenum pname,
		const GLfloat *params)
{
	context_dirty_i(ctx, TEX_GEN, ctx->Texture.CurrentUnit);
}

static void
nouveau_tex_env(GLcontext *ctx, GLenum target, GLenum pname,
		const GLfloat *param)
{
	switch (target) {
	case GL_TEXTURE_FILTER_CONTROL_EXT:
		context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
		break;
	default:
		context_dirty_i(ctx, TEX_ENV, ctx->Texture.CurrentUnit);
		break;
	}
}

static void
nouveau_tex_parameter(GLcontext *ctx, GLenum target,
		      struct gl_texture_object *t, GLenum pname,
		      const GLfloat *params)
{
	switch (pname) {
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
	case GL_TEXTURE_MIN_LOD:
	case GL_TEXTURE_MAX_LOD:
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
	case GL_TEXTURE_LOD_BIAS:
		context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
		break;

	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_BASE_LEVEL:
	case GL_TEXTURE_MAX_LEVEL:
		nouveau_texture_reallocate(ctx, t);
		context_dirty_i(ctx, TEX_OBJ, ctx->Texture.CurrentUnit);
		break;
	}
}

static void
nouveau_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	context_dirty(ctx, VIEWPORT);
}

void
nouveau_emit_nothing(GLcontext *ctx, int emit)
{
}

int
nouveau_next_dirty_state(GLcontext *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	int i = BITSET_FFS(nctx->dirty) - 1;

	if (i < 0 || i >= context_drv(ctx)->num_emit)
		return -1;

	return i;
}

void
nouveau_state_emit(GLcontext *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	const struct nouveau_driver *drv = context_drv(ctx);
	int i;

	while ((i = nouveau_next_dirty_state(ctx)) >= 0) {
		BITSET_CLEAR(nctx->dirty, i);
		drv->emit[i](ctx, i);
	}

	BITSET_ZERO(nctx->dirty);

	nouveau_bo_state_emit(ctx);
}

static void
nouveau_update_state(GLcontext *ctx, GLbitfield new_state)
{
	if (new_state & (_NEW_PROJECTION | _NEW_MODELVIEW))
		context_dirty(ctx, PROJECTION);

	if (new_state & _NEW_MODELVIEW)
		context_dirty(ctx, MODELVIEW);

	if (new_state & _NEW_CURRENT_ATTRIB &&
	    new_state & _NEW_LIGHT) {
		context_dirty(ctx, MATERIAL_FRONT_AMBIENT);
		context_dirty(ctx, MATERIAL_BACK_AMBIENT);
		context_dirty(ctx, MATERIAL_FRONT_DIFFUSE);
		context_dirty(ctx, MATERIAL_BACK_DIFFUSE);
		context_dirty(ctx, MATERIAL_FRONT_SPECULAR);
		context_dirty(ctx, MATERIAL_BACK_SPECULAR);
		context_dirty(ctx, MATERIAL_FRONT_SHININESS);
		context_dirty(ctx, MATERIAL_BACK_SHININESS);
	}

	_swrast_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);

	nouveau_state_emit(ctx);
}

void
nouveau_state_init(GLcontext *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	ctx->Driver.AlphaFunc = nouveau_alpha_func;
	ctx->Driver.BlendColor = nouveau_blend_color;
	ctx->Driver.BlendEquationSeparate = nouveau_blend_equation_separate;
	ctx->Driver.BlendFuncSeparate = nouveau_blend_func_separate;
	ctx->Driver.ClipPlane = nouveau_clip_plane;
	ctx->Driver.ColorMask = nouveau_color_mask;
	ctx->Driver.ColorMaterial = nouveau_color_material;
	ctx->Driver.CullFace = nouveau_cull_face;
	ctx->Driver.FrontFace = nouveau_front_face;
	ctx->Driver.DepthFunc = nouveau_depth_func;
	ctx->Driver.DepthMask = nouveau_depth_mask;
	ctx->Driver.DepthRange = nouveau_depth_range;
	ctx->Driver.DrawBuffer = nouveau_draw_buffer;
	ctx->Driver.DrawBuffers = nouveau_draw_buffers;
	ctx->Driver.Enable = nouveau_enable;
	ctx->Driver.Fogfv = nouveau_fog;
	ctx->Driver.Lightfv = nouveau_light;
	ctx->Driver.LightModelfv = nouveau_light_model;
	ctx->Driver.LineStipple = nouveau_line_stipple;
	ctx->Driver.LineWidth = nouveau_line_width;
	ctx->Driver.LogicOpcode = nouveau_logic_opcode;
	ctx->Driver.PointParameterfv = nouveau_point_parameter;
	ctx->Driver.PointSize = nouveau_point_size;
	ctx->Driver.PolygonMode = nouveau_polygon_mode;
	ctx->Driver.PolygonOffset = nouveau_polygon_offset;
	ctx->Driver.PolygonStipple = nouveau_polygon_stipple;
	ctx->Driver.RenderMode = nouveau_render_mode;
	ctx->Driver.Scissor = nouveau_scissor;
	ctx->Driver.ShadeModel = nouveau_shade_model;
	ctx->Driver.StencilFuncSeparate = nouveau_stencil_func_separate;
	ctx->Driver.StencilMaskSeparate = nouveau_stencil_mask_separate;
	ctx->Driver.StencilOpSeparate = nouveau_stencil_op_separate;
	ctx->Driver.TexGen = nouveau_tex_gen;
	ctx->Driver.TexEnv = nouveau_tex_env;
	ctx->Driver.TexParameter = nouveau_tex_parameter;
	ctx->Driver.Viewport = nouveau_viewport;

	ctx->Driver.UpdateState = nouveau_update_state;

	BITSET_ONES(nctx->dirty);
}
