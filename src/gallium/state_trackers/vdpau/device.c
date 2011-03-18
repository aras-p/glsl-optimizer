/**************************************************************************
 *
 * Copyright 2010 Younes Manton og Thomas Balling Sørensen.
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

#include <pipe/p_compiler.h>
#include <pipe/p_video_context.h>
#include <vl_winsys.h>
#include <util/u_memory.h>
#include <util/u_debug.h>
#include "vdpau_private.h"


PUBLIC VdpStatus
vdp_imp_device_create_x11(Display *display, int screen, VdpDevice *device,
                          VdpGetProcAddress **get_proc_address)
{
   VdpStatus ret;
   vlVdpDevice *dev = NULL;

   if (!(display && device && get_proc_address))
      return VDP_STATUS_INVALID_POINTER;

   if (!vlCreateHTAB()) {
      ret = VDP_STATUS_RESOURCES;
      goto no_htab;
   }

   dev = CALLOC(1, sizeof(vlVdpDevice));
   if (!dev) {
      ret = VDP_STATUS_RESOURCES;
      goto no_dev;
   }

   dev->display = display;
   dev->screen = screen;
   dev->vscreen = vl_screen_create(display, screen);
   if (!dev->vscreen)
	   {
      ret = VDP_STATUS_RESOURCES;
      goto no_vscreen;
   }

   *device = vlAddDataHTAB(dev);
   if (*device == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }

   *get_proc_address = &vlVdpGetProcAddress;
   debug_printf("[VDPAU] Device created succesfully\n");

   return VDP_STATUS_OK;

no_handle:
   /* Destroy vscreen */
no_vscreen:
   FREE(dev);
no_dev:
   vlDestroyHTAB();
no_htab:
   return ret;
}

PUBLIC VdpStatus
vlVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                      VdpPresentationQueueTarget *target)
{
   VdpStatus    ret;
   vlVdpPresentationQueueTarget *pqt = NULL;

   debug_printf("[VDPAU] Creating PresentationQueueTarget\n");

   if (!drawable)
      return VDP_STATUS_INVALID_HANDLE;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pqt = CALLOC(1, sizeof(vlVdpPresentationQueue));
   if (!pqt)
      return VDP_STATUS_RESOURCES;

   pqt->device = dev;
   pqt->drawable = drawable;

	*target = vlAddDataHTAB(pqt);
   if (*target == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }


   return VDP_STATUS_OK;
no_handle:
   FREE(dev);
   return ret;
}

VdpStatus
vlVdpDeviceDestroy(VdpDevice device)
{
   debug_printf("[VDPAU] Destroying destroy\n");

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   FREE(dev);
   vlDestroyHTAB();

   debug_printf("[VDPAU] Device destroyed succesfully\n");

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   if (!function_pointer)
      return VDP_STATUS_INVALID_POINTER;

   if (!vlGetFuncFTAB(function_id, function_pointer))
      return VDP_STATUS_INVALID_FUNC_ID;

   return VDP_STATUS_OK;
}

#define _ERROR_TYPE(TYPE,STRING) case TYPE: return STRING;

char const *
vlVdpGetErrorString (VdpStatus status)
{
   switch (status) {
   _ERROR_TYPE(VDP_STATUS_OK,"The operation completed successfully; no error.");
   _ERROR_TYPE(VDP_STATUS_NO_IMPLEMENTATION,"No backend implementation could be loaded.");
   _ERROR_TYPE(VDP_STATUS_DISPLAY_PREEMPTED,"The display was preempted, or a fatal error occurred. The application must re-initialize VDPAU.");
   _ERROR_TYPE(VDP_STATUS_INVALID_HANDLE,"An invalid handle value was provided. Either the handle does not exist at all, or refers to an object of an incorrect type.");
   _ERROR_TYPE(VDP_STATUS_INVALID_POINTER ,"An invalid pointer was provided. Typically, this means that a NULL pointer was provided for an 'output' parameter.");
   _ERROR_TYPE(VDP_STATUS_INVALID_CHROMA_TYPE ,"An invalid/unsupported VdpChromaType value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_Y_CB_CR_FORMAT,"An invalid/unsupported VdpYCbCrFormat value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_RGBA_FORMAT,"An invalid/unsupported VdpRGBAFormat value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_INDEXED_FORMAT,"An invalid/unsupported VdpIndexedFormat value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_COLOR_STANDARD,"An invalid/unsupported VdpColorStandard value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_COLOR_TABLE_FORMAT,"An invalid/unsupported VdpColorTableFormat value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_BLEND_FACTOR,"An invalid/unsupported VdpOutputSurfaceRenderBlendFactor value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_BLEND_EQUATION,"An invalid/unsupported VdpOutputSurfaceRenderBlendEquation value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_FLAG,"An invalid/unsupported flag value/combination was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_DECODER_PROFILE,"An invalid/unsupported VdpDecoderProfile value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_VIDEO_MIXER_FEATURE,"An invalid/unsupported VdpVideoMixerFeature value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER ,"An invalid/unsupported VdpVideoMixerParameter value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE,"An invalid/unsupported VdpVideoMixerAttribute value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_VIDEO_MIXER_PICTURE_STRUCTURE,"An invalid/unsupported VdpVideoMixerPictureStructure value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_FUNC_ID,"An invalid/unsupported VdpFuncId value was supplied.");
   _ERROR_TYPE(VDP_STATUS_INVALID_SIZE,"The size of a supplied object does not match the object it is being used with.\
      For example, a VdpVideoMixer is configured to process VdpVideoSurface objects of a specific size.\
      If presented with a VdpVideoSurface of a different size, this error will be raised.");
   _ERROR_TYPE(VDP_STATUS_INVALID_VALUE,"An invalid/unsupported value was supplied.\
      This is a catch-all error code for values of type other than those with a specific error code.");
   _ERROR_TYPE(VDP_STATUS_INVALID_STRUCT_VERSION,"An invalid/unsupported structure version was specified in a versioned structure. \
      This implies that the implementation is older than the header file the application was built against.");
   _ERROR_TYPE(VDP_STATUS_RESOURCES,"The system does not have enough resources to complete the requested operation at this time.");
   _ERROR_TYPE(VDP_STATUS_HANDLE_DEVICE_MISMATCH,"The set of handles supplied are not all related to the same VdpDevice.When performing operations \
      that operate on multiple surfaces, such as VdpOutputSurfaceRenderOutputSurface or VdpVideoMixerRender, \
      all supplied surfaces must have been created within the context of the same VdpDevice object. \
      This error is raised if they were not.");
   _ERROR_TYPE(VDP_STATUS_ERROR,"A catch-all error, used when no other error code applies.");
   }
}
