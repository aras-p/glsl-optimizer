#include "util/u_format.h"

#include "nvfx_context.h"
#include "nvfx_tex.h"
#include "nvfx_resource.h"

void
nv30_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso)
{
	if (cso->max_anisotropy >= 2)
	{
		if (cso->max_anisotropy >= 8)
			ps->en |= NV30_3D_TEX_ENABLE_ANISO_8X;
		else if (cso->max_anisotropy >= 4)
			ps->en |= NV30_3D_TEX_ENABLE_ANISO_4X;
		else if (cso->max_anisotropy >= 2)
			ps->en |= NV30_3D_TEX_ENABLE_ANISO_2X;
	}

	ps->filt |= (int)(cso->lod_bias * 256.0) & 0x1fff;

	ps->max_lod = (int)CLAMP(cso->max_lod, 0.0, 15.0);
	ps->min_lod = (int)CLAMP(cso->min_lod, 0.0, 15.0);

	ps->en |= NV30_3D_TEX_ENABLE_ENABLE;
}

void
nv30_sampler_view_init(struct pipe_context *pipe,
			  struct nvfx_sampler_view *sv)
{
	struct pipe_resource* pt = sv->base.texture;
	struct nvfx_texture_format *tf = &nvfx_texture_formats[sv->base.format];
	unsigned txf;
	unsigned level = pt->target == PIPE_TEXTURE_CUBE ? 0 : sv->base.u.tex.first_level;

	assert(tf->fmt[0] >= 0);

	txf = sv->u.init_fmt;
	txf |= (level != sv->base.u.tex.last_level ? NV30_3D_TEX_FORMAT_MIPMAP : 0);
	txf |= util_logbase2(u_minify(pt->width0, level)) << NV30_3D_TEX_FORMAT_BASE_SIZE_U__SHIFT;
	txf |= util_logbase2(u_minify(pt->height0, level)) << NV30_3D_TEX_FORMAT_BASE_SIZE_V__SHIFT;
	txf |= util_logbase2(u_minify(pt->depth0, level)) << NV30_3D_TEX_FORMAT_BASE_SIZE_W__SHIFT;
	txf |=  0x10000;

	sv->u.nv30.fmt[0] = tf->fmt[0] | txf;
	sv->u.nv30.fmt[1] = tf->fmt[1] | txf;
	sv->u.nv30.fmt[2] = tf->fmt[2] | txf;
	sv->u.nv30.fmt[3] = tf->fmt[3] | txf;

	sv->swizzle  |= (nvfx_subresource_pitch(pt, 0) << NV30_3D_TEX_SWIZZLE_RECT_PITCH__SHIFT);

	if(pt->height0 <= 1 || util_format_is_compressed(sv->base.format))
		sv->u.nv30.rect = -1;
	else
		sv->u.nv30.rect = !!(pt->flags & NOUVEAU_RESOURCE_FLAG_LINEAR);

	sv->lod_offset = sv->base.u.tex.first_level - level;
	sv->max_lod_limit = sv->base.u.tex.last_level - level;
}

void
nv30_fragtex_set(struct nvfx_context *nvfx, int unit)
{
	struct nvfx_sampler_state *ps = nvfx->tex_sampler[unit];
	struct nvfx_sampler_view* sv = (struct nvfx_sampler_view*)nvfx->fragment_sampler_views[unit];
	struct nouveau_bo *bo = ((struct nvfx_miptree *)sv->base.texture)->base.bo;
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned txf;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
	unsigned use_rect;
	unsigned max_lod = MIN2(ps->max_lod + sv->lod_offset, sv->max_lod_limit);
	unsigned min_lod = MIN2(ps->min_lod + sv->lod_offset, max_lod) ;

	if(sv->u.nv30.rect < 0)
	{
		/* in the case of compressed or 1D textures, we can get away with this,
		 * since the layout is the same
		 */
		use_rect = ps->fmt;
	}
	else
	{
		static boolean warned = FALSE;
		if( !!ps->fmt != sv->u.nv30.rect && !warned) {
			warned = TRUE;
			fprintf(stderr,
					"Unimplemented: coordinate normalization mismatch. Possible reasons:\n"
					"1. ARB_texture_non_power_of_two is being used despite the fact it isn't supported\n"
					"2. The state tracker is not using the appropriate coordinate normalization\n"
					"3. The state tracker is not supported\n");
		}

		use_rect  = sv->u.nv30.rect;
	}

	txf = sv->u.nv30.fmt[ps->compare + (use_rect ? 2 : 0)];

	MARK_RING(chan, 9, 2);
	BEGIN_RING(chan, eng3d, NV30_3D_TEX_OFFSET(unit), 8);
	OUT_RELOC(chan, bo, sv->offset, tex_flags | NOUVEAU_BO_LOW, 0, 0);
	OUT_RELOC(chan, bo, txf,
		tex_flags | NOUVEAU_BO_OR,
		NV30_3D_TEX_FORMAT_DMA0, NV30_3D_TEX_FORMAT_DMA1);
	OUT_RING(chan, (ps->wrap & sv->wrap_mask) | sv->wrap);
	OUT_RING(chan, ps->en | (min_lod << NV30_3D_TEX_ENABLE_MIPMAP_MIN_LOD__SHIFT) | (max_lod << NV30_3D_TEX_ENABLE_MIPMAP_MAX_LOD__SHIFT));
	OUT_RING(chan, sv->swizzle);
	OUT_RING(chan, ps->filt | sv->filt);
	OUT_RING(chan, sv->npot_size);
	OUT_RING(chan, ps->bcol);

	nvfx->hw_txf[unit] = txf;
	nvfx->hw_samplers |= (1 << unit);
}
