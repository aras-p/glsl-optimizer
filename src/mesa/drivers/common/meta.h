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


/**
 * Flags passed to _mesa_meta_begin().
 * XXX these flags may evolve...
 */
/*@{*/
#define META_ALPHA_TEST      0x1
#define META_BLEND           0x2  /**< includes logicop */
#define META_COLOR_MASK      0x4
#define META_DEPTH_TEST      0x8
#define META_FOG            0x10
#define META_RASTERIZATION  0x20
#define META_SCISSOR        0x40
#define META_SHADER         0x80
#define META_STENCIL_TEST  0x100
#define META_TRANSFORM     0x200 /**< modelview, projection */
#define META_TEXTURE       0x400
#define META_VERTEX        0x800
#define META_VIEWPORT     0x1000
#define META_PIXEL_STORE  0x2000
#define META_ALL            ~0x0
/*@}*/


extern void
_mesa_meta_init(GLcontext *ctx);

extern void
_mesa_meta_free(GLcontext *ctx);

extern void
_mesa_meta_blit_framebuffer(GLcontext *ctx,
                            GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter);

extern void
_mesa_meta_clear(GLcontext *ctx, GLbitfield buffers);

extern void
_mesa_meta_copy_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                       GLsizei width, GLsizei height,
                       GLint dstx, GLint dsty, GLenum type);

extern void
_mesa_meta_draw_pixels(GLcontext *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       const GLvoid *pixels);


#endif /* META_H */
