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

#include "util/u_memory.h"
#include "util/u_debug.h"

#include "vl/vl_csc.h"

#include "vdpau_private.h"

/**
 * Create a VdpVideoMixer.
 */
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
   VdpStatus ret;
   struct pipe_screen *screen;
   unsigned max_width, max_height, i;
   enum pipe_video_profile prof = PIPE_VIDEO_PROFILE_UNKNOWN;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating VideoMixer\n");

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   screen = dev->vscreen->pscreen;

   vmixer = CALLOC(1, sizeof(vlVdpVideoMixer));
   if (!vmixer)
      return VDP_STATUS_RESOURCES;

   vmixer->device = dev;
   vl_compositor_init(&vmixer->compositor, dev->context->pipe);

   vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_BT_601, NULL, true, vmixer->csc);
   if (!debug_get_bool_option("G3DVL_NO_CSC", FALSE))
      vl_compositor_set_csc_matrix(&vmixer->compositor, vmixer->csc);

   /*
    * TODO: Handle features
    */

   *mixer = vlAddDataHTAB(vmixer);
   if (*mixer == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }
   vmixer->chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   ret = VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER;
   for (i = 0; i < parameter_count; ++i) {
      switch (parameters[i]) {
      case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
         vmixer->video_width = *(uint32_t*)parameter_values[i];
         break;
      case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
         vmixer->video_height = *(uint32_t*)parameter_values[i];
         break;
      case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
         vmixer->chroma_format = ChromaToPipe(*(VdpChromaType*)parameter_values[i]);
         break;
      case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
         vmixer->max_layers = *(uint32_t*)parameter_values[i];
         break;
      default: goto no_params;
      }
   }
   ret = VDP_STATUS_INVALID_VALUE;
   if (vmixer->max_layers > 4) {
      VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Max layers > 4 not supported\n", vmixer->max_layers);
      goto no_params;
   }
   max_width = screen->get_video_param(screen, prof, PIPE_VIDEO_CAP_MAX_WIDTH);
   max_height = screen->get_video_param(screen, prof, PIPE_VIDEO_CAP_MAX_HEIGHT);
   if (vmixer->video_width < 48 ||
       vmixer->video_width > max_width) {
      VDPAU_MSG(VDPAU_TRACE, "[VDPAU] 48 < %u < %u not valid for width\n", vmixer->video_width, max_width);
      goto no_params;
   }
   if (vmixer->video_height < 48 ||
       vmixer->video_height > max_height) {
      VDPAU_MSG(VDPAU_TRACE, "[VDPAU] 48 < %u < %u  not valid for height\n", vmixer->video_height, max_height);
      goto no_params;
   }
   vmixer->luma_key_min = 0.f;
   vmixer->luma_key_max = 1.f;
   vmixer->noise_reduction_level = 0.f;
   vmixer->sharpness = 0.f;
   return VDP_STATUS_OK;

no_params:
   vlRemoveDataHTAB(*mixer);
no_handle:
   vl_compositor_cleanup(&vmixer->compositor);
   FREE(vmixer);
   return ret;
}

/**
 * Destroy a VdpVideoMixer.
 */
VdpStatus
vlVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
   vlVdpVideoMixer *vmixer;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying VideoMixer\n");

   vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;
   vlRemoveDataHTAB(mixer);

   vl_compositor_cleanup(&vmixer->compositor);

   FREE(vmixer);

   return VDP_STATUS_OK;
}

/**
 * Enable or disable features.
 */
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
    */

   return VDP_STATUS_OK;
}

/**
 * Perform a video post-processing and compositing operation.
 */
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
   struct pipe_video_rect src_rect, dst_rect, dst_clip;
   unsigned layer = 0;

   vlVdpVideoMixer *vmixer;
   vlVdpSurface *surf;
   vlVdpOutputSurface *dst;

   vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   surf = vlGetDataHTAB(video_surface_current);
   if (!surf)
      return VDP_STATUS_INVALID_HANDLE;

   if (surf->device != vmixer->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   if (vmixer->video_width > surf->video_buffer->width ||
       vmixer->video_height > surf->video_buffer->height ||
       vmixer->chroma_format != surf->video_buffer->chroma_format)
      return VDP_STATUS_INVALID_SIZE;

   if (layer_count > vmixer->max_layers)
      return VDP_STATUS_INVALID_VALUE;

   dst = vlGetDataHTAB(destination_surface);
   if (!dst)
      return VDP_STATUS_INVALID_HANDLE;

   if (background_surface != VDP_INVALID_HANDLE) {
      vlVdpOutputSurface *bg = vlGetDataHTAB(background_surface);
      if (!bg)
         return VDP_STATUS_INVALID_HANDLE;
      vl_compositor_set_rgba_layer(&vmixer->compositor, layer++, bg->sampler_view,
                                   RectToPipe(background_source_rect, &src_rect), NULL);
   }

   vl_compositor_clear_layers(&vmixer->compositor);
   vl_compositor_set_buffer_layer(&vmixer->compositor, layer++, surf->video_buffer,
                                  RectToPipe(video_source_rect, &src_rect), NULL);
   vl_compositor_render(&vmixer->compositor, dst->surface,
                        RectToPipe(destination_video_rect, &dst_rect),
                        RectToPipe(destination_rect, &dst_clip),
                        &dst->dirty_area);

   return VDP_STATUS_OK;
}

/**
 * Set attribute values.
 */
VdpStatus
vlVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer,
                                  uint32_t attribute_count,
                                  VdpVideoMixerAttribute const *attributes,
                                  void const *const *attribute_values)
{
   const VdpColor *background_color;
   union pipe_color_union color;
   const float *vdp_csc;
   float val;
   unsigned i;

   if (!(attributes && attribute_values))
      return VDP_STATUS_INVALID_POINTER;

   vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   for (i = 0; i < attribute_count; ++i) {
      switch (attributes[i]) {
      case VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR:
         background_color = attribute_values[i];
         color.f[0] = background_color->red;
         color.f[1] = background_color->green;
         color.f[2] = background_color->blue;
         color.f[3] = background_color->alpha;
         vl_compositor_set_clear_color(&vmixer->compositor, &color);
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX:
         vdp_csc = attribute_values[i];
         vmixer->custom_csc = !!vdp_csc;
         if (!vdp_csc)
            vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_BT_601, NULL, 1, vmixer->csc);
         else
            memcpy(vmixer->csc, vdp_csc, sizeof(float)*12);
         if (!debug_get_bool_option("G3DVL_NO_CSC", FALSE))
            vl_compositor_set_csc_matrix(&vmixer->compositor, vmixer->csc);
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL:
         val = *(float*)attribute_values[i];
         if (val < 0.f || val > 1.f)
            return VDP_STATUS_INVALID_VALUE;
         vmixer->noise_reduction_level = val;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA:
         val = *(float*)attribute_values[i];
         if (val < 0.f || val > 1.f)
            return VDP_STATUS_INVALID_VALUE;
         vmixer->luma_key_min = val;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA:
         val = *(float*)attribute_values[i];
         if (val < 0.f || val > 1.f)
            return VDP_STATUS_INVALID_VALUE;
         vmixer->luma_key_max = val;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL:
         val = *(float*)attribute_values[i];
         if (val < -1.f || val > 1.f)
            return VDP_STATUS_INVALID_VALUE;
         vmixer->sharpness = val;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE:
         if (*(uint8_t*)attribute_values[i] > 1)
            return VDP_STATUS_INVALID_VALUE;
         vmixer->skip_chroma_deint = *(uint8_t*)attribute_values[i];
         break;
      default:
         return VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE;
      }
   }

   return VDP_STATUS_OK;
}

/**
 * Retrieve whether features were requested at creation time.
 */
VdpStatus
vlVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer,
                                 uint32_t feature_count,
                                 VdpVideoMixerFeature const *features,
                                 VdpBool *feature_supports)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Retrieve whether features are enabled.
 */
VdpStatus
vlVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer,
                                 uint32_t feature_count,
                                 VdpVideoMixerFeature const *features,
                                 VdpBool *feature_enables)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Retrieve parameter values given at creation time.
 */
VdpStatus
vlVdpVideoMixerGetParameterValues(VdpVideoMixer mixer,
                                  uint32_t parameter_count,
                                  VdpVideoMixerParameter const *parameters,
                                  void *const *parameter_values)
{
   vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   unsigned i;
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   if (!parameter_count)
      return VDP_STATUS_OK;
   if (!(parameters && parameter_values))
      return VDP_STATUS_INVALID_POINTER;
   for (i = 0; i < parameter_count; ++i) {
      switch (parameters[i]) {
      case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
         *(uint32_t*)parameter_values[i] = vmixer->video_width;
         break;
      case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
         *(uint32_t*)parameter_values[i] = vmixer->video_height;
         break;
      case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
         *(VdpChromaType*)parameter_values[i] = PipeToChroma(vmixer->chroma_format);
         break;
      case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
         *(uint32_t*)parameter_values[i] = vmixer->max_layers;
         break;
      default:
         return VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER;
      }
   }
   return VDP_STATUS_OK;
}

/**
 * Retrieve current attribute values.
 */
VdpStatus
vlVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer,
                                  uint32_t attribute_count,
                                  VdpVideoMixerAttribute const *attributes,
                                  void *const *attribute_values)
{
   unsigned i;
   VdpCSCMatrix **vdp_csc;

   if (!(attributes && attribute_values))
      return VDP_STATUS_INVALID_POINTER;

   vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
      return VDP_STATUS_INVALID_HANDLE;

   for (i = 0; i < attribute_count; ++i) {
      switch (attributes[i]) {
      case VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR:
         vl_compositor_get_clear_color(&vmixer->compositor, attribute_values[i]);
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX:
         vdp_csc = attribute_values[i];
         if (!vmixer->custom_csc) {
             *vdp_csc = NULL;
            break;
         }
         memcpy(*vdp_csc, vmixer->csc, sizeof(float)*12);
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL:
         *(float*)attribute_values[i] = vmixer->noise_reduction_level;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA:
         *(float*)attribute_values[i] = vmixer->luma_key_min;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA:
         *(float*)attribute_values[i] = vmixer->luma_key_max;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL:
         *(float*)attribute_values[i] = vmixer->sharpness;
         break;
      case VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE:
         *(uint8_t*)attribute_values[i] = vmixer->skip_chroma_deint;
         break;
      default:
         return VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE;
      }
   }
   return VDP_STATUS_OK;
}

/**
 * Generate a color space conversion matrix.
 */
VdpStatus
vlVdpGenerateCSCMatrix(VdpProcamp *procamp,
                       VdpColorStandard standard,
                       VdpCSCMatrix *csc_matrix)
{
   float matrix[16];
   enum VL_CSC_COLOR_STANDARD vl_std;
   struct vl_procamp camp;

   if (!(csc_matrix && procamp))
      return VDP_STATUS_INVALID_POINTER;

   if (procamp->struct_version > VDP_PROCAMP_VERSION)
      return VDP_STATUS_INVALID_STRUCT_VERSION;

   switch (standard) {
      case VDP_COLOR_STANDARD_ITUR_BT_601: vl_std = VL_CSC_COLOR_STANDARD_BT_601; break;
      case VDP_COLOR_STANDARD_ITUR_BT_709: vl_std = VL_CSC_COLOR_STANDARD_BT_709; break;
      case VDP_COLOR_STANDARD_SMPTE_240M:  vl_std = VL_CSC_COLOR_STANDARD_SMPTE_240M; break;
      default: return VDP_STATUS_INVALID_COLOR_STANDARD;
   }
   camp.brightness = procamp->brightness;
   camp.contrast = procamp->contrast;
   camp.saturation = procamp->saturation;
   camp.hue = procamp->hue;
   vl_csc_get_matrix(vl_std, &camp, 1, matrix);
   memcpy(csc_matrix, matrix, sizeof(float)*12);
   return VDP_STATUS_OK;
}
