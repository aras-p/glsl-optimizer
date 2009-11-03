
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
	int level_depth;
	int level_x;
	int level_y;
};

static void
nv50_transfer_rect_m2mf(struct pipe_screen *pscreen,
			struct nouveau_bo *src_bo, unsigned src_offset,
			int src_pitch, unsigned src_tile_mode,
			int sx, int sy, int sw, int sh, int sd,
			struct nouveau_bo *dst_bo, unsigned dst_offset,
			int dst_pitch, unsigned dst_tile_mode,
			int dx, int dy, int dw, int dh, int dd,
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
		OUT_RING  (chan, sd);
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
		OUT_RING  (chan, dd);
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
			OUT_RING  (chan, (sy << 16) | (sx * cpp));
		} else {
			src_offset += (line_count * src_pitch);
		}
		if (dst_bo->tile_flags) {
			BEGIN_RING(chan, m2mf,
				NV50_MEMORY_TO_MEMORY_FORMAT_TILING_POSITION_OUT, 1);
			OUT_RING  (chan, (dy << 16) | (dx * cpp));
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

static INLINE unsigned
get_zslice_offset(unsigned tile_mode, unsigned z, unsigned pitch, unsigned ny)
{
	unsigned tile_h = get_tile_height(tile_mode);
	unsigned tile_d = get_tile_depth(tile_mode);

	/* pitch_2d == to next slice within this volume-tile */
	/* pitch_3d == to next slice in next 2D array of blocks */
	unsigned pitch_2d = tile_h * 64;
	unsigned pitch_3d = tile_d * align(ny, tile_h) * pitch;

	return (z % tile_d) * pitch_2d + (z / tile_d) * pitch_3d;
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
	unsigned nx, ny, image = 0;
	int ret;

	if (pt->target == PIPE_TEXTURE_CUBE)
		image = face;

	tx = CALLOC_STRUCT(nv50_transfer);
	if (!tx)
		return NULL;

	pipe_texture_reference(&tx->base.texture, pt);
	tx->base.format = pt->format;
	tx->base.width = w;
	tx->base.height = h;
	tx->base.block = pt->block;
	if (!pt->nblocksx[level]) {
		tx->base.nblocksx = pf_get_nblocksx(&pt->block,
						    pt->width[level]);
		tx->base.nblocksy = pf_get_nblocksy(&pt->block,
						    pt->height[level]);
	} else {
		tx->base.nblocksx = pt->nblocksx[level];
		tx->base.nblocksy = pt->nblocksy[level];
	}
	tx->base.stride = tx->base.nblocksx * pt->block.size;
	tx->base.usage = usage;

	tx->level_pitch = lvl->pitch;
	tx->level_width = mt->base.base.width[level];
	tx->level_height = mt->base.base.height[level];
	tx->level_depth = mt->base.base.depth[level];
	tx->level_offset = lvl->image_offset[image];
	tx->level_tiling = lvl->tile_mode;
	tx->level_x = pf_get_nblocksx(&tx->base.block, x);
	tx->level_y = pf_get_nblocksy(&tx->base.block, y);
	ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
			     tx->base.nblocksy * tx->base.stride, &tx->bo);
	if (ret) {
		FREE(tx);
		return NULL;
	}

	if (pt->target == PIPE_TEXTURE_3D)
		tx->level_offset += get_zslice_offset(lvl->tile_mode, zslice,
						      lvl->pitch,
						      tx->base.nblocksy);

	if (usage & PIPE_TRANSFER_READ) {
		nx = pf_get_nblocksx(&tx->base.block, tx->base.width);
		ny = pf_get_nblocksy(&tx->base.block, tx->base.height);

		nv50_transfer_rect_m2mf(pscreen, mt->base.bo, tx->level_offset,
					tx->level_pitch, tx->level_tiling,
					x, y,
					tx->base.nblocksx, tx->base.nblocksy,
					tx->level_depth,
					tx->bo, 0,
					tx->base.stride, tx->bo->tile_mode,
					0, 0,
					tx->base.nblocksx, tx->base.nblocksy, 1,
					tx->base.block.size, nx, ny,
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

	unsigned nx = pf_get_nblocksx(&tx->base.block, tx->base.width);
	unsigned ny = pf_get_nblocksy(&tx->base.block, tx->base.height);

	if (ptx->usage & PIPE_TRANSFER_WRITE) {
		struct pipe_screen *pscreen = ptx->texture->screen;

		nv50_transfer_rect_m2mf(pscreen, tx->bo, 0,
					tx->base.stride, tx->bo->tile_mode,
					0, 0,
					tx->base.nblocksx, tx->base.nblocksy, 1,
					mt->base.bo, tx->level_offset,
					tx->level_pitch, tx->level_tiling,
					tx->level_x, tx->level_y,
					tx->base.nblocksx, tx->base.nblocksy,
					tx->level_depth,
					tx->base.block.size, nx, ny,
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

void
nv50_upload_sifc(struct nv50_context *nv50,
		 struct nouveau_bo *bo, unsigned dst_offset, unsigned reloc,
		 unsigned dst_format, int dst_w, int dst_h, int dst_pitch,
		 void *src, unsigned src_format, int src_pitch,
		 int x, int y, int w, int h, int cpp)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *eng2d = nv50->screen->eng2d;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned line_dwords = (w * cpp + 3) / 4;

	reloc |= NOUVEAU_BO_WR;

	WAIT_RING (chan, 32);

	if (bo->tile_flags) {
		BEGIN_RING(chan, eng2d, NV50_2D_DST_FORMAT, 5);
		OUT_RING  (chan, dst_format);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, bo->tile_mode << 4);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	} else {
		BEGIN_RING(chan, eng2d, NV50_2D_DST_FORMAT, 2);
		OUT_RING  (chan, dst_format);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, eng2d, NV50_2D_DST_PITCH, 1);
		OUT_RING  (chan, dst_pitch);
	}

	BEGIN_RING(chan, eng2d, NV50_2D_DST_WIDTH, 4);
	OUT_RING  (chan, dst_w);
	OUT_RING  (chan, dst_h);
	OUT_RELOCh(chan, bo, dst_offset, reloc);
	OUT_RELOCl(chan, bo, dst_offset, reloc);

	/* NV50_2D_OPERATION_SRCCOPY assumed already set */

	BEGIN_RING(chan, eng2d, NV50_2D_SIFC_UNK0800, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, src_format);
	BEGIN_RING(chan, eng2d, NV50_2D_SIFC_WIDTH, 10);
	OUT_RING  (chan, w);
	OUT_RING  (chan, h);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, x);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, y);

	while (h--) {
		const uint32_t *p = src;
		unsigned count = line_dwords;

		while (count) {
			unsigned nr = MIN2(count, 1792);

			if (chan->pushbuf->remaining <= nr) {
				FIRE_RING (chan);

				BEGIN_RING(chan, eng2d,
					   NV50_2D_DST_ADDRESS_HIGH, 2);
				OUT_RELOCh(chan, bo, dst_offset, reloc);
				OUT_RELOCl(chan, bo, dst_offset, reloc);
			}
			assert(chan->pushbuf->remaining > nr);

			BEGIN_RING(chan, eng2d,
				   NV50_2D_SIFC_DATA | (2 << 29), nr);
			OUT_RINGp (chan, p, nr);

			p += nr;
			count -= nr;
		}

		src += src_pitch;
	}

	BEGIN_RING(chan, tesla, 0x1440, 1);
	OUT_RING  (chan, 0);
}
