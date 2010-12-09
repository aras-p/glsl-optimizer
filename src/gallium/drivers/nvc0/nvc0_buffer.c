
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#define NOUVEAU_NVC0
#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#undef NOUVEAU_NVC0

#include "nvc0_context.h"
#include "nvc0_resource.h"

#define NVC0_BUFFER_STATUS_USER_MEMORY 0xff

static INLINE boolean
nvc0_buffer_allocate(struct nvc0_screen *screen, struct nvc0_resource *buf,
                     unsigned domain)
{
   if (domain == NOUVEAU_BO_VRAM) {
      buf->mm = nvc0_mm_allocate(screen->mm_VRAM, buf->base.width0, &buf->bo,
                                 &buf->offset);
      if (!buf->bo)
         return nvc0_buffer_allocate(screen, buf, NOUVEAU_BO_GART);
   } else
   if (domain == NOUVEAU_BO_GART) {
      buf->mm = nvc0_mm_allocate(screen->mm_GART, buf->base.width0, &buf->bo,
                                 &buf->offset);
      if (!buf->bo)
         return FALSE;
   } else {
      assert(!domain);
      if (!buf->data)
         buf->data = MALLOC(buf->base.width0);
      if (!buf->data)
         return FALSE;
   }
   buf->domain = domain;
   return TRUE;
}

static INLINE void
release_allocation(struct nvc0_mm_allocation **mm, struct nvc0_fence *fence)
{
   (*mm)->next = fence->buffers;
   fence->buffers = (*mm);
   (*mm) = NULL;
}

static void
nvc0_buffer_destroy(struct pipe_screen *pscreen,
                    struct pipe_resource *presource)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nvc0_resource *res = nvc0_resource(presource);

   nouveau_bo_ref(NULL, &res->bo);

   if (res->mm)
      release_allocation(&res->mm, screen->fence.current);

   if (res->status != NVC0_BUFFER_STATUS_USER_MEMORY && res->data)
      FREE(res->data);

   FREE(res);
}

static INLINE uint32_t
nouveau_buffer_rw_flags(unsigned pipe)
{
   uint32_t flags = 0;

   if (pipe & PIPE_TRANSFER_READ)
      flags = NOUVEAU_BO_RD;
   if (pipe & PIPE_TRANSFER_WRITE)
      flags |= NOUVEAU_BO_WR;

   return flags;
}

static void *
nvc0_buffer_transfer_map(struct pipe_context *pipe,
                         struct pipe_transfer *transfer)
{
   struct nvc0_resource *res = nvc0_resource(transfer->resource);
   struct nvc0_fence *fence;
   uint8_t *map;
   int ret;
   uint32_t flags = nouveau_buffer_rw_flags(transfer->usage);

   if ((res->base.bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER)) &&
       (flags & NOUVEAU_BO_WR))
      nvc0_context(pipe)->vbo_dirty = TRUE;

   if (res->domain == 0)
      return res->data + transfer->box.x;

   if (res->domain == NOUVEAU_BO_VRAM) {
      NOUVEAU_ERR("transfers to/from VRAM buffers are not allowed\n");
      /* if this happens, migrate back to GART */
      return NULL;
   }

   if (res->score > -1024)
      --res->score;

   ret = nouveau_bo_map(res->bo, flags | NOUVEAU_BO_NOSYNC);
   if (ret)
      return NULL;
   map = res->bo->map;
   nouveau_bo_unmap(res->bo);

   fence = (flags == NOUVEAU_BO_RD) ? res->fence_wr : res->fence;

   if (fence) {
      if (nvc0_fence_wait(fence) == FALSE)
         NOUVEAU_ERR("failed to fence buffer\n");

      nvc0_fence_reference(&res->fence, NULL);
      nvc0_fence_reference(&res->fence_wr, NULL);
   }

   return map + transfer->box.x + res->offset;
}



static void
nvc0_buffer_transfer_flush_region(struct pipe_context *pipe,
                                  struct pipe_transfer *transfer,
                                  const struct pipe_box *box)
{
   struct nvc0_resource *res = nvc0_resource(transfer->resource);

   if (!res->bo)
      return;

   nouveau_screen_bo_map_flush_range(pipe->screen,
                                     res->bo,
                                     res->offset + transfer->box.x + box->x,
                                     box->width);
}

static void
nvc0_buffer_transfer_unmap(struct pipe_context *pipe,
                           struct pipe_transfer *transfer)
{
   struct nvc0_resource *res = nvc0_resource(transfer->resource);

   if (res->data)
      return;

   /* nouveau_screen_bo_unmap(pipe->screen, res->bo); */
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
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nvc0_resource *buffer;
   boolean ret;

   buffer = CALLOC_STRUCT(nvc0_resource);
   if (!buffer)
      return NULL;

   buffer->base = *templ;
   buffer->vtbl = &nvc0_buffer_vtbl;
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = pscreen;

   if (buffer->base.bind & PIPE_BIND_CONSTANT_BUFFER)
      ret = nvc0_buffer_allocate(screen, buffer, 0);
   else
      ret = nvc0_buffer_allocate(screen, buffer, NOUVEAU_BO_GART);

   if (ret == FALSE)
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

   buffer->data = ptr;
   buffer->status = NVC0_BUFFER_STATUS_USER_MEMORY;

   return &buffer->base;
}

/* Migrate a linear buffer (vertex, index, constants) USER -> GART -> VRAM. */
boolean
nvc0_buffer_migrate(struct nvc0_context *nvc0,
                    struct nvc0_resource *buf, unsigned domain)
{
   struct nvc0_screen *screen = nvc0_screen(buf->base.screen);
   struct nouveau_bo *bo;
   unsigned size = buf->base.width0;
   int ret;

   if (domain == NOUVEAU_BO_GART && buf->domain == 0) {
      if (!nvc0_buffer_allocate(screen, buf, domain))
         return FALSE;
      ret = nouveau_bo_map(buf->bo, NOUVEAU_BO_WR | NOUVEAU_BO_NOSYNC);
      if (ret)
         return ret;
      memcpy((uint8_t *)buf->bo->map + buf->offset, buf->data, size);
      nouveau_bo_unmap(buf->bo);
   } else
   if (domain == NOUVEAU_BO_VRAM && buf->domain == NOUVEAU_BO_GART) {
      struct nvc0_mm_allocation *mm = buf->mm;

      bo = buf->bo;
      buf->bo = NULL;
      buf->mm = NULL;
      nvc0_buffer_allocate(screen, buf, domain);

      nvc0_m2mf_copy_linear(nvc0, buf->bo, 0, NOUVEAU_BO_VRAM,
                            bo, 0, NOUVEAU_BO_GART, buf->base.width0);

      release_allocation(&mm, screen->fence.current);
      nouveau_bo_ref(NULL, &bo);
   } else
   if (domain == NOUVEAU_BO_VRAM && buf->domain == 0) {
      /* should use a scratch buffer instead here */
      if (!nvc0_buffer_migrate(nvc0, buf, NOUVEAU_BO_GART))
         return FALSE;
      return nvc0_buffer_migrate(nvc0, buf, NOUVEAU_BO_VRAM);
   } else
      return -1;

   buf->domain = domain;

   return TRUE;
}

/* Migrate data from glVertexAttribPointer(non-VBO) user buffers to GART.
 * MUST NOT FLUSH THE PUSH BUFFER, we could be in the middle of a method.
 */
boolean
nvc0_migrate_vertices(struct nvc0_resource *buf, unsigned base, unsigned size)
{
   struct nvc0_screen *screen = nvc0_screen(buf->base.screen);
   int ret;

   assert(buf->data && !buf->domain);

   if (!nvc0_buffer_allocate(screen, buf, NOUVEAU_BO_GART))
      return FALSE;
   ret = nouveau_bo_map_range(buf->bo, base + buf->offset, size,
                              NOUVEAU_BO_WR | NOUVEAU_BO_NOSYNC);
   if (ret)
      return FALSE;
   memcpy(buf->bo->map, buf->data + base, size);
   nouveau_bo_unmap(buf->bo);

   return TRUE;
}
