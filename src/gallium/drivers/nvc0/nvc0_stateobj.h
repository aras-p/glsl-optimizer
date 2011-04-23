
#ifndef __NVC0_STATEOBJ_H__
#define __NVC0_STATEOBJ_H__

#include "pipe/p_state.h"

#define SB_BEGIN_3D(so, m, s)                                                  \
   (so)->state[(so)->size++] =                                                 \
      (0x2 << 28) | ((s) << 16) | (NVC0_SUBCH_3D << 13) | ((NVC0_3D_##m) >> 2)

#define SB_IMMED_3D(so, m, d)                                                  \
   (so)->state[(so)->size++] =                                                 \
      (0x8 << 28) | ((d) << 16) | (NVC0_SUBCH_3D << 13) | ((NVC0_3D_##m) >> 2)

#define SB_DATA(so, u) (so)->state[(so)->size++] = (u)

#include "nv50/nv50_stateobj_tex.h"

struct nvc0_blend_stateobj {
   struct pipe_blend_state pipe;
   int size;
   uint32_t state[70];
};

struct nvc0_rasterizer_stateobj {
   struct pipe_rasterizer_state pipe;
   int size;
   uint32_t state[38];
};

struct nvc0_zsa_stateobj {
   struct pipe_depth_stencil_alpha_state pipe;
   int size;
   uint32_t state[26];
};

struct nvc0_vertex_element {
   struct pipe_vertex_element pipe;
   uint32_t state;
};

struct nvc0_vertex_stateobj {
   struct translate *translate;
   unsigned num_elements;
   uint32_t instance_elts;
   uint32_t instance_bufs;
   boolean need_conversion; /* e.g. VFETCH cannot convert f64 to f32 */
   unsigned vtx_size;
   unsigned vtx_per_packet_max;
   struct nvc0_vertex_element element[0];
};

/* will have to lookup index -> location qualifier from nvc0_program */
struct nvc0_transform_feedback_state {
   uint32_t stride[4];
   uint8_t varying_count[4];
   uint8_t varying_index[0];
};

#endif
