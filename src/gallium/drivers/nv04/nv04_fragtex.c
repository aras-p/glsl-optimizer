#include "nv04_context.h"
#include "nouveau/nouveau_util.h"

#define _(m,tf)                                                                \
{                                                                              \
  PIPE_FORMAT_##m,                                                             \
  NV04_DX5_TEXTURED_TRIANGLE_FORMAT_COLOR_##tf,                                               \
}

struct nv04_texture_format {
	uint	pipe;
	int     format;
};

static struct nv04_texture_format
nv04_texture_formats[] = {
	_(A8R8G8B8_UNORM, A8R8G8B8),
	_(X8R8G8B8_UNORM, X8R8G8B8),
	_(A1R5G5B5_UNORM, A1R5G5B5),
	_(A4R4G4B4_UNORM, A4R4G4B4),
	_(L8_UNORM,       Y8      ),
	_(A8_UNORM,       Y8      ),
};

static uint32_t
nv04_fragtex_format(uint pipe_format)
{
	struct nv04_texture_format *tf = nv04_texture_formats;
	int i;

	for (i=0; i< sizeof(nv04_texture_formats)/sizeof(nv04_texture_formats[0]); i++) {
		if (tf->pipe == pipe_format)
			return tf->format;
		tf++;
	}

	NOUVEAU_ERR("unknown texture format %s\n", pf_name(pipe_format));
	return 0;
}


static void
nv04_fragtex_build(struct nv04_context *nv04, int unit)
{
	struct nv04_miptree *nv04mt = nv04->tex_miptree[unit];
	struct pipe_texture *pt = &nv04mt->base;

	switch (pt->target) {
	case PIPE_TEXTURE_2D:
		break;
	default:
		NOUVEAU_ERR("Unknown target %d\n", pt->target);
		return;
	}

	nv04->fragtex.format = NV04_DX5_TEXTURED_TRIANGLE_FORMAT_ORIGIN_ZOH_CORNER 
		| NV04_DX5_TEXTURED_TRIANGLE_FORMAT_ORIGIN_FOH_CORNER
		| nv04_fragtex_format(pt->format)
		| ( (pt->last_level + 1) << NV04_DX5_TEXTURED_TRIANGLE_FORMAT_MIPMAP_LEVELS_SHIFT )
		| ( log2i(pt->width[0]) << NV04_DX5_TEXTURED_TRIANGLE_FORMAT_BASE_SIZE_U_SHIFT )
		| ( log2i(pt->height[0]) << NV04_DX5_TEXTURED_TRIANGLE_FORMAT_BASE_SIZE_V_SHIFT )
		| NV04_DX5_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_CLAMP_TO_EDGE
		| NV04_DX5_TEXTURED_TRIANGLE_FORMAT_ADDRESSV_CLAMP_TO_EDGE
		;
}


void
nv04_fragtex_bind(struct nv04_context *nv04)
{
	nv04_fragtex_build(nv04, 0);
}

