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


/**
 * A renderbuffer stores colors or depth values or stencil values.
 * A framebuffer object will have a collection of these.
 * Data are read/written to the buffer with a handful of Get/Put functions.
 *
 * Instances of this object are allocated with the Driver's NewRenderbuffer
 * hook.  Drivers will likely wrap this class inside a driver-specific
 * class to simulate inheritance.
 */
struct gl_renderbuffer
{
   GLuint Name;
   GLint RefCount;
   GLuint Width, Height;
   GLenum InternalFormat;
   GLenum _BaseFormat;  /* Either GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT or */
                        /* GL_STENCIL_INDEX. */

   GLenum DataType; /* GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, etc */
   GLvoid *Data;

   /* Delete this renderbuffer */
   void (*Delete)(GLcontext *ctx, struct gl_renderbuffer *rb);

   /* Allocate new storage for this renderbuffer */
   GLboolean (*AllocStorage)(GLcontext *ctx, struct gl_renderbuffer *rb,
                             GLenum internalFormat,
                             GLuint width, GLuint height);

   /* Return a pointer to the element/pixel at (x,y).
    * Should return NULL if the buffer memory can't be directly addressed.
    */
   void *(*GetPointer)(struct gl_renderbuffer *rb, GLint x, GLint y);

   /* Get/Read a row of values.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*GetRow)(struct gl_renderbuffer *rb,
                  GLint x, GLint y, GLuint count, void *values);

   /* Get/Read values at arbitrary locations
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*GetValues)(struct gl_renderbuffer *rb,
                     const GLint x[], const GLint y[],
                     GLuint count, void *values);

   /* Put/Write a row of values
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*PutRow)(struct gl_renderbuffer *rb,
                  GLint x, GLint y, GLuint count,
                  const void *values, const GLubyte *maek);

   /* Put/Write values at arbitrary locations
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*PutValues)(struct gl_renderbuffer *rb,
                     const GLint x[], const GLint y[], GLuint count,
                     const void *values, const GLubyte *mask);
};


/**
 * A renderbuffer attachment point points to either a texture object
 * (and specifies a mipmap level, cube face or 3D texture slice) or
 * points to a renderbuffer.
 */
struct gl_renderbuffer_attachment
{
   GLenum Type;  /* GL_NONE or GL_TEXTURE or GL_RENDERBUFFER_EXT */
   GLboolean Complete;

   /* IF Type == GL_RENDERBUFFER_EXT: */
   struct gl_renderbuffer *Renderbuffer;

   /* IF Type == GL_TEXTURE: */
   struct gl_texture_object *Texture;
   GLuint TextureLevel;
   GLuint CubeMapFace;  /* 0 .. 5, for cube map textures */
   GLuint Zoffset;      /* for 3D textures */
};


/**
 * A framebuffer object is basically a collection of rendering buffers.
 * (Though, a rendering buffer might actually be a texture image.)
 * All the renderbuffers/textures which we reference must have the same
 * width and height (and meet a few other requirements) in order for the
 * framebufffer object to be "complete".
 *
 * Instances of this object are allocated with the Driver's Newframebuffer
 * hook.  Drivers will likely wrap this class inside a driver-specific
 * class to simulate inheritance.
 */
struct gl_framebuffer
{
   GLuint Name;
   GLint RefCount;

   GLenum Status; /* One of the GL_FRAMEBUFFER_(IN)COMPLETE_* tokens */

   struct gl_renderbuffer_attachment ColorAttachment[MAX_COLOR_ATTACHMENTS];
   struct gl_renderbuffer_attachment DepthAttachment;
   struct gl_renderbuffer_attachment StencilAttachment;

   /* In unextended OpenGL, these vars are part of the GL_COLOR_BUFFER
    * attribute group and GL_PIXEL attribute group, respectively.
    */
   GLenum DrawBuffer[MAX_DRAW_BUFFERS];
   GLenum ReadBuffer;

   GLuint _Width, _Height;

   /** Delete this framebuffer */
   void (*Delete)(GLcontext *ctx, struct gl_framebuffer *fb);
};


extern struct gl_framebuffer *
_mesa_new_framebuffer(GLcontext *ctx, GLuint name);

extern void
_mesa_delete_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb);

extern struct gl_renderbuffer *
_mesa_new_renderbuffer(GLcontext *ctx, GLuint name);

extern void
_mesa_delete_renderbuffer(GLcontext *ctx, struct gl_renderbuffer *rb);


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
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname,
                                    GLint *params);

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
