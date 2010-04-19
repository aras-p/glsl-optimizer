/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "draw/draw_context.h"
#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_context.h"

#include "svga_hw_reg.h"

/* Hardware frontwinding is always set up as SVGA3D_FRONTWINDING_CW.
 */
static SVGA3dFace svga_translate_cullmode( unsigned mode,
                                           unsigned front_winding )
{
   switch (mode) {
   case PIPE_WINDING_NONE:
      return SVGA3D_FACE_NONE;
   case PIPE_WINDING_CCW:
      return SVGA3D_FACE_BACK;
   case PIPE_WINDING_CW:
      return SVGA3D_FACE_FRONT;
   case PIPE_WINDING_BOTH:
      return SVGA3D_FACE_FRONT_BACK;
   default:
      assert(0);
      return SVGA3D_FACE_NONE;
   }
}

static SVGA3dShadeMode svga_translate_flatshade( unsigned mode )
{
   return mode ? SVGA3D_SHADEMODE_FLAT : SVGA3D_SHADEMODE_SMOOTH;
}


static void *
svga_create_rasterizer_state(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *templ)
{
   struct svga_rasterizer_state *rast = CALLOC_STRUCT( svga_rasterizer_state );
   /* need this for draw module. */
   rast->templ = *templ;

   /* light_twoside          - XXX: need fragment shader varient */
   /* poly_smooth            - XXX: no fallback available */
   /* poly_stipple_enable    - draw module */
   /* sprite_coord_enable    - ? */
   /* point_quad_rasterization - ? */
   /* point_size_per_vertex  - ? */
   /* sprite_coord_mode      - ??? */
   /* bypass_vs_viewport_and_clip        - handled by viewport setup */
   /* flatshade_first        - handled by index translation */
   /* gl_rasterization_rules - XXX - viewport code */
   /* line_width             - draw module */
   /* fill_cw, fill_ccw      - draw module or index translation */

   rast->shademode = svga_translate_flatshade( templ->flatshade );
   rast->cullmode = svga_translate_cullmode( templ->cull_mode, 
                                             templ->front_winding );
   rast->scissortestenable = templ->scissor;
   rast->multisampleantialias = templ->multisample;
   rast->antialiasedlineenable = templ->line_smooth;
   rast->lastpixel = templ->line_last_pixel;
   rast->pointsize = templ->point_size;
   rast->hw_unfilled = PIPE_POLYGON_MODE_FILL;

   /* Use swtnl + decomposition implement these:
    */
   if (templ->poly_stipple_enable)
      rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;

   if (templ->line_width != 1.0 &&
       templ->line_width != 0.0)
      rast->need_pipeline |= SVGA_PIPELINE_FLAG_LINES;

   if (templ->line_stipple_enable) {
      /* LinePattern not implemented on all backends. 
       */
      if (0) {
         SVGA3dLinePattern lp;
         lp.repeat = templ->line_stipple_factor + 1;
         lp.pattern = templ->line_stipple_pattern;
         rast->linepattern = lp.uintValue;
      }
      else {
         rast->need_pipeline |= SVGA_PIPELINE_FLAG_LINES;
      }
   } 

   if (templ->point_smooth)
      rast->need_pipeline |= SVGA_PIPELINE_FLAG_POINTS;

   {
      boolean offset_cw = templ->offset_cw;
      boolean offset_ccw = templ->offset_ccw;
      boolean offset  = 0;
      int fill_cw = templ->fill_cw;
      int fill_ccw = templ->fill_ccw;
      int fill = PIPE_POLYGON_MODE_FILL;

      switch (templ->cull_mode) {
      case PIPE_WINDING_BOTH:
         offset = 0;
         fill = PIPE_POLYGON_MODE_FILL;
         break;

      case PIPE_WINDING_CW:
         offset = offset_ccw;
         fill = fill_ccw;
         break;

      case PIPE_WINDING_CCW:
         offset = offset_cw;
         fill = fill_cw;
         break;

      case PIPE_WINDING_NONE:
         if (fill_cw != fill_ccw || offset_cw != offset_ccw) 
         {
            /* Always need the draw module to work out different
             * front/back fill modes:
             */
            rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
         }
         else {
            offset = offset_ccw;
            fill = fill_ccw;
         }
         break;

      default:
         assert(0);
         break;
      }

      /* Unfilled primitive modes aren't implemented on all virtual
       * hardware.  We can do some unfilled processing with index
       * translation, but otherwise need the draw module:
       */
      if (fill != PIPE_POLYGON_MODE_FILL &&
          (templ->flatshade ||
           templ->light_twoside ||
           offset ||
           templ->cull_mode != PIPE_WINDING_NONE)) 
      {
         fill = PIPE_POLYGON_MODE_FILL;
         rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
      }

      /* If we are decomposing to lines, and lines need the pipeline,
       * then we also need the pipeline for tris.
       */
      if (fill == PIPE_POLYGON_MODE_LINE &&
          (rast->need_pipeline & SVGA_PIPELINE_FLAG_LINES))
      {
         fill = PIPE_POLYGON_MODE_FILL;
         rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
      }

      /* Similarly for points:
       */
      if (fill == PIPE_POLYGON_MODE_POINT &&
          (rast->need_pipeline & SVGA_PIPELINE_FLAG_POINTS))
      {
         fill = PIPE_POLYGON_MODE_FILL;
         rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
      }

      if (offset) {
         rast->slopescaledepthbias = templ->offset_scale;
         rast->depthbias = templ->offset_units;
      }

      rast->hw_unfilled = fill;
   }




   if (rast->need_pipeline & SVGA_PIPELINE_FLAG_TRIS) {
      /* Turn off stuff which will get done in the draw module:
       */
      rast->hw_unfilled = PIPE_POLYGON_MODE_FILL;
      rast->slopescaledepthbias = 0;
      rast->depthbias = 0;
   }

   return rast;
}

static void svga_bind_rasterizer_state( struct pipe_context *pipe,
                                        void *state )
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_rasterizer_state *raster = (struct svga_rasterizer_state *)state;

   svga->curr.rast = raster;

   draw_set_rasterizer_state(svga->swtnl.draw, raster ? &raster->templ : NULL,
                             state);
   
   svga->dirty |= SVGA_NEW_RAST;
}

static void svga_delete_rasterizer_state(struct pipe_context *pipe,
                                         void *raster)
{
   FREE(raster);
}


void svga_init_rasterizer_functions( struct svga_context *svga )
{
   svga->pipe.create_rasterizer_state = svga_create_rasterizer_state;
   svga->pipe.bind_rasterizer_state = svga_bind_rasterizer_state;
   svga->pipe.delete_rasterizer_state = svga_delete_rasterizer_state;
}


/***********************************************************************
 * Hardware state update
 */

