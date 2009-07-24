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


#include "lp_context.h"
#include "lp_state.h"
#include "pipe/p_shader_tokens.h"

static void
lp_push_quad_first( struct llvmpipe_context *lp,
                    struct quad_stage *quad )
{
   quad->next = lp->quad.first;
   lp->quad.first = quad;
}

static void
lp_build_depth_stencil( struct llvmpipe_context *lp )
{
   if (lp->depth_stencil->stencil[0].enabled ||
       lp->depth_stencil->stencil[1].enabled) {
      lp_push_quad_first( lp, lp->quad.stencil_test );
   }
   else if (lp->depth_stencil->depth.enabled &&
            lp->framebuffer.zsbuf) {
      lp_push_quad_first( lp, lp->quad.depth_test );
   }
}

void
lp_build_quad_pipeline(struct llvmpipe_context *lp)
{
   boolean early_depth_test =
      lp->depth_stencil->depth.enabled &&
      lp->framebuffer.zsbuf &&
      !lp->depth_stencil->alpha.enabled &&
      !lp->fs->info.uses_kill &&
      !lp->fs->info.writes_z;

   /* build up the pipeline in reverse order... */

   /* Color combine
    */
   lp->quad.first = lp->quad.output;

   if (lp->blend->colormask != 0xf) {
      lp_push_quad_first( lp, lp->quad.colormask );
   }

   if (lp->blend->blend_enable ||
       lp->blend->logicop_enable) {
      lp_push_quad_first( lp, lp->quad.blend );
   }

   if ((lp->rasterizer->poly_smooth && lp->reduced_prim == PIPE_PRIM_TRIANGLES) ||
       (lp->rasterizer->line_smooth && lp->reduced_prim == PIPE_PRIM_LINES) ||
       (lp->rasterizer->point_smooth && lp->reduced_prim == PIPE_PRIM_POINTS)) {
      lp_push_quad_first( lp, lp->quad.coverage );
   }

   /* Shade/Depth/Stencil/Alpha
    */
   if (lp->active_query_count) {
      lp_push_quad_first( lp, lp->quad.occlusion );
   }

   if (!early_depth_test) {
      lp_build_depth_stencil( lp );
   }

   if (lp->depth_stencil->alpha.enabled) {
      lp_push_quad_first( lp, lp->quad.alpha_test );
   }

   lp_push_quad_first( lp, lp->quad.shade );

   if (early_depth_test) {
      lp_build_depth_stencil( lp );
      lp_push_quad_first( lp, lp->quad.earlyz );
   }
}

