#include "intel_context.h"
#include "intel_tex.h"
#include "main/enums.h"
#include "main/formats.h"

/**
 * Choose hardware texture format given the user's glTexImage parameters.
 *
 * It works out that this function is fine for all the supported
 * hardware.  However, there is still a need to map the formats onto
 * hardware descriptors.
 *
 * Note that the i915 can actually support many more formats than
 * these if we take the step of simply swizzling the colors
 * immediately after sampling...
 */
gl_format
intelChooseTextureFormat(struct gl_context * ctx, GLint internalFormat,
                         GLenum format, GLenum type)
{
   struct intel_context *intel = intel_context(ctx);

#if 0
   printf("%s intFmt=0x%x format=0x%x type=0x%x\n",
          __FUNCTION__, internalFormat, format, type);
#endif

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV)
	 return MESA_FORMAT_ARGB4444;
      else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV)
	 return MESA_FORMAT_ARGB1555;
      else
	 return MESA_FORMAT_ARGB8888;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if (type == GL_UNSIGNED_SHORT_5_6_5)
	 return MESA_FORMAT_RGB565;
      else if (intel->has_xrgb_textures)
	 return MESA_FORMAT_XRGB8888;
      else
	 return MESA_FORMAT_ARGB8888;

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return MESA_FORMAT_ARGB8888;

   case GL_RGBA4:
   case GL_RGBA2:
      return MESA_FORMAT_ARGB4444;

   case GL_RGB5_A1:
      return MESA_FORMAT_ARGB1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      if (intel->has_xrgb_textures)
	 return MESA_FORMAT_XRGB8888;
      else
	 return MESA_FORMAT_ARGB8888;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return MESA_FORMAT_RGB565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return MESA_FORMAT_A8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return MESA_FORMAT_L8;

   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      /* i915 could implement this mode using MT_32BIT_RG1616.  However, this
       * would require an extra swizzle instruction in the fragment shader to
       * convert the { R, G, 1.0, 1.0 } to { R, R, R, G }.
       */
#ifndef I915
      return MESA_FORMAT_AL1616;
#else
      /* FALLTHROUGH */
#endif

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return MESA_FORMAT_AL88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return MESA_FORMAT_I8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE)
         return MESA_FORMAT_YCBCR;
      else
         return MESA_FORMAT_YCBCR_REV;

   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return MESA_FORMAT_RGB_FXT1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return MESA_FORMAT_RGBA_FXT1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return MESA_FORMAT_RGB_DXT1;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return MESA_FORMAT_RGBA_DXT1;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return MESA_FORMAT_RGBA_DXT3;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return MESA_FORMAT_RGBA_DXT5;

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
#if 0
      return MESA_FORMAT_Z16;
#else
      /* fall-through.
       * 16bpp depth texture can't be paired with a stencil buffer so
       * always used combined depth/stencil format.
       */
#endif
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      return MESA_FORMAT_S8_Z24;

#ifndef I915
   case GL_SRGB_EXT:
   case GL_SRGB8_EXT:
   case GL_SRGB_ALPHA_EXT:
   case GL_SRGB8_ALPHA8_EXT:
   case GL_COMPRESSED_SRGB_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_EXT:
   case GL_COMPRESSED_SLUMINANCE_EXT:
   case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
      return MESA_FORMAT_SARGB8;
   case GL_SLUMINANCE_EXT:
   case GL_SLUMINANCE8_EXT:
      if (intel->has_luminance_srgb)
         return MESA_FORMAT_SL8;
      else
         return MESA_FORMAT_SARGB8;
   case GL_SLUMINANCE_ALPHA_EXT:
   case GL_SLUMINANCE8_ALPHA8_EXT:
      if (intel->has_luminance_srgb)
         return MESA_FORMAT_SLA8;
      else
         return MESA_FORMAT_SARGB8;
   case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      return MESA_FORMAT_SRGB_DXT1;

   /* i915 could also do this */
   case GL_DUDV_ATI:
   case GL_DU8DV8_ATI:
      return MESA_FORMAT_DUDV8;
   case GL_RGBA_SNORM:
   case GL_RGBA8_SNORM:
      return MESA_FORMAT_SIGNED_RGBA8888_REV;

   /* i915 can do a RG16, but it can't do any of the other RED or RG formats.
    * In addition, it only implements the broken D3D mode where undefined
    * components are read as 1.0.  I'm not sure who thought reading
    * { R, G, 1.0, 1.0 } from a red-green texture would be useful.
    */
   case GL_RED:
   case GL_R8:
      return MESA_FORMAT_R8;
   case GL_R16:
      return MESA_FORMAT_R16;
   case GL_RG:
   case GL_RG8:
      return MESA_FORMAT_RG88;
   case GL_RG16:
      return MESA_FORMAT_RG1616;
#endif

   default:
      fprintf(stderr, "unexpected texture format %s in %s\n",
              _mesa_lookup_enum_by_nr(internalFormat), __FUNCTION__);
      return MESA_FORMAT_NONE;
   }

   return MESA_FORMAT_NONE;       /* never get here */
}

int intel_compressed_num_bytes(GLuint mesaFormat)
{
   GLuint bw, bh;
   GLuint block_size;

   block_size = _mesa_get_format_bytes(mesaFormat);
   _mesa_get_format_block_size(mesaFormat, &bw, &bh);

   return block_size / bw;
}
