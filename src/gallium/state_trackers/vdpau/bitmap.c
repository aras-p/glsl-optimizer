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

#include <vdpau/vdpau.h>

#include "vdpau_private.h"

/**
 * Create a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfaceCreate(VdpDevice device,
                         VdpRGBAFormat rgba_format,
                         uint32_t width, uint32_t height,
                         VdpBool frequently_accessed,
                         VdpBitmapSurface *surface)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating a bitmap surface\n");
   if (!surface)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Destroy a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Retrieve the parameters used to create a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface,
                                VdpRGBAFormat *rgba_format,
                                uint32_t *width, uint32_t *height,
                                VdpBool *frequently_accessed)
{
   if (!(rgba_format && width && height && frequently_accessed))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Copy image data from application memory in the surface's native format to
 * a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface,
                                void const *const *source_data,
                                uint32_t const *source_pitches,
                                VdpRect const *destination_rect)
{
   if (!(source_data && source_pitches && destination_rect))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}
