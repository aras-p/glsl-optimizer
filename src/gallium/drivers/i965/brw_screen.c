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


#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "util/u_string.h"

#include "brw_reg.h"
#include "brw_context.h"
#include "brw_screen.h"
#include "brw_buffer.h"
#include "brw_texture.h"
#include "brw_winsys.h"

#ifdef DEBUG
static const struct debug_named_value debug_names[] = {
   { "tex",   DEBUG_TEXTURE},
   { "state", DEBUG_STATE},
   { "ioctl", DEBUG_IOCTL},
   { "blit",  DEBUG_BLIT},
   { "curbe", DEBUG_CURBE},
   { "fall",  DEBUG_FALLBACKS},
   { "verb",  DEBUG_VERBOSE},
   { "bat",   DEBUG_BATCH},
   { "pix",   DEBUG_PIXEL},
   { "buf",   DEBUG_BUFMGR},
   { "reg",   DEBUG_REGION},
   { "fbo",   DEBUG_FBO},
   { "lock",  DEBUG_LOCK},
   { "sync",  DEBUG_SYNC},
   { "prim",  DEBUG_PRIMS },
   { "vert",  DEBUG_VERTS },
   { "dri",   DEBUG_DRI },
   { "dma",   DEBUG_DMA },
   { "san",   DEBUG_SANITY },
   { "sleep", DEBUG_SLEEP },
   { "stats", DEBUG_STATS },
   { "tile",  DEBUG_TILE },
   { "sing",  DEBUG_SINGLE_THREAD },
   { "thre",  DEBUG_SINGLE_THREAD },
   { "wm",    DEBUG_WM },
   { "urb",   DEBUG_URB },
   { "vs",    DEBUG_VS },
   { NULL,    0 }
};

int BRW_DEBUG = 0;
#endif


/*
 * Probe functions
 */


static const char *
brw_get_vendor(struct pipe_screen *screen)
{
   return "VMware, Inc.";
}

static const char *
brw_get_name(struct pipe_screen *screen)
{
   static char buffer[128];
   const char *chipset;

   switch (brw_screen(screen)->pci_id) {
   case PCI_CHIP_I965_G:
      chipset = "I965_G";
      break;
   case PCI_CHIP_I965_Q:
      chipset = "I965_Q";
      break;
   case PCI_CHIP_I965_G_1:
      chipset = "I965_G_1";
      break;
   case PCI_CHIP_I946_GZ:
      chipset = "I946_GZ";
      break;
   case PCI_CHIP_I965_GM:
      chipset = "I965_GM";
      break;
   case PCI_CHIP_I965_GME:
      chipset = "I965_GME";
      break;
   case PCI_CHIP_GM45_GM:
      chipset = "GM45_GM";
      break;
   case PCI_CHIP_IGD_E_G:
      chipset = "IGD_E_G";
      break;
   case PCI_CHIP_Q45_G:
      chipset = "Q45_G";
      break;
   case PCI_CHIP_G45_G:
      chipset = "G45_G";
      break;
   case PCI_CHIP_G41_G:
      chipset = "G41_G";
      break;
   case PCI_CHIP_B43_G:
      chipset = "B43_G";
      break;
   case PCI_CHIP_ILD_G:
      chipset = "ILD_G";
      break;
   case PCI_CHIP_ILM_G:
      chipset = "ILM_G";
      break;
   }

   util_snprintf(buffer, sizeof(buffer), "i965 (chipset: %s)", chipset);
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

static boolean
brw_is_format_supported(struct pipe_screen *screen,
                         enum pipe_format format, 
                         enum pipe_texture_target target,
                         unsigned tex_usage, 
                         unsigned geom_flags)
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
brw_fence_reference(struct pipe_screen *screen,
                     struct pipe_fence_handle **ptr,
                     struct pipe_fence_handle *fence)
{
   struct brw_screen *is = brw_screen(screen);

   is->iws->fence_reference(is->iws, ptr, fence);
}

static int
brw_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence,
                     unsigned flags)
{
   struct brw_screen *is = brw_screen(screen);

   return is->iws->fence_signalled(is->iws, fence);
}

static int
brw_fence_finish(struct pipe_screen *screen,
                  struct pipe_fence_handle *fence,
                  unsigned flags)
{
   struct brw_screen *is = brw_screen(screen);

   return is->iws->fence_finish(is->iws, fence);
}


/*
 * Generic functions
 */


static void
brw_destroy_screen(struct pipe_screen *screen)
{
   struct brw_screen *is = brw_screen(screen);

   if (is->iws)
      is->iws->destroy(is->iws);

   FREE(is);
}

/**
 * Create a new brw_screen object
 */
struct pipe_screen *
brw_create_screen(struct intel_winsys *iws, uint pci_id)
{
   struct brw_screen *is;
   struct brw_chipset chipset;

#ifdef DEBUG
   BRW_DEBUG = debug_get_flags_option("BRW_DEBUG", debug_names, 0);
   BRW_DEBUG |= debug_get_flags_option("INTEL_DEBUG", debug_names, 0);
#endif

   memset(&chipset, 0, sizeof chipset);

   chipset.pci_id = pci_id;

   switch (pci_id) {
   case PCI_CHIP_I965_G:
   case PCI_CHIP_I965_Q:
   case PCI_CHIP_I965_G_1:
   case PCI_CHIP_I946_GZ:
   case PCI_CHIP_I965_GM:
   case PCI_CHIP_I965_GME:
      chipset.is_965 = TRUE;
      break;

   case PCI_CHIP_GM45_GM:
   case PCI_CHIP_IGD_E_G:
   case PCI_CHIP_Q45_G:
   case PCI_CHIP_G45_G:
   case PCI_CHIP_G41_G:
   case PCI_CHIP_B43_G:
      chipset.is_g4x = TRUE;
      break;

   case PCI_CHIP_ILD_G:
   case PCI_CHIP_ILM_G:
      chipset.is_igdng = TRUE;
      break;

   default:
      debug_printf("%s: unknown pci id 0x%x, cannot create screen\n", 
                   __FUNCTION__, pci_id);
      return NULL;
   }


   is = CALLOC_STRUCT(brw_screen);
   if (!is)
      return NULL;

   is->chipset = chipset;
   is->iws = iws;
   is->base.winsys = NULL;
   is->base.destroy = brw_destroy_screen;
   is->base.get_name = brw_get_name;
   is->base.get_vendor = brw_get_vendor;
   is->base.get_param = brw_get_param;
   is->base.get_paramf = brw_get_paramf;
   is->base.is_format_supported = brw_is_format_supported;
   is->base.fence_reference = brw_fence_reference;
   is->base.fence_signalled = brw_fence_signalled;
   is->base.fence_finish = brw_fence_finish;

   brw_screen_init_texture_functions(is);
   brw_screen_init_buffer_functions(is);

   return &is->base;
}
