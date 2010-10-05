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
#include <vl_winsys.h>
#include <assert.h>
#include <pipe/p_screen.h>
#include <pipe/p_defines.h>
#include <math.h>
#include <util/u_debug.h>


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

   *information_string = INFORMATION_STRING;
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                   VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   struct vl_screen *vlscreen;
   uint32_t max_2d_texture_level;
   VdpStatus ret;
   
   debug_printf("[VDPAU] Querying video surfaces\n");

   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   
   vlscreen = vl_screen_create(dev->display, dev->screen);
   if (!vlscreen)
      return VDP_STATUS_RESOURCES;

   /* XXX: Current limits */ 
   *is_supported = true;
   if (surface_chroma_type != VDP_CHROMA_TYPE_420)  {
	  *is_supported = false;
	  goto no_sup;
   }

   max_2d_texture_level = vlscreen->pscreen->get_param( vlscreen->pscreen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS );
   if (!max_2d_texture_level)  {
      ret = VDP_STATUS_RESOURCES;
	  goto no_sup;
   }

   /* I am not quite sure if it is max_2d_texture_level-1 or just max_2d_texture_level */
   *max_width = *max_height = pow(2,max_2d_texture_level-1);
   
   vl_screen_destroy(vlscreen);
   
   return VDP_STATUS_OK;
   no_sup:
   return ret;
}

VdpStatus
vlVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
	struct vl_screen *vlscreen;
	
	debug_printf("[VDPAU] Querying get put video surfaces\n");
	
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   vlscreen = vl_screen_create(dev->display, dev->screen);
   if (!vlscreen)
      return VDP_STATUS_RESOURCES;

   if (bits_ycbcr_format != VDP_YCBCR_FORMAT_Y8U8V8A8 && bits_ycbcr_format != VDP_YCBCR_FORMAT_V8U8Y8A8) 
	                               *is_supported = vlscreen->pscreen->is_format_supported(vlscreen->pscreen,
                                   FormatToPipe(bits_ycbcr_format),
                                   PIPE_TEXTURE_2D,
								   1,
                                   PIPE_BIND_RENDER_TARGET, 
                                   PIPE_TEXTURE_GEOM_NON_SQUARE );
								   
   vl_screen_destroy(vlscreen);
								   
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile,
                              VdpBool *is_supported, uint32_t *max_level, uint32_t *max_macroblocks,
                              uint32_t *max_width, uint32_t *max_height)
{
   enum pipe_video_profile p_profile;
   uint32_t max_decode_width;
   uint32_t max_decode_height;
   uint32_t max_2d_texture_level;
   struct vl_screen *vlscreen;
   
   debug_printf("[VDPAU] Querying decoder\n");
	
   if (!(is_supported && max_level && max_macroblocks && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;
	  
   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   
   vlscreen = vl_screen_create(dev->display, dev->screen);
   if (!vlscreen)
      return VDP_STATUS_RESOURCES;

   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)	{
	   *is_supported = false;
	   return VDP_STATUS_OK;
   }
   
   if (p_profile != PIPE_VIDEO_PROFILE_MPEG2_SIMPLE && p_profile != PIPE_VIDEO_PROFILE_MPEG2_MAIN)  {
	   *is_supported = false;
	   return VDP_STATUS_OK;
   }
	   
   /* XXX hack, need to implement something more sane when the decoders have been implemented */
   max_2d_texture_level = vlscreen->pscreen->get_param( vlscreen->pscreen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS );
   max_decode_width = max_decode_height = pow(2,max_2d_texture_level-2);
   if (!(max_decode_width && max_decode_height))  
      return VDP_STATUS_RESOURCES;
	
   *is_supported = true;
   *max_width = max_decode_width;
   *max_height = max_decode_height;
   *max_level = 16;
   *max_macroblocks = (max_decode_width/16) * (max_decode_height/16);
   
   vl_screen_destroy(vlscreen);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{	
   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;
	  
   debug_printf("[VDPAU] Querying ouput surfaces\n");

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                    VdpBool *is_supported)
{
   debug_printf("[VDPAU] Querying output surfaces get put native cap\n");
	
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                VdpYCbCrFormat bits_ycbcr_format,
                                                VdpBool *is_supported)
{
   debug_printf("[VDPAU] Querying output surfaces put ycrcb cap\n");
   if (!is_supported)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
   debug_printf("[VDPAU] Querying bitmap surfaces\n");
   if (!(is_supported && max_width && max_height))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                   VdpBool *is_supported)
{
   debug_printf("[VDPAU] Querying mixer feature support\n");
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
