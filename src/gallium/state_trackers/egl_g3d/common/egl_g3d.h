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

#ifndef _EGL_G3D_H_
#define _EGL_G3D_H_

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_format.h"
#include "egldriver.h"
#include "egldisplay.h"
#include "eglcontext.h"
#include "eglsurface.h"
#include "eglconfig.h"
#include "eglscreen.h"
#include "eglmode.h"

#include "native.h"
#include "egl_st.h"

struct egl_g3d_driver {
   _EGLDriver base;
   const struct egl_g3d_st *stapis[NUM_EGL_G3D_STS];
   EGLint api_mask;

   EGLint probe_key;
};

struct egl_g3d_display {
   struct native_display *native;
};

struct egl_g3d_buffer {
   struct st_framebuffer *st_fb;
   uint attachment_mask;
};

struct egl_g3d_context {
   _EGLContext base;

   const struct egl_g3d_st *stapi;
   struct pipe_context *pipe;

   struct st_context *st_ctx;
   EGLBoolean force_validate;
   struct egl_g3d_buffer draw, read;
};

struct egl_g3d_surface {
   _EGLSurface base;
   struct native_surface *native;
   enum native_attachment render_att;
   struct pipe_surface *render_surface;
   unsigned int sequence_number;
};

struct egl_g3d_config {
   _EGLConfig base;
   const struct native_config *native;
};

struct egl_g3d_screen {
   _EGLScreen base;
   const struct native_connector *native;
   const struct native_mode **native_modes;
};

static INLINE struct egl_g3d_driver *
egl_g3d_driver(_EGLDriver *drv)
{
   return (struct egl_g3d_driver *) drv;
}

static INLINE struct egl_g3d_display *
egl_g3d_display(_EGLDisplay *dpy)
{
   /* note that it is not direct casting */
   return (struct egl_g3d_display *) dpy->DriverData;
}

static INLINE struct egl_g3d_context *
egl_g3d_context(_EGLContext *ctx)
{
   return (struct egl_g3d_context *) ctx;
}

static INLINE struct egl_g3d_surface *
egl_g3d_surface(_EGLSurface *surf)
{
   return (struct egl_g3d_surface *) surf;
}

static INLINE struct egl_g3d_config *
egl_g3d_config(_EGLConfig *conf)
{
   return (struct egl_g3d_config *) conf;
}

static INLINE struct egl_g3d_screen *
egl_g3d_screen(_EGLScreen *scr)
{
   return (struct egl_g3d_screen *) scr;
}

#endif /* _EGL_G3D_H_ */
