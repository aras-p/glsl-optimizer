/**
 * Extra utility functions related to EGL configs.
 */


#include <stdlib.h>
#include <string.h>
#include "eglconfigutil.h"


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
 * Convert a __GLcontextModes object to an _EGLConfig.
 */
EGLBoolean
_eglConfigFromContextModesRec(_EGLConfig *conf, const __GLcontextModes *m,
                              EGLint conformant, EGLint renderable_type)
{
   EGLint config_caveat, surface_type;

   /* must be RGBA */
   if (!m->rgbMode || !(m->renderType & GLX_RGBA_BIT))
      return EGL_FALSE;

   config_caveat = EGL_NONE;
   if (m->visualRating == GLX_SLOW_CONFIG)
      config_caveat = EGL_SLOW_CONFIG;

   if (m->visualRating == GLX_NON_CONFORMANT_CONFIG)
      conformant &= ~EGL_OPENGL_BIT;
   if (!(conformant & EGL_OPENGL_ES_BIT))
      config_caveat = EGL_NON_CONFORMANT_CONFIG;

   surface_type = 0;
   if (m->drawableType & GLX_WINDOW_BIT)
      surface_type |= EGL_WINDOW_BIT;
   if (m->drawableType & GLX_PIXMAP_BIT)
      surface_type |= EGL_PIXMAP_BIT;
   if (m->drawableType & GLX_PBUFFER_BIT)
      surface_type |= EGL_PBUFFER_BIT;

   SET_CONFIG_ATTRIB(conf, EGL_BUFFER_SIZE, m->rgbBits);
   SET_CONFIG_ATTRIB(conf, EGL_RED_SIZE, m->redBits);
   SET_CONFIG_ATTRIB(conf, EGL_GREEN_SIZE, m->greenBits);
   SET_CONFIG_ATTRIB(conf, EGL_BLUE_SIZE, m->blueBits);
   SET_CONFIG_ATTRIB(conf, EGL_ALPHA_SIZE, m->alphaBits);

   SET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGB, m->bindToTextureRgb);
   SET_CONFIG_ATTRIB(conf, EGL_BIND_TO_TEXTURE_RGBA, m->bindToTextureRgba);
   SET_CONFIG_ATTRIB(conf, EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER);
   SET_CONFIG_ATTRIB(conf, EGL_CONFIG_CAVEAT, config_caveat);

   SET_CONFIG_ATTRIB(conf, EGL_CONFORMANT, conformant);
   SET_CONFIG_ATTRIB(conf, EGL_DEPTH_SIZE, m->depthBits);
   SET_CONFIG_ATTRIB(conf, EGL_LEVEL, m->level);
   SET_CONFIG_ATTRIB(conf, EGL_MAX_PBUFFER_WIDTH, m->maxPbufferWidth);
   SET_CONFIG_ATTRIB(conf, EGL_MAX_PBUFFER_HEIGHT, m->maxPbufferHeight);
   SET_CONFIG_ATTRIB(conf, EGL_MAX_PBUFFER_PIXELS, m->maxPbufferPixels);

   SET_CONFIG_ATTRIB(conf, EGL_NATIVE_RENDERABLE, m->xRenderable);
   SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_ID, m->visualID);

   if (m->visualType != GLX_NONE)
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_TYPE, m->visualType);
   else
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_TYPE, EGL_NONE);

   SET_CONFIG_ATTRIB(conf, EGL_RENDERABLE_TYPE, renderable_type);
   SET_CONFIG_ATTRIB(conf, EGL_SAMPLE_BUFFERS, m->sampleBuffers);
   SET_CONFIG_ATTRIB(conf, EGL_SAMPLES, m->samples);
   SET_CONFIG_ATTRIB(conf, EGL_STENCIL_SIZE, m->stencilBits);

   SET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE, surface_type);

   /* what to do with GLX_TRANSPARENT_INDEX? */
   if (m->transparentPixel == GLX_TRANSPARENT_RGB) {
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_TYPE, EGL_TRANSPARENT_RGB);
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_RED_VALUE, m->transparentRed);
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_GREEN_VALUE, m->transparentGreen);
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_BLUE_VALUE, m->transparentBlue);
   }
   else {
      SET_CONFIG_ATTRIB(conf, EGL_TRANSPARENT_TYPE, EGL_NONE);
   }

   return EGL_TRUE;
}
