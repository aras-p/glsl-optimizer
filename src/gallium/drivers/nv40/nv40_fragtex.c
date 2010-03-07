#include "util/u_format.h"

#include "nv40_context.h"

#define _(m,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w,sx,sy,sz,sw)            \
{                                                                              \
  TRUE,                                                                        \
  PIPE_FORMAT_##m,                                                             \
  NV40TCL_TEX_FORMAT_FORMAT_##tf,                                              \
  (NV40TCL_TEX_SWIZZLE_S0_X_##ts0x | NV40TCL_TEX_SWIZZLE_S0_Y_##ts0y |         \
   NV40TCL_TEX_SWIZZLE_S0_Z_##ts0z | NV40TCL_TEX_SWIZZLE_S0_W_##ts0w |         \
   NV40TCL_TEX_SWIZZLE_S1_X_##ts1x | NV40TCL_TEX_SWIZZLE_S1_Y_##ts1y |         \
   NV40TCL_TEX_SWIZZLE_S1_Z_##ts1z | NV40TCL_TEX_SWIZZLE_S1_W_##ts1w),         \
  ((NV40TCL_TEX_FILTER_SIGNED_RED*sx) | (NV40TCL_TEX_FILTER_SIGNED_GREEN*sy) |       \
   (NV40TCL_TEX_FILTER_SIGNED_BLUE*sz) | (NV40TCL_TEX_FILTER_SIGNED_ALPHA*sw))       \
}

struct nv40_texture_format {
	boolean defined;
	uint	pipe;
	int     format;
	int     swizzle;
	int     sign;
};

static struct nv40_texture_format
nv40_texture_formats[] = {
	_(B8G8R8X8_UNORM, A8R8G8B8,   S1,   S1,   S1,  ONE, X, Y, Z, W, 0, 0, 0, 0),
	_(B8G8R8A8_UNORM, A8R8G8B8,   S1,   S1,   S1,   S1, X, Y, Z, W, 0, 0, 0, 0),
	_(B5G5R5A1_UNORM, A1R5G5B5,   S1,   S1,   S1,   S1, X, Y, Z, W, 0, 0, 0, 0),
	_(B4G4R4A4_UNORM, A4R4G4B4,   S1,   S1,   S1,   S1, X, Y, Z, W, 0, 0, 0, 0),
	_(B5G6R5_UNORM  , R5G6B5  ,   S1,   S1,   S1,  ONE, X, Y, Z, W, 0, 0, 0, 0),
	_(L8_UNORM      , L8      ,   S1,   S1,   S1,  ONE, X, X, X, X, 0, 0, 0, 0),
	_(A8_UNORM      , L8      , ZERO, ZERO, ZERO,   S1, X, X, X, X, 0, 0, 0, 0),
	_(R16_SNORM     , A16     , ZERO, ZERO,   S1,  ONE, X, X, X, Y, 1, 1, 1, 1),
	_(I8_UNORM      , L8      ,   S1,   S1,   S1,   S1, X, X, X, X, 0, 0, 0, 0),
	_(L8A8_UNORM    , A8L8    ,   S1,   S1,   S1,   S1, X, X, X, Y, 0, 0, 0, 0),
	_(Z16_UNORM     , Z16     ,   S1,   S1,   S1,  ONE, X, X, X, X, 0, 0, 0, 0),
	_(S8Z24_UNORM   , Z24     ,   S1,   S1,   S1,  ONE, X, X, X, X, 0, 0, 0, 0),
	_(DXT1_RGB      , DXT1    ,   S1,   S1,   S1,  ONE, X, Y, Z, W, 0, 0, 0, 0),
	_(DXT1_RGBA     , DXT1    ,   S1,   S1,   S1,   S1, X, Y, Z, W, 0, 0, 0, 0),
	_(DXT3_RGBA     , DXT3    ,   S1,   S1,   S1,   S1, X, Y, Z, W, 0, 0, 0, 0),
	_(DXT5_RGBA     , DXT5    ,   S1,   S1,   S1,   S1, X, Y, Z, W, 0, 0, 0, 0),
	{},
};

static struct nv40_texture_format *
nv40_fragtex_format(uint pipe_format)
{
	struct nv40_texture_format *tf = nv40_texture_formats;

	while (tf->defined) {
		if (tf->pipe == pipe_format)
			return tf;
		tf++;
	}

	NOUVEAU_ERR("unknown texture format %s\n", util_format_name(pipe_format));
	return NULL;
}


static struct nouveau_stateobj *
nv40_fragtex_build(struct nv40_context *nv40, int unit)
{
	struct nv40_sampler_state *ps = nv40->tex_sampler[unit];
	struct nv40_miptree *nv40mt = nv40->tex_miptree[unit];
	struct nouveau_bo *bo = nouveau_bo(nv40mt->buffer);
	struct pipe_texture *pt = &nv40mt->base;
	struct nv40_texture_format *tf;
	struct nouveau_stateobj *so;
	uint32_t txf, txs, txp;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	tf = nv40_fragtex_format(pt->format);
	if (!tf)
		assert(0);

	txf  = ps->fmt;
	txf |= tf->format | 0x8000;
	txf |= ((pt->last_level + 1) << NV40TCL_TEX_FORMAT_MIPMAP_COUNT_SHIFT);

	if (1) /* XXX */
		txf |= NV40TCL_TEX_FORMAT_NO_BORDER;

	switch (pt->target) {
	case PIPE_TEXTURE_CUBE:
		txf |= NV40TCL_TEX_FORMAT_CUBIC;
		/* fall-through */
	case PIPE_TEXTURE_2D:
		txf |= NV40TCL_TEX_FORMAT_DIMS_2D;
		break;
	case PIPE_TEXTURE_3D:
		txf |= NV40TCL_TEX_FORMAT_DIMS_3D;
		break;
	case PIPE_TEXTURE_1D:
		txf |= NV40TCL_TEX_FORMAT_DIMS_1D;
		break;
	default:
		NOUVEAU_ERR("Unknown target %d\n", pt->target);
		return NULL;
	}

	if (!(pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR)) {
		txp = 0;
	} else {
		txp  = nv40mt->level[0].pitch;
		txf |= NV40TCL_TEX_FORMAT_LINEAR;
	}

	txs = tf->swizzle;

	so = so_new(2, 9, 2);
	so_method(so, nv40->screen->curie, NV40TCL_TEX_OFFSET(unit), 8);
	so_reloc (so, bo, 0, tex_flags | NOUVEAU_BO_LOW, 0, 0);
	so_reloc (so, bo, txf, tex_flags | NOUVEAU_BO_OR,
		      NV40TCL_TEX_FORMAT_DMA0, NV40TCL_TEX_FORMAT_DMA1);
	so_data  (so, ps->wrap);
	so_data  (so, NV40TCL_TEX_ENABLE_ENABLE | ps->en);
	so_data  (so, txs);
	so_data  (so, ps->filt | tf->sign | 0x2000 /*voodoo*/);
	so_data  (so, (pt->width0 << NV40TCL_TEX_SIZE0_W_SHIFT) |
		       pt->height0);
	so_data  (so, ps->bcol);
	so_method(so, nv40->screen->curie, NV40TCL_TEX_SIZE1(unit), 1);
	so_data  (so, (pt->depth0 << NV40TCL_TEX_SIZE1_DEPTH_SHIFT) | txp);

	return so;
}

static boolean
nv40_fragtex_validate(struct nv40_context *nv40)
{
	struct nv40_fragment_program *fp = nv40->fragprog;
	struct nv40_state *state = &nv40->state;
	struct nouveau_stateobj *so;
	unsigned samplers, unit;

	samplers = state->fp_samplers & ~fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		so = so_new(1, 1, 0);
		so_method(so, nv40->screen->curie, NV40TCL_TEX_ENABLE(unit), 1);
		so_data  (so, 0);
		so_ref(so, &nv40->state.hw[NV40_STATE_FRAGTEX0 + unit]);
		state->dirty |= (1ULL << (NV40_STATE_FRAGTEX0 + unit));
	}

	samplers = nv40->dirty_samplers & fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		so = nv40_fragtex_build(nv40, unit);
		so_ref(so, &nv40->state.hw[NV40_STATE_FRAGTEX0 + unit]);
		so_ref(NULL, &so);
		state->dirty |= (1ULL << (NV40_STATE_FRAGTEX0 + unit));
	}

	nv40->state.fp_samplers = fp->samplers;
	return FALSE;
}

struct nv40_state_entry nv40_state_fragtex = {
	.validate = nv40_fragtex_validate,
	.dirty = {
		.pipe = NV40_NEW_SAMPLER | NV40_NEW_FRAGPROG,
		.hw = 0
	}
};

