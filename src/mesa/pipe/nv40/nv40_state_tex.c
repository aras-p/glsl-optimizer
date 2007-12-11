#include "nv40_context.h"
#include "nv40_dma.h"

#define _(m,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w)                        \
{                                                                              \
  TRUE,                                                                        \
  PIPE_FORMAT_##m,                                                             \
  NV40TCL_TEX_FORMAT_FORMAT_##tf,                                              \
  (NV40TCL_TEX_SWIZZLE_S0_X_##ts0x | NV40TCL_TEX_SWIZZLE_S0_Y_##ts0y |         \
   NV40TCL_TEX_SWIZZLE_S0_Z_##ts0z | NV40TCL_TEX_SWIZZLE_S0_W_##ts0w |         \
   NV40TCL_TEX_SWIZZLE_S1_X_##ts1x | NV40TCL_TEX_SWIZZLE_S1_Y_##ts1y |         \
   NV40TCL_TEX_SWIZZLE_S1_Z_##ts1z | NV40TCL_TEX_SWIZZLE_S1_W_##ts1w),         \
}

#define NV40TCL_TEX_FORMAT_FORMAT_Z16 0x1200
#define NV40TCL_TEX_FORMAT_FORMAT_Z24 0x1000

struct nv40_texture_format {
	boolean defined;
	uint	pipe;
	int     format;
	int     swizzle;
};

static struct nv40_texture_format
nv40_texture_formats[] = {
	_(A8R8G8B8_UNORM, A8R8G8B8,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(A1R5G5B5_UNORM, A1R5G5B5,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(A4R4G4B4_UNORM, A4R4G4B4,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(R5G6B5_UNORM  , R5G6B5  ,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(U_L8          , L8      ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(U_A8          , L8      , ZERO, ZERO, ZERO,   S1, X, X, X, X),
	_(U_I8          , L8      ,   S1,   S1,   S1,   S1, X, X, X, X),
	_(U_A8_L8       , A8L8    ,   S1,   S1,   S1,   S1, Z, W, X, Y),
	_(Z16_UNORM     , Z16     ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(Z24S8_UNORM   , Z24     ,   S1,   S1,   S1,  ONE, X, X, X, X),
//	_(RGB_DXT1      , 0x86,   S1,   S1,   S1,  ONE, X, Y, Z, W, 0x00, 0x00),
//	_(RGBA_DXT1     , 0x86,   S1,   S1,   S1,   S1, X, Y, Z, W, 0x00, 0x00),
//	_(RGBA_DXT3     , 0x87,   S1,   S1,   S1,   S1, X, Y, Z, W, 0x00, 0x00),
//	_(RGBA_DXT5     , 0x88,   S1,   S1,   S1,   S1, X, Y, Z, W, 0x00, 0x00),
	{},
};

static struct nv40_texture_format *
nv40_tex_format(uint pipe_format)
{
	struct nv40_texture_format *tf = nv40_texture_formats;

	while (tf->defined) {
		if (tf->pipe == pipe_format)
			return tf;
		tf++;
	}

	return NULL;
}


static void
nv40_tex_unit_enable(struct nv40_context *nv40, int unit)
{
	struct nv40_sampler_state *ps = nv40->tex_sampler[unit];
	struct pipe_texture *pt = nv40->tex_miptree[unit];
	struct nv40_miptree *nv40mt = (struct nv40_miptree *)pt;
	struct nv40_texture_format *tf;
	uint32_t txf, txs, txp;
	int swizzled = 0; /*XXX: implement in region code? */

	tf = nv40_tex_format(pt->format);
	if (!tf || !tf->defined) {
		NOUVEAU_ERR("Unsupported texture format: 0x%x\n", pt->format);
		return;
	}

	txf  = ps->fmt;
	txf |= tf->format | 0x8000;
	txf |= ((pt->last_level - pt->first_level + 1) <<
		NV40TCL_TEX_FORMAT_MIPMAP_COUNT_SHIFT);

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
		return;
	}

	if (swizzled) {
		txp = 0;
	} else {
		txp  = nv40mt->level[0].pitch;
		txf |= NV40TCL_TEX_FORMAT_LINEAR;
	}

	txs = tf->swizzle;
	if (pt->format == PIPE_FORMAT_U_A8_L8)
		txs |= (1<<16); /*nfi*/

	BEGIN_RING(curie, NV40TCL_TEX_OFFSET(unit), 8);
	OUT_RELOCl(nv40mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
		   NOUVEAU_BO_RD);
	OUT_RELOCd(nv40mt->buffer, txf, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
		   NOUVEAU_BO_OR | NOUVEAU_BO_RD, NV40TCL_TEX_FORMAT_DMA0,
		   NV40TCL_TEX_FORMAT_DMA1);
	OUT_RING  (ps->wrap);
	OUT_RING  (NV40TCL_TEX_ENABLE_ENABLE | ps->en |
		   (0x00078000) /* mipmap related? */);
	OUT_RING  (txs);
	OUT_RING  (ps->filt | 0x3fd6 /*voodoo*/);
	OUT_RING  ((pt->width[0] << NV40TCL_TEX_SIZE0_W_SHIFT) | pt->height[0]);
	OUT_RING  (ps->bcol);
	BEGIN_RING(curie, NV40TCL_TEX_SIZE1(unit), 1);
	OUT_RING  ((pt->depth[0] << NV40TCL_TEX_SIZE1_DEPTH_SHIFT) | txp);
}

void
nv40_state_tex_update(struct nv40_context *nv40)
{
	while (nv40->tex_dirty) {
		int unit = ffs(nv40->tex_dirty) - 1;

		if (nv40->tex_miptree[unit]) {
			nv40_tex_unit_enable(nv40, unit);
		} else {
			BEGIN_RING(curie, NV40TCL_TEX_ENABLE(unit), 1);
			OUT_RING  (0);
		}

		nv40->tex_dirty &= ~(1 << unit);
	}
}

