/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 1999-2009  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * GL_EXT/ARB_framebuffer_object extensions
 *
 * Authors:
 *   Brian Paul
 */

#include <stdbool.h>

#include "buffers.h"
#include "context.h"
#include "enums.h"
#include "fbobject.h"
#include "formats.h"
#include "framebuffer.h"
#include "glformats.h"
#include "hash.h"
#include "macros.h"
#include "multisample.h"
#include "mtypes.h"
#include "renderbuffer.h"
#include "state.h"
#include "teximage.h"
#include "texobj.h"


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

/* We bind this framebuffer when applications pass a NULL
 * drawable/surface in make current. */
static struct gl_framebuffer IncompleteFramebuffer;


static void
delete_dummy_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   /* no op */
}

static void
delete_dummy_framebuffer(struct gl_framebuffer *fb)
{
   /* no op */
}


void
_mesa_init_fbobjects(struct gl_context *ctx)
{
   mtx_init(&DummyFramebuffer.Mutex, mtx_plain);
   mtx_init(&DummyRenderbuffer.Mutex, mtx_plain);
   mtx_init(&IncompleteFramebuffer.Mutex, mtx_plain);
   DummyFramebuffer.Delete = delete_dummy_framebuffer;
   DummyRenderbuffer.Delete = delete_dummy_renderbuffer;
   IncompleteFramebuffer.Delete = delete_dummy_framebuffer;
}

struct gl_framebuffer *
_mesa_get_incomplete_framebuffer(void)
{
   return &IncompleteFramebuffer;
}

/**
 * Helper routine for getting a gl_renderbuffer.
 */
struct gl_renderbuffer *
_mesa_lookup_renderbuffer(struct gl_context *ctx, GLuint id)
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
struct gl_framebuffer *
_mesa_lookup_framebuffer(struct gl_context *ctx, GLuint id)
{
   struct gl_framebuffer *fb;

   if (id == 0)
      return NULL;

   fb = (struct gl_framebuffer *)
      _mesa_HashLookup(ctx->Shared->FrameBuffers, id);
   return fb;
}


/**
 * Mark the given framebuffer as invalid.  This will force the
 * test for framebuffer completeness to be done before the framebuffer
 * is used.
 */
static void
invalidate_framebuffer(struct gl_framebuffer *fb)
{
   fb->_Status = 0; /* "indeterminate" */
}


/**
 * Return the gl_framebuffer object which corresponds to the given
 * framebuffer target, such as GL_DRAW_FRAMEBUFFER.
 * Check support for GL_EXT_framebuffer_blit to determine if certain
 * targets are legal.
 * \return gl_framebuffer pointer or NULL if target is illegal
 */
static struct gl_framebuffer *
get_framebuffer_target(struct gl_context *ctx, GLenum target)
{
   bool have_fb_blit = _mesa_is_gles3(ctx) || _mesa_is_desktop_gl(ctx);
   switch (target) {
   case GL_DRAW_FRAMEBUFFER:
      return have_fb_blit ? ctx->DrawBuffer : NULL;
   case GL_READ_FRAMEBUFFER:
      return have_fb_blit ? ctx->ReadBuffer : NULL;
   case GL_FRAMEBUFFER_EXT:
      return ctx->DrawBuffer;
   default:
      return NULL;
   }
}


/**
 * Given a GL_*_ATTACHMENTn token, return a pointer to the corresponding
 * gl_renderbuffer_attachment object.
 * This function is only used for user-created FB objects, not the
 * default / window-system FB object.
 * If \p attachment is GL_DEPTH_STENCIL_ATTACHMENT, return a pointer to
 * the depth buffer attachment point.
 */
static struct gl_renderbuffer_attachment *
get_attachment(struct gl_context *ctx, struct gl_framebuffer *fb,
               GLenum attachment)
{
   GLuint i;

   assert(_mesa_is_user_fbo(fb));

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
      /* Only OpenGL ES 1.x forbids color attachments other than
       * GL_COLOR_ATTACHMENT0.  For all other APIs the limit set by the
       * hardware is used.
       */
      i = attachment - GL_COLOR_ATTACHMENT0_EXT;
      if (i >= ctx->Const.MaxColorAttachments
	  || (i > 0 && ctx->API == API_OPENGLES)) {
	 return NULL;
      }
      return &fb->Attachment[BUFFER_COLOR0 + i];
   case GL_DEPTH_STENCIL_ATTACHMENT:
      if (!_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
	 return NULL;
      /* fall-through */
   case GL_DEPTH_ATTACHMENT_EXT:
      return &fb->Attachment[BUFFER_DEPTH];
   case GL_STENCIL_ATTACHMENT_EXT:
      return &fb->Attachment[BUFFER_STENCIL];
   default:
      return NULL;
   }
}


/**
 * As above, but only used for getting attachments of the default /
 * window-system framebuffer (not user-created framebuffer objects).
 */
static struct gl_renderbuffer_attachment *
_mesa_get_fb0_attachment(struct gl_context *ctx, struct gl_framebuffer *fb,
                         GLenum attachment)
{
   assert(_mesa_is_winsys_fbo(fb));

   if (_mesa_is_gles3(ctx)) {
      assert(attachment == GL_BACK ||
             attachment == GL_DEPTH ||
             attachment == GL_STENCIL);
      switch (attachment) {
      case GL_BACK:
         /* Since there is no stereo rendering in ES 3.0, only return the
          * LEFT bits.
          */
         if (ctx->DrawBuffer->Visual.doubleBufferMode)
            return &fb->Attachment[BUFFER_BACK_LEFT];
         return &fb->Attachment[BUFFER_FRONT_LEFT];
      case GL_DEPTH:
      return &fb->Attachment[BUFFER_DEPTH];
      case GL_STENCIL:
         return &fb->Attachment[BUFFER_STENCIL];
      }
   }

   switch (attachment) {
   case GL_FRONT_LEFT:
      return &fb->Attachment[BUFFER_FRONT_LEFT];
   case GL_FRONT_RIGHT:
      return &fb->Attachment[BUFFER_FRONT_RIGHT];
   case GL_BACK_LEFT:
      return &fb->Attachment[BUFFER_BACK_LEFT];
   case GL_BACK_RIGHT:
      return &fb->Attachment[BUFFER_BACK_RIGHT];
   case GL_AUX0:
      if (fb->Visual.numAuxBuffers == 1) {
         return &fb->Attachment[BUFFER_AUX0];
      }
      return NULL;

   /* Page 336 (page 352 of the PDF) of the OpenGL 3.0 spec says:
    *
    *     "If the default framebuffer is bound to target, then attachment must
    *     be one of FRONT LEFT, FRONT RIGHT, BACK LEFT, BACK RIGHT, or AUXi,
    *     identifying a color buffer; DEPTH, identifying the depth buffer; or
    *     STENCIL, identifying the stencil buffer."
    *
    * Revision #34 of the ARB_framebuffer_object spec has essentially the same
    * language.  However, revision #33 of the ARB_framebuffer_object spec
    * says:
    *
    *     "If the default framebuffer is bound to <target>, then <attachment>
    *     must be one of FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT, AUXi,
    *     DEPTH_BUFFER, or STENCIL_BUFFER, identifying a color buffer, the
    *     depth buffer, or the stencil buffer, and <pname> may be
    *     FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE or
    *     FRAMEBUFFER_ATTACHMENT_OBJECT_NAME."
    *
    * The enum values for DEPTH_BUFFER and STENCIL_BUFFER have been removed
    * from glext.h, so shipping apps should not use those values.
    *
    * Note that neither EXT_framebuffer_object nor OES_framebuffer_object
    * support queries of the window system FBO.
    */
   case GL_DEPTH:
      return &fb->Attachment[BUFFER_DEPTH];
   case GL_STENCIL:
      return &fb->Attachment[BUFFER_STENCIL];
   default:
      return NULL;
   }
}



/**
 * Remove any texture or renderbuffer attached to the given attachment
 * point.  Update reference counts, etc.
 */
static void
remove_attachment(struct gl_context *ctx,
                  struct gl_renderbuffer_attachment *att)
{
   struct gl_renderbuffer *rb = att->Renderbuffer;

   /* tell driver that we're done rendering to this texture. */
   if (rb && rb->NeedsFinishRenderTexture)
      ctx->Driver.FinishRenderTexture(ctx, rb);

   if (att->Type == GL_TEXTURE) {
      ASSERT(att->Texture);
      _mesa_reference_texobj(&att->Texture, NULL); /* unbind */
      ASSERT(!att->Texture);
   }
   if (att->Type == GL_TEXTURE || att->Type == GL_RENDERBUFFER_EXT) {
      ASSERT(!att->Texture);
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL); /* unbind */
      ASSERT(!att->Renderbuffer);
   }
   att->Type = GL_NONE;
   att->Complete = GL_TRUE;
}

/**
 * Verify a couple error conditions that will lead to an incomplete FBO and
 * may cause problems for the driver's RenderTexture path.
 */
static bool
driver_RenderTexture_is_safe(const struct gl_renderbuffer_attachment *att)
{
   const struct gl_texture_image *const texImage =
      att->Texture->Image[att->CubeMapFace][att->TextureLevel];

   if (texImage->Width == 0 || texImage->Height == 0 || texImage->Depth == 0)
      return false;

   if ((texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY
        && att->Zoffset >= texImage->Height)
       || (texImage->TexObject->Target != GL_TEXTURE_1D_ARRAY
           && att->Zoffset >= texImage->Depth))
      return false;

   return true;
}

/**
 * Create a renderbuffer which will be set up by the driver to wrap the
 * texture image slice.
 *
 * By using a gl_renderbuffer (like user-allocated renderbuffers), drivers get
 * to share most of their framebuffer rendering code between winsys,
 * renderbuffer, and texture attachments.
 *
 * The allocated renderbuffer uses a non-zero Name so that drivers can check
 * it for determining vertical orientation, but we use ~0 to make it fairly
 * unambiguous with actual user (non-texture) renderbuffers.
 */
void
_mesa_update_texture_renderbuffer(struct gl_context *ctx,
                                  struct gl_framebuffer *fb,
                                  struct gl_renderbuffer_attachment *att)
{
   struct gl_texture_image *texImage;
   struct gl_renderbuffer *rb;

   texImage = att->Texture->Image[att->CubeMapFace][att->TextureLevel];

   rb = att->Renderbuffer;
   if (!rb) {
      rb = ctx->Driver.NewRenderbuffer(ctx, ~0);
      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture()");
         return;
      }
      _mesa_reference_renderbuffer(&att->Renderbuffer, rb);

      /* This can't get called on a texture renderbuffer, so set it to NULL
       * for clarity compared to user renderbuffers.
       */
      rb->AllocStorage = NULL;

      rb->NeedsFinishRenderTexture = ctx->Driver.FinishRenderTexture != NULL;
   }

   if (!texImage)
      return;

   rb->_BaseFormat = texImage->_BaseFormat;
   rb->Format = texImage->TexFormat;
   rb->InternalFormat = texImage->InternalFormat;
   rb->Width = texImage->Width2;
   rb->Height = texImage->Height2;
   rb->Depth = texImage->Depth2;
   rb->NumSamples = texImage->NumSamples;
   rb->TexImage = texImage;

   if (driver_RenderTexture_is_safe(att))
      ctx->Driver.RenderTexture(ctx, fb, att);
}

/**
 * Bind a texture object to an attachment point.
 * The previous binding, if any, will be removed first.
 */
static void
set_texture_attachment(struct gl_context *ctx,
                       struct gl_framebuffer *fb,
                       struct gl_renderbuffer_attachment *att,
                       struct gl_texture_object *texObj,
                       GLenum texTarget, GLuint level, GLuint zoffset,
                       GLboolean layered)
{
   struct gl_renderbuffer *rb = att->Renderbuffer;

   if (rb && rb->NeedsFinishRenderTexture)
      ctx->Driver.FinishRenderTexture(ctx, rb);

   if (att->Texture == texObj) {
      /* re-attaching same texture */
      ASSERT(att->Type == GL_TEXTURE);
   }
   else {
      /* new attachment */
      remove_attachment(ctx, att);
      att->Type = GL_TEXTURE;
      assert(!att->Texture);
      _mesa_reference_texobj(&att->Texture, texObj);
   }
   invalidate_framebuffer(fb);

   /* always update these fields */
   att->TextureLevel = level;
   att->CubeMapFace = _mesa_tex_target_to_face(texTarget);
   att->Zoffset = zoffset;
   att->Layered = layered;
   att->Complete = GL_FALSE;

   _mesa_update_texture_renderbuffer(ctx, fb, att);
}


/**
 * Bind a renderbuffer to an attachment point.
 * The previous binding, if any, will be removed first.
 */
static void
set_renderbuffer_attachment(struct gl_context *ctx,
                            struct gl_renderbuffer_attachment *att,
                            struct gl_renderbuffer *rb)
{
   /* XXX check if re-doing same attachment, exit early */
   remove_attachment(ctx, att);
   att->Type = GL_RENDERBUFFER_EXT;
   att->Texture = NULL; /* just to be safe */
   att->Complete = GL_FALSE;
   _mesa_reference_renderbuffer(&att->Renderbuffer, rb);
}


/**
 * Fallback for ctx->Driver.FramebufferRenderbuffer()
 * Attach a renderbuffer object to a framebuffer object.
 */
void
_mesa_framebuffer_renderbuffer(struct gl_context *ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{
   struct gl_renderbuffer_attachment *att;

   mtx_lock(&fb->Mutex);

   att = get_attachment(ctx, fb, attachment);
   ASSERT(att);
   if (rb) {
      set_renderbuffer_attachment(ctx, att, rb);
      if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
         /* do stencil attachment here (depth already done above) */
         att = get_attachment(ctx, fb, GL_STENCIL_ATTACHMENT_EXT);
         assert(att);
         set_renderbuffer_attachment(ctx, att, rb);
      }
      rb->AttachedAnytime = GL_TRUE;
   }
   else {
      remove_attachment(ctx, att);
      if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
         /* detach stencil (depth was detached above) */
         att = get_attachment(ctx, fb, GL_STENCIL_ATTACHMENT_EXT);
         assert(att);
         remove_attachment(ctx, att);
      }
   }

   invalidate_framebuffer(fb);

   mtx_unlock(&fb->Mutex);
}


/**
 * Fallback for ctx->Driver.ValidateFramebuffer()
 * Check if the renderbuffer's formats are supported by the software
 * renderer.
 * Drivers should probably override this.
 */
void
_mesa_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   gl_buffer_index buf;
   for (buf = 0; buf < BUFFER_COUNT; buf++) {
      const struct gl_renderbuffer *rb = fb->Attachment[buf].Renderbuffer;
      if (rb) {
         switch (rb->_BaseFormat) {
         case GL_ALPHA:
         case GL_LUMINANCE_ALPHA:
         case GL_LUMINANCE:
         case GL_INTENSITY:
         case GL_RED:
         case GL_RG:
            fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED;
            return;

         default:
            switch (rb->Format) {
            /* XXX This list is likely incomplete. */
            case MESA_FORMAT_R9G9B9E5_FLOAT:
               fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED;
               return;
            default:;
               /* render buffer format is supported by software rendering */
            }
         }
      }
   }
}


/**
 * Return true if the framebuffer has a combined depth/stencil
 * renderbuffer attached.
 */
GLboolean
_mesa_has_depthstencil_combined(const struct gl_framebuffer *fb)
{
   const struct gl_renderbuffer_attachment *depth =
         &fb->Attachment[BUFFER_DEPTH];
   const struct gl_renderbuffer_attachment *stencil =
         &fb->Attachment[BUFFER_STENCIL];

   if (depth->Type == stencil->Type) {
      if (depth->Type == GL_RENDERBUFFER_EXT &&
          depth->Renderbuffer == stencil->Renderbuffer)
         return GL_TRUE;

      if (depth->Type == GL_TEXTURE &&
          depth->Texture == stencil->Texture)
         return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * For debug only.
 */
static void
att_incomplete(const char *msg)
{
   if (MESA_DEBUG_FLAGS & DEBUG_INCOMPLETE_FBO) {
      _mesa_debug(NULL, "attachment incomplete: %s\n", msg);
   }
}


/**
 * For debug only.
 */
static void
fbo_incomplete(struct gl_context *ctx, const char *msg, int index)
{
   static GLuint msg_id;

   _mesa_gl_debug(ctx, &msg_id,
                  MESA_DEBUG_TYPE_OTHER,
                  MESA_DEBUG_SEVERITY_MEDIUM,
                  "FBO incomplete: %s [%d]\n", msg, index);

   if (MESA_DEBUG_FLAGS & DEBUG_INCOMPLETE_FBO) {
      _mesa_debug(NULL, "FBO Incomplete: %s [%d]\n", msg, index);
   }
}


/**
 * Is the given base format a legal format for a color renderbuffer?
 */
GLboolean
_mesa_is_legal_color_format(const struct gl_context *ctx, GLenum baseFormat)
{
   switch (baseFormat) {
   case GL_RGB:
   case GL_RGBA:
      return GL_TRUE;
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_ALPHA:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_framebuffer_object;
   case GL_RED:
   case GL_RG:
      return ctx->Extensions.ARB_texture_rg;
   default:
      return GL_FALSE;
   }
}


/**
 * Is the given base format a legal format for a color renderbuffer?
 */
static GLboolean
is_format_color_renderable(const struct gl_context *ctx, mesa_format format,
                           GLenum internalFormat)
{
   const GLenum baseFormat =
      _mesa_get_format_base_format(format);
   GLboolean valid;

   valid = _mesa_is_legal_color_format(ctx, baseFormat);
   if (!valid || _mesa_is_desktop_gl(ctx)) {
      return valid;
   }

   /* Reject additional cases for GLES */
   switch (internalFormat) {
   case GL_RGBA8_SNORM:
   case GL_RGB32F:
   case GL_RGB32I:
   case GL_RGB32UI:
   case GL_RGB16F:
   case GL_RGB16I:
   case GL_RGB16UI:
   case GL_RGB8_SNORM:
   case GL_RGB8I:
   case GL_RGB8UI:
   case GL_SRGB8:
   case GL_RGB9_E5:
   case GL_RG8_SNORM:
   case GL_R8_SNORM:
      return GL_FALSE;
   default:
      break;
   }

   if (format == MESA_FORMAT_B10G10R10A2_UNORM &&
       internalFormat != GL_RGB10_A2) {
      return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Is the given base format a legal format for a depth/stencil renderbuffer?
 */
static GLboolean
is_legal_depth_format(const struct gl_context *ctx, GLenum baseFormat)
{
   switch (baseFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL_EXT:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Test if an attachment point is complete and update its Complete field.
 * \param format if GL_COLOR, this is a color attachment point,
 *               if GL_DEPTH, this is a depth component attachment point,
 *               if GL_STENCIL, this is a stencil component attachment point.
 */
static void
test_attachment_completeness(const struct gl_context *ctx, GLenum format,
                             struct gl_renderbuffer_attachment *att)
{
   assert(format == GL_COLOR || format == GL_DEPTH || format == GL_STENCIL);

   /* assume complete */
   att->Complete = GL_TRUE;

   /* Look for reasons why the attachment might be incomplete */
   if (att->Type == GL_TEXTURE) {
      const struct gl_texture_object *texObj = att->Texture;
      struct gl_texture_image *texImage;
      GLenum baseFormat;

      if (!texObj) {
         att_incomplete("no texobj");
         att->Complete = GL_FALSE;
         return;
      }

      texImage = texObj->Image[att->CubeMapFace][att->TextureLevel];
      if (!texImage) {
         att_incomplete("no teximage");
         att->Complete = GL_FALSE;
         return;
      }
      if (texImage->Width < 1 || texImage->Height < 1) {
         att_incomplete("teximage width/height=0");
         att->Complete = GL_FALSE;
         return;
      }

      switch (texObj->Target) {
      case GL_TEXTURE_3D:
         if (att->Zoffset >= texImage->Depth) {
            att_incomplete("bad z offset");
            att->Complete = GL_FALSE;
            return;
         }
         break;
      case GL_TEXTURE_1D_ARRAY:
         if (att->Zoffset >= texImage->Height) {
            att_incomplete("bad 1D-array layer");
            att->Complete = GL_FALSE;
            return;
         }
         break;
      case GL_TEXTURE_2D_ARRAY:
         if (att->Zoffset >= texImage->Depth) {
            att_incomplete("bad 2D-array layer");
            att->Complete = GL_FALSE;
            return;
         }
         break;
      case GL_TEXTURE_CUBE_MAP_ARRAY:
         if (att->Zoffset >= texImage->Depth) {
            att_incomplete("bad cube-array layer");
            att->Complete = GL_FALSE;
            return;
         }
         break;
      }

      baseFormat = _mesa_get_format_base_format(texImage->TexFormat);

      if (format == GL_COLOR) {
         if (!_mesa_is_legal_color_format(ctx, baseFormat)) {
            att_incomplete("bad format");
            att->Complete = GL_FALSE;
            return;
         }
         if (_mesa_is_format_compressed(texImage->TexFormat)) {
            att_incomplete("compressed internalformat");
            att->Complete = GL_FALSE;
            return;
         }
      }
      else if (format == GL_DEPTH) {
         if (baseFormat == GL_DEPTH_COMPONENT) {
            /* OK */
         }
         else if (ctx->Extensions.ARB_depth_texture &&
                  baseFormat == GL_DEPTH_STENCIL) {
            /* OK */
         }
         else {
            att->Complete = GL_FALSE;
            att_incomplete("bad depth format");
            return;
         }
      }
      else {
         ASSERT(format == GL_STENCIL);
         if (ctx->Extensions.ARB_depth_texture &&
             baseFormat == GL_DEPTH_STENCIL) {
            /* OK */
         }
         else {
            /* no such thing as stencil-only textures */
            att_incomplete("illegal stencil texture");
            att->Complete = GL_FALSE;
            return;
         }
      }
   }
   else if (att->Type == GL_RENDERBUFFER_EXT) {
      const GLenum baseFormat =
         _mesa_get_format_base_format(att->Renderbuffer->Format);

      ASSERT(att->Renderbuffer);
      if (!att->Renderbuffer->InternalFormat ||
          att->Renderbuffer->Width < 1 ||
          att->Renderbuffer->Height < 1) {
         att_incomplete("0x0 renderbuffer");
         att->Complete = GL_FALSE;
         return;
      }
      if (format == GL_COLOR) {
         if (!_mesa_is_legal_color_format(ctx, baseFormat)) {
            att_incomplete("bad renderbuffer color format");
            att->Complete = GL_FALSE;
            return;
         }
      }
      else if (format == GL_DEPTH) {
         if (baseFormat == GL_DEPTH_COMPONENT) {
            /* OK */
         }
         else if (baseFormat == GL_DEPTH_STENCIL) {
            /* OK */
         }
         else {
            att_incomplete("bad renderbuffer depth format");
            att->Complete = GL_FALSE;
            return;
         }
      }
      else {
         assert(format == GL_STENCIL);
         if (baseFormat == GL_STENCIL_INDEX ||
             baseFormat == GL_DEPTH_STENCIL) {
            /* OK */
         }
         else {
            att->Complete = GL_FALSE;
            att_incomplete("bad renderbuffer stencil format");
            return;
         }
      }
   }
   else {
      ASSERT(att->Type == GL_NONE);
      /* complete */
      return;
   }
}


/**
 * Test if the given framebuffer object is complete and update its
 * Status field with the results.
 * Calls the ctx->Driver.ValidateFramebuffer() function to allow the
 * driver to make hardware-specific validation/completeness checks.
 * Also update the framebuffer's Width and Height fields if the
 * framebuffer is complete.
 */
void
_mesa_test_framebuffer_completeness(struct gl_context *ctx,
                                    struct gl_framebuffer *fb)
{
   GLuint numImages;
   GLenum intFormat = GL_NONE; /* color buffers' internal format */
   GLuint minWidth = ~0, minHeight = ~0, maxWidth = 0, maxHeight = 0;
   GLint numSamples = -1;
   GLint fixedSampleLocations = -1;
   GLint i;
   GLuint j;
   /* Covers max_layer_count, is_layered, and layer_tex_target */
   bool layer_info_valid = false;
   GLuint max_layer_count = 0, att_layer_count;
   bool is_layered = false;
   GLenum layer_tex_target = 0;

   assert(_mesa_is_user_fbo(fb));

   /* we're changing framebuffer fields here */
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   numImages = 0;
   fb->Width = 0;
   fb->Height = 0;
   fb->_AllColorBuffersFixedPoint = GL_TRUE;
   fb->_HasSNormOrFloatColorBuffer = GL_FALSE;

   /* Start at -2 to more easily loop over all attachment points.
    *  -2: depth buffer
    *  -1: stencil buffer
    * >=0: color buffer
    */
   for (i = -2; i < (GLint) ctx->Const.MaxColorAttachments; i++) {
      struct gl_renderbuffer_attachment *att;
      GLenum f;
      mesa_format attFormat;
      GLenum att_tex_target = GL_NONE;

      /*
       * XXX for ARB_fbo, only check color buffers that are named by
       * GL_READ_BUFFER and GL_DRAW_BUFFERi.
       */

      /* check for attachment completeness
       */
      if (i == -2) {
         att = &fb->Attachment[BUFFER_DEPTH];
         test_attachment_completeness(ctx, GL_DEPTH, att);
         if (!att->Complete) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            fbo_incomplete(ctx, "depth attachment incomplete", -1);
            return;
         }
      }
      else if (i == -1) {
         att = &fb->Attachment[BUFFER_STENCIL];
         test_attachment_completeness(ctx, GL_STENCIL, att);
         if (!att->Complete) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            fbo_incomplete(ctx, "stencil attachment incomplete", -1);
            return;
         }
      }
      else {
         att = &fb->Attachment[BUFFER_COLOR0 + i];
         test_attachment_completeness(ctx, GL_COLOR, att);
         if (!att->Complete) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            fbo_incomplete(ctx, "color attachment incomplete", i);
            return;
         }
      }

      /* get width, height, format of the renderbuffer/texture
       */
      if (att->Type == GL_TEXTURE) {
         const struct gl_texture_image *texImg = att->Renderbuffer->TexImage;
         att_tex_target = att->Texture->Target;
         minWidth = MIN2(minWidth, texImg->Width);
         maxWidth = MAX2(maxWidth, texImg->Width);
         minHeight = MIN2(minHeight, texImg->Height);
         maxHeight = MAX2(maxHeight, texImg->Height);
         f = texImg->_BaseFormat;
         attFormat = texImg->TexFormat;
         numImages++;

         if (!is_format_color_renderable(ctx, attFormat,
                                         texImg->InternalFormat) &&
             !is_legal_depth_format(ctx, f)) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT;
            fbo_incomplete(ctx, "texture attachment incomplete", -1);
            return;
         }

         if (numSamples < 0)
            numSamples = texImg->NumSamples;
         else if (numSamples != texImg->NumSamples) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
            fbo_incomplete(ctx, "inconsistent sample count", -1);
            return;
         }

         if (fixedSampleLocations < 0)
            fixedSampleLocations = texImg->FixedSampleLocations;
         else if (fixedSampleLocations != texImg->FixedSampleLocations) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
            fbo_incomplete(ctx, "inconsistent fixed sample locations", -1);
            return;
         }
      }
      else if (att->Type == GL_RENDERBUFFER_EXT) {
         minWidth = MIN2(minWidth, att->Renderbuffer->Width);
         maxWidth = MAX2(minWidth, att->Renderbuffer->Width);
         minHeight = MIN2(minHeight, att->Renderbuffer->Height);
         maxHeight = MAX2(minHeight, att->Renderbuffer->Height);
         f = att->Renderbuffer->InternalFormat;
         attFormat = att->Renderbuffer->Format;
         numImages++;

         if (numSamples < 0)
            numSamples = att->Renderbuffer->NumSamples;
         else if (numSamples != att->Renderbuffer->NumSamples) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
            fbo_incomplete(ctx, "inconsistent sample count", -1);
            return;
         }

         /* RENDERBUFFER has fixedSampleLocations implicitly true */
         if (fixedSampleLocations < 0)
            fixedSampleLocations = GL_TRUE;
         else if (fixedSampleLocations != GL_TRUE) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
            fbo_incomplete(ctx, "inconsistent fixed sample locations", -1);
            return;
         }
      }
      else {
         assert(att->Type == GL_NONE);
         continue;
      }

      /* check if integer color */
      fb->_IntegerColor = _mesa_is_format_integer_color(attFormat);

      /* Update _AllColorBuffersFixedPoint and _HasSNormOrFloatColorBuffer. */
      if (i >= 0) {
         GLenum type = _mesa_get_format_datatype(attFormat);

         fb->_AllColorBuffersFixedPoint =
            fb->_AllColorBuffersFixedPoint &&
            (type == GL_UNSIGNED_NORMALIZED || type == GL_SIGNED_NORMALIZED);

         fb->_HasSNormOrFloatColorBuffer =
            fb->_HasSNormOrFloatColorBuffer ||
            type == GL_SIGNED_NORMALIZED || type == GL_FLOAT;
      }

      /* Error-check width, height, format */
      if (numImages == 1) {
         /* save format */
         if (i >= 0) {
            intFormat = f;
         }
      }
      else {
         if (!ctx->Extensions.ARB_framebuffer_object) {
            /* check that width, height, format are same */
            if (minWidth != maxWidth || minHeight != maxHeight) {
               fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT;
               fbo_incomplete(ctx, "width or height mismatch", -1);
               return;
            }
            /* check that all color buffers are the same format */
            if (intFormat != GL_NONE && f != intFormat) {
               fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT;
               fbo_incomplete(ctx, "format mismatch", -1);
               return;
            }
         }
      }

      /* Check that the format is valid. (MESA_FORMAT_NONE means unsupported)
       */
      if (att->Type == GL_RENDERBUFFER &&
          att->Renderbuffer->Format == MESA_FORMAT_NONE) {
         fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED;
         fbo_incomplete(ctx, "unsupported renderbuffer format", i);
         return;
      }

      /* Check that layered rendering is consistent. */
      if (att->Layered) {
         if (att_tex_target == GL_TEXTURE_CUBE_MAP)
            att_layer_count = 6;
         else if (att_tex_target == GL_TEXTURE_1D_ARRAY)
            att_layer_count = att->Renderbuffer->Height;
         else
            att_layer_count = att->Renderbuffer->Depth;
      } else {
         att_layer_count = 0;
      }
      if (!layer_info_valid) {
         is_layered = att->Layered;
         max_layer_count = att_layer_count;
         layer_tex_target = att_tex_target;
         layer_info_valid = true;
      } else if (max_layer_count > 0 && layer_tex_target != att_tex_target) {
         fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS;
         fbo_incomplete(ctx, "layered framebuffer has mismatched targets", i);
         return;
      } else if (is_layered != att->Layered) {
         fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS;
         fbo_incomplete(ctx,
                        "framebuffer attachment layer mode is inconsistent",
                        i);
         return;
      } else if (att_layer_count > max_layer_count) {
         max_layer_count = att_layer_count;
      }
   }

   fb->MaxNumLayers = max_layer_count;

   if (numImages == 0) {
      fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT;
      fbo_incomplete(ctx, "no attachments", -1);
      return;
   }

   if (_mesa_is_desktop_gl(ctx) && !ctx->Extensions.ARB_ES2_compatibility) {
      /* Check that all DrawBuffers are present */
      for (j = 0; j < ctx->Const.MaxDrawBuffers; j++) {
	 if (fb->ColorDrawBuffer[j] != GL_NONE) {
	    const struct gl_renderbuffer_attachment *att
	       = get_attachment(ctx, fb, fb->ColorDrawBuffer[j]);
	    assert(att);
	    if (att->Type == GL_NONE) {
	       fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT;
	       fbo_incomplete(ctx, "missing drawbuffer", j);
	       return;
	    }
	 }
      }

      /* Check that the ReadBuffer is present */
      if (fb->ColorReadBuffer != GL_NONE) {
	 const struct gl_renderbuffer_attachment *att
	    = get_attachment(ctx, fb, fb->ColorReadBuffer);
	 assert(att);
	 if (att->Type == GL_NONE) {
	    fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT;
            fbo_incomplete(ctx, "missing readbuffer", -1);
	    return;
	 }
      }
   }

   /* Provisionally set status = COMPLETE ... */
   fb->_Status = GL_FRAMEBUFFER_COMPLETE_EXT;

   /* ... but the driver may say the FB is incomplete.
    * Drivers will most likely set the status to GL_FRAMEBUFFER_UNSUPPORTED
    * if anything.
    */
   if (ctx->Driver.ValidateFramebuffer) {
      ctx->Driver.ValidateFramebuffer(ctx, fb);
      if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         fbo_incomplete(ctx, "driver marked FBO as incomplete", -1);
      }
   }

   if (fb->_Status == GL_FRAMEBUFFER_COMPLETE_EXT) {
      /*
       * Note that if ARB_framebuffer_object is supported and the attached
       * renderbuffers/textures are different sizes, the framebuffer
       * width/height will be set to the smallest width/height.
       */
      fb->Width = minWidth;
      fb->Height = minHeight;

      /* finally, update the visual info for the framebuffer */
      _mesa_update_framebuffer_visual(ctx, fb);
   }
}


GLboolean GLAPIENTRY
_mesa_IsRenderbuffer(GLuint renderbuffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   if (renderbuffer) {
      struct gl_renderbuffer *rb =
         _mesa_lookup_renderbuffer(ctx, renderbuffer);
      if (rb != NULL && rb != &DummyRenderbuffer)
         return GL_TRUE;
   }
   return GL_FALSE;
}


static void
bind_renderbuffer(GLenum target, GLuint renderbuffer, bool allow_user_names)
{
   struct gl_renderbuffer *newRb;
   GET_CURRENT_CONTEXT(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindRenderbufferEXT(target)");
      return;
   }

   /* No need to flush here since the render buffer binding has no
    * effect on rendering state.
    */

   if (renderbuffer) {
      newRb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
      if (newRb == &DummyRenderbuffer) {
         /* ID was reserved, but no real renderbuffer object made yet */
         newRb = NULL;
      }
      else if (!newRb && !allow_user_names) {
         /* All RB IDs must be Gen'd */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glBindRenderbuffer(buffer)");
         return;
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
         newRb->RefCount = 1; /* referenced by hash table */
      }
   }
   else {
      newRb = NULL;
   }

   ASSERT(newRb != &DummyRenderbuffer);

   _mesa_reference_renderbuffer(&ctx->CurrentRenderbuffer, newRb);
}

void GLAPIENTRY
_mesa_BindRenderbuffer(GLenum target, GLuint renderbuffer)
{
   GET_CURRENT_CONTEXT(ctx);

   /* OpenGL ES glBindRenderbuffer and glBindRenderbufferOES use this same
    * entry point, but they allow the use of user-generated names.
    */
   bind_renderbuffer(target, renderbuffer, _mesa_is_gles(ctx));
}

void GLAPIENTRY
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
   /* This function should not be in the dispatch table for core profile /
    * OpenGL 3.1, so execution should never get here in those cases -- no
    * need for an explicit test.
    */
   bind_renderbuffer(target, renderbuffer, true);
}


/**
 * Remove the specified renderbuffer or texture from any attachment point in
 * the framebuffer.
 *
 * \returns
 * \c true if the renderbuffer was detached from an attachment point.  \c
 * false otherwise.
 */
bool
_mesa_detach_renderbuffer(struct gl_context *ctx,
                          struct gl_framebuffer *fb,
                          const void *att)
{
   unsigned i;
   bool progress = false;

   for (i = 0; i < BUFFER_COUNT; i++) {
      if (fb->Attachment[i].Texture == att
          || fb->Attachment[i].Renderbuffer == att) {
         remove_attachment(ctx, &fb->Attachment[i]);
         progress = true;
      }
   }

   /* Section 4.4.4 (Framebuffer Completeness), subsection "Whole Framebuffer
    * Completeness," of the OpenGL 3.1 spec says:
    *
    *     "Performing any of the following actions may change whether the
    *     framebuffer is considered complete or incomplete:
    *
    *     ...
    *
    *        - Deleting, with DeleteTextures or DeleteRenderbuffers, an object
    *          containing an image that is attached to a framebuffer object
    *          that is bound to the framebuffer."
    */
   if (progress)
      invalidate_framebuffer(fb);

   return progress;
}


void GLAPIENTRY
_mesa_DeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   for (i = 0; i < n; i++) {
      if (renderbuffers[i] > 0) {
	 struct gl_renderbuffer *rb;
	 rb = _mesa_lookup_renderbuffer(ctx, renderbuffers[i]);
	 if (rb) {
            /* check if deleting currently bound renderbuffer object */
            if (rb == ctx->CurrentRenderbuffer) {
               /* bind default */
               ASSERT(rb->RefCount >= 2);
               _mesa_BindRenderbuffer(GL_RENDERBUFFER_EXT, 0);
            }

            /* Section 4.4.2 (Attaching Images to Framebuffer Objects),
             * subsection "Attaching Renderbuffer Images to a Framebuffer,"
             * of the OpenGL 3.1 spec says:
             *
             *     "If a renderbuffer object is deleted while its image is
             *     attached to one or more attachment points in the currently
             *     bound framebuffer, then it is as if FramebufferRenderbuffer
             *     had been called, with a renderbuffer of 0, for each
             *     attachment point to which this image was attached in the
             *     currently bound framebuffer. In other words, this
             *     renderbuffer image is first detached from all attachment
             *     points in the currently bound framebuffer. Note that the
             *     renderbuffer image is specifically not detached from any
             *     non-bound framebuffers. Detaching the image from any
             *     non-bound framebuffers is the responsibility of the
             *     application.
             */
            if (_mesa_is_user_fbo(ctx->DrawBuffer)) {
               _mesa_detach_renderbuffer(ctx, ctx->DrawBuffer, rb);
            }
            if (_mesa_is_user_fbo(ctx->ReadBuffer)
                && ctx->ReadBuffer != ctx->DrawBuffer) {
               _mesa_detach_renderbuffer(ctx, ctx->ReadBuffer, rb);
            }

	    /* Remove from hash table immediately, to free the ID.
             * But the object will not be freed until it's no longer
             * referenced anywhere else.
             */
	    _mesa_HashRemove(ctx->Shared->RenderBuffers, renderbuffers[i]);

            if (rb != &DummyRenderbuffer) {
               /* no longer referenced by hash table */
               _mesa_reference_renderbuffer(&rb, NULL);
	    }
	 }
      }
   }
}


void GLAPIENTRY
_mesa_GenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

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
      mtx_lock(&ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->RenderBuffers, name, &DummyRenderbuffer);
      mtx_unlock(&ctx->Shared->Mutex);
   }
}


/**
 * Given an internal format token for a render buffer, return the
 * corresponding base format (one of GL_RGB, GL_RGBA, GL_STENCIL_INDEX,
 * GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL_EXT, GL_ALPHA, GL_LUMINANCE,
 * GL_LUMINANCE_ALPHA, GL_INTENSITY, etc).
 *
 * This is similar to _mesa_base_tex_format() but the set of valid
 * internal formats is different.
 *
 * Note that even if a format is determined to be legal here, validation
 * of the FBO may fail if the format is not supported by the driver/GPU.
 *
 * \param internalFormat  as passed to glRenderbufferStorage()
 * \return the base internal format, or 0 if internalFormat is illegal
 */
GLenum
_mesa_base_fbo_format(struct gl_context *ctx, GLenum internalFormat)
{
   /*
    * Notes: some formats such as alpha, luminance, etc. were added
    * with GL_ARB_framebuffer_object.
    */
   switch (internalFormat) {
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return (ctx->API == API_OPENGL_COMPAT &&
              ctx->Extensions.ARB_framebuffer_object) ? GL_ALPHA : 0;
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      return (ctx->API == API_OPENGL_COMPAT &&
              ctx->Extensions.ARB_framebuffer_object) ? GL_LUMINANCE : 0;
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      return (ctx->API == API_OPENGL_COMPAT &&
              ctx->Extensions.ARB_framebuffer_object) ? GL_LUMINANCE_ALPHA : 0;
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return (ctx->API == API_OPENGL_COMPAT &&
              ctx->Extensions.ARB_framebuffer_object) ? GL_INTENSITY : 0;
   case GL_RGB8:
      return GL_RGB;
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return _mesa_is_desktop_gl(ctx) ? GL_RGB : 0;
   case GL_SRGB8_EXT:
      return _mesa_is_desktop_gl(ctx) ? GL_RGB : 0;
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
      return GL_RGBA;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA12:
   case GL_RGBA16:
      return _mesa_is_desktop_gl(ctx) ? GL_RGBA : 0;
   case GL_RGB10_A2:
   case GL_SRGB8_ALPHA8_EXT:
      return _mesa_is_desktop_gl(ctx) || _mesa_is_gles3(ctx) ? GL_RGBA : 0;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* There are extensions for GL_STENCIL_INDEX1 and GL_STENCIL_INDEX4 in
       * OpenGL ES, but Mesa does not currently support them.
       */
      return _mesa_is_desktop_gl(ctx) ? GL_STENCIL_INDEX : 0;
   case GL_STENCIL_INDEX8_EXT:
      return GL_STENCIL_INDEX;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT32:
      return _mesa_is_desktop_gl(ctx) ? GL_DEPTH_COMPONENT : 0;
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
      return GL_DEPTH_COMPONENT;
   case GL_DEPTH_STENCIL:
      return _mesa_is_desktop_gl(ctx) ? GL_DEPTH_STENCIL : 0;
   case GL_DEPTH24_STENCIL8:
      return GL_DEPTH_STENCIL;
   case GL_DEPTH_COMPONENT32F:
      return ctx->Version >= 30
         || (ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_depth_buffer_float)
         ? GL_DEPTH_COMPONENT : 0;
   case GL_DEPTH32F_STENCIL8:
      return ctx->Version >= 30
         || (ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_depth_buffer_float)
         ? GL_DEPTH_STENCIL : 0;
   case GL_RED:
   case GL_R16:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_texture_rg
         ? GL_RED : 0;
   case GL_R8:
      return ctx->API != API_OPENGLES && ctx->Extensions.ARB_texture_rg
         ? GL_RED : 0;
   case GL_RG:
   case GL_RG16:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_texture_rg
         ? GL_RG : 0;
   case GL_RG8:
      return ctx->API != API_OPENGLES && ctx->Extensions.ARB_texture_rg
         ? GL_RG : 0;
   /* signed normalized texture formats */
   case GL_RED_SNORM:
   case GL_R8_SNORM:
   case GL_R16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_RED : 0;
   case GL_RG_SNORM:
   case GL_RG8_SNORM:
   case GL_RG16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_RG : 0;
   case GL_RGB_SNORM:
   case GL_RGB8_SNORM:
   case GL_RGB16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_RGB : 0;
   case GL_RGBA_SNORM:
   case GL_RGBA8_SNORM:
   case GL_RGBA16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_RGBA : 0;
   case GL_ALPHA_SNORM:
   case GL_ALPHA8_SNORM:
   case GL_ALPHA16_SNORM:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.EXT_texture_snorm &&
             ctx->Extensions.ARB_framebuffer_object ? GL_ALPHA : 0;
   case GL_LUMINANCE_SNORM:
   case GL_LUMINANCE8_SNORM:
   case GL_LUMINANCE16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_LUMINANCE : 0;
   case GL_LUMINANCE_ALPHA_SNORM:
   case GL_LUMINANCE8_ALPHA8_SNORM:
   case GL_LUMINANCE16_ALPHA16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_LUMINANCE_ALPHA : 0;
   case GL_INTENSITY_SNORM:
   case GL_INTENSITY8_SNORM:
   case GL_INTENSITY16_SNORM:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_snorm
         ? GL_INTENSITY : 0;

   case GL_R16F:
   case GL_R32F:
      return ((_mesa_is_desktop_gl(ctx) &&
               ctx->Extensions.ARB_texture_rg &&
               ctx->Extensions.ARB_texture_float) ||
              _mesa_is_gles3(ctx) /* EXT_color_buffer_float */ )
         ? GL_RED : 0;
   case GL_RG16F:
   case GL_RG32F:
      return ((_mesa_is_desktop_gl(ctx) &&
               ctx->Extensions.ARB_texture_rg &&
               ctx->Extensions.ARB_texture_float) ||
              _mesa_is_gles3(ctx) /* EXT_color_buffer_float */ )
         ? GL_RG : 0;
   case GL_RGB16F:
   case GL_RGB32F:
      return (_mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_texture_float)
         ? GL_RGB : 0;
   case GL_RGBA16F:
   case GL_RGBA32F:
      return ((_mesa_is_desktop_gl(ctx) &&
               ctx->Extensions.ARB_texture_float) ||
              _mesa_is_gles3(ctx) /* EXT_color_buffer_float */ )
         ? GL_RGBA : 0;
   case GL_ALPHA16F_ARB:
   case GL_ALPHA32F_ARB:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_texture_float &&
             ctx->Extensions.ARB_framebuffer_object ? GL_ALPHA : 0;
   case GL_LUMINANCE16F_ARB:
   case GL_LUMINANCE32F_ARB:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_texture_float &&
             ctx->Extensions.ARB_framebuffer_object ? GL_LUMINANCE : 0;
   case GL_LUMINANCE_ALPHA16F_ARB:
   case GL_LUMINANCE_ALPHA32F_ARB:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_texture_float &&
             ctx->Extensions.ARB_framebuffer_object ? GL_LUMINANCE_ALPHA : 0;
   case GL_INTENSITY16F_ARB:
   case GL_INTENSITY32F_ARB:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.ARB_texture_float &&
             ctx->Extensions.ARB_framebuffer_object ? GL_INTENSITY : 0;
   case GL_R11F_G11F_B10F:
      return ((_mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_packed_float) ||
              _mesa_is_gles3(ctx) /* EXT_color_buffer_float */ )
         ? GL_RGB : 0;

   case GL_RGBA8UI_EXT:
   case GL_RGBA16UI_EXT:
   case GL_RGBA32UI_EXT:
   case GL_RGBA8I_EXT:
   case GL_RGBA16I_EXT:
   case GL_RGBA32I_EXT:
      return ctx->Version >= 30
         || (_mesa_is_desktop_gl(ctx) &&
             ctx->Extensions.EXT_texture_integer) ? GL_RGBA : 0;

   case GL_RGB8UI_EXT:
   case GL_RGB16UI_EXT:
   case GL_RGB32UI_EXT:
   case GL_RGB8I_EXT:
   case GL_RGB16I_EXT:
   case GL_RGB32I_EXT:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_integer
         ? GL_RGB : 0;
   case GL_R8UI:
   case GL_R8I:
   case GL_R16UI:
   case GL_R16I:
   case GL_R32UI:
   case GL_R32I:
      return ctx->Version >= 30
         || (_mesa_is_desktop_gl(ctx) &&
             ctx->Extensions.ARB_texture_rg &&
             ctx->Extensions.EXT_texture_integer) ? GL_RED : 0;

   case GL_RG8UI:
   case GL_RG8I:
   case GL_RG16UI:
   case GL_RG16I:
   case GL_RG32UI:
   case GL_RG32I:
      return ctx->Version >= 30
         || (_mesa_is_desktop_gl(ctx) &&
             ctx->Extensions.ARB_texture_rg &&
             ctx->Extensions.EXT_texture_integer) ? GL_RG : 0;

   case GL_INTENSITY8I_EXT:
   case GL_INTENSITY8UI_EXT:
   case GL_INTENSITY16I_EXT:
   case GL_INTENSITY16UI_EXT:
   case GL_INTENSITY32I_EXT:
   case GL_INTENSITY32UI_EXT:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.EXT_texture_integer &&
             ctx->Extensions.ARB_framebuffer_object ? GL_INTENSITY : 0;

   case GL_LUMINANCE8I_EXT:
   case GL_LUMINANCE8UI_EXT:
   case GL_LUMINANCE16I_EXT:
   case GL_LUMINANCE16UI_EXT:
   case GL_LUMINANCE32I_EXT:
   case GL_LUMINANCE32UI_EXT:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.EXT_texture_integer &&
             ctx->Extensions.ARB_framebuffer_object ? GL_LUMINANCE : 0;

   case GL_LUMINANCE_ALPHA8I_EXT:
   case GL_LUMINANCE_ALPHA8UI_EXT:
   case GL_LUMINANCE_ALPHA16I_EXT:
   case GL_LUMINANCE_ALPHA16UI_EXT:
   case GL_LUMINANCE_ALPHA32I_EXT:
   case GL_LUMINANCE_ALPHA32UI_EXT:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.EXT_texture_integer &&
             ctx->Extensions.ARB_framebuffer_object ? GL_LUMINANCE_ALPHA : 0;

   case GL_ALPHA8I_EXT:
   case GL_ALPHA8UI_EXT:
   case GL_ALPHA16I_EXT:
   case GL_ALPHA16UI_EXT:
   case GL_ALPHA32I_EXT:
   case GL_ALPHA32UI_EXT:
      return ctx->API == API_OPENGL_COMPAT &&
             ctx->Extensions.EXT_texture_integer &&
             ctx->Extensions.ARB_framebuffer_object ? GL_ALPHA : 0;

   case GL_RGB10_A2UI:
      return (_mesa_is_desktop_gl(ctx) &&
              ctx->Extensions.ARB_texture_rgb10_a2ui)
         || _mesa_is_gles3(ctx) ? GL_RGBA : 0;

   case GL_RGB565:
      return _mesa_is_gles(ctx) || ctx->Extensions.ARB_ES2_compatibility
         ? GL_RGB : 0;
   default:
      return 0;
   }
}


/**
 * Invalidate a renderbuffer attachment.  Called from _mesa_HashWalk().
 */
static void
invalidate_rb(GLuint key, void *data, void *userData)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) data;
   struct gl_renderbuffer *rb = (struct gl_renderbuffer *) userData;

   /* If this is a user-created FBO */
   if (_mesa_is_user_fbo(fb)) {
      GLuint i;
      for (i = 0; i < BUFFER_COUNT; i++) {
         struct gl_renderbuffer_attachment *att = fb->Attachment + i;
         if (att->Type == GL_RENDERBUFFER &&
             att->Renderbuffer == rb) {
            /* Mark fb status as indeterminate to force re-validation */
            fb->_Status = 0;
            return;
         }
      }
   }
}


/** sentinal value, see below */
#define NO_SAMPLES 1000


/**
 * Helper function used by _mesa_RenderbufferStorage() and
 * _mesa_RenderbufferStorageMultisample().
 * samples will be NO_SAMPLES if called by _mesa_RenderbufferStorage().
 */
static void
renderbuffer_storage(GLenum target, GLenum internalFormat,
                     GLsizei width, GLsizei height, GLsizei samples)
{
   const char *func = samples == NO_SAMPLES ?
      "glRenderbufferStorage" : "glRenderbufferStorageMultisample";
   struct gl_renderbuffer *rb;
   GLenum baseFormat;
   GLenum sample_count_error;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API) {
      if (samples == NO_SAMPLES)
         _mesa_debug(ctx, "%s(%s, %s, %d, %d)\n",
                     func,
                     _mesa_lookup_enum_by_nr(target),
                     _mesa_lookup_enum_by_nr(internalFormat),
                     width, height);
      else
         _mesa_debug(ctx, "%s(%s, %s, %d, %d, %d)\n",
                     func,
                     _mesa_lookup_enum_by_nr(target),
                     _mesa_lookup_enum_by_nr(internalFormat),
                     width, height, samples);
   }

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", func);
      return;
   }

   baseFormat = _mesa_base_fbo_format(ctx, internalFormat);
   if (baseFormat == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(internalFormat=%s)",
                  func, _mesa_lookup_enum_by_nr(internalFormat));
      return;
   }

   if (width < 0 || width > (GLsizei) ctx->Const.MaxRenderbufferSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(width)", func);
      return;
   }

   if (height < 0 || height > (GLsizei) ctx->Const.MaxRenderbufferSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(height)", func);
      return;
   }

   if (samples == NO_SAMPLES) {
      /* NumSamples == 0 indicates non-multisampling */
      samples = 0;
   }
   else {
      /* check the sample count;
       * note: driver may choose to use more samples than what's requested
       */
      sample_count_error = _mesa_check_sample_count(ctx, target,
            internalFormat, samples);
      if (sample_count_error != GL_NO_ERROR) {
         _mesa_error(ctx, sample_count_error, "%s(samples)", func);
         return;
      }
   }

   rb = ctx->CurrentRenderbuffer;
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", func);
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   if (rb->InternalFormat == internalFormat &&
       rb->Width == (GLuint) width &&
       rb->Height == (GLuint) height &&
       rb->NumSamples == samples) {
      /* no change in allocation needed */
      return;
   }

   /* These MUST get set by the AllocStorage func */
   rb->Format = MESA_FORMAT_NONE;
   rb->NumSamples = samples;

   /* Now allocate the storage */
   ASSERT(rb->AllocStorage);
   if (rb->AllocStorage(ctx, rb, internalFormat, width, height)) {
      /* No error - check/set fields now */
      /* If rb->Format == MESA_FORMAT_NONE, the format is unsupported. */
      assert(rb->Width == (GLuint) width);
      assert(rb->Height == (GLuint) height);
      rb->InternalFormat = internalFormat;
      rb->_BaseFormat = baseFormat;
      assert(rb->_BaseFormat != 0);
   }
   else {
      /* Probably ran out of memory - clear the fields */
      rb->Width = 0;
      rb->Height = 0;
      rb->Format = MESA_FORMAT_NONE;
      rb->InternalFormat = GL_NONE;
      rb->_BaseFormat = GL_NONE;
      rb->NumSamples = 0;
   }

   /* Invalidate the framebuffers the renderbuffer is attached in. */
   if (rb->AttachedAnytime) {
      _mesa_HashWalk(ctx->Shared->FrameBuffers, invalidate_rb, rb);
   }
}


void GLAPIENTRY
_mesa_EGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
   struct gl_renderbuffer *rb;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.OES_EGL_image) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEGLImageTargetRenderbufferStorageOES(unsupported)");
      return;
   }

   if (target != GL_RENDERBUFFER) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "EGLImageTargetRenderbufferStorageOES");
      return;
   }

   rb = ctx->CurrentRenderbuffer;
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "EGLImageTargetRenderbufferStorageOES");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   ctx->Driver.EGLImageTargetRenderbufferStorage(ctx, rb, image);
}


/**
 * Helper function for _mesa_GetRenderbufferParameteriv() and
 * _mesa_GetFramebufferAttachmentParameteriv()
 * We have to be careful to respect the base format.  For example, if a
 * renderbuffer/texture was created with internalFormat=GL_RGB but the
 * driver actually chose a GL_RGBA format, when the user queries ALPHA_SIZE
 * we need to return zero.
 */
static GLint
get_component_bits(GLenum pname, GLenum baseFormat, mesa_format format)
{
   if (_mesa_base_format_has_channel(baseFormat, pname))
      return _mesa_get_format_bits(format, pname);
   else
      return 0;
}



void GLAPIENTRY
_mesa_RenderbufferStorage(GLenum target, GLenum internalFormat,
                             GLsizei width, GLsizei height)
{
   /* GL_ARB_fbo says calling this function is equivalent to calling
    * glRenderbufferStorageMultisample() with samples=0.  We pass in
    * a token value here just for error reporting purposes.
    */
   renderbuffer_storage(target, internalFormat, width, height, NO_SAMPLES);
}


void GLAPIENTRY
_mesa_RenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                     GLenum internalFormat,
                                     GLsizei width, GLsizei height)
{
   renderbuffer_storage(target, internalFormat, width, height, samples);
}


/**
 * OpenGL ES version of glRenderBufferStorage.
 */
void GLAPIENTRY
_es_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
			   GLsizei width, GLsizei height)
{
   switch (internalFormat) {
   case GL_RGB565:
      /* XXX this confuses GL_RENDERBUFFER_INTERNAL_FORMAT_OES */
      /* choose a closest format */
      internalFormat = GL_RGB5;
      break;
   default:
      break;
   }

   renderbuffer_storage(target, internalFormat, width, height, 0);
}


void GLAPIENTRY
_mesa_GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
   struct gl_renderbuffer *rb;
   GET_CURRENT_CONTEXT(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }

   rb = ctx->CurrentRenderbuffer;
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetRenderbufferParameterivEXT");
      return;
   }

   /* No need to flush here since we're just quering state which is
    * not effected by rendering.
    */

   switch (pname) {
   case GL_RENDERBUFFER_WIDTH_EXT:
      *params = rb->Width;
      return;
   case GL_RENDERBUFFER_HEIGHT_EXT:
      *params = rb->Height;
      return;
   case GL_RENDERBUFFER_INTERNAL_FORMAT_EXT:
      *params = rb->InternalFormat;
      return;
   case GL_RENDERBUFFER_RED_SIZE_EXT:
   case GL_RENDERBUFFER_GREEN_SIZE_EXT:
   case GL_RENDERBUFFER_BLUE_SIZE_EXT:
   case GL_RENDERBUFFER_ALPHA_SIZE_EXT:
   case GL_RENDERBUFFER_DEPTH_SIZE_EXT:
   case GL_RENDERBUFFER_STENCIL_SIZE_EXT:
      *params = get_component_bits(pname, rb->_BaseFormat, rb->Format);
      break;
   case GL_RENDERBUFFER_SAMPLES:
      if ((_mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_framebuffer_object)
          || _mesa_is_gles3(ctx)) {
         *params = rb->NumSamples;
         break;
      }
      /* fallthrough */
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }
}


GLboolean GLAPIENTRY
_mesa_IsFramebuffer(GLuint framebuffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   if (framebuffer) {
      struct gl_framebuffer *rb = _mesa_lookup_framebuffer(ctx, framebuffer);
      if (rb != NULL && rb != &DummyFramebuffer)
         return GL_TRUE;
   }
   return GL_FALSE;
}


/**
 * Check if any of the attachments of the given framebuffer are textures
 * (render to texture).  Call ctx->Driver.RenderTexture() for such
 * attachments.
 */
static void
check_begin_texture_render(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   GLuint i;
   ASSERT(ctx->Driver.RenderTexture);

   if (_mesa_is_winsys_fbo(fb))
      return; /* can't render to texture with winsys framebuffers */

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = fb->Attachment + i;
      if (att->Texture && att->Renderbuffer->TexImage
          && driver_RenderTexture_is_safe(att)) {
         ctx->Driver.RenderTexture(ctx, fb, att);
      }
   }
}


/**
 * Examine all the framebuffer's attachments to see if any are textures.
 * If so, call ctx->Driver.FinishRenderTexture() for each texture to
 * notify the device driver that the texture image may have changed.
 */
static void
check_end_texture_render(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   /* Skip if we know NeedsFinishRenderTexture won't be set. */
   if (_mesa_is_winsys_fbo(fb) && !ctx->Driver.BindRenderbufferTexImage)
      return;

   if (ctx->Driver.FinishRenderTexture) {
      GLuint i;
      for (i = 0; i < BUFFER_COUNT; i++) {
         struct gl_renderbuffer_attachment *att = fb->Attachment + i;
         struct gl_renderbuffer *rb = att->Renderbuffer;
         if (rb && rb->NeedsFinishRenderTexture) {
            ctx->Driver.FinishRenderTexture(ctx, rb);
         }
      }
   }
}


static void
bind_framebuffer(GLenum target, GLuint framebuffer, bool allow_user_names)
{
   struct gl_framebuffer *newDrawFb, *newReadFb;
   struct gl_framebuffer *oldDrawFb, *oldReadFb;
   GLboolean bindReadBuf, bindDrawBuf;
   GET_CURRENT_CONTEXT(ctx);

   switch (target) {
   case GL_DRAW_FRAMEBUFFER_EXT:
      bindDrawBuf = GL_TRUE;
      bindReadBuf = GL_FALSE;
      break;
   case GL_READ_FRAMEBUFFER_EXT:
      bindDrawBuf = GL_FALSE;
      bindReadBuf = GL_TRUE;
      break;
   case GL_FRAMEBUFFER_EXT:
      bindDrawBuf = GL_TRUE;
      bindReadBuf = GL_TRUE;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindFramebufferEXT(target)");
      return;
   }

   if (framebuffer) {
      /* Binding a user-created framebuffer object */
      newDrawFb = _mesa_lookup_framebuffer(ctx, framebuffer);
      if (newDrawFb == &DummyFramebuffer) {
         /* ID was reserved, but no real framebuffer object made yet */
         newDrawFb = NULL;
      }
      else if (!newDrawFb && !allow_user_names) {
         /* All FBO IDs must be Gen'd */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glBindFramebuffer(buffer)");
         return;
      }

      if (!newDrawFb) {
	 /* create new framebuffer object */
	 newDrawFb = ctx->Driver.NewFramebuffer(ctx, framebuffer);
	 if (!newDrawFb) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindFramebufferEXT");
	    return;
	 }
         _mesa_HashInsert(ctx->Shared->FrameBuffers, framebuffer, newDrawFb);
      }
      newReadFb = newDrawFb;
   }
   else {
      /* Binding the window system framebuffer (which was originally set
       * with MakeCurrent).
       */
      newDrawFb = ctx->WinSysDrawBuffer;
      newReadFb = ctx->WinSysReadBuffer;
   }

   ASSERT(newDrawFb);
   ASSERT(newDrawFb != &DummyFramebuffer);

   /* save pointers to current/old framebuffers */
   oldDrawFb = ctx->DrawBuffer;
   oldReadFb = ctx->ReadBuffer;

   /* check if really changing bindings */
   if (oldDrawFb == newDrawFb)
      bindDrawBuf = GL_FALSE;
   if (oldReadFb == newReadFb)
      bindReadBuf = GL_FALSE;

   /*
    * OK, now bind the new Draw/Read framebuffers, if they're changing.
    *
    * We also check if we're beginning and/or ending render-to-texture.
    * When a framebuffer with texture attachments is unbound, call
    * ctx->Driver.FinishRenderTexture().
    * When a framebuffer with texture attachments is bound, call
    * ctx->Driver.RenderTexture().
    *
    * Note that if the ReadBuffer has texture attachments we don't consider
    * that a render-to-texture case.
    */
   if (bindReadBuf) {
      FLUSH_VERTICES(ctx, _NEW_BUFFERS);

      /* check if old readbuffer was render-to-texture */
      check_end_texture_render(ctx, oldReadFb);

      _mesa_reference_framebuffer(&ctx->ReadBuffer, newReadFb);
   }

   if (bindDrawBuf) {
      FLUSH_VERTICES(ctx, _NEW_BUFFERS);

      /* check if old framebuffer had any texture attachments */
      if (oldDrawFb)
         check_end_texture_render(ctx, oldDrawFb);

      /* check if newly bound framebuffer has any texture attachments */
      check_begin_texture_render(ctx, newDrawFb);

      _mesa_reference_framebuffer(&ctx->DrawBuffer, newDrawFb);
   }

   if ((bindDrawBuf || bindReadBuf) && ctx->Driver.BindFramebuffer) {
      ctx->Driver.BindFramebuffer(ctx, target, newDrawFb, newReadFb);
   }
}

void GLAPIENTRY
_mesa_BindFramebuffer(GLenum target, GLuint framebuffer)
{
   GET_CURRENT_CONTEXT(ctx);

   /* OpenGL ES glBindFramebuffer and glBindFramebufferOES use this same entry
    * point, but they allow the use of user-generated names.
    */
   bind_framebuffer(target, framebuffer, _mesa_is_gles(ctx));
}


void GLAPIENTRY
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer)
{
   /* This function should not be in the dispatch table for core profile /
    * OpenGL 3.1, so execution should never get here in those cases -- no
    * need for an explicit test.
    */
   bind_framebuffer(target, framebuffer, true);
}


void GLAPIENTRY
_mesa_DeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   for (i = 0; i < n; i++) {
      if (framebuffers[i] > 0) {
	 struct gl_framebuffer *fb;
	 fb = _mesa_lookup_framebuffer(ctx, framebuffers[i]);
	 if (fb) {
            ASSERT(fb == &DummyFramebuffer || fb->Name == framebuffers[i]);

            /* check if deleting currently bound framebuffer object */
            if (fb == ctx->DrawBuffer) {
               /* bind default */
               ASSERT(fb->RefCount >= 2);
               _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }
            if (fb == ctx->ReadBuffer) {
               /* bind default */
               ASSERT(fb->RefCount >= 2);
               _mesa_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            }

	    /* remove from hash table immediately, to free the ID */
	    _mesa_HashRemove(ctx->Shared->FrameBuffers, framebuffers[i]);

            if (fb != &DummyFramebuffer) {
               /* But the object will not be freed until it's no longer
                * bound in any context.
                */
               _mesa_reference_framebuffer(&fb, NULL);
	    }
	 }
      }
   }
}


void GLAPIENTRY
_mesa_GenFramebuffers(GLsizei n, GLuint *framebuffers)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

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
      mtx_lock(&ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->FrameBuffers, name, &DummyFramebuffer);
      mtx_unlock(&ctx->Shared->Mutex);
   }
}


GLenum GLAPIENTRY
_mesa_CheckFramebufferStatus(GLenum target)
{
   struct gl_framebuffer *buffer;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glCheckFramebufferStatus(%s)\n",
                  _mesa_lookup_enum_by_nr(target));

   buffer = get_framebuffer_target(ctx, target);
   if (!buffer) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCheckFramebufferStatus(target)");
      return 0;
   }

   if (_mesa_is_winsys_fbo(buffer)) {
      /* EGL_KHR_surfaceless_context allows the winsys FBO to be incomplete. */
      if (buffer != &IncompleteFramebuffer) {
         return GL_FRAMEBUFFER_COMPLETE_EXT;
      } else {
         return GL_FRAMEBUFFER_UNDEFINED;
      }
   }

   /* No need to flush here */

   if (buffer->_Status != GL_FRAMEBUFFER_COMPLETE) {
      _mesa_test_framebuffer_completeness(ctx, buffer);
   }

   return buffer->_Status;
}


/**
 * Replicate the src attachment point. Used by framebuffer_texture() when
 * the same texture is attached at GL_DEPTH_ATTACHMENT and
 * GL_STENCIL_ATTACHMENT.
 */
static void
reuse_framebuffer_texture_attachment(struct gl_framebuffer *fb,
                                     gl_buffer_index dst,
                                     gl_buffer_index src)
{
   struct gl_renderbuffer_attachment *dst_att = &fb->Attachment[dst];
   struct gl_renderbuffer_attachment *src_att = &fb->Attachment[src];

   assert(src_att->Texture != NULL);
   assert(src_att->Renderbuffer != NULL);

   _mesa_reference_texobj(&dst_att->Texture, src_att->Texture);
   _mesa_reference_renderbuffer(&dst_att->Renderbuffer, src_att->Renderbuffer);
   dst_att->Type = src_att->Type;
   dst_att->Complete = src_att->Complete;
   dst_att->TextureLevel = src_att->TextureLevel;
   dst_att->Zoffset = src_att->Zoffset;
}


/**
 * Common code called by glFramebufferTexture1D/2D/3DEXT() and
 * glFramebufferTextureLayerEXT().
 *
 * \param textarget is the textarget that was passed to the
 * glFramebufferTexture...() function, or 0 if the corresponding function
 * doesn't have a textarget parameter.
 *
 * \param layered is true if this function was called from
 * glFramebufferTexture(), false otherwise.
 */
static void
framebuffer_texture(struct gl_context *ctx, const char *caller, GLenum target,
                    GLenum attachment, GLenum textarget, GLuint texture,
                    GLint level, GLint zoffset, GLboolean layered)
{
   struct gl_renderbuffer_attachment *att;
   struct gl_texture_object *texObj = NULL;
   struct gl_framebuffer *fb;
   GLenum maxLevelsTarget;

   fb = get_framebuffer_target(ctx, target);
   if (!fb) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%sEXT(target=0x%x)", caller, target);
      return;
   }

   /* check framebuffer binding */
   if (_mesa_is_winsys_fbo(fb)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFramebufferTexture%sEXT", caller);
      return;
   }

   /* The textarget, level, and zoffset parameters are only validated if
    * texture is non-zero.
    */
   if (texture) {
      GLboolean err = GL_TRUE;

      texObj = _mesa_lookup_texture(ctx, texture);
      if (texObj != NULL) {
         if (textarget == 0) {
            if (layered) {
               /* We're being called by glFramebufferTexture() and textarget
                * is not used.
                */
               switch (texObj->Target) {
               case GL_TEXTURE_3D:
               case GL_TEXTURE_1D_ARRAY_EXT:
               case GL_TEXTURE_2D_ARRAY_EXT:
               case GL_TEXTURE_CUBE_MAP:
               case GL_TEXTURE_CUBE_MAP_ARRAY:
               case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                  err = false;
                  break;
               case GL_TEXTURE_1D:
               case GL_TEXTURE_2D:
               case GL_TEXTURE_RECTANGLE:
               case GL_TEXTURE_2D_MULTISAMPLE:
                  /* These texture types are valid to pass to
                   * glFramebufferTexture(), but since they aren't layered, it
                   * is equivalent to calling glFramebufferTexture{1D,2D}().
                   */
                  err = false;
                  layered = false;
                  textarget = texObj->Target;
                  break;
               default:
                  err = true;
                  break;
               }
            } else {
               /* We're being called by glFramebufferTextureLayer() and
                * textarget is not used.  The only legal texture types for
                * that function are 3D and 1D/2D arrays textures.
                */
               err = (texObj->Target != GL_TEXTURE_3D) &&
                  (texObj->Target != GL_TEXTURE_1D_ARRAY_EXT) &&
                  (texObj->Target != GL_TEXTURE_2D_ARRAY_EXT) &&
                  (texObj->Target != GL_TEXTURE_CUBE_MAP_ARRAY) &&
                  (texObj->Target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
            }
         }
         else {
            /* Make sure textarget is consistent with the texture's type */
            err = (texObj->Target == GL_TEXTURE_CUBE_MAP)
                ? !_mesa_is_cube_face(textarget)
                : (texObj->Target != textarget);
         }
      }
      else {
         /* can't render to a non-existant texture */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glFramebufferTexture%sEXT(non existant texture)",
                     caller);
         return;
      }

      if (err) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glFramebufferTexture%sEXT(texture target mismatch)",
                     caller);
         return;
      }

      if (texObj->Target == GL_TEXTURE_3D) {
         const GLint maxSize = 1 << (ctx->Const.Max3DTextureLevels - 1);
         if (zoffset < 0 || zoffset >= maxSize) {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glFramebufferTexture%sEXT(zoffset)", caller);
            return;
         }
      }
      else if ((texObj->Target == GL_TEXTURE_1D_ARRAY_EXT) ||
               (texObj->Target == GL_TEXTURE_2D_ARRAY_EXT) ||
               (texObj->Target == GL_TEXTURE_CUBE_MAP_ARRAY) ||
               (texObj->Target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)) {
         if (zoffset < 0 ||
             zoffset >= (GLint) ctx->Const.MaxArrayTextureLayers) {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glFramebufferTexture%sEXT(layer)", caller);
            return;
         }
      }

      maxLevelsTarget = textarget ? textarget : texObj->Target;
      if ((level < 0) ||
          (level >= _mesa_max_texture_levels(ctx, maxLevelsTarget))) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glFramebufferTexture%sEXT(level)", caller);
         return;
      }
   }

   att = get_attachment(ctx, fb, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%sEXT(attachment)", caller);
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   mtx_lock(&fb->Mutex);
   if (texObj) {
      if (attachment == GL_DEPTH_ATTACHMENT &&
          texObj == fb->Attachment[BUFFER_STENCIL].Texture &&
          level == fb->Attachment[BUFFER_STENCIL].TextureLevel &&
          _mesa_tex_target_to_face(textarget) ==
          fb->Attachment[BUFFER_STENCIL].CubeMapFace &&
          zoffset == fb->Attachment[BUFFER_STENCIL].Zoffset) {
	 /* The texture object is already attached to the stencil attachment
	  * point. Don't create a new renderbuffer; just reuse the stencil
	  * attachment's. This is required to prevent a GL error in
	  * glGetFramebufferAttachmentParameteriv(GL_DEPTH_STENCIL).
	  */
	 reuse_framebuffer_texture_attachment(fb, BUFFER_DEPTH,
	                                      BUFFER_STENCIL);
      } else if (attachment == GL_STENCIL_ATTACHMENT &&
	         texObj == fb->Attachment[BUFFER_DEPTH].Texture &&
                 level == fb->Attachment[BUFFER_DEPTH].TextureLevel &&
                 _mesa_tex_target_to_face(textarget) ==
                 fb->Attachment[BUFFER_DEPTH].CubeMapFace &&
                 zoffset == fb->Attachment[BUFFER_DEPTH].Zoffset) {
	 /* As above, but with depth and stencil transposed. */
	 reuse_framebuffer_texture_attachment(fb, BUFFER_STENCIL,
	                                      BUFFER_DEPTH);
      } else {
	 set_texture_attachment(ctx, fb, att, texObj, textarget,
	                              level, zoffset, layered);
	 if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
	    /* Above we created a new renderbuffer and attached it to the
	     * depth attachment point. Now attach it to the stencil attachment
	     * point too.
	     */
	    assert(att == &fb->Attachment[BUFFER_DEPTH]);
	    reuse_framebuffer_texture_attachment(fb,BUFFER_STENCIL,
	                                         BUFFER_DEPTH);
	 }
      }

      /* Set the render-to-texture flag.  We'll check this flag in
       * glTexImage() and friends to determine if we need to revalidate
       * any FBOs that might be rendering into this texture.
       * This flag never gets cleared since it's non-trivial to determine
       * when all FBOs might be done rendering to this texture.  That's OK
       * though since it's uncommon to render to a texture then repeatedly
       * call glTexImage() to change images in the texture.
       */
      texObj->_RenderToTexture = GL_TRUE;
   }
   else {
      remove_attachment(ctx, att);
      if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
	 assert(att == &fb->Attachment[BUFFER_DEPTH]);
	 remove_attachment(ctx, &fb->Attachment[BUFFER_STENCIL]);
      }
   }

   invalidate_framebuffer(fb);

   mtx_unlock(&fb->Mutex);
}


void GLAPIENTRY
_mesa_FramebufferTexture1D(GLenum target, GLenum attachment,
                           GLenum textarget, GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   if (texture != 0) {
      GLboolean error;

      switch (textarget) {
      case GL_TEXTURE_1D:
         error = GL_FALSE;
         break;
      case GL_TEXTURE_1D_ARRAY:
         error = !ctx->Extensions.EXT_texture_array;
         break;
      default:
         error = GL_TRUE;
      }

      if (error) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glFramebufferTexture1DEXT(textarget=%s)",
                     _mesa_lookup_enum_by_nr(textarget));
         return;
      }
   }

   framebuffer_texture(ctx, "1D", target, attachment, textarget, texture,
                       level, 0, GL_FALSE);
}


void GLAPIENTRY
_mesa_FramebufferTexture2D(GLenum target, GLenum attachment,
                           GLenum textarget, GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   if (texture != 0) {
      GLboolean error;

      switch (textarget) {
      case GL_TEXTURE_2D:
         error = GL_FALSE;
         break;
      case GL_TEXTURE_RECTANGLE:
         error = _mesa_is_gles(ctx)
            || !ctx->Extensions.NV_texture_rectangle;
         break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         error = !ctx->Extensions.ARB_texture_cube_map;
         break;
      case GL_TEXTURE_2D_ARRAY:
         error = (_mesa_is_gles(ctx) && ctx->Version < 30)
            || !ctx->Extensions.EXT_texture_array;
         break;
      case GL_TEXTURE_2D_MULTISAMPLE:
      case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
         error = _mesa_is_gles(ctx)
            || !ctx->Extensions.ARB_texture_multisample;
         break;
      default:
         error = GL_TRUE;
      }

      if (error) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glFramebufferTexture2DEXT(textarget=%s)",
                     _mesa_lookup_enum_by_nr(textarget));
         return;
      }
   }

   framebuffer_texture(ctx, "2D", target, attachment, textarget, texture,
                       level, 0, GL_FALSE);
}


void GLAPIENTRY
_mesa_FramebufferTexture3D(GLenum target, GLenum attachment,
                           GLenum textarget, GLuint texture,
                           GLint level, GLint zoffset)
{
   GET_CURRENT_CONTEXT(ctx);

   if ((texture != 0) && (textarget != GL_TEXTURE_3D)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFramebufferTexture3DEXT(textarget)");
      return;
   }

   framebuffer_texture(ctx, "3D", target, attachment, textarget, texture,
                       level, zoffset, GL_FALSE);
}


void GLAPIENTRY
_mesa_FramebufferTextureLayer(GLenum target, GLenum attachment,
                              GLuint texture, GLint level, GLint layer)
{
   GET_CURRENT_CONTEXT(ctx);

   framebuffer_texture(ctx, "Layer", target, attachment, 0, texture,
                       level, layer, GL_FALSE);
}


void GLAPIENTRY
_mesa_FramebufferTexture(GLenum target, GLenum attachment,
                         GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   if (_mesa_has_geometry_shaders(ctx)) {
      framebuffer_texture(ctx, "Layer", target, attachment, 0, texture,
                          level, 0, GL_TRUE);
   } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "unsupported function (glFramebufferTexture) called");
   }
}


void GLAPIENTRY
_mesa_FramebufferRenderbuffer(GLenum target, GLenum attachment,
                              GLenum renderbufferTarget,
                              GLuint renderbuffer)
{
   struct gl_renderbuffer_attachment *att;
   struct gl_framebuffer *fb;
   struct gl_renderbuffer *rb;
   GET_CURRENT_CONTEXT(ctx);

   fb = get_framebuffer_target(ctx, target);
   if (!fb) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(target)");
      return;
   }

   if (renderbufferTarget != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(renderbufferTarget)");
      return;
   }

   if (_mesa_is_winsys_fbo(fb)) {
      /* Can't attach new renderbuffers to a window system framebuffer */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glFramebufferRenderbufferEXT");
      return;
   }

   att = get_attachment(ctx, fb, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(invalid attachment %s)",
                  _mesa_lookup_enum_by_nr(attachment));
      return;
   }

   if (renderbuffer) {
      rb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
      if (!rb) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glFramebufferRenderbufferEXT(non-existant"
                     " renderbuffer %u)", renderbuffer);
	 return;
      }
      else if (rb == &DummyRenderbuffer) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glFramebufferRenderbufferEXT(renderbuffer %u)",
                     renderbuffer);
	 return;
      }
   }
   else {
      /* remove renderbuffer attachment */
      rb = NULL;
   }

   if (attachment == GL_DEPTH_STENCIL_ATTACHMENT &&
       rb && rb->Format != MESA_FORMAT_NONE) {
      /* make sure the renderbuffer is a depth/stencil format */
      const GLenum baseFormat = _mesa_get_format_base_format(rb->Format);
      if (baseFormat != GL_DEPTH_STENCIL) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glFramebufferRenderbufferEXT(renderbuffer"
                     " is not DEPTH_STENCIL format)");
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   assert(ctx->Driver.FramebufferRenderbuffer);
   ctx->Driver.FramebufferRenderbuffer(ctx, fb, attachment, rb);

   /* Some subsequent GL commands may depend on the framebuffer's visual
    * after the binding is updated.  Update visual info now.
    */
   _mesa_update_framebuffer_visual(ctx, fb);
}


void GLAPIENTRY
_mesa_GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment,
                                          GLenum pname, GLint *params)
{
   const struct gl_renderbuffer_attachment *att;
   struct gl_framebuffer *buffer;
   GLenum err;
   GET_CURRENT_CONTEXT(ctx);

   /* The error differs in GL and GLES. */
   err = _mesa_is_desktop_gl(ctx) ? GL_INVALID_OPERATION : GL_INVALID_ENUM;

   buffer = get_framebuffer_target(ctx, target);
   if (!buffer) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(target)");
      return;
   }

   if (_mesa_is_winsys_fbo(buffer)) {
      /* Page 126 (page 136 of the PDF) of the OpenGL ES 2.0.25 spec
       * says:
       *
       *     "If the framebuffer currently bound to target is zero, then
       *     INVALID_OPERATION is generated."
       *
       * The EXT_framebuffer_object spec has the same wording, and the
       * OES_framebuffer_object spec refers to the EXT_framebuffer_object
       * spec.
       */
      if ((!_mesa_is_desktop_gl(ctx) ||
           !ctx->Extensions.ARB_framebuffer_object)
          && !_mesa_is_gles3(ctx)) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glGetFramebufferAttachmentParameteriv(bound FBO = 0)");
	 return;
      }

      if (_mesa_is_gles3(ctx) && attachment != GL_BACK &&
          attachment != GL_DEPTH && attachment != GL_STENCIL) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetFramebufferAttachmentParameteriv(attachment)");
         return;
      }
      /* the default / window-system FBO */
      att = _mesa_get_fb0_attachment(ctx, buffer, attachment);
   }
   else {
      /* user-created framebuffer FBO */
      att = get_attachment(ctx, buffer, attachment);
   }

   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(attachment)");
      return;
   }

   if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
      const struct gl_renderbuffer_attachment *depthAtt, *stencilAtt;
      if (pname == GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE) {
         /* This behavior is first specified in OpenGL 4.4 specification.
          *
          * From the OpenGL 4.4 spec page 275:
          *   "This query cannot be performed for a combined depth+stencil
          *    attachment, since it does not have a single format."
          */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetFramebufferAttachmentParameteriv("
                     "GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE"
                     " is invalid for depth+stencil attachment)");
         return;
      }
      /* the depth and stencil attachments must point to the same buffer */
      depthAtt = get_attachment(ctx, buffer, GL_DEPTH_ATTACHMENT);
      stencilAtt = get_attachment(ctx, buffer, GL_STENCIL_ATTACHMENT);
      if (depthAtt->Renderbuffer != stencilAtt->Renderbuffer) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetFramebufferAttachmentParameterivEXT(DEPTH/STENCIL"
                     " attachments differ)");
         return;
      }
   }

   /* No need to flush here */

   switch (pname) {
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT:
      *params = _mesa_is_winsys_fbo(buffer)
         ? GL_FRAMEBUFFER_DEFAULT : att->Type;
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT:
      if (att->Type == GL_RENDERBUFFER_EXT) {
	 *params = att->Renderbuffer->Name;
      }
      else if (att->Type == GL_TEXTURE) {
	 *params = att->Texture->Name;
      }
      else {
         assert(att->Type == GL_NONE);
         if (_mesa_is_desktop_gl(ctx) || _mesa_is_gles3(ctx)) {
            *params = 0;
         } else {
            goto invalid_pname_enum;
         }
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT:
      if (att->Type == GL_TEXTURE) {
	 *params = att->TextureLevel;
      }
      else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      else {
         goto invalid_pname_enum;
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT:
      if (att->Type == GL_TEXTURE) {
         if (att->Texture && att->Texture->Target == GL_TEXTURE_CUBE_MAP) {
            *params = GL_TEXTURE_CUBE_MAP_POSITIVE_X + att->CubeMapFace;
         }
         else {
            *params = 0;
         }
      }
      else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      else {
         goto invalid_pname_enum;
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT:
      if (ctx->API == API_OPENGLES) {
         goto invalid_pname_enum;
      } else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameterivEXT(pname)");
      } else if (att->Type == GL_TEXTURE) {
         if (att->Texture && att->Texture->Target == GL_TEXTURE_3D) {
            *params = att->Zoffset;
         }
         else {
            *params = 0;
         }
      }
      else {
         goto invalid_pname_enum;
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING:
      if ((!_mesa_is_desktop_gl(ctx) ||
           !ctx->Extensions.ARB_framebuffer_object)
          && !_mesa_is_gles3(ctx)) {
         goto invalid_pname_enum;
      }
      else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      else {
         if (ctx->Extensions.EXT_framebuffer_sRGB) {
            *params =
               _mesa_get_format_color_encoding(att->Renderbuffer->Format);
         }
         else {
            /* According to ARB_framebuffer_sRGB, we should return LINEAR
             * if the sRGB conversion is unsupported. */
            *params = GL_LINEAR;
         }
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE:
      if ((ctx->API != API_OPENGL_COMPAT ||
           !ctx->Extensions.ARB_framebuffer_object)
          && ctx->API != API_OPENGL_CORE
          && !_mesa_is_gles3(ctx)) {
         goto invalid_pname_enum;
      }
      else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      else {
         mesa_format format = att->Renderbuffer->Format;

         /* Page 235 (page 247 of the PDF) in section 6.1.13 of the OpenGL ES
          * 3.0.1 spec says:
          *
          *     "If pname is FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE.... If
          *     attachment is DEPTH_STENCIL_ATTACHMENT the query will fail and
          *     generate an INVALID_OPERATION error.
          */
         if (_mesa_is_gles3(ctx) &&
             attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetFramebufferAttachmentParameteriv(cannot query "
                        "GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE of "
                        "GL_DEPTH_STENCIL_ATTACHMENT");
            return;
         }

         if (format == MESA_FORMAT_S_UINT8) {
            /* special cases */
            *params = GL_INDEX;
         }
         else if (format == MESA_FORMAT_Z32_FLOAT_S8X24_UINT) {
            /* depends on the attachment parameter */
            if (attachment == GL_STENCIL_ATTACHMENT) {
               *params = GL_INDEX;
            }
            else {
               *params = GL_FLOAT;
            }
         }
         else {
            *params = _mesa_get_format_datatype(format);
         }
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
   case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
   case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
   case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
   case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
   case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
      if ((!_mesa_is_desktop_gl(ctx) ||
           !ctx->Extensions.ARB_framebuffer_object)
          && !_mesa_is_gles3(ctx)) {
         goto invalid_pname_enum;
      }
      else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      else if (att->Texture) {
         const struct gl_texture_image *texImage =
            _mesa_select_tex_image(ctx, att->Texture, att->Texture->Target,
                                   att->TextureLevel);
         if (texImage) {
            *params = get_component_bits(pname, texImage->_BaseFormat,
                                         texImage->TexFormat);
         }
         else {
            *params = 0;
         }
      }
      else if (att->Renderbuffer) {
         *params = get_component_bits(pname, att->Renderbuffer->_BaseFormat,
                                      att->Renderbuffer->Format);
      }
      else {
         _mesa_problem(ctx, "glGetFramebufferAttachmentParameterivEXT:"
                       " invalid FBO attachment structure");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_LAYERED:
      if (!_mesa_has_geometry_shaders(ctx)) {
         goto invalid_pname_enum;
      } else if (att->Type == GL_TEXTURE) {
         *params = att->Layered;
      } else if (att->Type == GL_NONE) {
         _mesa_error(ctx, err,
                     "glGetFramebufferAttachmentParameteriv(pname)");
      } else {
         goto invalid_pname_enum;
      }
      return;
   default:
      goto invalid_pname_enum;
   }

   return;

invalid_pname_enum:
   _mesa_error(ctx, GL_INVALID_ENUM,
               "glGetFramebufferAttachmentParameteriv(pname)");
   return;
}


static void
invalidate_framebuffer_storage(GLenum target, GLsizei numAttachments,
                               const GLenum *attachments, GLint x, GLint y,
                               GLsizei width, GLsizei height, const char *name)
{
   int i;
   struct gl_framebuffer *fb;
   GET_CURRENT_CONTEXT(ctx);

   fb = get_framebuffer_target(ctx, target);
   if (!fb) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", name);
      return;
   }

   if (numAttachments < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(numAttachments < 0)", name);
      return;
   }

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "If an attachment is specified that does not exist in the
    *     framebuffer bound to <target>, it is ignored."
    *
    * It also says:
    *
    *     "If <attachments> contains COLOR_ATTACHMENTm and m is greater than
    *     or equal to the value of MAX_COLOR_ATTACHMENTS, then the error
    *     INVALID_OPERATION is generated."
    *
    * No mention is made of GL_AUXi being out of range.  Therefore, we allow
    * any enum that can be allowed by the API (OpenGL ES 3.0 has a different
    * set of retrictions).
    */
   for (i = 0; i < numAttachments; i++) {
      if (_mesa_is_winsys_fbo(fb)) {
         switch (attachments[i]) {
         case GL_ACCUM:
         case GL_AUX0:
         case GL_AUX1:
         case GL_AUX2:
         case GL_AUX3:
            /* Accumulation buffers and auxilary buffers were removed in
             * OpenGL 3.1, and they never existed in OpenGL ES.
             */
            if (ctx->API != API_OPENGL_COMPAT)
               goto invalid_enum;
            break;
         case GL_COLOR:
         case GL_DEPTH:
         case GL_STENCIL:
            break;
         case GL_BACK_LEFT:
         case GL_BACK_RIGHT:
         case GL_FRONT_LEFT:
         case GL_FRONT_RIGHT:
            if (!_mesa_is_desktop_gl(ctx))
               goto invalid_enum;
            break;
         default:
            goto invalid_enum;
         }
      } else {
         switch (attachments[i]) {
         case GL_DEPTH_ATTACHMENT:
         case GL_STENCIL_ATTACHMENT:
            break;
         case GL_COLOR_ATTACHMENT0:
         case GL_COLOR_ATTACHMENT1:
         case GL_COLOR_ATTACHMENT2:
         case GL_COLOR_ATTACHMENT3:
         case GL_COLOR_ATTACHMENT4:
         case GL_COLOR_ATTACHMENT5:
         case GL_COLOR_ATTACHMENT6:
         case GL_COLOR_ATTACHMENT7:
         case GL_COLOR_ATTACHMENT8:
         case GL_COLOR_ATTACHMENT9:
         case GL_COLOR_ATTACHMENT10:
         case GL_COLOR_ATTACHMENT11:
         case GL_COLOR_ATTACHMENT12:
         case GL_COLOR_ATTACHMENT13:
         case GL_COLOR_ATTACHMENT14:
         case GL_COLOR_ATTACHMENT15: {
            unsigned k = attachments[i] - GL_COLOR_ATTACHMENT0;
            if (k >= ctx->Const.MaxColorAttachments) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "%s(attachment >= max. color attachments)", name);
               return;
            }
            break;
         }
         default:
            goto invalid_enum;
         }
      }
   }

   /* We don't actually do anything for this yet.  Just return after
    * validating the parameters and generating the required errors.
    */
   return;

invalid_enum:
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(attachment)", name);
   return;
}


void GLAPIENTRY
_mesa_InvalidateSubFramebuffer(GLenum target, GLsizei numAttachments,
                               const GLenum *attachments, GLint x, GLint y,
                               GLsizei width, GLsizei height)
{
   invalidate_framebuffer_storage(target, numAttachments, attachments,
                                  x, y, width, height,
                                  "glInvalidateSubFramebuffer");
}


void GLAPIENTRY
_mesa_InvalidateFramebuffer(GLenum target, GLsizei numAttachments,
                            const GLenum *attachments)
{
   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "The command
    *
    *        void InvalidateFramebuffer(enum target,
    *                                   sizei numAttachments,
    *                                   const enum *attachments);
    *
    *     is equivalent to the command InvalidateSubFramebuffer with <x>, <y>,
    *     <width>, <height> equal to 0, 0, <MAX_VIEWPORT_DIMS[0]>,
    *     <MAX_VIEWPORT_DIMS[1]> respectively."
    */
   invalidate_framebuffer_storage(target, numAttachments, attachments,
                                  0, 0,
                                  MAX_VIEWPORT_WIDTH, MAX_VIEWPORT_HEIGHT,
                                  "glInvalidateFramebuffer");
}


void GLAPIENTRY
_mesa_DiscardFramebufferEXT(GLenum target, GLsizei numAttachments,
                            const GLenum *attachments)
{
   struct gl_framebuffer *fb;
   GLint i;

   GET_CURRENT_CONTEXT(ctx);

   fb = get_framebuffer_target(ctx, target);
   if (!fb) {
      _mesa_error(ctx, GL_INVALID_ENUM,
         "glDiscardFramebufferEXT(target %s)",
         _mesa_lookup_enum_by_nr(target));
      return;
   }

   if (numAttachments < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDiscardFramebufferEXT(numAttachments < 0)");
      return;
   }

   for (i = 0; i < numAttachments; i++) {
      switch (attachments[i]) {
      case GL_COLOR:
      case GL_DEPTH:
      case GL_STENCIL:
         if (_mesa_is_user_fbo(fb))
            goto invalid_enum;
         break;
      case GL_COLOR_ATTACHMENT0:
      case GL_DEPTH_ATTACHMENT:
      case GL_STENCIL_ATTACHMENT:
         if (_mesa_is_winsys_fbo(fb))
            goto invalid_enum;
         break;
      default:
         goto invalid_enum;
      }
   }

   if (ctx->Driver.DiscardFramebuffer)
      ctx->Driver.DiscardFramebuffer(ctx, target, numAttachments, attachments);

   return;

invalid_enum:
   _mesa_error(ctx, GL_INVALID_ENUM,
               "glDiscardFramebufferEXT(attachment %s)",
              _mesa_lookup_enum_by_nr(attachments[i]));
}
