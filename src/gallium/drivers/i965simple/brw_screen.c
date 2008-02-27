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


#include "pipe/p_util.h"
#include "pipe/p_winsys.h"

#include "brw_context.h"
#include "brw_screen.h"
#include "brw_tex_layout.h"


static const char *
brw_get_vendor( struct pipe_screen *screen )
{
   return "Tungsten Graphics, Inc.";
}


static const char *
brw_get_name( struct pipe_screen *screen )
{
   static char buffer[128];
   const char *chipset;

   switch (brw_screen(screen)->pci_id) {
   case PCI_CHIP_I965_Q:
      chipset = "Intel(R) 965Q";
      break;
   case PCI_CHIP_I965_G:
   case PCI_CHIP_I965_G_1:
      chipset = "Intel(R) 965G";
      break;
   case PCI_CHIP_I965_GM:
      chipset = "Intel(R) 965GM";
      break;
   case PCI_CHIP_I965_GME:
      chipset = "Intel(R) 965GME/GLE";
      break;
   default:
      chipset = "unknown";
      break;
   }

   sprintf(buffer, "i965 (chipset: %s)", chipset);
   return buffer;
}


static int
brw_get_param(struct pipe_screen *screen, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return 8;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_GLSL:
      return 0;
   case PIPE_CAP_S3TC:
      return 0;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 0;
   case PIPE_CAP_POINT_SPRITE:
      return 0;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return 1;
   case PIPE_CAP_OCCLUSION_QUERY:
      return 0;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 11; /* max 1024x1024 */
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 8;  /* max 128x128x128 */
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 11; /* max 1024x1024 */
   default:
      return 0;
   }
}


static float
brw_get_paramf(struct pipe_screen *screen, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_LINE_WIDTH_AA:
      return 7.5;

   case PIPE_CAP_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_POINT_WIDTH_AA:
      return 255.0;

   case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
      return 4.0;

   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 16.0;

   default:
      return 0;
   }
}


static void
brw_destroy_screen( struct pipe_screen *screen )
{
   FREE(screen);
}


/**
 * Create a new brw_screen object
 */
struct pipe_screen *
brw_create_screen(struct pipe_winsys *winsys, uint pci_id)
{
   struct brw_screen *brwscreen = CALLOC_STRUCT(brw_screen);

   if (!brwscreen)
      return NULL;

   brwscreen->pci_id = pci_id;

   brwscreen->screen.winsys = winsys;

   brwscreen->screen.destroy = brw_destroy_screen;

   brwscreen->screen.get_name = brw_get_name;
   brwscreen->screen.get_vendor = brw_get_vendor;
   brwscreen->screen.get_param = brw_get_param;
   brwscreen->screen.get_paramf = brw_get_paramf;

   brw_init_screen_texture_funcs(&brwscreen->screen);

   return &brwscreen->screen;
}
