#include "nv30_context.h"

static INLINE int log2i(int i)
{
	int r = 0;

	if (i & 0xffff0000) {
		i >>= 16;
		r += 16;
	}
	if (i & 0x0000ff00) {
		i >>= 8;
		r += 8;
	}
	if (i & 0x000000f0) {
		i >>= 4;
		r += 4;
	}
	if (i & 0x0000000c) {
		i >>= 2;
		r += 2;
	}
	if (i & 0x00000002) {
		r += 1;
	}
	return r;
}

#define _(m,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w)                        \
{                                                                              \
  TRUE,                                                                        \
  PIPE_FORMAT_##m,                                                             \
  NV34TCL_TX_FORMAT_FORMAT_##tf,                                               \
  (NV34TCL_TX_SWIZZLE_S0_X_##ts0x | NV34TCL_TX_SWIZZLE_S0_Y_##ts0y |           \
   NV34TCL_TX_SWIZZLE_S0_Z_##ts0z | NV34TCL_TX_SWIZZLE_S0_W_##ts0w |           \
   NV34TCL_TX_SWIZZLE_S1_X_##ts1x | NV34TCL_TX_SWIZZLE_S1_Y_##ts1y |           \
   NV34TCL_TX_SWIZZLE_S1_Z_##ts1z | NV34TCL_TX_SWIZZLE_S1_W_##ts1w),           \
}

struct nv30_texture_format {
	boolean defined;
	uint	pipe;
	int     format;
	int     swizzle;
};

static struct nv30_texture_format
nv30_texture_formats[] = {
	_(A8R8G8B8_UNORM, A8R8G8B8,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(A1R5G5B5_UNORM, A1R5G5B5,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(A4R4G4B4_UNORM, A4R4G4B4,   S1,   S1,   S1,   S1, X, Y, Z, W),
//	_(R5G6B5_UNORM  , R5G6B5  ,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(L8_UNORM      , L8      ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(A8_UNORM      , L8      , ZERO, ZERO, ZERO,   S1, X, X, X, X),
	_(I8_UNORM      , L8      ,   S1,   S1,   S1,   S1, X, X, X, X),
	_(A8L8_UNORM    , A8L8    ,   S1,   S1,   S1,   S1, X, X, X, Y),
//	_(Z16_UNORM     , Z16     ,   S1,   S1,   S1,  ONE, X, X, X, X),
//	_(Z24S8_UNORM   , Z24     ,   S1,   S1,   S1,  ONE, X, X, X, X),
//	_(RGB_DXT1      , 0x86,   S1,   S1,   S1,  ONE, X, Y, Z, W, 0x00, 0x00),
//	_(RGBA_DXT1     , 0x86,   S1,   S1,   S1,   S1, X, Y, Z, W, 0x00, 0x00),
//	_(RGBA_DXT3     , 0x87,   S1,   S1,   S1,   S1, X, Y, Z, W, 0x00, 0x00),
//	_(RGBA_DXT5     , 0x88,   S1,   S1,   S1,   S1, X, Y, Z, W, 0x00, 0x00),
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

	return NULL;
}


static void
nv30_fragtex_build(struct nv30_context *nv30, int unit)
{
	struct nv30_sampler_state *ps = nv30->tex_sampler[unit];
	struct nv30_miptree *nv30mt = nv30->tex_miptree[unit];
	struct pipe_texture *pt = &nv30mt->base;
	struct nv30_texture_format *tf;
	uint32_t txf, txs, txp;
	int swizzled = 0; /*XXX: implement in region code? */

	tf = nv30_fragtex_format(pt->format);
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
/*	case PIPE_TEXTURE_CUBE:
		txf |= NV34TCL_TEX_FORMAT_CUBIC;*/
		/* fall-through */
	case PIPE_TEXTURE_2D:
		txf |= (2<<4);
		break;
	case PIPE_TEXTURE_3D:
		txf |= (3<<4);
		break;
	case PIPE_TEXTURE_1D:
		txf |= (1<<4);
		break;
	default:
		NOUVEAU_ERR("Unknown target %d\n", pt->target);
		return;
	}

	txs = tf->swizzle;

	BEGIN_RING(rankine, NV34TCL_TX_OFFSET(unit), 8);
	OUT_RELOCl(nv30mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
	OUT_RELOCd(nv30mt->buffer,txf,NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_OR | NOUVEAU_BO_RD, 1/*VRAM*/,2/*TT*/);
	OUT_RING  (ps->wrap);
	OUT_RING  (0x40000000); /* enable */
	OUT_RING  (txs);
	OUT_RING  (ps->filt | 0x2000 /* magic */);
	OUT_RING  ((pt->width[0] << 16) | pt->height[0]);
	OUT_RING  (ps->bcol);
}

void
nv30_fragtex_bind(struct nv30_context *nv30)
{
	struct nv30_fragment_program *fp = nv30->fragprog.active;
	unsigned samplers, unit;

	samplers = nv30->fp_samplers & ~fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		BEGIN_RING(rankine, NV34TCL_TX_ENABLE(unit), 1);
		OUT_RING  (0);
	}

	samplers = nv30->dirty_samplers & fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		nv30_fragtex_build(nv30, unit);
	}

	nv30->fp_samplers = fp->samplers;
}

