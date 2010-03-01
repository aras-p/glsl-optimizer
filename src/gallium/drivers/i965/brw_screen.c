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

#include "brw_reg.h"
#include "brw_context.h"
#include "brw_screen.h"
#include "brw_winsys.h"
#include "brw_debug.h"

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
   { "wins",  DEBUG_WINSYS},
   { "min",   DEBUG_MIN_URB},
   { "dis",   DEBUG_DISASSEM},
   { "sync",  DEBUG_SYNC},
   { "prim",  DEBUG_PRIMS },
   { "vert",  DEBUG_VERTS },
   { "dma",   DEBUG_DMA },
   { "san",   DEBUG_SANITY },
   { "sleep", DEBUG_SLEEP },
   { "stats", DEBUG_STATS },
   { "sing",  DEBUG_SINGLE_THREAD },
   { "thre",  DEBUG_SINGLE_THREAD },
   { "wm",    DEBUG_WM },
   { "urb",   DEBUG_URB },
   { "vs",    DEBUG_VS },
   { NULL,    0 }
};

static const struct debug_named_value dump_names[] = {
   { "asm",   DUMP_ASM},
   { "state", DUMP_STATE},
   { "batch", DUMP_BATCH},
   { NULL, 0 }
};

int BRW_DEBUG = 0;
int BRW_DUMP = 0;

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

   switch (brw_screen(screen)->chipset.pci_id) {
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
   default:
      chipset = "unknown";
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
   case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
      return 8;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 16; /* XXX correct? */
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
      PIPE_FORMAT_L8_UNORM,
      PIPE_FORMAT_I8_UNORM,
      PIPE_FORMAT_A8_UNORM,
      PIPE_FORMAT_L16_UNORM,
      /*PIPE_FORMAT_I16_UNORM,*/
      /*PIPE_FORMAT_A16_UNORM,*/
      PIPE_FORMAT_L8A8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM,
      PIPE_FORMAT_B5G5R5A1_UNORM,
      PIPE_FORMAT_B4G4R4A4_UNORM,
      PIPE_FORMAT_B8G8R8X8_UNORM,
      PIPE_FORMAT_B8G8R8A8_UNORM,
      /* video */
      PIPE_FORMAT_UYVY,
      PIPE_FORMAT_YUYV,
      /* compressed */
      /*PIPE_FORMAT_FXT1_RGBA,*/
      PIPE_FORMAT_DXT1_RGB,
      PIPE_FORMAT_DXT1_RGBA,
      PIPE_FORMAT_DXT3_RGBA,
      PIPE_FORMAT_DXT5_RGBA,
      /* sRGB */
      PIPE_FORMAT_A8B8G8R8_SRGB,
      PIPE_FORMAT_L8A8_SRGB,
      PIPE_FORMAT_L8_SRGB,
      PIPE_FORMAT_DXT1_SRGB,
      /* depth */
      PIPE_FORMAT_Z32_FLOAT,
      PIPE_FORMAT_Z24X8_UNORM,
      PIPE_FORMAT_Z24S8_UNORM,
      PIPE_FORMAT_Z16_UNORM,
      /* signed */
      PIPE_FORMAT_R8G8_SNORM,
      PIPE_FORMAT_R8G8B8A8_SNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   static const enum pipe_format render_supported[] = {
      PIPE_FORMAT_B8G8R8X8_UNORM,
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   static const enum pipe_format depth_supported[] = {
      PIPE_FORMAT_Z32_FLOAT,
      PIPE_FORMAT_Z24X8_UNORM,
      PIPE_FORMAT_Z24S8_UNORM,
      PIPE_FORMAT_Z16_UNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   const enum pipe_format *list;
   uint i;

   if (tex_usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL)
      list = depth_supported;
   else if (tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET)
      list = render_supported;
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
}

static int
brw_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence,
                     unsigned flags)
{
   return 0;                    /* XXX shouldn't this be a boolean? */
}

static int
brw_fence_finish(struct pipe_screen *screen,
                 struct pipe_fence_handle *fence,
                 unsigned flags)
{
   return 0;
}


/*
 * Generic functions
 */


static void
brw_destroy_screen(struct pipe_screen *screen)
{
   struct brw_screen *bscreen = brw_screen(screen);

   if (bscreen->sws)
      bscreen->sws->destroy(bscreen->sws);

   FREE(bscreen);
}

/**
 * Create a new brw_screen object
 */
struct pipe_screen *
brw_create_screen(struct brw_winsys_screen *sws, uint pci_id)
{
   struct brw_screen *bscreen;
   struct brw_chipset chipset;

#ifdef DEBUG
   BRW_DEBUG = debug_get_flags_option("BRW_DEBUG", debug_names, 0);
   BRW_DEBUG |= debug_get_flags_option("INTEL_DEBUG", debug_names, 0);
   BRW_DEBUG |= DEBUG_STATS | DEBUG_MIN_URB | DEBUG_WM;

   BRW_DUMP = debug_get_flags_option("BRW_DUMP", dump_names, 0);
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


   bscreen = CALLOC_STRUCT(brw_screen);
   if (!bscreen)
      return NULL;

   bscreen->chipset = chipset;
   bscreen->sws = sws;
   bscreen->base.winsys = NULL;
   bscreen->base.destroy = brw_destroy_screen;
   bscreen->base.get_name = brw_get_name;
   bscreen->base.get_vendor = brw_get_vendor;
   bscreen->base.get_param = brw_get_param;
   bscreen->base.get_paramf = brw_get_paramf;
   bscreen->base.is_format_supported = brw_is_format_supported;
   bscreen->base.context_create = brw_create_context;
   bscreen->base.fence_reference = brw_fence_reference;
   bscreen->base.fence_signalled = brw_fence_signalled;
   bscreen->base.fence_finish = brw_fence_finish;

   brw_screen_tex_init(bscreen);
   brw_screen_tex_surface_init(bscreen);
   brw_screen_buffer_init(bscreen);

   bscreen->no_tiling = debug_get_option("BRW_NO_TILING", FALSE) != NULL;
   
   
   return &bscreen->base;
}
