#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "nouveau/nouveau_winsys.h"
#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"
#include "nvfx_transfer.h"

struct nvfx_transfer {
	struct pipe_transfer base;
	struct pipe_surface *surface;
	boolean direct;
};

static void
nvfx_compatible_transfer_tex(struct pipe_resource *pt, unsigned width, unsigned height,
			     unsigned bind,
                             struct pipe_resource *template)
{
	memset(template, 0, sizeof(struct pipe_resource));
	template->target = pt->target;
	template->format = pt->format;
	template->width0 = width;
	template->height0 = height;
	template->depth0 = 1;
	template->last_level = 0;
	template->nr_samples = pt->nr_samples;
	template->bind = bind;
	template->usage = PIPE_USAGE_DYNAMIC;
	template->flags = NVFX_RESOURCE_FLAG_LINEAR;
}


static unsigned nvfx_transfer_bind_flags( unsigned transfer_usage )
{
	unsigned bind = 0;

#if 0
	if (transfer_usage & PIPE_TRANSFER_WRITE)
		bind |= PIPE_BIND_BLIT_SOURCE;

	if (transfer_usage & PIPE_TRANSFER_READ)
		bind |= PIPE_BIND_BLIT_DESTINATION;
#endif

	return bind;
}

struct pipe_transfer *
nvfx_miptree_transfer_new(struct pipe_context *pipe,
			  struct pipe_resource *pt,
			  struct pipe_subresource sr,
			  unsigned usage,
			  const struct pipe_box *box)
{
	struct pipe_screen *pscreen = pipe->screen;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	struct nvfx_transfer *tx;
	struct pipe_resource tx_tex_template, *tx_tex;
	static int no_transfer = -1;
	unsigned bind = nvfx_transfer_bind_flags(usage);
	if(no_transfer < 0)
		no_transfer = debug_get_bool_option("NOUVEAU_NO_TRANSFER", FALSE);


	tx = CALLOC_STRUCT(nvfx_transfer);
	if (!tx)
		return NULL;

	/* Don't handle 3D transfers yet.
	 */
	assert(box->depth == 1);

	pipe_resource_reference(&tx->base.resource, pt);
	tx->base.sr = sr;
	tx->base.usage = usage;
	tx->base.box = *box;
	tx->base.stride = mt->level[sr.level].pitch;

	/* Direct access to texture */
	if ((pt->usage == PIPE_USAGE_DYNAMIC ||
	     no_transfer) &&
	    pt->flags & NVFX_RESOURCE_FLAG_LINEAR)
	{
		tx->direct = true;

		/* XXX: just call the internal nvfx function.  
		 */
		tx->surface = pscreen->get_tex_surface(pscreen, pt,
	                                               sr.face, sr.level,
						       box->z,
	                                               bind);
		return &tx->base;
	}

	tx->direct = false;

	nvfx_compatible_transfer_tex(pt, box->width, box->height, bind, &tx_tex_template);

	tx_tex = pscreen->resource_create(pscreen, &tx_tex_template);
	if (!tx_tex)
	{
		FREE(tx);
		return NULL;
	}

	tx->base.stride = ((struct nvfx_miptree*)tx_tex)->level[0].pitch;

	tx->surface = pscreen->get_tex_surface(pscreen, tx_tex,
	                                       0, 0, 0,
	                                       bind);

	pipe_resource_reference(&tx_tex, NULL);

	if (!tx->surface)
	{
		pipe_surface_reference(&tx->surface, NULL);
		FREE(tx);
		return NULL;
	}

	if (usage & PIPE_TRANSFER_READ) {
		struct nvfx_screen *nvscreen = nvfx_screen(pscreen);
		struct pipe_surface *src;

		src = pscreen->get_tex_surface(pscreen, pt,
	                                       sr.face, sr.level, box->z,
	                                       0 /*PIPE_BIND_BLIT_SOURCE*/);

		/* TODO: Check if SIFM can deal with x,y,w,h when swizzling */
		/* TODO: Check if SIFM can un-swizzle */
		nvscreen->eng2d->copy(nvscreen->eng2d,
		                      tx->surface, 0, 0,
		                      src,
				      box->x, box->y,
		                      box->width, box->height);

		pipe_surface_reference(&src, NULL);
	}

	return &tx->base;
}

void
nvfx_miptree_transfer_del(struct pipe_context *pipe,
			  struct pipe_transfer *ptx)
{
	struct nvfx_transfer *tx = (struct nvfx_transfer *)ptx;

	if (!tx->direct && (ptx->usage & PIPE_TRANSFER_WRITE)) {
		struct pipe_screen *pscreen = pipe->screen;
		struct nvfx_screen *nvscreen = nvfx_screen(pscreen);
		struct pipe_surface *dst;

		dst = pscreen->get_tex_surface(pscreen,
					       ptx->resource,
	                                       ptx->sr.face,
					       ptx->sr.level,
					       ptx->box.z,
	                                       0 /*PIPE_BIND_BLIT_DESTINATION*/);

		/* TODO: Check if SIFM can deal with x,y,w,h when swizzling */
		nvscreen->eng2d->copy(nvscreen->eng2d,
		                      dst, ptx->box.x, ptx->box.y,
		                      tx->surface, 0, 0,
		                      ptx->box.width, ptx->box.height);

		pipe_surface_reference(&dst, NULL);
	}

	pipe_surface_reference(&tx->surface, NULL);
	pipe_resource_reference(&ptx->resource, NULL);
	FREE(ptx);
}

void *
nvfx_miptree_transfer_map(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	struct pipe_screen *pscreen = pipe->screen;
	struct nvfx_transfer *tx = (struct nvfx_transfer *)ptx;
	struct nv04_surface *ns = (struct nv04_surface *)tx->surface;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->surface->texture;
	uint8_t *map = nouveau_screen_bo_map(pscreen, mt->base.bo,
					     nouveau_screen_transfer_flags(ptx->usage));

	if(!tx->direct)
		return map + ns->base.offset;
	else
		return (map + ns->base.offset + 
			ptx->box.y * ns->pitch + 
			ptx->box.x * util_format_get_blocksize(ptx->resource->format));
}

void
nvfx_miptree_transfer_unmap(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	struct pipe_screen *pscreen = pipe->screen;
	struct nvfx_transfer *tx = (struct nvfx_transfer *)ptx;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->surface->texture;

	nouveau_screen_bo_unmap(pscreen, mt->base.bo);
}
