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
 * \brief  Quad depth testing
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "pipe/p_defines.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"


/**
 * Do depth testing for a quad.
 * Not static since it's used by the stencil code.
 */
void
sp_depth_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   struct softpipe_surface *sps = softpipe_surface(softpipe->framebuffer.zbuf);
   GLuint bzzzz[QUAD_SIZE];  /**< Z values fetched from depth buffer */
   GLuint qzzzz[QUAD_SIZE];  /**< Z values from the quad */
   GLuint zmask = 0;
   GLuint j;
   GLfloat scale;

   assert(sps); /* shouldn't get here if there's no zbuffer */

   /*
    * To increase efficiency, we should probably have multiple versions
    * of this function that are specifically for Z16, Z32 and FP Z buffers.
    * Try to effectively do that with codegen...
    */
   if (sps->surface.format == PIPE_FORMAT_U_Z16)
      scale = 65535.0;
   else if (sps->surface.format == PIPE_FORMAT_S8_Z24)
      scale = (float) ((1 << 24) - 1);
   else
      assert(0);  /* XXX fix this someday */

   /*
    * Convert quad's float depth values to int depth values.
    * If the Z buffer stores integer values, we _have_ to do the depth
    * compares with integers (not floats).  Otherwise, the float->int->float
    * conversion of Z values (which isn't an identity function) will cause
    * Z-fighting errors.
    */
   for (j = 0; j < QUAD_SIZE; j++) {
      qzzzz[j] = (GLuint) (quad->outputs.depth[j] * scale);
   }

   /* get zquad from zbuffer */
   sps->read_quad_z(sps, quad->x0, quad->y0, bzzzz);

   switch (softpipe->depth_test.func) {
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
      abort();
   }

   quad->mask &= zmask;

   if (softpipe->depth_test.writemask) {
      
      /* This is also efficient with sse / spe instructions: 
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (quad->mask & (1 << j)) {
	    bzzzz[j] = qzzzz[j];
	 }
      }

      /* write updated zquad to zbuffer */
      sps->write_quad_z(sps, quad->x0, quad->y0, bzzzz);
   }
}


static void
depth_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   sp_depth_test_quad(qs, quad);

   if (quad->mask)
      qs->next->run(qs->next, quad);
}


static void depth_test_begin(struct quad_stage *qs)
{
   if (qs->next->begin)
      qs->next->begin(qs->next);
}


struct quad_stage *sp_quad_depth_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = depth_test_begin;
   stage->run = depth_test_quad;

   return stage;
}
