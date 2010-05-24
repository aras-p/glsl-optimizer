/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <vl_winsys.h>
#include <state_tracker/xlib_sw_winsys.h>
#include <util/u_memory.h>
#include <softpipe/sp_public.h>
#include <softpipe/sp_video_context.h>

/* TODO: Find a good way to calculate this */
static enum pipe_format VisualToPipe(Visual *visual)
{
   assert(visual);
   return PIPE_FORMAT_B8G8R8X8_UNORM;
}

/* XXX: Not thread-safe */
static struct xlib_drawable xdraw;

void*
vl_displaytarget_get(struct vl_screen *vscreen, Drawable drawable,
                     unsigned *width_out, unsigned *height_out)
{
   Window root;
   int x, y;
   unsigned int width, height;
   unsigned int border_width;
   unsigned int depth;

   assert(vscreen);

   if (XGetGeometry(vscreen->display, drawable, &root, &x, &y, &width, &height, &border_width, &depth) == BadDrawable)
      return NULL;

   if (width_out) *width_out = width;
   if (height_out) *height_out = height;

   xdraw.depth = depth;
   xdraw.drawable = drawable;

   return &xdraw;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_screen *vscreen;
   struct sw_winsys *winsys;

   assert(display);

   vscreen = CALLOC_STRUCT(vl_screen);
   if (!vscreen)
      return NULL;

   winsys = xlib_create_sw_winsys(display);
   if (!winsys) {
      FREE(vscreen);
      return NULL;
   }

   vscreen->pscreen = softpipe_create_screen(winsys);
   if (!vscreen->pscreen) {
      winsys->destroy(winsys);
      FREE(vscreen);
      return NULL;
   }

   vscreen->display = display;
   xdraw.visual = XDefaultVisual(display, screen);
   vscreen->format = VisualToPipe(xdraw.visual);

   return vscreen;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   assert(vscreen);

   vscreen->pscreen->destroy(vscreen->pscreen);
   FREE(vscreen);
}

struct vl_context*
vl_video_create(struct vl_screen *vscreen,
                enum pipe_video_profile profile,
                enum pipe_video_chroma_format chroma_format,
                unsigned width, unsigned height)
{
   struct pipe_video_context *vpipe;
   struct vl_context *vctx;

   assert(vscreen);
   assert(width && height);
   assert(vscreen->pscreen->video_context_create);

   vpipe = vscreen->pscreen->video_context_create(vscreen->pscreen,
                                                  profile,
                                                  chroma_format,
                                                  width, height, NULL);
   if (!vpipe)
      return NULL;

   vctx = CALLOC_STRUCT(vl_context);
   if (!vctx) {
      vpipe->destroy(vpipe);
      return NULL;
   }

   vpipe->priv = vctx;
   vctx->vpipe = vpipe;
   vctx->vscreen = vscreen;

   return vctx;
}

void vl_video_destroy(struct vl_context *vctx)
{
   assert(vctx);

#if 1
   vctx->vpipe->destroy(vctx->vpipe);
#endif
   FREE(vctx);
}
