#include "nv20_context.h"
#include "nouveau/nouveau_util.h"

#define _(m,tf)                                                                \
{                                                                              \
  TRUE,                                                                        \
  PIPE_FORMAT_##m,                                                             \
  NV20TCL_TX_FORMAT_FORMAT_##tf,                                               \
}

struct nv20_texture_format {
	boolean defined;
	uint	pipe;
	int     format;
};

static struct nv20_texture_format
nv20_texture_formats[] = {
	_(A8R8G8B8_UNORM, A8R8G8B8),
	_(A1R5G5B5_UNORM, A1R5G5B5),
	_(A4R4G4B4_UNORM, A4R4G4B4),
	_(L8_UNORM      , L8      ),
	_(A8_UNORM      , A8      ),
	_(A8L8_UNORM    , A8L8    ),
/*	_(RGB_DXT1      , DXT1,   ), */
/*	_(RGBA_DXT1     , DXT1,   ), */
/*	_(RGBA_DXT3     , DXT3,   ), */
/*	_(RGBA_DXT5     , DXT5,   ), */
	{},
};

static struct nv20_texture_format *
nv20_fragtex_format(uint pipe_format)
{
	struct nv20_texture_format *tf = nv20_texture_formats;

	while (tf->defined) {
		if (tf->pipe == pipe_format)
			return tf;
		tf++;
	}

	return NULL;
}


static void
nv20_fragtex_build(struct nv20_context *nv20, int unit)
{
#if 0
	struct nv20_sampler_state *ps = nv20->tex_sampler[unit];
	struct nv20_miptree *nv20mt = nv20->tex_miptree[unit];
	struct pipe_texture *pt = &nv20mt->base;
	struct nv20_texture_format *tf;
	uint32_t txf, txs, txp;

	tf = nv20_fragtex_format(pt->format);
	if (!tf || !tf->defined) {
		NOUVEAU_ERR("Unsupported texture format: 0x%x\n", pt->format);
		return;
	}

	txf  = tf->format << 8;
	txf |= (pt->last_level + 1) << 16;
	txf |= log2i(pt->width[0]) << 20;
	txf |= log2i(pt->height[0]) << 24;
	txf |= log2i(pt->depth[0]) << 28;
	txf |= 8;

	switch (pt->target) {
	case PIPE_TEXTURE_CUBE:
		txf |= NV10TCL_TX_FORMAT_CUBE_MAP;
		/* fall-through */
	case PIPE_TEXTURE_2D:
		txf |= (2<<4);
		break;
	case PIPE_TEXTURE_1D:
		txf |= (1<<4);
		break;
	default:
		NOUVEAU_ERR("Unknown target %d\n", pt->target);
		return;
	}

	BEGIN_RING(kelvin, NV10TCL_TX_OFFSET(unit), 8);
	OUT_RELOCl(nv20mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
	OUT_RELOCd(nv20mt->buffer,txf,NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_OR | NOUVEAU_BO_RD, 1/*VRAM*/,2/*TT*/);
	OUT_RING  (ps->wrap);
	OUT_RING  (0x40000000); /* enable */
	OUT_RING  (txs);
	OUT_RING  (ps->filt | 0x2000 /* magic */);
	OUT_RING  ((pt->width[0] << 16) | pt->height[0]);
	OUT_RING  (ps->bcol);
#endif
}

void
nv20_fragtex_bind(struct nv20_context *nv20)
{
#if 0
	struct nv20_fragment_program *fp = nv20->fragprog.active;
	unsigned samplers, unit;

	samplers = nv20->fp_samplers & ~fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		BEGIN_RING(kelvin, NV10TCL_TX_ENABLE(unit), 1);
		OUT_RING  (0);
	}

	samplers = nv20->dirty_samplers & fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		nv20_fragtex_build(nv20, unit);
	}

	nv20->fp_samplers = fp->samplers;
#endif
}

