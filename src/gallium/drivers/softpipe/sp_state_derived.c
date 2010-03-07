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
#include "sp_context.h"
#include "sp_screen.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tex_tile_cache.h"


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
      const struct sp_fragment_shader *spfs = softpipe->fs;
      struct vertex_info *vinfo_vbuf = &softpipe->vertex_info_vbuf;
      const uint num = draw_num_shader_outputs(softpipe->draw);
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
      for (i = 0; i < spfs->info.num_inputs; i++) {
         int src;
         enum interp_mode interp;

         switch (spfs->info.input_interpolate[i]) {
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

         switch (spfs->info.input_semantic_name[i]) {
         case TGSI_SEMANTIC_POSITION:
            interp = INTERP_POS;
            break;

         case TGSI_SEMANTIC_COLOR:
            if (softpipe->rasterizer->flatshade) {
               interp = INTERP_CONSTANT;
            }
            break;
         }

         /* this includes texcoords and varying vars */
         src = draw_find_shader_output(softpipe->draw,
                                       spfs->info.input_semantic_name[i],
                                       spfs->info.input_semantic_index[i]);
         draw_emit_vertex_attr(vinfo, EMIT_4F, interp, src);
      }

      softpipe->psize_slot = draw_find_shader_output(softpipe->draw,
                                                 TGSI_SEMANTIC_PSIZE, 0);
      if (softpipe->psize_slot > 0) {
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
   /* SP_NEW_FRAMEBUFFER
    */
   uint surfWidth = sp->framebuffer.width;
   uint surfHeight = sp->framebuffer.height;

   /* SP_NEW_RASTERIZER
    */
   if (sp->rasterizer->scissor) {

      /* SP_NEW_SCISSOR
       *
       * clip to scissor rect:
       */
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


static void
update_tgsi_samplers( struct softpipe_context *softpipe )
{
   unsigned i;

   softpipe_reset_sampler_varients( softpipe );

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct softpipe_tex_tile_cache *tc = softpipe->tex_cache[i];
      if (tc->texture) {
         struct softpipe_texture *spt = softpipe_texture(tc->texture);
         if (spt->timestamp != tc->timestamp) {
	    sp_tex_tile_cache_validate_texture( tc );
            /*
            _debug_printf("INV %d %d\n", tc->timestamp, spt->timestamp);
            */
            tc->timestamp = spt->timestamp;
         }
      }
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      struct softpipe_tex_tile_cache *tc = softpipe->vertex_tex_cache[i];

      if (tc->texture) {
         struct softpipe_texture *spt = softpipe_texture(tc->texture);

         if (spt->timestamp != tc->timestamp) {
	    sp_tex_tile_cache_validate_texture(tc);
            tc->timestamp = spt->timestamp;
         }
      }
   }
}


/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void softpipe_update_derived( struct softpipe_context *softpipe )
{
   struct softpipe_screen *sp_screen = softpipe_screen(softpipe->pipe.screen);

   /* Check for updated textures.
    */
   if (softpipe->tex_timestamp != sp_screen->timestamp) {
      softpipe->tex_timestamp = sp_screen->timestamp;
      softpipe->dirty |= SP_NEW_TEXTURE;
   }
      
   if (softpipe->dirty & (SP_NEW_SAMPLER |
                          SP_NEW_TEXTURE |
                          SP_NEW_FS | 
                          SP_NEW_VS))
      update_tgsi_samplers( softpipe );

   if (softpipe->dirty & (SP_NEW_RASTERIZER |
                          SP_NEW_FS |
                          SP_NEW_VS))
      invalidate_vertex_layout( softpipe );

   if (softpipe->dirty & (SP_NEW_SCISSOR |
                          SP_NEW_RASTERIZER |
                          SP_NEW_FRAMEBUFFER))
      compute_cliprect(softpipe);

   if (softpipe->dirty & (SP_NEW_BLEND |
                          SP_NEW_DEPTH_STENCIL_ALPHA |
                          SP_NEW_FRAMEBUFFER |
                          SP_NEW_FS))
      sp_build_quad_pipeline(softpipe);

   softpipe->dirty = 0;
}
