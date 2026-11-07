/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2022  Yongang Luo       All Rights Reserved.
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
 * \file glheader.h
 * Wrapper for GL/gl*.h and GLES[3|2|]/gl*.h
 */


#ifndef GLHEADER_H
#define GLHEADER_H


/* glsl-optimizer change: no Khronos GL/GLES headers; only the types and
 * enums the compiler needs (see glminimal.h).
 */
#include "glminimal.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Custom Mesa types to save space. */
typedef unsigned char GLenum8; /* only for primitive modes */
typedef unsigned short GLenum16;
typedef unsigned char GLbitfield8;
typedef unsigned short GLbitfield16;
typedef GLuint64 GLbitfield64;

/* This is for packing the GL index type. The reason it's a structure is that
 * we don't want it to be used as integer accidentally.
 */
typedef struct {
   unsigned char value;
} GLindextype;

/* There is no formal spec for the following extension. */
#ifndef GL_ATI_texture_compression_3dc
#define GL_ATI_texture_compression_3dc                          1
#define GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI                   0x8837
#endif

/**
 * Internal token to represent a GLSL shader program (a collection of
 * one or more shaders that get linked together).  Note that GLSL
 * shaders and shader programs share one name space (one hash table)
 * so we need a value that's different from any of the
 * GL_VERTEX/FRAGMENT/GEOMETRY_PROGRAM tokens.
 */
#define GL_SHADER_PROGRAM_MESA                                  0x9999

#ifdef __cplusplus
}
#endif

#endif /* GLHEADER_H */
