#ifndef EGLCONFIGUTIL_INCLUDED
#define EGLCONFIGUTIL_INCLUDED


#include "GL/gl.h"
#include "GL/internal/glcore.h"
#include "eglconfig.h"


PUBLIC void
_eglConfigToContextModesRec(const _EGLConfig *config, __GLcontextModes *mode);


PUBLIC EGLBoolean
_eglConfigFromContextModesRec(_EGLConfig *conf, const __GLcontextModes *m,
                              EGLint conformant, EGLint renderable_type);


#endif /* EGLCONFIGUTIL_INCLUDED */
