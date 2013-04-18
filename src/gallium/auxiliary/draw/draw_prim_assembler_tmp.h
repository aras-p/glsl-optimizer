#define FUNC_VARS                               \
   struct draw_assembler *asmblr,               \
   const struct draw_prim_info *input_prims,    \
   const struct draw_vertex_info *input_verts,  \
   unsigned start,                              \
   unsigned count

#define FUNC_ENTER                                                \
   /* declare more local vars */                                  \
   const unsigned prim = input_prims->prim;                       \
   const unsigned prim_flags = input_prims->flags;                \
   const boolean quads_flatshade_last = FALSE;                    \
   const boolean last_vertex_last = !asmblr->draw->rasterizer->flatshade_first;  \
   switch (prim) {                                                  \
   case PIPE_PRIM_QUADS:                                            \
   case PIPE_PRIM_QUAD_STRIP:                                       \
   case PIPE_PRIM_POLYGON:                                          \
      debug_assert(!"unexpected primitive type in prim assembler"); \
      return;                                                       \
   default:                                                         \
      break;                                                        \
   }                                                                \


#define POINT(i0)                             prim_point(asmblr, i0)
#define LINE(flags, i0, i1)                   prim_line(asmblr, i0, i1)
#define TRIANGLE(flags, i0, i1, i2)           prim_tri(asmblr, i0, i1, i2)
#define LINE_ADJ(flags, i0, i1, i2, i3)       prim_line_adj(asmblr, i0, i1, i2, i3)
#define TRIANGLE_ADJ(flags,i0,i1,i2,i3,i4,i5) prim_tri_adj(asmblr,i0,i1,i2,i3,i4,i5)

#include "draw_decompose_tmp.h"
