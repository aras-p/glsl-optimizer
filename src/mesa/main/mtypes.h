/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file mtypes.h
 * Main Mesa data structures.
 *
 * Please try to mark derived values with a leading underscore ('_').
 */

#ifndef MTYPES_H
#define MTYPES_H

/*
 * glsl-optimizer change: minimal mtypes.h. Upstream this is the whole GL
 * context state (3700 lines, pulling in vbo, state tracker, glapi, matrix
 * and gallium state headers). The GLSL compiler only uses the members of
 * struct gl_context below; definitions are taken from Mesa 26.2.6 mtypes.h.
 */

#include <stdint.h>
#include <stdbool.h>
#include "util/glheader.h"
#include "main/consts_exts.h"
#include "main/shader_types.h"
#include "main/menums.h"
#include "main/config.h"
#include "compiler/shader_enums.h"
#include "compiler/shader_info.h"
#include "pipe/p_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gl_shared_state;
struct gl_program;
struct pipe_screen;

/**
 * Context state for GLSL vertex/fragment shaders.
 */
#define GLSL_DUMP      0x1  /**< Dump shaders to stdout */
#define GLSL_LOG       0x2  /**< Write shaders to files */
#define GLSL_UNIFORMS  0x4  /**< Print glUniform calls */
#define GLSL_NOP_VERT  0x8  /**< Force no-op vertex shaders */
#define GLSL_NOP_FRAG 0x10  /**< Force no-op fragment shaders */
#define GLSL_USE_PROG 0x20  /**< Log glUseProgram calls */
#define GLSL_REPORT_ERRORS 0x40  /**< Print compilation errors */
#define GLSL_DUMP_ON_ERROR 0x80 /**< Dump shaders to stderr on compile error */
#define GLSL_CACHE_INFO 0x100 /**< Print debug information about shader cache */
#define GLSL_CACHE_FALLBACK 0x200 /**< Force shader cache fallback paths */
#define GLSL_SOURCE 0x400 /**< Only dump GLSL */

/** Subset of upstream struct gl_pipeline_object */
struct gl_pipeline_object
{
   GLbitfield Flags;         /**< Mask of GLSL_x flags */
};

/** Subset of upstream struct dd_function_table (main/dd.h) */
struct dd_function_table
{
   /** Allocate a new program */
   struct gl_program * (*NewProgram)(struct gl_context *ctx,
                                     mesa_shader_stage stage,
                                     GLuint id, bool is_arb_asm);
};

/** Subset of upstream struct gl_context */
struct gl_context
{
   /** State possibly shared with other contexts in the address space */
   struct gl_shared_state *Shared;

   /** \name API function pointer tables */
   gl_api API;

   /** Driver function pointers */
   struct dd_function_table Driver;

   /** Core/Driver constants */
   struct gl_constants Const;

   /** Extension information */
   struct gl_extensions Extensions;

   /** GL version integer, for example 31 for GL 3.1, or 20 for GLES 2.0. */
   GLuint Version;

   /** Current GLSL program pipeline (only Flags are used) */
   struct gl_pipeline_object *_Shader;

   /** The gallium screen, for shader caps and NIR options */
   struct pipe_screen *screen;
};

#ifdef __cplusplus
}
#endif

#endif /* MTYPES_H */
