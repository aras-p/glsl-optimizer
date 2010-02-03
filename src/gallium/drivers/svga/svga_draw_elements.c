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
#include "util/u_upload_mgr.h"
#include "indices/u_indices.h"

#include "svga_cmd.h"
#include "svga_draw.h"
#include "svga_draw_private.h"
#include "svga_screen_buffer.h"
#include "svga_winsys.h"
#include "svga_context.h"

#include "svga_hw_reg.h"


static enum pipe_error
translate_indices( struct svga_hwtnl *hwtnl,
                   struct pipe_buffer *src,
                   unsigned offset,
                   unsigned nr,
                   unsigned index_size,
                   u_translate_func translate,
                   struct pipe_buffer **out_buf )
{
   struct pipe_screen *screen = hwtnl->svga->pipe.screen;
   unsigned size = index_size * nr;
   const void *src_map = NULL;
   struct pipe_buffer *dst = NULL;
   void *dst_map = NULL;

   dst = screen->buffer_create( screen, 32, 
                                PIPE_BUFFER_USAGE_INDEX |
                                PIPE_BUFFER_USAGE_CPU_WRITE |
                                PIPE_BUFFER_USAGE_GPU_READ, 
                                size );
   if (dst == NULL)
      goto fail;

   src_map = pipe_buffer_map( screen, src, PIPE_BUFFER_USAGE_CPU_READ );
   if (src_map == NULL)
      goto fail;

   dst_map = pipe_buffer_map( screen, dst, PIPE_BUFFER_USAGE_CPU_WRITE );
   if (dst_map == NULL)
      goto fail;

   translate( (const char *)src_map + offset,
              nr,
              dst_map );

   pipe_buffer_unmap( screen, src );
   pipe_buffer_unmap( screen, dst );

   *out_buf = dst;
   return PIPE_OK;

fail:
   if (src_map)
      screen->buffer_unmap( screen, src );

   if (dst_map)
      screen->buffer_unmap( screen, dst );

   if (dst)
      screen->buffer_destroy( dst );

   return PIPE_ERROR_OUT_OF_MEMORY;
}





enum pipe_error
svga_hwtnl_simple_draw_range_elements( struct svga_hwtnl *hwtnl,
                                       struct pipe_buffer *index_buffer,
                                       unsigned index_size,
                                       unsigned min_index,
                                       unsigned max_index,
                                       unsigned prim, 
                                       unsigned start,
                                       unsigned count,
                                       unsigned bias )
{
   struct pipe_buffer *upload_buffer = NULL;
   SVGA3dPrimitiveRange range;
   unsigned hw_prim;
   unsigned hw_count;
   unsigned index_offset = start * index_size;
   int ret = PIPE_OK;

   hw_prim = svga_translate_prim(prim, count, &hw_count);
   if (hw_count == 0)
      goto done;

   if (index_buffer && 
       svga_buffer_is_user_buffer(index_buffer)) 
   {
      assert( index_buffer->size >= index_offset + count * index_size );

      ret = u_upload_buffer( hwtnl->upload_ib,
                             index_offset,
                             count * index_size,
                             index_buffer,
                             &index_offset,
                             &upload_buffer );
      if (ret)
         goto done;

      /* Don't need to worry about refcounting index_buffer as this is
       * just a stack variable without a counted reference of its own.
       * The caller holds the reference.
       */
      index_buffer = upload_buffer;
   }

   range.primType = hw_prim;
   range.primitiveCount = hw_count;
   range.indexArray.offset = index_offset;
   range.indexArray.stride = index_size;
   range.indexWidth = index_size;
   range.indexBias = bias;
      
   ret = svga_hwtnl_prim( hwtnl, &range, min_index, max_index, index_buffer );
   if (ret)
      goto done;

done:
   if (upload_buffer)
      pipe_buffer_reference( &upload_buffer, NULL );

   return ret;
}




enum pipe_error
svga_hwtnl_draw_range_elements( struct svga_hwtnl *hwtnl,
                                struct pipe_buffer *index_buffer,
                                unsigned index_size,
                                unsigned min_index,
                                unsigned max_index,
                                unsigned prim, unsigned start, unsigned count,
                                unsigned bias)
{
   unsigned gen_prim, gen_size, gen_nr, gen_type;
   u_translate_func gen_func;
   enum pipe_error ret = PIPE_OK;

   if (hwtnl->api_fillmode != PIPE_POLYGON_MODE_FILL && 
       prim >= PIPE_PRIM_TRIANGLES) 
   {
      gen_type = u_unfilled_translator( prim,
                                        index_size,
                                        count,
                                        hwtnl->api_fillmode,
                                        &gen_prim,
                                        &gen_size,
                                        &gen_nr,
                                        &gen_func );
   }
   else
   {
      gen_type = u_index_translator( svga_hw_prims,
                                     prim,
                                     index_size,
                                     count,
                                     hwtnl->api_pv,
                                     hwtnl->hw_pv,
                                     &gen_prim,
                                     &gen_size,
                                     &gen_nr,
                                     &gen_func );
   }

   
   if (gen_type == U_TRANSLATE_MEMCPY) {
      /* No need for translation, just pass through to hardware: 
       */
      return svga_hwtnl_simple_draw_range_elements( hwtnl, index_buffer,
                                                    index_size,
                                                    min_index,
                                                    max_index,
                                                    gen_prim, start, count, bias );
   }
   else {
      struct pipe_buffer *gen_buf = NULL;

      /* Need to allocate a new index buffer and run the translate
       * func to populate it.  Could potentially cache this translated
       * index buffer with the original to avoid future
       * re-translations.  Not much point if we're just accelerating
       * GL though, as index buffers are typically used only once
       * there.
       */
      ret = translate_indices( hwtnl,
                               index_buffer,
                               start * index_size,
                               gen_nr,
                               gen_size,
                               gen_func,
                               &gen_buf );
      if (ret)
         goto done;

      ret = svga_hwtnl_simple_draw_range_elements( hwtnl,
                                                   gen_buf,
                                                   gen_size,
                                                   min_index,
                                                   max_index,
                                                   gen_prim,
                                                   0,
                                                   gen_nr,
                                                   bias );
      if (ret)
         goto done;

   done:
      if (gen_buf)
         pipe_buffer_reference( &gen_buf, NULL );

      return ret;
   }
}





