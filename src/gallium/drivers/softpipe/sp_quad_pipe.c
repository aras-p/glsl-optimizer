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


#include "sp_context.h"
#include "sp_state.h"
#include "pipe/p_shader_tokens.h"

static void
sp_push_quad_first(
   struct softpipe_context *sp,
   struct quad_stage *quad,
   uint i )
{
   quad->next = sp->quad[i].first;
   sp->quad[i].first = quad;
}

static void
sp_build_depth_stencil(
   struct softpipe_context *sp,
   uint i )
{
   if (sp->depth_stencil->stencil[0].enabled ||
       sp->depth_stencil->stencil[1].enabled) {
      sp_push_quad_first( sp, sp->quad[i].stencil_test, i );
   }
   else if (sp->depth_stencil->depth.enabled &&
            sp->framebuffer.zsbuf) {
      sp_push_quad_first( sp, sp->quad[i].depth_test, i );
   }
}

void
sp_build_quad_pipeline(struct softpipe_context *sp)
{
   uint i;

   boolean early_depth_test =
               sp->depth_stencil->depth.enabled &&
               sp->framebuffer.zsbuf &&
               !sp->depth_stencil->alpha.enabled &&
               !sp->fs->info.uses_kill &&
               !sp->fs->info.writes_z;

   /* build up the pipeline in reverse order... */
   for (i = 0; i < SP_NUM_QUAD_THREADS; i++) {
      sp->quad[i].first = sp->quad[i].output;

      if (sp->blend->colormask != 0xf) {
         sp_push_quad_first( sp, sp->quad[i].colormask, i );
      }

      if (sp->blend->blend_enable ||
          sp->blend->logicop_enable) {
         sp_push_quad_first( sp, sp->quad[i].blend, i );
      }

      if (sp->depth_stencil->depth.occlusion_count) {
         sp_push_quad_first( sp, sp->quad[i].occlusion, i );
      }

      if (sp->rasterizer->poly_smooth ||
          sp->rasterizer->line_smooth ||
          sp->rasterizer->point_smooth) {
         sp_push_quad_first( sp, sp->quad[i].coverage, i );
      }

      if (!early_depth_test) {
         sp_build_depth_stencil( sp, i );
      }

      if (sp->depth_stencil->alpha.enabled) {
         sp_push_quad_first( sp, sp->quad[i].alpha_test, i );
      }

      /* XXX always enable shader? */
      if (1) {
         sp_push_quad_first( sp, sp->quad[i].shade, i );
      }

      if (early_depth_test) {
         sp_build_depth_stencil( sp, i );
         sp_push_quad_first( sp, sp->quad[i].earlyz, i );
      }

#if !USE_DRAW_STAGE_PSTIPPLE
      if (sp->rasterizer->poly_stipple_enable) {
         sp_push_quad_first( sp, sp->quad[i].polygon_stipple, i );
      }
#endif
   }
}

