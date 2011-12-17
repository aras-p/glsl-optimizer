#include "util/u_format.h"
#include "nvfx_context.h"
#include "nvfx_tex.h"
#include "nvfx_resource.h"

void
nv40_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso)
{
	if (cso->max_anisotropy >= 2) {
		/* no idea, binary driver sets it, works without it.. meh.. */
		ps->wrap |= (1 << 5);

		if (cso->max_anisotropy >= 16)
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_16X;
		else if (cso->max_anisotropy >= 12)
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_12X;
		else if (cso->max_anisotropy >= 10)
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_10X;
		else if (cso->max_anisotropy >= 8)
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_8X;
		else if (cso->max_anisotropy >= 6)
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_6X;
		else if (cso->max_anisotropy >= 4)
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_4X;
		else
			ps->en |= NV40_3D_TEX_ENABLE_ANISO_2X;
	}

	ps->filt |= (int)(cso->lod_bias * 256.0) & 0x1fff;

	ps->max_lod = (int)(CLAMP(cso->max_lod, 0.0, 15.0 + (255.0 / 256.0)) * 256.0);
	ps->min_lod = (int)(CLAMP(cso->min_lod, 0.0, 15.0 + (255.0 / 256.0)) * 256.0);

	ps->en |= NV40_3D_TEX_ENABLE_ENABLE;
}

void
nv40_sampler_view_init(struct pipe_context *pipe,
			  struct nvfx_sampler_view *sv)
{
	struct pipe_resource* pt = sv->base.texture;
	struct nvfx_miptree* mt = (struct nvfx_miptree*)pt;
	struct nvfx_texture_format *tf = &nvfx_texture_formats[sv->base.format];
	unsigned txf;
	unsigned level = pt->target == PIPE_TEXTURE_CUBE ? 0 : sv->base.u.tex.first_level;
	assert(tf->fmt[4] >= 0);

	txf = sv->u.init_fmt;
	txf |= 0x8000;
	if(pt->target == PIPE_TEXTURE_CUBE)
		txf |= ((pt->last_level + 1) << NV40_3D_TEX_FORMAT_MIPMAP_COUNT__SHIFT);
	else
		txf |= (((sv->base.u.tex.last_level - sv->base.u.tex.first_level) + 1) << NV40_3D_TEX_FORMAT_MIPMAP_COUNT__SHIFT);

	if (!mt->linear_pitch)
		sv->u.nv40.npot_size2 = 0;
	else {
		sv->u.nv40.npot_size2  = mt->linear_pitch;
		txf |= NV40_3D_TEX_FORMAT_LINEAR;
	}

	sv->u.nv40.fmt[0] = tf->fmt[4] | txf;
	sv->u.nv40.fmt[1] = tf->fmt[5] | txf;

	sv->u.nv40.npot_size2 |= (u_minify(pt->depth0, level) << NV40_3D_TEX_SIZE1_DEPTH__SHIFT);

	sv->lod_offset = (sv->base.u.tex.first_level - level) * 256;
	sv->max_lod_limit = (sv->base.u.tex.last_level - level) * 256;
}

void
nv40_fragtex_set(struct nvfx_context *nvfx, int unit)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct nvfx_sampler_state *ps = nvfx->tex_sampler[unit];
	struct nvfx_sampler_view* sv = (struct nvfx_sampler_view*)nvfx->fragment_sampler_views[unit];
	struct nouveau_bo *bo = ((struct nvfx_miptree *)sv->base.texture)->base.bo;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
	unsigned txf;
	unsigned max_lod = MIN2(ps->max_lod + sv->lod_offset, sv->max_lod_limit);
	unsigned min_lod = MIN2(ps->min_lod + sv->lod_offset, max_lod);

	txf = sv->u.nv40.fmt[ps->compare] | ps->fmt;

	MARK_RING(chan, 11, 2);
	BEGIN_RING(chan, eng3d, NV30_3D_TEX_OFFSET(unit), 8);
	OUT_RELOC(chan, bo, sv->offset, tex_flags | NOUVEAU_BO_LOW, 0, 0);
	OUT_RELOC(chan, bo, txf, tex_flags | NOUVEAU_BO_OR,
			NV30_3D_TEX_FORMAT_DMA0, NV30_3D_TEX_FORMAT_DMA1);
	OUT_RING(chan, (ps->wrap & sv->wrap_mask) | sv->wrap);
	OUT_RING(chan, ps->en | (min_lod << 19) | (max_lod << 7));
	OUT_RING(chan, sv->swizzle);
	OUT_RING(chan, ps->filt | sv->filt);
	OUT_RING(chan, sv->npot_size);
	OUT_RING(chan, ps->bcol);
	BEGIN_RING(chan, eng3d, NV40_3D_TEX_SIZE1(unit), 1);
	OUT_RING(chan, sv->u.nv40.npot_size2);

	nvfx->hw_txf[unit] = txf;
	nvfx->hw_samplers |= (1 << unit);
}
