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
#include "pipe/p_defines.h"
#include "pipe/softpipe/sp_context.h"
#include "pipe/softpipe/sp_surface.h"


static void *
map_surface_buffer(struct pipe_buffer *pb, GLuint access_mode)
{
   /* no-op */
   return NULL;
}


static void
unmap_surface_buffer(struct pipe_buffer *pb)
{
   /* no-op */
}


static INLINE struct xmesa_renderbuffer *
xmesa_rb(struct softpipe_surface *sps)
{
   return (struct xmesa_renderbuffer *) sps->surface.rb;
}


/**
 * quad reading/writing
 * These functions are just wrappers around the existing renderbuffer
 * functions.
 */

static void
read_quad_f(struct softpipe_surface *sps, GLint x, GLint y,
            GLfloat (*rgba)[NUM_CHANNELS])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
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
read_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                GLfloat (*rrrr)[QUAD_SIZE])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
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
write_quad_f(struct softpipe_surface *sps, GLint x, GLint y,
             GLfloat (*rgba)[NUM_CHANNELS])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
   GLubyte temp[16];
   const GLfloat *src = (const GLfloat *) rgba;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   for (i = 0; i < 16; i++) {
      UNCLAMPED_FLOAT_TO_UBYTE(temp[i], src[i]);
   }
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y,     temp,     NULL);
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y + 1, temp + 8, NULL);
}

static void
write_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                 GLfloat (*rrrr)[QUAD_SIZE])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
   GLubyte temp[16];
   const GLfloat *src = (const GLfloat *) rrrr;
   GLuint i, j;
   GET_CURRENT_CONTEXT(ctx);
   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         UNCLAMPED_FLOAT_TO_UBYTE(temp[j * 4 + i], src[i * 4 + j]);
      }
   }
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y,     temp,     NULL);
   xrb->Base.PutRow(ctx, &xrb->Base, 2, x, y + 1, temp + 8, NULL);
}

static void
read_quad_ub(struct softpipe_surface *sps, GLint x, GLint y,
             GLubyte (*rgba)[NUM_CHANNELS])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y,     rgba);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y + 1, rgba + 2);
}

static void
write_quad_ub(struct softpipe_surface *sps, GLint x, GLint y,
              GLubyte (*rgba)[NUM_CHANNELS])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y,     rgba);
   xrb->Base.GetRow(ctx, &xrb->Base, 2, x, y + 1, rgba + 2);
}

static void
write_mono_row_ub(struct softpipe_surface *sps, GLuint count, GLint x, GLint y,
                  GLubyte rgba[NUM_CHANNELS])
{
   struct xmesa_renderbuffer *xrb = xmesa_rb(sps);
   GET_CURRENT_CONTEXT(ctx);
   xrb->Base.PutMonoRow(ctx, &xrb->Base, count, x, y, rgba, NULL);
}


static void
resize_surface(struct pipe_surface *ps, GLuint width, GLuint height)
{
   ps->width = width;
   ps->height = height;
}


/**
 * Called to create a pipe_surface for each X renderbuffer.
 */
struct pipe_surface *
xmesa_new_surface(struct xmesa_renderbuffer *xrb)
{
   struct softpipe_surface *sps;

   sps = CALLOC_STRUCT(softpipe_surface);
   if (!sps)
      return NULL;

   sps->surface.rb = xrb;
   sps->surface.width = xrb->Base.Width;
   sps->surface.height = xrb->Base.Height;

   sps->read_quad_f = read_quad_f;
   sps->read_quad_f_swz = read_quad_f_swz;
   sps->read_quad_ub = read_quad_ub;
   sps->write_quad_f = write_quad_f;
   sps->write_quad_f_swz = write_quad_f_swz;
   sps->write_quad_ub = write_quad_ub;
   sps->write_mono_row_ub = write_mono_row_ub;

   sps->surface.buffer.map = map_surface_buffer;
   sps->surface.buffer.unmap = unmap_surface_buffer;
   sps->surface.resize = resize_surface;

   return &sps->surface;
}
