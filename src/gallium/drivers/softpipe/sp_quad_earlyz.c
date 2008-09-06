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
 * \brief  Quad early-z testing
 */

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "sp_headers.h"
#include "sp_quad.h"


/**
 * All this stage does is compute the quad's Z values (which is normally
 * done by the shading stage).
 * The next stage will do the actual depth test.
 */
static void
earlyz_quad(
   struct quad_stage    *qs,
   struct quad_header   *quad )
{
   const float fx = (float) quad->input.x0;
   const float fy = (float) quad->input.y0;
   const float dzdx = quad->posCoef->dadx[2];
   const float dzdy = quad->posCoef->dady[2];
   const float z0 = quad->posCoef->a0[2] + dzdx * fx + dzdy * fy;

   quad->output.depth[0] = z0;
   quad->output.depth[1] = z0 + dzdx;
   quad->output.depth[2] = z0 + dzdy;
   quad->output.depth[3] = z0 + dzdx + dzdy;

   qs->next->run( qs->next, quad );
}

static void
earlyz_begin(
   struct quad_stage *qs )
{
   qs->next->begin( qs->next );
}

static void
earlyz_destroy(
   struct quad_stage *qs )
{
   FREE( qs );
}

struct quad_stage *
sp_quad_earlyz_stage(
   struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT( quad_stage );

   stage->softpipe = softpipe;
   stage->begin = earlyz_begin;
   stage->run = earlyz_quad;
   stage->destroy = earlyz_destroy;

   return stage;
}
