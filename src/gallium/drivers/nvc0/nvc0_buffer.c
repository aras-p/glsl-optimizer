
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#define NOUVEAU_NVC0
#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#undef NOUVEAU_NVC0

#include "nvc0_context.h"
#include "nvc0_resource.h"

static void
nvc0_buffer_destroy(struct pipe_screen *pscreen,
                    struct pipe_resource *presource)
{
   struct nvc0_resource *res = nvc0_resource(presource);

   if (res->bo)
      nouveau_screen_bo_release(pscreen, res->bo);

   if (res->data)
      FREE(res->data);

   FREE(res);
}

static void *
nvc0_buffer_transfer_map(struct pipe_context *pipe,
                         struct pipe_transfer *transfer)
{
   struct nvc0_resource *res = nvc0_resource(transfer->resource);
   uint8_t *map;
   uint32_t flags;

   if (res->base.bind & PIPE_BIND_VERTEX_BUFFER)
      nvc0_context(pipe)->vbo_dirty = TRUE;

// #ifdef NOUVEAU_USERPSACE_MM
   if (res->base.bind & PIPE_BIND_CONSTANT_BUFFER)
      return res->data + transfer->box.x;
// #endif
   flags = nouveau_screen_transfer_flags(transfer->usage);

   map = nouveau_screen_bo_map_range(pipe->screen,
                                     res->bo,
                                     transfer->box.x, transfer->box.width,
                                     flags);
   if (!map)
      return NULL;

   return map + transfer->box.x;
}



static void
nvc0_buffer_transfer_flush_region(struct pipe_context *pipe,
                                  struct pipe_transfer *transfer,
                                  const struct pipe_box *box)
{
   struct nvc0_resource *res = nvc0_resource(transfer->resource);

#ifdef NOUVEAU_USERPSACE_MM
   if (!res->bo)
      return;
#endif
   nouveau_screen_bo_map_flush_range(pipe->screen,
                                     res->bo,
                                     transfer->box.x + box->x,
                                     box->width);
}

static void
nvc0_buffer_transfer_unmap(struct pipe_context *pipe,
                           struct pipe_transfer *transfer)
{
   struct nvc0_resource *res = nvc0_resource(transfer->resource);

// #ifdef NOUVEAU_USERPSACE_MM
   if (res->data)
      return;
// #endif
   nouveau_screen_bo_unmap(pipe->screen, res->bo);
}

const struct u_resource_vtbl nvc0_buffer_vtbl =
{
   u_default_resource_get_handle,     /* get_handle */
   nvc0_buffer_destroy,               /* resource_destroy */
   NULL,                              /* is_resource_referenced */
   u_default_get_transfer,            /* get_transfer */
   u_default_transfer_destroy,        /* transfer_destroy */
   nvc0_buffer_transfer_map,          /* transfer_map */
   nvc0_buffer_transfer_flush_region, /* transfer_flush_region */
   nvc0_buffer_transfer_unmap,        /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};

struct pipe_resource *
nvc0_buffer_create(struct pipe_screen *pscreen,
                   const struct pipe_resource *templ)
{
   struct nvc0_resource *buffer;

   buffer = CALLOC_STRUCT(nvc0_resource);
   if (!buffer)
      return NULL;

   buffer->base = *templ;
   buffer->vtbl = &nvc0_buffer_vtbl;
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = pscreen;

   if (buffer->base.bind & PIPE_BIND_CONSTANT_BUFFER)
      buffer->data = MALLOC(buffer->base.width0);

   buffer->bo = nouveau_screen_bo_new(pscreen,
                                      16,
                                      buffer->base.usage,
                                      buffer->base.bind,
                                      buffer->base.width0);
   if (buffer->bo == NULL)
      goto fail;

   return &buffer->base;

fail:
   FREE(buffer);
   return NULL;
}


struct pipe_resource *
nvc0_user_buffer_create(struct pipe_screen *pscreen,
                        void *ptr,
                        unsigned bytes,
                        unsigned bind)
{
   struct nvc0_resource *buffer;

   buffer = CALLOC_STRUCT(nvc0_resource);
   if (!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->vtbl = &nvc0_buffer_vtbl;
   buffer->base.screen = pscreen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.bind = bind;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;

   buffer->bo = nouveau_screen_bo_user(pscreen, ptr, bytes);
   if (!buffer->bo)
      goto fail;
	
   return &buffer->base;

fail:
   FREE(buffer);
   return NULL;
}
