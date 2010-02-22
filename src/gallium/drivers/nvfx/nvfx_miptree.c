#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"

#include "nvfx_context.h"
#include "nv04_surface_2d.h"



static void
nvfx_miptree_layout(struct nvfx_miptree *mt)
{
	struct pipe_texture *pt = &mt->base;
	uint width = pt->width0;
	uint offset = 0;
	int nr_faces, l, f;
	uint wide_pitch = pt->tex_usage & (PIPE_TEXTURE_USAGE_SAMPLER |
		                           PIPE_TEXTURE_USAGE_DEPTH_STENCIL |
		                           PIPE_TEXTURE_USAGE_RENDER_TARGET |
		                           PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
		                           PIPE_TEXTURE_USAGE_SCANOUT);

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		nr_faces = pt->depth0;
	} else {
		nr_faces = 1;
	}

	for (l = 0; l <= pt->last_level; l++) {
		if (wide_pitch && (pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR))
			mt->level[l].pitch = align(util_format_get_stride(pt->format, pt->width0), 64);
		else
			mt->level[l].pitch = util_format_get_stride(pt->format, width);

		mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = u_minify(width, 1);
	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l < pt->last_level; l++) {
			mt->level[l].image_offset[f] = offset;

			if (!(pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR) &&
			    u_minify(pt->width0, l + 1) > 1 && u_minify(pt->height0, l + 1) > 1)
				offset += align(mt->level[l].pitch * u_minify(pt->height0, l), 64);
			else
				offset += mt->level[l].pitch * u_minify(pt->height0, l);
		}

		mt->level[l].image_offset[f] = offset;
		offset += mt->level[l].pitch * u_minify(pt->height0, l);
	}

	mt->total_size = offset;
}

static struct pipe_texture *
nvfx_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *pt)
{
	struct nvfx_miptree *mt;
	unsigned buf_usage = PIPE_BUFFER_USAGE_PIXEL |
	                     NOUVEAU_BUFFER_USAGE_TEXTURE;

	mt = MALLOC(sizeof(struct nvfx_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = pscreen;

	/* Swizzled textures must be POT */
	if (pt->width0 & (pt->width0 - 1) ||
	    pt->height0 & (pt->height0 - 1))
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
	else
	if (pt->tex_usage & (PIPE_TEXTURE_USAGE_SCANOUT |
	                     PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
	                     PIPE_TEXTURE_USAGE_DEPTH_STENCIL))
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
	else
	if (pt->tex_usage & PIPE_TEXTURE_USAGE_DYNAMIC)
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
	else {
		switch (pt->format) {
		case PIPE_FORMAT_B5G6R5_UNORM:
		case PIPE_FORMAT_L8A8_UNORM:
		case PIPE_FORMAT_A8_UNORM:
		case PIPE_FORMAT_L8_UNORM:
		case PIPE_FORMAT_I8_UNORM:
			/* TODO: we can actually swizzle these formats on nv40, we
				are just preserving the pre-unification behavior.
				The whole 2D code is going to be rewritten anyway. */
			if(nvfx_screen(pscreen)->is_nv4x) {
				mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;
				break;
			}
		/* TODO: Figure out which formats can be swizzled */
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B8G8R8X8_UNORM:
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

	/* apparently we can't render to swizzled surfaces smaller than 64 bytes, so make them linear.
	 * If the user did not ask for a render target, they can still render to it, but it will cost them an extra copy.
	 * This also happens for small mipmaps of large textures. */
	if (pt->tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET && util_format_get_stride(pt->format, pt->width0) < 64)
		mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;

	nvfx_miptree_layout(mt);

	mt->buffer = pscreen->buffer_create(pscreen, 256, buf_usage, mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}
	mt->bo = nouveau_bo(mt->buffer);
	return &mt->base;
}

static struct pipe_texture *
nvfx_miptree_blanket(struct pipe_screen *pscreen, const struct pipe_texture *pt,
		     const unsigned *stride, struct pipe_buffer *pb)
{
	struct nvfx_miptree *mt;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (pt->target != PIPE_TEXTURE_2D || pt->last_level != 0 ||
	    pt->depth0 != 1)
		return NULL;

	mt = CALLOC_STRUCT(nvfx_miptree);
	if (!mt)
		return NULL;

	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = pscreen;
	mt->level[0].pitch = stride[0];
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));

	/* Assume whoever created this buffer expects it to be linear for now */
	mt->base.tex_usage |= NOUVEAU_TEXTURE_USAGE_LINEAR;

	pipe_buffer_reference(&mt->buffer, pb);
	mt->bo = nouveau_bo(mt->buffer);
	return &mt->base;
}

static void
nvfx_miptree_destroy(struct pipe_texture *pt)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	int l;

	pipe_buffer_reference(&mt->buffer, NULL);
	for (l = 0; l <= pt->last_level; l++) {
		if (mt->level[l].image_offset)
			FREE(mt->level[l].image_offset);
	}

	FREE(mt);
}

static struct pipe_surface *
nvfx_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
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
	ns->pitch = mt->level[level].pitch;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ns->base.offset = mt->level[level].image_offset[face];
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		ns->base.offset = mt->level[level].image_offset[zslice];
	} else {
		ns->base.offset = mt->level[level].image_offset[0];
	}

	/* create a linear temporary that we can render into if necessary.
	 * Note that ns->pitch is always a multiple of 64 for linear surfaces and swizzled surfaces are POT, so
	 * ns->pitch & 63 is equivalent to (ns->pitch < 64 && swizzled)*/
	if((ns->pitch & 63) && (ns->base.usage & (PIPE_BUFFER_USAGE_GPU_WRITE | NOUVEAU_BUFFER_USAGE_NO_RENDER)) == PIPE_BUFFER_USAGE_GPU_WRITE)
		return &nv04_surface_wrap_for_render(pscreen, ((struct nvfx_screen*)pscreen)->eng2d, ns)->base;

	return &ns->base;
}

static void
nvfx_miptree_surface_del(struct pipe_surface *ps)
{
	struct nv04_surface* ns = (struct nv04_surface*)ps;
	if(ns->backing)
	{
		struct nvfx_screen* screen = (struct nvfx_screen*)ps->texture->screen;
		if(ns->backing->base.usage & PIPE_BUFFER_USAGE_GPU_WRITE)
			screen->eng2d->copy(screen->eng2d, &ns->backing->base, 0, 0, ps, 0, 0, ns->base.width, ns->base.height);
		nvfx_miptree_surface_del(&ns->backing->base);
	}

	pipe_texture_reference(&ps->texture, NULL);
	FREE(ps);
}

void
nvfx_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nvfx_miptree_create;
	pscreen->texture_destroy = nvfx_miptree_destroy;
	pscreen->get_tex_surface = nvfx_miptree_surface_new;
	pscreen->tex_surface_destroy = nvfx_miptree_surface_del;

	nouveau_screen(pscreen)->texture_blanket = nvfx_miptree_blanket;
}
