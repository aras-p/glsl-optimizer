/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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


#include "imports.h"
#include "formats.h"
#include "macros.h"
#include "glformats.h"


/**
 * Information about texture formats.
 */
struct gl_format_info
{
   mesa_format Name;

   /** text name for debugging */
   const char *StrName;

   enum mesa_format_layout Layout;

   /**
    * Base format is one of GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_ALPHA,
    * GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_INTENSITY, GL_YCBCR_MESA,
    * GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL.
    */
   GLenum BaseFormat;

   /**
    * Logical data type: one of  GL_UNSIGNED_NORMALIZED, GL_SIGNED_NORMALIZED,
    * GL_UNSIGNED_INT, GL_INT, GL_FLOAT.
    */
   GLenum DataType;

   GLubyte RedBits;
   GLubyte GreenBits;
   GLubyte BlueBits;
   GLubyte AlphaBits;
   GLubyte LuminanceBits;
   GLubyte IntensityBits;
   GLubyte DepthBits;
   GLubyte StencilBits;

   /**
    * To describe compressed formats.  If not compressed, Width=Height=1.
    */
   GLubyte BlockWidth, BlockHeight;
   GLubyte BytesPerBlock;

   uint8_t Swizzle[4];
};

#include "format_info.c"

static const struct gl_format_info *
_mesa_get_format_info(mesa_format format)
{
   const struct gl_format_info *info = &format_info[format];
   assert(info->Name == format);
   return info;
}


/** Return string name of format (for debugging) */
const char *
_mesa_get_format_name(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->StrName;
}



/**
 * Return bytes needed to store a block of pixels in the given format.
 * Normally, a block is 1x1 (a single pixel).  But for compressed formats
 * a block may be 4x4 or 8x4, etc.
 *
 * Note: not GLuint, so as not to coerce math to unsigned. cf. fdo #37351
 */
GLint
_mesa_get_format_bytes(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   ASSERT(info->BytesPerBlock);
   ASSERT(info->BytesPerBlock <= MAX_PIXEL_BYTES ||
          _mesa_is_format_compressed(format));
   return info->BytesPerBlock;
}


/**
 * Return bits per component for the given format.
 * \param format  one of MESA_FORMAT_x
 * \param pname  the component, such as GL_RED_BITS, GL_TEXTURE_BLUE_BITS, etc.
 */
GLint
_mesa_get_format_bits(mesa_format format, GLenum pname)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   switch (pname) {
   case GL_RED_BITS:
   case GL_TEXTURE_RED_SIZE:
   case GL_RENDERBUFFER_RED_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
      return info->RedBits;
   case GL_GREEN_BITS:
   case GL_TEXTURE_GREEN_SIZE:
   case GL_RENDERBUFFER_GREEN_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
      return info->GreenBits;
   case GL_BLUE_BITS:
   case GL_TEXTURE_BLUE_SIZE:
   case GL_RENDERBUFFER_BLUE_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
      return info->BlueBits;
   case GL_ALPHA_BITS:
   case GL_TEXTURE_ALPHA_SIZE:
   case GL_RENDERBUFFER_ALPHA_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
      return info->AlphaBits;
   case GL_TEXTURE_INTENSITY_SIZE:
      return info->IntensityBits;
   case GL_TEXTURE_LUMINANCE_SIZE:
      return info->LuminanceBits;
   case GL_INDEX_BITS:
      return 0;
   case GL_DEPTH_BITS:
   case GL_TEXTURE_DEPTH_SIZE_ARB:
   case GL_RENDERBUFFER_DEPTH_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
      return info->DepthBits;
   case GL_STENCIL_BITS:
   case GL_TEXTURE_STENCIL_SIZE_EXT:
   case GL_RENDERBUFFER_STENCIL_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
      return info->StencilBits;
   default:
      _mesa_problem(NULL, "bad pname in _mesa_get_format_bits()");
      return 0;
   }
}


GLuint
_mesa_get_format_max_bits(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   GLuint max = MAX2(info->RedBits, info->GreenBits);
   max = MAX2(max, info->BlueBits);
   max = MAX2(max, info->AlphaBits);
   max = MAX2(max, info->LuminanceBits);
   max = MAX2(max, info->IntensityBits);
   max = MAX2(max, info->DepthBits);
   max = MAX2(max, info->StencilBits);
   return max;
}


/**
 * Return the layout type of the given format.
 * The return value will be one of:
 *    MESA_FORMAT_LAYOUT_ARRAY
 *    MESA_FORMAT_LAYOUT_PACKED
 *    MESA_FORMAT_LAYOUT_OTHER
 */
extern enum mesa_format_layout
_mesa_get_format_layout(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->Layout;
}


/**
 * Return the data type (or more specifically, the data representation)
 * for the given format.
 * The return value will be one of:
 *    GL_UNSIGNED_NORMALIZED = unsigned int representing [0,1]
 *    GL_SIGNED_NORMALIZED = signed int representing [-1, 1]
 *    GL_UNSIGNED_INT = an ordinary unsigned integer
 *    GL_INT = an ordinary signed integer
 *    GL_FLOAT = an ordinary float
 */
GLenum
_mesa_get_format_datatype(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->DataType;
}


/**
 * Return the basic format for the given type.  The result will be one of
 * GL_RGB, GL_RGBA, GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_INTENSITY,
 * GL_YCBCR_MESA, GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL.
 */
GLenum
_mesa_get_format_base_format(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BaseFormat;
}


/**
 * Return the block size (in pixels) for the given format.  Normally
 * the block size is 1x1.  But compressed formats will have block sizes
 * of 4x4 or 8x4 pixels, etc.
 * \param bw  returns block width in pixels
 * \param bh  returns block height in pixels
 */
void
_mesa_get_format_block_size(mesa_format format, GLuint *bw, GLuint *bh)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   *bw = info->BlockWidth;
   *bh = info->BlockHeight;
}


/**
 * Returns the an array of four numbers representing the transformation
 * from the RGBA or SZ colorspace to the given format.  For array formats,
 * the i'th RGBA component is given by:
 *
 * if (swizzle[i] <= MESA_FORMAT_SWIZZLE_W)
 *    comp = data[swizzle[i]];
 * else if (swizzle[i] == MESA_FORMAT_SWIZZLE_ZERO)
 *    comp = 0;
 * else if (swizzle[i] == MESA_FORMAT_SWIZZLE_ONE)
 *    comp = 1;
 * else if (swizzle[i] == MESA_FORMAT_SWIZZLE_NONE)
 *    // data does not contain a channel of this format
 *
 * For packed formats, the swizzle gives the number of components left of
 * the least significant bit.
 *
 * Compressed formats have no swizzle.
 */
void
_mesa_get_format_swizzle(mesa_format format, uint8_t swizzle_out[4])
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   memcpy(swizzle_out, info->Swizzle, sizeof(info->Swizzle));
}


/** Is the given format a compressed format? */
GLboolean
_mesa_is_format_compressed(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BlockWidth > 1 || info->BlockHeight > 1;
}


/**
 * Determine if the given format represents a packed depth/stencil buffer.
 */
GLboolean
_mesa_is_format_packed_depth_stencil(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   return info->BaseFormat == GL_DEPTH_STENCIL;
}


/**
 * Is the given format a signed/unsigned integer color format?
 */
GLboolean
_mesa_is_format_integer_color(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return (info->DataType == GL_INT || info->DataType == GL_UNSIGNED_INT) &&
      info->BaseFormat != GL_DEPTH_COMPONENT &&
      info->BaseFormat != GL_DEPTH_STENCIL &&
      info->BaseFormat != GL_STENCIL_INDEX;
}


/**
 * Is the given format an unsigned integer format?
 */
GLboolean
_mesa_is_format_unsigned(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return _mesa_is_type_unsigned(info->DataType);
}


/**
 * Does the given format store signed values?
 */
GLboolean
_mesa_is_format_signed(mesa_format format)
{
   if (format == MESA_FORMAT_R11G11B10_FLOAT || 
       format == MESA_FORMAT_R9G9B9E5_FLOAT) {
      /* these packed float formats only store unsigned values */
      return GL_FALSE;
   }
   else {
      const struct gl_format_info *info = _mesa_get_format_info(format);
      return (info->DataType == GL_SIGNED_NORMALIZED ||
              info->DataType == GL_INT ||
              info->DataType == GL_FLOAT);
   }
}

/**
 * Is the given format an integer format?
 */
GLboolean
_mesa_is_format_integer(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return (info->DataType == GL_INT || info->DataType == GL_UNSIGNED_INT);
}

/**
 * Return color encoding for given format.
 * \return GL_LINEAR or GL_SRGB
 */
GLenum
_mesa_get_format_color_encoding(mesa_format format)
{
   /* XXX this info should be encoded in gl_format_info */
   switch (format) {
   case MESA_FORMAT_BGR_SRGB8:
   case MESA_FORMAT_A8B8G8R8_SRGB:
   case MESA_FORMAT_B8G8R8A8_SRGB:
   case MESA_FORMAT_A8R8G8B8_SRGB:
   case MESA_FORMAT_R8G8B8A8_SRGB:
   case MESA_FORMAT_L_SRGB8:
   case MESA_FORMAT_L8A8_SRGB:
   case MESA_FORMAT_A8L8_SRGB:
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
   case MESA_FORMAT_R8G8B8X8_SRGB:
   case MESA_FORMAT_ETC2_SRGB8:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
   case MESA_FORMAT_B8G8R8X8_SRGB:
   case MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM:
      return GL_SRGB;
   default:
      return GL_LINEAR;
   }
}


/**
 * Return TRUE if format is an ETC2 compressed format specified
 * by GL_ARB_ES3_compatibility.
 */
bool
_mesa_is_format_etc2(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_ETC2_RGB8:
   case MESA_FORMAT_ETC2_SRGB8:
   case MESA_FORMAT_ETC2_RGBA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_R11_EAC:
   case MESA_FORMAT_ETC2_RG11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
   case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * For an sRGB format, return the corresponding linear color space format.
 * For non-sRGB formats, return the format as-is.
 */
mesa_format
_mesa_get_srgb_format_linear(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_BGR_SRGB8:
      format = MESA_FORMAT_BGR_UNORM8;
      break;
   case MESA_FORMAT_A8B8G8R8_SRGB:
      format = MESA_FORMAT_A8B8G8R8_UNORM;
      break;
   case MESA_FORMAT_B8G8R8A8_SRGB:
      format = MESA_FORMAT_B8G8R8A8_UNORM;
      break;
   case MESA_FORMAT_A8R8G8B8_SRGB:
      format = MESA_FORMAT_A8R8G8B8_UNORM;
      break;
   case MESA_FORMAT_R8G8B8A8_SRGB:
      format = MESA_FORMAT_R8G8B8A8_UNORM;
      break;
   case MESA_FORMAT_L_SRGB8:
      format = MESA_FORMAT_L_UNORM8;
      break;
   case MESA_FORMAT_L8A8_SRGB:
      format = MESA_FORMAT_L8A8_UNORM;
      break;
   case MESA_FORMAT_A8L8_SRGB:
      format = MESA_FORMAT_A8L8_UNORM;
      break;
   case MESA_FORMAT_SRGB_DXT1:
      format = MESA_FORMAT_RGB_DXT1;
      break;
   case MESA_FORMAT_SRGBA_DXT1:
      format = MESA_FORMAT_RGBA_DXT1;
      break;
   case MESA_FORMAT_SRGBA_DXT3:
      format = MESA_FORMAT_RGBA_DXT3;
      break;
   case MESA_FORMAT_SRGBA_DXT5:
      format = MESA_FORMAT_RGBA_DXT5;
      break;
   case MESA_FORMAT_R8G8B8X8_SRGB:
      format = MESA_FORMAT_R8G8B8X8_UNORM;
      break;
   case MESA_FORMAT_X8B8G8R8_SRGB:
      format = MESA_FORMAT_X8B8G8R8_UNORM;
      break;
   case MESA_FORMAT_ETC2_SRGB8:
      format = MESA_FORMAT_ETC2_RGB8;
      break;
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
      format = MESA_FORMAT_ETC2_RGBA8_EAC;
      break;
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
      format = MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1;
      break;
   case MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM:
      format = MESA_FORMAT_BPTC_RGBA_UNORM;
      break;
   case MESA_FORMAT_B8G8R8X8_SRGB:
      format = MESA_FORMAT_B8G8R8X8_UNORM;
      break;
   case MESA_FORMAT_X8R8G8B8_SRGB:
      format = MESA_FORMAT_X8R8G8B8_UNORM;
      break;
   default:
      break;
   }
   return format;
}


/**
 * If the given format is a compressed format, return a corresponding
 * uncompressed format.
 */
mesa_format
_mesa_get_uncompressed_format(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_RGB_FXT1:
      return MESA_FORMAT_BGR_UNORM8;
   case MESA_FORMAT_RGBA_FXT1:
      return MESA_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_SRGB_DXT1:
      return MESA_FORMAT_BGR_UNORM8;
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
      return MESA_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT3:
      return MESA_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_RGBA_DXT5:
   case MESA_FORMAT_SRGBA_DXT5:
      return MESA_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_R_RGTC1_UNORM:
      return MESA_FORMAT_R_UNORM8;
   case MESA_FORMAT_R_RGTC1_SNORM:
      return MESA_FORMAT_R_SNORM8;
   case MESA_FORMAT_RG_RGTC2_UNORM:
      return MESA_FORMAT_R8G8_UNORM;
   case MESA_FORMAT_RG_RGTC2_SNORM:
      return MESA_FORMAT_R8G8_SNORM;
   case MESA_FORMAT_L_LATC1_UNORM:
      return MESA_FORMAT_L_UNORM8;
   case MESA_FORMAT_L_LATC1_SNORM:
      return MESA_FORMAT_L_SNORM8;
   case MESA_FORMAT_LA_LATC2_UNORM:
      return MESA_FORMAT_L8A8_UNORM;
   case MESA_FORMAT_LA_LATC2_SNORM:
      return MESA_FORMAT_L8A8_SNORM;
   case MESA_FORMAT_ETC1_RGB8:
   case MESA_FORMAT_ETC2_RGB8:
   case MESA_FORMAT_ETC2_SRGB8:
      return MESA_FORMAT_BGR_UNORM8;
   case MESA_FORMAT_ETC2_RGBA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
      return MESA_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_ETC2_R11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
      return MESA_FORMAT_R_UNORM16;
   case MESA_FORMAT_ETC2_RG11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
      return MESA_FORMAT_R16G16_UNORM;
   case MESA_FORMAT_BPTC_RGBA_UNORM:
   case MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM:
      return MESA_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_BPTC_RGB_UNSIGNED_FLOAT:
   case MESA_FORMAT_BPTC_RGB_SIGNED_FLOAT:
      return MESA_FORMAT_RGB_FLOAT32;
   default:
#ifdef DEBUG
      assert(!_mesa_is_format_compressed(format));
#endif
      return format;
   }
}


GLuint
_mesa_format_num_components(mesa_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return ((info->RedBits > 0) +
           (info->GreenBits > 0) +
           (info->BlueBits > 0) +
           (info->AlphaBits > 0) +
           (info->LuminanceBits > 0) +
           (info->IntensityBits > 0) +
           (info->DepthBits > 0) +
           (info->StencilBits > 0));
}


/**
 * Returns true if a color format has data stored in the R/G/B/A channels,
 * given an index from 0 to 3.
 */
bool
_mesa_format_has_color_component(mesa_format format, int component)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   assert(info->BaseFormat != GL_DEPTH_COMPONENT &&
          info->BaseFormat != GL_DEPTH_STENCIL &&
          info->BaseFormat != GL_STENCIL_INDEX);

   switch (component) {
   case 0:
      return (info->RedBits + info->IntensityBits + info->LuminanceBits) > 0;
   case 1:
      return (info->GreenBits + info->IntensityBits + info->LuminanceBits) > 0;
   case 2:
      return (info->BlueBits + info->IntensityBits + info->LuminanceBits) > 0;
   case 3:
      return (info->AlphaBits + info->IntensityBits) > 0;
   default:
      assert(!"Invalid color component: must be 0..3");
      return false;
   }
}


/**
 * Return number of bytes needed to store an image of the given size
 * in the given format.
 */
GLuint
_mesa_format_image_size(mesa_format format, GLsizei width,
                        GLsizei height, GLsizei depth)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format (2D only for now) */
      const GLuint bw = info->BlockWidth, bh = info->BlockHeight;
      const GLuint wblocks = (width + bw - 1) / bw;
      const GLuint hblocks = (height + bh - 1) / bh;
      const GLuint sz = wblocks * hblocks * info->BytesPerBlock;
      return sz * depth;
   }
   else {
      /* non-compressed */
      const GLuint sz = width * height * depth * info->BytesPerBlock;
      return sz;
   }
}


/**
 * Same as _mesa_format_image_size() but returns a 64-bit value to
 * accomodate very large textures.
 */
uint64_t
_mesa_format_image_size64(mesa_format format, GLsizei width,
                          GLsizei height, GLsizei depth)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format (2D only for now) */
      const uint64_t bw = info->BlockWidth, bh = info->BlockHeight;
      const uint64_t wblocks = (width + bw - 1) / bw;
      const uint64_t hblocks = (height + bh - 1) / bh;
      const uint64_t sz = wblocks * hblocks * info->BytesPerBlock;
      return sz * depth;
   }
   else {
      /* non-compressed */
      const uint64_t sz = ((uint64_t) width *
                           (uint64_t) height *
                           (uint64_t) depth *
                           info->BytesPerBlock);
      return sz;
   }
}



GLint
_mesa_format_row_stride(mesa_format format, GLsizei width)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format */
      const GLuint bw = info->BlockWidth;
      const GLuint wblocks = (width + bw - 1) / bw;
      const GLint stride = wblocks * info->BytesPerBlock;
      return stride;
   }
   else {
      const GLint stride = width * info->BytesPerBlock;
      return stride;
   }
}


/**
 * Debug/test: check that all formats are handled in the
 * _mesa_format_to_type_and_comps() function.  When new pixel formats
 * are added to Mesa, that function needs to be updated.
 * This is a no-op after the first call.
 */
static void
check_format_to_type_and_comps(void)
{
   mesa_format f;

   for (f = MESA_FORMAT_NONE + 1; f < MESA_FORMAT_COUNT; f++) {
      GLenum datatype = 0;
      GLuint comps = 0;
      /* This function will emit a problem/warning if the format is
       * not handled.
       */
      _mesa_format_to_type_and_comps(f, &datatype, &comps);
   }
}

/**
 * Do sanity checking of the format info table.
 */
void
_mesa_test_formats(void)
{
   GLuint i;

   STATIC_ASSERT(Elements(format_info) == MESA_FORMAT_COUNT);

   for (i = 0; i < MESA_FORMAT_COUNT; i++) {
      const struct gl_format_info *info = _mesa_get_format_info(i);
      assert(info);

      assert(info->Name == i);

      if (info->Name == MESA_FORMAT_NONE)
         continue;

      if (info->BlockWidth == 1 && info->BlockHeight == 1) {
         if (info->RedBits > 0) {
            GLuint t = info->RedBits + info->GreenBits
               + info->BlueBits + info->AlphaBits;
            assert(t / 8 <= info->BytesPerBlock);
            (void) t;
         }
      }

      assert(info->DataType == GL_UNSIGNED_NORMALIZED ||
             info->DataType == GL_SIGNED_NORMALIZED ||
             info->DataType == GL_UNSIGNED_INT ||
             info->DataType == GL_INT ||
             info->DataType == GL_FLOAT ||
             /* Z32_FLOAT_X24S8 has DataType of GL_NONE */
             info->DataType == GL_NONE);

      if (info->BaseFormat == GL_RGB) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits > 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RGBA) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits > 0);
         assert(info->AlphaBits > 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RG) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RED) {
         assert(info->RedBits > 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_LUMINANCE) {
         assert(info->RedBits == 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits > 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_INTENSITY) {
         assert(info->RedBits == 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits > 0);
      }
   }

   check_format_to_type_and_comps();
}



/**
 * Return datatype and number of components per texel for the given mesa_format.
 * Only used for mipmap generation code.
 */
void
_mesa_format_to_type_and_comps(mesa_format format,
                               GLenum *datatype, GLuint *comps)
{
   switch (format) {
   case MESA_FORMAT_A8B8G8R8_UNORM:
   case MESA_FORMAT_R8G8B8A8_UNORM:
   case MESA_FORMAT_B8G8R8A8_UNORM:
   case MESA_FORMAT_A8R8G8B8_UNORM:
   case MESA_FORMAT_X8B8G8R8_UNORM:
   case MESA_FORMAT_R8G8B8X8_UNORM:
   case MESA_FORMAT_B8G8R8X8_UNORM:
   case MESA_FORMAT_X8R8G8B8_UNORM:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_BGR_UNORM8:
   case MESA_FORMAT_RGB_UNORM8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_B5G6R5_UNORM:
   case MESA_FORMAT_R5G6B5_UNORM:
      *datatype = GL_UNSIGNED_SHORT_5_6_5;
      *comps = 3;
      return;

   case MESA_FORMAT_B4G4R4A4_UNORM:
   case MESA_FORMAT_A4R4G4B4_UNORM:
   case MESA_FORMAT_B4G4R4X4_UNORM:
      *datatype = GL_UNSIGNED_SHORT_4_4_4_4;
      *comps = 4;
      return;

   case MESA_FORMAT_B5G5R5A1_UNORM:
   case MESA_FORMAT_A1R5G5B5_UNORM:
   case MESA_FORMAT_B5G5R5X1_UNORM:
      *datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_B10G10R10A2_UNORM:
      *datatype = GL_UNSIGNED_INT_2_10_10_10_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_A1B5G5R5_UNORM:
      *datatype = GL_UNSIGNED_SHORT_5_5_5_1;
      *comps = 4;
      return;

   case MESA_FORMAT_L4A4_UNORM:
      *datatype = MESA_UNSIGNED_BYTE_4_4;
      *comps = 2;
      return;

   case MESA_FORMAT_L8A8_UNORM:
   case MESA_FORMAT_A8L8_UNORM:
   case MESA_FORMAT_R8G8_UNORM:
   case MESA_FORMAT_G8R8_UNORM:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_L16A16_UNORM:
   case MESA_FORMAT_A16L16_UNORM:
   case MESA_FORMAT_R16G16_UNORM:
   case MESA_FORMAT_G16R16_UNORM:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_R_UNORM16:
   case MESA_FORMAT_A_UNORM16:
   case MESA_FORMAT_L_UNORM16:
   case MESA_FORMAT_I_UNORM16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;

   case MESA_FORMAT_B2G3R3_UNORM:
      *datatype = GL_UNSIGNED_BYTE_3_3_2;
      *comps = 3;
      return;

   case MESA_FORMAT_A_UNORM8:
   case MESA_FORMAT_L_UNORM8:
   case MESA_FORMAT_I_UNORM8:
   case MESA_FORMAT_R_UNORM8:
   case MESA_FORMAT_S_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;

   case MESA_FORMAT_YCBCR:
   case MESA_FORMAT_YCBCR_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      *datatype = GL_UNSIGNED_INT_24_8_MESA;
      *comps = 2;
      return;

   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      *datatype = GL_UNSIGNED_INT_8_24_REV_MESA;
      *comps = 2;
      return;

   case MESA_FORMAT_Z_UNORM16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z_UNORM32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      *datatype = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
      *comps = 1;
      return;

   case MESA_FORMAT_R_SNORM8:
   case MESA_FORMAT_A_SNORM8:
   case MESA_FORMAT_L_SNORM8:
   case MESA_FORMAT_I_SNORM8:
      *datatype = GL_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_R8G8_SNORM:
   case MESA_FORMAT_L8A8_SNORM:
   case MESA_FORMAT_A8L8_SNORM:
      *datatype = GL_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_A8B8G8R8_SNORM:
   case MESA_FORMAT_R8G8B8A8_SNORM:
   case MESA_FORMAT_X8B8G8R8_SNORM:
      *datatype = GL_BYTE;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBA_UNORM16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;

   case MESA_FORMAT_R_SNORM16:
   case MESA_FORMAT_A_SNORM16:
   case MESA_FORMAT_L_SNORM16:
   case MESA_FORMAT_I_SNORM16:
      *datatype = GL_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_R16G16_SNORM:
   case MESA_FORMAT_LA_SNORM16:
      *datatype = GL_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_SNORM16:
      *datatype = GL_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_SNORM16:
      *datatype = GL_SHORT;
      *comps = 4;
      return;

   case MESA_FORMAT_BGR_SRGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_A8B8G8R8_SRGB:
   case MESA_FORMAT_B8G8R8A8_SRGB:
   case MESA_FORMAT_A8R8G8B8_SRGB:
   case MESA_FORMAT_R8G8B8A8_SRGB:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_L_SRGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_L8A8_SRGB:
   case MESA_FORMAT_A8L8_SRGB:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
   case MESA_FORMAT_R_RGTC1_UNORM:
   case MESA_FORMAT_R_RGTC1_SNORM:
   case MESA_FORMAT_RG_RGTC2_UNORM:
   case MESA_FORMAT_RG_RGTC2_SNORM:
   case MESA_FORMAT_L_LATC1_UNORM:
   case MESA_FORMAT_L_LATC1_SNORM:
   case MESA_FORMAT_LA_LATC2_UNORM:
   case MESA_FORMAT_LA_LATC2_SNORM:
   case MESA_FORMAT_ETC1_RGB8:
   case MESA_FORMAT_ETC2_RGB8:
   case MESA_FORMAT_ETC2_SRGB8:
   case MESA_FORMAT_ETC2_RGBA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_R11_EAC:
   case MESA_FORMAT_ETC2_RG11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
   case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
   case MESA_FORMAT_BPTC_RGBA_UNORM:
   case MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM:
   case MESA_FORMAT_BPTC_RGB_SIGNED_FLOAT:
   case MESA_FORMAT_BPTC_RGB_UNSIGNED_FLOAT:
      /* XXX generate error instead? */
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 0;
      return;

   case MESA_FORMAT_RGBA_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 4;
      return;
   case MESA_FORMAT_RGBA_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 3;
      return;
   case MESA_FORMAT_LA_FLOAT32:
   case MESA_FORMAT_RG_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 2;
      return;
   case MESA_FORMAT_LA_FLOAT16:
   case MESA_FORMAT_RG_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 2;
      return;
   case MESA_FORMAT_A_FLOAT32:
   case MESA_FORMAT_L_FLOAT32:
   case MESA_FORMAT_I_FLOAT32:
   case MESA_FORMAT_R_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 1;
      return;
   case MESA_FORMAT_A_FLOAT16:
   case MESA_FORMAT_L_FLOAT16:
   case MESA_FORMAT_I_FLOAT16:
   case MESA_FORMAT_R_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 1;
      return;

   case MESA_FORMAT_A_UINT8:
   case MESA_FORMAT_L_UINT8:
   case MESA_FORMAT_I_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_LA_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_A_UINT16:
   case MESA_FORMAT_L_UINT16:
   case MESA_FORMAT_I_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_LA_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_A_UINT32:
   case MESA_FORMAT_L_UINT32:
   case MESA_FORMAT_I_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_LA_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 2;
      return;
   case MESA_FORMAT_A_SINT8:
   case MESA_FORMAT_L_SINT8:
   case MESA_FORMAT_I_SINT8:
      *datatype = GL_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_LA_SINT8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_A_SINT16:
   case MESA_FORMAT_L_SINT16:
   case MESA_FORMAT_I_SINT16:
      *datatype = GL_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_LA_SINT16:
      *datatype = GL_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_A_SINT32:
   case MESA_FORMAT_L_SINT32:
   case MESA_FORMAT_I_SINT32:
      *datatype = GL_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_LA_SINT32:
      *datatype = GL_INT;
      *comps = 2;
      return;

   case MESA_FORMAT_R_SINT8:
      *datatype = GL_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_SINT8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_SINT8:
      *datatype = GL_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_SINT8:
      *datatype = GL_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_R_SINT16:
      *datatype = GL_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_SINT16:
      *datatype = GL_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_SINT16:
      *datatype = GL_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_SINT16:
      *datatype = GL_SHORT;
      *comps = 4;
      return;
   case MESA_FORMAT_R_SINT32:
      *datatype = GL_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_SINT32:
      *datatype = GL_INT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_SINT32:
      *datatype = GL_INT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_SINT32:
      *datatype = GL_INT;
      *comps = 4;
      return;

   /**
    * \name Non-normalized unsigned integer formats.
    */
   case MESA_FORMAT_R_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_R_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;
   case MESA_FORMAT_R_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 4;
      return;

   case MESA_FORMAT_R9G9B9E5_FLOAT:
      *datatype = GL_UNSIGNED_INT_5_9_9_9_REV;
      *comps = 3;
      return;

   case MESA_FORMAT_R11G11B10_FLOAT:
      *datatype = GL_UNSIGNED_INT_10F_11F_11F_REV;
      *comps = 3;
      return;

   case MESA_FORMAT_B10G10R10A2_UINT:
   case MESA_FORMAT_R10G10B10A2_UINT:
      *datatype = GL_UNSIGNED_INT_2_10_10_10_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_R8G8B8X8_SRGB:
   case MESA_FORMAT_X8B8G8R8_SRGB:
   case MESA_FORMAT_RGBX_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;

   case MESA_FORMAT_R8G8B8X8_SNORM:
   case MESA_FORMAT_RGBX_SINT8:
      *datatype = GL_BYTE;
      *comps = 4;
      return;

   case MESA_FORMAT_B10G10R10X2_UNORM:
      *datatype = GL_UNSIGNED_INT_2_10_10_10_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBX_UNORM16:
   case MESA_FORMAT_RGBX_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBX_SNORM16:
   case MESA_FORMAT_RGBX_SINT16:
      *datatype = GL_SHORT;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBX_FLOAT16:
      *datatype = GL_HALF_FLOAT;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBX_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBX_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBX_SINT32:
      *datatype = GL_INT;
      *comps = 4;
      return;

   case MESA_FORMAT_R10G10B10A2_UNORM:
      *datatype = GL_UNSIGNED_INT_2_10_10_10_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_G8R8_SNORM:
      *datatype = GL_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_G16R16_SNORM:
      *datatype = GL_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_B8G8R8X8_SRGB:
   case MESA_FORMAT_X8R8G8B8_SRGB:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;

   case MESA_FORMAT_COUNT:
      assert(0);
      return;

   case MESA_FORMAT_NONE:
   /* For debug builds, warn if any formats are not handled */
#ifdef DEBUG
   default:
#endif
      _mesa_problem(NULL, "bad format %s in _mesa_format_to_type_and_comps",
                    _mesa_get_format_name(format));
      *datatype = 0;
      *comps = 1;
   }
}

/**
 * Check if a mesa_format exactly matches a GL format/type combination
 * such that we can use memcpy() from one to the other.
 * \param mesa_format  a MESA_FORMAT_x value
 * \param format  the user-specified image format
 * \param type  the user-specified image datatype
 * \param swapBytes  typically the current pixel pack/unpack byteswap state
 * \return GL_TRUE if the formats match, GL_FALSE otherwise.
 */
GLboolean
_mesa_format_matches_format_and_type(mesa_format mesa_format,
				     GLenum format, GLenum type,
                                     GLboolean swapBytes)
{
   const GLboolean littleEndian = _mesa_little_endian();

   /* Note: When reading a GL format/type combination, the format lists channel
    * assignments from most significant channel in the type to least
    * significant.  A type with _REV indicates that the assignments are
    * swapped, so they are listed from least significant to most significant.
    *
    * For sanity, please keep this switch statement ordered the same as the
    * enums in formats.h.
    */

   switch (mesa_format) {

   case MESA_FORMAT_NONE:
   case MESA_FORMAT_COUNT:
      return GL_FALSE;

   case MESA_FORMAT_A8B8G8R8_UNORM:
   case MESA_FORMAT_A8B8G8R8_SRGB:
      if (format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8 && !swapBytes)
         return GL_TRUE;

      if (format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8_REV && swapBytes)
         return GL_TRUE;

      if (format == GL_RGBA && type == GL_UNSIGNED_BYTE && !littleEndian)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_UNSIGNED_INT_8_8_8_8_REV
          && !swapBytes)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_UNSIGNED_INT_8_8_8_8
          && swapBytes)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_UNSIGNED_BYTE && littleEndian)
         return GL_TRUE;

      return GL_FALSE;

   case MESA_FORMAT_R8G8B8A8_UNORM:
   case MESA_FORMAT_R8G8B8A8_SRGB:
      if (format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8_REV &&
          !swapBytes)
         return GL_TRUE;

      if (format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8 && swapBytes)
         return GL_TRUE;

      if (format == GL_RGBA && type == GL_UNSIGNED_BYTE && littleEndian)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_UNSIGNED_INT_8_8_8_8 &&
          !swapBytes)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_UNSIGNED_INT_8_8_8_8_REV &&
          swapBytes)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_UNSIGNED_BYTE && !littleEndian)
         return GL_TRUE;

      return GL_FALSE;

   case MESA_FORMAT_B8G8R8A8_UNORM:
   case MESA_FORMAT_B8G8R8A8_SRGB:
      if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV &&
          !swapBytes)
         return GL_TRUE;

      if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8 && swapBytes)
         return GL_TRUE;

      if (format == GL_BGRA && type == GL_UNSIGNED_BYTE && littleEndian)
         return GL_TRUE;

      return GL_FALSE;

   case MESA_FORMAT_A8R8G8B8_UNORM:
   case MESA_FORMAT_A8R8G8B8_SRGB:
      if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8 && !swapBytes)
         return GL_TRUE;

      if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV &&
          swapBytes)
         return GL_TRUE;

      if (format == GL_BGRA && type == GL_UNSIGNED_BYTE && !littleEndian)
         return GL_TRUE;

      return GL_FALSE;

   case MESA_FORMAT_X8B8G8R8_UNORM:
   case MESA_FORMAT_R8G8B8X8_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_B8G8R8X8_UNORM:
   case MESA_FORMAT_X8R8G8B8_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_BGR_UNORM8:
   case MESA_FORMAT_BGR_SRGB8:
      return format == GL_BGR && type == GL_UNSIGNED_BYTE && littleEndian;

   case MESA_FORMAT_RGB_UNORM8:
      return format == GL_RGB && type == GL_UNSIGNED_BYTE && littleEndian;

   case MESA_FORMAT_B5G6R5_UNORM:
      return format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 && !swapBytes;

   case MESA_FORMAT_R5G6B5_UNORM:
      /* Some of the 16-bit MESA_FORMATs that would seem to correspond to
       * GL_UNSIGNED_SHORT_* are byte-swapped instead of channel-reversed,
       * according to formats.h, so they can't be matched.
       */
      return GL_FALSE;

   case MESA_FORMAT_B4G4R4A4_UNORM:
      return format == GL_BGRA && type == GL_UNSIGNED_SHORT_4_4_4_4_REV &&
         !swapBytes;

   case MESA_FORMAT_A4R4G4B4_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_A1B5G5R5_UNORM:
      return format == GL_RGBA && type == GL_UNSIGNED_SHORT_5_5_5_1 &&
         !swapBytes;

   case MESA_FORMAT_B5G5R5A1_UNORM:
      return format == GL_BGRA && type == GL_UNSIGNED_SHORT_1_5_5_5_REV &&
         !swapBytes;

   case MESA_FORMAT_A1R5G5B5_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_L4A4_UNORM:
      return GL_FALSE;
   case MESA_FORMAT_L8A8_UNORM:
   case MESA_FORMAT_L8A8_SRGB:
      return format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_BYTE && littleEndian;
   case MESA_FORMAT_A8L8_UNORM:
   case MESA_FORMAT_A8L8_SRGB:
      return GL_FALSE;

   case MESA_FORMAT_L16A16_UNORM:
      return format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_SHORT && littleEndian && !swapBytes;
   case MESA_FORMAT_A16L16_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_B2G3R3_UNORM:
      return format == GL_RGB && type == GL_UNSIGNED_BYTE_3_3_2;

   case MESA_FORMAT_A_UNORM8:
      return format == GL_ALPHA && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_A_UNORM16:
      return format == GL_ALPHA && type == GL_UNSIGNED_SHORT && !swapBytes;
   case MESA_FORMAT_L_UNORM8:
   case MESA_FORMAT_L_SRGB8:
      return format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_L_UNORM16:
      return format == GL_LUMINANCE && type == GL_UNSIGNED_SHORT && !swapBytes;
   case MESA_FORMAT_I_UNORM8:
      return format == GL_RED && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_I_UNORM16:
      return format == GL_RED && type == GL_UNSIGNED_SHORT && !swapBytes;

   case MESA_FORMAT_YCBCR:
      return format == GL_YCBCR_MESA &&
             ((type == GL_UNSIGNED_SHORT_8_8_MESA && littleEndian != swapBytes) ||
              (type == GL_UNSIGNED_SHORT_8_8_REV_MESA && littleEndian == swapBytes));
   case MESA_FORMAT_YCBCR_REV:
      return format == GL_YCBCR_MESA &&
             ((type == GL_UNSIGNED_SHORT_8_8_MESA && littleEndian == swapBytes) ||
              (type == GL_UNSIGNED_SHORT_8_8_REV_MESA && littleEndian != swapBytes));

   case MESA_FORMAT_R_UNORM8:
      return format == GL_RED && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_R8G8_UNORM:
      return format == GL_RG && type == GL_UNSIGNED_BYTE && littleEndian;
   case MESA_FORMAT_G8R8_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_R_UNORM16:
      return format == GL_RED && type == GL_UNSIGNED_SHORT &&
         !swapBytes;
   case MESA_FORMAT_R16G16_UNORM:
      return format == GL_RG && type == GL_UNSIGNED_SHORT && littleEndian &&
         !swapBytes;
   case MESA_FORMAT_G16R16_UNORM:
      return GL_FALSE;

   case MESA_FORMAT_B10G10R10A2_UNORM:
      return format == GL_BGRA && type == GL_UNSIGNED_INT_2_10_10_10_REV &&
         !swapBytes;

   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      return format == GL_DEPTH_STENCIL && type == GL_UNSIGNED_INT_24_8 &&
         !swapBytes;
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      return GL_FALSE;

   case MESA_FORMAT_Z_UNORM16:
      return format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_SHORT &&
         !swapBytes;

   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      return GL_FALSE;

   case MESA_FORMAT_Z_UNORM32:
      return format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_INT &&
         !swapBytes;

   case MESA_FORMAT_S_UINT8:
      return format == GL_STENCIL_INDEX && type == GL_UNSIGNED_BYTE;

   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
      return GL_FALSE;

   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
      return GL_FALSE;

   case MESA_FORMAT_BPTC_RGBA_UNORM:
   case MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM:
   case MESA_FORMAT_BPTC_RGB_SIGNED_FLOAT:
   case MESA_FORMAT_BPTC_RGB_UNSIGNED_FLOAT:
      return GL_FALSE;

   case MESA_FORMAT_RGBA_FLOAT32:
      return format == GL_RGBA && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_RGBA_FLOAT16:
      return format == GL_RGBA && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_RGB_FLOAT32:
      return format == GL_RGB && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_RGB_FLOAT16:
      return format == GL_RGB && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_A_FLOAT32:
      return format == GL_ALPHA && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_A_FLOAT16:
      return format == GL_ALPHA && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_L_FLOAT32:
      return format == GL_LUMINANCE && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_L_FLOAT16:
      return format == GL_LUMINANCE && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_LA_FLOAT32:
      return format == GL_LUMINANCE_ALPHA && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_LA_FLOAT16:
      return format == GL_LUMINANCE_ALPHA && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_I_FLOAT32:
      return format == GL_RED && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_I_FLOAT16:
      return format == GL_RED && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_R_FLOAT32:
      return format == GL_RED && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_R_FLOAT16:
      return format == GL_RED && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_RG_FLOAT32:
      return format == GL_RG && type == GL_FLOAT && !swapBytes;
   case MESA_FORMAT_RG_FLOAT16:
      return format == GL_RG && type == GL_HALF_FLOAT && !swapBytes;

   case MESA_FORMAT_A_UINT8:
      return format == GL_ALPHA_INTEGER && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_A_UINT16:
      return format == GL_ALPHA_INTEGER && type == GL_UNSIGNED_SHORT &&
             !swapBytes;
   case MESA_FORMAT_A_UINT32:
      return format == GL_ALPHA_INTEGER && type == GL_UNSIGNED_INT &&
             !swapBytes;
   case MESA_FORMAT_A_SINT8:
      return format == GL_ALPHA_INTEGER && type == GL_BYTE;
   case MESA_FORMAT_A_SINT16:
      return format == GL_ALPHA_INTEGER && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_A_SINT32:
      return format == GL_ALPHA_INTEGER && type == GL_INT && !swapBytes;

   case MESA_FORMAT_I_UINT8:
      return format == GL_RED_INTEGER && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_I_UINT16:
      return format == GL_RED_INTEGER && type == GL_UNSIGNED_SHORT && !swapBytes;
   case MESA_FORMAT_I_UINT32:
      return format == GL_RED_INTEGER && type == GL_UNSIGNED_INT && !swapBytes;
   case MESA_FORMAT_I_SINT8:
      return format == GL_RED_INTEGER && type == GL_BYTE;
   case MESA_FORMAT_I_SINT16:
      return format == GL_RED_INTEGER && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_I_SINT32:
      return format == GL_RED_INTEGER && type == GL_INT && !swapBytes;

   case MESA_FORMAT_L_UINT8:
      return format == GL_LUMINANCE_INTEGER_EXT && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_L_UINT16:
      return format == GL_LUMINANCE_INTEGER_EXT && type == GL_UNSIGNED_SHORT &&
             !swapBytes;
   case MESA_FORMAT_L_UINT32:
      return format == GL_LUMINANCE_INTEGER_EXT && type == GL_UNSIGNED_INT &&
             !swapBytes;
   case MESA_FORMAT_L_SINT8:
      return format == GL_LUMINANCE_INTEGER_EXT && type == GL_BYTE;
   case MESA_FORMAT_L_SINT16:
      return format == GL_LUMINANCE_INTEGER_EXT && type == GL_SHORT &&
             !swapBytes;
   case MESA_FORMAT_L_SINT32:
      return format == GL_LUMINANCE_INTEGER_EXT && type == GL_INT && !swapBytes;

   case MESA_FORMAT_LA_UINT8:
      return format == GL_LUMINANCE_ALPHA_INTEGER_EXT &&
             type == GL_UNSIGNED_BYTE && !swapBytes;
   case MESA_FORMAT_LA_UINT16:
      return format == GL_LUMINANCE_ALPHA_INTEGER_EXT &&
             type == GL_UNSIGNED_SHORT && !swapBytes;
   case MESA_FORMAT_LA_UINT32:
      return format == GL_LUMINANCE_ALPHA_INTEGER_EXT &&
             type == GL_UNSIGNED_INT && !swapBytes;
   case MESA_FORMAT_LA_SINT8:
      return format == GL_LUMINANCE_ALPHA_INTEGER_EXT && type == GL_BYTE &&
             !swapBytes;
   case MESA_FORMAT_LA_SINT16:
      return format == GL_LUMINANCE_ALPHA_INTEGER_EXT && type == GL_SHORT &&
             !swapBytes;
   case MESA_FORMAT_LA_SINT32:
      return format == GL_LUMINANCE_ALPHA_INTEGER_EXT && type == GL_INT &&
             !swapBytes;

   case MESA_FORMAT_R_SINT8:
      return format == GL_RED_INTEGER && type == GL_BYTE;
   case MESA_FORMAT_RG_SINT8:
      return format == GL_RG_INTEGER && type == GL_BYTE && !swapBytes;
   case MESA_FORMAT_RGB_SINT8:
      return format == GL_RGB_INTEGER && type == GL_BYTE && !swapBytes;
   case MESA_FORMAT_RGBA_SINT8:
      return format == GL_RGBA_INTEGER && type == GL_BYTE && !swapBytes;
   case MESA_FORMAT_R_SINT16:
      return format == GL_RED_INTEGER && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_RG_SINT16:
      return format == GL_RG_INTEGER && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_RGB_SINT16:
      return format == GL_RGB_INTEGER && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_RGBA_SINT16:
      return format == GL_RGBA_INTEGER && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_R_SINT32:
      return format == GL_RED_INTEGER && type == GL_INT && !swapBytes;
   case MESA_FORMAT_RG_SINT32:
      return format == GL_RG_INTEGER && type == GL_INT && !swapBytes;
   case MESA_FORMAT_RGB_SINT32:
      return format == GL_RGB_INTEGER && type == GL_INT && !swapBytes;
   case MESA_FORMAT_RGBA_SINT32:
      return format == GL_RGBA_INTEGER && type == GL_INT && !swapBytes;

   case MESA_FORMAT_R_UINT8:
      return format == GL_RED_INTEGER && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_RG_UINT8:
      return format == GL_RG_INTEGER && type == GL_UNSIGNED_BYTE && !swapBytes;
   case MESA_FORMAT_RGB_UINT8:
      return format == GL_RGB_INTEGER && type == GL_UNSIGNED_BYTE && !swapBytes;
   case MESA_FORMAT_RGBA_UINT8:
      return format == GL_RGBA_INTEGER && type == GL_UNSIGNED_BYTE &&
             !swapBytes;
   case MESA_FORMAT_R_UINT16:
      return format == GL_RED_INTEGER && type == GL_UNSIGNED_SHORT &&
             !swapBytes;
   case MESA_FORMAT_RG_UINT16:
      return format == GL_RG_INTEGER && type == GL_UNSIGNED_SHORT && !swapBytes;
   case MESA_FORMAT_RGB_UINT16:
      return format == GL_RGB_INTEGER && type == GL_UNSIGNED_SHORT &&
             !swapBytes;
   case MESA_FORMAT_RGBA_UINT16:
      return format == GL_RGBA_INTEGER && type == GL_UNSIGNED_SHORT &&
             !swapBytes;
   case MESA_FORMAT_R_UINT32:
      return format == GL_RED_INTEGER && type == GL_UNSIGNED_INT && !swapBytes;
   case MESA_FORMAT_RG_UINT32:
      return format == GL_RG_INTEGER && type == GL_UNSIGNED_INT && !swapBytes;
   case MESA_FORMAT_RGB_UINT32:
      return format == GL_RGB_INTEGER && type == GL_UNSIGNED_INT && !swapBytes;
   case MESA_FORMAT_RGBA_UINT32:
      return format == GL_RGBA_INTEGER && type == GL_UNSIGNED_INT && !swapBytes;

   case MESA_FORMAT_R_SNORM8:
      return format == GL_RED && type == GL_BYTE;
   case MESA_FORMAT_R8G8_SNORM:
      return format == GL_RG && type == GL_BYTE && littleEndian &&
             !swapBytes;
   case MESA_FORMAT_X8B8G8R8_SNORM:
      return GL_FALSE;

   case MESA_FORMAT_A8B8G8R8_SNORM:
      if (format == GL_RGBA && type == GL_BYTE && !littleEndian)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_BYTE && littleEndian)
         return GL_TRUE;

      return GL_FALSE;

   case MESA_FORMAT_R8G8B8A8_SNORM:
      if (format == GL_RGBA && type == GL_BYTE && littleEndian)
         return GL_TRUE;

      if (format == GL_ABGR_EXT && type == GL_BYTE && !littleEndian)
         return GL_TRUE;

      return GL_FALSE;

   case MESA_FORMAT_R_SNORM16:
      return format == GL_RED && type == GL_SHORT &&
             !swapBytes;
   case MESA_FORMAT_R16G16_SNORM:
      return format == GL_RG && type == GL_SHORT && littleEndian && !swapBytes;
   case MESA_FORMAT_RGB_SNORM16:
      return format == GL_RGB && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_RGBA_SNORM16:
      return format == GL_RGBA && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_RGBA_UNORM16:
      return format == GL_RGBA && type == GL_UNSIGNED_SHORT &&
             !swapBytes;

   case MESA_FORMAT_R_RGTC1_UNORM:
   case MESA_FORMAT_R_RGTC1_SNORM:
   case MESA_FORMAT_RG_RGTC2_UNORM:
   case MESA_FORMAT_RG_RGTC2_SNORM:
      return GL_FALSE;

   case MESA_FORMAT_L_LATC1_UNORM:
   case MESA_FORMAT_L_LATC1_SNORM:
   case MESA_FORMAT_LA_LATC2_UNORM:
   case MESA_FORMAT_LA_LATC2_SNORM:
      return GL_FALSE;

   case MESA_FORMAT_ETC1_RGB8:
   case MESA_FORMAT_ETC2_RGB8:
   case MESA_FORMAT_ETC2_SRGB8:
   case MESA_FORMAT_ETC2_RGBA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_R11_EAC:
   case MESA_FORMAT_ETC2_RG11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
   case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
   case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
      return GL_FALSE;

   case MESA_FORMAT_A_SNORM8:
      return format == GL_ALPHA && type == GL_BYTE;
   case MESA_FORMAT_L_SNORM8:
      return format == GL_LUMINANCE && type == GL_BYTE;
   case MESA_FORMAT_L8A8_SNORM:
      return format == GL_LUMINANCE_ALPHA && type == GL_BYTE &&
             littleEndian && !swapBytes;
   case MESA_FORMAT_A8L8_SNORM:
      return format == GL_LUMINANCE_ALPHA && type == GL_BYTE &&
             !littleEndian && !swapBytes;
   case MESA_FORMAT_I_SNORM8:
      return format == GL_RED && type == GL_BYTE;
   case MESA_FORMAT_A_SNORM16:
      return format == GL_ALPHA && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_L_SNORM16:
      return format == GL_LUMINANCE && type == GL_SHORT && !swapBytes;
   case MESA_FORMAT_LA_SNORM16:
      return format == GL_LUMINANCE_ALPHA && type == GL_SHORT &&
             littleEndian && !swapBytes;
   case MESA_FORMAT_I_SNORM16:
      return format == GL_RED && type == GL_SHORT && littleEndian &&
             !swapBytes;

   case MESA_FORMAT_B10G10R10A2_UINT:
      return (format == GL_BGRA_INTEGER_EXT &&
              type == GL_UNSIGNED_INT_2_10_10_10_REV &&
              !swapBytes);

   case MESA_FORMAT_R10G10B10A2_UINT:
      return (format == GL_RGBA_INTEGER_EXT &&
              type == GL_UNSIGNED_INT_2_10_10_10_REV &&
              !swapBytes);

   case MESA_FORMAT_R9G9B9E5_FLOAT:
      return format == GL_RGB && type == GL_UNSIGNED_INT_5_9_9_9_REV &&
         !swapBytes;

   case MESA_FORMAT_R11G11B10_FLOAT:
      return format == GL_RGB && type == GL_UNSIGNED_INT_10F_11F_11F_REV &&
         !swapBytes;

   case MESA_FORMAT_Z_FLOAT32:
      return format == GL_DEPTH_COMPONENT && type == GL_FLOAT && !swapBytes;

   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return format == GL_DEPTH_STENCIL &&
             type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV && !swapBytes;

   case MESA_FORMAT_B4G4R4X4_UNORM:
   case MESA_FORMAT_B5G5R5X1_UNORM:
   case MESA_FORMAT_R8G8B8X8_SNORM:
   case MESA_FORMAT_R8G8B8X8_SRGB:
   case MESA_FORMAT_X8B8G8R8_SRGB:
   case MESA_FORMAT_RGBX_UINT8:
   case MESA_FORMAT_RGBX_SINT8:
   case MESA_FORMAT_B10G10R10X2_UNORM:
   case MESA_FORMAT_RGBX_UNORM16:
   case MESA_FORMAT_RGBX_SNORM16:
   case MESA_FORMAT_RGBX_FLOAT16:
   case MESA_FORMAT_RGBX_UINT16:
   case MESA_FORMAT_RGBX_SINT16:
   case MESA_FORMAT_RGBX_FLOAT32:
   case MESA_FORMAT_RGBX_UINT32:
   case MESA_FORMAT_RGBX_SINT32:
      return GL_FALSE;

   case MESA_FORMAT_R10G10B10A2_UNORM:
      return format == GL_RGBA && type == GL_UNSIGNED_INT_2_10_10_10_REV &&
         !swapBytes;

   case MESA_FORMAT_G8R8_SNORM:
      return format == GL_RG && type == GL_BYTE && !littleEndian &&
         !swapBytes;

   case MESA_FORMAT_G16R16_SNORM:
      return format == GL_RG && type == GL_SHORT && !littleEndian &&
         !swapBytes;

   case MESA_FORMAT_B8G8R8X8_SRGB:
   case MESA_FORMAT_X8R8G8B8_SRGB:
      return GL_FALSE;
   }

   return GL_FALSE;
}

