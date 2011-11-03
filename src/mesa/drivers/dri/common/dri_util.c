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


#include <xf86drm.h>
#include "dri_util.h"
#include "utils.h"
#include "xmlpool.h"
#include "../glsl/glsl_parser_extras.h"

PUBLIC const char __dri2ConfigOptions[] =
   DRI_CONF_BEGIN
      DRI_CONF_SECTION_PERFORMANCE
         DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_1)
      DRI_CONF_SECTION_END
   DRI_CONF_END;

static const uint __dri2NConfigOptions = 1;

static void dri_get_drawable(__DRIdrawable *pdp);
static void dri_put_drawable(__DRIdrawable *pdp);

/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/

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
    /* Let driver unbind drawable from context */
    driDriverAPI.UnbindContext(pcp);

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


    /* XXX this is disabled so that if we call SwapBuffers on an unbound
     * window we can determine the last context bound to the window and
     * use that context's lock. (BrianP, 2-Dec-2000)
     */
    pcp->driDrawablePriv = pcp->driReadablePriv = NULL;

    return GL_TRUE;
}

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

    /* Call device-specific MakeCurrent */
    return driDriverAPI.MakeCurrent(pcp, pdp, prp);
}

static __DRIdrawable *
dri2CreateNewDrawable(__DRIscreen *screen,
		      const __DRIconfig *config,
		      void *loaderPrivate)
{
    __DRIdrawable *pdraw;

    pdraw = malloc(sizeof *pdraw);
    if (!pdraw)
	return NULL;

    pdraw->driContextPriv = NULL;
    pdraw->loaderPrivate = loaderPrivate;
    pdraw->refcount = 1;
    pdraw->lastStamp = 0;
    pdraw->w = 0;
    pdraw->h = 0;
    pdraw->driScreenPriv = screen;

    if (!driDriverAPI.CreateBuffer(screen, pdraw, &config->modes, 0)) {
       free(pdraw);
       return NULL;
    }

    pdraw->dri2.stamp = pdraw->lastStamp + 1;

    return pdraw;
}

static __DRIbuffer *
dri2AllocateBuffer(__DRIscreen *screen,
		   unsigned int attachment, unsigned int format,
		   int width, int height)
{
    return driDriverAPI.AllocateBuffer(screen, attachment, format,
				       width, height);
}

static void
dri2ReleaseBuffer(__DRIscreen *screen, __DRIbuffer *buffer)
{
   driDriverAPI.ReleaseBuffer(screen, buffer);
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


static void dri_get_drawable(__DRIdrawable *pdp)
{
    pdp->refcount++;
}
	
static void dri_put_drawable(__DRIdrawable *pdp)
{
    __DRIscreen *psp;

    if (pdp) {
	pdp->refcount--;
	if (pdp->refcount)
	    return;

	psp = pdp->driScreenPriv;
        driDriverAPI.DestroyBuffer(pdp);
	free(pdp);
    }
}

static void
driDestroyDrawable(__DRIdrawable *pdp)
{
    dri_put_drawable(pdp);
}

/*@}*/


/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/

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
	driDriverAPI.DestroyContext(pcp);
	free(pcp);
    }
}

static unsigned int
dri2GetAPIMask(__DRIscreen *screen)
{
    return screen->api_mask;
}

static __DRIcontext *
dri2CreateNewContextForAPI(__DRIscreen *screen, int api,
			   const __DRIconfig *config,
			   __DRIcontext *shared, void *data)
{
    __DRIcontext *context;
    const struct gl_config *modes = (config != NULL) ? &config->modes : NULL;
    void *shareCtx = (shared != NULL) ? shared->driverPrivate : NULL;
    gl_api mesa_api;

    if (!(screen->api_mask & (1 << api)))
	return NULL;

    switch (api) {
    case __DRI_API_OPENGL:
	    mesa_api = API_OPENGL;
	    break;
    case __DRI_API_GLES:
	    mesa_api = API_OPENGLES;
	    break;
    case __DRI_API_GLES2:
	    mesa_api = API_OPENGLES2;
	    break;
    default:
	    return NULL;
    }

    context = malloc(sizeof *context);
    if (!context)
	return NULL;

    context->driScreenPriv = screen;
    context->driDrawablePriv = NULL;
    context->loaderPrivate = data;
    
    if (!driDriverAPI.CreateContext(mesa_api, modes,
				    context, shareCtx) ) {
        free(context);
        return NULL;
    }

    return context;
}


static __DRIcontext *
dri2CreateNewContext(__DRIscreen *screen, const __DRIconfig *config,
		      __DRIcontext *shared, void *data)
{
   return dri2CreateNewContextForAPI(screen, __DRI_API_OPENGL,
				     config, shared, data);
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
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/

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

	if (driDriverAPI.DestroyScreen)
	    driDriverAPI.DestroyScreen(psp);

	driDestroyOptionCache(&psp->optionCache);
	driDestroyOptionInfo(&psp->optionInfo);

	free(psp);
    }
}

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
    }
}

/**
 * DRI2
 */
static __DRIscreen *
dri2CreateNewScreen(int scrn, int fd,
		    const __DRIextension **extensions,
		    const __DRIconfig ***driver_configs, void *data)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;
    drmVersionPtr version;

    psp = calloc(1, sizeof(*psp));
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    version = drmGetVersion(fd);
    if (version) {
	psp->drm_version.major = version->version_major;
	psp->drm_version.minor = version->version_minor;
	psp->drm_version.patch = version->version_patchlevel;
	drmFreeVersion(version);
    }

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;

    psp->api_mask = (1 << __DRI_API_OPENGL);
    *driver_configs = driDriverAPI.InitScreen(psp);
    if (*driver_configs == NULL) {
	free(psp);
	return NULL;
    }

    psp->loaderPrivate = data;

    driParseOptionInfo(&psp->optionInfo, __dri2ConfigOptions,
		       __dri2NConfigOptions);
    driParseConfigFiles(&psp->optionCache, &psp->optionInfo, psp->myNum,
			"dri2");

    return psp;
}

static const __DRIextension **driGetExtensions(__DRIscreen *psp)
{
    return psp->extensions;
}

/** Core interface */
const __DRIcoreExtension driCoreExtension = {
    { __DRI_CORE, __DRI_CORE_VERSION },
    NULL,
    driDestroyScreen,
    driGetExtensions,
    driGetConfigAttrib,
    driIndexConfigAttrib,
    NULL,
    driDestroyDrawable,
    NULL,
    NULL,
    driCopyContext,
    driDestroyContext,
    driBindContext,
    driUnbindContext
};

/** DRI2 interface */
const __DRIdri2Extension driDRI2Extension = {
    { __DRI_DRI2, __DRI_DRI2_VERSION },
    dri2CreateNewScreen,
    dri2CreateNewDrawable,
    dri2CreateNewContext,
    dri2GetAPIMask,
    dri2CreateNewContextForAPI,
    dri2AllocateBuffer,
    dri2ReleaseBuffer
};

const __DRI2configQueryExtension dri2ConfigQueryExtension = {
   { __DRI2_CONFIG_QUERY, __DRI2_CONFIG_QUERY_VERSION },
   dri2ConfigQueryb,
   dri2ConfigQueryi,
   dri2ConfigQueryf,
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
