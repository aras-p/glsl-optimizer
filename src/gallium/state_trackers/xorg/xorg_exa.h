#ifndef XORG_EXA_H
#define XORG_EXA_H

#include "xorg_tracker.h"

#include <pipe/p_state.h>

struct cso_context;
struct xorg_shaders;

/* src + mask + dst */
#define MAX_EXA_SAMPLERS 3

struct exa_context
{
   ExaDriverPtr pExa;
   struct pipe_context *pipe;
   struct pipe_screen *scrn;
   struct cso_context *cso;
   struct xorg_shaders *shaders;

   struct pipe_constant_buffer vs_const_buffer;
   struct pipe_constant_buffer fs_const_buffer;

   struct pipe_texture *bound_textures[MAX_EXA_SAMPLERS];
   int num_bound_samplers;

   float solid_color[4];
   boolean has_solid_color;

   struct {
      struct exa_pixmap_priv *src;
      struct exa_pixmap_priv *dst;
   } copy;
};


struct exa_pixmap_priv
{
   int flags;
   int tex_flags;

   struct pipe_texture *tex;
   struct pipe_texture *depth_stencil_tex;

   struct pipe_transfer *map_transfer;
   unsigned map_count;
};

struct pipe_surface *
exa_gpu_surface(struct exa_context *exa, struct exa_pixmap_priv *priv);

void xorg_exa_flush(struct exa_context *exa, uint pipeFlushFlags,
                    struct pipe_fence_handle **fence);
void xorg_exa_finish(struct exa_context *exa);

#endif
