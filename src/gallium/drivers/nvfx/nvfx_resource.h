#ifndef NVFX_RESOURCE_H
#define NVFX_RESOURCE_H

#include "util/u_transfer.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_double_list.h"
#include "util/u_surfaces.h"
#include "util/u_dirty_surfaces.h"
#include <nouveau/nouveau_bo.h>

struct pipe_resource;
struct nv04_region;

struct nvfx_resource {
	struct pipe_resource base;
	struct nouveau_bo *bo;
};

static INLINE
struct nvfx_resource *nvfx_resource(struct pipe_resource *resource)
{
	return (struct nvfx_resource *)resource;
}

#define NVFX_RESOURCE_FLAG_USER (NOUVEAU_RESOURCE_FLAG_DRV_PRIV << 0)

/* is resource mapped into the GPU's address space (i.e. VRAM or GART) ? */
static INLINE boolean
nvfx_resource_mapped_by_gpu(struct pipe_resource *resource)
{
   return nvfx_resource(resource)->bo->handle;
}

/* is resource in VRAM? */
static inline int
nvfx_resource_on_gpu(struct pipe_resource* pr)
{
#if 0
	// a compiler error here means you need to apply libdrm-nouveau-add-domain.patch to libdrm
	// TODO: return FALSE if not VRAM and on a PCI-E system
	return ((struct nvfx_resource*)pr)->bo->domain & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART);
#else
	return TRUE;
#endif
}

#define NVFX_MAX_TEXTURE_LEVELS  16

/* We have the following invariants for render temporaries
 *
 * 1. Render temporaries are always linear
 * 2. Render temporaries are always up to date
 * 3. Currently, render temporaries are destroyed when the resource is used for sampling, but kept for any other use
 *
 * Also, we do NOT flush temporaries on any pipe->flush().
 * This is fine, as long as scanout targets and shared resources never need temps.
 *
 * TODO: we may want to also support swizzled temporaries to improve performance in some cases.
 */

struct nvfx_miptree {
        struct nvfx_resource base;

        unsigned linear_pitch; /* for linear textures, 0 for swizzled and compressed textures with level-dependent minimal pitch */
        unsigned face_size; /* 128-byte aligned face/total size */
        unsigned level_offset[NVFX_MAX_TEXTURE_LEVELS];

        struct util_surfaces surfaces;
        struct util_dirty_surfaces dirty_surfaces;
};

struct nvfx_surface {
	struct util_dirty_surface base;
	unsigned pitch;
	unsigned offset;

	struct nvfx_miptree* temp;
};

static INLINE struct nouveau_bo *
nvfx_surface_buffer(struct pipe_surface *surf)
{
	struct nvfx_resource *mt = nvfx_resource(surf->texture);

	return mt->bo;
}

static INLINE struct util_dirty_surfaces*
nvfx_surface_get_dirty_surfaces(struct pipe_surface* surf)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)surf->texture;
	return &mt->dirty_surfaces;
}

void
nvfx_init_resource_functions(struct pipe_context *pipe);

void
nvfx_screen_init_resource_functions(struct pipe_screen *pscreen);


/* Internal:
 */

struct pipe_resource *
nvfx_miptree_create(struct pipe_screen *pscreen, const struct pipe_resource *pt);

void
nvfx_miptree_destroy(struct pipe_screen *pscreen,
                     struct pipe_resource *presource);

struct pipe_resource *
nvfx_miptree_from_handle(struct pipe_screen *pscreen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle);

void
nvfx_miptree_surface_del(struct pipe_context *pipe, struct pipe_surface *ps);

struct pipe_surface *
nvfx_miptree_surface_new(struct pipe_context *pipe, struct pipe_resource *pt,
			 const struct pipe_surface *surf_tmpl);

/* only for miptrees, don't use for buffers */

/* NOTE: for swizzled 3D textures, this just returns the offset of the mipmap level */
static inline unsigned
nvfx_subresource_offset(struct pipe_resource* pt, unsigned face, unsigned level, unsigned zslice)
{
	if(pt->target == PIPE_BUFFER)
		return 0;
	else
	{
		struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;

		unsigned offset = mt->level_offset[level];
		if (pt->target == PIPE_TEXTURE_CUBE)
			offset += mt->face_size * face;
		else if (pt->target == PIPE_TEXTURE_3D && mt->linear_pitch)
			offset += zslice * util_format_get_2d_size(pt->format, (mt->linear_pitch ? mt->linear_pitch : util_format_get_stride(pt->format, u_minify(pt->width0, level))),  u_minify(pt->height0, level));
		return offset;
	}
}

static inline unsigned
nvfx_subresource_pitch(struct pipe_resource* pt, unsigned level)
{
	if(pt->target == PIPE_BUFFER)
		return ((struct nvfx_resource*)pt)->bo->size;
	else
	{
		struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;

		if(mt->linear_pitch)
			return mt->linear_pitch;
		else
			return util_format_get_stride(pt->format, u_minify(pt->width0, level));
	}
}

void
nvfx_surface_create_temp(struct pipe_context* pipe, struct pipe_surface* surf);

void
nvfx_surface_flush(struct pipe_context* pipe, struct pipe_surface* surf);

struct nvfx_buffer
{
	struct nvfx_resource base;
	uint8_t* data;
	unsigned size;

	/* the range of data not yet uploaded to the GPU bo */
	unsigned dirty_begin;
	unsigned dirty_end;

	/* whether all transfers were unsynchronized */
	boolean dirty_unsynchronized;

	/* whether it would have been profitable to upload
	 * the latest updated data to the GPU immediately */
	boolean last_update_static;

	/* how many bytes we need to draw before we deem
	 * the buffer to be static
	 */
	long long bytes_to_draw_until_static;
};

static inline struct nvfx_buffer* nvfx_buffer(struct pipe_resource* pr)
{
	return (struct nvfx_buffer*)pr;
}

/* this is an heuristic to determine whether we are better off uploading the
 * buffer to the GPU, or just continuing pushing it on the FIFO
 */
static inline boolean nvfx_buffer_seems_static(struct nvfx_buffer* buffer)
{
	return buffer->last_update_static
		|| buffer->bytes_to_draw_until_static < 0;
}

struct pipe_resource *
nvfx_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template);

void
nvfx_buffer_destroy(struct pipe_screen *pscreen,
                    struct pipe_resource *presource);

struct pipe_resource *
nvfx_user_buffer_create(struct pipe_screen *screen,
			void *ptr,
			unsigned bytes,
			unsigned usage);

void
nvfx_buffer_upload(struct nvfx_buffer* buffer);

#endif
