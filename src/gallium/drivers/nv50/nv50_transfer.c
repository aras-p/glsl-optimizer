
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"

#include "nv50_context.h"

struct nv50_transfer {
	struct pipe_transfer base;
	struct pipe_buffer *buffer;
	struct nv50_miptree_level *level;
	int level_pitch;
	int level_width;
	int level_height;
	int level_x;
	int level_y;
};

static void
nv50_transfer_rect_m2mf(struct pipe_screen *pscreen, struct pipe_buffer *src,
			int src_pitch, int sx, int sy, int sw, int sh,
			struct pipe_buffer *dst, int dst_pitch, int dx, int dy,
			int dw, int dh, int cpp, int width, int height,
			unsigned src_reloc, unsigned dst_reloc)
{
	struct nv50_screen *screen = nv50_screen(pscreen);
	struct nouveau_winsys *nvws = screen->nvws;
	struct nouveau_channel *chan = nvws->channel;
	struct nouveau_grobj *m2mf = screen->m2mf;
	struct nouveau_bo *src_bo = nvws->get_bo(src);
	struct nouveau_bo *dst_bo = nvws->get_bo(dst);
	unsigned src_offset = 0, dst_offset = 0;

	src_reloc |= NOUVEAU_BO_RD;
	dst_reloc |= NOUVEAU_BO_WR;

	WAIT_RING (chan, 14);

	if (!src_bo->tiled) {
		BEGIN_RING(chan, m2mf, 0x0200, 1);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, m2mf, 0x0314, 1);
		OUT_RING  (chan, src_pitch);
		src_offset = (sy * src_pitch) + (sx * cpp);
	} else {
		BEGIN_RING(chan, m2mf, 0x0200, 6);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, sw * cpp);
		OUT_RING  (chan, sh);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	if (!dst_bo->tiled) {
		BEGIN_RING(chan, m2mf, 0x021c, 1);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, m2mf, 0x0318, 1);
		OUT_RING  (chan, dst_pitch);
		dst_offset = (dy * dst_pitch) + (dx * cpp);
	} else {
		BEGIN_RING(chan, m2mf, 0x021c, 6);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, dw * cpp);
		OUT_RING  (chan, dh);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	while (height) {
		int line_count = height > 2047 ? 2047 : height;

		WAIT_RING (chan, 15);
		BEGIN_RING(chan, m2mf, 0x0238, 2);
		OUT_RELOCh(chan, src_bo, src_offset, src_reloc);
		OUT_RELOCh(chan, dst_bo, dst_offset, dst_reloc);
		BEGIN_RING(chan, m2mf, 0x030c, 2);
		OUT_RELOCl(chan, src_bo, src_offset, src_reloc);
		OUT_RELOCl(chan, dst_bo, dst_offset, dst_reloc);
		if (src_bo->tiled) {
			BEGIN_RING(chan, m2mf, 0x0218, 1);
			OUT_RING  (chan, (dy << 16) | sx);
		} else {
			src_offset += (line_count * src_pitch);
		}
		if (dst_bo->tiled) {
			BEGIN_RING(chan, m2mf, 0x0234, 1);
			OUT_RING  (chan, (sy << 16) | dx);
		} else {
			dst_offset += (line_count * dst_pitch);
		}
		BEGIN_RING(chan, m2mf, 0x031c, 4);
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
	struct nv50_miptree *mt = nv50_miptree(pt);
	struct nv50_miptree_level *lvl = &mt->level[level];
	struct nv50_transfer *tx;
	unsigned image = 0;

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

	tx->level = lvl;
	tx->level_pitch = lvl->pitch;
	tx->level_width = mt->base.width[level];
	tx->level_height = mt->base.height[level];
	tx->level_x = x;
	tx->level_y = y;
	tx->buffer =
		pipe_buffer_create(pscreen, 0, NOUVEAU_BUFFER_USAGE_TRANSFER,
				   w * tx->base.block.size * h);

	if (usage != PIPE_TRANSFER_WRITE) {
		nv50_transfer_rect_m2mf(pscreen, mt->buffer, tx->level_pitch,
					x, y, tx->level_width, tx->level_height,
					tx->buffer, tx->base.stride, 0, 0,
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
		nv50_transfer_rect_m2mf(pscreen, tx->buffer, tx->base.stride,
					0, 0, tx->base.width, tx->base.height,
					mt->buffer, tx->level_pitch,
					tx->level_x, tx->level_y,
					tx->level_width, tx->level_height,
					tx->base.block.size, tx->base.width,
					tx->base.height, NOUVEAU_BO_GART,
					NOUVEAU_BO_VRAM | NOUVEAU_BO_GART);
	}

	pipe_buffer_reference(&tx->buffer, NULL);
	pipe_texture_reference(&ptx->texture, NULL);
	FREE(ptx);
}

static void *
nv50_transfer_map(struct pipe_screen *pscreen, struct pipe_transfer *ptx)
{
	struct nv50_transfer *tx = (struct nv50_transfer *)ptx;
	unsigned flags = 0;

	if (ptx->usage & PIPE_TRANSFER_WRITE)
		flags |= PIPE_BUFFER_USAGE_CPU_WRITE;
	if (ptx->usage & PIPE_TRANSFER_READ)
		flags |= PIPE_BUFFER_USAGE_CPU_READ;

	return pipe_buffer_map(pscreen, tx->buffer, flags);
}

static void
nv50_transfer_unmap(struct pipe_screen *pscreen, struct pipe_transfer *ptx)
{
	struct nv50_transfer *tx = (struct nv50_transfer *)ptx;

	pipe_buffer_unmap(pscreen, tx->buffer);
}

void
nv50_transfer_init_screen_functions(struct pipe_screen *pscreen)
{
	pscreen->get_tex_transfer = nv50_transfer_new;
	pscreen->tex_transfer_destroy = nv50_transfer_del;
	pscreen->transfer_map = nv50_transfer_map;
	pscreen->transfer_unmap = nv50_transfer_unmap;
}
