/**
 * EGL Configuration (pixel format) functions.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "eglconfig.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"


#define MIN2(A, B)  (((A) < (B)) ? (A) : (B))


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


void
_eglSetConfigAttrib(_EGLConfig *config, EGLint attr, EGLint val)
{
   assert(attr >= FIRST_ATTRIB);
   assert(attr < FIRST_ATTRIB + MAX_ATTRIBS);
   config->Attrib[attr - FIRST_ATTRIB] = val;
}


/**
 * Init the given _EGLconfig to default values.
 * \param id  the configuration's ID.
 */
void
_eglInitConfig(_EGLConfig *config, EGLint id)
{
   memset(config, 0, sizeof(*config));
   config->Handle = id;
   _eglSetConfigAttrib(config, EGL_CONFIG_ID,               id);
   _eglSetConfigAttrib(config, EGL_BIND_TO_TEXTURE_RGB,     EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_BIND_TO_TEXTURE_RGBA,    EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_CONFIG_CAVEAT,           EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_NATIVE_RENDERABLE,       EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_NATIVE_VISUAL_TYPE,      EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_MIN_SWAP_INTERVAL,       EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_MAX_SWAP_INTERVAL,       EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_SURFACE_TYPE,            
                   EGL_SCREEN_BIT_MESA | EGL_PBUFFER_BIT |
                   EGL_PIXMAP_BIT | EGL_WINDOW_BIT);
   _eglSetConfigAttrib(config, EGL_TRANSPARENT_TYPE,        EGL_NONE);
   _eglSetConfigAttrib(config, EGL_TRANSPARENT_RED_VALUE,   EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_TRANSPARENT_GREEN_VALUE, EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_TRANSPARENT_BLUE_VALUE,  EGL_DONT_CARE);
#ifdef EGL_VERSION_1_2
   _eglSetConfigAttrib(config, EGL_COLOR_BUFFER_TYPE,       EGL_RGB_BUFFER);
   _eglSetConfigAttrib(config, EGL_RENDERABLE_TYPE,         EGL_OPENGL_ES_BIT);
#endif /* EGL_VERSION_1_2 */
}


/**
 * Given an EGLConfig handle, return the corresponding _EGLConfig object.
 */
_EGLConfig *
_eglLookupConfig(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config)
{
   EGLint i;
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   for (i = 0; i < disp->NumConfigs; i++) {
      if (disp->Configs[i].Handle == config) {
          return disp->Configs + i;
      }
   }
   return NULL;
}


/**
 * Add the given _EGLConfig to the given display.
 */
_EGLConfig *
_eglAddConfig(_EGLDisplay *display, const _EGLConfig *config)
{
   _EGLConfig *newConfigs;
   EGLint n;

   n = display->NumConfigs;

   newConfigs = (_EGLConfig *) realloc(display->Configs,
                                       (n + 1) * sizeof(_EGLConfig));
   if (newConfigs) {
      display->Configs = newConfigs;
      display->Configs[n] = *config; /* copy struct */
      display->Configs[n].Handle = n;
      display->NumConfigs++;
      return display->Configs + n;
   }
   else {
      return NULL;
   }
}


/**
 * Parse the attrib_list to fill in the fields of the given _eglConfig
 * Return EGL_FALSE if any errors, EGL_TRUE otherwise.
 */
EGLBoolean
_eglParseConfigAttribs(_EGLConfig *config, const EGLint *attrib_list)
{
   EGLint i;

   /* set all config attribs to EGL_DONT_CARE */
   for (i = 0; i < MAX_ATTRIBS; i++) {
      config->Attrib[i] = EGL_DONT_CARE;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      const EGLint attr = attrib_list[i];
      if (attr >= EGL_BUFFER_SIZE &&
          attr <= EGL_MAX_SWAP_INTERVAL) {
         EGLint k = attr - FIRST_ATTRIB;
         assert(k >= 0);
         assert(k < MAX_ATTRIBS);
         config->Attrib[k] = attrib_list[++i];
      }
#ifdef EGL_VERSION_1_2
      else if (attr == EGL_COLOR_BUFFER_TYPE) {
         EGLint bufType = attrib_list[++i];
         if (bufType != EGL_RGB_BUFFER && bufType != EGL_LUMINANCE_BUFFER) {
            _eglError(EGL_BAD_ATTRIBUTE, "eglChooseConfig");
            return EGL_FALSE;
         }
         _eglSetConfigAttrib(config, EGL_COLOR_BUFFER_TYPE, bufType);
      }
      else if (attr == EGL_RENDERABLE_TYPE) {
         EGLint renType = attrib_list[++i];
         if (renType & ~(EGL_OPENGL_ES_BIT | EGL_OPENVG_BIT)) {
            _eglError(EGL_BAD_ATTRIBUTE, "eglChooseConfig");
            return EGL_FALSE;
         }
         _eglSetConfigAttrib(config, EGL_RENDERABLE_TYPE, renType);
      }
      else if (attr == EGL_ALPHA_MASK_SIZE ||
               attr == EGL_LUMINANCE_SIZE) {
         EGLint value = attrib_list[++i];
         _eglSetConfigAttrib(config, attr, value);
      }
#endif /* EGL_VERSION_1_2 */
      else {
         _eglError(EGL_BAD_ATTRIBUTE, "eglChooseConfig");
         return EGL_FALSE;
      }
   }
   return EGL_TRUE;
}


#define EXACT 1
#define ATLEAST 2
#define MASK 3
#define SMALLER 4
#define SPECIAL 5
#define NONE 6

struct sort_info {
   EGLint Attribute;
   EGLint MatchCriteria;
   EGLint SortOrder;
};

/* This encodes the info from Table 3.5 of the EGL spec, ordered by
 * Sort Priority.
 *
 * XXX To do: EGL 1.2 attribs
 */
static struct sort_info SortInfo[] = {
   { EGL_CONFIG_CAVEAT,           EXACT,   SPECIAL },
   { EGL_RED_SIZE,                ATLEAST, SPECIAL },
   { EGL_GREEN_SIZE,              ATLEAST, SPECIAL },
   { EGL_BLUE_SIZE,               ATLEAST, SPECIAL },
   { EGL_ALPHA_SIZE,              ATLEAST, SPECIAL },
   { EGL_BUFFER_SIZE,             ATLEAST, SMALLER },
   { EGL_SAMPLE_BUFFERS,          ATLEAST, SMALLER },
   { EGL_SAMPLES,                 ATLEAST, SMALLER },
   { EGL_DEPTH_SIZE,              ATLEAST, SMALLER },
   { EGL_STENCIL_SIZE,            ATLEAST, SMALLER },
   { EGL_NATIVE_VISUAL_TYPE,      EXACT,   SPECIAL },
   { EGL_CONFIG_ID,               EXACT,   SMALLER },
   { EGL_BIND_TO_TEXTURE_RGB,     EXACT,   NONE    },
   { EGL_BIND_TO_TEXTURE_RGBA,    EXACT,   NONE    },
   { EGL_LEVEL,                   EXACT,   NONE    },
   { EGL_NATIVE_RENDERABLE,       EXACT,   NONE    },
   { EGL_MAX_SWAP_INTERVAL,       EXACT,   NONE    },
   { EGL_MIN_SWAP_INTERVAL,       EXACT,   NONE    },
   { EGL_SURFACE_TYPE,            MASK,    NONE    },
   { EGL_TRANSPARENT_TYPE,        EXACT,   NONE    },
   { EGL_TRANSPARENT_RED_VALUE,   EXACT,   NONE    },
   { EGL_TRANSPARENT_GREEN_VALUE, EXACT,   NONE    },
   { EGL_TRANSPARENT_BLUE_VALUE,  EXACT,   NONE    },
   { 0, 0, 0 }
};


/**
 * Return EGL_TRUE if the attributes of c meet or exceed the minimums
 * specified by min.
 */
static EGLBoolean
_eglConfigQualifies(const _EGLConfig *c, const _EGLConfig *min)
{
   EGLint i;
   for (i = 0; SortInfo[i].Attribute != 0; i++) {
      const EGLint mv = GET_CONFIG_ATTRIB(min, SortInfo[i].Attribute);
      if (mv != EGL_DONT_CARE) {
         const EGLint cv = GET_CONFIG_ATTRIB(c, SortInfo[i].Attribute);
         if (SortInfo[i].MatchCriteria == EXACT) {
            if (cv != mv) {
               return EGL_FALSE;
            }
         }
         else if (SortInfo[i].MatchCriteria == ATLEAST) {
            if (cv < mv) {
               return EGL_FALSE;
            }
         }
         else {
            assert(SortInfo[i].MatchCriteria == MASK);
            if ((mv & cv) != mv) {
               return EGL_FALSE;
            }
         }
      }
   }
   return EGL_TRUE;
}


/**
 * Compare configs 'a' and 'b' and return -1 if a belongs before b,
 * 1 if a belongs after b, or 0 if they're equal.
 * Used by qsort().
 */
static int
_eglCompareConfigs(const void *a, const void *b)
{
   const _EGLConfig *aConfig = (const _EGLConfig *) a;
   const _EGLConfig *bConfig = (const _EGLConfig *) b;
   EGLint i;

   for (i = 0; SortInfo[i].Attribute != 0; i++) {
      const EGLint aVal = GET_CONFIG_ATTRIB(aConfig, SortInfo[i].Attribute);
      const EGLint bVal = GET_CONFIG_ATTRIB(bConfig, SortInfo[i].Attribute);
      if (SortInfo[i].SortOrder == SMALLER) {
         if (aVal < bVal)
            return -1;
         else if (aVal > bVal)
            return 1;
         /* else, continue examining attribute values */
      }
      else if (SortInfo[i].SortOrder == SPECIAL) {
         if (SortInfo[i].Attribute == EGL_CONFIG_CAVEAT) {
            /* values are EGL_NONE, SLOW_CONFIG, or NON_CONFORMANT_CONFIG */
            if (aVal < bVal)
               return -1;
            else if (aVal > bVal)
               return 1;
         }
         else if (SortInfo[i].Attribute == EGL_RED_SIZE ||
                  SortInfo[i].Attribute == EGL_GREEN_SIZE ||
                  SortInfo[i].Attribute == EGL_BLUE_SIZE ||
                  SortInfo[i].Attribute == EGL_ALPHA_SIZE) {
            if (aVal > bVal)
               return -1;
            else if (aVal < bVal)
               return 1;
         }
         else {
            assert(SortInfo[i].Attribute == EGL_NATIVE_VISUAL_TYPE);
            if (aVal < bVal)
               return -1;
            else if (aVal > bVal)
               return 1;
         }
      }
      else {
         assert(SortInfo[i].SortOrder == NONE);
         /* continue examining attribute values */
      }
   }

   /* all attributes identical */
   return 0;
}


/**
 * Typical fallback routine for eglChooseConfig
 */
EGLBoolean
_eglChooseConfig(_EGLDriver *drv, EGLDisplay dpy, const EGLint *attrib_list,
                 EGLConfig *configs, EGLint config_size, EGLint *num_configs)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig **configList, criteria;
   EGLint i, count;

   /* parse the attrib_list to initialize criteria */
   if (!_eglParseConfigAttribs(&criteria, attrib_list)) {
      return EGL_FALSE;
   }

   /* allocate array of config pointers */
   configList = (_EGLConfig **) malloc(config_size * sizeof(_EGLConfig *));
   if (!configList) {
      _eglError(EGL_BAD_CONFIG, "eglChooseConfig(out of memory)");
      return EGL_FALSE;
   }

   /* make array of pointers to qualifying configs */
   for (i = count = 0; i < disp->NumConfigs && count < config_size; i++) {
      if (_eglConfigQualifies(disp->Configs + i, &criteria)) {
         configList[count++] = disp->Configs + i;
      }
   }

   /* sort array of pointers */
   qsort(configList, count, sizeof(_EGLConfig *), _eglCompareConfigs);

   /* copy config handles to output array */
   for (i = 0; i < count; i++) {
      configs[i] = configList[i]->Handle;
   }

   free(configList);

   *num_configs = count;

   return EGL_TRUE;
}


/**
 * Fallback for eglGetConfigAttrib.
 */
EGLBoolean
_eglGetConfigAttrib(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                    EGLint attribute, EGLint *value)
{
   const _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);
   const EGLint k = attribute - FIRST_ATTRIB;
   if (k >= 0 && k < MAX_ATTRIBS) {
      *value = conf->Attrib[k];
      return EGL_TRUE;
   }
   else {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetConfigAttrib");
      return EGL_FALSE;
   }
}


/**
 * Fallback for eglGetConfigs.
 */
EGLBoolean
_eglGetConfigs(_EGLDriver *drv, EGLDisplay dpy, EGLConfig *configs,
               EGLint config_size, EGLint *num_config)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   if (!drv->Initialized) {
      _eglError(EGL_NOT_INITIALIZED, "eglGetConfigs");
      return EGL_FALSE;
   }

   if (configs) {
      EGLint i;
      *num_config = MIN2(disp->NumConfigs, config_size);
      for (i = 0; i < *num_config; i++) {
         configs[i] = disp->Configs[i].Handle;
      }
   }
   else {
      /* just return total number of supported configs */
      *num_config = disp->NumConfigs;
   }

   return EGL_TRUE;
}


/**
 * Creates a set of \c __GLcontextModes that a driver will expose.
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
 * \param ptr_to_modes  Pointer to a pointer to a linked list of
 *                      \c __GLcontextModes.  Upon completion, a pointer to
 *                      the next element to be process will be stored here.
 *                      If the function fails and returns \c GL_FALSE, this
 *                      value will be unmodified, but some elements in the
 *                      linked list may be modified.
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
GLboolean
_eglFillInConfigs(_EGLConfig * configs,
                  GLenum fb_format, GLenum fb_type,
                  const uint8_t * depth_bits, const uint8_t * stencil_bits,
                  unsigned num_depth_stencil_bits,
                  const GLenum * db_modes, unsigned num_db_modes,
                  int visType)
{
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
}
