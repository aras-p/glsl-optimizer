/* $Id: image.h,v 1.2 1999/11/03 17:27:05 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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





#ifndef IMAGE_H
#define IMAGE_H


#include "types.h"


extern void gl_flip_bytes( GLubyte *p, GLuint n );


extern void gl_swap2( GLushort *p, GLuint n );

extern void gl_swap4( GLuint *p, GLuint n );


extern GLint gl_sizeof_type( GLenum type );

extern GLint gl_sizeof_packed_type( GLenum type );

extern GLint gl_components_in_format( GLenum format );

extern GLint gl_bytes_per_pixel( GLenum format, GLenum type );

extern GLboolean gl_is_legal_format_and_type( GLenum format, GLenum type );


extern GLvoid *
gl_pixel_addr_in_image( const struct gl_pixelstore_attrib *packing,
                        const GLvoid *image, GLsizei width,
                        GLsizei height, GLenum format, GLenum type,
                        GLint img, GLint row, GLint column );


extern struct gl_image *
gl_unpack_bitmap( GLcontext *ctx, GLsizei width, GLsizei height,
                  const GLubyte *bitmap,
                  const struct gl_pixelstore_attrib *packing );


extern void gl_unpack_polygon_stipple( const GLcontext *ctx,
                                       const GLubyte *pattern,
                                       GLuint dest[32] );


extern void gl_pack_polygon_stipple( const GLcontext *ctx,
                                     const GLuint pattern[32],
                                     GLubyte *dest );


extern struct gl_image *
gl_unpack_image( GLcontext *ctx, GLint width, GLint height,
                 GLenum srcFormat, GLenum srcType, const GLvoid *pixels,
                 const struct gl_pixelstore_attrib *packing );



struct gl_image *
gl_unpack_image3D( GLcontext *ctx, GLint width, GLint height,GLint depth,
                   GLenum srcFormat, GLenum srcType, const GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing );


extern void
gl_pack_rgba_span( const GLcontext *ctx,
                   GLuint n, CONST GLubyte rgba[][4],
                   GLenum format, GLenum type, GLvoid *dest,
                   const struct gl_pixelstore_attrib *packing,
                   GLboolean applyTransferOps );


extern void gl_free_image( struct gl_image *image );


extern GLboolean gl_image_error_test( GLcontext *ctx,
                                      const struct gl_image *image,
                                      const char *msg );


/*
 * New (3.3) functions
 */


extern void
_mesa_unpack_ubyte_color_span( const GLcontext *ctx,
                               GLuint n, GLenum dstFormat, GLubyte dest[],
                               GLenum srcFormat, GLenum srcType,
                               const GLvoid *source,
                               const struct gl_pixelstore_attrib *unpacking,
                               GLboolean applyTransferOps );


extern void
_mesa_unpack_index_span( const GLcontext *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *unpacking,
                         GLboolean applyTransferOps );

extern void *
_mesa_unpack_image( GLsizei width, GLsizei height, GLsizei depth,
                    GLenum format, GLenum type, const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *unpack );


#endif
