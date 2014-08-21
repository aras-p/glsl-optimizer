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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nv50/nv50_context.h"
#include "nv50/nv50_resource.h"
#include "nv50/nv50_texture.xml.h"
#include "nv50/nv50_defs.xml.h"

#include "util/u_format.h"

#define NV50_TIC_0_SWIZZLE__MASK                      \
   (NV50_TIC_0_MAPA__MASK | NV50_TIC_0_MAPB__MASK |   \
    NV50_TIC_0_MAPG__MASK | NV50_TIC_0_MAPR__MASK)

static INLINE uint32_t
nv50_tic_swizzle(uint32_t tc, unsigned swz, boolean tex_int)
{
   switch (swz) {
   case PIPE_SWIZZLE_RED:
      return (tc & NV50_TIC_0_MAPR__MASK) >> NV50_TIC_0_MAPR__SHIFT;
   case PIPE_SWIZZLE_GREEN:
      return (tc & NV50_TIC_0_MAPG__MASK) >> NV50_TIC_0_MAPG__SHIFT;
   case PIPE_SWIZZLE_BLUE:
      return (tc & NV50_TIC_0_MAPB__MASK) >> NV50_TIC_0_MAPB__SHIFT;
   case PIPE_SWIZZLE_ALPHA:
      return (tc & NV50_TIC_0_MAPA__MASK) >> NV50_TIC_0_MAPA__SHIFT;
   case PIPE_SWIZZLE_ONE:
      return tex_int ? NV50_TIC_MAP_ONE_INT : NV50_TIC_MAP_ONE_FLOAT;
   case PIPE_SWIZZLE_ZERO:
   default:
      return NV50_TIC_MAP_ZERO;
   }
}

struct pipe_sampler_view *
nv50_create_sampler_view(struct pipe_context *pipe,
                         struct pipe_resource *res,
                         const struct pipe_sampler_view *templ)
{
   uint32_t flags = 0;

   if (templ->target == PIPE_TEXTURE_RECT || templ->target == PIPE_BUFFER)
      flags |= NV50_TEXVIEW_SCALED_COORDS;

   return nv50_create_texture_view(pipe, res, templ, flags, templ->target);
}

struct pipe_sampler_view *
nv50_create_texture_view(struct pipe_context *pipe,
                         struct pipe_resource *texture,
                         const struct pipe_sampler_view *templ,
                         uint32_t flags,
                         enum pipe_texture_target target)
{
   const struct util_format_description *desc;
   uint64_t addr;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t depth;
   struct nv50_tic_entry *view;
   struct nv50_miptree *mt = nv50_miptree(texture);
   boolean tex_int;

   view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
      return NULL;

   view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
   view->pipe.context = pipe;

   view->id = -1;

   pipe_resource_reference(&view->pipe.texture, texture);

   tic = &view->tic[0];

   desc = util_format_description(view->pipe.format);

   /* TIC[0] */

   tic[0] = nv50_format_table[view->pipe.format].tic;

   tex_int = util_format_is_pure_integer(view->pipe.format);

   swz[0] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_r, tex_int);
   swz[1] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_g, tex_int);
   swz[2] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_b, tex_int);
   swz[3] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_a, tex_int);
   tic[0] = (tic[0] & ~NV50_TIC_0_SWIZZLE__MASK) |
      (swz[0] << NV50_TIC_0_MAPR__SHIFT) |
      (swz[1] << NV50_TIC_0_MAPG__SHIFT) |
      (swz[2] << NV50_TIC_0_MAPB__SHIFT) |
      (swz[3] << NV50_TIC_0_MAPA__SHIFT);

   addr = mt->base.address;

   depth = MAX2(mt->base.base.array_size, mt->base.base.depth0);

   if (mt->base.base.array_size > 1) {
      /* there doesn't seem to be a base layer field in TIC */
      addr += view->pipe.u.tex.first_layer * mt->layer_stride;
      depth = view->pipe.u.tex.last_layer - view->pipe.u.tex.first_layer + 1;
   }

   tic[2] = 0x10001000 | NV50_TIC_2_NO_BORDER;

   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
      tic[2] |= NV50_TIC_2_COLORSPACE_SRGB;

   if (!(flags & NV50_TEXVIEW_SCALED_COORDS))
      tic[2] |= NV50_TIC_2_NORMALIZED_COORDS;

   if (unlikely(!nouveau_bo_memtype(nv04_resource(texture)->bo))) {
      if (target == PIPE_BUFFER) {
         addr += view->pipe.u.buf.first_element * desc->block.bits / 8;
         tic[2] |= NV50_TIC_2_LINEAR | NV50_TIC_2_TARGET_BUFFER;
         tic[3] = 0;
         tic[4] = /* width */
            view->pipe.u.buf.last_element - view->pipe.u.buf.first_element + 1;
         tic[5] = 0;
      } else {
         tic[2] |= NV50_TIC_2_LINEAR | NV50_TIC_2_TARGET_RECT;
         tic[3] = mt->level[0].pitch;
         tic[4] = mt->base.base.width0;
         tic[5] = (1 << 16) | (mt->base.base.height0);
      }
      tic[6] =
      tic[7] = 0;
      tic[1] = addr;
      tic[2] |= addr >> 32;
      return &view->pipe;
   }

   tic[1] = addr;
   tic[2] |= (addr >> 32) & 0xff;

   tic[2] |=
      ((mt->level[0].tile_mode & 0x0f0) << (22 - 4)) |
      ((mt->level[0].tile_mode & 0xf00) << (25 - 8));

   switch (target) {
   case PIPE_TEXTURE_1D:
      tic[2] |= NV50_TIC_2_TARGET_1D;
      break;
   case PIPE_TEXTURE_2D:
      tic[2] |= NV50_TIC_2_TARGET_2D;
      break;
   case PIPE_TEXTURE_RECT:
      tic[2] |= NV50_TIC_2_TARGET_RECT;
      break;
   case PIPE_TEXTURE_3D:
      tic[2] |= NV50_TIC_2_TARGET_3D;
      break;
   case PIPE_TEXTURE_CUBE:
      depth /= 6;
      tic[2] |= NV50_TIC_2_TARGET_CUBE;
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      tic[2] |= NV50_TIC_2_TARGET_1D_ARRAY;
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      tic[2] |= NV50_TIC_2_TARGET_2D_ARRAY;
      break;
   case PIPE_TEXTURE_CUBE_ARRAY:
      depth /= 6;
      tic[2] |= NV50_TIC_2_TARGET_CUBE_ARRAY;
      break;
   case PIPE_BUFFER:
      assert(0); /* should be linear and handled above ! */
      tic[2] |= NV50_TIC_2_TARGET_BUFFER | NV50_TIC_2_LINEAR;
      break;
   default:
      NOUVEAU_ERR("invalid texture target: %d\n", mt->base.base.target);
      return FALSE;
   }

   tic[3] = (flags & NV50_TEXVIEW_FILTER_MSAA8) ? 0x20000000 : 0x00300000;

   tic[4] = (1 << 31) | (mt->base.base.width0 << mt->ms_x);

   tic[5] = (mt->base.base.height0 << mt->ms_y) & 0xffff;
   tic[5] |= depth << 16;
   tic[5] |= mt->base.base.last_level << NV50_TIC_5_LAST_LEVEL__SHIFT;

   tic[6] = (mt->ms_x > 1) ? 0x88000000 : 0x03000000; /* sampling points */

   tic[7] = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;

   if (unlikely(!(tic[2] & NV50_TIC_2_NORMALIZED_COORDS)))
      if (mt->base.base.last_level)
         tic[5] &= ~NV50_TIC_5_LAST_LEVEL__MASK;

   return &view->pipe;
}

static boolean
nv50_validate_tic(struct nv50_context *nv50, int s)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nouveau_bo *txc = nv50->screen->txc;
   unsigned i;
   boolean need_flush = FALSE;

   assert(nv50->num_textures[s] <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nv50->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nv50->textures[s][i]);
      struct nv04_resource *res;

      if (!tic) {
         BEGIN_NV04(push, NV50_3D(BIND_TIC(s)), 1);
         PUSH_DATA (push, (i << 1) | 0);
         continue;
      }
      res = &nv50_miptree(tic->pipe.texture)->base;

      if (tic->id < 0) {
         tic->id = nv50_screen_tic_alloc(nv50->screen, tic);

         BEGIN_NV04(push, NV50_2D(DST_FORMAT), 2);
         PUSH_DATA (push, NV50_SURFACE_FORMAT_R8_UNORM);
         PUSH_DATA (push, 1);
         BEGIN_NV04(push, NV50_2D(DST_PITCH), 5);
         PUSH_DATA (push, 262144);
         PUSH_DATA (push, 65536);
         PUSH_DATA (push, 1);
         PUSH_DATAh(push, txc->offset);
         PUSH_DATA (push, txc->offset);
         BEGIN_NV04(push, NV50_2D(SIFC_BITMAP_ENABLE), 2);
         PUSH_DATA (push, 0);
         PUSH_DATA (push, NV50_SURFACE_FORMAT_R8_UNORM);
         BEGIN_NV04(push, NV50_2D(SIFC_WIDTH), 10);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         PUSH_DATA (push, 0);
         PUSH_DATA (push, 1);
         PUSH_DATA (push, 0);
         PUSH_DATA (push, 1);
         PUSH_DATA (push, 0);
         PUSH_DATA (push, tic->id * 32);
         PUSH_DATA (push, 0);
         PUSH_DATA (push, 0);
         BEGIN_NI04(push, NV50_2D(SIFC_DATA), 8);
         PUSH_DATAp(push, &tic->tic[0], 8);

         need_flush = TRUE;
      } else
      if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         BEGIN_NV04(push, NV50_3D(TEX_CACHE_CTL), 1);
         PUSH_DATA (push, 0x20);
      }

      nv50->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);

      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      res->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;

      BCTX_REFN(nv50->bufctx_3d, TEXTURES, res, RD);

      BEGIN_NV04(push, NV50_3D(BIND_TIC(s)), 1);
      PUSH_DATA (push, (tic->id << 9) | (i << 1) | 1);
   }
   for (; i < nv50->state.num_textures[s]; ++i) {
      BEGIN_NV04(push, NV50_3D(BIND_TIC(s)), 1);
      PUSH_DATA (push, (i << 1) | 0);
   }
   if (nv50->num_textures[s]) {
      BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
      PUSH_DATA (push, ((NV50_CB_AUX_TEX_MS_OFFSET + 16 * s * 2 * 4) << (8 - 2)) | NV50_CB_AUX);
      BEGIN_NI04(push, NV50_3D(CB_DATA(0)), nv50->num_textures[s] * 2);
      for (i = 0; i < nv50->num_textures[s]; i++) {
         struct nv50_tic_entry *tic = nv50_tic_entry(nv50->textures[s][i]);
         struct nv50_miptree *res;

         if (!tic) {
            PUSH_DATA (push, 0);
            PUSH_DATA (push, 0);
            continue;
         }
         res = nv50_miptree(tic->pipe.texture);
         PUSH_DATA (push, res->ms_x);
         PUSH_DATA (push, res->ms_y);
      }
   }
   nv50->state.num_textures[s] = nv50->num_textures[s];

   return need_flush;
}

void nv50_validate_textures(struct nv50_context *nv50)
{
   boolean need_flush;

   need_flush  = nv50_validate_tic(nv50, 0);
   need_flush |= nv50_validate_tic(nv50, 1);
   need_flush |= nv50_validate_tic(nv50, 2);

   if (need_flush) {
      BEGIN_NV04(nv50->base.pushbuf, NV50_3D(TIC_FLUSH), 1);
      PUSH_DATA (nv50->base.pushbuf, 0);
   }
}

static boolean
nv50_validate_tsc(struct nv50_context *nv50, int s)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   unsigned i;
   boolean need_flush = FALSE;

   assert(nv50->num_samplers[s] <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nv50->num_samplers[s]; ++i) {
      struct nv50_tsc_entry *tsc = nv50_tsc_entry(nv50->samplers[s][i]);

      if (!tsc) {
         BEGIN_NV04(push, NV50_3D(BIND_TSC(s)), 1);
         PUSH_DATA (push, (i << 4) | 0);
         continue;
      }
      if (tsc->id < 0) {
         tsc->id = nv50_screen_tsc_alloc(nv50->screen, tsc);

         nv50_sifc_linear_u8(&nv50->base, nv50->screen->txc,
                             65536 + tsc->id * 32,
                             NOUVEAU_BO_VRAM, 32, tsc->tsc);
         need_flush = TRUE;
      }
      nv50->screen->tsc.lock[tsc->id / 32] |= 1 << (tsc->id % 32);

      BEGIN_NV04(push, NV50_3D(BIND_TSC(s)), 1);
      PUSH_DATA (push, (tsc->id << 12) | (i << 4) | 1);
   }
   for (; i < nv50->state.num_samplers[s]; ++i) {
      BEGIN_NV04(push, NV50_3D(BIND_TSC(s)), 1);
      PUSH_DATA (push, (i << 4) | 0);
   }
   nv50->state.num_samplers[s] = nv50->num_samplers[s];

   return need_flush;
}

void nv50_validate_samplers(struct nv50_context *nv50)
{
   boolean need_flush;

   need_flush  = nv50_validate_tsc(nv50, 0);
   need_flush |= nv50_validate_tsc(nv50, 1);
   need_flush |= nv50_validate_tsc(nv50, 2);

   if (need_flush) {
      BEGIN_NV04(nv50->base.pushbuf, NV50_3D(TSC_FLUSH), 1);
      PUSH_DATA (nv50->base.pushbuf, 0);
   }
}

/* There can be up to 4 different MS levels (1, 2, 4, 8). To simplify the
 * shader logic, allow each one to take up 8 offsets.
 */
#define COMBINE(x, y) x, y
#define DUMMY 0, 0
static const uint32_t msaa_sample_xy_offsets[] = {
   /* MS1 */
   COMBINE(0, 0),
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,

   /* MS2 */
   COMBINE(0, 0),
   COMBINE(1, 0),
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,

   /* MS4 */
   COMBINE(0, 0),
   COMBINE(1, 0),
   COMBINE(0, 1),
   COMBINE(1, 1),
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,

   /* MS8 */
   COMBINE(0, 0),
   COMBINE(1, 0),
   COMBINE(0, 1),
   COMBINE(1, 1),
   COMBINE(2, 0),
   COMBINE(3, 0),
   COMBINE(2, 1),
   COMBINE(3, 1),
};

void nv50_upload_ms_info(struct nouveau_pushbuf *push)
{
   BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
   PUSH_DATA (push, (NV50_CB_AUX_MS_OFFSET << (8 - 2)) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_3D(CB_DATA(0)), Elements(msaa_sample_xy_offsets));
   PUSH_DATAp(push, msaa_sample_xy_offsets, Elements(msaa_sample_xy_offsets));
}
