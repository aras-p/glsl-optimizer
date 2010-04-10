
#ifndef NVFX_RESOURCE_H
#define NVFX_RESOURCE_H

#include "util/u_transfer.h"

struct pipe_resource;
struct nouveau_bo;


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

#define NVFX_MAX_TEXTURE_LEVELS  16

struct nvfx_miptree {
	struct nvfx_resource base;
	uint total_size;

	struct {
		uint pitch;
		uint *image_offset;
	} level[NVFX_MAX_TEXTURE_LEVELS];

	unsigned image_nr;
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


#endif
