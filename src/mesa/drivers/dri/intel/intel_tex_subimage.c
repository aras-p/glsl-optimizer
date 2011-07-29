
/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/mtypes.h"
#include "main/pbo.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texcompress.h"
#include "main/enums.h"

#include "intel_context.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static void
intelTexSubimage(struct gl_context * ctx,
                 GLint dims,
                 GLenum target, GLint level,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLint width, GLint height, GLint depth,
                 GLsizei imageSize,
                 GLenum format, GLenum type, const void *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLuint dstRowStride = 0;
   drm_intel_bo *temp_bo = NULL, *dst_bo = NULL;
   unsigned int blit_x = 0, blit_y = 0;

   DBG("%s target %s level %d offset %d,%d %dx%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       level, xoffset, yoffset, width, height);

   intel_flush(ctx);

   pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
					format, type, pixels, packing,
					"glTexSubImage");
   if (!pixels)
      return;

   intel_prepare_render(intel);

   /* Map buffer if necessary.  Need to lock to prevent other contexts
    * from uploading the buffer under us.
    */
   if (intelImage->mt) {
      dst_bo = intel_region_buffer(intel, intelImage->mt->region,
				   INTEL_WRITE_PART);

      if (intelImage->mt->region->tiling != I915_TILING_Y &&
	  intel->gen < 6 && target == GL_TEXTURE_2D &&
	  drm_intel_bo_busy(dst_bo))
      {
	 unsigned long pitch;
	 uint32_t tiling_mode = I915_TILING_NONE;

	 temp_bo = drm_intel_bo_alloc_tiled(intel->bufmgr,
					    "subimage blit bo",
					    width, height,
					    intelImage->mt->cpp,
					    &tiling_mode,
					    &pitch,
					    0);
         if (temp_bo == NULL)
            return;

	 if (drm_intel_gem_bo_map_gtt(temp_bo)) {
            drm_intel_bo_unreference(temp_bo);
            return;
         }

	 texImage->Data = temp_bo->virtual;
	 texImage->ImageOffsets[0] = 0;
	 dstRowStride = pitch;

	 intel_miptree_get_image_offset(intelImage->mt, level,
					intelImage->base.Face, 0,
					&blit_x, &blit_y);
	 blit_x += xoffset;
	 blit_y += yoffset;
	 xoffset = 0;
	 yoffset = 0;
      } else {
	 texImage->Data = intel_miptree_image_map(intel,
						  intelImage->mt,
						  intelImage->base.Face,
						  intelImage->base.Level,
						  &dstRowStride,
						  texImage->ImageOffsets);
      }
   } else {
      dstRowStride = texImage->RowStride * _mesa_get_format_bytes(texImage->TexFormat);
   }

   assert(dstRowStride);

   if (!_mesa_texstore(ctx, dims, texImage->_BaseFormat,
		       texImage->TexFormat,
		       texImage->Data,
		       xoffset, yoffset, zoffset,
		       dstRowStride,
		       texImage->ImageOffsets,
		       width, height, depth,
		       format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

   if (temp_bo) {
      GLboolean ret;
      unsigned int dst_pitch = intelImage->mt->region->pitch *
	 intelImage->mt->cpp;

      drm_intel_gem_bo_unmap_gtt(temp_bo);
      texImage->Data = NULL;

      ret = intelEmitCopyBlit(intel,
			      intelImage->mt->cpp,
			      dstRowStride / intelImage->mt->cpp,
			      temp_bo, 0, GL_FALSE,
			      dst_pitch / intelImage->mt->cpp, dst_bo, 0,
			      intelImage->mt->region->tiling,
			      0, 0, blit_x, blit_y, width, height,
			      GL_COPY);
      assert(ret);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);

   if (temp_bo) {
      drm_intel_bo_unreference(temp_bo);
      temp_bo = NULL;
   } else if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      texImage->Data = NULL;
   }
}


static void
intelTexSubImage3D(struct gl_context * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   intelTexSubimage(ctx, 3,
                    target, level,
                    xoffset, yoffset, zoffset,
                    width, height, depth, 0,
                    format, type, pixels, packing, texObj, texImage);
}


static void
intelTexSubImage2D(struct gl_context * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset, GLint yoffset,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   intelTexSubimage(ctx, 2,
                    target, level,
                    xoffset, yoffset, 0,
                    width, height, 1, 0,
                    format, type, pixels, packing, texObj, texImage);
}


static void
intelTexSubImage1D(struct gl_context * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset,
                   GLsizei width,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   intelTexSubimage(ctx, 1,
                    target, level,
                    xoffset, 0, 0,
                    width, 1, 1, 0,
                    format, type, pixels, packing, texObj, texImage);
}

void
intelInitTextureSubImageFuncs(struct dd_function_table *functions)
{
   functions->TexSubImage1D = intelTexSubImage1D;
   functions->TexSubImage2D = intelTexSubImage2D;
   functions->TexSubImage3D = intelTexSubImage3D;
}
