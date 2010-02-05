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

/*
 * Vertex submission helper definitions shared among the software and
 * hardware TnL paths.
 */

#include "nouveau_gldefs.h"

#include "main/light.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"

#define OUT_INDICES_L(r, i, d, n)		\
	BATCH_OUT_L(i + d, n);			\
	(void)r
#define OUT_INDICES_I16(r, i, d, n)				\
	BATCH_OUT_I16(r->ib.extract_u(&r->ib, 0, i) + d,	\
		      r->ib.extract_u(&r->ib, 0, i + 1) + d)
#define OUT_INDICES_I32(r, i, d, n)			\
	BATCH_OUT_I32(r->ib.extract_u(&r->ib, 0, i) + d)

/*
 * Emit <n> vertices using BATCH_OUT_<out>, MAX_OUT_<out> at a time,
 * grouping them in packets of length MAX_PACKET.
 *
 * out:   hardware index data type.
 * ctx:   GL context.
 * start: element within the index buffer to begin with.
 * delta: integer correction that will be added to each index found in
 *        the index buffer.
 */
#define EMIT_VBO(out, ctx, start, delta, n) do {			\
		struct nouveau_render_state *render = to_render_state(ctx); \
		int npush = n;						\
									\
		while (npush) {						\
			int npack = MIN2(npush, MAX_PACKET * MAX_OUT_##out); \
			npush -= npack;					\
									\
			BATCH_PACKET_##out((npack + MAX_OUT_##out - 1)	\
					   / MAX_OUT_##out);		\
			while (npack) {					\
				int nout = MIN2(npack, MAX_OUT_##out);	\
				npack -= nout;				\
									\
				OUT_INDICES_##out(render, start, delta, \
						  nout);		\
				start += nout;				\
			}						\
		}							\
	} while (0)

/*
 * Emit the <n>-th element of the array <a>, using IMM_OUT.
 */
#define EMIT_IMM(ctx, a, n) do {					\
		struct nouveau_attr_info *info =			\
			&TAG(vertex_attrs)[(a)->attr];			\
		int m;							\
									\
		if (!info->emit) {					\
			IMM_PACKET(info->imm_method, info->imm_fields);	\
									\
			for (m = 0; m < (a)->fields; m++)		\
				IMM_OUT((a)->extract_f(a, n, m));	\
									\
			for (m = (a)->fields; m < info->imm_fields; m++) \
				IMM_OUT(((float []){0, 0, 0, 1})[m]);	\
									\
		} else {						\
			info->emit(ctx, a, (a)->buf + n * (a)->stride);	\
		}							\
	} while (0)

/*
 * Select an appropriate dispatch function for the given index buffer.
 */
static void
get_array_dispatch(struct nouveau_array_state *a, dispatch_t *dispatch)
{
	if (!a->fields) {
		auto void f(GLcontext *, unsigned int, int, unsigned int);

		void f(GLcontext *ctx, unsigned int start, int delta,
		       unsigned int n) {
			struct nouveau_channel *chan = context_chan(ctx);
			RENDER_LOCALS(ctx);

			EMIT_VBO(L, ctx, start, delta, n);
		};

		*dispatch = f;

	} else if (a->type == GL_UNSIGNED_INT) {
		auto void f(GLcontext *, unsigned int, int, unsigned int);

		void f(GLcontext *ctx, unsigned int start, int delta,
		       unsigned int n) {
			struct nouveau_channel *chan = context_chan(ctx);
			RENDER_LOCALS(ctx);

			EMIT_VBO(I32, ctx, start, delta, n);
		};

		*dispatch = f;

	} else {
		auto void f(GLcontext *, unsigned int, int, unsigned int);

		void f(GLcontext *ctx, unsigned int start, int delta,
		       unsigned int n) {
			struct nouveau_channel *chan = context_chan(ctx);
			RENDER_LOCALS(ctx);

			EMIT_VBO(I32, ctx, start, delta, n & 1);
			EMIT_VBO(I16, ctx, start, delta, n & ~1);
		};

		*dispatch = f;
	}
}

/*
 * Select appropriate element extraction functions for the given
 * array.
 */
static void
get_array_extract(struct nouveau_array_state *a,
		  extract_u_t *extract_u, extract_f_t *extract_f)
{
#define EXTRACT(in_t, out_t, k)						\
	({								\
		auto out_t f(struct nouveau_array_state *, int, int);	\
		out_t f(struct nouveau_array_state *a, int i, int j) {	\
			in_t x = ((in_t *)(a->buf + i * a->stride))[j];	\
									\
			return (out_t)x / (k);				\
		};							\
		f;							\
	});

	switch (a->type) {
	case GL_BYTE:
		*extract_u = EXTRACT(char, unsigned, 1);
		*extract_f = EXTRACT(char, float, SCHAR_MAX);
		break;
	case GL_UNSIGNED_BYTE:
		*extract_u = EXTRACT(unsigned char, unsigned, 1);
		*extract_f = EXTRACT(unsigned char, float, UCHAR_MAX);
		break;
	case GL_SHORT:
		*extract_u = EXTRACT(short, unsigned, 1);
		*extract_f = EXTRACT(short, float, SHRT_MAX);
		break;
	case GL_UNSIGNED_SHORT:
		*extract_u = EXTRACT(unsigned short, unsigned, 1);
		*extract_f = EXTRACT(unsigned short, float, USHRT_MAX);
		break;
	case GL_INT:
		*extract_u = EXTRACT(int, unsigned, 1);
		*extract_f = EXTRACT(int, float, INT_MAX);
		break;
	case GL_UNSIGNED_INT:
		*extract_u = EXTRACT(unsigned int, unsigned, 1);
		*extract_f = EXTRACT(unsigned int, float, UINT_MAX);
		break;
	case GL_FLOAT:
		*extract_u = EXTRACT(float, unsigned, 1.0 / UINT_MAX);
		*extract_f = EXTRACT(float, float, 1);
		break;

	default:
		assert(0);
	}
}

/*
 * Returns a pointer to a chunk of <size> bytes long GART memory. <bo>
 * will be updated with the buffer object the memory is located in.
 *
 * If <offset> is provided, it will be updated with the offset within
 * <bo> of the allocated memory. Otherwise the returned memory will
 * always be located right at the beginning of <bo>.
 */
static inline void *
get_scratch_vbo(GLcontext *ctx, unsigned size, struct nouveau_bo **bo,
		unsigned *offset)
{
	struct nouveau_scratch_state *scratch = &to_render_state(ctx)->scratch;
	void *buf;

	if (scratch->buf && offset &&
	    size <= RENDER_SCRATCH_SIZE - scratch->offset) {
		nouveau_bo_ref(scratch->bo[scratch->index], bo);

		buf = scratch->buf + scratch->offset;
		*offset = scratch->offset;
		scratch->offset += size;

	} else if (size <= RENDER_SCRATCH_SIZE) {
		scratch->index = (scratch->index + 1) % RENDER_SCRATCH_COUNT;
		nouveau_bo_ref(scratch->bo[scratch->index], bo);

		nouveau_bo_map(*bo, NOUVEAU_BO_WR);
		buf = scratch->buf = (*bo)->map;
		nouveau_bo_unmap(*bo);

		if (offset)
			*offset = 0;
		scratch->offset = size;

	} else {
		nouveau_bo_new(context_dev(ctx),
			       NOUVEAU_BO_MAP | NOUVEAU_BO_GART, 0, size, bo);

		nouveau_bo_map(*bo, NOUVEAU_BO_WR);
		buf = (*bo)->map;
		nouveau_bo_unmap(*bo);

		if (offset)
			*offset = 0;
	}

	return buf;
}

/*
 * Returns how many vertices you can draw using <n> pushbuf dwords.
 */
static inline unsigned
get_max_vertices(GLcontext *ctx, const struct _mesa_index_buffer *ib,
		 unsigned n)
{
	struct nouveau_render_state *render = to_render_state(ctx);

	if (render->mode == IMM) {
		return MAX2(0, n - 4) / (render->vertex_size / 4 +
					 render->attr_count);
	} else {
		unsigned max_out;

		if (ib) {
			switch (ib->type) {
			case GL_UNSIGNED_INT:
				max_out = MAX_OUT_I32;
				break;

			case GL_UNSIGNED_SHORT:
				max_out = MAX_OUT_I16;
				break;

			case GL_UNSIGNED_BYTE:
				max_out = MAX_OUT_I16;
				break;
			}
		} else {
			max_out = MAX_OUT_L;
		}

		return MAX2(0, n - 7) * max_out * MAX_PACKET / (1 + MAX_PACKET);
	}
}

#include "nouveau_vbo_t.c"
#include "nouveau_swtnl_t.c"

static void
TAG(emit_material)(GLcontext *ctx, struct nouveau_array_state *a,
		   const void *v)
{
	const int attr = a->attr - VERT_ATTRIB_GENERIC0;
	const int state = ((int []) {
				NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT,
				NOUVEAU_STATE_MATERIAL_BACK_AMBIENT,
				NOUVEAU_STATE_MATERIAL_FRONT_DIFFUSE,
				NOUVEAU_STATE_MATERIAL_BACK_DIFFUSE,
				NOUVEAU_STATE_MATERIAL_FRONT_SPECULAR,
				NOUVEAU_STATE_MATERIAL_BACK_SPECULAR,
				NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT,
				NOUVEAU_STATE_MATERIAL_BACK_AMBIENT,
				NOUVEAU_STATE_MATERIAL_FRONT_SHININESS,
				NOUVEAU_STATE_MATERIAL_BACK_SHININESS
			}) [attr];

	COPY_4V(ctx->Light.Material.Attrib[attr], (float *)v);
	_mesa_update_material(ctx, 1 << attr);

	context_drv(ctx)->emit[state](ctx, state);
}

static void
TAG(render_prims)(GLcontext *ctx, const struct gl_client_array **arrays,
		  const struct _mesa_prim *prims, GLuint nr_prims,
		  const struct _mesa_index_buffer *ib,
		  GLboolean index_bounds_valid,
		  GLuint min_index, GLuint max_index)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nouveau_validate_framebuffer(ctx);

	if (nctx->fallback == HWTNL)
		TAG(vbo_render_prims)(ctx, arrays, prims, nr_prims, ib,
				      index_bounds_valid, min_index, max_index);

	if (nctx->fallback == SWTNL)
		_tnl_vbo_draw_prims(ctx, arrays, prims, nr_prims, ib,
				    index_bounds_valid, min_index, max_index);
}

void
TAG(render_init)(GLcontext *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_scratch_state *scratch = &render->scratch;
	int ret, i;

	for (i = 0; i < RENDER_SCRATCH_COUNT; i++) {
		ret = nouveau_bo_new(context_dev(ctx),
				     NOUVEAU_BO_MAP | NOUVEAU_BO_GART,
				     0, RENDER_SCRATCH_SIZE, &scratch->bo[i]);
		assert(!ret);
	}

	for (i = 0; i < VERT_ATTRIB_MAX; i++)
		render->map[i] = -1;

	TAG(swtnl_init)(ctx);
	vbo_set_draw_func(ctx, TAG(render_prims));
}

void
TAG(render_destroy)(GLcontext *ctx)
{
	TAG(swtnl_destroy)(ctx);
}
