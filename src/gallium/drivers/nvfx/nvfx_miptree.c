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
#include "nvfx_transfer.h"

static void
nvfx_miptree_choose_format(struct nvfx_miptree *mt)
{
	struct pipe_resource *pt = &mt->base.base;
	unsigned uniform_pitch = 0;
	static int no_swizzle = -1;
	if(no_swizzle < 0)
		no_swizzle = debug_get_bool_option("NV40_NO_SWIZZLE", FALSE); /* this will break things on nv30 */

	if (!util_is_pot(pt->width0) ||
	    !util_is_pot(pt->height0) ||
	    !util_is_pot(pt->depth0) ||
	    (!nvfx_screen(pt->screen)->is_nv4x && pt->target == PIPE_TEXTURE_RECT)
	    )
		uniform_pitch = 1;

	if (
		(pt->bind & (PIPE_BIND_SCANOUT | PIPE_BIND_DISPLAY_TARGET))
		|| (pt->usage & PIPE_USAGE_DYNAMIC) || (pt->usage & PIPE_USAGE_STAGING)
		|| util_format_is_compressed(pt->format)
		|| no_swizzle
	)
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;

	/* non compressed formats with uniform pitch must be linear, and vice versa */
	if(!util_format_is_s3tc(pt->format)
		&& (uniform_pitch || mt->base.base.flags & NVFX_RESOURCE_FLAG_LINEAR))
	{
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
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
			|| (util_is_pot(pt->width0) && util_is_pot(pt->height0)));
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
					    mt->linear_pitch,
					    whandle);
}


static void
nvfx_miptree_destroy(struct pipe_screen *screen, struct pipe_resource *pt)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)pt;
	nouveau_screen_bo_release(screen, mt->base.bo);
	FREE(mt);
}




struct u_resource_vtbl nvfx_miptree_vtbl = 
{
   nvfx_miptree_get_handle,	      /* get_handle */
   nvfx_miptree_destroy,	      /* resource_destroy */
   NULL,			      /* is_resource_referenced */
   nvfx_transfer_new,	  	      /* get_transfer */
   util_staging_transfer_destroy,     /* transfer_destroy */
   nvfx_transfer_map,		      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   nvfx_transfer_unmap,	              /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};

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
        mt->base.vtbl = &nvfx_miptree_vtbl;
        pipe_reference_init(&mt->base.base.reference, 1);
        mt->base.base.screen = pscreen;

        // set this to the actual capabilities, we use it to decide whether to use the 3D engine for copies
        // TODO: is this the correct way to use Gallium?
        mt->base.base.bind = pt->bind | PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_DEPTH_STENCIL;

        // on our current driver (and the driver too), format support does not depend on geometry, so don't bother computing it
        // TODO: may want to revisit this
        if(!pscreen->is_format_supported(pscreen, pt->format, pt->target, 0, PIPE_BIND_RENDER_TARGET, 0))
                mt->base.base.bind &=~ PIPE_BIND_RENDER_TARGET;
        if(!pscreen->is_format_supported(pscreen, pt->format, pt->target, 0, PIPE_BIND_SAMPLER_VIEW, 0))
                mt->base.base.bind &=~ PIPE_BIND_SAMPLER_VIEW;
        if(!pscreen->is_format_supported(pscreen, pt->format, pt->target, 0, PIPE_BIND_DEPTH_STENCIL, 0))
                mt->base.base.bind &=~ PIPE_BIND_DEPTH_STENCIL;

        return mt;
}


struct pipe_resource *
nvfx_miptree_create(struct pipe_screen *pscreen, const struct pipe_resource *pt)
{
	struct nvfx_miptree* mt = nvfx_miptree_create_skeleton(pscreen, pt);
	nvfx_miptree_choose_format(mt);

        unsigned size = nvfx_miptree_layout(mt);

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
        if(whandle->stride) {
		mt->linear_pitch = whandle->stride;
		mt->base.base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
        } else
		nvfx_miptree_choose_format(mt);

        nvfx_miptree_layout(mt);

        unsigned stride;
        mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
        if (mt->base.bo == NULL) {
                FREE(mt);
                return NULL;
        }
        return &mt->base.base;
}

/* Surface helpers, not strictly required to implement the resource vtbl:
 */
struct pipe_surface *
nvfx_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_resource *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nvfx_surface *ns;

	ns = CALLOC_STRUCT(nvfx_surface);
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
	ns->pitch = nvfx_subresource_pitch(pt, level);
	ns->base.offset = nvfx_subresource_offset(pt, face, level, zslice);

	return &ns->base;
}

void
nvfx_miptree_surface_del(struct pipe_surface *ps)
{
	pipe_resource_reference(&ps->texture, NULL);
	FREE(ps);
}
