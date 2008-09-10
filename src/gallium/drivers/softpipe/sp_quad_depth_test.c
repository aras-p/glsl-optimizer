/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * \brief  Quad depth testing
 */

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"
#include "sp_tile_cache.h"


/**
 * Do depth testing for a quad.
 * Not static since it's used by the stencil code.
 */

/*
 * To increase efficiency, we should probably have multiple versions
 * of this function that are specifically for Z16, Z32 and FP Z buffers.
 * Try to effectively do that with codegen...
 */

void
sp_depth_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   struct pipe_surface *ps = softpipe->framebuffer.zsbuf;
   const enum pipe_format format = ps->format;
   unsigned bzzzz[QUAD_SIZE];  /**< Z values fetched from depth buffer */
   unsigned qzzzz[QUAD_SIZE];  /**< Z values from the quad */
   unsigned zmask = 0;
   unsigned j;
   struct softpipe_cached_tile *tile
      = sp_get_cached_tile(softpipe, softpipe->zsbuf_cache, quad->input.x0, quad->input.y0);

   assert(ps); /* shouldn't get here if there's no zbuffer */

   /*
    * Convert quad's float depth values to int depth values (qzzzz).
    * If the Z buffer stores integer values, we _have_ to do the depth
    * compares with integers (not floats).  Otherwise, the float->int->float
    * conversion of Z values (which isn't an identity function) will cause
    * Z-fighting errors.
    *
    * Also, get the zbuffer values (bzzzz) from the cached tile.
    */
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      {
         float scale = 65535.0;

         for (j = 0; j < QUAD_SIZE; j++) {
            qzzzz[j] = (unsigned) (quad->output.depth[j] * scale);
         }

         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            bzzzz[j] = tile->data.depth16[y][x];
         }
      }
      break;
   case PIPE_FORMAT_Z32_UNORM:
      {
         double scale = (double) (uint) ~0UL;

         for (j = 0; j < QUAD_SIZE; j++) {
            qzzzz[j] = (unsigned) (quad->output.depth[j] * scale);
         }

         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            bzzzz[j] = tile->data.depth32[y][x];
         }
      }
      break;
   case PIPE_FORMAT_X8Z24_UNORM:
      /* fall-through */
   case PIPE_FORMAT_S8Z24_UNORM:
      {
         float scale = (float) ((1 << 24) - 1);

         for (j = 0; j < QUAD_SIZE; j++) {
            qzzzz[j] = (unsigned) (quad->output.depth[j] * scale);
         }

         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            bzzzz[j] = tile->data.depth32[y][x] & 0xffffff;
         }
      }
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
      /* fall-through */
   case PIPE_FORMAT_Z24S8_UNORM:
      {
         float scale = (float) ((1 << 24) - 1);

         for (j = 0; j < QUAD_SIZE; j++) {
            qzzzz[j] = (unsigned) (quad->output.depth[j] * scale);
         }

         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            bzzzz[j] = tile->data.depth32[y][x] >> 8;
         }
      }
      break;
   default:
      assert(0);
   }

   switch (softpipe->depth_stencil->depth.func) {
   case PIPE_FUNC_NEVER:
      /* zmask = 0 */
      break;
   case PIPE_FUNC_LESS:
      /* Note this is pretty much a single sse or cell instruction.  
       * Like this:  quad->mask &= (quad->outputs.depth < zzzz);
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] < bzzzz[j]) 
	    zmask |= 1 << j;
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] == bzzzz[j]) 
	    zmask |= 1 << j;
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] <= bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_GREATER:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] > bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_NOTEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] != bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_GEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] >= bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_ALWAYS:
      zmask = MASK_ALL;
      break;
   default:
      assert(0);
   }

   quad->inout.mask &= zmask;

   if (softpipe->depth_stencil->depth.writemask) {
      
      /* This is also efficient with sse / spe instructions: 
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (quad->inout.mask & (1 << j)) {
	    bzzzz[j] = qzzzz[j];
	 }
      }

      /* put updated Z values back into cached tile */
      switch (format) {
      case PIPE_FORMAT_Z16_UNORM:
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            tile->data.depth16[y][x] = (ushort) bzzzz[j];
         }
         break;
      case PIPE_FORMAT_X8Z24_UNORM:
         /* fall-through */
         /* (yes, this falls through to a different case than above) */
      case PIPE_FORMAT_Z32_UNORM:
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            tile->data.depth32[y][x] = bzzzz[j];
         }
         break;
      case PIPE_FORMAT_S8Z24_UNORM:
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            uint s8z24 = tile->data.depth32[y][x];
            s8z24 = (s8z24 & 0xff000000) | bzzzz[j];
            tile->data.depth32[y][x] = s8z24;
         }
         break;
      case PIPE_FORMAT_Z24S8_UNORM:
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            uint z24s8 = tile->data.depth32[y][x];
            z24s8 = (z24s8 & 0xff) | (bzzzz[j] << 8);
            tile->data.depth32[y][x] = z24s8;
         }
         break;
      case PIPE_FORMAT_Z24X8_UNORM:
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = quad->input.x0 % TILE_SIZE + (j & 1);
            int y = quad->input.y0 % TILE_SIZE + (j >> 1);
            tile->data.depth32[y][x] = bzzzz[j] << 8;
         }
         break;
      default:
         assert(0);
      }
   }
}


static void
depth_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   sp_depth_test_quad(qs, quad);

   if (quad->inout.mask)
      qs->next->run(qs->next, quad);
}


static void depth_test_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void depth_test_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *sp_quad_depth_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = depth_test_begin;
   stage->run = depth_test_quad;
   stage->destroy = depth_test_destroy;

   return stage;
}
