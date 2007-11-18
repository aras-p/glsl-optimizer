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
#include "pipe/p_util.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"
#include "sp_tile_cache.h"

static void
earlyz_quad(
   struct quad_stage    *qs,
   struct quad_header   *quad )
{
   uint  i;
   const float fx = (float) quad->x0;
   const float fy = (float) quad->y0;
   float xy[4][2];

   xy[0][0] = fx;
   xy[1][0] = fx + 1.0f;
   xy[2][0] = fx;
   xy[3][0] = fx + 1.0f;

   xy[0][1] = fy;
   xy[1][1] = fy;
   xy[2][1] = fy + 1.0f;
   xy[3][1] = fy + 1.0f;

   for (i = 0; i < QUAD_SIZE; i++) {
      quad->outputs.depth[i] =
         quad->coef[0].a0[2] +
         quad->coef[0].dadx[2] * xy[i][0] +
         quad->coef[0].dady[2] * xy[i][1];
   }

   if (qs->next) {
      qs->next->run( qs->next, quad );
   }
}

static void
earlyz_begin(
   struct quad_stage *qs )
{
   if (qs->next) {
      qs->next->begin( qs->next );
   }
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
