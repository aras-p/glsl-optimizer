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

#include "nvc0/nvc0_context.h"
#include "nvc0/nvc0_resource.h"
#include "nv50/nv50_texture.xml.h"
#include "nv50/nv50_defs.xml.h"

#include "util/u_format.h"

#define NVE4_TIC_ENTRY_INVALID 0x000fffff
#define NVE4_TSC_ENTRY_INVALID 0xfff00000

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
nvc0_create_sampler_view(struct pipe_context *pipe,
                         struct pipe_resource *res,
                         const struct pipe_sampler_view *templ)
{
   uint32_t flags = 0;

   if (res->target == PIPE_TEXTURE_RECT || res->target == PIPE_BUFFER)
      flags |= NV50_TEXVIEW_SCALED_COORDS;

   return nvc0_create_texture_view(pipe, res, templ, flags, res->target);
}

struct pipe_sampler_view *
nvc0_create_texture_view(struct pipe_context *pipe,
                         struct pipe_resource *texture,
                         const struct pipe_sampler_view *templ,
                         uint32_t flags,
                         enum pipe_texture_target target)
{
   const struct util_format_description *desc;
   uint64_t address;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t width, height;
   uint32_t depth;
   struct nv50_tic_entry *view;
   struct nv50_miptree *mt;
   boolean tex_int;

   view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
      return NULL;
   mt = nv50_miptree(texture);

   view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
   view->pipe.context = pipe;

   view->id = -1;

   pipe_resource_reference(&view->pipe.texture, texture);

   tic = &view->tic[0];

   desc = util_format_description(view->pipe.format);

   tic[0] = nvc0_format_table[view->pipe.format].tic;

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

   address = mt->base.address;

   tic[2] = 0x10001000 | NV50_TIC_2_NO_BORDER;

   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
      tic[2] |= NV50_TIC_2_COLORSPACE_SRGB;

   if (!(flags & NV50_TEXVIEW_SCALED_COORDS))
      tic[2] |= NV50_TIC_2_NORMALIZED_COORDS;

   /* check for linear storage type */
   if (unlikely(!nouveau_bo_memtype(nv04_resource(texture)->bo))) {
      if (texture->target == PIPE_BUFFER) {
         assert(!(tic[2] & NV50_TIC_2_NORMALIZED_COORDS));
         address +=
            view->pipe.u.buf.first_element * desc->block.bits / 8;
         tic[2] |= NV50_TIC_2_LINEAR | NV50_TIC_2_TARGET_BUFFER;
         tic[3] = 0;
         tic[4] = /* width */
            view->pipe.u.buf.last_element - view->pipe.u.buf.first_element + 1;
         tic[5] = 0;
      } else {
         /* must be 2D texture without mip maps */
         tic[2] |= NV50_TIC_2_LINEAR | NV50_TIC_2_TARGET_RECT;
         tic[3] = mt->level[0].pitch;
         tic[4] = mt->base.base.width0;
         tic[5] = (1 << 16) | mt->base.base.height0;
      }
      tic[6] =
      tic[7] = 0;
      tic[1] = address;
      tic[2] |= address >> 32;
      return &view->pipe;
   }

   tic[2] |=
      ((mt->level[0].tile_mode & 0x0f0) << (22 - 4)) |
      ((mt->level[0].tile_mode & 0xf00) << (25 - 8));

   depth = MAX2(mt->base.base.array_size, mt->base.base.depth0);

   if (mt->base.base.array_size > 1) {
      /* there doesn't seem to be a base layer field in TIC */
      address += view->pipe.u.tex.first_layer * mt->layer_stride;
      depth = view->pipe.u.tex.last_layer - view->pipe.u.tex.first_layer + 1;
   }
   tic[1] = address;
   tic[2] |= address >> 32;

   switch (target) {
   case PIPE_TEXTURE_1D:
      tic[2] |= NV50_TIC_2_TARGET_1D;
      break;
   case PIPE_TEXTURE_2D:
      tic[2] |= NV50_TIC_2_TARGET_2D;
      break;
   case PIPE_TEXTURE_RECT:
      tic[2] |= NV50_TIC_2_TARGET_2D;
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
   default:
      NOUVEAU_ERR("unexpected/invalid texture target: %d\n",
                  mt->base.base.target);
      return FALSE;
   }

   tic[3] = (flags & NV50_TEXVIEW_FILTER_MSAA8) ? 0x20000000 : 0x00300000;

   if (flags & NV50_TEXVIEW_ACCESS_RESOLVE) {
      width = mt->base.base.width0 << mt->ms_x;
      height = mt->base.base.height0 << mt->ms_y;
   } else {
      width = mt->base.base.width0;
      height = mt->base.base.height0;
   }

   tic[4] = (1 << 31) | width;

   tic[5] = height & 0xffff;
   tic[5] |= depth << 16;
   tic[5] |= mt->base.base.last_level << 28;

   /* sampling points: (?) */
   if (flags & NV50_TEXVIEW_ACCESS_RESOLVE)
      tic[6] = (mt->ms_x > 1) ? 0x88000000 : 0x03000000;
   else
      tic[6] = 0x03000000;

   tic[7] = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;
   tic[7] |= mt->ms_mode << 12;

   return &view->pipe;
}

static boolean
nvc0_validate_tic(struct nvc0_context *nvc0, int s)
{
   uint32_t commands[32];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bo *txc = nvc0->screen->txc;
   unsigned i;
   unsigned n = 0;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
      struct nv04_resource *res;
      const boolean dirty = !!(nvc0->textures_dirty[s] & (1 << i));

      if (!tic) {
         if (dirty)
            commands[n++] = (i << 1) | 0;
         continue;
      }
      res = nv04_resource(tic->pipe.texture);

      if (tic->id < 0) {
         tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);

         PUSH_SPACE(push, 17);
         BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
         PUSH_DATAh(push, txc->offset + (tic->id * 32));
         PUSH_DATA (push, txc->offset + (tic->id * 32));
         BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
         PUSH_DATA (push, 0x100111);
         BEGIN_NIC0(push, NVC0_M2MF(DATA), 8);
         PUSH_DATAp(push, &tic->tic[0], 8);

         need_flush = TRUE;
      } else
      if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
         PUSH_DATA (push, (tic->id << 4) | 1);
         NOUVEAU_DRV_STAT(&nvc0->screen->base, tex_cache_flush_count, 1);
      }
      nvc0->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);

      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      res->status |=  NOUVEAU_BUFFER_STATUS_GPU_READING;

      if (!dirty)
         continue;
      commands[n++] = (tic->id << 9) | (i << 1) | 1;

      BCTX_REFN(nvc0->bufctx_3d, TEX(s, i), res, RD);
   }
   for (; i < nvc0->state.num_textures[s]; ++i)
      commands[n++] = (i << 1) | 0;

   nvc0->state.num_textures[s] = nvc0->num_textures[s];

   if (n) {
      BEGIN_NIC0(push, NVC0_3D(BIND_TIC(s)), n);
      PUSH_DATAp(push, commands, n);
   }
   nvc0->textures_dirty[s] = 0;

   return need_flush;
}

static boolean
nve4_validate_tic(struct nvc0_context *nvc0, unsigned s)
{
   struct nouveau_bo *txc = nvc0->screen->txc;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
      struct nv04_resource *res;
      const boolean dirty = !!(nvc0->textures_dirty[s] & (1 << i));

      if (!tic) {
         nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
         continue;
      }
      res = nv04_resource(tic->pipe.texture);

      if (tic->id < 0) {
         tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);

         PUSH_SPACE(push, 16);
         BEGIN_NVC0(push, NVE4_P2MF(DST_ADDRESS_HIGH), 2);
         PUSH_DATAh(push, txc->offset + (tic->id * 32));
         PUSH_DATA (push, txc->offset + (tic->id * 32));
         BEGIN_NVC0(push, NVE4_P2MF(LINE_LENGTH_IN), 2);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         BEGIN_1IC0(push, NVE4_P2MF(EXEC), 9);
         PUSH_DATA (push, 0x1001);
         PUSH_DATAp(push, &tic->tic[0], 8);

         need_flush = TRUE;
      } else
      if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
         PUSH_DATA (push, (tic->id << 4) | 1);
      }
      nvc0->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);

      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      res->status |=  NOUVEAU_BUFFER_STATUS_GPU_READING;

      nvc0->tex_handles[s][i] &= ~NVE4_TIC_ENTRY_INVALID;
      nvc0->tex_handles[s][i] |= tic->id;
      if (dirty)
         BCTX_REFN(nvc0->bufctx_3d, TEX(s, i), res, RD);
   }
   for (; i < nvc0->state.num_textures[s]; ++i) {
      nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
      nvc0->textures_dirty[s] |= 1 << i;
   }

   nvc0->state.num_textures[s] = nvc0->num_textures[s];

   return need_flush;
}

void nvc0_validate_textures(struct nvc0_context *nvc0)
{
   boolean need_flush;

   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      need_flush  = nve4_validate_tic(nvc0, 0);
      need_flush |= nve4_validate_tic(nvc0, 3);
      need_flush |= nve4_validate_tic(nvc0, 4);
   } else {
      need_flush  = nvc0_validate_tic(nvc0, 0);
      need_flush |= nvc0_validate_tic(nvc0, 3);
      need_flush |= nvc0_validate_tic(nvc0, 4);
   }

   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(TIC_FLUSH), 1);
      PUSH_DATA (nvc0->base.pushbuf, 0);
   }
}

static boolean
nvc0_validate_tsc(struct nvc0_context *nvc0, int s)
{
   uint32_t commands[16];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   unsigned n = 0;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_samplers[s]; ++i) {
      struct nv50_tsc_entry *tsc = nv50_tsc_entry(nvc0->samplers[s][i]);

      if (!(nvc0->samplers_dirty[s] & (1 << i)))
         continue;
      if (!tsc) {
         commands[n++] = (i << 4) | 0;
         continue;
      }
      if (tsc->id < 0) {
         tsc->id = nvc0_screen_tsc_alloc(nvc0->screen, tsc);

         nvc0_m2mf_push_linear(&nvc0->base, nvc0->screen->txc,
                               65536 + tsc->id * 32, NOUVEAU_BO_VRAM,
                               32, tsc->tsc);
         need_flush = TRUE;
      }
      nvc0->screen->tsc.lock[tsc->id / 32] |= 1 << (tsc->id % 32);

      commands[n++] = (tsc->id << 12) | (i << 4) | 1;
   }
   for (; i < nvc0->state.num_samplers[s]; ++i)
      commands[n++] = (i << 4) | 0;

   nvc0->state.num_samplers[s] = nvc0->num_samplers[s];

   if (n) {
      BEGIN_NIC0(push, NVC0_3D(BIND_TSC(s)), n);
      PUSH_DATAp(push, commands, n);
   }
   nvc0->samplers_dirty[s] = 0;

   return need_flush;
}

boolean
nve4_validate_tsc(struct nvc0_context *nvc0, int s)
{
   struct nouveau_bo *txc = nvc0->screen->txc;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_samplers[s]; ++i) {
      struct nv50_tsc_entry *tsc = nv50_tsc_entry(nvc0->samplers[s][i]);

      if (!tsc) {
         nvc0->tex_handles[s][i] |= NVE4_TSC_ENTRY_INVALID;
         continue;
      }
      if (tsc->id < 0) {
         tsc->id = nvc0_screen_tsc_alloc(nvc0->screen, tsc);

         PUSH_SPACE(push, 16);
         BEGIN_NVC0(push, NVE4_P2MF(DST_ADDRESS_HIGH), 2);
         PUSH_DATAh(push, txc->offset + 65536 + (tsc->id * 32));
         PUSH_DATA (push, txc->offset + 65536 + (tsc->id * 32));
         BEGIN_NVC0(push, NVE4_P2MF(LINE_LENGTH_IN), 2);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         BEGIN_1IC0(push, NVE4_P2MF(EXEC), 9);
         PUSH_DATA (push, 0x1001);
         PUSH_DATAp(push, &tsc->tsc[0], 8);

         need_flush = TRUE;
      }
      nvc0->screen->tsc.lock[tsc->id / 32] |= 1 << (tsc->id % 32);

      nvc0->tex_handles[s][i] &= ~NVE4_TSC_ENTRY_INVALID;
      nvc0->tex_handles[s][i] |= tsc->id << 20;
   }
   for (; i < nvc0->state.num_samplers[s]; ++i) {
      nvc0->tex_handles[s][i] |= NVE4_TSC_ENTRY_INVALID;
      nvc0->samplers_dirty[s] |= 1 << i;
   }

   nvc0->state.num_samplers[s] = nvc0->num_samplers[s];

   return need_flush;
}

void nvc0_validate_samplers(struct nvc0_context *nvc0)
{
   boolean need_flush;

   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      need_flush  = nve4_validate_tsc(nvc0, 0);
      need_flush |= nve4_validate_tsc(nvc0, 3);
      need_flush |= nve4_validate_tsc(nvc0, 4);
   } else {
      need_flush  = nvc0_validate_tsc(nvc0, 0);
      need_flush |= nvc0_validate_tsc(nvc0, 3);
      need_flush |= nvc0_validate_tsc(nvc0, 4);
   }

   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(TSC_FLUSH), 1);
      PUSH_DATA (nvc0->base.pushbuf, 0);
   }
}

/* Upload the "diagonal" entries for the possible texture sources ($t == $s).
 * At some point we might want to get a list of the combinations used by a
 * shader and fill in those entries instead of having it extract the handles.
 */
void
nve4_set_tex_handles(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint64_t address;
   unsigned s;

   if (nvc0->screen->base.class_3d < NVE4_3D_CLASS)
      return;
   address = nvc0->screen->uniform_bo->offset + (5 << 16);

   for (s = 0; s < 5; ++s, address += (1 << 9)) {
      uint32_t dirty = nvc0->textures_dirty[s] | nvc0->samplers_dirty[s];
      if (!dirty)
         continue;
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
      PUSH_DATA (push, 512);
      PUSH_DATAh(push, address);
      PUSH_DATA (push, address);
      do {
         int i = ffs(dirty) - 1;
         dirty &= ~(1 << i);

         BEGIN_NVC0(push, NVC0_3D(CB_POS), 2);
         PUSH_DATA (push, (8 + i) * 4);
         PUSH_DATA (push, nvc0->tex_handles[s][i]);
      } while (dirty);

      nvc0->textures_dirty[s] = 0;
      nvc0->samplers_dirty[s] = 0;
   }
}


static const uint8_t nve4_su_format_map[PIPE_FORMAT_COUNT];
static const uint16_t nve4_su_format_aux_map[PIPE_FORMAT_COUNT];
static const uint16_t nve4_suldp_lib_offset[PIPE_FORMAT_COUNT];

void
nve4_set_surface_info(struct nouveau_pushbuf *push,
                      struct pipe_surface *psf,
                      struct nvc0_screen *screen)
{
   struct nv50_surface *sf = nv50_surface(psf);
   struct nv04_resource *res;
   uint64_t address;
   uint32_t *const info = push->cur;
   uint8_t log2cpp;

   if (psf && !nve4_su_format_map[psf->format])
      NOUVEAU_ERR("unsupported surface format, try is_format_supported() !\n");

   push->cur += 16;

   if (!psf || !nve4_su_format_map[psf->format]) {
      memset(info, 0, 16 * sizeof(*info));

      info[0] = 0xbadf0000;
      info[1] = 0x80004000;
      info[12] = nve4_suldp_lib_offset[PIPE_FORMAT_R32G32B32A32_UINT] +
         screen->lib_code->start;
      return;
   }
   res = nv04_resource(sf->base.texture);

   address = res->address + sf->offset;

   info[8] = sf->width;
   info[9] = sf->height;
   info[10] = sf->depth;
   switch (res->base.target) {
   case PIPE_TEXTURE_1D_ARRAY:
      info[11] = 1;
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      info[11] = 2;
      break;
   case PIPE_TEXTURE_3D:
      info[11] = 3;
      break;
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      info[11] = 4;
      break;
   default:
      info[11] = 0;
      break;
   }
   log2cpp = (0xf000 & nve4_su_format_aux_map[sf->base.format]) >> 12;

   info[12] = nve4_suldp_lib_offset[sf->base.format] + screen->lib_code->start;

   /* limit in bytes for raw access */
   info[13] = (0x06 << 22) | ((sf->width << log2cpp) - 1);

   info[1] = nve4_su_format_map[sf->base.format];

#if 0
   switch (util_format_get_blocksizebits(sf->base.format)) {
   case  16: info[1] |= 1 << 16; break;
   case  32: info[1] |= 2 << 16; break;
   case  64: info[1] |= 3 << 16; break;
   case 128: info[1] |= 4 << 16; break;
   default:
      break;
   }
#else
   info[1] |= log2cpp << 16;
   info[1] |=  0x4000;
   info[1] |= (0x0f00 & nve4_su_format_aux_map[sf->base.format]);
#endif

   if (res->base.target == PIPE_BUFFER) {
      info[0]  = address >> 8;
      info[2]  = sf->width - 1;
      info[2] |= (0xff & nve4_su_format_aux_map[sf->base.format]) << 22;
      info[3]  = 0;
      info[4]  = 0;
      info[5]  = 0;
      info[6]  = 0;
      info[7]  = 0;
      info[14] = 0;
      info[15] = 0;
   } else {
      struct nv50_miptree *mt = nv50_miptree(&res->base);
      struct nv50_miptree_level *lvl = &mt->level[sf->base.u.tex.level];
      const unsigned z = sf->base.u.tex.first_layer;

      if (z) {
         if (mt->layout_3d) {
            address += nvc0_mt_zslice_offset(mt, psf->u.tex.level, z);
            /* doesn't work if z passes z-tile boundary */
            assert(sf->depth == 1);
         } else {
            address += mt->layer_stride * z;
         }
      }
      info[0]  = address >> 8;
      info[2]  = sf->width - 1;
      /* NOTE: this is really important: */
      info[2] |= (0xff & nve4_su_format_aux_map[sf->base.format]) << 22;
      info[3]  = (0x88 << 24) | (lvl->pitch / 64);
      info[4]  = sf->height - 1;
      info[4] |= (lvl->tile_mode & 0x0f0) << 25;
      info[4] |= NVC0_TILE_SHIFT_Y(lvl->tile_mode) << 22;
      info[5]  = mt->layer_stride >> 8;
      info[6]  = sf->depth - 1;
      info[6] |= (lvl->tile_mode & 0xf00) << 21;
      info[6] |= NVC0_TILE_SHIFT_Z(lvl->tile_mode) << 22;
      info[7]  = 0;
      info[14] = mt->ms_x;
      info[15] = mt->ms_y;
   }
}

static INLINE void
nvc0_update_surface_bindings(struct nvc0_context *nvc0)
{
   /* TODO */
}

static INLINE void
nve4_update_surface_bindings(struct nvc0_context *nvc0)
{
   /* TODO */
}

void
nvc0_validate_surfaces(struct nvc0_context *nvc0)
{
   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      nve4_update_surface_bindings(nvc0);
   } else {
      nvc0_update_surface_bindings(nvc0);
   }
}


static const uint8_t nve4_su_format_map[PIPE_FORMAT_COUNT] =
{
   [PIPE_FORMAT_R32G32B32A32_FLOAT] = NVE4_IMAGE_FORMAT_RGBA32_FLOAT,
   [PIPE_FORMAT_R32G32B32A32_SINT] = NVE4_IMAGE_FORMAT_RGBA32_SINT,
   [PIPE_FORMAT_R32G32B32A32_UINT] = NVE4_IMAGE_FORMAT_RGBA32_UINT,
   [PIPE_FORMAT_R16G16B16A16_FLOAT] = NVE4_IMAGE_FORMAT_RGBA16_FLOAT,
   [PIPE_FORMAT_R16G16B16A16_UNORM] = NVE4_IMAGE_FORMAT_RGBA16_UNORM,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = NVE4_IMAGE_FORMAT_RGBA16_SNORM,
   [PIPE_FORMAT_R16G16B16A16_SINT] = NVE4_IMAGE_FORMAT_RGBA16_SINT,
   [PIPE_FORMAT_R16G16B16A16_UINT] = NVE4_IMAGE_FORMAT_RGBA16_UINT,
   [PIPE_FORMAT_R8G8B8A8_UNORM] = NVE4_IMAGE_FORMAT_RGBA8_UNORM,
   [PIPE_FORMAT_R8G8B8A8_SNORM] = NVE4_IMAGE_FORMAT_RGBA8_SNORM,
   [PIPE_FORMAT_R8G8B8A8_SINT] = NVE4_IMAGE_FORMAT_RGBA8_SINT,
   [PIPE_FORMAT_R8G8B8A8_UINT] = NVE4_IMAGE_FORMAT_RGBA8_UINT,
   [PIPE_FORMAT_R11G11B10_FLOAT] = NVE4_IMAGE_FORMAT_R11G11B10_FLOAT,
   [PIPE_FORMAT_R10G10B10A2_UNORM] = NVE4_IMAGE_FORMAT_RGB10_A2_UNORM,
/* [PIPE_FORMAT_R10G10B10A2_UINT] = NVE4_IMAGE_FORMAT_RGB10_A2_UINT, */
   [PIPE_FORMAT_R32G32_FLOAT] = NVE4_IMAGE_FORMAT_RG32_FLOAT,
   [PIPE_FORMAT_R32G32_SINT] = NVE4_IMAGE_FORMAT_RG32_SINT,
   [PIPE_FORMAT_R32G32_UINT] = NVE4_IMAGE_FORMAT_RG32_UINT,
   [PIPE_FORMAT_R16G16_FLOAT] = NVE4_IMAGE_FORMAT_RG16_FLOAT,
   [PIPE_FORMAT_R16G16_UNORM] = NVE4_IMAGE_FORMAT_RG16_UNORM,
   [PIPE_FORMAT_R16G16_SNORM] = NVE4_IMAGE_FORMAT_RG16_SNORM,
   [PIPE_FORMAT_R16G16_SINT] = NVE4_IMAGE_FORMAT_RG16_SINT,
   [PIPE_FORMAT_R16G16_UINT] = NVE4_IMAGE_FORMAT_RG16_UINT,
   [PIPE_FORMAT_R8G8_UNORM] = NVE4_IMAGE_FORMAT_RG8_UNORM,
   [PIPE_FORMAT_R8G8_SNORM] = NVE4_IMAGE_FORMAT_RG8_SNORM,
   [PIPE_FORMAT_R8G8_SINT] = NVE4_IMAGE_FORMAT_RG8_SINT,
   [PIPE_FORMAT_R8G8_UINT] = NVE4_IMAGE_FORMAT_RG8_UINT,
   [PIPE_FORMAT_R32_FLOAT] = NVE4_IMAGE_FORMAT_R32_FLOAT,
   [PIPE_FORMAT_R32_SINT] = NVE4_IMAGE_FORMAT_R32_SINT,
   [PIPE_FORMAT_R32_UINT] = NVE4_IMAGE_FORMAT_R32_UINT,
   [PIPE_FORMAT_R16_FLOAT] = NVE4_IMAGE_FORMAT_R16_FLOAT,
   [PIPE_FORMAT_R16_UNORM] = NVE4_IMAGE_FORMAT_R16_UNORM,
   [PIPE_FORMAT_R16_SNORM] = NVE4_IMAGE_FORMAT_R16_SNORM,
   [PIPE_FORMAT_R16_SINT] = NVE4_IMAGE_FORMAT_R16_SINT,
   [PIPE_FORMAT_R16_UINT] = NVE4_IMAGE_FORMAT_R16_UINT,
   [PIPE_FORMAT_R8_UNORM] = NVE4_IMAGE_FORMAT_R8_UNORM,
   [PIPE_FORMAT_R8_SNORM] = NVE4_IMAGE_FORMAT_R8_SNORM,
   [PIPE_FORMAT_R8_SINT] = NVE4_IMAGE_FORMAT_R8_SINT,
   [PIPE_FORMAT_R8_UINT] = NVE4_IMAGE_FORMAT_R8_UINT,
};

/* Auxiliary format description values for surface instructions.
 * (log2(bytes per pixel) << 12) | (unk8 << 8) | unk22
 */
static const uint16_t nve4_su_format_aux_map[PIPE_FORMAT_COUNT] =
{
   [PIPE_FORMAT_R32G32B32A32_FLOAT] = 0x4842,
   [PIPE_FORMAT_R32G32B32A32_SINT] = 0x4842,
   [PIPE_FORMAT_R32G32B32A32_UINT] = 0x4842,

   [PIPE_FORMAT_R16G16B16A16_UNORM] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_SINT] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_UINT] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_FLOAT] = 0x3933,

   [PIPE_FORMAT_R32G32_FLOAT] = 0x3433,
   [PIPE_FORMAT_R32G32_SINT] = 0x3433,
   [PIPE_FORMAT_R32G32_UINT] = 0x3433,

   [PIPE_FORMAT_R10G10B10A2_UNORM] = 0x2a24,
/* [PIPE_FORMAT_R10G10B10A2_UINT] = 0x2a24, */
   [PIPE_FORMAT_R8G8B8A8_UNORM] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_SNORM] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_SINT] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_UINT] = 0x2a24,
   [PIPE_FORMAT_R11G11B10_FLOAT] = 0x2a24,

   [PIPE_FORMAT_R16G16_UNORM] = 0x2524,
   [PIPE_FORMAT_R16G16_SNORM] = 0x2524,
   [PIPE_FORMAT_R16G16_SINT] = 0x2524,
   [PIPE_FORMAT_R16G16_UINT] = 0x2524,
   [PIPE_FORMAT_R16G16_FLOAT] = 0x2524,

   [PIPE_FORMAT_R32_SINT] = 0x2024,
   [PIPE_FORMAT_R32_UINT] = 0x2024,
   [PIPE_FORMAT_R32_FLOAT] = 0x2024,

   [PIPE_FORMAT_R8G8_UNORM] = 0x1615,
   [PIPE_FORMAT_R8G8_SNORM] = 0x1615,
   [PIPE_FORMAT_R8G8_SINT] = 0x1615,
   [PIPE_FORMAT_R8G8_UINT] = 0x1615,

   [PIPE_FORMAT_R16_UNORM] = 0x1115,
   [PIPE_FORMAT_R16_SNORM] = 0x1115,
   [PIPE_FORMAT_R16_SINT] = 0x1115,
   [PIPE_FORMAT_R16_UINT] = 0x1115,
   [PIPE_FORMAT_R16_FLOAT] = 0x1115,

   [PIPE_FORMAT_R8_UNORM] = 0x0206,
   [PIPE_FORMAT_R8_SNORM] = 0x0206,
   [PIPE_FORMAT_R8_SINT] = 0x0206,
   [PIPE_FORMAT_R8_UINT] = 0x0206
};

/* NOTE: These are hardcoded offsets for the shader library.
 * TODO: Automate them.
 */
static const uint16_t nve4_suldp_lib_offset[PIPE_FORMAT_COUNT] =
{
   [PIPE_FORMAT_R32G32B32A32_FLOAT] = 0x218,
   [PIPE_FORMAT_R32G32B32A32_SINT]  = 0x218,
   [PIPE_FORMAT_R32G32B32A32_UINT]  = 0x218,
   [PIPE_FORMAT_R16G16B16A16_UNORM] = 0x248,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = 0x2b8,
   [PIPE_FORMAT_R16G16B16A16_SINT]  = 0x330,
   [PIPE_FORMAT_R16G16B16A16_UINT]  = 0x388,
   [PIPE_FORMAT_R16G16B16A16_FLOAT] = 0x3d8,
   [PIPE_FORMAT_R32G32_FLOAT]       = 0x428,
   [PIPE_FORMAT_R32G32_SINT]        = 0x468,
   [PIPE_FORMAT_R32G32_UINT]        = 0x468,
   [PIPE_FORMAT_R10G10B10A2_UNORM]  = 0x4a8,
/* [PIPE_FORMAT_R10G10B10A2_UINT]   = 0x530, */
   [PIPE_FORMAT_R8G8B8A8_UNORM]     = 0x588,
   [PIPE_FORMAT_R8G8B8A8_SNORM]     = 0x5f8,
   [PIPE_FORMAT_R8G8B8A8_SINT]      = 0x670,
   [PIPE_FORMAT_R8G8B8A8_UINT]      = 0x6c8,
   [PIPE_FORMAT_B5G6R5_UNORM]       = 0x718,
   [PIPE_FORMAT_B5G5R5X1_UNORM]     = 0x7a0,
   [PIPE_FORMAT_R16G16_UNORM]       = 0x828,
   [PIPE_FORMAT_R16G16_SNORM]       = 0x890,
   [PIPE_FORMAT_R16G16_SINT]        = 0x8f0,
   [PIPE_FORMAT_R16G16_UINT]        = 0x948,
   [PIPE_FORMAT_R16G16_FLOAT]       = 0x998,
   [PIPE_FORMAT_R32_FLOAT]          = 0x9e8,
   [PIPE_FORMAT_R32_SINT]           = 0xa30,
   [PIPE_FORMAT_R32_UINT]           = 0xa30,
   [PIPE_FORMAT_R8G8_UNORM]         = 0xa78,
   [PIPE_FORMAT_R8G8_SNORM]         = 0xae0,
   [PIPE_FORMAT_R8G8_UINT]          = 0xb48,
   [PIPE_FORMAT_R8G8_SINT]          = 0xb98,
   [PIPE_FORMAT_R16_UNORM]          = 0xbe8,
   [PIPE_FORMAT_R16_SNORM]          = 0xc48,
   [PIPE_FORMAT_R16_SINT]           = 0xca0,
   [PIPE_FORMAT_R16_UINT]           = 0xce8,
   [PIPE_FORMAT_R16_FLOAT]          = 0xd30,
   [PIPE_FORMAT_R8_UNORM]           = 0xd88,
   [PIPE_FORMAT_R8_SNORM]           = 0xde0,
   [PIPE_FORMAT_R8_SINT]            = 0xe38,
   [PIPE_FORMAT_R8_UINT]            = 0xe88,
   [PIPE_FORMAT_R11G11B10_FLOAT]    = 0xed0
};
