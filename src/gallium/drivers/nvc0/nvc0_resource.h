
#ifndef __NVC0_RESOURCE_H__
#define __NVC0_RESOURCE_H__

#include "util/u_transfer.h"
#include "util/u_double_list.h"
#define NOUVEAU_NVC0
#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_fence.h"
#include "nouveau/nouveau_buffer.h"
#undef NOUVEAU_NVC0

void
nvc0_init_resource_functions(struct pipe_context *pcontext);

void
nvc0_screen_init_resource_functions(struct pipe_screen *pscreen);

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
   struct nv04_resource base;
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

/* Internal functions:
 */
struct pipe_resource *
nvc0_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *tmp);

struct pipe_resource *
nvc0_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *template,
                         struct winsys_handle *whandle);

struct pipe_surface *
nvc0_miptree_surface_new(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_surface *templ);

void
nvc0_miptree_surface_del(struct pipe_context *, struct pipe_surface *);

uint32_t
nvc0_miptree_zslice_offset(struct nvc0_miptree *, unsigned l, unsigned z);

#endif
