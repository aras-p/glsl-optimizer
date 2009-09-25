#ifndef EGLCONFIG_INCLUDED
#define EGLCONFIG_INCLUDED


#include <assert.h>
#include "egltypedefs.h"


#define _EGL_CONFIG_FIRST_ATTRIB EGL_BUFFER_SIZE
#define _EGL_CONFIG_LAST_ATTRIB EGL_CONFORMANT
#define _EGL_CONFIG_NUM_ATTRIBS \
   (_EGL_CONFIG_LAST_ATTRIB - _EGL_CONFIG_FIRST_ATTRIB + 1)

#define _EGL_CONFIG_STORAGE_SIZE _EGL_CONFIG_NUM_ATTRIBS


struct _egl_config
{
   EGLConfig Handle;   /* the public/opaque handle which names this config */
   EGLint Storage[_EGL_CONFIG_STORAGE_SIZE];
};


#define SET_CONFIG_ATTRIB(CONF, ATTR, VAL) _eglSetConfigKey(CONF, ATTR, VAL)
#define GET_CONFIG_ATTRIB(CONF, ATTR) _eglGetConfigKey(CONF, ATTR)


/**
 * Given a key, return an index into the storage of the config.
 * Return -1 if the key is invalid.
 */
static INLINE EGLint
_eglIndexConfig(const _EGLConfig *conf, EGLint key)
{
   (void) conf;
   if (key >= _EGL_CONFIG_FIRST_ATTRIB &&
       key < _EGL_CONFIG_FIRST_ATTRIB + _EGL_CONFIG_NUM_ATTRIBS)
      return key - _EGL_CONFIG_FIRST_ATTRIB;
   else
      return -1;
}


/**
 * Reset all keys in the config to a given value.
 */
static INLINE void
_eglResetConfigKeys(_EGLConfig *conf, EGLint val)
{
   EGLint i;
   for (i = 0; i < _EGL_CONFIG_NUM_ATTRIBS; i++)
      conf->Storage[i] = val;
}


/**
 * Update a config for a given key.
 */
static INLINE void
_eglSetConfigKey(_EGLConfig *conf, EGLint key, EGLint val)
{
   EGLint idx = _eglIndexConfig(conf, key);
   assert(idx >= 0);
   conf->Storage[idx] = val;
}


/**
 * Return the value for a given key.
 */
static INLINE EGLint
_eglGetConfigKey(const _EGLConfig *conf, EGLint key)
{
   EGLint idx = _eglIndexConfig(conf, key);
   assert(idx >= 0);
   return conf->Storage[idx];
}


/**
 * Set a given attribute.
 *
 * Because _eglGetConfigAttrib is already used as a fallback driver
 * function, this function is not considered to have a good name.
 * SET_CONFIG_ATTRIB is preferred over this function.
 */
static INLINE void
_eglSetConfigAttrib(_EGLConfig *conf, EGLint attr, EGLint val)
{
   SET_CONFIG_ATTRIB(conf, attr, val);
}


extern void
_eglInitConfig(_EGLConfig *config, EGLint id);


extern EGLConfig
_eglGetConfigHandle(_EGLConfig *config);


extern _EGLConfig *
_eglLookupConfig(EGLConfig config, _EGLDisplay *dpy);


extern _EGLConfig *
_eglAddConfig(_EGLDisplay *display, _EGLConfig *config);


extern EGLBoolean
_eglParseConfigAttribs(_EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglChooseConfig(_EGLDriver *drv, _EGLDisplay *dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);


extern EGLBoolean
_eglGetConfigAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglGetConfigs(_EGLDriver *drv, _EGLDisplay *dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);


#endif /* EGLCONFIG_INCLUDED */
