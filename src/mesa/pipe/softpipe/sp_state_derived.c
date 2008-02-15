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

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_vertex.h"
#include "pipe/draw/draw_private.h"
#include "sp_context.h"
#include "sp_state.h"


static int
find_vs_output(const struct pipe_shader_state *vs,
               uint semantic_name,
               uint semantic_index)
{
   uint i;
   for (i = 0; i < vs->num_outputs; i++) {
      if (vs->output_semantic_name[i] == semantic_name &&
          vs->output_semantic_index[i] == semantic_index)
         return i;
   }
   return -1;
}


/**
 * Mark the current vertex layout as "invalid".
 * We'll validate the vertex layout later, when we start to actually
 * render a point or line or tri.
 */
static void
invalidate_vertex_layout(struct softpipe_context *softpipe)
{
   softpipe->vertex_info.num_attribs =  0;
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
softpipe_get_vertex_info(struct softpipe_context *softpipe)
{
   struct vertex_info *vinfo = &softpipe->vertex_info;

   if (vinfo->num_attribs == 0) {
      /* compute vertex layout now */
      const struct pipe_shader_state *vs = &softpipe->vs->shader;
      const struct pipe_shader_state *fs = &softpipe->fs->shader;
      const enum interp_mode colorInterp
         = softpipe->rasterizer->flatshade ? INTERP_CONSTANT : INTERP_LINEAR;
      uint i;

      if (softpipe->vbuf) {
         /* if using the post-transform vertex buffer, tell draw_vbuf to
          * simply emit the whole post-xform vertex as-is:
          */
         struct vertex_info *vinfo_vbuf = &softpipe->vertex_info_vbuf;
         vinfo_vbuf->num_attribs = 0;
         draw_emit_vertex_attr(vinfo_vbuf, EMIT_ALL, INTERP_NONE, 0);
         vinfo_vbuf->size = 4 * vs->num_outputs
                          + sizeof(struct vertex_header) / 4;
      }

      /*
       * Loop over fragment shader inputs, searching for the matching output
       * from the vertex shader.
       */
      vinfo->num_attribs = 0;
      for (i = 0; i < fs->num_inputs; i++) {
         int src;
         switch (fs->input_semantic_name[i]) {
         case TGSI_SEMANTIC_POSITION:
            src = find_vs_output(vs, TGSI_SEMANTIC_POSITION, 0);
            assert(src >= 0);
            draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_POS, src);
            break;

         case TGSI_SEMANTIC_COLOR:
            src = find_vs_output(vs, TGSI_SEMANTIC_COLOR, 
                                 fs->input_semantic_index[i]);
            if (src < 0)
               src = 0;
            assert(src >= 0);
            draw_emit_vertex_attr(vinfo, EMIT_4F, colorInterp, src);
            break;

         case TGSI_SEMANTIC_FOG:
            src = find_vs_output(vs, TGSI_SEMANTIC_FOG, 0);
#if 1
            if (src < 0) /* XXX temp hack, try demos/fogcoord.c with this */
               src = 0;
#endif
            assert(src >= 0);
            draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
            break;

         case TGSI_SEMANTIC_GENERIC:
            /* this includes texcoords and varying vars */
            src = find_vs_output(vs, TGSI_SEMANTIC_GENERIC,
                                 fs->input_semantic_index[i]);
            assert(src >= 0);
            draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
            break;

         default:
            assert(0);
         }
      }

      softpipe->psize_slot = find_vs_output(vs, TGSI_SEMANTIC_PSIZE, 0);
      if (softpipe->psize_slot >= 0) {
         draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_CONSTANT,
                               softpipe->psize_slot);
      }

      draw_compute_vertex_size(vinfo);
   }

   return vinfo;
}


/**
 * Called from vbuf module.
 *
 * Note that there's actually two different vertex layouts in softpipe.
 *
 * The normal one is computed in softpipe_get_vertex_info() above and is
 * used by the point/line/tri "setup" code.
 *
 * The other one (this one) is only used by the vbuf module (which is
 * not normally used by default but used in testing).  For the vbuf module,
 * we basically want to pass-through the draw module's vertex layout as-is.
 * When the softpipe vbuf code begins drawing, the normal vertex layout
 * will come into play again.
 */
struct vertex_info *
softpipe_get_vbuf_vertex_info(struct softpipe_context *softpipe)
{
   (void) softpipe_get_vertex_info(softpipe);
   return &softpipe->vertex_info_vbuf;
}


/**
 * Recompute cliprect from scissor bounds, scissor enable and surface size.
 */
static void
compute_cliprect(struct softpipe_context *sp)
{
   unsigned surfWidth, surfHeight;

   if (sp->framebuffer.num_cbufs > 0) {
      surfWidth = sp->framebuffer.cbufs[0]->width;
      surfHeight = sp->framebuffer.cbufs[0]->height;
   }
   else {
      /* no surface? */
      surfWidth = sp->scissor.maxx;
      surfHeight = sp->scissor.maxy;
   }

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


/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void softpipe_update_derived( struct softpipe_context *softpipe )
{
   if (softpipe->dirty & (SP_NEW_RASTERIZER |
                          SP_NEW_FS |
                          SP_NEW_VS))
      invalidate_vertex_layout( softpipe );

   if (softpipe->dirty & (SP_NEW_SCISSOR |
                          SP_NEW_DEPTH_STENCIL_ALPHA |
                          SP_NEW_FRAMEBUFFER))
      compute_cliprect(softpipe);

   if (softpipe->dirty & (SP_NEW_BLEND |
                          SP_NEW_DEPTH_STENCIL_ALPHA |
                          SP_NEW_FRAMEBUFFER |
                          SP_NEW_RASTERIZER |
                          SP_NEW_FS | 
			  SP_NEW_QUERY))
      sp_build_quad_pipeline(softpipe);

   softpipe->dirty = 0;
}
