/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 Intel Corporation.  All Rights Reserved.
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

#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "imports.h"
#include "macros.h"
#include "teximage.h"
#include "texobj.h"
#include "fbobject.h"
#include "buffers.h"
#include "state.h"
#include "mtypes.h"
#include "meta.h"

/* This function makes a texture view without bothering with all of the API
 * checks.  Most of them are the same for CopyTexSubImage so checking would
 * be redundant.  The one major difference is that we don't check for
 * whether the texture is immutable or not.  However, since the view will
 * be created and then immediately destroyed, this should not be a problem.
 */
static bool
make_view(struct gl_context *ctx, struct gl_texture_image *tex_image,
          struct gl_texture_image **view_tex_image, GLuint *view_tex_name,
          GLenum internal_format)
{
   struct gl_texture_object *tex_obj = tex_image->TexObject;
   struct gl_texture_object *view_tex_obj;
   mesa_format tex_format;

   /* Set up the new texture object */
   _mesa_GenTextures(1, view_tex_name);
   view_tex_obj = _mesa_lookup_texture(ctx, *view_tex_name);
   if (!view_tex_obj)
      return false;

   tex_format = _mesa_choose_texture_format(ctx, view_tex_obj, tex_obj->Target,
                                           0, internal_format,
                                           GL_NONE, GL_NONE);

   if (!ctx->Driver.TestProxyTexImage(ctx, tex_obj->Target, 0, tex_format,
                                      tex_image->Width, tex_image->Height,
                                      tex_image->Depth, 0)) {
      _mesa_DeleteTextures(1, view_tex_name);
      *view_tex_name = 0;
      return false;
   }

   view_tex_obj->Target = tex_obj->Target;

   *view_tex_image = _mesa_get_tex_image(ctx, view_tex_obj, tex_obj->Target, 0);

   if (!*view_tex_image) {
      _mesa_DeleteTextures(1, view_tex_name);
      *view_tex_name = 0;
      return false;
   }

   _mesa_init_teximage_fields(ctx, *view_tex_image,
                              tex_image->Width, tex_image->Height,
                              tex_image->Depth,
                              0, internal_format, tex_format);

   view_tex_obj->MinLevel = tex_image->Level;
   view_tex_obj->NumLevels = 1;
   view_tex_obj->MinLayer = tex_obj->MinLayer;
   view_tex_obj->NumLayers = tex_obj->NumLayers;
   view_tex_obj->Immutable = tex_obj->Immutable;
   view_tex_obj->ImmutableLevels = tex_obj->ImmutableLevels;
   view_tex_obj->Target = tex_obj->Target;

   if (ctx->Driver.TextureView != NULL &&
       !ctx->Driver.TextureView(ctx, view_tex_obj, tex_obj)) {
      _mesa_DeleteTextures(1, view_tex_name);
      *view_tex_name = 0;
      return false; /* driver recorded error */
   }

   return true;
}

/** A partial implementation of glCopyImageSubData
 *
 * This is a partial implementation of glCopyImageSubData that works only
 * if both textures are uncompressed and the destination texture is
 * renderable.  It uses a slight abuse of a texture view (see make_view) to
 * turn the source texture into the destination texture type and then uses
 * _mesa_meta_BlitFramebuffers to do the copy.
 */
bool
_mesa_meta_CopyImageSubData_uncompressed(struct gl_context *ctx,
                                         struct gl_texture_image *src_tex_image,
                                         int src_x, int src_y, int src_z,
                                         struct gl_texture_image *dst_tex_image,
                                         int dst_x, int dst_y, int dst_z,
                                         int src_width, int src_height)
{
   GLuint src_view_texture = 0;
   struct gl_texture_image *src_view_tex_image;
   GLuint fbos[2];
   bool success = false;
   GLbitfield mask;
   GLenum status, attachment;

   if (_mesa_is_format_compressed(dst_tex_image->TexFormat))
      return false;

   if (_mesa_is_format_compressed(src_tex_image->TexFormat))
      return false;

   if (src_tex_image->InternalFormat == dst_tex_image->InternalFormat) {
      src_view_tex_image = src_tex_image;
   } else {
      if (!make_view(ctx, src_tex_image, &src_view_tex_image, &src_view_texture,
                     dst_tex_image->InternalFormat))
         goto cleanup;
   }

   /* We really only need to stash the bound framebuffers. */
   _mesa_meta_begin(ctx, 0);

   _mesa_GenFramebuffers(2, fbos);
   _mesa_BindFramebuffer(GL_READ_FRAMEBUFFER, fbos[0]);
   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[1]);

   switch (_mesa_get_format_base_format(src_tex_image->TexFormat)) {
   case GL_DEPTH_COMPONENT:
      attachment = GL_DEPTH_ATTACHMENT;
      mask = GL_DEPTH_BUFFER_BIT;
      break;
   case GL_DEPTH_STENCIL:
      attachment = GL_DEPTH_STENCIL_ATTACHMENT;
      mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
      break;
   case GL_STENCIL_INDEX:
      attachment = GL_STENCIL_ATTACHMENT;
      mask = GL_STENCIL_BUFFER_BIT;
      break;
   default:
      attachment = GL_COLOR_ATTACHMENT0;
      mask = GL_COLOR_BUFFER_BIT;
      _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0);
      _mesa_ReadBuffer(GL_COLOR_ATTACHMENT0);
   }

   _mesa_meta_bind_fbo_image(GL_READ_FRAMEBUFFER, attachment,
                             src_view_tex_image, src_z);

   status = _mesa_CheckFramebufferStatus(GL_READ_FRAMEBUFFER);
   if (status != GL_FRAMEBUFFER_COMPLETE)
      goto meta_end;

   _mesa_meta_bind_fbo_image(GL_DRAW_FRAMEBUFFER, attachment,
                             dst_tex_image, dst_z);

   status = _mesa_CheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
   if (status != GL_FRAMEBUFFER_COMPLETE)
      goto meta_end;

   /* Since we've bound a new draw framebuffer, we need to update its
    * derived state -- _Xmin, etc -- for BlitFramebuffer's clipping to
    * be correct.
    */
   _mesa_update_state(ctx);

   /* We skip the core BlitFramebuffer checks for format consistency.
    * We have already created views to ensure that the texture formats
    * match.
    */
   ctx->Driver.BlitFramebuffer(ctx, src_x, src_y,
                               src_x + src_width, src_y + src_height,
                               dst_x, dst_y,
                               dst_x + src_width, dst_y + src_height,
                               mask, GL_NEAREST);

   success = true;

meta_end:
   _mesa_DeleteFramebuffers(2, fbos);
   _mesa_meta_end(ctx);

cleanup:
   _mesa_DeleteTextures(1, &src_view_texture);

   return success;
}
