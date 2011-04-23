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

#include "nv50_context.h"
#include "nv50_resource.h"

#include "nv50_defs.xml.h"

/* return TRUE for formats that can be converted among each other by NV50_2D */
static INLINE boolean
nv50_2d_format_faithful(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_SRGB:
   case PIPE_FORMAT_B8G8R8X8_SRGB:
   case PIPE_FORMAT_B5G6R5_UNORM:
   case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_R8_UNORM:
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
   case PIPE_FORMAT_R32G32B32_FLOAT:
      return TRUE;
   default:
      return FALSE;
   }
}

static INLINE uint8_t
nv50_2d_format(enum pipe_format format)
{
   uint8_t id = nv50_format_table[format].rt;

   /* Hardware values for color formats range from 0xc0 to 0xff,
    * but the 2D engine doesn't support all of them.
    */
   if ((id >= 0xc0) && (0xff0843e080608409ULL & (1ULL << (id - 0xc0))))
      return id;

   switch (util_format_get_blocksize(format)) {
   case 1:
      return NV50_SURFACE_FORMAT_R8_UNORM;
   case 2:
      return NV50_SURFACE_FORMAT_R16_UNORM;
   case 4:
      return NV50_SURFACE_FORMAT_A8R8G8B8_UNORM;
   default:
      return 0;
   }
}

static int
nv50_2d_texture_set(struct nouveau_channel *chan, int dst,
                    struct nv50_miptree *mt, unsigned level, unsigned layer)
{
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
   uint32_t flags = mt->base.domain | (dst ? NOUVEAU_BO_WR : NOUVEAU_BO_RD);
   uint32_t offset = mt->level[level].offset;

   format = nv50_2d_format(mt->base.base.format);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                  util_format_name(mt->base.base.format));
      return 1;
   }

   width = u_minify(mt->base.base.width0, level);
   height = u_minify(mt->base.base.height0, level);

   offset = mt->level[level].offset;
   if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
      depth = 1;
      layer = 0;
   } else {
      depth = u_minify(mt->base.base.depth0, level);
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
      OUT_RING  (chan, mt->level[level].tile_mode << 4);
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
      BEGIN_RING(chan, RING_2D_(NV50_2D_CLIP_X), 4);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, width);
      OUT_RING  (chan, height);
   }
#endif
   return 0;
}

static int
nv50_2d_texture_do_copy(struct nouveau_channel *chan,
                        struct nv50_miptree *dst, unsigned dst_level,
                        unsigned dx, unsigned dy, unsigned dz,
                        struct nv50_miptree *src, unsigned src_level,
                        unsigned sx, unsigned sy, unsigned sz,
                        unsigned w, unsigned h)
{
   int ret;

   ret = MARK_RING(chan, 2 * 16 + 32, 4);
   if (ret)
      return ret;

   ret = nv50_2d_texture_set(chan, 1, dst, dst_level, dz);
   if (ret)
      return ret;

   ret = nv50_2d_texture_set(chan, 0, src, src_level, sz);
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
nv50_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct nv50_screen *screen = nv50_context(pipe)->screen;
   int ret;
   unsigned dst_layer = dstz, src_layer = src_box->z;

   /* Fallback for buffers. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   assert((src->format == dst->format) ||
          (nv50_2d_format_faithful(src->format) &&
           nv50_2d_format_faithful(dst->format)));

   for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nv50_2d_texture_do_copy(screen->base.channel,
                                    nv50_miptree(dst), dst_level,
                                    dstx, dsty, dst_layer,
                                    nv50_miptree(src), src_level,
                                    src_box->x, src_box->y, src_layer,
                                    src_box->width, src_box->height);
      if (ret)
         return;
   }
}

static void
nv50_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         const float *rgba,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nv50_screen *screen = nv50->screen;
   struct nouveau_channel *chan = screen->base.channel;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
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
   BEGIN_RING(chan, RING_3D(RT_ADDRESS_HIGH(0)), 5);
   OUT_RELOCh(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
   OUT_RING  (chan, nv50_format_table[dst->format].rt);
   OUT_RING  (chan, mt->level[sf->base.u.tex.level].tile_mode << 4);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(RT_HORIZ(0)), 2);
   OUT_RING  (chan, sf->width);
   OUT_RING  (chan, sf->height);
   BEGIN_RING(chan, RING_3D(RT_ARRAY_MODE), 1);
   OUT_RING  (chan, 1);

   /* NOTE: only works with D3D clear flag (5097/0x143c bit 4) */

   BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
   OUT_RING  (chan, (width << 16) | dstx);
   OUT_RING  (chan, (height << 16) | dsty);

   BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
   OUT_RING  (chan, 0x3c);

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

static void
nv50_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nv50_screen *screen = nv50->screen;
   struct nouveau_channel *chan = screen->base.channel;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t mode = 0;

   if (clear_flags & PIPE_CLEAR_DEPTH) {
      BEGIN_RING(chan, RING_3D(CLEAR_DEPTH), 1);
      OUT_RINGf (chan, depth);
      mode |= NV50_3D_CLEAR_BUFFERS_Z;
   }

   if (clear_flags & PIPE_CLEAR_STENCIL) {
      BEGIN_RING(chan, RING_3D(CLEAR_STENCIL), 1);
      OUT_RING  (chan, stencil & 0xff);
      mode |= NV50_3D_CLEAR_BUFFERS_S;
   }

   if (MARK_RING(chan, 17, 2))
      return;

   BEGIN_RING(chan, RING_3D(ZETA_ADDRESS_HIGH), 5);
   OUT_RELOCh(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, bo, sf->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
   OUT_RING  (chan, nv50_format_table[dst->format].rt);
   OUT_RING  (chan, mt->level[sf->base.u.tex.level].tile_mode << 4);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(ZETA_HORIZ), 3);
   OUT_RING  (chan, sf->width);
   OUT_RING  (chan, sf->height);
   OUT_RING  (chan, (1 << 16) | 1);

   BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
   OUT_RING  (chan, (width << 16) | dstx);
   OUT_RING  (chan, (height << 16) | dsty);

   BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
   OUT_RING  (chan, mode);

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

void
nv50_clear(struct pipe_context *pipe, unsigned buffers,
           const float *rgba, double depth, unsigned stencil)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct pipe_framebuffer_state *fb = &nv50->framebuffer;
   unsigned i;
   const unsigned dirty = nv50->dirty;
   uint32_t mode = 0;

   /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   nv50->dirty &= NV50_NEW_FRAMEBUFFER;
   if (!nv50_state_validate(nv50))
      return;

   if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_RING(chan, RING_3D(CLEAR_COLOR(0)), 4);
      OUT_RINGf (chan, rgba[0]);
      OUT_RINGf (chan, rgba[1]);
      OUT_RINGf (chan, rgba[2]);
      OUT_RINGf (chan, rgba[3]);
      mode =
         NV50_3D_CLEAR_BUFFERS_R | NV50_3D_CLEAR_BUFFERS_G |
         NV50_3D_CLEAR_BUFFERS_B | NV50_3D_CLEAR_BUFFERS_A;
   }

   if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_RING(chan, RING_3D(CLEAR_DEPTH), 1);
      OUT_RING  (chan, fui(depth));
      mode |= NV50_3D_CLEAR_BUFFERS_Z;
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_RING(chan, RING_3D(CLEAR_STENCIL), 1);
      OUT_RING  (chan, stencil & 0xff);
      mode |= NV50_3D_CLEAR_BUFFERS_S;
   }

   BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
   OUT_RING  (chan, mode);

   for (i = 1; i < fb->nr_cbufs; i++) {
      BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
      OUT_RING  (chan, (i << 6) | 0x3c);
   }

   nv50->dirty = dirty & ~NV50_NEW_FRAMEBUFFER;
}

void
nv50_init_surface_functions(struct nv50_context *nv50)
{
   struct pipe_context *pipe = &nv50->base.pipe;

   pipe->resource_copy_region = nv50_resource_copy_region;
   pipe->clear_render_target = nv50_clear_render_target;
   pipe->clear_depth_stencil = nv50_clear_depth_stencil;
}


