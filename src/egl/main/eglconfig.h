#ifndef EGLCONFIG_INCLUDED
#define EGLCONFIG_INCLUDED


#include <assert.h>
#include "egltypedefs.h"


/* update _eglValidationTable and _eglOffsetOfConfig before updating this
 * struct */
struct _egl_config
{
   _EGLDisplay *Display;

   /* core */
   EGLint BufferSize;
   EGLint AlphaSize;
   EGLint BlueSize;
   EGLint GreenSize;
   EGLint RedSize;
   EGLint DepthSize;
   EGLint StencilSize;
   EGLint ConfigCaveat;
   EGLint ConfigID;
   EGLint Level;
   EGLint MaxPbufferHeight;
   EGLint MaxPbufferPixels;
   EGLint MaxPbufferWidth;
   EGLint NativeRenderable;
   EGLint NativeVisualID;
   EGLint NativeVisualType;
   EGLint Samples;
   EGLint SampleBuffers;
   EGLint SurfaceType;
   EGLint TransparentType;
   EGLint TransparentBlueValue;
   EGLint TransparentGreenValue;
   EGLint TransparentRedValue;
   EGLint BindToTextureRGB;
   EGLint BindToTextureRGBA;
   EGLint MinSwapInterval;
   EGLint MaxSwapInterval;
   EGLint LuminanceSize;
   EGLint AlphaMaskSize;
   EGLint ColorBufferType;
   EGLint RenderableType;
   EGLint MatchNativePixmap;
   EGLint Conformant;

   /* extensions */
   EGLint YInvertedNOK;
};


/**
 * Map an EGL attribute enum to the offset of the member in _EGLConfig.
 */
static INLINE EGLint
_eglOffsetOfConfig(EGLint attr)
{
   switch (attr) {
#define ATTRIB_MAP(attr, memb) case attr: return offsetof(_EGLConfig, memb)
   /* core */
   ATTRIB_MAP(EGL_BUFFER_SIZE,               BufferSize);
   ATTRIB_MAP(EGL_ALPHA_SIZE,                AlphaSize);
   ATTRIB_MAP(EGL_BLUE_SIZE,                 BlueSize);
   ATTRIB_MAP(EGL_GREEN_SIZE,                GreenSize);
   ATTRIB_MAP(EGL_RED_SIZE,                  RedSize);
   ATTRIB_MAP(EGL_DEPTH_SIZE,                DepthSize);
   ATTRIB_MAP(EGL_STENCIL_SIZE,              StencilSize);
   ATTRIB_MAP(EGL_CONFIG_CAVEAT,             ConfigCaveat);
   ATTRIB_MAP(EGL_CONFIG_ID,                 ConfigID);
   ATTRIB_MAP(EGL_LEVEL,                     Level);
   ATTRIB_MAP(EGL_MAX_PBUFFER_HEIGHT,        MaxPbufferHeight);
   ATTRIB_MAP(EGL_MAX_PBUFFER_PIXELS,        MaxPbufferPixels);
   ATTRIB_MAP(EGL_MAX_PBUFFER_WIDTH,         MaxPbufferWidth);
   ATTRIB_MAP(EGL_NATIVE_RENDERABLE,         NativeRenderable);
   ATTRIB_MAP(EGL_NATIVE_VISUAL_ID,          NativeVisualID);
   ATTRIB_MAP(EGL_NATIVE_VISUAL_TYPE,        NativeVisualType);
   ATTRIB_MAP(EGL_SAMPLES,                   Samples);
   ATTRIB_MAP(EGL_SAMPLE_BUFFERS,            SampleBuffers);
   ATTRIB_MAP(EGL_SURFACE_TYPE,              SurfaceType);
   ATTRIB_MAP(EGL_TRANSPARENT_TYPE,          TransparentType);
   ATTRIB_MAP(EGL_TRANSPARENT_BLUE_VALUE,    TransparentBlueValue);
   ATTRIB_MAP(EGL_TRANSPARENT_GREEN_VALUE,   TransparentGreenValue);
   ATTRIB_MAP(EGL_TRANSPARENT_RED_VALUE,     TransparentRedValue);
   ATTRIB_MAP(EGL_BIND_TO_TEXTURE_RGB,       BindToTextureRGB);
   ATTRIB_MAP(EGL_BIND_TO_TEXTURE_RGBA,      BindToTextureRGBA);
   ATTRIB_MAP(EGL_MIN_SWAP_INTERVAL,         MinSwapInterval);
   ATTRIB_MAP(EGL_MAX_SWAP_INTERVAL,         MaxSwapInterval);
   ATTRIB_MAP(EGL_LUMINANCE_SIZE,            LuminanceSize);
   ATTRIB_MAP(EGL_ALPHA_MASK_SIZE,           AlphaMaskSize);
   ATTRIB_MAP(EGL_COLOR_BUFFER_TYPE,         ColorBufferType);
   ATTRIB_MAP(EGL_RENDERABLE_TYPE,           RenderableType);
   ATTRIB_MAP(EGL_MATCH_NATIVE_PIXMAP,       MatchNativePixmap);
   ATTRIB_MAP(EGL_CONFORMANT,                Conformant);
   /* extensions */
   ATTRIB_MAP(EGL_Y_INVERTED_NOK,            YInvertedNOK);
#undef ATTRIB_MAP
   default:
      return -1;
   }
}


/**
 * Update a config for a given key.
 *
 * Note that a valid key is not necessarily a valid attribute.  There are gaps
 * in the attribute enums.  The separation is to catch application errors.
 * Drivers should never set a key that is an invalid attribute.
 */
static INLINE void
_eglSetConfigKey(_EGLConfig *conf, EGLint key, EGLint val)
{
   EGLint offset = _eglOffsetOfConfig(key);
   assert(offset >= 0);
   *((EGLint *) ((char *) conf + offset)) = val;
}


/**
 * Return the value for a given key.
 */
static INLINE EGLint
_eglGetConfigKey(const _EGLConfig *conf, EGLint key)
{
   EGLint offset = _eglOffsetOfConfig(key);
   assert(offset >= 0);
   return *((EGLint *) ((char *) conf + offset));
}


PUBLIC void
_eglInitConfig(_EGLConfig *config, _EGLDisplay *dpy, EGLint id);


PUBLIC EGLConfig
_eglLinkConfig(_EGLConfig *conf);


extern _EGLConfig *
_eglLookupConfig(EGLConfig config, _EGLDisplay *dpy);


/**
 * Return the handle of a linked config.
 */
static INLINE EGLConfig
_eglGetConfigHandle(_EGLConfig *conf)
{
   return (EGLConfig) conf;
}


PUBLIC EGLBoolean
_eglValidateConfig(const _EGLConfig *conf, EGLBoolean for_matching);


PUBLIC EGLBoolean
_eglMatchConfig(const _EGLConfig *conf, const _EGLConfig *criteria);


PUBLIC EGLBoolean
_eglParseConfigAttribList(_EGLConfig *conf, _EGLDisplay *dpy,
                          const EGLint *attrib_list);


PUBLIC EGLint
_eglCompareConfigs(const _EGLConfig *conf1, const _EGLConfig *conf2,
                   const _EGLConfig *criteria, EGLBoolean compare_id);


PUBLIC void
_eglSortConfigs(const _EGLConfig **configs, EGLint count,
                EGLint (*compare)(const _EGLConfig *, const _EGLConfig *,
                                  void *),
                void *priv_data);


extern EGLBoolean
_eglChooseConfig(_EGLDriver *drv, _EGLDisplay *dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);


extern EGLBoolean
_eglGetConfigAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglGetConfigs(_EGLDriver *drv, _EGLDisplay *dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);


#endif /* EGLCONFIG_INCLUDED */
