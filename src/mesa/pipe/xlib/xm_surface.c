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


/*
 * Dithering kernels and lookup tables.
 */

const int xmesa_kernel8[DITH_DY * DITH_DX] = {
    0 * MAXC,  8 * MAXC,  2 * MAXC, 10 * MAXC,
   12 * MAXC,  4 * MAXC, 14 * MAXC,  6 * MAXC,
    3 * MAXC, 11 * MAXC,  1 * MAXC,  9 * MAXC,
   15 * MAXC,  7 * MAXC, 13 * MAXC,  5 * MAXC,
};

const int xmesa_kernel1[16] = {
   0*47,  9*47,  4*47, 12*47,     /* 47 = (255*3)/16 */
   6*47,  2*47, 14*47,  8*47,
  10*47,  1*47,  5*47, 11*47,
   7*47, 13*47,  3*47, 15*47
};



#define CLIP_TILE \
   do { \
      if (x >= ps->width) \
         return; \
      if (y >= ps->height) \
         return; \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height -y; \
   } while(0)



/*
 * The following functions are used to trap XGetImage() calls which
 * generate BadMatch errors if the drawable isn't mapped.
 */

#ifndef XFree86Server
static int caught_xgetimage_error = 0;
static int (*old_xerror_handler)( XMesaDisplay *dpy, XErrorEvent *ev );
static unsigned long xgetimage_serial;

/*
 * This is the error handler which will be called if XGetImage fails.
 */
static int xgetimage_error_handler( XMesaDisplay *dpy, XErrorEvent *ev )
{
   if (ev->serial==xgetimage_serial && ev->error_code==BadMatch) {
      /* caught the expected error */
      caught_xgetimage_error = 0;
   }
   else {
      /* call the original X error handler, if any.  otherwise ignore */
      if (old_xerror_handler) {
         (*old_xerror_handler)( dpy, ev );
      }
   }
   return 0;
}


/*
 * Call this right before XGetImage to setup error trap.
 */
static void catch_xgetimage_errors( XMesaDisplay *dpy )
{
   xgetimage_serial = NextRequest( dpy );
   old_xerror_handler = XSetErrorHandler( xgetimage_error_handler );
   caught_xgetimage_error = 0;
}


/*
 * Call this right after XGetImage to check if an error occured.
 */
static int check_xgetimage_errors( void )
{
   /* restore old handler */
   (void) XSetErrorHandler( old_xerror_handler );
   /* return 0=no error, 1=error caught */
   return caught_xgetimage_error;
}
#endif


/**
 * Wrapper for XGetImage() that catches BadMatch errors that can occur
 * when the window is unmapped or the x/y/w/h extend beyond the window
 * bounds.
 * If build into xserver, wrap the internal GetImage method.
 */
static XMesaImage *
xget_image(XMesaDisplay *dpy, Drawable d, int x, int y, uint w, uint h)
{
#ifdef XFree86Server
   uint bpp = 4; /* XXX fix this */
   XMesaImage *ximage = (XMesaImage *) malloc(sizeof(XMesaImage));
   if (ximage) {
      ximage->data = malloc(width * height * bpp);
   }
   (*dpy->GetImage)(d, x, y, w, h, ZPixmap, ~0L, (pointer)ximage->data);
   ximage->width = w;
   ximage->height = h;
   ximage->bytes_per_row = w * bpp;
   return ximage;
#else
   int error;
   XMesaImage *ximage;
   catch_xgetimage_errors(dpy);
   ximage = XGetImage(dpy, d, x, y, w, h, AllPlanes, ZPixmap);
   error = check_xgetimage_errors();
   return ximage;
#endif
}



/**
 * Return raw pixels from pixmap or XImage.
 */
void
xmesa_get_tile(struct pipe_context *pipe, struct pipe_surface *ps,
               uint x, uint y, uint w, uint h, void *p, int dst_stride)
{
   const uint w0 = w;
   struct xmesa_surface *xms = xmesa_surface(ps);
   XMesaImage *ximage = NULL;
   ubyte *dst = (ubyte *) p;
   uint i;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_get_tile(pipe, ps, x, y, w, h, p, dst_stride);
      return;
   }

   CLIP_TILE;

   if (!xms->ximage) {
      /* XImage = pixmap data */
      assert(xms->drawable);
      ximage = xget_image(xms->display, xms->drawable, x, y, w, h);
      if (!ximage)
         return;
      x = y = 0;
   }
   else {
      ximage = xms->ximage;
   }

   /* this could be optimized/simplified */
   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      if (!dst_stride) {
         dst_stride = w0 * 4;
      }
      for (i = 0; i < h; i++) {
         memcpy(dst, ximage->data + y * ximage->bytes_per_line + x * 4, 4 * w);
         dst += dst_stride;
      }
      break;
   case PIPE_FORMAT_U_R5_G6_B5:
      if (!dst_stride) {
         dst_stride = w0 * 2;
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
   const uint w0 = w;
   struct xmesa_surface *xms = xmesa_surface(ps);
   const ubyte *src = (const ubyte *) p;
   XMesaImage *ximage;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_put_tile(pipe, ps, x, y, w, h, p, src_stride);
      return;
   }

   CLIP_TILE;

   if (xms->ximage) {
      /* put to ximage */
      ximage = xms->ximage;
      char *dst;
      uint i;

      /* this could be optimized/simplified */
      switch (ps->format) {
      case PIPE_FORMAT_U_A8_R8_G8_B8:
         if (!src_stride) {
            src_stride = w0 * 4;
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
            src_stride = w0 * 2;
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


void
xmesa_get_tile_rgba(struct pipe_context *pipe, struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, float *pixels)
{
   const uint w0 = w;
   struct xmesa_surface *xms = xmesa_surface(ps);
   XMesaImage *ximage = NULL;
   float *pRow = pixels;
   uint i, j;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_get_tile_rgba(pipe, ps, x, y, w, h, pixels);
      return;
   }

   CLIP_TILE;

   if (!xms->ximage) {
      /* XImage = pixmap data */
      assert(xms->drawable);
      ximage = xget_image(xms->display, xms->drawable, x, y, w, h);
      if (!ximage)
         return;
      x = y = 0;
   }
   else {
      ximage = xms->ximage;
   }
   
   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      {
         const uint *src
            = (uint *) (ximage->data + y * ximage->bytes_per_line + x * 4);
         for (i = 0; i < h; i++) {
            float *p = pRow;
            for (j = 0; j < w; j++) {
               uint pix = src[j];
               ubyte r = ((pix >> 16) & 0xff);
               ubyte g = ((pix >>  8) & 0xff);
               ubyte b = ( pix        & 0xff);
               ubyte a = ((pix >> 24) & 0xff);
               p[0] = UBYTE_TO_FLOAT(r);
               p[1] = UBYTE_TO_FLOAT(g);
               p[2] = UBYTE_TO_FLOAT(b);
               p[3] = UBYTE_TO_FLOAT(a);
               p += 4;
            }
            src += ximage->width;
            pRow += 4 * w0;
         }
      }
      break;
   case PIPE_FORMAT_U_B8_G8_R8_A8:
      {
         const uint *src
            = (uint *) (ximage->data + y * ximage->bytes_per_line + x * 4);
         for (i = 0; i < h; i++) {
            float *p = pRow;
            for (j = 0; j < w; j++) {
               uint pix = src[j];
               ubyte r = ((pix >>  8) & 0xff);
               ubyte g = ((pix >> 16) & 0xff);
               ubyte b = ((pix >> 24) & 0xff);
               ubyte a = ( pix        & 0xff);
               p[0] = UBYTE_TO_FLOAT(r);
               p[1] = UBYTE_TO_FLOAT(g);
               p[2] = UBYTE_TO_FLOAT(b);
               p[3] = UBYTE_TO_FLOAT(a);
               p += 4;
            }
            src += ximage->width;
            pRow += 4 * w0;
         }
      }
      break;
   case PIPE_FORMAT_U_R5_G6_B5:
      {
         ushort *src
            = (ushort *) (ximage->data + y * ximage->bytes_per_line + x * 2);
         for (i = 0; i < h; i++) {
            float *p = pRow;
            for (j = 0; j < w; j++) {
               uint pix = src[j];
               ubyte r = (pix >> 8) | ((pix >> 13) & 0x7);
               ubyte g = (pix >> 3) | ((pix >>  9) & 0x3);
               ubyte b = ((pix & 0x1f) << 3) | ((pix >> 2) & 0x3);
               p[0] = UBYTE_TO_FLOAT(r);
               p[1] = UBYTE_TO_FLOAT(g);
               p[2] = UBYTE_TO_FLOAT(b);
               p[3] = 1.0;
               p += 4;
            }
            src += ximage->width;
            pRow += 4 * w0;
         }
      }
      break;
   default:
      fprintf(stderr, "Bad format in xmesa_get_tile_rgba()\n");
      assert(0);
   }

   if (!xms->ximage) {
      XMesaDestroyImage(ximage);
   }
}


void
xmesa_put_tile_rgba(struct pipe_context *pipe, struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, const float *pixels)
{
   const uint x0 = x, y0 = y, w0 = w;
   struct xmesa_surface *xms = xmesa_surface(ps);
   XMesaImage *ximage;
   uint i, j;

   if (!xms->drawable && !xms->ximage) {
      /* not an X surface */
      softpipe_put_tile_rgba(pipe, ps, x, y, w, h, pixels);
      return;
   }

   CLIP_TILE;

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
         const float *pRow = pixels;
         for (i = 0; i < h; i++) {
            const float *p = pRow;
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
            pRow += 4 * w0;
         }
      }
      break;
   case PIPE_FORMAT_U_B8_G8_R8_A8:
      {
         uint *dst
            = (uint *) (ximage->data + y * ximage->bytes_per_line + x * 4);
         const float *pRow = pixels;
         for (i = 0; i < h; i++) {
            const float *p = pRow;
            for (j = 0; j < w; j++) {
               ubyte r, g, b, a;
               UNCLAMPED_FLOAT_TO_UBYTE(r, p[0]);
               UNCLAMPED_FLOAT_TO_UBYTE(g, p[1]);
               UNCLAMPED_FLOAT_TO_UBYTE(b, p[2]);
               UNCLAMPED_FLOAT_TO_UBYTE(a, p[3]);
               dst[j] = PACK_8B8G8R8A(r, g, b, a);
               p += 4;
            }
            dst += ximage->width;
            pRow += 4 * w0;
         }
      }
      break;
   case PIPE_FORMAT_U_R5_G6_B5:
      {
         ushort *dst =
            (ushort *) (ximage->data + y * ximage->bytes_per_line + x * 2);
         const float *pRow = pixels;
         for (i = 0; i < h; i++) {
            const float *p = pRow;
            for (j = 0; j < w; j++) {
               ubyte r, g, b;
               UNCLAMPED_FLOAT_TO_UBYTE(r, p[0]);
               UNCLAMPED_FLOAT_TO_UBYTE(g, p[1]);
               UNCLAMPED_FLOAT_TO_UBYTE(b, p[2]);
               dst[j] = PACK_5R6G5B(r, g, b);
               p += 4;
            }
            dst += ximage->width;
            pRow += 4 * w0;
         }
      }
      break;
      
   default:
      fprintf(stderr, "Bad format in xmesa_put_tile_rgba()\n");
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
xmesa_new_color_surface(struct pipe_winsys *winsys, GLuint pipeFormat)
{
   struct xmesa_surface *xms = CALLOC_STRUCT(xmesa_surface);

   assert(pipeFormat);

   xms->surface.format = pipeFormat;
   xms->surface.refcount = 1;
   xms->surface.winsys = winsys;

   /* Note, the region we allocate doesn't actually have any storage
    * since we're drawing into an XImage or Pixmap.
    * The region's size will get set in the xmesa_alloc_front/back_storage()
    * functions.
    */
   xms->surface.region = winsys->region_alloc(winsys, 1, 0x0);

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
   xms->surface.winsys = pipe->winsys;

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
      case PIPE_FORMAT_U_B8_G8_R8_A8:
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

