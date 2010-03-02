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
#include "util/u_simple_screen.h"
#include "util/u_simple_screen.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "sp_texture.h"
#include "sp_winsys.h"
#include "sp_screen.h"
#include "sp_context.h"


static const char *
softpipe_get_vendor(struct pipe_screen *screen)
{
   return "VMware, Inc.";
}


static const char *
softpipe_get_name(struct pipe_screen *screen)
{
   return "softpipe";
}


static int
softpipe_get_param(struct pipe_screen *screen, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return PIPE_MAX_SAMPLERS;
   case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
      return PIPE_MAX_VERTEX_SAMPLERS;
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
   case PIPE_CAP_MAX_CONST_BUFFERS:
      return PIPE_MAX_CONSTANT_BUFFERS;
   case PIPE_CAP_MAX_CONST_BUFFER_SIZE:
      return 4096 * 4 * sizeof(float);
   case PIPE_CAP_INDEP_BLEND_ENABLE:
      return 1;
   case PIPE_CAP_INDEP_BLEND_FUNC:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 1;
   default:
      return 0;
   }
}


static float
softpipe_get_paramf(struct pipe_screen *screen, int param)
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
softpipe_is_format_supported( struct pipe_screen *screen,
                              enum pipe_format format, 
                              enum pipe_texture_target target,
                              unsigned tex_usage, 
                              unsigned geom_flags )
{
   assert(target == PIPE_TEXTURE_1D ||
          target == PIPE_TEXTURE_2D ||
          target == PIPE_TEXTURE_3D ||
          target == PIPE_TEXTURE_CUBE);

   switch(format) {
   case PIPE_FORMAT_L16_UNORM:
   case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_UYVY:
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
   case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_R8G8_SNORM:
   case PIPE_FORMAT_R5SG5SB6U_NORM:
   case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
   case PIPE_FORMAT_R8G8B8A8_SNORM:
   case PIPE_FORMAT_NONE:
      return FALSE;
   default:
      return TRUE;
   }
}


static void
softpipe_destroy_screen( struct pipe_screen *screen )
{
   struct pipe_winsys *winsys = screen->winsys;

   if(winsys->destroy)
      winsys->destroy(winsys);

   FREE(screen);
}



/**
 * Create a new pipe_screen object
 * Note: we're not presently subclassing pipe_screen (no softpipe_screen).
 */
struct pipe_screen *
softpipe_create_screen(struct pipe_winsys *winsys)
{
   struct softpipe_screen *screen = CALLOC_STRUCT(softpipe_screen);

   if (!screen)
      return NULL;

   screen->base.winsys = winsys;

   screen->base.destroy = softpipe_destroy_screen;

   screen->base.get_name = softpipe_get_name;
   screen->base.get_vendor = softpipe_get_vendor;
   screen->base.get_param = softpipe_get_param;
   screen->base.get_paramf = softpipe_get_paramf;
   screen->base.is_format_supported = softpipe_is_format_supported;
   screen->base.context_create = softpipe_create_context;

   softpipe_init_screen_texture_funcs(&screen->base);
   u_simple_screen_init(&screen->base);

   return &screen->base;
}
