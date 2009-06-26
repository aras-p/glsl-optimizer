/**************************************************************************

Copyright (C) 2004 Nicolai Haehnle.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "radeon_common.h"

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

static void radeonEmitVec4(uint32_t *out, GLvoid * data, int stride, int count)
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

void radeonEmitVec8(uint32_t *out, GLvoid * data, int stride, int count)
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

void radeonEmitVec12(uint32_t *out, GLvoid * data, int stride, int count)
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

static void radeonEmitVec16(uint32_t *out, GLvoid * data, int stride, int count)
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

void rcommon_emit_vector(GLcontext * ctx, struct radeon_aos *aos,
			 GLvoid * data, int size, int stride, int count)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	uint32_t *out;

	if (stride == 0) {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * 4, 32);
		count = 1;
		aos->stride = 0;
	} else {
		radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * count * 4, 32);
		aos->stride = size;
	}

	aos->components = size;
	aos->count = count;

	out = (uint32_t*)((char*)aos->bo->ptr + aos->offset);
	switch (size) {
	case 1: radeonEmitVec4(out, data, stride, count); break;
	case 2: radeonEmitVec8(out, data, stride, count); break;
	case 3: radeonEmitVec12(out, data, stride, count); break;
	case 4: radeonEmitVec16(out, data, stride, count); break;
	default:
		assert(0);
		break;
	}
}

void radeonRefillCurrentDmaRegion(radeonContextPtr rmesa, int size)
{

	size = MAX2(size, MAX_DMA_BUF_SZ);

	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA))
		fprintf(stderr, "%s %d\n", __FUNCTION__, rmesa->dma.nr_released_bufs);

	if (rmesa->dma.flush) {
		rmesa->dma.flush(rmesa->glCtx);
	}

	if (rmesa->dma.nr_released_bufs > 4) {
		rcommonFlushCmdBuf(rmesa, __FUNCTION__);
		rmesa->dma.nr_released_bufs = 0;
	}

	radeonReleaseDmaRegion(rmesa);

again_alloc:	
	rmesa->dma.current = radeon_bo_open(rmesa->radeonScreen->bom,
					    0, size, 4, RADEON_GEM_DOMAIN_GTT,
					    0);

	if (!rmesa->dma.current) {
		rcommonFlushCmdBuf(rmesa, __FUNCTION__);
		rmesa->dma.nr_released_bufs = 0;
		goto again_alloc;
	}

	rmesa->dma.current_used = 0;
	rmesa->dma.current_vertexptr = 0;
	
	radeon_validate_bo(rmesa, rmesa->dma.current, RADEON_GEM_DOMAIN_GTT, 0);

	if (radeon_revalidate_bos(rmesa->glCtx) == GL_FALSE)
	  fprintf(stderr,"failure to revalidate BOs - badness\n");

	if (!rmesa->dma.current) {
        /* Cmd buff have been flushed in radeon_revalidate_bos */
		rmesa->dma.nr_released_bufs = 0;
		goto again_alloc;
	}

	radeon_bo_map(rmesa->dma.current, 1);
}

/* Allocates a region from rmesa->dma.current.  If there isn't enough
 * space in current, grab a new buffer (and discard what was left of current)
 */
void radeonAllocDmaRegion(radeonContextPtr rmesa,
			  struct radeon_bo **pbo, int *poffset,
			  int bytes, int alignment)
{
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, bytes);

	if (rmesa->dma.flush)
		rmesa->dma.flush(rmesa->glCtx);

	assert(rmesa->dma.current_used == rmesa->dma.current_vertexptr);

	alignment--;
	rmesa->dma.current_used = (rmesa->dma.current_used + alignment) & ~alignment;

	if (!rmesa->dma.current || rmesa->dma.current_used + bytes > rmesa->dma.current->size)
		radeonRefillCurrentDmaRegion(rmesa, (bytes + 15) & ~15);

	*poffset = rmesa->dma.current_used;
	*pbo = rmesa->dma.current;
	radeon_bo_ref(*pbo);

	/* Always align to at least 16 bytes */
	rmesa->dma.current_used = (rmesa->dma.current_used + bytes + 15) & ~15;
	rmesa->dma.current_vertexptr = rmesa->dma.current_used;

	assert(rmesa->dma.current_used <= rmesa->dma.current->size);
}

void radeonReturnDmaRegion(radeonContextPtr rmesa, int return_bytes)
{
	if (!rmesa->dma.current)
		return;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, return_bytes);
	rmesa->dma.current_used -= return_bytes;
	rmesa->dma.current_vertexptr = rmesa->dma.current_used;
}

void radeonReleaseDmaRegion(radeonContextPtr rmesa)
{
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %p\n", __FUNCTION__, rmesa->dma.current);
	if (rmesa->dma.current) {
		rmesa->dma.nr_released_bufs++;
		radeon_bo_unmap(rmesa->dma.current);
	        radeon_bo_unref(rmesa->dma.current);
	}
	rmesa->dma.current = NULL;
}


/* Flush vertices in the current dma region.
 */
void rcommon_flush_last_swtcl_prim( GLcontext *ctx  )
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	struct radeon_dma *dma = &rmesa->dma;
		

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %p\n", __FUNCTION__, dma->current);
	dma->flush = NULL;

	if (dma->current) {
	    GLuint current_offset = dma->current_used;

	    assert (dma->current_used +
		    rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
		    dma->current_vertexptr);

	    if (dma->current_used != dma->current_vertexptr) {
		    dma->current_used = dma->current_vertexptr;

		    rmesa->vtbl.swtcl_flush(ctx, current_offset);
	    }
	    rmesa->swtcl.numverts = 0;
	}
}
/* Alloc space in the current dma region.
 */
void *
rcommonAllocDmaLowVerts( radeonContextPtr rmesa, int nverts, int vsize )
{
	GLuint bytes = vsize * nverts;
	void *head;
restart:
	if (!rmesa->dma.current || rmesa->dma.current_vertexptr + bytes > rmesa->dma.current->size) {
                radeonRefillCurrentDmaRegion(rmesa, bytes);
	}

        if (!rmesa->dma.flush) {
		/* make sure we have enough space to use this in cmdbuf */
   		rcommonEnsureCmdBufSpace(rmesa,
			      rmesa->hw.max_state_size + (12*sizeof(int)),
			      __FUNCTION__);
		/* if cmdbuf flushed DMA restart */
		if (!rmesa->dma.current)
			goto restart;
                rmesa->glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
                rmesa->dma.flush = rcommon_flush_last_swtcl_prim;
        }

	ASSERT( vsize == rmesa->swtcl.vertex_size * 4 );
        ASSERT( rmesa->dma.flush == rcommon_flush_last_swtcl_prim );
        ASSERT( rmesa->dma.current_used +
                rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
                rmesa->dma.current_vertexptr );

	head = (rmesa->dma.current->ptr + rmesa->dma.current_vertexptr);
	rmesa->dma.current_vertexptr += bytes;
	rmesa->swtcl.numverts += nverts;
	return head;
}

void radeonReleaseArrays( GLcontext *ctx, GLuint newinputs )
{
   radeonContextPtr radeon = RADEON_CONTEXT( ctx );
   int i;

   if (radeon->dma.flush) {
       radeon->dma.flush(radeon->glCtx);
   }
   if (radeon->tcl.elt_dma_bo) {
	   radeon_bo_unref(radeon->tcl.elt_dma_bo);
	   radeon->tcl.elt_dma_bo = NULL;
   }
   for (i = 0; i < radeon->tcl.aos_count; i++) {
      if (radeon->tcl.aos[i].bo) {
         radeon_bo_unref(radeon->tcl.aos[i].bo);
         radeon->tcl.aos[i].bo = NULL;
      }
   }
}
