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
 * \file xm_surface.h
 * Code to allow the softpipe code to write to X windows/buffers.
 * This is a bit of a hack for now.  We've basically got two different
 * abstractions for color buffers: gl_renderbuffer and softpipe_surface.
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

#include "pipe/p_state.h"
#include "pipe/softpipe/sp_context.h"
#include "pipe/softpipe/sp_surface.h"


/**
 * An xm_surface is derived from a softpipe_surface
 */
struct xmesa_surface
{
   struct softpipe_surface sps;
   struct xmesa_renderbuffer *xrb;  /** ptr back to matching xmesa_renderbuffer */
   struct gl_renderbuffer *rb; /* ptr to matching gl_renderbuffer */
};


/**
 * Cast wrapper
 */
static INLINE struct xmesa_surface *
xmesa_surface(struct softpipe_surface *sps)
{
   return (struct xmesa_surface *) sps;
}


/**
 * quad reading/writing
 * These functions are just wrappers around the existing renderbuffer
 * functions.
 */

static void
read_quad_f(struct softpipe_surface *gs, GLint x, GLint y,
            GLfloat (*rgba)[NUM_CHANNELS])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GLubyte temp[16];
   GLfloat *dst = (GLfloat *) rgba;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y,     temp);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y + 1, temp + 8);
   for (i = 0; i < 16; i++) {
      dst[i] = UBYTE_TO_FLOAT(temp[i]);
   }
}

static void
read_quad_f_swz(struct softpipe_surface *gs, GLint x, GLint y,
                GLfloat (*rrrr)[QUAD_SIZE])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GLubyte temp[16];
   GLfloat *dst = (GLfloat *) rrrr;
   GLuint i, j;
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y,     temp);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y + 1, temp + 8);
   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         dst[j * 4 + i] = UBYTE_TO_FLOAT(temp[i * 4 + j]);
      }
   }
}

static void
write_quad_f(struct softpipe_surface *gs, GLint x, GLint y,
             GLfloat (*rgba)[NUM_CHANNELS])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GLubyte temp[16];
   const GLfloat *src = (const GLfloat *) rgba;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   for (i = 0; i < 16; i++) {
      temp[i] = FLOAT_TO_UBYTE(src[i]);
   }
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y,     temp,     NULL);
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y + 1, temp + 8, NULL);
}

static void
write_quad_f_swz(struct softpipe_surface *gs, GLint x, GLint y,
                 GLfloat (*rrrr)[QUAD_SIZE])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GLubyte temp[16];
   const GLfloat *src = (const GLfloat *) rrrr;
   GLuint i, j;
   GET_CURRENT_CONTEXT(ctx);
   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         temp[j * 4 + i] = FLOAT_TO_UBYTE(src[i * 4 + j]);
      }
   }
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y,     temp,     NULL);
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y + 1, temp + 8, NULL);
}

static void
read_quad_ub(struct softpipe_surface *gs, GLint x, GLint y,
             GLubyte (*rgba)[NUM_CHANNELS])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y,     rgba);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y + 1, rgba + 2);
}

static void
write_quad_ub(struct softpipe_surface *gs, GLint x, GLint y,
              GLubyte (*rgba)[NUM_CHANNELS])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y,     rgba);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y + 1, rgba + 2);
}

static void
write_mono_row_ub(struct softpipe_surface *gs, GLuint count, GLint x, GLint y,
                  GLubyte rgba[NUM_CHANNELS])
{
   struct xmesa_surface *xmsurf = xmesa_surface(gs);
   struct xmesa_renderbuffer *xrb = xmsurf->xrb;
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.PutMonoRow(ctx, &xrb->Base, count, x, y, rgba, NULL);
}


static struct xmesa_surface *
create_surface(XMesaContext xmctx, struct xmesa_renderbuffer *xrb)
{
   struct xmesa_surface *xmsurf;

   xmsurf = CALLOC_STRUCT(xmesa_surface);
   if (xmsurf) {
      xmsurf->xrb = xrb;
      xmsurf->sps.surface.width = xrb->Base.Width;
      xmsurf->sps.surface.height = xrb->Base.Height;

      xmsurf->sps.read_quad_f = read_quad_f;
      xmsurf->sps.read_quad_f_swz = read_quad_f_swz;
      xmsurf->sps.read_quad_ub = read_quad_ub;
      xmsurf->sps.write_quad_f = write_quad_f;
      xmsurf->sps.write_quad_f_swz = write_quad_f_swz;
      xmsurf->sps.write_quad_ub = write_quad_ub;
      xmsurf->sps.write_mono_row_ub = write_mono_row_ub;

#if 0
      if (xrb->ximage) {
         xmsurf->sps.surface.ptr = (GLubyte *) xrb->ximage->data;
         xmsurf->sps.surface.stride = xrb->ximage->bytes_per_line;
         xmsurf->sps.surface.cpp = xrb->ximage->depth;

      }
#endif
   }
   return xmsurf;
}


static void
free_surface(struct softpipe_surface *sps)
{
   /* XXX may need to do more in the future */
   free(sps);
}


/**
 * Return generic surface pointer corresponding to the current color buffer.
 */
struct pipe_surface *
xmesa_get_color_surface(GLcontext *ctx, GLuint buf)
{
   XMesaContext xmctx = XMESA_CONTEXT(ctx);
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0][buf];   
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);
   struct softpipe_surface *sps = (struct softpipe_surface *) xrb->pSurface;

   if (!sps) {
      xrb->pSurface = create_surface(xmctx, xrb);
   }
   else if (sps->surface.width != rb->Width ||
            sps->surface.height != rb->Height) {
      free_surface(sps);
      xrb->pSurface = create_surface(xmctx, xrb);
   }

   return (struct pipe_surface *) xrb->pSurface;
}


static void
read_quad_z(struct softpipe_surface *sps,
            GLint x, GLint y, GLfloat zzzz[QUAD_SIZE])
{
   struct xmesa_surface *xmsurf = xmesa_surface(sps);
   struct gl_renderbuffer *rb = xmsurf->rb;
   GLushort temp[4];
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   rb->GetRow(ctx, rb, 2, x, y,     temp);
   rb->GetRow(ctx, rb, 2, x, y + 1, temp + 2);
   for (i = 0; i < 4; i++) {
      zzzz[i] = USHORT_TO_FLOAT(temp[i]);
   }
}

static void
write_quad_z(struct softpipe_surface *sps,
             GLint x, GLint y, const GLfloat zzzz[QUAD_SIZE])
{
   struct xmesa_surface *xmsurf = xmesa_surface(sps);
   struct gl_renderbuffer *rb = xmsurf->rb;
   GLushort temp[4];
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   for (i = 0; i < 4; i++) {
      CLAMPED_FLOAT_TO_USHORT(temp[i], zzzz[i]);
   }
   rb->PutRow(ctx, rb, 2, x, y,     temp,     NULL);
   rb->PutRow(ctx, rb, 2, x, y + 1, temp + 2, NULL);
}


static struct xmesa_surface *
create_z_surface(XMesaContext xmctx, struct gl_renderbuffer *rb)
{
   struct xmesa_surface *xmsurf;

   xmsurf = CALLOC_STRUCT(xmesa_surface);
   if (xmsurf) {
      xmsurf->sps.surface.width = rb->Width;
      xmsurf->sps.surface.height = rb->Height;
      xmsurf->sps.read_quad_z = read_quad_z;
      xmsurf->sps.write_quad_z = write_quad_z;
      xmsurf->rb = rb;
   }
   return xmsurf;
}

/**
 * Return a pipe_surface that wraps the current Z/depth buffer.
 * XXX this is pretty much a total hack until gl_renderbuffers and
 * pipe_surfaces are merged...
 */
struct pipe_surface *
xmesa_get_z_surface(GLcontext *ctx)
{
   XMesaContext xmctx = XMESA_CONTEXT(ctx);
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_DepthBuffer;
   static struct xmesa_surface *xms = NULL;

   if (!rb)
      return NULL;

   if (!xms) {
      xms = create_z_surface(xmctx, rb);
   }
   else if (xms->sps.surface.width != rb->Width ||
            xms->sps.surface.height != rb->Height) {
      free_surface(&xms->sps);
      xms = create_z_surface(xmctx, rb);
   }

   return (struct pipe_surface *) &xms->sps.surface;
}


struct pipe_surface *
xmesa_get_stencil_surface(GLcontext *ctx)
{
   return NULL;
}

