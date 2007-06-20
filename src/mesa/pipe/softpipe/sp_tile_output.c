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

/* Vertices are just an array of floats, with all the attributes
 * packed.  We currently assume a layout like:
 *
 * attr[0][0..3] - window position
 * attr[1..n][0..3] - remaining attributes.
 *
 * Attributes are assumed to be 4 floats wide but are packed so that
 * all the enabled attributes run contiguously.
 */

#include "glheader.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_tile.h"


static void mask_copy( GLfloat (*dest)[4],
		       GLfloat (*src)[4],
		       GLuint mask )
{
   GLuint i, j;

   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 for (j = 0; j < 4; j++) {
	    dest[j][i] = src[j][i];
	 }
      }
   }
}


/**
 * Write quad to framebuffer, taking mask into account.
 *
 * Note that surfaces support only full quad reads and writes.
 */
void quad_output( struct softpipe_context *softpipe,
		  struct quad_header *quad )
{
   GLuint i;

   for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
      struct softpipe_surface *sps
         = (struct softpipe_surface *) softpipe->framebuffer.cbufs[i];

      if (quad->mask != MASK_ALL) {
         GLfloat tmp[4][QUAD_SIZE];

         /* Yes, we'll probably have a masked write as well, but this is
          * how blend will be done at least.
          */

         sps->read_quad_f_swz(sps, quad->x0, quad->y0, tmp);

         mask_copy( tmp, quad->outputs.color, quad->mask );

         sps->write_quad_f_swz(sps, quad->x0, quad->y0, tmp);
      }
      else {
         sps->write_quad_f_swz(sps, quad->x0, quad->y0, quad->outputs.color);
      }
   }
}
