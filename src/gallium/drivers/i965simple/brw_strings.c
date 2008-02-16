/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "brw_context.h"
#include "brw_reg.h"


static const char *brw_get_vendor( struct pipe_context *pipe )
{
   return "Tungsten Graphics, Inc.";
}


static const char *brw_get_name( struct pipe_context *pipe )
{
   static char buffer[128];
   const char *chipset;

   switch (brw_context(pipe)->pci_id) {
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


void
brw_init_string_functions(struct brw_context *brw)
{
   brw->pipe.get_name = brw_get_name;
   brw->pipe.get_vendor = brw_get_vendor;
}
