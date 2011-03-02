
#ifndef __NV50_RESOURCE_H__
#define __NV50_RESOURCE_H__

#include "util/u_transfer.h"
#include "util/u_double_list.h"
#define NOUVEAU_NVC0
#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_buffer.h"
#undef NOUVEAU_NVC0

void
nv50_init_resource_functions(struct pipe_context *pcontext);

void
nv50_screen_init_resource_functions(struct pipe_screen *pscreen);

#define NV50_TILE_DIM_SHIFT(m, d) (((m) >> (d * 4)) & 0xf)

#define NV50_TILE_PITCH(m)  (64 << 0)
#define NV50_TILE_HEIGHT(m) ( 4 << NV50_TILE_DIM_SHIFT(m, 0))
#define NV50_TILE_DEPTH(m)  ( 1 << NV50_TILE_DIM_SHIFT(m, 1))

#define NV50_TILE_SIZE_2D(m) ((64 * 4) <<                     \
                              NV50_TILE_DIM_SHIFT(m, 0))

#define NV50_TILE_SIZE(m) (NV50_TILE_SIZE_2D(m) << NV50_TILE_DIM_SHIFT(m, 1))

struct nv50_miptree_level {
   uint32_t offset;
   uint32_t pitch;
   uint32_t tile_mode;
};

#define NV50_MAX_TEXTURE_LEVELS 16

struct nv50_miptree {
   struct nv04_resource base;
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

/* Internal functions:
 */
struct pipe_resource *
nv50_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *tmp);

struct pipe_resource *
nv50_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *template,
                         struct winsys_handle *whandle);

struct pipe_surface *
nv50_miptree_surface_new(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_surface *templ);

void
nv50_miptree_surface_del(struct pipe_context *, struct pipe_surface *);

#endif
