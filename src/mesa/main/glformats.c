/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
 * Copyright (c) 2012 Intel Corporation
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "imports.h"
#include "glformats.h"


/**
 * Test if the given format is an integer (non-normalized) format.
 */
GLboolean
_mesa_is_enum_format_integer(GLenum format)
{
   switch (format) {
   /* generic integer formats */
   case GL_RED_INTEGER_EXT:
   case GL_GREEN_INTEGER_EXT:
   case GL_BLUE_INTEGER_EXT:
   case GL_ALPHA_INTEGER_EXT:
   case GL_RGB_INTEGER_EXT:
   case GL_RGBA_INTEGER_EXT:
   case GL_BGR_INTEGER_EXT:
   case GL_BGRA_INTEGER_EXT:
   case GL_LUMINANCE_INTEGER_EXT:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
   case GL_RG_INTEGER:
   /* specific integer formats */
   case GL_RGBA32UI_EXT:
   case GL_RGB32UI_EXT:
   case GL_RG32UI:
   case GL_R32UI:
   case GL_ALPHA32UI_EXT:
   case GL_INTENSITY32UI_EXT:
   case GL_LUMINANCE32UI_EXT:
   case GL_LUMINANCE_ALPHA32UI_EXT:
   case GL_RGBA16UI_EXT:
   case GL_RGB16UI_EXT:
   case GL_RG16UI:
   case GL_R16UI:
   case GL_ALPHA16UI_EXT:
   case GL_INTENSITY16UI_EXT:
   case GL_LUMINANCE16UI_EXT:
   case GL_LUMINANCE_ALPHA16UI_EXT:
   case GL_RGBA8UI_EXT:
   case GL_RGB8UI_EXT:
   case GL_RG8UI:
   case GL_R8UI:
   case GL_ALPHA8UI_EXT:
   case GL_INTENSITY8UI_EXT:
   case GL_LUMINANCE8UI_EXT:
   case GL_LUMINANCE_ALPHA8UI_EXT:
   case GL_RGBA32I_EXT:
   case GL_RGB32I_EXT:
   case GL_RG32I:
   case GL_R32I:
   case GL_ALPHA32I_EXT:
   case GL_INTENSITY32I_EXT:
   case GL_LUMINANCE32I_EXT:
   case GL_LUMINANCE_ALPHA32I_EXT:
   case GL_RGBA16I_EXT:
   case GL_RGB16I_EXT:
   case GL_RG16I:
   case GL_R16I:
   case GL_ALPHA16I_EXT:
   case GL_INTENSITY16I_EXT:
   case GL_LUMINANCE16I_EXT:
   case GL_LUMINANCE_ALPHA16I_EXT:
   case GL_RGBA8I_EXT:
   case GL_RGB8I_EXT:
   case GL_RG8I:
   case GL_R8I:
   case GL_ALPHA8I_EXT:
   case GL_INTENSITY8I_EXT:
   case GL_LUMINANCE8I_EXT:
   case GL_LUMINANCE_ALPHA8I_EXT:
   case GL_RGB10_A2UI:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Test if the given type is an integer (non-normalized) format.
 */
GLboolean
_mesa_is_type_integer(GLenum type)
{
   switch (type) {
   case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_SHORT:
   case GL_UNSIGNED_SHORT:
   case GL_BYTE:
   case GL_UNSIGNED_BYTE:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Test if the given format or type is an integer (non-normalized) format.
 */
extern GLboolean
_mesa_is_enum_format_or_type_integer(GLenum format, GLenum type)
{
   return _mesa_is_enum_format_integer(format) || _mesa_is_type_integer(type);
}


GLboolean
_mesa_is_type_unsigned(GLenum type)
{
   switch (type) {
   case GL_UNSIGNED_INT:
   case GL_UNSIGNED_INT_8_8_8_8:
   case GL_UNSIGNED_INT_8_8_8_8_REV:
   case GL_UNSIGNED_INT_10_10_10_2:
   case GL_UNSIGNED_INT_2_10_10_10_REV:

   case GL_UNSIGNED_SHORT:
   case GL_UNSIGNED_SHORT_4_4_4_4:
   case GL_UNSIGNED_SHORT_5_5_5_1:
   case GL_UNSIGNED_SHORT_5_6_5:
   case GL_UNSIGNED_SHORT_5_6_5_REV:
   case GL_UNSIGNED_SHORT_4_4_4_4_REV:
   case GL_UNSIGNED_SHORT_1_5_5_5_REV:
   case GL_UNSIGNED_SHORT_8_8_MESA:
   case GL_UNSIGNED_SHORT_8_8_REV_MESA:

   case GL_UNSIGNED_BYTE:
   case GL_UNSIGNED_BYTE_3_3_2:
   case GL_UNSIGNED_BYTE_2_3_3_REV:
      return GL_TRUE;

   default:
      return GL_FALSE;
   }
}


/**
 * Convert various base formats to the cooresponding integer format.
 */
GLenum
_mesa_base_format_to_integer_format(GLenum format)
{
   switch(format) {
   case GL_RED:
      return GL_RED_INTEGER;
   case GL_GREEN:
      return GL_GREEN_INTEGER;
   case GL_BLUE:
      return GL_BLUE_INTEGER;
   case GL_RG:
      return GL_RG_INTEGER;
   case GL_RGB:
      return GL_RGB_INTEGER;
   case GL_RGBA:
      return GL_RGBA_INTEGER;
   case GL_BGR:
      return GL_BGR_INTEGER;
   case GL_BGRA:
      return GL_BGRA_INTEGER;
   case GL_ALPHA:
      return GL_ALPHA_INTEGER;
   case GL_LUMINANCE:
      return GL_LUMINANCE_INTEGER_EXT;
   case GL_LUMINANCE_ALPHA:
      return GL_LUMINANCE_ALPHA_INTEGER_EXT;
   }

   return format;
}

