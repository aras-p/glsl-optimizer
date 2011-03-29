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

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_string.h"
#include "util/u_math.h"

#include "svga_winsys.h"
#include "svga_public.h"
#include "svga_context.h"
#include "svga_screen.h"
#include "svga_resource_texture.h"
#include "svga_resource.h"
#include "svga_debug.h"

#include "svga3d_shaderdefs.h"


#ifdef DEBUG
int SVGA_DEBUG = 0;

static const struct debug_named_value svga_debug_flags[] = {
   { "dma",      DEBUG_DMA, NULL },
   { "tgsi",     DEBUG_TGSI, NULL },
   { "pipe",     DEBUG_PIPE, NULL },
   { "state",    DEBUG_STATE, NULL },
   { "screen",   DEBUG_SCREEN, NULL },
   { "tex",      DEBUG_TEX, NULL },
   { "swtnl",    DEBUG_SWTNL, NULL },
   { "const",    DEBUG_CONSTS, NULL },
   { "viewport", DEBUG_VIEWPORT, NULL },
   { "views",    DEBUG_VIEWS, NULL },
   { "perf",     DEBUG_PERF, NULL },
   { "flush",    DEBUG_FLUSH, NULL },
   { "sync",     DEBUG_SYNC, NULL },
   { "cache",    DEBUG_CACHE, NULL },
   DEBUG_NAMED_VALUE_END
};
#endif

static const char *
svga_get_vendor( struct pipe_screen *pscreen )
{
   return "VMware, Inc.";
}


static const char *
svga_get_name( struct pipe_screen *pscreen )
{
#ifdef DEBUG
   /* Only return internal details in the DEBUG version:
    */
   return "SVGA3D; build: DEBUG; mutex: " PIPE_ATOMIC;
#else
   return "SVGA3D; build: RELEASE; ";
#endif
}




static float
svga_get_paramf(struct pipe_screen *screen, enum pipe_cap param)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   SVGA3dDevCapResult result;

   switch (param) {
   case PIPE_CAP_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_LINE_WIDTH_AA:
      return 7.0;

   case PIPE_CAP_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_POINT_WIDTH_AA:
      /* Keep this to a reasonable size to avoid failures in
       * conform/pntaa.c:
       */
      return SVGA_MAX_POINTSIZE;

   case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
      if(!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY, &result))
         return 4.0;
      return result.u;

   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 16.0;

   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return 16;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 16;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_GLSL:
      return svgascreen->use_ps30 && svgascreen->use_vs30;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 1;
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      if(!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_RENDER_TARGETS, &result))
         return 1;
      if(!result.u)
         return 1;
      return MIN2(result.u, PIPE_MAX_COLOR_BUFS);
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_TIMER_QUERY:
      return 0;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;

   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      {
         unsigned levels = SVGA_MAX_TEXTURE_LEVELS;
         if (sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH, &result))
            levels = MIN2(util_logbase2(result.u) + 1, levels);
         else
            levels = 12 /* 2048x2048 */;
         if (sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT, &result))
            levels = MIN2(util_logbase2(result.u) + 1, levels);
         else
            levels = 12 /* 2048x2048 */;
         return levels;
      }

   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VOLUME_EXTENT, &result))
         return 8;  /* max 128x128x128 */
      return MIN2(util_logbase2(result.u) + 1, SVGA_MAX_TEXTURE_LEVELS);

   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      /*
       * No mechanism to query the host, and at least limited to 2048x2048 on
       * certain hardware.
       */
      return MIN2(screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS),
                  12.0 /* 2048x2048 */);

   case PIPE_CAP_TEXTURE_MIRROR_REPEAT: /* req. for GL 1.4 */
      return 1;

   case PIPE_CAP_BLEND_EQUATION_SEPARATE: /* req. for GL 1.5 */
      return 1;

   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;

   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
      return 1;
   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
      return 0;

   default:
      return 0;
   }
}


/* This is a fairly pointless interface
 */
static int
svga_get_param(struct pipe_screen *screen, enum pipe_cap param)
{
   return (int) svga_get_paramf( screen, param );
}

static int svga_get_shader_param(struct pipe_screen *screen, unsigned shader, enum pipe_shader_cap param)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   SVGA3dDevCapResult result;

   switch (shader)
   {
   case PIPE_SHADER_FRAGMENT:
      switch (param)
      {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         return svgascreen->use_ps30 ? 512 : 96;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return SVGA3D_MAX_NESTING_LEVEL;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return 10;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return svgascreen->use_ps30 ? 224 : 16;
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return 1;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS, &result))
            return svgascreen->use_ps30 ? 32 : 12;
         return result.u;
      case PIPE_SHADER_CAP_MAX_ADDRS:
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	 /* 
	  * Although PS 3.0 has some addressing abilities it can only represent
	  * loops that can be statically determined and unrolled. Given we can
	  * only handle a subset of the cases that the state tracker already
	  * does it is better to defer loop unrolling to the state tracker.
	  */
         return 0;
      case PIPE_SHADER_CAP_MAX_PREDS:
         return svgascreen->use_ps30 ? 1 : 0;
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
         return 1;
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         return 0;
      case PIPE_SHADER_CAP_SUBROUTINES:
         return 0;
      }
      break;
   case PIPE_SHADER_VERTEX:
      switch (param)
      {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS, &result))
            return svgascreen->use_vs30 ? 512 : 256;
         return result.u;
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         /* XXX: until we have vertex texture support */
         return 0;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return SVGA3D_MAX_NESTING_LEVEL;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return 16;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return 256;
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return 1;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS, &result))
            return svgascreen->use_vs30 ? 32 : 12;
         return result.u;
      case PIPE_SHADER_CAP_MAX_ADDRS:
         return svgascreen->use_vs30 ? 1 : 0;
      case PIPE_SHADER_CAP_MAX_PREDS:
         return svgascreen->use_vs30 ? 1 : 0;
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
         return 1;
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
         return svgascreen->use_vs30 ? 1 : 0;
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
         return 0;
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         return 1;
      case PIPE_SHADER_CAP_SUBROUTINES:
         return 0;
      default:
         break;
      }
      break;
   default:
      break;
   }
   return 0;
}

static INLINE SVGA3dDevCapIndex
svga_translate_format_cap(enum pipe_format format)
{
   switch(format) {
   
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8;

   case PIPE_FORMAT_B5G6R5_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_R5G6B5;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4;

   case PIPE_FORMAT_Z16_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_Z_D16;
   case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8;
   case PIPE_FORMAT_X8Z24_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8;

   case PIPE_FORMAT_A8_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_ALPHA8;
   case PIPE_FORMAT_L8_UNORM:
      return SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8;

   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
      return SVGA3D_DEVCAP_SURFACEFMT_DXT1;
   case PIPE_FORMAT_DXT3_RGBA:
      return SVGA3D_DEVCAP_SURFACEFMT_DXT3;
   case PIPE_FORMAT_DXT5_RGBA:
      return SVGA3D_DEVCAP_SURFACEFMT_DXT5;

   default:
      return SVGA3D_DEVCAP_MAX;
   }
}


static boolean
svga_is_format_supported( struct pipe_screen *screen,
                          enum pipe_format format,
                          enum pipe_texture_target target,
                          unsigned sample_count,
                          unsigned tex_usage)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;
   SVGA3dDevCapIndex index;
   SVGA3dDevCapResult result;

   assert(tex_usage);

   if (sample_count > 1)
      return FALSE;

   /* Override host capabilities */
   if (tex_usage & PIPE_BIND_RENDER_TARGET) {
      switch(format) { 

      /* Often unsupported/problematic. This means we end up with the same
       * visuals for all virtual hardware implementations.
       */
      case PIPE_FORMAT_B4G4R4A4_UNORM:
      case PIPE_FORMAT_B5G5R5A1_UNORM:
         return FALSE;
         
      default:
         break;
      }
   }
   
   /* Try to query the host */
   index = svga_translate_format_cap(format);
   if( index < SVGA3D_DEVCAP_MAX && 
       sws->get_cap(sws, index, &result) )
   {
      SVGA3dSurfaceFormatCaps mask;
      
      mask.value = 0;
      if (tex_usage & PIPE_BIND_RENDER_TARGET)
         mask.offscreenRenderTarget = 1;
      if (tex_usage & PIPE_BIND_DEPTH_STENCIL)
         mask.zStencil = 1;
      if (tex_usage & PIPE_BIND_SAMPLER_VIEW)
         mask.texture = 1;

      if ((result.u & mask.value) == mask.value)
         return TRUE;
      else
         return FALSE;
   }

   /* Use our translate functions directly rather than relying on a
    * duplicated list of supported formats which is prone to getting
    * out of sync:
    */
   if(tex_usage & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL))
      return svga_translate_format_render(format) != SVGA3D_FORMAT_INVALID;
   else
      return svga_translate_format(format) != SVGA3D_FORMAT_INVALID;
}


static void
svga_fence_reference(struct pipe_screen *screen,
                     struct pipe_fence_handle **ptr,
                     struct pipe_fence_handle *fence)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;
   sws->fence_reference(sws, ptr, fence);
}


static boolean
svga_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;
   return sws->fence_signalled(sws, fence, 0) == 0;
}


static boolean
svga_fence_finish(struct pipe_screen *screen,
                  struct pipe_fence_handle *fence,
                  uint64_t timeout)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;

   SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "%s fence_ptr %p\n",
            __FUNCTION__, fence);

   return sws->fence_finish(sws, fence, 0) == 0;
}


static void
svga_destroy_screen( struct pipe_screen *screen )
{
   struct svga_screen *svgascreen = svga_screen(screen);
   
   svga_screen_cache_cleanup(svgascreen);

   pipe_mutex_destroy(svgascreen->swc_mutex);
   pipe_mutex_destroy(svgascreen->tex_mutex);

   svgascreen->sws->destroy(svgascreen->sws);
   
   FREE(svgascreen);
}


/**
 * Create a new svga_screen object
 */
struct pipe_screen *
svga_screen_create(struct svga_winsys_screen *sws)
{
   struct svga_screen *svgascreen;
   struct pipe_screen *screen;
   SVGA3dDevCapResult result;

#ifdef DEBUG
   SVGA_DEBUG = debug_get_flags_option("SVGA_DEBUG", svga_debug_flags, 0 );
#endif

   svgascreen = CALLOC_STRUCT(svga_screen);
   if (!svgascreen)
      goto error1;

   svgascreen->debug.force_level_surface_view =
      debug_get_bool_option("SVGA_FORCE_LEVEL_SURFACE_VIEW", FALSE);
   svgascreen->debug.force_surface_view =
      debug_get_bool_option("SVGA_FORCE_SURFACE_VIEW", FALSE);
   svgascreen->debug.force_sampler_view =
      debug_get_bool_option("SVGA_FORCE_SAMPLER_VIEW", FALSE);
   svgascreen->debug.no_surface_view =
      debug_get_bool_option("SVGA_NO_SURFACE_VIEW", FALSE);
   svgascreen->debug.no_sampler_view =
      debug_get_bool_option("SVGA_NO_SAMPLER_VIEW", FALSE);

   screen = &svgascreen->screen;

   screen->destroy = svga_destroy_screen;
   screen->get_name = svga_get_name;
   screen->get_vendor = svga_get_vendor;
   screen->get_param = svga_get_param;
   screen->get_shader_param = svga_get_shader_param;
   screen->get_paramf = svga_get_paramf;
   screen->is_format_supported = svga_is_format_supported;
   screen->context_create = svga_context_create;
   screen->fence_reference = svga_fence_reference;
   screen->fence_signalled = svga_fence_signalled;
   screen->fence_finish = svga_fence_finish;
   svgascreen->sws = sws;

   svga_init_screen_resource_functions(svgascreen);

   if (sws->get_hw_version) {
      svgascreen->hw_version = sws->get_hw_version(sws);
   } else {
      svgascreen->hw_version = SVGA3D_HWVERSION_WS65_B1;
   }

   svgascreen->use_ps30 =
      sws->get_cap(sws, SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION, &result) &&
      result.u >= SVGA3DPSVERSION_30 ? TRUE : FALSE;

   svgascreen->use_vs30 =
      sws->get_cap(sws, SVGA3D_DEVCAP_VERTEX_SHADER_VERSION, &result) &&
      result.u >= SVGA3DVSVERSION_30 ? TRUE : FALSE;

#if 1
   /* Shader model 2.0 is unsupported at the moment. */
   if(!svgascreen->use_ps30 || !svgascreen->use_vs30)
      goto error2;
#else
   if(debug_get_bool_option("SVGA_NO_SM30", FALSE))
      svgascreen->use_vs30 = svgascreen->use_ps30 = FALSE;
#endif

   pipe_mutex_init(svgascreen->tex_mutex);
   pipe_mutex_init(svgascreen->swc_mutex);

   svga_screen_cache_init(svgascreen);

   return screen;
error2:
   FREE(svgascreen);
error1:
   return NULL;
}

struct svga_winsys_screen *
svga_winsys_screen(struct pipe_screen *screen)
{
   return svga_screen(screen)->sws;
}

#ifdef DEBUG
struct svga_screen *
svga_screen(struct pipe_screen *screen)
{
   assert(screen);
   assert(screen->destroy == svga_destroy_screen);
   return (struct svga_screen *)screen;
}
#endif
