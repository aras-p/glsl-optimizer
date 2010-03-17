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

struct nvfx_transfer {
	struct pipe_transfer base;
	struct pipe_surface *surface;
	boolean direct;
};

static void
nvfx_compatible_transfer_tex(struct pipe_texture *pt, unsigned width, unsigned height,
                             struct pipe_texture *template)
{
	memset(template, 0, sizeof(struct pipe_texture));
	template->target = pt->target;
	template->format = pt->format;
	template->width0 = width;
	template->height0 = height;
	template->depth0 = 1;
	template->last_level = 0;
	template->nr_samples = pt->nr_samples;

	template->tex_usage = PIPE_TEXTURE_USAGE_DYNAMIC |
	                      NOUVEAU_TEXTURE_USAGE_LINEAR;
}

static struct pipe_transfer *
nvfx_transfer_new(struct pipe_context *pcontext, struct pipe_texture *pt,
		  unsigned face, unsigned level, unsigned zslice,
		  enum pipe_transfer_usage usage,
		  unsigned x, unsigned y, unsigned w, unsigned h)
{
        struct pipe_screen *pscreen = pcontext->screen;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	struct nvfx_transfer *tx;
	struct pipe_texture tx_tex_template, *tx_tex;
	static int no_transfer = -1;
	if(no_transfer < 0)
		no_transfer = debug_get_bool_option("NOUVEAU_NO_TRANSFER", TRUE/*XXX:FALSE*/);

	tx = CALLOC_STRUCT(nvfx_transfer);
	if (!tx)
		return NULL;

	pipe_texture_reference(&tx->base.texture, pt);
	tx->base.x = x;
	tx->base.y = y;
	tx->base.width = w;
	tx->base.height = h;
	tx->base.stride = mt->level[level].pitch;
	tx->base.usage = usage;
	tx->base.face = face;
	tx->base.level = level;
	tx->base.zslice = zslice;

	/* Direct access to texture */
	if ((pt->tex_usage & PIPE_TEXTURE_USAGE_DYNAMIC || no_transfer) &&
	    pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR)
	{
		tx->direct = true;
		tx->surface = pscreen->get_tex_surface(pscreen, pt,
	                                               face, level, zslice,
	                                               pipe_transfer_buffer_flags(&tx->base));
		return &tx->base;
	}

	tx->direct = false;

	nvfx_compatible_transfer_tex(pt, w, h, &tx_tex_template);

	tx_tex = pscreen->texture_create(pscreen, &tx_tex_template);
	if (!tx_tex)
	{
		FREE(tx);
		return NULL;
	}

	tx->base.stride = ((struct nvfx_miptree*)tx_tex)->level[0].pitch;

	tx->surface = pscreen->get_tex_surface(pscreen, tx_tex,
	                                       0, 0, 0,
	                                       pipe_transfer_buffer_flags(&tx->base));

	pipe_texture_reference(&tx_tex, NULL);

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
	                                       face, level, zslice,
	                                       PIPE_BUFFER_USAGE_GPU_READ);

		/* TODO: Check if SIFM can deal with x,y,w,h when swizzling */
		/* TODO: Check if SIFM can un-swizzle */
		nvscreen->eng2d->copy(nvscreen->eng2d,
		                      tx->surface, 0, 0,
		                      src, x, y,
		                      w, h);

		pipe_surface_reference(&src, NULL);
	}

	return &tx->base;
}

static void
nvfx_transfer_del(struct pipe_context *pcontext,
                  struct pipe_transfer *ptx)
{
	struct nvfx_transfer *tx = (struct nvfx_transfer *)ptx;

	if (!tx->direct && (ptx->usage & PIPE_TRANSFER_WRITE)) {
		struct pipe_screen *pscreen = pcontext->screen;
		struct nvfx_screen *nvscreen = nvfx_screen(pscreen);
		struct pipe_surface *dst;

		dst = pscreen->get_tex_surface(pscreen, ptx->texture,
	                                       ptx->face, ptx->level, ptx->zslice,
	                                       PIPE_BUFFER_USAGE_GPU_WRITE | NOUVEAU_BUFFER_USAGE_NO_RENDER);

		/* TODO: Check if SIFM can deal with x,y,w,h when swizzling */
		nvscreen->eng2d->copy(nvscreen->eng2d,
		                      dst, tx->base.x, tx->base.y,
		                      tx->surface, 0, 0,
		                      tx->base.width, tx->base.height);

		pipe_surface_reference(&dst, NULL);
	}

	pipe_surface_reference(&tx->surface, NULL);
	pipe_texture_reference(&ptx->texture, NULL);
	FREE(ptx);
}

static void *
nvfx_transfer_map(struct pipe_context *pcontext, struct pipe_transfer *ptx)
{
        struct pipe_screen *pscreen = pcontext->screen;
	struct nvfx_transfer *tx = (struct nvfx_transfer *)ptx;
	struct nv04_surface *ns = (struct nv04_surface *)tx->surface;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->surface->texture;
	void *map = pipe_buffer_map(pscreen, mt->buffer,
	                            pipe_transfer_buffer_flags(ptx));

	if(!tx->direct)
		return map + ns->base.offset;
	else
		return map + ns->base.offset + ptx->y * ns->pitch + ptx->x * util_format_get_blocksize(ptx->texture->format);
}

static void
nvfx_transfer_unmap(struct pipe_context *pcontext, struct pipe_transfer *ptx)
{
	struct pipe_screen *pscreen = pcontext->screen;
	struct nvfx_transfer *tx = (struct nvfx_transfer *)ptx;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->surface->texture;

	pipe_buffer_unmap(pscreen, mt->buffer);
}

void
nvfx_init_transfer_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.get_tex_transfer = nvfx_transfer_new;
	nvfx->pipe.tex_transfer_destroy = nvfx_transfer_del;
	nvfx->pipe.transfer_map = nvfx_transfer_map;
	nvfx->pipe.transfer_unmap = nvfx_transfer_unmap;
}
