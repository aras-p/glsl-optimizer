
/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/bufferobj.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/pbo.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texcompress.h"
#include "main/enums.h"

#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static bool
intel_blit_texsubimage(struct gl_context * ctx,
		       struct gl_texture_image *texImage,
		       GLint xoffset, GLint yoffset,
		       GLint width, GLint height,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   /* Try to do a blit upload of the subimage if the texture is
    * currently busy.
    */
   if (!intelImage->mt)
      return false;

   /* The blitter can't handle Y tiling */
   if (intelImage->mt->region->tiling == I915_TILING_Y)
      return false;

   if (texImage->TexObject->Target != GL_TEXTURE_2D)
      return false;

   if (!drm_intel_bo_busy(intelImage->mt->region->bo))
      return false;

   DBG("BLT subimage %s target %s level %d offset %d,%d %dx%d\n",
       __FUNCTION__,
       _mesa_lookup_enum_by_nr(texImage->TexObject->Target),
       texImage->Level, xoffset, yoffset, width, height);

   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1,
					format, type, pixels, packing,
					"glTexSubImage");
   if (!pixels)
      return false;

   struct intel_mipmap_tree *temp_mt =
      intel_miptree_create(intel, GL_TEXTURE_2D, texImage->TexFormat,
                           0, 0,
                           width, height, 1,
                           false, INTEL_MIPTREE_TILING_NONE);
   if (!temp_mt)
      goto err;

   GLubyte *dst = intel_miptree_map_raw(intel, temp_mt);
   if (!dst)
      goto err;

   if (!_mesa_texstore(ctx, 2, texImage->_BaseFormat,
		       texImage->TexFormat,
		       temp_mt->region->pitch,
		       &dst,
		       width, height, 1,
		       format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

   intel_miptree_unmap_raw(intel, temp_mt);

   bool ret;

   ret = intel_miptree_blit(intel,
                            temp_mt, 0, 0,
                            0, 0, false,
                            intelImage->mt, texImage->Level, texImage->Face,
                            xoffset, yoffset, false,
                            width, height, GL_COPY);
   assert(ret);

   intel_miptree_release(&temp_mt);
   _mesa_unmap_teximage_pbo(ctx, packing);

   return ret;

err:
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   intel_miptree_release(&temp_mt);
   _mesa_unmap_teximage_pbo(ctx, packing);
   return false;
}

static void
intelTexSubImage(struct gl_context * ctx,
                 GLuint dims,
                 struct gl_texture_image *texImage,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLsizei width, GLsizei height, GLsizei depth,
                 GLenum format, GLenum type,
                 const GLvoid * pixels,
                 const struct gl_pixelstore_attrib *packing)
{
   /* The intel_blit_texsubimage() function only handles 2D images */
   if (dims != 2 || !intel_blit_texsubimage(ctx, texImage,
			       xoffset, yoffset,
			       width, height,
			       format, type, pixels, packing)) {
      _mesa_store_texsubimage(ctx, dims, texImage,
                              xoffset, yoffset, zoffset,
                              width, height, depth,
                              format, type, pixels, packing);
   }
}

void
intelInitTextureSubImageFuncs(struct dd_function_table *functions)
{
   functions->TexSubImage = intelTexSubImage;
}
