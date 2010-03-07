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

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_upload_mgr.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_draw.h"
#include "svga_tgsi.h"
#include "svga_screen.h"
#include "svga_screen_buffer.h"

#include "svga_hw_reg.h"


static int
upload_user_buffers( struct svga_context *svga )
{
   enum pipe_error ret = PIPE_OK;
   int i;
   int nr;

   if (0) 
      debug_printf("%s: %d\n", __FUNCTION__, svga->curr.num_vertex_buffers);

   nr = svga->curr.num_vertex_buffers;

   for (i = 0; i < nr; i++) 
   {
      if (svga_buffer_is_user_buffer(svga->curr.vb[i].buffer))
      {
         struct svga_buffer *buffer = svga_buffer(svga->curr.vb[i].buffer);

         if (!buffer->uploaded.buffer) {
            ret = u_upload_buffer( svga->upload_vb,
                                   0,
                                   buffer->base.size,
                                   &buffer->base,
                                   &buffer->uploaded.offset,
                                   &buffer->uploaded.buffer );
            if (ret)
               return ret;

            if (0)
               debug_printf("%s: %d: orig buf %p upl buf %p ofs %d sz %d\n",
                            __FUNCTION__,
                            i,
                            buffer,
                            buffer->uploaded.buffer,
                            buffer->uploaded.offset,
                            buffer->base.size);
         }

         pipe_buffer_reference( &svga->curr.vb[i].buffer, buffer->uploaded.buffer );
         svga->curr.vb[i].buffer_offset = buffer->uploaded.offset;
      }
   }

   if (0)
      debug_printf("%s: DONE\n", __FUNCTION__);

   return ret;
}


/***********************************************************************
 */


static int emit_hw_vs_vdecl( struct svga_context *svga,
                             unsigned dirty )
{
   const struct pipe_vertex_element *ve = svga->curr.ve;
   SVGA3dVertexDecl decl;
   unsigned i;

   assert(svga->curr.num_vertex_elements >=
          svga->curr.vs->base.info.file_count[TGSI_FILE_INPUT]);

   svga_hwtnl_reset_vdecl( svga->hwtnl, 
                           svga->curr.num_vertex_elements );

   for (i = 0; i < svga->curr.num_vertex_elements; i++) {
      const struct pipe_vertex_buffer *vb = &svga->curr.vb[ve[i].vertex_buffer_index];
      unsigned usage, index;


      svga_generate_vdecl_semantics( i, &usage, &index );

      /* SVGA_NEW_VELEMENT
       */
      decl.identity.type = svga->state.sw.ve_format[i];
      decl.identity.method = SVGA3D_DECLMETHOD_DEFAULT;
      decl.identity.usage = usage;
      decl.identity.usageIndex = index;
      decl.array.stride = vb->stride;
      decl.array.offset = (vb->buffer_offset +
                           ve[i].src_offset);

      svga_hwtnl_vdecl( svga->hwtnl,
                        i,
                        &decl,
                        vb->buffer );
   }

   return 0;
}


static int emit_hw_vdecl( struct svga_context *svga,
                          unsigned dirty )
{
   int ret = 0;

   /* SVGA_NEW_NEED_SWTNL
    */
   if (svga->state.sw.need_swtnl)
      return 0; /* Do not emit during swtnl */

   /* If we get to here, we know that we're going to draw.  Upload
    * userbuffers now and try to combine multiple userbuffers from
    * multiple draw calls into a single host buffer for performance.
    */
   if (svga->curr.any_user_vertex_buffers &&
       SVGA_COMBINE_USERBUFFERS)
   {
      ret = upload_user_buffers( svga );
      if (ret)
         return ret;

      svga->curr.any_user_vertex_buffers = FALSE;
   }

   return emit_hw_vs_vdecl( svga, dirty );
}


struct svga_tracked_state svga_hw_vdecl = 
{
   "hw vertex decl state (hwtnl version)",
   ( SVGA_NEW_NEED_SWTNL |
     SVGA_NEW_VELEMENT |
     SVGA_NEW_VBUFFER |
     SVGA_NEW_RAST |
     SVGA_NEW_FS |
     SVGA_NEW_VS ),
   emit_hw_vdecl
};






