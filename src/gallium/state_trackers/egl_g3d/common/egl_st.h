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

#ifndef _EGL_ST_H_
#define _EGL_ST_H_

#include "GL/gl.h" /* for GL types */
#include "GL/internal/glcore.h"  /* for __GLcontextModes */

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"

/* avoid calling st functions directly */
#if 1

#define ST_SURFACE_FRONT_LEFT   0
#define ST_SURFACE_BACK_LEFT    1
#define ST_SURFACE_FRONT_RIGHT  2
#define ST_SURFACE_BACK_RIGHT   3

#define ST_TEXTURE_2D    0x2

struct st_context;
struct st_framebuffer;
typedef void (*st_proc)();

#else
#include "state_tracker/st_public.h"
#endif

/* remember to update egl_g3d_get_st() when update the enums */
enum egl_g3d_st_api {
   EGL_G3D_ST_OPENGL_ES = 0,
   EGL_G3D_ST_OPENVG,
   EGL_G3D_ST_OPENGL_ES2,
   EGL_G3D_ST_OPENGL,

   NUM_EGL_G3D_STS
};

struct egl_g3d_st {
#define ST_PUBLIC(name, ret, ...) ret (*name)(__VA_ARGS__);
#include "st_public_tmp.h"
   /* fields must be added here */
   EGLint api_bit;
};

const struct egl_g3d_st *
egl_g3d_get_st(enum egl_g3d_st_api api);

#endif /* _EGL_ST_H_ */
