
/**
 * quad alpha test
 */

#include "glheader.h"
#include "imports.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"


static void
alpha_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   GLuint j;
   const GLfloat ref = softpipe->alpha_test.ref;

   switch (softpipe->alpha_test.func) {
   case PIPE_FUNC_NEVER:
      quad->mask = 0x0;
      break;
   case PIPE_FUNC_LESS:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->mask & (1 << j)) {
            if (quad->outputs.color[3][j] >= ref) {
               /* fail */
               quad->mask &= (1 << j);
            }
         }
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->mask & (1 << j)) {
            if (quad->outputs.color[3][j] != ref) {
               /* fail */
               quad->mask &= (1 << j);
            }
         }
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->mask & (1 << j)) {
            if (quad->outputs.color[3][j] > ref) {
               /* fail */
               quad->mask &= (1 << j);
            }
         }
      }
      break;
      /* XXX fill in remaining cases */
   default:
      abort();
   }

   qs->next->run(qs->next, quad);
}


struct quad_stage *
sp_quad_alpha_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->run = alpha_test_quad;

   return stage;
}
