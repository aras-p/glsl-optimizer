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

#include "svga_cmd.h"

#include "util/u_framebuffer.h"
#include "util/u_inlines.h"

#include "svga_context.h"
#include "svga_screen.h"
#include "svga_surface.h"
#include "svga_resource_texture.h"


static void svga_set_scissor_states( struct pipe_context *pipe,
                                     unsigned start_slot,
                                     unsigned num_scissors,
                                     const struct pipe_scissor_state *scissors )
{
   struct svga_context *svga = svga_context(pipe);

   memcpy( &svga->curr.scissor, scissors, sizeof(*scissors) );
   svga->dirty |= SVGA_NEW_SCISSOR;
}


static void svga_set_polygon_stipple( struct pipe_context *pipe,
                                      const struct pipe_poly_stipple *stipple )
{
   /* overridden by the draw module */
}


void svga_cleanup_framebuffer(struct svga_context *svga)
{
   struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   struct pipe_framebuffer_state *curr = &svga->curr.framebuffer;
   struct pipe_framebuffer_state *hw = &svga->state.hw_clear.framebuffer;
   unsigned i;

   for (i = 0; i < svgascreen->max_color_buffers; i++) {
      pipe_surface_reference(&curr->cbufs[i], NULL);
      pipe_surface_reference(&hw->cbufs[i], NULL);
   }

   pipe_surface_reference(&curr->zsbuf, NULL);
   pipe_surface_reference(&hw->zsbuf, NULL);
}


#define DEPTH_BIAS_SCALE_FACTOR_D16    ((float)(1<<15))
#define DEPTH_BIAS_SCALE_FACTOR_D24S8  ((float)(1<<23))
#define DEPTH_BIAS_SCALE_FACTOR_D32    ((float)(1<<31))


static void svga_set_framebuffer_state(struct pipe_context *pipe,
				       const struct pipe_framebuffer_state *fb)
{
   struct svga_context *svga = svga_context(pipe);
   struct pipe_framebuffer_state *dst = &svga->curr.framebuffer;
   boolean propagate = FALSE;
   unsigned i;

   dst->width = fb->width;
   dst->height = fb->height;
   dst->nr_cbufs = fb->nr_cbufs;

   /* check if we need to propagate any of the target surfaces */
   for (i = 0; i < dst->nr_cbufs; i++) {
      struct pipe_surface *s = i < fb->nr_cbufs ? fb->cbufs[i] : NULL;
      if (dst->cbufs[i] && dst->cbufs[i] != s) {
         if (svga_surface_needs_propagation(dst->cbufs[i])) {
            propagate = TRUE;
            break;
         }
      }
   }

   if (propagate) {
      /* make sure that drawing calls comes before propagation calls */
      svga_hwtnl_flush_retry( svga );
   
      for (i = 0; i < dst->nr_cbufs; i++) {
         struct pipe_surface *s = i < fb->nr_cbufs ? fb->cbufs[i] : NULL;
         if (dst->cbufs[i] && dst->cbufs[i] != s)
            svga_propagate_surface(svga, dst->cbufs[i]);
      }
   }

   /* XXX: Actually the virtual hardware may support rendertargets with
    * different size, depending on the host API and driver, but since we cannot
    * know that make no such assumption here. */
   for(i = 0; i < fb->nr_cbufs; ++i) {
      if (fb->zsbuf && fb->cbufs[i]) {
         assert(fb->zsbuf->width == fb->cbufs[i]->width); 
         assert(fb->zsbuf->height == fb->cbufs[i]->height); 
      }
   }

   util_copy_framebuffer_state(dst, fb);

   /* Set the rendered-to flags */
   for (i = 0; i < dst->nr_cbufs; i++) {
      struct pipe_surface *s = dst->cbufs[i];
      if (s) {
         struct svga_texture *t = svga_texture(s->texture);
         svga_set_texture_rendered_to(t, s->u.tex.first_layer, s->u.tex.level);
      }
   }

   if (svga->curr.framebuffer.zsbuf)
   {
      switch (svga->curr.framebuffer.zsbuf->format) {
      case PIPE_FORMAT_Z16_UNORM:
         svga->curr.depthscale = 1.0f / DEPTH_BIAS_SCALE_FACTOR_D16;
         break;
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      case PIPE_FORMAT_Z24X8_UNORM:
      case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      case PIPE_FORMAT_X8Z24_UNORM:
         svga->curr.depthscale = 1.0f / DEPTH_BIAS_SCALE_FACTOR_D24S8;
         break;
      case PIPE_FORMAT_Z32_UNORM:
         svga->curr.depthscale = 1.0f / DEPTH_BIAS_SCALE_FACTOR_D32;
         break;
      case PIPE_FORMAT_Z32_FLOAT:
         svga->curr.depthscale = 1.0f / ((float)(1<<23));
         break;
      default:
         svga->curr.depthscale = 0.0f;
         break;
      }

      /* Set rendered-to flag */
      {
         struct pipe_surface *s = dst->zsbuf;
         struct svga_texture *t = svga_texture(s->texture);
         svga_set_texture_rendered_to(t, s->u.tex.first_layer, s->u.tex.level);
      }
   }
   else {
      svga->curr.depthscale = 0.0f;
   }

   svga->dirty |= SVGA_NEW_FRAME_BUFFER;
}



static void svga_set_clip_state( struct pipe_context *pipe,
                                 const struct pipe_clip_state *clip )
{
   struct svga_context *svga = svga_context(pipe);

   svga->curr.clip = *clip; /* struct copy */

   svga->dirty |= SVGA_NEW_CLIP;
}



/* Called when driver state tracker notices changes to the viewport
 * matrix:
 */
static void svga_set_viewport_states( struct pipe_context *pipe,
                                      unsigned start_slot,
                                      unsigned num_viewports,
                                      const struct pipe_viewport_state *viewports )
{
   struct svga_context *svga = svga_context(pipe);

   svga->curr.viewport = *viewports; /* struct copy */

   svga->dirty |= SVGA_NEW_VIEWPORT;
}



void svga_init_misc_functions( struct svga_context *svga )
{
   svga->pipe.set_scissor_states = svga_set_scissor_states;
   svga->pipe.set_polygon_stipple = svga_set_polygon_stipple;
   svga->pipe.set_framebuffer_state = svga_set_framebuffer_state;
   svga->pipe.set_clip_state = svga_set_clip_state;
   svga->pipe.set_viewport_states = svga_set_viewport_states;
}


