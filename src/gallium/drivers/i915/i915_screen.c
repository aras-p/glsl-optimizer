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


#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_string.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_screen.h"
#include "i915_buffer.h"
#include "i915_texture.h"
#include "intel_winsys.h"


/*
 * Probe functions
 */


static const char *
i915_get_vendor(struct pipe_screen *screen)
{
   return "VMware, Inc.";
}

static const char *
i915_get_name(struct pipe_screen *screen)
{
   static char buffer[128];
   const char *chipset;

   switch (i915_screen(screen)->pci_id) {
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
   case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
      return 0;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 8;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_GLSL:
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
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;
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
i915_is_format_supported(struct pipe_screen *screen,
                         enum pipe_format format, 
                         enum pipe_texture_target target,
                         unsigned tex_usage, 
                         unsigned geom_flags)
{
   static const enum pipe_format tex_supported[] = {
      PIPE_FORMAT_A8B8G8R8_UNORM,
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM,
      PIPE_FORMAT_L8_UNORM,
      PIPE_FORMAT_A8_UNORM,
      PIPE_FORMAT_I8_UNORM,
      PIPE_FORMAT_L8A8_UNORM,
      PIPE_FORMAT_UYVY,
      PIPE_FORMAT_YUYV,
      PIPE_FORMAT_Z24S8_UNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   static const enum pipe_format surface_supported[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM,
      PIPE_FORMAT_Z24S8_UNORM,
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


/*
 * Fence functions
 */


static void
i915_fence_reference(struct pipe_screen *screen,
                     struct pipe_fence_handle **ptr,
                     struct pipe_fence_handle *fence)
{
   struct i915_screen *is = i915_screen(screen);

   is->iws->fence_reference(is->iws, ptr, fence);
}

static int
i915_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence,
                     unsigned flags)
{
   struct i915_screen *is = i915_screen(screen);

   return is->iws->fence_signalled(is->iws, fence);
}

static int
i915_fence_finish(struct pipe_screen *screen,
                  struct pipe_fence_handle *fence,
                  unsigned flags)
{
   struct i915_screen *is = i915_screen(screen);

   return is->iws->fence_finish(is->iws, fence);
}


/*
 * Generic functions
 */


static void
i915_destroy_screen(struct pipe_screen *screen)
{
   struct i915_screen *is = i915_screen(screen);

   if (is->iws)
      is->iws->destroy(is->iws);

   FREE(is);
}

/**
 * Create a new i915_screen object
 */
struct pipe_screen *
i915_create_screen(struct intel_winsys *iws, uint pci_id)
{
   struct i915_screen *is = CALLOC_STRUCT(i915_screen);

   if (!is)
      return NULL;

   switch (pci_id) {
   case PCI_CHIP_I915_G:
   case PCI_CHIP_I915_GM:
      is->is_i945 = FALSE;
      break;

   case PCI_CHIP_I945_G:
   case PCI_CHIP_I945_GM:
   case PCI_CHIP_I945_GME:
   case PCI_CHIP_G33_G:
   case PCI_CHIP_Q33_G:
   case PCI_CHIP_Q35_G:
      is->is_i945 = TRUE;
      break;

   default:
      debug_printf("%s: unknown pci id 0x%x, cannot create screen\n", 
                   __FUNCTION__, pci_id);
      FREE(is);
      return NULL;
   }

   is->pci_id = pci_id;
   is->iws = iws;

   is->base.winsys = NULL;

   is->base.destroy = i915_destroy_screen;

   is->base.get_name = i915_get_name;
   is->base.get_vendor = i915_get_vendor;
   is->base.get_param = i915_get_param;
   is->base.get_paramf = i915_get_paramf;
   is->base.is_format_supported = i915_is_format_supported;

   is->base.context_create = i915_create_context;

   is->base.fence_reference = i915_fence_reference;
   is->base.fence_signalled = i915_fence_signalled;
   is->base.fence_finish = i915_fence_finish;

   i915_init_screen_texture_functions(is);
   i915_init_screen_buffer_functions(is);

   return &is->base;
}
