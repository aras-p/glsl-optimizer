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


#include "context.h"
#include "fbobject.h"
#include "hash.h"


/* XXX these are temporary here! */
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX_EXT                   0x8D45
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT   0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_STATUS_ERROR_EXT                      0x8CDE
#define GL_FRAMEBUFFER_BINDING_EXT             0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT            0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT           0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT           0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT   0x0506


struct gl_render_buffer_object
{
   GLint RefCount;
   GLuint Name;

   GLuint width, height;
};


struct gl_frame_buffer_object
{
   GLint RefCount;
   GLuint Name;

};



/**
 * Helper routine for getting a gl_render_buffer_object.
 */
static struct gl_render_buffer_object *
LookupRenderbuffer(GLcontext *ctx, GLuint id)
{
   struct gl_render_buffer_object *rb;

   if (!id == 0)
      return NULL;

   rb = (struct gl_render_buffer_object *)
      _mesa_HashLookup(ctx->Shared->RenderBuffers, id);
   return rb;
}


/**
 * Helper routine for getting a gl_frame_buffer_object.
 */
static struct gl_frame_buffer_object *
LookupFramebuffer(GLcontext *ctx, GLuint id)
{
   struct gl_frame_buffer_object *rb;

   if (!id == 0)
      return NULL;

   rb = (struct gl_frame_buffer_object *)
      _mesa_HashLookup(ctx->Shared->FrameBuffers, id);
   return rb;
}


/**
 * Allocate a new gl_render_buffer_object.
 * XXX make this a device driver function.
 */
static struct gl_render_buffer_object *
NewRenderBuffer(GLcontext *ctx, GLuint name)
{
   struct gl_render_buffer_object *rb = CALLOC_STRUCT(gl_render_buffer_object);
   if (rb) {
      rb->Name = name;
      rb->RefCount = 1;
   }
   return rb;
}


/**
 * Allocate a new gl_frame_buffer_object.
 * XXX make this a device driver function.
 */
static struct gl_frame_buffer_object *
NewFrameBuffer(GLcontext *ctx, GLuint name)
{
   struct gl_frame_buffer_object *fb = CALLOC_STRUCT(gl_frame_buffer_object);
   if (fb) {
      fb->Name = name;
      fb->RefCount = 1;
   }
   return fb;
}



GLboolean
_mesa_IsRenderbufferEXT(GLuint renderbuffer)
{
   struct gl_render_buffer_object *rb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   rb = LookupRenderbuffer(ctx, renderbuffer);
   return rb ? GL_TRUE : GL_FALSE;
}


void
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
   struct gl_render_buffer_object *rb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
   case GL_RENDERBUFFER_EXT:
      break;
   case GL_FRAMEBUFFER_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindRenderbufferEXT(target)");
      return;
   }

   rb = LookupRenderbuffer(ctx, renderbuffer);
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glIsRenderbufferEXT(renderbuffer)");
      return;
   }


}


void
_mesa_DeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   for (i = 0; i < n; i++) {
      struct gl_render_buffer_object *rb;
      rb = LookupRenderbuffer(ctx, renderbuffers[i]);
      if (rb) {
         /* remove from hash table immediately, to free the ID */
         _mesa_HashRemove(ctx->Shared->RenderBuffers, renderbuffers[i]);

         /* But the object will not be freed until it's no longer bound in
          * any context.
          */
         rb->RefCount--;
         if (rb->RefCount == 0) {
            _mesa_free(rb); /* XXX call device driver function */
         }
      }
   }
}


void
_mesa_GenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenRenderbuffersEXT(n)");
      return;
   }

   if (!renderbuffers)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->RenderBuffers, n);

   for (i = 0; i < n; i++) {
      struct gl_render_buffer_object *rb;
      GLuint name = first + i;
      rb = NewRenderBuffer(ctx, name);
      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenRenderbuffersEXT");
         return;
      }

      /* insert into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->RenderBuffers, name, rb);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

      renderbuffers[i] = name;
   }
}



void
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
                             GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
   case GL_RENDERBUFFER_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glRenderbufferStorageEXT(target)");
      return;
   }

   switch (internalFormat) {
   case GL_STENCIL_INDEX_EXT:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glRenderbufferStorageEXT(internalFormat)");
      return;
   }


   if (width > 0 /*value of MAX_RENDERBUFFER_SIZE_EXT*/) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRenderbufferStorageEXT(width)");
      return;
   }

   if (height > 0 /*value of MAX_RENDERBUFFER_SIZE_EXT*/) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRenderbufferStorageEXT(height)");
      return;
   }



}



void
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
   case GL_RENDERBUFFER_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }

   switch (pname) {
   case GL_RENDERBUFFER_WIDTH_EXT:
   case GL_RENDERBUFFER_HEIGHT_EXT:
   case GL_RENDERBUFFER_INTERNAL_FORMAT_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }

   if (1/*GL_RENDERBUFFER_BINDING_EXT is zero*/) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetRenderbufferParameterivEXT");
      return;
   }

   *params = 0;
}



GLboolean
_mesa_IsFramebufferEXT(GLuint framebuffer)
{
   struct gl_frame_buffer_object *fb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   fb = LookupFramebuffer(ctx, framebuffer);
   return fb ? GL_TRUE : GL_FALSE;
}


void
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
   case GL_FRAMEBUFFER_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glBindFramebufferEXT(target)");
      return;
   }


}


void
_mesa_DeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   for (i = 0; i < n; i++) {
      struct gl_frame_buffer_object *fb;
      fb = LookupFramebuffer(ctx, framebuffers[i]);
      if (fb) {
         /* remove from hash table immediately, to free the ID */
         _mesa_HashRemove(ctx->Shared->FrameBuffers, framebuffers[i]);

         /* But the object will not be freed until it's no longer bound in
          * any context.
          */
         fb->RefCount--;
         if (fb->RefCount == 0) {
            _mesa_free(fb); /* XXX call device driver function */
         }
      }
   }
}


void
_mesa_GenFramebuffersEXT(GLsizei n, GLuint *framebuffers)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenFramebuffersEXT(n)");
      return;
   }

   if (!framebuffers)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->FrameBuffers, n);

   for (i = 0; i < n; i++) {
      struct gl_frame_buffer_object *fb;
      GLuint name = first + i;
      fb = NewFrameBuffer(ctx, name);
      if (!fb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenFramebuffersEXT");
         return;
      }

      /* insert into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->FrameBuffers, name, fb);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

      framebuffers[i] = name;
   }
}



GLenum
_mesa_CheckFramebufferStatusEXT(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FRAMEBUFFER_STATUS_ERROR_EXT);

   switch (target) {
   case GL_FRAMEBUFFER_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glCheckFramebufferStatus(target)");
      return GL_FRAMEBUFFER_STATUS_ERROR_EXT;
   }

   /* return one of:
      GL_FRAMEBUFFER_COMPLETE_EXT
      GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT
      GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT
      GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT
      GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT
      GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT
      GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT
      GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT
      GL_FRAMEBUFFER_UNSUPPORTED_EXT
      GL_FRAMEBUFFER_STATUS_ERROR_EXT
   */
   return GL_FRAMEBUFFER_STATUS_ERROR_EXT;
}



/**
 * Do error checking common to glFramebufferTexture1D/2D/3DEXT.
 * \return GL_TRUE if any error, GL_FALSE otherwise
 */
static GLboolean
error_check_framebuffer_texture(GLcontext *ctx,
                                GLuint dims, GLenum target, GLenum attachment,
                                GLint level)
{
   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%dDEXT(target)", dims);
      return GL_TRUE;
   }

   switch (attachment) {
   case GL_COLOR_ATTACHMENT0_EXT:
   case GL_COLOR_ATTACHMENT1_EXT:
   case GL_COLOR_ATTACHMENT2_EXT:
   case GL_COLOR_ATTACHMENT3_EXT:
   case GL_COLOR_ATTACHMENT4_EXT:
   case GL_COLOR_ATTACHMENT5_EXT:
   case GL_COLOR_ATTACHMENT6_EXT:
   case GL_COLOR_ATTACHMENT7_EXT:
   case GL_COLOR_ATTACHMENT8_EXT:
   case GL_COLOR_ATTACHMENT9_EXT:
   case GL_COLOR_ATTACHMENT10_EXT:
   case GL_COLOR_ATTACHMENT11_EXT:
   case GL_COLOR_ATTACHMENT12_EXT:
   case GL_COLOR_ATTACHMENT13_EXT:
   case GL_COLOR_ATTACHMENT14_EXT:
   case GL_COLOR_ATTACHMENT15_EXT:
   case GL_DEPTH_ATTACHMENT_EXT:
   case GL_STENCIL_ATTACHMENT_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%dDEXT(level)", dims);
      return GL_TRUE;
   }

   if (level < 0 /* || level too large */) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glFramebufferTexture%dDEXT(level)", dims);
      return GL_TRUE;
   }

   if (target == GL_TEXTURE_RECTANGLE_ARB && level != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glFramebufferTexture%dDEXT(level)", dims);
      return GL_TRUE;
   }

   return GL_FALSE;
}


void
_mesa_FramebufferTexture1DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 1, target, attachment, level))
      return;

}


void
_mesa_FramebufferTexture2DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 1, target, attachment, level))
      return;
}


void
_mesa_FramebufferTexture3DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level, GLint zoffset)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 1, target, attachment, level))
      return;

   /* check that zoffset isn't too large */
}



void
_mesa_FramebufferRenderbufferEXT(GLenum target, GLenum attachment,
                                 GLenum renderbufferTarget,
                                 GLuint renderbuffer)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(target)");
      return;
   }

   switch (attachment) {
   case GL_COLOR_ATTACHMENT0_EXT:
   case GL_COLOR_ATTACHMENT1_EXT:
   case GL_COLOR_ATTACHMENT2_EXT:
   case GL_COLOR_ATTACHMENT3_EXT:
   case GL_COLOR_ATTACHMENT4_EXT:
   case GL_COLOR_ATTACHMENT5_EXT:
   case GL_COLOR_ATTACHMENT6_EXT:
   case GL_COLOR_ATTACHMENT7_EXT:
   case GL_COLOR_ATTACHMENT8_EXT:
   case GL_COLOR_ATTACHMENT9_EXT:
   case GL_COLOR_ATTACHMENT10_EXT:
   case GL_COLOR_ATTACHMENT11_EXT:
   case GL_COLOR_ATTACHMENT12_EXT:
   case GL_COLOR_ATTACHMENT13_EXT:
   case GL_COLOR_ATTACHMENT14_EXT:
   case GL_COLOR_ATTACHMENT15_EXT:
   case GL_DEPTH_ATTACHMENT_EXT:
   case GL_STENCIL_ATTACHMENT_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                 "glFramebufferRenderbufferEXT(attachment)");
      return;
   }

   if (renderbufferTarget != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                 "glFramebufferRenderbufferEXT(renderbufferTarget)");
      return;
   }


}



void
_mesa_GetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment,
                                             GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(target)");
      return;
   }

   switch (attachment) {
   case GL_COLOR_ATTACHMENT0_EXT:
   case GL_COLOR_ATTACHMENT1_EXT:
   case GL_COLOR_ATTACHMENT2_EXT:
   case GL_COLOR_ATTACHMENT3_EXT:
   case GL_COLOR_ATTACHMENT4_EXT:
   case GL_COLOR_ATTACHMENT5_EXT:
   case GL_COLOR_ATTACHMENT6_EXT:
   case GL_COLOR_ATTACHMENT7_EXT:
   case GL_COLOR_ATTACHMENT8_EXT:
   case GL_COLOR_ATTACHMENT9_EXT:
   case GL_COLOR_ATTACHMENT10_EXT:
   case GL_COLOR_ATTACHMENT11_EXT:
   case GL_COLOR_ATTACHMENT12_EXT:
   case GL_COLOR_ATTACHMENT13_EXT:
   case GL_COLOR_ATTACHMENT14_EXT:
   case GL_COLOR_ATTACHMENT15_EXT:
   case GL_DEPTH_ATTACHMENT_EXT:
   case GL_STENCIL_ATTACHMENT_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(attachment)");
      return;
   }

   switch (pname) {
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(pname)");
      return;
   }

   *params = 0;
}



void
_mesa_GenerateMipmapEXT(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_3D:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGenerateMipmapEXT(target)");
      return;
   }

}
