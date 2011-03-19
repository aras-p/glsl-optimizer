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

#include "util/u_inlines.h"
#include "util/u_prim.h"
#include "util/u_time.h"
#include "indices/u_indices.h"

#include "svga_hw_reg.h"
#include "svga_context.h"
#include "svga_screen.h"
#include "svga_draw.h"
#include "svga_state.h"
#include "svga_swtnl.h"
#include "svga_debug.h"



static enum pipe_error
retry_draw_range_elements( struct svga_context *svga,
                           struct pipe_resource *index_buffer,
                           unsigned index_size,
                           int index_bias,
                           unsigned min_index,
                           unsigned max_index,
                           unsigned prim, 
                           unsigned start, 
                           unsigned count,
                           boolean do_retry )
{
   enum pipe_error ret = 0;

   svga_hwtnl_set_unfilled( svga->hwtnl,
                            svga->curr.rast->hw_unfilled );

   svga_hwtnl_set_flatshade( svga->hwtnl,
                             svga->curr.rast->templ.flatshade,
                             svga->curr.rast->templ.flatshade_first );


   ret = svga_update_state( svga, SVGA_STATE_HW_DRAW );
   if (ret)
      goto retry;

   ret = svga_hwtnl_draw_range_elements( svga->hwtnl,
                                         index_buffer, index_size, index_bias,
                                         min_index, max_index,
                                         prim, start, count );
   if (ret)
      goto retry;

   return PIPE_OK;

retry:
   svga_context_flush( svga, NULL );

   if (do_retry)
   {
      return retry_draw_range_elements( svga,
                                        index_buffer, index_size, index_bias,
                                        min_index, max_index,
                                        prim, start, count,
                                        FALSE );
   }

   return ret;
}


static enum pipe_error
retry_draw_arrays( struct svga_context *svga,
                   unsigned prim, 
                   unsigned start, 
                   unsigned count,
                   boolean do_retry )
{
   enum pipe_error ret;

   svga_hwtnl_set_unfilled( svga->hwtnl,
                            svga->curr.rast->hw_unfilled );

   svga_hwtnl_set_flatshade( svga->hwtnl,
                             svga->curr.rast->templ.flatshade,
                             svga->curr.rast->templ.flatshade_first );

   ret = svga_update_state( svga, SVGA_STATE_HW_DRAW );
   if (ret)
      goto retry;

   ret = svga_hwtnl_draw_arrays( svga->hwtnl, prim,
                                 start, count );
   if (ret)
      goto retry;

   return 0;

retry:
   if (ret == PIPE_ERROR_OUT_OF_MEMORY && do_retry) 
   {
      svga_context_flush( svga, NULL );

      return retry_draw_arrays( svga,
                                prim,
                                start,
                                count,
                                FALSE );
   }

   return ret;
}


static void
svga_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct svga_context *svga = svga_context( pipe );
   unsigned reduced_prim = u_reduced_prim( info->mode );
   unsigned count = info->count;
   enum pipe_error ret = 0;

   if (!u_trim_pipe_prim( info->mode, &count ))
      return;

   if (svga->state.sw.need_swtnl != svga->prev_draw_swtnl) {
      /* We're switching between SW and HW drawing.  Do a flush to avoid
       * mixing HW and SW rendering with the same vertex buffer.
       */
      pipe->flush(pipe, NULL);
      svga->prev_draw_swtnl = svga->state.sw.need_swtnl;
   }

   /*
    * Mark currently bound target surfaces as dirty
    * doesn't really matter if it is done before drawing.
    *
    * TODO If we ever normaly return something other then
    * true we should not mark it as dirty then.
    */
   svga_mark_surfaces_dirty(svga_context(pipe));

   if (svga->curr.reduced_prim != reduced_prim) {
      svga->curr.reduced_prim = reduced_prim;
      svga->dirty |= SVGA_NEW_REDUCED_PRIMITIVE;
   }
   
   svga_update_state_retry( svga, SVGA_STATE_NEED_SWTNL );

#ifdef DEBUG
   if (svga->curr.vs->base.id == svga->debug.disable_shader ||
       svga->curr.fs->base.id == svga->debug.disable_shader)
      return;
#endif

   if (svga->state.sw.need_swtnl) {
      ret = svga_swtnl_draw_vbo( svga, info );
   }
   else {
      if (info->indexed && svga->curr.ib.buffer) {
         unsigned offset;

         assert(svga->curr.ib.offset % svga->curr.ib.index_size == 0);
         offset = svga->curr.ib.offset / svga->curr.ib.index_size;

         ret = retry_draw_range_elements( svga,
                                          svga->curr.ib.buffer,
                                          svga->curr.ib.index_size,
                                          info->index_bias,
                                          info->min_index,
                                          info->max_index,
                                          info->mode,
                                          info->start + offset,
                                          info->count,
                                          TRUE );
      }
      else {
         ret = retry_draw_arrays( svga,
                                  info->mode,
                                  info->start,
                                  info->count,
                                  TRUE );
      }
   }

   if (SVGA_DEBUG & DEBUG_FLUSH) {
      svga_hwtnl_flush_retry( svga );
      svga_context_flush(svga, NULL);
   }
}


void svga_init_draw_functions( struct svga_context *svga )
{
   svga->pipe.draw_vbo = svga_draw_vbo;
}
