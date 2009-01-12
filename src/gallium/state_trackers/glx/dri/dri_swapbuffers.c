/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_swapbuffers.h"

#include "pipe/p_context.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"


static void
blit_swapbuffers(__DRIdrawablePrivate *dPriv,
                 __DRIcontextPrivate *cPriv,
		 struct pipe_surface *src,
		 const drm_clip_rect_t *rect)
{
   struct dri_screen *screen = dri_screen(dPriv->driScreenPriv);
   struct dri_drawable *fb = dri_drawable(dPriv);
   struct dri_context *context = dri_context(cPriv);

   const int nbox = dPriv->numClipRects;
   const drm_clip_rect_t *pbox = dPriv->pClipRects;

   struct pipe_surface *dest = fb->front_surface;
   const int backWidth = fb->stfb->Base.Width;
   const int backHeight = fb->stfb->Base.Height;
   int i;

   for (i = 0; i < nbox; i++, pbox++) {
      drm_clip_rect_t box;
      drm_clip_rect_t sbox;
	 
      if (pbox->x1 > pbox->x2 ||
	  pbox->y1 > pbox->y2 ||
	  (pbox->x2 - pbox->x1) > dest->width || 
	  (pbox->y2 - pbox->y1) > dest->height) 
	 continue;

      box = *pbox;

      if (rect) {
	 drm_clip_rect_t rrect;

	 rrect.x1 = dPriv->x + rect->x1;
	 rrect.y1 = (dPriv->h - rect->y1 - rect->y2) + dPriv->y;
	 rrect.x2 = rect->x2 + rrect.x1;
	 rrect.y2 = rect->y2 + rrect.y1;
	 if (rrect.x1 > box.x1)
	    box.x1 = rrect.x1;
	 if (rrect.y1 > box.y1)
	    box.y1 = rrect.y1;
	 if (rrect.x2 < box.x2)
	    box.x2 = rrect.x2;
	 if (rrect.y2 < box.y2)
	    box.y2 = rrect.y2;

	 if (box.x1 > box.x2 || box.y1 > box.y2)
	    continue;
      }

      /* restrict blit to size of actually rendered area */
      if (box.x2 - box.x1 > backWidth)
	 box.x2 = backWidth + box.x1;
      if (box.y2 - box.y1 > backHeight)
	 box.y2 = backHeight + box.y1;

      debug_printf("%s: box %d,%d-%d,%d\n", __FUNCTION__,
                   box.x1, box.y1, box.x2, box.y2);

      sbox.x1 = box.x1 - dPriv->x;
      sbox.y1 = box.y1 - dPriv->y;

      ctx->st->pipe->surface_copy( ctx->st->pipe,
                                   FALSE,
                                   dest,
                                   box.x1, box.y1,
                                   src,
                                   sbox.x1, sbox.y1,
                                   box.x2 - box.x1, 
                                   box.y2 - box.y1 );
   }
}

/**
 * Display a colorbuffer surface in an X window.
 * Used for SwapBuffers and flushing front buffer rendering.
 *
 * \param dPriv  the window/drawable to display into
 * \param surf  the surface to display
 * \param rect  optional subrect of surface to display (may be NULL).
 */
void
dri_display_surface(__DRIdrawablePrivate *dPriv,
                    struct pipe_surface *source,
                    const drm_clip_rect_t *rect)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_screen *screen = dri_screen(dPriv->driScreenPriv);
   struct dri_context *context = screen->dummy_context;
   struct pipe_winsys *winsys = screen->winsys;
      
   if (!context) 
      return;

   if (drawable->last_swap_fence) {
      winsys->fence_finish( winsys,
                            drawable->last_swap_fence,
                            0 );

      winsys->fence_reference( winsys,
                               &drawable->last_swap_fence,
                               NULL );
   }

   drawable->last_swap_fence = drawable->first_swap_fence;
   drawable->first_swap_fence = NULL;

   /* The lock_hardware is required for the cliprects.  Buffer offsets
    * should work regardless.
    */
   dri_lock_hardware(context, drawable);
   {
      if (dPriv->numClipRects) {
         blit_swapbuffers( context, dPriv, source, rect );
      }
   }
   dri_unlock_hardware(context);

   if (drawble->stamp != drawable->dPriv->lastStamp) {
      dri_update_window_size( dpriv );
   }
}



/**
 * This will be called a drawable is known to have moved/resized.
 */
void
dri_update_window_size(__DRIdrawablePrivate *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   st_resize_framebuffer(drawable->stfb, dPriv->w, dPriv->h);
   drawable->stamp = dPriv->lastStamp;
}



void
dri_swap_buffers(__DRIdrawablePrivate * dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_surface *back_surf;

   assert(drawable);
   assert(drawable->stfb);

   back_surf = st_get_framebuffer_surface(drawable->stfb,
                                          ST_SURFACE_BACK_LEFT);
   if (back_surf) {
      st_notify_swapbuffers(drawable->stfb);
      dri_display_surface(dPriv, back_surf, NULL);
      st_notify_swapbuffers_complete(drawable->stfb);
   }
}


/**
 * Called via glXCopySubBufferMESA() to copy a subrect of the back
 * buffer to the front buffer/screen.
 */
void
dri_copy_sub_buffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_surface *back_surf;

   assert(drawable);
   assert(drawable->stfb);

   back_surf = st_get_framebuffer_surface(drawable->stfb,
                                          ST_SURFACE_BACK_LEFT);
   if (back_surf) {
      drm_clip_rect_t rect;
      rect.x1 = x;
      rect.y1 = y;
      rect.x2 = w;
      rect.y2 = h;

      st_notify_swapbuffers(drawable->stfb);
      dri_display_surface(dPriv, back_surf, &rect);
   }
}



/*
 * The state tracker keeps track of whether the fake frontbuffer has
 * been touched by any rendering since the last time we copied its
 * contents to the real frontbuffer.  Our task is easy:
 */
static void
dri_flush_frontbuffer( struct pipe_winsys *winsys,
                       struct pipe_surface *surf,
                       void *context_private)
{
   struct dri_context *dri = (struct dri_context *) context_private;
   __DRIdrawablePrivate *dPriv = dri->driDrawable;

   dri_display_surface(dPriv, surf, NULL);
}



/* Need to create a surface which wraps the front surface to support
 * client-side swapbuffers.
 */
static void
dri_create_front_surface(struct dri_screen *screen, 
                         struct pipe_winsys *winsys, 
                         unsigned handle)
{
   struct pipe_screen *pipe_screen = screen->pipe_screen;
   struct pipe_texture *texture;
   struct pipe_texture templat;
   struct pipe_surface *surface;
   struct pipe_buffer *buffer;
   unsigned pitch;

   assert(screen->front.cpp == 4);

//   buffer = dri_buffer_from_handle(screen->winsys,
//                                        "front", handle);

   if (!buffer)
      return;

   screen->front.buffer = dri_bo(buffer);

   memset(&templat, 0, sizeof(templat));
   templat.tex_usage |= PIPE_TEXTURE_USAGE_DISPLAY_TARGET;
   templat.target = PIPE_TEXTURE_2D;
   templat.last_level = 0;
   templat.depth[0] = 1;
   templat.format = PIPE_FORMAT_A8R8G8B8_UNORM;
   templat.width[0] = screen->front.width;
   templat.height[0] = screen->front.height;
   pf_get_block(templat.format, &templat.block);
   pitch = screen->front.pitch;

   texture = pipe_screen->texture_blanket(pipe_screen,
                                          &templat,
                                          &pitch,
                                          buffer);

   /* Unref the buffer we don't need it anyways */
   pipe_buffer_reference(screen, &buffer, NULL);

   surface = pipe_screen->get_tex_surface(pipe_screen,
                                          texture,
                                          0,
                                          0,
                                          0,
                                          PIPE_BUFFER_USAGE_GPU_WRITE);

   screen->front.texture = texture;
   screen->front.surface = surface;
}
