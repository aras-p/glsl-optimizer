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

#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_inlines.h"

#include <X11/Xlibint.h>

#include "state_tracker/xlib_sw_winsys.h"
#include "softpipe/sp_public.h"

#include "vl_winsys.h"

struct vl_xsp_screen
{
   struct vl_screen base;
   Display *display;
   int screen;
   Visual visual;
   struct xlib_drawable xdraw;
   struct pipe_surface *drawable_surface;
};

struct pipe_surface*
vl_drawable_surface_get(struct vl_context *vctx, Drawable drawable)
{
   struct vl_screen *vscreen = vctx->vscreen;
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vscreen;
   Window root;
   int x, y;
   unsigned int width, height;
   unsigned int border_width;
   unsigned int depth;
   struct pipe_resource templat, *drawable_tex;
   struct pipe_surface surf_template, *drawable_surface = NULL;

   assert(vscreen);
   assert(drawable != None);

   if (XGetGeometry(xsp_screen->display, drawable, &root, &x, &y, &width, &height, &border_width, &depth) == BadDrawable)
      return NULL;

   xsp_screen->xdraw.drawable = drawable;

   if (xsp_screen->drawable_surface) {
      if (xsp_screen->drawable_surface->width == width &&
          xsp_screen->drawable_surface->height == height) {
         pipe_surface_reference(&drawable_surface, xsp_screen->drawable_surface);
         return drawable_surface;
      }
      else
         pipe_surface_reference(&xsp_screen->drawable_surface, NULL);
   }

   memset(&templat, 0, sizeof(struct pipe_resource));
   templat.target = PIPE_TEXTURE_2D;
   /* XXX: Need to figure out drawable's format */
   templat.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   templat.last_level = 0;
   templat.width0 = width;
   templat.height0 = height;
   templat.depth0 = 1;
   templat.usage = PIPE_USAGE_DEFAULT;
   templat.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_DISPLAY_TARGET;
   templat.flags = 0;

   drawable_tex = vscreen->pscreen->resource_create(vscreen->pscreen, &templat);
   if (!drawable_tex)
      return NULL;

   memset(&surf_template, 0, sizeof(surf_template));
   surf_template.format = templat.format;
   surf_template.usage = PIPE_BIND_RENDER_TARGET;
   xsp_screen->drawable_surface = vctx->pipe->create_surface(vctx->pipe, drawable_tex,
                                                             &surf_template);
   pipe_resource_reference(&drawable_tex, NULL);

   if (!xsp_screen->drawable_surface)
      return NULL;

   pipe_surface_reference(&drawable_surface, xsp_screen->drawable_surface);

   xsp_screen->xdraw.depth = 24/*util_format_get_blocksizebits(templat.format) /
                             util_format_get_blockwidth(templat.format)*/;

   return drawable_surface;
}

void*
vl_contextprivate_get(struct vl_context *vctx, struct pipe_surface *drawable_surface)
{
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vctx->vscreen;

   assert(vctx);
   assert(drawable_surface);
   assert(xsp_screen->drawable_surface == drawable_surface);

   return &xsp_screen->xdraw;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_xsp_screen *xsp_screen;
   struct sw_winsys *winsys;

   assert(display);

   xsp_screen = CALLOC_STRUCT(vl_xsp_screen);
   if (!xsp_screen)
      return NULL;

   winsys = xlib_create_sw_winsys(display);
   if (!winsys) {
      FREE(xsp_screen);
      return NULL;
   }

   xsp_screen->base.pscreen = softpipe_create_screen(winsys);
   if (!xsp_screen->base.pscreen) {
      winsys->destroy(winsys);
      FREE(xsp_screen);
      return NULL;
   }

   xsp_screen->display = display;
   xsp_screen->screen = screen;
   xsp_screen->xdraw.visual = XDefaultVisual(display, screen);

   return &xsp_screen->base;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vscreen;

   assert(vscreen);

   pipe_surface_reference(&xsp_screen->drawable_surface, NULL);
   vscreen->pscreen->destroy(vscreen->pscreen);
   FREE(vscreen);
}

struct vl_context*
vl_video_create(struct vl_screen *vscreen)
{
   struct pipe_context *pipe;
   struct vl_context *vctx;

   assert(vscreen);

   pipe = vscreen->pscreen->context_create(vscreen->pscreen, NULL);
   if (!pipe)
      return NULL;

   vctx = CALLOC_STRUCT(vl_context);
   if (!vctx) {
      pipe->destroy(pipe);
      return NULL;
   }

   vctx->pipe = pipe;
   vctx->vscreen = vscreen;

   return vctx;
}

void vl_video_destroy(struct vl_context *vctx)
{
   assert(vctx);

   vctx->pipe->destroy(vctx->pipe);
   FREE(vctx);
}
