/* $Id: xm_dd.c,v 1.31 2002/03/19 16:48:06 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0.2
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
/* $XFree86: xc/extras/Mesa/src/X/xm_dd.c,v 1.2 2002/02/26 23:37:31 tsi Exp $ */

#include "glxheader.h"
#include "context.h"
#include "depth.h"
#include "drawpix.h"
#include "extensions.h"
#include "macros.h"
#include "mem.h"
#include "mtypes.h"
#include "state.h"
#include "texstore.h"
#include "texformat.h"
#include "xmesaP.h"
#include "array_cache/acache.h"
#include "swrast/s_context.h"
#include "swrast/swrast.h"
#include "swrast/s_alphabuf.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"


/*
 * Return the size (width, height) of the X window for the given GLframebuffer.
 * Output:  width - width of buffer in pixels.
 *          height - height of buffer in pixels.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   /* We can do this cast because the first field in the XMesaBuffer
    * struct is a GLframebuffer struct.  If this weren't true, we'd
    * need a pointer from the GLframebuffer to the XMesaBuffer.
    */
   const XMesaBuffer xmBuffer = (XMesaBuffer) buffer;
   unsigned int winwidth, winheight;
#ifndef XFree86Server
   Window root;
   int winx, winy;
   unsigned int bw, d;

   _glthread_LOCK_MUTEX(_xmesa_lock);
   XGetGeometry( xmBuffer->xm_visual->display, xmBuffer->frontbuffer, &root,
		 &winx, &winy, &winwidth, &winheight, &bw, &d );
   _glthread_UNLOCK_MUTEX(_xmesa_lock);
#else
   /* XFree86 GLX renderer */
   winwidth = xmBuffer->frontbuffer->width;
   winheight = xmBuffer->frontbuffer->height;
#endif

   (void)kernel8;		/* Muffle compiler */

   *width = winwidth;
   *height = winheight;
}


static void
finish( GLcontext *ctx )
{
#ifdef XFree86Server
      /* NOT_NEEDED */
#else
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (xmesa) {
      _glthread_LOCK_MUTEX(_xmesa_lock);
      XSync( xmesa->display, False );
      _glthread_UNLOCK_MUTEX(_xmesa_lock);
   }
#endif
}


static void
flush( GLcontext *ctx )
{
#ifdef XFree86Server
      /* NOT_NEEDED */
#else
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (xmesa) {
      _glthread_LOCK_MUTEX(_xmesa_lock);
      XFlush( xmesa->display );
      _glthread_UNLOCK_MUTEX(_xmesa_lock);
   }
#endif
}




static GLboolean
set_draw_buffer( GLcontext *ctx, GLenum mode )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (mode == GL_FRONT_LEFT) {
      /* write to front buffer */
      xmesa->xm_buffer->buffer = xmesa->xm_buffer->frontbuffer;
      xmesa_update_span_funcs(ctx);
      return GL_TRUE;
   }
   else if (mode==GL_BACK_LEFT && xmesa->xm_buffer->db_state) {
      /* write to back buffer */
      if (xmesa->xm_buffer->backpixmap) {
         xmesa->xm_buffer->buffer =
	     (XMesaDrawable)xmesa->xm_buffer->backpixmap;
      }
      else if (xmesa->xm_buffer->backimage) {
         xmesa->xm_buffer->buffer = None;
      }
      else {
         /* just in case there wasn't enough memory for back buffer */
         xmesa->xm_buffer->buffer = xmesa->xm_buffer->frontbuffer;
      }
      xmesa_update_span_funcs(ctx);
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


void
xmesa_set_read_buffer( GLcontext *ctx, GLframebuffer *buffer, GLenum mode )
{
   XMesaBuffer target;
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;

   if (buffer == ctx->DrawBuffer) {
      target = xmesa->xm_buffer;
      xmesa->use_read_buffer = GL_FALSE;
   }
   else {
      ASSERT(buffer == ctx->ReadBuffer);
      target = xmesa->xm_read_buffer;
      xmesa->use_read_buffer = GL_TRUE;
   }

   if (mode == GL_FRONT_LEFT) {
      target->buffer = target->frontbuffer;
      xmesa_update_span_funcs(ctx);
   }
   else if (mode==GL_BACK_LEFT && xmesa->xm_read_buffer->db_state) {
      if (target->backpixmap) {
         target->buffer = (XMesaDrawable)xmesa->xm_buffer->backpixmap;
      }
      else if (target->backimage) {
         target->buffer = None;
      }
      else {
         /* just in case there wasn't enough memory for back buffer */
         target->buffer = target->frontbuffer;
      }
      xmesa_update_span_funcs(ctx);
   }
   else {
      _mesa_problem(ctx, "invalid buffer in set_read_buffer() in xmesa2.c");
   }
}



static void
clear_index( GLcontext *ctx, GLuint index )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   xmesa->clearpixel = (unsigned long) index;
   XMesaSetForeground( xmesa->display, xmesa->xm_buffer->cleargc,
                       (unsigned long) index );
}


static void
clear_color( GLcontext *ctx, const GLchan color[4] )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   xmesa->clearcolor[0] = color[0];
   xmesa->clearcolor[1] = color[1];
   xmesa->clearcolor[2] = color[2];
   xmesa->clearcolor[3] = color[3];
   xmesa->clearpixel = xmesa_color_to_pixel( xmesa, color[0], color[1],
                                             color[2], color[3],
                                             xmesa->xm_visual->undithered_pf );
   _glthread_LOCK_MUTEX(_xmesa_lock);
   XMesaSetForeground( xmesa->display, xmesa->xm_buffer->cleargc,
                       xmesa->clearpixel );
   _glthread_UNLOCK_MUTEX(_xmesa_lock);
}



/* Set index mask ala glIndexMask */
static void
index_mask( GLcontext *ctx, GLuint mask )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (xmesa->xm_buffer->buffer != XIMAGE) {
      unsigned long m;
      if (mask==0xffffffff) {
	 m = ((unsigned long)~0L);
      }
      else {
         m = (unsigned long) mask;
      }
      XMesaSetPlaneMask( xmesa->display, xmesa->xm_buffer->cleargc, m );
   }
}


/* Implements glColorMask() */
static void
color_mask(GLcontext *ctx,
           GLboolean rmask, GLboolean gmask, GLboolean bmask, GLboolean amask)
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   int xclass = GET_VISUAL_CLASS(xmesa->xm_visual);
   (void) amask;

   if (xclass == TrueColor || xclass == DirectColor) {
      unsigned long m;
      if (rmask && gmask && bmask) {
         m = ((unsigned long)~0L);
      }
      else {
         m = 0;
         if (rmask)   m |= GET_REDMASK(xmesa->xm_visual);
         if (gmask)   m |= GET_GREENMASK(xmesa->xm_visual);
         if (bmask)   m |= GET_BLUEMASK(xmesa->xm_visual);
      }
      XMesaSetPlaneMask( xmesa->display, xmesa->xm_buffer->cleargc, m );
   }
}



/**********************************************************************/
/*** glClear implementations                                        ***/
/**********************************************************************/


static void
clear_front_pixmap( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (all) {
      XMesaFillRectangle( xmesa->display, xmesa->xm_buffer->frontbuffer,
                          xmesa->xm_buffer->cleargc,
                          0, 0,
                          xmesa->xm_buffer->width+1,
                          xmesa->xm_buffer->height+1 );
   }
   else {
      XMesaFillRectangle( xmesa->display, xmesa->xm_buffer->frontbuffer,
                          xmesa->xm_buffer->cleargc,
                          x, xmesa->xm_buffer->height - y - height,
                          width, height );
   }
}


static void
clear_back_pixmap( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (all) {
      XMesaFillRectangle( xmesa->display, xmesa->xm_buffer->backpixmap,
                          xmesa->xm_buffer->cleargc,
                          0, 0,
                          xmesa->xm_buffer->width+1,
                          xmesa->xm_buffer->height+1 );
   }
   else {
      XMesaFillRectangle( xmesa->display, xmesa->xm_buffer->backpixmap,
                          xmesa->xm_buffer->cleargc,
                          x, xmesa->xm_buffer->height - y - height,
                          width, height );
   }
}


static void
clear_8bit_ximage( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (all) {
      size_t n = xmesa->xm_buffer->backimage->bytes_per_line
         * xmesa->xm_buffer->backimage->height;
      MEMSET( xmesa->xm_buffer->backimage->data, xmesa->clearpixel, n );
   }
   else {
      GLint i;
      for (i=0;i<height;i++) {
         GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y+i );
         MEMSET( ptr, xmesa->clearpixel, width );
      }
   }
}


static void
clear_HPCR_ximage( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   if (all) {
      GLint i, c16 = (xmesa->xm_buffer->backimage->bytes_per_line>>4)<<4;
      GLubyte *ptr  = (GLubyte *)xmesa->xm_buffer->backimage->data;
      for (i=0; i<xmesa->xm_buffer->backimage->height; i++) {
         GLint j;
         GLubyte *sptr = xmesa->xm_visual->hpcr_clear_ximage_pattern[0];
         if (i&1) {
            sptr += 16;
         }
         for (j=0; j<c16; j+=16) {
            ptr[0] = sptr[0];
            ptr[1] = sptr[1];
            ptr[2] = sptr[2];
            ptr[3] = sptr[3];
            ptr[4] = sptr[4];
            ptr[5] = sptr[5];
            ptr[6] = sptr[6];
            ptr[7] = sptr[7];
            ptr[8] = sptr[8];
            ptr[9] = sptr[9];
            ptr[10] = sptr[10];
            ptr[11] = sptr[11];
            ptr[12] = sptr[12];
            ptr[13] = sptr[13];
            ptr[14] = sptr[14];
            ptr[15] = sptr[15];
            ptr += 16;
         }
         for (; j<xmesa->xm_buffer->backimage->bytes_per_line; j++) {
            *ptr = sptr[j&15];
            ptr++;
         }
      }
   }
   else {
      GLint i;
      for (i=y; i<y+height; i++) {
         GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, i );
         int j;
         GLubyte *sptr = xmesa->xm_visual->hpcr_clear_ximage_pattern[0];
         if (i&1) {
            sptr += 16;
         }
         for (j=x; j<x+width; j++) {
            *ptr = sptr[j&15];
            ptr++;
         }
      }
   }
}


static void
clear_16bit_ximage( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   register GLuint pixel = (GLuint) xmesa->clearpixel;
   if (xmesa->swapbytes) {
      pixel = ((pixel >> 8) & 0x00ff) | ((pixel << 8) & 0xff00);
   }
   if (all) {
      register GLuint n;
      register GLuint *ptr4 = (GLuint *) xmesa->xm_buffer->backimage->data;
      if ((pixel & 0xff) == ((pixel >> 8) & 0xff)) {
         /* low and high bytes are equal so use memset() */
         n = xmesa->xm_buffer->backimage->bytes_per_line
            * xmesa->xm_buffer->height;
         MEMSET( ptr4, pixel & 0xff, n );
      }
      else {
         pixel = pixel | (pixel<<16);
         n = xmesa->xm_buffer->backimage->bytes_per_line
            * xmesa->xm_buffer->height / 4;
         do {
            *ptr4++ = pixel;
               n--;
         } while (n!=0);

         if ((xmesa->xm_buffer->backimage->bytes_per_line *
              xmesa->xm_buffer->height) & 0x2)
            *(GLushort *)ptr4 = pixel & 0xffff;
      }
   }
   else {
      register int i, j;
      for (j=0;j<height;j++) {
         register GLushort *ptr2 = PIXELADDR2( xmesa->xm_buffer, x, y+j );
         for (i=0;i<width;i++) {
            *ptr2++ = pixel;
         }
      }
   }
}


/* Optimized code provided by Nozomi Ytow <noz@xfree86.org> */
static void
clear_24bit_ximage( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLubyte r = xmesa->clearcolor[0];
   const GLubyte g = xmesa->clearcolor[1];
   const GLubyte b = xmesa->clearcolor[2];
   register GLuint clearPixel;
   if (xmesa->swapbytes) {
      clearPixel = (b << 16) | (g << 8) | r;
   }
   else {
      clearPixel = (r << 16) | (g << 8) | b;
   }

   if (all) {
      if (r==g && g==b) {
         /* same value for all three components (gray) */
         const GLint w3 = xmesa->xm_buffer->width * 3;
         const GLint h = xmesa->xm_buffer->height;
         GLint i;
         for (i = 0; i < h; i++) {
            bgr_t *ptr3 = PIXELADDR3(xmesa->xm_buffer, 0, i);
            MEMSET(ptr3, r, w3);
         }
      }
      else {
         /* the usual case */
         const GLint w = xmesa->xm_buffer->width;
         const GLint h = xmesa->xm_buffer->height;
         GLint i, j;
         for (i = 0; i < h; i++) {
            bgr_t *ptr3 = PIXELADDR3(xmesa->xm_buffer, 0, i);
            for (j = 0; j < w; j++) {
               ptr3->r = r;
               ptr3->g = g;
               ptr3->b = b;
               ptr3++;
            }
         }
#if 0 /* this code doesn't work for all window widths */
         register GLuint *ptr4 = (GLuint *) ptr3;
         register GLuint px;
         GLuint pixel4[3];
         register GLuint *p = pixel4;
         pixel4[0] = clearPixel | (clearPixel << 24);
         pixel4[1] = (clearPixel << 16) | (clearPixel >> 8);
         pixel4[2] = (clearPixel << 8) | (clearPixel >>  16);
         switch (3 & (int)(ptr3 - (bgr_t*) ptr4)){
            case 0:
               break;
            case 1:
               px = *ptr4 & 0x00ffffff;
               px |= pixel4[0] & 0xff000000;
               *ptr4++ = px;
               px = *ptr4 & 0xffff0000;
               px |= pixel4[2] & 0x0000ffff;
               *ptr4 = px;
               if (0 == --n)
                  break;
            case 2:
               px = *ptr4 & 0x0000fffff;
               px |= pixel4[1] & 0xffff0000;
               *ptr4++ = px;
               px = *ptr4 & 0xffffff00;
               px |= pixel4[2] & 0x000000ff;
               *ptr4 = px;
               if (0 == --n)
                  break;
            case 3:
               px = *ptr4 & 0x000000ff;
               px |= pixel4[2] & 0xffffff00;
               *ptr4++ = px;
               --n;
               break;
         }
         while (n > 3) {
            p = pixel4;
            *ptr4++ = *p++;
            *ptr4++ = *p++;
            *ptr4++ = *p++;
            n -= 4;
         }
         switch (n) {
            case 3:
               p = pixel4;
               *ptr4++ = *p++;
               *ptr4++ = *p++;
               px = *ptr4 & 0xffffff00;
               px |= clearPixel & 0xff;
               *ptr4 = px;
               break;
            case 2:
               p = pixel4;
               *ptr4++ = *p++;
               px = *ptr4 & 0xffff0000;
               px |= *p & 0xffff;
               *ptr4 = px;
               break;
            case 1:
               px = *ptr4 & 0xff000000;
               px |= *p & 0xffffff;
               *ptr4 = px;
               break;
            case 0:
               break;
         }
#endif
      }
   }
   else {
      /* only clear subrect of color buffer */
      if (r==g && g==b) {
         /* same value for all three components (gray) */
         GLint j;
         for (j=0;j<height;j++) {
            bgr_t *ptr3 = PIXELADDR3( xmesa->xm_buffer, x, y+j );
            MEMSET(ptr3, r, 3 * width);
         }
      }
      else {
         /* non-gray clear color */
         GLint i, j;
         for (j = 0; j < height; j++) {
            bgr_t *ptr3 = PIXELADDR3( xmesa->xm_buffer, x, y+j );
            for (i = 0; i < width; i++) {
               ptr3->r = r;
               ptr3->g = g;
               ptr3->b = b;
               ptr3++;
            }
         }
#if 0 /* this code might not always (seems ptr3 always == ptr4) */
         GLint j;
         GLuint pixel4[3];
         pixel4[0] = clearPixel | (clearPixel << 24);
         pixel4[1] = (clearPixel << 16) | (clearPixel >> 8);
         pixel4[2] = (clearPixel << 8) | (clearPixel >>  16);
         for (j=0;j<height;j++) {
            bgr_t *ptr3 = PIXELADDR3( xmesa->xm_buffer, x, y+j );
            register GLuint *ptr4 = (GLuint *)ptr3;
            register GLuint *p, px;
            GLuint w = width;
            switch (3 & (int)(ptr3 - (bgr_t*) ptr4)){
               case 0:
                  break;
               case 1:
                  px = *ptr4 & 0x00ffffff;
                  px |= pixel4[0] & 0xff000000;
                  *ptr4++ = px;
                  px = *ptr4 & 0xffff0000;
                  px |= pixel4[2] & 0x0000ffff;
                  *ptr4 = px;
                  if (0 == --w)
                     break;
               case 2:
                  px = *ptr4 & 0x0000fffff;
                  px |= pixel4[1] & 0xffff0000;
                  *ptr4++ = px;
                  px = *ptr4 & 0xffffff00;
                  px |= pixel4[2] & 0x000000ff;
                  *ptr4 = px;
                  if (0 == --w)
                     break;
               case 3:
                  px = *ptr4 & 0x000000ff;
                  px |= pixel4[2] & 0xffffff00;
                  *ptr4++ = px;
                  --w;
                  break;
            }
            while (w > 3){
               p = pixel4;
               *ptr4++ = *p++;
               *ptr4++ = *p++;
               *ptr4++ = *p++;
               w -= 4;
            }
            switch (w) {
               case 3:
                  p = pixel4;
                  *ptr4++ = *p++;
                  *ptr4++ = *p++;
                  px = *ptr4 & 0xffffff00;
                  px |= *p & 0xff;
                  *ptr4 = px;
                  break;
               case 2:
                  p = pixel4;
                  *ptr4++ = *p++;
                  px = *ptr4 & 0xffff0000;
                  px |= *p & 0xffff;
                  *ptr4 = px;
                  break;
               case 1:
                  px = *ptr4 & 0xff000000;
                  px |= pixel4[0] & 0xffffff;
                  *ptr4 = px;
                  break;
               case 0:
                  break;
            }
         }
#endif
      }
   }
}


static void
clear_32bit_ximage( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   register GLuint pixel = (GLuint) xmesa->clearpixel;
   if (xmesa->swapbytes) {
      pixel = ((pixel >> 24) & 0x000000ff)
            | ((pixel >> 8)  & 0x0000ff00)
            | ((pixel << 8)  & 0x00ff0000)
            | ((pixel << 24) & 0xff000000);
   }
   if (all) {
      register GLint n = xmesa->xm_buffer->width * xmesa->xm_buffer->height;
      register GLuint *ptr4 = (GLuint *) xmesa->xm_buffer->backimage->data;
      if (pixel==0) {
         MEMSET( ptr4, pixel, 4*n );
      }
      else {
         do {
            *ptr4++ = pixel;
            n--;
         } while (n!=0);
      }
   }
   else {
      register int i, j;
      for (j=0;j<height;j++) {
         register GLuint *ptr4 = PIXELADDR4( xmesa->xm_buffer, x, y+j );
         for (i=0;i<width;i++) {
            *ptr4++ = pixel;
         }
      }
   }
}


static void
clear_nbit_ximage( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaImage *img = xmesa->xm_buffer->backimage;
   if (all) {
      register int i, j;
      width = xmesa->xm_buffer->width;
      height = xmesa->xm_buffer->height;
      for (j=0;j<height;j++) {
         for (i=0;i<width;i++) {
            XMesaPutPixel( img, i, j, xmesa->clearpixel );
         }
      }
   }
   else {
      /* TODO: optimize this */
      register int i, j;
      y = FLIP(xmesa->xm_buffer, y);
      for (j=0;j<height;j++) {
         for (i=0;i<width;i++) {
            XMesaPutPixel( img, x+i, y-j, xmesa->clearpixel );
         }
      }
   }
}



static void
clear_buffers( GLcontext *ctx, GLbitfield mask,
               GLboolean all, GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;

   if ((mask & (DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT)) &&
       xmesa->xm_buffer->mesa_buffer.UseSoftwareAlphaBuffers &&
       ctx->Color.ColorMask[ACOMP]) {
      _mesa_clear_alpha_buffers(ctx);
   }

   /* we can't handle color or index masking */
   if (*colorMask == 0xffffffff && ctx->Color.IndexMask == 0xffffffff) {
      if (mask & DD_FRONT_LEFT_BIT) {
	 ASSERT(xmesa->xm_buffer->front_clear_func);
	 (*xmesa->xm_buffer->front_clear_func)( ctx, all, x, y, width, height );
	 mask &= ~DD_FRONT_LEFT_BIT;
      }
      if (mask & DD_BACK_LEFT_BIT) {
	 ASSERT(xmesa->xm_buffer->back_clear_func);
	 (*xmesa->xm_buffer->back_clear_func)( ctx, all, x, y, width, height );
	 mask &= ~DD_BACK_LEFT_BIT;
      }
   }

   if (mask)
      _swrast_Clear( ctx, mask, all, x, y, width, height );
}


/*
 * When we detect that the user has resized the window this function will
 * get called.  Here we'll reallocate the back buffer, depth buffer,
 * stencil buffer etc. to match the new window size.
 */
void
xmesa_resize_buffers( GLframebuffer *buffer )
{
   int height = (int) buffer->Height;
   /* We can do this cast because the first field in the XMesaBuffer
    * struct is a GLframebuffer struct.  If this weren't true, we'd
    * need a pointer from the GLframebuffer to the XMesaBuffer.
    */
   XMesaBuffer xmBuffer = (XMesaBuffer) buffer;

   xmBuffer->width = buffer->Width;
   xmBuffer->height = buffer->Height;
   xmesa_alloc_back_buffer( xmBuffer );

   /* Needed by FLIP macro */
   xmBuffer->bottom = height - 1;

   if (xmBuffer->backimage) {
      /* Needed by PIXELADDR1 macro */
      xmBuffer->ximage_width1 = xmBuffer->backimage->bytes_per_line;
      xmBuffer->ximage_origin1 = (GLubyte *) xmBuffer->backimage->data
         + xmBuffer->ximage_width1 * (height-1);

      /* Needed by PIXELADDR2 macro */
      xmBuffer->ximage_width2 = xmBuffer->backimage->bytes_per_line / 2;
      xmBuffer->ximage_origin2 = (GLushort *) xmBuffer->backimage->data
         + xmBuffer->ximage_width2 * (height-1);

      /* Needed by PIXELADDR3 macro */
      xmBuffer->ximage_width3 = xmBuffer->backimage->bytes_per_line;
      xmBuffer->ximage_origin3 = (GLubyte *) xmBuffer->backimage->data
         + xmBuffer->ximage_width3 * (height-1);

      /* Needed by PIXELADDR4 macro */
      xmBuffer->ximage_width4 = xmBuffer->backimage->width;
      xmBuffer->ximage_origin4 = (GLuint *) xmBuffer->backimage->data
         + xmBuffer->ximage_width4 * (height-1);
   }

   _swrast_alloc_buffers( buffer );
}

#if 0
/*
 * This function implements glDrawPixels() with an XPutImage call when
 * drawing to the front buffer (X Window drawable).
 * The image format must be GL_BGRA to match the PF_8R8G8B pixel format.
 * XXX top/bottom edge clipping is broken!
 */
static GLboolean
drawpixels_8R8G8B( GLcontext *ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid *pixels )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   assert(dpy);
   assert(buffer);
   assert(gc);

   /* XXX also check for pixel scale/bias/lookup/zooming! */
   if (format == GL_BGRA && type == GL_UNSIGNED_BYTE) {
      int dstX = x;
      int dstY = y;
      int w = width;
      int h = height;
      int srcX = unpack->SkipPixels;
      int srcY = unpack->SkipRows;
      if (_mesa_clip_pixelrect(ctx, &dstX, &dstY, &w, &h, &srcX, &srcY)) {
         XMesaImage ximage;
         MEMSET(&ximage, 0, sizeof(XMesaImage));
         ximage.width = width;
         ximage.height = height;
         ximage.format = ZPixmap;
         ximage.data = (char *) pixels + (height - 1) * width * 4;
         ximage.byte_order = LSBFirst;
         ximage.bitmap_unit = 32;
         ximage.bitmap_bit_order = LSBFirst;
         ximage.bitmap_pad = 32;
         ximage.depth = 24;
         ximage.bytes_per_line = -width * 4;
         ximage.bits_per_pixel = 32;
         ximage.red_mask   = 0xff0000;
         ximage.green_mask = 0x00ff00;
         ximage.blue_mask  = 0x0000ff;
         dstY = FLIP(xmesa->xm_buffer,dstY) - height + 1;
         XPutImage(dpy, buffer, gc, &ximage, srcX, srcY, dstX, dstY, w, h);
         return GL_TRUE;
      }
   }
   return GL_FALSE;
}
#endif



static const GLubyte *
get_string( GLcontext *ctx, GLenum name )
{
   (void) ctx;
   switch (name) {
      case GL_RENDERER:
#ifdef XFree86Server
         return (const GLubyte *) "Mesa GLX Indirect";
#else
         return (const GLubyte *) "Mesa X11";
#endif
      case GL_VENDOR:
#ifdef XFree86Server
         return (const GLubyte *) "Mesa project: www.mesa3d.org";
#else
         return NULL;
#endif
      default:
         return NULL;
   }
}


static void
enable( GLcontext *ctx, GLenum pname, GLboolean state )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;

   switch (pname) {
      case GL_DITHER:
         if (state)
            xmesa->pixelformat = xmesa->xm_visual->dithered_pf;
         else
            xmesa->pixelformat = xmesa->xm_visual->undithered_pf;
         break;
      default:
         ;  /* silence compiler warning */
   }
}


void xmesa_update_state( GLcontext *ctx, GLuint new_state )
{
   const XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;

   /* Propogate statechange information to swrast and swrast_setup
    * modules.  The X11 driver has no internal GL-dependent state.
    */
   _swrast_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );


   /* setup pointers to front and back buffer clear functions */
   xmesa->xm_buffer->front_clear_func = clear_front_pixmap;
   if (xmesa->xm_buffer->backpixmap != XIMAGE) {
      xmesa->xm_buffer->back_clear_func = clear_back_pixmap;
   }
   else if (sizeof(GLushort)!=2 || sizeof(GLuint)!=4) {
      xmesa->xm_buffer->back_clear_func = clear_nbit_ximage;
   }
   else switch (xmesa->xm_visual->BitsPerPixel) {
   case 8:
      if (xmesa->xm_visual->hpcr_clear_flag) {
	 xmesa->xm_buffer->back_clear_func = clear_HPCR_ximage;
      }
      else {
	 xmesa->xm_buffer->back_clear_func = clear_8bit_ximage;
      }
      break;
   case 16:
      xmesa->xm_buffer->back_clear_func = clear_16bit_ximage;
      break;
   case 24:
      xmesa->xm_buffer->back_clear_func = clear_24bit_ximage;
      break;
   case 32:
      xmesa->xm_buffer->back_clear_func = clear_32bit_ximage;
      break;
   default:
      xmesa->xm_buffer->back_clear_func = clear_nbit_ximage;
      break;
   }

   xmesa_update_span_funcs(ctx);
}



/* Setup pointers and other driver state that is constant for the life
 * of a context.
 */
void xmesa_init_pointers( GLcontext *ctx )
{
   TNLcontext *tnl;

   ctx->Driver.GetString = get_string;
   ctx->Driver.GetBufferSize = get_buffer_size;
   ctx->Driver.Flush = flush;
   ctx->Driver.Finish = finish;
    
   /* Software rasterizer pixel paths:
    */
   ctx->Driver.Accum = _swrast_Accum;
   ctx->Driver.Bitmap = _swrast_Bitmap;
   ctx->Driver.Clear = clear_buffers;
   ctx->Driver.ResizeBuffers = xmesa_resize_buffers;
   ctx->Driver.CopyPixels = _swrast_CopyPixels;
   ctx->Driver.DrawPixels = _swrast_DrawPixels;
   ctx->Driver.ReadPixels = _swrast_ReadPixels;

   /* Software texture functions:
    */
   ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
   ctx->Driver.TexImage1D = _mesa_store_teximage1d;
   ctx->Driver.TexImage2D = _mesa_store_teximage2d;
   ctx->Driver.TexImage3D = _mesa_store_teximage3d;
   ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
   ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
   ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
   ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

   ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
   ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
   ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;

   ctx->Driver.BaseCompressedTexFormat = _mesa_base_compressed_texformat;
   ctx->Driver.CompressedTextureSize = _mesa_compressed_texture_size;
   ctx->Driver.GetCompressedTexImage = _mesa_get_compressed_teximage;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;


   /* Statechange callbacks:
    */
   ctx->Driver.SetDrawBuffer = set_draw_buffer;
   ctx->Driver.ClearIndex = clear_index;
   ctx->Driver.ClearColor = clear_color;
   ctx->Driver.IndexMask = index_mask;
   ctx->Driver.ColorMask = color_mask;
   ctx->Driver.Enable = enable;


   /* Initialize the TNL driver interface:
    */
   tnl = TNL_CONTEXT(ctx);
   tnl->Driver.RunPipeline = _tnl_run_pipeline;
   
   /* Install swsetup for tnl->Driver.Render.*:
    */
   _swsetup_Wakeup(ctx);

   (void) DitherValues;  /* silenced unused var warning */
}





#define XMESA_NEW_POINT  (_NEW_POINT | \
                          _NEW_RENDERMODE | \
                          _SWRAST_NEW_RASTERMASK)

#define XMESA_NEW_LINE   (_NEW_LINE | \
                          _NEW_TEXTURE | \
                          _NEW_LIGHT | \
                          _NEW_DEPTH | \
                          _NEW_RENDERMODE | \
                          _SWRAST_NEW_RASTERMASK)

#define XMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                            _NEW_TEXTURE | \
                            _NEW_LIGHT | \
                            _NEW_DEPTH | \
                            _NEW_RENDERMODE | \
                            _SWRAST_NEW_RASTERMASK)


/* Extend the software rasterizer with our line/point/triangle
 * functions.
 */
void xmesa_register_swrast_functions( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );

   swrast->choose_point = xmesa_choose_point;
   swrast->choose_line = xmesa_choose_line;
   swrast->choose_triangle = xmesa_choose_triangle;

   swrast->invalidate_point |= XMESA_NEW_POINT;
   swrast->invalidate_line |= XMESA_NEW_LINE;
   swrast->invalidate_triangle |= XMESA_NEW_TRIANGLE;
}
