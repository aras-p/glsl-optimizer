#ifndef XORG_EXA_H
#define XORG_EXA_H

#include "xorg_tracker.h"

#include <pipe/p_state.h>

struct cso_context;
struct xorg_shaders;

struct exa_context
{
   ExaDriverPtr pExa;
   struct pipe_context *ctx;
   struct pipe_screen *scrn;
   struct cso_context *cso;
   struct xorg_shaders *shaders;

   struct pipe_constant_buffer vs_const_buffer;
   struct pipe_constant_buffer fs_const_buffer;

   float vertices[4][2][4];
};


struct exa_pixmap_priv
{
   int flags;
   int tex_flags;

   struct pipe_texture *tex;
   unsigned int color;
   struct pipe_surface *src_surf; /* for copies */

   struct pipe_transfer *map_transfer;
   unsigned map_count;
};

struct pipe_surface *
exa_gpu_surface(struct exa_context *exa, struct exa_pixmap_priv *priv);


#endif
