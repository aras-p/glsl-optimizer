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

#include <stdio.h>

#include <vdpau/vdpau.h>

#include <util/u_debug.h>
#include <util/u_memory.h>

#include "vdpau_private.h"

VdpStatus
vlVdpPresentationQueueCreate(VdpDevice device,
                             VdpPresentationQueueTarget presentation_queue_target,
                             VdpPresentationQueue *presentation_queue)
{
   vlVdpPresentationQueue *pq = NULL;
   struct pipe_video_context *context;
   VdpStatus ret;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating PresentationQueue\n");

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
   pq->drawable = pqt->drawable;
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
   vlVdpPresentationQueue *pq;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying PresentationQueue\n");

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   pq->compositor->destroy(pq->compositor);

   vlRemoveDataHTAB(presentation_queue);
   FREE(pq);

   return VDP_STATUS_OK;
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
   static int dump_window = -1;

   vlVdpPresentationQueue *pq;
   vlVdpOutputSurface *surf;
   struct pipe_surface *drawable_surface;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   drawable_surface = vl_drawable_surface_get(pq->device->context, pq->drawable);
   if (!drawable_surface)
      return VDP_STATUS_INVALID_HANDLE;

   surf = vlGetDataHTAB(surface);
   if (!surf)
      return VDP_STATUS_INVALID_HANDLE;

   pq->compositor->clear_layers(pq->compositor);
   pq->compositor->set_rgba_layer(pq->compositor, 0, surf->sampler_view, NULL, NULL);
   pq->compositor->render_picture(pq->compositor, PIPE_MPEG12_PICTURE_TYPE_FRAME,
                                  drawable_surface, NULL, NULL);

   pq->device->context->vpipe->screen->flush_frontbuffer
   (
      pq->device->context->vpipe->screen,
      drawable_surface->texture,
      0, 0,
      vl_contextprivate_get(pq->device->context, drawable_surface)
   );

   if(dump_window == -1) {
      dump_window = debug_get_num_option("VDPAU_DUMP", 0);
   }

   if(dump_window) {
      static unsigned int framenum = 0;
      char cmd[256];

      sprintf(cmd, "xwd -id %d -out vdpau_frame_%08d.xwd", (int)pq->drawable, ++framenum);
      if (system(cmd) != 0)
         VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Dumping surface %d failed.\n", surface);
   }

   return VDP_STATUS_OK;
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
