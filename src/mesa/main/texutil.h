
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifndef TEXUTIL_H
#define TEXUTIL_H


#include "types.h"



                           /* msb <------ TEXEL BITS -----------> lsb */
typedef enum {             /* ---- ---- ---- ---- ---- ---- ---- ---- */
   MESA_I8,                /*                               IIII IIII */
   MESA_L8,                /*                               LLLL LLLL */
   MESA_A8,                /*                               AAAA AAAA */
   MESA_C8,                /*                               CCCC CCCC */
   MESA_A8_L8,             /*                     AAAA AAAA LLLL LLLL */
   MESA_R5_G6_B5,          /*                     RRRR RGGG GGGB BBBB */
   MESA_A4_R4_G4_B4,       /*                     AAAA RRRR GGGG BBBB */
   MESA_A1_R5_G5_B5,       /*                     ARRR RRGG GGGB BBBB */
   MESA_A8_R8_G8_B8        /* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
} MesaIntTexFormat;




extern GLboolean
_mesa_convert_teximage(MesaIntTexFormat dstFormat,
                       GLint dstWidth, GLint dstHeight, GLvoid *dstImage,
                       GLint dstRowStride,
                       GLint srcWidth, GLint srcHeight,
                       GLenum srcFormat, GLenum srcType,
                       const GLvoid *srcImage,
                       const struct gl_pixelstore_attrib *packing);



extern GLboolean
_mesa_convert_texsubimage(MesaIntTexFormat dstFormat,
                          GLint dstXoffset, GLint dstYoffset,
                          GLint dstWidth, GLint dstHeight, GLvoid *dstImage,
                          GLint dstRowStride,
                          GLint width, GLint height,
                          GLint srcWidth, GLint srcHeight,
                          GLenum srcFormat, GLenum srcType,
                          const GLvoid *srcImage,
                          const struct gl_pixelstore_attrib *packing);


extern void
_mesa_unconvert_teximage(MesaIntTexFormat srcFormat,
                         GLint srcWidth, GLint srcHeight,
                         const GLvoid *srcImage, GLint srcRowStride,
                         GLint dstWidth, GLint dstHeight,
                         GLenum dstFormat, GLubyte *dstImage);


extern void
_mesa_set_teximage_component_sizes(MesaIntTexFormat mesaFormat,
                                   struct gl_texture_image *texImage);


#endif

