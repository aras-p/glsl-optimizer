
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#define NOUVEAU_NVC0
#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_mm.h"
#undef NOUVEAU_NVC0

#include "nv50_context.h"
#include "nv50_resource.h"

struct nv50_transfer {
   struct pipe_transfer base;
};

static INLINE struct nv50_transfer *
nv50_transfer(struct pipe_transfer *transfer)
{
   return (struct nv50_transfer *)transfer;
}

static INLINE boolean
nv50_buffer_allocate(struct nv50_screen *screen, struct nv50_resource *buf,
                     unsigned domain)
{
   if (domain == NOUVEAU_BO_VRAM) {
      buf->mm = nouveau_mm_allocate(screen->base.mm_VRAM, buf->base.width0,
                                    &buf->bo, &buf->offset);
      if (!buf->bo)
         return nv50_buffer_allocate(screen, buf, NOUVEAU_BO_GART);
   } else
   if (domain == NOUVEAU_BO_GART) {
      buf->mm = nouveau_mm_allocate(screen->base.mm_GART, buf->base.width0,
                                    &buf->bo, &buf->offset);
      if (!buf->bo)
         return FALSE;
   }
   if (domain != NOUVEAU_BO_GART) {
      if (!buf->data) {
         buf->data = MALLOC(buf->base.width0);
         if (!buf->data)
            return FALSE;
      }
   }
   buf->domain = domain;
   return TRUE;
}

static INLINE void
release_allocation(struct nouveau_mm_allocation **mm, struct nouveau_fence *fence)
{
   nouveau_fence_work(fence, nouveau_mm_free_work, *mm);
   (*mm) = NULL;
}

INLINE void
nv50_buffer_release_gpu_storage(struct nv50_resource *buf)
{
   nouveau_bo_ref(NULL, &buf->bo);

   if (buf->mm)
      release_allocation(&buf->mm, buf->fence);

   buf->domain = 0;
}

static INLINE boolean
nv50_buffer_reallocate(struct nv50_screen *screen, struct nv50_resource *buf,
                       unsigned domain)
{
   nv50_buffer_release_gpu_storage(buf);

   return nv50_buffer_allocate(screen, buf, domain);
}

static void
nv50_buffer_destroy(struct pipe_screen *pscreen,
                    struct pipe_resource *presource)
{
   struct nv50_resource *res = nv50_resource(presource);

   nv50_buffer_release_gpu_storage(res);

   if (res->data && !(res->status & NV50_BUFFER_STATUS_USER_MEMORY))
      FREE(res->data);

   FREE(res);
}

/* Maybe just migrate to GART right away if we actually need to do this. */
boolean
nv50_buffer_download(struct nv50_context *nv50, struct nv50_resource *buf,
                     unsigned start, unsigned size)
{
   struct nouveau_mm_allocation *mm;
   struct nouveau_bo *bounce = NULL;
   uint32_t offset;

   assert(buf->domain == NOUVEAU_BO_VRAM);

   mm = nouveau_mm_allocate(nv50->screen->base.mm_GART, size, &bounce, &offset);
   if (!bounce)
      return FALSE;

   nv50_m2mf_copy_linear(nv50, bounce, offset, NOUVEAU_BO_GART,
                         buf->bo, buf->offset + start, NOUVEAU_BO_VRAM,
                         size);

   if (nouveau_bo_map_range(bounce, offset, size, NOUVEAU_BO_RD))
      return FALSE;
   memcpy(buf->data + start, bounce->map, size);
   nouveau_bo_unmap(bounce);

   buf->status &= ~NV50_BUFFER_STATUS_DIRTY;

   nouveau_bo_ref(NULL, &bounce);
   if (mm)
      nouveau_mm_free(mm);
   return TRUE;
}

static boolean
nv50_buffer_upload(struct nv50_context *nv50, struct nv50_resource *buf,
                   unsigned start, unsigned size)
{
   struct nouveau_mm_allocation *mm;
   struct nouveau_bo *bounce = NULL;
   uint32_t offset;

   if (size <= 192) {
      nv50_sifc_linear_u8(nv50, buf->bo, buf->domain, buf->offset + start,
                          size, buf->data + start);
      return TRUE;
   }

   mm = nouveau_mm_allocate(nv50->screen->base.mm_GART, size, &bounce, &offset);
   if (!bounce)
      return FALSE;

   nouveau_bo_map_range(bounce, offset, size,
                        NOUVEAU_BO_WR | NOUVEAU_BO_NOSYNC);
   memcpy(bounce->map, buf->data + start, size);
   nouveau_bo_unmap(bounce);

   nv50_m2mf_copy_linear(nv50, buf->bo, buf->offset + start, NOUVEAU_BO_VRAM,
                         bounce, offset, NOUVEAU_BO_GART, size);

   nouveau_bo_ref(NULL, &bounce);
   if (mm)
      release_allocation(&mm, nv50->screen->base.fence.current);

   if (start == 0 && size == buf->base.width0)
      buf->status &= ~NV50_BUFFER_STATUS_DIRTY;
   return TRUE;
}

static struct pipe_transfer *
nv50_buffer_transfer_get(struct pipe_context *pipe,
                         struct pipe_resource *resource,
                         unsigned level,
                         unsigned usage,
                         const struct pipe_box *box)
{
   struct nv50_resource *buf = nv50_resource(resource);
   struct nv50_transfer *xfr = CALLOC_STRUCT(nv50_transfer);
   if (!xfr)
      return NULL;

   xfr->base.resource = resource;
   xfr->base.box.x = box->x;
   xfr->base.box.width = box->width;
   xfr->base.usage = usage;

   if (buf->domain == NOUVEAU_BO_VRAM) {
      if (usage & PIPE_TRANSFER_READ) {
         if (buf->status & NV50_BUFFER_STATUS_DIRTY)
            nv50_buffer_download(nv50_context(pipe), buf, 0, buf->base.width0);
      }
   }

   return &xfr->base;
}

static void
nv50_buffer_transfer_destroy(struct pipe_context *pipe,
                             struct pipe_transfer *transfer)
{
   struct nv50_resource *buf = nv50_resource(transfer->resource);
   struct nv50_transfer *xfr = nv50_transfer(transfer);

   if (xfr->base.usage & PIPE_TRANSFER_WRITE) {
      /* writing is worse */
      nv50_buffer_adjust_score(nv50_context(pipe), buf, -5000);

      if (buf->domain == NOUVEAU_BO_VRAM) {
         nv50_buffer_upload(nv50_context(pipe), buf,
                            transfer->box.x, transfer->box.width);
      }

      if (buf->domain != 0 && (buf->base.bind & (PIPE_BIND_VERTEX_BUFFER |
                                                 PIPE_BIND_INDEX_BUFFER)))
         nv50_context(pipe)->vbo_dirty = TRUE;
   }

   FREE(xfr);
}

static INLINE boolean
nv50_buffer_sync(struct nv50_resource *buf, unsigned rw)
{
   if (rw == PIPE_TRANSFER_READ) {
      if (!buf->fence_wr)
         return TRUE;
      if (!nouveau_fence_wait(buf->fence_wr))
         return FALSE;
   } else {
      if (!buf->fence)
         return TRUE;
      if (!nouveau_fence_wait(buf->fence))
         return FALSE;

      nouveau_fence_ref(NULL, &buf->fence);
   }
   nouveau_fence_ref(NULL, &buf->fence_wr);

   return TRUE;
}

static INLINE boolean
nv50_buffer_busy(struct nv50_resource *buf, unsigned rw)
{
   if (rw == PIPE_TRANSFER_READ)
      return (buf->fence_wr && !nouveau_fence_signalled(buf->fence_wr));
   else
      return (buf->fence && !nouveau_fence_signalled(buf->fence));
}

static void *
nv50_buffer_transfer_map(struct pipe_context *pipe,
                         struct pipe_transfer *transfer)
{
   struct nv50_transfer *xfr = nv50_transfer(transfer);
   struct nv50_resource *buf = nv50_resource(transfer->resource);
   struct nouveau_bo *bo = buf->bo;
   uint8_t *map;
   int ret;
   uint32_t offset = xfr->base.box.x;
   uint32_t flags;

   nv50_buffer_adjust_score(nv50_context(pipe), buf, -250);

   if (buf->domain != NOUVEAU_BO_GART)
      return buf->data + offset;

   if (buf->mm)
      flags = NOUVEAU_BO_NOSYNC | NOUVEAU_BO_RDWR;
   else
      flags = nouveau_screen_transfer_flags(xfr->base.usage);

   offset += buf->offset;

   ret = nouveau_bo_map_range(buf->bo, offset, xfr->base.box.width, flags);
   if (ret)
      return NULL;
   map = bo->map;

   /* Unmap right now. Since multiple buffers can share a single nouveau_bo,
    * not doing so might make future maps fail or trigger "reloc while mapped"
    * errors. For now, mappings to userspace are guaranteed to be persistent.
    */
   nouveau_bo_unmap(bo);

   if (buf->mm) {
      if (xfr->base.usage & PIPE_TRANSFER_DONTBLOCK) {
         if (nv50_buffer_busy(buf, xfr->base.usage & PIPE_TRANSFER_READ_WRITE))
            return NULL;
      } else
      if (!(xfr->base.usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
         nv50_buffer_sync(buf, xfr->base.usage & PIPE_TRANSFER_READ_WRITE);
      }
   }
   return map;
}



static void
nv50_buffer_transfer_flush_region(struct pipe_context *pipe,
                                  struct pipe_transfer *transfer,
                                  const struct pipe_box *box)
{
   struct nv50_resource *res = nv50_resource(transfer->resource);
   struct nouveau_bo *bo = res->bo;
   unsigned offset = res->offset + transfer->box.x + box->x;

   /* not using non-snoop system memory yet, no need for cflush */
   if (1)
      return;

   /* XXX: maybe need to upload for VRAM buffers here */

   nouveau_screen_bo_map_flush_range(pipe->screen, bo, offset, box->width);
}

static void
nv50_buffer_transfer_unmap(struct pipe_context *pipe,
                           struct pipe_transfer *transfer)
{
   /* we've called nouveau_bo_unmap right after map */
}

const struct u_resource_vtbl nv50_buffer_vtbl =
{
   u_default_resource_get_handle,     /* get_handle */
   nv50_buffer_destroy,               /* resource_destroy */
   NULL,                              /* is_resource_referenced */
   nv50_buffer_transfer_get,          /* get_transfer */
   nv50_buffer_transfer_destroy,      /* transfer_destroy */
   nv50_buffer_transfer_map,          /* transfer_map */
   nv50_buffer_transfer_flush_region, /* transfer_flush_region */
   nv50_buffer_transfer_unmap,        /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};

struct pipe_resource *
nv50_buffer_create(struct pipe_screen *pscreen,
                   const struct pipe_resource *templ)
{
   struct nv50_screen *screen = nv50_screen(pscreen);
   struct nv50_resource *buffer;
   boolean ret;

   buffer = CALLOC_STRUCT(nv50_resource);
   if (!buffer)
      return NULL;

   buffer->base = *templ;
   buffer->vtbl = &nv50_buffer_vtbl;
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = pscreen;

   if (buffer->base.bind & PIPE_BIND_CONSTANT_BUFFER)
      ret = nv50_buffer_allocate(screen, buffer, 0);
   else
      ret = nv50_buffer_allocate(screen, buffer, NOUVEAU_BO_GART);

   if (ret == FALSE)
      goto fail;

   return &buffer->base;

fail:
   FREE(buffer);
   return NULL;
}


struct pipe_resource *
nv50_user_buffer_create(struct pipe_screen *pscreen,
                        void *ptr,
                        unsigned bytes,
                        unsigned bind)
{
   struct nv50_resource *buffer;

   buffer = CALLOC_STRUCT(nv50_resource);
   if (!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->vtbl = &nv50_buffer_vtbl;
   buffer->base.screen = pscreen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.bind = bind;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;

   buffer->data = ptr;
   buffer->status = NV50_BUFFER_STATUS_USER_MEMORY;

   return &buffer->base;
}

/* Like download, but for GART buffers. Merge ? */
static INLINE boolean
nv50_buffer_data_fetch(struct nv50_resource *buf,
                       struct nouveau_bo *bo, unsigned offset, unsigned size)
{
   if (!buf->data) {
      buf->data = MALLOC(size);
      if (!buf->data)
         return FALSE;
   }
   if (nouveau_bo_map_range(bo, offset, size, NOUVEAU_BO_RD))
      return FALSE;
   memcpy(buf->data, bo->map, size);
   nouveau_bo_unmap(bo);

   return TRUE;
}

/* Migrate a linear buffer (vertex, index, constants) USER -> GART -> VRAM. */
boolean
nv50_buffer_migrate(struct nv50_context *nv50,
                    struct nv50_resource *buf, const unsigned new_domain)
{
   struct nv50_screen *screen = nv50_screen(buf->base.screen);
   struct nouveau_bo *bo;
   const unsigned old_domain = buf->domain;
   unsigned size = buf->base.width0;
   unsigned offset;
   int ret;

   assert(new_domain != old_domain);

   if (new_domain == NOUVEAU_BO_GART && old_domain == 0) {
      if (!nv50_buffer_allocate(screen, buf, new_domain))
         return FALSE;
      ret = nouveau_bo_map_range(buf->bo, buf->offset, size, NOUVEAU_BO_WR |
                                 NOUVEAU_BO_NOSYNC);
      if (ret)
         return ret;
      memcpy(buf->bo->map, buf->data, size);
      nouveau_bo_unmap(buf->bo);
      FREE(buf->data);
   } else
   if (old_domain != 0 && new_domain != 0) {
      struct nouveau_mm_allocation *mm = buf->mm;

      if (new_domain == NOUVEAU_BO_VRAM) {
         /* keep a system memory copy of our data in case we hit a fallback */
         if (!nv50_buffer_data_fetch(buf, buf->bo, buf->offset, size))
            return FALSE;
         debug_printf("migrating %u KiB to VRAM\n", size / 1024);
      }

      offset = buf->offset;
      bo = buf->bo;
      buf->bo = NULL;
      buf->mm = NULL;
      nv50_buffer_allocate(screen, buf, new_domain);

      nv50_m2mf_copy_linear(nv50, buf->bo, buf->offset, new_domain,
                            bo, offset, old_domain, buf->base.width0);

      nouveau_bo_ref(NULL, &bo);
      if (mm)
         release_allocation(&mm, screen->base.fence.current);
   } else
   if (new_domain == NOUVEAU_BO_VRAM && old_domain == 0) {
      if (!nv50_buffer_allocate(screen, buf, NOUVEAU_BO_VRAM))
         return FALSE;
      if (!nv50_buffer_upload(nv50, buf, 0, buf->base.width0))
         return FALSE;
   } else
      return FALSE;

   assert(buf->domain == new_domain);
   return TRUE;
}

/* Migrate data from glVertexAttribPointer(non-VBO) user buffers to GART.
 * We'd like to only allocate @size bytes here, but then we'd have to rebase
 * the vertex indices ...
 */
boolean
nv50_user_buffer_upload(struct nv50_resource *buf, unsigned base, unsigned size)
{
   struct nv50_screen *screen = nv50_screen(buf->base.screen);
   int ret;

   assert(buf->status & NV50_BUFFER_STATUS_USER_MEMORY);

   buf->base.width0 = base + size;
   if (!nv50_buffer_reallocate(screen, buf, NOUVEAU_BO_GART))
      return FALSE;

   ret = nouveau_bo_map_range(buf->bo, buf->offset + base, size,
                              NOUVEAU_BO_WR | NOUVEAU_BO_NOSYNC);
   if (ret)
      return FALSE;
   memcpy(buf->bo->map, buf->data + base, size);
   nouveau_bo_unmap(buf->bo);

   return TRUE;
}
