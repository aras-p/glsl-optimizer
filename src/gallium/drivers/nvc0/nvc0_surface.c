/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>

#include "pipe/p_defines.h"

#include "util/u_inlines.h"
#include "util/u_pack_color.h"
#include "util/u_format.h"
#include "util/u_surface.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"
#include "nvc0_transfer.h"

#include "nv50/nv50_defs.xml.h"

#define NVC0_ENG2D_SUPPORTED_FORMATS 0xff9ccfe1cce3ccc9ULL

/* return TRUE for formats that can be converted among each other by NVC0_2D */
static INLINE boolean
nvc0_2d_format_faithful(enum pipe_format format)
{
   uint8_t id = nvc0_format_table[format].rt;

   return (id >= 0xc0) && (NVC0_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0)));
}

static INLINE uint8_t
nvc0_2d_format(enum pipe_format format)
{
   uint8_t id = nvc0_format_table[format].rt;

   /* Hardware values for color formats range from 0xc0 to 0xff,
    * but the 2D engine doesn't support all of them.
    */
   if (nvc0_2d_format_faithful(format))
      return id;

   switch (util_format_get_blocksize(format)) {
   case 1:
      return NV50_SURFACE_FORMAT_R8_UNORM;
   case 2:
      return NV50_SURFACE_FORMAT_R16_UNORM;
   case 4:
      return NV50_SURFACE_FORMAT_A8R8G8B8_UNORM;
   case 8:
      return NV50_SURFACE_FORMAT_R16G16B16A16_UNORM;
   case 16:
      return NV50_SURFACE_FORMAT_R32G32B32A32_FLOAT;
   default:
      return 0;
   }
}

static int
nvc0_2d_texture_set(struct nouveau_channel *chan, int dst,
                    struct nvc0_miptree *mt, unsigned level, unsigned layer)
{
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NVC0_2D_DST_FORMAT : NVC0_2D_SRC_FORMAT;
   uint32_t flags = mt->base.domain | (dst ? NOUVEAU_BO_WR : NOUVEAU_BO_RD);
   uint32_t offset = mt->level[level].offset;

   format = nvc0_2d_format(mt->base.base.format);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                  util_format_name(mt->base.base.format));
      return 1;
   }

   width = u_minify(mt->base.base.width0, level);
   height = u_minify(mt->base.base.height0, level);
   depth = u_minify(mt->base.base.depth0, level);

   /* layer has to be < depth, and depth > tile depth / 2 */

   if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
      layer = 0;
      depth = 1;
   } else
   if (!dst) {
      offset += nvc0_miptree_zslice_offset(mt, level, layer);
      layer = 0;
   }

   if (!(bo->tile_flags & NOUVEAU_BO_TILE_LAYOUT_MASK)) {
      BEGIN_RING(chan, RING_2D_(mthd), 2);
      OUT_RING  (chan, format);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_2D_(mthd + 0x14), 5);
      OUT_RING  (chan, mt->level[level].pitch);
      OUT_RING  (chan, width);
      OUT_RING  (chan, height);
      OUT_RELOCh(chan, bo, offset, flags);
      OUT_RELOCl(chan, bo, offset, flags);
   } else {
      BEGIN_RING(chan, RING_2D_(mthd), 5);
      OUT_RING  (chan, format);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, mt->level[level].tile_mode);
      OUT_RING  (chan, depth);
      OUT_RING  (chan, layer);
      BEGIN_RING(chan, RING_2D_(mthd + 0x18), 4);
      OUT_RING  (chan, width);
      OUT_RING  (chan, height);
      OUT_RELOCh(chan, bo, offset, flags);
      OUT_RELOCl(chan, bo, offset, flags);
   }

#if 0
   if (dst) {
      BEGIN_RING(chan, RING_2D_(NVC0_2D_CLIP_X), 4);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, width);
      OUT_RING  (chan, height);
   }
#endif
   return 0;
}

static int
nvc0_2d_texture_do_copy(struct nouveau_channel *chan,
                        struct nvc0_miptree *dst, unsigned dst_level,
                        unsigned dx, unsigned dy, unsigned dz,
                        struct nvc0_miptree *src, unsigned src_level,
                        unsigned sx, unsigned sy, unsigned sz,
                        unsigned w, unsigned h)
{
   int ret;

   ret = MARK_RING(chan, 2 * 16 + 32, 4);
   if (ret)
      return ret;

   ret = nvc0_2d_texture_set(chan, 1, dst, dst_level, dz);
   if (ret)
      return ret;

   ret = nvc0_2d_texture_set(chan, 0, src, src_level, sz);
   if (ret)
      return ret;

   /* 0/1 = CENTER/CORNER, 10/00 = POINT/BILINEAR */
   BEGIN_RING(chan, RING_2D(BLIT_CONTROL), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_2D(BLIT_DST_X), 4);
   OUT_RING  (chan, dx);
   OUT_RING  (chan, dy);
   OUT_RING  (chan, w);
   OUT_RING  (chan, h);
   BEGIN_RING(chan, RING_2D(BLIT_DU_DX_FRACT), 4);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_2D(BLIT_SRC_X_FRACT), 4);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, sx);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, sy);

   return 0;
}

static void
nvc0_setup_m2mf_rect(struct nvc0_m2mf_rect *rect,
                     struct pipe_resource *restrict res, unsigned l,
                     unsigned x, unsigned y, unsigned z)
{
   struct nvc0_miptree *mt = nvc0_miptree(res);
   const unsigned w = u_minify(res->width0, l);
   const unsigned h = u_minify(res->height0, l);

   rect->bo = mt->base.bo;
   rect->domain = mt->base.domain;
   rect->base = mt->level[l].offset;
   rect->pitch = mt->level[l].pitch;
   if (util_format_is_plain(res->format)) {
      rect->width = w;
      rect->height = h;
      rect->x = x;
      rect->y = y;
   } else {
      rect->width = util_format_get_nblocksx(res->format, w);
      rect->height = util_format_get_nblocksy(res->format, h);
      rect->x = util_format_get_nblocksx(res->format, x);
      rect->y = util_format_get_nblocksy(res->format, y);
   }
   rect->tile_mode = mt->level[l].tile_mode;
   rect->cpp = util_format_get_blocksize(res->format);

   if (mt->layout_3d) {
      rect->z = z;
      rect->depth = u_minify(res->depth0, l);
   } else {
      rect->base += z * mt->layer_stride;
      rect->z = 0;
      rect->depth = 1;
   }
}

static void
nvc0_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct nvc0_screen *screen = nvc0_context(pipe)->screen;
   int ret;
   unsigned dst_layer = dstz, src_layer = src_box->z;

   /* Fallback for buffers. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   nv04_resource(dst)->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;

   if (src->format == dst->format) {
      struct nvc0_m2mf_rect drect, srect;
      unsigned i;
      unsigned nx = util_format_get_nblocksx(src->format, src_box->width);
      unsigned ny = util_format_get_nblocksy(src->format, src_box->height);

      nvc0_setup_m2mf_rect(&drect, dst, dst_level, dstx, dsty, dstz);
      nvc0_setup_m2mf_rect(&srect, src, src_level,
                           src_box->x, src_box->y, src_box->z);

      for (i = 0; i < src_box->depth; ++i) {
         nvc0_m2mf_transfer_rect(&screen->base.base, &drect, &srect, nx, ny);

         if (nvc0_miptree(dst)->layout_3d)
            drect.z++;
         else
            drect.base += nvc0_miptree(dst)->layer_stride;

         if (nvc0_miptree(src)->layout_3d)
            srect.z++;
         else
            srect.base += nvc0_miptree(src)->layer_stride;
      }
      return;
   }

   assert(nvc0_2d_format_faithful(src->format));
   assert(nvc0_2d_format_faithful(dst->format));

   for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nvc0_2d_texture_do_copy(screen->base.channel,
                                    nvc0_miptree(dst), dst_level,
                                    dstx, dsty, dst_layer,
                                    nvc0_miptree(src), src_level,
                                    src_box->x, src_box->y, src_layer,
                                    src_box->width, src_box->height);
      if (ret)
         return;
   }
}

static void
nvc0_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         const float *rgba,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
	struct nvc0_context *nv50 = nvc0_context(pipe);
	struct nvc0_screen *screen = nv50->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nvc0_miptree *mt = nvc0_miptree(dst->texture);
	struct nvc0_surface *sf = nvc0_surface(dst);
	struct nouveau_bo *bo = mt->base.bo;

	BEGIN_RING(chan, RING_3D(CLEAR_COLOR(0)), 4);
	OUT_RINGf (chan, rgba[0]);
	OUT_RINGf (chan, rgba[1]);
	OUT_RINGf (chan, rgba[2]);
	OUT_RINGf (chan, rgba[3]);

	if (MARK_RING(chan, 18, 2))
		return;

	BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, RING_3D(RT_ADDRESS_HIGH(0)), 9);
	OUT_RELOCh(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RING  (chan, sf->width);
	OUT_RING  (chan, sf->height);
	OUT_RING  (chan, nvc0_format_table[dst->format].rt);
	OUT_RING  (chan, (mt->layout_3d << 16) |
              mt->level[sf->base.u.tex.level].tile_mode);
	OUT_RING  (chan, dst->u.tex.first_layer + sf->depth);
	OUT_RING  (chan, mt->layer_stride >> 2);
	OUT_RING  (chan, dst->u.tex.first_layer);

	BEGIN_RING(chan, RING_3D(CLIP_RECT_HORIZ(0)), 2);
	OUT_RING  (chan, ((dstx + width) << 16) | dstx);
	OUT_RING  (chan, ((dsty + height) << 16) | dsty);
	IMMED_RING(chan, RING_3D(CLIP_RECTS_EN), 1);

	BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
	OUT_RING  (chan, 0x3c);

	IMMED_RING(chan, RING_3D(CLIP_RECTS_EN), 0);

	nv50->dirty |= NVC0_NEW_FRAMEBUFFER;
}

static void
nvc0_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
	struct nvc0_context *nv50 = nvc0_context(pipe);
	struct nvc0_screen *screen = nv50->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nvc0_miptree *mt = nvc0_miptree(dst->texture);
	struct nvc0_surface *sf = nvc0_surface(dst);
	struct nouveau_bo *bo = mt->base.bo;
	uint32_t mode = 0;
	int unk = mt->base.base.target == PIPE_TEXTURE_2D;

	if (clear_flags & PIPE_CLEAR_DEPTH) {
		BEGIN_RING(chan, RING_3D(CLEAR_DEPTH), 1);
		OUT_RINGf (chan, depth);
		mode |= NVC0_3D_CLEAR_BUFFERS_Z;
	}

	if (clear_flags & PIPE_CLEAR_STENCIL) {
		BEGIN_RING(chan, RING_3D(CLEAR_STENCIL), 1);
		OUT_RING  (chan, stencil & 0xff);
		mode |= NVC0_3D_CLEAR_BUFFERS_S;
	}

	if (MARK_RING(chan, 17, 2))
		return;

	BEGIN_RING(chan, RING_3D(ZETA_ADDRESS_HIGH), 5);
	OUT_RELOCh(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RING  (chan, nvc0_format_table[dst->format].rt);
	OUT_RING  (chan, mt->level[sf->base.u.tex.level].tile_mode);
	OUT_RING  (chan, mt->layer_stride >> 2);
	BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, RING_3D(ZETA_HORIZ), 3);
	OUT_RING  (chan, sf->width);
	OUT_RING  (chan, sf->height);
	OUT_RING  (chan, (unk << 16) | (dst->u.tex.first_layer + sf->depth));
	BEGIN_RING(chan, RING_3D(ZETA_BASE_LAYER), 1);
	OUT_RING  (chan, dst->u.tex.first_layer);

	BEGIN_RING(chan, RING_3D(CLIP_RECT_HORIZ(0)), 2);
	OUT_RING  (chan, ((dstx + width) << 16) | dstx);
	OUT_RING  (chan, ((dsty + height) << 16) | dsty);
	IMMED_RING(chan, RING_3D(CLIP_RECTS_EN), 1);

	BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
	OUT_RING  (chan, mode);

	IMMED_RING(chan, RING_3D(CLIP_RECTS_EN), 0);

	nv50->dirty |= NVC0_NEW_FRAMEBUFFER;
}

void
nvc0_clear(struct pipe_context *pipe, unsigned buffers,
           const float *rgba, double depth, unsigned stencil)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
   unsigned i;
   const unsigned dirty = nvc0->dirty;
   uint32_t mode = 0;

   /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   nvc0->dirty &= NVC0_NEW_FRAMEBUFFER;
   if (!nvc0_state_validate(nvc0))
      return;

   if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_RING(chan, RING_3D(CLEAR_COLOR(0)), 4);
      OUT_RINGf (chan, rgba[0]);
      OUT_RINGf (chan, rgba[1]);
      OUT_RINGf (chan, rgba[2]);
      OUT_RINGf (chan, rgba[3]);
      mode =
         NVC0_3D_CLEAR_BUFFERS_R | NVC0_3D_CLEAR_BUFFERS_G |
         NVC0_3D_CLEAR_BUFFERS_B | NVC0_3D_CLEAR_BUFFERS_A;
   }

   if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_RING(chan, RING_3D(CLEAR_DEPTH), 1);
      OUT_RING  (chan, fui(depth));
      mode |= NVC0_3D_CLEAR_BUFFERS_Z;
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_RING(chan, RING_3D(CLEAR_STENCIL), 1);
      OUT_RING  (chan, stencil & 0xff);
      mode |= NVC0_3D_CLEAR_BUFFERS_S;
   }

   BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
   OUT_RING  (chan, mode);

   for (i = 1; i < fb->nr_cbufs; i++) {
      BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
      OUT_RING  (chan, (i << 6) | 0x3c);
   }

   nvc0->dirty = dirty & ~NVC0_NEW_FRAMEBUFFER;
}

void
nvc0_init_surface_functions(struct nvc0_context *nvc0)
{
   struct pipe_context *pipe = &nvc0->base.pipe;

   pipe->resource_copy_region = nvc0_resource_copy_region;
   pipe->clear_render_target = nvc0_clear_render_target;
   pipe->clear_depth_stencil = nvc0_clear_depth_stencil;
}


