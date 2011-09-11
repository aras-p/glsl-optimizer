#include "nvfx_context.h"
#include "nvfx_resource.h"
#include "nvfx_tex.h"

static void *
nvfx_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_sampler_state *ps;

	ps = MALLOC(sizeof(struct nvfx_sampler_state));

	/* on nv30, we use this as an internal flag */
	ps->fmt = cso->normalized_coords ? 0 : NV40_3D_TEX_FORMAT_RECT;
	ps->en = 0;
	ps->filt = nvfx_tex_filter(cso) | 0x2000; /*voodoo*/
	ps->wrap = (nvfx_tex_wrap_mode(cso->wrap_s) << NV30_3D_TEX_WRAP_S__SHIFT) |
		    (nvfx_tex_wrap_mode(cso->wrap_t) << NV30_3D_TEX_WRAP_T__SHIFT) |
		    (nvfx_tex_wrap_mode(cso->wrap_r) << NV30_3D_TEX_WRAP_R__SHIFT);
	ps->compare = FALSE;

	if(cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE)
	{
		ps->wrap |= nvfx_tex_wrap_compare_mode(cso->compare_func);
		ps->compare = TRUE;
	}
	ps->bcol = nvfx_tex_border_color(cso->border_color.f);

	if(nvfx->is_nv4x)
		nv40_sampler_state_init(pipe, ps, cso);
	else
		nv30_sampler_state_init(pipe, ps, cso);

	return (void *)ps;
}

static void
nvfx_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nvfx_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nvfx->tex_sampler[unit] = sampler[unit];
		nvfx->dirty_samplers |= (1 << unit);
	}

	for (unit = nr; unit < nvfx->nr_samplers; unit++) {
		nvfx->tex_sampler[unit] = NULL;
		nvfx->dirty_samplers |= (1 << unit);
	}

	nvfx->nr_samplers = nr;
	nvfx->dirty |= NVFX_NEW_SAMPLER;
}

static struct pipe_sampler_view *
nvfx_create_sampler_view(struct pipe_context *pipe,
			 struct pipe_resource *pt,
			 const struct pipe_sampler_view *templ)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_sampler_view *sv = CALLOC_STRUCT(nvfx_sampler_view);
	struct nvfx_texture_format *tf = &nvfx_texture_formats[templ->format];
	unsigned txf;

	if (!sv)
		return NULL;

	sv->base = *templ;
	sv->base.reference.count = 1;
	sv->base.texture = NULL;
	pipe_resource_reference(&sv->base.texture, pt);
	sv->base.context = pipe;

	txf = NV30_3D_TEX_FORMAT_NO_BORDER;

	switch (pt->target) {
	case PIPE_TEXTURE_CUBE:
		txf |= NV30_3D_TEX_FORMAT_CUBIC;
		/* fall-through */
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_RECT:
		txf |= NV30_3D_TEX_FORMAT_DIMS_2D;
		break;
	case PIPE_TEXTURE_3D:
		txf |= NV30_3D_TEX_FORMAT_DIMS_3D;
		break;
	case PIPE_TEXTURE_1D:
		txf |= NV30_3D_TEX_FORMAT_DIMS_1D;
		break;
	default:
		assert(0);
	}
	sv->u.init_fmt = txf;

	sv->swizzle = 0
			| (tf->src[sv->base.swizzle_r] << NV30_3D_TEX_SWIZZLE_S0_Z__SHIFT)
			| (tf->src[sv->base.swizzle_g] << NV30_3D_TEX_SWIZZLE_S0_Y__SHIFT)
			| (tf->src[sv->base.swizzle_b] << NV30_3D_TEX_SWIZZLE_S0_X__SHIFT)
			| (tf->src[sv->base.swizzle_a] << NV30_3D_TEX_SWIZZLE_S0_W__SHIFT)
			| (tf->comp[sv->base.swizzle_r] << NV30_3D_TEX_SWIZZLE_S1_Z__SHIFT)
			| (tf->comp[sv->base.swizzle_g] << NV30_3D_TEX_SWIZZLE_S1_Y__SHIFT)
			| (tf->comp[sv->base.swizzle_b] << NV30_3D_TEX_SWIZZLE_S1_X__SHIFT)
			| (tf->comp[sv->base.swizzle_a] << NV30_3D_TEX_SWIZZLE_S1_W__SHIFT);

	sv->filt = tf->sign;
	sv->wrap = tf->wrap;
	sv->wrap_mask = ~0;

	if (pt->target == PIPE_TEXTURE_CUBE)
	{
		sv->offset = 0;
		sv->npot_size = (pt->width0 << NV30_3D_TEX_NPOT_SIZE_W__SHIFT) | pt->height0;
	}
	else
	{
		sv->offset = nvfx_subresource_offset(pt, 0, sv->base.u.tex.first_level, 0);
		sv->npot_size = (u_minify(pt->width0, sv->base.u.tex.first_level) << NV30_3D_TEX_NPOT_SIZE_W__SHIFT) | u_minify(pt->height0, sv->base.u.tex.first_level);

		/* apparently, we need to ignore the t coordinate for 1D textures to fix piglit tex1d-2dborder */
		if(pt->target == PIPE_TEXTURE_1D)
		{
			sv->wrap_mask &=~ NV30_3D_TEX_WRAP_T__MASK;
			sv->wrap |= NV30_3D_TEX_WRAP_T_REPEAT;
		}
	}

	if(nvfx->is_nv4x)
		nv40_sampler_view_init(pipe, sv);
	else
		nv30_sampler_view_init(pipe, sv);

	return &sv->base;
}

static void
nvfx_sampler_view_destroy(struct pipe_context *pipe,
			  struct pipe_sampler_view *view)
{
	pipe_resource_reference(&view->texture, NULL);
	FREE(view);
}

static void
nvfx_set_fragment_sampler_views(struct pipe_context *pipe,
				unsigned nr,
				struct pipe_sampler_view **views)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		pipe_sampler_view_reference(&nvfx->fragment_sampler_views[unit],
                                            views[unit]);
		nvfx->dirty_samplers |= (1 << unit);
	}

	for (unit = nr; unit < nvfx->nr_textures; unit++) {
		pipe_sampler_view_reference(&nvfx->fragment_sampler_views[unit],
                                            NULL);
		nvfx->dirty_samplers |= (1 << unit);
	}

	nvfx->nr_textures = nr;
	nvfx->dirty |= NVFX_NEW_SAMPLER;
}

void
nvfx_fragtex_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned samplers, unit;

	samplers = nvfx->dirty_samplers;
	if(!samplers)
		return;

	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		if(nvfx->fragment_sampler_views[unit] && nvfx->tex_sampler[unit]) {
			util_dirty_surfaces_use_for_sampling(&nvfx->pipe,
					&((struct nvfx_miptree*)nvfx->fragment_sampler_views[unit]->texture)->dirty_surfaces,
					nvfx_surface_flush);

			if(!nvfx->is_nv4x)
				nv30_fragtex_set(nvfx, unit);
			else
				nv40_fragtex_set(nvfx, unit);
		} else {
			/* this is OK for nv40 too */
			BEGIN_RING(chan, eng3d, NV30_3D_TEX_ENABLE(unit), 1);
			OUT_RING(chan, 0);
			nvfx->hw_samplers &= ~(1 << unit);
		}
	}
	nvfx->dirty_samplers = 0;
	nvfx->relocs_needed &=~ NVFX_RELOCATE_FRAGTEX;
}

void
nvfx_fragtex_relocate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	unsigned samplers, unit;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	samplers = nvfx->hw_samplers;
	while (samplers) {
		struct nvfx_miptree* mt;
		struct nouveau_bo *bo;

		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		mt = (struct nvfx_miptree*)nvfx->fragment_sampler_views[unit]->texture;
		bo = mt->base.bo;

		MARK_RING(chan, 3, 3);
		OUT_RELOC(chan, bo, RING_3D(NV30_3D_TEX_OFFSET(unit), 2), tex_flags | NOUVEAU_BO_DUMMY, 0, 0);
		OUT_RELOC(chan, bo, 0, tex_flags | NOUVEAU_BO_LOW | NOUVEAU_BO_DUMMY, 0, 0);
		OUT_RELOC(chan, bo, nvfx->hw_txf[unit], tex_flags | NOUVEAU_BO_OR | NOUVEAU_BO_DUMMY,
				NV30_3D_TEX_FORMAT_DMA0, NV30_3D_TEX_FORMAT_DMA1);
	}
	nvfx->relocs_needed &=~ NVFX_RELOCATE_FRAGTEX;
}

void
nvfx_init_sampling_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.create_sampler_state = nvfx_sampler_state_create;
	nvfx->pipe.bind_fragment_sampler_states = nvfx_sampler_state_bind;
	nvfx->pipe.delete_sampler_state = nvfx_sampler_state_delete;
	nvfx->pipe.set_fragment_sampler_views = nvfx_set_fragment_sampler_views;
	nvfx->pipe.create_sampler_view = nvfx_create_sampler_view;
	nvfx->pipe.sampler_view_destroy = nvfx_sampler_view_destroy;
}

#define NV30_3D_TEX_FORMAT_FORMAT_DXT1_RECT NV30_3D_TEX_FORMAT_FORMAT_DXT1
#define NV30_3D_TEX_FORMAT_FORMAT_DXT3_RECT NV30_3D_TEX_FORMAT_FORMAT_DXT3
#define NV30_3D_TEX_FORMAT_FORMAT_DXT5_RECT NV30_3D_TEX_FORMAT_FORMAT_DXT5

#define NV40_3D_TEX_FORMAT_FORMAT_HILO16 NV40_3D_TEX_FORMAT_FORMAT_A16L16

#define NV30_3D_TEX_FORMAT_FORMAT_RGBA16F 0x00004a00
#define NV30_3D_TEX_FORMAT_FORMAT_RGBA16F_RECT NV30_3D_TEX_FORMAT_FORMAT_RGBA16F
#define NV30_3D_TEX_FORMAT_FORMAT_RGBA32F 0x00004b00
#define NV30_3D_TEX_FORMAT_FORMAT_RGBA32F_RECT NV30_3D_TEX_FORMAT_FORMAT_RGBA32F
#define NV30_3D_TEX_FORMAT_FORMAT_R32F 0x00004c00
#define NV30_3D_TEX_FORMAT_FORMAT_R32F_RECT NV30_3D_TEX_FORMAT_FORMAT_R32F

// TODO: guess!
#define NV40_3D_TEX_FORMAT_FORMAT_R32F 0x00001c00

#define SRGB 0x00700000

#define __(m,tf,tfc,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w,sign,wrap) \
[PIPE_FORMAT_##m] = { \
  {NV30_3D_TEX_FORMAT_FORMAT_##tf, \
  NV30_3D_TEX_FORMAT_FORMAT_##tfc, \
  NV30_3D_TEX_FORMAT_FORMAT_##tf##_RECT, \
  NV30_3D_TEX_FORMAT_FORMAT_##tfc##_RECT, \
  NV40_3D_TEX_FORMAT_FORMAT_##tf, \
  NV40_3D_TEX_FORMAT_FORMAT_##tfc}, \
  sign, wrap, \
  {ts0z, ts0y, ts0x, ts0w, 0, 1}, {ts1z, ts1y, ts1x, ts1w, 0, 0} \
}

#define _(m,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w,sign, wrap) \
	__(m,tf,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w,sign, wrap)

/* Depth formats works by reading the depth value most significant 8/16 bits.
 * We are losing precision, but nVidia loses even more by using A8R8G8B8 instead of HILO16
 * There is no 32-bit integer texture support, so other things are infeasible.
 *
 * TODO: is it possible to read 16 bits for Z16? A16 doesn't seem to work, either due to normalization or endianness issues
 */

#define T 2

#define X 3
#define Y 2
#define Z 1
#define W 0

#define SNORM ((NV30_3D_TEX_FILTER_SIGNED_RED) | (NV30_3D_TEX_FILTER_SIGNED_GREEN) | (NV30_3D_TEX_FILTER_SIGNED_BLUE) | (NV30_3D_TEX_FILTER_SIGNED_ALPHA))
#define UNORM 0

struct nvfx_texture_format
nvfx_texture_formats[PIPE_FORMAT_COUNT] = {
	[0 ... PIPE_FORMAT_COUNT - 1] = {{-1, -1, -1, -1, -1, -1}},
	_(B8G8R8X8_UNORM,	A8R8G8B8,	T, T, T, 1, X, Y, Z, W, UNORM, 0),
	_(B8G8R8X8_SRGB,	A8R8G8B8,	T, T, T, 1, X, Y, Z, W, UNORM, SRGB),
	_(B8G8R8A8_UNORM,	A8R8G8B8,	T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(B8G8R8A8_SRGB,	A8R8G8B8,	T, T, T, T, X, Y, Z, W, UNORM, SRGB),

	_(R8G8B8A8_UNORM,	A8R8G8B8,	T, T, T, T, Z, Y, X, W, UNORM, 0),
	_(R8G8B8A8_SRGB,	A8R8G8B8,	T, T, T, T, Z, Y, X, W, UNORM, SRGB),
	_(R8G8B8X8_UNORM,	A8R8G8B8,	T, T, T, 1, Z, Y, X, W, UNORM, 0),

	_(A8R8G8B8_UNORM,	A8R8G8B8,	T, T, T, T, W, Z, Y, X, UNORM, 0),
	_(A8R8G8B8_SRGB,	A8R8G8B8,	T, T, T, T, W, Z, Y, X, UNORM, SRGB),
	_(A8B8G8R8_UNORM,	A8R8G8B8,	T, T, T, T, W, X, Y, Z, UNORM, 0),
	_(A8B8G8R8_SRGB,	A8R8G8B8,	T, T, T, T, W, X, Y, Z, UNORM, SRGB),
	_(X8R8G8B8_UNORM,	A8R8G8B8,	T, T, T, 1, W, Z, Y, X, UNORM, 0),
	_(X8R8G8B8_SRGB,	A8R8G8B8,	T, T, T, 1, W, Z, Y, X, UNORM, SRGB),

	_(B5G5R5A1_UNORM,	A1R5G5B5, 	T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(B5G5R5X1_UNORM,	A1R5G5B5, 	T, T, T, 1, X, Y, Z, W, UNORM, 0),

	_(B4G4R4A4_UNORM,	A4R4G4B4, 	T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(B4G4R4X4_UNORM,	A4R4G4B4, 	T, T, T, 1, X, Y, Z, W, UNORM, 0),

	_(B5G6R5_UNORM,		R5G6B5, 	T, T, T, 1, X, Y, Z, W, UNORM, 0),

	_(R8_UNORM,		L8,		T, 0, 0, 1, X, X, X, X, UNORM, 0),
	_(R8_SNORM,		L8,		T, 0, 0, 1, X, X, X, X, SNORM, 0),
	_(L8_UNORM,		L8,		T, T, T, 1, X, X, X, X, UNORM, 0),
	_(L8_SRGB,		L8,		T, T, T, 1, X, X, X, X, UNORM, SRGB),
	_(A8_UNORM,		L8, 		0, 0, 0, T, X, X, X, X, UNORM, 0),
	_(I8_UNORM,		L8, 		T, T, T, T, X, X, X, X, UNORM, 0),

	_(R8G8_UNORM,		A8L8, 		T, T, T, T, X, X, X, W, UNORM, 0),
	_(R8G8_SNORM,		A8L8, 		T, T, T, T, X, X, X, W, SNORM, 0),
	_(L8A8_UNORM,		A8L8, 		T, T, T, T, X, X, X, W, UNORM, 0),
	_(L8A8_SRGB,		A8L8,		T, T, T, T, X, X, X, W, UNORM, SRGB),

	_(DXT1_RGB,		DXT1,		T, T, T, 1, X, Y, Z, W, UNORM, 0),
	_(DXT1_SRGB,		DXT1,		T, T, T, 1, X, Y, Z, W, UNORM, SRGB),
	_(DXT1_RGBA,		DXT1,		T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(DXT1_SRGBA,		DXT1,		T, T, T, T, X, Y, Z, W, UNORM, SRGB),
	_(DXT3_RGBA,		DXT3,		T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(DXT3_SRGBA,		DXT3,		T, T, T, T, X, Y, Z, W, UNORM, SRGB),
	_(DXT5_RGBA,		DXT5,		T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(DXT5_SRGBA,		DXT5,		T, T, T, T, X, Y, Z, W, UNORM, SRGB),

	__(Z16_UNORM,		A8L8, Z16,	T, T, T, 1, W, W, W, W, UNORM, 0),
	__(S8_UINT_Z24_UNORM,HILO16,Z24,	T, T, T, 1, W, W, W, W, UNORM, 0),
	__(X8Z24_UNORM,		HILO16,Z24,	T, T, T, 1, W, W, W, W, UNORM, 0),

	_(R16_UNORM,		A16,		T, 0, 0, 1, X, X, X, X, UNORM, 0),
	_(R16_SNORM,		A16,		T, 0, 0, 1, X, X, X, X, SNORM, 0),
	_(R16G16_UNORM,		HILO16,		T, T, 0, 1, X, Y, X, X, UNORM, 0),
	_(R16G16_SNORM,		HILO16,		T, T, 0, 1, X, Y, X, X, SNORM, 0),

	_(R16G16B16A16_FLOAT,		RGBA16F,	T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(R32G32B32A32_FLOAT,		RGBA32F,	T, T, T, T, X, Y, Z, W, UNORM, 0),
	_(R32_FLOAT,		R32F,	T, 0, 0, 1, X, X, X, X, UNORM, 0)
};
