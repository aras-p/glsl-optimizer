
#include "egl_tracker.h"

#include "egllog.h"

void
drm_visual_modes_destroy(__GLcontextModes *modes)
{
   _eglLog(_EGL_DEBUG, "%s", __FUNCTION__);

   while (modes) {
      __GLcontextModes * const next = modes->next;
      free(modes);
      modes = next;
   }
}

__GLcontextModes *
drm_visual_modes_create(unsigned count, size_t minimum_size)
{
	/* This code copied from libGLX, and modified */
	const size_t size = (minimum_size > sizeof(__GLcontextModes))
		? minimum_size : sizeof(__GLcontextModes);
	__GLcontextModes * head = NULL;
	__GLcontextModes ** next;
	unsigned   i;

	_eglLog(_EGL_DEBUG, "%s %d %d", __FUNCTION__, count, minimum_size);

	next = & head;
	for (i = 0 ; i < count ; i++) {
		*next = (__GLcontextModes *) calloc(1, size);
		if (*next == NULL) {
			drm_visual_modes_destroy(head);
			head = NULL;
			break;
		}

		(*next)->doubleBufferMode = 1;
		(*next)->visualID = GLX_DONT_CARE;
		(*next)->visualType = GLX_DONT_CARE;
		(*next)->visualRating = GLX_NONE;
		(*next)->transparentPixel = GLX_NONE;
		(*next)->transparentRed = GLX_DONT_CARE;
		(*next)->transparentGreen = GLX_DONT_CARE;
		(*next)->transparentBlue = GLX_DONT_CARE;
		(*next)->transparentAlpha = GLX_DONT_CARE;
		(*next)->transparentIndex = GLX_DONT_CARE;
		(*next)->xRenderable = GLX_DONT_CARE;
		(*next)->fbconfigID = GLX_DONT_CARE;
		(*next)->swapMethod = GLX_SWAP_UNDEFINED_OML;
		(*next)->bindToTextureRgb = GLX_DONT_CARE;
		(*next)->bindToTextureRgba = GLX_DONT_CARE;
		(*next)->bindToMipmapTexture = GLX_DONT_CARE;
		(*next)->bindToTextureTargets = 0;
		(*next)->yInverted = GLX_DONT_CARE;

		next = & ((*next)->next);
	}

	return head;
}

__GLcontextModes *
drm_visual_from_config(_EGLConfig *conf)
{
	__GLcontextModes *visual;
	(void)conf;

	visual = drm_visual_modes_create(1, sizeof(*visual));
	visual->redBits = 8;
	visual->greenBits = 8;
	visual->blueBits = 8;
	visual->alphaBits = 8;

	visual->rgbBits = 32;
	visual->doubleBufferMode = 1;

	visual->depthBits = 24;
	visual->haveDepthBuffer = visual->depthBits > 0;
	visual->stencilBits = 8;
	visual->haveStencilBuffer = visual->stencilBits > 0;

	return visual;
}
