/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "vdpau_private.h"
#include <vdpau/vdpau.h>
#include <util/u_debug.h>
#include <util/u_memory.h>

VdpStatus
vlVdpOutputSurfaceCreate(VdpDevice device,
                         VdpRGBAFormat rgba_format,
                         uint32_t width, uint32_t height,
                         VdpOutputSurface  *surface)
{
   vlVdpOutputSurface *vlsurface = NULL;

   debug_printf("[VDPAU] Creating output surface\n");
   if (!(width && height))
      return VDP_STATUS_INVALID_SIZE;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   vlsurface = CALLOC(1, sizeof(vlVdpOutputSurface));
   if (!vlsurface)
      return VDP_STATUS_RESOURCES;

   vlsurface->width = width;
   vlsurface->height = height;
   vlsurface->format = FormatRGBAToPipe(rgba_format);

   *surface = vlAddDataHTAB(vlsurface);
   if (*surface == 0) {
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   return VDP_STATUS_OK;
}
