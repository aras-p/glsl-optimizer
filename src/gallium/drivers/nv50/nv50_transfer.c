
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"

#include "nv50_context.h"

struct nv50_transfer {
	struct pipe_transfer base;
	struct nouveau_bo *bo;
	unsigned level_offset;
	unsigned level_tiling;
	int level_pitch;
	int level_width;
	int level_height;
	int level_x;
	int level_y;
};

static void
nv50_transfer_rect_m2mf(struct pipe_screen *pscreen,
			struct nouveau_bo *src_bo, unsigned src_offset,
			int src_pitch, unsigned src_tile_mode,
			int sx, int sy, int sw, int sh,
			struct nouveau_bo *dst_bo, unsigned dst_offset,
			int dst_pitch, unsigned dst_tile_mode,
			int dx, int dy, int dw, int dh,
			int cpp, int width, int height,
			unsigned src_reloc, unsigned dst_reloc)
{
	struct nv50_screen *screen = nv50_screen(pscreen);
	struct nouveau_channel *chan = screen->m2mf->channel;
	struct nouveau_grobj *m2mf = screen->m2mf;

	src_reloc |= NOUVEAU_BO_RD;
	dst_reloc |= NOUVEAU_BO_WR;

	WAIT_RING (chan, 14);

	if (!src_bo->tile_flags) {
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_IN, 1);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_PITCH_IN, 1);
		OUT_RING  (chan, src_pitch);
		src_offset += (sy * src_pitch) + (sx * cpp);
	} else {
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_IN, 6);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, src_tile_mode << 4);
		OUT_RING  (chan, sw * cpp);
		OUT_RING  (chan, sh);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	if (!dst_bo->tile_flags) {
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_OUT, 1);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_PITCH_OUT, 1);
		OUT_RING  (chan, dst_pitch);
		dst_offset += (dy * dst_pitch) + (dx * cpp);
	} else {
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_OUT, 6);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, dst_tile_mode << 4);
		OUT_RING  (chan, dw * cpp);
		OUT_RING  (chan, dh);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	while (height) {
		int line_count = height > 2047 ? 2047 : height;

		WAIT_RING (chan, 15);
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN_HIGH, 2);
		OUT_RELOCh(chan, src_bo, src_offset, src_reloc);
		OUT_RELOCh(chan, dst_bo, dst_offset, dst_reloc);
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 2);
		OUT_RELOCl(chan, src_bo, src_offset, src_reloc);
		OUT_RELOCl(chan, dst_bo, dst_offset, dst_reloc);
		if (src_bo->tile_flags) {
			BEGIN_RING(chan, m2mf,
				NV50_MEMORY_TO_MEMORY_FORMAT_TILING_POSITION_IN, 1);
			OUT_RING  (chan, (sy << 16) | sx);
		} else {
			src_offset += (line_count * src_pitch);
		}
		if (dst_bo->tile_flags) {
			BEGIN_RING(chan, m2mf,
				NV50_MEMORY_TO_MEMORY_FORMAT_TILING_POSITION_OUT, 1);
			OUT_RING  (chan, (dy << 16) | dx);
		} else {
			dst_offset += (line_count * dst_pitch);
		}
		BEGIN_RING(chan, m2mf,
			NV50_MEMORY_TO_MEMORY_FORMAT_LINE_LENGTH_IN, 4);
		OUT_RING  (chan, width * cpp);
		OUT_RING  (chan, line_count);
		OUT_RING  (chan, 0x00000101);
		OUT_RING  (chan, 0);
		FIRE_RING (chan);

		height -= line_count;
		sy += line_count;
		dy += line_count;
	}
}

static struct pipe_transfer *
nv50_transfer_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
		  unsigned face, unsigned level, unsigned zslice,
		  enum pipe_transfer_usage usage,
		  unsigned x, unsigned y, unsigned w, unsigned h)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nv50_miptree *mt = nv50_miptree(pt);
	struct nv50_miptree_level *lvl = &mt->level[level];
	struct nv50_transfer *tx;
	unsigned image = 0;
	int ret;

	if (pt->target == PIPE_TEXTURE_CUBE)
		image = face;
	else
	if (pt->target == PIPE_TEXTURE_3D)
		image = zslice;

	tx = CALLOC_STRUCT(nv50_transfer);
	if (!tx)
		return NULL;

	pipe_texture_reference(&tx->base.texture, pt);
	tx->base.format = pt->format;
	tx->base.width = w;
	tx->base.height = h;
	tx->base.block = pt->block;
	tx->base.nblocksx = pt->nblocksx[level];
	tx->base.nblocksy = pt->nblocksy[level];
	tx->base.stride = (w * pt->block.size);
	tx->base.usage = usage;

	tx->level_pitch = lvl->pitch;
	tx->level_width = mt->base.base.width[level];
	tx->level_height = mt->base.base.height[level];
	tx->level_offset = lvl->image_offset[image];
	tx->level_tiling = lvl->tile_mode;
	tx->level_x = x;
	tx->level_y = y;
	ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
			     w * pt->block.size * h, &tx->bo);
	if (ret) {
		FREE(tx);
		return NULL;
	}

	if (usage != PIPE_TRANSFER_WRITE) {
		nv50_transfer_rect_m2mf(pscreen, mt->base.bo, tx->level_offset,
					tx->level_pitch, tx->level_tiling,
					x, y,
					tx->level_width, tx->level_height,
					tx->bo, 0, tx->base.stride,
					tx->bo->tile_mode, 0, 0,
					tx->base.width, tx->base.height,
					tx->base.block.size, w, h,
					NOUVEAU_BO_VRAM | NOUVEAU_BO_GART,
					NOUVEAU_BO_GART);
	}

	return &tx->base;
}

static void
nv50_transfer_del(struct pipe_transfer *ptx)
{
	struct nv50_transfer *tx = (struct nv50_transfer *)ptx;
	struct nv50_miptree *mt = nv50_miptree(ptx->texture);

	if (ptx->usage != PIPE_TRANSFER_READ) {
		struct pipe_screen *pscreen = ptx->texture->screen;
		nv50_transfer_rect_m2mf(pscreen, tx->bo, 0, tx->base.stride,
					tx->bo->tile_mode, 0, 0,
					tx->base.width, tx->base.height,
					mt->base.bo, tx->level_offset,
					tx->level_pitch, tx->level_tiling,
					tx->level_x, tx->level_y,
					tx->level_width, tx->level_height,
					tx->base.block.size, tx->base.width,
					tx->base.height,
					NOUVEAU_BO_GART, NOUVEAU_BO_VRAM |
					NOUVEAU_BO_GART);
	}

	nouveau_bo_ref(NULL, &tx->bo);
	pipe_texture_reference(&ptx->texture, NULL);
	FREE(ptx);
}

static void *
nv50_transfer_map(struct pipe_screen *pscreen, struct pipe_transfer *ptx)
{
	struct nv50_transfer *tx = (struct nv50_transfer *)ptx;
	unsigned flags = 0;
	int ret;

	if (ptx->usage & PIPE_TRANSFER_WRITE)
		flags |= NOUVEAU_BO_WR;
	if (ptx->usage & PIPE_TRANSFER_READ)
		flags |= NOUVEAU_BO_RD;

	ret = nouveau_bo_map(tx->bo, flags);
	if (ret)
		return NULL;
	return tx->bo->map;
}

static void
nv50_transfer_unmap(struct pipe_screen *pscreen, struct pipe_transfer *ptx)
{
	struct nv50_transfer *tx = (struct nv50_transfer *)ptx;

	nouveau_bo_unmap(tx->bo);
}

void
nv50_transfer_init_screen_functions(struct pipe_screen *pscreen)
{
	pscreen->get_tex_transfer = nv50_transfer_new;
	pscreen->tex_transfer_destroy = nv50_transfer_del;
	pscreen->transfer_map = nv50_transfer_map;
	pscreen->transfer_unmap = nv50_transfer_unmap;
}
