/*
 * Copyright 2010 Christoph Bumiller
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

#include "pipe/p_defines.h"
#include "util/u_framebuffer.h"

#ifdef NV50_WITH_DRAW_MODULE
#include "draw/draw_context.h"
#endif

#include "nv50/nv50_context.h"
#include "nv50/nv50_screen.h"
#include "nv50/nv50_resource.h"

static void
nv50_flush(struct pipe_context *pipe,
           struct pipe_fence_handle **fence,
           unsigned flags)
{
   struct nouveau_screen *screen = nouveau_screen(pipe->screen);

   if (fence)
      nouveau_fence_ref(screen->fence.current, (struct nouveau_fence **)fence);

   PUSH_KICK(screen->pushbuf);

   nouveau_context_update_frame_stats(nouveau_context(pipe));
}

static void
nv50_texture_barrier(struct pipe_context *pipe)
{
   struct nouveau_pushbuf *push = nv50_context(pipe)->base.pushbuf;

   BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(TEX_CACHE_CTL), 1);
   PUSH_DATA (push, 0x20);
}

static void
nv50_memory_barrier(struct pipe_context *pipe, unsigned flags)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   int i, s;

   if (flags & PIPE_BARRIER_MAPPED_BUFFER) {
      for (i = 0; i < nv50->num_vtxbufs; ++i) {
         if (!nv50->vtxbuf[i].buffer)
            continue;
         if (nv50->vtxbuf[i].buffer->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
            nv50->base.vbo_dirty = TRUE;
      }

      if (nv50->idxbuf.buffer &&
          nv50->idxbuf.buffer->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
         nv50->base.vbo_dirty = TRUE;

      for (s = 0; s < 3 && !nv50->cb_dirty; ++s) {
         uint32_t valid = nv50->constbuf_valid[s];

         while (valid && !nv50->cb_dirty) {
            const unsigned i = ffs(valid) - 1;
            struct pipe_resource *res;

            valid &= ~(1 << i);
            if (nv50->constbuf[s][i].user)
               continue;

            res = nv50->constbuf[s][i].u.buf;
            if (!res)
               continue;

            if (res->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
               nv50->cb_dirty = TRUE;
         }
      }
   }
}

void
nv50_default_kick_notify(struct nouveau_pushbuf *push)
{
   struct nv50_screen *screen = push->user_priv;

   if (screen) {
      nouveau_fence_next(&screen->base);
      nouveau_fence_update(&screen->base, TRUE);
      if (screen->cur_ctx)
         screen->cur_ctx->state.flushed = TRUE;
   }
}

static void
nv50_context_unreference_resources(struct nv50_context *nv50)
{
   unsigned s, i;

   nouveau_bufctx_del(&nv50->bufctx_3d);
   nouveau_bufctx_del(&nv50->bufctx);

   util_unreference_framebuffer_state(&nv50->framebuffer);

   assert(nv50->num_vtxbufs <= PIPE_MAX_ATTRIBS);
   for (i = 0; i < nv50->num_vtxbufs; ++i)
      pipe_resource_reference(&nv50->vtxbuf[i].buffer, NULL);

   pipe_resource_reference(&nv50->idxbuf.buffer, NULL);

   for (s = 0; s < 3; ++s) {
      assert(nv50->num_textures[s] <= PIPE_MAX_SAMPLERS);
      for (i = 0; i < nv50->num_textures[s]; ++i)
         pipe_sampler_view_reference(&nv50->textures[s][i], NULL);

      for (i = 0; i < NV50_MAX_PIPE_CONSTBUFS; ++i)
         if (!nv50->constbuf[s][i].user)
            pipe_resource_reference(&nv50->constbuf[s][i].u.buf, NULL);
   }
}

static void
nv50_destroy(struct pipe_context *pipe)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   if (nv50_context_screen(nv50)->cur_ctx == nv50)
      nv50_context_screen(nv50)->cur_ctx = NULL;
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, NULL);
   nouveau_pushbuf_kick(nv50->base.pushbuf, nv50->base.pushbuf->channel);

   nv50_context_unreference_resources(nv50);

#ifdef NV50_WITH_DRAW_MODULE
   draw_destroy(nv50->draw);
#endif

   FREE(nv50->blit);

   nouveau_context_destroy(&nv50->base);
}

static int
nv50_invalidate_resource_storage(struct nouveau_context *ctx,
                                 struct pipe_resource *res,
                                 int ref)
{
   struct nv50_context *nv50 = nv50_context(&ctx->pipe);
   unsigned s, i;

   if (res->bind & PIPE_BIND_RENDER_TARGET) {
      assert(nv50->framebuffer.nr_cbufs <= PIPE_MAX_COLOR_BUFS);
      for (i = 0; i < nv50->framebuffer.nr_cbufs; ++i) {
         if (nv50->framebuffer.cbufs[i] &&
             nv50->framebuffer.cbufs[i]->texture == res) {
            nv50->dirty |= NV50_NEW_FRAMEBUFFER;
            nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_FB);
            if (!--ref)
               return ref;
         }
      }
   }
   if (res->bind & PIPE_BIND_DEPTH_STENCIL) {
      if (nv50->framebuffer.zsbuf &&
          nv50->framebuffer.zsbuf->texture == res) {
         nv50->dirty |= NV50_NEW_FRAMEBUFFER;
         nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_FB);
         if (!--ref)
            return ref;
      }
   }

   if (res->bind & PIPE_BIND_VERTEX_BUFFER) {
      assert(nv50->num_vtxbufs <= PIPE_MAX_ATTRIBS);
      for (i = 0; i < nv50->num_vtxbufs; ++i) {
         if (nv50->vtxbuf[i].buffer == res) {
            nv50->dirty |= NV50_NEW_ARRAYS;
            nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_VERTEX);
            if (!--ref)
               return ref;
         }
      }
   }
   if (res->bind & PIPE_BIND_INDEX_BUFFER) {
      if (nv50->idxbuf.buffer == res)
         if (!--ref)
            return ref;
   }

   if (res->bind & PIPE_BIND_SAMPLER_VIEW) {
      for (s = 0; s < 3; ++s) {
      assert(nv50->num_textures[s] <= PIPE_MAX_SAMPLERS);
      for (i = 0; i < nv50->num_textures[s]; ++i) {
         if (nv50->textures[s][i] &&
             nv50->textures[s][i]->texture == res) {
            nv50->dirty |= NV50_NEW_TEXTURES;
            nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_TEXTURES);
            if (!--ref)
               return ref;
         }
      }
      }
   }

   if (res->bind & PIPE_BIND_CONSTANT_BUFFER) {
      for (s = 0; s < 3; ++s) {
      assert(nv50->num_vtxbufs <= NV50_MAX_PIPE_CONSTBUFS);
      for (i = 0; i < nv50->num_vtxbufs; ++i) {
         if (!nv50->constbuf[s][i].user &&
             nv50->constbuf[s][i].u.buf == res) {
            nv50->dirty |= NV50_NEW_CONSTBUF;
            nv50->constbuf_dirty[s] |= 1 << i;
            nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_CB(s, i));
            if (!--ref)
               return ref;
         }
      }
      }
   }

   return ref;
}

static void
nv50_context_get_sample_position(struct pipe_context *, unsigned, unsigned,
                                 float *);

struct pipe_context *
nv50_create(struct pipe_screen *pscreen, void *priv)
{
   struct nv50_screen *screen = nv50_screen(pscreen);
   struct nv50_context *nv50;
   struct pipe_context *pipe;
   int ret;
   uint32_t flags;

   nv50 = CALLOC_STRUCT(nv50_context);
   if (!nv50)
      return NULL;
   pipe = &nv50->base.pipe;

   if (!nv50_blitctx_create(nv50))
      goto out_err;

   nv50->base.pushbuf = screen->base.pushbuf;
   nv50->base.client = screen->base.client;

   ret = nouveau_bufctx_new(screen->base.client, NV50_BIND_COUNT,
                            &nv50->bufctx_3d);
   if (!ret)
      ret = nouveau_bufctx_new(screen->base.client, 2, &nv50->bufctx);
   if (ret)
      goto out_err;

   nv50->base.screen    = &screen->base;
   nv50->base.copy_data = nv50_m2mf_copy_linear;
   nv50->base.push_data = nv50_sifc_linear_u8;
   /* FIXME: Make it possible to use this again. The problem is that there is
    * some clever logic in the card that allows for multiple renders to happen
    * when there are only constbuf changes. However that relies on the
    * constbuf updates happening to the right constbuf slots. Currently
    * implementation just makes it go through a separate slot which doesn't
    * properly update the right constbuf data.
   nv50->base.push_cb   = nv50_cb_push;
    */

   nv50->screen = screen;
   pipe->screen = pscreen;
   pipe->priv = priv;

   pipe->destroy = nv50_destroy;

   pipe->draw_vbo = nv50_draw_vbo;
   pipe->clear = nv50_clear;

   pipe->flush = nv50_flush;
   pipe->texture_barrier = nv50_texture_barrier;
   pipe->memory_barrier = nv50_memory_barrier;
   pipe->get_sample_position = nv50_context_get_sample_position;

   if (!screen->cur_ctx) {
      screen->cur_ctx = nv50;
      nouveau_pushbuf_bufctx(screen->base.pushbuf, nv50->bufctx);
   }
   nv50->base.pushbuf->kick_notify = nv50_default_kick_notify;

   nv50_init_query_functions(nv50);
   nv50_init_surface_functions(nv50);
   nv50_init_state_functions(nv50);
   nv50_init_resource_functions(pipe);

   nv50->base.invalidate_resource_storage = nv50_invalidate_resource_storage;

#ifdef NV50_WITH_DRAW_MODULE
   /* no software fallbacks implemented */
   nv50->draw = draw_create(pipe);
   assert(nv50->draw);
   draw_set_rasterize_stage(nv50->draw, nv50_draw_render_stage(nv50));
#endif

   if (screen->base.device->chipset < 0x84 ||
       debug_get_bool_option("NOUVEAU_PMPEG", FALSE)) {
      /* PMPEG */
      nouveau_context_init_vdec(&nv50->base);
   } else if (screen->base.device->chipset < 0x98 ||
              screen->base.device->chipset == 0xa0) {
      /* VP2 */
      pipe->create_video_codec = nv84_create_decoder;
      pipe->create_video_buffer = nv84_video_buffer_create;
   } else {
      /* VP3/4 */
      pipe->create_video_codec = nv98_create_decoder;
      pipe->create_video_buffer = nv98_video_buffer_create;
   }

   flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->code);
   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->uniforms);
   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->txc);
   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->stack_bo);

   flags = NOUVEAU_BO_GART | NOUVEAU_BO_WR;

   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->fence.bo);
   BCTX_REFN_bo(nv50->bufctx, FENCE, flags, screen->fence.bo);

   nv50->base.scratch.bo_size = 2 << 20;

   return pipe;

out_err:
   if (nv50->bufctx_3d)
      nouveau_bufctx_del(&nv50->bufctx_3d);
   if (nv50->bufctx)
      nouveau_bufctx_del(&nv50->bufctx);
   if (nv50->blit)
      FREE(nv50->blit);
   FREE(nv50);
   return NULL;
}

void
nv50_bufctx_fence(struct nouveau_bufctx *bufctx, boolean on_flush)
{
   struct nouveau_list *list = on_flush ? &bufctx->current : &bufctx->pending;
   struct nouveau_list *it;

   for (it = list->next; it != list; it = it->next) {
      struct nouveau_bufref *ref = (struct nouveau_bufref *)it;
      struct nv04_resource *res = ref->priv;
      if (res)
         nv50_resource_validate(res, (unsigned)ref->priv_data);
   }
}

static void
nv50_context_get_sample_position(struct pipe_context *pipe,
                                 unsigned sample_count, unsigned sample_index,
                                 float *xy)
{
   static const uint8_t ms1[1][2] = { { 0x8, 0x8 } };
   static const uint8_t ms2[2][2] = {
      { 0x4, 0x4 }, { 0xc, 0xc } }; /* surface coords (0,0), (1,0) */
   static const uint8_t ms4[4][2] = {
      { 0x6, 0x2 }, { 0xe, 0x6 },   /* (0,0), (1,0) */
      { 0x2, 0xa }, { 0xa, 0xe } }; /* (0,1), (1,1) */
   static const uint8_t ms8[8][2] = {
      { 0x1, 0x7 }, { 0x5, 0x3 },   /* (0,0), (1,0) */
      { 0x3, 0xd }, { 0x7, 0xb },   /* (0,1), (1,1) */
      { 0x9, 0x5 }, { 0xf, 0x1 },   /* (2,0), (3,0) */
      { 0xb, 0xf }, { 0xd, 0x9 } }; /* (2,1), (3,1) */
#if 0
   /* NOTE: there are alternative modes for MS2 and MS8, currently not used */
   static const uint8_t ms8_alt[8][2] = {
      { 0x9, 0x5 }, { 0x7, 0xb },   /* (2,0), (1,1) */
      { 0xd, 0x9 }, { 0x5, 0x3 },   /* (3,1), (1,0) */
      { 0x3, 0xd }, { 0x1, 0x7 },   /* (0,1), (0,0) */
      { 0xb, 0xf }, { 0xf, 0x1 } }; /* (2,1), (3,0) */
#endif

   const uint8_t (*ptr)[2];

   switch (sample_count) {
   case 0:
   case 1: ptr = ms1; break;
   case 2: ptr = ms2; break;
   case 4: ptr = ms4; break;
   case 8: ptr = ms8; break;
   default:
      assert(0);
      return; /* bad sample count -> undefined locations */
   }
   xy[0] = ptr[sample_index][0] * 0.0625f;
   xy[1] = ptr[sample_index][1] * 0.0625f;
}
