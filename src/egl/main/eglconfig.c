#include <string.h>
#include <assert.h>
#include "eglconfig.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"


#define MIN2(A, B)  (((A) < (B)) ? (A) : (B))


/**
 * Init the given _EGLconfig to default values.
 * \param id  the configuration's ID.
 */
void
_eglInitConfig(_EGLConfig *config, EGLint id)
{
   memset(config, 0, sizeof(*config));
   config->Handle = id;
   SET_CONFIG_ATTRIB(config, EGL_CONFIG_ID,               id);
   SET_CONFIG_ATTRIB(config, EGL_BIND_TO_TEXTURE_RGB,     EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_BIND_TO_TEXTURE_RGBA,    EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_CONFIG_CAVEAT,           EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_NATIVE_RENDERABLE,       EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_NATIVE_VISUAL_TYPE,      EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_MIN_SWAP_INTERVAL,       EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_MAX_SWAP_INTERVAL,       EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_SURFACE_TYPE,            EGL_WINDOW_BIT);
   SET_CONFIG_ATTRIB(config, EGL_TRANSPARENT_TYPE,        EGL_NONE);
   SET_CONFIG_ATTRIB(config, EGL_TRANSPARENT_RED_VALUE,   EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_TRANSPARENT_GREEN_VALUE, EGL_DONT_CARE);
   SET_CONFIG_ATTRIB(config, EGL_TRANSPARENT_BLUE_VALUE,  EGL_DONT_CARE);
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
 * Parse the attrib_list to fill in the fields of the given _egl_config
 * Return EGL_FALSE if any errors, EGL_TRUE otherwise.
 */
EGLBoolean
_eglParseConfigAttribs(_EGLConfig *config, const EGLint *attrib_list)
{
   EGLint i;

   /* XXX set all config attribs to EGL_DONT_CARE */

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      EGLint k = attrib_list[i] - FIRST_ATTRIB;
      if (k >= 0 && k < MAX_ATTRIBS) {
         config->Attrib[k] = attrib_list[++i];
      }
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
EGLBoolean
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
 */
EGLint
_eglCompareConfigs(const _EGLConfig *a, const _EGLConfig *b)
{
   EGLint i;
   for (i = 0; SortInfo[i].Attribute != 0; i++) {
      const EGLint av = GET_CONFIG_ATTRIB(a, SortInfo[i].Attribute);
      const EGLint bv = GET_CONFIG_ATTRIB(b, SortInfo[i].Attribute);
      if (SortInfo[i].SortOrder == SMALLER) {
         if (av < bv)
            return -1;
         else if (av > bv)
            return 1;
         /* else, continue examining attribute values */
      }
      else if (SortInfo[i].SortOrder == SPECIAL) {
         if (SortInfo[i].Attribute == EGL_CONFIG_CAVEAT) {
            /* values are EGL_NONE, SLOW_CONFIG, or NON_CONFORMANT_CONFIG */
            if (av < bv)
               return -1;
            else if (av > bv)
               return 1;
         }
         else if (SortInfo[i].Attribute == EGL_RED_SIZE ||
                  SortInfo[i].Attribute == EGL_GREEN_SIZE ||
                  SortInfo[i].Attribute == EGL_BLUE_SIZE ||
                  SortInfo[i].Attribute == EGL_ALPHA_SIZE) {
            if (av > bv)
               return -1;
            else if (av < bv)
               return 1;
         }
         else {
            assert(SortInfo[i].Attribute == EGL_NATIVE_VISUAL_TYPE);
            if (av < bv)
               return -1;
            else if (av > bv)
               return 1;
         }
      }
      else {
         assert(SortInfo[i].SortOrder == NONE);
         /* continue examining attribute values */
      }
   }
   return 0;
}


/**
 * Typical fallback routine for eglChooseConfig
 */
EGLBoolean
_eglChooseConfig(_EGLDriver *drv, EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   _EGLConfig criteria;
   EGLint i;

   /* parse the attrib_list to initialize criteria */
   if (!_eglParseConfigAttribs(&criteria, attrib_list)) {
      return EGL_FALSE;
   }

   *num_config = 0;
   for (i = 0; i < disp->NumConfigs; i++) {
      const _EGLConfig *conf = disp->Configs + i;
      if (_eglConfigQualifies(conf, &criteria)) {
         if (*num_config < config_size) {
            /* save */
            configs[*num_config] = conf->Handle;
            (*num_config)++;
         }
         else {
            break;
         }
      }
   }

   /* XXX sort the list here */

   return EGL_TRUE;
}


/**
 * Fallback for eglGetConfigAttrib.
 */
EGLBoolean
_eglGetConfigAttrib(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
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
_eglGetConfigs(_EGLDriver *drv, EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);

   if (!drv->Initialized) {
      _eglError(EGL_NOT_INITIALIZED, "eglGetConfigs");
      return EGL_FALSE;
   }

   *num_config = MIN2(disp->NumConfigs, config_size);
   if (configs) {
      EGLint i;
      for (i = 0; i < *num_config; i++) {
         configs[i] = disp->Configs[i].Handle;
      }
   }
   return EGL_TRUE;
}
