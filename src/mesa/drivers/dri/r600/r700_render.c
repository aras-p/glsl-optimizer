/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */

#include "main/glheader.h"
#include "main/state.h"
#include "main/imports.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/dd.h"
#include "main/simple_list.h"
#include "main/api_arrayelt.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_vp_build.h"
#include "tnl/t_context.h"
#include "tnl/t_vertex.h"
#include "vbo/vbo_context.h"

#include "r600_context.h"
#include "r600_cmdbuf.h"

#include "r600_tex.h"

#include "r700_vertprog.h"
#include "r700_fragprog.h"
#include "r700_state.h"

#include "radeon_buffer_objects.h"
#include "radeon_common_context.h"

void r700WaitForIdle(context_t *context);
void r700WaitForIdleClean(context_t *context);
static unsigned int r700PrimitiveType(int prim);
GLboolean r700SyncSurf(context_t *context,
		       struct radeon_bo *pbo,
		       uint32_t read_domain,
		       uint32_t write_domain,
		       uint32_t sync_type);

void r700WaitForIdle(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    BEGIN_BATCH_NO_AUTOSTATE(3);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
    R600_OUT_BATCH(mmWAIT_UNTIL - ASIC_CONFIG_BASE_INDEX);
    R600_OUT_BATCH(WAIT_3D_IDLE_bit);

    END_BATCH();
    COMMIT_BATCH();
}

void r700WaitForIdleClean(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    BEGIN_BATCH_NO_AUTOSTATE(5);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_EVENT_WRITE, 0));
    R600_OUT_BATCH(CACHE_FLUSH_AND_INV_EVENT);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
    R600_OUT_BATCH(mmWAIT_UNTIL - ASIC_CONFIG_BASE_INDEX);
    R600_OUT_BATCH(WAIT_3D_IDLE_bit | WAIT_3D_IDLECLEAN_bit);

    END_BATCH();
    COMMIT_BATCH();
}

void r700Start3D(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
    {
        BEGIN_BATCH_NO_AUTOSTATE(2);
        R600_OUT_BATCH(CP_PACKET3(R600_IT_START_3D_CMDBUF, 0));
        R600_OUT_BATCH(0);
        END_BATCH();
    }

    BEGIN_BATCH_NO_AUTOSTATE(3);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_CONTEXT_CONTROL, 1));
    R600_OUT_BATCH(0x80000000);
    R600_OUT_BATCH(0x80000000);
    END_BATCH();

    COMMIT_BATCH();
}

GLboolean r700SyncSurf(context_t *context,
		       struct radeon_bo *pbo,
		       uint32_t read_domain,
		       uint32_t write_domain,
		       uint32_t sync_type)
{
    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_RENDER | RADEON_STATE, RADEON_TRACE, "%s\n", __func__);
    uint32_t cp_coher_size;

    if (!pbo)
	    return GL_FALSE;

    if (pbo->size == 0xffffffff)
	    cp_coher_size = 0xffffffff;
    else
	    cp_coher_size = ((pbo->size + 255) >> 8);

    BEGIN_BATCH_NO_AUTOSTATE(5 + 2);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SURFACE_SYNC, 3));
    R600_OUT_BATCH(sync_type);
    R600_OUT_BATCH(cp_coher_size);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(10);
    R600_OUT_BATCH_RELOC(0,
			 pbo,
			 0,
			 read_domain, write_domain, 0);
    END_BATCH();
    COMMIT_BATCH();

    return GL_TRUE;
}

static unsigned int r700PrimitiveType(int prim)
{
    switch (prim & PRIM_MODE_MASK)
    {
    case GL_POINTS:
        return DI_PT_POINTLIST;
        break;
    case GL_LINES:
        return DI_PT_LINELIST;
        break;
    case GL_LINE_STRIP:
        return DI_PT_LINESTRIP;
        break;
    case GL_LINE_LOOP:
        return DI_PT_LINELOOP;
        break;
    case GL_TRIANGLES:
        return DI_PT_TRILIST;
        break;
    case GL_TRIANGLE_STRIP:
        return DI_PT_TRISTRIP;
        break;
    case GL_TRIANGLE_FAN:
        return DI_PT_TRIFAN;
        break;
    case GL_QUADS:
        return DI_PT_QUADLIST;
        break;
    case GL_QUAD_STRIP:
        return DI_PT_QUADSTRIP;
        break;
    case GL_POLYGON:
        return DI_PT_POLYGON;
        break;
    default:
        assert(0);
        return -1;
        break;
    }
}

static int r700NumVerts(int num_verts, int prim)
{
	int verts_off = 0;

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
		verts_off = 0;
		break;
	case GL_LINES:
		verts_off = num_verts % 2;
		break;
	case GL_LINE_STRIP:
		if (num_verts < 2)
			verts_off = num_verts;
		break;
	case GL_LINE_LOOP:
		if (num_verts < 2)
			verts_off = num_verts;
		break;
	case GL_TRIANGLES:
		verts_off = num_verts % 3;
		break;
	case GL_TRIANGLE_STRIP:
		if (num_verts < 3)
			verts_off = num_verts;
		break;
	case GL_TRIANGLE_FAN:
		if (num_verts < 3)
			verts_off = num_verts;
		break;
	case GL_QUADS:
		verts_off = num_verts % 4;
		break;
	case GL_QUAD_STRIP:
		if (num_verts < 4)
			verts_off = num_verts;
		else
			verts_off = num_verts % 2;
		break;
	case GL_POLYGON:
		if (num_verts < 3)
			verts_off = num_verts;
		break;
	default:
		assert(0);
		return -1;
		break;
	}

	return num_verts - verts_off;
}

static void r700RunRenderPrimitive(GLcontext * ctx, int start, int end, int prim)
{
    context_t *context = R700_CONTEXT(ctx);
    BATCH_LOCALS(&context->radeon);
    int type, total_emit;
    int num_indices;
    uint32_t vgt_draw_initiator = 0;
    uint32_t vgt_index_type     = 0;
    uint32_t vgt_primitive_type = 0;
    uint32_t vgt_num_indices    = 0;

    type = r700PrimitiveType(prim);
    num_indices = r700NumVerts(end - start, prim);

    radeon_print(RADEON_RENDER, RADEON_TRACE,
		 "%s type %x num_indices %d\n",
		 __func__, type, num_indices);

    if (type < 0 || num_indices <= 0)
	    return;

    SETfield(vgt_primitive_type, type,
	     VGT_PRIMITIVE_TYPE__PRIM_TYPE_shift, VGT_PRIMITIVE_TYPE__PRIM_TYPE_mask);

    SETfield(vgt_index_type, DI_INDEX_SIZE_32_BIT, INDEX_TYPE_shift, INDEX_TYPE_mask);

    if(GL_TRUE != context->ind_buf.is_32bit)
    {
            SETfield(vgt_index_type, DI_INDEX_SIZE_16_BIT, INDEX_TYPE_shift, INDEX_TYPE_mask);
    }

    vgt_num_indices = num_indices;
    SETfield(vgt_draw_initiator, DI_SRC_SEL_DMA, SOURCE_SELECT_shift, SOURCE_SELECT_mask);
    SETfield(vgt_draw_initiator, DI_MAJOR_MODE_0, MAJOR_MODE_shift, MAJOR_MODE_mask);

    total_emit =   3  /* VGT_PRIMITIVE_TYPE */
	         + 2  /* VGT_INDEX_TYPE */
	         + 2  /* NUM_INSTANCES */
	         + 5 + 2; /* DRAW_INDEX */

    BEGIN_BATCH_NO_AUTOSTATE(total_emit);
    // prim
    R600_OUT_BATCH_REGSEQ(VGT_PRIMITIVE_TYPE, 1);
    R600_OUT_BATCH(vgt_primitive_type);
    // index type
    R600_OUT_BATCH(CP_PACKET3(R600_IT_INDEX_TYPE, 0));
    R600_OUT_BATCH(vgt_index_type);
    // num instances
    R600_OUT_BATCH(CP_PACKET3(R600_IT_NUM_INSTANCES, 0));
    R600_OUT_BATCH(1);
    // draw packet
    R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX, 3));
    R600_OUT_BATCH(context->ind_buf.bo_offset);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(vgt_num_indices);
    R600_OUT_BATCH(vgt_draw_initiator);
    R600_OUT_BATCH_RELOC(context->ind_buf.bo_offset,
			 context->ind_buf.bo,
			 context->ind_buf.bo_offset,
			 RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
    COMMIT_BATCH();
}

static void r700RunRenderPrimitiveImmediate(GLcontext * ctx, int start, int end, int prim)
{
    context_t *context = R700_CONTEXT(ctx);
    BATCH_LOCALS(&context->radeon);
    int type, i;
    uint32_t num_indices, total_emit = 0;
    uint32_t vgt_draw_initiator = 0;
    uint32_t vgt_index_type     = 0;
    uint32_t vgt_primitive_type = 0;
    uint32_t vgt_num_indices    = 0;

    type = r700PrimitiveType(prim);
    num_indices = r700NumVerts(end - start, prim);

    radeon_print(RADEON_RENDER, RADEON_TRACE,
		 "%s type %x num_indices %d\n",
		 __func__, type, num_indices);

    if (type < 0 || num_indices <= 0)
	    return;

    SETfield(vgt_primitive_type, type,
	     VGT_PRIMITIVE_TYPE__PRIM_TYPE_shift, VGT_PRIMITIVE_TYPE__PRIM_TYPE_mask);

    if (num_indices > 0xffff)
    {
	    SETfield(vgt_index_type, DI_INDEX_SIZE_32_BIT, INDEX_TYPE_shift, INDEX_TYPE_mask);
    }
    else
    {
            SETfield(vgt_index_type, DI_INDEX_SIZE_16_BIT, INDEX_TYPE_shift, INDEX_TYPE_mask);
    }

    vgt_num_indices = num_indices;
    SETfield(vgt_draw_initiator, DI_MAJOR_MODE_0, MAJOR_MODE_shift, MAJOR_MODE_mask);

    if (start == 0)
    {
	SETfield(vgt_draw_initiator, DI_SRC_SEL_AUTO_INDEX, SOURCE_SELECT_shift, SOURCE_SELECT_mask);
    }
    else
    {
	if (num_indices > 0xffff)
	{
		total_emit += num_indices;
	}
	else
	{
		total_emit += (num_indices + 1) / 2;
	}
	SETfield(vgt_draw_initiator, DI_SRC_SEL_IMMEDIATE, SOURCE_SELECT_shift, SOURCE_SELECT_mask);
    }

    total_emit +=   3 /* VGT_PRIMITIVE_TYPE */
	          + 2 /* VGT_INDEX_TYPE */
	          + 2 /* NUM_INSTANCES */
	          + 3; /* DRAW */

    BEGIN_BATCH_NO_AUTOSTATE(total_emit);
    // prim
    R600_OUT_BATCH_REGSEQ(VGT_PRIMITIVE_TYPE, 1);
    R600_OUT_BATCH(vgt_primitive_type);
    // index type
    R600_OUT_BATCH(CP_PACKET3(R600_IT_INDEX_TYPE, 0));
    R600_OUT_BATCH(vgt_index_type);
    // num instances
    R600_OUT_BATCH(CP_PACKET3(R600_IT_NUM_INSTANCES, 0));
    R600_OUT_BATCH(1);
    // draw packet
    if(start == 0)
    {
        R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX_AUTO, 1));
        R600_OUT_BATCH(vgt_num_indices);
        R600_OUT_BATCH(vgt_draw_initiator);
    }
    else
    {
	if (num_indices > 0xffff)
        {
	    R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX_IMMD, (num_indices + 1)));
	    R600_OUT_BATCH(vgt_num_indices);
	    R600_OUT_BATCH(vgt_draw_initiator);
	    for (i = start; i < (start + num_indices); i++)
	    {
		R600_OUT_BATCH(i);
	    }
	}
	else
        {
	    R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX_IMMD, (((num_indices + 1) / 2) + 1)));
	    R600_OUT_BATCH(vgt_num_indices);
	    R600_OUT_BATCH(vgt_draw_initiator);
	    for (i = start; i < (start + num_indices); i += 2)
	    {
		if ((i + 1) == (start + num_indices))
		{
		    R600_OUT_BATCH(i);
		}
		else
		{
		    R600_OUT_BATCH(((i + 1) << 16) | (i));
		}
	    }
	}
    }

    END_BATCH();
    COMMIT_BATCH();
}

/* start 3d, idle, cb/db flush */
#define PRE_EMIT_STATE_BUFSZ 5 + 5 + 14

static GLuint r700PredictRenderSize(GLcontext* ctx,
				    const struct _mesa_prim *prim,
				    const struct _mesa_index_buffer *ib,
				    GLuint nr_prims)
{
    context_t *context = R700_CONTEXT(ctx);
    GLboolean flushed;
    GLuint dwords, i;
    GLuint state_size;

    dwords = PRE_EMIT_STATE_BUFSZ;
    if (ib)
	    dwords += nr_prims * 14;
    else {
	    for (i = 0; i < nr_prims; ++i)
	    {
		    if (prim[i].start == 0)
			    dwords += 10;
		    else if (prim[i].count > 0xffff)
			    dwords += prim[i].count + 10;
		    else
			    dwords += ((prim[i].count + 1) / 2) + 10;
	    }
    }

    state_size = radeonCountStateEmitSize(&context->radeon);
    flushed = rcommonEnsureCmdBufSpace(&context->radeon,
				       dwords + state_size,
				       __FUNCTION__);
    if (flushed)
	    dwords += radeonCountStateEmitSize(&context->radeon);
    else
	    dwords += state_size;

    radeon_print(RADEON_RENDER, RADEON_VERBOSE, "%s: total prediction size is %d.\n", __FUNCTION__, dwords);
    return dwords;

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
static void r700ConvertAttrib(GLcontext *ctx, int count, 
                              const struct gl_client_array *input, 
                              struct StreamDesc *attr)
{
    context_t *context = R700_CONTEXT(ctx);
    const GLvoid *src_ptr;
    GLboolean mapped_named_bo = GL_FALSE;
    GLfloat *dst_ptr;
    GLuint stride;

    stride = (input->StrideB == 0) ? getTypeSize(input->Type) * input->Size : input->StrideB;

    /* Convert value for first element only */
    if (input->StrideB == 0)
    {
        count = 1;
    }

    if (input->BufferObj->Name) 
    {
        if (!input->BufferObj->Pointer) 
        {
            ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER, GL_READ_ONLY_ARB, input->BufferObj);
            mapped_named_bo = GL_TRUE;
        }

        src_ptr = ADD_POINTERS(input->BufferObj->Pointer, input->Ptr);
    } 
    else 
    {
        src_ptr = input->Ptr;
    }

    radeonAllocDmaRegion(&context->radeon, &attr->bo, &attr->bo_offset, 
                         sizeof(GLfloat) * input->Size * count, 32);

    radeon_bo_map(attr->bo, 1);

    dst_ptr = (GLfloat *)ADD_POINTERS(attr->bo->ptr, attr->bo_offset);

    assert(src_ptr != NULL);

    switch (input->Type) 
    {
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

    radeon_bo_unmap(attr->bo);

    if (mapped_named_bo) 
    {
        ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER, input->BufferObj);
    }
}

static void r700AlignDataToDword(GLcontext *ctx, 
                                 const struct gl_client_array *input, 
                                 int count, 
                                 struct StreamDesc *attr)
{
    context_t *context = R700_CONTEXT(ctx);
    const int dst_stride = (input->StrideB + 3) & ~3;
    const int size = getTypeSize(input->Type) * input->Size * count;
    GLboolean mapped_named_bo = GL_FALSE;

    radeonAllocDmaRegion(&context->radeon, &attr->bo, &attr->bo_offset, size, 32);

    radeon_bo_map(attr->bo, 1);

    if (!input->BufferObj->Pointer) 
    {
        ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER, GL_READ_ONLY_ARB, input->BufferObj);
        mapped_named_bo = GL_TRUE;
    }

    {
        GLvoid *src_ptr = ADD_POINTERS(input->BufferObj->Pointer, input->Ptr);
        GLvoid *dst_ptr = ADD_POINTERS(attr->bo->ptr, attr->bo_offset);
        int i;

        for (i = 0; i < count; ++i) 
        {
            memcpy(dst_ptr, src_ptr, input->StrideB);
            src_ptr += input->StrideB;
            dst_ptr += dst_stride;
        }
    }

    radeon_bo_unmap(attr->bo);
    if (mapped_named_bo) 
    {
        ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER, input->BufferObj);
    }

    attr->stride = dst_stride;
}

static void r700SetupStreams(GLcontext *ctx, const struct gl_client_array *input[], int count)
{
	context_t *context = R700_CONTEXT(ctx);
    GLuint stride;
    int ret;
    int i, index;

    R600_STATECHANGE(context, vtx);

    for(index = 0; index < context->nNumActiveAos; index++) 
    {
        struct radeon_aos *aos = &context->radeon.tcl.aos[index];
        i = context->stream_desc[index].element;

        stride = (input[i]->StrideB == 0) ? getTypeSize(input[i]->Type) * input[i]->Size : input[i]->StrideB;

        if (input[i]->Type == GL_DOUBLE || input[i]->Type == GL_UNSIGNED_INT || input[i]->Type == GL_INT ||
#if MESA_BIG_ENDIAN
            getTypeSize(input[i]->Type) != 4 || 
#endif
            stride < 4) 
        {
            r700ConvertAttrib(ctx, count, input[i], &context->stream_desc[index]);
        } 
        else 
        {
            if (input[i]->BufferObj->Name) 
            {
                if (stride % 4 != 0) 
                {
                    assert(((intptr_t) input[i]->Ptr) % input[i]->StrideB == 0);
                    r700AlignDataToDword(ctx, input[i], count, &context->stream_desc[index]);
                    context->stream_desc[index].is_named_bo = GL_FALSE;
                } 
                else 
                {
                    context->stream_desc[index].stride = input[i]->StrideB;
                    context->stream_desc[index].bo_offset = (intptr_t) input[i]->Ptr;
                    context->stream_desc[index].bo = get_radeon_buffer_object(input[i]->BufferObj)->bo;
                    context->stream_desc[index].is_named_bo = GL_TRUE;
                }
            } 
            else 
            {
                int size;
                int local_count = count;
                uint32_t *dst;

                if (input[i]->StrideB == 0) 
                {
                    size = getTypeSize(input[i]->Type) * input[i]->Size;
                    local_count = 1;
                } 
                else 
                {
                    size = getTypeSize(input[i]->Type) * input[i]->Size * local_count;
                }

                radeonAllocDmaRegion(&context->radeon, &context->stream_desc[index].bo, 
                                     &context->stream_desc[index].bo_offset, size, 32);

                radeon_bo_map(context->stream_desc[index].bo, 1);
                assert(context->stream_desc[index].bo->ptr != NULL);


                dst = (uint32_t *)ADD_POINTERS(context->stream_desc[index].bo->ptr, 
                                               context->stream_desc[index].bo_offset);

                switch (context->stream_desc[index].dwords) 
                {
                case 1:                     
                    radeonEmitVec4(dst, input[i]->Ptr, input[i]->StrideB, local_count);
                    break;
                case 2: 
                    radeonEmitVec8(dst, input[i]->Ptr, input[i]->StrideB, local_count); 
                    break;
                case 3: 
                    radeonEmitVec12(dst, input[i]->Ptr, input[i]->StrideB, local_count); 
                    break;
                case 4: 
                    radeonEmitVec16(dst, input[i]->Ptr, input[i]->StrideB, local_count); 
                    break;
                default: 
                    assert(0); 
                    break;
                }
		radeon_bo_unmap(context->stream_desc[index].bo);
            }
        }

        aos->count = context->stream_desc[index].stride == 0 ? 1 : count;
        aos->stride = context->stream_desc[index].stride / sizeof(float);
        aos->components = context->stream_desc[index].dwords;
        aos->bo = context->stream_desc[index].bo;
        aos->offset = context->stream_desc[index].bo_offset;

        if(context->stream_desc[index].is_named_bo) 
        {
            radeon_cs_space_add_persistent_bo(context->radeon.cmdbuf.cs, 
                                              context->stream_desc[index].bo, 
                                              RADEON_GEM_DOMAIN_GTT, 0);
        }
    }

    ret = radeon_cs_space_check_with_bo(context->radeon.cmdbuf.cs, 
                                        first_elem(&context->radeon.dma.reserved)->bo, 
                                        RADEON_GEM_DOMAIN_GTT, 0);    
}

static void r700FreeData(GLcontext *ctx)
{
    /* Need to zero tcl.aos[n].bo and tcl.elt_dma_bo
     * to prevent double unref in radeonReleaseArrays
     * called during context destroy
     */
    context_t *context = R700_CONTEXT(ctx);

    int i;

    for (i = 0; i < context->nNumActiveAos; i++)
    {
        if (!context->stream_desc[i].is_named_bo)
        {
	        radeon_bo_unref(context->stream_desc[i].bo);
        }
        context->radeon.tcl.aos[i].bo = NULL;
    }

    if (context->ind_buf.bo != NULL)
    {
            radeon_bo_unref(context->ind_buf.bo);
    }
}

static void r700FixupIndexBuffer(GLcontext *ctx, const struct _mesa_index_buffer *mesa_ind_buf)
{
    context_t *context = R700_CONTEXT(ctx);
    GLvoid *src_ptr;
    GLuint *out;
    int i;
    GLboolean mapped_named_bo = GL_FALSE;

    if (mesa_ind_buf->obj->Name && !mesa_ind_buf->obj->Pointer)
    {
        ctx->Driver.MapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY_ARB, mesa_ind_buf->obj);
        mapped_named_bo = GL_TRUE;
        assert(mesa_ind_buf->obj->Pointer != NULL);
    }
    src_ptr = ADD_POINTERS(mesa_ind_buf->obj->Pointer, mesa_ind_buf->ptr);

    if (mesa_ind_buf->type == GL_UNSIGNED_BYTE)
    {
        GLuint size = sizeof(GLushort) * ((mesa_ind_buf->count + 1) & ~1);
        GLubyte *in = (GLubyte *)src_ptr;

	radeonAllocDmaRegion(&context->radeon, &context->ind_buf.bo,
			     &context->ind_buf.bo_offset, size, 4);

	radeon_bo_map(context->ind_buf.bo, 1);
	assert(context->ind_buf.bo->ptr != NULL);
	out = (GLuint *)ADD_POINTERS(context->ind_buf.bo->ptr, context->ind_buf.bo_offset);

        for (i = 0; i + 1 < mesa_ind_buf->count; i += 2)
        {
            *out++ = in[i] | in[i + 1] << 16;
        }

        if (i < mesa_ind_buf->count)
        {
            *out++ = in[i];
        }

	radeon_bo_unmap(context->ind_buf.bo);
#if MESA_BIG_ENDIAN
    }
    else
    { /* if (mesa_ind_buf->type == GL_UNSIGNED_SHORT) */
        GLushort *in = (GLushort *)src_ptr;
        GLuint size = sizeof(GLushort) * ((mesa_ind_buf->count + 1) & ~1);

	radeonAllocDmaRegion(&context->radeon, &context->ind_buf.bo,
			     &context->ind_buf.bo_offset, size, 4);

	radeon_bo_map(context->ind_buf.bo, 1);
	assert(context->ind_buf.bo->ptr != NULL);
	out = (GLuint *)ADD_POINTERS(context->ind_buf.bo->ptr, context->ind_buf.bo_offset);

        for (i = 0; i + 1 < mesa_ind_buf->count; i += 2)
        {
            *out++ = in[i] | in[i + 1] << 16;
        }

        if (i < mesa_ind_buf->count)
        {
            *out++ = in[i];
        }
	radeon_bo_unmap(context->ind_buf.bo);
#endif
    }

    context->ind_buf.is_32bit = GL_FALSE;
    context->ind_buf.count = mesa_ind_buf->count;

    if (mapped_named_bo)
    {
        ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, mesa_ind_buf->obj);
    }
}

static void r700SetupIndexBuffer(GLcontext *ctx, const struct _mesa_index_buffer *mesa_ind_buf)
{
    context_t *context = R700_CONTEXT(ctx);

    if (!mesa_ind_buf) {
        context->ind_buf.bo = NULL;
        return;
    }

#if MESA_BIG_ENDIAN
    if (mesa_ind_buf->type == GL_UNSIGNED_INT)
#else
    if (mesa_ind_buf->type != GL_UNSIGNED_BYTE)
#endif
    {
        const GLvoid *src_ptr;
        GLvoid *dst_ptr;
        GLboolean mapped_named_bo = GL_FALSE;

        if (mesa_ind_buf->obj->Name && !mesa_ind_buf->obj->Pointer)
        {
	        ctx->Driver.MapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY_ARB, mesa_ind_buf->obj);
	        assert(mesa_ind_buf->obj->Pointer != NULL);
	        mapped_named_bo = GL_TRUE;
        }

        src_ptr = ADD_POINTERS(mesa_ind_buf->obj->Pointer, mesa_ind_buf->ptr);

        const GLuint size = mesa_ind_buf->count * getTypeSize(mesa_ind_buf->type);

	radeonAllocDmaRegion(&context->radeon, &context->ind_buf.bo,
			     &context->ind_buf.bo_offset, size, 4);
	radeon_bo_map(context->ind_buf.bo, 1);
	assert(context->ind_buf.bo->ptr != NULL);
	dst_ptr = ADD_POINTERS(context->ind_buf.bo->ptr, context->ind_buf.bo_offset);

        memcpy(dst_ptr, src_ptr, size);

	radeon_bo_unmap(context->ind_buf.bo);
        context->ind_buf.is_32bit = (mesa_ind_buf->type == GL_UNSIGNED_INT);
        context->ind_buf.count = mesa_ind_buf->count;

        if (mapped_named_bo)
        {
	        ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER, mesa_ind_buf->obj);
        }
    }
    else
    {
	    r700FixupIndexBuffer(ctx, mesa_ind_buf);
    }
}

static GLboolean check_fallbacks(GLcontext *ctx)
{
	if (ctx->RenderMode != GL_RENDER)
		return GL_TRUE;

	return GL_FALSE;
}

static GLboolean r700TryDrawPrims(GLcontext *ctx,
				  const struct gl_client_array *arrays[],
				  const struct _mesa_prim *prim,
				  GLuint nr_prims,
				  const struct _mesa_index_buffer *ib,
				  GLuint min_index,
				  GLuint max_index )
{
    context_t *context = R700_CONTEXT(ctx);
    radeonContextPtr radeon = &context->radeon;
    GLuint i, id = 0;
    struct radeon_renderbuffer *rrb;

    if (ctx->NewState)
        _mesa_update_state( ctx );

    if (check_fallbacks(ctx))
	    return GL_FALSE;

    _tnl_UpdateFixedFunctionProgram(ctx);
    r700SetVertexFormat(ctx, arrays, max_index + 1);
    /* shaders need to be updated before buffers are validated */
    r700UpdateShaders(ctx);
    if (!r600ValidateBuffers(ctx))
	    return GL_FALSE;

    /* always emit CB base to prevent
     * lock ups on some chips.
     */
    R600_STATECHANGE(context, cb_target);
    /* mark vtx as dirty since it changes per-draw */
    R600_STATECHANGE(context, vtx);

    r700SetScissor(context);
    r700SetupVertexProgram(ctx);
    r700SetupFragmentProgram(ctx);
    r700UpdateShaderStates(ctx);

    GLuint emit_end = r700PredictRenderSize(ctx, prim, ib, nr_prims)
                    + context->radeon.cmdbuf.cs->cdw;

    r700SetupIndexBuffer(ctx, ib);
    r700SetupStreams(ctx, arrays, max_index + 1);

    radeonEmitState(radeon);

    radeon_debug_add_indent();
    for (i = 0; i < nr_prims; ++i)
    {
	    if (context->ind_buf.bo)
		    r700RunRenderPrimitive(ctx,
					   prim[i].start,
					   prim[i].start + prim[i].count,
					   prim[i].mode);
	    else
		    r700RunRenderPrimitiveImmediate(ctx,
						    prim[i].start,
						    prim[i].start + prim[i].count,
						    prim[i].mode);
    }
    radeon_debug_remove_indent();

    /* Flush render op cached for last several quads. */
    /* XXX drm should handle this in fence submit */
    r700WaitForIdleClean(context);

    rrb = radeon_get_colorbuffer(&context->radeon);
    if (rrb && rrb->bo)
	    r700SyncSurf(context, rrb->bo, 0, RADEON_GEM_DOMAIN_VRAM,
			 CB_ACTION_ENA_bit | (1 << (id + 6)));

    rrb = radeon_get_depthbuffer(&context->radeon);
    if (rrb && rrb->bo)
	    r700SyncSurf(context, rrb->bo, 0, RADEON_GEM_DOMAIN_VRAM,
			 DB_ACTION_ENA_bit | DB_DEST_BASE_ENA_bit);

    r700FreeData(ctx);

    if (emit_end < context->radeon.cmdbuf.cs->cdw)
    {
        WARN_ONCE("Rendering was %d commands larger than predicted size."
            " We might overflow  command buffer.\n", context->radeon.cmdbuf.cs->cdw - emit_end);
    }

    return GL_TRUE;
}

static void r700DrawPrims(GLcontext *ctx,
			  const struct gl_client_array *arrays[],
			  const struct _mesa_prim *prim,
			  GLuint nr_prims,
			  const struct _mesa_index_buffer *ib,
			  GLboolean index_bounds_valid,
			  GLuint min_index,
			  GLuint max_index)
{
	GLboolean retval = GL_FALSE;

	/* This check should get folded into just the places that
	 * min/max index are really needed.
	 */
	if (!index_bounds_valid) {
		vbo_get_minmax_index(ctx, prim, ib, &min_index, &max_index);
	}

	if (min_index) {
		vbo_rebase_prims( ctx, arrays, prim, nr_prims, ib, min_index, max_index, r700DrawPrims );
		return;
	}

	/* Make an attempt at drawing */
	retval = r700TryDrawPrims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);

	/* If failed run tnl pipeline - it should take care of fallbacks */
	if (!retval) {
		_swsetup_Wakeup(ctx);
		_tnl_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
	}
}

void r700InitDraw(GLcontext *ctx)
{
	struct vbo_context *vbo = vbo_context(ctx);

	/* to be enabled */
	vbo->draw_prims = r700DrawPrims;
}


