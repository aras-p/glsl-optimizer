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

#include "pipe/p_compiler.h"
#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "svga_context.h"
#include "svga_draw.h"
#include "svga_draw_private.h"
#include "svga_debug.h"
#include "svga_screen.h"
#include "svga_screen_buffer.h"
#include "svga_screen_texture.h"
#include "svga_winsys.h"
#include "svga_cmd.h"


struct svga_hwtnl *svga_hwtnl_create( struct svga_context *svga,
                                      struct u_upload_mgr *upload_ib,
                                      struct svga_winsys_context *swc )
{
   struct svga_hwtnl *hwtnl = CALLOC_STRUCT(svga_hwtnl);
   if (hwtnl == NULL)
      goto fail;

   hwtnl->svga = svga;
   hwtnl->upload_ib = upload_ib;
   
   hwtnl->cmd.swc = swc;

   return hwtnl;

fail:
   return NULL;
}

void svga_hwtnl_destroy( struct svga_hwtnl *hwtnl )
{
   int i, j;

   for (i = 0; i < PIPE_PRIM_MAX; i++) {
      for (j = 0; j < IDX_CACHE_MAX; j++) {
         pipe_buffer_reference( &hwtnl->index_cache[i][j].buffer,
                                NULL );
      }
   }

   for (i = 0; i < hwtnl->cmd.vdecl_count; i++)
      pipe_buffer_reference(&hwtnl->cmd.vdecl_vb[i], NULL);

   for (i = 0; i < hwtnl->cmd.prim_count; i++)
      pipe_buffer_reference(&hwtnl->cmd.prim_ib[i], NULL);
      

   FREE(hwtnl);
}


void svga_hwtnl_set_flatshade( struct svga_hwtnl *hwtnl,
                               boolean flatshade,
                               boolean flatshade_first )
{
   hwtnl->hw_pv = PV_FIRST;
   hwtnl->api_pv = (flatshade && !flatshade_first) ? PV_LAST : PV_FIRST;
}                               

void svga_hwtnl_set_unfilled( struct svga_hwtnl *hwtnl,
                              unsigned mode )
{
   hwtnl->api_fillmode = mode;
}                               

void svga_hwtnl_reset_vdecl( struct svga_hwtnl *hwtnl,
                             unsigned count )
{
   unsigned i;

   assert(hwtnl->cmd.prim_count == 0);

   for (i = count; i < hwtnl->cmd.vdecl_count; i++) {
      pipe_buffer_reference(&hwtnl->cmd.vdecl_vb[i],
                            NULL);
   }

   hwtnl->cmd.vdecl_count = count;
}


void svga_hwtnl_vdecl( struct svga_hwtnl *hwtnl,
                          unsigned i,
                          const SVGA3dVertexDecl *decl,
                          struct pipe_buffer *vb)
{
   assert(hwtnl->cmd.prim_count == 0);

   assert( i < hwtnl->cmd.vdecl_count );

   hwtnl->cmd.vdecl[i] = *decl;

   pipe_buffer_reference(&hwtnl->cmd.vdecl_vb[i],
                         vb);   
}



enum pipe_error
svga_hwtnl_flush( struct svga_hwtnl *hwtnl )
{
   struct svga_winsys_context *swc = hwtnl->cmd.swc;
   struct svga_context *svga = hwtnl->svga;
   enum pipe_error ret;

   if (hwtnl->cmd.prim_count) {
      struct svga_winsys_surface *vb_handle[SVGA3D_INPUTREG_MAX];
      struct svga_winsys_surface *ib_handle[QSZ];
      struct svga_winsys_surface *handle;
      SVGA3dVertexDecl *vdecl;
      SVGA3dPrimitiveRange *prim;
      unsigned i;

      for (i = 0; i < hwtnl->cmd.vdecl_count; i++) {
         handle = svga_buffer_handle(svga, hwtnl->cmd.vdecl_vb[i]);
         if (handle == NULL)
            return PIPE_ERROR_OUT_OF_MEMORY;

         vb_handle[i] = handle;
      }

      for (i = 0; i < hwtnl->cmd.prim_count; i++) {
         if (hwtnl->cmd.prim_ib[i]) {
            handle = svga_buffer_handle(svga, hwtnl->cmd.prim_ib[i]);
            if (handle == NULL)
               return PIPE_ERROR_OUT_OF_MEMORY;
         }
         else
            handle = NULL;

         ib_handle[i] = handle;
      }

      SVGA_DBG(DEBUG_DMA, "draw to sid %p, %d prims\n",
               svga->curr.framebuffer.cbufs[0] ?
               svga_surface(svga->curr.framebuffer.cbufs[0])->handle : NULL,
               hwtnl->cmd.prim_count);

      ret = SVGA3D_BeginDrawPrimitives(swc, 
                                       &vdecl, 
                                       hwtnl->cmd.vdecl_count, 
                                       &prim, 
                                       hwtnl->cmd.prim_count);
      if (ret != PIPE_OK) 
         return ret;

      
      memcpy( vdecl,
              hwtnl->cmd.vdecl,
              hwtnl->cmd.vdecl_count * sizeof hwtnl->cmd.vdecl[0]);

      for (i = 0; i < hwtnl->cmd.vdecl_count; i++) {
         /* Given rangeHint is considered to be relative to indexBias, and 
          * indexBias varies per primitive, we cannot accurately supply an 
          * rangeHint when emitting more than one primitive per draw command.
          */
         if (hwtnl->cmd.prim_count == 1) {
            vdecl[i].rangeHint.first = hwtnl->cmd.min_index[0];
            vdecl[i].rangeHint.last = hwtnl->cmd.max_index[0] + 1;
         }
         else {
            vdecl[i].rangeHint.first = 0;
            vdecl[i].rangeHint.last = 0;
         }

         swc->surface_relocation(swc,
                                 &vdecl[i].array.surfaceId,
                                 vb_handle[i],
                                 PIPE_BUFFER_USAGE_GPU_READ);
      }

      memcpy( prim,
              hwtnl->cmd.prim,
              hwtnl->cmd.prim_count * sizeof hwtnl->cmd.prim[0]);

      for (i = 0; i < hwtnl->cmd.prim_count; i++) {
         swc->surface_relocation(swc,
                                 &prim[i].indexArray.surfaceId,
                                 ib_handle[i],
                                 PIPE_BUFFER_USAGE_GPU_READ);
         pipe_buffer_reference(&hwtnl->cmd.prim_ib[i], NULL);
      }
      
      SVGA_FIFOCommitAll( swc );
      hwtnl->cmd.prim_count = 0;
   }

   return PIPE_OK;
}





/***********************************************************************
 * Internal functions:
 */

enum pipe_error svga_hwtnl_prim( struct svga_hwtnl *hwtnl,
                                 const SVGA3dPrimitiveRange *range,
                                 unsigned min_index,
                                 unsigned max_index,
                                 struct pipe_buffer *ib )
{
   int ret = PIPE_OK;

#ifdef DEBUG
   {
      unsigned i;
      for (i = 0; i < hwtnl->cmd.vdecl_count; i++) {
         struct pipe_buffer *vb = hwtnl->cmd.vdecl_vb[i];
         unsigned size = vb ? vb->size : 0;
         unsigned offset = hwtnl->cmd.vdecl[i].array.offset;
         unsigned stride = hwtnl->cmd.vdecl[i].array.stride;
         unsigned index_bias = range->indexBias;
         unsigned width;

         assert(vb);
         assert(size);
         assert(offset < size);
         assert(index_bias >= 0);
         assert(min_index <= max_index);
         assert(offset + index_bias*stride < size);
         if (min_index != ~0) {
            assert(offset + (index_bias + min_index) * stride < size);
         }

         switch (hwtnl->cmd.vdecl[i].identity.type) {
         case SVGA3D_DECLTYPE_FLOAT1:
            width = 4;
            break;
         case SVGA3D_DECLTYPE_FLOAT2:
            width = 4*2;
            break;
         case SVGA3D_DECLTYPE_FLOAT3:
            width = 4*3;
            break;
         case SVGA3D_DECLTYPE_FLOAT4:
            width = 4*4;
            break;
         case SVGA3D_DECLTYPE_D3DCOLOR:
            width = 4;
            break;
         case SVGA3D_DECLTYPE_UBYTE4:
            width = 1*4;
            break;
         case SVGA3D_DECLTYPE_SHORT2:
            width = 2*2;
            break;
         case SVGA3D_DECLTYPE_SHORT4:
            width = 2*4;
            break;
         case SVGA3D_DECLTYPE_UBYTE4N:
            width = 1*4;
            break;
         case SVGA3D_DECLTYPE_SHORT2N:
            width = 2*2;
            break;
         case SVGA3D_DECLTYPE_SHORT4N:
            width = 2*4;
            break;
         case SVGA3D_DECLTYPE_USHORT2N:
            width = 2*2;
            break;
         case SVGA3D_DECLTYPE_USHORT4N:
            width = 2*4;
            break;
         case SVGA3D_DECLTYPE_UDEC3:
            width = 4;
            break;
         case SVGA3D_DECLTYPE_DEC3N:
            width = 4;
            break;
         case SVGA3D_DECLTYPE_FLOAT16_2:
            width = 2*2;
            break;
         case SVGA3D_DECLTYPE_FLOAT16_4:
            width = 2*4;
            break;
         default:
            assert(0);
            width = 0;
            break;
         }

         assert(!stride || width <= stride);
         if (max_index != ~0) {
            assert(offset + (index_bias + max_index) * stride + width <= size);
         }
      }

      assert(range->indexWidth == range->indexArray.stride);

      if(ib) {
         unsigned size = ib->size;
         unsigned offset = range->indexArray.offset;
         unsigned stride = range->indexArray.stride;
         unsigned count;

         assert(size);
         assert(offset < size);
         assert(stride);

         switch (range->primType) {
         case SVGA3D_PRIMITIVE_POINTLIST:
            count = range->primitiveCount;
            break;
         case SVGA3D_PRIMITIVE_LINELIST:
            count = range->primitiveCount * 2;
            break;
         case SVGA3D_PRIMITIVE_LINESTRIP:
            count = range->primitiveCount + 1;
            break;
         case SVGA3D_PRIMITIVE_TRIANGLELIST:
            count = range->primitiveCount * 3;
            break;
         case SVGA3D_PRIMITIVE_TRIANGLESTRIP:
            count = range->primitiveCount + 2;
            break;
         case SVGA3D_PRIMITIVE_TRIANGLEFAN:
            count = range->primitiveCount + 2;
            break;
         default:
            assert(0);
            count = 0;
            break;
         }

         assert(offset + count*stride <= size);
      }
   }
#endif

   if (hwtnl->cmd.prim_count+1 >= QSZ) {
      ret = svga_hwtnl_flush( hwtnl );
      if (ret != PIPE_OK)
         return ret;
   }
   
   /* min/max indices are relative to bias */
   hwtnl->cmd.min_index[hwtnl->cmd.prim_count] = min_index;
   hwtnl->cmd.max_index[hwtnl->cmd.prim_count] = max_index;

   hwtnl->cmd.prim[hwtnl->cmd.prim_count] = *range;

   pipe_buffer_reference(&hwtnl->cmd.prim_ib[hwtnl->cmd.prim_count], ib);
   hwtnl->cmd.prim_count++;

   return ret;
}
