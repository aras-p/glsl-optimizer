/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file context.h
 * Mesa context and visual-related functions.
 *
 * There are three large Mesa data types/classes which are meant to be
 * used by device drivers:
 * - struct gl_context: this contains the Mesa rendering state
 * - struct gl_config:  this describes the color buffer (RGB vs. ci), whether
 *   or not there's a depth buffer, stencil buffer, etc.
 * - struct gl_framebuffer:  contains pointers to the depth buffer, stencil
 *   buffer, accum buffer and alpha buffers.
 *
 * These types should be encapsulated by corresponding device driver
 * data types.  See xmesa.h and xmesaP.h for an example.
 *
 * In OOP terms, struct gl_context, struct gl_config, and struct gl_framebuffer
 * are base classes which the device driver must derive from.
 *
 * The following functions create and destroy these data types.
 */


/* glsl-optimizer change: Retain compiler API/version/extension predicates only. Omit rendering
 * context lifecycle, dispatch, begin/end, vertex flushing and VBO helpers.
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "errors.h"
#include "extensions.h"
#include "mtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline bool
_mesa_is_desktop_gl(const struct gl_context *ctx)
{
   return ctx->API == API_OPENGL_COMPAT || ctx->API == API_OPENGL_CORE;
}


/**
 * Checks if the context is for any GLES version
 */
static inline bool
_mesa_is_gles(const struct gl_context *ctx)
{
   return ctx->API == API_OPENGLES || ctx->API == API_OPENGLES2;
}


/**
 * Checks if the context is for GLES 3.0 or later
 */
static inline bool
_mesa_is_gles3(const struct gl_context *ctx)
{
   return ctx->API == API_OPENGLES2 && ctx->Version >= 30;
}


/**
 * Checks if the context is for GLES 3.1 or later
 */
static inline bool
_mesa_is_gles31(const struct gl_context *ctx)
{
   return ctx->API == API_OPENGLES2 && ctx->Version >= 31;
}


/**
 * Checks if the context is for GLES 3.2 or later
 */
static inline bool
_mesa_is_gles32(const struct gl_context *ctx)
{
   return ctx->API == API_OPENGLES2 && ctx->Version >= 32;
}


static inline bool
_mesa_is_no_error_enabled(const struct gl_context *ctx)
{
   return ctx->Const.ContextFlags & GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR;
}


static inline bool
_mesa_has_integer_textures(const struct gl_context *ctx)
{
   return _mesa_has_EXT_texture_integer(ctx) || _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_half_float_textures(const struct gl_context *ctx)
{
   return _mesa_has_ARB_texture_float(ctx) ||
          _mesa_has_OES_texture_half_float(ctx) || _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_float_textures(const struct gl_context *ctx)
{
   return _mesa_has_ARB_texture_float(ctx) ||
          _mesa_has_OES_texture_float(ctx) || _mesa_is_gles3(ctx);
 }

static inline bool
_mesa_has_texture_rgb10_a2ui(const struct gl_context *ctx)
{
   return _mesa_has_ARB_texture_rgb10_a2ui(ctx) || _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_float_depth_buffer(const struct gl_context *ctx)
{
   return _mesa_has_ARB_depth_buffer_float(ctx) || _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_packed_float(const struct gl_context *ctx)
{
   return _mesa_has_EXT_packed_float(ctx) || _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_rg_textures(const struct gl_context *ctx)
{
   return _mesa_has_ARB_texture_rg(ctx) || _mesa_has_EXT_texture_rg(ctx) ||
          _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_texture_shared_exponent(const struct gl_context *ctx)
{
   return _mesa_has_EXT_texture_shared_exponent(ctx) || _mesa_is_gles3(ctx);
}

static inline bool
_mesa_has_texture_type_2_10_10_10_REV(const struct gl_context *ctx)
{
   return _mesa_is_desktop_gl(ctx) ||
          _mesa_has_EXT_texture_type_2_10_10_10_REV(ctx);
}

/**
 * Checks if the context supports geometry shaders.
 */
static inline bool
_mesa_has_geometry_shaders(const struct gl_context *ctx)
{
   return _mesa_has_OES_geometry_shader(ctx) ||
          (_mesa_is_desktop_gl(ctx) && ctx->Version >= 32);
}


/**
 * Checks if the context supports compute shaders.
 */
static inline bool
_mesa_has_compute_shaders(const struct gl_context *ctx)
{
   return _mesa_has_ARB_compute_shader(ctx) ||
      (ctx->API == API_OPENGLES2 && ctx->Version >= 31);
}

/**
 * Checks if the context supports tessellation.
 */
static inline bool
_mesa_has_tessellation(const struct gl_context *ctx)
{
   /* _mesa_has_EXT_tessellation_shader(ctx) is redundant with the OES
    * check, so don't bother calling it.
    */
   return _mesa_has_OES_tessellation_shader(ctx) ||
          _mesa_has_ARB_tessellation_shader(ctx);
}

static inline bool
_mesa_has_texture_cube_map_array(const struct gl_context *ctx)
{
   return _mesa_has_ARB_texture_cube_map_array(ctx) ||
          _mesa_has_OES_texture_cube_map_array(ctx);
}

static inline bool
_mesa_has_texture_view(const struct gl_context *ctx)
{
   return _mesa_has_ARB_texture_view(ctx) ||
          _mesa_has_OES_texture_view(ctx);
}

#ifdef __cplusplus
}
#endif


#endif /* CONTEXT_H */
