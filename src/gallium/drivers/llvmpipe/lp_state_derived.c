/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "draw/draw_private.h"
#include "lp_context.h"
#include "lp_screen.h"
#include "lp_state.h"


/**
 * Mark the current vertex layout as "invalid".
 * We'll validate the vertex layout later, when we start to actually
 * render a point or line or tri.
 */
static void
invalidate_vertex_layout(struct llvmpipe_context *llvmpipe)
{
   llvmpipe->vertex_info.num_attribs =  0;
}


/**
 * The vertex info describes how to convert the post-transformed vertices
 * (simple float[][4]) used by the 'draw' module into vertices for
 * rasterization.
 *
 * This function validates the vertex layout and returns a pointer to a
 * vertex_info object.
 */
struct vertex_info *
llvmpipe_get_vertex_info(struct llvmpipe_context *llvmpipe)
{
   struct vertex_info *vinfo = &llvmpipe->vertex_info;

   if (vinfo->num_attribs == 0) {
      /* compute vertex layout now */
      const struct lp_fragment_shader *lpfs = llvmpipe->fs;
      struct vertex_info *vinfo_vbuf = &llvmpipe->vertex_info_vbuf;
      const uint num = draw_num_shader_outputs(llvmpipe->draw);
      uint i;

      /* Tell draw_vbuf to simply emit the whole post-xform vertex
       * as-is.  No longer any need to try and emit draw vertex_header
       * info.
       */
      vinfo_vbuf->num_attribs = 0;
      for (i = 0; i < num; i++) {
	 draw_emit_vertex_attr(vinfo_vbuf, EMIT_4F, INTERP_PERSPECTIVE, i);
      }
      draw_compute_vertex_size(vinfo_vbuf);

      /*
       * Loop over fragment shader inputs, searching for the matching output
       * from the vertex shader.
       */
      vinfo->num_attribs = 0;
      for (i = 0; i < lpfs->info.num_inputs; i++) {
         int src;
         enum interp_mode interp;

         switch (lpfs->info.input_interpolate[i]) {
         case TGSI_INTERPOLATE_CONSTANT:
            interp = INTERP_CONSTANT;
            break;
         case TGSI_INTERPOLATE_LINEAR:
            interp = INTERP_LINEAR;
            break;
         case TGSI_INTERPOLATE_PERSPECTIVE:
            interp = INTERP_PERSPECTIVE;
            break;
         default:
            assert(0);
            interp = INTERP_LINEAR;
         }

         switch (lpfs->info.input_semantic_name[i]) {
         case TGSI_SEMANTIC_POSITION:
            interp = INTERP_POS;
            break;

         case TGSI_SEMANTIC_COLOR:
            if (llvmpipe->rasterizer->flatshade) {
               interp = INTERP_CONSTANT;
            }
            break;
         }

         /* this includes texcoords and varying vars */
         src = draw_find_shader_output(llvmpipe->draw,
                                   lpfs->info.input_semantic_name[i],
                                   lpfs->info.input_semantic_index[i]);
         draw_emit_vertex_attr(vinfo, EMIT_4F, interp, src);
      }

      llvmpipe->psize_slot = draw_find_shader_output(llvmpipe->draw,
                                                 TGSI_SEMANTIC_PSIZE, 0);
      if (llvmpipe->psize_slot > 0) {
         draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_CONSTANT,
                               llvmpipe->psize_slot);
      }

      draw_compute_vertex_size(vinfo);
   }

   return vinfo;
}


/**
 * Called from vbuf module.
 *
 * Note that there's actually two different vertex layouts in llvmpipe.
 *
 * The normal one is computed in llvmpipe_get_vertex_info() above and is
 * used by the point/line/tri "setup" code.
 *
 * The other one (this one) is only used by the vbuf module (which is
 * not normally used by default but used in testing).  For the vbuf module,
 * we basically want to pass-through the draw module's vertex layout as-is.
 * When the llvmpipe vbuf code begins drawing, the normal vertex layout
 * will come into play again.
 */
struct vertex_info *
llvmpipe_get_vbuf_vertex_info(struct llvmpipe_context *llvmpipe)
{
   (void) llvmpipe_get_vertex_info(llvmpipe);
   return &llvmpipe->vertex_info_vbuf;
}


/**
 * Recompute cliprect from scissor bounds, scissor enable and surface size.
 */
static void
compute_cliprect(struct llvmpipe_context *lp)
{
   /* LP_NEW_FRAMEBUFFER
    */
   uint surfWidth = lp->framebuffer.width;
   uint surfHeight = lp->framebuffer.height;

   /* LP_NEW_RASTERIZER
    */
   if (lp->rasterizer->scissor) {

      /* LP_NEW_SCISSOR
       *
       * clip to scissor rect:
       */
      lp->cliprect.minx = MAX2(lp->scissor.minx, 0);
      lp->cliprect.miny = MAX2(lp->scissor.miny, 0);
      lp->cliprect.maxx = MIN2(lp->scissor.maxx, surfWidth);
      lp->cliprect.maxy = MIN2(lp->scissor.maxy, surfHeight);
   }
   else {
      /* clip to surface bounds */
      lp->cliprect.minx = 0;
      lp->cliprect.miny = 0;
      lp->cliprect.maxx = surfWidth;
      lp->cliprect.maxy = surfHeight;
   }
}


/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void llvmpipe_update_derived( struct llvmpipe_context *llvmpipe )
{
   struct llvmpipe_screen *lp_screen = llvmpipe_screen(llvmpipe->pipe.screen);

   /* Check for updated textures.
    */
   if (llvmpipe->tex_timestamp != lp_screen->timestamp) {
      llvmpipe->tex_timestamp = lp_screen->timestamp;
      llvmpipe->dirty |= LP_NEW_TEXTURE;
   }
      
   if (llvmpipe->dirty & (LP_NEW_SAMPLER |
                          LP_NEW_TEXTURE)) {
      /* TODO */
   }

   if (llvmpipe->dirty & (LP_NEW_RASTERIZER |
                          LP_NEW_FS |
                          LP_NEW_VS))
      invalidate_vertex_layout( llvmpipe );

   if (llvmpipe->dirty & (LP_NEW_SCISSOR |
                          LP_NEW_RASTERIZER |
                          LP_NEW_FRAMEBUFFER))
      compute_cliprect(llvmpipe);

   if (llvmpipe->dirty & (LP_NEW_FS |
                          LP_NEW_BLEND |
                          LP_NEW_DEPTH_STENCIL_ALPHA |
                          LP_NEW_SAMPLER |
                          LP_NEW_TEXTURE))
      llvmpipe_update_fs( llvmpipe );


   llvmpipe->dirty = 0;
}
