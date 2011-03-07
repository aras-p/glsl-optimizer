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
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "cell/common.h"
#include "cell_context.h"
#include "cell_screen.h"
#include "cell_texture.h"
#include "cell_public.h"

#include "state_tracker/sw_winsys.h"


static const char *
cell_get_vendor(struct pipe_screen *screen)
{
   return "VMware, Inc.";
}


static const char *
cell_get_name(struct pipe_screen *screen)
{
   return "Cell";
}


static int
cell_get_param(struct pipe_screen *screen, enum pipe_cap param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return CELL_MAX_SAMPLERS;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return CELL_MAX_SAMPLERS;
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
      return 1;
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_TIMER_QUERY:
      return 0;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 10;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return CELL_MAX_TEXTURE_LEVELS;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 8;  /* max 128x128x128 */
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return CELL_MAX_TEXTURE_LEVELS;
   case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
      return 1; /* XXX not really true */
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
      return 0; /* XXX to do */
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
      return 1;
   default:
      return 0;
   }
}

static int
cell_get_shader_param(struct pipe_screen *screen, unsigned shader, enum pipe_shader_cap param)
{
   switch(shader)
   {
   case PIPE_SHADER_FRAGMENT:
      return tgsi_exec_get_shader_param(param);
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
      return draw_get_shader_param(shader, param);
   default:
      return 0;
   }
}

static float
cell_get_paramf(struct pipe_screen *screen, enum pipe_cap param)
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
      return 0.0;

   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 16.0; /* arbitrary */

   default:
      return 0;
   }
}


static boolean
cell_is_format_supported( struct pipe_screen *screen,
                          enum pipe_format format,
                          enum pipe_texture_target target,
                          unsigned sample_count,
                          unsigned tex_usage)
{
   struct sw_winsys *winsys = cell_screen(screen)->winsys;

   if (sample_count > 1)
      return FALSE;

   if (tex_usage & (PIPE_BIND_DISPLAY_TARGET |
                    PIPE_BIND_SCANOUT |
                    PIPE_BIND_SHARED)) {
      if (!winsys->is_displaytarget_format_supported(winsys, tex_usage, format))
         return FALSE;
   }

   /* only a few formats are known to work at this time */
   switch (format) {
   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
      return TRUE;
   default:
      return FALSE;
   }
}


static void
cell_destroy_screen( struct pipe_screen *screen )
{
   struct cell_screen *sp_screen = cell_screen(screen);
   struct sw_winsys *winsys = sp_screen->winsys;

   if(winsys->destroy)
      winsys->destroy(winsys);

   FREE(screen);
}



/**
 * Create a new pipe_screen object
 * Note: we're not presently subclassing pipe_screen (no cell_screen) but
 * that would be the place to put SPU thread/context info...
 */
struct pipe_screen *
cell_create_screen(struct sw_winsys *winsys)
{
   struct cell_screen *screen = CALLOC_STRUCT(cell_screen);

   if (!screen)
      return NULL;

   screen->winsys = winsys;
   screen->base.winsys = NULL;

   screen->base.destroy = cell_destroy_screen;

   screen->base.get_name = cell_get_name;
   screen->base.get_vendor = cell_get_vendor;
   screen->base.get_param = cell_get_param;
   screen->base.get_shader_param = cell_get_shader_param;
   screen->base.get_paramf = cell_get_paramf;
   screen->base.is_format_supported = cell_is_format_supported;
   screen->base.context_create = cell_create_context;

   cell_init_screen_texture_funcs(&screen->base);

   return &screen->base;
}
