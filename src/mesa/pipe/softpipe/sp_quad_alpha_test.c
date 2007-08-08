
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
   const GLfloat ref = softpipe->alpha_test.ref;
   GLuint passMask = 0x0, j;

   switch (softpipe->alpha_test.func) {
   case PIPE_FUNC_NEVER:
      quad->mask = 0x0;
      break;
   case PIPE_FUNC_LESS:
      /*
       * If mask were an array [4] we could do this SIMD-style:
       * passMask = (quad->outputs.color[3] <= vec4(ref));
       */
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->outputs.color[3][j] < ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->outputs.color[3][j] == ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->outputs.color[3][j] <= ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_GREATER:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->outputs.color[3][j] > ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_NOTEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->outputs.color[3][j] != ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_GEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->outputs.color[3][j] >= ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_ALWAYS:
      passMask = MASK_ALL;
      break;
   default:
      abort();
   }

   quad->mask &= passMask;

   if (quad->mask)
      qs->next->run(qs->next, quad);
}


static void alpha_test_begin(struct quad_stage *qs)
{
   if (qs->next->begin)
      qs->next->begin(qs->next);
}


struct quad_stage *
sp_quad_alpha_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = alpha_test_begin;
   stage->run = alpha_test_quad;

   return stage;
}
