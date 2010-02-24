/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "util/u_memory.h"
#include "util/u_format.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "lp_texture.h"
#include "lp_buffer.h"
#include "lp_fence.h"
#include "lp_winsys.h"
#include "lp_jit.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_debug.h"

#ifdef DEBUG
int LP_DEBUG = 0;

static const struct debug_named_value lp_debug_flags[] = {
   { "pipe",   DEBUG_PIPE },
   { "tgsi",   DEBUG_TGSI },
   { "tex",    DEBUG_TEX },
   { "asm",    DEBUG_ASM },
   { "setup",  DEBUG_SETUP },
   { "rast",   DEBUG_RAST },
   { "query",  DEBUG_QUERY },
   { "screen", DEBUG_SCREEN },
   { "jit",    DEBUG_JIT },
   { "show_tiles",    DEBUG_SHOW_TILES },
   { "show_subtiles", DEBUG_SHOW_SUBTILES },
   { "counters", DEBUG_COUNTERS },
   { "nopt", DEBUG_NO_LLVM_OPT },
   {NULL, 0}
};
#endif


static const char *
llvmpipe_get_vendor(struct pipe_screen *screen)
{
   return "VMware, Inc.";
}


static const char *
llvmpipe_get_name(struct pipe_screen *screen)
{
   return "llvmpipe";
}


static int
llvmpipe_get_param(struct pipe_screen *screen, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return PIPE_MAX_SAMPLERS;
   case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
      return 0;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return PIPE_MAX_SAMPLERS + PIPE_MAX_VERTEX_SAMPLERS;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_GLSL:
      return 1;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 0;
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return PIPE_MAX_COLOR_BUFS;
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
      return 1;
   case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
      return 1;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 13; /* max 4Kx4K */
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 9;  /* max 256x256x256 */
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 13; /* max 4Kx4K */
   case PIPE_CAP_TGSI_CONT_SUPPORTED:
      return 1;
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
      return 1;
   case PIPE_CAP_INDEP_BLEND_ENABLE:
      return 0;
   case PIPE_CAP_INDEP_BLEND_FUNC:
      return 0;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 0;
   default:
      return 0;
   }
}


static float
llvmpipe_get_paramf(struct pipe_screen *screen, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_LINE_WIDTH_AA:
      return 255.0; /* arbitrary */
   case PIPE_CAP_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_POINT_WIDTH_AA:
      return 255.0; /* arbitrary */
   case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
      return 16.0; /* not actually signficant at this time */
   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 16.0; /* arbitrary */
   default:
      return 0;
   }
}


/**
 * Query format support for creating a texture, drawing surface, etc.
 * \param format  the format to test
 * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
 */
static boolean
llvmpipe_is_format_supported( struct pipe_screen *_screen,
                              enum pipe_format format, 
                              enum pipe_texture_target target,
                              unsigned tex_usage, 
                              unsigned geom_flags )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_winsys *winsys = screen->winsys;
   const struct util_format_description *format_desc;

   format_desc = util_format_description(format);
   if(!format_desc)
      return FALSE;

   assert(target == PIPE_TEXTURE_1D ||
          target == PIPE_TEXTURE_2D ||
          target == PIPE_TEXTURE_3D ||
          target == PIPE_TEXTURE_CUBE);

   switch(format) {
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
      return FALSE;
   default:
      break;
   }

   if(tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET) {
      if(format_desc->block.width != 1 ||
         format_desc->block.height != 1)
         return FALSE;

      if(format_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
         return FALSE;

      if(format_desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB &&
         format_desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB)
         return FALSE;
   }

   if(tex_usage & PIPE_TEXTURE_USAGE_DISPLAY_TARGET) {
      if(!winsys->is_displaytarget_format_supported(winsys, format))
         return FALSE;
   }

   if(tex_usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL) {
      if(format_desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
         return FALSE;

      /* FIXME: Temporary restriction. See lp_state_fs.c. */
      if(format_desc->block.bits != 32)
         return FALSE;
   }

   /* FIXME: Temporary restrictions. See lp_bld_sample_soa.c */
   if(tex_usage & PIPE_TEXTURE_USAGE_SAMPLER) {
      if(format_desc->block.width != 1 ||
         format_desc->block.height != 1)
         return FALSE;

      if(format_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
         return FALSE;

      if(format_desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB &&
         format_desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
         return FALSE;

      /* not supported yet */
      if (format == PIPE_FORMAT_Z16_UNORM)
         return FALSE;
   }

   return TRUE;
}


static struct pipe_buffer *
llvmpipe_surface_buffer_create(struct pipe_screen *screen,
                               unsigned width, unsigned height,
                               enum pipe_format format,
                               unsigned tex_usage,
                               unsigned usage,
                               unsigned *stride)
{
   /* This function should never be used */
   assert(0);
   return NULL;
}


static void
llvmpipe_flush_frontbuffer(struct pipe_screen *_screen,
                           struct pipe_surface *surface,
                           void *context_private)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_winsys *winsys = screen->winsys;
   struct llvmpipe_texture *texture = llvmpipe_texture(surface->texture);

   assert(texture->dt);
   if (texture->dt)
      winsys->displaytarget_display(winsys, texture->dt, context_private);
}


static void
llvmpipe_destroy_screen( struct pipe_screen *_screen )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_winsys *winsys = screen->winsys;

   lp_jit_screen_cleanup(screen);

   if(winsys->destroy)
      winsys->destroy(winsys);

   FREE(screen);
}



/**
 * Create a new pipe_screen object
 * Note: we're not presently subclassing pipe_screen (no llvmpipe_screen).
 */
struct pipe_screen *
llvmpipe_create_screen(struct llvmpipe_winsys *winsys)
{
   struct llvmpipe_screen *screen = CALLOC_STRUCT(llvmpipe_screen);

#ifdef DEBUG
   LP_DEBUG = debug_get_flags_option("LP_DEBUG", lp_debug_flags, 0 );
#endif

   if (!screen)
      return NULL;

   screen->winsys = winsys;

   screen->base.destroy = llvmpipe_destroy_screen;

   screen->base.get_name = llvmpipe_get_name;
   screen->base.get_vendor = llvmpipe_get_vendor;
   screen->base.get_param = llvmpipe_get_param;
   screen->base.get_paramf = llvmpipe_get_paramf;
   screen->base.is_format_supported = llvmpipe_is_format_supported;

   screen->base.surface_buffer_create = llvmpipe_surface_buffer_create;
   screen->base.context_create = llvmpipe_create_context;
   screen->base.flush_frontbuffer = llvmpipe_flush_frontbuffer;

   llvmpipe_init_screen_texture_funcs(&screen->base);
   llvmpipe_init_screen_buffer_funcs(&screen->base);
   llvmpipe_init_screen_fence_funcs(&screen->base);

   lp_jit_screen_init(screen);

   return &screen->base;
}
