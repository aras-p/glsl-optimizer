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

#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "cell_context.h"
#include "cell_batch.h"
#include "cell_state.h"
#include "cell_state_emit.h"


/**
 * Determine how to map vertex program outputs to fragment program inputs.
 * Basically, this will be used when computing the triangle interpolation
 * coefficients from the post-transform vertex attributes.
 */
static void
calculate_vertex_layout( struct cell_context *cell )
{
   const struct cell_fragment_shader_state *fs = cell->fs;
   const enum interp_mode colorInterp
      = cell->rasterizer->flatshade ? INTERP_CONSTANT : INTERP_LINEAR;
   struct vertex_info *vinfo = &cell->vertex_info;
   uint i;
   int src;

#if 0
   if (cell->vbuf) {
      /* if using the post-transform vertex buffer, tell draw_vbuf to
       * simply emit the whole post-xform vertex as-is:
       */
      struct vertex_info *vinfo_vbuf = &cell->vertex_info_vbuf;
      vinfo_vbuf->num_attribs = 0;
      draw_emit_vertex_attr(vinfo_vbuf, EMIT_ALL, INTERP_NONE, 0);
      vinfo_vbuf->size = 4 * vs->num_outputs + sizeof(struct vertex_header)/4;
   }
#endif

   /* reset vinfo */
   vinfo->num_attribs = 0;

   /* we always want to emit vertex pos */
   src = draw_find_shader_output(cell->draw, TGSI_SEMANTIC_POSITION, 0);
   assert(src >= 0);
   draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_POS, src);


   /*
    * Loop over fragment shader inputs, searching for the matching output
    * from the vertex shader.
    */
   for (i = 0; i < fs->info.num_inputs; i++) {
      switch (fs->info.input_semantic_name[i]) {
      case TGSI_SEMANTIC_POSITION:
         /* already done above */
         break;

      case TGSI_SEMANTIC_COLOR:
         src = draw_find_shader_output(cell->draw, TGSI_SEMANTIC_COLOR, 
                                   fs->info.input_semantic_index[i]);
         assert(src >= 0);
         draw_emit_vertex_attr(vinfo, EMIT_4F, colorInterp, src);
         break;

      case TGSI_SEMANTIC_FOG:
         src = draw_find_shader_output(cell->draw, TGSI_SEMANTIC_FOG, 0);
#if 1
         if (src < 0) /* XXX temp hack, try demos/fogcoord.c with this */
            src = 0;
#endif
         assert(src >= 0);
         draw_emit_vertex_attr(vinfo, EMIT_1F, INTERP_PERSPECTIVE, src);
         break;

      case TGSI_SEMANTIC_GENERIC:
         /* this includes texcoords and varying vars */
         src = draw_find_shader_output(cell->draw, TGSI_SEMANTIC_GENERIC,
                              fs->info.input_semantic_index[i]);
         assert(src >= 0);
         draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
         break;

      default:
         assert(0);
      }
   }

   draw_compute_vertex_size(vinfo);

   /* XXX only signal this if format really changes */
   cell->dirty |= CELL_NEW_VERTEX_INFO;
}


#if 0
/**
 * Recompute cliprect from scissor bounds, scissor enable and surface size.
 */
static void
compute_cliprect(struct cell_context *sp)
{
   uint surfWidth = sp->framebuffer.width;
   uint surfHeight = sp->framebuffer.height;

   if (sp->rasterizer->scissor) {
      /* clip to scissor rect */
      sp->cliprect.minx = MAX2(sp->scissor.minx, 0);
      sp->cliprect.miny = MAX2(sp->scissor.miny, 0);
      sp->cliprect.maxx = MIN2(sp->scissor.maxx, surfWidth);
      sp->cliprect.maxy = MIN2(sp->scissor.maxy, surfHeight);
   }
   else {
      /* clip to surface bounds */
      sp->cliprect.minx = 0;
      sp->cliprect.miny = 0;
      sp->cliprect.maxx = surfWidth;
      sp->cliprect.maxy = surfHeight;
   }
}
#endif



/**
 * Update derived state, send current state to SPUs prior to rendering.
 */
void cell_update_derived( struct cell_context *cell )
{
   if (cell->dirty & (CELL_NEW_RASTERIZER |
                      CELL_NEW_FS |
                      CELL_NEW_VS))
      calculate_vertex_layout( cell );

#if 0
   if (cell->dirty & (CELL_NEW_SCISSOR |
                      CELL_NEW_DEPTH_STENCIL_ALPHA |
                      CELL_NEW_FRAMEBUFFER))
      compute_cliprect(cell);
#endif

   cell_emit_state(cell);

   cell->dirty = 0;
}
