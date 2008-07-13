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


#include "glxheader.h"
#include "GL/xmesa.h"
#include "xmesaP.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "softpipe/sp_context.h"
#include "softpipe/sp_clear.h"
#include "softpipe/sp_tile_cache.h"
#include "softpipe/sp_surface.h"
#include "state_tracker/st_context.h"


#define CLIP_TILE \
   do { \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height -y; \
   } while(0)


static INLINE struct xmesa_surface *
xmesa_surface(struct pipe_surface *ps)
{
   return (struct xmesa_surface *) ps;
}


static INLINE struct xmesa_renderbuffer *
xmesa_rb(struct pipe_surface *ps)
{
   struct xmesa_surface *xms = xmesa_surface(ps);
   return xms->xrb;
}


#define FLIP(Y) Y = xrb->St.Base.Height - (Y) - 1;


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
      softpipe_get_tile_rgba(ps, x, y, w, h, p);
   }
}


void
xmesa_put_tile_rgba(struct pipe_context *pipe, struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h, const float *p)
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
         for (j = 0; j < w * 4; j++) {
            UNCLAMPED_FLOAT_TO_UBYTE(tmp[j], p[j]);
         }
         xrb->St.Base.PutRow(ctx, &xrb->St.Base, w, x, y - i, tmp, NULL);
         p += w0 * 4;
      }
#if 0 /* debug: flush */
      {
         XMesaContext xm = XMESA_CONTEXT(ctx);
         XSync(xm->display, 0);
      }
#endif
   }
   else {
      /* other softpipe surface */
      softpipe_put_tile_rgba(ps, x, y, w, h, p);
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


boolean
xmesa_is_format_supported(struct pipe_context *pipe, uint format)
{
   switch( format ) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
   case PIPE_FORMAT_S_R16_G16_B16_A16:
   case PIPE_FORMAT_S8_Z24:
      return TRUE;
   };
   return FALSE;
}


/**
 * Called via pipe->clear()
 */
void
xmesa_clear(struct pipe_context *pipe, struct pipe_surface *ps, GLuint value)
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(ps);

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

   if (xrb && xrb->ximage) {
      /* clearing back color buffer */
      GET_CURRENT_CONTEXT(ctx);
      xmesa_clear_buffers(ctx, BUFFER_BIT_BACK_LEFT);
   }
   else if (xrb && xrb->pixmap) {
      /* clearing front color buffer */
      GET_CURRENT_CONTEXT(ctx);
      xmesa_clear_buffers(ctx, BUFFER_BIT_FRONT_LEFT);
   }
   else {
      /* clearing other buffer */
      softpipe_clear(pipe, ps, value);
   }
}

