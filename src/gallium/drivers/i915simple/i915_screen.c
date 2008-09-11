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
#include "pipe/p_winsys.h"
#include "pipe/p_inlines.h"
#include "util/u_string.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_screen.h"
#include "i915_texture.h"


static const char *
i915_get_vendor( struct pipe_screen *pscreen )
{
   return "Tungsten Graphics, Inc.";
}


static const char *
i915_get_name( struct pipe_screen *pscreen )
{
   static char buffer[128];
   const char *chipset;

   switch (i915_screen(pscreen)->pci_id) {
   case PCI_CHIP_I915_G:
      chipset = "915G";
      break;
   case PCI_CHIP_I915_GM:
      chipset = "915GM";
      break;
   case PCI_CHIP_I945_G:
      chipset = "945G";
      break;
   case PCI_CHIP_I945_GM:
      chipset = "945GM";
      break;
   case PCI_CHIP_I945_GME:
      chipset = "945GME";
      break;
   case PCI_CHIP_G33_G:
      chipset = "G33";
      break;
   case PCI_CHIP_Q35_G:
      chipset = "Q35";
      break;
   case PCI_CHIP_Q33_G:
      chipset = "Q33";
      break;
   default:
      chipset = "unknown";
      break;
   }

   util_snprintf(buffer, sizeof(buffer), "i915 (chipset: %s)", chipset);
   return buffer;
}


static int
i915_get_param(struct pipe_screen *screen, int param)
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
i915_get_paramf(struct pipe_screen *screen, int param)
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


static boolean
i915_is_format_supported( struct pipe_screen *screen,
                          enum pipe_format format, 
                          enum pipe_texture_target target,
                          unsigned tex_usage, 
                          unsigned geom_flags )
{
   static const enum pipe_format tex_supported[] = {
      PIPE_FORMAT_R8G8B8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_R5G6B5_UNORM,
      PIPE_FORMAT_L8_UNORM,
      PIPE_FORMAT_A8_UNORM,
      PIPE_FORMAT_I8_UNORM,
      PIPE_FORMAT_A8L8_UNORM,
      PIPE_FORMAT_YCBCR,
      PIPE_FORMAT_YCBCR_REV,
      PIPE_FORMAT_S8Z24_UNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   static const enum pipe_format surface_supported[] = {
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_R5G6B5_UNORM,
      PIPE_FORMAT_S8Z24_UNORM,
      /*PIPE_FORMAT_R16G16B16A16_SNORM,*/
      PIPE_FORMAT_NONE  /* list terminator */
   };
   const enum pipe_format *list;
   uint i;

   if(tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET)
      list = surface_supported;
   else
      list = tex_supported;

   for (i = 0; list[i] != PIPE_FORMAT_NONE; i++) {
      if (list[i] == format)
         return TRUE;
   }

   return FALSE;
}


static void
i915_destroy_screen( struct pipe_screen *screen )
{
   struct pipe_winsys *winsys = screen->winsys;

   if(winsys->destroy)
      winsys->destroy(winsys);

   FREE(screen);
}


static void *
i915_surface_map( struct pipe_screen *screen,
                  struct pipe_surface *surface,
                  unsigned flags )
{
   char *map = pipe_buffer_map( screen, surface->buffer, flags );
   if (map == NULL)
      return NULL;

   if (surface->texture &&
       (flags & PIPE_BUFFER_USAGE_CPU_WRITE)) 
   {
      /* Do something to notify contexts of a texture change.  
       */
      /* i915_screen(screen)->timestamp++; */
   }
   
   return map + surface->offset;
}

static void
i915_surface_unmap(struct pipe_screen *screen,
                   struct pipe_surface *surface)
{
   pipe_buffer_unmap( screen, surface->buffer );
}



/**
 * Create a new i915_screen object
 */
struct pipe_screen *
i915_create_screen(struct pipe_winsys *winsys, uint pci_id)
{
   struct i915_screen *i915screen = CALLOC_STRUCT(i915_screen);

   if (!i915screen)
      return NULL;

   switch (pci_id) {
   case PCI_CHIP_I915_G:
   case PCI_CHIP_I915_GM:
      i915screen->is_i945 = FALSE;
      break;

   case PCI_CHIP_I945_G:
   case PCI_CHIP_I945_GM:
   case PCI_CHIP_I945_GME:
   case PCI_CHIP_G33_G:
   case PCI_CHIP_Q33_G:
   case PCI_CHIP_Q35_G:
      i915screen->is_i945 = TRUE;
      break;

   default:
      debug_printf("%s: unknown pci id 0x%x, cannot create screen\n", 
                   __FUNCTION__, pci_id);
      return NULL;
   }

   i915screen->pci_id = pci_id;

   i915screen->screen.winsys = winsys;

   i915screen->screen.destroy = i915_destroy_screen;

   i915screen->screen.get_name = i915_get_name;
   i915screen->screen.get_vendor = i915_get_vendor;
   i915screen->screen.get_param = i915_get_param;
   i915screen->screen.get_paramf = i915_get_paramf;
   i915screen->screen.is_format_supported = i915_is_format_supported;
   i915screen->screen.surface_map = i915_surface_map;
   i915screen->screen.surface_unmap = i915_surface_unmap;

   i915_init_screen_texture_functions(&i915screen->screen);

   return &i915screen->screen;
}
