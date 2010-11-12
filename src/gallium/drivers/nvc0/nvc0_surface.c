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

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nv50_defs.xml.h"

/* return TRUE for formats that can be converted among each other by NVC0_2D */
static INLINE boolean
nvc0_2d_format_faithful(enum pipe_format format)
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
nvc0_2d_format(enum pipe_format format)
{
   uint8_t id = nvc0_format_table[format].rt;

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
nvc0_surface_set(struct nvc0_screen *screen, struct pipe_surface *ps, int dst)
{
   struct nvc0_miptree *mt = nvc0_miptree(ps->texture);
   struct nouveau_channel *chan = screen->base.channel;
   struct nouveau_bo *bo = nvc0_miptree(ps->texture)->base.bo;
   int format, mthd = dst ? NVC0_2D_DST_FORMAT : NVC0_2D_SRC_FORMAT;
   int flags = NOUVEAU_BO_VRAM | (dst ? NOUVEAU_BO_WR : NOUVEAU_BO_RD);

   format = nvc0_2d_format(ps->format);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                  util_format_name(ps->format));
      return 1;
   }

   if (!bo->tile_flags) {
      BEGIN_RING(chan, RING_2D_(mthd), 2);
      OUT_RING  (chan, format);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_2D_(mthd + 0x14), 5);
      OUT_RING  (chan, mt->level[ps->level].pitch);
      OUT_RING  (chan, ps->width);
      OUT_RING  (chan, ps->height);
      OUT_RELOCh(chan, bo, ps->offset, flags);
      OUT_RELOCl(chan, bo, ps->offset, flags);
   } else {
      BEGIN_RING(chan, RING_2D_(mthd), 5);
      OUT_RING  (chan, format);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, mt->level[ps->level].tile_mode);
      OUT_RING  (chan, 1);
      OUT_RING  (chan, 0);
      BEGIN_RING(chan, RING_2D_(mthd + 0x18), 4);
      OUT_RING  (chan, ps->width);
      OUT_RING  (chan, ps->height);
      OUT_RELOCh(chan, bo, ps->offset, flags);
      OUT_RELOCl(chan, bo, ps->offset, flags);
   }
 
#if 0
   if (dst) {
      BEGIN_RING(chan, RING_2D_(NVC0_2D_CLIP_X), 4);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, surf->width);
      OUT_RING  (chan, surf->height);
   }
#endif
   return 0;
}

static int
nvc0_surface_do_copy(struct nvc0_screen *screen,
                     struct pipe_surface *dst, int dx, int dy,
                     struct pipe_surface *src, int sx, int sy,
                     int w, int h)
{
   struct nouveau_channel *chan = screen->base.channel;
   int ret;

   ret = MARK_RING(chan, 2*16 + 32, 4);
   if (ret)
      return ret;

   ret = nvc0_surface_set(screen, dst, 1);
   if (ret)
      return ret;

   ret = nvc0_surface_set(screen, src, 0);
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
nvc0_surface_copy(struct pipe_context *pipe,
		  struct pipe_resource *dest, struct pipe_subresource subdst,
		  unsigned destx, unsigned desty, unsigned destz,
		  struct pipe_resource *src, struct pipe_subresource subsrc,
		  unsigned srcx, unsigned srcy, unsigned srcz,
		  unsigned width, unsigned height)
{
   struct nvc0_context *nv50 = nvc0_context(pipe);
   struct nvc0_screen *screen = nv50->screen;
   struct pipe_surface *ps_dst, *ps_src;

   assert((src->format == dest->format) ||
          (nvc0_2d_format_faithful(src->format) &&
           nvc0_2d_format_faithful(dest->format)));

   ps_src = nvc0_miptree_surface_new(pipe->screen, src, subsrc.face,
                                     subsrc.level, srcz, 0 /* bind flags */);
   ps_dst = nvc0_miptree_surface_new(pipe->screen, dest, subdst.face,
                                     subdst.level, destz, 0 /* bind flags */);

   nvc0_surface_do_copy(screen, ps_dst, destx, desty, ps_src, srcx,
                        srcy, width, height);

   nvc0_miptree_surface_del(ps_src);
   nvc0_miptree_surface_del(ps_dst);
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
	BEGIN_RING(chan, RING_3D(RT_ADDRESS_HIGH(0)), 8);
	OUT_RELOCh(chan, bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RING  (chan, dst->width);
	OUT_RING  (chan, dst->height);
	OUT_RING  (chan, nvc0_format_table[dst->format].rt);
	OUT_RING  (chan, mt->level[dst->level].tile_mode);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);

	/* NOTE: only works with D3D clear flag (5097/0x143c bit 4) */

	BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
	OUT_RING  (chan, (width << 16) | dstx);
	OUT_RING  (chan, (height << 16) | dsty);

	BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
	OUT_RING  (chan, 0x3c);

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
	struct nouveau_bo *bo = mt->base.bo;
	uint32_t mode = 0;

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
	OUT_RELOCh(chan, bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RING  (chan, nvc0_format_table[dst->format].rt);
	OUT_RING  (chan, mt->level[dst->level].tile_mode);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, RING_3D(ZETA_HORIZ), 3);
	OUT_RING  (chan, dst->width);
	OUT_RING  (chan, dst->height);
	OUT_RING  (chan, (1 << 16) | 1);

	BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
	OUT_RING  (chan, (width << 16) | dstx);
	OUT_RING  (chan, (height << 16) | dsty);

	BEGIN_RING(chan, RING_3D(CLEAR_BUFFERS), 1);
	OUT_RING  (chan, mode);

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
	nvc0->pipe.resource_copy_region = nvc0_surface_copy;
	nvc0->pipe.clear_render_target = nvc0_clear_render_target;
	nvc0->pipe.clear_depth_stencil = nvc0_clear_depth_stencil;
}


