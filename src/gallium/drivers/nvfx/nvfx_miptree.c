#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_staging.h"
#include "state_tracker/drm_driver.h"
#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"
#include "nvfx_screen.h"
#include "nvfx_resource.h"

static void
nvfx_miptree_choose_format(struct nvfx_miptree *mt)
{
	struct pipe_resource *pt = &mt->base.base;
	unsigned uniform_pitch = 0;
	static int no_swizzle = -1;
	if(no_swizzle < 0)
		no_swizzle = debug_get_bool_option("NV40_NO_SWIZZLE", FALSE); /* this will break things on nv30 */

	if (!util_is_power_of_two(pt->width0) ||
	    !util_is_power_of_two(pt->height0) ||
	    !util_is_power_of_two(pt->depth0) ||
	    (!nvfx_screen(pt->screen)->is_nv4x && pt->target == PIPE_TEXTURE_RECT)
	    )
		uniform_pitch = 1;

	if (
		(pt->bind & (PIPE_BIND_SCANOUT | PIPE_BIND_DISPLAY_TARGET))
		|| (pt->usage & PIPE_USAGE_DYNAMIC) || (pt->usage & PIPE_USAGE_STAGING)
		|| util_format_is_compressed(pt->format)
		|| no_swizzle
	)
		mt->base.base.flags |= NOUVEAU_RESOURCE_FLAG_LINEAR;

	/* non compressed formats with uniform pitch must be linear, and vice versa */
	if(!util_format_is_s3tc(pt->format)
		&& (uniform_pitch || mt->base.base.flags & NOUVEAU_RESOURCE_FLAG_LINEAR))
	{
		mt->base.base.flags |= NOUVEAU_RESOURCE_FLAG_LINEAR;
		uniform_pitch = 1;
	}

	if(uniform_pitch)
	{
		mt->linear_pitch = util_format_get_stride(pt->format, pt->width0);

		// TODO: this is only a constraint for rendering and not sampling, apparently
		// we may also want this unconditionally
		if(pt->bind & (PIPE_BIND_SAMPLER_VIEW |
			PIPE_BIND_DEPTH_STENCIL |
			PIPE_BIND_RENDER_TARGET |
			PIPE_BIND_DISPLAY_TARGET |
			PIPE_BIND_SCANOUT))
			mt->linear_pitch = align(mt->linear_pitch, 64);
	}
	else
		mt->linear_pitch = 0;
}

static unsigned
nvfx_miptree_layout(struct nvfx_miptree *mt)
{
	struct pipe_resource* pt = &mt->base.base;
        uint offset = 0;

	if(!nvfx_screen(pt->screen)->is_nv4x)
	{
		assert(pt->target == PIPE_TEXTURE_RECT
			|| (util_is_power_of_two(pt->width0) && util_is_power_of_two(pt->height0)));
	}

	for (unsigned l = 0; l <= pt->last_level; l++)
	{
		unsigned size;
		mt->level_offset[l] = offset;

		if(mt->linear_pitch)
			size = mt->linear_pitch;
		else
			size = util_format_get_stride(pt->format, u_minify(pt->width0, l));
		size = util_format_get_2d_size(pt->format, size, u_minify(pt->height0, l));

		if(pt->target == PIPE_TEXTURE_3D)
			size *= u_minify(pt->depth0, l);

		offset += size;
	}

	offset = align(offset, 128);
	mt->face_size = offset;
	if(mt->base.base.target == PIPE_TEXTURE_CUBE)
		offset += 5 * mt->face_size;
	return offset;
}

static void
nvfx_miptree_surface_final_destroy(struct pipe_surface* ps)
{
	struct nvfx_surface* ns = (struct nvfx_surface*)ps;
	pipe_resource_reference(&ps->texture, 0);
	pipe_resource_reference((struct pipe_resource**)&ns->temp, 0);
	FREE(ps);
}

void
nvfx_miptree_destroy(struct pipe_screen *screen, struct pipe_resource *pt)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	util_surfaces_destroy(&mt->surfaces, pt, nvfx_miptree_surface_final_destroy);
	nouveau_screen_bo_release(screen, mt->base.bo);
	FREE(mt);
}

static struct nvfx_miptree*
nvfx_miptree_create_skeleton(struct pipe_screen *pscreen, const struct pipe_resource *pt)
{
        struct nvfx_miptree *mt;

        if(pt->width0 > 4096 || pt->height0 > 4096)
                return NULL;

        mt = CALLOC_STRUCT(nvfx_miptree);
        if (!mt)
                return NULL;

        mt->base.base = *pt;
        util_dirty_surfaces_init(&mt->dirty_surfaces);

        pipe_reference_init(&mt->base.base.reference, 1);
        mt->base.base.screen = pscreen;

        // set this to the actual capabilities, we use it to decide whether to use the 3D engine for copies
        // TODO: is this the correct way to use Gallium?
        mt->base.base.bind = pt->bind | PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_DEPTH_STENCIL;

        // on our current driver (and the driver too), format support does not depend on geometry, so don't bother computing it
        // TODO: may want to revisit this
        if(!pscreen->is_format_supported(pscreen, pt->format, pt->target, 0, PIPE_BIND_RENDER_TARGET))
                mt->base.base.bind &=~ PIPE_BIND_RENDER_TARGET;
        if(!pscreen->is_format_supported(pscreen, pt->format, pt->target, 0, PIPE_BIND_SAMPLER_VIEW))
                mt->base.base.bind &=~ PIPE_BIND_SAMPLER_VIEW;
        if(!pscreen->is_format_supported(pscreen, pt->format, pt->target, 0, PIPE_BIND_DEPTH_STENCIL))
                mt->base.base.bind &=~ PIPE_BIND_DEPTH_STENCIL;

        return mt;
}


struct pipe_resource *
nvfx_miptree_create(struct pipe_screen *pscreen, const struct pipe_resource *pt)
{
	struct nvfx_miptree* mt = nvfx_miptree_create_skeleton(pscreen, pt);
        unsigned size;
	nvfx_miptree_choose_format(mt);

        size = nvfx_miptree_layout(mt);

	mt->base.bo = nouveau_screen_bo_new(pscreen, 256, pt->usage, pt->bind, size);

	if (!mt->base.bo) {
		FREE(mt);
		return NULL;
	}
	return &mt->base.base;
}

// TODO: redo this, just calling miptree_layout
struct pipe_resource *
nvfx_miptree_from_handle(struct pipe_screen *pscreen, const struct pipe_resource *template, struct winsys_handle *whandle)
{
        struct nvfx_miptree* mt = nvfx_miptree_create_skeleton(pscreen, template);
        unsigned stride;
        if(whandle->stride) {
		mt->linear_pitch = whandle->stride;
		mt->base.base.flags |= NOUVEAU_RESOURCE_FLAG_LINEAR;
        } else
		nvfx_miptree_choose_format(mt);

        nvfx_miptree_layout(mt);

        mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
        if (mt->base.bo == NULL) {
                FREE(mt);
                return NULL;
        }
        return &mt->base.base;
}

struct pipe_surface *
nvfx_miptree_surface_new(struct pipe_context *pipe, struct pipe_resource *pt,
			 const struct pipe_surface *surf_tmpl)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	unsigned level = surf_tmpl->u.tex.level;
	struct nvfx_surface *ns = NULL;

	assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);
	if(util_surfaces_get(&mt->surfaces, sizeof(struct nvfx_surface), pipe,
                             pt, level, surf_tmpl->u.tex.first_layer,
                             surf_tmpl->usage, (struct pipe_surface **)&ns)) {
                util_dirty_surface_init(&ns->base);
                ns->pitch = nvfx_subresource_pitch(pt, level);
                ns->offset = nvfx_subresource_offset(pt, surf_tmpl->u.tex.first_layer, level, surf_tmpl->u.tex.first_layer);
	}

	return &ns->base.base;
}

void
nvfx_miptree_surface_del(struct pipe_context *pipe, struct pipe_surface *ps)
{
	struct nvfx_surface* ns = (struct nvfx_surface*)ps;

	if(!ns->temp)
	{
		assert(!util_dirty_surface_is_dirty(&ns->base));
		util_surfaces_detach(&((struct nvfx_miptree*)ps->texture)->surfaces, ps);
		pipe_resource_reference(&ps->texture, 0);
		FREE(ps);
	}
}
