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

#include "nvc0_context.h"
#include "nvc0_screen.h"
#include "nvc0_resource.h"

#include "nouveau/nouveau_reloc.h"

static void
nvc0_flush(struct pipe_context *pipe,
           struct pipe_fence_handle **fence)
{
   struct nouveau_screen *screen = &nvc0_context(pipe)->screen->base;

   if (fence)
      nouveau_fence_ref(screen->fence.current, (struct nouveau_fence **)fence);

   /* Try to emit before firing to avoid having to flush again right after
    * in case we have to wait on this fence.
    */
   nouveau_fence_emit(screen->fence.current);

   FIRE_RING(screen->channel);
}

static void
nvc0_texture_barrier(struct pipe_context *pipe)
{
   struct nouveau_channel *chan = nvc0_context(pipe)->screen->base.channel;

   IMMED_RING(chan, RING_3D(SERIALIZE), 0);
   IMMED_RING(chan, RING_3D(TEX_CACHE_CTL), 0);
}

static void
nvc0_context_unreference_resources(struct nvc0_context *nvc0)
{
   unsigned s, i;

   for (i = 0; i < NVC0_BUFCTX_COUNT; ++i)
      nvc0_bufctx_reset(nvc0, i);

   for (i = 0; i < nvc0->num_vtxbufs; ++i)
      pipe_resource_reference(&nvc0->vtxbuf[i].buffer, NULL);

   pipe_resource_reference(&nvc0->idxbuf.buffer, NULL);

   for (s = 0; s < 5; ++s) {
      for (i = 0; i < nvc0->num_textures[s]; ++i)
         pipe_sampler_view_reference(&nvc0->textures[s][i], NULL);

      for (i = 0; i < 16; ++i)
         pipe_resource_reference(&nvc0->constbuf[s][i], NULL);
   }

   for (i = 0; i < nvc0->num_tfbbufs; ++i)
      pipe_resource_reference(&nvc0->tfbbuf[i], NULL);
}

static void
nvc0_destroy(struct pipe_context *pipe)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);

   nvc0_context_unreference_resources(nvc0);

   draw_destroy(nvc0->draw);

   if (nvc0->screen->cur_ctx == nvc0) {
      nvc0->screen->base.channel->user_private = NULL;
      nvc0->screen->cur_ctx = NULL;
   }

   FREE(nvc0);
}

void
nvc0_default_flush_notify(struct nouveau_channel *chan)
{
   struct nvc0_context *nvc0 = chan->user_private;

   if (!nvc0)
      return;

   nouveau_fence_update(&nvc0->screen->base, TRUE);
   nouveau_fence_next(&nvc0->screen->base);
}

struct pipe_context *
nvc0_create(struct pipe_screen *pscreen, void *priv)
{
   struct pipe_winsys *pipe_winsys = pscreen->winsys;
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nvc0_context *nvc0;
   struct pipe_context *pipe;

   nvc0 = CALLOC_STRUCT(nvc0_context);
   if (!nvc0)
      return NULL;
   pipe = &nvc0->base.pipe;

   nvc0->screen = screen;
   nvc0->base.screen    = &screen->base;
   nvc0->base.copy_data = nvc0_m2mf_copy_linear;
   nvc0->base.push_data = nvc0_m2mf_push_linear;

   pipe->winsys = pipe_winsys;
   pipe->screen = pscreen;
   pipe->priv = priv;

   pipe->destroy = nvc0_destroy;

   pipe->draw_vbo = nvc0_draw_vbo;
   pipe->clear = nvc0_clear;

   pipe->flush = nvc0_flush;
   pipe->texture_barrier = nvc0_texture_barrier;

   if (!screen->cur_ctx)
      screen->cur_ctx = nvc0;
   screen->base.channel->user_private = nvc0;
   screen->base.channel->flush_notify = nvc0_default_flush_notify;

   nvc0_init_query_functions(nvc0);
   nvc0_init_surface_functions(nvc0);
   nvc0_init_state_functions(nvc0);
   nvc0_init_resource_functions(pipe);

   nvc0->draw = draw_create(pipe);
   assert(nvc0->draw);
   draw_set_rasterize_stage(nvc0->draw, nvc0_draw_render_stage(nvc0));

   return pipe;
}

struct resident {
   struct nv04_resource *res;
   uint32_t flags;
};

void
nvc0_bufctx_add_resident(struct nvc0_context *nvc0, int ctx,
                         struct nv04_resource *resource, uint32_t flags)
{
   struct resident rsd = { resource, flags };

   if (!resource->bo)
      return;
   nvc0->residents_size += sizeof(struct resident);

   /* We don't need to reference the resource here, it will be referenced
    * in the context/state, and bufctx will be reset when state changes.
    */
   util_dynarray_append(&nvc0->residents[ctx], struct resident, rsd);
}

void
nvc0_bufctx_del_resident(struct nvc0_context *nvc0, int ctx,
                         struct nv04_resource *resource)
{
   struct resident *rsd, *top;
   unsigned i;

   for (i = 0; i < nvc0->residents[ctx].size / sizeof(struct resident); ++i) {
      rsd = util_dynarray_element(&nvc0->residents[ctx], struct resident, i);

      if (rsd->res == resource) {
         top = util_dynarray_pop_ptr(&nvc0->residents[ctx], struct resident);
         if (rsd != top)
            *rsd = *top;
         nvc0->residents_size -= sizeof(struct resident);
         break;
      }
   }
}

void
nvc0_bufctx_emit_relocs(struct nvc0_context *nvc0)
{
   struct resident *rsd;
   struct util_dynarray *array;
   unsigned ctx, i, n;

   n  = nvc0->residents_size / sizeof(struct resident);
   n += NVC0_SCREEN_RESIDENT_BO_COUNT;

   MARK_RING(nvc0->screen->base.channel, n, n);

   for (ctx = 0; ctx < NVC0_BUFCTX_COUNT; ++ctx) {
      array = &nvc0->residents[ctx];

      n = array->size / sizeof(struct resident);
      for (i = 0; i < n; ++i) {
         rsd = util_dynarray_element(array, struct resident, i);

         nvc0_resource_validate(rsd->res, rsd->flags);
      }
   }

   nvc0_screen_make_buffers_resident(nvc0->screen);
}
