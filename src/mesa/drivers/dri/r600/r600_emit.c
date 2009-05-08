/**************************************************************************

Copyright 2008, 2009 Advanced Micro Devices Inc. (AMD)

Copyright (C) Advanced Micro Devices Inc. (AMD)  2009.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/image.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r600_context.h"
#include "r600_emit.h"

#if defined(USE_X86_ASM)
#define COPY_DWORDS( dst, src, nr )					\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (__tmp), "=D" (dst), "=S" (__tmp)	\
			      : "0" (nr),				\
			        "D" ((long)dst),			\
			        "S" ((long)src) );			\
} while (0)
#else
#define COPY_DWORDS( dst, src, nr )		\
do {						\
   int j;					\
   for ( j = 0 ; j < nr ; j++ )			\
      dst[j] = ((int *)src)[j];			\
   dst += nr;					\
} while (0)
#endif

static void r600EmitVec4(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 4)
		COPY_DWORDS(out, data, count);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out++;
			data += stride;
		}
}

static void r600EmitVec8(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 8)
		COPY_DWORDS(out, data, count * 2);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out += 2;
			data += stride;
		}
}

static void r600EmitVec12(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 12) {
		COPY_DWORDS(out, data, count * 3);
    }
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out += 3;
			data += stride;
		}
}

static void r600EmitVec16(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 16)
		COPY_DWORDS(out, data, count * 4);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out[3] = *(int *)(data + 12);
			out += 4;
			data += stride;
		}
}

/* Emit vertex data to GART memory
 * Route inputs to the vertex processor
 * This function should never return R600_FALLBACK_TCL when using software tcl.
 */
int r600EmitArrays(GLcontext * ctx)
{
	
	return R600_FALLBACK_NONE;
}

void r600EmitCacheFlush(r600ContextPtr rmesa)
{
	BATCH_LOCALS(&rmesa->radeon);
/*
	BEGIN_BATCH_NO_AUTOSTATE(4);
	OUT_BATCH_REGVAL(R600_RB3D_DSTCACHE_CTLSTAT,
		R600_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
		R600_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
	OUT_BATCH_REGVAL(R600_ZB_ZCACHE_CTLSTAT,
		R600_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
		R600_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);
	END_BATCH();
	COMMIT_BATCH();
*/
}

GLboolean r600EmitShader(GLcontext * ctx, 
                         void ** shaderbo,
			             GLvoid * data, 
                         int sizeinDWORD) 
{
    radeonContextPtr radeonctx = RADEON_CONTEXT(ctx);

    struct radeon_bo * pbo;
    uint32_t *out;

shader_again_alloc:	
	pbo = radeon_bo_open(radeonctx->radeonScreen->bom,
					     0, 
                         sizeinDWORD * 4, 
                         256, 
                         RADEON_GEM_DOMAIN_GTT,
					     0);

	if (!pbo) 
    {
		rcommonFlushCmdBuf(radeonctx, __FUNCTION__);
		goto shader_again_alloc;
	}

	radeon_validate_bo(radeonctx, pbo, RADEON_GEM_DOMAIN_GTT, 0);

	if (radeon_revalidate_bos(radeonctx->glCtx) == GL_FALSE)
    {
	    fprintf(stderr,"failure to revalidate BOs - badness\n");
    }
	  
	radeon_bo_map(pbo, 1);

    radeon_bo_ref(pbo);

    out = (uint32_t*)(pbo->ptr);

    memcpy(out, data, sizeinDWORD * 4);

    *shaderbo = (void*)pbo;

    return GL_TRUE;
}

GLboolean r600DeleteShader(GLcontext * ctx, 
                           void * shaderbo) 
{
    struct radeon_bo * pbo = (struct radeon_bo *)shaderbo;

    radeon_bo_unmap(pbo);
    radeon_bo_unref(pbo); /* when bo->cref <= 0, bo will be bo_free */

    return GL_TRUE;
}

GLboolean r600EmitVec(GLcontext * ctx, 
                      struct radeon_aos *aos,
			          GLvoid * data, 
                      int size, 
                      int stride, 
                      int count)
{
    radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	uint32_t *out;

	if (stride == 0) 
    {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * count * 4, 32);
		aos->stride = 0;
	} 
    else 
    {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * count * 4, 32);
		aos->stride = size;
	}

	aos->components = size;
	aos->count = count;

	out = (uint32_t*)((char*)aos->bo->ptr + aos->offset);
	switch (size) {
	case 1: r600EmitVec4(out, data, stride, count); break;
	case 2: r600EmitVec8(out, data, stride, count); break;
	case 3: r600EmitVec12(out, data, stride, count); break;
	case 4: r600EmitVec16(out, data, stride, count); break;
	default:
		assert(0);
		break;
	}

    return GL_TRUE;
}

void r600ReleaseVec(GLcontext * ctx)
{
    radeonReleaseArrays(ctx, ~0);
}

void r600FreeDmaRegion(context_t *context, 
                       void * shaderbo)
{
    struct radeon_bo *pbo = (struct radeon_bo *)shaderbo;
    if(pbo) 
    {
        radeon_bo_unref(pbo);
    }
}
