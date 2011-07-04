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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "draw/draw_context.h"
#include "pipe/p_defines.h"

#include "nv50_context.h"
#include "nv50_screen.h"
#include "nv50_resource.h"

#include "nouveau/nouveau_reloc.h"

static void
nv50_flush(struct pipe_context *pipe,
           struct pipe_fence_handle **fence)
{
   struct nouveau_screen *screen = &nv50_context(pipe)->screen->base;

   if (fence)
      nouveau_fence_ref(screen->fence.current, (struct nouveau_fence **)fence);

   /* Try to emit before firing to avoid having to flush again right after
    * in case we have to wait on this fence.
    */
   nouveau_fence_emit(screen->fence.current);

   FIRE_RING(screen->channel);
}

static void
nv50_texture_barrier(struct pipe_context *pipe)
{
   struct nouveau_channel *chan = nv50_context(pipe)->screen->base.channel;

   BEGIN_RING(chan, RING_3D(SERIALIZE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(TEX_CACHE_CTL), 1);
   OUT_RING  (chan, 0x20);
}

void
nv50_default_flush_notify(struct nouveau_channel *chan)
{
   struct nv50_context *nv50 = chan->user_private;

   if (!nv50)
      return;

   nouveau_fence_update(&nv50->screen->base, TRUE);
   nouveau_fence_next(&nv50->screen->base);
}

static void
nv50_context_unreference_resources(struct nv50_context *nv50)
{
   unsigned s, i;

   for (i = 0; i < NV50_BUFCTX_COUNT; ++i)
      nv50_bufctx_reset(nv50, i);

   for (i = 0; i < nv50->num_vtxbufs; ++i)
      pipe_resource_reference(&nv50->vtxbuf[i].buffer, NULL);

   pipe_resource_reference(&nv50->idxbuf.buffer, NULL);

   for (s = 0; s < 3; ++s) {
      for (i = 0; i < nv50->num_textures[s]; ++i)
         pipe_sampler_view_reference(&nv50->textures[s][i], NULL);

      for (i = 0; i < 16; ++i)
         pipe_resource_reference(&nv50->constbuf[s][i], NULL);
   }
}

static void
nv50_destroy(struct pipe_context *pipe)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50_context_unreference_resources(nv50);

   draw_destroy(nv50->draw);

   if (nv50->screen->cur_ctx == nv50) {
      nv50->screen->base.channel->user_private = NULL;
      nv50->screen->cur_ctx = NULL;
   }

   FREE(nv50);
}

struct pipe_context *
nv50_create(struct pipe_screen *pscreen, void *priv)
{
   struct pipe_winsys *pipe_winsys = pscreen->winsys;
   struct nv50_screen *screen = nv50_screen(pscreen);
   struct nv50_context *nv50;
   struct pipe_context *pipe;

   nv50 = CALLOC_STRUCT(nv50_context);
   if (!nv50)
      return NULL;
   pipe = &nv50->base.pipe;

   nv50->screen = screen;
   nv50->base.screen    = &screen->base;
   nv50->base.copy_data = nv50_m2mf_copy_linear;
   nv50->base.push_data = nv50_sifc_linear_u8;

   pipe->winsys = pipe_winsys;
   pipe->screen = pscreen;
   pipe->priv = priv;

   pipe->destroy = nv50_destroy;

   pipe->draw_vbo = nv50_draw_vbo;
   pipe->clear = nv50_clear;

   pipe->flush = nv50_flush;
   pipe->texture_barrier = nv50_texture_barrier;

   if (!screen->cur_ctx)
      screen->cur_ctx = nv50;
   screen->base.channel->user_private = nv50;
   screen->base.channel->flush_notify = nv50_default_flush_notify;

   nv50_init_query_functions(nv50);
   nv50_init_surface_functions(nv50);
   nv50_init_state_functions(nv50);
   nv50_init_resource_functions(pipe);

   nv50->draw = draw_create(pipe);
   assert(nv50->draw);
   draw_set_rasterize_stage(nv50->draw, nv50_draw_render_stage(nv50));

   return pipe;
}

struct resident {
   struct nv04_resource *res;
   uint32_t flags;
};

void
nv50_bufctx_add_resident(struct nv50_context *nv50, int ctx,
                         struct nv04_resource *resource, uint32_t flags)
{
   struct resident rsd = { resource, flags };

   if (!resource->bo)
      return;
   nv50->residents_size += sizeof(struct resident);

   /* We don't need to reference the resource here, it will be referenced
    * in the context/state, and bufctx will be reset when state changes.
    */
   util_dynarray_append(&nv50->residents[ctx], struct resident, rsd);
}

void
nv50_bufctx_del_resident(struct nv50_context *nv50, int ctx,
                         struct nv04_resource *resource)
{
   struct resident *rsd, *top;
   unsigned i;

   for (i = 0; i < nv50->residents[ctx].size / sizeof(struct resident); ++i) {
      rsd = util_dynarray_element(&nv50->residents[ctx], struct resident, i);

      if (rsd->res == resource) {
         top = util_dynarray_pop_ptr(&nv50->residents[ctx], struct resident);
         if (rsd != top)
            *rsd = *top;
         nv50->residents_size -= sizeof(struct resident);
         break;
      }
   }
}

void
nv50_bufctx_emit_relocs(struct nv50_context *nv50)
{
   struct resident *rsd;
   struct util_dynarray *array;
   unsigned ctx, i, n;

   n  = nv50->residents_size / sizeof(struct resident);
   n += NV50_SCREEN_RESIDENT_BO_COUNT;

   MARK_RING(nv50->screen->base.channel, n, n);

   for (ctx = 0; ctx < NV50_BUFCTX_COUNT; ++ctx) {
      array = &nv50->residents[ctx];

      n = array->size / sizeof(struct resident);
      for (i = 0; i < n; ++i) {
         rsd = util_dynarray_element(array, struct resident, i);

         nv50_resource_validate(rsd->res, rsd->flags);
      }
   }

   nv50_screen_make_buffers_resident(nv50->screen);
}
