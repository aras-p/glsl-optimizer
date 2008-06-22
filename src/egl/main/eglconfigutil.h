
#ifndef EGLCONFIGUTIL_INCLUDED
#define EGLCONFIGUTIL_INCLUDED

#include "eglconfig.h"
#include "GL/internal/glcore.h"


extern void
_eglConfigToContextModesRec(const _EGLConfig *config, __GLcontextModes *mode);


extern EGLBoolean
_eglFillInConfigs( _EGLConfig *configs,
                   EGLenum fb_format, EGLenum fb_type,
                   const u_int8_t * depth_bits, const u_int8_t * stencil_bits,
                   unsigned num_depth_stencil_bits,
                   const EGLenum * db_modes, unsigned num_db_modes,
                   int visType );



#endif /* EGLCONFIGUTIL_INCLUDED */
