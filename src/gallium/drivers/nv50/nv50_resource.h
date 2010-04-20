
#ifndef NV50_RESOURCE_H
#define NV50_RESOURCE_H

#include "util/u_transfer.h"

#include "nouveau/nouveau_winsys.h"

struct pipe_resource;
struct nouveau_bo;


/* This gets further specialized into either buffer or texture
 * structures.  In the future we'll want to remove much of that
 * distinction, but for now try to keep as close to the existing code
 * as possible and use the vtbl struct to choose between the two
 * underlying implementations.
 */
struct nv50_resource {
	struct pipe_resource base;
	const struct u_resource_vtbl *vtbl;
	struct nouveau_bo *bo;
};

struct nv50_miptree_level {
	int *image_offset;
	unsigned pitch;
	unsigned tile_mode;
};

#define NV50_MAX_TEXTURE_LEVELS 16

struct nv50_miptree {
	struct nv50_resource base;

	struct nv50_miptree_level level[NV50_MAX_TEXTURE_LEVELS];
	int image_nr;
	int total_size;
};

static INLINE struct nv50_miptree *
nv50_miptree(struct pipe_resource *pt)
{
	return (struct nv50_miptree *)pt;
}


static INLINE 
struct nv50_resource *nv50_resource(struct pipe_resource *resource)
{
	return (struct nv50_resource *)resource;
}

/* is resource mapped into the GPU's address space (i.e. VRAM or GART) ? */
static INLINE boolean
nv50_resource_mapped_by_gpu(struct pipe_resource *resource)
{
   return nv50_resource(resource)->bo->handle;
}

void
nv50_init_resource_functions(struct pipe_context *pcontext);

void
nv50_screen_init_resource_functions(struct pipe_screen *pscreen);

/* Internal functions
 */
struct pipe_resource *
nv50_miptree_create(struct pipe_screen *pscreen,
		    const struct pipe_resource *tmp);

struct pipe_resource *
nv50_miptree_from_handle(struct pipe_screen *pscreen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle);

struct pipe_resource *
nv50_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template);

struct pipe_resource *
nv50_user_buffer_create(struct pipe_screen *screen,
			void *ptr,
			unsigned bytes,
			unsigned usage);


struct pipe_surface *
nv50_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_resource *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags);

void
nv50_miptree_surface_del(struct pipe_surface *ps);


#endif
