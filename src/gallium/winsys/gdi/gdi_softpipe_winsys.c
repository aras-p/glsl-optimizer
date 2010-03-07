/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

/**
 * @file
 * Softpipe support.
 *
 * @author Keith Whitwell
 * @author Brian Paul
 * @author Jose Fonseca
 */


#include <windows.h>

#include "util/u_simple_screen.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "softpipe/sp_winsys.h"
#include "softpipe/sp_texture.h"
#include "stw_winsys.h"


struct gdi_softpipe_buffer
{
   struct pipe_buffer base;
   boolean userBuffer;  /** Is this a user-space buffer? */
   void *data;
   void *mapped;
};


/** Cast wrapper */
static INLINE struct gdi_softpipe_buffer *
gdi_softpipe_buffer( struct pipe_buffer *buf )
{
   return (struct gdi_softpipe_buffer *)buf;
}


static void *
gdi_softpipe_buffer_map(struct pipe_winsys *winsys,
                        struct pipe_buffer *buf,
                        unsigned flags)
{
   struct gdi_softpipe_buffer *gdi_softpipe_buf = gdi_softpipe_buffer(buf);
   gdi_softpipe_buf->mapped = gdi_softpipe_buf->data;
   return gdi_softpipe_buf->mapped;
}


static void
gdi_softpipe_buffer_unmap(struct pipe_winsys *winsys,
                          struct pipe_buffer *buf)
{
   struct gdi_softpipe_buffer *gdi_softpipe_buf = gdi_softpipe_buffer(buf);
   gdi_softpipe_buf->mapped = NULL;
}


static void
gdi_softpipe_buffer_destroy(struct pipe_buffer *buf)
{
   struct gdi_softpipe_buffer *oldBuf = gdi_softpipe_buffer(buf);

   if (oldBuf->data) {
      if (!oldBuf->userBuffer)
         align_free(oldBuf->data);

      oldBuf->data = NULL;
   }

   FREE(oldBuf);
}


static const char *
gdi_softpipe_get_name(struct pipe_winsys *winsys)
{
   return "softpipe";
}


static struct pipe_buffer *
gdi_softpipe_buffer_create(struct pipe_winsys *winsys,
                           unsigned alignment,
                           unsigned usage,
                           unsigned size)
{
   struct gdi_softpipe_buffer *buffer = CALLOC_STRUCT(gdi_softpipe_buffer);

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;

   buffer->data = align_malloc(size, alignment);

   return &buffer->base;
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer *
gdi_softpipe_user_buffer_create(struct pipe_winsys *winsys,
                               void *ptr,
                               unsigned bytes)
{
   struct gdi_softpipe_buffer *buffer;

   buffer = CALLOC_STRUCT(gdi_softpipe_buffer);
   if(!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.size = bytes;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}


static struct pipe_buffer *
gdi_softpipe_surface_buffer_create(struct pipe_winsys *winsys,
                                   unsigned width, unsigned height,
                                   enum pipe_format format,
                                   unsigned usage,
                                   unsigned tex_usage,
                                   unsigned *stride)
{
   const unsigned alignment = 64;
   unsigned nblocksy;

   nblocksy = util_format_get_nblocksy(format, height);
   *stride = align(util_format_get_stride(format, width), alignment);

   return winsys->buffer_create(winsys, alignment,
                                usage,
                                *stride * nblocksy);
}


static void
gdi_softpipe_dummy_flush_frontbuffer(struct pipe_winsys *winsys,
                                     struct pipe_surface *surface,
                                     void *context_private)
{
   assert(0);
}


static void
gdi_softpipe_fence_reference(struct pipe_winsys *winsys,
                             struct pipe_fence_handle **ptr,
                             struct pipe_fence_handle *fence)
{
}


static int
gdi_softpipe_fence_signalled(struct pipe_winsys *winsys,
                             struct pipe_fence_handle *fence,
                             unsigned flag)
{
   return 0;
}


static int
gdi_softpipe_fence_finish(struct pipe_winsys *winsys,
                          struct pipe_fence_handle *fence,
                          unsigned flag)
{
   return 0;
}


static void
gdi_softpipe_destroy(struct pipe_winsys *winsys)
{
   FREE(winsys);
}


static struct pipe_screen *
gdi_softpipe_screen_create(void)
{
   static struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = CALLOC_STRUCT(pipe_winsys);
   if(!winsys)
      return NULL;

   winsys->destroy = gdi_softpipe_destroy;

   winsys->buffer_create = gdi_softpipe_buffer_create;
   winsys->user_buffer_create = gdi_softpipe_user_buffer_create;
   winsys->buffer_map = gdi_softpipe_buffer_map;
   winsys->buffer_unmap = gdi_softpipe_buffer_unmap;
   winsys->buffer_destroy = gdi_softpipe_buffer_destroy;

   winsys->surface_buffer_create = gdi_softpipe_surface_buffer_create;

   winsys->fence_reference = gdi_softpipe_fence_reference;
   winsys->fence_signalled = gdi_softpipe_fence_signalled;
   winsys->fence_finish = gdi_softpipe_fence_finish;

   winsys->flush_frontbuffer = gdi_softpipe_dummy_flush_frontbuffer;
   winsys->get_name = gdi_softpipe_get_name;

   screen = softpipe_create_screen(winsys);
   if(!screen)
      gdi_softpipe_destroy(winsys);

   return screen;
}


static void
gdi_softpipe_present(struct pipe_screen *screen,
                     struct pipe_surface *surface,
                     HDC hDC)
{
    struct softpipe_texture *texture;
    struct gdi_softpipe_buffer *buffer;
    BITMAPINFO bmi;

    texture = softpipe_texture(surface->texture);
                                               
    buffer = gdi_softpipe_buffer(texture->buffer);

    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = texture->stride[surface->level] / util_format_get_blocksize(surface->format);
    bmi.bmiHeader.biHeight= -(long)surface->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = util_format_get_blocksizebits(surface->format);
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    StretchDIBits(hDC,
                  0, 0, surface->width, surface->height,
                  0, 0, surface->width, surface->height,
                  buffer->data, &bmi, 0, SRCCOPY);
}


static const struct stw_winsys stw_winsys = {
   &gdi_softpipe_screen_create,
   &gdi_softpipe_present,
   NULL, /* get_adapter_luid */
   NULL, /* shared_surface_open */
   NULL, /* shared_surface_close */
   NULL  /* compose */
};


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
   switch (fdwReason) {
   case DLL_PROCESS_ATTACH:
      stw_init(&stw_winsys);
      stw_init_thread();
      break;

   case DLL_THREAD_ATTACH:
      stw_init_thread();
      break;

   case DLL_THREAD_DETACH:
      stw_cleanup_thread();
      break;

   case DLL_PROCESS_DETACH:
      stw_cleanup_thread();
      stw_cleanup();
      break;
   }
   return TRUE;
}
