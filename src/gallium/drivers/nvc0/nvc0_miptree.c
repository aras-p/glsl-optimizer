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

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"
#include "nvc0_transfer.h"

static INLINE uint32_t
get_tile_dims(unsigned nx, unsigned ny, unsigned nz)
{
   uint32_t tile_mode = 0x000;

   if (ny > 64) tile_mode = 0x040; /* height 128 tiles */
   else
   if (ny > 32) tile_mode = 0x030; /* height 64 tiles */
   else
   if (ny > 16) tile_mode = 0x020; /* height 32 tiles */
   else
   if (ny >  8) tile_mode = 0x010; /* height 16 tiles */

   if (nz == 1)
      return tile_mode;
   else
   if (tile_mode > 0x020)
      tile_mode = 0x020;

   if (nz > 16 && tile_mode < 0x020)
      return tile_mode | 0x500; /* depth 32 tiles */
   if (nz > 8) return tile_mode | 0x400; /* depth 16 tiles */
   if (nz > 4) return tile_mode | 0x300; /* depth 8 tiles */
   if (nz > 2) return tile_mode | 0x200; /* depth 4 tiles */

   return tile_mode | 0x100;
}

static INLINE unsigned
calc_zslice_offset(uint32_t tile_mode, unsigned z, unsigned pitch, unsigned nbh)
{
   unsigned tile_h = NVC0_TILE_HEIGHT(tile_mode);
   unsigned tile_d_shift = NVC0_TILE_DIM_SHIFT(tile_mode, 2);
   unsigned tile_d = 1 << tile_d_shift;

   /* stride_2d == to next slice within this volume tile */
   /* stride_3d == size (in bytes) of a volume tile */
   unsigned stride_2d = tile_h * NVC0_TILE_PITCH(tile_mode);
   unsigned stride_3d = tile_d * align(nbh, tile_h) * pitch;

   return (z & (tile_d - 1)) * stride_2d + (z >> tile_d_shift) * stride_3d;
}

static void
nvc0_miptree_destroy(struct pipe_screen *pscreen, struct pipe_resource *pt)
{
   struct nvc0_miptree *mt = nvc0_miptree(pt);

   nouveau_screen_bo_release(pscreen, mt->base.bo);

   FREE(mt);
}

static boolean
nvc0_miptree_get_handle(struct pipe_screen *pscreen,
                        struct pipe_resource *pt,
                        struct winsys_handle *whandle)
{
   struct nvc0_miptree *mt = nvc0_miptree(pt);
   unsigned stride;

   if (!mt || !mt->base.bo)
      return FALSE;

   stride = util_format_get_stride(mt->base.base.format,
                                   mt->base.base.width0);

   return nouveau_screen_bo_get_handle(pscreen,
                                       mt->base.bo,
                                       stride,
                                       whandle);
}

const struct u_resource_vtbl nvc0_miptree_vtbl =
{
   nvc0_miptree_get_handle,         /* get_handle */
   nvc0_miptree_destroy,            /* resource_destroy */
   NULL,                            /* is_resource_referenced */
   nvc0_miptree_transfer_new,       /* get_transfer */
   nvc0_miptree_transfer_del,       /* transfer_destroy */
   nvc0_miptree_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region, /* transfer_flush_region */
   nvc0_miptree_transfer_unmap,     /* transfer_unmap */
   u_default_transfer_inline_write  /* transfer_inline_write */
};

struct pipe_resource *
nvc0_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *templ)
{
   struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nvc0_miptree *mt = CALLOC_STRUCT(nvc0_miptree);
   struct pipe_resource *pt = &mt->base.base;
   int ret;
   unsigned w, h, d, l, alloc_size;
   uint32_t tile_flags;

   if (!mt)
      return NULL;

   mt->base.vtbl = &nvc0_miptree_vtbl;
   *pt = *templ;
   pipe_reference_init(&pt->reference, 1);
   pt->screen = pscreen;

   mt->layout_3d = pt->target == PIPE_TEXTURE_3D;

   w = pt->width0;
   h = pt->height0;
   d = mt->layout_3d ? pt->depth0 : 1;

   switch (pt->format) {
   case PIPE_FORMAT_Z16_UNORM:
      tile_flags = 0x0700; /* COMPRESSED */
      tile_flags = 0x0200; /* NORMAL ? */
      tile_flags = 0x0100; /* NORMAL ? */
      break;
   case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
      tile_flags = 0x5300; /* MSAA 4, COMPRESSED */
      tile_flags = 0x4600; /* NORMAL */
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
      tile_flags = 0x1100; /* NORMAL */
      if (w * h >= 128 * 128 && 0)
         tile_flags = 0x1700; /* COMPRESSED, requires magic */
      break;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      tile_flags = 0xf500; /* COMPRESSED */
      tile_flags = 0xf700; /* MSAA 2 */
      tile_flags = 0xf900; /* MSAA 4 */
      tile_flags = 0xfe00; /* NORMAL */
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_USCALED:
      tile_flags = 0xce00; /* COMPRESSED */
      tile_flags = 0xcf00; /* MSAA 2, COMPRESSED */
      tile_flags = 0xd000; /* MSAA 4, COMPRESSED */
      tile_flags = 0xc300; /* NORMAL */
      break;
   case PIPE_FORMAT_R16G16B16A16_UNORM:
      tile_flags = 0xe900; /* COMPRESSED */
      break;
   default:
      tile_flags = 0xe000; /* MSAA 4, COMPRESSED 32 BIT */
      tile_flags = 0xfe00; /* NORMAL 32 BIT */
      if (w * h >= 128 * 128 && 0)
         tile_flags = 0xdb00; /* COMPRESSED 32 BIT, requires magic */
      break;
   }

   /* For 3D textures, a mipmap is spanned by all the layers, for array
    * textures and cube maps, each layer contains its own mipmaps.
    */
   for (l = 0; l <= pt->last_level; ++l) {
      struct nvc0_miptree_level *lvl = &mt->level[l];
      unsigned nbx = util_format_get_nblocksx(pt->format, w);
      unsigned nby = util_format_get_nblocksy(pt->format, h);
      unsigned blocksize = util_format_get_blocksize(pt->format);

      lvl->offset = mt->total_size;
      lvl->tile_mode = get_tile_dims(nbx, nby, d);
      lvl->pitch = align(nbx * blocksize, NVC0_TILE_PITCH(lvl->tile_mode));

      mt->total_size += lvl->pitch *
         align(nby, NVC0_TILE_HEIGHT(lvl->tile_mode)) *
         align(d, NVC0_TILE_DEPTH(lvl->tile_mode));

      w = u_minify(w, 1);
      h = u_minify(h, 1);
      d = u_minify(d, 1);
   }

   if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size,
                               NVC0_TILE_SIZE(mt->level[0].tile_mode));
      mt->total_size = mt->layer_stride * pt->array_size;
   }

   alloc_size = mt->total_size;
   if (tile_flags == 0x1700)
      alloc_size *= 3; /* HiZ, XXX: correct size */

   ret = nouveau_bo_new_tile(dev, NOUVEAU_BO_VRAM, 256, alloc_size,
                             mt->level[0].tile_mode, tile_flags,
                             &mt->base.bo);
   if (ret) {
      FREE(mt);
      return NULL;
   }
   mt->base.domain = NOUVEAU_BO_VRAM;

   return pt;
}

struct pipe_resource *
nvc0_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *templ,
                         struct winsys_handle *whandle)
{
   struct nvc0_miptree *mt;
   unsigned stride;

   /* only supports 2D, non-mipmapped textures for the moment */
   if ((templ->target != PIPE_TEXTURE_2D &&
        templ->target != PIPE_TEXTURE_RECT) ||
       templ->last_level != 0 ||
       templ->depth0 != 1 ||
       templ->array_size > 1)
      return NULL;

   mt = CALLOC_STRUCT(nvc0_miptree);
   if (!mt)
      return NULL;

   mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
   if (mt->base.bo == NULL) {
      FREE(mt);
      return NULL;
   }

   mt->base.base = *templ;
   mt->base.vtbl = &nvc0_miptree_vtbl;
   pipe_reference_init(&mt->base.base.reference, 1);
   mt->base.base.screen = pscreen;
   mt->level[0].pitch = stride;
   mt->level[0].offset = 0;
   mt->level[0].tile_mode = mt->base.bo->tile_mode;

   /* no need to adjust bo reference count */
   return &mt->base.base;
}


/* Surface functions.
 */

struct pipe_surface *
nvc0_miptree_surface_new(struct pipe_context *pipe,
                         struct pipe_resource *pt,
                         const struct pipe_surface *templ)
{
   struct nvc0_miptree *mt = nvc0_miptree(pt); /* guaranteed */
   struct nvc0_surface *ns;
   struct pipe_surface *ps;
   struct nvc0_miptree_level *lvl = &mt->level[templ->u.tex.level];

   ns = CALLOC_STRUCT(nvc0_surface);
   if (!ns)
      return NULL;
   ps = &ns->base;

   pipe_reference_init(&ps->reference, 1);
   pipe_resource_reference(&ps->texture, pt);
   ps->context = pipe;
   ps->format = pt->format;
   ps->usage = templ->usage;
   ps->u.tex.level = templ->u.tex.level;
   ps->u.tex.first_layer = templ->u.tex.first_layer;
   ps->u.tex.last_layer = templ->u.tex.last_layer;

   ns->width = u_minify(pt->width0, ps->u.tex.level);
   ns->height = u_minify(pt->height0, ps->u.tex.level);
   ns->depth = ps->u.tex.last_layer - ps->u.tex.first_layer + 1;
   ns->offset = lvl->offset;

   /* comment says there are going to be removed, but they're used by the st */
   ps->width = ns->width;
   ps->height = ns->height;

   if (mt->layout_3d) {
      unsigned zslice = ps->u.tex.first_layer;

      /* TODO: re-layout the texture to use only depth 1 tiles in this case: */
      if (ns->depth > 1 && (zslice & (NVC0_TILE_DEPTH(lvl->tile_mode) - 1)))
         NOUVEAU_ERR("Creating unsupported 3D surface of slices [%u:%u].\n",
                     zslice, ps->u.tex.last_layer);

      ns->offset += calc_zslice_offset(lvl->tile_mode, zslice, lvl->pitch,
                                       util_format_get_nblocksy(pt->format,
                                                                ns->height));
   } else {
      ns->offset += mt->layer_stride * ps->u.tex.first_layer;
   }

   return ps;
}

void
nvc0_miptree_surface_del(struct pipe_context *pipe, struct pipe_surface *ps)
{
   struct nvc0_surface *s = nvc0_surface(ps);

   pipe_resource_reference(&ps->texture, NULL);

   FREE(s);
}
