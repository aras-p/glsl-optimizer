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

#include <assert.h>
#include <math.h>

#include "vdpau_private.h"
#include "vl_winsys.h"
#include "pipe/p_screen.h"
#include "pipe/p_defines.h"
#include "util/u_debug.h"

/**
 * Retrieve the VDPAU version implemented by the backend.
 */
VdpStatus
vlVdpGetApiVersion(uint32_t *api_version)
{
   if (!api_version)
      return VDP_STATUS_INVALID_POINTER;

   *api_version = 1;
   return VDP_STATUS_OK;
}

/**
 * Retrieve an implementation-specific string description of the implementation.
 * This typically includes detailed version information.
 */
VdpStatus
vlVdpGetInformationString(char const **information_string)
{
   if (!information_string)
      return VDP_STATUS_INVALID_POINTER;

   *information_string = INFORMATION_STRING;
   return VDP_STATUS_OK;
}

/**
 * Query the implementation's VdpVideoSurface capabilities.
 */
VdpStatus
vlVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                   VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   vlVdpDevice *dev;
   struct pipe_screen *pscreen;
   uint32_t max_2d_texture_level;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpVideoSurface capabilities\n");

   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pscreen = dev->vscreen->pscreen;
   if (!pscreen)
      return VDP_STATUS_RESOURCES;

   /* XXX: Current limits */
   *is_supported = true;
   if (surface_chroma_type != VDP_CHROMA_TYPE_420)
      *is_supported = false;

   max_2d_texture_level = pscreen->get_param(pscreen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS);
   if (!max_2d_texture_level)
      return VDP_STATUS_RESOURCES;

   /* I am not quite sure if it is max_2d_texture_level-1 or just max_2d_texture_level */
   *max_width = *max_height = pow(2,max_2d_texture_level-1);

   return VDP_STATUS_OK;
}

/**
 * Query the implementation's VdpVideoSurface GetBits/PutBits capabilities.
 */
VdpStatus
vlVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
   vlVdpDevice *dev;
   struct pipe_screen *pscreen;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpVideoSurface get/put bits YCbCr capabilities\n");

   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pscreen = dev->vscreen->pscreen;
   if (!pscreen)
      return VDP_STATUS_RESOURCES;

   *is_supported = pscreen->is_video_format_supported
   (
      pscreen,
      FormatYCBCRToPipe(bits_ycbcr_format),
      PIPE_VIDEO_PROFILE_UNKNOWN
   );

   return VDP_STATUS_OK;
}

/**
 * Query the implementation's VdpDecoder capabilities.
 */
VdpStatus
vlVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile,
                              VdpBool *is_supported, uint32_t *max_level, uint32_t *max_macroblocks,
                              uint32_t *max_width, uint32_t *max_height)
{
   vlVdpDevice *dev;
   struct pipe_screen *pscreen;
   enum pipe_video_profile p_profile;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpDecoder capabilities\n");

   if (!(is_supported && max_level && max_macroblocks && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pscreen = dev->vscreen->pscreen;
   if (!pscreen)
      return VDP_STATUS_RESOURCES;

   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)	{
      *is_supported = false;
      return VDP_STATUS_OK;
   }
   
   *is_supported = pscreen->get_video_param(pscreen, p_profile, PIPE_VIDEO_CAP_SUPPORTED);
   if (*is_supported) {
      *max_width = pscreen->get_video_param(pscreen, p_profile, PIPE_VIDEO_CAP_MAX_WIDTH); 
      *max_height = pscreen->get_video_param(pscreen, p_profile, PIPE_VIDEO_CAP_MAX_HEIGHT);
      *max_level = 16;
      *max_macroblocks = (*max_width/16)*(*max_height/16);
   } else {
      *max_width = 0;
      *max_height = 0;
      *max_level = 0;
      *max_macroblocks = 0;
   }

   return VDP_STATUS_OK;
}

/**
 * Query the implementation's VdpOutputSurface capabilities.
 */
VdpStatus
vlVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpOutputSurface capabilities\n");

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Query the implementation's capability to perform a PutBits operation using
 * application data matching the surface's format.
 */
VdpStatus
vlVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                    VdpBool *is_supported)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpOutputSurface get/put bits native capabilities\n");

   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Query the implementation's capability to perform a PutBits operation using
 * application data in a specific indexed format.
 */
VdpStatus
vlVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpIndexedFormat bits_indexed_format,
                                                  VdpColorTableFormat color_table_format,
                                                  VdpBool *is_supported)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpOutputSurface put bits indexed capabilities\n");

   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Query the implementation's capability to perform a PutBits operation using
 * application data in a specific YCbCr/YUB format.
 */
VdpStatus
vlVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                VdpYCbCrFormat bits_ycbcr_format,
                                                VdpBool *is_supported)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpOutputSurface put bits YCbCr capabilities\n");

   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Query the implementation's VdpBitmapSurface capabilities.
 */
VdpStatus
vlVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpBitmapSurface capabilities\n");

   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Query the implementation's support for a specific feature.
 */
VdpStatus
vlVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                   VdpBool *is_supported)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Querying VdpVideoMixer feature support\n");

   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Query the implementation's support for a specific parameter.
 */
VdpStatus
vlVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                     VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   switch (parameter) {
   case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
   case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
   case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
   case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
      *is_supported = VDP_TRUE;
      break;
   default:
      *is_supported = VDP_FALSE;
      break;
   }
   return VDP_STATUS_OK;
}

/**
 * Query the implementation's supported for a specific parameter.
 */
VdpStatus
vlVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                        void *min_value, void *max_value)
{
   vlVdpDevice *dev = vlGetDataHTAB(device);
   struct pipe_screen *screen;
   enum pipe_video_profile prof = PIPE_VIDEO_PROFILE_UNKNOWN;
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   if (!(min_value && max_value))
      return VDP_STATUS_INVALID_POINTER;
   screen = dev->vscreen->pscreen;
   switch (parameter) {
   case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
      *(uint32_t*)min_value = 48;
      *(uint32_t*)max_value = screen->get_video_param(screen, prof, PIPE_VIDEO_CAP_MAX_WIDTH);
      break;
   case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
      *(uint32_t*)min_value = 48;
      *(uint32_t*)max_value = screen->get_video_param(screen, prof, PIPE_VIDEO_CAP_MAX_HEIGHT);
      break;

   case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
      *(uint32_t*)min_value = 0;
      *(uint32_t*)max_value = 4;
      break;

   case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
   default:
      return VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER;
   }
   return VDP_STATUS_OK;
}

/**
 * Query the implementation's support for a specific attribute.
 */
VdpStatus
vlVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                     VdpBool *is_supported)
{
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   switch (attribute) {
   case VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR:
   case VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX:
   case VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL:
   case VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL:
   case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA:
   case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA:
   case VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE:
      *is_supported = VDP_TRUE;
      break;
   default:
      *is_supported = VDP_FALSE;
   }
   return VDP_STATUS_OK;
}

/**
 * Query the implementation's supported for a specific attribute.
 */
VdpStatus
vlVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                        void *min_value, void *max_value)
{
   if (!(min_value && max_value))
      return VDP_STATUS_INVALID_POINTER;

   switch (attribute) {
   case VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL:
   case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA:
   case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA:
      *(float*)min_value = 0.f;
      *(float*)max_value = 1.f;
      break;
   case VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL:
      *(float*)min_value = -1.f;
      *(float*)max_value = 1.f;
      break;
   case VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE:
      *(uint8_t*)min_value = 0;
      *(uint8_t*)max_value = 1;
      break;
   case VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR:
   case VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX:
   default:
      return VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE;
   }
   return VDP_STATUS_OK;
}
