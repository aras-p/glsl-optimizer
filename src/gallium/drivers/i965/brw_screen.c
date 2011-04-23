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


#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_string.h"

#include "brw_reg.h"
#include "brw_context.h"
#include "brw_screen.h"
#include "brw_winsys.h"
#include "brw_public.h"
#include "brw_debug.h"
#include "brw_resource.h"

#ifdef DEBUG
static const struct debug_named_value debug_names[] = {
   { "tex",   DEBUG_TEXTURE, NULL },
   { "state", DEBUG_STATE, NULL },
   { "ioctl", DEBUG_IOCTL, NULL },
   { "blit",  DEBUG_BLIT, NULL },
   { "curbe", DEBUG_CURBE, NULL },
   { "fall",  DEBUG_FALLBACKS, NULL },
   { "verb",  DEBUG_VERBOSE, NULL },
   { "bat",   DEBUG_BATCH, NULL },
   { "pix",   DEBUG_PIXEL, NULL },
   { "wins",  DEBUG_WINSYS, NULL },
   { "min",   DEBUG_MIN_URB, NULL },
   { "dis",   DEBUG_DISASSEM, NULL },
   { "sync",  DEBUG_SYNC, NULL },
   { "prim",  DEBUG_PRIMS, NULL },
   { "vert",  DEBUG_VERTS, NULL },
   { "dma",   DEBUG_DMA, NULL },
   { "san",   DEBUG_SANITY, NULL },
   { "sleep", DEBUG_SLEEP, NULL },
   { "stats", DEBUG_STATS, NULL },
   { "sing",  DEBUG_SINGLE_THREAD, NULL },
   { "thre",  DEBUG_SINGLE_THREAD, NULL },
   { "wm",    DEBUG_WM, NULL },
   { "urb",   DEBUG_URB, NULL },
   { "vs",    DEBUG_VS, NULL },
   DEBUG_NAMED_VALUE_END
};

static const struct debug_named_value dump_names[] = {
   { "asm",   DUMP_ASM, NULL },
   { "state", DUMP_STATE, NULL },
   { "batch", DUMP_BATCH, NULL },
   DEBUG_NAMED_VALUE_END
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
   default:
      chipset = "unknown";
      break;
   }

   util_snprintf(buffer, sizeof(buffer), "i965 (chipset: %s)", chipset);
   return buffer;
}

static int
brw_get_param(struct pipe_screen *screen, enum pipe_cap param)
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
   case PIPE_CAP_TIMER_QUERY:
      return 0;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return BRW_MAX_TEXTURE_2D_LEVELS;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return BRW_MAX_TEXTURE_3D_LEVELS;
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return BRW_MAX_TEXTURE_2D_LEVELS;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
      /* disable for now */
      return 0;
   default:
      return 0;
   }
}

static int
brw_get_shader_param(struct pipe_screen *screen, unsigned shader, enum pipe_shader_cap param)
{
   switch(shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_GEOMETRY:
      break;
   default:
      return 0;
   }

   /* XXX: these are just shader model 4.0 values, fix this! */
   switch(param) {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
         return 65536;
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         return 65536;
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
         return 65536;
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         return 65536;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return 65536;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return 32;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return 4096;
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return PIPE_MAX_CONSTANT_BUFFERS;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         return 4096;
      case PIPE_SHADER_CAP_MAX_ADDRS:
         return 0;
      case PIPE_SHADER_CAP_MAX_PREDS:
         return 0;
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
         return 1;
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
          return 1;
      case PIPE_SHADER_CAP_SUBROUTINES:
          return 1;
      default:
         assert(0);
         return 0;
      }
}

static float
brw_get_paramf(struct pipe_screen *screen, enum pipe_cap param)
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
                         unsigned sample_count,
                         unsigned tex_usage)
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
      PIPE_FORMAT_Z24_UNORM_S8_USCALED,
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
      PIPE_FORMAT_Z24_UNORM_S8_USCALED,
      PIPE_FORMAT_Z16_UNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   const enum pipe_format *list;
   uint i;

   if (!util_format_is_supported(format, tex_usage))
      return FALSE;

   if (sample_count > 1)
      return FALSE;

   if (tex_usage & PIPE_BIND_DEPTH_STENCIL)
      list = depth_supported;
   else if (tex_usage & PIPE_BIND_RENDER_TARGET)
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

static boolean
brw_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence)
{
   return TRUE;
}

static boolean
brw_fence_finish(struct pipe_screen *screen,
                 struct pipe_fence_handle *fence,
                 uint64_t timeout)
{
   return TRUE;
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
brw_screen_create(struct brw_winsys_screen *sws)
{
   struct brw_screen *bscreen;
#ifdef DEBUG
   BRW_DEBUG = debug_get_flags_option("BRW_DEBUG", debug_names, 0);
   BRW_DEBUG |= debug_get_flags_option("INTEL_DEBUG", debug_names, 0);
   BRW_DEBUG |= DEBUG_STATS | DEBUG_MIN_URB | DEBUG_WM;

   BRW_DUMP = debug_get_flags_option("BRW_DUMP", dump_names, 0);
#endif

   bscreen = CALLOC_STRUCT(brw_screen);
   if (!bscreen)
      return NULL;

   bscreen->pci_id = sws->pci_id;
   if (IS_GEN6(sws->pci_id)) {
       bscreen->gen = 6;
       bscreen->needs_ff_sync = TRUE;
   } else if (IS_GEN5(sws->pci_id)) {
       bscreen->gen = 5;
       bscreen->needs_ff_sync = TRUE;
   } else if (IS_965(sws->pci_id)) {
       bscreen->gen = 4;
       if (IS_G4X(sws->pci_id)) {
	   bscreen->is_g4x = true;
       }
   } else {
      debug_printf("%s: unknown pci id 0x%x, cannot create screen\n", 
                   __FUNCTION__, sws->pci_id);
      free(bscreen);
      return NULL;
   }

   sws->gen = bscreen->gen;
   bscreen->sws = sws;
   bscreen->base.winsys = NULL;
   bscreen->base.destroy = brw_destroy_screen;
   bscreen->base.get_name = brw_get_name;
   bscreen->base.get_vendor = brw_get_vendor;
   bscreen->base.get_param = brw_get_param;
   bscreen->base.get_shader_param = brw_get_shader_param;
   bscreen->base.get_paramf = brw_get_paramf;
   bscreen->base.is_format_supported = brw_is_format_supported;
   bscreen->base.context_create = brw_create_context;
   bscreen->base.fence_reference = brw_fence_reference;
   bscreen->base.fence_signalled = brw_fence_signalled;
   bscreen->base.fence_finish = brw_fence_finish;

   brw_init_screen_resource_functions(bscreen);

   bscreen->no_tiling = debug_get_option("BRW_NO_TILING", FALSE) != NULL;
   
   
   return &bscreen->base;
}
