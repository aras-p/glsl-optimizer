/**************************************************************************
 *
 * Copyright 2010 Younes Manton.
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

VdpStatus
vlVdpGetApiVersion(uint32_t *api_version)
{
   if (!api_version)
      return VDP_STATUS_INVALID_POINTER;

   *api_version = 1;
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpGetInformationString(char const **information_string)
{
   if (!information_string)
      return VDP_STATUS_INVALID_POINTER;

   *information_string = "VDPAU-G3DVL";
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                   VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile,
                              VdpBool *is_supported, uint32_t *max_level, uint32_t *max_macroblocks,
                              uint32_t *max_width, uint32_t *max_height)
{
   if (!(is_supported && max_level && max_macroblocks && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                    VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                VdpYCbCrFormat bits_ycbcr_format,
                                                VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                   VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                     VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                        void *min_value, void *max_value)
{
   if (!(min_value && max_value))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                     VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                        void *min_value, void *max_value)
{
   if (!(min_value && max_value))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}
