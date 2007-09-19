#include "mtypes.h"

#include "nouveau_context.h"
#include "nouveau_fifo.h"
#include "nouveau_mem.h"
#include "nouveau_msg.h"
#include "nouveau_object.h"
#include "nouveau_reg.h"

#define MAX_MEMFMT_LENGTH 32768

/* Unstrided blit using NV_MEMORY_TO_MEMORY_FORMAT */
GLboolean
nouveau_memformat_flat_emit(GLcontext * ctx,
			    nouveau_mem * dst, nouveau_mem * src,
			    GLuint dst_offset, GLuint src_offset,
			    GLuint size)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	uint32_t src_handle, dst_handle;
	GLuint count;

	if (src_offset + size > src->size) {
		MESSAGE("src out of nouveau_mem bounds\n");
		return GL_FALSE;
	}
	if (dst_offset + size > dst->size) {
		MESSAGE("dst out of nouveau_mem bounds\n");
		return GL_FALSE;
	}

	src_handle = (src->type & NOUVEAU_MEM_FB) ? NvDmaFB : NvDmaTT;
	dst_handle = (dst->type & NOUVEAU_MEM_FB) ? NvDmaFB : NvDmaTT;
	src_offset += nouveau_mem_gpu_offset_get(ctx, src);
	dst_offset += nouveau_mem_gpu_offset_get(ctx, dst);

	BEGIN_RING_SIZE(NvSubMemFormat,
			NV_MEMORY_TO_MEMORY_FORMAT_OBJECT_IN, 2);
	OUT_RING(src_handle);
	OUT_RING(dst_handle);

	count = (size / MAX_MEMFMT_LENGTH) + 
		((size % MAX_MEMFMT_LENGTH) ? 1 : 0);

	while (count--) {
		GLuint length =
		    (size > MAX_MEMFMT_LENGTH) ? MAX_MEMFMT_LENGTH : size;

		BEGIN_RING_SIZE(NvSubMemFormat,
				NV_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
		OUT_RING(src_offset);
		OUT_RING(dst_offset);
		OUT_RING(0);	/* pitch in */
		OUT_RING(0);	/* pitch out */
		OUT_RING(length);	/* line length */
		OUT_RING(1);	/* number of lines */
		OUT_RING((1 << 8) /* dst_inc */ |(1 << 0) /* src_inc */ );
		OUT_RING(0);	/* buffer notify? */
		FIRE_RING();

		src_offset += length;
		dst_offset += length;
		size -= length;
	}

	return GL_TRUE;
}

void nouveau_mem_free(GLcontext * ctx, nouveau_mem * mem)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	struct drm_nouveau_mem_free memf;

	if (NOUVEAU_DEBUG & DEBUG_MEM) {
		fprintf(stderr, "%s: type=0x%x, offset=0x%x, size=0x%x\n",
			__func__, mem->type, (GLuint) mem->offset,
			(GLuint) mem->size);
	}

	if (mem->map)
		drmUnmap(mem->map, mem->size);
	memf.flags = mem->type;
	memf.offset = mem->offset;
	drmCommandWrite(nmesa->driFd, DRM_NOUVEAU_MEM_FREE, &memf,
			sizeof(memf));
	FREE(mem);
}

nouveau_mem *nouveau_mem_alloc(GLcontext *ctx, uint32_t flags, GLuint size,
			       GLuint align)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	struct drm_nouveau_mem_alloc mema;
	nouveau_mem *mem;
	int ret;

	if (NOUVEAU_DEBUG & DEBUG_MEM) {
		fprintf(stderr,
			"%s: requested: flags=0x%x, size=0x%x, align=0x%x\n",
			__func__, flags, (GLuint) size, align);
	}

	mem = CALLOC(sizeof(nouveau_mem));
	if (!mem)
		return NULL;

	mema.flags = flags;
	mema.size = mem->size = size;
	mema.alignment = align;
	mem->map = NULL;
	ret = drmCommandWriteRead(nmesa->driFd, DRM_NOUVEAU_MEM_ALLOC,
				  &mema, sizeof(mema));
	if (ret) {
		FREE(mem);
		return NULL;
	}
	mem->offset = mema.offset;
	mem->type = mema.flags;

	if (NOUVEAU_DEBUG & DEBUG_MEM) {
		fprintf(stderr,
			"%s: actual: type=0x%x, offset=0x%x, size=0x%x\n",
			__func__, mem->type, (GLuint) mem->offset,
			(GLuint) mem->size);
	}

	if (flags & NOUVEAU_MEM_MAPPED)
		ret = drmMap(nmesa->driFd, mema.map_handle, mem->size,
			     &mem->map);
	if (ret) {
		mem->map = NULL;
		nouveau_mem_free(ctx, mem);
		mem = NULL;
	}

	return mem;
}

uint32_t nouveau_mem_gpu_offset_get(GLcontext * ctx, nouveau_mem * mem)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	return mem->offset;
}
