#include "util/u_format.h"

#include "nvfx_context.h"
#include "nouveau/nouveau_util.h"
#include "nvfx_tex.h"

void
nv30_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso)
{
	if (cso->max_anisotropy >= 8) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_8X;
	} else
	if (cso->max_anisotropy >= 4) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_4X;
	} else
	if (cso->max_anisotropy >= 2) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_2X;
	}

	{
		float limit;

		limit = CLAMP(cso->lod_bias, -16.0, 15.0);
		ps->filt |= (int)(cso->lod_bias * 256.0) & 0x1fff;

		limit = CLAMP(cso->max_lod, 0.0, 15.0);
		ps->en |= (int)(limit) << 14 /*NV34TCL_TX_ENABLE_MIPMAP_MAX_LOD_SHIFT*/;

		limit = CLAMP(cso->min_lod, 0.0, 15.0);
		ps->en |= (int)(limit) << 26 /*NV34TCL_TX_ENABLE_MIPMAP_MIN_LOD_SHIFT*/;
	}
}

#define _(m,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w)                        \
{                                                                              \
  TRUE,                                                                        \
  PIPE_FORMAT_##m,                                                             \
  NV34TCL_TX_FORMAT_FORMAT_##tf,                                               \
  (NV34TCL_TX_SWIZZLE_S0_X_##ts0x | NV34TCL_TX_SWIZZLE_S0_Y_##ts0y |           \
   NV34TCL_TX_SWIZZLE_S0_Z_##ts0z | NV34TCL_TX_SWIZZLE_S0_W_##ts0w |           \
   NV34TCL_TX_SWIZZLE_S1_X_##ts1x | NV34TCL_TX_SWIZZLE_S1_Y_##ts1y |           \
   NV34TCL_TX_SWIZZLE_S1_Z_##ts1z | NV34TCL_TX_SWIZZLE_S1_W_##ts1w)            \
}

struct nv30_texture_format {
	boolean defined;
	uint	pipe;
	int     format;
	int     swizzle;
};

static struct nv30_texture_format
nv30_texture_formats[] = {
	_(B8G8R8X8_UNORM, A8R8G8B8,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(B8G8R8A8_UNORM, A8R8G8B8,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(B5G5R5A1_UNORM, A1R5G5B5,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(B4G4R4A4_UNORM, A4R4G4B4,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(B5G6R5_UNORM  , R5G6B5  ,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(L8_UNORM      , L8      ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(A8_UNORM      , L8      , ZERO, ZERO, ZERO,   S1, X, X, X, X),
	_(I8_UNORM      , L8      ,   S1,   S1,   S1,   S1, X, X, X, X),
	_(L8A8_UNORM    , A8L8    ,   S1,   S1,   S1,   S1, X, X, X, Y),
	_(Z16_UNORM     , R5G6B5  ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(S8Z24_UNORM   , A8R8G8B8,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(DXT1_RGB      , DXT1    ,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(DXT1_RGBA     , DXT1    ,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(DXT3_RGBA     , DXT3    ,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(DXT5_RGBA     , DXT5    ,   S1,   S1,   S1,   S1, X, Y, Z, W),
	{},
};

static struct nv30_texture_format *
nv30_fragtex_format(uint pipe_format)
{
	struct nv30_texture_format *tf = nv30_texture_formats;

	while (tf->defined) {
		if (tf->pipe == pipe_format)
			return tf;
		tf++;
	}

	NOUVEAU_ERR("unknown texture format %s\n", util_format_name(pipe_format));
	return NULL;
}


struct nouveau_stateobj *
nv30_fragtex_build(struct nvfx_context *nvfx, int unit)
{
	struct nvfx_sampler_state *ps = nvfx->tex_sampler[unit];
	struct nvfx_miptree *nv30mt = (struct nvfx_miptree *)nvfx->fragment_sampler_views[unit]->texture;
	struct pipe_texture *pt = &nv30mt->base;
	struct nouveau_bo *bo = nouveau_bo(nv30mt->buffer);
	struct nv30_texture_format *tf;
	struct nouveau_stateobj *so;
	uint32_t txf, txs;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	tf = nv30_fragtex_format(pt->format);
	if (!tf)
		return NULL;

	txf  = tf->format;
	txf |= ((pt->last_level>0) ? NV34TCL_TX_FORMAT_MIPMAP : 0);
	txf |= log2i(pt->width0) << NV34TCL_TX_FORMAT_BASE_SIZE_U_SHIFT;
	txf |= log2i(pt->height0) << NV34TCL_TX_FORMAT_BASE_SIZE_V_SHIFT;
	txf |= log2i(pt->depth0) << NV34TCL_TX_FORMAT_BASE_SIZE_W_SHIFT;
	txf |= NV34TCL_TX_FORMAT_NO_BORDER | 0x10000;

	switch (pt->target) {
	case PIPE_TEXTURE_CUBE:
		txf |= NV34TCL_TX_FORMAT_CUBIC;
		/* fall-through */
	case PIPE_TEXTURE_2D:
		txf |= NV34TCL_TX_FORMAT_DIMS_2D;
		break;
	case PIPE_TEXTURE_3D:
		txf |= NV34TCL_TX_FORMAT_DIMS_3D;
		break;
	case PIPE_TEXTURE_1D:
		txf |= NV34TCL_TX_FORMAT_DIMS_1D;
		break;
	default:
		NOUVEAU_ERR("Unknown target %d\n", pt->target);
		return NULL;
	}

	txs = tf->swizzle;

	so = so_new(1, 8, 2);
	so_method(so, nvfx->screen->eng3d, NV34TCL_TX_OFFSET(unit), 8);
	so_reloc (so, bo, 0, tex_flags | NOUVEAU_BO_LOW, 0, 0);
	so_reloc (so, bo, txf, tex_flags | NOUVEAU_BO_OR,
		      NV34TCL_TX_FORMAT_DMA0, NV34TCL_TX_FORMAT_DMA1);
	so_data  (so, ps->wrap);
	so_data  (so, NV34TCL_TX_ENABLE_ENABLE | ps->en);
	so_data  (so, txs);
	so_data  (so, ps->filt | 0x2000 /*voodoo*/);
	so_data  (so, (pt->width0 << NV34TCL_TX_NPOT_SIZE_W_SHIFT) |
		       pt->height0);
	so_data  (so, ps->bcol);

	return so;
}
