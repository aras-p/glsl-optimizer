/*
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "main/glheader.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"
#include "main/teximage.h"
#include "main/texenv.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/texparam.h"
#include "main/varray.h"
#include "main/attrib.h"
#include "main/enable.h"
#include "main/buffers.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/depth.h"
#include "main/hash.h"
#include "main/mipmap.h"
#include "main/blend.h"
#include "glapi/dispatch.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_pixel.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"

static const char *intel_fp_tex2d =
      "!!ARBfp1.0\n"
      "TEX result.color, fragment.texcoord[0], texture[0], 2D;\n"
      "END\n";

static GLboolean
intel_generate_mipmap_level(GLcontext *ctx, GLuint tex_name,
			    int level, int width, int height)
{
   struct intel_context *intel = intel_context(ctx);
   GLfloat vertices[4][2];
   GLint status;

   /* Set to source from the previous level */
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, level - 1);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1);

   /* Set to draw into the current level */
   _mesa_FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
				 GL_COLOR_ATTACHMENT0_EXT,
				 GL_TEXTURE_2D,
				 tex_name,
				 level);
   /* Choose to render to the color attachment. */
   _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

   status = _mesa_CheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
      return GL_FALSE;

   meta_set_passthrough_transform(&intel->meta);

   /* XXX: Doing it right would involve setting up the transformation to do
    * 0-1 mapping or something, and not changing the vertex data.
    */
   vertices[0][0] = 0;
   vertices[0][1] = 0;
   vertices[1][0] = width;
   vertices[1][1] = 0;
   vertices[2][0] = width;
   vertices[2][1] = height;
   vertices[3][0] = 0;
   vertices[3][1] = height;

   _mesa_VertexPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), &vertices);
   _mesa_Enable(GL_VERTEX_ARRAY);
   meta_set_default_texrect(&intel->meta);

   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   meta_restore_texcoords(&intel->meta);
   meta_restore_transform(&intel->meta);

   return GL_TRUE;
}

static GLboolean
intel_generate_mipmap_2d(GLcontext *ctx,
			 GLenum target,
			 struct gl_texture_object *texObj)
{
   struct intel_context *intel = intel_context(ctx);
   GLint old_active_texture;
   int level, max_levels, start_level, end_level;
   GLuint fb_name;
   GLboolean success = GL_FALSE;
   struct gl_framebuffer *saved_fbo = NULL;
   struct gl_buffer_object *saved_array_buffer = NULL;
   struct gl_buffer_object *saved_element_buffer = NULL;

   _mesa_PushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT |
		    GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
		    GL_DEPTH_BUFFER_BIT);
   _mesa_PushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   old_active_texture = ctx->Texture.CurrentUnit;
   _mesa_reference_framebuffer(&saved_fbo, ctx->DrawBuffer);

   /* use default array/index buffers */
   _mesa_reference_buffer_object(ctx, &saved_array_buffer,
                                 ctx->Array.ArrayBufferObj);
   _mesa_reference_buffer_object(ctx, &ctx->Array.ArrayBufferObj,
                                 ctx->Shared->NullBufferObj);   
   _mesa_reference_buffer_object(ctx, &saved_element_buffer,
                                 ctx->Array.ElementArrayBufferObj);
   _mesa_reference_buffer_object(ctx, &ctx->Array.ElementArrayBufferObj,
                                 ctx->Shared->NullBufferObj);   

   _mesa_Disable(GL_POLYGON_STIPPLE);
   _mesa_Disable(GL_DEPTH_TEST);
   _mesa_Disable(GL_STENCIL_TEST);
   _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   _mesa_DepthMask(GL_FALSE);

   /* Bind the given texture to GL_TEXTURE_2D with linear filtering for our
    * minification.
    */
   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB);
   _mesa_Enable(GL_TEXTURE_2D);
   _mesa_BindTexture(GL_TEXTURE_2D, texObj->Name);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		       GL_LINEAR_MIPMAP_NEAREST);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   /* Bind the new renderbuffer to the color attachment point. */
   _mesa_GenFramebuffersEXT(1, &fb_name);
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_name);

   meta_set_fragment_program(&intel->meta, &intel->meta.tex2d_fp,
			     intel_fp_tex2d);
   meta_set_passthrough_vertex_program(&intel->meta);

   max_levels = _mesa_max_texture_levels(ctx, texObj->Target);
   start_level = texObj->BaseLevel;
   end_level = texObj->MaxLevel;

   /* Loop generating level+1 from level. */
   for (level = start_level; level < end_level && level < max_levels - 1; level++) {
      const struct gl_texture_image *srcImage;
      int width, height;

      srcImage = _mesa_select_tex_image(ctx, texObj, target, level);
      if (srcImage->Border != 0)
	 goto fail;

      width = srcImage->Width / 2;
      if (width < 1)
	 width = 1;
      height = srcImage->Height / 2;
      if (height < 1)
	 height = 1;

      if (width == srcImage->Width &&
	  height == srcImage->Height) {
	 /* Neither _mesa_max_texture_levels nor texObj->MaxLevel are the
	  * maximum texture level for the object, so break out when we've gone
	  * over the edge.
	  */
	 break;
      }

      /* Make sure that there's space allocated for the target level.
       * We could skip this if there's already space allocated and save some
       * time.
       */
      _mesa_TexImage2D(GL_TEXTURE_2D, level + 1, srcImage->InternalFormat,
		       width, height, 0,
		       GL_RGBA, GL_UNSIGNED_INT, NULL);

      if (!intel_generate_mipmap_level(ctx, texObj->Name, level + 1,
				       width, height))
	 goto fail;
   }

   success = GL_TRUE;

fail:
   meta_restore_fragment_program(&intel->meta);
   meta_restore_vertex_program(&intel->meta);

   /* restore array/index buffers */
   _mesa_reference_buffer_object(ctx, &ctx->Array.ArrayBufferObj,
                                 saved_array_buffer);
   _mesa_reference_buffer_object(ctx, &saved_array_buffer, NULL);
   _mesa_reference_buffer_object(ctx, &ctx->Array.ElementArrayBufferObj,
                                 saved_element_buffer);
   _mesa_reference_buffer_object(ctx, &saved_element_buffer, NULL);


   _mesa_DeleteFramebuffersEXT(1, &fb_name);
   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB + old_active_texture);
   if (saved_fbo)
      _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, saved_fbo->Name);
   _mesa_reference_framebuffer(&saved_fbo, NULL);
   _mesa_PopClientAttrib();
   _mesa_PopAttrib();

   return success;
}


/**
 * Generate new mipmap data from BASE+1 to BASE+p (the minimally-sized mipmap
 * level).
 *
 * The texture object's miptree must be mapped.
 *
 * It would be really nice if this was just called by Mesa whenever mipmaps
 * needed to be regenerated, rather than us having to remember to do so in
 * each texture image modification path.
 *
 * This function should also include an accelerated path.
 */
void
intel_generate_mipmap(GLcontext *ctx, GLenum target,
                      struct gl_texture_object *texObj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   int face, i;

   /* HW path */
   if (target == GL_TEXTURE_2D &&
       ctx->Extensions.EXT_framebuffer_object &&
       ctx->Extensions.ARB_fragment_program &&
       ctx->Extensions.ARB_vertex_program) {
      GLboolean success;

      /* We'll be accessing this texture using GL entrypoints, which should
       * be resilient against other access to this texture.
       */
      _mesa_unlock_texture(ctx, texObj);
      success = intel_generate_mipmap_2d(ctx, target, texObj);
      _mesa_lock_texture(ctx, texObj);

      if (success)
	 return;
   }

   /* SW path */
   intel_tex_map_level_images(intel, intelObj, texObj->BaseLevel);
   _mesa_generate_mipmap(ctx, target, texObj);
   intel_tex_unmap_level_images(intel, intelObj, texObj->BaseLevel);

   /* Update the level information in our private data in the new images, since
    * it didn't get set as part of a normal TexImage path.
    */
   for (face = 0; face < nr_faces; face++) {
      for (i = texObj->BaseLevel + 1; i < texObj->MaxLevel; i++) {
         struct intel_texture_image *intelImage;

	 intelImage = intel_texture_image(texObj->Image[face][i]);
	 if (intelImage == NULL)
	    break;

	 intelImage->level = i;
	 intelImage->face = face;
	 /* Unreference the miptree to signal that the new Data is a bare
	  * pointer from mesa.
	  */
	 intel_miptree_release(intel, &intelImage->mt);
      }
   }
}
