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
#include "pipe/p_state.h"


#include "svga_context.h"
#include "svga_state.h"
#include "svga_debug.h"
#include "svga_hw_reg.h"

/***********************************************************************
 */

static INLINE SVGA3dDeclType 
svga_translate_vertex_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_R32_FLOAT:            return SVGA3D_DECLTYPE_FLOAT1;
   case PIPE_FORMAT_R32G32_FLOAT:         return SVGA3D_DECLTYPE_FLOAT2;
   case PIPE_FORMAT_R32G32B32_FLOAT:      return SVGA3D_DECLTYPE_FLOAT3;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:   return SVGA3D_DECLTYPE_FLOAT4;
   case PIPE_FORMAT_A8R8G8B8_UNORM:       return SVGA3D_DECLTYPE_D3DCOLOR;
   case PIPE_FORMAT_R8G8B8A8_USCALED:     return SVGA3D_DECLTYPE_UBYTE4;
   case PIPE_FORMAT_R16G16_SSCALED:       return SVGA3D_DECLTYPE_SHORT2;
   case PIPE_FORMAT_R16G16B16A16_SSCALED: return SVGA3D_DECLTYPE_SHORT4;
   case PIPE_FORMAT_R8G8B8A8_UNORM:       return SVGA3D_DECLTYPE_UBYTE4N;
   case PIPE_FORMAT_R16G16_SNORM:         return SVGA3D_DECLTYPE_SHORT2N;
   case PIPE_FORMAT_R16G16B16A16_SNORM:   return SVGA3D_DECLTYPE_SHORT4N;
   case PIPE_FORMAT_R16G16_UNORM:         return SVGA3D_DECLTYPE_USHORT2N;
   case PIPE_FORMAT_R16G16B16A16_UNORM:   return SVGA3D_DECLTYPE_USHORT4N;

   /* These formats don't exist yet:
    * 
   case PIPE_FORMAT_R10G10B10_USCALED:    return SVGA3D_DECLTYPE_UDEC3;
   case PIPE_FORMAT_R10G10B10_SNORM:      return SVGA3D_DECLTYPE_DEC3N;
   case PIPE_FORMAT_R16G16_FLOAT:         return SVGA3D_DECLTYPE_FLOAT16_2;
   case PIPE_FORMAT_R16G16B16A16_FLOAT:   return SVGA3D_DECLTYPE_FLOAT16_4;
   */

   default:
      /* There are many formats without hardware support.  This case
       * will be hit regularly, meaning we'll need swvfetch.
       */
      return SVGA3D_DECLTYPE_MAX;
   }
}


static int update_need_swvfetch( struct svga_context *svga,
                                 unsigned dirty )
{
   unsigned i;
   boolean need_swvfetch = FALSE;

   for (i = 0; i < svga->curr.num_vertex_elements; i++) {
      svga->state.sw.ve_format[i] = svga_translate_vertex_format(svga->curr.ve[i].src_format);
      if (svga->state.sw.ve_format[i] == SVGA3D_DECLTYPE_MAX) {
         need_swvfetch = TRUE;
         break;
      }
   }

   if (need_swvfetch != svga->state.sw.need_swvfetch) {
      svga->state.sw.need_swvfetch = need_swvfetch;
      svga->dirty |= SVGA_NEW_NEED_SWVFETCH;
   }
   
   return 0;
}

struct svga_tracked_state svga_update_need_swvfetch = 
{
   "update need_swvfetch",
   ( SVGA_NEW_VELEMENT ),
   update_need_swvfetch
};


/*********************************************************************** 
 */

static int update_need_pipeline( struct svga_context *svga,
                                 unsigned dirty )
{
   
   boolean need_pipeline = FALSE;
   struct svga_vertex_shader *vs = svga->curr.vs;

   /* SVGA_NEW_RAST, SVGA_NEW_REDUCED_PRIMITIVE
    */
   if (svga->curr.rast->need_pipeline & (1 << svga->curr.reduced_prim)) {
      SVGA_DBG(DEBUG_SWTNL, "%s: rast need_pipeline (%d) & prim (%x)\n", 
                 __FUNCTION__,
                 svga->curr.rast->need_pipeline,
                 (1 << svga->curr.reduced_prim) );
      need_pipeline = TRUE;
   }

   /* EDGEFLAGS
    */
    if (vs->base.info.writes_edgeflag) {
      SVGA_DBG(DEBUG_SWTNL, "%s: edgeflags\n", __FUNCTION__);
      need_pipeline = TRUE;
   }

   /* SVGA_NEW_CLIP 
    */
   if (svga->curr.clip.nr) {
      SVGA_DBG(DEBUG_SWTNL, "%s: userclip\n", __FUNCTION__);
      need_pipeline = TRUE;
   }

   if (need_pipeline != svga->state.sw.need_pipeline) {
      svga->state.sw.need_pipeline = need_pipeline;
      svga->dirty |= SVGA_NEW_NEED_PIPELINE;
   }

   return 0;
}


struct svga_tracked_state svga_update_need_pipeline = 
{
   "need pipeline",
   (SVGA_NEW_RAST |
    SVGA_NEW_CLIP |
    SVGA_NEW_VS |
    SVGA_NEW_REDUCED_PRIMITIVE),
   update_need_pipeline
};


/*********************************************************************** 
 */

static int update_need_swtnl( struct svga_context *svga,
                              unsigned dirty )
{
   boolean need_swtnl;

   if (svga->debug.no_swtnl) {
      svga->state.sw.need_swvfetch = 0;
      svga->state.sw.need_pipeline = 0;
   }

   need_swtnl = (svga->state.sw.need_swvfetch ||
                 svga->state.sw.need_pipeline);

   if (svga->debug.force_swtnl) {
      need_swtnl = 1;
   }

   if (need_swtnl != svga->state.sw.need_swtnl) {
      SVGA_DBG(DEBUG_SWTNL|DEBUG_PERF,
               "%s need_swvfetch: %s, need_pipeline %s\n",
               __FUNCTION__,
               svga->state.sw.need_swvfetch ? "true" : "false",
               svga->state.sw.need_pipeline ? "true" : "false");

      svga->state.sw.need_swtnl = need_swtnl;
      svga->dirty |= SVGA_NEW_NEED_SWTNL;
      svga->swtnl.new_vdecl = TRUE;
   }
  
   return 0;
}


struct svga_tracked_state svga_update_need_swtnl =
{
   "need swtnl",
   (SVGA_NEW_NEED_PIPELINE |
    SVGA_NEW_NEED_SWVFETCH),
   update_need_swtnl
};
