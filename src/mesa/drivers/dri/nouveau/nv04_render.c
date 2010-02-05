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
#include "nouveau_util.h"
#include "nouveau_class.h"
#include "nv04_driver.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#define NUM_VERTEX_ATTRS 6

static void
swtnl_update_viewport(GLcontext *ctx)
{
	float *viewport = to_nv04_context(ctx)->viewport;
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	get_viewport_scale(ctx, viewport);
	get_viewport_translate(ctx, &viewport[MAT_TX]);

	/* It wants normalized Z coordinates. */
	viewport[MAT_SZ] /= fb->_DepthMaxF;
	viewport[MAT_TZ] /= fb->_DepthMaxF;
}

static void
swtnl_emit_attr(GLcontext *ctx, struct tnl_attr_map *m, int attr, int emit)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, attr))
		*m = (struct tnl_attr_map) {
			.attrib = attr,
			.format = emit,
		};
	else
		*m = (struct tnl_attr_map) {
			.format = EMIT_PAD,
			.offset = _tnl_format_info[emit].attrsize,
		};
}

static void
swtnl_choose_attrs(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct nouveau_grobj *fahrenheit = nv04_context_engine(ctx);
	struct nv04_context *nctx = to_nv04_context(ctx);
	static struct tnl_attr_map map[NUM_VERTEX_ATTRS];
	int n = 0;

	tnl->vb.AttribPtr[VERT_ATTRIB_POS] = tnl->vb.NdcPtr;

	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_FOG, EMIT_1UB_1F);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_TEX0, EMIT_2F);
	if (nv04_mtex_engine(fahrenheit))
		swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_TEX1, EMIT_2F);

	swtnl_update_viewport(ctx);

	_tnl_install_attrs(ctx, map, n, nctx->viewport, 0);
}

/* TnL renderer entry points */

static void
swtnl_start(GLcontext *ctx)
{
	swtnl_choose_attrs(ctx);
}

static void
swtnl_finish(GLcontext *ctx)
{
	FIRE_RING(context_chan(ctx));
}

static void
swtnl_primitive(GLcontext *ctx, GLenum mode)
{
}

static void
swtnl_reset_stipple(GLcontext *ctx)
{
}

/* Primitive rendering */

#define BEGIN_PRIMITIVE(n)						\
	struct nouveau_channel *chan = context_chan(ctx);		\
	struct nouveau_grobj *fahrenheit = nv04_context_engine(ctx);	\
	int vertex_len = TNL_CONTEXT(ctx)->clipspace.vertex_size / 4;	\
									\
	if (nv04_mtex_engine(fahrenheit))				\
		BEGIN_RING(chan, fahrenheit,				\
			   NV04_MULTITEX_TRIANGLE_TLMTVERTEX_SX(0),	\
			   n * vertex_len);				\
	else								\
		BEGIN_RING(chan, fahrenheit,				\
			   NV04_TEXTURED_TRIANGLE_TLVERTEX_SX(0),	\
			   n * vertex_len);				\

#define OUT_VERTEX(i)						\
	OUT_RINGp(chan, _tnl_get_vertex(ctx, i), vertex_len);

#define END_PRIMITIVE(draw)						\
	if (nv04_mtex_engine(fahrenheit)) {				\
		BEGIN_RING(chan, fahrenheit,				\
			   NV04_MULTITEX_TRIANGLE_DRAWPRIMITIVE(0), 1); \
		OUT_RING(chan, draw);					\
	} else {							\
		BEGIN_RING(chan, fahrenheit,				\
			   NV04_TEXTURED_TRIANGLE_DRAWPRIMITIVE(0), 1); \
		OUT_RING(chan, draw);					\
	}

static void
swtnl_points(GLcontext *ctx, GLuint first, GLuint last)
{
}

static void
swtnl_line(GLcontext *ctx, GLuint v1, GLuint v2)
{
}

static void
swtnl_triangle(GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3)
{
	BEGIN_PRIMITIVE(3);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
	OUT_VERTEX(v3);
	END_PRIMITIVE(0x210);
}

static void
swtnl_quad(GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4)
{
	BEGIN_PRIMITIVE(4);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
	OUT_VERTEX(v3);
	OUT_VERTEX(v4);
	END_PRIMITIVE(0x320210);
}

/* TnL initialization. */
void
nv04_render_init(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	tnl->Driver.RunPipeline = _tnl_run_pipeline;
	tnl->Driver.Render.Interp = _tnl_interp;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
	tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;

	tnl->Driver.Render.Start = swtnl_start;
	tnl->Driver.Render.Finish = swtnl_finish;
	tnl->Driver.Render.PrimitiveNotify = swtnl_primitive;
	tnl->Driver.Render.ResetLineStipple = swtnl_reset_stipple;

	tnl->Driver.Render.Points = swtnl_points;
	tnl->Driver.Render.Line = swtnl_line;
	tnl->Driver.Render.Triangle = swtnl_triangle;
	tnl->Driver.Render.Quad = swtnl_quad;

	_tnl_need_projected_coords(ctx, GL_TRUE);
	_tnl_init_vertices(ctx, tnl->vb.Size,
			   NUM_VERTEX_ATTRS * 4 * sizeof(GLfloat));
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_wakeup(ctx);
}

void
nv04_render_destroy(GLcontext *ctx)
{
}
