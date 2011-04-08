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
vlVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpPresentationQueueCreate(VdpDevice device,
                             VdpPresentationQueueTarget presentation_queue_target,
                             VdpPresentationQueue *presentation_queue)
{
   vlVdpPresentationQueue *pq = NULL;
   struct pipe_video_context *context;
   VdpStatus ret;

   _debug_printf("[VDPAU] Creating PresentationQueue\n");

   if (!presentation_queue)
      return VDP_STATUS_INVALID_POINTER;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   vlVdpPresentationQueueTarget *pqt = vlGetDataHTAB(presentation_queue_target);
   if (!pqt)
      return VDP_STATUS_INVALID_HANDLE;

   if (dev != pqt->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   context = dev->context->vpipe;

   pq = CALLOC(1, sizeof(vlVdpPresentationQueue));
   if (!pq)
      return VDP_STATUS_RESOURCES;

   pq->device = dev;
   pq->compositor = context->create_compositor(context);
   if (!pq->compositor) {
      ret = VDP_STATUS_ERROR;
      goto no_compositor;
   }

   *presentation_queue = vlAddDataHTAB(pq);
   if (*presentation_queue == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }

   return VDP_STATUS_OK;
no_handle:
no_compositor:
   FREE(pq);
   return ret;
}

VdpStatus
vlVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                         VdpColor *const background_color)
{
   if (!background_color)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                         VdpColor *const background_color)
{
   if (!background_color)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                              VdpTime *current_time)
{
   if (!current_time)
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue,
                              VdpOutputSurface surface,
                              uint32_t clip_width,
                              uint32_t clip_height,
                              VdpTime  earliest_presentation_time)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                            VdpOutputSurface surface,
                                            VdpTime *first_presentation_time)
{
   if (!first_presentation_time)
      return VDP_STATUS_INVALID_POINTER;

   //return VDP_STATUS_NO_IMPLEMENTATION;
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                         VdpOutputSurface surface,
                                         VdpPresentationQueueStatus *status,
                                         VdpTime *first_presentation_time)
{
   if (!(status && first_presentation_time))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}
