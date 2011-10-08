#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

#include "pipe/p_screen.h"
#include "util/u_memory.h"
typedef uint32_t u32;

extern int nouveau_mesa_debug;

struct nouveau_bo;

struct nouveau_screen {
	struct pipe_screen base;
	struct nouveau_device *device;
	struct nouveau_channel *channel;

	/* note that OpenGL doesn't distinguish between these, so
	 * these almost always should be set to the same value */
	unsigned vertex_buffer_flags;
	unsigned index_buffer_flags;
	unsigned sysmem_bindings;

	struct {
		struct nouveau_fence *head;
		struct nouveau_fence *tail;
		struct nouveau_fence *current;
		u32 sequence;
		u32 sequence_ack;
		void (*emit)(struct pipe_screen *, u32 *sequence);
		u32  (*update)(struct pipe_screen *);
	} fence;

	struct nouveau_mman *mm_VRAM;
	struct nouveau_mman *mm_GART;
};

static INLINE struct nouveau_screen *
nouveau_screen(struct pipe_screen *pscreen)
{
	return (struct nouveau_screen *)pscreen;
}



/* Not really sure if this is needed, or whether the individual
 * drivers are happy to talk to the bo functions themselves.  In a way
 * this is what we'd expect from a regular winsys interface.
 */
struct nouveau_bo *
nouveau_screen_bo_new(struct pipe_screen *pscreen, unsigned alignment,
		      unsigned usage, unsigned bind, unsigned size);
void *
nouveau_screen_bo_map(struct pipe_screen *pscreen,
		      struct nouveau_bo *pb,
		      unsigned usage);
void *
nouveau_screen_bo_map_range(struct pipe_screen *pscreen, struct nouveau_bo *bo,
			    unsigned offset, unsigned length, unsigned usage);
void
nouveau_screen_bo_map_flush_range(struct pipe_screen *pscreen, struct nouveau_bo *bo,
				  unsigned offset, unsigned length);
void
nouveau_screen_bo_unmap(struct pipe_screen *pscreen, struct nouveau_bo *bo);
void
nouveau_screen_bo_release(struct pipe_screen *pscreen, struct nouveau_bo *bo);

boolean
nouveau_screen_bo_get_handle(struct pipe_screen *pscreen,
			     struct nouveau_bo *bo,
			     unsigned stride,
			     struct winsys_handle *whandle);
struct nouveau_bo *
nouveau_screen_bo_from_handle(struct pipe_screen *pscreen,
			      struct winsys_handle *whandle,
			      unsigned *out_stride);


int nouveau_screen_init(struct nouveau_screen *, struct nouveau_device *);
void nouveau_screen_fini(struct nouveau_screen *);

void nouveau_screen_init_vdec(struct nouveau_screen *);


#ifndef NOUVEAU_NVC0
static INLINE unsigned
RING_3D(unsigned mthd, unsigned size)
{
	return (7 << 13) | (size << 18) | mthd;
}

static INLINE unsigned
RING_3D_NI(unsigned mthd, unsigned size)
{
	return 0x40000000 | (7 << 13) | (size << 18) | mthd;
}
#endif

#endif
