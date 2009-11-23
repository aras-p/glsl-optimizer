/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


/**
 * Code to convert compressed/paletted texture images to ordinary 4-byte RGBA.
 * See the GL_OES_compressed_paletted_texture spec at
 * http://khronos.org/registry/gles/extensions/OES/OES_compressed_paletted_texture.txt
 */


#include <stdlib.h>
#include <assert.h>
#include "GLES/gl.h"
#include "GLES/glext.h"


void GL_APIENTRY _es_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

void GL_APIENTRY _mesa_TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void GL_APIENTRY _mesa_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);


static const struct {
   GLenum format;
   GLuint palette_size;
   GLuint size;
} formats[] = {
   { GL_PALETTE4_RGB8_OES,      16, 3 },
   { GL_PALETTE4_RGBA8_OES,     16, 4 },
   { GL_PALETTE4_R5_G6_B5_OES,  16, 2 },
   { GL_PALETTE4_RGBA4_OES,     16, 2 },
   { GL_PALETTE4_RGB5_A1_OES,   16, 2 },
   { GL_PALETTE8_RGB8_OES,     256, 3 },
   { GL_PALETTE8_RGBA8_OES,    256, 4 },
   { GL_PALETTE8_R5_G6_B5_OES, 256, 2 },
   { GL_PALETTE8_RGBA4_OES,    256, 2 },
   { GL_PALETTE8_RGB5_A1_OES,  256, 2 }
};


/**
 * Get a color/entry from the palette.  Convert to GLubyte/RGBA format.
 */
static void
get_palette_entry(GLenum format, const void *palette, GLuint index,
                  GLubyte rgba[4])
{
   switch (format) {
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE8_RGB8_OES:
      {
         const GLubyte *pal = (const GLubyte *) palette;
         rgba[0] = pal[index * 3 + 0];
         rgba[1] = pal[index * 3 + 1];
         rgba[2] = pal[index * 3 + 2];
         rgba[3] = 255;
      }
      break;
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE8_RGBA8_OES:
      {
         const GLubyte *pal = (const GLubyte *) palette;
         rgba[0] = pal[index * 4 + 0];
         rgba[1] = pal[index * 4 + 1];
         rgba[2] = pal[index * 4 + 2];
         rgba[3] = pal[index * 4 + 3];
      }
      break;
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
      {
         const GLushort *pal = (const GLushort *) palette;
         const GLushort color = pal[index];
         rgba[0] = ((color >> 8) & 0xf8) | ((color >> 11) & 0x3);
         rgba[1] = ((color >> 3) & 0xfc) | ((color >> 1 ) & 0x3);
         rgba[2] = ((color << 3) & 0xf8) | ((color      ) & 0x7);
         rgba[3] = 255;
      }
      break;
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE8_RGBA4_OES:
      {
         const GLushort *pal = (const GLushort *) palette;
         const GLushort color = pal[index];
         rgba[0] = ((color & 0xf000) >> 8) | ((color & 0xf000) >> 12);
         rgba[1] = ((color & 0x0f00) >> 4) | ((color & 0x0f00) >>  8);
         rgba[2] = ((color & 0x00f0)     ) | ((color & 0x00f0) >>  4);
         rgba[3] = ((color & 0x000f) << 4) | ((color & 0x000f)      );
      }
      break;
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB5_A1_OES:
      {
         const GLushort *pal = (const GLushort *) palette;
         const GLushort color = pal[index];
         rgba[0] = ((color >> 8) & 0xf8) | ((color >> 11) & 0x7);
         rgba[1] = ((color >> 3) & 0xf8) | ((color >>  6) & 0x7);
         rgba[2] = ((color << 2) & 0xf8) | ((color >>  1) & 0x7);
         rgba[3] = (color & 0x1) * 255;
      }
      break;
   default:
      assert(0);
   }
}


/**
 * Convert paletted texture to simple GLubyte/RGBA format.
 */
static void
paletted_to_rgba(GLenum src_format,
                 const void *palette,
                 const void *indexes,
                 GLsizei width, GLsizei height,
                 GLubyte *rgba)
{
   GLuint pal_ents, i;

   assert(src_format >= GL_PALETTE4_RGB8_OES);
   assert(src_format <= GL_PALETTE8_RGB5_A1_OES);
   assert(formats[src_format - GL_PALETTE4_RGB8_OES].format == src_format);

   pal_ents = formats[src_format - GL_PALETTE4_RGB8_OES].palette_size;

   if (pal_ents == 16) {
      /* 4 bits per index */
      const GLubyte *ind = (const GLubyte *) indexes;

      if (width * height == 1) {
         /* special case the only odd-sized image */
         GLuint index0 = ind[0] >> 4;
         get_palette_entry(src_format, palette, index0, rgba);
         return;
      }
      /* two pixels per iteration */
      for (i = 0; i < width * height / 2; i++) {
         GLuint index0 = ind[i] >> 4;
         GLuint index1 = ind[i] & 0xf;
         get_palette_entry(src_format, palette, index0, rgba + i * 8);
         get_palette_entry(src_format, palette, index1, rgba + i * 8 + 4);
      }
   }
   else {
      /* 8 bits per index */
      const GLubyte *ind = (const GLubyte *) indexes;
      for (i = 0; i < width * height; i++) {
         GLuint index = ind[i];
         get_palette_entry(src_format, palette, index, rgba + i * 4);
      }
   }
}


/**
 * Convert a call to glCompressedTexImage2D() where internalFormat is a
 *  compressed palette format into a regular GLubyte/RGBA glTexImage2D() call.
 */
static void
cpal_compressed_teximage2d(GLenum target, GLint level,
                           GLenum internalFormat,
                           GLsizei width, GLsizei height,
                           const void *pixels)
{
   GLuint pal_ents, pal_ent_size, pal_bytes;
   const GLint num_levels = level + 1;
   GLint lvl;
   const GLubyte *indexes;

   assert(internalFormat >= GL_PALETTE4_RGB8_OES);
   assert(internalFormat <= GL_PALETTE8_RGB5_A1_OES);
   assert(formats[internalFormat - GL_PALETTE4_RGB8_OES].format == internalFormat);

   pal_ents = formats[internalFormat - GL_PALETTE4_RGB8_OES].palette_size;
   pal_ent_size = formats[internalFormat - GL_PALETTE4_RGB8_OES].size;
   pal_bytes = pal_ents * pal_ent_size;

   /* first image follows the palette */
   indexes = (const GLubyte *) pixels + pal_bytes;

   /* No worries about glPixelStore state since the only supported parameter is
    * GL_UNPACK_ALIGNMENT and it doesn't matter when unpacking GLubyte/RGBA.
    */

   for (lvl = 0; lvl < num_levels; lvl++) {
      /* Allocate GLubyte/RGBA dest image buffer */
      GLubyte *rgba = (GLubyte *) malloc(width * height * 4);

      if (pixels)
         paletted_to_rgba(internalFormat, pixels, indexes, width, height, rgba);

      _mesa_TexImage2D(target, lvl, GL_RGBA, width, height, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, rgba);

      free(rgba);

      /* advance index pointer to point to next src mipmap */
      if (pal_ents == 4)
         indexes += width * height / 2;
      else
         indexes += width * height;

      /* next mipmap level size */
      if (width > 1)
         width /= 2;
      if (height > 1)
         height /= 2;
   }
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
                                 width, height, data);
      break;
   default:
      _mesa_CompressedTexImage2DARB(target, level, internalFormat,
                                    width, height, border, imageSize, data);
   }
}
