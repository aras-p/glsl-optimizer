
/**
 * quad alpha test
 */

#include "sp_context.h"
#include "sp_quad.h"
#include "sp_quad_pipe.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"

#define ALPHATEST( FUNC, COMP )                                         \
   static void                                                          \
   alpha_test_quads_##FUNC( struct quad_stage *qs,                      \
                           struct quad_header *quads[],                 \
                           unsigned nr )                                \
   {                                                                    \
      const float ref = qs->softpipe->depth_stencil->alpha.ref_value;   \
      const uint cbuf = 0; /* only output[0].alpha is tested */         \
      unsigned pass_nr = 0;                                             \
      unsigned i;                                                       \
                                                                        \
      for (i = 0; i < nr; i++) {                                        \
         const float *aaaa = quads[i]->output.color[cbuf][3];           \
         unsigned passMask = 0;                                         \
                                                                        \
         if (aaaa[0] COMP ref) passMask |= (1 << 0);                    \
         if (aaaa[1] COMP ref) passMask |= (1 << 1);                    \
         if (aaaa[2] COMP ref) passMask |= (1 << 2);                    \
         if (aaaa[3] COMP ref) passMask |= (1 << 3);                    \
                                                                        \
         quads[i]->inout.mask &= passMask;                              \
                                                                        \
         if (quads[i]->inout.mask)                                      \
            quads[pass_nr++] = quads[i];                                \
      }                                                                 \
                                                                        \
      if (pass_nr)                                                      \
         qs->next->run(qs->next, quads, pass_nr);                       \
   }


ALPHATEST( LESS,     < )
ALPHATEST( EQUAL,    == )
ALPHATEST( LEQUAL,   <= )
ALPHATEST( GREATER,  > )
ALPHATEST( NOTEQUAL, != )
ALPHATEST( GEQUAL,   >= )


/* XXX: Incorporate into shader using KILP.
 */
static void
alpha_test_quad(struct quad_stage *qs, 
                struct quad_header *quads[], 
                unsigned nr)
{
   switch (qs->softpipe->depth_stencil->alpha.func) {
   case PIPE_FUNC_LESS:
      alpha_test_quads_LESS( qs, quads, nr );
      break;
   case PIPE_FUNC_EQUAL:
      alpha_test_quads_EQUAL( qs, quads, nr );
      break;
   case PIPE_FUNC_LEQUAL:
      alpha_test_quads_LEQUAL( qs, quads, nr );
      break;
   case PIPE_FUNC_GREATER:
      alpha_test_quads_GREATER( qs, quads, nr );
      break;
   case PIPE_FUNC_NOTEQUAL:
      alpha_test_quads_NOTEQUAL( qs, quads, nr );
      break;
   case PIPE_FUNC_GEQUAL:
      alpha_test_quads_GEQUAL( qs, quads, nr );
      break;
   case PIPE_FUNC_ALWAYS:
      assert(0); /* should be caught earlier */
      qs->next->run(qs->next, quads, nr);
      break;
   case PIPE_FUNC_NEVER:
   default:
      assert(0); /* should be caught earlier */
      return;
   }
}


static void alpha_test_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void alpha_test_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *
sp_quad_alpha_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = alpha_test_begin;
   stage->run = alpha_test_quad;
   stage->destroy = alpha_test_destroy;

   return stage;
}
