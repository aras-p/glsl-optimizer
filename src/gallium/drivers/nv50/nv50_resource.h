
#ifndef __NV50_RESOURCE_H__
#define __NV50_RESOURCE_H__

#include "util/u_transfer.h"
#include "util/u_double_list.h"
#define NOUVEAU_NVC0
#include "nouveau/nouveau_winsys.h"
#undef NOUVEAU_NVC0

#include "nv50_fence.h"

struct pipe_resource;
struct nouveau_bo;
struct nv50_context;

#define NV50_BUFFER_SCORE_MIN -25000
#define NV50_BUFFER_SCORE_MAX  25000
#define NV50_BUFFER_SCORE_VRAM_THRESHOLD 20000

/* DIRTY: buffer was (or will be after the next flush) written to by GPU and
 *  resource->data has not been updated to reflect modified VRAM contents
 *
 * USER_MEMORY: resource->data is a pointer to client memory and may change
 *  between GL calls
 */
#define NV50_BUFFER_STATUS_DIRTY       (1 << 0)
#define NV50_BUFFER_STATUS_USER_MEMORY (1 << 7)

/* Resources, if mapped into the GPU's address space, are guaranteed to
 * have constant virtual addresses.
 * The address of a resource will lie within the nouveau_bo referenced,
 * and this bo should be added to the memory manager's validation list.
 */
struct nv50_resource {
   struct pipe_resource base;
   const struct u_resource_vtbl *vtbl;

   uint8_t *data;
   struct nouveau_bo *bo;
   uint32_t offset;

   uint8_t status;
   uint8_t domain;

   int16_t score; /* low if mapped very often, if high can move to VRAM */

   struct nv50_fence *fence;
   struct nv50_fence *fence_wr;

   struct nv50_mm_allocation *mm;
};

void
nv50_buffer_release_gpu_storage(struct nv50_resource *);

boolean
nv50_buffer_download(struct nv50_context *, struct nv50_resource *,
                     unsigned start, unsigned size);

boolean
nv50_buffer_migrate(struct nv50_context *,
                    struct nv50_resource *, unsigned domain);

static INLINE void
nv50_buffer_adjust_score(struct nv50_context *nv50, struct nv50_resource *res,
                         int16_t score)
{
   if (score < 0) {
      if (res->score > NV50_BUFFER_SCORE_MIN)
         res->score += score;
   } else
   if (score > 0){
      if (res->score < NV50_BUFFER_SCORE_MAX)
         res->score += score;
      if (res->domain == NOUVEAU_BO_GART &&
          res->score > NV50_BUFFER_SCORE_VRAM_THRESHOLD)
         nv50_buffer_migrate(nv50, res, NOUVEAU_BO_VRAM);
   }
}

/* XXX: wait for fence (atm only using this for vertex push) */
static INLINE void *
nv50_resource_map_offset(struct nv50_context *nv50,
                         struct nv50_resource *res, uint32_t offset,
                         uint32_t flags)
{
   void *map;

   nv50_buffer_adjust_score(nv50, res, -250);

   if ((res->domain == NOUVEAU_BO_VRAM) &&
       (res->status & NV50_BUFFER_STATUS_DIRTY))
      nv50_buffer_download(nv50, res, 0, res->base.width0);

   if ((res->domain != NOUVEAU_BO_GART) ||
       (res->status & NV50_BUFFER_STATUS_USER_MEMORY))
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
nv50_resource_unmap(struct nv50_resource *res)
{
   /* no-op */
}

#define NV50_TILE_DIM_SHIFT(m, d) (((m) >> (d * 4)) & 0xf)

#define NV50_TILE_PITCH(m)  (64 << 0)
#define NV50_TILE_HEIGHT(m) ( 4 << NV50_TILE_DIM_SHIFT(m, 0))
#define NV50_TILE_DEPTH(m)  ( 1 << NV50_TILE_DIM_SHIFT(m, 1))

#define NV50_TILE_SIZE_2D(m) ((64 * 8) <<                     \
                              NV50_TILE_DIM_SHIFT(m, 0))

#define NV50_TILE_SIZE(m) (NV50_TILE_SIZE_2D(m) << NV50_TILE_DIM_SHIFT(m, 1))

struct nv50_miptree_level {
   uint32_t offset;
   uint32_t pitch;
   uint32_t tile_mode;
};

#define NV50_MAX_TEXTURE_LEVELS 16

struct nv50_miptree {
   struct nv50_resource base;
   struct nv50_miptree_level level[NV50_MAX_TEXTURE_LEVELS];
   uint32_t total_size;
   uint32_t layer_stride;
   boolean layout_3d; /* TRUE if layer count varies with mip level */
};

static INLINE struct nv50_miptree *
nv50_miptree(struct pipe_resource *pt)
{
   return (struct nv50_miptree *)pt;
}

static INLINE struct nv50_resource *
nv50_resource(struct pipe_resource *resource)
{
   return (struct nv50_resource *)resource;
}

/* is resource mapped into the GPU's address space (i.e. VRAM or GART) ? */
static INLINE boolean
nv50_resource_mapped_by_gpu(struct pipe_resource *resource)
{
   return nv50_resource(resource)->domain != 0;
}

void
nv50_init_resource_functions(struct pipe_context *pcontext);

void
nv50_screen_init_resource_functions(struct pipe_screen *pscreen);

/* Internal functions:
 */
struct pipe_resource *
nv50_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *tmp);

struct pipe_resource *
nv50_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *template,
                         struct winsys_handle *whandle);

struct pipe_resource *
nv50_buffer_create(struct pipe_screen *pscreen,
                   const struct pipe_resource *templ);

struct pipe_resource *
nv50_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
                        unsigned usage);


struct pipe_surface *
nv50_miptree_surface_new(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_surface *templ);

void
nv50_miptree_surface_del(struct pipe_context *, struct pipe_surface *);

boolean
nv50_user_buffer_upload(struct nv50_resource *, unsigned base, unsigned size);

#endif
