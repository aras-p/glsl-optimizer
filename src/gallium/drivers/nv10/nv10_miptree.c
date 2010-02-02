#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"

#include "nv10_context.h"
#include "nv10_screen.h"

static void
nv10_miptree_layout(struct nv10_miptree *nv10mt)
{
	struct pipe_texture *pt = &nv10mt->base;
	boolean swizzled = FALSE;
	uint width = pt->width0;
	uint offset = 0;
	int nr_faces, l, f;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else {
		nr_faces = 1;
	}
	
	for (l = 0; l <= pt->last_level; l++) {
		if (swizzled)
			nv10mt->level[l].pitch = util_format_get_stride(pt->format, width);
		else
			nv10mt->level[l].pitch = util_format_get_stride(pt->format, pt->width0);
		nv10mt->level[l].pitch = (nv10mt->level[l].pitch + 63) & ~63;

		nv10mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = u_minify(width, 1);

	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l <= pt->last_level; l++) {
			nv10mt->level[l].image_offset[f] = offset;
			offset += nv10mt->level[l].pitch * u_minify(pt->height0, l);
		}
	}

	nv10mt->total_size = offset;
}

static struct pipe_texture *
nv10_miptree_blanket(struct pipe_screen *pscreen, const struct pipe_texture *pt,
		     const unsigned *stride, struct pipe_buffer *pb)
{
	struct nv10_miptree *mt;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (pt->target != PIPE_TEXTURE_2D || pt->last_level != 0 ||
	    pt->depth0 != 1)
		return NULL;

	mt = CALLOC_STRUCT(nv10_miptree);
	if (!mt)
		return NULL;

	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = pscreen;
	mt->level[0].pitch = stride[0];
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));

	pipe_buffer_reference(&mt->buffer, pb);
	mt->bo = nouveau_bo(mt->buffer);
	return &mt->base;
}

static struct pipe_texture *
nv10_miptree_create(struct pipe_screen *screen, const struct pipe_texture *pt)
{
	struct nv10_miptree *mt;

	mt = MALLOC(sizeof(struct nv10_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = screen;

	nv10_miptree_layout(mt);

	mt->buffer = screen->buffer_create(screen, 256, PIPE_BUFFER_USAGE_PIXEL,
					   mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}
	mt->bo = nouveau_bo(mt->buffer);
	
	return &mt->base;
}

static void
nv10_miptree_destroy(struct pipe_texture *pt)
{
	struct nv10_miptree *nv10mt = (struct nv10_miptree *)pt;
        int l;

        pipe_buffer_reference(&nv10mt->buffer, NULL);
        for (l = 0; l <= pt->last_level; l++) {
		if (nv10mt->level[l].image_offset)
			FREE(nv10mt->level[l].image_offset);
        }
        FREE(nv10mt);
}

static void
nv10_miptree_update(struct pipe_context *pipe, struct pipe_texture *mt,
		    uint face, uint levels)
{
}


static struct pipe_surface *
nv10_miptree_surface_get(struct pipe_screen *screen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv10_miptree *nv10mt = (struct nv10_miptree *)pt;
	struct nv04_surface *ns;

	ns = CALLOC_STRUCT(nv04_surface);
	if (!ns)
		return NULL;
	pipe_texture_reference(&ns->base.texture, pt);
	ns->base.format = pt->format;
	ns->base.width = u_minify(pt->width0, level);
	ns->base.height = u_minify(pt->height0, level);
	ns->base.usage = flags;
	pipe_reference_init(&ns->base.reference, 1);
	ns->base.face = face;
	ns->base.level = level;
	ns->base.zslice = zslice;
	ns->pitch = nv10mt->level[level].pitch;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ns->base.offset = nv10mt->level[level].image_offset[face];
	} else {
		ns->base.offset = nv10mt->level[level].image_offset[0];
	}

	return &ns->base;
}

static void
nv10_miptree_surface_destroy(struct pipe_surface *surface)
{
}

void nv10_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv10_miptree_create;
	pscreen->texture_blanket = nv10_miptree_blanket;
	pscreen->texture_destroy = nv10_miptree_destroy;
	pscreen->get_tex_surface = nv10_miptree_surface_get;
	pscreen->tex_surface_destroy = nv10_miptree_surface_destroy;
}

