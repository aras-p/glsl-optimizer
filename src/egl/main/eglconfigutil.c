/**
 * Extra utility functions related to EGL configs.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "eglconfigutil.h"
#include "egllog.h"


/**
 * Convert an _EGLConfig to a __GLcontextModes object.
 * NOTE: This routine may be incomplete - we're only making sure that
 * the fields needed by Mesa (for _mesa_create_context/framebuffer) are
 * set correctly.
 */
void
_eglConfigToContextModesRec(const _EGLConfig *config, __GLcontextModes *mode)
{
   memset(mode, 0, sizeof(*mode));

   mode->rgbMode = GL_TRUE; /* no color index */
   mode->colorIndexMode = GL_FALSE;
   mode->doubleBufferMode = GL_TRUE;  /* always DB for now */
   mode->stereoMode = GL_FALSE;

   mode->redBits = GET_CONFIG_ATTRIB(config, EGL_RED_SIZE);
   mode->greenBits = GET_CONFIG_ATTRIB(config, EGL_GREEN_SIZE);
   mode->blueBits = GET_CONFIG_ATTRIB(config, EGL_BLUE_SIZE);
   mode->alphaBits = GET_CONFIG_ATTRIB(config, EGL_ALPHA_SIZE);
   mode->rgbBits = GET_CONFIG_ATTRIB(config, EGL_BUFFER_SIZE);

   /* no rgba masks - fix? */

   mode->depthBits = GET_CONFIG_ATTRIB(config, EGL_DEPTH_SIZE);
   mode->haveDepthBuffer = mode->depthBits > 0;

   mode->stencilBits = GET_CONFIG_ATTRIB(config, EGL_STENCIL_SIZE);
   mode->haveStencilBuffer = mode->stencilBits > 0;

   /* no accum */

   mode->level = GET_CONFIG_ATTRIB(config, EGL_LEVEL);
   mode->samples = GET_CONFIG_ATTRIB(config, EGL_SAMPLES);
   mode->sampleBuffers = GET_CONFIG_ATTRIB(config, EGL_SAMPLE_BUFFERS);

   /* surface type - not really needed */
   mode->visualType = GLX_TRUE_COLOR;
   mode->renderType = GLX_RGBA_BIT;
}



/**
 * Creates a set of \c _EGLConfigs that a driver will expose.
 * 
 * A set of \c __GLcontextModes will be created based on the supplied
 * parameters.  The number of modes processed will be 2 *
 * \c num_depth_stencil_bits * \c num_db_modes.
 * 
 * For the most part, data is just copied from \c depth_bits, \c stencil_bits,
 * \c db_modes, and \c visType into each \c __GLcontextModes element.
 * However, the meanings of \c fb_format and \c fb_type require further
 * explanation.  The \c fb_format specifies which color components are in
 * each pixel and what the default order is.  For example, \c GL_RGB specifies
 * that red, green, blue are available and red is in the "most significant"
 * position and blue is in the "least significant".  The \c fb_type specifies
 * the bit sizes of each component and the actual ordering.  For example, if
 * \c GL_UNSIGNED_SHORT_5_6_5_REV is specified with \c GL_RGB, bits [15:11]
 * are the blue value, bits [10:5] are the green value, and bits [4:0] are
 * the red value.
 * 
 * One sublte issue is the combination of \c GL_RGB  or \c GL_BGR and either
 * of the \c GL_UNSIGNED_INT_8_8_8_8 modes.  The resulting mask values in the
 * \c __GLcontextModes structure is \b identical to the \c GL_RGBA or
 * \c GL_BGRA case, except the \c alphaMask is zero.  This means that, as
 * far as this routine is concerned, \c GL_RGB with \c GL_UNSIGNED_INT_8_8_8_8
 * still uses 32-bits.
 *
 * If in doubt, look at the tables used in the function.
 * 
 * \param configs       the array of configs generated
 * \param fb_format     Format of the framebuffer.  Currently only \c GL_RGB,
 *                      \c GL_RGBA, \c GL_BGR, and \c GL_BGRA are supported.
 * \param fb_type       Type of the pixels in the framebuffer.  Currently only
 *                      \c GL_UNSIGNED_SHORT_5_6_5, 
 *                      \c GL_UNSIGNED_SHORT_5_6_5_REV,
 *                      \c GL_UNSIGNED_INT_8_8_8_8, and
 *                      \c GL_UNSIGNED_INT_8_8_8_8_REV are supported.
 * \param depth_bits    Array of depth buffer sizes to be exposed.
 * \param stencil_bits  Array of stencil buffer sizes to be exposed.
 * \param num_depth_stencil_bits  Number of entries in both \c depth_bits and
 *                      \c stencil_bits.
 * \param db_modes      Array of buffer swap modes.  If an element has a
 *                      value of \c GLX_NONE, then it represents a
 *                      single-buffered mode.  Other valid values are
 *                      \c GLX_SWAP_EXCHANGE_OML, \c GLX_SWAP_COPY_OML, and
 *                      \c GLX_SWAP_UNDEFINED_OML.  See the
 *                      GLX_OML_swap_method extension spec for more details.
 * \param num_db_modes  Number of entries in \c db_modes.
 * \param visType       GLX visual type.  Usually either \c GLX_TRUE_COLOR or
 *                      \c GLX_DIRECT_COLOR.
 * 
 * \returns
 * \c GL_TRUE on success or \c GL_FALSE on failure.  Currently the only
 * cause of failure is a bad parameter (i.e., unsupported \c fb_format or
 * \c fb_type).
 * 
 * \todo
 * There is currently no way to support packed RGB modes (i.e., modes with
 * exactly 3 bytes per pixel) or floating-point modes.  This could probably
 * be done by creating some new, private enums with clever names likes
 * \c GL_UNSIGNED_3BYTE_8_8_8, \c GL_4FLOAT_32_32_32_32, 
 * \c GL_4HALF_16_16_16_16, etc.  We can cross that bridge when we come to it.
 */
EGLBoolean
_eglFillInConfigs(_EGLConfig * configs,
                  GLenum fb_format, GLenum fb_type,
                  const uint8_t * depth_bits, const uint8_t * stencil_bits,
                  unsigned num_depth_stencil_bits,
                  const GLenum * db_modes, unsigned num_db_modes,
                  int visType)
{
#if 0
   static const uint8_t bits_table[3][4] = {
            /* R  G  B  A */
            { 5, 6, 5, 0 },  /* Any GL_UNSIGNED_SHORT_5_6_5 */
            { 8, 8, 8, 0 },  /* Any RGB with any GL_UNSIGNED_INT_8_8_8_8 */
            { 8, 8, 8, 8 }  /* Any RGBA with any GL_UNSIGNED_INT_8_8_8_8 */
         };

   /* The following arrays are all indexed by the fb_type masked with 0x07.
    * Given the four supported fb_type values, this results in valid array
    * indices of 3, 4, 5, and 7.
    */
   static const uint32_t masks_table_rgb[8][4] = {
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x0000F800, 0x000007E0, 0x0000001F, 0x00000000},  /* 5_6_5       */
            {0x0000001F, 0x000007E0, 0x0000F800, 0x00000000},  /* 5_6_5_REV   */
            {0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000},  /* 8_8_8_8     */
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000}  /* 8_8_8_8_REV */
         };

   static const uint32_t masks_table_rgba[8][4] = {
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x0000F800, 0x000007E0, 0x0000001F, 0x00000000},  /* 5_6_5       */
            {0x0000001F, 0x000007E0, 0x0000F800, 0x00000000},  /* 5_6_5_REV   */
            {0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF},  /* 8_8_8_8     */
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000},  /* 8_8_8_8_REV */
         };

   static const uint32_t masks_table_bgr[8][4] = {
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x0000001F, 0x000007E0, 0x0000F800, 0x00000000},  /* 5_6_5       */
            {0x0000F800, 0x000007E0, 0x0000001F, 0x00000000},  /* 5_6_5_REV   */
            {0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000},  /* 8_8_8_8     */
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000},  /* 8_8_8_8_REV */
         };

   static const uint32_t masks_table_bgra[8][4] = {
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x0000001F, 0x000007E0, 0x0000F800, 0x00000000},  /* 5_6_5       */
            {0x0000F800, 0x000007E0, 0x0000001F, 0x00000000},  /* 5_6_5_REV   */
            {0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF},  /* 8_8_8_8     */
            {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000},  /* 8_8_8_8_REV */
         };

   static const uint8_t bytes_per_pixel[8] = {
            0, 0, 0, 2, 2, 4, 0, 4
         };

   const uint8_t * bits;
   const uint32_t * masks;
   const int index = fb_type & 0x07;
   _EGLConfig *config;
   unsigned i;
   unsigned j;
   unsigned k;

   if ( bytes_per_pixel[index] == 0 ) {
      _eglLog(_EGL_INFO,
              "[%s:%u] Framebuffer type 0x%04x has 0 bytes per pixel.",
              __FUNCTION__, __LINE__, fb_type);
      return GL_FALSE;
   }

   /* Valid types are GL_UNSIGNED_SHORT_5_6_5 and GL_UNSIGNED_INT_8_8_8_8 and
    * the _REV versions.
    *
    * Valid formats are GL_RGBA, GL_RGB, and GL_BGRA.
    */
   switch ( fb_format ) {
   case GL_RGB:
      bits = (bytes_per_pixel[index] == 2) ? bits_table[0] : bits_table[1];
      masks = masks_table_rgb[index];
      break;

   case GL_RGBA:
      bits = (bytes_per_pixel[index] == 2) ? bits_table[0] : bits_table[2];
      masks = masks_table_rgba[index];
      break;

   case GL_BGR:
      bits = (bytes_per_pixel[index] == 2) ? bits_table[0] : bits_table[1];
      masks = masks_table_bgr[index];
      break;

   case GL_BGRA:
      bits = (bytes_per_pixel[index] == 2) ? bits_table[0] : bits_table[2];
      masks = masks_table_bgra[index];
      break;

   default:
      _eglLog(_EGL_WARNING,
              "[%s:%u] Framebuffer format 0x%04x is not GL_RGB, GL_RGBA, GL_BGR, or GL_BGRA.",
              __FUNCTION__, __LINE__, fb_format);
      return GL_FALSE;
   }

   config = configs;
   for (k = 0; k < num_depth_stencil_bits; k++) {
      for (i = 0; i < num_db_modes; i++)  {
         for (j = 0; j < 2; j++) {
            _eglSetConfigAttrib(config, EGL_RED_SIZE, bits[0]);
            _eglSetConfigAttrib(config, EGL_GREEN_SIZE, bits[1]);
            _eglSetConfigAttrib(config, EGL_BLUE_SIZE, bits[2]);
            _eglSetConfigAttrib(config, EGL_ALPHA_SIZE, bits[3]);
            _eglSetConfigAttrib(config, EGL_BUFFER_SIZE,
                                bits[0] + bits[1] + bits[2] + bits[3]);

            _eglSetConfigAttrib(config, EGL_STENCIL_SIZE, stencil_bits[k]);
            _eglSetConfigAttrib(config, EGL_DEPTH_SIZE, depth_bits[i]);

            _eglSetConfigAttrib(config, EGL_SURFACE_TYPE, EGL_SCREEN_BIT_MESA |
                         EGL_PBUFFER_BIT | EGL_PIXMAP_BIT | EGL_WINDOW_BIT);

            config++;
         }
      }
   }
   return GL_TRUE;
#else
   return GL_FALSE;
#endif
}

