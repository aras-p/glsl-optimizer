/**************************************************************************
 *
 * Copyright 2009 Maciej Cencora
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHOR(S) AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <stdlib.h>

#include "main/glheader.h"
#include "main/context.h"
#include "main/state.h"
#include "main/api_validate.h"
#include "main/enums.h"

#include "r300_reg.h"
#include "r300_context.h"
#include "r300_emit.h"
#include "r300_render.h"
#include "r300_state.h"
#include "r300_tex.h"

#include "tnl/tnl.h"
#include "tnl/t_vp_build.h"
#include "vbo/vbo_context.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"

static void r300FixupIndexBuffer(GLcontext *ctx, const struct _mesa_index_buffer *mesa_ind_buf, struct gl_buffer_object **bo, GLuint *nr_bo)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_index_buffer *ind_buf = &r300->ind_buf;
	GLvoid *src_ptr;

	if (!mesa_ind_buf) {
		ind_buf->ptr = NULL;
		return;
	}

	ind_buf->count = mesa_ind_buf->count;
	if (mesa_ind_buf->obj->Name && !mesa_ind_buf->obj->Pointer) {
		bo[*nr_bo] = mesa_ind_buf->obj;
		(*nr_bo)++;
		ctx->Driver.MapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY_ARB, mesa_ind_buf->obj);
		assert(mesa_ind_buf->obj->Pointer != NULL);
	}
	src_ptr = ADD_POINTERS(mesa_ind_buf->obj->Pointer, mesa_ind_buf->ptr);

	if (mesa_ind_buf->type == GL_UNSIGNED_BYTE) {
		GLubyte *in = (GLubyte *)src_ptr;
		GLushort *out = _mesa_malloc(sizeof(GLushort) * mesa_ind_buf->count);
		int i;

		for (i = 0; i < mesa_ind_buf->count; ++i) {
			out[i] = (GLushort) in[i];
		}

		ind_buf->ptr = out;
		ind_buf->free_needed = GL_TRUE;
		ind_buf->is_32bit = GL_FALSE;
	} else if (mesa_ind_buf->type == GL_UNSIGNED_SHORT) {
		ind_buf->ptr = src_ptr;
		ind_buf->free_needed = GL_FALSE;
		ind_buf->is_32bit = GL_FALSE;
	} else {
		ind_buf->ptr = src_ptr;
		ind_buf->free_needed = GL_FALSE;
		ind_buf->is_32bit = GL_TRUE;
	}
}

static int getTypeSize(GLenum type)
{
	switch (type) {
		case GL_DOUBLE:
			return sizeof(GLdouble);
		case GL_FLOAT:
			return sizeof(GLfloat);
		case GL_INT:
			return sizeof(GLint);
		case GL_UNSIGNED_INT:
			return sizeof(GLuint);
		case GL_SHORT:
			return sizeof(GLshort);
		case GL_UNSIGNED_SHORT:
			return sizeof(GLushort);
		case GL_BYTE:
			return sizeof(GLbyte);
		case GL_UNSIGNED_BYTE:
			return sizeof(GLubyte);
		default:
			assert(0);
			return 0;
	}
}

#define CONVERT( TYPE, MACRO ) do {		\
	GLuint i, j, sz;				\
	sz = input->Size;				\
	if (input->Normalized) {			\
		for (i = 0; i < count; i++) {		\
			const TYPE *in = (TYPE *)src_ptr;		\
			for (j = 0; j < sz; j++) {		\
				*dst_ptr++ = MACRO(*in);		\
				in++;				\
			}					\
			src_ptr += input->StrideB;			\
		}						\
	} else {					\
		for (i = 0; i < count; i++) {		\
			const TYPE *in = (TYPE *)src_ptr;		\
			for (j = 0; j < sz; j++) {		\
				*dst_ptr++ = (GLfloat)(*in);		\
				in++;				\
			}					\
			src_ptr += input->StrideB;			\
		}						\
	}						\
} while (0)

static void r300TranslateAttrib(GLcontext *ctx, GLuint attr, int count, const struct gl_client_array *input, struct gl_buffer_object **bo, GLuint *nr_bo)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_buffer *vbuf = &r300->vbuf;
	struct vertex_attribute r300_attr;
	const void *src_ptr;
	GLenum type;

	if (input->BufferObj->Name) {
		if (!input->BufferObj->Pointer) {
			bo[*nr_bo] = input->BufferObj;
			(*nr_bo)++;
			ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER, GL_READ_ONLY_ARB, input->BufferObj);
			assert(input->BufferObj->Pointer != NULL);
		}

		src_ptr = ADD_POINTERS(input->BufferObj->Pointer, input->Ptr);
	} else
		src_ptr = input->Ptr;

	if (input->Type == GL_DOUBLE || input->Type == GL_UNSIGNED_INT || input->Type == GL_INT || input->StrideB < 4){
		if (RADEON_DEBUG & DEBUG_FALLBACKS) {
			fprintf(stderr, "%s: Converting vertex attributes, attribute data format %x,", __FUNCTION__, input->Type);
			fprintf(stderr, "stride %d, components %d\n", input->StrideB, input->Size);
		}

		GLfloat *dst_ptr, *tmp;
		tmp = dst_ptr = _mesa_malloc(sizeof(GLfloat) * input->Size * count);

		switch (input->Type) {
			case GL_DOUBLE:
				CONVERT(GLdouble, (GLfloat));
				break;
			case GL_UNSIGNED_INT:
				CONVERT(GLuint, UINT_TO_FLOAT);
				break;
			case GL_INT:
				CONVERT(GLint, INT_TO_FLOAT);
				break;
			case GL_UNSIGNED_SHORT:
				CONVERT(GLushort, USHORT_TO_FLOAT);
				break;
			case GL_SHORT:
				CONVERT(GLshort, SHORT_TO_FLOAT);
				break;
			case GL_UNSIGNED_BYTE:
				assert(input->Format != GL_BGRA);
				CONVERT(GLubyte, UBYTE_TO_FLOAT);
				break;
			case GL_BYTE:
				CONVERT(GLbyte, BYTE_TO_FLOAT);
				break;
			default:
				assert(0);
				break;
		}

		type = GL_FLOAT;
		r300_attr.free_needed = GL_TRUE;
		r300_attr.data = tmp;
		r300_attr.stride = sizeof(GLfloat) * input->Size;
		r300_attr.dwords = input->Size;
	} else {
		type = input->Type;
		r300_attr.free_needed = GL_FALSE;
		r300_attr.data = (GLvoid *)src_ptr;
		r300_attr.stride = input->StrideB;
		r300_attr.dwords = (getTypeSize(type) * input->Size  + 3)/ 4;
	}

	r300_attr.size = input->Size;
	r300_attr.element = attr;
	r300_attr.dst_loc = vbuf->num_attribs;

	switch (type) {
		case GL_FLOAT:
			switch (input->Size) {
				case 1: r300_attr.data_type = R300_DATA_TYPE_FLOAT_1; break;
				case 2: r300_attr.data_type = R300_DATA_TYPE_FLOAT_2; break;
				case 3: r300_attr.data_type = R300_DATA_TYPE_FLOAT_3; break;
				case 4: r300_attr.data_type = R300_DATA_TYPE_FLOAT_4; break;
			}
			r300_attr._signed = 0;
			r300_attr.normalize = 0;
			break;
		case GL_SHORT:
			r300_attr._signed = 1;
			r300_attr.normalize = input->Normalized;
			switch (input->Size) {
				case 1:
				case 2:
					r300_attr.data_type = R300_DATA_TYPE_SHORT_2;
					break;
				case 3:
				case 4:
					r300_attr.data_type = R300_DATA_TYPE_SHORT_4;
					break;
			}
			break;
		case GL_BYTE:
			r300_attr._signed = 1;
			r300_attr.normalize = input->Normalized;
			r300_attr.data_type = R300_DATA_TYPE_BYTE;
			break;
		case GL_UNSIGNED_SHORT:
			r300_attr._signed = 0;
			r300_attr.normalize = input->Normalized;
			switch (input->Size) {
				case 1:
				case 2:
					r300_attr.data_type = R300_DATA_TYPE_SHORT_2;
					break;
				case 3:
				case 4:
					r300_attr.data_type = R300_DATA_TYPE_SHORT_4;
					break;
			}
			break;
		case GL_UNSIGNED_BYTE:
			r300_attr._signed = 0;
			r300_attr.normalize = input->Normalized;
			if (input->Format == GL_BGRA)
				r300_attr.data_type = R300_DATA_TYPE_D3DCOLOR;
			else
				r300_attr.data_type = R300_DATA_TYPE_BYTE;
			break;

		default:
		case GL_DOUBLE:
		case GL_INT:
		case GL_UNSIGNED_INT:
			assert(0);
			break;
	}

	switch (input->Size) {
		case 4:
			r300_attr.swizzle = SWIZZLE_XYZW;
			break;
		case 3:
			r300_attr.swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
			break;
		case 2:
			r300_attr.swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_ZERO, SWIZZLE_ONE);
			break;
		case 1:
			r300_attr.swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ONE);
			break;
	}

	r300_attr.write_mask = MASK_XYZW;

	vbuf->attribs[vbuf->num_attribs] = r300_attr;
	++vbuf->num_attribs;
}

static void r300SetVertexFormat(GLcontext *ctx, const struct gl_client_array *arrays[], int count, struct gl_buffer_object **bo, GLuint *nr_bo)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_buffer *vbuf = &r300->vbuf;

	{
		int i, tmp;

		tmp = r300->selected_vp->key.InputsRead;
		i = 0;
		vbuf->num_attribs = 0;
		while (tmp) {
			/* find first enabled bit */
			while (!(tmp & 1)) {
				tmp >>= 1;
				++i;
			}

			r300TranslateAttrib(ctx, i, count, arrays[i], bo, nr_bo);

			tmp >>= 1;
			++i;
		}
	}

	r300SwitchFallback(ctx, R300_FALLBACK_AOS_LIMIT, vbuf->num_attribs > R300_MAX_AOS_ARRAYS);
	if (r300->fallback)
		return;

	{
		int i;

		for (i = 0; i < vbuf->num_attribs; i++) {
			rcommon_emit_vector(ctx, &r300->radeon.tcl.aos[i],
						vbuf->attribs[i].data, vbuf->attribs[i].dwords,
						vbuf->attribs[i].stride, count);
		}

		r300->radeon.tcl.aos_count = vbuf->num_attribs;
	}
}

static void r300FreeData(GLcontext *ctx, struct gl_buffer_object **bo, GLuint nr_bo)
{
	{
		struct r300_vertex_buffer *vbuf = &R300_CONTEXT(ctx)->vbuf;
		int i;

		for (i = 0; i < vbuf->num_attribs; i++) {
			if (vbuf->attribs[i].free_needed)
				_mesa_free(vbuf->attribs[i].data);
		}
	}

	{
		struct r300_index_buffer *ind_buf = &R300_CONTEXT(ctx)->ind_buf;
		if (ind_buf->free_needed)
			_mesa_free(ind_buf->ptr);
	}

	{
		int i;

		for (i = 0; i < nr_bo; ++i) {
			ctx->Driver.UnmapBuffer(ctx, 0, bo[i]);
		}
	}
}

static GLboolean r300TryDrawPrims(GLcontext *ctx,
					 const struct gl_client_array *arrays[],
					 const struct _mesa_prim *prim,
					 GLuint nr_prims,
					 const struct _mesa_index_buffer *ib,
					 GLuint min_index,
					 GLuint max_index )
{
	struct r300_context *r300 = R300_CONTEXT(ctx);
	struct gl_buffer_object *bo[VERT_ATTRIB_MAX+1];
	GLuint i, nr_bo = 0;

	if (ctx->NewState)
		_mesa_update_state( ctx );

	if (r300->options.hw_tcl_enabled)
		_tnl_UpdateFixedFunctionProgram(ctx);

	r300UpdateShaders(r300);

	r300SwitchFallback(ctx, R300_FALLBACK_INVALID_BUFFERS, !r300ValidateBuffers(ctx));

	r300FixupIndexBuffer(ctx, ib, bo, &nr_bo);

	r300SetVertexFormat(ctx, arrays, max_index + 1, bo, &nr_bo);

	if (r300->fallback)
		return GL_FALSE;

	r300SetupVAP(ctx, r300->selected_vp->key.InputsRead, r300->selected_vp->key.OutputsWritten);

	r300UpdateShaderStates(r300);

	r300EmitCacheFlush(r300);
	radeonEmitState(&r300->radeon);

	for (i = 0; i < nr_prims; ++i) {
		r300RunRenderPrimitive(ctx, prim[i].start, prim[i].start + prim[i].count, prim[i].mode);
	}

	r300EmitCacheFlush(r300);

	radeonReleaseArrays(ctx, ~0);

	r300FreeData(ctx, bo, nr_bo);

	return GL_TRUE;
}

/* TODO: rebase if number of indices in any of primitives is > 8192 for 32bit indices or 16384 for 16bit indices */

static void r300DrawPrims(GLcontext *ctx,
			 const struct gl_client_array *arrays[],
			 const struct _mesa_prim *prim,
			 GLuint nr_prims,
			 const struct _mesa_index_buffer *ib,
			 GLuint min_index,
			 GLuint max_index)
{
	GLboolean retval;

	if (min_index) {
		vbo_rebase_prims( ctx, arrays, prim, nr_prims, ib, min_index, max_index, r300DrawPrims );
		return;
	}

	/* Make an attempt at drawing */
	retval = r300TryDrawPrims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);

	/* If failed run tnl pipeline - it should take care of fallbacks */
	if (!retval)
		_tnl_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
}

void r300InitDraw(GLcontext *ctx)
{
	struct vbo_context *vbo = vbo_context(ctx);

	vbo->draw_prims = r300DrawPrims;
}
