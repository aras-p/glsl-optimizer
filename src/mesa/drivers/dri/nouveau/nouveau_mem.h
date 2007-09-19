#ifndef __NOUVEAU_MEM_H__
#define __NOUVEAU_MEM_H__

typedef struct nouveau_mem_t {
	int type;
	uint64_t offset;
	uint64_t size;
	void *map;
} nouveau_mem;

extern nouveau_mem *nouveau_mem_alloc(GLcontext *, uint32_t flags,
				      GLuint size, GLuint align);
extern void nouveau_mem_free(GLcontext *, nouveau_mem *);
extern uint32_t nouveau_mem_gpu_offset_get(GLcontext *, nouveau_mem *);

extern GLboolean nouveau_memformat_flat_emit(GLcontext *,
					     nouveau_mem *dst,
					     nouveau_mem *src,
					     GLuint dst_offset,
					     GLuint src_offset,
					     GLuint size);

#endif
