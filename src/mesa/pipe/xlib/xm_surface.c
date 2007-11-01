/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


/**
 * \file xm_surface.c
 * Code to allow the softpipe code to write to X windows/buffers.
 * This is a bit of a hack for now.  We've basically got two different
 * abstractions for color buffers: gl_renderbuffer and pipe_surface.
 * They'll need to get merged someday...
 * For now, they're separate things that point to each other.
 */


#include "GL/xmesa.h"
#include "glxheader.h"
#include "xmesaP.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/macros.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/softpipe/sp_context.h"
#include "pipe/softpipe/sp_clear.h"
#include "pipe/softpipe/sp_tile_cache.h"
#include "pipe/softpipe/sp_surface.h"
#include "state_tracker/st_context.h"


#define CLIP_TILE \
   do { \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height -y; \
   } while(0)


static INLINE struct xmesa_renderbuffer *
xmesa_rb(struct pipe_surface *ps)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   return xms->xrb;
}


#define FLIP(Y) Y = xrb->St.Base.Height - (Y) - 1;


/**
 * Return raw pixels from pixmap or XImage.
 */
void
xmesa_get_tile(struct pipe_context *pipe, struct pipe_surface *ps,
               uint x, uint y, uint w, uint h, void *p, int dst_stride)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   XMesaImage *ximage = NULL;
   ubyte *dst = (ubyte *) p;
   uint i;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_get_tile(pipe, ps, x, y, w, h, p, dst_stride);
      return;
   }

   if (!xms->ximage) {
      /* XImage = pixmap data */
      assert(xms->drawable);
      ximage = XGetImage(xms->display, xms->drawable, x, y, w, h,
                         AllPlanes, ZPixmap);
      x = y = 0;
   }
   else {
      ximage = xms->ximage;
   }
   
   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      if (!dst_stride) {
         dst_stride = w * 4;
      }
      for (i = 0; i < h; i++) {
         memcpy(dst, ximage->data + y * ximage->bytes_per_line + x * 4, 4 * w);
         dst += dst_stride;
      }
      break;
   case PIPE_FORMAT_U_R5_G6_B5:
      if (!dst_stride) {
         dst_stride = w * 2;
      }
      for (i = 0; i < h; i++) {
         memcpy(dst, ximage->data + y * ximage->bytes_per_line + x * 2, 4 * 2);
         dst += dst_stride;
      }
      break;
   default:
      assert(0);
   }

   if (!xms->ximage) {
      XMesaDestroyImage(ximage);
   }
}


/**
 * Put raw pixels into pixmap or XImage.
 */
void
xmesa_put_tile(struct pipe_context *pipe, struct pipe_surface *ps,
               uint x, uint y, uint w, uint h, const void *p, int src_stride)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   const ubyte *src = (const ubyte *) p;
   XMesaImage *ximage;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_put_tile(pipe, ps, x, y, w, h, p, src_stride);
      return;
   }

   if (xms->ximage) {
      /* put to ximage */
      ximage = xms->ximage;
      char *dst;
      int i;

      switch (ps->format) {
      case PIPE_FORMAT_U_A8_R8_G8_B8:
         if (!src_stride) {
            src_stride = w * 4;
         }
         dst = ximage->data + y * ximage->bytes_per_line + x * 4;
         for (i = 0; i < h; i++) {
            memcpy(dst, src, w * 4);
            dst += ximage->bytes_per_line;
            src += src_stride;
         }
         break;
      case PIPE_FORMAT_U_R5_G6_B5:
         if (!src_stride) {
            src_stride = w * 2;
         }
         dst = ximage->data + y * ximage->bytes_per_line + x * 2;
         for (i = 0; i < h; i++) {
            memcpy(dst, src, w * 2);
            dst += ximage->bytes_per_line;
            src += src_stride;
         }
         break;
      default:
         assert(0);
      }
   }
   else {
      /* put to pixmap/window */
      /* Create temp XImage for data */
#ifdef XFree86Server
      ximage = XMesaCreateImage(GET_VISUAL_DEPTH(v), w, h, p);
#else
      XVisualInfo *visinfo = xms->xrb->Parent->xm_visual->visinfo;
      ximage = XCreateImage(xms->display,
                            visinfo->visual,
                            visinfo->depth,
                            ZPixmap, 0,   /* format, offset */
                            (char *) p,   /* data */
                            w, h,         /* width, height */
                            32,           /* bitmap_pad */
                            0);           /* bytes_per_line */
#endif

      /* send XImage data to pixmap */
      XPutImage(xms->display, xms->drawable, xms->gc,
                ximage, 0, 0, x, y, w, h);
      /* clean-up */
      ximage->data = NULL; /* prevents freeing user data at 'p' */
      XMesaDestroyImage(ximage);
   }
}



/**
 * XXX rewrite to stop using renderbuffer->GetRow()
 */
void
xmesa_get_tile_rgba(struct pipe_context *pipe, struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, float *p)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   struct xmesa_renderbuffer *xrb = xms->xrb;

   if (xrb) {
      /* this is a front/back color buffer */
      GLubyte tmp[MAX_WIDTH * 4];
      GLuint i, j;
      uint w0 = w;
      GET_CURRENT_CONTEXT(ctx);

      CLIP_TILE;

      FLIP(y);
      for (i = 0; i < h; i++) {
         xrb->St.Base.GetRow(ctx, &xrb->St.Base, w, x, y - i, tmp);
         for (j = 0; j < w * 4; j++) {
            p[j] = UBYTE_TO_FLOAT(tmp[j]);
         }
         p += w0 * 4;
      }
   }
   else {
      /* other softpipe surface */
      softpipe_get_tile_rgba(pipe, ps, x, y, w, h, p);
   }
}


/**
 * XXX rewrite to stop using renderbuffer->PutRow()
 */
void
xmesa_put_tile_rgba(struct pipe_context *pipe, struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, const float *p)
{
   const uint x0 = x, y0 = y;
   struct xmesa_surface *xms = xmesa_surface(ps);
   XMesaImage *ximage;
   uint i, j;

   CLIP_TILE;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_put_tile_rgba(pipe, ps, x, y, w, h, p);
      return;
   }

   if (!xms->ximage) {
      /* create temp XImage */
      char *data = (char *) malloc(w * h * 4);
#ifdef XFree86Server
      ximage = XMesaCreateImage(GET_VISUAL_DEPTH(v), w, h, data);
#else
      XVisualInfo *visinfo = xms->xrb->Parent->xm_visual->visinfo;
      ximage = XCreateImage(xms->display,
                            visinfo->visual,
                            visinfo->depth,
                            ZPixmap, 0,   /* format, offset */
                            data,         /* data */
                            w, h,         /* width, height */
                            32,           /* bitmap_pad */
                            0);           /* bytes_per_line */
#endif
      x = y = 0;
   }
   else {
      ximage = xms->ximage;
   }

   /* convert floats to ximage's format */
   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      {
         uint *dst
            = (uint *) (ximage->data + y * ximage->bytes_per_line + x * 4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               ubyte r, g, b, a;
               UNCLAMPED_FLOAT_TO_UBYTE(r, p[0]);
               UNCLAMPED_FLOAT_TO_UBYTE(g, p[1]);
               UNCLAMPED_FLOAT_TO_UBYTE(b, p[2]);
               UNCLAMPED_FLOAT_TO_UBYTE(a, p[3]);
               dst[j] = PACK_8A8R8G8B(r, g, b, a);
               p += 4;
            }
            dst += ximage->width;
         }
      }
      break;
   case PIPE_FORMAT_U_R5_G6_B5:
      {
         ushort *dst =
            (ushort *) (ximage->data + y * ximage->bytes_per_line + x * 2);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               ubyte r, g, b;
               UNCLAMPED_FLOAT_TO_UBYTE(r, p[0]);
               UNCLAMPED_FLOAT_TO_UBYTE(g, p[1]);
               UNCLAMPED_FLOAT_TO_UBYTE(b, p[2]);
               dst[j] = PACK_5R6G5B(r, g, b);
               p += 4;
            }
            dst += ximage->width;
         }
      }
      break;
      
   default:
      assert(0);
   }

   if (!xms->ximage) {
      /* send XImage data to pixmap */
      XPutImage(xms->display, xms->drawable, xms->gc,
                ximage, 0, 0, x0, y0, w, h);
      /* clean-up */
      free(ximage->data);
      ximage->data = NULL;
      XMesaDestroyImage(ximage);
   }
}


static void
clear_pixmap_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                     uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   assert(xms);
   assert(xms->display);
   assert(xms->drawable);
   assert(xms->gc);
   XMesaSetForeground( xms->display, xms->gc, value );
   XMesaFillRectangle( xms->display, xms->drawable, xms->gc,
                       0, 0, ps->width, ps->height);
}

static void
clear_nbit_ximage_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                          uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   int width = xms->surface.width;
   int height = xms->surface.height;
   int i, j;
   for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
         XMesaPutPixel(xms->ximage, i, j, value);
      }
   }
}

static void
clear_8bit_ximage_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                          uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   memset(xms->ximage->data,
          value,
          xms->ximage->bytes_per_line * xms->ximage->height);
}

static void
clear_16bit_ximage_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                           uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   const int n = xms->ximage->width * xms->ximage->height;
   ushort *dst = (ushort *) xms->ximage->data;
   int i;
   for (i = 0; i < n; i++) {
      dst[i] = value;
   }
}


/* Optimized code provided by Nozomi Ytow <noz@xfree86.org> */
static void
clear_24bit_ximage_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                           uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   const ubyte r = (value      ) & 0xff;
   const ubyte g = (value >>  8) & 0xff;
   const ubyte b = (value >> 16) & 0xff;

   if (r == g && g == b) {
      /* same value for all three components (gray) */
      memset(xms->ximage->data, r,
             xms->ximage->bytes_per_line * xms->ximage->height);
   }
   else {
      /* non-gray clear color */
      const int n = xms->ximage->width * xms->ximage->height;
      int i;
      bgr_t *ptr3 = (bgr_t *) xms->ximage->data;
      for (i = 0; i < n; i++) {
         ptr3->r = r;
         ptr3->g = g;
         ptr3->b = b;
         ptr3++;
      }
   }
}

static void
clear_32bit_ximage_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                           uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);

   if (value == 0) {
      /* common case */
      memset(xms->ximage->data, value,
             xms->ximage->bytes_per_line * xms->ximage->height);
   }
   else {
      const int n = xms->ximage->width * xms->ximage->height;
      uint *dst = (uint *) xms->ximage->data;
      int i;
      for (i = 0; i < n; i++)
         dst[i] = value;
   }
}




/**
 * Called to create a pipe_surface for each X renderbuffer.
 * Note: this is being used instead of pipe->surface_alloc() since we
 * have special/unique quad read/write functions for X.
 */
struct pipe_surface *
xmesa_new_color_surface(struct pipe_context *pipe, GLuint pipeFormat)
{
   struct xmesa_surface *xms = CALLOC_STRUCT(xmesa_surface);

   assert(pipeFormat);

   xms->surface.format = pipeFormat;
   xms->surface.refcount = 1;

   /* Note, the region we allocate doesn't actually have any storage
    * since we're drawing into an XImage or Pixmap.
    * The region's size will get set in the xmesa_alloc_front/back_storage()
    * functions.
    */
   if (pipe)
      xms->surface.region = pipe->winsys->region_alloc(pipe->winsys,
                                                       1, 0, 0, 0x0);

   return &xms->surface;
}


/**
 * Called via pipe->surface_alloc() to create new surfaces (textures,
 * renderbuffers, etc.
 */
struct pipe_surface *
xmesa_surface_alloc(struct pipe_context *pipe, GLuint pipeFormat)
{
   struct xmesa_surface *xms = CALLOC_STRUCT(xmesa_surface);

   assert(pipe);
   assert(pipeFormat);

   xms->surface.format = pipeFormat;
   xms->surface.refcount = 1;

   return &xms->surface;
}


/**
 * Called via pipe->clear() to clear entire surface to a certain value.
 * If the surface is not an X pixmap or XImage, pass the call to
 * softpipe_clear().
 */
void
xmesa_clear(struct pipe_context *pipe, struct pipe_surface *ps, uint value)
{
   struct xmesa_surface *xms = xmesa_surface(ps);

   /* XXX actually, we should just discard any cached tiles from this
    * surface since we don't want to accidentally re-use them after clearing.
    */
   pipe->flush(pipe, 0);

   {
      struct softpipe_context *sp = softpipe_context(pipe);
      if (ps == sp_tile_cache_get_surface(sp->cbuf_cache[0])) {
         float clear[4];
         clear[0] = 0.2; /* XXX hack */
         clear[1] = 0.2;
         clear[2] = 0.2;
         clear[3] = 0.2;
         sp_tile_cache_clear(sp->cbuf_cache[0], clear);
      }
   }

#if 1
   (void) clear_8bit_ximage_surface;
   (void) clear_24bit_ximage_surface;
#endif

   if (xms->ximage) {
      /* back color buffer */
      switch (xms->surface.format) {
      case PIPE_FORMAT_U_R5_G6_B5:
         clear_16bit_ximage_surface(pipe, ps, value);
         break;
      case PIPE_FORMAT_U_A8_R8_G8_B8:
         clear_32bit_ximage_surface(pipe, ps, value);
         break;
      default:
         clear_nbit_ximage_surface(pipe, ps, value);
         break;
      }
   }
   else if (xms->drawable) {
      /* front color buffer */
      clear_pixmap_surface(pipe, ps, value);
   }
   else {
      /* other kind of buffer */
      softpipe_clear(pipe, ps, value);
   }
}


/** XXX unfinished sketch... */
struct pipe_surface *
xmesa_create_front_surface(XMesaVisual vis, Window win)
{
   struct xmesa_surface *xms = CALLOC_STRUCT(xmesa_surface);
   if (!xms) {
      return NULL;
   }

   xms->display = vis->display;
   xms->drawable = win;

   xms->surface.format = PIPE_FORMAT_U_A8_R8_G8_B8;
   xms->surface.refcount = 1;
#if 0
   xms->surface.region = pipe->winsys->region_alloc(pipe->winsys,
                                                    1, 0, 0, 0x0);
#endif
   return &xms->surface;
}

