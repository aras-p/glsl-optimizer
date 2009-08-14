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

#include "radeon_buffer_objects.h"

#include "tnl/tnl.h"
#include "tnl/t_vp_build.h"
#include "vbo/vbo_context.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"


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

static void r300FixupIndexBuffer(GLcontext *ctx, const struct _mesa_index_buffer *mesa_ind_buf)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLvoid *src_ptr;
	GLuint *out;
	int i;
	GLboolean mapped_named_bo = GL_FALSE;

	if (mesa_ind_buf->obj->Name && !mesa_ind_buf->obj->Pointer) {
		ctx->Driver.MapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY_ARB, mesa_ind_buf->obj);
		mapped_named_bo = GL_TRUE;
		assert(mesa_ind_buf->obj->Pointer != NULL);
	}
	src_ptr = ADD_POINTERS(mesa_ind_buf->obj->Pointer, mesa_ind_buf->ptr);

	if (mesa_ind_buf->type == GL_UNSIGNED_BYTE) {
		GLuint size = sizeof(GLushort) * ((mesa_ind_buf->count + 1) & ~1);
		GLubyte *in = (GLubyte *)src_ptr;

		radeonAllocDmaRegion(&r300->radeon, &r300->ind_buf.bo, &r300->ind_buf.bo_offset, size, 4);

		assert(r300->ind_buf.bo->ptr != NULL);
		out = (GLuint *)ADD_POINTERS(r300->ind_buf.bo->ptr, r300->ind_buf.bo_offset);

		for (i = 0; i + 1 < mesa_ind_buf->count; i += 2) {
			*out++ = in[i] | in[i + 1] << 16;
		}

		if (i < mesa_ind_buf->count) {
			*out++ = in[i];
		}

#if MESA_BIG_ENDIAN
	} else { /* if (mesa_ind_buf->type == GL_UNSIGNED_SHORT) */
		GLushort *in = (GLushort *)src_ptr;
		size = sizeof(GLushort) * ((mesa_ind_buf->count + 1) & ~1);

		radeonAllocDmaRegion(&r300->radeon, &r300->ind_buf.bo, &r300->ind_buf.bo_offet, size, 4);

		assert(r300->ind_buf.bo->ptr != NULL)
		out = (GLuint *)ADD_POINTERS(r300->ind_buf.bo->ptr, r300->ind_buf.bo_offset);

		for (i = 0; i + 1 < mesa_ind_buf->count; i += 2) {
			*out++ = in[i] | in[i + 1] << 16;
		}

		if (i < mesa_ind_buf->count) {
			*out++ = in[i];
		}
#endif
	}

	r300->ind_buf.is_32bit = GL_FALSE;
	r300->ind_buf.count = mesa_ind_buf->count;

	if (mapped_named_bo) {
		ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, mesa_ind_buf->obj);
	}
}


static void r300SetupIndexBuffer(GLcontext *ctx, const struct _mesa_index_buffer *mesa_ind_buf)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (!mesa_ind_buf) {
		r300->ind_buf.bo = NULL;
		return;
	}

#if MESA_BIG_ENDIAN
	if (mesa_ind_buf->type == GL_UNSIGNED_INT) {
#else
	if (mesa_ind_buf->type != GL_UNSIGNED_BYTE) {
#endif
		const GLvoid *src_ptr;
		GLvoid *dst_ptr;
		GLboolean mapped_named_bo = GL_FALSE;

		if (mesa_ind_buf->obj->Name && !mesa_ind_buf->obj->Pointer) {
			ctx->Driver.MapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY_ARB, mesa_ind_buf->obj);
			assert(mesa_ind_buf->obj->Pointer != NULL);
			mapped_named_bo = GL_TRUE;
		}

		src_ptr = ADD_POINTERS(mesa_ind_buf->obj->Pointer, mesa_ind_buf->ptr);

		const GLuint size = mesa_ind_buf->count * getTypeSize(mesa_ind_buf->type);

		radeonAllocDmaRegion(&r300->radeon, &r300->ind_buf.bo, &r300->ind_buf.bo_offset, size, 4);

		assert(r300->ind_buf.bo->ptr != NULL);
		dst_ptr = ADD_POINTERS(r300->ind_buf.bo->ptr, r300->ind_buf.bo_offset);
		_mesa_memcpy(dst_ptr, src_ptr, size);

		r300->ind_buf.is_32bit = (mesa_ind_buf->type == GL_UNSIGNED_INT);
		r300->ind_buf.count = mesa_ind_buf->count;

		if (mapped_named_bo) {
			ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, mesa_ind_buf->obj);
		}
	} else {
		r300FixupIndexBuffer(ctx, mesa_ind_buf);
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
			src_ptr += stride;			\
		}						\
	} else {					\
		for (i = 0; i < count; i++) {		\
			const TYPE *in = (TYPE *)src_ptr;		\
			for (j = 0; j < sz; j++) {		\
				*dst_ptr++ = (GLfloat)(*in);		\
				in++;				\
			}					\
			src_ptr += stride;			\
		}						\
	}						\
} while (0)

/**
 * Convert attribute data type to float
 * If the attribute uses named buffer object replace the bo with newly allocated bo
 */
static void r300ConvertAttrib(GLcontext *ctx, int count, const struct gl_client_array *input, struct vertex_attribute *attr)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	const GLvoid *src_ptr;
	GLboolean mapped_named_bo = GL_FALSE;
	GLfloat *dst_ptr;
	GLuint stride;

	stride = (input->StrideB == 0) ? getTypeSize(input->Type) * input->Size : input->StrideB;

	/* Convert value for first element only */
	if (input->StrideB == 0)
		count = 1;

	if (input->BufferObj->Name) {
		if (!input->BufferObj->Pointer) {
			ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER, GL_READ_ONLY_ARB, input->BufferObj);
			mapped_named_bo = GL_TRUE;
		}

		src_ptr = ADD_POINTERS(input->BufferObj->Pointer, input->Ptr);
	} else {
		src_ptr = input->Ptr;
	}

	radeonAllocDmaRegion(&r300->radeon, &attr->bo, &attr->bo_offset, sizeof(GLfloat) * input->Size * count, 32);
	dst_ptr = (GLfloat *)ADD_POINTERS(attr->bo->ptr, attr->bo_offset);

	if (RADEON_DEBUG & DEBUG_FALLBACKS) {
		fprintf(stderr, "%s: Converting vertex attributes, attribute data format %x,", __FUNCTION__, input->Type);
		fprintf(stderr, "stride %d, components %d\n", stride, input->Size);
	}

	assert(src_ptr != NULL);

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

	if (mapped_named_bo) {
		ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER, input->BufferObj);
	}
}

static void r300AlignDataToDword(GLcontext *ctx, const struct gl_client_array *input, int count, struct vertex_attribute *attr)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	const int dst_stride = (input->StrideB + 3) & ~3;
	const int size = getTypeSize(input->Type) * input->Size * count;
	GLboolean mapped_named_bo = GL_FALSE;

	radeonAllocDmaRegion(&r300->radeon, &attr->bo, &attr->bo_offset, size, 32);

	if (!input->BufferObj->Pointer) {
		ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER, GL_READ_ONLY_ARB, input->BufferObj);
		mapped_named_bo = GL_TRUE;
	}

	{
		GLvoid *src_ptr = ADD_POINTERS(input->BufferObj->Pointer, input->Ptr);
		GLvoid *dst_ptr = ADD_POINTERS(attr->bo->ptr, attr->bo_offset);
		int i;

		for (i = 0; i < count; ++i) {
			_mesa_memcpy(dst_ptr, src_ptr, input->StrideB);
			src_ptr += input->StrideB;
			dst_ptr += dst_stride;
		}
	}

	if (mapped_named_bo) {
		ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER, input->BufferObj);
	}

	attr->stride = dst_stride;
}

static void r300TranslateAttrib(GLcontext *ctx, GLuint attr, int count, const struct gl_client_array *input)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_buffer *vbuf = &r300->vbuf;
	struct vertex_attribute r300_attr;
	GLenum type;
	GLuint stride;

	stride = (input->StrideB == 0) ? getTypeSize(input->Type) * input->Size : input->StrideB;

	if (input->Type == GL_DOUBLE || input->Type == GL_UNSIGNED_INT || input->Type == GL_INT ||
#if MESA_BIG_ENDIAN
	    getTypeSize(input->Type) != 4 ||
#endif
	    stride < 4) {

		type = GL_FLOAT;

		r300ConvertAttrib(ctx, count, input, &r300_attr);
		if (input->StrideB == 0) {
			r300_attr.stride = 0;
		} else {
			r300_attr.stride = sizeof(GLfloat) * input->Size;
		}
		r300_attr.dwords = input->Size;
		r300_attr.is_named_bo = GL_FALSE;
	} else {
		type = input->Type;
		r300_attr.dwords = (getTypeSize(type) * input->Size + 3)/ 4;
		if (input->BufferObj->Name) {
			if (stride % 4 != 0) {
				assert(((int) input->Ptr) % input->StrideB == 0);
				r300AlignDataToDword(ctx, input, count, &r300_attr);
				r300_attr.is_named_bo = GL_FALSE;
			} else {
				r300_attr.stride = input->StrideB;
				r300_attr.bo_offset = (GLuint) input->Ptr;
				r300_attr.bo = get_radeon_buffer_object(input->BufferObj)->bo;
				r300_attr.is_named_bo = GL_TRUE;
			}
		} else {
			int size;
			uint32_t *dst;

			if (input->StrideB == 0) {
				size = getTypeSize(input->Type) * input->Size;
				count = 1;
				r300_attr.stride = 0;
			} else {
				size = getTypeSize(input->Type) * input->Size * count;
				r300_attr.stride = (getTypeSize(type) * input->Size + 3) & ~3;
			}

			radeonAllocDmaRegion(&r300->radeon, &r300_attr.bo, &r300_attr.bo_offset, size, 32);
			assert(r300_attr.bo->ptr != NULL);
			dst = (uint32_t *)ADD_POINTERS(r300_attr.bo->ptr, r300_attr.bo_offset);
			switch (r300_attr.dwords) {
				case 1: radeonEmitVec4(dst, input->Ptr, input->StrideB, count); break;
				case 2: radeonEmitVec8(dst, input->Ptr, input->StrideB, count); break;
				case 3: radeonEmitVec12(dst, input->Ptr, input->StrideB, count); break;
				case 4: radeonEmitVec16(dst, input->Ptr, input->StrideB, count); break;
				default: assert(0); break;
			}

			r300_attr.is_named_bo = GL_FALSE;
		}
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

static void r300SetVertexFormat(GLcontext *ctx, const struct gl_client_array *arrays[], int count)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_buffer *vbuf = &r300->vbuf;

	{
		int i, tmp;

		tmp = r300->selected_vp->code.InputsRead;
		i = 0;
		vbuf->num_attribs = 0;
		while (tmp) {
			/* find first enabled bit */
			while (!(tmp & 1)) {
				tmp >>= 1;
				++i;
			}

			r300TranslateAttrib(ctx, i, count, arrays[i]);

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
			struct radeon_aos *aos = &r300->radeon.tcl.aos[i];

			aos->count = vbuf->attribs[i].stride == 0 ? 1 : count;
			aos->stride = vbuf->attribs[i].stride / sizeof(float);
			aos->offset = vbuf->attribs[i].bo_offset;
			aos->components = vbuf->attribs[i].dwords;
			aos->bo = vbuf->attribs[i].bo;

			radeon_cs_space_check_with_bo(r300->radeon.cmdbuf.cs,
										  r300->vbuf.attribs[i].bo,
										  RADEON_GEM_DOMAIN_GTT, 0);
			if (vbuf->attribs[i].is_named_bo) {
				radeon_cs_space_add_persistent_bo(r300->radeon.cmdbuf.cs,
												  r300->vbuf.attribs[i].bo,
												  RADEON_GEM_DOMAIN_GTT, 0);
			}
		}
		r300->radeon.tcl.aos_count = vbuf->num_attribs;

		if (r300->ind_buf.bo) {
			radeon_cs_space_check_with_bo(r300->radeon.cmdbuf.cs,
										  r300->ind_buf.bo,
										  RADEON_GEM_DOMAIN_GTT, 0);
		}
	}
}

static void r300FreeData(GLcontext *ctx)
{
	/* Need to zero tcl.aos[n].bo and tcl.elt_dma_bo
	 * to prevent double unref in radeonReleaseArrays
	 * called during context destroy
	 */
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	{
		int i;

		for (i = 0; i < r300->vbuf.num_attribs; i++) {
			if (!r300->vbuf.attribs[i].is_named_bo) {
				radeon_bo_unref(r300->vbuf.attribs[i].bo);
			}
			r300->radeon.tcl.aos[i].bo = NULL;
		}
	}

	{
		if (r300->ind_buf.bo != NULL) {
			radeon_bo_unref(r300->ind_buf.bo);
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
	GLuint i;

	if (ctx->NewState)
		_mesa_update_state( ctx );

	if (r300->options.hw_tcl_enabled)
		_tnl_UpdateFixedFunctionProgram(ctx);

	r300UpdateShaders(r300);

	r300SwitchFallback(ctx, R300_FALLBACK_INVALID_BUFFERS, !r300ValidateBuffers(ctx));

	r300SetupIndexBuffer(ctx, ib);

	/* ensure we have the cmd buf space in advance to cover
 	 * the state + DMA AOS pointers */
	rcommonEnsureCmdBufSpace(&r300->radeon,
                           r300->radeon.hw.max_state_size + (50*sizeof(int)),
                           __FUNCTION__);

	r300SetVertexFormat(ctx, arrays, max_index + 1);

	if (r300->fallback)
		return GL_FALSE;

	r300SetupVAP(ctx, r300->selected_vp->code.InputsRead, r300->selected_vp->code.OutputsWritten);

	r300UpdateShaderStates(r300);

	r300EmitCacheFlush(r300);
	radeonEmitState(&r300->radeon);

	for (i = 0; i < nr_prims; ++i) {
		r300RunRenderPrimitive(ctx, prim[i].start, prim[i].start + prim[i].count, prim[i].mode);
	}

	r300EmitCacheFlush(r300);

	r300FreeData(ctx);

	return GL_TRUE;
}

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
