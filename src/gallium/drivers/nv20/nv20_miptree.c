#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "nv20_context.h"
#include "nv20_screen.h"

static void
nv20_miptree_layout(struct nv20_miptree *nv20mt)
{
	struct pipe_texture *pt = &nv20mt->base;
	uint width = pt->width[0], height = pt->height[0];
	uint offset = 0;
	int nr_faces, l, f;
	uint wide_pitch = pt->tex_usage & (PIPE_TEXTURE_USAGE_SAMPLER |
		                           PIPE_TEXTURE_USAGE_DEPTH_STENCIL |
		                           PIPE_TEXTURE_USAGE_RENDER_TARGET |
		                           PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
		                           PIPE_TEXTURE_USAGE_PRIMARY);

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else {
		nr_faces = 1;
	}
	
	for (l = 0; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;
		pt->nblocksx[l] = pf_get_nblocksx(&pt->block, width);
		pt->nblocksy[l] = pf_get_nblocksy(&pt->block, height);

		if (wide_pitch && (pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR))
			nv20mt->level[l].pitch = align(pt->width[0] * pt->block.size, 64);
		else
			nv20mt->level[l].pitch = pt->width[l] * pt->block.size;

		nv20mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l < pt->last_level; l++) {
			nv20mt->level[l].image_offset[f] = offset;

			if (!(pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR) &&
			    pt->width[l + 1] > 1 && pt->height[l + 1] > 1)
				offset += align(nv20mt->level[l].pitch * pt->height[l], 64);
			else
				offset += nv20mt->level[l].pitch * pt->height[l];
		}

		nv20mt->level[l].image_offset[f] = offset;
		offset += nv20mt->level[l].pitch * pt->height[l];
	}

	nv20mt->total_size = offset;
}

static struct pipe_texture *
nv20_miptree_blanket(struct pipe_screen *pscreen, const struct pipe_texture *pt,
		     const unsigned *stride, struct pipe_buffer *pb)
{
	struct nv20_miptree *mt;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (pt->target != PIPE_TEXTURE_2D || pt->last_level != 0 ||
	    pt->depth[0] != 1)
		return NULL;

	mt = CALLOC_STRUCT(nv20_miptree);
	if (!mt)
		return NULL;

	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = pscreen;
	mt->level[0].pitch = stride[0];
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));

	pipe_buffer_reference(&mt->buffer, pb);
	return &mt->base;
}

static struct pipe_texture *
nv20_miptree_create(struct pipe_screen *screen, const struct pipe_texture *pt)
{
	struct nv20_miptree *mt;
	unsigned buf_usage = PIPE_BUFFER_USAGE_PIXEL |
	                     NOUVEAU_BUFFER_USAGE_TEXTURE;

	mt = MALLOC(sizeof(struct nv20_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = screen;

	/* Swizzled textures must be POT */
	if (pt->width[0] & (pt->width[0] - 1) ||
	    pt->height[0] & (pt->height[0] - 1))
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
	else
	if (pt->tex_usage & (PIPE_TEXTURE_USAGE_PRIMARY |
	                     PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
	                     PIPE_TEXTURE_USAGE_DEPTH_STENCIL))
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
	else
	if (pt->tex_usage & PIPE_TEXTURE_USAGE_DYNAMIC)
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
	else {
		switch (pt->format) {
		/* TODO: Figure out which formats can be swizzled */
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_X8R8G8B8_UNORM:
		case PIPE_FORMAT_R16_SNORM:
		{
			if (debug_get_bool_option("NOUVEAU_NO_SWIZZLE", FALSE))
				mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
			break;
		}
		default:
			mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
		}
	}

	if (pt->tex_usage & PIPE_TEXTURE_USAGE_DYNAMIC)
		buf_usage |= PIPE_BUFFER_USAGE_CPU_READ_WRITE;

	nv20_miptree_layout(mt);

	mt->buffer = screen->buffer_create(screen, 256, buf_usage, mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}
	
	return &mt->base;
}

static void
nv20_miptree_destroy(struct pipe_texture *pt)
{
	struct nv20_miptree *nv20mt = (struct nv20_miptree *)pt;
        int l;

        pipe_buffer_reference(&nv20mt->buffer, NULL);
        for (l = 0; l <= pt->last_level; l++) {
		if (nv20mt->level[l].image_offset)
			FREE(nv20mt->level[l].image_offset);
        }
}

static struct pipe_surface *
nv20_miptree_surface_get(struct pipe_screen *screen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv20_miptree *nv20mt = (struct nv20_miptree *)pt;
	struct nv04_surface *ns;

	ns = CALLOC_STRUCT(nv04_surface);
	if (!ns)
		return NULL;
	pipe_texture_reference(&ns->base.texture, pt);
	ns->base.format = pt->format;
	ns->base.width = pt->width[level];
	ns->base.height = pt->height[level];
	ns->base.usage = flags;
	pipe_reference_init(&ns->base.reference, 1);
	ns->base.face = face;
	ns->base.level = level;
	ns->base.zslice = zslice;
	ns->pitch = nv20mt->level[level].pitch;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ns->base.offset = nv20mt->level[level].image_offset[face];
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		ns->base.offset = nv20mt->level[level].image_offset[zslice];
	} else {
		ns->base.offset = nv20mt->level[level].image_offset[0];
	}

	return &ns->base;
}

static void
nv20_miptree_surface_destroy(struct pipe_surface *ps)
{
	pipe_texture_reference(&ps->texture, NULL);
	FREE(ps);
}

void nv20_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv20_miptree_create;
	pscreen->texture_blanket = nv20_miptree_blanket;
	pscreen->texture_destroy = nv20_miptree_destroy;
	pscreen->get_tex_surface = nv20_miptree_surface_get;
	pscreen->tex_surface_destroy = nv20_miptree_surface_destroy;
}

