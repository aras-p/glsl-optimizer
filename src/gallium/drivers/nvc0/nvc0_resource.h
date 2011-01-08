
#ifndef __NVC0_RESOURCE_H__
#define __NVC0_RESOURCE_H__

#include "util/u_transfer.h"
#include "util/u_double_list.h"
#define NOUVEAU_NVC0
#include "nouveau/nouveau_winsys.h"
#undef NOUVEAU_NVC0

#include "nvc0_fence.h"

struct pipe_resource;
struct nouveau_bo;
struct nvc0_context;

#define NVC0_BUFFER_SCORE_MIN -25000
#define NVC0_BUFFER_SCORE_MAX  25000
#define NVC0_BUFFER_SCORE_VRAM_THRESHOLD 20000

#define NVC0_BUFFER_STATUS_DIRTY       (1 << 0)
#define NVC0_BUFFER_STATUS_USER_MEMORY (1 << 7)

/* Resources, if mapped into the GPU's address space, are guaranteed to
 * have constant virtual addresses.
 * The address of a resource will lie within the nouveau_bo referenced,
 * and this bo should be added to the memory manager's validation list.
 */
struct nvc0_resource {
   struct pipe_resource base;
   const struct u_resource_vtbl *vtbl;

   uint8_t *data;
   struct nouveau_bo *bo;
   uint32_t offset;

   uint8_t status;
   uint8_t domain;

   int16_t score; /* low if mapped very often, if high can move to VRAM */

   struct nvc0_fence *fence;
   struct nvc0_fence *fence_wr;

   struct nvc0_mm_allocation *mm;
};

boolean
nvc0_buffer_download(struct nvc0_context *, struct nvc0_resource *,
                     unsigned start, unsigned size);

boolean
nvc0_buffer_migrate(struct nvc0_context *,
                    struct nvc0_resource *, unsigned domain);

static INLINE void
nvc0_buffer_adjust_score(struct nvc0_context *nvc0, struct nvc0_resource *res,
                         int16_t score)
{
   if (score < 0) {
      if (res->score > NVC0_BUFFER_SCORE_MIN)
         res->score += score;
   } else
   if (score > 0){
      if (res->score < NVC0_BUFFER_SCORE_MAX)
         res->score += score;
      if (res->domain == NOUVEAU_BO_GART &&
          res->score > NVC0_BUFFER_SCORE_VRAM_THRESHOLD)
         nvc0_buffer_migrate(nvc0, res, NOUVEAU_BO_VRAM);
   }
}

/* XXX: wait for fence (atm only using this for vertex push) */
static INLINE void *
nvc0_resource_map_offset(struct nvc0_context *nvc0,
                         struct nvc0_resource *res, uint32_t offset,
                         uint32_t flags)
{
   void *map;

   nvc0_buffer_adjust_score(nvc0, res, -250);

   if ((res->domain == NOUVEAU_BO_VRAM) &&
       (res->status & NVC0_BUFFER_STATUS_DIRTY))
      nvc0_buffer_download(nvc0, res, 0, res->base.width0);

   if (res->domain != NOUVEAU_BO_GART)
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
nvc0_resource_unmap(struct nvc0_resource *res)
{
   /* no-op */
}

#define NVC0_TILE_DIM_SHIFT(m, d) (((m) >> (d * 4)) & 0xf)

#define NVC0_TILE_PITCH(m)  (64 << NVC0_TILE_DIM_SHIFT(m, 0))
#define NVC0_TILE_HEIGHT(m) ( 8 << NVC0_TILE_DIM_SHIFT(m, 1))
#define NVC0_TILE_DEPTH(m)  ( 1 << NVC0_TILE_DIM_SHIFT(m, 2))

#define NVC0_TILE_SIZE_2D(m) (((64 * 8) <<                     \
                               NVC0_TILE_DIM_SHIFT(m, 0)) <<   \
                              NVC0_TILE_DIM_SHIFT(m, 1))

#define NVC0_TILE_SIZE(m) (NVC0_TILE_SIZE_2D(m) << NVC0_TILE_DIM_SHIFT(m, 2))

struct nvc0_miptree_level {
   uint32_t offset;
   uint32_t pitch;
   uint32_t tile_mode;
};

#define NVC0_MAX_TEXTURE_LEVELS 16

struct nvc0_miptree {
   struct nvc0_resource base;
   struct nvc0_miptree_level level[NVC0_MAX_TEXTURE_LEVELS];
   uint32_t total_size;
   uint32_t layer_stride;
   boolean layout_3d; /* TRUE if layer count varies with mip level */
};

static INLINE struct nvc0_miptree *
nvc0_miptree(struct pipe_resource *pt)
{
   return (struct nvc0_miptree *)pt;
}

static INLINE struct nvc0_resource *
nvc0_resource(struct pipe_resource *resource)
{
   return (struct nvc0_resource *)resource;
}

/* is resource mapped into the GPU's address space (i.e. VRAM or GART) ? */
static INLINE boolean
nvc0_resource_mapped_by_gpu(struct pipe_resource *resource)
{
   return nvc0_resource(resource)->domain != 0;
}

void
nvc0_init_resource_functions(struct pipe_context *pcontext);

void
nvc0_screen_init_resource_functions(struct pipe_screen *pscreen);

/* Internal functions:
 */
struct pipe_resource *
nvc0_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *tmp);

struct pipe_resource *
nvc0_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *template,
                         struct winsys_handle *whandle);

struct pipe_resource *
nvc0_buffer_create(struct pipe_screen *pscreen,
                   const struct pipe_resource *templ);

struct pipe_resource *
nvc0_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
                        unsigned usage);


struct pipe_surface *
nvc0_miptree_surface_new(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_surface *templ);

void
nvc0_miptree_surface_del(struct pipe_context *, struct pipe_surface *);

boolean
nvc0_migrate_vertices(struct nvc0_resource *buf, unsigned base, unsigned size);

#endif
