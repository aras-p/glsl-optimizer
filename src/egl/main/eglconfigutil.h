#ifndef EGLCONFIGUTIL_INCLUDED
#define EGLCONFIGUTIL_INCLUDED


#include "GL/gl.h"
#include "GL/internal/glcore.h"
#include "eglconfig.h"


extern void
_eglConfigToContextModesRec(const _EGLConfig *config, __GLcontextModes *mode);


extern EGLBoolean
_eglConfigFromContextModesRec(_EGLConfig *conf, const __GLcontextModes *m,
                              EGLint conformant, EGLint renderable_type);


extern EGLBoolean
_eglFillInConfigs( _EGLConfig *configs,
                   EGLenum fb_format, EGLenum fb_type,
                   const uint8_t * depth_bits, const uint8_t * stencil_bits,
                   unsigned num_depth_stencil_bits,
                   const EGLenum * db_modes, unsigned num_db_modes,
                   int visType );



#endif /* EGLCONFIGUTIL_INCLUDED */
