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


/**
 * Notes:
 *
 * None of the GL_EXT_framebuffer_object functions are compiled into
 * display lists.
 */



/*
 * When glGenRender/FramebuffersEXT() is called we insert pointers to
 * these placeholder objects into the hash table.
 * Later, when the object ID is first bound, we replace the placeholder
 * with the real frame/renderbuffer.
 */
static struct gl_framebuffer DummyFramebuffer;
static struct gl_renderbuffer DummyRenderbuffer;


#define IS_CUBE_FACE(TARGET) \
   ((TARGET) >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && \
    (TARGET) <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)


/**
 * Helper routine for getting a gl_renderbuffer.
 */
static struct gl_renderbuffer *
lookup_renderbuffer(GLcontext *ctx, GLuint id)
{
   struct gl_renderbuffer *rb;

   if (id == 0)
      return NULL;

   rb = (struct gl_renderbuffer *)
      _mesa_HashLookup(ctx->Shared->RenderBuffers, id);
   return rb;
}


/**
 * Helper routine for getting a gl_framebuffer.
 */
static struct gl_framebuffer *
lookup_framebuffer(GLcontext *ctx, GLuint id)
{
   struct gl_framebuffer *fb;

   if (id == 0)
      return NULL;

   fb = (struct gl_framebuffer *)
      _mesa_HashLookup(ctx->Shared->FrameBuffers, id);
   return fb;
}


/**
 * Allocate a new gl_framebuffer.
 * This is the default function for ctx->Driver.NewFramebuffer().
 */
struct gl_framebuffer *
_mesa_new_framebuffer(GLcontext *ctx, GLuint name)
{
   struct gl_framebuffer *fb = CALLOC_STRUCT(gl_framebuffer);
   if (fb) {
      fb->Name = name;
      fb->RefCount = 1;
      fb->Delete = _mesa_delete_framebuffer;
   }
   return fb;
}


/**
 * Delete a gl_framebuffer.
 * This is the default function for framebuffer->Delete().
 */
void
_mesa_delete_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb)
{
   (void) ctx;
   _mesa_free(fb);
}


/**
 * Allocate the actual storage for a renderbuffer with the given format
 * and dimensions.
 * This is the default function for gl_renderbuffer->AllocStorage().
 * All incoming parameters will have already been checked for errors.
 * If memory allocate fails, the function must call
 * _mesa_error(GL_OUT_OF_MEMORY) and then return.
 * \return GL_TRUE for success, GL_FALSE for failure.
 */
static GLboolean
alloc_renderbuffer_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   /* First, free any existing storage */
   if (rb->Data) {
      _mesa_free(rb->Data);
   }

   /* Now, allocate new storage */
   rb->Data = _mesa_malloc(width * height * 4); /* XXX fix size! */
   if (rb->Data == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glRenderbufferStorageEXT");
      return GL_FALSE;
   }

   /* We set these fields now if the allocation worked */
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return GL_TRUE;
}


/**
 * Allocate a new gl_renderbuffer.
 * This is the default function for ctx->Driver.NewRenderbuffer().
 */
struct gl_renderbuffer *
_mesa_new_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct gl_renderbuffer *rb = CALLOC_STRUCT(gl_renderbuffer);
   if (rb) {
      rb->Name = name;
      rb->RefCount = 1;
      rb->Delete = _mesa_delete_renderbuffer;
      rb->AllocStorage = alloc_renderbuffer_storage;
      /* other fields are zero */
   }
   return rb;
}


/**
 * Delete a gl_framebuffer.
 * This is the default function for framebuffer->Delete().
 */
void
_mesa_delete_renderbuffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   (void) ctx;
   if (rb->Data) {
      _mesa_free(rb->Data);
   }
   _mesa_free(rb);
}


/**
 * Given a GL_*_ATTACHMENTn token, return a pointer to the corresponding
 * gl_renderbuffer_attachment object.
 */
static struct gl_renderbuffer_attachment *
get_attachment(GLcontext *ctx, struct gl_framebuffer *fb, GLenum attachment)
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
      return &fb->ColorAttachment[i];
   case GL_DEPTH_ATTACHMENT_EXT:
      return &fb->DepthAttachment;
   case GL_STENCIL_ATTACHMENT_EXT:
      return &fb->StencilAttachment;
   default:
      return NULL;
   }
}


/**
 * Remove any texture or renderbuffer attached to the given attachment
 * point.  Update reference counts, etc.
 */
static void
remove_attachment(GLcontext *ctx, struct gl_renderbuffer_attachment *att)
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
         att->Renderbuffer->Delete(ctx, att->Renderbuffer);
      }
      att->Renderbuffer = NULL;
   }
   att->Type = GL_NONE;
   att->Complete = GL_TRUE;
}


/**
 * Bind a texture object to an attachment point.
 * The previous binding, if any, will be removed first.
 */
static void
set_texture_attachment(GLcontext *ctx,
		     struct gl_renderbuffer_attachment *att,
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
   att->Complete = GL_FALSE;
   texObj->RefCount++;

   /* XXX when we attach to a texture, we should probably set the
    * att->Renderbuffer pointer to a "wrapper renderbuffer" which
    * makes the texture image look like renderbuffer.
    */
}


/**
 * Bind a renderbuffer to an attachment point.
 * The previous binding, if any, will be removed first.
 */
static void
set_renderbuffer_attachment(GLcontext *ctx,
                            struct gl_renderbuffer_attachment *att,
                            struct gl_renderbuffer *rb)
{
   remove_attachment(ctx, att);
   att->Type = GL_RENDERBUFFER_EXT;
   att->Renderbuffer = rb;
   att->Texture = NULL; /* just to be safe */
   att->Complete = GL_FALSE;
   rb->RefCount++;
}


/**
 * Test if an attachment point is complete and update its Complete field.
 * \param format if GL_COLOR, this is a color attachment point,
 *               if GL_DEPTH, this is a depth component attachment point,
 *               if GL_STENCIL, this is a stencil component attachment point.
 */
static void
test_attachment_completeness(const GLcontext *ctx, GLenum format,
                             struct gl_renderbuffer_attachment *att)
{
   assert(format == GL_COLOR || format == GL_DEPTH || format == GL_STENCIL);

   /* assume complete */
   att->Complete = GL_TRUE;

   if (att->Type == GL_NONE)
      return; /* complete */

   /* Look for reasons why the attachment might be incomplete */
   if (att->Type == GL_TEXTURE) {
      struct gl_texture_object *texObj = att->Texture;
      struct gl_texture_image *texImage;

      assert(texObj);

      texImage = texObj->Image[att->CubeMapFace][att->TextureLevel];
      if (!texImage) {
         att->Complete = GL_FALSE;
         return;
      }
      if (texImage->Width < 1 || texImage->Height < 1) {
         att->Complete = GL_FALSE;
         return;
      }
      if (texObj->Target == GL_TEXTURE_3D && att->Zoffset >= texImage->Depth) {
         att->Complete = GL_FALSE;
         return;
      }

      if (format == GL_COLOR) {
         if (texImage->Format != GL_RGB && texImage->Format != GL_RGBA) {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else if (format == GL_DEPTH) {
         if (texImage->Format != GL_DEPTH_COMPONENT) {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else {
         /* no such thing as stencil textures */
         att->Complete = GL_FALSE;
         return;
      }
   }
   else {
      assert(att->Type == GL_RENDERBUFFER_EXT);

      if (att->Renderbuffer->Width < 1 || att->Renderbuffer->Height < 1) {
         att->Complete = GL_FALSE;
         return;
      }
      if (format == GL_COLOR) {
         if (att->Renderbuffer->_BaseFormat != GL_RGB &&
             att->Renderbuffer->_BaseFormat != GL_RGBA) {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else if (format == GL_DEPTH) {
         if (att->Renderbuffer->_BaseFormat != GL_DEPTH_COMPONENT) {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else {
         assert(format == GL_STENCIL);
         if (att->Renderbuffer->_BaseFormat != GL_STENCIL_INDEX) {
            att->Complete = GL_FALSE;
            return;
         }
      }
   }
}


/**
 * Test if the given framebuffer object is complete and update its
 * Status field with the results.
 */
static void
test_framebuffer_completeness(GLcontext *ctx,
                              struct gl_framebuffer *fb)
{
   GLint i;
   GLuint numImages, width, height;
   GLenum intFormat;

   /* Set to COMPLETE status, then try to find reasons for being incomplete */
   fb->Status = GL_FRAMEBUFFER_COMPLETE_EXT;

   numImages = 0;

   /* Start at -2 to more easily loop over all attachment points */
   for (i = -2; i < ctx->Const.MaxColorAttachments; i++) {
      struct gl_renderbuffer_attachment *att;
      GLuint w, h;
      GLenum f;

      if (i == -2) {
         att = &fb->DepthAttachment;
         test_attachment_completeness(ctx, GL_DEPTH, att);
         if (!att->Complete) {
            fb->Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            return;
         }
      }
      else if (i == -1) {
         att = &fb->StencilAttachment;
         test_attachment_completeness(ctx, GL_STENCIL, att);
         if (!att->Complete) {
            fb->Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            return;
         }
      }
      else {
         att = &fb->ColorAttachment[i];
         test_attachment_completeness(ctx, GL_COLOR, att);
         if (!att->Complete) {
            fb->Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            return;
         }
      }

      if (att->Type == GL_TEXTURE) {
         w = att->Texture->Image[att->CubeMapFace][att->TextureLevel]->Width;
         h = att->Texture->Image[att->CubeMapFace][att->TextureLevel]->Height;
         f = att->Texture->Image[att->CubeMapFace][att->TextureLevel]->Format;
         numImages++;
      }
      else if (att->Type == GL_RENDERBUFFER_EXT) {
         w = att->Renderbuffer->Width;
         h = att->Renderbuffer->Height;
         f = att->Renderbuffer->InternalFormat;
         numImages++;
      }
      else {
         assert(att->Type == GL_NONE);
         continue;
      }

      if (numImages == 1) {
         /* set required width, height and format */
         width = w;
         height = h;
         if (i >= 0)
            intFormat = f;
      }
      else {
         /* check that width, height, format are same */
         if (w != width || h != height) {
            fb->Status = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT;
            return;
         }
         if (i >= 0 && f != intFormat) {
            fb->Status = GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT;
            return;
         }

      }
   }

   /* Check that all DrawBuffers are present */
   for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      if (fb->DrawBuffer[i] != GL_NONE) {
         struct gl_renderbuffer_attachment *att
            = get_attachment(ctx, fb, fb->DrawBuffer[i]);
         if (att->Type == GL_NONE) {
            fb->Status = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT;
            return;
         }
      }
   }

   /* Check that the ReadBuffer is present */
   if (fb->ReadBuffer != GL_NONE) {
      struct gl_renderbuffer_attachment *att
         = get_attachment(ctx, fb, fb->ReadBuffer);
      if (att->Type == GL_NONE) {
         fb->Status = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT;
         return;
      }
   }

   if (numImages == 0) {
      fb->Status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT;
      return;
   }
}



GLboolean GLAPIENTRY
_mesa_IsRenderbufferEXT(GLuint renderbuffer)
{
   const struct gl_renderbuffer *rb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   rb = lookup_renderbuffer(ctx, renderbuffer);
   return rb ? GL_TRUE : GL_FALSE;
}


void GLAPIENTRY
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
   struct gl_renderbuffer *newRb, *oldRb;
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
	 newRb = ctx->Driver.NewRenderbuffer(ctx, renderbuffer);
	 if (!newRb) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindRenderbufferEXT");
	    return;
	 }
         ASSERT(newRb->AllocStorage);
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
         oldRb->Delete(ctx, oldRb);
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
      if (renderbuffers[i] > 0) {
	 struct gl_renderbuffer *rb;
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
                  rb->Delete(ctx, rb);
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
      renderbuffers[i] = name;
      /* insert dummy placeholder into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->RenderBuffers, name, &DummyRenderbuffer);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   }
}


/**
 * Given an internal format token for a render buffer, return the
 * corresponding base format.
 * \return one of GL_RGB, GL_RGBA, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT
 *  or zero if error.
 */
static GLenum
base_internal_format(GLcontext *ctx, GLenum internalFormat)
{
   switch (internalFormat) {
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return GL_RGB;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return GL_RGBA;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      return GL_STENCIL_INDEX;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      return GL_DEPTH_COMPONENT;
   /* XXX add floating point formats eventually */
   default:
      return 0;
   }
}


void GLAPIENTRY
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
                             GLsizei width, GLsizei height)
{
   struct gl_renderbuffer *rb;
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

   rb = ctx->CurrentRenderbuffer;

   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glRenderbufferStorageEXT");
      return;
   }

   /* Now allocate the storage */
   ASSERT(rb->AllocStorage);
   if (rb->AllocStorage(ctx, rb, internalFormat, width, height)) {
      /* No error - check/set fields now */
      assert(rb->Width == width);
      assert(rb->Height == height);
      assert(rb->InternalFormat);
      rb->_BaseFormat = baseFormat;
   }
   else {
      /* Probably ran out of memory - clear the fields */
      rb->Width = 0;
      rb->Height = 0;
      rb->InternalFormat = GL_NONE;
      rb->_BaseFormat = GL_NONE;
   }

   /* XXX if this renderbuffer is attached anywhere, invalidate attachment
    * points???
    */
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
   const struct gl_framebuffer *fb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   fb = lookup_framebuffer(ctx, framebuffer);
   return fb ? GL_TRUE : GL_FALSE;
}


void GLAPIENTRY
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer)
{
   struct gl_framebuffer *newFb, *oldFb;
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
         /* ID was reserved, but no real framebuffer object made yet */
         newFb = NULL;
      }
      if (!newFb) {
	 /* create new framebuffer object */
	 newFb = ctx->Driver.NewFramebuffer(ctx, framebuffer);
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
         oldFb->Delete(ctx, oldFb);
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
      if (framebuffers[i] > 0) {
	 struct gl_framebuffer *fb;
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
                  fb->Delete(ctx, fb);
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
      framebuffers[i] = name;
      /* insert dummy placeholder into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->FrameBuffers, name, &DummyFramebuffer);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   }
}



GLenum GLAPIENTRY
_mesa_CheckFramebufferStatusEXT(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FRAMEBUFFER_STATUS_ERROR_EXT);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCheckFramebufferStatus(target)");
      return GL_FRAMEBUFFER_STATUS_ERROR_EXT;
   }

   if (!ctx->CurrentFramebuffer) {
      /* The window system / default framebuffer is always complete */
      return GL_FRAMEBUFFER_COMPLETE_EXT;
   }

   test_framebuffer_completeness(ctx, ctx->CurrentFramebuffer);
   return ctx->CurrentFramebuffer->Status;
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
   struct gl_renderbuffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 1, target, attachment,
				       textarget, texture, level))
      return;

   ASSERT(textarget == GL_TEXTURE_1D);

   att = get_attachment(ctx, ctx->CurrentFramebuffer, attachment);
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
   struct gl_renderbuffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 2, target, attachment,
				       textarget, texture, level))
      return;

   ASSERT(textarget == GL_TEXTURE_2D ||
	  textarget == GL_TEXTURE_RECTANGLE_ARB ||
	  IS_CUBE_FACE(textarget));

   att = get_attachment(ctx, ctx->CurrentFramebuffer, attachment);
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
   struct gl_renderbuffer_attachment *att;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (error_check_framebuffer_texture(ctx, 3, target, attachment,
				       textarget, texture, level))
      return;

   ASSERT(textarget == GL_TEXTURE_3D);

   att = get_attachment(ctx, ctx->CurrentFramebuffer, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glFramebufferTexture1DEXT(attachment)");
      return;
   }

   if (texture) {
      const GLint maxSize = 1 << (ctx->Const.Max3DTextureLevels - 1);
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
      if (zoffset < 0 || zoffset >= maxSize) {
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
   struct gl_renderbuffer_attachment *att;
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

   att = get_attachment(ctx, ctx->CurrentFramebuffer, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                 "glFramebufferRenderbufferEXT(attachment)");
      return;
   }

   if (renderbuffer) {
      struct gl_renderbuffer *rb;
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
   const struct gl_renderbuffer_attachment *att;
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

   att = get_attachment(ctx, ctx->CurrentFramebuffer, attachment);
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
