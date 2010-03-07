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

#include "nouveau_bufferobj.h"
#include "nouveau_util.h"

#include "main/bufferobj.h"
#include "main/image.h"

/* Arbitrary pushbuf length we can assume we can get with a single
 * WAIT_RING. */
#define PUSHBUF_DWORDS 2048

/* Functions to set up struct nouveau_array_state from something like
 * a GL array or index buffer. */

static void
vbo_init_array(struct nouveau_array_state *a, int attr, int stride,
	       int fields, int type, struct gl_buffer_object *obj,
	       const void *ptr, GLboolean map)
{
	a->attr = attr;
	a->stride = stride;
	a->fields = fields;
	a->type = type;

	if (_mesa_is_bufferobj(obj)) {
		nouveau_bo_ref(to_nouveau_bufferobj(obj)->bo, &a->bo);
		a->offset = (intptr_t)ptr;

		if (map) {
			nouveau_bo_map(a->bo, NOUVEAU_BO_RD);
			a->buf = a->bo->map + a->offset;
		} else {
			a->buf = NULL;
		}

	} else {
		nouveau_bo_ref(NULL, &a->bo);
		a->offset = 0;

		if (map)
			a->buf = ptr;
		else
			a->buf = NULL;
	}

	if (a->buf)
		get_array_extract(a, &a->extract_u, &a->extract_f);
}

static void
vbo_deinit_array(struct nouveau_array_state *a)
{
	if (a->bo) {
		if (a->bo->map)
			nouveau_bo_unmap(a->bo);
		nouveau_bo_ref(NULL, &a->bo);
	}

	a->buf = NULL;
	a->fields = 0;
}

static void
vbo_init_arrays(GLcontext *ctx, const struct _mesa_index_buffer *ib,
		const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	if (ib)
		vbo_init_array(&render->ib, 0, 0, ib->count, ib->type,
			       ib->obj, ib->ptr, GL_TRUE);

	for (i = 0; i < render->attr_count; i++) {
		int attr = render->map[i];

		if (attr >= 0) {
			const struct gl_client_array *array = arrays[attr];
			int stride;

			if (render->mode == VBO &&
			    !_mesa_is_bufferobj(array->BufferObj))
				/* Pack client buffers. */
				stride = align(_mesa_sizeof_type(array->Type)
					       * array->Size, 4);
			else
				stride = array->StrideB;

			vbo_init_array(&render->attrs[attr], attr,
				       stride, array->Size, array->Type,
				       array->BufferObj, array->Ptr,
				       render->mode == IMM);
		}
	}
}

static void
vbo_deinit_arrays(GLcontext *ctx, const struct _mesa_index_buffer *ib,
		const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	if (ib)
		vbo_deinit_array(&render->ib);

	for (i = 0; i < render->attr_count; i++) {
		int *attr = &render->map[i];

		if (*attr >= 0) {
			vbo_deinit_array(&render->attrs[*attr]);
			*attr = -1;
		}
	}

	render->attr_count = 0;
}

/* Make some rendering decisions from the GL context. */

static void
vbo_choose_render_mode(GLcontext *ctx, const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	render->mode = VBO;

	if (ctx->Light.Enabled) {
		for (i = 0; i < MAT_ATTRIB_MAX; i++) {
			if (arrays[VERT_ATTRIB_GENERIC0 + i]->StrideB) {
				render->mode = IMM;
				break;
			}
		}
	}

	if (render->mode == VBO)
		render->attr_count = NUM_VERTEX_ATTRS;
	else
		render->attr_count = 0;
}

static void
vbo_emit_attr(GLcontext *ctx, const struct gl_client_array **arrays, int attr)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_render_state *render = to_render_state(ctx);
	const struct gl_client_array *array = arrays[attr];
	struct nouveau_array_state *a = &render->attrs[attr];
	RENDER_LOCALS(ctx);

	if (!array->StrideB) {
		if (attr >= VERT_ATTRIB_GENERIC0)
			/* nouveau_update_state takes care of materials. */
			return;

		/* Constant attribute. */
		vbo_init_array(a, attr, array->StrideB, array->Size,
			       array->Type, array->BufferObj, array->Ptr,
			       GL_TRUE);
		EMIT_IMM(ctx, a, 0);
		vbo_deinit_array(a);

	} else {
		/* Varying attribute. */
		struct nouveau_attr_info *info = &TAG(vertex_attrs)[attr];

		if (render->mode == VBO) {
			render->map[info->vbo_index] = attr;
			render->vertex_size += array->_ElementSize;
		} else {
			render->map[render->attr_count++] = attr;
			render->vertex_size += 4 * info->imm_fields;
		}
	}
}

#define MAT(a) (VERT_ATTRIB_GENERIC0 + MAT_ATTRIB_##a)

static void
vbo_choose_attrs(GLcontext *ctx, const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	/* Reset the vertex size. */
	render->vertex_size = 0;

	vbo_emit_attr(ctx, arrays, VERT_ATTRIB_COLOR0);
	if (ctx->Fog.ColorSumEnabled && !ctx->Light.Enabled)
		vbo_emit_attr(ctx, arrays, VERT_ATTRIB_COLOR1);

	for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
		if (ctx->Texture._EnabledCoordUnits & (1 << i))
			vbo_emit_attr(ctx, arrays, VERT_ATTRIB_TEX0 + i);
	}

	if (ctx->Fog.Enabled && ctx->Fog.FogCoordinateSource == GL_FOG_COORD)
		vbo_emit_attr(ctx, arrays, VERT_ATTRIB_FOG);

	if (ctx->Light.Enabled) {
		vbo_emit_attr(ctx, arrays, VERT_ATTRIB_NORMAL);

		vbo_emit_attr(ctx, arrays, MAT(FRONT_AMBIENT));
		vbo_emit_attr(ctx, arrays, MAT(FRONT_DIFFUSE));
		vbo_emit_attr(ctx, arrays, MAT(FRONT_SPECULAR));
		vbo_emit_attr(ctx, arrays, MAT(FRONT_SHININESS));

		if (ctx->Light.Model.TwoSide) {
			vbo_emit_attr(ctx, arrays, MAT(BACK_AMBIENT));
			vbo_emit_attr(ctx, arrays, MAT(BACK_DIFFUSE));
			vbo_emit_attr(ctx, arrays, MAT(BACK_SPECULAR));
			vbo_emit_attr(ctx, arrays, MAT(BACK_SHININESS));
		}
	}

	vbo_emit_attr(ctx, arrays, VERT_ATTRIB_POS);
}

static unsigned
get_max_client_stride(GLcontext *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i, s = 0;

	for (i = 0; i < render->attr_count; i++) {
		int attr = render->map[i];
		struct nouveau_array_state *a = &render->attrs[attr];

		if (attr >= 0 && !a->bo)
			s = MAX2(a->stride, s);
	}

	return s;
}

static void
TAG(vbo_render_prims)(GLcontext *ctx, const struct gl_client_array **arrays,
		      const struct _mesa_prim *prims, GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLboolean index_bounds_valid,
		      GLuint min_index, GLuint max_index);

static GLboolean
vbo_maybe_split(GLcontext *ctx, const struct gl_client_array **arrays,
	    const struct _mesa_prim *prims, GLuint nr_prims,
	    const struct _mesa_index_buffer *ib,
	    GLuint min_index, GLuint max_index)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_render_state *render = to_render_state(ctx);
	unsigned pushbuf_avail = PUSHBUF_DWORDS - 2 * nctx->bo.count,
		vert_avail = get_max_vertices(ctx, NULL, pushbuf_avail),
		idx_avail = get_max_vertices(ctx, ib, pushbuf_avail);
	int stride;

	/* Try to keep client buffers smaller than the scratch BOs. */
	if (!ib && render->mode == VBO &&
	    (stride = get_max_client_stride(ctx)))
		    vert_avail = MIN2(vert_avail,
				      RENDER_SCRATCH_SIZE / stride);


	if ((ib && ib->count > idx_avail) ||
	    (!ib && max_index - min_index > vert_avail)) {
		struct split_limits limits = {
			.max_verts = vert_avail,
			.max_indices = idx_avail,
			.max_vb_size = ~0,
		};

		vbo_split_prims(ctx, arrays, prims, nr_prims, ib, min_index,
				max_index, TAG(vbo_render_prims), &limits);
		return GL_TRUE;
	}

	return GL_FALSE;
}

/* VBO rendering path. */

static void
vbo_bind_vertices(GLcontext *ctx, const struct gl_client_array **arrays,
		  GLint basevertex, GLuint min_index, GLuint max_index)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	for (i = 0; i < NUM_VERTEX_ATTRS; i++) {
		int attr = render->map[i];

		if (attr >= 0) {
			const struct gl_client_array *array = arrays[attr];
			struct nouveau_array_state *a = &render->attrs[attr];
			unsigned delta = (basevertex + min_index)
				* array->StrideB;

			if (a->bo) {
				a->offset = (intptr_t)array->Ptr + delta;
			} else {
				int j, n = max_index - min_index + 1;
				char *sp = (char *)array->Ptr + delta;
				char *dp = get_scratch_vbo(ctx, n * a->stride,
							   &a->bo, &a->offset);

				for (j = 0; j < n; j++)
					memcpy(dp + j * a->stride,
					       sp + j * array->StrideB,
					       a->stride);
			}
		}
	}

	TAG(render_bind_vertices)(ctx);
}

static void
vbo_draw_vbo(GLcontext *ctx, const struct gl_client_array **arrays,
	     const struct _mesa_prim *prims, GLuint nr_prims,
	     const struct _mesa_index_buffer *ib, GLuint min_index,
	     GLuint max_index)
{
	struct nouveau_channel *chan = context_chan(ctx);
	dispatch_t dispatch;
	int delta = -min_index, basevertex = 0, i;
	RENDER_LOCALS(ctx);

	get_array_dispatch(&to_render_state(ctx)->ib, &dispatch);

	TAG(render_set_format)(ctx);

	for (i = 0; i < nr_prims; i++) {
		unsigned start = prims[i].start,
			count = prims[i].count;

		if (i == 0 || basevertex != prims[i].basevertex) {
			basevertex = prims[i].basevertex;
			vbo_bind_vertices(ctx, arrays, basevertex,
					  min_index, max_index);
		}

		if (count > get_max_vertices(ctx, ib, AVAIL_RING(chan)))
			WAIT_RING(chan, PUSHBUF_DWORDS);

		BATCH_BEGIN(nvgl_primitive(prims[i].mode));
		dispatch(ctx, start, delta, count);
		BATCH_END();
	}

	FIRE_RING(chan);
}

/* Immediate rendering path. */

static unsigned
extract_id(struct nouveau_array_state *a, int i, int j)
{
	return j;
}

static void
vbo_draw_imm(GLcontext *ctx, const struct gl_client_array **arrays,
	     const struct _mesa_prim *prims, GLuint nr_prims,
	     const struct _mesa_index_buffer *ib, GLuint min_index,
	     GLuint max_index)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_channel *chan = context_chan(ctx);
	extract_u_t extract = ib ? render->ib.extract_u : extract_id;
	int i, j, k;
	RENDER_LOCALS(ctx);

	for (i = 0; i < nr_prims; i++) {
		unsigned start = prims[i].start,
			end = start + prims[i].count;

		if (prims[i].count > get_max_vertices(ctx, ib,
						      AVAIL_RING(chan)))
			WAIT_RING(chan, PUSHBUF_DWORDS);

		BATCH_BEGIN(nvgl_primitive(prims[i].mode));

		for (; start < end; start++) {
			j = prims[i].basevertex +
				extract(&render->ib, 0, start);

			for (k = 0; k < render->attr_count; k++)
				EMIT_IMM(ctx, &render->attrs[render->map[k]],
					 j);
		}

		BATCH_END();
	}

	FIRE_RING(chan);
}

/* draw_prims entry point when we're doing hw-tnl. */

static void
TAG(vbo_render_prims)(GLcontext *ctx, const struct gl_client_array **arrays,
		      const struct _mesa_prim *prims, GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLboolean index_bounds_valid,
		      GLuint min_index, GLuint max_index)
{
	struct nouveau_render_state *render = to_render_state(ctx);

	if (!index_bounds_valid)
		vbo_get_minmax_index(ctx, prims, ib, &min_index, &max_index);

	vbo_choose_render_mode(ctx, arrays);
	vbo_choose_attrs(ctx, arrays);

	if (vbo_maybe_split(ctx, arrays, prims, nr_prims, ib, min_index,
			    max_index))
		return;

	vbo_init_arrays(ctx, ib, arrays);

	if (render->mode == VBO)
		vbo_draw_vbo(ctx, arrays, prims, nr_prims, ib, min_index,
			     max_index);
	else
		vbo_draw_imm(ctx, arrays, prims, nr_prims, ib, min_index,
			     max_index);

	vbo_deinit_arrays(ctx, ib, arrays);
}
