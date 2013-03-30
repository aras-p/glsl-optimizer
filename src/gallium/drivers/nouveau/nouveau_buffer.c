
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_surface.h"

#include "nouveau_screen.h"
#include "nouveau_context.h"
#include "nouveau_winsys.h"
#include "nouveau_fence.h"
#include "nouveau_buffer.h"
#include "nouveau_mm.h"

#define NOUVEAU_TRANSFER_PUSHBUF_THRESHOLD 192

struct nouveau_transfer {
   struct pipe_transfer base;

   uint8_t *map;
   struct nouveau_bo *bo;
   struct nouveau_mm_allocation *mm;
   uint32_t offset;
};

static INLINE struct nouveau_transfer *
nouveau_transfer(struct pipe_transfer *transfer)
{
   return (struct nouveau_transfer *)transfer;
}

static INLINE boolean
nouveau_buffer_malloc(struct nv04_resource *buf)
{
   if (!buf->data)
      buf->data = align_malloc(buf->base.width0, NOUVEAU_MIN_BUFFER_MAP_ALIGN);
   return !!buf->data;
}

static INLINE boolean
nouveau_buffer_allocate(struct nouveau_screen *screen,
                        struct nv04_resource *buf, unsigned domain)
{
   uint32_t size = buf->base.width0;

   if (buf->base.bind & (PIPE_BIND_CONSTANT_BUFFER |
                         PIPE_BIND_COMPUTE_RESOURCE |
                         PIPE_BIND_SHADER_RESOURCE))
      size = align(size, 0x100);

   if (domain == NOUVEAU_BO_VRAM) {
      buf->mm = nouveau_mm_allocate(screen->mm_VRAM, size,
                                    &buf->bo, &buf->offset);
      if (!buf->bo)
         return nouveau_buffer_allocate(screen, buf, NOUVEAU_BO_GART);
      NOUVEAU_DRV_STAT(screen, buf_obj_current_bytes_vid, buf->base.width0);
   } else
   if (domain == NOUVEAU_BO_GART) {
      buf->mm = nouveau_mm_allocate(screen->mm_GART, size,
                                    &buf->bo, &buf->offset);
      if (!buf->bo)
         return FALSE;
      NOUVEAU_DRV_STAT(screen, buf_obj_current_bytes_sys, buf->base.width0);
   } else {
      assert(domain == 0);
      if (!nouveau_buffer_malloc(buf))
         return FALSE;
   }
   buf->domain = domain;
   if (buf->bo)
      buf->address = buf->bo->offset + buf->offset;

   return TRUE;
}

static INLINE void
release_allocation(struct nouveau_mm_allocation **mm,
                   struct nouveau_fence *fence)
{
   nouveau_fence_work(fence, nouveau_mm_free_work, *mm);
   (*mm) = NULL;
}

INLINE void
nouveau_buffer_release_gpu_storage(struct nv04_resource *buf)
{
   nouveau_bo_ref(NULL, &buf->bo);

   if (buf->mm)
      release_allocation(&buf->mm, buf->fence);

   if (buf->domain == NOUVEAU_BO_VRAM)
      NOUVEAU_DRV_STAT_RES(buf, buf_obj_current_bytes_vid, -(uint64_t)buf->base.width0);
   if (buf->domain == NOUVEAU_BO_GART)
      NOUVEAU_DRV_STAT_RES(buf, buf_obj_current_bytes_sys, -(uint64_t)buf->base.width0);

   buf->domain = 0;
}

static INLINE boolean
nouveau_buffer_reallocate(struct nouveau_screen *screen,
                          struct nv04_resource *buf, unsigned domain)
{
   nouveau_buffer_release_gpu_storage(buf);

   nouveau_fence_ref(NULL, &buf->fence);
   nouveau_fence_ref(NULL, &buf->fence_wr);

   buf->status &= NOUVEAU_BUFFER_STATUS_REALLOC_MASK;

   return nouveau_buffer_allocate(screen, buf, domain);
}

static void
nouveau_buffer_destroy(struct pipe_screen *pscreen,
                       struct pipe_resource *presource)
{
   struct nv04_resource *res = nv04_resource(presource);

   nouveau_buffer_release_gpu_storage(res);

   if (res->data && !(res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY))
      align_free(res->data);

   nouveau_fence_ref(NULL, &res->fence);
   nouveau_fence_ref(NULL, &res->fence_wr);

   FREE(res);

   NOUVEAU_DRV_STAT(nouveau_screen(pscreen), buf_obj_current_count, -1);
}

static uint8_t *
nouveau_transfer_staging(struct nouveau_context *nv,
                         struct nouveau_transfer *tx, boolean permit_pb)
{
   const unsigned adj = tx->base.box.x & NOUVEAU_MIN_BUFFER_MAP_ALIGN_MASK;
   const unsigned size = align(tx->base.box.width, 4) + adj;

   if (!nv->push_data)
      permit_pb = FALSE;

   if ((size <= NOUVEAU_TRANSFER_PUSHBUF_THRESHOLD) && permit_pb) {
      tx->map = align_malloc(size, NOUVEAU_MIN_BUFFER_MAP_ALIGN);
      if (tx->map)
         tx->map += adj;
   } else {
      tx->mm =
         nouveau_mm_allocate(nv->screen->mm_GART, size, &tx->bo, &tx->offset);
      if (tx->bo) {
         tx->offset += adj;
         if (!nouveau_bo_map(tx->bo, 0, NULL))
            tx->map = (uint8_t *)tx->bo->map + tx->offset;
      }
   }
   return tx->map;
}

/* Maybe just migrate to GART right away if we actually need to do this. */
static boolean
nouveau_transfer_read(struct nouveau_context *nv, struct nouveau_transfer *tx)
{
   struct nv04_resource *buf = nv04_resource(tx->base.resource);
   const unsigned base = tx->base.box.x;
   const unsigned size = tx->base.box.width;

   NOUVEAU_DRV_STAT(nv->screen, buf_read_bytes_staging_vid, size);

   nv->copy_data(nv, tx->bo, tx->offset, NOUVEAU_BO_GART,
                 buf->bo, buf->offset + base, buf->domain, size);

   if (nouveau_bo_wait(tx->bo, NOUVEAU_BO_RD, nv->client))
      return FALSE;

   if (buf->data)
      memcpy(buf->data + base, tx->map, size);

   return TRUE;
}

static void
nouveau_transfer_write(struct nouveau_context *nv, struct nouveau_transfer *tx,
                       unsigned offset, unsigned size)
{
   struct nv04_resource *buf = nv04_resource(tx->base.resource);
   uint8_t *data = tx->map + offset;
   const unsigned base = tx->base.box.x + offset;
   const boolean can_cb = !((base | size) & 3);

   if (buf->data)
      memcpy(data, buf->data + base, size);
   else
      buf->status |= NOUVEAU_BUFFER_STATUS_DIRTY;

   if (buf->domain == NOUVEAU_BO_VRAM)
      NOUVEAU_DRV_STAT(nv->screen, buf_write_bytes_staging_vid, size);
   if (buf->domain == NOUVEAU_BO_GART)
      NOUVEAU_DRV_STAT(nv->screen, buf_write_bytes_staging_sys, size);

   if (tx->bo)
      nv->copy_data(nv, buf->bo, buf->offset + base, buf->domain,
                    tx->bo, tx->offset + offset, NOUVEAU_BO_GART, size);
   else
   if ((buf->base.bind & PIPE_BIND_CONSTANT_BUFFER) && nv->push_cb && can_cb)
      nv->push_cb(nv, buf->bo, buf->domain, buf->offset, buf->base.width0,
                  base, size / 4, (const uint32_t *)data);
   else
      nv->push_data(nv, buf->bo, buf->offset + base, buf->domain, size, data);
}


static INLINE boolean
nouveau_buffer_sync(struct nv04_resource *buf, unsigned rw)
{
   if (rw == PIPE_TRANSFER_READ) {
      if (!buf->fence_wr)
         return TRUE;
      NOUVEAU_DRV_STAT_RES(buf, buf_non_kernel_fence_sync_count,
                           !nouveau_fence_signalled(buf->fence_wr));
      if (!nouveau_fence_wait(buf->fence_wr))
         return FALSE;
   } else {
      if (!buf->fence)
         return TRUE;
      NOUVEAU_DRV_STAT_RES(buf, buf_non_kernel_fence_sync_count,
                           !nouveau_fence_signalled(buf->fence));
      if (!nouveau_fence_wait(buf->fence))
         return FALSE;

      nouveau_fence_ref(NULL, &buf->fence);
   }
   nouveau_fence_ref(NULL, &buf->fence_wr);

   return TRUE;
}

static INLINE boolean
nouveau_buffer_busy(struct nv04_resource *buf, unsigned rw)
{
   if (rw == PIPE_TRANSFER_READ)
      return (buf->fence_wr && !nouveau_fence_signalled(buf->fence_wr));
   else
      return (buf->fence && !nouveau_fence_signalled(buf->fence));
}

static INLINE void
nouveau_buffer_transfer_init(struct nouveau_transfer *tx,
                             struct pipe_resource *resource,
                             const struct pipe_box *box,
                             unsigned usage)
{
   tx->base.resource = resource;
   tx->base.level = 0;
   tx->base.usage = usage;
   tx->base.box.x = box->x;
   tx->base.box.y = 0;
   tx->base.box.z = 0;
   tx->base.box.width = box->width;
   tx->base.box.height = 1;
   tx->base.box.depth = 1;
   tx->base.stride = 0;
   tx->base.layer_stride = 0;

   tx->bo = NULL;
   tx->map = NULL;
}

static INLINE void
nouveau_buffer_transfer_del(struct nouveau_context *nv,
                            struct nouveau_transfer *tx)
{
   if (tx->map) {
      if (likely(tx->bo)) {
         nouveau_bo_ref(NULL, &tx->bo);
         if (tx->mm)
            release_allocation(&tx->mm, nv->screen->fence.current);
      } else {
         align_free(tx->map -
                    (tx->base.box.x & NOUVEAU_MIN_BUFFER_MAP_ALIGN_MASK));
      }
   }
}

static boolean
nouveau_buffer_cache(struct nouveau_context *nv, struct nv04_resource *buf)
{
   struct nouveau_transfer tx;
   boolean ret;
   tx.base.resource = &buf->base;
   tx.base.box.x = 0;
   tx.base.box.width = buf->base.width0;
   tx.bo = NULL;

   if (!buf->data)
      if (!nouveau_buffer_malloc(buf))
         return FALSE;
   if (!(buf->status & NOUVEAU_BUFFER_STATUS_DIRTY))
      return TRUE;
   nv->stats.buf_cache_count++;

   if (!nouveau_transfer_staging(nv, &tx, FALSE))
      return FALSE;

   ret = nouveau_transfer_read(nv, &tx);
   if (ret) {
      buf->status &= ~NOUVEAU_BUFFER_STATUS_DIRTY;
      memcpy(buf->data, tx.map, buf->base.width0);
   }
   nouveau_buffer_transfer_del(nv, &tx);
   return ret;
}


#define NOUVEAU_TRANSFER_DISCARD \
   (PIPE_TRANSFER_DISCARD_RANGE | PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE)

static INLINE boolean
nouveau_buffer_should_discard(struct nv04_resource *buf, unsigned usage)
{
   if (!(usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE))
      return FALSE;
   if (unlikely(buf->base.bind & PIPE_BIND_SHARED))
      return FALSE;
   return buf->mm && nouveau_buffer_busy(buf, PIPE_TRANSFER_WRITE);
}

static void *
nouveau_buffer_transfer_map(struct pipe_context *pipe,
                            struct pipe_resource *resource,
                            unsigned level, unsigned usage,
                            const struct pipe_box *box,
                            struct pipe_transfer **ptransfer)
{
   struct nouveau_context *nv = nouveau_context(pipe);
   struct nv04_resource *buf = nv04_resource(resource);
   struct nouveau_transfer *tx = MALLOC_STRUCT(nouveau_transfer);
   uint8_t *map;
   int ret;

   if (!tx)
      return NULL;
   nouveau_buffer_transfer_init(tx, resource, box, usage);
   *ptransfer = &tx->base;

   if (usage & PIPE_TRANSFER_READ)
      NOUVEAU_DRV_STAT(nv->screen, buf_transfers_rd, 1);
   if (usage & PIPE_TRANSFER_WRITE)
      NOUVEAU_DRV_STAT(nv->screen, buf_transfers_wr, 1);

   if (buf->domain == NOUVEAU_BO_VRAM) {
      if (usage & NOUVEAU_TRANSFER_DISCARD) {
         if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE)
            buf->status &= NOUVEAU_BUFFER_STATUS_REALLOC_MASK;
         nouveau_transfer_staging(nv, tx, TRUE);
      } else {
         if (buf->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
            if (buf->data) {
               align_free(buf->data);
               buf->data = NULL;
            }
            nouveau_transfer_staging(nv, tx, FALSE);
            nouveau_transfer_read(nv, tx);
         } else {
            if (usage & PIPE_TRANSFER_WRITE)
               nouveau_transfer_staging(nv, tx, TRUE);
            if (!buf->data)
               nouveau_buffer_cache(nv, buf);
         }
      }
      return buf->data ? (buf->data + box->x) : tx->map;
   } else
   if (unlikely(buf->domain == 0)) {
      return buf->data + box->x;
   }

   if (nouveau_buffer_should_discard(buf, usage)) {
      int ref = buf->base.reference.count - 1;
      nouveau_buffer_reallocate(nv->screen, buf, buf->domain);
      if (ref > 0) /* any references inside context possible ? */
         nv->invalidate_resource_storage(nv, &buf->base, ref);
   }

   ret = nouveau_bo_map(buf->bo,
                        buf->mm ? 0 : nouveau_screen_transfer_flags(usage),
                        nv->client);
   if (ret) {
      FREE(tx);
      return NULL;
   }
   map = (uint8_t *)buf->bo->map + buf->offset + box->x;

   /* using kernel fences only if !buf->mm */
   if ((usage & PIPE_TRANSFER_UNSYNCHRONIZED) || !buf->mm)
      return map;

   if (nouveau_buffer_busy(buf, usage & PIPE_TRANSFER_READ_WRITE)) {
      if (unlikely(usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE)) {
         /* Discarding was not possible, must sync because
          * subsequent transfers might use UNSYNCHRONIZED. */
         nouveau_buffer_sync(buf, usage & PIPE_TRANSFER_READ_WRITE);
      } else
      if (usage & PIPE_TRANSFER_DISCARD_RANGE) {
         nouveau_transfer_staging(nv, tx, TRUE);
         map = tx->map;
      } else
      if (nouveau_buffer_busy(buf, PIPE_TRANSFER_READ)) {
         if (usage & PIPE_TRANSFER_DONTBLOCK)
            map = NULL;
         else
            nouveau_buffer_sync(buf, usage & PIPE_TRANSFER_READ_WRITE);
      } else {
         nouveau_transfer_staging(nv, tx, TRUE);
         if (tx->map)
            memcpy(tx->map, map, box->width);
         map = tx->map;
      }
   }
   if (!map)
      FREE(tx);
   return map;
}



static void
nouveau_buffer_transfer_flush_region(struct pipe_context *pipe,
                                     struct pipe_transfer *transfer,
                                     const struct pipe_box *box)
{
   struct nouveau_transfer *tx = nouveau_transfer(transfer);
   if (tx->map)
      nouveau_transfer_write(nouveau_context(pipe), tx, box->x, box->width);
}

static void
nouveau_buffer_transfer_unmap(struct pipe_context *pipe,
                              struct pipe_transfer *transfer)
{
   struct nouveau_context *nv = nouveau_context(pipe);
   struct nouveau_transfer *tx = nouveau_transfer(transfer);
   struct nv04_resource *buf = nv04_resource(transfer->resource);

   if (tx->base.usage & PIPE_TRANSFER_WRITE) {
      if (!(tx->base.usage & PIPE_TRANSFER_FLUSH_EXPLICIT) && tx->map)
         nouveau_transfer_write(nv, tx, 0, tx->base.box.width);

      if (likely(buf->domain)) {
         const uint8_t bind = buf->base.bind;
         /* make sure we invalidate dedicated caches */
         if (bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
            nv->vbo_dirty = TRUE;
         if (bind & (PIPE_BIND_CONSTANT_BUFFER))
            nv->cb_dirty = TRUE;
      }
   }

   if (!tx->bo && (tx->base.usage & PIPE_TRANSFER_WRITE))
      NOUVEAU_DRV_STAT(nv->screen, buf_write_bytes_direct, tx->base.box.width);

   nouveau_buffer_transfer_del(nv, tx);
   FREE(tx);
}


void
nouveau_copy_buffer(struct nouveau_context *nv,
                    struct nv04_resource *dst, unsigned dstx,
                    struct nv04_resource *src, unsigned srcx, unsigned size)
{
   assert(dst->base.target == PIPE_BUFFER && src->base.target == PIPE_BUFFER);

   if (likely(dst->domain) && likely(src->domain)) {
      nv->copy_data(nv,
                    dst->bo, dst->offset + dstx, dst->domain,
                    src->bo, src->offset + srcx, src->domain, size);

      dst->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      nouveau_fence_ref(nv->screen->fence.current, &dst->fence);
      nouveau_fence_ref(nv->screen->fence.current, &dst->fence_wr);

      src->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;
      nouveau_fence_ref(nv->screen->fence.current, &src->fence);
   } else {
      struct pipe_box src_box;
      src_box.x = srcx;
      src_box.y = 0;
      src_box.z = 0;
      src_box.width = size;
      src_box.height = 1;
      src_box.depth = 1;
      util_resource_copy_region(&nv->pipe,
                                &dst->base, 0, dstx, 0, 0,
                                &src->base, 0, &src_box);
   }
}


void *
nouveau_resource_map_offset(struct nouveau_context *nv,
                            struct nv04_resource *res, uint32_t offset,
                            uint32_t flags)
{
   if (unlikely(res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY))
      return res->data + offset;

   if (res->domain == NOUVEAU_BO_VRAM) {
      if (!res->data || (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING))
         nouveau_buffer_cache(nv, res);
   }
   if (res->domain != NOUVEAU_BO_GART)
      return res->data + offset;

   if (res->mm) {
      unsigned rw;
      rw = (flags & NOUVEAU_BO_WR) ? PIPE_TRANSFER_WRITE : PIPE_TRANSFER_READ;
      nouveau_buffer_sync(res, rw);
      if (nouveau_bo_map(res->bo, 0, NULL))
         return NULL;
   } else {
      if (nouveau_bo_map(res->bo, flags, nv->client))
         return NULL;
   }
   return (uint8_t *)res->bo->map + res->offset + offset;
}


const struct u_resource_vtbl nouveau_buffer_vtbl =
{
   u_default_resource_get_handle,     /* get_handle */
   nouveau_buffer_destroy,               /* resource_destroy */
   nouveau_buffer_transfer_map,          /* transfer_map */
   nouveau_buffer_transfer_flush_region, /* transfer_flush_region */
   nouveau_buffer_transfer_unmap,        /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};

struct pipe_resource *
nouveau_buffer_create(struct pipe_screen *pscreen,
                      const struct pipe_resource *templ)
{
   struct nouveau_screen *screen = nouveau_screen(pscreen);
   struct nv04_resource *buffer;
   boolean ret;

   buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
      return NULL;

   buffer->base = *templ;
   buffer->vtbl = &nouveau_buffer_vtbl;
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = pscreen;

   if (buffer->base.bind &
       (screen->vidmem_bindings & screen->sysmem_bindings)) {
      switch (buffer->base.usage) {
      case PIPE_USAGE_DEFAULT:
      case PIPE_USAGE_IMMUTABLE:
      case PIPE_USAGE_STATIC:
         buffer->domain = NOUVEAU_BO_VRAM;
         break;
      case PIPE_USAGE_DYNAMIC:
         /* For most apps, we'd have to do staging transfers to avoid sync
          * with this usage, and GART -> GART copies would be suboptimal.
          */
         buffer->domain = NOUVEAU_BO_VRAM;
         break;
      case PIPE_USAGE_STAGING:
      case PIPE_USAGE_STREAM:
         buffer->domain = NOUVEAU_BO_GART;
         break;
      default:
         assert(0);
         break;
      }
   } else {
      if (buffer->base.bind & screen->vidmem_bindings)
         buffer->domain = NOUVEAU_BO_VRAM;
      else
      if (buffer->base.bind & screen->sysmem_bindings)
         buffer->domain = NOUVEAU_BO_GART;
   }
   ret = nouveau_buffer_allocate(screen, buffer, buffer->domain);

   if (ret == FALSE)
      goto fail;

   if (buffer->domain == NOUVEAU_BO_VRAM && screen->hint_buf_keep_sysmem_copy)
      nouveau_buffer_cache(NULL, buffer);

   NOUVEAU_DRV_STAT(screen, buf_obj_current_count, 1);

   return &buffer->base;

fail:
   FREE(buffer);
   return NULL;
}


struct pipe_resource *
nouveau_user_buffer_create(struct pipe_screen *pscreen, void *ptr,
                           unsigned bytes, unsigned bind)
{
   struct nv04_resource *buffer;

   buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->vtbl = &nouveau_buffer_vtbl;
   buffer->base.screen = pscreen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.bind = bind;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;

   buffer->data = ptr;
   buffer->status = NOUVEAU_BUFFER_STATUS_USER_MEMORY;

   return &buffer->base;
}

static INLINE boolean
nouveau_buffer_data_fetch(struct nouveau_context *nv, struct nv04_resource *buf,
                          struct nouveau_bo *bo, unsigned offset, unsigned size)
{
   if (!nouveau_buffer_malloc(buf))
      return FALSE;
   if (nouveau_bo_map(bo, NOUVEAU_BO_RD, nv->client))
      return FALSE;
   memcpy(buf->data, (uint8_t *)bo->map + offset, size);
   return TRUE;
}

/* Migrate a linear buffer (vertex, index, constants) USER -> GART -> VRAM. */
boolean
nouveau_buffer_migrate(struct nouveau_context *nv,
                       struct nv04_resource *buf, const unsigned new_domain)
{
   struct nouveau_screen *screen = nv->screen;
   struct nouveau_bo *bo;
   const unsigned old_domain = buf->domain;
   unsigned size = buf->base.width0;
   unsigned offset;
   int ret;

   assert(new_domain != old_domain);

   if (new_domain == NOUVEAU_BO_GART && old_domain == 0) {
      if (!nouveau_buffer_allocate(screen, buf, new_domain))
         return FALSE;
      ret = nouveau_bo_map(buf->bo, 0, nv->client);
      if (ret)
         return ret;
      memcpy((uint8_t *)buf->bo->map + buf->offset, buf->data, size);
      align_free(buf->data);
   } else
   if (old_domain != 0 && new_domain != 0) {
      struct nouveau_mm_allocation *mm = buf->mm;

      if (new_domain == NOUVEAU_BO_VRAM) {
         /* keep a system memory copy of our data in case we hit a fallback */
         if (!nouveau_buffer_data_fetch(nv, buf, buf->bo, buf->offset, size))
            return FALSE;
         if (nouveau_mesa_debug)
            debug_printf("migrating %u KiB to VRAM\n", size / 1024);
      }

      offset = buf->offset;
      bo = buf->bo;
      buf->bo = NULL;
      buf->mm = NULL;
      nouveau_buffer_allocate(screen, buf, new_domain);

      nv->copy_data(nv, buf->bo, buf->offset, new_domain,
                    bo, offset, old_domain, buf->base.width0);

      nouveau_bo_ref(NULL, &bo);
      if (mm)
         release_allocation(&mm, screen->fence.current);
   } else
   if (new_domain == NOUVEAU_BO_VRAM && old_domain == 0) {
      struct nouveau_transfer tx;
      if (!nouveau_buffer_allocate(screen, buf, NOUVEAU_BO_VRAM))
         return FALSE;
      tx.base.resource = &buf->base;
      tx.base.box.x = 0;
      tx.base.box.width = buf->base.width0;
      tx.bo = NULL;
      if (!nouveau_transfer_staging(nv, &tx, FALSE))
         return FALSE;
      nouveau_transfer_write(nv, &tx, 0, tx.base.box.width);
      nouveau_buffer_transfer_del(nv, &tx);
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
nouveau_user_buffer_upload(struct nouveau_context *nv,
                           struct nv04_resource *buf,
                           unsigned base, unsigned size)
{
   struct nouveau_screen *screen = nouveau_screen(buf->base.screen);
   int ret;

   assert(buf->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY);

   buf->base.width0 = base + size;
   if (!nouveau_buffer_reallocate(screen, buf, NOUVEAU_BO_GART))
      return FALSE;

   ret = nouveau_bo_map(buf->bo, 0, nv->client);
   if (ret)
      return FALSE;
   memcpy((uint8_t *)buf->bo->map + buf->offset + base, buf->data + base, size);

   return TRUE;
}


/* Scratch data allocation. */

static INLINE int
nouveau_scratch_bo_alloc(struct nouveau_context *nv, struct nouveau_bo **pbo,
                         unsigned size)
{
   return nouveau_bo_new(nv->screen->device, NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
                         4096, size, NULL, pbo);
}

void
nouveau_scratch_runout_release(struct nouveau_context *nv)
{
   if (!nv->scratch.nr_runout)
      return;
   do {
      --nv->scratch.nr_runout;
      nouveau_bo_ref(NULL, &nv->scratch.runout[nv->scratch.nr_runout]);
   } while (nv->scratch.nr_runout);

   FREE(nv->scratch.runout);
   nv->scratch.end = 0;
   nv->scratch.runout = NULL;
}

/* Allocate an extra bo if we can't fit everything we need simultaneously.
 * (Could happen for very large user arrays.)
 */
static INLINE boolean
nouveau_scratch_runout(struct nouveau_context *nv, unsigned size)
{
   int ret;
   const unsigned n = nv->scratch.nr_runout++;

   nv->scratch.runout = REALLOC(nv->scratch.runout,
                                (n + 0) * sizeof(*nv->scratch.runout),
                                (n + 1) * sizeof(*nv->scratch.runout));
   nv->scratch.runout[n] = NULL;

   ret = nouveau_scratch_bo_alloc(nv, &nv->scratch.runout[n], size);
   if (!ret) {
      ret = nouveau_bo_map(nv->scratch.runout[n], 0, NULL);
      if (ret)
         nouveau_bo_ref(NULL, &nv->scratch.runout[--nv->scratch.nr_runout]);
   }
   if (!ret) {
      nv->scratch.current = nv->scratch.runout[n];
      nv->scratch.offset = 0;
      nv->scratch.end = size;
      nv->scratch.map = nv->scratch.current->map;
   }
   return !ret;
}

/* Continue to next scratch buffer, if available (no wrapping, large enough).
 * Allocate it if it has not yet been created.
 */
static INLINE boolean
nouveau_scratch_next(struct nouveau_context *nv, unsigned size)
{
   struct nouveau_bo *bo;
   int ret;
   const unsigned i = (nv->scratch.id + 1) % NOUVEAU_MAX_SCRATCH_BUFS;

   if ((size > nv->scratch.bo_size) || (i == nv->scratch.wrap))
      return FALSE;
   nv->scratch.id = i;

   bo = nv->scratch.bo[i];
   if (!bo) {
      ret = nouveau_scratch_bo_alloc(nv, &bo, nv->scratch.bo_size);
      if (ret)
         return FALSE;
      nv->scratch.bo[i] = bo;
   }
   nv->scratch.current = bo;
   nv->scratch.offset = 0;
   nv->scratch.end = nv->scratch.bo_size;

   ret = nouveau_bo_map(bo, NOUVEAU_BO_WR, nv->client);
   if (!ret)
      nv->scratch.map = bo->map;
   return !ret;
}

static boolean
nouveau_scratch_more(struct nouveau_context *nv, unsigned min_size)
{
   boolean ret;

   ret = nouveau_scratch_next(nv, min_size);
   if (!ret)
      ret = nouveau_scratch_runout(nv, min_size);
   return ret;
}


/* Copy data to a scratch buffer and return address & bo the data resides in. */
uint64_t
nouveau_scratch_data(struct nouveau_context *nv,
                     const void *data, unsigned base, unsigned size,
                     struct nouveau_bo **bo)
{
   unsigned bgn = MAX2(base, nv->scratch.offset);
   unsigned end = bgn + size;

   if (end >= nv->scratch.end) {
      end = base + size;
      if (!nouveau_scratch_more(nv, end))
         return 0;
      bgn = base;
   }
   nv->scratch.offset = align(end, 4);

   memcpy(nv->scratch.map + bgn, (const uint8_t *)data + base, size);

   *bo = nv->scratch.current;
   return (*bo)->offset + (bgn - base);
}

void *
nouveau_scratch_get(struct nouveau_context *nv,
                    unsigned size, uint64_t *gpu_addr, struct nouveau_bo **pbo)
{
   unsigned bgn = nv->scratch.offset;
   unsigned end = nv->scratch.offset + size;

   if (end >= nv->scratch.end) {
      end = size;
      if (!nouveau_scratch_more(nv, end))
         return NULL;
      bgn = 0;
   }
   nv->scratch.offset = align(end, 4);

   *pbo = nv->scratch.current;
   *gpu_addr = nv->scratch.current->offset + bgn;
   return nv->scratch.map + bgn;
}
