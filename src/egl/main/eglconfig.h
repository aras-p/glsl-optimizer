#ifndef EGLCONFIG_INCLUDED
#define EGLCONFIG_INCLUDED


#include "egltypedefs.h"


#define MAX_ATTRIBS 100
#define FIRST_ATTRIB EGL_BUFFER_SIZE


struct _egl_config
{
   EGLConfig Handle;   /* the public/opaque handle which names this config */
   EGLint Attrib[MAX_ATTRIBS];
};



#define SET_CONFIG_ATTRIB(CONF, ATTR, VAL) ((CONF)->Attrib[(ATTR) - FIRST_ATTRIB] = VAL)
#define GET_CONFIG_ATTRIB(CONF, ATTR) ((CONF)->Attrib[(ATTR) - FIRST_ATTRIB])


extern void
_eglInitConfig(_EGLConfig *config, EGLint id);


extern _EGLConfig *
_eglLookupConfig(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config);


extern EGLBoolean
_eglParseConfigAttribs(_EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglConfigQualifies(const _EGLConfig *c, const _EGLConfig *min);


extern EGLint
_eglCompareConfigs(const _EGLConfig *a, const _EGLConfig *b);


extern EGLBoolean
_eglChooseConfig(_EGLDriver *drv, EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);


extern EGLBoolean
_eglGetConfigAttrib(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglGetConfigs(_EGLDriver *drv, EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);


#endif /* EGLCONFIG_INCLUDED */
