/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2011 VMware, Inc.
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

/**
 * Functions for mapping/unmapping texture images.
 */


#include "main/context.h"
#include "main/fbobject.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"


/**
 * Allocate a new swrast_texture_image (a subclass of gl_texture_image).
 * Called via ctx->Driver.NewTextureImage().
 */
struct gl_texture_image *
_swrast_new_texture_image( struct gl_context *ctx )
{
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(swrast_texture_image);
}


/**
 * Free a swrast_texture_image (a subclass of gl_texture_image).
 * Called via ctx->Driver.DeleteTextureImage().
 */
void
_swrast_delete_texture_image(struct gl_context *ctx,
                             struct gl_texture_image *texImage)
{
   /* Nothing special for the subclass yet */
   _mesa_delete_texture_image(ctx, texImage);
}

static unsigned int
texture_slices(struct gl_texture_image *texImage)
{
   if (texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY)
      return texImage->Height;
   else
      return texImage->Depth;
}

unsigned int
_swrast_teximage_slice_height(struct gl_texture_image *texImage)
{
   /* For 1D array textures, the slices are all 1 pixel high, and Height is
    * the number of slices.
    */
   if (texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY)
      return 1;
   else
      return texImage->Height;
}

/**
 * Called via ctx->Driver.AllocTextureImageBuffer()
 */
GLboolean
_swrast_alloc_texture_image_buffer(struct gl_context *ctx,
                                   struct gl_texture_image *texImage)
{
   struct swrast_texture_image *swImg = swrast_texture_image(texImage);
   GLuint bytesPerSlice;
   GLuint slices = texture_slices(texImage);
   GLuint i;

   if (!_swrast_init_texture_image(texImage))
      return GL_FALSE;

   bytesPerSlice = _mesa_format_image_size(texImage->TexFormat, texImage->Width,
                                           _swrast_teximage_slice_height(texImage), 1);

   assert(!swImg->Buffer);
   swImg->Buffer = _mesa_align_malloc(bytesPerSlice * slices, 512);
   if (!swImg->Buffer)
      return GL_FALSE;

   /* RowStride and ImageSlices[] describe how to address texels in 'Data' */
   swImg->RowStride = _mesa_format_row_stride(texImage->TexFormat,
                                              texImage->Width);

   for (i = 0; i < slices; i++) {
      swImg->ImageSlices[i] = swImg->Buffer + bytesPerSlice * i;
   }

   return GL_TRUE;
}


/**
 * Code that overrides ctx->Driver.AllocTextureImageBuffer may use this to
 * initialize the fields of swrast_texture_image without allocating the image
 * buffer or initializing RowStride or the contents of ImageSlices.
 *
 * Returns GL_TRUE on success, GL_FALSE on memory allocation failure.
 */
GLboolean
_swrast_init_texture_image(struct gl_texture_image *texImage)
{
   struct swrast_texture_image *swImg = swrast_texture_image(texImage);

   if ((texImage->Width == 1 || _mesa_is_pow_two(texImage->Width2)) &&
       (texImage->Height == 1 || _mesa_is_pow_two(texImage->Height2)) &&
       (texImage->Depth == 1 || _mesa_is_pow_two(texImage->Depth2)))
      swImg->_IsPowerOfTwo = GL_TRUE;
   else
      swImg->_IsPowerOfTwo = GL_FALSE;

   /* Compute Width/Height/DepthScale for mipmap lod computation */
   if (texImage->TexObject->Target == GL_TEXTURE_RECTANGLE_NV) {
      /* scale = 1.0 since texture coords directly map to texels */
      swImg->WidthScale = 1.0;
      swImg->HeightScale = 1.0;
      swImg->DepthScale = 1.0;
   }
   else {
      swImg->WidthScale = (GLfloat) texImage->Width;
      swImg->HeightScale = (GLfloat) texImage->Height;
      swImg->DepthScale = (GLfloat) texImage->Depth;
   }

   assert(!swImg->ImageSlices);
   swImg->ImageSlices = calloc(texture_slices(texImage), sizeof(void *));
   if (!swImg->ImageSlices)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Called via ctx->Driver.FreeTextureImageBuffer()
 */
void
_swrast_free_texture_image_buffer(struct gl_context *ctx,
                                  struct gl_texture_image *texImage)
{
   struct swrast_texture_image *swImage = swrast_texture_image(texImage);

   _mesa_align_free(swImage->Buffer);
   swImage->Buffer = NULL;

   free(swImage->ImageSlices);
   swImage->ImageSlices = NULL;
}


/**
 * Error checking for debugging only.
 */
static void
check_map_teximage(const struct gl_texture_image *texImage,
                   GLuint slice, GLuint x, GLuint y, GLuint w, GLuint h)
{

   if (texImage->TexObject->Target == GL_TEXTURE_1D)
      assert(y == 0 && h == 1);

   assert(x < texImage->Width || texImage->Width == 0);
   assert(y < texImage->Height || texImage->Height == 0);
   assert(x + w <= texImage->Width);
   assert(y + h <= texImage->Height);
}

/**
 * Map a 2D slice of a texture image into user space.
 * (x,y,w,h) defines a region of interest (ROI).  Reading/writing texels
 * outside of the ROI is undefined.
 *
 * \param texImage  the texture image
 * \param slice  the 3D image slice or array texture slice
 * \param x, y, w, h  region of interest
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 * \param mapOut  returns start of mapping of region of interest
 * \param rowStrideOut  returns row stride (in bytes)
 */
void
_swrast_map_teximage(struct gl_context *ctx,
                     struct gl_texture_image *texImage,
                     GLuint slice,
                     GLuint x, GLuint y, GLuint w, GLuint h,
                     GLbitfield mode,
                     GLubyte **mapOut,
                     GLint *rowStrideOut)
{
   struct swrast_texture_image *swImage = swrast_texture_image(texImage);
   GLubyte *map;
   GLint stride, texelSize;
   GLuint bw, bh;

   check_map_teximage(texImage, slice, x, y, w, h);

   if (!swImage->Buffer) {
      /* Either glTexImage was called with a NULL <pixels> argument or
       * we ran out of memory when allocating texture memory,
       */
      *mapOut = NULL;
      *rowStrideOut = 0;
      return;
   }

   texelSize = _mesa_get_format_bytes(texImage->TexFormat);
   stride = _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
   _mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);

   assert(x % bw == 0);
   assert(y % bh == 0);

   /* This function can only be used with a swrast-allocated buffer, in which
    * case ImageSlices is populated with pointers into Buffer.
    */
   assert(swImage->Buffer);
   assert(swImage->Buffer == swImage->ImageSlices[0]);

   assert(slice < texture_slices(texImage));
   map = swImage->ImageSlices[slice];

   /* apply x/y offset to map address */
   map += stride * (y / bh) + texelSize * (x / bw);

   *mapOut = map;
   *rowStrideOut = stride;
}

void
_swrast_unmap_teximage(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLuint slice)
{
   /* nop */
}


void
_swrast_map_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   const GLuint faces = _mesa_num_tex_faces(texObj->Target);
   GLuint face, level;

   for (face = 0; face < faces; face++) {
      for (level = texObj->BaseLevel; level < MAX_TEXTURE_LEVELS; level++) {
         struct gl_texture_image *texImage = texObj->Image[face][level];
         struct swrast_texture_image *swImage = swrast_texture_image(texImage);
         unsigned int i, slices;

         if (!texImage)
            continue;

         /* In the case of a swrast-allocated texture buffer, the ImageSlices
          * and RowStride are always available.
          */
         if (swImage->Buffer) {
            assert(swImage->ImageSlices[0] == swImage->Buffer);
            continue;
         }

         if (!swImage->ImageSlices) {
            swImage->ImageSlices =
               calloc(texture_slices(texImage), sizeof(void *));
            if (!swImage->ImageSlices)
               continue;
         }

         slices = texture_slices(texImage);

         for (i = 0; i < slices; i++) {
            GLubyte *map;
            GLint rowStride;

            if (swImage->ImageSlices[i])
               continue;

            ctx->Driver.MapTextureImage(ctx, texImage, i,
                                        0, 0,
                                        texImage->Width, texImage->Height,
                                        GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                                        &map, &rowStride);

            swImage->ImageSlices[i] = map;
            /* A swrast-using driver has to return the same rowstride for
             * every slice of the same texture, since we don't track them
             * separately.
             */
            if (i == 0)
               swImage->RowStride = rowStride;
            else
               assert(swImage->RowStride == rowStride);
         }
      }
   }
}


void
_swrast_unmap_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   const GLuint faces = _mesa_num_tex_faces(texObj->Target);
   GLuint face, level;

   for (face = 0; face < faces; face++) {
      for (level = texObj->BaseLevel; level < MAX_TEXTURE_LEVELS; level++) {
         struct gl_texture_image *texImage = texObj->Image[face][level];
         struct swrast_texture_image *swImage = swrast_texture_image(texImage);
         unsigned int i, slices;

         if (!texImage)
            continue;

         if (swImage->Buffer)
            return;

         if (!swImage->ImageSlices)
            continue;

         slices = texture_slices(texImage);

         for (i = 0; i < slices; i++) {
            if (swImage->ImageSlices[i]) {
               ctx->Driver.UnmapTextureImage(ctx, texImage, i);
               swImage->ImageSlices[i] = NULL;
            }
         }
      }
   }
}


/**
 * Map all textures for reading prior to software rendering.
 */
void
_swrast_map_textures(struct gl_context *ctx)
{
   int unit;

   for (unit = 0; unit <= ctx->Texture._MaxEnabledTexImageUnit; unit++) {
      struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;

      if (texObj)
         _swrast_map_texture(ctx, texObj);
   }
}


/**
 * Unmap all textures for reading prior to software rendering.
 */
void
_swrast_unmap_textures(struct gl_context *ctx)
{
   int unit;
   for (unit = 0; unit <= ctx->Texture._MaxEnabledTexImageUnit; unit++) {
      struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;

      if (texObj)
         _swrast_unmap_texture(ctx, texObj);
   }
}
