/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * quad blending
 */

#include "glheader.h"
#include "imports.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"


static void
depth_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   GLuint j;
   struct softpipe_surface *sps = softpipe_surface(softpipe->framebuffer.zbuf);
   GLfloat zzzz[QUAD_SIZE];  /**< Z for four pixels in quad */
   GLuint zmask = 0;

   assert(sps); /* shouldn't get here if there's no zbuffer */

   /* get zquad from zbuffer */
   sps->read_quad_z(sps, quad->x0, quad->y0, zzzz);

   switch (softpipe->depth_test.func) {
   case PIPE_FUNC_NEVER:
      break;
   case PIPE_FUNC_LESS:
      /* Note this is pretty much a single sse or cell instruction.  
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (quad->outputs.depth[j] < zzzz[j]) 
	    zmask |= 1 << j;
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (quad->outputs.depth[j] == zzzz[j]) 
	    zmask |= 1 << j;
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (quad->outputs.depth[j] <= zzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
      /* XXX fill in remaining cases */
   default:
      abort();
   }

   quad->mask &= zmask;

   if (softpipe->depth_test.writemask) {
      
      /* This is also efficient with sse / spe instructions: 
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (zmask & (1 << j)) {
	    zzzz[j] = quad->outputs.depth[j];
	 }
      }

      /* write updated zquad to zbuffer */
      sps->write_quad_z(sps, quad->x0, quad->y0, zzzz);
   }

   qs->next->run(qs->next, quad);
}




struct quad_stage *sp_quad_depth_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->run = depth_test_quad;

   return stage;
}
