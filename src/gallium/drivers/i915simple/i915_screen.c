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

#include "i915_reg.h"
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

   sprintf(buffer, "i915 (chipset: %s)", chipset);
   return buffer;
}



static void
i915_destroy_screen( struct pipe_screen *screen )
{
   FREE(screen);
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
      winsys->printf(winsys, 
                     "%s: unknown pci id 0x%x, cannot create screen\n", 
                     __FUNCTION__, pci_id);
      return NULL;
   }

   i915screen->pci_id = pci_id;

   i915screen->screen.winsys = winsys;

   i915screen->screen.destroy = i915_destroy_screen;

   i915screen->screen.get_name = i915_get_name;
   i915screen->screen.get_vendor = i915_get_vendor;

   i915_init_screen_string_functions(&i915screen->screen);
   i915_init_screen_texture_functions(&i915screen->screen);

   return &i915screen->screen;
}
