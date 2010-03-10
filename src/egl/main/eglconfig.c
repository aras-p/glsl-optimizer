/**
 * EGL Configuration (pixel format) functions.
 */


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "eglconfig.h"
#include "egldisplay.h"
#include "eglcurrent.h"
#include "egllog.h"


#define MIN2(A, B)  (((A) < (B)) ? (A) : (B))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


/**
 * Init the given _EGLconfig to default values.
 * \param id  the configuration's ID.
 *
 * Note that id must be positive for the config to be valid.
 * It is also recommended that when there are N configs, their
 * IDs are from 1 to N respectively.
 */
void
_eglInitConfig(_EGLConfig *config, _EGLDisplay *dpy, EGLint id)
{
   memset(config, 0, sizeof(*config));

   config->Display = dpy;

   /* some attributes take non-zero default values */
   SET_CONFIG_ATTRIB(config, EGL_CONFIG_ID,               id);
   SET_CONFIG_ATTRIB(config, EGL_CONFIG_CAVEAT,           EGL_NONE);
   SET_CONFIG_ATTRIB(config, EGL_TRANSPARENT_TYPE,        EGL_NONE);
   SET_CONFIG_ATTRIB(config, EGL_NATIVE_VISUAL_TYPE,      EGL_NONE);
#ifdef EGL_VERSION_1_2
   SET_CONFIG_ATTRIB(config, EGL_COLOR_BUFFER_TYPE,       EGL_RGB_BUFFER);
#endif /* EGL_VERSION_1_2 */
}


/**
 * Link a config to a display and return the handle of the link.
 * The handle can be passed to client directly.
 *
 * Note that we just save the ptr to the config (we don't copy the config).
 */
EGLConfig
_eglAddConfig(_EGLDisplay *dpy, _EGLConfig *conf)
{
   _EGLConfig **configs;

   /* sanity check */
   assert(GET_CONFIG_ATTRIB(conf, EGL_CONFIG_ID) > 0);

   configs = dpy->Configs;
   if (dpy->NumConfigs >= dpy->MaxConfigs) {
      EGLint new_size = dpy->MaxConfigs + 16;
      assert(dpy->NumConfigs < new_size);

      configs = realloc(dpy->Configs, new_size * sizeof(dpy->Configs[0]));
      if (!configs)
         return (EGLConfig) NULL;

      dpy->Configs = configs;
      dpy->MaxConfigs = new_size;
   }

   conf->Display = dpy;
   dpy->Configs[dpy->NumConfigs++] = conf;

   return (EGLConfig) conf;
}


EGLBoolean
_eglCheckConfigHandle(EGLConfig config, _EGLDisplay *dpy)
{
   EGLint num_configs = (dpy) ? dpy->NumConfigs : 0;
   EGLint i;

   for (i = 0; i < num_configs; i++) {
      _EGLConfig *conf = dpy->Configs[i];
      if (conf == (_EGLConfig *) config) {
         assert(conf->Display == dpy);
         break;
      }
   }
   return (i < num_configs);
}


enum {
   /* types */
   ATTRIB_TYPE_INTEGER,
   ATTRIB_TYPE_BOOLEAN,
   ATTRIB_TYPE_BITMASK,
   ATTRIB_TYPE_ENUM,
   ATTRIB_TYPE_PSEUDO, /* non-queryable */
   ATTRIB_TYPE_PLATFORM, /* platform-dependent */
   /* criteria */
   ATTRIB_CRITERION_EXACT,
   ATTRIB_CRITERION_ATLEAST,
   ATTRIB_CRITERION_MASK,
   ATTRIB_CRITERION_SPECIAL,
   ATTRIB_CRITERION_IGNORE
};


/* EGL spec Table 3.1 and 3.4 */
static const struct {
   EGLint attr;
   EGLint type;
   EGLint criterion;
   EGLint default_value;
} _eglValidationTable[] =
{
   { EGL_BUFFER_SIZE,               ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_RED_SIZE,                  ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_GREEN_SIZE,                ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_BLUE_SIZE,                 ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_LUMINANCE_SIZE,            ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_ALPHA_SIZE,                ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_ALPHA_MASK_SIZE,           ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_BIND_TO_TEXTURE_RGB,       ATTRIB_TYPE_BOOLEAN,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_BIND_TO_TEXTURE_RGBA,      ATTRIB_TYPE_BOOLEAN,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_COLOR_BUFFER_TYPE,         ATTRIB_TYPE_ENUM,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_RGB_BUFFER },
   { EGL_CONFIG_CAVEAT,             ATTRIB_TYPE_ENUM,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_CONFIG_ID,                 ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_CONFORMANT,                ATTRIB_TYPE_BITMASK,
                                    ATTRIB_CRITERION_MASK,
                                    0 },
   { EGL_DEPTH_SIZE,                ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_LEVEL,                     ATTRIB_TYPE_PLATFORM,
                                    ATTRIB_CRITERION_EXACT,
                                    0 },
   { EGL_MAX_PBUFFER_WIDTH,         ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_IGNORE,
                                    0 },
   { EGL_MAX_PBUFFER_HEIGHT,        ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_IGNORE,
                                    0 },
   { EGL_MAX_PBUFFER_PIXELS,        ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_IGNORE,
                                    0 },
   { EGL_MAX_SWAP_INTERVAL,         ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_MIN_SWAP_INTERVAL,         ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_NATIVE_RENDERABLE,         ATTRIB_TYPE_BOOLEAN,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_NATIVE_VISUAL_ID,          ATTRIB_TYPE_PLATFORM,
                                    ATTRIB_CRITERION_IGNORE,
                                    0 },
   { EGL_NATIVE_VISUAL_TYPE,        ATTRIB_TYPE_PLATFORM,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_RENDERABLE_TYPE,           ATTRIB_TYPE_BITMASK,
                                    ATTRIB_CRITERION_MASK,
                                    EGL_OPENGL_ES_BIT },
   { EGL_SAMPLE_BUFFERS,            ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_SAMPLES,                   ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_STENCIL_SIZE,              ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_ATLEAST,
                                    0 },
   { EGL_SURFACE_TYPE,              ATTRIB_TYPE_BITMASK,
                                    ATTRIB_CRITERION_MASK,
                                    EGL_WINDOW_BIT },
   { EGL_TRANSPARENT_TYPE,          ATTRIB_TYPE_ENUM,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_NONE },
   { EGL_TRANSPARENT_RED_VALUE,     ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_TRANSPARENT_GREEN_VALUE,   ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   { EGL_TRANSPARENT_BLUE_VALUE,    ATTRIB_TYPE_INTEGER,
                                    ATTRIB_CRITERION_EXACT,
                                    EGL_DONT_CARE },
   /* these are not real attributes */
   { EGL_MATCH_NATIVE_PIXMAP,       ATTRIB_TYPE_PSEUDO,
                                    ATTRIB_CRITERION_SPECIAL,
                                    EGL_NONE },
   /* there is a gap before EGL_SAMPLES */
   { 0x3030,                        ATTRIB_TYPE_PSEUDO,
                                    ATTRIB_CRITERION_IGNORE,
                                    0 },
   { EGL_NONE,                      ATTRIB_TYPE_PSEUDO,
                                    ATTRIB_CRITERION_IGNORE,
                                    0 }
};


/**
 * Return true if a config is valid.  When for_matching is true,
 * EGL_DONT_CARE is accepted as a valid attribute value, and checks
 * for conflicting attribute values are skipped.
 *
 * Note that some attributes are platform-dependent and are not
 * checked.
 */
EGLBoolean
_eglValidateConfig(const _EGLConfig *conf, EGLBoolean for_matching)
{
   EGLint i, attr, val;
   EGLBoolean valid = EGL_TRUE;
   EGLint red_size = 0, green_size = 0, blue_size = 0, luminance_size = 0;
   EGLint alpha_size = 0, buffer_size = 0;

   /* all attributes should have been listed */
   assert(ARRAY_SIZE(_eglValidationTable) == _EGL_CONFIG_NUM_ATTRIBS);

   /* check attributes by their types */
   for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
      EGLint mask;

      attr = _eglValidationTable[i].attr;
      val = GET_CONFIG_ATTRIB(conf, attr);

      switch (_eglValidationTable[i].type) {
      case ATTRIB_TYPE_INTEGER:
         switch (attr) {
         case EGL_CONFIG_ID:
            /* config id must be positive */
            if (val <= 0)
               valid = EGL_FALSE;
            break;
         case EGL_SAMPLE_BUFFERS:
            /* there can be at most 1 sample buffer */
            if (val > 1)
               valid = EGL_FALSE;
            break;
         case EGL_RED_SIZE:
            red_size = val;
            break;
         case EGL_GREEN_SIZE:
            green_size = val;
            break;
         case EGL_BLUE_SIZE:
            blue_size = val;
            break;
         case EGL_LUMINANCE_SIZE:
            luminance_size = val;
            break;
         case EGL_ALPHA_SIZE:
            alpha_size = val;
            break;
         case EGL_BUFFER_SIZE:
            buffer_size = val;
            break;
         }
         if (val < 0)
            valid = EGL_FALSE;
         break;
      case ATTRIB_TYPE_BOOLEAN:
         if (val != EGL_TRUE && val != EGL_FALSE)
            valid = EGL_FALSE;
         break;
      case ATTRIB_TYPE_ENUM:
         switch (attr) {
         case EGL_CONFIG_CAVEAT:
            if (val != EGL_NONE && val != EGL_SLOW_CONFIG &&
                val != EGL_NON_CONFORMANT_CONFIG)
               valid = EGL_FALSE;
            break;
         case EGL_TRANSPARENT_TYPE:
            if (val != EGL_NONE && val != EGL_TRANSPARENT_RGB)
               valid = EGL_FALSE;
            break;
         case EGL_COLOR_BUFFER_TYPE:
            if (val != EGL_RGB_BUFFER && val != EGL_LUMINANCE_BUFFER)
               valid = EGL_FALSE;
            break;
         default:
            assert(0);
            break;
         }
         break;
      case ATTRIB_TYPE_BITMASK:
         switch (attr) {
         case EGL_SURFACE_TYPE:
            mask = EGL_PBUFFER_BIT |
                   EGL_PIXMAP_BIT |
                   EGL_WINDOW_BIT |
                   EGL_VG_COLORSPACE_LINEAR_BIT |
                   EGL_VG_ALPHA_FORMAT_PRE_BIT |
                   EGL_MULTISAMPLE_RESOLVE_BOX_BIT |
                   EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
            if (conf->Display->Extensions.MESA_screen_surface)
               mask |= EGL_SCREEN_BIT_MESA;
            break;
         case EGL_RENDERABLE_TYPE:
         case EGL_CONFORMANT:
            mask = EGL_OPENGL_ES_BIT |
                   EGL_OPENVG_BIT |
                   EGL_OPENGL_ES2_BIT |
                   EGL_OPENGL_BIT;
            break;
         default:
            assert(0);
            break;
         }
         if (val & ~mask)
            valid = EGL_FALSE;
         break;
      case ATTRIB_TYPE_PLATFORM:
         /* unable to check platform-dependent attributes here */
         break;
      case ATTRIB_TYPE_PSEUDO:
         /* pseudo attributes should not be set */
         if (val != 0)
            valid = EGL_FALSE;
         break;
      default:
         assert(0);
         break;
      }

      if (!valid && for_matching) {
         /* accept EGL_DONT_CARE as a valid value */
         if (val == EGL_DONT_CARE)
            valid = EGL_TRUE;
         if (_eglValidationTable[i].criterion == ATTRIB_CRITERION_SPECIAL)
            valid = EGL_TRUE;
      }
      if (!valid) {
         _eglLog(_EGL_DEBUG,
               "attribute 0x%04x has an invalid value 0x%x", attr, val);
         break;
      }
   }

   /* any invalid attribute value should have been catched */
   if (!valid || for_matching)
      return valid;

   /* now check for conflicting attribute values */

   switch (GET_CONFIG_ATTRIB(conf, EGL_COLOR_BUFFER_TYPE)) {
   case EGL_RGB_BUFFER:
      if (luminance_size)
         valid = EGL_FALSE;
      if (red_size + green_size + blue_size + alpha_size != buffer_size)
         valid = EGL_FALSE;
      break;
   case EGL_LUMINANCE_BUFFER:
      if (red_size || green_size || blue_size)
         valid = EGL_FALSE;
      if (luminance_size + alpha_size != buffer_size)
         valid = EGL_FALSE;
      break;
   }
   if (!valid) {
      _eglLog(_EGL_DEBUG, "conflicting color buffer type and channel sizes");
      return EGL_FALSE;
   }

   val = GET_CONFIG_ATTRIB(conf, EGL_SAMPLE_BUFFERS);
   if (!val && GET_CONFIG_ATTRIB(conf, EGL_SAMPLES))
      valid = EGL_FALSE;
   if (!valid) {
      _eglLog(_EGL_DEBUG, "conflicting samples and sample buffers");
      return EGL_FALSE;
   }

   val = GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE);
   if (!(val & EGL_WINDOW_BIT)) {
      if (GET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_ID) != 0 ||
          GET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_TYPE) != EGL_NONE)
         valid = EGL_FALSE;
   }
   if (!(val & EGL_PBUFFER_BIT)) {
      if (GET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGB) ||
          GET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGBA))
         valid = EGL_FALSE;
   }
   if (!valid) {
      _eglLog(_EGL_DEBUG, "conflicting surface type and native visual/texture binding");
      return EGL_FALSE;
   }

   return valid;
}


/**
 * Return true if a config matches the criteria.  This and
 * _eglParseConfigAttribList together implement the algorithm
 * described in "Selection of EGLConfigs".
 *
 * Note that attributes that are special (currently, only
 * EGL_MATCH_NATIVE_PIXMAP) are ignored.
 */
EGLBoolean
_eglMatchConfig(const _EGLConfig *conf, const _EGLConfig *criteria)
{
   EGLint attr, val, i;
   EGLBoolean matched = EGL_TRUE;

   for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
      EGLint cmp;
      if (_eglValidationTable[i].criterion == ATTRIB_CRITERION_IGNORE)
         continue;

      attr = _eglValidationTable[i].attr;
      cmp = GET_CONFIG_ATTRIB(criteria, attr);
      if (cmp == EGL_DONT_CARE)
         continue;

      val = GET_CONFIG_ATTRIB(conf, attr);
      switch (_eglValidationTable[i].criterion) {
      case ATTRIB_CRITERION_EXACT:
         if (val != cmp)
            matched = EGL_FALSE;
         break;
      case ATTRIB_CRITERION_ATLEAST:
         if (val < cmp)
            matched = EGL_FALSE;
         break;
      case ATTRIB_CRITERION_MASK:
         if ((val & cmp) != cmp)
            matched = EGL_FALSE;
         break;
      case ATTRIB_CRITERION_SPECIAL:
         /* ignored here */
         break;
      default:
         assert(0);
         break;
      }

      if (!matched) {
#ifdef DEBUG
         _eglLog(_EGL_DEBUG,
               "the value (0x%x) of attribute 0x%04x did not meet the criteria (0x%x)",
               val, attr, cmp);
#endif
         break;
      }
   }

   return matched;
}


/**
 * Initialize a criteria config from the given attribute list.
 * Return EGL_FALSE if any of the attribute is invalid.
 */
EGLBoolean
_eglParseConfigAttribList(_EGLConfig *conf, const EGLint *attrib_list)
{
   EGLint attr, val, i;
   EGLint config_id = 0, level = 0;
   EGLBoolean has_native_visual_type = EGL_FALSE;
   EGLBoolean has_transparent_color = EGL_FALSE;

   /* reset to default values */
   for (i = 0; i < ARRAY_SIZE(_eglValidationTable); i++) {
      attr = _eglValidationTable[i].attr;
      val = _eglValidationTable[i].default_value;
      SET_CONFIG_ATTRIB(conf, attr, val);
   }

   /* parse the list */
   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i += 2) {
      EGLint idx;

      attr = attrib_list[i];
      val = attrib_list[i + 1];

      idx = _eglIndexConfig(conf, attr);
      if (idx < 0)
         return EGL_FALSE;
      conf->Storage[idx] = val;

      /* rememeber some attributes for post-processing */
      switch (attr) {
      case EGL_CONFIG_ID:
         config_id = val;
         break;
      case EGL_LEVEL:
         level = val;
         break;
      case EGL_NATIVE_VISUAL_TYPE:
         has_native_visual_type = EGL_TRUE;
         break;
      case EGL_TRANSPARENT_RED_VALUE:
      case EGL_TRANSPARENT_GREEN_VALUE:
      case EGL_TRANSPARENT_BLUE_VALUE:
         has_transparent_color = EGL_TRUE;
         break;
      default:
         break;
      }
   }

   if (!_eglValidateConfig(conf, EGL_TRUE))
      return EGL_FALSE;

   /* the spec says that EGL_LEVEL cannot be EGL_DONT_CARE */
   if (level == EGL_DONT_CARE)
      return EGL_FALSE;

   /* ignore other attributes when EGL_CONFIG_ID is given */
   if (config_id > 0) {
      _eglResetConfigKeys(conf, EGL_DONT_CARE);
      SET_CONFIG_ATTRIB(conf, EGL_CONFIG_ID, config_id);
   }
   else {
      if (has_native_visual_type) {
         val = GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE);
         if (!(val & EGL_WINDOW_BIT))
            SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_TYPE, EGL_DONT_CARE);
      }

      if (has_transparent_color) {
         val = GET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_TYPE);
         if (val == EGL_NONE) {
            SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_RED_VALUE,
                              EGL_DONT_CARE);
            SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_GREEN_VALUE,
                              EGL_DONT_CARE);
            SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_BLUE_VALUE,
                              EGL_DONT_CARE);
         }
      }
   }

   return EGL_TRUE;
}


/**
 * Decide the ordering of conf1 and conf2, under the given criteria.
 * When compare_id is true, this implements the algorithm described
 * in "Sorting of EGLConfigs".  When compare_id is false,
 * EGL_CONFIG_ID is not compared.
 *
 * It returns a negative integer if conf1 is considered to come
 * before conf2;  a positive integer if conf2 is considered to come
 * before conf1;  zero if the ordering cannot be decided.
 *
 * Note that EGL_NATIVE_VISUAL_TYPE is platform-dependent and is
 * ignored here.
 */
EGLint
_eglCompareConfigs(const _EGLConfig *conf1, const _EGLConfig *conf2,
                   const _EGLConfig *criteria, EGLBoolean compare_id)
{
   const EGLint compare_attribs[] = {
      EGL_BUFFER_SIZE,
      EGL_SAMPLE_BUFFERS,
      EGL_SAMPLES,
      EGL_DEPTH_SIZE,
      EGL_STENCIL_SIZE,
      EGL_ALPHA_MASK_SIZE,
   };
   EGLint val1, val2;
   EGLBoolean rgb_buffer;
   EGLint i;

   if (conf1 == conf2)
      return 0;

   /* the enum values have the desired ordering */
   assert(EGL_NONE < EGL_SLOW_CONFIG);
   assert(EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG);
   val1 = GET_CONFIG_ATTRIB(conf1, EGL_CONFIG_CAVEAT);
   val2 = GET_CONFIG_ATTRIB(conf2, EGL_CONFIG_CAVEAT);
   if (val1 != val2)
      return (val1 - val2);

   /* the enum values have the desired ordering */
   assert(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER);
   val1 = GET_CONFIG_ATTRIB(conf1, EGL_COLOR_BUFFER_TYPE);
   val2 = GET_CONFIG_ATTRIB(conf2, EGL_COLOR_BUFFER_TYPE);
   if (val1 != val2)
      return (val1 - val2);
   rgb_buffer = (val1 == EGL_RGB_BUFFER);

   if (criteria) {
      val1 = val2 = 0;
      if (rgb_buffer) {
         if (GET_CONFIG_ATTRIB(criteria, EGL_RED_SIZE) > 0) {
            val1 += GET_CONFIG_ATTRIB(conf1, EGL_RED_SIZE);
            val2 += GET_CONFIG_ATTRIB(conf2, EGL_RED_SIZE);
         }
         if (GET_CONFIG_ATTRIB(criteria, EGL_GREEN_SIZE) > 0) {
            val1 += GET_CONFIG_ATTRIB(conf1, EGL_GREEN_SIZE);
            val2 += GET_CONFIG_ATTRIB(conf2, EGL_GREEN_SIZE);
         }
         if (GET_CONFIG_ATTRIB(criteria, EGL_BLUE_SIZE) > 0) {
            val1 += GET_CONFIG_ATTRIB(conf1, EGL_BLUE_SIZE);
            val2 += GET_CONFIG_ATTRIB(conf2, EGL_BLUE_SIZE);
         }
      }
      else {
         if (GET_CONFIG_ATTRIB(criteria, EGL_LUMINANCE_SIZE) > 0) {
            val1 += GET_CONFIG_ATTRIB(conf1, EGL_LUMINANCE_SIZE);
            val2 += GET_CONFIG_ATTRIB(conf2, EGL_LUMINANCE_SIZE);
         }
      }
      if (GET_CONFIG_ATTRIB(criteria, EGL_ALPHA_SIZE) > 0) {
         val1 += GET_CONFIG_ATTRIB(conf1, EGL_ALPHA_SIZE);
         val2 += GET_CONFIG_ATTRIB(conf2, EGL_ALPHA_SIZE);
      }
   }
   else {
      /* assume the default criteria, which gives no specific ordering */
      val1 = val2 = 0;
   }

   /* for color bits, larger one is preferred */
   if (val1 != val2)
      return (val2 - val1);

   for (i = 0; i < ARRAY_SIZE(compare_attribs); i++) {
      val1 = GET_CONFIG_ATTRIB(conf1, compare_attribs[i]);
      val2 = GET_CONFIG_ATTRIB(conf2, compare_attribs[i]);
      if (val1 != val2)
         return (val1 - val2);
   }

   /* EGL_NATIVE_VISUAL_TYPE cannot be compared here */

   if (compare_id) {
      val1 = GET_CONFIG_ATTRIB(conf1, EGL_CONFIG_ID);
      val2 = GET_CONFIG_ATTRIB(conf2, EGL_CONFIG_ID);
      assert(val1 != val2);
   }
   else {
      val1 = val2 = 0;
   }

   return (val1 - val2);
}


static INLINE
void _eglSwapConfigs(const _EGLConfig **conf1, const _EGLConfig **conf2)
{
   const _EGLConfig *tmp = *conf1;
   *conf1 = *conf2;
   *conf2 = tmp;
}


/**
 * Quick sort an array of configs.  This differs from the standard
 * qsort() in that the compare function accepts an additional
 * argument.
 */
void
_eglSortConfigs(const _EGLConfig **configs, EGLint count,
                EGLint (*compare)(const _EGLConfig *, const _EGLConfig *,
                                  void *),
                void *priv_data)
{
   const EGLint pivot = 0;
   EGLint i, j;

   if (count <= 1)
      return;

   _eglSwapConfigs(&configs[pivot], &configs[count / 2]);
   i = 1;
   j = count - 1;
   do {
      while (i < count && compare(configs[i], configs[pivot], priv_data) < 0)
         i++;
      while (compare(configs[j], configs[pivot], priv_data) > 0)
         j--;
      if (i < j) {
         _eglSwapConfigs(&configs[i], &configs[j]);
         i++;
         j--;
      }
      else if (i == j) {
         i++;
         j--;
         break;
      }
   } while (i <= j);
   _eglSwapConfigs(&configs[pivot], &configs[j]);

   _eglSortConfigs(configs, j, compare, priv_data);
   _eglSortConfigs(configs + i, count - i, compare, priv_data);
}


static int
_eglFallbackCompare(const _EGLConfig *conf1, const _EGLConfig *conf2,
                   void *priv_data)
{
   const _EGLConfig *criteria = (const _EGLConfig *) priv_data;
   return _eglCompareConfigs(conf1, conf2, criteria, EGL_TRUE);
}


/**
 * Typical fallback routine for eglChooseConfig
 */
EGLBoolean
_eglChooseConfig(_EGLDriver *drv, _EGLDisplay *disp, const EGLint *attrib_list,
                 EGLConfig *configs, EGLint config_size, EGLint *num_configs)
{
   _EGLConfig **configList, criteria;
   EGLint i, count;

   if (!num_configs)
      return _eglError(EGL_BAD_PARAMETER, "eglChooseConfigs");

   _eglInitConfig(&criteria, disp, 0);
   if (!_eglParseConfigAttribList(&criteria, attrib_list))
      return _eglError(EGL_BAD_ATTRIBUTE, "eglChooseConfig");

   /* allocate array of config pointers */
   configList = (_EGLConfig **)
      malloc(disp->NumConfigs * sizeof(_EGLConfig *));
   if (!configList)
      return _eglError(EGL_BAD_ALLOC, "eglChooseConfig(out of memory)");

   /* perform selection of configs */
   count = 0;
   for (i = 0; i < disp->NumConfigs; i++) {
      if (_eglMatchConfig(disp->Configs[i], &criteria))
         configList[count++] = disp->Configs[i];
   }

   /* perform sorting of configs */
   if (configs && count) {
      _eglSortConfigs((const _EGLConfig **) configList, count,
                      _eglFallbackCompare, (void *) &criteria);
      count = MIN2(count, config_size);
      for (i = 0; i < count; i++)
         configs[i] = _eglGetConfigHandle(configList[i]);
   }

   free(configList);

   *num_configs = count;

   return EGL_TRUE;
}


static INLINE EGLBoolean
_eglIsConfigAttribValid(_EGLConfig *conf, EGLint attr)
{
   if (_eglIndexConfig(conf, attr) < 0)
      return EGL_FALSE;

   /* there are some holes in the range */
   switch (attr) {
   case 0x3030 /* a gap before EGL_SAMPLES */:
   case EGL_NONE:
#ifdef EGL_VERSION_1_4
   case EGL_MATCH_NATIVE_PIXMAP:
#endif
      return EGL_FALSE;
   default:
      break;
   }

   return EGL_TRUE;
}


/**
 * Fallback for eglGetConfigAttrib.
 */
EGLBoolean
_eglGetConfigAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                    EGLint attribute, EGLint *value)
{
   if (!_eglIsConfigAttribValid(conf, attribute))
      return _eglError(EGL_BAD_ATTRIBUTE, "eglGetConfigAttrib");
   if (!value)
      return _eglError(EGL_BAD_PARAMETER, "eglGetConfigAttrib");

   *value = GET_CONFIG_ATTRIB(conf, attribute);
   return EGL_TRUE;
}


/**
 * Fallback for eglGetConfigs.
 */
EGLBoolean
_eglGetConfigs(_EGLDriver *drv, _EGLDisplay *disp, EGLConfig *configs,
               EGLint config_size, EGLint *num_config)
{
   if (!num_config)
      return _eglError(EGL_BAD_PARAMETER, "eglGetConfigs");

   if (configs) {
      EGLint i;
      *num_config = MIN2(disp->NumConfigs, config_size);
      for (i = 0; i < *num_config; i++)
         configs[i] = _eglGetConfigHandle(disp->Configs[i]);
   }
   else {
      /* just return total number of supported configs */
      *num_config = disp->NumConfigs;
   }

   return EGL_TRUE;
}
