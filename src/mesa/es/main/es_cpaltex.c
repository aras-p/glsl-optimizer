/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


/**
 * Code to convert compressed/paletted texture images to ordinary images.
 * See the GL_OES_compressed_paletted_texture spec at
 * http://khronos.org/registry/gles/extensions/OES/OES_compressed_paletted_texture.txt
 *
 * XXX this makes it impossible to add hardware support...
 */


#include "GLES/gl.h"
#include "GLES/glext.h"

#include "main/compiler.h" /* for ASSERT */


void GL_APIENTRY _es_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

void GL_APIENTRY _mesa_GetIntegerv(GLenum pname, GLint *params);
void GL_APIENTRY _mesa_PixelStorei(GLenum pname, GLint param);
void GL_APIENTRY _mesa_TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void GL_APIENTRY _mesa_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

void *_mesa_get_current_context(void);
void _mesa_error(void *ctx, GLenum error, const char *fmtString, ... );


static const struct cpal_format_info {
   GLenum cpal_format;
   GLenum format;
   GLenum type;
   GLuint palette_size;
   GLuint size;
} formats[] = {
   { GL_PALETTE4_RGB8_OES,     GL_RGB,  GL_UNSIGNED_BYTE,           16, 3 },
   { GL_PALETTE4_RGBA8_OES,    GL_RGBA, GL_UNSIGNED_BYTE,           16, 4 },
   { GL_PALETTE4_R5_G6_B5_OES, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,    16, 2 },
   { GL_PALETTE4_RGBA4_OES,    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4,  16, 2 },
   { GL_PALETTE4_RGB5_A1_OES,  GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,  16, 2 },
   { GL_PALETTE8_RGB8_OES,     GL_RGB,  GL_UNSIGNED_BYTE,          256, 3 },
   { GL_PALETTE8_RGBA8_OES,    GL_RGBA, GL_UNSIGNED_BYTE,          256, 4 },
   { GL_PALETTE8_R5_G6_B5_OES, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,   256, 2 },
   { GL_PALETTE8_RGBA4_OES,    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 256, 2 },
   { GL_PALETTE8_RGB5_A1_OES,  GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 256, 2 }
};


/**
 * Get a color/entry from the palette.
 */
static GLuint
get_palette_entry(const struct cpal_format_info *info, const GLubyte *palette,
                  GLuint index, GLubyte *pixel)
{
   memcpy(pixel, palette + info->size * index, info->size);
   return info->size;
}


/**
 * Convert paletted texture to color texture.
 */
static void
paletted_to_color(const struct cpal_format_info *info, const GLubyte *palette,
                  const void *indices, GLuint num_pixels, GLubyte *image)
{
   GLubyte *pix = image;
   GLuint remain, i;

   if (info->palette_size == 16) {
      /* 4 bits per index */
      const GLubyte *ind = (const GLubyte *) indices;

      /* two pixels per iteration */
      remain = num_pixels % 2;
      for (i = 0; i < num_pixels / 2; i++) {
         pix += get_palette_entry(info, palette, (ind[i] >> 4) & 0xf, pix);
         pix += get_palette_entry(info, palette, ind[i] & 0xf, pix);
      }
      if (remain) {
         get_palette_entry(info, palette, (ind[i] >> 4) & 0xf, pix);
      }
   }
   else {
      /* 8 bits per index */
      const GLubyte *ind = (const GLubyte *) indices;
      for (i = 0; i < num_pixels; i++)
         pix += get_palette_entry(info, palette, ind[i], pix);
   }
}


static const struct cpal_format_info *
cpal_get_info(GLint level, GLenum internalFormat,
              GLsizei width, GLsizei height, GLsizei imageSize)
{
   const struct cpal_format_info *info;
   GLint lvl, num_levels;
   GLsizei w, h, expect_size;

   info = &formats[internalFormat - GL_PALETTE4_RGB8_OES];
   ASSERT(info->cpal_format == internalFormat);

   if (level > 0) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_VALUE,
            "glCompressedTexImage2D(level=%d)", level);
      return NULL;
   }

   num_levels = -level + 1;
   expect_size = info->palette_size * info->size;
   for (lvl = 0; lvl < num_levels; lvl++) {
      w = width >> lvl;
      if (!w)
         w = 1;
      h = height >> lvl;
      if (!h)
         h = 1;

      if (info->palette_size == 16)
         expect_size += (w * h  + 1) / 2;
      else
         expect_size += w * h;
   }
   if (expect_size > imageSize) {
      _mesa_error(_mesa_get_current_context(), GL_INVALID_VALUE,
            "glCompressedTexImage2D(imageSize=%d)", imageSize);
      return NULL;
   }
   return info;
}

/**
 * Convert a call to glCompressedTexImage2D() where internalFormat is a
 *  compressed palette format into a regular GLubyte/RGBA glTexImage2D() call.
 */
static void
cpal_compressed_teximage2d(GLenum target, GLint level, GLenum internalFormat,
                           GLsizei width, GLsizei height, GLsizei imageSize,
                           const void *palette)
{
   const struct cpal_format_info *info;
   GLint lvl, num_levels;
   const GLubyte *indices;
   GLint saved_align, align;

   info = cpal_get_info(level, internalFormat, width, height, imageSize);
   if (!info)
      return;

   info = &formats[internalFormat - GL_PALETTE4_RGB8_OES];
   ASSERT(info->cpal_format == internalFormat);
   num_levels = -level + 1;

   /* first image follows the palette */
   indices = (const GLubyte *) palette + info->palette_size * info->size;

   _mesa_GetIntegerv(GL_UNPACK_ALIGNMENT, &saved_align);
   align = saved_align;

   for (lvl = 0; lvl < num_levels; lvl++) {
      GLsizei w, h;
      GLuint num_texels;
      GLubyte *image = NULL;

      w = width >> lvl;
      if (!w)
         w = 1;
      h = height >> lvl;
      if (!h)
         h = 1;
      num_texels = w * h;
      if (w * info->size % align) {
         _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, 1);
         align = 1;
      }

      /* allocate and fill dest image buffer */
      if (palette) {
         image = (GLubyte *) malloc(num_texels * info->size);
         paletted_to_color(info, palette, indices, num_texels, image);
      }

      _mesa_TexImage2D(target, lvl, info->format, w, h, 0,
                       info->format, info->type, image);
      if (image)
         free(image);

      /* advance index pointer to point to next src mipmap */
      if (info->palette_size == 16)
         indices += (num_texels + 1) / 2;
      else
         indices += num_texels;
   }

   if (saved_align != align)
      _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, saved_align);
}


void GL_APIENTRY
_es_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat,
                            GLsizei width, GLsizei height, GLint border,
                            GLsizei imageSize, const GLvoid *data)
{
   switch (internalFormat) {
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB8_OES:
   case GL_PALETTE8_RGBA8_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
   case GL_PALETTE8_RGBA4_OES:
   case GL_PALETTE8_RGB5_A1_OES:
      cpal_compressed_teximage2d(target, level, internalFormat,
                                 width, height, imageSize, data);
      break;
   default:
      _mesa_CompressedTexImage2DARB(target, level, internalFormat,
                                    width, height, border, imageSize, data);
   }
}
