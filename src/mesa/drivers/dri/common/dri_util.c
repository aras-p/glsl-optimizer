/*
 * (C) Copyright IBM Corporation 2002, 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file dri_util.c
 * DRI utility functions.
 *
 * This module acts as glue between GLX and the actual hardware driver.  A DRI
 * driver doesn't really \e have to use any of this - it's optional.  But, some
 * useful stuff is done here that otherwise would have to be duplicated in most
 * drivers.
 * 
 * Basically, these utility functions take care of some of the dirty details of
 * screen initialization, context creation, context binding, DRM setup, etc.
 *
 * These functions are compiled into each DRI driver so libGL.so knows nothing
 * about them.
 */


#include <stdbool.h>
#ifndef __NOT_HAVE_DRM_H
#include <xf86drm.h>
#endif
#include "dri_util.h"
#include "utils.h"
#include "xmlpool.h"
#include "../glsl/glsl_parser_extras.h"
#include "main/mtypes.h"
#include "main/version.h"
#include "main/errors.h"
#include "main/macros.h"

PUBLIC const char __dri2ConfigOptions[] =
   DRI_CONF_BEGIN
      DRI_CONF_SECTION_PERFORMANCE
         DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_1)
      DRI_CONF_SECTION_END
   DRI_CONF_END;

/*****************************************************************/
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/

static void
setupLoaderExtensions(__DRIscreen *psp,
		      const __DRIextension **extensions)
{
    int i;

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_DRI2_LOADER) == 0)
	    psp->dri2.loader = (__DRIdri2LoaderExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_IMAGE_LOOKUP) == 0)
	    psp->dri2.image = (__DRIimageLookupExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_USE_INVALIDATE) == 0)
	    psp->dri2.useInvalidate = (__DRIuseInvalidateExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_SWRAST_LOADER) == 0)
	    psp->swrast_loader = (__DRIswrastLoaderExtension *) extensions[i];
        if (strcmp(extensions[i]->name, __DRI_IMAGE_LOADER) == 0)
           psp->image.loader = (__DRIimageLoaderExtension *) extensions[i];
    }
}

/**
 * This pointer determines which driver API we'll use in the case of the
 * loader not passing us an explicit driver extensions list (that would,
 * itself, contain a pointer to a driver API.)
 *
 * A driver's driDriverGetExtensions_drivername() can update this pointer to
 * what it's returning, and a loader that is ignorant of createNewScreen2()
 * will get the correct driver screen created, as long as no other
 * driDriverGetExtensions() happened in between the first one and the
 * createNewScreen().
 *
 * This allows the X Server to not require the significant dri_interface.h
 * updates for doing createNewScreen2(), which would discourage backporting of
 * the X Server patches to support the new loader interface.
 */
const struct __DriverAPIRec *globalDriverAPI = &driDriverAPI;

/**
 * This is the first entrypoint in the driver called by the DRI driver loader
 * after dlopen()ing it.
 *
 * It's used to create global state for the driver across contexts on the same
 * Display.
 */
static __DRIscreen *
driCreateNewScreen2(int scrn, int fd,
                    const __DRIextension **extensions,
                    const __DRIextension **driver_extensions,
                    const __DRIconfig ***driver_configs, void *data)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;

    psp = calloc(1, sizeof(*psp));
    if (!psp)
	return NULL;

    /* By default, use the global driDriverAPI symbol (non-megadrivers). */
    psp->driver = globalDriverAPI;

    /* If the driver exposes its vtable through its extensions list
     * (megadrivers), use that instead.
     */
    if (driver_extensions) {
       for (int i = 0; driver_extensions[i]; i++) {
          if (strcmp(driver_extensions[i]->name, __DRI_DRIVER_VTABLE) == 0) {
             psp->driver =
                ((__DRIDriverVtableExtension *)driver_extensions[i])->vtable;
          }
       }
    }

    setupLoaderExtensions(psp, extensions);

#ifndef __NOT_HAVE_DRM_H
    if (fd != -1) {
       drmVersionPtr version = drmGetVersion(fd);
       if (version) {
          psp->drm_version.major = version->version_major;
          psp->drm_version.minor = version->version_minor;
          psp->drm_version.patch = version->version_patchlevel;
          drmFreeVersion(version);
       }
    }
#endif

    psp->loaderPrivate = data;

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;

    *driver_configs = psp->driver->InitScreen(psp);
    if (*driver_configs == NULL) {
	free(psp);
	return NULL;
    }

    int gl_version_override = _mesa_get_gl_version_override();
    if (gl_version_override >= 31) {
       psp->max_gl_core_version = MAX2(psp->max_gl_core_version,
                                       gl_version_override);
    } else {
       psp->max_gl_compat_version = MAX2(psp->max_gl_compat_version,
                                         gl_version_override);
    }

    psp->api_mask = (1 << __DRI_API_OPENGL);
    if (psp->max_gl_core_version > 0)
       psp->api_mask |= (1 << __DRI_API_OPENGL_CORE);
    if (psp->max_gl_es1_version > 0)
       psp->api_mask |= (1 << __DRI_API_GLES);
    if (psp->max_gl_es2_version > 0)
       psp->api_mask |= (1 << __DRI_API_GLES2);
    if (psp->max_gl_es2_version >= 30)
       psp->api_mask |= (1 << __DRI_API_GLES3);

    driParseOptionInfo(&psp->optionInfo, __dri2ConfigOptions);
    driParseConfigFiles(&psp->optionCache, &psp->optionInfo, psp->myNum, "dri2");


    return psp;
}

static __DRIscreen *
dri2CreateNewScreen(int scrn, int fd,
		    const __DRIextension **extensions,
		    const __DRIconfig ***driver_configs, void *data)
{
   return driCreateNewScreen2(scrn, fd, extensions, NULL,
                               driver_configs, data);
}

/** swrast driver createNewScreen entrypoint. */
static __DRIscreen *
driSWRastCreateNewScreen(int scrn, const __DRIextension **extensions,
                         const __DRIconfig ***driver_configs, void *data)
{
   return driCreateNewScreen2(scrn, -1, extensions, NULL,
                               driver_configs, data);
}

static __DRIscreen *
driSWRastCreateNewScreen2(int scrn, const __DRIextension **extensions,
                          const __DRIextension **driver_extensions,
                          const __DRIconfig ***driver_configs, void *data)
{
   return driCreateNewScreen2(scrn, -1, extensions, driver_extensions,
                               driver_configs, data);
}

/**
 * Destroy the per-screen private information.
 * 
 * \internal
 * This function calls __DriverAPIRec::DestroyScreen on \p screenPrivate, calls
 * drmClose(), and finally frees \p screenPrivate.
 */
static void driDestroyScreen(__DRIscreen *psp)
{
    if (psp) {
	/* No interaction with the X-server is possible at this point.  This
	 * routine is called after XCloseDisplay, so there is no protocol
	 * stream open to the X-server anymore.
	 */

       _mesa_destroy_shader_compiler();

	psp->driver->DestroyScreen(psp);

	driDestroyOptionCache(&psp->optionCache);
	driDestroyOptionInfo(&psp->optionInfo);

	free(psp);
    }
}

static const __DRIextension **driGetExtensions(__DRIscreen *psp)
{
    return psp->extensions;
}

/*@}*/


static bool
validate_context_version(__DRIscreen *screen,
                         int mesa_api,
                         unsigned major_version,
                         unsigned minor_version,
                         unsigned *dri_ctx_error)
{
   unsigned req_version = 10 * major_version + minor_version;
   unsigned max_version = 0;

   switch (mesa_api) {
   case API_OPENGL_COMPAT:
      max_version = screen->max_gl_compat_version;
      break;
   case API_OPENGL_CORE:
      max_version = screen->max_gl_core_version;
      break;
   case API_OPENGLES:
      max_version = screen->max_gl_es1_version;
      break;
   case API_OPENGLES2:
      max_version = screen->max_gl_es2_version;
      break;
   default:
      max_version = 0;
      break;
   }

   if (max_version == 0) {
      *dri_ctx_error = __DRI_CTX_ERROR_BAD_API;
      return false;
   } else if (req_version > max_version) {
      *dri_ctx_error = __DRI_CTX_ERROR_BAD_VERSION;
      return false;
   }

   return true;
}

/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/

static __DRIcontext *
driCreateContextAttribs(__DRIscreen *screen, int api,
                        const __DRIconfig *config,
                        __DRIcontext *shared,
                        unsigned num_attribs,
                        const uint32_t *attribs,
                        unsigned *error,
                        void *data)
{
    __DRIcontext *context;
    const struct gl_config *modes = (config != NULL) ? &config->modes : NULL;
    void *shareCtx = (shared != NULL) ? shared->driverPrivate : NULL;
    gl_api mesa_api;
    unsigned major_version = 1;
    unsigned minor_version = 0;
    uint32_t flags = 0;
    bool notify_reset = false;

    assert((num_attribs == 0) || (attribs != NULL));

    if (!(screen->api_mask & (1 << api))) {
	*error = __DRI_CTX_ERROR_BAD_API;
	return NULL;
    }

    switch (api) {
    case __DRI_API_OPENGL:
	mesa_api = API_OPENGL_COMPAT;
	break;
    case __DRI_API_GLES:
	mesa_api = API_OPENGLES;
	break;
    case __DRI_API_GLES2:
    case __DRI_API_GLES3:
	mesa_api = API_OPENGLES2;
	break;
    case __DRI_API_OPENGL_CORE:
        mesa_api = API_OPENGL_CORE;
        break;
    default:
	*error = __DRI_CTX_ERROR_BAD_API;
	return NULL;
    }

    for (unsigned i = 0; i < num_attribs; i++) {
	switch (attribs[i * 2]) {
	case __DRI_CTX_ATTRIB_MAJOR_VERSION:
	    major_version = attribs[i * 2 + 1];
	    break;
	case __DRI_CTX_ATTRIB_MINOR_VERSION:
	    minor_version = attribs[i * 2 + 1];
	    break;
	case __DRI_CTX_ATTRIB_FLAGS:
	    flags = attribs[i * 2 + 1];
	    break;
        case __DRI_CTX_ATTRIB_RESET_STRATEGY:
            notify_reset = (attribs[i * 2 + 1]
                            != __DRI_CTX_RESET_NO_NOTIFICATION);
            break;
	default:
	    /* We can't create a context that satisfies the requirements of an
	     * attribute that we don't understand.  Return failure.
	     */
	    assert(!"Should not get here.");
	    *error = __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE;
	    return NULL;
	}
    }

    /* Mesa does not support the GL_ARB_compatibilty extension or the
     * compatibility profile.  This means that we treat a API_OPENGL_COMPAT 3.1 as
     * API_OPENGL_CORE and reject API_OPENGL_COMPAT 3.2+.
     */
    if (mesa_api == API_OPENGL_COMPAT && major_version == 3 && minor_version == 1)
       mesa_api = API_OPENGL_CORE;

    if (mesa_api == API_OPENGL_COMPAT
        && ((major_version > 3)
            || (major_version == 3 && minor_version >= 2))) {
       *error = __DRI_CTX_ERROR_BAD_API;
       return NULL;
    }

    /* The EGL_KHR_create_context spec says:
     *
     *     "Flags are only defined for OpenGL context creation, and specifying
     *     a flags value other than zero for other types of contexts,
     *     including OpenGL ES contexts, will generate an error."
     *
     * The GLX_EXT_create_context_es2_profile specification doesn't say
     * anything specific about this case.  However, none of the known flags
     * have any meaning in an ES context, so this seems safe.
     */
    if (mesa_api != API_OPENGL_COMPAT
        && mesa_api != API_OPENGL_CORE
        && flags != 0) {
	*error = __DRI_CTX_ERROR_BAD_FLAG;
	return NULL;
    }

    /* There are no forward-compatible contexts before OpenGL 3.0.  The
     * GLX_ARB_create_context spec says:
     *
     *     "Forward-compatible contexts are defined only for OpenGL versions
     *     3.0 and later."
     *
     * Forward-looking contexts are supported by silently converting the
     * requested API to API_OPENGL_CORE.
     *
     * In Mesa, a debug context is the same as a regular context.
     */
    if ((flags & __DRI_CTX_FLAG_FORWARD_COMPATIBLE) != 0) {
       mesa_api = API_OPENGL_CORE;
    }

    const uint32_t allowed_flags = (__DRI_CTX_FLAG_DEBUG
                                    | __DRI_CTX_FLAG_FORWARD_COMPATIBLE
                                    | __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS);
    if (flags & ~allowed_flags) {
	*error = __DRI_CTX_ERROR_UNKNOWN_FLAG;
	return NULL;
    }

    if (!validate_context_version(screen, mesa_api,
                                  major_version, minor_version, error))
       return NULL;

    context = calloc(1, sizeof *context);
    if (!context) {
	*error = __DRI_CTX_ERROR_NO_MEMORY;
	return NULL;
    }

    context->loaderPrivate = data;

    context->driScreenPriv = screen;
    context->driDrawablePriv = NULL;
    context->driReadablePriv = NULL;

    if (!screen->driver->CreateContext(mesa_api, modes, context,
                                       major_version, minor_version,
                                       flags, notify_reset, error, shareCtx)) {
        free(context);
        return NULL;
    }

    *error = __DRI_CTX_ERROR_SUCCESS;
    return context;
}

void
driContextSetFlags(struct gl_context *ctx, uint32_t flags)
{
    if ((flags & __DRI_CTX_FLAG_FORWARD_COMPATIBLE) != 0)
        ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT;
    if ((flags & __DRI_CTX_FLAG_DEBUG) != 0) {
       _mesa_set_debug_state_int(ctx, GL_DEBUG_OUTPUT, GL_TRUE);
        ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_DEBUG_BIT;
    }
}

static __DRIcontext *
driCreateNewContextForAPI(__DRIscreen *screen, int api,
                          const __DRIconfig *config,
                          __DRIcontext *shared, void *data)
{
    unsigned error;

    return driCreateContextAttribs(screen, api, config, shared, 0, NULL,
                                   &error, data);
}

static __DRIcontext *
driCreateNewContext(__DRIscreen *screen, const __DRIconfig *config,
                    __DRIcontext *shared, void *data)
{
    return driCreateNewContextForAPI(screen, __DRI_API_OPENGL,
                                     config, shared, data);
}

/**
 * Destroy the per-context private information.
 * 
 * \internal
 * This function calls __DriverAPIRec::DestroyContext on \p contextPrivate, calls
 * drmDestroyContext(), and finally frees \p contextPrivate.
 */
static void
driDestroyContext(__DRIcontext *pcp)
{
    if (pcp) {
	pcp->driScreenPriv->driver->DestroyContext(pcp);
	free(pcp);
    }
}

static int
driCopyContext(__DRIcontext *dest, __DRIcontext *src, unsigned long mask)
{
    (void) dest;
    (void) src;
    (void) mask;
    return GL_FALSE;
}

/*@}*/


/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/

static void dri_get_drawable(__DRIdrawable *pdp);
static void dri_put_drawable(__DRIdrawable *pdp);

/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static int driBindContext(__DRIcontext *pcp,
			  __DRIdrawable *pdp,
			  __DRIdrawable *prp)
{
    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (!pcp)
	return GL_FALSE;

    /* Bind the drawable to the context */
    pcp->driDrawablePriv = pdp;
    pcp->driReadablePriv = prp;
    if (pdp) {
	pdp->driContextPriv = pcp;
	dri_get_drawable(pdp);
    }
    if (prp && pdp != prp) {
	dri_get_drawable(prp);
    }

    return pcp->driScreenPriv->driver->MakeCurrent(pcp, pdp, prp);
}

/**
 * Unbind context.
 * 
 * \param scrn the screen.
 * \param gc context.
 *
 * \return \c GL_TRUE on success, or \c GL_FALSE on failure.
 * 
 * \internal
 * This function calls __DriverAPIRec::UnbindContext, and then decrements
 * __DRIdrawableRec::refcount which must be non-zero for a successful
 * return.
 * 
 * While casting the opaque private pointers associated with the parameters
 * into their respective real types it also assures they are not \c NULL. 
 */
static int driUnbindContext(__DRIcontext *pcp)
{
    __DRIdrawable *pdp;
    __DRIdrawable *prp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (pcp == NULL)
	return GL_FALSE;

    pdp = pcp->driDrawablePriv;
    prp = pcp->driReadablePriv;

    /* already unbound */
    if (!pdp && !prp)
	return GL_TRUE;

    pcp->driScreenPriv->driver->UnbindContext(pcp);

    assert(pdp);
    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    dri_put_drawable(pdp);

    if (prp != pdp) {
	if (prp->refcount == 0) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	dri_put_drawable(prp);
    }

    pcp->driDrawablePriv = NULL;
    pcp->driReadablePriv = NULL;

    return GL_TRUE;
}

/*@}*/


static void dri_get_drawable(__DRIdrawable *pdp)
{
    pdp->refcount++;
}

static void dri_put_drawable(__DRIdrawable *pdp)
{
    if (pdp) {
	pdp->refcount--;
	if (pdp->refcount)
	    return;

	pdp->driScreenPriv->driver->DestroyBuffer(pdp);
	free(pdp);
    }
}

static __DRIdrawable *
driCreateNewDrawable(__DRIscreen *screen,
                     const __DRIconfig *config,
                     void *data)
{
    __DRIdrawable *pdraw;

    pdraw = malloc(sizeof *pdraw);
    if (!pdraw)
	return NULL;

    pdraw->loaderPrivate = data;

    pdraw->driScreenPriv = screen;
    pdraw->driContextPriv = NULL;
    pdraw->refcount = 0;
    pdraw->lastStamp = 0;
    pdraw->w = 0;
    pdraw->h = 0;

    dri_get_drawable(pdraw);

    if (!screen->driver->CreateBuffer(screen, pdraw, &config->modes,
                                      GL_FALSE)) {
       free(pdraw);
       return NULL;
    }

    pdraw->dri2.stamp = pdraw->lastStamp + 1;

    return pdraw;
}

static void
driDestroyDrawable(__DRIdrawable *pdp)
{
    dri_put_drawable(pdp);
}

static __DRIbuffer *
dri2AllocateBuffer(__DRIscreen *screen,
		   unsigned int attachment, unsigned int format,
		   int width, int height)
{
    return screen->driver->AllocateBuffer(screen, attachment, format,
                                          width, height);
}

static void
dri2ReleaseBuffer(__DRIscreen *screen, __DRIbuffer *buffer)
{
    screen->driver->ReleaseBuffer(screen, buffer);
}


static int
dri2ConfigQueryb(__DRIscreen *screen, const char *var, GLboolean *val)
{
   if (!driCheckOption(&screen->optionCache, var, DRI_BOOL))
      return -1;

   *val = driQueryOptionb(&screen->optionCache, var);

   return 0;
}

static int
dri2ConfigQueryi(__DRIscreen *screen, const char *var, GLint *val)
{
   if (!driCheckOption(&screen->optionCache, var, DRI_INT) &&
       !driCheckOption(&screen->optionCache, var, DRI_ENUM))
      return -1;

    *val = driQueryOptioni(&screen->optionCache, var);

    return 0;
}

static int
dri2ConfigQueryf(__DRIscreen *screen, const char *var, GLfloat *val)
{
   if (!driCheckOption(&screen->optionCache, var, DRI_FLOAT))
      return -1;

    *val = driQueryOptionf(&screen->optionCache, var);

    return 0;
}

static unsigned int
driGetAPIMask(__DRIscreen *screen)
{
    return screen->api_mask;
}

/**
 * swrast swapbuffers entrypoint.
 *
 * DRI2 implements this inside the loader with only flushes handled by the
 * driver.
 */
static void
driSwapBuffers(__DRIdrawable *pdp)
{
    assert(pdp->driScreenPriv->swrast_loader);

    pdp->driScreenPriv->driver->SwapBuffers(pdp);
}

/** Core interface */
const __DRIcoreExtension driCoreExtension = {
    .base = { __DRI_CORE, __DRI_CORE_VERSION },

    .createNewScreen            = NULL,
    .destroyScreen              = driDestroyScreen,
    .getExtensions              = driGetExtensions,
    .getConfigAttrib            = driGetConfigAttrib,
    .indexConfigAttrib          = driIndexConfigAttrib,
    .createNewDrawable          = NULL,
    .destroyDrawable            = driDestroyDrawable,
    .swapBuffers                = driSwapBuffers, /* swrast */
    .createNewContext           = driCreateNewContext, /* swrast */
    .copyContext                = driCopyContext,
    .destroyContext             = driDestroyContext,
    .bindContext                = driBindContext,
    .unbindContext              = driUnbindContext
};

/** DRI2 interface */
const __DRIdri2Extension driDRI2Extension = {
    .base = { __DRI_DRI2, 4 },

    .createNewScreen            = dri2CreateNewScreen,
    .createNewDrawable          = driCreateNewDrawable,
    .createNewContext           = driCreateNewContext,
    .getAPIMask                 = driGetAPIMask,
    .createNewContextForAPI     = driCreateNewContextForAPI,
    .allocateBuffer             = dri2AllocateBuffer,
    .releaseBuffer              = dri2ReleaseBuffer,
    .createContextAttribs       = driCreateContextAttribs,
    .createNewScreen2           = driCreateNewScreen2,
};

const __DRIswrastExtension driSWRastExtension = {
    .base = { __DRI_SWRAST, 4 },

    .createNewScreen            = driSWRastCreateNewScreen,
    .createNewDrawable          = driCreateNewDrawable,
    .createNewContextForAPI     = driCreateNewContextForAPI,
    .createContextAttribs       = driCreateContextAttribs,
    .createNewScreen2           = driSWRastCreateNewScreen2,
};

const __DRI2configQueryExtension dri2ConfigQueryExtension = {
   .base = { __DRI2_CONFIG_QUERY, 1 },

   .configQueryb        = dri2ConfigQueryb,
   .configQueryi        = dri2ConfigQueryi,
   .configQueryf        = dri2ConfigQueryf,
};

void
dri2InvalidateDrawable(__DRIdrawable *drawable)
{
    drawable->dri2.stamp++;
}

/**
 * Check that the gl_framebuffer associated with dPriv is the right size.
 * Resize the gl_framebuffer if needed.
 * It's expected that the dPriv->driverPrivate member points to a
 * gl_framebuffer object.
 */
void
driUpdateFramebufferSize(struct gl_context *ctx, const __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) dPriv->driverPrivate;
   if (fb && (dPriv->w != fb->Width || dPriv->h != fb->Height)) {
      ctx->Driver.ResizeBuffers(ctx, fb, dPriv->w, dPriv->h);
      /* if the driver needs the hw lock for ResizeBuffers, the drawable
         might have changed again by now */
      assert(fb->Width == dPriv->w);
      assert(fb->Height == dPriv->h);
   }
}

uint32_t
driGLFormatToImageFormat(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_B5G6R5_UNORM:
      return __DRI_IMAGE_FORMAT_RGB565;
   case MESA_FORMAT_B8G8R8X8_UNORM:
      return __DRI_IMAGE_FORMAT_XRGB8888;
   case MESA_FORMAT_B10G10R10A2_UNORM:
      return __DRI_IMAGE_FORMAT_ARGB2101010;
   case MESA_FORMAT_B10G10R10X2_UNORM:
      return __DRI_IMAGE_FORMAT_XRGB2101010;
   case MESA_FORMAT_B8G8R8A8_UNORM:
      return __DRI_IMAGE_FORMAT_ARGB8888;
   case MESA_FORMAT_R8G8B8A8_UNORM:
      return __DRI_IMAGE_FORMAT_ABGR8888;
   case MESA_FORMAT_R8G8B8X8_UNORM:
      return __DRI_IMAGE_FORMAT_XBGR8888;
   case MESA_FORMAT_R_UNORM8:
      return __DRI_IMAGE_FORMAT_R8;
   case MESA_FORMAT_R8G8_UNORM:
      return __DRI_IMAGE_FORMAT_GR88;
   case MESA_FORMAT_NONE:
      return __DRI_IMAGE_FORMAT_NONE;
   case MESA_FORMAT_B8G8R8A8_SRGB:
      return __DRI_IMAGE_FORMAT_SARGB8;
   default:
      return 0;
   }
}

mesa_format
driImageFormatToGLFormat(uint32_t image_format)
{
   switch (image_format) {
   case __DRI_IMAGE_FORMAT_RGB565:
      return MESA_FORMAT_B5G6R5_UNORM;
   case __DRI_IMAGE_FORMAT_XRGB8888:
      return MESA_FORMAT_B8G8R8X8_UNORM;
   case __DRI_IMAGE_FORMAT_ARGB2101010:
      return MESA_FORMAT_B10G10R10A2_UNORM;
   case __DRI_IMAGE_FORMAT_XRGB2101010:
      return MESA_FORMAT_B10G10R10X2_UNORM;
   case __DRI_IMAGE_FORMAT_ARGB8888:
      return MESA_FORMAT_B8G8R8A8_UNORM;
   case __DRI_IMAGE_FORMAT_ABGR8888:
      return MESA_FORMAT_R8G8B8A8_UNORM;
   case __DRI_IMAGE_FORMAT_XBGR8888:
      return MESA_FORMAT_R8G8B8X8_UNORM;
   case __DRI_IMAGE_FORMAT_R8:
      return MESA_FORMAT_R_UNORM8;
   case __DRI_IMAGE_FORMAT_GR88:
      return MESA_FORMAT_R8G8_UNORM;
   case __DRI_IMAGE_FORMAT_SARGB8:
      return MESA_FORMAT_B8G8R8A8_SRGB;
   case __DRI_IMAGE_FORMAT_NONE:
      return MESA_FORMAT_NONE;
   default:
      return MESA_FORMAT_NONE;
   }
}

/** Image driver interface */
const __DRIimageDriverExtension driImageDriverExtension = {
    .base = { __DRI_IMAGE_DRIVER, 1 },

    .createNewScreen2           = driCreateNewScreen2,
    .createNewDrawable          = driCreateNewDrawable,
    .getAPIMask                 = driGetAPIMask,
    .createContextAttribs       = driCreateContextAttribs,
};

/* swrast copy sub buffer entrypoint. */
static void driCopySubBuffer(__DRIdrawable *pdp, int x, int y,
                             int w, int h)
{
    assert(pdp->driScreenPriv->swrast_loader);

    pdp->driScreenPriv->driver->CopySubBuffer(pdp, x, y, w, h);
}

/* for swrast only */
const __DRIcopySubBufferExtension driCopySubBufferExtension = {
   .base = { __DRI_COPY_SUB_BUFFER, 1 },

   .copySubBuffer               = driCopySubBuffer,
};
