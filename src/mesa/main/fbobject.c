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


/*
 * Authors:
 *   Brian Paul
 */


#include "context.h"
#include "fbobject.h"
#include "hash.h"
#include "teximage.h"
#include "texstore.h"


static struct gl_frame_buffer_object DummyFramebuffer;
static struct gl_render_buffer_object DummyRenderbuffer;


#define IS_CUBE_FACE(TARGET) \
   ((TARGET) >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && \
    (TARGET) <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)


/**
 * Helper routine for getting a gl_render_buffer_object.
 */
static struct gl_render_buffer_object *
lookup_renderbuffer(GLcontext *ctx, GLuint id)
{
   struct gl_render_buffer_object *rb;

   if (id == 0)
      return NULL;

   rb = (struct gl_render_buffer_object *)
      _mesa_HashLookup(ctx->Shared->RenderBuffers, id);
   return rb;
}


/**
 * Helper routine for getting a gl_frame_buffer_object.
 */
static struct gl_frame_buffer_object *
lookup_framebuffer(GLcontext *ctx, GLuint id)
{
   struct gl_frame_buffer_object *fb;

   if (id == 0)
      return NULL;

   fb = (struct gl_frame_buffer_object *)
      _mesa_HashLookup(ctx->Shared->FrameBuffers, id);
   return fb;
}


/**
 * Allocate a new gl_render_buffer_object.
 * XXX make this a device driver function.
 */
static struct gl_render_buffer_object *
new_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct gl_render_buffer_object *rb = CALLOC_STRUCT(gl_render_buffer_object);
   if (rb) {
      rb->Name = name;
      rb->RefCount = 1;
      /* other fields are zero */
   }
   return rb;
}


/**
 * Allocate a new gl_frame_buffer_object.
 * XXX make this a device driver function.
 */
static struct gl_frame_buffer_object *
new_framebuffer(GLcontext *ctx, GLuint name)
{
   struct gl_frame_buffer_object *fb = CALLOC_STRUCT(gl_frame_buffer_object);
   if (fb) {
      fb->Name = name;
      fb->RefCount = 1;
   }
   return fb;
}


static struct gl_render_buffer_attachment *
get_attachment(GLcontext *ctx, GLenum attachment)
{
   GLuint i;

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
      i = attachment - GL_COLOR_ATTACHMENT0_EXT;
      if (i >= ctx->Const.MaxColorAttachments) {
	 return NULL;
      }
      return &ctx->CurrentFramebuffer->ColorAttachment[i];
   case GL_DEPTH_ATTACHMENT_EXT:
      return &ctx->CurrentFramebuffer->DepthAttachment;
   case GL_STENCIL_ATTACHMENT_EXT:
      return &ctx->CurrentFramebuffer->StencilAttachment;
   default:
      return NULL;
   }
}


static void
remove_attachment(GLcontext *ctx, struct gl_render_buffer_attachment *att)
{
   if (att->Type == GL_TEXTURE) {
      ASSERT(att->Texture);
      ASSERT(!att->Renderbuffer);
      att->Texture->RefCount--;
      if (att->Texture->RefCount == 0) {
	 ctx->Driver.DeleteTexture(ctx, att->Texture);
      }
      att->Texture = NULL;
   }
   else if (att->Type == GL_RENDERBUFFER_EXT) {
      ASSERT(att->Renderbuffer);
      ASSERT(!att->Texture);
      att->Renderbuffer->RefCount--;
      if (att->Renderbuffer->RefCount == 0) {
	 _mesa_free(att->Renderbuffer); /* XXX driver free */
      }
      att->Renderbuffer = NULL;
   }
   att->Type = GL_NONE;
}


static void
set_texture_attachment(GLcontext *ctx,
		     struct gl_render_buffer_attachment *att,
		     struct gl_texture_object *texObj,
		     GLenum texTarget, GLuint level, GLuint zoffset)
{
   remove_attachment(ctx, att);
   att->Type = GL_TEXTURE;
   att->Texture = texObj;
   att->TextureLevel = level;
   if (IS_CUBE_FACE(texTarget)) {
      att->CubeMapFace = texTarget - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
   }
   else {
      att->CubeMapFace = 0;
   }
   att->Zoffset = zoffset;
   texObj->RefCount++;
}


static void
set_renderbuffer_attachment(GLcontext *ctx,
			  struct gl_render_buffer_attachment *att,
			  struct gl_render_buffer_object *rb)
{
   remove_attachment(ctx, att);
   att->Type = GL_RENDERBUFFER_EXT;
   att->Renderbuffer = rb;
   rb->RefCount++;
}


GLboolean GLAPIENTRY
_mesa_IsRenderbufferEXT(GLuint renderbuffer)
{
   struct gl_render_buffer_object *rb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   rb = lookup_renderbuffer(ctx, renderbuffer);
   return rb ? GL_TRUE : GL_FALSE;
}


void GLAPIENTRY
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
   struct gl_render_buffer_object *newRb, *oldRb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                  "glBindRenderbufferEXT(target)");
      return;
   }

   if (renderbuffer) {
      newRb = lookup_renderbuffer(ctx, renderbuffer);
      if (newRb == &DummyRenderbuffer) {
         /* ID was reserved, but no real renderbuffer object made yet */
         newRb = NULL;
      }
      if (!newRb) {
	 /* create new renderbuffer object */
	 newRb = new_renderbuffer(ctx, renderbuffer);
	 if (!newRb) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindRenderbufferEXT");
	    return;
	 }
         _mesa_HashInsert(ctx->Shared->RenderBuffers, renderbuffer, newRb);
      }
      newRb->RefCount++;
   }
   else {
      newRb = NULL;
   }

   oldRb = ctx->CurrentRenderbuffer;
   if (oldRb) {
      oldRb->RefCount--;
      if (oldRb->RefCount == 0) {
	 _mesa_free(oldRb);  /* XXX device driver function */
      }
   }

   ASSERT(newRb != &DummyRenderbuffer);

   ctx->CurrentRenderbuffer = newRb;
}


void GLAPIENTRY
_mesa_DeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   for (i = 0; i < n; i++) {
      if (renderbuffers[i]) {
	 struct gl_render_buffer_object *rb;
	 rb = lookup_renderbuffer(ctx, renderbuffers[i]);
	 if (rb) {
	    /* remove from hash table immediately, to free the ID */
	    _mesa_HashRemove(ctx->Shared->RenderBuffers, renderbuffers[i]);

            if (rb != &DummyRenderbuffer) {
               /* But the object will not be freed until it's no longer
                * bound in any context.
                */
               rb->RefCount--;
               if (rb->RefCount == 0) {
                  _mesa_free(rb); /* XXX call device driver function */
               }
	    }
	 }
      }
   }
}


void GLAPIENTRY
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
      GLuint name = first + i;

      /* insert dummy placeholder into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->RenderBuffers, name, &DummyRenderbuffer);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

      renderbuffers[i] = name;
   }
}


static GLenum
base_internal_format(GLcontext *ctx, GLenum internalFormat)
{
   switch (internalFormat) {
      /*case GL_ALPHA:*/
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      /*case GL_LUMINANCE:*/
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      /*case GL_LUMINANCE_ALPHA:*/
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      /*case GL_INTENSITY:*/
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      /*case GL_RGB:*/
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      /*case GL_RGBA:*/
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      case GL_STENCIL_INDEX1_EXT:
      case GL_STENCIL_INDEX4_EXT:
      case GL_STENCIL_INDEX8_EXT:
      case GL_STENCIL_INDEX16_EXT:
         return GL_STENCIL_INDEX;
      default:
         ; /* fallthrough */
   }

   if (ctx->Extensions.SGIX_depth_texture) {
      switch (internalFormat) {
         /*case GL_DEPTH_COMPONENT:*/
         case GL_DEPTH_COMPONENT16_SGIX:
         case GL_DEPTH_COMPONENT24_SGIX:
         case GL_DEPTH_COMPONENT32_SGIX:
            return GL_DEPTH_COMPONENT;
         default:
            ; /* fallthrough */
      }
   }

   return 0;
}


void GLAPIENTRY
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
                             GLsizei width, GLsizei height)
{
   GLenum baseFormat;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glRenderbufferStorageEXT(target)");
      return;
   }

   baseFormat = base_internal_format(ctx, internalFormat);
   if (baseFormat == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glRenderbufferStorageEXT(internalFormat)");
      return;
   }

   if (width < 1 || width > ctx->Const.MaxRenderbufferSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRenderbufferStorageEXT(width)");
      return;
   }

   if (height < 1 || height > ctx->Const.MaxRenderbufferSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRenderbufferStorageEXT(height)");
      return;
   }

   /* XXX this check isn't in the spec, but seems necessary */
   if (!ctx->CurrentRenderbuffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glRenderbufferStorageEXT");
      return;
   }

   if (ctx->CurrentRenderbuffer->Data) {
      /* XXX device driver free */
      _mesa_free(ctx->CurrentRenderbuffer->Data);
   }

   /* XXX device driver allocate, fix size */
   ctx->CurrentRenderbuffer->Data = _mesa_malloc(width * height * 4);
   if (ctx->CurrentRenderbuffer->Data == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glRenderbufferStorageEXT");
      return;
   }
   ctx->CurrentRenderbuffer->InternalFormat = internalFormat;
   ctx->CurrentRenderbuffer->Width = width;
   ctx->CurrentRenderbuffer->Height = height;
}


void GLAPIENTRY
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }

   if (!ctx->CurrentRenderbuffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetRenderbufferParameterivEXT");
      return;
   }

   switch (pname) {
   case GL_RENDERBUFFER_WIDTH_EXT:
      *params = ctx->CurrentRenderbuffer->Width;
      return;
   case GL_RENDERBUFFER_HEIGHT_EXT:
      *params = ctx->CurrentRenderbuffer->Height;
      return;
   case GL_RENDERBUFFER_INTERNAL_FORMAT_EXT:
      *params = ctx->CurrentRenderbuffer->InternalFormat;
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }
}


GLboolean GLAPIENTRY
_mesa_IsFramebufferEXT(GLuint framebuffer)
{
   struct gl_frame_buffer_object *fb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   fb = lookup_framebuffer(ctx, framebuffer);
   return fb ? GL_TRUE : GL_FALSE;
}


void GLAPIENTRY
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer)
{
   struct gl_frame_buffer_object *newFb, *oldFb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_FRAMEBUFFER_EXT) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                  "glBindFramebufferEXT(target)");
      return;
   }

   if (framebuffer) {
      newFb = lookup_framebuffer(ctx, framebuffer);
      if (newFb == &DummyFramebuffer) {
         newFb = NULL;
      }
      if (!newFb) {
	 /* create new framebuffer object */
	 newFb = new_framebuffer(ctx, framebuffer);
	 if (!newFb) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindFramebufferEXT");
	    return;
	 }
         _mesa_HashInsert(ctx->Shared->FrameBuffers, framebuffer, newFb);
      }
      newFb->RefCount++;
   }
   else {
      newFb = NULL;
   }

   oldFb = ctx->CurrentFramebuffer;
   if (oldFb) {
      oldFb->RefCount--;
      if (oldFb->RefCount == 0) {
	 _mesa_free(oldFb);  /* XXX device driver function */
      }
   }

   ASSERT(newFb != &DummyFramebuffer);

   ctx->CurrentFramebuffer = newFb;
}


void GLAPIENTRY
_mesa_DeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   for (i = 0; i < n; i++) {
      if (framebuffers[i]) {
	 struct gl_frame_buffer_object *fb;
	 fb = lookup_framebuffer(ctx, framebuffers[i]);
	 if (fb) {
	    /* remove from hash table immediately, to free the ID */
	    _mesa_HashRemove(ctx->Shared->FrameBuffers, framebuffers[i]);

            if (fb != &DummyFramebuffer) {
               /* But the object will not be freed until it's no longer
                * bound in any context.
                */
               fb->RefCount--;
               if (fb->RefCount == 0) {
                  _mesa_free(fb); /* XXX call device driver function */
               }
	    }
	 }
      }
   }
}


void GLAPIENTRY
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
      GLuint name = first + i;

      /* insert into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->FrameBuffers, name, &DummyFramebuffer);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

      framebuffers[i] = name;
   }
}



GLenum GLAPIENTRY
_mesa_CheckFramebufferStatusEXT(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FRAMEBUFFER_STATUS_ERROR_EXT);

   if (target != GL_FRAMEBUFFER_EXT) {
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
error_check_framebuffer_texture(GLcontext *ctx, GLuint dims,
                                GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture, GLint level)
{
   ASSERT(dims >= 1 && dims <= 3);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%dDEXT(target)", dims);
      return GL_TRUE;
   }

   if (ctx->CurrentFramebuffer == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFramebufferTexture%dDEXT", dims);
      return GL_TRUE;
   }

   /* only check textarget, level if texture ID is non-zero*/
   if (texture) {
      if ((dims == 1 && textarget != GL_TEXTURE_1D) ||
	  (dims == 3 && textarget != GL_TEXTURE_3D) ||
	  (dims == 2 && textarget != GL_TEXTURE_2D &&
	   textarget != GL_TEXTURE_RECTANGLE_ARB &&
	   !IS_CUBE_FACE(textarget))) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferTexture%dDEXT(textarget)", dims);
	 return GL_TRUE;
      }

      if ((level < 0) || level >= _mesa_max_texture_levels(ctx, textarget)) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferTexture%dDEXT(level)", dims);
	 return GL_TRUE;
      }
   }

   return GL_FALSE;
}


void GLAPIENTRY
_mesa_FramebufferTexture1DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level)
{
   struct gl_render_buffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 1, target, attachment,
				       textarget, texture, level))
      return;

   ASSERT(textarget == GL_TEXTURE_1D);

   att = get_attachment(ctx, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glFramebufferTexture1DEXT(attachment)");
      return;
   }

   if (texture) {
      struct gl_texture_object *texObj = (struct gl_texture_object *)
	 _mesa_HashLookup(ctx->Shared->TexObjects, texture);
      if (!texObj) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferTexture1DEXT(texture)");
	 return;
      }
      if (texObj->Target != textarget) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, /* XXX correct error? */
		     "glFramebufferTexture1DEXT(texture target)");
	 return;
      }
      set_texture_attachment(ctx, att, texObj, textarget, level, 0);
   }
   else {
      remove_attachment(ctx, att);
   }

   /* XXX call a driver function to signal new attachment? */
}


void GLAPIENTRY
_mesa_FramebufferTexture2DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level)
{
   struct gl_render_buffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 2, target, attachment,
				       textarget, texture, level))
      return;

   ASSERT(textarget == GL_TEXTURE_2D ||
	  textarget == GL_TEXTURE_RECTANGLE_ARB ||
	  IS_CUBE_FACE(textarget));

   att = get_attachment(ctx, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glFramebufferTexture2DEXT(attachment)");
      return;
   }

   if (texture) {
      struct gl_texture_object *texObj = (struct gl_texture_object *)
	 _mesa_HashLookup(ctx->Shared->TexObjects, texture);
      if (!texObj) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferTexture2DEXT(texture)");
	 return;
      }
      if ((texObj->Target == GL_TEXTURE_2D && textarget != GL_TEXTURE_2D) ||
	  (texObj->Target == GL_TEXTURE_RECTANGLE_ARB
	   && textarget != GL_TEXTURE_RECTANGLE_ARB) ||
	  (texObj->Target == GL_TEXTURE_CUBE_MAP
	   && !IS_CUBE_FACE(textarget))) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, /* XXX correct error? */
		     "glFramebufferTexture2DEXT(texture target)");
	 return;
      }
      set_texture_attachment(ctx, att, texObj, textarget, level, 0);
   }
   else {
      remove_attachment(ctx, att);
   }

}


void GLAPIENTRY
_mesa_FramebufferTexture3DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level, GLint zoffset)
{
   struct gl_render_buffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 3, target, attachment,
				       textarget, texture, level))
      return;

   ASSERT(textarget == GL_TEXTURE_3D);

   att = get_attachment(ctx, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glFramebufferTexture1DEXT(attachment)");
      return;
   }

   if (texture) {
      struct gl_texture_object *texObj = (struct gl_texture_object *)
	 _mesa_HashLookup(ctx->Shared->TexObjects, texture);
      if (!texObj) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferTexture3DEXT(texture)");
	 return;
      }
      if (texObj->Target != textarget) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, /* XXX correct error? */
		     "glFramebufferTexture3DEXT(texture target)");
	 return;
      }
      if (zoffset >= texObj->Image[0][level]->Depth) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferTexture3DEXT(zoffset)");
	 return;
      }
      set_texture_attachment(ctx, att, texObj, textarget, level, zoffset);
   }
   else {
      remove_attachment(ctx, att);
   }
}


void GLAPIENTRY
_mesa_FramebufferRenderbufferEXT(GLenum target, GLenum attachment,
                                 GLenum renderbufferTarget,
                                 GLuint renderbuffer)
{
   struct gl_render_buffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(target)");
      return;
   }

   if (renderbufferTarget != GL_RENDERBUFFER_EXT) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                 "glFramebufferRenderbufferEXT(renderbufferTarget)");
      return;
   }

   if (ctx->CurrentFramebuffer == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glFramebufferRenderbufferEXT");
      return;
   }

   att = get_attachment(ctx, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                 "glFramebufferRenderbufferEXT(attachment)");
      return;
   }

   if (renderbuffer) {
      struct gl_render_buffer_object *rb;
      rb = lookup_renderbuffer(ctx, renderbuffer);
      if (!rb) {
	 _mesa_error(ctx, GL_INVALID_VALUE,
		     "glFramebufferRenderbufferEXT(renderbuffer)");
	 return;
      }
      set_renderbuffer_attachment(ctx, att, rb);
   }
   else {
      remove_attachment(ctx, att);
   }
}


void GLAPIENTRY
_mesa_GetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment,
                                             GLenum pname, GLint *params)
{
   const struct gl_render_buffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(target)");
      return;
   }

   if (ctx->CurrentFramebuffer == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFramebufferAttachmentParameterivEXT");
      return;
   }

   att = get_attachment(ctx, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(attachment)");
      return;
   }

   switch (pname) {
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT:
      *params = att->Type;
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT:
      if (att->Type == GL_RENDERBUFFER_EXT) {
	 *params = att->Renderbuffer->Name;
      }
      else if (att->Type == GL_TEXTURE) {
	 *params = att->Texture->Name;
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT:
      if (att->Type == GL_TEXTURE) {
	 *params = att->TextureLevel;
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT:
      if (att->Type == GL_TEXTURE) {
	 *params = GL_TEXTURE_CUBE_MAP_POSITIVE_X + att->CubeMapFace;
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT:
      if (att->Type == GL_TEXTURE) {
	 *params = att->Zoffset;
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(pname)");
      return;
   }
}


void GLAPIENTRY
_mesa_GenerateMipmapEXT(GLenum target)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGenerateMipmapEXT(target)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   /* XXX this might not handle cube maps correctly */
   _mesa_generate_mipmap(ctx, target, texUnit, texObj);
}
