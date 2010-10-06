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
 #include <util/u_memory.h>
  #include <util/u_debug.h>
  #include "vdpau_private.h"

 
 VdpStatus 	
 vlVdpVideoMixerCreate (VdpDevice device, 
						uint32_t feature_count, 
						VdpVideoMixerFeature const *features, 
						uint32_t parameter_count, 
						VdpVideoMixerParameter const *parameters, 
						void const *const *parameter_values, 
						VdpVideoMixer *mixer)
{
	VdpStatus ret;
	vlVdpVideoMixer *vmixer = NULL;
	
	debug_printf("[VDPAU] Creating VideoMixer\n");
	
	vlVdpDevice *dev = vlGetDataHTAB(device);
	if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
	  
	vmixer = CALLOC(1, sizeof(vlVdpVideoMixer));
	if (!vmixer)
      return VDP_STATUS_RESOURCES;
	  
	vmixer->device = dev;
	  /*
	   * TODO: Handle features and parameters
	   * */
	  
	*mixer = vlAddDataHTAB(vmixer);
    if (*mixer == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
    }
   
   
   return VDP_STATUS_OK;
   no_handle:
   return ret;
}

VdpStatus
vlVdpVideoMixerSetFeatureEnables (
			VdpVideoMixer mixer, 
			uint32_t feature_count, 
			VdpVideoMixerFeature const *features, 
			VdpBool const *feature_enables)
{
	debug_printf("[VDPAU] Setting VideoMixer features\n");
	
	if (!(features && feature_enables))	
		return VDP_STATUS_INVALID_POINTER;
	
	vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
	if (!vmixer)
		return VDP_STATUS_INVALID_HANDLE;
		
	/*
	   * TODO: Set features
	   * */
	
	
	return VDP_STATUS_OK;
}

VdpStatus vlVdpVideoMixerRender (
		VdpVideoMixer mixer, 
		VdpOutputSurface background_surface, 
		VdpRect const *background_source_rect, 
		VdpVideoMixerPictureStructure current_picture_structure, 
		uint32_t video_surface_past_count, 
		VdpVideoSurface const *video_surface_past, 
		VdpVideoSurface video_surface_current, 
		uint32_t video_surface_future_count, 
		VdpVideoSurface const *video_surface_future, 
		VdpRect const *video_source_rect, 
		VdpOutputSurface destination_surface, 
		VdpRect const *destination_rect, 
		VdpRect const *destination_video_rect, 
		uint32_t layer_count, 
		VdpLayer const *layers)
{
	if (!(background_source_rect && video_surface_past && video_surface_future && video_source_rect && destination_rect && destination_video_rect && layers))	
		return VDP_STATUS_INVALID_POINTER;

	return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerSetAttributeValues (
		VdpVideoMixer mixer, 
		uint32_t attribute_count, 
		VdpVideoMixerAttribute const *attributes, 
		void const *const *attribute_values)
{
	if (!(attributes && attribute_values))	
		return VDP_STATUS_INVALID_POINTER;
	
	vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
	if (!vmixer)
		return VDP_STATUS_INVALID_HANDLE;
	
	return VDP_STATUS_OK;
}