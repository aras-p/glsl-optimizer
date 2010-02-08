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
#include "util/u_upload_mgr.h"

#include "svga_context.h"
#include "svga_screen.h"
#include "svga_screen_texture.h"
#include "svga_screen_buffer.h"
#include "svga_winsys.h"
#include "svga_swtnl.h"
#include "svga_draw.h"
#include "svga_debug.h"
#include "svga_state.h"


static void svga_destroy( struct pipe_context *pipe )
{
   struct svga_context *svga = svga_context( pipe );
   unsigned shader;

   svga_cleanup_framebuffer( svga );
   svga_cleanup_tss_binding( svga );

   svga_hwtnl_destroy( svga->hwtnl );

   svga_cleanup_vertex_state(svga);
   
   svga->swc->destroy(svga->swc);
   
   svga_destroy_swtnl( svga );

   u_upload_destroy( svga->upload_vb );
   u_upload_destroy( svga->upload_ib );

   util_bitmask_destroy( svga->vs_bm );
   util_bitmask_destroy( svga->fs_bm );

   for(shader = 0; shader < PIPE_SHADER_TYPES; ++shader)
      pipe_buffer_reference( &svga->curr.cb[shader], NULL );

   FREE( svga );
}

static unsigned int
svga_is_texture_referenced( struct pipe_context *pipe,
			    struct pipe_texture *texture,
			    unsigned face, unsigned level)
{
   struct svga_texture *tex = svga_texture(texture);
   struct svga_screen *ss = svga_screen(pipe->screen);

   /**
    * The screen does not cache texture writes.
    */

   if (!tex->handle || ss->sws->surface_is_flushed(ss->sws, tex->handle))
      return PIPE_UNREFERENCED;

   /**
    * sws->surface_is_flushed() does not distinguish between read references
    * and write references. So assume a reference is both.
    */

   return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}

static unsigned int
svga_is_buffer_referenced( struct pipe_context *pipe,
			   struct pipe_buffer *buf)

{
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_buffer *sbuf = svga_buffer(buf);

   /**
    * XXX: Check this.
    * The screen may cache buffer writes, but when we map, we map out
    * of those cached writes, so we don't need to set a
    * PIPE_REFERENCED_FOR_WRITE flag for cached buffers.
    */

   if (!sbuf->handle || ss->sws->surface_is_flushed(ss->sws, sbuf->handle))
     return PIPE_UNREFERENCED;

   /**
    * sws->surface_is_flushed() does not distinguish between read references
    * and write references. So assume a reference is both,
    * however, we make an exception for index- and vertex buffers, to avoid
    * a flush in st_bufferobj_get_subdata, during display list replay.
    */

   if (sbuf->base.usage & (PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_INDEX))
      return PIPE_REFERENCED_FOR_READ;

   return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
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

   svga->pipe.winsys = screen->winsys;
   svga->pipe.screen = screen;
   svga->pipe.priv = priv;
   svga->pipe.destroy = svga_destroy;
   svga->pipe.clear = svga_clear;

   svga->pipe.is_texture_referenced = svga_is_texture_referenced;
   svga->pipe.is_buffer_referenced = svga_is_buffer_referenced;

   svga->swc = svgascreen->sws->context_create(svgascreen->sws);
   if(!svga->swc)
      goto no_swc;

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

   /* debug */
   svga->debug.no_swtnl = debug_get_bool_option("SVGA_NO_SWTNL", FALSE);
   svga->debug.force_swtnl = debug_get_bool_option("SVGA_FORCE_SWTNL", FALSE);
   svga->debug.use_min_mipmap = debug_get_bool_option("SVGA_USE_MIN_MIPMAP", FALSE);
   svga->debug.disable_shader = debug_get_num_option("SVGA_DISABLE_SHADER", ~0);

   if (!svga_init_swtnl(svga))
      goto no_swtnl;

   svga->fs_bm = util_bitmask_create();
   if (svga->fs_bm == NULL)
      goto no_fs_bm;

   svga->vs_bm = util_bitmask_create();
   if (svga->vs_bm == NULL)
      goto no_vs_bm;

   svga->upload_ib = u_upload_create( svga->pipe.screen,
                                      32 * 1024,
                                      16,
                                      PIPE_BUFFER_USAGE_INDEX );
   if (svga->upload_ib == NULL)
      goto no_upload_ib;

   svga->upload_vb = u_upload_create( svga->pipe.screen,
                                      128 * 1024,
                                      16,
                                      PIPE_BUFFER_USAGE_VERTEX );
   if (svga->upload_vb == NULL)
      goto no_upload_vb;

   svga->hwtnl = svga_hwtnl_create( svga,
                                    svga->upload_ib,
                                    svga->swc );
   if (svga->hwtnl == NULL)
      goto no_hwtnl;


   ret = svga_emit_initial_state( svga );
   if (ret)
      goto no_state;
   
   /* Avoid shortcircuiting state with initial value of zero.
    */
   memset(&svga->state.hw_clear, 0xcd, sizeof(svga->state.hw_clear));
   memset(&svga->state.hw_clear.framebuffer, 0x0, 
          sizeof(svga->state.hw_clear.framebuffer));

   memset(&svga->state.hw_draw, 0xcd, sizeof(svga->state.hw_draw));
   memset(&svga->state.hw_draw.views, 0x0, sizeof(svga->state.hw_draw.views));
   svga->state.hw_draw.num_views = 0;

   svga->dirty = ~0;

   LIST_INITHEAD(&svga->dirty_buffers);

   return &svga->pipe;

no_state:
   svga_hwtnl_destroy( svga->hwtnl );
no_hwtnl:
   u_upload_destroy( svga->upload_vb );
no_upload_vb:
   u_upload_destroy( svga->upload_ib );
no_upload_ib:
   util_bitmask_destroy( svga->vs_bm );
no_vs_bm:
   util_bitmask_destroy( svga->fs_bm );
no_fs_bm:
   svga_destroy_swtnl(svga);
no_swtnl:
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

   svga->curr.nr_fbs = 0;

   /* Unmap upload manager buffers: 
    */
   u_upload_flush(svga->upload_vb);
   u_upload_flush(svga->upload_ib);

   /* Flush screen, to ensure that texture dma uploads are processed
    * before submitting commands.
    */
   svga_screen_flush(svgascreen, NULL);
   
   svga_context_flush_buffers(svga);

   /* Flush pending commands to hardware:
    */
   svga->swc->flush(svga->swc, pfence);

   if (SVGA_DEBUG & DEBUG_SYNC) {
      if (pfence && *pfence)
         svga->pipe.screen->fence_finish( svga->pipe.screen, *pfence, 0);
   }
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

