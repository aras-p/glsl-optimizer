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


struct gl_render_buffer_object
{
   GLint RefCount;
   GLuint Name;
   GLuint Width, Height;
   GLenum InternalFormat;
   GLenum _BaseFormat;  /* Either GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT or */
                        /* GL_STENCIL_INDEX. */
   GLvoid *Data;
};


struct gl_render_buffer_attachment
{
   GLenum Type;  /* GL_NONE or GL_TEXTURE or GL_RENDERBUFFER_EXT */
   /* IF Type == GL_RENDERBUFFER_EXT: */
   struct gl_render_buffer_object *Renderbuffer;
   /* IF Type == GL_TEXTURE: */
   struct gl_texture_object *Texture;
   GLuint TextureLevel;
   GLuint CubeMapFace;  /* 0 .. 5, for cube map textures */
   GLuint Zoffset;      /* for 3D textures */
   GLboolean Complete;
};


struct gl_frame_buffer_object
{
   GLint RefCount;
   GLuint Name;

   GLenum Status;

   struct gl_render_buffer_attachment ColorAttachment[MAX_COLOR_ATTACHMENTS];
   struct gl_render_buffer_attachment DepthAttachment;
   struct gl_render_buffer_attachment StencilAttachment;

   /* In unextended OpenGL, these vars are part of the GL_COLOR_BUFFER
    * attribute group and GL_PIXEL attribute group, respectively.
    */
   GLenum DrawBuffer[MAX_DRAW_BUFFERS];
   GLenum ReadBuffer;
};


extern GLboolean GLAPIENTRY
_mesa_IsRenderbufferEXT(GLuint renderbuffer);

extern void GLAPIENTRY
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer);

extern void GLAPIENTRY
_mesa_DeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers);

extern void GLAPIENTRY
_mesa_GenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers);

extern void GLAPIENTRY
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalformat,
                             GLsizei width, GLsizei height);

extern void GLAPIENTRY
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params);

extern GLboolean GLAPIENTRY
_mesa_IsFramebufferEXT(GLuint framebuffer);

extern void GLAPIENTRY
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer);

extern void GLAPIENTRY
_mesa_DeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers);

extern void GLAPIENTRY
_mesa_GenFramebuffersEXT(GLsizei n, GLuint *framebuffers);

extern GLenum GLAPIENTRY
_mesa_CheckFramebufferStatusEXT(GLenum target);

extern void GLAPIENTRY
_mesa_FramebufferTexture1DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level);

extern void GLAPIENTRY
_mesa_FramebufferTexture2DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level);

extern void GLAPIENTRY
_mesa_FramebufferTexture3DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level, GLint zoffset);

extern void GLAPIENTRY
_mesa_FramebufferRenderbufferEXT(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget,
                                 GLuint renderbuffer);

extern void GLAPIENTRY
_mesa_GetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment,
                                             GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GenerateMipmapEXT(GLenum target);


#endif /* FBOBJECT_H */
