#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"

#include "nvfx_context.h"
#include "nvfx_resource.h"
#include "nvfx_transfer.h"
#include "nv04_surface_2d.h"

#include "nouveau/nouveau_util.h"

/* Currently using separate implementations for buffers and textures,
 * even though gallium has a unified abstraction of these objects.
 * Eventually these should be combined, and mechanisms like transfers
 * be adapted to work for both buffer and texture uploads.
 */

static void
nvfx_miptree_layout(struct nvfx_miptree *mt)
{
	struct pipe_resource *pt = &mt->base.base;
	uint width = pt->width0;
	uint offset = 0;
	int nr_faces, l, f;
	uint wide_pitch = pt->bind & (PIPE_BIND_SAMPLER_VIEW |
				      PIPE_BIND_DEPTH_STENCIL |
				      PIPE_BIND_RENDER_TARGET |
				      PIPE_BIND_DISPLAY_TARGET |
				      PIPE_BIND_SCANOUT);

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		nr_faces = pt->depth0;
	} else {
		nr_faces = 1;
	}

	for (l = 0; l <= pt->last_level; l++) {
		if (wide_pitch && (pt->flags & NVFX_RESOURCE_FLAG_LINEAR))
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

			if (!(pt->flags & NVFX_RESOURCE_FLAG_LINEAR) &&
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

static boolean
nvfx_miptree_get_handle(struct pipe_screen *pscreen,
			struct pipe_resource *ptexture,
			struct winsys_handle *whandle)
{
	struct nvfx_miptree* mt = (struct nvfx_miptree*)ptexture;

	if (!mt || !mt->base.bo)
		return FALSE;

	return nouveau_screen_bo_get_handle(pscreen,
					    mt->base.bo,
					    mt->level[0].pitch,
					    whandle);
}


static void
nvfx_miptree_destroy(struct pipe_screen *screen, struct pipe_resource *pt)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	int l;

	nouveau_screen_bo_release(screen, mt->base.bo);

	for (l = 0; l <= pt->last_level; l++) {
		if (mt->level[l].image_offset)
			FREE(mt->level[l].image_offset);
	}

	FREE(mt);
}




struct u_resource_vtbl nvfx_miptree_vtbl = 
{
   nvfx_miptree_get_handle,	      /* get_handle */
   nvfx_miptree_destroy,	      /* resource_destroy */
   NULL,			      /* is_resource_referenced */
   nvfx_miptree_transfer_new,	      /* get_transfer */
   nvfx_miptree_transfer_del,     /* transfer_destroy */
   nvfx_miptree_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   nvfx_miptree_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};



struct pipe_resource *
nvfx_miptree_create(struct pipe_screen *pscreen, const struct pipe_resource *pt)
{
	struct nvfx_miptree *mt;
	static int no_swizzle = -1;
	if(no_swizzle < 0)
		no_swizzle = debug_get_bool_option("NOUVEAU_NO_SWIZZLE", FALSE);

	mt = CALLOC_STRUCT(nvfx_miptree);
	if (!mt)
		return NULL;

	mt->base.base = *pt;
	mt->base.vtbl = &nvfx_miptree_vtbl;
	pipe_reference_init(&mt->base.base.reference, 1);
	mt->base.base.screen = pscreen;

	/* Swizzled textures must be POT */
	if (pt->width0 & (pt->width0 - 1) ||
	    pt->height0 & (pt->height0 - 1))
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
	else
	if (pt->bind & (PIPE_BIND_SCANOUT |
			PIPE_BIND_DISPLAY_TARGET |
			PIPE_BIND_DEPTH_STENCIL))
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
	else
	if (pt->_usage == PIPE_USAGE_DYNAMIC)
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
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
				mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
				break;
			}
		/* TODO: Figure out which formats can be swizzled */
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B8G8R8X8_UNORM:
		case PIPE_FORMAT_R16_SNORM:
		{
			if (no_swizzle)
				mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
			break;
		}
		default:
			mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
		}
	}

	/* apparently we can't render to swizzled surfaces smaller than 64 bytes, so make them linear.
	 * If the user did not ask for a render target, they can still render to it, but it will cost them an extra copy.
	 * This also happens for small mipmaps of large textures. */
	if (pt->bind & PIPE_BIND_RENDER_TARGET &&
	    util_format_get_stride(pt->format, pt->width0) < 64)
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;

	nvfx_miptree_layout(mt);

	mt->base.bo = nouveau_screen_bo_new(pscreen, 256,
            pt->_usage, pt->bind, mt->total_size);
	if (!mt->base.bo) {
		FREE(mt);
		return NULL;
	}
	return &mt->base.base;
}




struct pipe_resource *
nvfx_miptree_from_handle(struct pipe_screen *pscreen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
	struct nvfx_miptree *mt;
	unsigned stride;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (template->target != PIPE_TEXTURE_2D ||
	    template->last_level != 0 ||
	    template->depth0 != 1)
		return NULL;

	mt = CALLOC_STRUCT(nvfx_miptree);
	if (!mt)
		return NULL;

	mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
	if (mt->base.bo == NULL) {
		FREE(mt);
		return NULL;
	}

	mt->base.base = *template;
	mt->base.vtbl = &nvfx_miptree_vtbl;
	pipe_reference_init(&mt->base.base.reference, 1);
	mt->base.base.screen = pscreen;
	mt->level[0].pitch = stride;
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));

	/* Assume whoever created this buffer expects it to be linear for now */
	mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;

	/* XXX: Need to adjust bo refcount??
	 */
	/* nouveau_bo_ref(bo, &mt->base.bo); */
	return &mt->base.base;
}





/* Surface helpers, not strictly required to implement the resource vtbl:
 */
struct pipe_surface *
nvfx_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_resource *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	struct nv04_surface *ns;

	ns = CALLOC_STRUCT(nv04_surface);
	if (!ns)
		return NULL;
	pipe_resource_reference(&ns->base.texture, pt);
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

	/* create a linear temporary that we can render into if
	 * necessary.
	 *
	 * Note that ns->pitch is always a multiple of 64 for linear
	 * surfaces and swizzled surfaces are POT, so ns->pitch & 63
	 * is equivalent to (ns->pitch < 64 && swizzled)
	 */

	if ((ns->pitch & 63) && 
	    (ns->base.usage & PIPE_BIND_RENDER_TARGET))
	{
		struct nv04_surface_2d* eng2d  =
			((struct nvfx_screen*)pscreen)->eng2d;

		ns = nv04_surface_wrap_for_render(pscreen, eng2d, ns);
	}

	return &ns->base;
}

void
nvfx_miptree_surface_del(struct pipe_surface *ps)
{
	struct nv04_surface* ns = (struct nv04_surface*)ps;
	if(ns->backing)
	{
		struct nvfx_screen* screen = (struct nvfx_screen*)ps->texture->screen;
		if(ns->backing->base.usage & PIPE_BIND_BLIT_DESTINATION)
			screen->eng2d->copy(screen->eng2d, &ns->backing->base, 0, 0, ps, 0, 0, ns->base.width, ns->base.height);
		nvfx_miptree_surface_del(&ns->backing->base);
	}

	pipe_resource_reference(&ps->texture, NULL);
	FREE(ps);
}
