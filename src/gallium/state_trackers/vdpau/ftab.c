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
#include "vdpau_private.h"

static void* ftab[67] =
{
   0, /* VDP_FUNC_ID_GET_ERROR_STRING */
   0, /* VDP_FUNC_ID_GET_PROC_ADDRESS */
   &vlVdpGetApiVersion, /* VDP_FUNC_ID_GET_API_VERSION */
   0,
   &vlVdpGetInformationString, /* VDP_FUNC_ID_GET_INFORMATION_STRING */
   &vlVdpDeviceDestroy, /* VDP_FUNC_ID_DEVICE_DESTROY */
   0, /* VDP_FUNC_ID_GENERATE_CSC_MATRIX */
   &vlVdpVideoSurfaceQueryCapabilities, /* VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES */
   &vlVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities, /* VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES */
   &vlVdpVideoSurfaceCreate, /* VDP_FUNC_ID_VIDEO_SURFACE_CREATE */
   0, /* VDP_FUNC_ID_VIDEO_SURFACE_DESTROY */
   0, /* VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS */
   0, /* VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR */
   0, /* VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR */
   &vlVdpOutputSurfaceQueryCapabilities, /* VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES */
   &vlVdpOutputSurfaceQueryGetPutBitsNativeCapabilities, /* VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES */
   &vlVdpOutputSurfaceQueryPutBitsYCbCrCapabilities, /* VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_CREATE */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR */
   &vlVdpBitmapSurfaceQueryCapabilities, /* VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES */
   0, /* VDP_FUNC_ID_BITMAP_SURFACE_CREATE */
   0, /* VDP_FUNC_ID_BITMAP_SURFACE_DESTROY */
   0, /* VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS */
   0, /* VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE */
   0,
   0,
   0,
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE */
   0, /* VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA */
   &vlVdpDecoderQueryCapabilities, /* VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES */
   0, /* VDP_FUNC_ID_DECODER_CREATE */
   0, /* VDP_FUNC_ID_DECODER_DESTROY */
   0, /* VDP_FUNC_ID_DECODER_GET_PARAMETERS */
   0, /* VDP_FUNC_ID_DECODER_RENDER */
   &vlVdpVideoMixerQueryFeatureSupport, /* VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT */
   &vlVdpVideoMixerQueryParameterSupport, /* VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT */
   &vlVdpVideoMixerQueryAttributeSupport, /* VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT */
   &vlVdpVideoMixerQueryParameterValueRange, /* VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE */
   &vlVdpVideoMixerQueryAttributeValueRange, /* VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_CREATE */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_DESTROY */
   0, /* VDP_FUNC_ID_VIDEO_MIXER_RENDER */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR */
   0,
   0,
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE */
   0, /* VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS */
   0  /* VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER */
};

static void* ftab_winsys[1] =
{
   0  /* VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11 */
};

boolean vlGetFuncFTAB(VdpFuncId function_id, void **func)
{
   assert(func);
   if (function_id < VDP_FUNC_ID_BASE_WINSYS) {
      if (function_id > 66)
         return FALSE;
      *func = ftab[function_id];
   }
   else {
      function_id -= VDP_FUNC_ID_BASE_WINSYS;
      if (function_id > 0)
        return FALSE;
      *func = ftab_winsys[function_id];
   }
   return TRUE;
}
