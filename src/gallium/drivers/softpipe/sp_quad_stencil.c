
/**
 * \brief Quad stencil testing
 */


#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"


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
static unsigned
do_stencil_test(const ubyte stencilVals[QUAD_SIZE], unsigned func,
                unsigned ref, unsigned valMask)
{
   unsigned passMask = 0x0;
   unsigned j;

   ref &= valMask;

   switch (func) {
   case PIPE_FUNC_NEVER:
      /* passMask = 0x0 */
      break;
   case PIPE_FUNC_LESS:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (ref < (stencilVals[j] & valMask)) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (ref == (stencilVals[j] & valMask)) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (ref <= (stencilVals[j] & valMask)) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_GREATER:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (ref > (stencilVals[j] & valMask)) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_NOTEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (ref != (stencilVals[j] & valMask)) {
            passMask |= (1 << j);
         }
      }
      break;
   case PIPE_FUNC_GEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
         if (ref >= (stencilVals[j] & valMask)) {
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
apply_stencil_op(ubyte stencilVals[QUAD_SIZE],
                 unsigned mask, unsigned op, ubyte ref, ubyte wrtMask)
{
   unsigned j;
   ubyte newstencil[QUAD_SIZE];

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
   struct pipe_surface *ps = softpipe->framebuffer.zsbuf;
   unsigned func, zFailOp, zPassOp, failOp;
   ubyte ref, wrtMask, valMask;
   ubyte stencilVals[QUAD_SIZE];
   struct softpipe_cached_tile *tile
      = sp_get_cached_tile(softpipe, softpipe->zsbuf_cache, quad->input.x0, quad->input.y0);
   uint j;
   uint face = quad->input.facing;

   if (!softpipe->depth_stencil->stencil[1].enabled) {
      /* single-sided stencil test, use front (face=0) state */
      face = 0;
   }

   /* choose front or back face function, operator, etc */
   /* XXX we could do these initializations once per primitive */
   func    = softpipe->depth_stencil->stencil[face].func;
   failOp  = softpipe->depth_stencil->stencil[face].fail_op;
   zFailOp = softpipe->depth_stencil->stencil[face].zfail_op;
   zPassOp = softpipe->depth_stencil->stencil[face].zpass_op;
   ref     = softpipe->depth_stencil->stencil[face].ref_value;
   wrtMask = softpipe->depth_stencil->stencil[face].write_mask;
   valMask = softpipe->depth_stencil->stencil[face].value_mask;

   assert(ps); /* shouldn't get here if there's no stencil buffer */

   /* get stencil values from cached tile */
   switch (ps->format) {
   case PIPE_FORMAT_S8Z24_UNORM:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = quad->input.x0 % TILE_SIZE + (j & 1);
         int y = quad->input.y0 % TILE_SIZE + (j >> 1);
         stencilVals[j] = tile->data.depth32[y][x] >> 24;
      }
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = quad->input.x0 % TILE_SIZE + (j & 1);
         int y = quad->input.y0 % TILE_SIZE + (j >> 1);
         stencilVals[j] = tile->data.depth32[y][x] & 0xff;
      }
      break;
   case PIPE_FORMAT_S8_UNORM:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = quad->input.x0 % TILE_SIZE + (j & 1);
         int y = quad->input.y0 % TILE_SIZE + (j >> 1);
         stencilVals[j] = tile->data.stencil8[y][x];
      }
      break;
   default:
      assert(0);
   }

   /* do the stencil test first */
   {
      unsigned passMask, failMask;
      passMask = do_stencil_test(stencilVals, func, ref, valMask);
      failMask = quad->inout.mask & ~passMask;
      quad->inout.mask &= passMask;

      if (failOp != PIPE_STENCIL_OP_KEEP) {
         apply_stencil_op(stencilVals, failMask, failOp, ref, wrtMask);
      }
   }

   if (quad->inout.mask) {
      /* now the pixels that passed the stencil test are depth tested */
      if (softpipe->depth_stencil->depth.enabled) {
         const unsigned origMask = quad->inout.mask;

         sp_depth_test_quad(qs, quad);  /* quad->mask is updated */

         /* update stencil buffer values according to z pass/fail result */
         if (zFailOp != PIPE_STENCIL_OP_KEEP) {
            const unsigned failMask = origMask & ~quad->inout.mask;
            apply_stencil_op(stencilVals, failMask, zFailOp, ref, wrtMask);
         }

         if (zPassOp != PIPE_STENCIL_OP_KEEP) {
            const unsigned passMask = origMask & quad->inout.mask;
            apply_stencil_op(stencilVals, passMask, zPassOp, ref, wrtMask);
         }
      }
      else {
         /* no depth test, apply Zpass operator to stencil buffer values */
         apply_stencil_op(stencilVals, quad->inout.mask, zPassOp, ref, wrtMask);
      }

   }

   /* put new stencil values into cached tile */
   switch (ps->format) {
   case PIPE_FORMAT_S8Z24_UNORM:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = quad->input.x0 % TILE_SIZE + (j & 1);
         int y = quad->input.y0 % TILE_SIZE + (j >> 1);
         uint s8z24 = tile->data.depth32[y][x];
         s8z24 = (stencilVals[j] << 24) | (s8z24 & 0xffffff);
         tile->data.depth32[y][x] = s8z24;
      }
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = quad->input.x0 % TILE_SIZE + (j & 1);
         int y = quad->input.y0 % TILE_SIZE + (j >> 1);
         uint z24s8 = tile->data.depth32[y][x];
         z24s8 = (z24s8 & 0xffffff00) | stencilVals[j];
         tile->data.depth32[y][x] = z24s8;
      }
      break;
   case PIPE_FORMAT_S8_UNORM:
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = quad->input.x0 % TILE_SIZE + (j & 1);
         int y = quad->input.y0 % TILE_SIZE + (j >> 1);
         tile->data.stencil8[y][x] = stencilVals[j];
      }
      break;
   default:
      assert(0);
   }

   if (quad->inout.mask)
      qs->next->run(qs->next, quad);
}


static void stencil_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void stencil_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *sp_quad_stencil_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = stencil_begin;
   stage->run = stencil_test_quad;
   stage->destroy = stencil_destroy;

   return stage;
}
