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
#include <X11/Xutil.h>
#include <util/u_simple_screen.h>
#include <pipe/p_state.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include <util/u_math.h>
#include <softpipe/sp_winsys.h>
#include <softpipe/sp_video_context.h>
#include <softpipe/sp_texture.h>

/* TODO: Find a good way to calculate this */
static enum pipe_format VisualToPipe(Visual *visual)
{
   assert(visual);
   return PIPE_FORMAT_B8G8R8X8_UNORM;
}

struct xsp_pipe_winsys
{
   struct pipe_winsys base;
   Display *display;
   int screen;
   XImage *fbimage;
};

struct xsp_context
{
   struct vl_context base;

   Drawable drawable;
};

struct xsp_buffer
{
   struct pipe_buffer base;
   boolean is_user_buffer;
   void *data;
   void *mapped_data;
};

static struct pipe_buffer* xsp_buffer_create(struct pipe_winsys *pws, unsigned alignment, unsigned usage, unsigned size)
{
   struct xsp_buffer *buffer;

   assert(pws);

   buffer = calloc(1, sizeof(struct xsp_buffer));
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;
   buffer->data = align_malloc(size, alignment);

   return (struct pipe_buffer*)buffer;
}

static struct pipe_buffer* xsp_user_buffer_create(struct pipe_winsys *pws, void *data, unsigned size)
{
   struct xsp_buffer *buffer;

   assert(pws);

   buffer = calloc(1, sizeof(struct xsp_buffer));
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.size = size;
   buffer->is_user_buffer = TRUE;
   buffer->data = data;

   return (struct pipe_buffer*)buffer;
}

static void* xsp_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buffer, unsigned flags)
{
   struct xsp_buffer *xsp_buf = (struct xsp_buffer*)buffer;

   assert(pws);
   assert(buffer);

   xsp_buf->mapped_data = xsp_buf->data;

   return xsp_buf->mapped_data;
}

static void xsp_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buffer)
{
   struct xsp_buffer *xsp_buf = (struct xsp_buffer*)buffer;

   assert(pws);
   assert(buffer);

   xsp_buf->mapped_data = NULL;
}

static void xsp_buffer_destroy(struct pipe_buffer *buffer)
{
   struct xsp_buffer *xsp_buf = (struct xsp_buffer*)buffer;

   assert(buffer);

   if (!xsp_buf->is_user_buffer)
      align_free(xsp_buf->data);

   free(xsp_buf);
}

static struct pipe_buffer* xsp_surface_buffer_create
(
   struct pipe_winsys *pws,
   unsigned width,
   unsigned height,
   enum pipe_format format,
   unsigned usage,
   unsigned tex_usage,
   unsigned *stride
)
{
   const unsigned int ALIGNMENT = 1;
   unsigned nblocksy;

   nblocksy = util_format_get_nblocksy(format, height);
   *stride = align(util_format_get_stride(format, width), ALIGNMENT);

   return pws->buffer_create(pws, ALIGNMENT, usage,
                             *stride * nblocksy);
}

static void xsp_fence_reference(struct pipe_winsys *pws, struct pipe_fence_handle **ptr, struct pipe_fence_handle *fence)
{
   assert(pws);
   assert(ptr);
   assert(fence);
}

static int xsp_fence_signalled(struct pipe_winsys *pws, struct pipe_fence_handle *fence, unsigned flag)
{
   assert(pws);
   assert(fence);

   return 0;
}

static int xsp_fence_finish(struct pipe_winsys *pws, struct pipe_fence_handle *fence, unsigned flag)
{
   assert(pws);
   assert(fence);

   return 0;
}

static void xsp_flush_frontbuffer(struct pipe_winsys *pws, struct pipe_surface *surface, void *context_private)
{
   struct xsp_pipe_winsys *xsp_winsys;
   struct xsp_context *xsp_context;

   assert(pws);
   assert(surface);
   assert(context_private);

   xsp_winsys = (struct xsp_pipe_winsys*)pws;
   xsp_context = (struct xsp_context*)context_private;
   xsp_winsys->fbimage->width = surface->width;
   xsp_winsys->fbimage->height = surface->height;
   xsp_winsys->fbimage->bytes_per_line = surface->width * (xsp_winsys->fbimage->bits_per_pixel >> 3);
   xsp_winsys->fbimage->data = (char*)((struct xsp_buffer *)softpipe_texture(surface->texture)->buffer)->data + surface->offset;

   XPutImage
   (
      xsp_winsys->display, xsp_context->drawable,
      XDefaultGC(xsp_winsys->display, xsp_winsys->screen),
      xsp_winsys->fbimage, 0, 0, 0, 0,
      surface->width, surface->height
   );
   XFlush(xsp_winsys->display);
}

static const char* xsp_get_name(struct pipe_winsys *pws)
{
   assert(pws);
   return "X11 SoftPipe";
}

static void xsp_destroy(struct pipe_winsys *pws)
{
   struct xsp_pipe_winsys *xsp_winsys = (struct xsp_pipe_winsys*)pws;

   assert(pws);

   /* XDestroyImage() wants to free the data as well */
   xsp_winsys->fbimage->data = NULL;

   XDestroyImage(xsp_winsys->fbimage);
   FREE(xsp_winsys);
}

Drawable
vl_video_bind_drawable(struct vl_context *vctx, Drawable drawable)
{
   struct xsp_context *xsp_context = (struct xsp_context*)vctx;
   Drawable old_drawable;

   assert(vctx);

   old_drawable = xsp_context->drawable;
   xsp_context->drawable = drawable;

   return old_drawable;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_screen *vscreen;
   struct xsp_pipe_winsys *xsp_winsys;

   assert(display);

   vscreen = CALLOC_STRUCT(vl_screen);
   if (!vscreen)
      return NULL;

   xsp_winsys = CALLOC_STRUCT(xsp_pipe_winsys);
   if (!xsp_winsys) {
      FREE(vscreen);
      return NULL;
   }

   xsp_winsys->base.buffer_create = xsp_buffer_create;
   xsp_winsys->base.user_buffer_create = xsp_user_buffer_create;
   xsp_winsys->base.buffer_map = xsp_buffer_map;
   xsp_winsys->base.buffer_unmap = xsp_buffer_unmap;
   xsp_winsys->base.buffer_destroy = xsp_buffer_destroy;
   xsp_winsys->base.surface_buffer_create = xsp_surface_buffer_create;
   xsp_winsys->base.fence_reference = xsp_fence_reference;
   xsp_winsys->base.fence_signalled = xsp_fence_signalled;
   xsp_winsys->base.fence_finish = xsp_fence_finish;
   xsp_winsys->base.flush_frontbuffer = xsp_flush_frontbuffer;
   xsp_winsys->base.get_name = xsp_get_name;
   xsp_winsys->base.destroy = xsp_destroy;
   xsp_winsys->display = display;
   xsp_winsys->screen = screen;
   xsp_winsys->fbimage = XCreateImage
   (
      display,
      XDefaultVisual(display, screen),
      XDefaultDepth(display, screen),
      ZPixmap,
      0,
      NULL,
      0, /* Don't know the width and height until flush_frontbuffer */
      0,
      32,
      0
   );

   if (!xsp_winsys->fbimage) {
      FREE(xsp_winsys);
      FREE(vscreen);
      return NULL;
   }

   XInitImage(xsp_winsys->fbimage);

   vscreen->pscreen = softpipe_create_screen(&xsp_winsys->base);

   if (!vscreen->pscreen) {
      FREE(vscreen);
      XDestroyImage(xsp_winsys->fbimage);
      FREE(xsp_winsys);
      return NULL;
   }

   vscreen->format = VisualToPipe(XDefaultVisual(display, screen));

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
   struct xsp_context *xsp_context;

   assert(vscreen);
   assert(width && height);

   vpipe = sp_video_create(vscreen->pscreen, profile, chroma_format, width, height);
   if (!vpipe)
      return NULL;

   xsp_context = CALLOC_STRUCT(xsp_context);
   if (!xsp_context) {
      vpipe->destroy(vpipe);
      return NULL;
   }

   vpipe->priv = xsp_context;
   xsp_context->base.vpipe = vpipe;
   xsp_context->base.vscreen = vscreen;

   return &xsp_context->base;
}

void vl_video_destroy(struct vl_context *vctx)
{
   assert(vctx);

   vctx->vpipe->destroy(vctx->vpipe);
   FREE(vctx);
}
