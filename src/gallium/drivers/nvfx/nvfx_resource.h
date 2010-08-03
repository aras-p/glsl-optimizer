
#ifndef NVFX_RESOURCE_H
#define NVFX_RESOURCE_H

#include "util/u_transfer.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include <nouveau/nouveau_bo.h>

struct pipe_resource;


/* This gets further specialized into either buffer or texture
 * structures.  In the future we'll want to remove much of that
 * distinction, but for now try to keep as close to the existing code
 * as possible and use the vtbl struct to choose between the two
 * underlying implementations.
 */
struct nvfx_resource {
	struct pipe_resource base;
	struct u_resource_vtbl *vtbl;
	struct nouveau_bo *bo;
};

#define NVFX_RESOURCE_FLAG_LINEAR (PIPE_RESOURCE_FLAG_DRV_PRIV << 0)

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

struct nvfx_miptree {
        struct nvfx_resource base;

        unsigned linear_pitch; /* for linear textures, 0 for swizzled and compressed textures with level-dependent minimal pitch */
        unsigned face_size; /* 128-byte aligned face/total size */
        unsigned level_offset[NVFX_MAX_TEXTURE_LEVELS];
};

struct nvfx_surface {
	struct pipe_surface base;
	unsigned pitch;
};

static INLINE 
struct nvfx_resource *nvfx_resource(struct pipe_resource *resource)
{
	return (struct nvfx_resource *)resource;
}

static INLINE struct nouveau_bo *
nvfx_surface_buffer(struct pipe_surface *surf)
{
	struct nvfx_resource *mt = nvfx_resource(surf->texture);

	return mt->bo;
}


void
nvfx_init_resource_functions(struct pipe_context *pipe);

void
nvfx_screen_init_resource_functions(struct pipe_screen *pscreen);


/* Internal:
 */

struct pipe_resource *
nvfx_miptree_create(struct pipe_screen *pscreen, const struct pipe_resource *pt);

struct pipe_resource *
nvfx_miptree_from_handle(struct pipe_screen *pscreen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle);

struct pipe_resource *
nvfx_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template);

struct pipe_resource *
nvfx_user_buffer_create(struct pipe_screen *screen,
			void *ptr,
			unsigned bytes,
			unsigned usage);



void
nvfx_miptree_surface_del(struct pipe_surface *ps);

struct pipe_surface *
nvfx_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_resource *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags);

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

#endif
