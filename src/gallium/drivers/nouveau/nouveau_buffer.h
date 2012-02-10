#ifndef __NOUVEAU_BUFFER_H__
#define __NOUVEAU_BUFFER_H__

#include "util/u_transfer.h"
#include "util/u_double_list.h"

struct pipe_resource;
struct nouveau_context;
struct nouveau_bo;

/* DIRTY: buffer was (or will be after the next flush) written to by GPU and
 *  resource->data has not been updated to reflect modified VRAM contents
 *
 * USER_MEMORY: resource->data is a pointer to client memory and may change
 *  between GL calls
 */
#define NOUVEAU_BUFFER_STATUS_GPU_READING (1 << 0)
#define NOUVEAU_BUFFER_STATUS_GPU_WRITING (1 << 1)
#define NOUVEAU_BUFFER_STATUS_USER_MEMORY (1 << 7)

/* Resources, if mapped into the GPU's address space, are guaranteed to
 * have constant virtual addresses (nv50+).
 *
 * The address of a resource will lie within the nouveau_bo referenced,
 * and this bo should be added to the memory manager's validation list.
 */
struct nv04_resource {
   struct pipe_resource base;
   const struct u_resource_vtbl *vtbl;

   uint8_t *data;
   struct nouveau_bo *bo;
   uint32_t offset;

   uint8_t status;
   uint8_t domain;

   struct nouveau_fence *fence;
   struct nouveau_fence *fence_wr;

   struct nouveau_mm_allocation *mm;
};

void
nouveau_buffer_release_gpu_storage(struct nv04_resource *);

boolean
nouveau_buffer_download(struct nouveau_context *, struct nv04_resource *,
                        unsigned start, unsigned size);

boolean
nouveau_buffer_migrate(struct nouveau_context *,
                       struct nv04_resource *, unsigned domain);

/* XXX: wait for fence (atm only using this for vertex push) */
static INLINE void *
nouveau_resource_map_offset(struct nouveau_context *pipe,
                            struct nv04_resource *res, uint32_t offset,
                            uint32_t flags)
{
   void *map;

   if ((res->domain == NOUVEAU_BO_VRAM) &&
       (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING))
      nouveau_buffer_download(pipe, res, 0, res->base.width0);

   if ((res->domain != NOUVEAU_BO_GART) ||
       (res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY))
      return res->data + offset;

   if (res->mm)
      flags |= NOUVEAU_BO_NOSYNC;

   if (nouveau_bo_map_range(res->bo, res->offset + offset,
                            res->base.width0, flags))
      return NULL;

   map = res->bo->map;
   nouveau_bo_unmap(res->bo);
   return map;
}

static INLINE void
nouveau_resource_unmap(struct nv04_resource *res)
{
   /* no-op */
}

static INLINE struct nv04_resource *
nv04_resource(struct pipe_resource *resource)
{
   return (struct nv04_resource *)resource;
}

/* is resource mapped into the GPU's address space (i.e. VRAM or GART) ? */
static INLINE boolean
nouveau_resource_mapped_by_gpu(struct pipe_resource *resource)
{
   return nv04_resource(resource)->domain != 0;
}

struct pipe_resource *
nouveau_buffer_create(struct pipe_screen *pscreen,
                      const struct pipe_resource *templ);

struct pipe_resource *
nouveau_user_buffer_create(struct pipe_screen *screen, void *ptr,
                           unsigned bytes, unsigned usage);

boolean
nouveau_user_buffer_upload(struct nv04_resource *, unsigned base,
                           unsigned size);

#endif
