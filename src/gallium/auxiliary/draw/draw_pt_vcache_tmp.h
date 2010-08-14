#define FUNC_VARS                      \
   struct draw_pt_front_end *frontend, \
   pt_elt_func get_elt,                \
   const void *elts,                   \
   int elt_bias,                       \
   unsigned count

#define LOCAL_VARS \
   struct vcache_frontend *vcache = (struct vcache_frontend *) frontend;   \
   struct draw_context *draw = vcache->draw;                               \
   const unsigned prim = vcache->input_prim;                               \
   const boolean last_vertex_last = !(draw->rasterizer->flatshade &&       \
                                      draw->rasterizer->flatshade_first);

#define GET_ELT(idx) (get_elt(elts, idx) + elt_bias)

#define FUNC_EXIT do { vcache_flush(vcache); } while (0)

#include "draw_decompose_tmp.h"
