/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef FBOBJECT_H
#define FBOBJECT_H


extern GLboolean
_mesa_IsRenderbufferEXT(GLuint renderbuffer);

extern void
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer);

extern void
_mesa_DeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers);

extern void
_mesa_GenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers);

extern void
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalformat,
                             GLsizei width, GLsizei height);

extern void
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params);

extern GLboolean
_mesa_IsFramebufferEXT(GLuint framebuffer);

extern void
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer);

extern void
_mesa_DeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers);

extern void
_mesa_GenFramebuffersEXT(GLsizei n, GLuint *framebuffers);

extern GLenum
_mesa_CheckFramebufferStatusEXT(GLenum target);

extern void
_mesa_FramebufferTexture1DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level);

extern void
_mesa_FramebufferTexture2DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level);

extern void
_mesa_FramebufferTexture3DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level, GLint zoffset);

extern void
_mesa_FramebufferRenderbufferEXT(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget,
                                 GLuint renderbuffer);

extern void
_mesa_GetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment,
                                             GLenum pname, GLint *params);

extern void
_mesa_GenerateMipmapEXT(GLenum target);


#endif /* FBOBJECT_H */
