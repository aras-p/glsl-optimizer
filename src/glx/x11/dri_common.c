/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "glxclient.h"
#include "glcontextmodes.h"
#include "dri_common.h"

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

_X_HIDDEN void InfoMessageF(const char *f, ...)
{
    va_list args;
    const char *env;

    if ((env = getenv("LIBGL_DEBUG")) && strstr(env, "verbose")) {
	fprintf(stderr, "libGL: ");
	va_start(args, f);
	vfprintf(stderr, f, args);
	va_end(args);
    }
}

/**
 * Print error to stderr, unless LIBGL_DEBUG=="quiet".
 */
_X_HIDDEN void ErrorMessageF(const char *f, ...)
{
    va_list args;
    const char *env;

    if ((env = getenv("LIBGL_DEBUG")) && !strstr(env, "quiet")) {
	fprintf(stderr, "libGL error: ");
	va_start(args, f);
	vfprintf(stderr, f, args);
	va_end(args);
    }
}

#ifndef DEFAULT_DRIVER_DIR
/* this is normally defined in Mesa/configs/default with DRI_DRIVER_SEARCH_PATH */
#define DEFAULT_DRIVER_DIR "/usr/local/lib/dri"
#endif

/**
 * Try to \c dlopen the named driver.
 *
 * This function adds the "_dri.so" suffix to the driver name and searches the
 * directories specified by the \c LIBGL_DRIVERS_PATH environment variable in
 * order to find the driver.
 *
 * \param driverName - a name like "tdfx", "i810", "mga", etc.
 *
 * \returns
 * A handle from \c dlopen, or \c NULL if driver file not found.
 */
_X_HIDDEN void *driOpenDriver(const char *driverName)
{
   void *glhandle, *handle;
   const char *libPaths, *p, *next;
   char realDriverName[200];
   int len;

   /* Attempt to make sure libGL symbols will be visible to the driver */
   glhandle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);

   libPaths = NULL;
   if (geteuid() == getuid()) {
      /* don't allow setuid apps to use LIBGL_DRIVERS_PATH */
      libPaths = getenv("LIBGL_DRIVERS_PATH");
      if (!libPaths)
         libPaths = getenv("LIBGL_DRIVERS_DIR"); /* deprecated */
   }
   if (libPaths == NULL)
       libPaths = DEFAULT_DRIVER_DIR;

   handle = NULL;
   for (p = libPaths; *p; p = next) {
       next = strchr(p, ':');
       if (next == NULL) {
	   len = strlen(p);
	   next = p + len;
       } else {
	   len = next - p;
	   next++;
       }

#ifdef GLX_USE_TLS
      snprintf(realDriverName, sizeof realDriverName,
	       "%.*s/tls/%s_dri.so", len, p, driverName);
      InfoMessageF("OpenDriver: trying %s\n", realDriverName);
      handle = dlopen(realDriverName, RTLD_NOW | RTLD_GLOBAL);
#endif

      if ( handle == NULL ) {
	 snprintf(realDriverName, sizeof realDriverName,
		  "%.*s/%s_dri.so", len, p, driverName);
	 InfoMessageF("OpenDriver: trying %s\n", realDriverName);
	 handle = dlopen(realDriverName, RTLD_NOW | RTLD_GLOBAL);
      }

      if ( handle != NULL )
	  break;
      else
	 ErrorMessageF("dlopen %s failed (%s)\n", realDriverName, dlerror());
   }

   if (!handle)
      ErrorMessageF("unable to load driver: %s_dri.so\n", driverName);

   if (glhandle)
      dlclose(glhandle);

   return handle;
}

_X_HIDDEN const __DRIsystemTimeExtension systemTimeExtension = {
    { __DRI_SYSTEM_TIME, __DRI_SYSTEM_TIME_VERSION },
    __glXGetUST,
    __driGetMscRateOML
};

#define __ATTRIB(attrib, field) \
    { attrib, offsetof(__GLcontextModes, field) }

static const struct { unsigned int attrib, offset; } attribMap[] = {
    __ATTRIB(__DRI_ATTRIB_BUFFER_SIZE,			rgbBits),
    __ATTRIB(__DRI_ATTRIB_LEVEL,			level),
    __ATTRIB(__DRI_ATTRIB_RED_SIZE,			redBits),
    __ATTRIB(__DRI_ATTRIB_GREEN_SIZE,			greenBits),
    __ATTRIB(__DRI_ATTRIB_BLUE_SIZE,			blueBits),
    __ATTRIB(__DRI_ATTRIB_ALPHA_SIZE,			alphaBits),
    __ATTRIB(__DRI_ATTRIB_DEPTH_SIZE,			depthBits),
    __ATTRIB(__DRI_ATTRIB_STENCIL_SIZE,			stencilBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_RED_SIZE,		accumRedBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_GREEN_SIZE,		accumGreenBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_BLUE_SIZE,		accumBlueBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_ALPHA_SIZE,		accumAlphaBits),
    __ATTRIB(__DRI_ATTRIB_SAMPLE_BUFFERS,		sampleBuffers),
    __ATTRIB(__DRI_ATTRIB_SAMPLES,			samples),
    __ATTRIB(__DRI_ATTRIB_DOUBLE_BUFFER,		doubleBufferMode),
    __ATTRIB(__DRI_ATTRIB_STEREO,			stereoMode),
    __ATTRIB(__DRI_ATTRIB_AUX_BUFFERS,			numAuxBuffers),
#if 0
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_TYPE,		transparentPixel),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_INDEX_VALUE,	transparentIndex),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_RED_VALUE,	transparentRed),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_GREEN_VALUE,	transparentGreen),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_BLUE_VALUE,	transparentBlue),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_ALPHA_VALUE,	transparentAlpha),
    __ATTRIB(__DRI_ATTRIB_RED_MASK,			redMask),
    __ATTRIB(__DRI_ATTRIB_GREEN_MASK,			greenMask),
    __ATTRIB(__DRI_ATTRIB_BLUE_MASK,			blueMask),
    __ATTRIB(__DRI_ATTRIB_ALPHA_MASK,			alphaMask),
#endif
    __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_WIDTH,		maxPbufferWidth),
    __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_HEIGHT,		maxPbufferHeight),
    __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_PIXELS,		maxPbufferPixels),
    __ATTRIB(__DRI_ATTRIB_OPTIMAL_PBUFFER_WIDTH,	optimalPbufferWidth),
    __ATTRIB(__DRI_ATTRIB_OPTIMAL_PBUFFER_HEIGHT,	optimalPbufferHeight),
#if 0
    __ATTRIB(__DRI_ATTRIB_SWAP_METHOD,			swapMethod),
#endif
    __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_RGB,		bindToTextureRgb),
    __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_RGBA,		bindToTextureRgba),
    __ATTRIB(__DRI_ATTRIB_BIND_TO_MIPMAP_TEXTURE,	bindToMipmapTexture),
    __ATTRIB(__DRI_ATTRIB_YINVERTED,			yInverted),
};

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

static int
scalarEqual(__GLcontextModes *mode, unsigned int attrib, unsigned int value)
{
    unsigned int glxValue;
    int i;

    for (i = 0; i < ARRAY_SIZE(attribMap); i++)
	if (attribMap[i].attrib == attrib) {
	    glxValue = *(unsigned int *) ((char *) mode + attribMap[i].offset);
	    return glxValue == GLX_DONT_CARE || glxValue == value;
	}

    return GL_TRUE; /* Is a non-existing attribute equal to value? */
}

static int
driConfigEqual(const __DRIcoreExtension *core,
	       __GLcontextModes *modes, const __DRIconfig *driConfig)
{
    unsigned int attrib, value, glxValue;
    int i;

    i = 0;
    while (core->indexConfigAttrib(driConfig, i++, &attrib, &value)) {
	switch (attrib) {
	case __DRI_ATTRIB_RENDER_TYPE:
	    glxValue = 0;
	    if (value & __DRI_ATTRIB_RGBA_BIT) {
		glxValue |= GLX_RGBA_BIT;
	    } else if (value & __DRI_ATTRIB_COLOR_INDEX_BIT) {
		glxValue |= GLX_COLOR_INDEX_BIT;
	    }
	    if (glxValue != modes->renderType)
		return GL_FALSE;
	    break;

	case __DRI_ATTRIB_CONFIG_CAVEAT:
	    if (value & __DRI_ATTRIB_NON_CONFORMANT_CONFIG)
		glxValue = GLX_NON_CONFORMANT_CONFIG;
	    else if (value & __DRI_ATTRIB_SLOW_BIT)
		glxValue = GLX_SLOW_CONFIG;
	    else
		glxValue = GLX_NONE;
	    if (glxValue != modes->visualRating)
		return GL_FALSE;
	    break;

	case __DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS:
	    glxValue = 0;
	    if (value & __DRI_ATTRIB_TEXTURE_1D_BIT)
		glxValue |= GLX_TEXTURE_1D_BIT_EXT;
	    if (value & __DRI_ATTRIB_TEXTURE_2D_BIT)
		glxValue |= GLX_TEXTURE_2D_BIT_EXT;
	    if (value & __DRI_ATTRIB_TEXTURE_RECTANGLE_BIT)
		glxValue |= GLX_TEXTURE_RECTANGLE_BIT_EXT;
	    if (modes->bindToTextureTargets != GLX_DONT_CARE &&
		glxValue != modes->bindToTextureTargets)
		return GL_FALSE;
	    break;	

	default:
	    if (!scalarEqual(modes, attrib, value))
		return GL_FALSE;
	}
    }

    return GL_TRUE;
}

static __GLcontextModes *
createDriMode(const __DRIcoreExtension *core,
	      __GLcontextModes *modes, const __DRIconfig **driConfigs)
{
    __GLXDRIconfigPrivate *config;
    int i;

    for (i = 0; driConfigs[i]; i++) {
	if (driConfigEqual(core, modes, driConfigs[i]))
	    break;
    }

    if (driConfigs[i] == NULL)
	return NULL;

    config = Xmalloc(sizeof *config);
    if (config == NULL)
	return NULL;

    config->modes = *modes;
    config->driConfig = driConfigs[i];

    return &config->modes;
}

_X_HIDDEN __GLcontextModes *
driConvertConfigs(const __DRIcoreExtension *core,
		  __GLcontextModes *modes, const __DRIconfig **configs)
{
    __GLcontextModes head, *tail, *m;

    tail = &head;
    head.next = NULL;
    for (m = modes; m; m = m->next) {
	tail->next = createDriMode(core, m, configs);
	if (tail->next == NULL) {
	    /* no matching dri config for m */
	    continue;
	}


	tail = tail->next;
    }

    _gl_context_modes_destroy(modes);

    return head.next;
}

_X_HIDDEN void
driBindExtensions(__GLXscreenConfigs *psc, int dri2)
{
    const __DRIextension **extensions;
    int i;

    extensions = psc->core->getExtensions(psc->__driScreen);

    for (i = 0; extensions[i]; i++) {
#ifdef __DRI_COPY_SUB_BUFFER
	if (strcmp(extensions[i]->name, __DRI_COPY_SUB_BUFFER) == 0) {
	    psc->driCopySubBuffer = (__DRIcopySubBufferExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_MESA_copy_sub_buffer");
	}
#endif

#ifdef __DRI_SWAP_CONTROL
	if (strcmp(extensions[i]->name, __DRI_SWAP_CONTROL) == 0) {
	    psc->swapControl = (__DRIswapControlExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_SGI_swap_control");
	    __glXEnableDirectExtension(psc, "GLX_MESA_swap_control");
	}
#endif

#ifdef __DRI_ALLOCATE
	if (strcmp(extensions[i]->name, __DRI_ALLOCATE) == 0) {
	    psc->allocate = (__DRIallocateExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_MESA_allocate_memory");
	}
#endif

#ifdef __DRI_FRAME_TRACKING
	if (strcmp(extensions[i]->name, __DRI_FRAME_TRACKING) == 0) {
	    psc->frameTracking = (__DRIframeTrackingExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_MESA_swap_frame_usage");
	}
#endif

#ifdef __DRI_MEDIA_STREAM_COUNTER
	if (strcmp(extensions[i]->name, __DRI_MEDIA_STREAM_COUNTER) == 0) {
	    psc->msc = (__DRImediaStreamCounterExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_SGI_video_sync");
	}
#endif

#ifdef __DRI_SWAP_BUFFER_COUNTER
	/* No driver supports this at this time and the extension is
	 * not defined in dri_interface.h.  Will enable
	 * GLX_OML_sync_control if implemented. */
#endif

#ifdef __DRI_READ_DRAWABLE
	if (strcmp(extensions[i]->name, __DRI_READ_DRAWABLE) == 0) {
	    __glXEnableDirectExtension(psc, "GLX_SGI_make_current_read");
	}
#endif

#ifdef __DRI_TEX_BUFFER
	if ((strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0) && dri2) {
	    psc->texBuffer = (__DRItexBufferExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_EXT_texture_from_pixmap");
	}
#endif

	/* Ignore unknown extensions */
    }
}

#endif /* GLX_DIRECT_RENDERING */
