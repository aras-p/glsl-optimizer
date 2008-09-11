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
   config->Handle = (EGLConfig) id;
   _eglSetConfigAttrib(config, EGL_CONFIG_ID,               id);
   _eglSetConfigAttrib(config, EGL_BIND_TO_TEXTURE_RGB,     EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_BIND_TO_TEXTURE_RGBA,    EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_CONFIG_CAVEAT,           EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_NATIVE_RENDERABLE,       EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_NATIVE_VISUAL_TYPE,      EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_MIN_SWAP_INTERVAL,       EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_MAX_SWAP_INTERVAL,       EGL_DONT_CARE);
   _eglSetConfigAttrib(config, EGL_SURFACE_TYPE,            EGL_WINDOW_BIT);
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
 * Return the public handle for an internal _EGLConfig.
 * This is the inverse of _eglLookupConfig().
 */
EGLConfig
_eglGetConfigHandle(_EGLConfig *config)
{
   return config ? config->Handle : 0;
}


/**
 * Given an EGLConfig handle, return the corresponding _EGLConfig object.
 * This is the inverse of _eglGetConfigHandle().
 */
_EGLConfig *
_eglLookupConfig(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config)
{
   EGLint i;
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   for (i = 0; i < disp->NumConfigs; i++) {
      if (disp->Configs[i]->Handle == config) {
          return disp->Configs[i];
      }
   }
   return NULL;
}


/**
 * Add the given _EGLConfig to the given display.
 * Note that we just save the ptr to the config (we don't copy the config).
 */
_EGLConfig *
_eglAddConfig(_EGLDisplay *display, _EGLConfig *config)
{
   _EGLConfig **newConfigs;
   EGLint n;

   /* do some sanity checks on the config's attribs */
   assert(GET_CONFIG_ATTRIB(config, EGL_CONFIG_ID) > 0);
   assert(GET_CONFIG_ATTRIB(config, EGL_RENDERABLE_TYPE) != 0x0);
   assert(GET_CONFIG_ATTRIB(config, EGL_SURFACE_TYPE) != 0x0);
   assert(GET_CONFIG_ATTRIB(config, EGL_RED_SIZE) > 0);
   assert(GET_CONFIG_ATTRIB(config, EGL_GREEN_SIZE) > 0);
   assert(GET_CONFIG_ATTRIB(config, EGL_BLUE_SIZE) > 0);

   n = display->NumConfigs;

   /* realloc array of ptrs */
   newConfigs = (_EGLConfig **) realloc(display->Configs,
                                        (n + 1) * sizeof(_EGLConfig *));
   if (newConfigs) {
      display->Configs = newConfigs;
      display->Configs[n] = config;
      display->NumConfigs++;
      return config;
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
         if (renType & ~(EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENVG_BIT)) {
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
      if (_eglConfigQualifies(disp->Configs[i], &criteria)) {
         configList[count++] = disp->Configs[i];
      }
   }

   /* sort array of pointers */
   qsort(configList, count, sizeof(_EGLConfig *), _eglCompareConfigs);

   /* copy config handles to output array */
   if (configs) {
      for (i = 0; i < count; i++) {
         configs[i] = configList[i]->Handle;
      }
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
         configs[i] = disp->Configs[i]->Handle;
      }
   }
   else {
      /* just return total number of supported configs */
      *num_config = disp->NumConfigs;
   }

   return EGL_TRUE;
}
