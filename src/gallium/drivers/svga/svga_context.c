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

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"

#include "svga_context.h"
#include "svga_screen.h"
#include "svga_surface.h"
#include "svga_resource_texture.h"
#include "svga_resource_buffer.h"
#include "svga_resource.h"
#include "svga_winsys.h"
#include "svga_swtnl.h"
#include "svga_draw.h"
#include "svga_debug.h"
#include "svga_state.h"

DEBUG_GET_ONCE_BOOL_OPTION(no_swtnl, "SVGA_NO_SWTNL", FALSE)
DEBUG_GET_ONCE_BOOL_OPTION(force_swtnl, "SVGA_FORCE_SWTNL", FALSE);
DEBUG_GET_ONCE_BOOL_OPTION(use_min_mipmap, "SVGA_USE_MIN_MIPMAP", FALSE);
DEBUG_GET_ONCE_NUM_OPTION(disable_shader, "SVGA_DISABLE_SHADER", ~0);
DEBUG_GET_ONCE_BOOL_OPTION(no_line_width, "SVGA_NO_LINE_WIDTH", FALSE);
DEBUG_GET_ONCE_BOOL_OPTION(force_hw_line_stipple, "SVGA_FORCE_HW_LINE_STIPPLE", FALSE);

static void svga_destroy( struct pipe_context *pipe )
{
   struct svga_context *svga = svga_context( pipe );
   struct svga_winsys_screen *sws = svga_screen(pipe->screen)->sws;
   unsigned shader;

   util_blitter_destroy(svga->blitter);

   svga_cleanup_framebuffer( svga );
   svga_cleanup_tss_binding( svga );

   svga_hwtnl_destroy( svga->hwtnl );

   svga_cleanup_vertex_state(svga);
   
   svga->swc->destroy(svga->swc);
   
   svga_destroy_swtnl( svga );

   util_bitmask_destroy( svga->shader_id_bm );

   for (shader = 0; shader < PIPE_SHADER_TYPES; ++shader) {
      pipe_resource_reference( &svga->curr.cbufs[shader].buffer, NULL );
      sws->surface_reference(sws, &svga->state.hw_draw.hw_cb[shader], NULL);
   }

   FREE( svga );
}



struct pipe_context *svga_context_create( struct pipe_screen *screen,
					  void *priv )
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_context *svga = NULL;
   enum pipe_error ret;

   svga = CALLOC_STRUCT(svga_context);
   if (svga == NULL)
      goto no_svga;

   LIST_INITHEAD(&svga->dirty_buffers);

   svga->pipe.screen = screen;
   svga->pipe.priv = priv;
   svga->pipe.destroy = svga_destroy;
   svga->pipe.clear = svga_clear;

   svga->swc = svgascreen->sws->context_create(svgascreen->sws);
   if(!svga->swc)
      goto no_swc;

   svga_init_resource_functions(svga);
   svga_init_blend_functions(svga);
   svga_init_blit_functions(svga);
   svga_init_depth_stencil_functions(svga);
   svga_init_draw_functions(svga);
   svga_init_flush_functions(svga);
   svga_init_misc_functions(svga);
   svga_init_rasterizer_functions(svga);
   svga_init_sampler_functions(svga);
   svga_init_fs_functions(svga);
   svga_init_vs_functions(svga);
   svga_init_vertex_functions(svga);
   svga_init_constbuffer_functions(svga);
   svga_init_query_functions(svga);
   svga_init_surface_functions(svga);


   /* debug */
   svga->debug.no_swtnl = debug_get_option_no_swtnl();
   svga->debug.force_swtnl = debug_get_option_force_swtnl();
   svga->debug.use_min_mipmap = debug_get_option_use_min_mipmap();
   svga->debug.disable_shader = debug_get_option_disable_shader();
   svga->debug.no_line_width = debug_get_option_no_line_width();
   svga->debug.force_hw_line_stipple = debug_get_option_force_hw_line_stipple();

   svga->shader_id_bm = util_bitmask_create();
   if (svga->shader_id_bm == NULL)
      goto no_shader_bm;

   svga->hwtnl = svga_hwtnl_create(svga);
   if (svga->hwtnl == NULL)
      goto no_hwtnl;

   if (!svga_init_swtnl(svga))
      goto no_swtnl;

   ret = svga_emit_initial_state( svga );
   if (ret != PIPE_OK)
      goto no_state;
   
   /* Avoid shortcircuiting state with initial value of zero.
    */
   memset(&svga->state.hw_clear, 0xcd, sizeof(svga->state.hw_clear));
   memset(&svga->state.hw_clear.framebuffer, 0x0, 
          sizeof(svga->state.hw_clear.framebuffer));

   memset(&svga->state.hw_draw, 0xcd, sizeof(svga->state.hw_draw));
   memset(&svga->state.hw_draw.views, 0x0, sizeof(svga->state.hw_draw.views));
   svga->state.hw_draw.num_views = 0;
   memset(&svga->state.hw_draw.hw_cb, 0x0, sizeof(svga->state.hw_draw.hw_cb));

   svga->dirty = ~0;

   return &svga->pipe;

no_state:
   svga_destroy_swtnl(svga);
no_swtnl:
   svga_hwtnl_destroy( svga->hwtnl );
no_hwtnl:
   util_bitmask_destroy( svga->shader_id_bm );
no_shader_bm:
   svga->swc->destroy(svga->swc);
no_swc:
   FREE(svga);
no_svga:
   return NULL;
}


void svga_context_flush( struct svga_context *svga, 
                         struct pipe_fence_handle **pfence )
{
   struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   struct pipe_fence_handle *fence = NULL;

   svga->curr.nr_fbs = 0;

   /* Ensure that texture dma uploads are processed
    * before submitting commands.
    */
   svga_context_flush_buffers(svga);

   /* Flush pending commands to hardware:
    */
   svga->swc->flush(svga->swc, &fence);

   svga_screen_cache_flush(svgascreen, fence);

   /* To force the re-emission of rendertargets and texture sampler bindings on
    * the next command buffer.
    */
   svga->rebind.rendertargets = TRUE;
   svga->rebind.texture_samplers = TRUE;
   if (svga_have_gb_objects(svga)) {
      svga->rebind.vs = TRUE;
      svga->rebind.fs = TRUE;
   }

   if (SVGA_DEBUG & DEBUG_SYNC) {
      if (fence)
         svga->pipe.screen->fence_finish( svga->pipe.screen, fence,
                                          PIPE_TIMEOUT_INFINITE);
   }

   if(pfence)
      svgascreen->sws->fence_reference(svgascreen->sws, pfence, fence);

   svgascreen->sws->fence_reference(svgascreen->sws, &fence, NULL);
}


void svga_hwtnl_flush_retry( struct svga_context *svga )
{
   enum pipe_error ret = PIPE_OK;

   ret = svga_hwtnl_flush( svga->hwtnl );
   if (ret == PIPE_ERROR_OUT_OF_MEMORY) {
      svga_context_flush( svga, NULL );
      ret = svga_hwtnl_flush( svga->hwtnl );
   }

   assert(ret == 0);
}


/**
 * Flush the primitive queue if this buffer is referred.
 *
 * Otherwise DMA commands on the referred buffer will be emitted too late.
 */
void svga_hwtnl_flush_buffer( struct svga_context *svga,
                              struct pipe_resource *buffer )
{
   if (svga_hwtnl_is_buffer_referred(svga->hwtnl, buffer)) {
      svga_hwtnl_flush_retry(svga);
   }
}


/* Emit all operations pending on host surfaces.
 */ 
void svga_surfaces_flush(struct svga_context *svga)
{
   struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   unsigned i;

   /* Emit buffered drawing commands.
    */
   svga_hwtnl_flush_retry( svga );

   /* Emit back-copy from render target view to texture.
    */
   for (i = 0; i < svgascreen->max_color_buffers; i++) {
      if (svga->curr.framebuffer.cbufs[i])
         svga_propagate_surface(svga, svga->curr.framebuffer.cbufs[i]);
   }

   if (svga->curr.framebuffer.zsbuf)
      svga_propagate_surface(svga, svga->curr.framebuffer.zsbuf);

}


struct svga_winsys_context *
svga_winsys_context( struct pipe_context *pipe )
{
   return svga_context( pipe )->swc;
}
