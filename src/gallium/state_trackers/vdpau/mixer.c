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

#include <vl/vl_csc.h>

#include "vdpau_private.h"

VdpStatus
vlVdpVideoMixerCreate(VdpDevice device,
                      uint32_t feature_count,
                      VdpVideoMixerFeature const *features,
                      uint32_t parameter_count,
                      VdpVideoMixerParameter const *parameters,
                      void const *const *parameter_values,
                      VdpVideoMixer *mixer)
{
   vlVdpVideoMixer *vmixer = NULL;
   struct pipe_video_context *context;
   VdpStatus ret;
   float csc[16];

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating VideoMixer\n");

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   context = dev->context->vpipe;

   vmixer = CALLOC(1, sizeof(vlVdpVideoMixer));
   if (!vmixer)
      return VDP_STATUS_RESOURCES;

   vmixer->device = dev;
   vmixer->compositor = context->create_compositor(context);

   vl_csc_get_matrix
   (
      debug_get_bool_option("G3DVL_NO_CSC", FALSE) ?
      VL_CSC_COLOR_STANDARD_IDENTITY : VL_CSC_COLOR_STANDARD_BT_601,
      NULL, true, csc
   );
   vmixer->compositor->set_csc_matrix(vmixer->compositor, csc);

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
vlVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
   vlVdpVideoMixer *vmixer;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying VideoMixer\n");

   vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   vmixer->compositor->destroy(vmixer->compositor);

   FREE(vmixer);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer,
                                 uint32_t feature_count,
                                 VdpVideoMixerFeature const *features,
                                 VdpBool const *feature_enables)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Setting VideoMixer features\n");

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

VdpStatus vlVdpVideoMixerRender(VdpVideoMixer mixer,
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
   vlVdpVideoMixer *vmixer;
   vlVdpSurface *surf;
   vlVdpOutputSurface *dst;

   vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   surf = vlGetDataHTAB(video_surface_current);
   if (!surf)
      return VDP_STATUS_INVALID_HANDLE;

   dst = vlGetDataHTAB(destination_surface);
   if (!dst)
      return VDP_STATUS_INVALID_HANDLE;

   vmixer->compositor->clear_layers(vmixer->compositor);
   vmixer->compositor->set_buffer_layer(vmixer->compositor, 0, surf->video_buffer, NULL, NULL);
   vmixer->compositor->render_picture(vmixer->compositor, PIPE_MPEG12_PICTURE_TYPE_FRAME,
                                      dst->surface, NULL, NULL);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer,
                                  uint32_t attribute_count,
                                  VdpVideoMixerAttribute const *attributes,
                                  void const *const *attribute_values)
{
   if (!(attributes && attribute_values))
      return VDP_STATUS_INVALID_POINTER;

   vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   /*
    * TODO: Implement the function
    *
    * */

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer,
                                 uint32_t feature_count,
                                 VdpVideoMixerFeature const *features,
                                 VdpBool *feature_supports)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer,
                                 uint32_t feature_count,
                                 VdpVideoMixerFeature const *features,
                                 VdpBool *feature_enables)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerGetParameterValues(VdpVideoMixer mixer,
                                  uint32_t parameter_count,
                                  VdpVideoMixerParameter const *parameters,
                                  void *const *parameter_values)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer,
                                  uint32_t attribute_count,
                                  VdpVideoMixerAttribute const *attributes,
                                  void *const *attribute_values)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpGenerateCSCMatrix(VdpProcamp *procamp,
                       VdpColorStandard standard,
                       VdpCSCMatrix *csc_matrix)
{
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Generating CSCMatrix\n");
   if (!(csc_matrix && procamp))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_OK;
}
