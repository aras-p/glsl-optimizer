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
#include <time.h>
#include <sys/timeb.h>

#include <vdpau/vdpau.h>

#include "util/u_debug.h"
#include "util/u_memory.h"

#include "vdpau_private.h"

/**
 * Create a VdpPresentationQueue.
 */
VdpStatus
vlVdpPresentationQueueCreate(VdpDevice device,
                             VdpPresentationQueueTarget presentation_queue_target,
                             VdpPresentationQueue *presentation_queue)
{
   vlVdpPresentationQueue *pq = NULL;
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

   pq = CALLOC(1, sizeof(vlVdpPresentationQueue));
   if (!pq)
      return VDP_STATUS_RESOURCES;

   pq->device = dev;
   pq->drawable = pqt->drawable;

   if (!vl_compositor_init(&pq->compositor, dev->context->pipe)) {
      ret = VDP_STATUS_ERROR;
      goto no_compositor;
   }

   vl_compositor_reset_dirty_area(&pq->dirty_area);

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

/**
 * Destroy a VdpPresentationQueue.
 */
VdpStatus
vlVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
   vlVdpPresentationQueue *pq;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying PresentationQueue\n");

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   vl_compositor_cleanup(&pq->compositor);

   vlRemoveDataHTAB(presentation_queue);
   FREE(pq);

   return VDP_STATUS_OK;
}

/**
 * Configure the background color setting.
 */
VdpStatus
vlVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                         VdpColor *const background_color)
{
   vlVdpPresentationQueue *pq;
   union pipe_color_union color;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Setting background color\n");

   if (!background_color)
      return VDP_STATUS_INVALID_POINTER;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   color.f[0] = background_color->red;
   color.f[1] = background_color->green;
   color.f[2] = background_color->blue;
   color.f[3] = background_color->alpha;

   vl_compositor_set_clear_color(&pq->compositor, &color);

   return VDP_STATUS_OK;
}

/**
 * Retrieve the current background color setting.
 */
VdpStatus
vlVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                         VdpColor *const background_color)
{
   vlVdpPresentationQueue *pq;
   union pipe_color_union color;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Getting background color\n");

   if (!background_color)
      return VDP_STATUS_INVALID_POINTER;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   vl_compositor_get_clear_color(&pq->compositor, &color);

   background_color->red = color.f[0];
   background_color->green = color.f[1];
   background_color->blue = color.f[2];
   background_color->alpha = color.f[3];

   return VDP_STATUS_OK;
}

/**
 * Retrieve the presentation queue's "current" time.
 */
VdpStatus
vlVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                              VdpTime *current_time)
{
   vlVdpPresentationQueue *pq;
   struct timespec ts;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Getting queue time\n");

   if (!current_time)
      return VDP_STATUS_INVALID_POINTER;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   clock_gettime(CLOCK_REALTIME, &ts);
   *current_time = (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;

   return VDP_STATUS_OK;
}

/**
 * Enter a surface into the presentation queue.
 */
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

   struct pipe_context *pipe;
   struct pipe_surface *drawable_surface;
   struct pipe_video_rect src_rect, dst_clip;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   drawable_surface = vl_drawable_surface_get(pq->device->context, pq->drawable);
   if (!drawable_surface)
      return VDP_STATUS_INVALID_HANDLE;

   surf = vlGetDataHTAB(surface);
   if (!surf)
      return VDP_STATUS_INVALID_HANDLE;

   surf->timestamp = (vlVdpTime)earliest_presentation_time;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.w = drawable_surface->width;
   src_rect.h = drawable_surface->height;

   dst_clip.x = 0;
   dst_clip.y = 0;
   dst_clip.w = clip_width;
   dst_clip.h = clip_height;

   vl_compositor_clear_layers(&pq->compositor);
   vl_compositor_set_rgba_layer(&pq->compositor, 0, surf->sampler_view, &src_rect, NULL);
   vl_compositor_render(&pq->compositor, drawable_surface, NULL, &dst_clip, &pq->dirty_area);

   pipe = pq->device->context->pipe;

   pipe->screen->flush_frontbuffer
   (
      pipe->screen,
      drawable_surface->texture,
      0, 0,
      vl_contextprivate_get(pq->device->context, drawable_surface)
   );

   pipe->screen->fence_reference(pipe->screen, &surf->fence, NULL);
   pipe->flush(pipe, &surf->fence);

   if (dump_window == -1) {
      dump_window = debug_get_num_option("VDPAU_DUMP", 0);
   }

   if (dump_window) {
      static unsigned int framenum = 0;
      char cmd[256];

      sprintf(cmd, "xwd -id %d -out vdpau_frame_%08d.xwd", (int)pq->drawable, ++framenum);
      if (system(cmd) != 0)
         VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Dumping surface %d failed.\n", surface);
   }

   pipe_surface_reference(&drawable_surface, NULL);

   return VDP_STATUS_OK;
}

/**
 * Wait for a surface to finish being displayed.
 */
VdpStatus
vlVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                            VdpOutputSurface surface,
                                            VdpTime *first_presentation_time)
{
   vlVdpPresentationQueue *pq;
   vlVdpOutputSurface *surf;
   struct pipe_screen *screen;

   if (!first_presentation_time)
      return VDP_STATUS_INVALID_POINTER;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   surf = vlGetDataHTAB(surface);
   if (!surf)
      return VDP_STATUS_INVALID_HANDLE;

   if (surf->fence) {
      screen = pq->device->context->pipe->screen;
      screen->fence_finish(screen, surf->fence, 0);
   }

   // We actually need to query the timestamp of the last VSYNC event from the hardware
   vlVdpPresentationQueueGetTime(presentation_queue, first_presentation_time);

   return VDP_STATUS_OK;
}

/**
 * Poll the current queue status of a surface.
 */
VdpStatus
vlVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                         VdpOutputSurface surface,
                                         VdpPresentationQueueStatus *status,
                                         VdpTime *first_presentation_time)
{
   vlVdpPresentationQueue *pq;
   vlVdpOutputSurface *surf;
   struct pipe_screen *screen;

   if (!(status && first_presentation_time))
      return VDP_STATUS_INVALID_POINTER;

   pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
      return VDP_STATUS_INVALID_HANDLE;

   surf = vlGetDataHTAB(surface);
   if (!surf)
      return VDP_STATUS_INVALID_HANDLE;

   *first_presentation_time = 0;

   if (!surf->fence) {
      *status = VDP_PRESENTATION_QUEUE_STATUS_IDLE;
   } else {
      screen = pq->device->context->pipe->screen;
      if (screen->fence_signalled(screen, surf->fence)) {
         screen->fence_reference(screen, &surf->fence, NULL);
         *status = VDP_PRESENTATION_QUEUE_STATUS_VISIBLE;

         // We actually need to query the timestamp of the last VSYNC event from the hardware
         vlVdpPresentationQueueGetTime(presentation_queue, first_presentation_time);
         *first_presentation_time += 1;
      } else {
         *status = VDP_PRESENTATION_QUEUE_STATUS_QUEUED;
      }
   }

   return VDP_STATUS_OK;
}
