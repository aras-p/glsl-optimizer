
/**
 * \brief Quad stencil testing
 */


#include "main/glheader.h"
#include "main/imports.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"


/** Only 8-bit stencil supported */
#define STENCIL_MAX 0xff


/**
 * Do the basic stencil test (compare stencil buffer values against the
 * reference value.
 *
 * \param stencilVals  the stencil values from the stencil buffer
 * \param func  the stencil func (PIPE_FUNC_x)
 * \param ref  the stencil reference value
 * \param valMask  the stencil value mask indicating which bits of the stencil
 *                 values and ref value are to be used.
 * \return mask indicating which pixels passed the stencil test
 */
static GLbitfield
do_stencil_test(const GLubyte stencilVals[QUAD_SIZE], GLuint func,
                GLbitfield ref, GLbitfield valMask)
{
   GLbitfield passMask = 0x0;
   GLuint j;

   ref &= valMask;

   switch (func) {
   case PIPE_FUNC_NEVER:
      /* passMask = 0x0 */
      break;
   case PIPE_FUNC_LESS:
      for (j = 0; j < QUAD_SIZE; j++) {
         if ((stencilVals[j] & valMask) < ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if ((stencilVals[j] & valMask) == ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if ((stencilVals[j] & valMask) <= ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_GREATER:
      for (j = 0; j < QUAD_SIZE; j++) {
         if ((stencilVals[j] & valMask) > ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_NOTEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if ((stencilVals[j] & valMask) != ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_GEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if ((stencilVals[j] & valMask) >= ref) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_ALWAYS:
      passMask = MASK_ALL;
      break;
   default:
      assert(0);
   }

   return passMask;
}


/**
 * Apply the stencil operator to stencil values.
 *
 * \param stencilVals  the stencil buffer values (read and written)
 * \param mask  indicates which pixels to update
 * \param op  the stencil operator (PIPE_STENCIL_OP_x)
 * \param ref  the stencil reference value
 * \param wrtMask  writemask controlling which bits are changed in the
 *                 stencil values
 */
static void
apply_stencil_op(GLubyte stencilVals[QUAD_SIZE],
                 GLbitfield mask, GLuint op, GLubyte ref, GLubyte wrtMask)
{
   GLuint j;
   GLubyte newstencil[QUAD_SIZE];

   for (j = 0; j < QUAD_SIZE; j++) {
      newstencil[j] = stencilVals[j];
   }

   switch (op) {
   case PIPE_STENCIL_OP_KEEP:
      /* no-op */
      break;
   case PIPE_STENCIL_OP_ZERO:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            newstencil[j] = 0;
         }
      }
      break;
   case PIPE_STENCIL_OP_REPLACE:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            newstencil[j] = ref;
         }
      }
      break;
   case PIPE_STENCIL_OP_INCR:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            if (stencilVals[j] < STENCIL_MAX) {
               newstencil[j] = stencilVals[j] + 1;
            }
         }
      }
      break;
   case PIPE_STENCIL_OP_DECR:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            if (stencilVals[j] > 0) {
               newstencil[j] = stencilVals[j] - 1;
            }
         }
      }
      break;
   case PIPE_STENCIL_OP_INCR_WRAP:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            newstencil[j] = stencilVals[j] + 1;
         }
      }
      break;
   case PIPE_STENCIL_OP_DECR_WRAP:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            newstencil[j] = stencilVals[j] - 1;
         }
      }
      break;
   case PIPE_STENCIL_OP_INVERT:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (mask & (1 << j)) {
            newstencil[j] = ~stencilVals[j];
         }
      }
      break;
   default:
      assert(0);
   }

   /*
    * update the stencil values
    */
   if (wrtMask != STENCIL_MAX) {
      /* apply bit-wise stencil buffer writemask */
      for (j = 0; j < QUAD_SIZE; j++) {
         stencilVals[j] = (wrtMask & newstencil[j]) | (~wrtMask & stencilVals[j]);
      }
   }
   else {
      for (j = 0; j < QUAD_SIZE; j++) {
         stencilVals[j] = newstencil[j];
      }
   }
}


/**
 * Do stencil (and depth) testing.  Stenciling depends on the outcome of
 * depth testing.
 */
static void
stencil_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   struct softpipe_surface *s_surf = softpipe_surface(softpipe->framebuffer.sbuf);
   GLuint func, zFailOp, zPassOp, failOp;
   GLubyte ref, wrtMask, valMask;
   GLubyte stencilVals[QUAD_SIZE];

   /* choose front or back face function, operator, etc */
   /* XXX we could do these initializations once per primitive */
   if (softpipe->stencil.back_enabled && quad->facing) {
      func = softpipe->stencil.back_func;
      failOp = softpipe->stencil.back_fail_op;
      zFailOp = softpipe->stencil.back_zfail_op;
      zPassOp = softpipe->stencil.back_zpass_op;
      ref = softpipe->stencil.ref_value[1];
      wrtMask = softpipe->stencil.write_mask[1];
      valMask = softpipe->stencil.value_mask[1];
   }
   else {
      func = softpipe->stencil.front_func;
      failOp = softpipe->stencil.front_fail_op;
      zFailOp = softpipe->stencil.front_zfail_op;
      zPassOp = softpipe->stencil.front_zpass_op;
      ref = softpipe->stencil.ref_value[0];
      wrtMask = softpipe->stencil.write_mask[0];
      valMask = softpipe->stencil.value_mask[0];
   }

   assert(s_surf); /* shouldn't get here if there's no stencil buffer */
   s_surf->read_quad_stencil(s_surf, quad->x0, quad->y0, stencilVals);

   /* do the stencil test first */
   {
      GLbitfield passMask, failMask;
      passMask = do_stencil_test(stencilVals, func, ref, valMask);
      failMask = quad->mask & ~passMask;
      quad->mask &= passMask;

      if (failOp != PIPE_STENCIL_OP_KEEP) {
         apply_stencil_op(stencilVals, failMask, failOp, ref, wrtMask);
      }
   }

   if (quad->mask) {

      /* now the pixels that passed the stencil test are depth tested */
      if (softpipe->depth_test.enabled) {
         const GLbitfield origMask = quad->mask;

         sp_depth_test_quad(qs, quad);  /* quad->mask is updated */

         /* update stencil buffer values according to z pass/fail result */
         if (zFailOp != PIPE_STENCIL_OP_KEEP) {
            const GLbitfield failMask = origMask & ~quad->mask;
            apply_stencil_op(stencilVals, failMask, zFailOp, ref, wrtMask);
         }

         if (zPassOp != PIPE_STENCIL_OP_KEEP) {
            const GLbitfield passMask = origMask & quad->mask;
            apply_stencil_op(stencilVals, passMask, zPassOp, ref, wrtMask);
         }
      }
      else {
         /* no depth test, apply Zpass operator to stencil buffer values */
         apply_stencil_op(stencilVals, quad->mask, zPassOp, ref, wrtMask);
      }

   }

   s_surf->write_quad_stencil(s_surf, quad->x0, quad->y0, stencilVals);

   if (quad->mask)
      qs->next->run(qs->next, quad);
}




struct quad_stage *sp_quad_stencil_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->run = stencil_test_quad;

   return stage;
}
