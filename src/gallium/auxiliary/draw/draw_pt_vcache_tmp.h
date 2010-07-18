#define FUNC_VARS                      \
   struct draw_pt_front_end *frontend, \
   unsigned start,                     \
   unsigned count

#define LOCAL_VARS \
   struct vcache_frontend *vcache = (struct vcache_frontend *) frontend;   \
   struct draw_context *draw = vcache->draw;                               \
   const unsigned prim = vcache->input_prim;                               \
   const void *elts = draw_pt_elt_ptr(draw, start);                        \
   pt_elt_func get_elt = draw_pt_elt_func(draw);                           \
   const int elt_bias = draw->pt.user.eltBias;                             \
   const boolean last_vertex_last = !(draw->rasterizer->flatshade &&       \
                                      draw->rasterizer->flatshade_first);  \
   const unsigned prim_flags = 0x0;

#define GET_ELT(idx) (get_elt(elts, idx) + elt_bias)

#define FUNC_EXIT do { vcache_flush(vcache); } while (0)

#include "draw_decompose_tmp.h"
