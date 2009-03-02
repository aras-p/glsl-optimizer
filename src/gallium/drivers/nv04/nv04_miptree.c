#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "nv04_context.h"
#include "nv04_screen.h"

static void
nv04_miptree_layout(struct nv04_miptree *nv04mt)
{
	struct pipe_texture *pt = &nv04mt->base;
	uint width = pt->width[0], height = pt->height[0];
	uint offset = 0;
	int nr_faces, l;

	nr_faces = 1;

	for (l = 0; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;

		pt->nblocksx[l] = pf_get_nblocksx(&pt->block, width);
		pt->nblocksy[l] = pf_get_nblocksy(&pt->block, height);
		
		nv04mt->level[l].pitch = pt->width[0];
		nv04mt->level[l].pitch = (nv04mt->level[l].pitch + 63) & ~63;

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
	}

	for (l = 0; l <= pt->last_level; l++) {

		nv04mt->level[l].image_offset = offset;
		offset += nv04mt->level[l].pitch * pt->height[l];
	}

	nv04mt->total_size = offset;
}

static struct pipe_texture *
nv04_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *pt)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv04_miptree *mt;

	mt = MALLOC(sizeof(struct nv04_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = pscreen;

	//mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;

	nv04_miptree_layout(mt);

	mt->buffer = ws->buffer_create(ws, 256, PIPE_BUFFER_USAGE_PIXEL |
						NOUVEAU_BUFFER_USAGE_TEXTURE,
						mt->total_size);
	if (!mt->buffer) {
		printf("failed %d byte alloc\n",mt->total_size);
		FREE(mt);
		return NULL;
	}
	
	return &mt->base;
}

static struct pipe_texture *
nv04_miptree_blanket(struct pipe_screen *pscreen, const struct pipe_texture *pt,
		     const unsigned *stride, struct pipe_buffer *pb)
{
	struct nv04_miptree *mt;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (pt->target != PIPE_TEXTURE_2D || pt->last_level != 0 ||
	    pt->depth[0] != 1)
		return NULL;

	mt = CALLOC_STRUCT(nv04_miptree);
	if (!mt)
		return NULL;

	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = pscreen;
	mt->level[0].pitch = stride[0];
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));

	pipe_buffer_reference(pscreen, &mt->buffer, pb);
	return &mt->base;
}

static void
nv04_miptree_release(struct pipe_screen *pscreen, struct pipe_texture **ppt)
{
	struct pipe_texture *pt = *ppt;
	struct nv04_miptree *mt = (struct nv04_miptree *)pt;
	int l;

	*ppt = NULL;
	if (--pt->refcount)
		return;

	pipe_buffer_reference(pscreen, &mt->buffer, NULL);
	for (l = 0; l <= pt->last_level; l++) {
		if (mt->level[l].image_offset)
			FREE(mt->level[l].image_offset);
	}

	FREE(mt);
}

static struct pipe_surface *
nv04_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv04_miptree *nv04mt = (struct nv04_miptree *)pt;
	struct nv04_surface *ns;

	ns = CALLOC_STRUCT(nv04_surface);
	if (!ns)
		return NULL;
	pipe_texture_reference(&ns->base.texture, pt);
	ns->base.format = pt->format;
	ns->base.width = pt->width[level];
	ns->base.height = pt->height[level];
	ns->base.usage = flags;
	ns->base.status = PIPE_SURFACE_STATUS_DEFINED;
	ns->base.refcount = 1;
	ns->base.face = face;
	ns->base.level = level;
	ns->base.zslice = zslice;
	ns->pitch = nv04mt->level[level].pitch;

	ns->base.offset = nv04mt->level[level].image_offset;

	return &ns->base;
}

static void
nv04_miptree_surface_del(struct pipe_screen *pscreen,
			 struct pipe_surface **psurface)
{
	struct pipe_surface *ps = *psurface;

	*psurface = NULL;
	if (--ps->refcount > 0)
		return;

	pipe_texture_reference(&ps->texture, NULL);
	FREE(ps);
}

void
nv04_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv04_miptree_create;
	pscreen->texture_blanket = nv04_miptree_blanket;
	pscreen->texture_release = nv04_miptree_release;
	pscreen->get_tex_surface = nv04_miptree_surface_new;
	pscreen->tex_surface_release = nv04_miptree_surface_del;
}

