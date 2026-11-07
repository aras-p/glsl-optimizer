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
#ifndef CONTEXT_H
#define CONTEXT_H

/*
 * glsl-optimizer change: minimal context.h. Only the API/version predicates
 * the GLSL compiler uses, copied from Mesa 26.2.6 main/context.h (which
 * otherwise is the GL context API and pulls in vbo/state tracker headers).
 */

#include "main/errors.h"
#include "main/extensions.h"
#include "main/mtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if the context is for Desktop GL Compatibility
 */
static inline bool
_mesa_is_desktop_gl_compat(const struct gl_context *ctx)
{
#if HAVE_OPENGL
   return ctx->API == API_OPENGL_COMPAT;
#else
   return false;
#endif
}

/**
 * Checks if the context is for Desktop GL Core
 */
static inline bool
_mesa_is_desktop_gl_core(const struct gl_context *ctx)
{
#if HAVE_OPENGL
   return ctx->API == API_OPENGL_CORE;
#else
   return false;
#endif
}

/**
 * Checks if the context is for Desktop GL (Compatibility or Core)
 */
static inline bool
_mesa_is_desktop_gl(const struct gl_context *ctx)
{
   return _mesa_is_desktop_gl_compat(ctx) || _mesa_is_desktop_gl_core(ctx);
}

/**
 * Checks if the context is for GLES 2.0 or later
 */
static inline bool
_mesa_is_gles2(const struct gl_context *ctx)
{
   return _mesa_is_api_gles2(ctx->API);
}

/**
 * Checks if the context is for GLES 3.0 or later
 */
static inline bool
_mesa_is_gles3(const struct gl_context *ctx)
{
   return _mesa_is_gles2(ctx) && ctx->Version >= 30;
}

/**
 * Checks if the context is for GLES 3.1 or later
 */
static inline bool
_mesa_is_gles31(const struct gl_context *ctx)
{
   return _mesa_is_gles2(ctx) && ctx->Version >= 31;
}

/**
 * Checks if the context is for GLES 3.2 or later
 */
static inline bool
_mesa_is_gles32(const struct gl_context *ctx)
{
   return _mesa_is_gles2(ctx) && ctx->Version >= 32;
}

static inline bool
_mesa_is_gles2_compatible(const struct gl_context *ctx)
{
   return _mesa_is_gles2(ctx) || _mesa_has_ARB_ES2_compatibility(ctx);
}

static inline bool
_mesa_is_gles3_compatible(const struct gl_context *ctx)
{
   return _mesa_is_gles3(ctx) || _mesa_has_ARB_ES3_compatibility(ctx);
}

static inline bool
_mesa_is_gles31_compatible(const struct gl_context *ctx)
{
   return _mesa_is_gles31(ctx) || _mesa_has_ARB_ES3_1_compatibility(ctx);
}

static inline bool
_mesa_is_gles32_compatible(const struct gl_context *ctx)
{
   return _mesa_is_gles32(ctx) || _mesa_has_ARB_ES3_2_compatibility(ctx);
}

#ifdef __cplusplus
}
#endif

#endif /* CONTEXT_H */
