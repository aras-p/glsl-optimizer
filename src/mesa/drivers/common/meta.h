/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef META_H
#define META_H


extern void
_mesa_meta_init(GLcontext *ctx);

extern void
_mesa_meta_free(GLcontext *ctx);

extern void
_mesa_meta_BlitFramebuffer(GLcontext *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);

extern void
_mesa_meta_Clear(GLcontext *ctx, GLbitfield buffers);

extern void
_mesa_meta_CopyPixels(GLcontext *ctx, GLint srcx, GLint srcy,
                      GLsizei width, GLsizei height,
                      GLint dstx, GLint dsty, GLenum type);

extern void
_mesa_meta_DrawPixels(GLcontext *ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid *pixels);

extern void
_mesa_meta_Bitmap(GLcontext *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLubyte *bitmap);

extern GLboolean
_mesa_meta_check_generate_mipmap_fallback(GLcontext *ctx, GLenum target,
                                          struct gl_texture_object *texObj);

extern void
_mesa_meta_GenerateMipmap(GLcontext *ctx, GLenum target,
                          struct gl_texture_object *texObj);

extern void
_mesa_meta_CopyTexImage1D(GLcontext *ctx, GLenum target, GLint level,
                          GLenum internalFormat, GLint x, GLint y,
                          GLsizei width, GLint border);

extern void
_mesa_meta_CopyTexImage2D(GLcontext *ctx, GLenum target, GLint level,
                          GLenum internalFormat, GLint x, GLint y,
                          GLsizei width, GLsizei height, GLint border);

extern void
_mesa_meta_CopyTexSubImage1D(GLcontext *ctx, GLenum target, GLint level,
                             GLint xoffset,
                             GLint x, GLint y, GLsizei width);

extern void
_mesa_meta_CopyTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                             GLint xoffset, GLint yoffset,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height);

extern void
_mesa_meta_CopyTexSubImage3D(GLcontext *ctx, GLenum target, GLint level,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height);

extern void
_mesa_meta_CopyColorTable(GLcontext *ctx,
                          GLenum target, GLenum internalformat,
                          GLint x, GLint y, GLsizei width);

extern void
_mesa_meta_CopyColorSubTable(GLcontext *ctx,GLenum target, GLsizei start,
                             GLint x, GLint y, GLsizei width);

extern void
_mesa_meta_CopyConvolutionFilter1D(GLcontext *ctx, GLenum target,
                                   GLenum internalFormat,
                                   GLint x, GLint y, GLsizei width);

extern void
_mesa_meta_CopyConvolutionFilter2D(GLcontext *ctx, GLenum target,
                                   GLenum internalFormat, GLint x, GLint y,
                                   GLsizei width, GLsizei height);


#endif /* META_H */
