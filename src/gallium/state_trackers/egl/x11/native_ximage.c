/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "pipe/p_compiler.h"
#include "util/u_simple_screen.h"
#include "util/u_inlines.h"
#include "softpipe/sp_winsys.h"
#include "egllog.h"

#include "sw_winsys.h"
#include "native_x11.h"
#include "x11_screen.h"

enum ximage_surface_type {
   XIMAGE_SURFACE_TYPE_WINDOW,
   XIMAGE_SURFACE_TYPE_PIXMAP,
   XIMAGE_SURFACE_TYPE_PBUFFER
};

struct ximage_display {
   struct native_display base;
   Display *dpy;
   boolean own_dpy;

   struct x11_screen *xscr;
   int xscr_number;

   struct native_event_handler *event_handler;

   boolean use_xshm;

   struct pipe_winsys *winsys;
   struct ximage_config *configs;
   int num_configs;
};

struct ximage_buffer {
   XImage *ximage;

   struct pipe_texture *texture;
   XShmSegmentInfo *shm_info;
   boolean xshm_attached;
};

struct ximage_surface {
   struct native_surface base;
   Drawable drawable;
   enum ximage_surface_type type;
   enum pipe_format color_format;
   XVisualInfo visual;
   struct ximage_display *xdpy;

   GC gc;

   unsigned int server_stamp;
   unsigned int client_stamp;
   int width, height;
   struct ximage_buffer buffers[NUM_NATIVE_ATTACHMENTS];
   uint valid_mask;
};

struct ximage_config {
   struct native_config base;
   const XVisualInfo *visual;
};

static INLINE struct ximage_display *
ximage_display(const struct native_display *ndpy)
{
   return (struct ximage_display *) ndpy;
}

static INLINE struct ximage_surface *
ximage_surface(const struct native_surface *nsurf)
{
   return (struct ximage_surface *) nsurf;
}

static INLINE struct ximage_config *
ximage_config(const struct native_config *nconf)
{
   return (struct ximage_config *) nconf;
}

static void
ximage_surface_free_buffer(struct native_surface *nsurf,
                           enum native_attachment which)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   struct ximage_buffer *xbuf = &xsurf->buffers[which];

   pipe_texture_reference(&xbuf->texture, NULL);

   if (xbuf->shm_info) {
      if (xbuf->xshm_attached)
         XShmDetach(xsurf->xdpy->dpy, xbuf->shm_info);
      if (xbuf->shm_info->shmaddr != (void *) -1)
         shmdt(xbuf->shm_info->shmaddr);
      if (xbuf->shm_info->shmid != -1)
         shmctl(xbuf->shm_info->shmid, IPC_RMID, 0);

      xbuf->shm_info->shmaddr = (void *) -1;
      xbuf->shm_info->shmid = -1;
   }
}

static boolean
ximage_surface_alloc_buffer(struct native_surface *nsurf,
                            enum native_attachment which)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   struct ximage_buffer *xbuf = &xsurf->buffers[which];
   struct pipe_screen *screen = xsurf->xdpy->base.screen;
   struct pipe_texture templ;

   /* free old data */
   if (xbuf->texture)
      ximage_surface_free_buffer(&xsurf->base, which);

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = xsurf->color_format;
   templ.width0 = xsurf->width;
   templ.height0 = xsurf->height;
   templ.depth0 = 1;
   templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;

   if (xbuf->shm_info) {
      struct pipe_buffer *pbuf;
      unsigned stride, size;
      void *addr = NULL;

      stride = util_format_get_stride(xsurf->color_format, xsurf->width);
      /* alignment should depend on visual? */
      stride = align(stride, 4);
      size = stride * xsurf->height;

      /* create and attach shm object */
      xbuf->shm_info->shmid = shmget(IPC_PRIVATE, size, 0755);
      if (xbuf->shm_info->shmid != -1) {
         xbuf->shm_info->shmaddr =
            shmat(xbuf->shm_info->shmid, NULL, 0);
         if (xbuf->shm_info->shmaddr != (void *) -1) {
            if (XShmAttach(xsurf->xdpy->dpy, xbuf->shm_info)) {
               addr = xbuf->shm_info->shmaddr;
               xbuf->xshm_attached = TRUE;
            }
         }
      }

      if (addr) {
         pbuf = screen->user_buffer_create(screen, addr, size);
         if (pbuf) {
            xbuf->texture =
               screen->texture_blanket(screen, &templ, &stride, pbuf);
            pipe_buffer_reference(&pbuf, NULL);
         }
      }
   }
   else {
      xbuf->texture = screen->texture_create(screen, &templ);
   }

   /* clean up the buffer if allocation failed */
   if (!xbuf->texture)
      ximage_surface_free_buffer(&xsurf->base, which);

   return (xbuf->texture != NULL);
}

/**
 * Update the geometry of the surface.  Return TRUE if the geometry has changed
 * since last call.
 */
static boolean
ximage_surface_update_geometry(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   Status ok;
   Window root;
   int x, y;
   unsigned int w, h, border, depth;
   boolean updated = FALSE;

   /* pbuffer has fixed geometry */
   if (xsurf->type == XIMAGE_SURFACE_TYPE_PBUFFER)
      return FALSE;

   ok = XGetGeometry(xsurf->xdpy->dpy, xsurf->drawable,
         &root, &x, &y, &w, &h, &border, &depth);
   if (ok && (xsurf->width != w || xsurf->height != h)) {
      xsurf->width = w;
      xsurf->height = h;

      xsurf->server_stamp++;
      updated = TRUE;
   }

   return updated;
}

static void
ximage_surface_notify_invalid(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   struct ximage_display *xdpy = xsurf->xdpy;

   xdpy->event_handler->invalid_surface(&xdpy->base,
         &xsurf->base, xsurf->server_stamp);
}

/**
 * Update the buffers of the surface.  It is a slow function due to the
 * round-trip to the server.
 */
static boolean
ximage_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   boolean updated;
   uint new_valid;
   int att;

   updated = ximage_surface_update_geometry(&xsurf->base);
   if (updated) {
      /* all buffers become invalid */
      xsurf->valid_mask = 0x0;
   }
   else {
      buffer_mask &= ~xsurf->valid_mask;
      /* all requested buffers are valid */
      if (!buffer_mask) {
         xsurf->client_stamp = xsurf->server_stamp;
         return TRUE;
      }
   }

   new_valid = 0x0;
   for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
      if (native_attachment_mask_test(buffer_mask, att)) {
         struct ximage_buffer *xbuf = &xsurf->buffers[att];

         /* reallocate the texture */
         if (!ximage_surface_alloc_buffer(&xsurf->base, att))
            break;

         /* update ximage */
         if (xbuf->ximage) {
            xbuf->ximage->width = xsurf->width;
            xbuf->ximage->height = xsurf->height;
         }

         new_valid |= (1 << att);
         if (buffer_mask == new_valid)
            break;
      }
   }

   xsurf->valid_mask |= new_valid;
   xsurf->client_stamp = xsurf->server_stamp;

   return (new_valid == buffer_mask);
}

static boolean
ximage_surface_draw_buffer(struct native_surface *nsurf,
                           enum native_attachment which)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   struct ximage_buffer *xbuf = &xsurf->buffers[which];
   struct pipe_screen *screen = xsurf->xdpy->base.screen;
   struct pipe_transfer *transfer;

   if (xsurf->type == XIMAGE_SURFACE_TYPE_PBUFFER)
      return TRUE;

   assert(xsurf->drawable && xbuf->ximage && xbuf->texture);

   transfer = screen->get_tex_transfer(screen, xbuf->texture,
         0, 0, 0, PIPE_TRANSFER_READ, 0, 0, xsurf->width, xsurf->height);
   if (!transfer)
      return FALSE;

   xbuf->ximage->bytes_per_line = transfer->stride;
   xbuf->ximage->data = screen->transfer_map(screen, transfer);
   if (!xbuf->ximage->data) {
      screen->tex_transfer_destroy(transfer);
      return FALSE;
   }


   if (xbuf->shm_info)
      XShmPutImage(xsurf->xdpy->dpy, xsurf->drawable, xsurf->gc,
            xbuf->ximage, 0, 0, 0, 0, xsurf->width, xsurf->height, False);
   else
      XPutImage(xsurf->xdpy->dpy, xsurf->drawable, xsurf->gc,
            xbuf->ximage, 0, 0, 0, 0, xsurf->width, xsurf->height);

   xbuf->ximage->data = NULL;
   screen->transfer_unmap(screen, transfer);

   /*
    * softpipe allows the pipe transfer to be re-used, but we don't want to
    * rely on that behavior.
    */
   screen->tex_transfer_destroy(transfer);

   XSync(xsurf->xdpy->dpy, FALSE);

   return TRUE;
}

static boolean
ximage_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   boolean ret;

   ret = ximage_surface_draw_buffer(&xsurf->base,
         NATIVE_ATTACHMENT_FRONT_LEFT);
   /* force buffers to be updated in next validation call */
   xsurf->server_stamp++;
   ximage_surface_notify_invalid(&xsurf->base);

   return ret;
}

static boolean
ximage_surface_swap_buffers(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   struct ximage_buffer *xfront, *xback, xtmp;
   boolean ret;

   /* display the back buffer first */
   ret = ximage_surface_draw_buffer(nsurf, NATIVE_ATTACHMENT_BACK_LEFT);
   /* force buffers to be updated in next validation call */
   xsurf->server_stamp++;
   ximage_surface_notify_invalid(&xsurf->base);

   xfront = &xsurf->buffers[NATIVE_ATTACHMENT_FRONT_LEFT];
   xback = &xsurf->buffers[NATIVE_ATTACHMENT_BACK_LEFT];

   /* skip swapping so that the front buffer is allocated only when needed */
   if (!xfront->texture)
      return ret;

   xtmp = *xfront;
   *xfront = *xback;
   *xback = xtmp;

   return ret;
}

static boolean
ximage_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                        unsigned int *seq_num, struct pipe_texture **textures,
                        int *width, int *height)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);

   if (xsurf->client_stamp != xsurf->server_stamp ||
       (xsurf->valid_mask & attachment_mask) != attachment_mask) {
      if (!ximage_surface_update_buffers(&xsurf->base, attachment_mask))
         return FALSE;
   }

   if (seq_num)
      *seq_num = xsurf->client_stamp;

   if (textures) {
      int att;
      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         if (native_attachment_mask_test(attachment_mask, att)) {
            struct ximage_buffer *xbuf = &xsurf->buffers[att];

            textures[att] = NULL;
            pipe_texture_reference(&textures[att], xbuf->texture);
         }
      }
   }

   if (width)
      *width = xsurf->width;
   if (height)
      *height = xsurf->height;

   return TRUE;
}

static void
ximage_surface_wait(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   XSync(xsurf->xdpy->dpy, FALSE);
   /* TODO XGetImage and update the front texture */
}

static void
ximage_surface_destroy(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   int i;

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      struct ximage_buffer *xbuf = &xsurf->buffers[i];
      ximage_surface_free_buffer(&xsurf->base, i);
      /* xbuf->shm_info is owned by xbuf->ximage? */
      if (xbuf->ximage) {
         XDestroyImage(xbuf->ximage);
         xbuf->ximage = NULL;
      }
   }

   if (xsurf->type != XIMAGE_SURFACE_TYPE_PBUFFER)
      XFreeGC(xsurf->xdpy->dpy, xsurf->gc);
   free(xsurf);
}

static struct ximage_surface *
ximage_display_create_surface(struct native_display *ndpy,
                              enum ximage_surface_type type,
                              Drawable drawable,
                              const struct native_config *nconf)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   struct ximage_config *xconf = ximage_config(nconf);
   struct ximage_surface *xsurf;
   int i;

   xsurf = CALLOC_STRUCT(ximage_surface);
   if (!xsurf)
      return NULL;

   xsurf->xdpy = xdpy;
   xsurf->type = type;
   xsurf->color_format = xconf->base.color_format;
   xsurf->drawable = drawable;

   if (xsurf->type != XIMAGE_SURFACE_TYPE_PBUFFER) {
      xsurf->drawable = drawable;
      xsurf->visual = *xconf->visual;

      xsurf->gc = XCreateGC(xdpy->dpy, xsurf->drawable, 0, NULL);
      if (!xsurf->gc) {
         free(xsurf);
         return NULL;
      }

      /* initialize the geometry */
      ximage_surface_update_buffers(&xsurf->base, 0x0);

      for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
         struct ximage_buffer *xbuf = &xsurf->buffers[i];

         if (xdpy->use_xshm) {
            xbuf->shm_info = calloc(1, sizeof(*xbuf->shm_info));
            if (xbuf->shm_info) {
               /* initialize shm info */
               xbuf->shm_info->shmid = -1;
               xbuf->shm_info->shmaddr = (void *) -1;
               xbuf->shm_info->readOnly = TRUE;

               xbuf->ximage = XShmCreateImage(xsurf->xdpy->dpy,
                     xsurf->visual.visual,
                     xsurf->visual.depth,
                     ZPixmap, NULL,
                     xbuf->shm_info,
                     0, 0);
            }
         }
         else {
            xbuf->ximage = XCreateImage(xsurf->xdpy->dpy,
                  xsurf->visual.visual,
                  xsurf->visual.depth,
                  ZPixmap, 0,   /* format, offset */
                  NULL,         /* data */
                  0, 0,         /* size */
                  8,            /* bitmap_pad */
                  0);           /* bytes_per_line */
         }

         if (!xbuf->ximage) {
            XFreeGC(xdpy->dpy, xsurf->gc);
            free(xsurf);
            return NULL;
         }
      }
   }

   xsurf->base.destroy = ximage_surface_destroy;
   xsurf->base.swap_buffers = ximage_surface_swap_buffers;
   xsurf->base.flush_frontbuffer = ximage_surface_flush_frontbuffer;
   xsurf->base.validate = ximage_surface_validate;
   xsurf->base.wait = ximage_surface_wait;

   return xsurf;
}

static struct native_surface *
ximage_display_create_window_surface(struct native_display *ndpy,
                                     EGLNativeWindowType win,
                                     const struct native_config *nconf)
{
   struct ximage_surface *xsurf;

   xsurf = ximage_display_create_surface(ndpy, XIMAGE_SURFACE_TYPE_WINDOW,
         (Drawable) win, nconf);
   return (xsurf) ? &xsurf->base : NULL;
}

static struct native_surface *
ximage_display_create_pixmap_surface(struct native_display *ndpy,
                                     EGLNativePixmapType pix,
                                     const struct native_config *nconf)
{
   struct ximage_surface *xsurf;

   xsurf = ximage_display_create_surface(ndpy, XIMAGE_SURFACE_TYPE_PIXMAP,
         (Drawable) pix, nconf);
   return (xsurf) ? &xsurf->base : NULL;
}

static struct native_surface *
ximage_display_create_pbuffer_surface(struct native_display *ndpy,
                                      const struct native_config *nconf,
                                      uint width, uint height)
{
   struct ximage_surface *xsurf;

   xsurf = ximage_display_create_surface(ndpy, XIMAGE_SURFACE_TYPE_PBUFFER,
         (Drawable) None, nconf);
   if (xsurf) {
      xsurf->width = width;
      xsurf->height = height;
   }
   return (xsurf) ? &xsurf->base : NULL;
}

static enum pipe_format
choose_format(const XVisualInfo *vinfo)
{
   enum pipe_format fmt;
   /* TODO elaborate the formats */
   switch (vinfo->depth) {
   case 32:
      fmt = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 24:
      fmt = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   case 16:
      fmt = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      fmt = PIPE_FORMAT_NONE;
      break;
   }

   return fmt;
}

static const struct native_config **
ximage_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   const struct native_config **configs;
   int i;

   /* first time */
   if (!xdpy->configs) {
      const XVisualInfo *visuals;
      int num_visuals, count, j;

      visuals = x11_screen_get_visuals(xdpy->xscr, &num_visuals);
      if (!visuals)
         return NULL;

      /*
       * Create two configs for each visual.
       * One with depth/stencil buffer; one without
       */
      xdpy->configs = calloc(num_visuals * 2, sizeof(*xdpy->configs));
      if (!xdpy->configs)
         return NULL;

      count = 0;
      for (i = 0; i < num_visuals; i++) {
         for (j = 0; j < 2; j++) {
            struct ximage_config *xconf = &xdpy->configs[count];
            __GLcontextModes *mode = &xconf->base.mode;

            xconf->visual = &visuals[i];
            xconf->base.color_format = choose_format(xconf->visual);
            if (xconf->base.color_format == PIPE_FORMAT_NONE)
               continue;

            x11_screen_convert_visual(xdpy->xscr, xconf->visual, mode);
            /* support double buffer mode */
            mode->doubleBufferMode = TRUE;

            xconf->base.depth_format = PIPE_FORMAT_NONE;
            xconf->base.stencil_format = PIPE_FORMAT_NONE;
            /* create the second config with depth/stencil buffer */
            if (j == 1) {
               xconf->base.depth_format = PIPE_FORMAT_Z24S8_UNORM;
               xconf->base.stencil_format = PIPE_FORMAT_Z24S8_UNORM;
               mode->depthBits = 24;
               mode->stencilBits = 8;
               mode->haveDepthBuffer = TRUE;
               mode->haveStencilBuffer = TRUE;
            }

            mode->maxPbufferWidth = 4096;
            mode->maxPbufferHeight = 4096;
            mode->maxPbufferPixels = 4096 * 4096;
            mode->drawableType =
               GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
            mode->swapMethod = GLX_SWAP_EXCHANGE_OML;

            if (mode->alphaBits)
               mode->bindToTextureRgba = TRUE;
            else
               mode->bindToTextureRgb = TRUE;

            count++;
         }
      }

      xdpy->num_configs = count;
   }

   configs = malloc(xdpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < xdpy->num_configs; i++)
         configs[i] = (const struct native_config *) &xdpy->configs[i];
      if (num_configs)
         *num_configs = xdpy->num_configs;
   }
   return configs;
}

static boolean
ximage_display_is_pixmap_supported(struct native_display *ndpy,
                                   EGLNativePixmapType pix,
                                   const struct native_config *nconf)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   enum pipe_format fmt;
   uint depth;

   depth = x11_drawable_get_depth(xdpy->xscr, (Drawable) pix);
   switch (depth) {
   case 32:
      fmt = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 24:
      fmt = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   case 16:
      fmt = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      fmt = PIPE_FORMAT_NONE;
      break;
   }

   return (fmt == nconf->color_format);
}

static int
ximage_display_get_param(struct native_display *ndpy,
                         enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
      /* private buffers are allocated */
      val = FALSE;
      break;
   default:
      val = 0;
      break;
   }

   return val;
}

static void
ximage_display_destroy(struct native_display *ndpy)
{
   struct ximage_display *xdpy = ximage_display(ndpy);

   if (xdpy->configs)
      free(xdpy->configs);

   xdpy->base.screen->destroy(xdpy->base.screen);
   free(xdpy->winsys);

   x11_screen_destroy(xdpy->xscr);
   if (xdpy->own_dpy)
      XCloseDisplay(xdpy->dpy);
   free(xdpy);
}

struct native_display *
x11_create_ximage_display(EGLNativeDisplayType dpy,
                          struct native_event_handler *event_handler,
                          boolean use_xshm)
{
   struct ximage_display *xdpy;

   xdpy = CALLOC_STRUCT(ximage_display);
   if (!xdpy)
      return NULL;

   xdpy->dpy = dpy;
   if (!xdpy->dpy) {
      xdpy->dpy = XOpenDisplay(NULL);
      if (!xdpy->dpy) {
         free(xdpy);
         return NULL;
      }
      xdpy->own_dpy = TRUE;
   }

   xdpy->xscr_number = DefaultScreen(xdpy->dpy);
   xdpy->xscr = x11_screen_create(xdpy->dpy, xdpy->xscr_number);
   if (!xdpy->xscr) {
      free(xdpy);
      return NULL;
   }

   xdpy->event_handler = event_handler;

   xdpy->use_xshm =
      (use_xshm && x11_screen_support(xdpy->xscr, X11_SCREEN_EXTENSION_XSHM));

   xdpy->winsys = create_sw_winsys();
   xdpy->base.screen = softpipe_create_screen(xdpy->winsys);

   xdpy->base.destroy = ximage_display_destroy;
   xdpy->base.get_param = ximage_display_get_param;

   xdpy->base.get_configs = ximage_display_get_configs;
   xdpy->base.is_pixmap_supported = ximage_display_is_pixmap_supported;
   xdpy->base.create_window_surface = ximage_display_create_window_surface;
   xdpy->base.create_pixmap_surface = ximage_display_create_pixmap_surface;
   xdpy->base.create_pbuffer_surface = ximage_display_create_pbuffer_surface;

   return &xdpy->base;
}
